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

#include <rclcpp/rclcpp.hpp>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <linux/nl80211.h>

namespace connectivity_check
{

class RssNode : public rclcpp::Node
{
public:
  explicit RssNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  // Config constants (compile-time, no params needed)
  static constexpr double UPDATE_RATE_HZ = 1.0;
  static constexpr const char * WIFI_INTERFACE = "wlp3s0";
  static constexpr const char * ROUTER_IP = "192.168.10.1";
  static constexpr int PING_TIMEOUT_MS = 1000;
  static constexpr int PING_PACKET_SIZE = 1472;  // MTU-sized payload (1500 - 20 IP - 8 ICMP)
  static constexpr int PING_COUNT_PER_CYCLE = 3;  // Multiple pings for packet loss %

  // Timer callback
  void timer_callback();

  // Netlink / RSS
  bool init_netlink();
  bool read_rss_once(double & rss_dbm);

  // Ping
  bool init_ping();
  bool ping_once(double & rtt_ms);

  static uint16_t checksum(void * data, size_t len);

  // Netlink state
  struct nl_sock * nl_sock_ = nullptr;
  int nl80211_id_ = -1;
  int ifindex_ = -1;

  // Ping state
  int ping_sock_ = -1;
  uint16_t ping_seq_ = 0;

  // ROS
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace connectivity_check
