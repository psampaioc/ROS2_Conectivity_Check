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

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>
#include <rclcpp/rclcpp.hpp>

#include "conectivity_check/ping_checker.hpp"

namespace conectivity_check
{

bool load_ping_config(const std::string & config_file, PingConfig & config)
{
  try {
    YAML::Node root = YAML::LoadFile(config_file);
    YAML::Node ping = root["ping_checker"];
    if (!ping) {
      RCLCPP_ERROR(rclcpp::get_logger("ping_checker"), "No 'ping_checker' section in config");
      return false;
    }

    config.update_rate_hz = ping["update_rate_hz"].as<double>(1.0);
    config.ping_count = ping["ping_count"].as<int>(3);
    config.timeout_ms = ping["timeout_ms"].as<int>(1000);
    config.packet_size = ping["packet_size"].as<int>(56);
    config.log_throttle_sec = ping["log_throttle_sec"].as<double>(30.0);

    if (ping["targets"]) {
      for (const auto & t : ping["targets"]) {
        PingTarget target;
        target.label = t["label"].as<std::string>();
        target.host = t["host"].as<std::string>();
        target.enabled = t["enabled"].as<bool>(true);
        target.interface = t["interface"].as<std::string>("");
        config.targets.push_back(target);
      }
    }
    return true;
  } catch (const YAML::Exception & e) {
    RCLCPP_ERROR(rclcpp::get_logger("ping_checker"), "Failed to parse config: %s", e.what());
    return false;
  }
}

}  // namespace conectivity_check

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("ping_checker");

  // Get config file parameter
  std::string config_file = node->declare_parameter<std::string>("config_file", "");
  if (config_file.empty()) {
    RCLCPP_ERROR(node->get_logger(), "config_file parameter not set");
    return 1;
  }

  conectivity_check::PingConfig config;
  if (!conectivity_check::load_ping_config(config_file, config)) {
    return 1;
  }

  conectivity_check::PingChecker checker(node);
  if (!checker.init(config)) {
    RCLCPP_ERROR(node->get_logger(), "Failed to initialize ping checker");
    return 1;
  }

  RCLCPP_INFO(node->get_logger(), "Ping checker started with %zu targets", config.targets.size());

  rclcpp::spin(node);
  checker.shutdown();
  rclcpp::shutdown();
  return 0;
}
