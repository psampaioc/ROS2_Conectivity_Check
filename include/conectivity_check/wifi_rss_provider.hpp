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

#pragma once

#include <linux/nl80211.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>

#include <string>

#include "conectivity_check/rss_provider.hpp"

namespace conectivity_check
{

class WifiRssProviderNl80211 : public RssProvider {
public:
  WifiRssProviderNl80211()
  : logger_(rclcpp::get_logger("wifi_nl80211_unset")) {}
  ~WifiRssProviderNl80211() override {shutdown();}

  bool init(const RssConfig & config, rclcpp::Logger logger) override;
  bool read_once(msg::RssMeasurement & msg) override;
  void shutdown() override;

private:
  struct nl_sock * sock_ = nullptr;
  int nl80211_id_ = -1;
  std::string iface_;
  int ifindex_ = -1;
  rclcpp::Logger logger_;

  struct StationInfo
  {
    int32_t signal_dbm = 0;
    int32_t noise_dbm = 0;
    double tx_bitrate_mbps = 0.0;
    uint32_t link_quality = 0;
    std::string ssid;
    std::string bssid;
    uint32_t freq_mhz = 0;
  };

  static bool parse_station_info(struct nl_msg * msg, StationInfo & info);
  static int station_handler(struct nl_msg * msg, void * arg);
};

class WifiRssProviderIw : public RssProvider {
public:
  WifiRssProviderIw()
  : logger_(rclcpp::get_logger("wifi_iw_unset")) {}
  ~WifiRssProviderIw() override {shutdown();}

  bool init(const RssConfig & config, rclcpp::Logger logger) override;
  bool read_once(msg::RssMeasurement & msg) override;
  void shutdown() override {}

private:
  std::string iface_;
  rclcpp::Logger logger_;
};

class WifiRssProviderProc : public RssProvider {
public:
  WifiRssProviderProc()
  : logger_(rclcpp::get_logger("wifi_proc_unset")) {}
  ~WifiRssProviderProc() override {shutdown();}

  bool init(const RssConfig & config, rclcpp::Logger logger) override;
  bool read_once(msg::RssMeasurement & msg) override;
  void shutdown() override {}

private:
  std::string iface_;
  rclcpp::Logger logger_;
};

}  // namespace conectivity_check
