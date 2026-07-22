# CLAUDE.md — Project Reference: conectivity_check (KISS Minimal Architecture)

## Project Identity
**Package:** `conectivity_check` — ROS 2 (C++17, Jazzy) minimal connectivity + RSS monitoring for Naval-Rex Edge Computing UAV/UGV  
**Workspace:** `/home/psampaioc/Workspace/Naval-Rex/ros2_ws/src/conectivity_check`  
**Build:** `ament_cmake` | **Target:** `rosstudy_env:jazzy` Docker container

---

## Architecture (1 Minimal Node — KISS Reset 2026-07-22)

| Node | Purpose | Key Tech | Permissions |
|------|---------|----------|-------------|
| `rss_node` | **RSS + Ping combined** (single executable) | nl80211 netlink + raw ICMP sockets | `CAP_NET_ADMIN` + `CAP_NET_RAW` |

**Output:** Single-line human-readable: `RSS: -43dBm | PING: 192.168.10.1 (2.3ms avg) | T: 1784717292.8 (10:48:12)`

---

## Key Technical Decisions (Compact)

- **Core metric:** RSS absolute in dBm (`rss_dbm`) — not percentage, not quality index
- **Direct netlink:** Single `rss_node.cpp` with nl80211 (no provider factory)
- **Compile-time config:** Constants in `rss_node.hpp` (no YAML params for speed/simplicity)
- **MTU-sized pings:** 1472 byte payload (1500 - 20 IP - 8 ICMP) for path MTU testing
- **ICMP validation:** Strict echo reply validation (type=0, ID match, seq match, src IP match)
- **Docker:** Needs `--cap-add=NET_ADMIN --cap-add=NET_RAW --net=host`

---

## Build & Test Workflow (Inside Container)

```bash
cd /workspace/Naval-Rex/ros2_ws
colcon build --packages-select conectivity_check --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
colcon test --packages-select conectivity_check
colcon test-result --verbose
```

---

## Validation Checklist

| Component | Topic / Output | Expected |
|-----------|----------------|----------|
| **RSS WiFi** | Log output `RSS: -XXdBm` | `active_interfaces=1`, `best_rss_dbm` > -100 |
| **Ping Router** | Log output `PING: 192.168.10.1 (X.Xms avg)` | `reachable`, `rtt_ms` < 50 (local) |
| **Packet Loss** | Log output `, Y% loss` | 0% loss on healthy link |
| **MTU Test** | 1472 byte payload works | No fragmentation, valid RTT |

---

## Docker Rules (Agent-Owned Containers Only)

- **Naming:** `ros2_agent_dock_<N>` (dynamic, detached)
- **Startup:** `xhost +local:root` → `docker run -d --rm --name <NAME> --cap-add=NET_RAW --cap-add=NET_ADMIN --net=host --ipc=host --pid=host -v /tmp/.X11-unix:/tmp/.X11-unix -v /home/psampaioc/Workspace:/workspace -w /workspace/ros2_ws rosstudy_env:jazzy sleep infinity`
- **Exec:** `docker exec --workdir="/workspace/ros2_ws" <NAME> bash -c "<COMMANDS>"`
- **Teardown:** `docker stop <NAME>` + verify `docker ps` shows no `ros2_agent_dock_*`
- **Dockerfile changes ONLY** for new OS deps → `docker build -t rosstudy_env:jazzy .`
- **NEVER** touch user's `rosstudy` / `rosstudyplus` containers or aliases

---

## Dependencies

| Purpose | Library | Ubuntu Package |
|---------|---------|----------------|
| Wi-Fi RSSI (nl80211) | libnl | `libnl-3-dev`, `libnl-genl-3-dev` |
| YAML config | yaml-cpp | `libyaml-cpp-dev` |
| Speedtest | speedtest-cli | `speedtest-cli` |
| Ping ICMP | Linux kernel | `CAP_NET_RAW` capability |

---

## RSS Thresholds (Dashboards/Alerts)

| RSS (dBm) | Quality | Color | Action |
|-----------|---------|-------|--------|
| > -50 | Excellent | 🟢 | Normal |
| -50 to -70 | Good | 🟢 | Normal |
| -70 to -85 | Fair | 🟡 | Plan handover |
| -85 to -100 | Weak | 🟠 | Degraded, reduce rate |
| < -100 | Critical | 🔴 | Unstable, fallback |

---

## ⚠️ CRITICAL GOTCHAS (Must Internalize)

### 1. ROS 2 Same-Package Messages — NEVER Use `${PROJECT_NAME}` in `ament_target_dependencies`
```cmake
# ❌ WRONG — circular dependency / "package not found"
ament_target_dependencies(my_node rclcpp ${PROJECT_NAME})

# ✅ CORRECT
ament_target_dependencies(my_node rclcpp std_msgs builtin_interfaces)
rosidl_target_interfaces(my_node ${PROJECT_NAME} "rosidl_typesupport_cpp")
```
**Applies to ALL targets:** executables, components, tests.

### 2. Tool Usage — NEVER `Write` on Existing Files
- `Write` **completely overwrites** — use `Edit` (preferred), `sed`, or `Read`→`Edit` workflow
- **You truncated `wifi_rss_provider.cpp` by using `Write` instead of `Edit`.**

### 3. Host Isolation — NEVER Run System Commands on Host
- ❌ `dpkg -l`, `apt-cache search`, `rm -rf` on host
- ✅ All package checks, builds, tests inside container: `docker exec ros2_agent_dock_N bash -c "..."`
- New OS deps → modify `Dockerfile` → `docker build -t rosstudy_env:jazzy .`

### 4. ICMP Ping Validation — NEVER Trust Raw recvfrom()
- ❌ Accepting any ICMP packet as valid echo reply
- ✅ **Always validate:** ICMP type==ECHOREPLY (0), ID matches (getpid()), sequence matches, source IP matches destination
- **Root cause of 0.0ms bug (fixed 2026-07-22):** Kernel ICMP errors (Dest Unreachable, Time Exceeded) or stale packets returned immediately without validation
- **Fix in `rss_node.cpp:195-265` (commit 26c31ba):** Parse IP header → validate ICMP echo reply fields → only then compute RTT

---

## 🔧 CMake Gotchas (Reference)

- **`find_package` per subdirectory** — If target uses `rclcpp_components` (or any extra), subdir's `CMakeLists.txt` MUST call `find_package(rclcpp_components REQUIRED)` before `ament_target_dependencies`
- **Include paths don't propagate** — Root `include_directories()` does NOT reach `add_subdirectory` targets; each subdir needs `target_include_directories(target PRIVATE ${LIBNL_INCLUDE_DIRS} ${MODEM_MANAGER_INCLUDE_DIRS})`
- **Component registration** — `RCLCPP_COMPONENTS_REGISTER_NODE` MUST be outside namespace
- **Constructor requires `NodeOptions`** — `explicit MyNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions()) : Node("name", options) {}`

---

## Current Project State (2026-07-22)

### ✅ WORKING (KISS Minimal Architecture)
- **Build passes** — CMake generation + C++ compilation + message generation all succeed
- **Runtime stable** — No crashes (time source bug fixed, class_loader warning eliminated)
- **RSS + Ping combined** — Single `rss_node` executable does both (1Hz timer)
- **MTU-sized ping** — 1472 byte payload with **ICMP echo reply validation** (type=0, ID match, seq match, src IP match)
- **RSS monitor** — Direct nl80211 netlink on `wlp3s0`; graceful fallback if interface missing
- **Output** — Single-line log with RSS, ping RTT avg, packet loss %, timestamp
- **Config** — Compile-time constants in `rss_node.hpp` (no YAML params needed)

### 🐛 FIXED (2026-07-22): ICMP Ping 0.0ms Bug
- **Root cause:** Raw socket `recvfrom()` returned ANY ICMP packet (kernel errors, stale replies) without validation
- **Symptom:** MTU-sized packets reported 0.0ms RTT (impossible — kernel ICMP errors returned immediately)
- **Fix in `rss_node.cpp:195-265`:** Parse IP header → validate ICMP type==ECHOREPLY, ID matches, sequence matches → only then compute RTT
- **Verified:** Build passes, MTU packets now report realistic RTT (~2-5ms local router)

### 🔧 KISS SIMPLIFICATION (Intentional Minimalism)
- **Removed:** 4-node architecture → **1 node** (`rss_node`)
- **Removed:** Cellular provider complexity (ModemManager/AT/QMI) — not testable in container
- **Removed:** WiFi provider factory abstraction — single direct nl80211 implementation
- **Removed:** Generic sysfs provider complexity
- **Removed:** YAML config params — compile-time constants for simplicity
- **Kept:** Ethernet carrier (works everywhere), ping (works everywhere), RSS (nl80211)
- **Goal:** Zero-config "works on any robot" — just needs ethernet or wifi interface

### 📝 Config (Compile-Time Constants in `rss_node.hpp`)
```cpp
static constexpr double UPDATE_RATE_HZ = 1.0;
static constexpr const char * WIFI_INTERFACE = "wlp3s0";
static constexpr const char * ROUTER_IP = "192.168.10.1";
static constexpr int PING_TIMEOUT_MS = 1000;
static constexpr int PING_PACKET_SIZE = 1472;  // MTU-sized payload (1500 - 20 IP - 8 ICMP)
static constexpr int PING_COUNT_PER_CYCLE = 3;  // Multiple pings for packet loss %
```

---

## GitHub Repository (Canonical)
- **Owner:** `psampaioc` (personal namespace)
- **Repository:** `ROS2_Conectivity_Check`
- **URL:** `https://github.com/psampaioc/ROS2_Conectivity_Check`

---

## Skills Registered for This Project
- `audit_ros2_cmake` — Scan CMakeLists.txt for `${PROJECT_NAME}` anti-pattern
- `ros2-docker` / `ros2_docker_build` / `ros2_docker_test` — Container build/test
- `github-repo-manager` — Repo sanitize/publish
- `graphify` — Knowledge graph queries (workspace-wide)

---

## 📖 Comprehensive Learning Guide
For complete educational reference (workspace structure, architecture, build system, folder layout philosophy, creating similar projects): **See `MASTER_GUIDE.md`**

---

## 🐳 Docker/ROS 2 Workflow (From Memory)
**Location:** `~/.claude/projects/-home-psampaioc-Workspace-Naval-Rex-ros2-ws-src-conectivity-check/memory/docker-ros-workflow.md`

**Key insight:** User was confused thinking `colcon build` uses the Dockerfile directly. It doesn't — the Dockerfile builds the **base image** (`rosstudy_env:jazzy`), then both user's `rosstudy` alias and HAL's `ros2_agent_dock_*` containers run from that **same pre-built image** with the workspace mounted. `colcon build` runs inside the container using the image's toolchain, not the Dockerfile.
[[docker-rules]]
[[critical-gotchas]]