# RSSI dBm Reference Cheat Sheet

## Quick Reference

| RSSI (dBm) | Quality | Color | Expected Behavior | Action |
|------------|---------|-------|-------------------|--------|
| **> -30** | **Excellent** | 🟢 | Maximum signal strength, right next to AP | Optimal |
| **-30 to -50** | **Excellent** | 🟢 | Very strong, same room as AP | Normal operation |
| **-50 to -60** | **Good** | 🟢 | Strong, 1-2 walls away | Normal operation |
| **-60 to -70** | **Fair** | 🟡 | Usable, several walls / distance | Monitor, may roam |
| **-70 to -80** | **Weak** | 🟠 | Marginal, frequent drops | Plan handover |
| **-80 to -90** | **Poor** | 🟠 | Unstable, high packet loss | Degraded, reduce rate |
| **-90 to -100** | **Critical** | 🔴 | Barely connected | Fallback / roam now |
| **< -100** | **No Link** | ⚫ | Disconnected / not associated | No connection |

---

## What You'll See in Output

```
RSS: -55dBm | PING: 192.168.1.1 (3.1ms avg) | T: 1784656834 (18:00:34)
RSS: -72dBm | PING: 192.168.1.1 (12.4ms avg, 33% loss) | T: 1784656835 (18:00:35)
RSS: N/A | PING: 192.168.1.1 (100% loss) | T: 1784656836 (18:00:36)
```

### Field Meanings

| Field | Meaning |
|-------|---------|
| `RSS: -55dBm` | WiFi signal strength (higher = better, -30 is max) |
| `RSS: N/A` | WiFi interface not associated / down |
| `PING: 3.1ms avg` | Average RTT over 3 pings |
| `PING: 12.4ms avg, 33% loss` | Average RTT + packet loss % |
| `PING: 100% loss` | All 3 pings failed |
| `T: 1784656834 (18:00:34)` | Unix timestamp + human time |

---

## Ping RTT Reference (Local LAN)

| RTT | Quality |
|-----|---------|
| < 1ms | Excellent (wired / very close WiFi) |
| 1-5ms | Good (typical WiFi LAN) |
| 5-20ms | Fair (distance / interference) |
| 20-50ms | Poor (congestion / weak signal) |
| > 50ms | Critical (likely packet loss) |
| timeout | No route / host down |

---

## Packet Loss Interpretation

| Loss % | Meaning |
|--------|---------|
| 0% | Perfect |
| 1-5% | Minor interference |
| 5-20% | Degraded, investigate |
| 20-50% | Severe, unreliable |
| 50-99% | Nearly unusable |
| 100% | No connectivity |

---

## Config Constants (in `src/rss_node.cpp`)

```cpp
static constexpr const char * WIFI_INTERFACE = "wlp3s0";  // Change to your interface
static constexpr const char * ROUTER_IP = "192.168.1.1";   // Change to your router IP
static constexpr double UPDATE_RATE_HZ = 1.0;              // Output rate
static constexpr int PING_COUNT_PER_CYCLE = 3;             // Pings per cycle for loss %
static constexpr int PING_TIMEOUT_MS = 1000;               // Per-ping timeout
static constexpr int PING_PACKET_SIZE = 56;                // ICMP payload size
```

---

## Quick Troubleshooting

| Symptom | Likely Cause |
|---------|--------------|
| `RSS: N/A` | WiFi down, wrong interface name, not associated |
| `RSS: -100` to `-200` | Very weak / disconnected |
| `PING: 100% loss` | Router IP wrong, no route, firewall, router down |
| High RTT + high loss | WiFi signal too weak (check RSS first) |
| Low RTT but high loss | Network congestion / duplex mismatch |