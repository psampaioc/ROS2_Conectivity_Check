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

#include <memory>
#include <string>

#include <rclcpp/logger.hpp>

#include "conectivity_check/msg/rss_measurement.hpp"

namespace conectivity_check
{

struct RssConfig
{
  std::string interface_name;
  std::string type;            // "wifi", "cellular", "generic", "ethernet"
  std::string label;
  std::string method;  // "nl80211", "iw", "proc", "modemmanager", "at", "qmi", "sysfs", "carrier_only"
  // Extra params per method
  std::string modem_path;      // For ModemManager
  std::string at_device;       // For AT commands
  std::string sysfs_path;      // For sysfs
};

class RssProvider {
public:
  virtual ~RssProvider() = default;

    // Initialize provider (open sockets, connect DBus, etc.)
  virtual bool init(const RssConfig & config, rclcpp::Logger logger) = 0;

    // Perform a single RSS reading
    // Returns false on error (fills msg with NaN or leaves fields empty)
  virtual bool read_once(conectivity_check::msg::RssMeasurement & msg) = 0;

    // Cleanup
  virtual void shutdown() = 0;

    // Factory method
  static std::unique_ptr<RssProvider> create(const std::string & type, const std::string & method);
};

}  // namespace conectivity_check
