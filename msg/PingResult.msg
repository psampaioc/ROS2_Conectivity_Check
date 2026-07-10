builtin_interfaces/Time stamp
string target              # IP/hostname testado
string label               # "edge", "router", "internet", "custom"
float64 rtt_ms             # RTT médio (ms)
float64 jitter_ms          # Jitter (ms)
float64 packet_loss_pct    # Packet loss (%)
bool reachable             # True se respondeu
string error               # Vazio se OK