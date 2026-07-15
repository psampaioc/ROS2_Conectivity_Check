# ROS 2 Workspace & conectivity_check Mastery Guide
## Complete Educational Reference from Beginner to Advanced

---

## 📚 TABLE OF CONTENTS

1. [ROS 2 Workspace Fundamentals](#1-ros-2-workspace-fundamentals)
2. [Naval-Rex ros2_ws Deep Dive](#2-naval-rex-ros2_ws-deep-dive)
3. [conectivity_check Project Architecture](#3-conectivity_check-project-architecture)
4. [Building from Scratch: Decision Log](#4-building-from-scratch-decision-log)
5. [Folder Layout Philosophy](#5-folder-layout-philosophy)
6. [What's Required vs Optional](#6-whats-required-vs-optional)
7. [How to Build, Run & Test](#7-how-to-build-run--test)
8. [Using This in Production](#8-using-this-in-production)
9. [Common Pitfalls & Debugging](#9-common-pitfalls--debugging)
10. [Creating Your Own: Template Checklist](#10-creating-your-own-template-checklist)

---

## 1. ROS 2 WORKSPACE FUNDAMENTALS

### 1.1 What is a ROS 2 Workspace?

A **ROS 2 workspace** is a directory structure that organizes your ROS 2 packages. It follows a specific convention that tools like `colcon` (the build tool) understand.

```
ros2_ws/                          ← WORKSPACE ROOT
├── src/                          ← SOURCE PACKAGES (YOUR CODE)
│   ├── package_a/
│   ├── package_b/
│   └── package_c/
├── build/                        ← BUILD ARTIFACTS (GENERATED, DON'T TOUCH)
│   ├── package_a/
│   ├── package_b/
│   └── package_c/
├── install/                      ← INSTALLED ARTIFACTS (GENERATED, SOURCE THIS)
│   ├── package_a/
│   ├── package_b/
│   ├── package_c/
│   ├── setup.bash                ← SOURCE THIS TO USE PACKAGES
│   ├── setup.zsh
│   └── local_setup.bash
└── log/                          ← BUILD LOGS (GENERATED)
```

### 1.2 The Three Magic Directories

| Directory | Purpose | Should You Edit? | Gets Committed? |
|-----------|---------|------------------|-----------------|
| `src/` | **Your source packages** | YES - this is where you work | YES |
| `build/` | Intermediate build files (CMake cache, object files) | NO - auto-generated | NO (gitignore) |
| `install/` | Final installed packages (headers, libs, executables, share) | NO - auto-generated | NO (gitignore) |

**Key Insight:** You only ever edit files in `src/`. The `build/` and `install/` directories are **completely disposable** — you can delete them anytime and rebuild.

### 1.3 The Build Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                     colcon build                                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  1. Reads package.xml from each package in src/                │
│  2. Determines dependency order (topological sort)             │
│  3. For each package:                                           │
│     a. Creates build/<pkg>/ - runs CMake                       │
│     b. Compiles source → object files                          │
│     c. Links → libraries/executables                           │
│     d. Installs to install/<pkg>/                              │
│  4. Generates setup.bash with all paths                        │
└─────────────────────────────────────────────────────────────────┘
```

### 1.4 Why You See "Duplicate" Names

**This is the #1 confusion point for beginners.**

You might see:
- `src/conectivity_check/` — **Your source code** (edit this)
- `build/conectivity_check/` — **Build intermediates** (don't touch)
- `install/conectivity_check/` — **Installed artifacts** (don't touch)
- `.claude/worktrees/sanitize-repo/conectivity_check/` — **Git worktree** (temporary)

**They are NOT duplicates.** They are the SAME package at DIFFERENT STAGES of the build pipeline:

| Location | Stage | Analogy |
|----------|-------|---------|
| `src/conectivity_check/` | Source | Raw ingredients |
| `build/conectivity_check/` | Building | Cooking in progress |
| `install/conectivity_check/` | Installed | Plated meal ready to serve |

---

## 2. NAVAL-REX ROS2_WS DEEP DIVE

### 2.1 Actual Directory Tree (What You Have)

```
<YOUR_WORKSPACE_ROOT>/ros2_ws/
├── src/
│   ├── conectivity_check/          ← YOUR PACKAGE (edit this!)
│   │   ├── config/connectivity.yaml
│   │   ├── include/conectivity_check/*.hpp
│   │   ├── src/
│   │   │   ├── connectivity_monitor/
│   │   │   ├── ping_checker/
│   │   │   ├── rss_monitor/
│   │   │   └── speedtest_server/
│   │   ├── msg/*.msg
│   │   ├── srv/TriggerSpeedtest.srv
│   │   ├── launch/connectivity_stack.launch.py
│   │   ├── test/test_messages.cpp
│   │   ├── CMakeLists.txt
│   │   ├── package.xml
│   │   ├── README.md
│   │   ├── SPEC_TECNICO.md
│   │   ├── CLAUDE.md
│   │   └── Dockerfile
│   │
│   └── naval_rex_core/             ← ANOTHER PACKAGE (separate!)
│       ├── naval_rex_core/         ← Python package
│       ├── msg/
│       ├── config/
│       ├── launch/
│       ├── package.xml
│       ├── setup.py
│       └── setup.cfg
│
├── build/                          ← AUTO-GENERATED (ignore)
│   ├── conectivity_check/
│   └── naval_rex_core/
│
├── install/                        ← AUTO-GENERATED (ignore)
│   ├── conectivity_check/
│   ├── naval_rex_core/
│   ├── setup.bash                  ← SOURCE THIS!
│   └── local_setup.bash
│
├── log/                            ← AUTO-GENERATED (ignore)
│   └── latest_build/
│
└── prompt.txt                      ← Random file (ignore)
```

### 2.2 Two Packages, Different Purposes

| Package | Language | Purpose |
|---------|----------|---------|
| `conectivity_check` | **C++17** | Core connectivity stack (4 nodes, custom msgs) |
| `naval_rex_core` | **Python** | Higher-level Naval-Rex logic (separate concern) |

**They are independent.** They share the workspace only for build convenience.

### 2.3 What's in `build/` and `install/` (For Curiosity)

```
build/conectivity_check/
├── CMakeCache.txt                    # CMake configuration cache
├── CMakeFiles/                       # Build rules per target
├── conectivity_check__rosidl_generator_cpp/  # Generated C++ msg code
├── conectivity_check__rosidl_typesupport_cpp/
├── ament_cmake_core/                 # ament CMake macros
├── ament_cppcheck/                   # Static analysis output
├── ament_cpplint/                    # Lint output
├── ament_uncrustify/                 # Format check output
├── ament_xmllint/                    # XML validation output
├── test_results/                     # GTest results (XML)
└── compile_commands.json             # For clangd/LSP

install/conectivity_check/
├── lib/
│   ├── libconectivity_check__rosidl_typesupport_cpp.so
│   ├── connectivity_monitor          ← Executable
│   ├── ping_checker                  ← Executable
│   ├── rss_monitor                   ← Executable
│   └── speedtest_server              ← Executable
├── include/conectivity_check/        ← Public headers
├── share/conectivity_check/
│   ├── config/connectivity.yaml      ← Installed config
│   ├── launch/connectivity_stack.launch.py
│   ├── package.xml
│   └── resource/                     ← ament index
└── local_setup.bash                  ← Source for just this pkg
```

---

### 3.2 Message Definitions (The Contracts)
## 3. CONETIVY_CHECK PROJECT ARCHITECTURE

### 3.1 High-Level Design: 3 Independent Nodes (KISS Architecture)

```
┌──────────────────────────────────────────────────────────────────┐
│                    CONETIVY_CHECK STACK (KISS)                   │
├─────────────────┬─────────────────┬──────────────────────────────┤
│  ping_checker   │   rss_monitor   │      connectivity_monitor    │
│  (L3/L4 ICMP)   │  (L1/L2 RSS)    │      (Aggregator)            │
├─────────────────┼─────────────────┼──────────────────────────────┤
│ Raw ICMP socket │ nl80211 netlink │ Subscribes to ping + RSS     │
│ CAP_NET_RAW     │ (direct, no     │ Single-line human summary    │
│ Multi-target    │  provider factory)                          │
└────────┬────────┴────────┬────────┴──────────────┬──────────────┘
         │                │                       │
         ▼                ▼                       ▼
    /ping/<label>   /rss/<iface>             /summary (1 line!)
    /ping/summary   /rss/summary
```

**Key Principle:** Nodes are **independent**. You can run just `rss_monitor` without `ping_checker`. They communicate only via ROS 2 topics.

**Architecture Decision (2026-07-15):** Hard reset from complex provider factory to KISS netlink:
- ❌ Removed: Cellular/ModemManager/AT/QMI providers (not testable in container)
- ❌ Removed: Generic sysfs provider abstraction
- ❌ Removed: Provider factory pattern
- ✅ Kept: Direct nl80211 netlink in single rss_monitor.cpp
- ✅ Kept: Single WiFi interface from YAML (default `wlp3s0`)
- ✅ Kept: Ethernet carrier detection via carrier_only (works everywhere)

### 3.2 Message Definitions (The Contracts)

All custom messages in `msg/`:

| Message | Purpose | Key Fields |
|---------|---------|------------|
| `PingResult.msg` | Single ping result | `label`, `host`, `reachable`, `rtt_ms`, `jitter_ms`, `packet_loss_pct`, `error` |
| `PingSummary.msg` | Aggregated ping | `reachable_count`, `total_count`, `all_reachable`, `worst_rtt_ms`, `worst_label` |
| `RssMeasurement.msg` | **Core RSS metric** | `interface`, `type`, `label`, **`rss_dbm`**, `snr_db`, `bitrate_mbps`, `ssid`, `bssid`, `link_up`, `driver_info`... |
| `RssSummary.msg` | Best/worst RSS | `active_interfaces`, `best_interface`, `worst_interface`, `best_rss_dbm`, `worst_rss_dbm`, `any_wifi`, `any_cellular` |
| `SpeedtestResult.msg` | Speedtest output | `download_mbps`, `upload_mbps`, `latency_ms`, `server_name`, `timestamp` |
| `ConnectivitySummary.msg` | **Unified view** | `worst_ping_rtt_ms`, `worst_ping_label`, `ping_all_reachable`, `ping_reachable_count`, `ping_total_count`, `best_rss_dbm`, `worst_rss_dbm`, `best_interface`, `worst_interface`, `active_interfaces`, `any_wifi`, `any_cellular`, `overall_healthy`, **`human_readable_summary`** |

**Critical Design Decision:** `rss_dbm` is **absolute dBm** (e.g., -62.5), NOT percentage. This enables cross-technology comparison (WiFi -65 vs Cellular -95).
**No code changes needed to add interfaces** — just edit YAML.

---

### 3.3 Configuration-Driven (`config/connectivity.yaml`) — KISS Format

```yaml
# ============================================
# CONETIVY_CHECK - KISS CONFIGURATION
# ============================================

# Ping Checker - ICMP to multiple targets
ping_checker:
  update_rate_hz: 1.0
  ping_count: 3
  timeout_ms: 1000
  packet_size: 56
  targets:
    - label: "router"       # Appears in topic: /connectivity/ping/router
      host: "192.168.1.1"
      enabled: true
      interface: ""         # Empty = default route
    - label: "internet"
      host: "8.8.8.8"
      enabled: true
      interface: ""

# RSS Monitor - Single WiFi interface (KISS)
rss_monitor:
  update_rate_hz: 1.0
  interface: "wlp3s0"       # Default WiFi interface (from YAML)

# Connectivity Monitor - Aggregator (optional)
connectivity_monitor:
  update_rate_hz: 1.0
  enabled: true

# Speedtest Server - On-demand bandwidth test
speedtest_server:
  enabled: true
  provider: "speedtest-cli"
  timeout_sec: 120
```

**Key Changes from v1:**
- Single `interface` key instead of `interfaces` array
- Removed `type`, `method`, `modem_path`, `at_device`, `sysfs_path` — netlink auto-detects
- Cellular providers removed (not testable in container)

---

## 4. BUILDING FROM SCRATCH: DECISION LOG


---

## 4. BUILDING FROM SCRATCH: DECISION LOG

### 4.1 Step-by-Step Creation Process

#### Step 1: Create Workspace
```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws
```

#### Step 2: Create Package Skeleton
```bash
cd src
# For C++ package with custom messages:
ros2 pkg create --build-type ament_cmake conectivity_check
# This creates:
# conectivity_check/
# ├── CMakeLists.txt
# ├── package.xml
# ├── include/conectivity_check/
# └── src/
```

#### Step 3: Add Message Definitions
```bash
mkdir -p conectivity_check/msg conectivity_check/srv
# Create .msg files in msg/
# Create .srv files in srv/
```

#### Step 4: Configure CMakeLists.txt for Messages
```cmake
# Find rosidl for message generation
find_package(rosidl_default_generators REQUIRED)

# Generate messages
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/PingResult.msg"
  "msg/RssMeasurement.msg"
  # ... all messages
  "srv/TriggerSpeedtest.srv"
)
```

#### Step 5: Add Dependencies in package.xml
```xml
<depend>rclcpp</depend>
<depend>std_msgs</depend>
<depend>builtin_interfaces</depend>
<depend>yaml-cpp</depend>
<depend>libnl-3-dev</depend>          <!-- For WiFi nl80211 -->
<depend>libmm-glib-dev</depend>       <!-- For ModemManager -->
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

#### Step 6: Write Node Code with Component Support
```cpp
// Constructor MUST take NodeOptions for composability
class MyNode : public rclcpp::Node {
public:
  explicit MyNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
  : Node("my_node", options) {}
};
// Register OUTSIDE namespace
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(my_pkg::MyNode)
```

#### Step 7: Use rosidl_target_interfaces (CRITICAL!)
```cmake
# WRONG - causes circular dependency:
# ament_target_dependencies(my_node rclcpp ${PROJECT_NAME})

# CORRECT:
ament_target_dependencies(my_node rclcpp std_msgs builtin_interfaces)
rosidl_target_interfaces(my_node ${PROJECT_NAME} "rosidl_typesupport_cpp")
```

---

## 5. FOLDER LAYOUT PHILOSOPHY

### 5.1 Why This Structure?

```
conectivity_check/
├── config/              # YAML configs (installed to share/)
├── include/             # PUBLIC headers (installed)
│   └── conectivity_check/
│       ├── rss_provider.hpp       # Abstract interface
│       ├── wifi_rss_provider.hpp  # Concrete declarations
│       ├── cellular_rss_provider.hpp
│       ├── generic_rss_provider.hpp
│       ├── ping_checker.hpp
│       └── rss_monitor.hpp
├── launch/              # Launch files (installed)
├── msg/                 # .msg files (interface definitions)
├── srv/                 # .srv files (service definitions)
├── src/                 # PRIVATE implementation
│   ├── connectivity_monitor/   # One subdir per node
│   │   ├── CMakeLists.txt
│   │   └── main.cpp
│   ├── ping_checker/
│   │   ├── CMakeLists.txt
│   │   ├── ping_checker.cpp
│   │   └── ping_checker_main.cpp
│   ├── rss_monitor/           # Core RSS logic
│   │   ├── CMakeLists.txt
│   │   ├── rss_monitor.cpp       # Node implementation
│   │   ├── rss_monitor_main.cpp  # Entry point
│   │   ├── rss_provider.cpp      # Factory
│   │   ├── wifi_rss_provider.cpp
│   │   ├── cellular_rss_provider.cpp
│   │   └── generic_rss_provider.cpp
│   └── speedtest_server/
├── test/                # Unit/integration tests
├── CMakeLists.txt       # Root: messages, subdirs
├── package.xml          # Dependencies, metadata
├── README.md            # User documentation
├── SPEC_TECNICO.md      # Technical spec (internal)
├── CLAUDE.md            # Agent instructions (gitignored)
└── Dockerfile           # Build environment
```

### 5.2 Design Principles

| Principle | Applied Here |
|-----------|--------------|
| **Separation of concerns** | Each node = own subdir in `src/` |
| **Public vs Private** | `include/` = public API (installed); `src/` = implementation (not installed) |
| **One node per executable** | Each `src/<node>/main.cpp` → one executable |
| **Messages are the API** | `.msg` files define contracts between nodes |
| **Config over code** | YAML drives behavior; no recompile for new interfaces |
| **Factory pattern** | `RssProvider::create()` hides implementation details |

### 5.3 CMakeLists.txt Hierarchy

```
Root CMakeLists.txt
    ├── find_package() for all deps
    ├── rosidl_generate_interfaces()
    ├── include_directories(include)  ← PUBLIC headers
    └── add_subdirectory(src)         ← Delegates to sub-CMakeLists
        ├── src/connectivity_monitor/CMakeLists.txt
        │   ├── find_package(rclcpp_components)  ← Per-subdir!
        │   ├── add_executable(...)
        │   ├── ament_target_dependencies(...)
        │   ├── rosidl_target_interfaces(...)    ← SAME-PKG msgs!
        │   └── install(TARGETS ...)
        ├── src/ping_checker/CMakeLists.txt
        ├── src/rss_monitor/CMakeLists.txt
        │   ├── target_include_directories(... ${LIBNL_INCLUDE_DIRS} ${MODEM_MANAGER_INCLUDE_DIRS})
        │   └── ...
        └── src/speedtest_server/CMakeLists.txt
```

**Key Rule:** Each subdirectory that uses extra deps (like `rclcpp_components`, `libnl`, `ModemManager`) MUST call `find_package()` locally.

---

## 6. WHAT'S REQUIRED VS OPTIONAL

### 6.1 Required for ANY ROS 2 C++ Package

| File | Purpose |
|------|---------|
| `package.xml` | Metadata, dependencies, build type |
| `CMakeLists.txt` | Build instructions |
| At least one `.cpp` with `main()` or component node | Executable |

### 6.2 Required for Custom Messages

| File | Purpose |
|------|---------|
| `msg/*.msg` | Message definitions |
| `srv/*.srv` | Service definitions (if needed) |
| `rosidl_generate_interfaces()` in CMake | Generates C++/Python code |
| `rosidl_target_interfaces()` in each target | Links generated code |

### 6.3 Required for This Project Specifically

| Component | Why Needed |
|-----------|------------|
| `libnl-3-dev`, `libnl-genl-3-dev` | WiFi nl80211 netlink API |
| `libmm-glib-dev`, `modemmanager` | Cellular ModemManager DBus |
| `libyaml-cpp-dev` | YAML config parsing |
| `speedtest-cli` | Speedtest binary |
| `CAP_NET_RAW` capability | ICMP raw sockets (ping) |
| `CAP_NET_ADMIN` | Some nl80211 operations |

### 6.4 Optional But Recommended

| File | Purpose |
|------|---------|
| `launch/*.launch.py` | Composable launch (start all nodes) |
| `config/*.yaml` | Runtime configuration |
| `test/*.cpp` | Unit tests (GTest) |
| `README.md` | User documentation |
| `Dockerfile` | Reproducible build environment |
| `CLAUDE.md` | Agent instructions (gitignored) |

### 6.5 What You Can Delete Safely

| File/Dir | Safe to Remove? |
|----------|-----------------|
| `build/`, `install/`, `log/` | YES — regenerated on build |
| `.claude/` | YES — agent config only |
| `CLAUDE.md` | YES — local agent memory |
| `SPEC_TECNICO.md` | YES — internal spec |
| Individual test files | YES — if not testing |
| Unused provider implementations | YES — if not in factory |

---

## 7. HOW TO BUILD, RUN & TEST

### 7.1 Prerequisites (One-Time Setup)

```bash
# On host: install Docker
# Then build the dev container:
cd ~/Workspace/Naval-Rex/ros2_ws/src/conectivity_check
docker build -t rosstudy_env:jazzy .
```

### 7.2 Start Build Container (Agent Way - Headless)

```bash
# Allow GUI (for RViz/PlotJuggler later)
xhost +local:root

# Start detached container with NET_RAW for ping_checker
docker run -d --rm \
  --name ros2_agent_dock_1 \
  --net=host --ipc=host --pid=host \
  --cap-add=NET_RAW \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v ~/Workspace:/workspace \
  -w /workspace \
  rosstudy_env:jazzy \
  sleep infinity
```

### 7.3 Build Inside Container

```bash
# Build the package
docker exec --workdir="/workspace" ros2_agent_dock_1 bash -c "
  cd /workspace/Naval-Rex/ros2_ws
  colcon build --packages-select conectivity_check --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
"
```

### 7.4 Source & Run

```bash
# Run full stack
docker exec --workdir="/workspace" ros2_agent_dock_1 bash -c "
  cd /workspace/Naval-Rex/ros2_ws
  source install/setup.bash
  ros2 launch conectivity_check connectivity_stack.launch.py
"

# Or run individual nodes:
docker exec --workdir="/workspace" ros2_agent_dock_1 bash -c "
  cd /workspace/Naval-Rex/ros2_ws
  source install/setup.bash
  ros2 run conectivity_check rss_monitor
"
```

### 7.5 Verify It Works (In Another Terminal or &)

```bash
# Check topics
docker exec --workdir="/workspace" ros2_agent_dock_1 bash -c "
  cd /workspace/Naval-Rex/ros2_ws
  source install/setup.bash
  
  # List connectivity topics
  ros2 topic list | grep connectivity
  
  # Human-readable summary (4 lines max!)
  ros2 topic echo /connectivity/summary --once
  
  # Individual RSS
  ros2 topic echo /connectivity/rss/wlan0 --once
  ros2 topic echo /connectivity/rss/eth0 --once
  
  # Individual ping
  ros2 topic echo /connectivity/ping/router --once
  ros2 topic echo /connectivity/ping/internet --once
  
  # Call speedtest service
  ros2 service call /connectivity/speedtest std_srvs/srv/Trigger
"
```

### 7.6 Expected Output Example

```
/connectivity/summary output:
PING: OK (2/2 reachable, worst: internet @ 14.5ms)
SIGNAL: OK (1 active, WiFi, best: wlan0 @ -62dBm)
HEALTH: HEALTHY
TIME: 1784040522.550071744
```

### 7.7 Run Tests

```bash
docker exec --workdir="/workspace" ros2_agent_dock_1 bash -c "
  cd /workspace/Naval-Rex/ros2_ws
  colcon test --packages-select conectivity_check
  colcon test-result --verbose
"
```

### 7.7 Cleanup (MANDATORY for Agent Containers)

```bash
docker stop ros2_agent_dock_1
docker ps  # Verify no ros2_agent_dock_* remain
```

---

## 8. USING THIS IN PRODUCTION

### 8.1 Docker Compose for Deployment

```yaml
# docker-compose.yml
services:
  conectivity:
    build: .
    image: conectivity_check:latest
    network_mode: host
    ipc: host
    pid: host
    cap_add:
      - NET_RAW       # For ping_checker
      - NET_ADMIN     # For nl80211
    devices:
      - "/dev/ttyUSB0:/dev/ttyUSB0"   # Modem serial
      - "/dev/ttyUSB1:/dev/ttyUSB1"
      - "/dev/cdc-wdm0:/dev/cdc-wdm0" # QMI
    volumes:
      - "/run/dbus:/run/dbus"         # ModemManager
    group_add:
      - "dialout"                     # Serial access
    environment:
      - ROS_DOMAIN_ID=0
    command: >
      bash -c "source /workspace/install/setup.bash &&
               ros2 launch conectivity_check connectivity_stack.launch.py"
    restart: unless-stopped
```

### 8.2 Configuration for Production

Edit `config/connectivity.yaml`:
- Set real interface names (`wlan0`, `wwan0`, `eth0`)
- Set real target IPs for ping
- Adjust `update_rate_hz` (1 Hz default, can go to 10 Hz)
- Disable unused nodes: `enabled: false`

### 8.3 Monitoring Integration

| Tool | How to Visualize |
|------|------------------|
| **PlotJuggler** | `ros2 run plotjuggler plotjuggler` → plot `rss_dbm` vs time |
| **Foxglove** | Connect via WebSocket, add Gauge panels for RSS |
| **RViz** | MarkerArray with colored spheres on map |
| **Grafana + ROS 2 Bridge** | Telegraf/Prometheus scrape custom metrics |

### 8.4 Threshold-Based Alerting (Concept)

```python
# Pseudocode for alerting node
def rss_callback(msg):
    if msg.rss_dbm < -100:
        trigger_alert("CRITICAL", f"{msg.interface} RSS {msg.rss_dbm} dBm")
    elif msg.rss_dbm < -85:
        trigger_alert("WARNING", f"{msg.interface} RSS {msg.rss_dbm} dBm")

def ping_callback(msg):
    if not msg.reachable:
        trigger_alert("CRITICAL", f"Ping {msg.label} ({msg.target}) unreachable")
    elif msg.rtt_ms > 100:
        trigger_alert("WARNING", f"Ping {msg.label} high RTT {msg.rtt_ms}ms")
```

---

## 9. COMMON PITFALLS & DEBUGGING

### 9.1 Build Failures

| Error | Cause | Fix |
|-------|-------|-----|
| `ament_target_dependencies: package 'conectivity_check' not found` | Used `${PROJECT_NAME}` in `ament_target_dependencies` | Use `rosidl_target_interfaces(target ${PROJECT_NAME} ...)` instead |
| `modemmanager.h not found` | Wrong include | Use `#include <ModemManager/ModemManager.h>` |
| `NL80211_ATTR_SIGNAL_MBM not found` | libnl version mismatch | Check `nl80211.h` for correct constant names |
| `RCLCPP_COMPONENTS_REGISTER_NODE` linker error | Macro inside namespace | Move `RCLCPP_COMPONENTS_REGISTER_NODE` **outside** namespace |

### 9.2 Runtime Issues

| Symptom | Cause | Fix |
|---------|-------|-----|
| `ping_checker` fails to start | Missing `CAP_NET_RAW` | Add `--cap-add=NET_RAW` to docker run |
| `rss_monitor` shows no WiFi data | Wrong interface name or method | Check `iw dev`; try `method: "iw"` fallback |
| Cellular shows `registered: false` | ModemManager not running / no DBus | `systemctl start ModemManager`; check `/run/dbus` mount |
| Speedtest times out | No internet / blocked | Check firewall; test `speedtest-cli` manually first |

### 9.3 Debugging Commands

```bash
# See all topics
ros2 topic list | grep connectivity

# Echo with rate limit
ros2 topic echo /connectivity/rss/wlan0 --once

# Check node info
ros2 node info /rss_monitor

# Check parameters
ros2 param list /rss_monitor
ros2 param get /rss_monitor config_file

# Introspect messages
ros2 interface show conectivity_check/msg/RssMeasurement

# Check capabilities
getpcaps $(pgrep -f ping_checker)

# Test raw ping
ping -c 3 8.8.8.8

# Test WiFi RSSI manually
iw dev wlan0 link
cat /proc/net/wireless

# Test ModemManager
mmcli -L
mmcli -m 0 --signal-get
```

---

## 10. CREATING YOUR OWN: TEMPLATE CHECKLIST

### 10.1 New Package Checklist

```bash
# 1. Create package
ros2 pkg create --build-type ament_cmake my_package
cd my_package

# 2. Add messages (if needed)
mkdir msg srv
# Write .msg/.srv files

# 3. Edit package.xml - add:
# <depend>rclcpp</depend>
# <depend>std_msgs</depend>
# <buildtool_depend>rosidl_default_generators</buildtool_depend>
# <exec_depend>rosidl_default_runtime</exec_depend>
# <member_of_group>rosidl_interface_packages</member_of_group>

# 4. Edit CMakeLists.txt - add:
# find_package(rosidl_default_generators REQUIRED)
# rosidl_generate_interfaces(${PROJECT_NAME} "msg/MyMsg.msg")

# 5. Create node structure:
mkdir -p include/my_package src/my_node
# Write header in include/
# Write cpp in src/my_node/
# Write main.cpp with component constructor

# 6. Edit src/my_node/CMakeLists.txt:
# find_package(rclcpp_components REQUIRED)
# add_executable(my_node main.cpp my_node.cpp)
# ament_target_dependencies(my_node rclcpp std_msgs)
# rosidl_target_interfaces(my_node ${PROJECT_NAME} "rosidl_typesupport_cpp")
# install(TARGETS my_node DESTINATION lib/${PROJECT_NAME})

# 7. Build & test
cd ~/ros2_ws
colcon build --packages-select my_package
source install/setup.bash
ros2 run my_package my_node
```

### 10.2 Architecture Decision Record (Template)

When starting a new project, document:

```
# ADR: [Title]
## Context
What problem are we solving?

## Decision
What architecture did we choose?

## Consequences
- Positive: ...
- Negative: ...
- Risks: ...

## Alternatives Considered
1. Option A - rejected because...
2. Option B - rejected because...
```

---

## 11. QUICK REFERENCE: FILE PURPOSE MAP

| Path | Purpose | Edit? |
|------|---------|-------|
| `src/conectivity_check/` | **Your source of truth** | YES |
| `src/conectivity_check/config/` | Runtime YAML config | YES |
| `src/conectivity_check/include/` | Public C++ API | YES |
| `src/conectivity_check/src/` | Private implementations | YES |
| `src/conectivity_check/msg/` | ROS message definitions | YES |
| `src/conectivity_check/srv/` | ROS service definitions | YES |
| `src/conectivity_check/launch/` | Launch files | YES |
| `src/conectivity_check/test/` | Unit tests | YES |
| `src/conectivity_check/CMakeLists.txt` | Root build config | YES |
| `src/conectivity_check/package.xml` | Package metadata | YES |
| `build/` | **Build artifacts** | NO |
| `install/` | **Installed artifacts** | NO |
| `log/` | **Build logs** | NO |
| `.claude/` | Agent config (gitignored) | Local only |
| `CLAUDE.md` | Agent memory (gitignored) | Local only |

---

## 12. FINAL WORDS

### You Now Understand:
1. **Workspace anatomy** — `src/` vs `build/` vs `install/`
2. **Package anatomy** — Where code, config, messages, launch live
3. **Architecture** — 4 independent nodes, polymorphic providers, YAML-driven
4. **Build system** — CMake hierarchy, `rosidl_target_interfaces`, component nodes
5. **Runtime** — Docker permissions, capabilities, DBus, serial devices
6. **Production** — docker-compose, monitoring, alerting
7. **Creation** — Step-by-step template for new packages

### Next Steps for Mastery:
1. **Add unit tests** for each provider
2. **Implement ModemManager provider** (currently uses fallback)
3. **Add RViz plugin** for visual RSS heatmap
4. **Create systemd service** for auto-start on boot
5. **Add hysteresis logic** to prevent flapping alerts

### The Golden Rule:
> **Everything in `src/` is yours. Everything in `build/` and `install/` belongs to the build system. Never edit generated files.**

---

*Document Version: 2026-07-14*  
*Project: conectivity_check (working, simplified, human-readable)*  
*Workspace: ~/Workspace/Naval-Rex/ros2_ws*  
*Author: HAL (Consultoria & Engenharia) — for learning purposes*