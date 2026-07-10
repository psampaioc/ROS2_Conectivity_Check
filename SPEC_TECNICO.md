# Spec Técnico: Sistema de Verificação de Conectividade + RSS ROS 2 (conectivity_check)

**Projeto:** Naval-Rex — Edge Computing UAV/UGV  
**Workspace:** `/home/psampaioc/Workspace/Naval-Rex/ros2_ws/src/conectivity_check`  
**ROS 2 Distro:** Jazzy (Ubuntu 24.04)  
**Build System:** ament_cmake (C++)  
**Linguagem:** C++17  
**Autor:** HAL (Consultoria & Engenharia)  
**Data:** 2026-07-09  
**Status:** **DRAFT — Aguardando Aprovação**

---

## 1. Visão Geral

### 1.1 Objetivo
Sistema ROS 2 em **C++** que monitora **duas dimensões complementares**:

| Dimensão | O que mede | Método | Frequência |
|----------|------------|--------|------------|
| **Conectividade (L3/L4)** | Ping ICMP: RTT, jitter, packet loss | `icmp` socket (CAP_NET_RAW) | 1 Hz (config) |
| **Qualidade do Link Físico (L1/L2)** | **RSS (Received Signal Strength) em dBm** + métricas associadas | Wi-Fi: `nl80211`/`iw` · Celular: ModemManager/AT · Genérico: sysfs | 1 Hz (config) |

> **Por que os dois?** Ping diz "consegue chegar?". RSS diz "quão bom é o sinal de rádio?". Juntos dão visão completa: link fraco mas ping OK = instabilidade futura; ping ruim mas RSS forte = congestionamento/roteamento.

### 1.2 Arquitetura de Nós (Modular, C++)

```
conectivity_check (pacote)
├── connectivity_monitor      # Nó principal: orquestra, agrega, publica summary
├── ping_checker              # Nó/Componente: pings periódicos multi-target
├── rss_monitor               # Nó/Componente: RSS por interface (Wi-Fi/Cellular/Genérico)
└── speedtest_server          # Nó: service/action para speedtest sob demanda
```

**Design:** 4 nós independentes (launch juntos ou separados). Comunicação via tópicos ROS 2. Configuração **única YAML** compartilhada.

---

## 2. Tópicos Publicados (Interfaces ROS 2)

### 2.1 Conectividade (Ping)

| Tópico | Tipo | Descrição |
|--------|------|-----------|
| `/connectivity/ping/<label>` | `conectivity_check/msg/PingResult` | Ping para cada target (edge, router, internet) |
| `/connectivity/ping/summary` | `conectivity_check/msg/PingSummary` | Agregado: reachable count, all_reachable, pior RTT |

### 2.2 RSS / Qualidade de Sinal (O que você pediu)

| Tópico | Tipo | Descrição |
|--------|------|-----------|
| `/connectivity/rss/<iface>` | `conectivity_check/msg/RssMeasurement` | **RSS absoluto em dBm** + métricas por interface |
| `/connectivity/rss/summary` | `conectivity_check/msg/RssSummary` | Melhor/pior RSS, contagem interfaces ativas |

> **Exemplo de tópicos reais:**
> - `/connectivity/rss/wlan0` → RSS Wi-Fi (-65 dBm, SNR 25 dB, noise -90 dBm)
> - `/connectivity/rss/wwan0` → RSS Celular (RSRP -95 dBm, RSRQ -12 dB, SINR 15 dB)
> - `/connectivity/rss/radio0` → RSS Rádio genérico (-72 dBm, driver-specific)

### 2.3 Speedtest (Sob Demanda)

| Interface | Tipo | Descrição |
|-----------|------|-----------|
| `/connectivity/speedtest` | Service `Trigger` → `SpeedtestResult` | Executa speedtest-cli/iperf3, retorna JSON no response |

---

## 3. Mensagens ROS 2 (`.msg`)

### 3.1 Ping (Conectividade L3)
```msg
# msg/PingResult.msg
builtin_interfaces/Time stamp
string target              # IP/hostname testado
string label               # "edge", "router", "internet", "custom"
float64 rtt_ms             # RTT médio (ms)
float64 jitter_ms          # Jitter (ms)
float64 packet_loss_pct    # Packet loss (%)
bool reachable             # True se respondeu
string error               # Vazio se OK
```

```msg
# msg/PingSummary.msg
builtin_interfaces/Time stamp
PingResult[] results       # Array com todos targets
bool all_reachable
int32 reachable_count
float64 worst_rtt_ms
string worst_label
```

### 3.2 RSS / Sinal (Qualidade Física L1/L2) — **O Core do Pedido**
```msg
# msg/RssMeasurement.msg — UMA medição por interface
builtin_interfaces/Time stamp
string interface           # Nome da interface: "wlan0", "wwan0", "radio0", "eth0"
string type                # "wifi", "cellular", "generic", "ethernet"
string label               # Label amigável do YAML: "main_wifi", "4g_modem", "telemetry_radio"

# === RSS ABSOLUTO (dBm) — Medida principal ===
float64 rss_dbm            # **Received Signal Strength em dBm (valor absoluto, ex: -65.3)**
                           # ESTE É O "RSS" QUE VOCÊ PEDIU: potência absoluta do sinal

# === Métricas complementares por tipo ===
# --- Wi-Fi (nl80211/iw) ---
float64 noise_dbm          # Noise floor (dBm)
float64 snr_db             # Signal-to-Noise Ratio = rss_dbm - noise_dbm (dB)
float64 link_quality       # 0-100% (driver-dependent)
float64 bitrate_mbps       # TX bitrate atual (Mbps)
string ssid                # SSID conectado
string bssid               # BSSID (MAC do AP)
uint32 frequency_mhz       # Canal (MHz)
string security            # WPA2, WPA3, open, etc.

# --- Celular (ModemManager/AT/QMI) ---
float64 rsrp_dbm           # Reference Signal Received Power (dBm) — LTE/5G
float64 rsrq_db            # Reference Signal Received Quality (dB)
float64 sinr_db            # Signal-to-Interference+Noise Ratio (dB)
int32 cqi                  # Channel Quality Indicator (0-15 LTE, 0-30 NR)
string operator_name       # "Claro", "Vivo", "TIM", etc.
string technology          # "LTE", "NR5G", "LTE+NR"
string band                # "B3", "n78", etc.
uint32 pci                 # Physical Cell Identity
bool registered            # Registrado na rede?

# --- Genérico / Ethernet / Outro ---
float64 snr_db             # Se disponível (driver-specific)
string driver_info         # Info do driver (ex: "ath9k", "rtl8821cu", "cdc_ether")
bool link_up               # Carrier detect (L1 link)
```

```msg
# msg/RssSummary.msg — Visão rápida para dashboard
builtin_interfaces/Time stamp
RssMeasurement[] measurements
float64 best_rss_dbm       # Maior RSS (menos negativo)
string best_interface
float64 worst_rss_dbm      # Menor RSS (mais negativo)
string worst_interface
int32 active_interfaces    # Com link_up=true
bool any_cellular          # True se tem interface cellular ativa
bool any_wifi              # True se tem interface wifi ativa
```

### 3.3 Speedtest
```msg
# msg/SpeedtestResult.msg
builtin_interfaces/Time stamp
string server_name
string server_country
float64 latency_ms
float64 download_mbps
float64 upload_mbps
float64 jitter_ms
string client_ip
string isp
bool success
string error_message
```

---

## 4. Configuração — YAML Único ("Menu" Central)

### 4.1 `config/connectivity.yaml`
```yaml
# ============================================
# CONECTIVITY CHECK - CONFIGURAÇÃO CENTRAL
# Edite aqui. Comente (#) o que não usar.
# ============================================

# --- Global ---
global:
  update_rate_hz: 1.0              # Frequência de todos os monitores
  log_level: "INFO"
  log_throttle_sec: 30.0

# ============================================
# PING CHECKER (Conectividade L3/L4)
# ============================================
ping_checker:
  enabled: true
  ping_count: 3                    # Pacotes por ciclo
  timeout_ms: 1000                 # Timeout por pacote
  packet_size: 56                  # Bytes
  
  targets:                         # Lista de alvos para ping
    - label: "edge"
      host: "192.168.1.10"         # IP fixo Edge Station
      # host: "edge-station.local" # Ou mDNS
      enabled: true
      interface: ""                # Vazio = rota default; ou "wlan0", "eth0" para forçar
    
    - label: "router"
      host: "192.168.1.1"          # Gateway
      enabled: true
      interface: ""
    
    - label: "internet"
      host: "8.8.8.8"              # Google DNS
      # host: "1.1.1.1"            # Cloudflare
      enabled: true
      interface: ""
    
    # EXEMPLO: Target extra (radio, sensor, etc.)
    # - label: "radio_link"
    #   host: "192.168.10.1"
    #   enabled: false

# ============================================
# RSS MONITOR (Qualidade de Sinal L1/L2) — CORE
# ============================================
rss_monitor:
  enabled: true
  
  interfaces:                      # Interfaces para monitorar RSS
    # --- Wi-Fi ---
    - name: "wlan0"
      type: "wifi"
      label: "main_wifi"
      enabled: true
      method: "nl80211"            # "nl80211" (libnl), "iw" (subprocess), "proc" (/proc/net/wireless)
      # method: "iw"               # Mais simples, usa `iw dev wlan0 link`
      
    # --- Celular 4G/5G (ModemManager) ---
    - name: "wwan0"
      type: "cellular"
      label: "4g_modem"
      enabled: true
      method: "modemmanager"       # "modemmanager" (DBus), "at" (AT commands), "qmi" (qmicli)
      modem_path: "/org/freedesktop/ModemManager1/Modem/0"
      # method: "at"
      # at_device: "/dev/ttyUSB2"
      
    # --- Rádio Genérico / Outro (sysfs/driver-specific) ---
    # - name: "radio0"
    #   type: "generic"
    #   label: "telemetry_radio"
    #   enabled: false
    #   method: "sysfs"
    #   sysfs_path: "/sys/class/net/radio0/device/signal_strength"
    
    # --- Ethernet (apenas link_up, sem RSS real) ---
    - name: "eth0"
      type: "ethernet"
      label: "wired"
      enabled: true
      method: "carrier_only"       # Só reporta link_up=true/false

# ============================================
# SPEEDTEST SERVER (Sob Demanda)
# ============================================
speedtest_server:
  enabled: true
  provider: "speedtest-cli"        # "speedtest-cli" ou "iperf3"
  timeout_sec: 120
  # preferred_server_id: ""
```

---

## 5. Estrutura do Pacote (C++, ament_cmake)

```
conectivity_check/
├── CMakeLists.txt
├── package.xml
├── config/
│   └── connectivity.yaml
├── msg/
│   ├── PingResult.msg
│   ├── PingSummary.msg
│   ├── RssMeasurement.msg
│   ├── RssSummary.msg
│   └── SpeedtestResult.msg
├── srv/
│   └── TriggerSpeedtest.srv       # std_srvs/Trigger + custom response via msg
├── launch/
│   └── connectivity_stack.launch.py
├── src/
│   ├── connectivity_monitor/      # Nó principal (agregador)
│   │   ├── connectivity_monitor.cpp
│   │   └── CMakeLists.txt
│   ├── ping_checker/              # Nó ping
│   │   ├── ping_checker.cpp
│   │   ├── ping_checker.hpp
│   │   └── CMakeLists.txt
│   ├── rss_monitor/               # Nó RSS (core)
│   │   ├── rss_monitor.cpp
│   │   ├── rss_monitor.hpp
│   │   ├── rss_provider.hpp       # Interface abstrata
│   │   ├── wifi_rss_provider.cpp  # nl80211 / iw
│   │   ├── cellular_rss_provider.cpp # ModemManager / AT
│   │   ├── generic_rss_provider.cpp  # sysfs / driver
│   │   └── CMakeLists.txt
│   └── speedtest_server/          # Nó speedtest
│       ├── speedtest_server.cpp
│       └── CMakeLists.txt
├── include/
│   └── conectivity_check/         # Headers públicos
│       ├── ping_checker.hpp
│       ├── rss_monitor.hpp
│       └── types.hpp
├── test/
│   ├── test_ping_checker.cpp
│   ├── test_rss_providers.cpp
│   └── CMakeLists.txt
└── README.md
```

---

## 6. Implementação C++ — Pontos-Chave

### 6.1 Dependências (package.xml)
```xml
<build_depend>ament_cmake</build_depend>
<build_depend>rclcpp</build_depend>
<build_depend>std_msgs</build_depend>
<build_depend>std_srvs</build_depend>
<build_depend>builtin_interfaces</build_depend>
<build_depend>rosidl_default_generators</build_depend>
<build_depend>rosidl_interface_packages</build_depend>

<!-- Para RSS providers -->
<depend>libnl-3-dev</depend>           # nl80211 (Wi-Fi RSSI nativo C++)
<depend>libnl-genl-3-dev</depend>
<depend>modemmanager</depend>          # ModemManager (DBus)
<depend>libmm-glib-dev</depend>        # ModemManager GLib (opcional, para C++ nativo)

<!-- Runtime -->
<exec_depend>speedtest-cli</exec_depend>
<exec_depend>iproute2</exec_depend>
<exec_depend>iw</exec_depend>
<exec_depend>modemmanager</exec_depend>

<test_depend>ament_lint_auto</test_depend>
<test_depend>ament_lint_common</test_depend>
<test_depend>gtest</test_depend>
```

### 6.2 RSS Provider — Interface Abstrata (Polimorfismo)
```cpp
// include/conectivity_check/rss_provider.hpp
#pragma once
#include "conectivity_check/msg/rss_measurement.hpp"
#include <string>
#include <memory>

namespace conectivity_check
{

struct RssConfig {
    std::string interface_name;
    std::string type;          // "wifi", "cellular", "generic", "ethernet"
    std::string label;
    std::string method;        // "nl80211", "iw", "modemmanager", "at", "sysfs", "carrier_only"
    // Params extras por method
    std::string modem_path;    // Para ModemManager
    std::string at_device;     // Para AT commands
    std::string sysfs_path;    // Para sysfs
};

class RssProvider {
public:
    virtual ~RssProvider() = default;
    
    // Inicializa provider (abre sockets, conecta DBus, etc.)
    virtual bool init(const RssConfig& config, rclcpp::Logger logger) = 0;
    
    // Faz uma leitura única de RSS
    // Retorna false se erro (preenche msg.error ou deixa campos NaN)
    virtual bool read_once(conectivity_check::msg::RssMeasurement& msg) = 0;
    
    // Cleanup
    virtual void shutdown() = 0;
    
    // Factory
    static std::unique_ptr<RssProvider> create(const std::string& type, const std::string& method);
};

} // namespace conectivity_check
```

### 6.3 Wi-Fi RSS Provider (nl80211 nativo C++ — Performance)
```cpp
// src/rss_monitor/wifi_rss_provider.cpp
// Usa libnl (nl80211) para ler RSSI direto do kernel — sem subprocess, tempo real
#include "conectivity_check/rss_provider.hpp"
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

namespace conectivity_check
{

class WifiRssProviderNl80211 : public RssProvider {
    struct nl_sock* sock_ = nullptr;
    int nl80211_id_ = -1;
    std::string iface_;
    int ifindex_ = -1;
    rclcpp::Logger logger_;

public:
    bool init(const RssConfig& config, rclcpp::Logger logger) override {
        logger_ = logger;
        iface_ = config.interface_name;
        
        // Abre socket netlink
        sock_ = nl_socket_alloc();
        if (!sock_) return false;
        
        if (nl_connect(sock_, NETLINK_GENERIC) < 0) return false;
        if (genl_connect(sock_) < 0) return false;
        
        nl80211_id_ = genl_ctrl_resolve(sock_, "nl80211");
        if (nl80211_id_ < 0) return false;
        
        ifindex_ = nl_name2index(nl_socket_get_fd(sock_), iface_.c_str());
        return ifindex_ > 0;
    }
    
    bool read_once(msg::RssMeasurement& msg) override {
        msg.stamp = rclcpp::Clock().now();
        msg.interface = iface_;
        msg.type = "wifi";
        msg.label = "main_wifi";  // viria do config
        
        // Monta mensagem NL80211_CMD_GET_STATION para pegar signal/noise/tx_bitrate
        struct nl_msg* nlmsg = nlmsg_alloc();
        genlmsg_put(nlmsg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id_, 0,
                    NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
        nla_put_u32(nlmsg, NL80211_ATTR_IFINDEX, ifindex_);
        
        // Callback para parse da resposta
        struct CallbackData { msg::RssMeasurement* out; bool done = false; };
        CallbackData cbdata{&msg};
        
        nl_socket_modify_cb(sock_, NL_CB_VALID, NL_CB_CUSTOM, 
            [](struct nl_msg* msg, void* arg) {
                auto* data = static_cast<CallbackData*>(arg);
                auto nla = parse_nl80211_station(msg);  // Implementar parse
                data->out->rss_dbm = nla.signal_dbm;
                data->out->noise_dbm = nla.noise_dbm;
                data->out->snr_db = nla.signal_dbm - nla.noise_dbm;
                data->out->bitrate_mbps = nla.tx_bitrate_mbps;
                data->out->link_quality = nla.link_quality;
                data->out->ssid = nla.ssid;
                data->out->bssid = nla.bssid;
                data->out->frequency_mhz = nla.freq_mhz;
                data->done = true;
                return NL_SKIP;
            }, &cbdata);
        
        nl_send_auto(sock_, nlmsg);
        nl_recvmsgs_default(sock_);
        nlmsg_free(nlmsg);
        
        return cbdata.done;
    }
    
    void shutdown() override {
        if (sock_) { nl_socket_free(sock_); sock_ = nullptr; }
    }
};

} // namespace conectivity_check
```

> **Nota:** `libnl` é a forma nativa C++ de acessar `nl80211` — zero subprocess, latência mínima, ideal para real-time. Alternativa mais simples: `popen("iw dev wlan0 link", "r")` e parse de texto (mais lento, menos deps).

### 6.4 Cellular RSS Provider (ModemManager via libmm-glib ou DBus)
```cpp
// src/rss_monitor/cellular_rss_provider.cpp
// Usa ModemManager GLib API (C) ou sd-bus (systemd) para C++ nativo
#include "conectivity_check/rss_provider.hpp"
#include <modemmanager.h>

namespace conectivity_check
{

class CellularRssProviderModemManager : public RssProvider {
    MMModem* modem_ = nullptr;
    std::string modem_path_;
    rclcpp::Logger logger_;

public:
    bool init(const RssConfig& config, rclcpp::Logger logger) override {
        logger_ = logger;
        modem_path_ = config.modem_path;
        
        // Inicializa ModemManager client
        GError* error = nullptr;
        MMManager* manager = mm_manager_new(&error);
        if (!manager) return false;
        
        MMModem* modem = mm_manager_get_modem_by_path(manager, modem_path_.c_str(), &error);
        if (!modem) { g_object_unref(manager); return false; }
        
        modem_ = modem;
        g_object_unref(manager);
        return true;
    }
    
    bool read_once(msg::RssMeasurement& msg) override {
        msg.stamp = rclcpp::Clock().now();
        msg.interface = "wwan0";
        msg.type = "cellular";
        msg.label = "4g_modem";
        
        GError* error = nullptr;
        MMModemSignalQuality* quality = mm_modem_get_signal_quality(modem_, &error);
        if (!quality) return false;
        
        // Extrai métricas (GLib API)
        msg.rss_dbm = mm_modem_signal_quality_get_rssi(quality);      // dBm
        msg.rsrp_dbm = mm_modem_signal_quality_get_rsrp(quality);     // dBm (LTE/5G)
        msg.rsrq_db = mm_modem_signal_quality_get_rsrq(quality);      // dB
        msg.sinr_db = mm_modem_signal_quality_get_sinr(quality);      // dB
        
        // Info extra
        MMModem3gpp* modem3gpp = MM_MODEM_3GPP(modem_);
        if (modem3gpp) {
            msg.operator_name = mm_modem_3gpp_get_operator_name(modem3gpp);
            msg.technology = mm_modem_3gpp_get_access_technology_string(modem3gpp);
        }
        
        g_object_unref(quality);
        return true;
    }
    
    void shutdown() override {
        if (modem_) { g_object_unref(modem_); modem_ = nullptr; }
    }
};

} // namespace conectivity_check
```

### 6.5 Nó RSS Monitor (Orquestra Providers)
```cpp
// src/rss_monitor/rss_monitor.cpp
#include "rclcpp/rclcpp.hpp"
#include "conectivity_check/rss_provider.hpp"
#include "conectivity_check/msg/rss_measurement.hpp"
#include "conectivity_check/msg/rss_summary.hpp"
#include <memory>
#include <vector>
#include <map>

namespace conectivity_check
{

class RssMonitorNode : public rclcpp::Node {
public:
    RssMonitorNode() : Node("rss_monitor") {
        // 1. Carrega config do YAML (params)
        declare_parameter("interfaces", std::vector<rclcpp::ParameterValue>{});
        auto interfaces_param = get_parameter("interfaces").as_array();
        
        // 2. Cria providers para cada interface
        for (const auto& iface_param : interfaces_param) {
            auto iface = iface_param.as_map();
            if (!iface.at("enabled").as_bool()) continue;
            
            RssConfig config;
            config.interface_name = iface.at("name").as_string();
            config.type = iface.at("type").as_string();
            config.label = iface.at("label").as_string();
            config.method = iface.at("method").as_string();
            config.modem_path = iface.count("modem_path") ? iface.at("modem_path").as_string() : "";
            config.at_device = iface.count("at_device") ? iface.at("at_device").as_string() : "";
            config.sysfs_path = iface.count("sysfs_path") ? iface.at("sysfs_path").as_string() : "";
            
            auto provider = RssProvider::create(config.type, config.method);
            if (provider && provider->init(config, get_logger())) {
                providers_[config.interface_name] = std::move(provider);
                
                // Publisher por interface
                auto pub = create_publisher<msg::RssMeasurement>(
                    "/connectivity/rss/" + config.interface_name, 10);
                publishers_[config.interface_name] = pub;
            }
        }
        
        // Summary publisher
        summary_pub_ = create_publisher<msg::RssSummary>("/connectivity/rss/summary", 10);
        
        // Timer
        double rate = get_parameter("global.update_rate_hz").as_double();
        timer_ = create_wall_timer(
            std::chrono::milliseconds(static_cast<int>(1000.0 / rate)),
            std::bind(&RssMonitorNode::timer_callback, this));
    }

private:
    void timer_callback() {
        msg::RssSummary summary;
        summary.stamp = this->now();
        double best_rss = -200.0;
        double worst_rss = 0.0;
        int active = 0;
        
        for (auto& [iface, provider] : providers_) {
            msg::RssMeasurement meas;
            if (provider->read_once(meas)) {
                meas.label = iface;  // ou do config
                publishers_[iface]->publish(meas);
                summary.measurements.push_back(meas);
                
                if (meas.link_up || meas.rss_dbm > -200) {
                    active++;
                    if (meas.rss_dbm > best_rss) {
                        best_rss = meas.rss_dbm;
                        summary.best_interface = iface;
                    }
                    if (meas.rss_dbm < worst_rss || worst_rss == 0) {
                        worst_rss = meas.rss_dbm;
                        summary.worst_interface = iface;
                    }
                }
            }
        }
        
        summary.active_interfaces = active;
        summary.best_rss_dbm = best_rss;
        summary.worst_rss_dbm = worst_rss;
        summary_pub_->publish(summary);
    }
    
    std::map<std::string, std::unique_ptr<RssProvider>> providers_;
    std::map<std::string, rclcpp::Publisher<msg::RssMeasurement>::SharedPtr> publishers_;
    rclcpp::Publisher<msg::RssSummary>::SharedPtr summary_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
};

} // namespace conectivity_check
```

---

## 7. Apresentação dos Resultados (Como Você Vê os Dados)

### 7.1 No Terminal (ros2 topic echo)
```bash
# RSS Wi-Fi (wlan0)
$ ros2 topic echo /connectivity/rss/wlan0
stamp:
  sec: 1720545600
  nanosec: 123456789
interface: "wlan0"
type: "wifi"
label: "main_wifi"
rss_dbm: -62.5                    # <<< RSS ABSOLUTO EM dBm (O QUE VOCÊ PEDIU)
noise_dbm: -95.0
snr_db: 32.5                      # SNR = rss - noise
link_quality: 78.0                # 0-100%
bitrate_mbps: 433.0
ssid: "NavalRex_Edge"
bssid: "aa:bb:cc:dd:ee:ff"
frequency_mhz: 5180
security: "WPA2"

# RSS Celular (wwan0)
$ ros2 topic echo /connectivity/rss/wwan0
stamp: ...
interface: "wwan0"
type: "cellular"
label: "4g_modem"
rss_dbm: -89.0                    # <<< RSS ABSOLUTO (GERAL)
rsrp_dbm: -95.0                   # <<< RSRP (LTE/5G específico)
rsrq_db: -11.0
sinr_db: 18.0
operator_name: "Claro"
technology: "LTE"
band: "B3"
registered: True

# Summary (dashboard rápido)
$ ros2 topic echo /connectivity/rss/summary
stamp: ...
measurements: [wlan0_data, wwan0_data, eth0_data]
best_rss_dbm: -62.5
best_interface: "wlan0"
worst_rss_dbm: -89.0
worst_interface: "wwan0"
active_interfaces: 3
any_wifi: True
any_cellular: True
```

### 7.2 No Foxglove / RViz / PlotJuggler
- **PlotJuggler**: Plota `rss_dbm` vs tempo por interface → vê variações, quedas, handover
- **Foxglove**: Painel com gauges (medidores) para cada interface, colorido por thresholds
- **RViz**: MarkerArray com esferas coloridas (verde/amarelo/vermelho) no mapa mostrando qualidade de sinal

### 7.3 Thresholds Sugeridos (para alertas/colorização)
| RSS (dBm) | Qualidade | Cor | Ação |
|-----------|-----------|-----|------|
| > -50 | Excelente | 🟢 Verde | Normal |
| -50 a -70 | Bom | 🟢 Verde | Normal |
| -70 a -85 | Regular | 🟡 Amarelo | Atenção, planejar handover |
| -85 a -100 | Fraco | 🟠 Laranja | Degradado, reduzir taxa |
| < -100 | Crítico | 🔴 Vermelho | Link instável, fallback |

---

## 8. Launch File (Inicia Tudo Junto)

### 8.1 `launch/connectivity_stack.launch.py`
```python
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node, PushRosNamespace
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    config_arg = DeclareLaunchArgument(
        'config_file',
        default_value=PathJoinSubstitution([
            FindPackageShare('conectivity_check'),
            'config',
            'connectivity.yaml'
        ]),
        description='Path to connectivity.yaml'
    )

    namespace_arg = DeclareLaunchArgument(
        'namespace', default_value='', description='Namespace'
    )

    return LaunchDescription([
        config_arg,
        namespace_arg,
        
        GroupAction([
            PushRosNamespace(LaunchConfiguration('namespace')),
            
            # Nó principal (agregador opcional, ou pode remover se não precisar)
            Node(
                package='conectivity_check',
                executable='connectivity_monitor',
                name='connectivity_monitor',
                output='screen',
                parameters=[LaunchConfiguration('config_file')],
            ),
            
            # Ping Checker
            Node(
                package='conectivity_check',
                executable='ping_checker',
                name='ping_checker',
                output='screen',
                parameters=[LaunchConfiguration('config_file')],
            ),
            
            # RSS Monitor (CORE)
            Node(
                package='conectivity_check',
                executable='rss_monitor',
                name='rss_monitor',
                output='screen',
                parameters=[LaunchConfiguration('config_file')],
            ),
            
            # Speedtest Server
            Node(
                package='conectivity_check',
                executable='speedtest_server',
                name='speedtest_server',
                output='screen',
                parameters=[LaunchConfiguration('config_file')],
            ),
        ]),
    ])
```

---

## 9. Testes e Validação

### 9.1 Build
```bash
# No container (rosstudy / rosstudyplus)
cd /workspace
colcon build --packages-select conectivity_check --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
```

### 9.2 Testes Unitários (GTest)
```bash
# Compila e roda testes
colcon test --packages-select conectivity_check
colcon test-result --verbose
```

### 9.3 Validação Manual (Checklist)
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

### 9.4 Permissões Docker (C++ nativo — sem CAP_NET_RAW para RSS)
```dockerfile
# Para ping_checker (ICMP raw socket):
cap_add:
  - CAP_NET_RAW

# Para rss_monitor (nl80211/ModemManager):
# - nl80211: CAP_NET_ADMIN (opcional, para alguns comandos)
# - ModemManager: acesso a /run/dbus + group dialout
devices:
  - "/dev/ttyUSB0:/dev/ttyUSB0"   # Modem AT
  - "/dev/ttyUSB1:/dev/ttyUSB1"
  - "/dev/ttyUSB2:/dev/ttyUSB2"
  - "/dev/cdc-wdm0:/dev/cdc-wdm0" # QMI
volumes:
  - "/run/dbus:/run/dbus"
group_add:
  - "dialout"
```

---

## 10. Próximos Passos (Após Aprovação)

| Etapa | Arquivos | Tempo |
|-------|----------|-------|
| 1 | `CMakeLists.txt`, `package.xml`, `config/connectivity.yaml` | 30 min |
| 2 | 5 `.msg` em `msg/` + `rosidl_generate_interfaces` | 20 min |
| 3 | `rss_provider.hpp` + 3 providers (Wi-Fi nl80211, Cellular MM, Generic) | 2h |
| 4 | `rss_monitor.cpp` (nó) | 45 min |
| 5 | `ping_checker.cpp` (nó, ICMP socket) | 45 min |
| 6 | `speedtest_server.cpp` (nó, subprocess speedtest-cli) | 30 min |
| 7 | `connectivity_monitor.cpp` (agregador opcional) | 30 min |
| 8 | `launch/connectivity_stack.launch.py` | 15 min |
| 9 | Testes GTest + build + validação no container | 1h |
| **Total** | **~6-7 horas** | |

---

## 11. Aprovação

> **Revise o spec.**  
> Responda **"APROVADO"** ou aponte ajustes.

**Checklist Crítico:**
- [ ] **RSS absoluto em dBm** (`rss_dbm` no `RssMeasurement`) atende sua necessidade?
- [ ] **C++ com libnl (nl80211) + ModemManager GLib** para performance real-time OK?
- [ ] **4 nós separados** (ping, rss, speedtest, monitor) fazem sentido? Ou prefere 1 nó com componentes?
- [ ] **YAML único** com `interfaces[]` cobrindo Wi-Fi, Cellular, Generic, Ethernet?
- [ ] **Providers polimórficos** (fácil adicionar novo tipo: LoRa, sat, etc.)?
- [ ] **Apresentação clara**: tópicos por interface + summary + thresholds sugeridos?
- [ ] **Docker permissions** viáveis no seu `rosstudy_env:jazzy`?

---

**Fim do Spec C++ com RSS Real**  
*HAL — Naval-Rex Edge Computing*  
*2026-07-09*