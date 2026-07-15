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

#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cmath>
#include <chrono>
#include <random>
#include <unordered_map>

#include "conectivity_check/ping_checker.hpp"

namespace conectivity_check
{

struct PingChecker::Impl
{
  rclcpp::Node::SharedPtr node;
  PingConfig config;
  std::vector<rclcpp::Publisher<msg::PingResult>::SharedPtr> pubs;
  std::unordered_map<std::string, rclcpp::Publisher<msg::PingResult>::SharedPtr> pub_map;
  rclcpp::Publisher<msg::PingSummary>::SharedPtr summary_pub;
  rclcpp::TimerBase::SharedPtr timer;
  int sock = -1;
  uint16_t seq = 0;
  std::mt19937 rng{std::random_device{}()};
  std::chrono::steady_clock::time_point last_log;

  explicit Impl(rclcpp::Node::SharedPtr n)
  : node(n), last_log(std::chrono::steady_clock::now()) {}

  bool init_socket()
  {
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
      RCLCPP_ERROR(node->get_logger(), "Failed to create ICMP socket: %s (need CAP_NET_RAW)",
          strerror(errno));
      return false;
    }

    // Set receive timeout
    struct timeval tv = {static_cast<time_t>(config.timeout_ms / 1000),
      static_cast<suseconds_t>((config.timeout_ms % 1000) * 1000)};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Bind to interface if specified
    for (const auto & target : config.targets) {
      if (!target.interface.empty()) {
        struct ifreq ifr;
        strncpy(ifr.ifr_name, target.interface.c_str(), IFNAMSIZ - 1);
        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
          RCLCPP_WARN(node->get_logger(), "Failed to bind to interface %s: %s",
              target.interface.c_str(), strerror(errno));
        }
      }
    }

    return true;
  }

  uint16_t checksum(void * data, size_t len)
  {
    uint32_t sum = 0;
    uint16_t * ptr = static_cast<uint16_t *>(data);
    while (len > 1) {
      sum += *ptr++;
      len -= 2;
    }
    if (len) {sum += *reinterpret_cast<uint8_t *>(ptr);}
    while (sum >> 16) {sum = (sum & 0xFFFF) + (sum >> 16);}
    return ~sum;
  }

  bool ping_once(const PingTarget & target, msg::PingResult & result)
  {
    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    if (inet_pton(AF_INET, target.host.c_str(), &dest.sin_addr) <= 0) {
      // Try hostname resolution
      struct hostent * he = gethostbyname(target.host.c_str());
      if (!he) {
        result.error = "DNS resolution failed";
        return false;
      }
      dest.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
    }

    // Build ICMP packet
    char packet[64 + sizeof(struct icmphdr)];
    struct icmphdr * icmp = reinterpret_cast<struct icmphdr *>(packet);
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = getpid() & 0xFFFF;
    icmp->un.echo.sequence = seq++;
    icmp->checksum = 0;
    memset(packet + sizeof(struct icmphdr), 0xA5, config.packet_size);
    icmp->checksum = checksum(packet, sizeof(packet));

    auto t1 = std::chrono::high_resolution_clock::now();
    ssize_t sent = sendto(sock, packet, sizeof(packet), 0,
        reinterpret_cast<struct sockaddr *>(&dest), sizeof(dest));
    if (sent <= 0) {return false;}

    // Receive
    char recv_buf[1024];
    struct sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    ssize_t recv_len = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
        reinterpret_cast<struct sockaddr *>(&from), &from_len);
    auto t2 = std::chrono::high_resolution_clock::now();

    if (recv_len <= 0) {return false;}

    double rtt = std::chrono::duration<double, std::milli>(t2 - t1).count();

    result.stamp = node->now();
    result.target = target.host;
    result.label = target.label;
    result.rtt_ms = rtt;
    result.reachable = true;
    result.error = "";
    return true;
  }

  void run_cycle()
  {
    std::vector<msg::PingResult> results;
    results.reserve(config.targets.size());

    for (const auto & target : config.targets) {
      if (!target.enabled) {continue;}

      msg::PingResult result;
      result.target = target.host;
      result.label = target.label;
      result.reachable = false;
      result.rtt_ms = 0;
      result.jitter_ms = 0;
      result.packet_loss_pct = 0;

      int success_count = 0;
      std::vector<double> rtts;

      for (int i = 0; i < config.ping_count; ++i) {
        if (ping_once(target, result)) {
          success_count++;
          rtts.push_back(result.rtt_ms);
        }
        usleep(10000);  // 10ms between pings
      }

      if (success_count > 0) {
        result.reachable = true;
        double sum = 0;
        for (double r : rtts) {sum += r;}
        result.rtt_ms = sum / rtts.size();

        // Jitter (std deviation)
        if (rtts.size() > 1) {
          double mean = result.rtt_ms;
          double sq_sum = 0;
          for (double r : rtts) {sq_sum += (r - mean) * (r - mean);}
          result.jitter_ms = sqrt(sq_sum / rtts.size());
        }
      } else {
        result.error = "No response";
      }

      result.packet_loss_pct = 100.0 * (config.ping_count - success_count) / config.ping_count;

      // Publish individual result using pub_map
      auto pub_it = pub_map.find(target.label);
      if (pub_it != pub_map.end() && pub_it->second) {
        pub_it->second->publish(result);
      }
      results.push_back(result);
    }

    // Publish summary
    msg::PingSummary summary;
    summary.stamp = node->now();
    summary.results = results;
    summary.reachable_count = 0;
    summary.worst_rtt_ms = 0;

    for (const auto & r : results) {
      if (r.reachable) {
        summary.reachable_count++;
        if (r.rtt_ms > summary.worst_rtt_ms) {
          summary.worst_rtt_ms = r.rtt_ms;
          summary.worst_label = r.label;
        }
      }
    }
    summary.all_reachable = (summary.reachable_count == static_cast<int>(results.size()));

    summary_pub->publish(summary);

    // Throttled logging
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_log).count();
    if (elapsed > config.log_throttle_sec) {
      RCLCPP_INFO(node->get_logger(), "Ping: %d/%zu reachable, worst: %s (%.1f ms)",
                  summary.reachable_count, results.size(),
                  summary.worst_label.c_str(), summary.worst_rtt_ms);
      last_log = now;
    }
  }
};

PingChecker::PingChecker(rclcpp::Node::SharedPtr node)
: impl_(std::make_unique<Impl>(node)) {}
PingChecker::~PingChecker() {shutdown();}

bool PingChecker::init(const PingConfig & config)
{
  impl_->config = config;

  // Create publishers for each target
  for (const auto & target : config.targets) {
    if (target.enabled) {
      auto pub = impl_->node->create_publisher<msg::PingResult>("/connectivity/ping/" +
          target.label, 10);
      impl_->pubs.push_back(pub);
      impl_->pub_map[target.label] = pub;
    }
  }

  impl_->summary_pub = impl_->node->create_publisher<msg::PingSummary>("/connectivity/ping/summary",
      10);

  if (!impl_->init_socket()) {return false;}

  impl_->timer = impl_->node->create_wall_timer(
    std::chrono::milliseconds(static_cast<int>(1000.0 / config.update_rate_hz)),
    [this]() {impl_->run_cycle();});

  return true;
}

void PingChecker::spin_once()
{
  // Timer handles it
}

void PingChecker::shutdown()
{
  if (impl_->sock >= 0) {
    close(impl_->sock);
    impl_->sock = -1;
  }
  impl_->timer.reset();
}

}  // namespace conectivity_check

// PingCheckerNode - ROS 2 node that uses PingChecker
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_components/register_node_macro.hpp"

namespace conectivity_check
{

class PingCheckerNode : public rclcpp::Node
{
public:
  explicit PingCheckerNode(const rclcpp::NodeOptions & options)
  : Node("ping_checker", options)
  {
    // Declare parameters
    declare_parameter("update_rate_hz", 1.0);
    declare_parameter("ping_count", 3);
    declare_parameter("packet_size", 56);
    declare_parameter("timeout_ms", 1000);
    declare_parameter("log_throttle_sec", 30);
    declare_parameter("targets", std::vector<std::string>({"8.8.8.8"}));

    // Load config from parameters
    PingConfig config;
    config.update_rate_hz = get_parameter("update_rate_hz").as_double();
    config.ping_count = get_parameter("ping_count").as_int();
    config.packet_size = get_parameter("packet_size").as_int();
    config.timeout_ms = get_parameter("timeout_ms").as_int();
    config.log_throttle_sec = get_parameter("log_throttle_sec").as_int();

    auto target_list = get_parameter("targets").as_string_array();
    for (size_t i = 0; i < target_list.size(); ++i) {
      PingTarget target;
      target.host = target_list[i];
      target.label = "target_" + std::to_string(i);
      target.enabled = true;
      config.targets.push_back(target);
    }

    checker_ = std::make_unique<PingChecker>(shared_from_this());
    if (!checker_->init(config)) {
      RCLCPP_ERROR(get_logger(), "Failed to initialize ping checker");
    }
  }

private:
  std::unique_ptr<PingChecker> checker_;
};

}  // namespace conectivity_check

RCLCPP_COMPONENTS_REGISTER_NODE(conectivity_check::PingCheckerNode)
