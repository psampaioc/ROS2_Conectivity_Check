// Copyright 2026 Pedro Sampaio
// Licensed under the Apache License, Version 2.0

#include "connectivity_check/rss_node.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <linux/nl80211.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netlink/genl/ctrl.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

namespace connectivity_check
{

struct StationInfo {
  int8_t signal_dbm = 0;
  std::string ssid;
  std::string bssid;
  uint32_t freq_mhz = 0;
};

RssNode::RssNode(const rclcpp::NodeOptions & options)
: Node("rss_node", options)
{
  RCLCPP_INFO(get_logger(), "Starting RSS + Ping Node (minimal)");
  RCLCPP_INFO(get_logger(), "Interface: %s, Router: %s, Rate: %.1f Hz",
              WIFI_INTERFACE, ROUTER_IP, UPDATE_RATE_HZ);

  // Initialize netlink for RSSI
  if (!init_netlink()) {
    RCLCPP_FATAL(get_logger(), "Failed to init netlink (need CAP_NET_ADMIN?)");
    rclcpp::shutdown();
    return;
  }

  // Initialize ping socket
  if (!init_ping()) {
    RCLCPP_FATAL(get_logger(), "Failed to init ping socket (need CAP_NET_RAW?)");
    rclcpp::shutdown();
    return;
  }

  // Timer at configured rate
  auto period = std::chrono::milliseconds(static_cast<int>(1000.0 / UPDATE_RATE_HZ));
  timer_ = create_wall_timer(period, std::bind(&RssNode::timer_callback, this));

  RCLCPP_INFO(get_logger(), "RSS + Ping node started");
}

bool RssNode::init_netlink()
{
  nl_sock_ = nl_socket_alloc();
  if (!nl_sock_) {
    RCLCPP_ERROR(get_logger(), "Failed to allocate netlink socket");
    return false;
  }

  nl_socket_disable_seq_check(nl_sock_);
  nl_socket_disable_auto_ack(nl_sock_);

  if (nl_connect(nl_sock_, NETLINK_GENERIC) < 0) {
    RCLCPP_ERROR(get_logger(), "Failed to connect netlink to NETLINK_GENERIC");
    nl_socket_free(nl_sock_);
    nl_sock_ = nullptr;
    return false;
  }

  nl80211_id_ = genl_ctrl_resolve(nl_sock_, "nl80211");
  if (nl80211_id_ < 0) {
    RCLCPP_ERROR(get_logger(), "nl80211 not found in kernel");
    nl_socket_free(nl_sock_);
    nl_sock_ = nullptr;
    return false;
  }

  ifindex_ = if_nametoindex(WIFI_INTERFACE);
  if (ifindex_ <= 0) {
    RCLCPP_ERROR(get_logger(), "Interface %s not found (ifindex=%d)", WIFI_INTERFACE, ifindex_);
    nl_socket_free(nl_sock_);
    nl_sock_ = nullptr;
    return false;
  }

  RCLCPP_INFO(get_logger(), "Netlink ready: nl80211_id=%d, iface=%s, ifindex=%d",
              nl80211_id_, WIFI_INTERFACE, ifindex_);
  return true;
}

bool RssNode::read_rss_once(double & rss_dbm)
{
  struct nl_msg * nlmsg = nlmsg_alloc();
  if (!nlmsg) {
    return false;
  }

  genlmsg_put(nlmsg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id_, 0,
              NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
  nla_put_u32(nlmsg, NL80211_ATTR_IFINDEX, ifindex_);

  StationInfo info{};
  nl_socket_modify_cb(nl_sock_, NL_CB_VALID, NL_CB_CUSTOM,
                      [](struct nl_msg * msg, void * arg) -> int {
                        auto * info = static_cast<StationInfo *>(arg);
                        struct genlmsghdr * gnlh = static_cast<struct genlmsghdr *>(nlmsg_data(nlmsg_hdr(msg)));
                        struct nlattr * tb[NL80211_ATTR_MAX + 1];
                        nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), nullptr);

                        if (tb[NL80211_ATTR_STA_INFO]) {
                          struct nlattr * sta_tb[NL80211_STA_INFO_MAX + 1];
                          nla_parse_nested(sta_tb, NL80211_STA_INFO_MAX, tb[NL80211_ATTR_STA_INFO], nullptr);
                          if (sta_tb[NL80211_STA_INFO_SIGNAL]) {
                            info->signal_dbm = nla_get_s8(sta_tb[NL80211_STA_INFO_SIGNAL]);
                          }
                        }
                        if (tb[NL80211_ATTR_BSSID]) {
                          uint8_t * mac = static_cast<uint8_t *>(nla_data(tb[NL80211_ATTR_BSSID]));
                          std::stringstream ss;
                          for (int i = 0; i < 6; ++i) {
                            if (i > 0) ss << ":";
                            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
                          }
                          info->bssid = ss.str();
                        }
                        if (tb[NL80211_ATTR_SSID]) {
                          info->ssid = std::string(static_cast<const char *>(nla_data(tb[NL80211_ATTR_SSID])),
                                                   nla_len(tb[NL80211_ATTR_SSID]));
                        }
                        if (tb[NL80211_ATTR_WIPHY_FREQ]) {
                          info->freq_mhz = nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]);
                        }
                        return NL_SKIP;
                      }, &info);

  int ret = nl_send_auto(nl_sock_, nlmsg);
  if (ret < 0) {
    nlmsg_free(nlmsg);
    return false;
  }

  nl_recvmsgs_default(nl_sock_);
  nlmsg_free(nlmsg);

  if (info.signal_dbm == 0) {
    return false;  // No station info (not associated)
  }

  rss_dbm = static_cast<double>(info.signal_dbm);
  return true;
}

bool RssNode::init_ping()
{
  ping_sock_ = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (ping_sock_ < 0) {
    RCLCPP_ERROR(get_logger(), "Failed to create ICMP socket: %s (need CAP_NET_RAW)", strerror(errno));
    return false;
  }

  // Set receive timeout
  struct timeval tv = {PING_TIMEOUT_MS / 1000, (PING_TIMEOUT_MS % 1000) * 1000};
  setsockopt(ping_sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  RCLCPP_INFO(get_logger(), "Ping socket ready (target: %s)", ROUTER_IP);
  return true;
}

uint16_t RssNode::checksum(void * data, size_t len)
{
  uint32_t sum = 0;
  uint16_t * ptr = static_cast<uint16_t *>(data);
  while (len > 1) {
    sum += *ptr++;
    len -= 2;
  }
  if (len) {
    sum += *reinterpret_cast<uint8_t *>(ptr);
  }
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return ~sum;
}

bool RssNode::ping_once(double & rtt_ms)
{
  struct sockaddr_in dest{};
  dest.sin_family = AF_INET;
  if (inet_pton(AF_INET, ROUTER_IP, &dest.sin_addr) <= 0) {
    return false;
  }

  // Build ICMP packet with dynamic buffer for MTU-sized payloads
  const size_t packet_size = sizeof(struct icmphdr) + PING_PACKET_SIZE;
  std::vector<char> packet(packet_size);
  struct icmphdr * icmp = reinterpret_cast<struct icmphdr *>(packet.data());
  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->un.echo.id = getpid() & 0xFFFF;
  icmp->un.echo.sequence = ping_seq_++;
  icmp->checksum = 0;
  memset(packet.data() + sizeof(struct icmphdr), 0xA5, PING_PACKET_SIZE);
  icmp->checksum = checksum(packet.data(), packet.size());

  auto t1 = std::chrono::high_resolution_clock::now();
  ssize_t sent = sendto(ping_sock_, packet.data(), packet.size(), 0,
                        reinterpret_cast<struct sockaddr *>(&dest), sizeof(dest));
  if (sent <= 0) {
    return false;
  }

  // Receive - buffer large enough for ICMP response + potential IP header
  char recv_buf[2048];
  struct sockaddr_in from{};
  socklen_t from_len = sizeof(from);
  ssize_t recv_len = recvfrom(ping_sock_, recv_buf, sizeof(recv_buf), 0,
                              reinterpret_cast<struct sockaddr *>(&from), &from_len);
  auto t2 = std::chrono::high_resolution_clock::now();

  if (recv_len <= 0) {
    return false;
  }

  // Parse IP header to find ICMP header offset
  struct iphdr * ip_hdr = reinterpret_cast<struct iphdr *>(recv_buf);
  size_t ip_hdr_len = ip_hdr->ihl * 4;
  if (recv_len < static_cast<ssize_t>(ip_hdr_len + sizeof(struct icmphdr))) {
    return false;  // Packet too small
  }

  // Validate source IP matches destination
  if (ip_hdr->saddr != dest.sin_addr.s_addr) {
    return false;
  }

  // Get ICMP header
  struct icmphdr * recv_icmp = reinterpret_cast<struct icmphdr *>(recv_buf + ip_hdr_len);

  // Validate ICMP type is Echo Reply
  if (recv_icmp->type != ICMP_ECHOREPLY) {
    return false;
  }

  // Validate ICMP ID matches our sent packet
  if (recv_icmp->un.echo.id != icmp->un.echo.id) {
    return false;
  }

  // Validate ICMP sequence matches our sent packet
  if (recv_icmp->un.echo.sequence != icmp->un.echo.sequence) {
    return false;
  }

  rtt_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
  return true;
}

void RssNode::timer_callback()
{
  // Read RSSI
  double rss_dbm = -200.0;
  bool has_rss = read_rss_once(rss_dbm);

  // Ping router multiple times for packet loss %
  int success_count = 0;
  double rtt_sum = 0.0;
  for (int i = 0; i < PING_COUNT_PER_CYCLE; ++i) {
    double rtt_ms = 0.0;
    if (ping_once(rtt_ms)) {
      success_count++;
      rtt_sum += rtt_ms;
    }
    usleep(10000);  // 10ms between pings
  }
  double packet_loss_pct = 100.0 * (PING_COUNT_PER_CYCLE - success_count) / PING_COUNT_PER_CYCLE;
  double avg_rtt_ms = success_count > 0 ? rtt_sum / success_count : 0.0;
  bool has_ping = success_count > 0;

  // Timestamp
  auto now = this->now();
  auto now_sec = now.seconds();
  std::time_t time_t_now = static_cast<std::time_t>(now_sec);
  std::tm tm_now{};
  localtime_r(&time_t_now, &tm_now);
  std::ostringstream time_ss;
  time_ss << std::put_time(&tm_now, "%H:%M:%S");

  // Single-line output
  std::ostringstream ss;
  if (has_rss) {
    ss << "RSS: " << static_cast<int>(rss_dbm) << "dBm";
  } else {
    ss << "RSS: N/A";
  }
  ss << " | ";
  if (has_ping) {
    ss << "PING: " << ROUTER_IP << " (" << std::fixed << std::setprecision(1) << avg_rtt_ms << "ms avg";
    if (packet_loss_pct > 0) {
      ss << ", " << std::fixed << std::setprecision(0) << packet_loss_pct << "% loss";
    }
    ss << ")";
  } else {
    ss << "PING: " << ROUTER_IP << " (100% loss)";
  }
  ss << " | T: " << now_sec << " (" << time_ss.str() << ")";

  RCLCPP_INFO(get_logger(), "%s", ss.str().c_str());
}

}  // namespace connectivity_check

#include <rclcpp/rclcpp.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<connectivity_check::RssNode>(rclcpp::NodeOptions());
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}