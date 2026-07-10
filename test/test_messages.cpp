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
#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include "conectivity_check/msg/ping_result.hpp"
#include "conectivity_check/msg/rss_measurement.hpp"

TEST(PingResultTest, DefaultConstruction) {
    conectivity_check::msg::PingResult msg;
    EXPECT_EQ(msg.target, "");
    EXPECT_EQ(msg.label, "");
    EXPECT_DOUBLE_EQ(msg.rtt_ms, 0.0);
    EXPECT_DOUBLE_EQ(msg.jitter_ms, 0.0);
    EXPECT_DOUBLE_EQ(msg.packet_loss_pct, 0.0);
    EXPECT_FALSE(msg.reachable);
    EXPECT_EQ(msg.error, "");
}

TEST(RssMeasurementTest, DefaultConstruction) {
    conectivity_check::msg::RssMeasurement msg;
    EXPECT_EQ(msg.interface, "");
    EXPECT_EQ(msg.type, "");
    EXPECT_EQ(msg.label, "");
    EXPECT_DOUBLE_EQ(msg.rss_dbm, 0.0);
    EXPECT_DOUBLE_EQ(msg.noise_dbm, 0.0);
    EXPECT_DOUBLE_EQ(msg.snr_db, 0.0);
    EXPECT_FALSE(msg.link_up);
}

TEST(RssMeasurementTest, WifiFields) {
    conectivity_check::msg::RssMeasurement msg;
    msg.interface = "wlan0";
    msg.type = "wifi";
    msg.label = "main_wifi";
    msg.rss_dbm = -65.5;
    msg.noise_dbm = -95.0;
    msg.snr_db = 29.5;
    msg.ssid = "TestAP";
    msg.bssid = "aa:bb:cc:dd:ee:ff";
    msg.frequency_mhz = 5180;
    msg.security = "WPA2";

    EXPECT_EQ(msg.interface, "wlan0");
    EXPECT_EQ(msg.type, "wifi");
    EXPECT_DOUBLE_EQ(msg.rss_dbm, -65.5);
    EXPECT_DOUBLE_EQ(msg.snr_db, 29.5);
    EXPECT_EQ(msg.ssid, "TestAP");
}

TEST(RssMeasurementTest, CellularFields) {
    conectivity_check::msg::RssMeasurement msg;
    msg.interface = "wwan0";
    msg.type = "cellular";
    msg.label = "4g_modem";
    msg.rss_dbm = -89.0;
    msg.rsrp_dbm = -95.0;
    msg.rsrq_db = -11.0;
    msg.sinr_db = 18.0;
    msg.operator_name = "Claro";
    msg.technology = "LTE";
    msg.band = "B3";
    msg.registered = true;

    EXPECT_EQ(msg.type, "cellular");
    EXPECT_DOUBLE_EQ(msg.rsrp_dbm, -95.0);
    EXPECT_EQ(msg.operator_name, "Claro");
    EXPECT_TRUE(msg.registered);
}

TEST(RssMeasurementTest, EthernetFields) {
    conectivity_check::msg::RssMeasurement msg;
    msg.interface = "eth0";
    msg.type = "ethernet";
    msg.label = "wired";
    msg.link_up = true;
    msg.rss_dbm = 0.0;
    msg.driver_info = "carrier_only";

    EXPECT_EQ(msg.type, "ethernet");
    EXPECT_TRUE(msg.link_up);
}

TEST(RssMeasurementTest, GenericFields) {
    conectivity_check::msg::RssMeasurement msg;
    msg.interface = "radio0";
    msg.type = "generic";
    msg.label = "telemetry_radio";
    msg.rss_dbm = -72.0;
    msg.driver_info = "si4463";

    EXPECT_EQ(msg.type, "generic");
    EXPECT_DOUBLE_EQ(msg.rss_dbm, -72.0);
    EXPECT_EQ(msg.driver_info, "si4463");
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
