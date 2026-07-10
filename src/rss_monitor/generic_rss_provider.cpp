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
#include "conectivity_check/generic_rss_provider.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <rclcpp/clock.hpp>
#include <rclcpp/logging.hpp>

namespace conectivity_check
{

GenericRssProviderSysfs::GenericRssProviderSysfs()
: logger_(rclcpp::get_logger("generic_sysfs_unset")) {}

EthernetRssProviderCarrier::EthernetRssProviderCarrier()
: logger_(rclcpp::get_logger("ethernet_carrier_unset")) {}

bool GenericRssProviderSysfs::init(const RssConfig & config, rclcpp::Logger logger)
{
  logger_ = logger;
  sysfs_path_ = config.sysfs_path;
  RCLCPP_INFO(logger_, "Generic sysfs provider initialized for %s", sysfs_path_.c_str());
  return std::filesystem::exists(sysfs_path_);
}

bool GenericRssProviderSysfs::read_once(msg::RssMeasurement & msg)
{
  msg.stamp = rclcpp::Clock().now();
  msg.interface = "radio0";
  msg.type = "generic";
  msg.label = "telemetry_radio";

  std::ifstream file(sysfs_path_);
  if (!file.is_open()) {return false;}

  std::string line;
  if (std::getline(file, line)) {
    try {
      msg.rss_dbm = std::stod(line);
      msg.driver_info = "sysfs";
      return true;
    } catch (...) {
      return false;
    }
  }
  return false;
}

bool EthernetRssProviderCarrier::init(const RssConfig & config, rclcpp::Logger logger)
{
  logger_ = logger;
  iface_ = config.interface_name;
  RCLCPP_INFO(logger_, "Ethernet carrier provider initialized for %s", iface_.c_str());
  return true;
}

bool EthernetRssProviderCarrier::read_once(msg::RssMeasurement & msg)
{
  msg.stamp = rclcpp::Clock().now();
  msg.interface = iface_;
  msg.type = "ethernet";
  msg.label = "wired";
  msg.driver_info = "carrier_only";

  std::string carrier_path = "/sys/class/net/" + iface_ + "/carrier";
  std::ifstream file(carrier_path);
  if (file.is_open()) {
    int carrier = 0;
    file >> carrier;
    msg.link_up = (carrier == 1);
  } else {
    msg.link_up = false;
  }

  // No real RSS for ethernet, but report link status
  msg.rss_dbm = msg.link_up ? 0.0 : -200.0;  // 0 dBm = link up, -200 = down
  return true;
}

}  // namespace conectivity_check
