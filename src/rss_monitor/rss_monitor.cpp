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
#include <filesystem>
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
#include <rclcpp/rclcpp.hpp>
#include <sstream>
#include <yaml-cpp/yaml.h>

#include <ament_index_cpp/get_package_share_directory.hpp>

#include "conectivity_check/msg/rss_measurement.hpp"
#include "conectivity_check/msg/rss_summary.hpp"

namespace conectivity_check
{

struct StationInfo {
  int8_t signal_dbm = 0;
  int8_t noise_dbm = 0;
  double tx_bitrate_mbps = 0.0;
  int link_quality = 0;
  std::string ssid;
  std::string bssid;
  uint32_t freq_mhz = 0;
};

class RssMonitorNode : public rclcpp::Node
{
public:
  explicit RssMonitorNode(const rclcpp::NodeOptions & options)
  : Node("rss_monitor", options)
  {
    // Load configuration
    load_config();

    // Initialize netlink socket
    if (!init_netlink()) {
      RCLCPP_FATAL(get_logger(), "Failed to initialize netlink. Need CAP_NET_ADMIN?");
      rclcpp::shutdown();
      return;
    }

    // Summary publisher
    summary_pub_ = create_publisher<msg::RssSummary>("/connectivity/rss/summary", 10);

    // Timer
    timer_ = create_wall_timer(
      std::chrono::milliseconds(static_cast<int>(1000.0 / update_rate_hz_)),
      std::bind(&RssMonitorNode::timer_callback, this));

    RCLCPP_INFO(get_logger(), "RSS Monitor (KISS netlink) started for interface: %s", iface_.c_str());
  }

  ~RssMonitorNode()
  {
    if (sock_) {
      nl_socket_free(sock_);
      sock_ = nullptr;
    }
  }

private:
  void load_config()
  {
    // Get config file path
    std::string config_file = declare_parameter("config_file", std::string(""));
    if (config_file.empty()) {
      try {
        std::string pkg_share = ament_index_cpp::get_package_share_directory("conectivity_check");
        config_file = pkg_share + "/config/connectivity.yaml";
      } catch (const std::exception & e) {
        RCLCPP_WARN(get_logger(), "Failed to locate package share: %s", e.what());
      }
    }

    // Default values
    update_rate_hz_ = 1.0;
    iface_ = "wlp3s0";  // Default interface

    if (!std::filesystem::exists(config_file)) {
      RCLCPP_WARN(get_logger(), "Config file not found: %s, using defaults", config_file.c_str());
      return;
    }

    try {
      YAML::Node config = YAML::LoadFile(config_file);

      if (config["rss_monitor"]) {
        if (config["rss_monitor"]["update_rate_hz"]) {
          update_rate_hz_ = config["rss_monitor"]["update_rate_hz"].as<double>(1.0);
        }
        if (config["rss_monitor"]["interface"]) {
          std::string yaml_iface = config["rss_monitor"]["interface"].as<std::string>("");
          if (!yaml_iface.empty()) {
            iface_ = yaml_iface;
          }
        }
      }
    } catch (const std::exception & e) {
      RCLCPP_WARN(get_logger(), "Failed to parse config: %s", e.what());
    }

    RCLCPP_INFO(get_logger(), "Config loaded: interface=%s, rate=%.1f Hz", iface_.c_str(), update_rate_hz_);
  }

  bool init_netlink()
  {
    sock_ = nl_socket_alloc();
    if (!sock_) {
      RCLCPP_ERROR(get_logger(), "Failed to allocate netlink socket");
      return false;
    }

    // Disable sequence checking and auto-ack for simpler usage
    nl_socket_disable_seq_check(sock_);
    nl_socket_disable_auto_ack(sock_);

    if (nl_connect(sock_, NETLINK_GENERIC) < 0) {
      RCLCPP_ERROR(get_logger(), "Failed to connect netlink socket to NETLINK_GENERIC");
      nl_socket_free(sock_);
      sock_ = nullptr;
      return false;
    }

    // NOTE: genl_connect() NOT needed after nl_connect to NETLINK_GENERIC
    // genl_ctrl_resolve works directly on the NETLINK_GENERIC socket
    nl80211_id_ = genl_ctrl_resolve(sock_, "nl80211");
    if (nl80211_id_ < 0) {
      RCLCPP_ERROR(get_logger(), "nl80211 not found in kernel");
      nl_socket_free(sock_);
      sock_ = nullptr;
      return false;
    }

    ifindex_ = if_nametoindex(iface_.c_str());
    if (ifindex_ <= 0) {
      RCLCPP_ERROR(get_logger(), "Interface %s not found (ifindex=%d)", iface_.c_str(), ifindex_);
      nl_socket_free(sock_);
      sock_ = nullptr;
      return false;
    }

    RCLCPP_INFO(get_logger(), "Netlink initialized: nl80211_id=%d, iface=%s, ifindex=%d", nl80211_id_, iface_.c_str(), ifindex_);
    return true;
  }

  void timer_callback()
  {
    msg::RssSummary summary;
    summary.stamp = now();

    msg::RssMeasurement meas;
    if (read_rss_once(meas)) {
      summary.measurements.push_back(meas);
      summary.active_interfaces = meas.link_up ? 1 : 0;
      summary.best_rss_dbm = meas.rss_dbm;
      summary.worst_rss_dbm = meas.rss_dbm;
      summary.best_interface = meas.interface;
      summary.worst_interface = meas.interface;
      summary.any_wifi = (meas.type == "wifi");
      summary.any_cellular = false;
    } else {
      summary.active_interfaces = 0;
      summary.best_rss_dbm = -200.0;
      summary.worst_rss_dbm = -200.0;
    }

    summary_pub_->publish(summary);
  }

  bool read_rss_once(msg::RssMeasurement & msg)
  {
    msg.stamp = rclcpp::Clock().now();
    msg.interface = iface_;
    msg.type = "wifi";
    msg.label = "main_wifi";

    struct nl_msg * nlmsg = nlmsg_alloc();
    if (!nlmsg) {
      return false;
    }

    genlmsg_put(nlmsg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id_, 0,
                NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
    nla_put_u32(nlmsg, NL80211_ATTR_IFINDEX, ifindex_);

    StationInfo info{};
    nl_socket_modify_cb(sock_, NL_CB_VALID, NL_CB_CUSTOM, station_handler, &info);

    int ret = nl_send_auto(sock_, nlmsg);
    if (ret < 0) {
      nlmsg_free(nlmsg);
      return false;
    }

    nl_recvmsgs_default(sock_);
    nlmsg_free(nlmsg);

    if (info.signal_dbm == 0) {
      // No station info - interface might be down or not associated
      msg.link_up = false;
      msg.rss_dbm = -200.0;
      msg.driver_info = "no_station";
      return false;
    }

    // Success - fill measurement
    msg.rss_dbm = static_cast<double>(info.signal_dbm);
    msg.noise_dbm = static_cast<double>(info.noise_dbm);
    msg.snr_db = msg.rss_dbm - msg.noise_dbm;
    msg.bitrate_mbps = info.tx_bitrate_mbps;
    msg.link_quality = static_cast<double>(info.link_quality);
    msg.ssid = info.ssid;
    msg.bssid = info.bssid;
    msg.frequency_mhz = info.freq_mhz;
    msg.security = "WPA2";  // Simplified
    msg.link_up = true;
    msg.driver_info = "nl80211";

    return true;
  }

  static int station_handler(struct nl_msg * nlmsg, void * arg)
  {
    auto * info = static_cast<StationInfo *>(arg);
    parse_station_info(nlmsg, *info);
    return NL_SKIP;
  }

  static bool parse_station_info(struct nl_msg * nlmsg, StationInfo & info)
  {
    struct genlmsghdr * gnlh = static_cast<struct genlmsghdr *>(nlmsg_data(nlmsg_hdr(nlmsg)));
    struct nlattr * tb[NL80211_ATTR_MAX + 1];
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), nullptr);

    if (tb[NL80211_ATTR_STA_INFO]) {
      struct nlattr * sta_tb[NL80211_STA_INFO_MAX + 1];
      nla_parse_nested(sta_tb, NL80211_STA_INFO_MAX, tb[NL80211_ATTR_STA_INFO], nullptr);

      if (sta_tb[NL80211_STA_INFO_SIGNAL]) {
        info.signal_dbm = nla_get_s8(sta_tb[NL80211_STA_INFO_SIGNAL]);
      }
      if (sta_tb[NL80211_STA_INFO_TX_BITRATE]) {
        struct nlattr * rate_tb[NL80211_RATE_INFO_MAX + 1];
        nla_parse_nested(rate_tb, NL80211_RATE_INFO_MAX, sta_tb[NL80211_STA_INFO_TX_BITRATE], nullptr);
        if (rate_tb[NL80211_RATE_INFO_BITRATE]) {
          info.tx_bitrate_mbps = nla_get_u16(rate_tb[NL80211_RATE_INFO_BITRATE]) / 10.0;
        }
      }
    }

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

    // Estimate noise and link quality from signal
    if (info.signal_dbm > -50) {info.noise_dbm = -95; info.link_quality = 100;}
    else if (info.signal_dbm > -60) {info.noise_dbm = -92; info.link_quality = 80;}
    else if (info.signal_dbm > -70) {info.noise_dbm = -90; info.link_quality = 60;}
    else if (info.signal_dbm > -80) {info.noise_dbm = -88; info.link_quality = 40;}
    else if (info.signal_dbm > -90) {info.noise_dbm = -85; info.link_quality = 20;}
    else {info.noise_dbm = -82; info.link_quality = 0;}

    return true;
  }

  // Config
  double update_rate_hz_ = 1.0;
  std::string iface_ = "wlp3s0";

  // Netlink
  struct nl_sock * sock_ = nullptr;
  int nl80211_id_ = -1;
  int ifindex_ = -1;

  // ROS
  rclcpp::Publisher<msg::RssSummary>::SharedPtr summary_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace conectivity_check

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(conectivity_check::RssMonitorNode)

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<conectivity_check::RssMonitorNode>(rclcpp::NodeOptions());
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}