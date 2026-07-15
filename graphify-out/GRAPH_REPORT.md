# Graph Report - /home/psampaioc/Workspace/Naval-Rex/ros2_ws/src/conectivity_check  (2026-07-14)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 253 nodes · 315 edges · 15 communities
- Extraction: 99% EXTRACTED · 1% INFERRED · 0% AMBIGUOUS · INFERRED: 4 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `540259e0`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- WifiRssProviderNl80211
- PingChecker::Impl
- CellularRssProviderAt
- RssProvider
- RssMonitorNode
- RssConfig
- SpeedtestServerNode
- ConnectivityMonitorNode
- PingChecker
- cellular_rss_provider.cpp
- PingConfig
- TEST
- generic_rss_provider.cpp

## God Nodes (most connected - your core abstractions)
1. `PingChecker::Impl` - 23 edges
2. `RssMonitorNode` - 18 edges
3. `RssConfig` - 16 edges
4. `RssProvider` - 15 edges
5. `WifiRssProviderNl80211` - 15 edges
6. `SpeedtestServerNode` - 14 edges
7. `ConnectivityMonitorNode` - 13 edges
8. `CellularRssProviderAt` - 12 edges
9. `PingConfig` - 12 edges
10. `CellularRssProviderQmi` - 10 edges

## Surprising Connections (you probably didn't know these)
- `CellularRssProviderAt::read_once()` --calls--> `send_at_command`  [INFERRED]
  src/rss_monitor/cellular_rss_provider.cpp → include/conectivity_check/cellular_rss_provider.hpp
- `load_ping_config()` --references--> `PingConfig`  [EXTRACTED]
  src/ping_checker/ping_checker_main.cpp → include/conectivity_check/ping_checker.hpp
- `PingChecker::Impl` --references--> `PingConfig`  [EXTRACTED]
  src/ping_checker/ping_checker.cpp → include/conectivity_check/ping_checker.hpp
- `PingChecker::init()` --references--> `PingConfig`  [EXTRACTED]
  src/ping_checker/ping_checker.cpp → include/conectivity_check/ping_checker.hpp
- `RssMonitorNode::RssMonitorNode()` --calls--> `load_config`  [INFERRED]
  src/rss_monitor/rss_monitor.cpp → include/conectivity_check/rss_monitor.hpp

## Import Cycles
- None detected.

## Communities (15 total, 0 thin omitted)

### Community 0 - "WifiRssProviderNl80211"
Cohesion: 0.09
Nodes (22): Logger, string, WifiRssProviderIw, iface_, init, logger_, read_once, WifiRssProviderNl80211 (+14 more)

### Community 1 - "PingChecker::Impl"
Cohesion: 0.09
Nodes (20): mt19937, PingResult, SharedPtr, string, vector, PingChecker::Impl, config, last_log (+12 more)

### Community 2 - "CellularRssProviderAt"
Cohesion: 0.10
Nodes (19): CellularRssProviderAt, at_device_, fd_, init, logger_, read_once, shutdown, CellularRssProviderQmi (+11 more)

### Community 3 - "RssProvider"
Cohesion: 0.09
Nodes (19): EthernetRssProviderCarrier, iface_, init, logger_, read_once, GenericRssProviderSysfs, init, logger_ (+11 more)

### Community 4 - "RssMonitorNode"
Cohesion: 0.09
Nodes (20): Node, SharedPtr, string, unique_ptr, vector, RssMonitorNode, create_providers, interfaces_ (+12 more)

### Community 5 - "RssConfig"
Cohesion: 0.11
Nodes (21): string, RssConfig, at_device, interface_name, label, method, modem_path, sysfs_path (+13 more)

### Community 6 - "SpeedtestServerNode"
Cohesion: 0.13
Nodes (13): Request, Response, shared_ptr, SpeedtestResult, Node, SharedPtr, string, SpeedtestServerNode (+5 more)

### Community 7 - "ConnectivityMonitorNode"
Cohesion: 0.15
Nodes (11): ConnectivityMonitorNode, last_ping_, last_rss_, ping_sub_, rss_sub_, summary_pub_, timer_, Node (+3 more)

### Community 8 - "PingChecker"
Cohesion: 0.15
Nodes (12): Impl, unique_ptr, PingChecker, impl_, init, shutdown, spin_once, Node (+4 more)

### Community 9 - "cellular_rss_provider.cpp"
Cohesion: 0.18
Nodes (9): send_at_command, CellularRssProviderAt::init(), CellularRssProviderAt::read_once(), CellularRssProviderAt::send_at_command(), CellularRssProviderQmi::init(), CellularRssProviderQmi::read_once(), Logger, RssMeasurement (+1 more)

### Community 10 - "PingConfig"
Cohesion: 0.15
Nodes (13): string, PingConfig, log_throttle_sec, packet_size, ping_count, targets, timeout_ms, update_rate_hz (+5 more)

### Community 11 - "TEST"
Cohesion: 0.20
Nodes (8): CellularFields, DefaultConstruction, EthernetFields, GenericFields, PingResultTest, RssMeasurementTest, TEST(), WifiFields

### Community 12 - "generic_rss_provider.cpp"
Cohesion: 0.28
Nodes (6): Logger, RssMeasurement, EthernetRssProviderCarrier::init(), EthernetRssProviderCarrier::read_once(), GenericRssProviderSysfs::init(), GenericRssProviderSysfs::read_once()

## Knowledge Gaps
- **88 isolated node(s):** `init`, `read_once`, `shutdown`, `at_device_`, `fd_` (+83 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `RssProvider` connect `RssProvider` to `WifiRssProviderNl80211`, `CellularRssProviderAt`, `RssMonitorNode`?**
  _High betweenness centrality (0.227) - this node is a cross-community bridge._
- **Why does `PingConfig` connect `PingConfig` to `PingChecker::Impl`, `CellularRssProviderAt`?**
  _High betweenness centrality (0.181) - this node is a cross-community bridge._
- **Why does `RssConfig` connect `RssConfig` to `cellular_rss_provider.cpp`, `CellularRssProviderAt`, `generic_rss_provider.cpp`?**
  _High betweenness centrality (0.181) - this node is a cross-community bridge._
- **What connects `init`, `read_once`, `shutdown` to the rest of the system?**
  _88 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `WifiRssProviderNl80211` be split into smaller, more focused modules?**
  _Cohesion score 0.08994708994708994 - nodes in this community are weakly interconnected._
- **Should `PingChecker::Impl` be split into smaller, more focused modules?**
  _Cohesion score 0.08994708994708994 - nodes in this community are weakly interconnected._
- **Should `CellularRssProviderAt` be split into smaller, more focused modules?**
  _Cohesion score 0.09971509971509972 - nodes in this community are weakly interconnected._