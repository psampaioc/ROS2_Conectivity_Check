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
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

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

    RCLCPP_INFO(get_logger(), "Connectivity Monitor (aggregator) started");
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

  void publish_summary()
  {
    msg::ConnectivitySummary summary;
    summary.stamp = now();

    if (last_ping_) {
      summary.ping_reachable_count = last_ping_->reachable_count;
      summary.ping_total_count = last_ping_->results.size();
      summary.ping_all_reachable = last_ping_->all_reachable;
      summary.worst_ping_rtt_ms = last_ping_->worst_rtt_ms;
      summary.worst_ping_label = last_ping_->worst_label;
    }

    if (last_rss_) {
      summary.best_rss_dbm = last_rss_->best_rss_dbm;
      summary.worst_rss_dbm = last_rss_->worst_rss_dbm;
      summary.best_interface = last_rss_->best_interface;
      summary.worst_interface = last_rss_->worst_interface;
      summary.active_interfaces = last_rss_->active_interfaces;
      summary.any_wifi = last_rss_->any_wifi;
      summary.any_cellular = last_rss_->any_cellular;
    }

    // Overall health assessment
    bool ping_ok = last_ping_ && last_ping_->all_reachable;
    bool rss_ok = last_rss_ && last_rss_->active_interfaces > 0;
    summary.connectivity_ok = ping_ok;
    summary.signal_ok = rss_ok;
    summary.overall_healthy = ping_ok && rss_ok;

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

RCLCPP_COMPONENTS_REGISTER_NODE(conectivity_check::ConnectivityMonitorNode)
