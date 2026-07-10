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

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "conectivity_check/msg/rss_measurement.hpp"
#include "conectivity_check/msg/rss_summary.hpp"
#include "conectivity_check/rss_provider.hpp"

namespace conectivity_check
{

class RssMonitorNode : public rclcpp::Node
{
public:
  explicit RssMonitorNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  void timer_callback();
  void load_config();
  void create_providers();

  struct InterfaceConfig
  {
    std::string name;
    std::string type;
    std::string label;
    bool enabled = true;
    std::string method;
    std::string modem_path;
    std::string at_device;
    std::string sysfs_path;
  };

  std::vector<InterfaceConfig> interfaces_;
  std::map<std::string, std::unique_ptr<RssProvider>> providers_;
  std::map<std::string, rclcpp::Publisher<msg::RssMeasurement>::SharedPtr> publishers_;
  rclcpp::Publisher<msg::RssSummary>::SharedPtr summary_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  double update_rate_hz_ = 1.0;
};

}  // namespace conectivity_check
