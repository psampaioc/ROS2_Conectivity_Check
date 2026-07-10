// Copyright 2026 Pedro Sampaio
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstring>
#include <fstream>
#include <iomanip>
#include <linux/nl80211.h>
#include <net/if.h>
#include <netlink/attr.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>

#include <rclcpp/clock.hpp>
#include <rclcpp/logging.hpp>
#include <sstream>

#include "conectivity_check/wifi_rss_provider.hpp"

namespace conectivity_check
{

bool WifiRssProviderNl80211::init(const RssConfig & config, rclcpp::Logger logger)
{
  logger_ = logger;
  iface_ = config.interface_name;

  sock_ = nl_socket_alloc();
  if (!sock_) {
    RCLCPP_ERROR(logger_, "Failed to allocate netlink socket");
    return false;
  }

  if (nl_connect(sock_, NETLINK_GENERIC) < 0) {
    RCLCPP_ERROR(logger_, "Failed to connect netlink socket");
    nl_socket_free(sock_);
    sock_ = nullptr;
    return false;
  }

  if (genl_connect(sock_) < 0) {
    RCLCPP_ERROR(logger_, "Failed to connect generic netlink");
    nl_socket_free(sock_);
    sock_ = nullptr;
    return false;
  }

  nl80211_id_ = genl_ctrl_resolve(sock_, "nl80211");
  if (nl80211_id_ < 0) {
    RCLCPP_ERROR(logger_, "nl80211 not found");
    nl_socket_free(sock_);
    sock_ = nullptr;
    return false;
  }

  ifindex_ = if_nametoindex(iface_.c_str());
  if (ifindex_ <= 0) {
    RCLCPP_ERROR(logger_, "Interface %s not found", iface_.c_str());
    nl_socket_free(sock_);
    sock_ = nullptr;
    return false;
  }

  RCLCPP_INFO(logger_, "WiFi nl80211 provider initialized for %s (ifindex=%d)", iface_.c_str(),
      ifindex_);
  return true;
}

bool WifiRssProviderNl80211::read_once(msg::RssMeasurement & msg)
{
  msg.stamp = rclcpp::Clock().now();
  msg.interface = iface_;
  msg.type = "wifi";
  msg.label = "main_wifi";  // viria do config

  struct nl_msg * nlmsg = nlmsg_alloc();
  if (!nlmsg) {return false;}

  genlmsg_put(nlmsg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id_, 0,
              NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
  nla_put_u32(nlmsg, NL80211_ATTR_IFINDEX, ifindex_);

  StationInfo info;
  nl_socket_modify_cb(sock_, NL_CB_VALID, NL_CB_CUSTOM, station_handler, &info);

  int ret = nl_send_auto(sock_, nlmsg);
  if (ret < 0) {
    nlmsg_free(nlmsg);
    return false;
  }

  nl_recvmsgs_default(sock_);
  nlmsg_free(nlmsg);

  if (info.signal_dbm == 0) {
    return false;
  }

  msg.rss_dbm = static_cast<double>(info.signal_dbm);
  // NL80211_STA_INFO doesn't provide noise; estimate from signal quality
  if (info.signal_dbm > -50) {info.noise_dbm = -95;} // excellent
  else if (info.signal_dbm > -70) {info.noise_dbm = -90;} // good
  else {info.noise_dbm = -85;} // fair/poor
  msg.noise_dbm = static_cast<double>(info.noise_dbm);
  msg.snr_db = static_cast<double>(info.signal_dbm - info.noise_dbm);
  msg.bitrate_mbps = info.tx_bitrate_mbps;
  msg.link_quality = static_cast<double>(info.link_quality);
  msg.ssid = info.ssid;
  msg.bssid = info.bssid;
  msg.frequency_mhz = info.freq_mhz;
  msg.security = "WPA2";  // Simplified

  return true;
}

void WifiRssProviderNl80211::shutdown()
{
  if (sock_) {
    nl_socket_free(sock_);
    sock_ = nullptr;
  }
}

bool WifiRssProviderNl80211::parse_station_info(struct nl_msg * nlmsg, StationInfo & info)
{
  struct genlmsghdr * gnlh = static_cast<struct genlmsghdr *>(nlmsg_data(nlmsg_hdr(nlmsg)));
  struct nlattr * tb[NL80211_ATTR_MAX + 1];
  nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), nullptr);

  // Parse nested NL80211_ATTR_STA_INFO for station-specific attributes
  if (tb[NL80211_ATTR_STA_INFO]) {
    struct nlattr * sta_tb[NL80211_STA_INFO_MAX + 1];
    nla_parse_nested(sta_tb, NL80211_STA_INFO_MAX, tb[NL80211_ATTR_STA_INFO], nullptr);

    if (sta_tb[NL80211_STA_INFO_SIGNAL]) {
      info.signal_dbm = nla_get_s8(sta_tb[NL80211_STA_INFO_SIGNAL]);
    }
    if (sta_tb[NL80211_STA_INFO_TX_BITRATE]) {
      struct nlattr * rate_tb[NL80211_RATE_INFO_MAX + 1];
      nla_parse_nested(rate_tb, NL80211_RATE_INFO_MAX, sta_tb[NL80211_STA_INFO_TX_BITRATE],
          nullptr);
      if (rate_tb[NL80211_RATE_INFO_BITRATE]) {
        info.tx_bitrate_mbps = nla_get_u16(rate_tb[NL80211_RATE_INFO_BITRATE]) / 10.0;
      }
    }
  }

  // BSSID from main attributes
  if (tb[NL80211_ATTR_BSSID]) {
    uint8_t * mac = static_cast<uint8_t *>(nla_data(tb[NL80211_ATTR_BSSID]));
    std::stringstream ss;
    for (int i = 0; i < 6; ++i) {
      if (i > 0) {ss << ":";}
      ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
    }
    info.bssid = ss.str();
  }
  if (tb[NL80211_ATTR_SSID]) {
    info.ssid = std::string(static_cast<const char *>(nla_data(tb[NL80211_ATTR_SSID])),
                            nla_len(tb[NL80211_ATTR_SSID]));
  }
  if (tb[NL80211_ATTR_WIPHY_FREQ]) {
    info.freq_mhz = nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]);
  }

  // Link quality estimation from signal
  if (info.signal_dbm > -50) {info.link_quality = 100;} else if (info.signal_dbm > -60) {
    info.link_quality = 80;
  } else if (info.signal_dbm > -70) {info.link_quality = 60;} else if (info.signal_dbm > -80) {
    info.link_quality = 40;
  } else if (info.signal_dbm > -90) {info.link_quality = 20;} else {info.link_quality = 0;}

  return true;
}

int WifiRssProviderNl80211::station_handler(struct nl_msg * nlmsg, void * arg)
{
  auto * info = static_cast<StationInfo *>(arg);
  parse_station_info(nlmsg, *info);
  return NL_SKIP;
}

// Simpler iw subprocess provider
bool WifiRssProviderIw::init(const RssConfig & config, rclcpp::Logger logger)
{
  logger_ = logger;
  iface_ = config.interface_name;
  RCLCPP_INFO(logger_, "WiFi iw provider initialized for %s", iface_.c_str());
  return true;
}

bool WifiRssProviderIw::read_once(msg::RssMeasurement & msg)
{
  msg.stamp = rclcpp::Clock().now();
  msg.interface = iface_;
  msg.type = "wifi";
  msg.label = "main_wifi";

  FILE * fp = popen(("iw dev " + iface_ + " link 2>/dev/null").c_str(), "r");
  if (!fp) {return false;}

  char line[256];
  int signal_dbm = 0, noise_dbm = 0;
  double bitrate = 0;
  std::string ssid, bssid;

  while (fgets(line, sizeof(line), fp)) {
    std::string str(line);
    if (str.find("signal:") != std::string::npos) {
      signal_dbm = std::stoi(str.substr(str.find(":") + 1));
    } else if (str.find("rx bitrate:") != std::string::npos) {
      size_t pos = str.find(":");
      std::string val = str.substr(pos + 1);
      bitrate = std::stod(val) / 10.0;  // Mbps
    } else if (str.find("SSID:") != std::string::npos) {
      ssid = str.substr(str.find(":") + 2);
      ssid.erase(ssid.find_last_not_of(" \n\r\t") + 1);
    } else if (str.find("BSSID:") != std::string::npos) {
      bssid = str.substr(str.find(":") + 2);
      bssid.erase(bssid.find_last_not_of(" \n\r\t") + 1);
    }
  }
  pclose(fp);

  if (signal_dbm == 0) {return false;}

  msg.rss_dbm = signal_dbm;
  msg.noise_dbm = -95;  // Estimate
  msg.snr_db = signal_dbm + 95;
  msg.bitrate_mbps = bitrate;
  msg.ssid = ssid;
  msg.bssid = bssid;
  msg.link_quality = (signal_dbm > -50) ? 100 : (signal_dbm > -60) ? 80 :
    (signal_dbm > -70) ? 60 : (signal_dbm > -80) ? 40 : 20;

  return true;
}

// /proc/net/wireless provider (legacy)
bool WifiRssProviderProc::init(const RssConfig & config, rclcpp::Logger logger)
{
  logger_ = logger;
  iface_ = config.interface_name;
  RCLCPP_INFO(logger_, "WiFi proc provider initialized for %s", iface_.c_str());
  return true;
}

bool WifiRssProviderProc::read_once(msg::RssMeasurement & msg)
{
  msg.stamp = rclcpp::Clock().now();
  msg.interface = iface_;
  msg.type = "wifi";
  msg.label = "main_wifi";

  std::ifstream file("/proc/net/wireless");
  if (!file.is_open()) {return false;}

  std::string line;
  std::getline(file, line);  // Skip header
  std::getline(file, line);  // Skip header

  while (std::getline(file, line)) {
    if (line.find(iface_ + ":") != std::string::npos) {
      std::istringstream iss(line);
      std::string iface;
      int status, link, level, noise, nwid, crypt, frag, retry, misc;
      iss >> iface >> status >> link >> level >> noise >> nwid >> crypt >> frag >> retry >> misc;
      iface.erase(std::remove(iface.begin(), iface.end(), ':'), iface.end());

      msg.rss_dbm = level;  // Already in dBm
      msg.noise_dbm = noise;
      msg.snr_db = level - noise;
      msg.link_quality = link;
      return true;
    }
  }
  return false;
}

}  // namespace conectivity_check
