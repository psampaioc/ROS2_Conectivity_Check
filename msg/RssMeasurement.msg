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
float64 snr_db_generic       # Se disponível (driver-specific)
string driver_info           # Info do driver (ex: "ath9k", "rtl8821cu", "cdc_ether")
bool link_up                 # Carrier detect (L1 link)