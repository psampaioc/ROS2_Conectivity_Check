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

#include <rclcpp/rclcpp.hpp>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

#include "conectivity_check/msg/ping_summary.hpp"
#include "conectivity_check/msg/rss_summary.hpp"
#include "conectivity_check/msg/connectivity_summary.hpp"

namespace conectivity_check
{

class ConnectivityMonitorNode : public rclcpp::Node
{
public:
  explicit ConnectivityMonitorNode(const rclcpp::NodeOptions & options)
  : Node("connectivity_monitor", options)
  {
    ping_sub_ = create_subscription<msg::PingSummary>(
      "/connectivity/ping/summary", 10,
      std::bind(&ConnectivityMonitorNode::ping_callback, this, std::placeholders::_1));

    rss_sub_ = create_subscription<msg::RssSummary>(
      "/connectivity/rss/summary", 10,
      std::bind(&ConnectivityMonitorNode::rss_callback, this, std::placeholders::_1));

    summary_pub_ = create_publisher<msg::ConnectivitySummary>("/connectivity/summary", 10);

    timer_ = create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&ConnectivityMonitorNode::publish_summary, this));

    RCLCPP_INFO(get_logger(), "Connectivity Monitor (KISS aggregator with dB ruler) started");
  }

private:
  void ping_callback(const msg::PingSummary::SharedPtr msg)
  {
    last_ping_ = msg;
  }

  void rss_callback(const msg::RssSummary::SharedPtr msg)
  {
    last_rss_ = msg;
  }

  std::string db_ruler(double rss_dbm)
  {
    if (rss_dbm <= -200) return "[===|----|----|----] -200 DISCONNECTED";
    if (rss_dbm <= -100) return "[===|----|----|----] -100 CRITICAL";
    if (rss_dbm <= -85)  return "[=====|---|----|----] -85  WEAK";
    if (rss_dbm <= -70)  return "[=========|---|----] -70  FAIR";
    if (rss_dbm <= -50)  return "[=============|----] -50  GOOD";
    return "[==================] -30  EXCELLENT";
  }

  void publish_summary()
  {
    msg::ConnectivitySummary summary;
    summary.stamp = now();

    std::ostringstream ss;

    // PING: reachable/total
    int ping_reachable = last_ping_ ? last_ping_->reachable_count : 0;
    int ping_total = last_ping_ ? static_cast<int>(last_ping_->results.size()) : 0;
    ss << "P: " << ping_reachable << "/" << ping_total << " | ";

    // RSS: best RSS in dBm
    std::string rss_str = "N/A";
    double rss_dbm = -200.0;
    if (last_rss_ && last_rss_->active_interfaces > 0) {
      rss_dbm = last_rss_->best_rss_dbm;
      rss_str = std::to_string(static_cast<int>(rss_dbm)) + "dBm";
    }
    ss << "RSS: " << rss_str << " | ";

    // dB RULER (replaces HEALTHY/DEGRADED)
    ss << "SCALE: " << db_ruler(rss_dbm) << " | ";

    // TIME: unix timestamp + human readable hh:mm:ss
    auto now_sec = summary.stamp.sec;
    std::time_t time_t_now = static_cast<std::time_t>(now_sec);
    std::tm tm_now{};
    localtime_r(&time_t_now, &tm_now);
    std::ostringstream time_ss;
    time_ss << std::put_time(&tm_now, "%H:%M:%S");
    ss << "T: " << now_sec << " (" << time_ss.str() << ")";

    summary.summary = ss.str();
    summary.healthy = (ping_reachable == ping_total && ping_total > 0 && rss_dbm > -100);
    // healthy = all ping reachable AND rss not critical

    summary_pub_->publish(summary);
  }

  rclcpp::Subscription<msg::PingSummary>::SharedPtr ping_sub_;
  rclcpp::Subscription<msg::RssSummary>::SharedPtr rss_sub_;
  rclcpp::Publisher<msg::ConnectivitySummary>::SharedPtr summary_pub_;
  rclcpp::TimerBase::SharedPtr timer_;

  msg::PingSummary::SharedPtr last_ping_;
  msg::RssSummary::SharedPtr last_rss_;
};

}  // namespace conectivity_check

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<conectivity_check::ConnectivityMonitorNode>(rclcpp::NodeOptions());
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}