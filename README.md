# connectivity_check — Minimal RSS + Ping Monitor

A **single-node** ROS 2 (C++17, Jazzy) connectivity monitor combining:
- **WiFi RSSI** via nl80211 netlink (direct, no provider abstraction)
- **ICMP ping** to router (raw socket, multiple pings per cycle for packet loss %)

Outputs a single human-readable log line at 1 Hz.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    rss_node (single executable)             │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────────┐    ┌──────────────────────────────┐  │
│  │ nl80211 netlink  │    │ ICMP raw socket              │  │
│  │ (CAP_NET_ADMIN)  │    │ (CAP_NET_RAW)                │  │
│  └────────┬─────────┘    └──────────────┬───────────────┘  │
│           │                             │                  │
│           └──────────────┬──────────────┘                  │
│                          ▼                                 │
│              ┌─────────────────────┐                       │
│              │ Timer callback (1Hz)│                       │
│              │ Single log output   │                       │
│              └─────────────────────┘                       │
└─────────────────────────────────────────────────────────────┘
```

**No custom messages, no YAML config, no launch files, no multiple nodes.**

## Output Format

```
RSS: -53dBm | PING: 192.168.1.1 (2.7ms avg) | T: 1784660161 (18:56:01)
RSS: -72dBm | PING: 192.168.1.1 (12.4ms avg, 33% loss) | T: 1784660162 (18:56:02)
RSS: N/A | PING: 192.168.1.1 (100% loss) | T: 1784660163 (18:56:03)
```

| Field | Meaning |
|-------|---------|
| `RSS: -53dBm` | WiFi signal strength (higher = better, -30 max) |
| `RSS: N/A` | WiFi interface down / not associated |
| `PING: 2.7ms avg` | Average RTT over 3 pings |
| `PING: 12.4ms avg, 33% loss` | Average RTT + packet loss % |
| `PING: 100% loss` | All pings failed |
| `T: 1784660161 (18:56:01)` | Unix timestamp + human time |

## Quick Start (Docker)

```bash
# 1. Build base image (one-time, if not exists)
docker build -t rosstudy_env:jazzy .

# 2. Run container with required capabilities
xhost +local:root
docker run -d --rm --name connectivity_test \
  --cap-add=NET_RAW --cap-add=NET_ADMIN \
  --net=host --ipc=host --pid=host \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /home/psampaioc/Workspaces/Naval-Rex:/workspace \
  -w /workspace \
  rosstudy_env:jazzy sleep infinity

# 3. Build package
docker exec --workdir="/workspace/conectivity_ws" connectivity_test bash -c "
  source /opt/ros/jazzy/setup.bash
  colcon build --packages-select connectivity_check --symlink-install
"

# 4. Run node (from build dir works immediately)
docker exec --workdir="/workspace/conectivity_ws" connectivity_test bash -c "
  source /opt/ros/jazzy/setup.bash
  ./build/connectivity_check/rss_node
"

# Or run via ros2 run (after sourcing install)
docker exec --workdir="/workspace/conectivity_ws" connectivity_test bash -c "
  source /opt/ros/jazzy/setup.bash
  source install/setup.bash
  ros2 run connectivity_check rss_node
"
```

## Configuration

Edit constants at top of `src/connectivity_check/src/rss_node.cpp`:

```cpp
static constexpr const char * WIFI_INTERFACE = "wlp3s0";   // Your WiFi interface
static constexpr const char * ROUTER_IP = "192.168.1.1";    // Target to ping
static constexpr double UPDATE_RATE_HZ = 1.0;               // Output rate
static constexpr int PING_COUNT_PER_CYCLE = 3;              // Pings per cycle
static constexpr int PING_TIMEOUT_MS = 1000;                // Per-ping timeout
static constexpr int PING_PACKET_SIZE = 56;                 // ICMP payload
```

## RSSI Reference (Cheat Sheet)

| RSSI (dBm) | Quality | Action |
|------------|---------|--------|
| > -50 | 🟢 Excellent | Normal |
| -50 to -70 | 🟢 Good | Normal |
| -70 to -85 | 🟡 Fair | Monitor |
| -85 to -100 | 🟠 Weak | Degraded |
| < -100 | 🔴 Critical | Unstable |
| N/A | ⚫ No link | Disconnected |

See `RSSI_CHEAT_SHEET.md` for full reference including ping RTT and packet loss interpretation.

## Dependencies

| Purpose | Library | Ubuntu Package |
|---------|---------|----------------|
| nl80211 netlink | libnl | `libnl-3-dev`, `libnl-genl-3-dev` |
| ROS 2 | rclcpp, std_msgs | `ros-jazzy-rclcpp`, `ros-jazzy-std-msgs` |
| Build | ament_cmake | `ros-jazzy-ament-cmake` |

**Runtime capabilities required:** `CAP_NET_RAW` (ping), `CAP_NET_ADMIN` (netlink)

## Build (Native, without Docker)

```bash
# In ROS 2 Jazzy environment
cd /home/psampaioc/Workspaces/Naval-Rex/conectivity_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-select connectivity_check --symlink-install
source install/setup.bash
ros2 run connectivity_check rss_node
```

## License

Apache-2.0