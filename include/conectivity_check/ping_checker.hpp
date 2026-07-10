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
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "conectivity_check/msg/ping_result.hpp"
#include "conectivity_check/msg/ping_summary.hpp"

namespace conectivity_check
{

struct PingTarget
{
  std::string label;
  std::string host;
  bool enabled = true;
  std::string interface;  // Empty = default route
};

struct PingConfig
{
  double update_rate_hz = 1.0;
  int ping_count = 3;
  int timeout_ms = 1000;
  int packet_size = 56;
  std::vector<PingTarget> targets;
  double log_throttle_sec = 30.0;
};

class PingChecker {
public:
  explicit PingChecker(rclcpp::Node::SharedPtr node);
  ~PingChecker();

  bool init(const PingConfig & config);
  void spin_once();
  void shutdown();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace conectivity_check
