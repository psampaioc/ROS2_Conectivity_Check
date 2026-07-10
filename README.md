# ROS2 Conectivity Check

Sistema ROS 2 (C++17, Jazzy) para monitoramento de **conectividade (L3/L4 via ICMP)** + **qualidade de sinal RSS (L1/L2: Wi-Fi, Celular, Genérico)** + **Speedtest sob demanda**.

## Arquitetura

```
conectivity_check/
├── ping_checker       # Nó: pings periódicos multi-target (precisa CAP_NET_RAW)
├── rss_monitor        # Nó: RSS por interface (Wi-Fi nl80211, Cellular ModemManager, Genérico sysfs)
├── connectivity_monitor # Nó: agregador opcional (summary unificado)
└── speedtest_server   # Nó: service/action para speedtest-cli/iperf3
```

## Tópicos ROS 2

| Tópico | Tipo | Descrição |
|--------|------|-----------|
| `/connectivity/ping/<label>` | `PingResult` | Ping para cada target (edge, router, internet) |
| `/connectivity/ping/summary` | `PingSummary` | Agregado: reachable count, worst RTT |
| `/connectivity/rss/<iface>` | `RssMeasurement` | **RSS absoluto em dBm** + métricas por interface |
| `/connectivity/rss/summary` | `RssSummary` | Melhor/pior RSS, contagem interfaces ativas |
| `/connectivity/summary` | `ConnectivitySummary` | Visão unificada (ping + RSS) |
| `/connectivity/speedtest` | `Trigger` → `SpeedtestResult` | Executa speedtest sob demanda |

### Exemplo de saída RSS Wi-Fi (`/connectivity/rss/wlan0`)
```yaml
stamp:
  sec: 1720545600
  nanosec: 123456789
interface: "wlan0"
type: "wifi"
label: "main_wifi"
rss_dbm: -62.5                    # <<< RSS ABSOLUTO EM dBm
noise_dbm: -95.0
snr_db: 32.5
link_quality: 78.0
bitrate_mbps: 433.0
ssid: "MyNetwork_Edge"
bssid: "aa:bb:cc:dd:ee:ff"
frequency_mhz: 5180
security: "WPA2"
```

### Exemplo de saída RSS Celular (`/connectivity/rss/wwan0`)
```yaml
interface: "wwan0"
type: "cellular"
label: "4g_modem"
rss_dbm: -89.0
rsrp_dbm: -95.0
rsrq_db: -11.0
sinr_db: 18.0
operator_name: "Claro"
technology: "LTE"
band: "B3"
registered: true
```

## Configuração (`config/connectivity.yaml`)

```yaml
global:
  update_rate_hz: 1.0

ping_checker:
  enabled: true
  targets:
    - label: "edge"
      host: "192.168.1.10"
    - label: "router"
      host: "192.168.1.1"
    - label: "internet"
      host: "8.8.8.8"

rss_monitor:
  enabled: true
  interfaces:
    - name: "wlan0"
      type: "wifi"
      label: "main_wifi"
      method: "nl80211"      # libnl nativo (performance)
    - name: "wwan0"
      type: "cellular"
      label: "4g_modem"
      method: "modemmanager" # ModemManager DBus
    - name: "eth0"
      type: "ethernet"
      label: "wired"
      method: "carrier_only"
```

## Build

```bash
# No container (rosstudy)
cd /workspace
colcon build --packages-select conectivity_check --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
```

## Permissões Docker

### ping_checker (ICMP raw socket)
```yaml
cap_add:
  - CAP_NET_RAW
```

### rss_monitor (Wi-Fi nl80211 / Cellular ModemManager)
```yaml
# Wi-Fi nl80211: precisa CAP_NET_ADMIN para alguns comandos
cap_add:
  - CAP_NET_ADMIN

# Cellular ModemManager: acesso D-Bus + device serial
devices:
  - "/dev/ttyUSB0:/dev/ttyUSB0"
  - "/dev/ttyUSB1:/dev/ttyUSB1"
  - "/dev/ttyUSB2:/dev/ttyUSB2"
  - "/dev/cdc-wdm0:/dev/cdc-wdm0"
volumes:
  - "/run/dbus:/run/dbus"
group_add:
  - "dialout"
```

## Executar

```bash
# Stack completa
ros2 launch conectivity_check connectivity_stack.launch.py

# Ou nós individuais
ros2 run conectivity_check rss_monitor
ros2 run conectivity_check ping_checker
```

## Validação Manual

| Teste | Comando | Esperado |
|-------|---------|----------|
| **RSS Wi-Fi** | `ros2 topic echo /connectivity/rss/wlan0` | `rss_dbm` ~ -40 a -80, `snr_db` > 10 |
| **RSS Celular** | `ros2 topic echo /connectivity/rss/wwan0` | `rsrp_dbm` ~ -80 a -110, `sinr_db` > 5 |
| **Ping Edge** | `ros2 topic echo /connectivity/ping/edge` | `reachable=true`, `rtt_ms` < 50 |
| **Ping Internet** | `ros2 topic echo /connectivity/ping/internet` | `reachable=true`, `rtt_ms` < 100 |
| **Summary RSS** | `ros2 topic echo /connectivity/rss/summary` | `best_interface`, `worst_interface` corretos |
| **Speedtest** | `ros2 service call /connectivity/speedtest std_srvs/srv/Trigger` | Retorna download/upload Mbps |
| **Desliga Wi-Fi** | `nmcli dev disconnect wlan0` | `rss_dbm` para de atualizar ou `link_up=false` |
| **Desconecta modem** | `mmcli -m 0 --disable` | `registered=false`, `rss_dbm` NaN |

## Thresholds Sugeridos (Dashboard/Alertas)

| RSS (dBm) | Qualidade | Cor | Ação |
|-----------|-----------|-----|------|
| > -50 | Excelente | 🟢 | Normal |
| -50 a -70 | Bom | 🟢 | Normal |
| -70 a -85 | Regular | 🟡 | Atenção, planejar handover |
| -85 a -100 | Fraco | 🟠 | Degradado, reduzir taxa |
| < -100 | Crítico | 🔴 | Link instável, fallback |

## Visualização

- **PlotJuggler**: Plota `rss_dbm` vs tempo por interface
- **Foxglove**: Gauges coloridos por threshold
- **RViz**: MarkerArray com esferas verde/amarelo/vermelho no mapa

## Dependências Principais

| Componente | Biblioteca | Pacote Ubuntu |
|------------|------------|---------------|
| Wi-Fi RSSI | libnl (nl80211) | `libnl-3-dev`, `libnl-genl-3-dev` |
| Celular RSS | ModemManager GLib | `libmm-glib-dev`, `modemmanager` |
| Config YAML | yaml-cpp | `libyaml-cpp-dev` |
| Speedtest | speedtest-cli | `speedtest-cli` |
| Ping | Linux raw socket | `CAP_NET_RAW` |

## Estrutura de Código

```
src/
├── connectivity_monitor/     # Agregador (opcional)
├── ping_checker/             # ICMP ping (precisa CAP_NET_RAW)
├── rss_monitor/              # Core RSS
│   ├── rss_provider.hpp/cpp  # Interface abstrata + factory
│   ├── wifi_rss_provider.*   # nl80211 / iw / proc
│   ├── cellular_rss_provider.* # ModemManager / AT / QMI
│   └── generic_rss_provider.* # sysfs / carrier
└── speedtest_server/         # speedtest-cli / iperf3
```

## Estender Providers

Para adicionar novo tipo (ex: LoRa, Satélite):

1. Crie `lora_rss_provider.hpp/cpp` herdando `RssProvider`
2. Implemente `init()`, `read_once()`, `shutdown()`
3. Registre no factory em `rss_provider.cpp`
4. Adicione no YAML: `type: "lora"`, `method: "custom"`

## Licença

Apache-2.0