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

#include "conectivity_check/rss_provider.hpp"

namespace conectivity_check
{

class CellularRssProviderAt : public RssProvider {
public:
  CellularRssProviderAt();
  ~CellularRssProviderAt() override {shutdown();}

  bool init(const RssConfig & config, rclcpp::Logger logger) override;
  bool read_once(msg::RssMeasurement & msg) override;
  void shutdown() override;

private:
  std::string at_device_;
  int fd_ = -1;
  rclcpp::Logger logger_;
  bool send_at_command(const std::string & cmd, std::string & response);
};

class CellularRssProviderQmi : public RssProvider {
public:
  CellularRssProviderQmi();
  ~CellularRssProviderQmi() override {shutdown();}

  bool init(const RssConfig & config, rclcpp::Logger logger) override;
  bool read_once(msg::RssMeasurement & msg) override;
  void shutdown() override {}

private:
  std::string qmi_device_;
  rclcpp::Logger logger_;
};

}  // namespace conectivity_check
