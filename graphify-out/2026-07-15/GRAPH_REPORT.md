# Graph Report - /home/psampaioc/Workspace/Naval-Rex/ros2_ws/src/conectivity_check  (2026-07-15)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 371 nodes · 434 edges · 28 communities (26 shown, 2 thin omitted)
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 2 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `e2d0387d`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- RssConfig
- RssProvider
- RssMonitorNode
- PingChecker::Impl
- WifiRssProviderNl80211
- PingConfig
- README_Gaphify.md
- SpeedtestServerNode
- RssMonitorNode
- ConnectivityMonitorNode
- EthernetRssProviderCarrier
- cellular_rss_provider.cpp
- TEST
- 4.1 Step-by-Step Creation Process
- 7. HOW TO BUILD, RUN & TEST
- ROS 2 Workspace & conectivity_check Mastery Guide
- PingCheckerNode
- 6. WHAT'S REQUIRED VS OPTIONAL
- 1. ROS 2 WORKSPACE FUNDAMENTALS
- 3. CONECTIVITY_CHECK PROJECT ARCHITECTURE
- 8. USING THIS IN PRODUCTION
- 12. FINAL WORDS
- 2. NAVAL-REX ROS2_WS DEEP DIVE
- 5. FOLDER LAYOUT PHILOSOPHY
- 9. COMMON PITFALLS & DEBUGGING
- unique_ptr
- vector

## God Nodes (most connected - your core abstractions)
1. `PingChecker::Impl` - 23 edges
2. `RssMonitorNode` - 18 edges
3. `RssMonitorNode` - 18 edges
4. `RssConfig` - 16 edges
5. `ROS 2 Workspace & conectivity_check Mastery Guide` - 15 edges
6. `RssProvider` - 15 edges
7. `WifiRssProviderNl80211` - 15 edges
8. `SpeedtestServerNode` - 14 edges
9. `ConnectivityMonitorNode` - 14 edges
10. `CellularRssProviderAt` - 12 edges

## Surprising Connections (you probably didn't know these)
- `CellularRssProviderAt::read_once()` --calls--> `send_at_command`  [INFERRED]
  src/rss_monitor/cellular_rss_provider.cpp → include/conectivity_check/cellular_rss_provider.hpp
- `CellularRssProviderAt::init()` --references--> `RssConfig`  [EXTRACTED]
  src/rss_monitor/cellular_rss_provider.cpp → include/conectivity_check/rss_provider.hpp
- `CellularRssProviderQmi::init()` --references--> `RssConfig`  [EXTRACTED]
  src/rss_monitor/cellular_rss_provider.cpp → include/conectivity_check/rss_provider.hpp
- `WifiRssProviderNl80211::station_handler()` --calls--> `parse_station_info`  [INFERRED]
  src/rss_monitor/wifi_rss_provider.cpp → include/conectivity_check/wifi_rss_provider.hpp
- `load_ping_config()` --references--> `PingConfig`  [EXTRACTED]
  src/ping_checker/ping_checker_main.cpp → include/conectivity_check/ping_checker.hpp

## Import Cycles
- None detected.

## Communities (28 total, 2 thin omitted)

### Community 0 - "RssConfig"
Cohesion: 0.08
Nodes (27): string, RssConfig, at_device, interface_name, label, method, modem_path, sysfs_path (+19 more)

### Community 1 - "RssProvider"
Cohesion: 0.08
Nodes (22): CellularRssProviderAt, at_device_, fd_, init, logger_, read_once, shutdown, CellularRssProviderQmi (+14 more)

### Community 2 - "RssMonitorNode"
Cohesion: 0.08
Nodes (22): RssMeasurement, Node, NodeOptions, SharedPtr, string, main(), RssMonitorNode, iface_ (+14 more)

### Community 3 - "PingChecker::Impl"
Cohesion: 0.09
Nodes (22): mt19937, PingConfig, PingResult, PingTarget, SharedPtr, string, PingChecker::Impl, config (+14 more)

### Community 4 - "WifiRssProviderNl80211"
Cohesion: 0.09
Nodes (22): Logger, string, WifiRssProviderIw, iface_, init, logger_, read_once, WifiRssProviderNl80211 (+14 more)

### Community 5 - "PingConfig"
Cohesion: 0.09
Nodes (24): Impl, string, unique_ptr, vector, PingChecker, impl_, init, shutdown (+16 more)

### Community 6 - "README_Gaphify.md"
Cohesion: 0.08
Nodes (24): Benchmarks, Built on graphify: Penpax, Common commands, Community and links, Development setup, Environment variables, Full command reference, Git workflow (+16 more)

### Community 7 - "SpeedtestServerNode"
Cohesion: 0.13
Nodes (13): Request, Response, shared_ptr, SpeedtestResult, Node, SharedPtr, string, SpeedtestServerNode (+5 more)

### Community 8 - "RssMonitorNode"
Cohesion: 0.12
Nodes (17): Node, SharedPtr, string, unique_ptr, vector, RssMonitorNode, create_providers, interfaces_ (+9 more)

### Community 9 - "ConnectivityMonitorNode"
Cohesion: 0.14
Nodes (12): ConnectivityMonitorNode, last_ping_, last_rss_, ping_sub_, rss_sub_, summary_pub_, timer_, Node (+4 more)

### Community 10 - "EthernetRssProviderCarrier"
Cohesion: 0.15
Nodes (12): EthernetRssProviderCarrier, iface_, init, logger_, read_once, GenericRssProviderSysfs, init, logger_ (+4 more)

### Community 11 - "cellular_rss_provider.cpp"
Cohesion: 0.18
Nodes (9): send_at_command, CellularRssProviderAt::init(), CellularRssProviderAt::read_once(), CellularRssProviderAt::send_at_command(), CellularRssProviderQmi::init(), CellularRssProviderQmi::read_once(), Logger, RssMeasurement (+1 more)

### Community 12 - "TEST"
Cohesion: 0.20
Nodes (8): CellularFields, DefaultConstruction, EthernetFields, GenericFields, PingResultTest, RssMeasurementTest, TEST(), WifiFields

### Community 13 - "4.1 Step-by-Step Creation Process"
Cohesion: 0.22
Nodes (9): 4.1 Step-by-Step Creation Process, 4. BUILDING FROM SCRATCH: DECISION LOG, Step 1: Create Workspace, Step 2: Create Package Skeleton, Step 3: Add Message Definitions, Step 4: Configure CMakeLists.txt for Messages, Step 5: Add Dependencies in package.xml, Step 6: Write Node Code with Component Support (+1 more)

### Community 14 - "7. HOW TO BUILD, RUN & TEST"
Cohesion: 0.22
Nodes (9): 7.1 Prerequisites (One-Time Setup), 7.2 Start Build Container (Agent Way - Headless), 7.3 Build Inside Container, 7.4 Source & Run, 7.5 Verify It Works (In Another Terminal or &), 7.6 Expected Output Example, 7.7 Cleanup (MANDATORY for Agent Containers), 7.7 Run Tests (+1 more)

### Community 15 - "ROS 2 Workspace & conectivity_check Mastery Guide"
Cohesion: 0.25
Nodes (7): 10.1 New Package Checklist, 10.2 Architecture Decision Record (Template), 10. CREATING YOUR OWN: TEMPLATE CHECKLIST, 11. QUICK REFERENCE: FILE PURPOSE MAP, Complete Educational Reference from Beginner to Advanced, ROS 2 Workspace & conectivity_check Mastery Guide, 📚 TABLE OF CONTENTS

### Community 16 - "PingCheckerNode"
Cohesion: 0.29
Nodes (6): PingChecker, Node, NodeOptions, PingCheckerNode, checker_, unique_ptr

### Community 17 - "6. WHAT'S REQUIRED VS OPTIONAL"
Cohesion: 0.33
Nodes (6): 6.1 Required for ANY ROS 2 C++ Package, 6.2 Required for Custom Messages, 6.3 Required for This Project Specifically, 6.4 Optional But Recommended, 6.5 What You Can Delete Safely, 6. WHAT'S REQUIRED VS OPTIONAL

### Community 18 - "1. ROS 2 WORKSPACE FUNDAMENTALS"
Cohesion: 0.40
Nodes (5): 1.1 What is a ROS 2 Workspace?, 1.2 The Three Magic Directories, 1.3 The Build Flow, 1.4 Why You See "Duplicate" Names, 1. ROS 2 WORKSPACE FUNDAMENTALS

### Community 19 - "3. CONECTIVITY_CHECK PROJECT ARCHITECTURE"
Cohesion: 0.40
Nodes (5): 3.1 High-Level Design: 4 Independent Nodes, 3.2 Message Definitions (The Contracts), 3.3 Polymorphic RSS Provider Factory (The Crown Jewel), 3.4 Configuration-Driven (`config/connectivity.yaml`), 3. CONECTIVITY_CHECK PROJECT ARCHITECTURE

### Community 20 - "8. USING THIS IN PRODUCTION"
Cohesion: 0.40
Nodes (5): 8.1 Docker Compose for Deployment, 8.2 Configuration for Production, 8.3 Monitoring Integration, 8.4 Threshold-Based Alerting (Concept), 8. USING THIS IN PRODUCTION

### Community 21 - "12. FINAL WORDS"
Cohesion: 0.50
Nodes (4): 12. FINAL WORDS, Next Steps for Mastery:, The Golden Rule:, You Now Understand:

### Community 22 - "2. NAVAL-REX ROS2_WS DEEP DIVE"
Cohesion: 0.50
Nodes (4): 2.1 Actual Directory Tree (What You Have), 2.2 Two Packages, Different Purposes, 2.3 What's in `build/` and `install/` (For Curiosity), 2. NAVAL-REX ROS2_WS DEEP DIVE

### Community 23 - "5. FOLDER LAYOUT PHILOSOPHY"
Cohesion: 0.50
Nodes (4): 5.1 Why This Structure?, 5.2 Design Principles, 5.3 CMakeLists.txt Hierarchy, 5. FOLDER LAYOUT PHILOSOPHY

### Community 24 - "9. COMMON PITFALLS & DEBUGGING"
Cohesion: 0.50
Nodes (4): 9.1 Build Failures, 9.2 Runtime Issues, 9.3 Debugging Commands, 9. COMMON PITFALLS & DEBUGGING

## Knowledge Gaps
- **176 isolated node(s):** `Complete Educational Reference from Beginner to Advanced`, `📚 TABLE OF CONTENTS`, `1.1 What is a ROS 2 Workspace?`, `1.2 The Three Magic Directories`, `1.3 The Build Flow` (+171 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `RssProvider` connect `RssProvider` to `RssMonitorNode`, `EthernetRssProviderCarrier`, `WifiRssProviderNl80211`?**
  _High betweenness centrality (0.088) - this node is a cross-community bridge._
- **Why does `RssConfig` connect `RssConfig` to `RssProvider`, `cellular_rss_provider.cpp`?**
  _High betweenness centrality (0.067) - this node is a cross-community bridge._
- **What connects `Complete Educational Reference from Beginner to Advanced`, `📚 TABLE OF CONTENTS`, `1.1 What is a ROS 2 Workspace?` to the rest of the system?**
  _176 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `RssConfig` be split into smaller, more focused modules?**
  _Cohesion score 0.08064516129032258 - nodes in this community are weakly interconnected._
- **Should `RssProvider` be split into smaller, more focused modules?**
  _Cohesion score 0.08172043010752689 - nodes in this community are weakly interconnected._
- **Should `RssMonitorNode` be split into smaller, more focused modules?**
  _Cohesion score 0.08387096774193549 - nodes in this community are weakly interconnected._
- **Should `PingChecker::Impl` be split into smaller, more focused modules?**
  _Cohesion score 0.08505747126436781 - nodes in this community are weakly interconnected._