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
#include "conectivity_check/cellular_rss_provider.hpp"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <cmath>
#include <rclcpp/logging.hpp>
#include <rclcpp/clock.hpp>

namespace conectivity_check
{

CellularRssProviderAt::CellularRssProviderAt()
: logger_(rclcpp::get_logger("cellular_at_unset")) {}

CellularRssProviderQmi::CellularRssProviderQmi()
: logger_(rclcpp::get_logger("cellular_qmi_unset")) {}

bool CellularRssProviderAt::init(const RssConfig & config, rclcpp::Logger logger)
{
  logger_ = logger;
  at_device_ = config.at_device;

  fd_ = open(at_device_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd_ < 0) {
    RCLCPP_ERROR(logger_, "Failed to open %s: %s", at_device_.c_str(), strerror(errno));
    return false;
  }

  // Configure serial port (115200 8N1)
  struct termios tty;
  if (tcgetattr(fd_, &tty) != 0) {
    close(fd_);
    fd_ = -1;
    return false;
  }

  cfsetospeed(&tty, B115200);
  cfsetispeed(&tty, B115200);
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 5;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    close(fd_);
    fd_ = -1;
    return false;
  }

  RCLCPP_INFO(logger_, "Cellular AT provider initialized for %s", at_device_.c_str());
  return true;
}

bool CellularRssProviderAt::send_at_command(const std::string & cmd, std::string & response)
{
  if (fd_ < 0) {return false;}

  std::string full_cmd = cmd + "\r";
  if (write(fd_, full_cmd.c_str(), full_cmd.size()) != static_cast<ssize_t>(full_cmd.size())) {
    return false;
  }

  // Read response (wait up to 1 second)
  char buf[1024];
  response.clear();
  fd_set fds;
  struct timeval tv = {1, 0};

  FD_ZERO(&fds);
  FD_SET(fd_, &fds);

  int ret = select(fd_ + 1, &fds, nullptr, nullptr, &tv);
  if (ret > 0 && FD_ISSET(fd_, &fds)) {
    ssize_t n = read(fd_, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      response = buf;
      return true;
    }
  }
  return false;
}

bool CellularRssProviderAt::read_once(msg::RssMeasurement & msg)
{
  msg.stamp = rclcpp::Clock().now();
  msg.interface = "wwan0";
  msg.type = "cellular";
  msg.label = "4g_modem";

  std::string response;

  // AT+CSQ for RSSI
  if (send_at_command("AT+CSQ", response)) {
    // Response: +CSQ: <rssi>,<ber>
    // rssi 0-31, 99=unknown. dBm = rssi*2 - 113
    size_t pos = response.find("+CSQ:");
    if (pos != std::string::npos) {
      int rssi = std::stoi(response.substr(pos + 5));
      if (rssi != 99) {
        msg.rss_dbm = rssi * 2 - 113;
      }
    }
  }

  // AT+QENG for LTE/5G details (Quectel)
  if (send_at_command("AT+QENG=\"servingcell\"", response)) {
    // Parse serving cell info for RSRP, RSRQ, etc.
    // This is modem-specific
  }

  return !std::isnan(msg.rss_dbm);
}

void CellularRssProviderAt::shutdown()
{
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

// QMI provider (placeholder)
bool CellularRssProviderQmi::init(const RssConfig & config, rclcpp::Logger logger)
{
  logger_ = logger;
  // Implementation would use libqmi-glib or subprocess qmicli
  (void)config;
  return false;  // Placeholder
}

bool CellularRssProviderQmi::read_once(msg::RssMeasurement & msg)
{
  (void)msg;
  return false;  // Placeholder
}

}  // namespace conectivity_check
