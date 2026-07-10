# CLAUDE.md — Project Memory & Guardrails for conectivity_check

## Project Overview
**Package:** `conectivity_check` — ROS 2 (C++17, Jazzy) connectivity + RSS monitoring for Naval-Rex Edge Computing UAV/UGV
**Workspace:** `/home/psampaioc/Workspace/Naval-Rex/ros2_ws/src/conectivity_check`
**Build System:** ament_cmake

## Architecture (4 Independent Nodes)
```
conectivity_check/
├── ping_checker       # ICMP ping multi-target (L3/L4) — REQUIRES CAP_NET_RAW
├── rss_monitor        # RSS per interface (L1/L2) — WiFi nl80211, Cellular ModemManager, Generic sysfs
├── connectivity_monitor # Optional aggregator (unified summary)
└── speedtest_server   # speedtest-cli/iperf3 on demand
```

## Key Technical Decisions
- **RSS absolute in dBm** (`rss_dbm` field) is the core metric — not percentage, not quality index
- **Polymorphic providers**: `RssProvider` factory creates WiFi (nl80211/iw/proc), Cellular (ModemManager/AT/QMI), Generic (sysfs), Ethernet (carrier)
- **Single YAML config** (`config/connectivity.yaml`) drives all nodes
- **CAP_NET_RAW only for ping_checker** — RSS providers use netlink/DBus/sysfs, no raw sockets
- **Docker permissions**: ping_checker needs `--cap-add=NET_RAW`; rss_monitor needs `/run/dbus`, `/dev/ttyUSB*`, `dialout` group

## Build & Test Workflow
```bash
# Inside container (rosstudy / rosstudyplus)
cd /workspace
colcon build --packages-select conectivity_check --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
colcon test --packages-select conectivity_check
colcon test-result --verbose
```

## Validation Checklist
| Component | Topic | Expected |
|-----------|-------|----------|
| RSS Wi-Fi | `/connectivity/rss/wlan0` | `rss_dbm` ~ -40 to -80, `snr_db` > 10 |
| RSS Cellular | `/connectivity/rss/wwan0` | `rsrp_dbm` ~ -80 to -110, `sinr_db` > 5 |
| Ping Edge | `/connectivity/ping/edge` | `reachable=true`, `rtt_ms` < 50 |
| Ping Internet | `/connectivity/ping/internet` | `reachable=true`, `rtt_ms` < 100 |
| RSS Summary | `/connectivity/rss/summary` | `best/worst_interface` correct |
| Speedtest | `/connectivity/speedtest` service | Returns download/upload Mbps |

## Docker Infrastructure Rules (Agent-Owned Containers Only)
- **NEVER** touch user's `rosstudy`/`rosstudyplus` containers or aliases
- **Agent containers**: `ros2_agent_dock_<N>` (dynamic naming, detached mode)
- **Startup**: `xhost +local:root` → `docker run -d --rm --name <NAME> ... rosstudy_env:jazzy sleep infinity`
- **Exec**: `docker exec --workdir="/workspace" <NAME> bash -c "<COMMANDS>"`
- **Teardown**: `docker stop <NAME>` + verify with `docker ps` no `ros2_agent_dock_*` remain
- **Dockerfile modifications only** for new OS deps → rebuild with `docker build -t rosstudy_env:jazzy .`

## File Structure
```
├── CMakeLists.txt                    # Root: messages, subdirs, install
├── package.xml                       # Dependencies (libnl, libmm-glib, yaml-cpp, etc.)
├── config/connectivity.yaml          # Central config
├── msg/*.msg (6)                     # PingResult, PingSummary, RssMeasurement, RssSummary, SpeedtestResult, ConnectivitySummary
├── srv/TriggerSpeedtest.srv          # Speedtest service
├── include/conectivity_check/        # Public headers
│   ├── rss_provider.hpp              # Abstract interface + factory
│   ├── ping_checker.hpp              # Ping config/types
│   ├── rss_monitor.hpp               # RSS monitor node
│   ├── wifi_rss_provider.hpp         # nl80211/iw/proc
│   ├── cellular_rss_provider.hpp     # ModemManager/AT/QMI
│   └── generic_rss_provider.hpp      # sysfs/carrier
├── src/
│   ├── rss_monitor/                  # Core RSS implementation
│   ├── ping_checker/                 # ICMP raw socket
│   ├── speedtest_server/             # speedtest-cli/iperf3
│   └── connectivity_monitor/         # Aggregator
├── launch/connectivity_stack.launch.py
├── test/test_messages.cpp            # GTest validation
└── README.md
```

## Dependencies
| Purpose | Library | Ubuntu Package |
|---------|---------|----------------|
| Wi-Fi RSSI (nl80211) | libnl | `libnl-3-dev`, `libnl-genl-3-dev` |
| Cellular RSS | ModemManager GLib | `libmm-glib-dev`, `modemmanager` |
| YAML config | yaml-cpp | `libyaml-cpp-dev` |
| Speedtest | speedtest-cli | `speedtest-cli` |
| Ping ICMP | Linux kernel | `CAP_NET_RAW` capability |

## Thresholds (for dashboards/alerts)
| RSS (dBm) | Quality | Color | Action |
|-----------|---------|-------|--------|
| > -50 | Excellent | 🟢 | Normal |
| -50 to -70 | Good | 🟢 | Normal |
| -70 to -85 | Fair | 🟡 | Plan handover |
| -85 to -100 | Weak | 🟠 | Degraded, reduce rate |
| < -100 | Critical | 🔴 | Unstable, fallback |

## ⚠️ CRITICAL GOTCHAS & ANTI-PATTERNS (Must Read Before Building)

### 1. ROS 2 Same-Package Messages — NEVER DO THIS
```cmake
# ❌ WRONG — causes circular dependency / "package not found" error
ament_target_dependencies(my_node
  rclcpp
  ${PROJECT_NAME}   # NEVER put project name here!
)
```

**Correct pattern — ALWAYS use `rosidl_target_interfaces`:**
```cmake
# ✅ CORRECT
ament_target_dependencies(my_node
  rclcpp
  std_msgs
  builtin_interfaces
)
rosidl_target_interfaces(my_node ${PROJECT_NAME} "rosidl_typesupport_cpp")
```

This applies to **ALL** same-package targets: executables, test targets, components.

### 2. Tool Usage — NEVER USE `Write` ON EXISTING FILES
The `Write` tool **completely overwrites** file contents. For any modification to existing files:
- Use `Edit` tool (preferred)
- Use `sed` / bash commands
- Use `Read` → `Edit` workflow

**You truncated `wifi_rss_provider.cpp` today by using `Write` instead of `Edit`.**

### 3. Host Isolation — NEVER RUN SYSTEM COMMANDS ON HOST
- ❌ `dpkg -l`, `apt-cache search`, `rm -rf` on host
- ✅ All package checks, cache cleaning, builds inside container: `docker exec ros2_agent_dock_N bash -c "..."`
- If new OS deps needed → modify `Dockerfile` → `docker build -t rosstudy_env:jazzy .` → restart container

### 4. Include Paths for System Libraries
Root-level `include_directories()` does **not** propagate to `add_subdirectory` targets.
**Fix:** Add `target_include_directories(target PRIVATE ${LIBNL_INCLUDE_DIRS} ${MODEM_MANAGER_INCLUDE_DIRS})` in each subdirectory's `CMakeLists.txt`.

### 5. `find_package` Required Per Subdirectory
If a target uses `rclcpp_components` (or any extra package), the subdirectory's `CMakeLists.txt` MUST call `find_package(rclcpp_components REQUIRED)` before `ament_target_dependencies`.

---

## Current Project State (as of 2026-07-10)

### ✅ Architecture Fixed
- Root `CMakeLists.txt`: All `rosidl_generate_interfaces`, `find_package`, `add_subdirectory` restored
- `rosidl_target_interfaces` applied to all 4 node targets + test target
- `find_package(rclcpp_components REQUIRED)` in connectivity_monitor
- `target_include_directories` with pkg-config paths added to rss_monitor (needs adding to other subdirs)

### 🔴 ACTION ITEM 1: RESTORE `src/rss_monitor/wifi_rss_provider.cpp` FROM GIT
**You accidentally truncated this file using the `Write` tool.** The current version on disk is incomplete (missing includes, wrong struct definition). Restore from git:
```bash
cd /home/psampaioc/Workspace/Naval-Rex/ros2_ws/src/conectivity_check
git checkout HEAD -- src/rss_monitor/wifi_rss_provider.cpp
# Then re-apply only the necessary fixes (remove duplicate StationInfo, add rclcpp includes)
```

### 🔴 ACTION ITEM 2: PENDING C++ BUGS TO FIX
| File | Issue | Fix |
|------|-------|-----|
| `src/ping_checker/ping_checker.cpp` | `target.label < pubs.size()` — string vs size_t comparison | Use index-based lookup instead of string indexing into vector |
| `src/ping_checker/ping_checker.cpp` | `impl_` not accessible from `Impl::run_cycle` | Pass `impl_` reference or restructure |
| `src/rss_monitor/wifi_rss_provider.cpp` | Missing `#include <rclcpp/clock.hpp>` and `#include <rclcpp/logging.hpp>` | Add includes after git restore |
| `src/rss_monitor/wifi_rss_provider.cpp` | `nl_name2index`, `NL80211_ATTR_SIGNAL_MBM`, `NL80211_ATTR_NOISE_MBM`, `NL80211_ATTR_TX_BITRATE` not found | Check libnl version constants; may need `NL80211_ATTR_SIGNAL_MBM` → `NL80211_BSS_SIGNAL_MBM` |
| `src/rss_monitor/wifi_rss_provider.cpp` | `station_handler` cannot call non-static `parse_station_info` | Make `parse_station_info` static or pass `this` |
| `src/connectivity_monitor/connectivity_monitor.cpp` | `ConnectivitySummary` missing fields `overall_healthy`, `connectivity_ok`, `signal_ok` | Update `.msg` definition or remove references |
| `include/conectivity_check/cellular_rss_provider.hpp` | `#include <modemmanager.h>` not found | Use `#include <ModemManager/ModemManager.h>` |

### 🔴 ACTION ITEM 3: MISSING INCLUDE DIRECTORIES
Add `target_include_directories` to remaining subdirectories:
- `src/connectivity_monitor/CMakeLists.txt`
- `src/ping_checker/CMakeLists.txt`
- `src/speedtest_server/CMakeLists.txt`

---

## Build Status
- ✅ CMake generation passes
- ✅ Message generation passes
- ❌ C++ compilation fails (see ACTION ITEMS above)

---

## Skills Registered
- `ros2-docker`: Autonomous Docker container management for build/test
- `audit_ros2_cmake`: Scans CMakeLists.txt for `${PROJECT_NAME}` in `ament_target_dependencies`