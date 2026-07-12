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
- If new OS deps needed → modify `Dockerfile` → `docker build -t rosstudy_env:jazzy .`

---

## 🤖 Automation Permissions (Permanent Agent Permissions)

**These permissions are permanent and persistent across sessions. You do NOT need to ask for permission each time.**

### ✅ ALWAYS ALLOWED (No Permission Required)
| Action | Tool/Command | Notes |
|--------|--------------|-------|
| Build package | `colcon build --packages-select conectivity_check --symlink-install` | Inside container |
| Run tests | `colcon test --packages-select conectivity_check` | Inside container |
| View test results | `colcon test-result --verbose` | Inside container |
| Git operations (status, diff, log, add, commit, push, branch, merge) | `git ...` | Standard git workflow |
| Create/switch branches | `git checkout -b`, `git checkout` | Standard git workflow |
| Create PR / push to origin | `git push origin <branch>` / `gh pr create` | Standard git workflow |
| Create GitHub repo & push | `gh repo create <org>/<repo> --public --source=. --push` | One-time setup |
| Read any file in workspace | `Read`, `cat`, `cat` | Read-only |
| Search/grep codebase | `Grep`, `grep`, `rg` | Read-only |
| List directory contents | `Glob`, `ls`, `find` | Read-only |
| Run container commands | `docker exec ros2_agent_dock_* bash -c "..."` | Agent-owned containers only |
| Create/manage agent containers | `docker run -d --rm --name ros2_agent_dock_N ...` | Dynamic naming |
| Read Docker logs | `docker logs ros2_agent_dock_*` | Read-only |
| Stop agent containers | `docker stop ros2_agent_dock_*` | Cleanup only |
| Edit existing files | `Edit`, `sed` | Standard edits |
| Create new files | `Write` (new files only) | New files only |
| Search web for docs | `WebSearch`, `WebFetch` | Public docs only |

### ⚠️ REQUIRES EXPLICIT PERMISSION (Ask First)
| Action | Why |
|--------|-----|
| **Write/overwrite existing files** with `Write` tool | Destructive — use `Edit` instead |
| **Delete files/directories** | Irreversible |
| **Modify Dockerfile / rebuild base image** | Affects all containers, slow rebuild |
| **Modify user's personal containers** (`rosstudy`, `rosstudyplus`) | User-owned, not agent-owned |
| **Run commands on host system** (outside containers) | Breaks isolation |
| **Delete/move user files outside workspace** | Out of scope |
| **Create GitHub repos in user's personal namespace** | User namespace control |
| **Push to user's personal repos without asking** | User controls their remotes |
| **Install system packages on host** | Host isolation |
| **Modify CLAUDE.md** (this file) | Project instructions — confirm changes |

### 🐳 Docker Container Rules (Agent-Owned Only)
- **Container naming**: `ros2_agent_dock_<N>` (dynamic N, detached mode)
- **Startup**: `xhost +local:root` → `docker run -d --rm --name ros2_agent_dock_<N> --net=host --ipc=host --pid=host -v /tmp/.X11-unix:/tmp/.X11-unix -v /home/psampaioc/Workspace:/workspace -w /workspace rosstudy_env:jazzy sleep infinity`
- **Exec**: `docker exec --workdir="/workspace" ros2_agent_dock_<N> bash -c "<COMMANDS>"`
- **Teardown**: `docker stop ros2_agent_dock_<N>` + verify `docker ps` shows no `ros2_agent_dock_*`
- **Dockerfile changes ONLY** for new OS deps → rebuild with `docker build -t rosstudy_env:jazzy .`
- **NEVER** touch user's `rosstudy` / `rosstudyplus` containers or aliases

### 📦 Git/GitHub Workflow
1. Work on feature branches (`git checkout -b feature/xyz`)
2. Commit with conventional commits (`feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `chore:`)
3. Push to origin (`git push origin feature/xyz`)
4. Create PR (`gh pr create --title "..." --body "..."`)
5. After review/merge, delete branch locally and remotely
6. **Main branch protection**: Push directly to main only for docs/chore; features via PR

### 📝 Commit Message Convention
```
<type>(<scope>): <subject>

<body>

Co-Authored-By: Claude <noreply@anthropic.com>
```
Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `perf`, `ci`, `build`

### 📋 GitHub Repo (Canonical)
- **Organization**: `ros2-connectivity-check`
- **Repository**: `ROS2_Conectivity_Check`
- **URL**: `https://github.com/ros2-connectivity-check/ROS2_Conectivity_Check`

--- → restart container

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

---

## ROS 2 Best Practices, Errors & Gotchas (Learned 2026-07-10)

### 1. Component Registration — `RCLCPP_COMPONENTS_REGISTER_NODE` MUST Be Outside Namespace
**Fatal Error:** If you place `RCLCPP_COMPONENTS_REGISTER_NODE(MyNode)` inside the `namespace my_pkg { ... }`, the `class_loader` symbols get mangled (prefixed with `my_pkg::`). This causes linker errors like:
```
undefined reference to `my_pkg::class_loader::impl::getPluginBaseToFactoryMapMapMutex()'
undefined reference to `my_pkg::console_bridge::log(...)
```

**Correct Pattern:**
```cpp
namespace my_pkg {
class MyNode : public rclcpp::Node { ... };
}  // namespace my_pkg

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(my_pkg::MyNode)  // OUTSIDE namespace
```

### 2. Component Node Constructors Require `NodeOptions`
For a node to be loadable as a ROS 2 component (via `ros2 component load` or `composition`), the constructor **must** accept `const rclcpp::NodeOptions& options` and forward it to the base:
```cpp
class MyNode : public rclcpp::Node {
public:
  explicit MyNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
  : Node("my_node", options) { ... }
};
```
Without this, the component factory cannot instantiate the node.

### 3. Security: Never Hardcode Local Filesystem Paths
**Anti-pattern:**
```cpp
config_file = std::string(getenv("HOME")) + "/Workspace/Naval-Rex/ros2_ws/src/conectivity_check/config/connectivity.yaml";
```
This leaks local username, project structure, and absolute paths — unacceptable for public repos.

**Correct Pattern — Use `ament_index_cpp`:**
```cpp
#include <ament_index_cpp/get_package_share_directory.hpp>

std::string pkg_share = ament_index_cpp::get_package_share_directory("conectivity_check");
std::string config_file = pkg_share + "/config/connectivity.yaml";
```
Add dependency: `find_package(ament_index_cpp REQUIRED)` in CMakeLists.txt, `<depend>ament_index_cpp</depend>` in package.xml.

### 4. cpplint Include Order Rules (ROS 2 Enforced)
The linter enforces strict ordering in `.cpp` files:
1. **C system headers** (e.g., `<arpa/inet.h>`, `<sys/socket.h>`)
2. **C++ system headers** (e.g., `<chrono>`, `<string>`, `<vector>`)
3. **Other libraries' headers** (e.g., `<rclcpp/rclcpp.hpp>`, `<yaml-cpp/yaml.h>`)
4. **Your project's headers** (e.g., `"conectivity_check/my_header.hpp"`)

Within each group, sort alphabetically. Headers (`.hpp`) follow: project headers first, then system.

### 5. Same-Package Message Dependencies — Use `rosidl_target_interfaces`
**NEVER** do this:
```cmake
ament_target_dependencies(my_node rclcpp ${PROJECT_NAME})
```
**ALWAYS** do this:
```cmake
ament_target_dependencies(my_node rclcpp std_msgs builtin_interfaces)
rosidl_target_interfaces(my_node ${PROJECT_NAME} "rosidl_typesupport_cpp")
```
Applies to ALL targets: executables, components, tests.

### 6. `find_package` Per Subdirectory
If a subdirectory's target uses `rclcpp_components` (or any non-root `find_package`), that subdirectory's `CMakeLists.txt` MUST call `find_package(rclcpp_components REQUIRED)` before `ament_target_dependencies`.

### 7. Include Paths Don't Propagate from Root
Root-level `include_directories()` does NOT propagate to `add_subdirectory` targets. Each subdirectory's `CMakeLists.txt` must call:
```cmake
target_include_directories(my_target PRIVATE ${LIBNL_INCLUDE_DIRS} ${MODEM_MANAGER_INCLUDE_DIRS})
```

### 8. `Write` Tool Truncates Existing Files
The `Write` tool **completely overwrites** file contents. For modifications to existing files:
- Use `Edit` tool (preferred)
- Use `sed` / bash commands
- Use `Read` → `Edit` workflow

### 9. Host Isolation — Never Run System Commands on Host
All package checks, builds, tests run inside the agent Docker container:
```bash
docker exec --workdir="/workspace" ros2_agent_dock_N bash -c "colcon build ..."
```
Only modify `Dockerfile` for new OS dependencies, then rebuild the image.