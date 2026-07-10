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
#include "conectivity_check/rss_provider.hpp"
#include "conectivity_check/wifi_rss_provider.hpp"
#include "conectivity_check/cellular_rss_provider.hpp"
#include "conectivity_check/generic_rss_provider.hpp"
#include "rclcpp/rclcpp.hpp"

namespace conectivity_check
{

std::unique_ptr<RssProvider> RssProvider::create(
  const std::string & type,
  const std::string & method)
{
  if (type == "wifi") {
    if (method == "nl80211") {
      return std::make_unique<WifiRssProviderNl80211>();
    } else if (method == "iw") {
      return std::make_unique<WifiRssProviderIw>();
    } else if (method == "proc") {
      return std::make_unique<WifiRssProviderProc>();
    }
  } else if (type == "cellular") {
    if (method == "modemmanager") {
      RCLCPP_WARN(rclcpp::get_logger("rss_provider"), "ModemManager provider not implemented yet");
      return nullptr;
    } else if (method == "at") {
      return std::make_unique<CellularRssProviderAt>();
    } else if (method == "qmi") {
      return std::make_unique<CellularRssProviderQmi>();
    }
  } else if (type == "generic") {
    if (method == "sysfs") {
      return std::make_unique<GenericRssProviderSysfs>();
    }
  } else if (type == "ethernet") {
    if (method == "carrier_only") {
      return std::make_unique<EthernetRssProviderCarrier>();
    }
  }

  RCLCPP_WARN(rclcpp::get_logger("rss_provider"), "Unknown provider type=%s method=%s",
      type.c_str(), method.c_str());
  return nullptr;
}

}  // namespace conectivity_check
