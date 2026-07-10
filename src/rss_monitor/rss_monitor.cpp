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

#include <filesystem>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <rclcpp/rclcpp.hpp>
#include <yaml-cpp/yaml.h>

#include "conectivity_check/rss_monitor.hpp"
#include "conectivity_check/rss_provider.hpp"
#include "conectivity_check/wifi_rss_provider.hpp"
#include "conectivity_check/cellular_rss_provider.hpp"
#include "conectivity_check/generic_rss_provider.hpp"

namespace conectivity_check
{

RssMonitorNode::RssMonitorNode(const rclcpp::NodeOptions & options)
: Node("rss_monitor", options)
{
  load_config();
  create_providers();

  // Summary publisher
  summary_pub_ = create_publisher<msg::RssSummary>("/connectivity/rss/summary", 10);

  // Timer
  timer_ = create_wall_timer(
    std::chrono::milliseconds(static_cast<int>(1000.0 / update_rate_hz_)),
    std::bind(&RssMonitorNode::timer_callback, this));

  RCLCPP_INFO(get_logger(), "RSS Monitor started with %zu providers", providers_.size());
}

void RssMonitorNode::load_config()
{
  std::string config_file = declare_parameter("config_file", std::string(""));
  if (config_file.empty()) {
    try {
      std::string pkg_share = ament_index_cpp::get_package_share_directory("conectivity_check");
      config_file = pkg_share + "/config/connectivity.yaml";
    } catch (const std::exception & e) {
      RCLCPP_WARN(get_logger(), "Failed to locate package share directory: %s", e.what());
      return;
    }
  }

  if (!std::filesystem::exists(config_file)) {
    RCLCPP_WARN(get_logger(), "Config file not found: %s", config_file.c_str());
    return;
  }

  YAML::Node config = YAML::LoadFile(config_file);

  if (config["global"]) {
    update_rate_hz_ = config["global"]["update_rate_hz"].as<double>(1.0);
  }

  if (config["rss_monitor"] && config["rss_monitor"]["interfaces"]) {
    for (const auto & iface : config["rss_monitor"]["interfaces"]) {
      InterfaceConfig ic;
      ic.name = iface["name"].as<std::string>();
      ic.type = iface["type"].as<std::string>();
      ic.label = iface["label"].as<std::string>();
      ic.enabled = iface["enabled"].as<bool>(true);
      ic.method = iface["method"].as<std::string>();
      ic.modem_path = iface["modem_path"].as<std::string>("");
      ic.at_device = iface["at_device"].as<std::string>("");
      ic.sysfs_path = iface["sysfs_path"].as<std::string>("");
      if (ic.enabled) {interfaces_.push_back(ic);}
    }
  }
}

void RssMonitorNode::create_providers()
{
  for (const auto & iface : interfaces_) {
    auto provider = RssProvider::create(iface.type, iface.method);
    if (!provider) {
      RCLCPP_WARN(get_logger(), "Unknown provider type=%s method=%s", iface.type.c_str(),
          iface.method.c_str());
      continue;
    }

    RssConfig config;
    config.interface_name = iface.name;
    config.type = iface.type;
    config.label = iface.label;
    config.method = iface.method;
    config.modem_path = iface.modem_path;
    config.at_device = iface.at_device;
    config.sysfs_path = iface.sysfs_path;

    if (provider->init(config, get_logger())) {
      providers_[iface.name] = std::move(provider);

      // Publisher per interface
      std::string topic = "/connectivity/rss/" + iface.name;
      publishers_[iface.name] = create_publisher<msg::RssMeasurement>(topic, 10);
      RCLCPP_INFO(get_logger(), "Provider ready for %s (%s) on topic %s", iface.name.c_str(),
          iface.type.c_str(), topic.c_str());
    } else {
      RCLCPP_ERROR(get_logger(), "Failed to initialize provider for %s", iface.name.c_str());
    }
  }
}

void RssMonitorNode::timer_callback()
{
  msg::RssSummary summary;
  summary.stamp = now();
  double best_rss = -200.0;
  double worst_rss = 0.0;
  int active = 0;
  summary.any_wifi = false;
  summary.any_cellular = false;

  for (auto & [iface, provider] : providers_) {
    msg::RssMeasurement meas;
    if (provider->read_once(meas)) {
      // Override label from config
      for (const auto & ic : interfaces_) {
        if (ic.name == iface) {meas.label = ic.label;}
      }

      if (publishers_.count(iface)) {
        publishers_[iface]->publish(meas);
      }
      summary.measurements.push_back(meas);

      if (meas.link_up || meas.rss_dbm > -200) {
        active++;
        if (meas.type == "wifi") {summary.any_wifi = true;}
        if (meas.type == "cellular") {summary.any_cellular = true;}

        if (meas.rss_dbm > best_rss) {
          best_rss = meas.rss_dbm;
          summary.best_interface = iface;
        }
        if (meas.rss_dbm < worst_rss || worst_rss == 0) {
          worst_rss = meas.rss_dbm;
          summary.worst_interface = iface;
        }
      }
    }
  }

  summary.active_interfaces = active;
  summary.best_rss_dbm = best_rss;
  summary.worst_rss_dbm = worst_rss;

  summary_pub_->publish(summary);
}

}  // namespace conectivity_check

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(conectivity_check::RssMonitorNode)
