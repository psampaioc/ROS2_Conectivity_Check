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

#include <array>
#include <cstdio>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "conectivity_check/msg/speedtest_result.hpp"

namespace conectivity_check
{

class SpeedtestServerNode : public rclcpp::Node
{
public:
  SpeedtestServerNode()
  : Node("speedtest_server")
  {
    declare_parameter("provider", "speedtest-cli");
    declare_parameter("timeout_sec", 120);
    declare_parameter("preferred_server_id", "");

    provider_ = get_parameter("provider").as_string();
    timeout_sec_ = get_parameter("timeout_sec").as_int();
    preferred_server_ = get_parameter("preferred_server_id").as_string();

    service_ = create_service<std_srvs::srv::Trigger>(
      "/connectivity/speedtest",
      std::bind(&SpeedtestServerNode::handle_speedtest, this, std::placeholders::_1,
        std::placeholders::_2));

    result_pub_ = create_publisher<msg::SpeedtestResult>("/connectivity/speedtest/result", 10);

    RCLCPP_INFO(get_logger(), "Speedtest server ready (provider: %s)", provider_.c_str());
  }

private:
  void handle_speedtest(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    (void)request;
    RCLCPP_INFO(get_logger(), "Speedtest requested");

    // Run in background to not block service thread
    std::thread([this]() {
        run_speedtest();
    }).detach();

    response->success = true;
    response->message = "Speedtest started, check /connectivity/speedtest/result topic";
  }

  void run_speedtest()
  {
    msg::SpeedtestResult result;
    result.stamp = this->now();
    result.success = false;

    std::string cmd;
    if (provider_ == "speedtest-cli") {
      cmd = "speedtest-cli --json --timeout " + std::to_string(timeout_sec_);
      if (!preferred_server_.empty()) {
        cmd += " --server " + preferred_server_;
      }
    } else if (provider_ == "iperf3") {
      cmd = "iperf3 -c iperf.he.net -J -t 10";  // Public iperf3 server
    } else {
      result.error_message = "Unknown provider: " + provider_;
      result_pub_->publish(result);
      return;
    }

    std::string output = run_command(cmd);
    if (output.empty()) {
      result.error_message = "Command failed or timed out";
      result_pub_->publish(result);
      return;
    }

    parse_output(output, result);
    result_pub_->publish(result);

    RCLCPP_INFO(get_logger(), "Speedtest completed: %.1f/%.1f Mbps",
                result.download_mbps, result.upload_mbps);
  }

  std::string run_command(const std::string & cmd)
  {
    std::string full_cmd = cmd + " 2>&1";
    FILE * pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) {return "";}

    std::string result;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result += buffer;
    }
    pclose(pipe);
    return result;
  }

  void parse_output(const std::string & output, msg::SpeedtestResult & result)
  {
    if (provider_ == "speedtest-cli") {
      // Parse JSON output
      // {"download": 12345678, "upload": 8765432, "ping": 25.4,
      //  "server": {"name": "Server1", "country": "BR", ...},
      //  "client": {"ip": "1.2.3.4", "isp": "ISP Name"}}
      size_t pos;

      // Download (bits/s -> Mbps)
      pos = output.find("\"download\":");
      if (pos != std::string::npos) {
        size_t end = output.find(",", pos);
        double bits = std::stod(output.substr(pos + 11, end - pos - 11));
        result.download_mbps = bits / 1'000'000.0;
      }

      // Upload
      pos = output.find("\"upload\":");
      if (pos != std::string::npos) {
        size_t end = output.find(",", pos);
        double bits = std::stod(output.substr(pos + 9, end - pos - 9));
        result.upload_mbps = bits / 1'000'000.0;
      }

      // Latency
      pos = output.find("\"ping\":");
      if (pos != std::string::npos) {
        size_t end = output.find(",", pos);
        result.latency_ms = std::stod(output.substr(pos + 7, end - pos - 7));
      }

      // Server name
      pos = output.find("\"name\":\"");
      if (pos != std::string::npos) {
        size_t end = output.find("\"", pos + 8);
        result.server_name = output.substr(pos + 8, end - pos - 8);
      }

      // Server country
      pos = output.find("\"country\":\"");
      if (pos != std::string::npos) {
        size_t end = output.find("\"", pos + 11);
        result.server_country = output.substr(pos + 11, end - pos - 11);
      }

      // Client IP
      pos = output.find("\"ip\":\"");
      if (pos != std::string::npos) {
        size_t end = output.find("\"", pos + 6);
        result.client_ip = output.substr(pos + 6, end - pos - 6);
      }

      // ISP
      pos = output.find("\"isp\":\"");
      if (pos != std::string::npos) {
        size_t end = output.find("\"", pos + 7);
        result.isp = output.substr(pos + 7, end - pos - 7);
      }

      result.success = (result.download_mbps > 0 || result.upload_mbps > 0);
      if (!result.success) {result.error_message = "Failed to parse speedtest output";}
    } else if (provider_ == "iperf3") {
      // Parse iperf3 JSON output
      // {"start":{...},"intervals":[...],"end":{"sum_sent":{"bits_per_second":...},"sum_received":{"bits_per_second":...}}}
      size_t pos = output.find("\"bits_per_second\":");
      if (pos != std::string::npos) {
        // Find last occurrence (upload)
        size_t last_pos = output.rfind("\"bits_per_second\":");
        if (last_pos != std::string::npos) {
          size_t end = output.find(",", last_pos);
          double bits = std::stod(output.substr(last_pos + 18, end - last_pos - 18));
          result.upload_mbps = bits / 1'000'000.0;
        }
        // First occurrence is download
        size_t end = output.find(",", pos);
        double bits = std::stod(output.substr(pos + 18, end - pos - 18));
        result.download_mbps = bits / 1'000'000.0;
      }
      result.success = (result.download_mbps > 0 || result.upload_mbps > 0);
    }
  }

  std::string provider_;
  int timeout_sec_;
  std::string preferred_server_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr service_;
  rclcpp::Publisher<msg::SpeedtestResult>::SharedPtr result_pub_;
};

}  // namespace conectivity_check

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<conectivity_check::SpeedtestServerNode>());
  rclcpp::shutdown();
  return 0;
}
