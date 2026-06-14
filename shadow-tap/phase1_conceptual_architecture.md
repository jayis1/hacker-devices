# ShadowTap — Phase 1: Conceptual Architecture

## 1. System Purpose

ShadowTap is a **PoE-powered stealth network tap and MITM implant** designed for authorized red-team engagements. It is a palm-sized device that inserts inline between an Ethernet switch and a target host, transparently bridging traffic while enabling selective packet injection, modification, and capture — all powered parasitically from the same PoE link it taps.

### Primary Use Cases
- **Transparent network tap**: Passively bridge two Gigabit Ethernet links with zero packet loss and sub-microsecond latency
- **Active MITM**: Selectively intercept, modify, or inject frames at Layer 2–4 (ARP spoofing, DNS hijacking, DHCP injection, TLS downgrade attempts)
- **PoE passthrough**: Draw operating power from 802.3af/at PoE while delivering remaining power to the downstream device
- **Covert exfiltration**: Encapsulate captured traffic over an encrypted BLE back-channel to the operator's phone/laptop
- **On-device PCAP**: Store captured packets to microSD for later retrieval

## 2. Attack Surface

| Attack Vector | Capability | Protocol Layer |
|---|---|---|
| Ethernet inline tap | Passive L2 frame mirroring | L1–L2 |
| ARP cache poisoning | Redirect target gateway | L2–L3 |
| DNS spoofing | Resolve domains to attacker IP | L3–L4 (UDP) |
| DHCP rogue server | Assign attacker DNS/gateway | L3–L4 (UDP) |
| Frame injection | Craft and inject arbitrary frames | L2–L4 |
| TLS SNI filtering | Identify HTTPS targets by SNI | L4–L6 |
| ICMP tunneling | Exfiltrate data inside ICMP payload | L3 |
| BLE C2 channel | Encrypted command/control from phone | L2 (BLE) |

## 3. Performance Targets

| Metric | Target | Notes |
|---|---|---|
| Line-rate forwarding | 1 Gbps full-duplex | Zero packet loss on both ports |
| Tap latency overhead | < 500 ns added | Per-frame bridging delay |
| MITM modification latency | < 5 µs | For modified/injected frames |
| PoE power budget | 13 W max (802.3af Class 3) | Including BLE radio |
| BLE throughput | 250 kbps (encrypted C2) | AES-256-CTR over BLE SMP |
| PCAP capture rate | Sustained 500 Mbps to microSD | Via DMA |
| Boot time | < 2 s to operational | From PoE power-on |
| Form factor | 60 mm × 40 mm × 15 mm | Fits inline in ethernet run |

## 4. Constraints

1. **Power**: Must operate within 802.3af PoE (12.95 W available after loss). Cannot use 802.3at-only (25.5 W) as sole source — must degrade gracefully.
2. **Latency**: Inline insertion must not trigger carrier-grade timeouts. Forwarding must be cut-through capable.
3. **Transparency**: Must not advertise its MAC on the tapped segment unless explicitly commanded. Bridge operates in promiscuous, non-learning mode by default.
4. **Cost**: Total BOM under $100 at quantity 100.
5. **Detectability**: Must not respond to LLDP, CDP, or NDP unless configured. No link-flap on insertion.
6. **Regulatory**: BLE radio must use certified module (FCC pre-certified).

## 5. Block Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                          ShadowTap                                │
│                                                                    │
│  ┌──────────┐    ┌─────────────────────────┐    ┌──────────┐      │
│  │ RJ45 #1  │───▶│                         │───▶│ RJ45 #2  │      │
│  │ (Uplink) │◀──│   Ethernet Switch Fabric │◀──│ (Target) │      │
│  │ MagJack  │   │                         │    │ MagJack  │      │
│  └────┬─────┘   │  ┌───────┐  ┌────────┐  │    └──────────┘      │
│       │         │  │ PHY #1│  │ PHY #2 │  │                       │
│       │         │  │88E1510│  │88E1510 │  │                       │
│       │         │  └───┬───┘  └───┬────┘  │                       │
│       │         │      │ RGMII     │ RGMII │                       │
│       │         │  ┌───▼───────────▼────┐  │                       │
│       │         │  │                    │  │                       │
│       │         │  │   i.MX RT1062      │  │                       │
│       │         │  │   (Cortex-M7)      │  │                       │
│       │         │  │   600 MHz          │  │                       │
│       │         │  │                    │  │                       │
│       │         │  │ ┌────────────────┐ │  │                       │
│       │         │  │ │ Frame Matcher  │ │  │                       │
│       │         │  │ │ (CAM-like)     │ │  │                       │
│       │         │  │ └────────────────┘ │  │                       │
│       │         │  │ ┌────────────────┐ │  │                       │
│       │         │  │ │ MITM Engine    │ │  │                       │
│       │         │  │ │ (modify/inject)│ │  │                       │
│       │         │  │ └────────────────┘ │  │                       │
│       │         │  └─────────┬───────────┘  │                       │
│       │         │            │              │                       │
│       │         │      ┌────┼────┐         │                       │
│       │         │      │    │    │         │                       │
│       │         │   ┌──▼─┐┌▼──┐┌▼───┐      │                       │
│       │         │   │SDIO││SPI││UART│      │                       │
│       │         │   │    ││   ││    │      │                       │
│       │         │   └──┬─┘└─┬─┘└─┬──┘      │                       │
│       │         └──────┼─────┼─────┼────────┘                       │
│       │                │     │     │                                 │
│  ┌────▼────┐    ┌─────▼┐ ┌──▼───┐ ┌▼───────┐    ┌──────────────┐   │
│  │ PoE PD  │    │micro │ │NOR   │ │ nRF52840│    │ Status LEDs  │   │
│  │ TPS2378 │    │SD    │ │Flash │ │ BLE M.2 │    │ (4× RGB)     │   │
│  │ + DC-DC │    │Slot  │ │16 MB │ │ Module  │    │              │   │
│  └─────────┘    └──────┘ └──────┘ └─────────┘    └──────────────┘   │
│                                                                    │
└──────────────────────────────────────────────────────────────────┘
```

## 6. Data Flow

### 6.1 Transparent Tap Mode (default)
```
Uplink ──▶ PHY1 ──▶ RGMII RX ──▶ ENET1 DMA ──▶ Frame Buffer ──▶ ENET2 DMA ──▶ RGMII TX ──▶ PHY2 ──▶ Target
                                          │
                                          ├──▶ Matcher (match? → MITM Engine)
                                          │
                                          └──▶ PCAP Writer ──▶ microSD / BLE
```

1. Frames arrive on either port via the PHY's RGMII interface
2. DMA engines move frames into SRAM buffers (zero-copy ring)
3. Cut-through: first 64 bytes of frame forwarded before full frame received (fast-path)
4. Frame matcher inspects headers against programmed rules (MAC, IP, port, SNI)
5. Unmatched frames: forwarded unmodified to opposite port
6. Matched frames: diverted to MITM engine for modification or injection
7. Parallel copy to PCAP engine for on-device capture

### 6.2 MITM Active Mode
```
Frame in ──▶ Matcher ──▶ MITM Engine ──▶ Modified Frame ──▶ Inject ──▶ Out Port
                              │
                              ├──▶ ARP Poison: craft gratuitous ARP, inject
                              ├──▶ DNS Spoof: match DNS query, forge reply
                              ├──▶ DHCP Rogue: offer with attacker DNS
                              └──▶ Inject: arbitrary frame construction
```

### 6.3 BLE C2 Channel
```
Operator Phone ◀──▶ BLE SMP (encrypted) ◀──▶ nRF52840 ◀──▶ UART (115200) ◀──▶ i.MX RT1062
                                                                        │
                                                                        ├──▶ Rule updates
                                                                        ├──▶ PCAP stream
                                                                        └──▶ Status/commands
```

## 7. Bus Topology

| Bus | Master | Slaves | Speed | Purpose |
|---|---|---|---|---|
| RGMII #0 | i.MX RT1062 ENET1 | 88E1510 PHY #1 | 125 MHz clock | Uplink Ethernet |
| RGMII #1 | i.MX RT1062 ENET2 | 88E1510 PHY #2 | 125 MHz clock | Target Ethernet |
| SDIO | i.MX RT1062 uSDHC1 | microSD card | SDR50 (50 MHz) | PCAP storage |
| SPI #0 | i.MX RT1062 LPSPI1 | IS25LP016D NOR flash | 60 MHz | Boot / config |
| UART #1 | i.MX RT1062 LPUART1 | nRF52840 (M.2) | 115200 bps | BLE C2 |
| I²C #0 | i.MX RT1062 LPI2C1 | AT24C02 EEPROM | 400 kHz | Device ID / config |
| I²C #1 | i.MX RT1062 LPI2C2 | TPS2378 PoE ctrl | 100 kHz | PoE status |
| GPIO | i.MX RT1062 GPIO1/2 | LEDs, buttons, resets | — | Status / control |

## 8. Power Architecture

```
PoE Uplink ──▶ MagJack CT ──▶ TPS2378 (PD Controller) ──▶ TPS562201 (5V @ 3A) ──┬──▶ 3.3V (TLV62569, 3A) ──▶ SoC, PHYs, Flash, SD
                                                                                  └──▶ 1.2V (TLV62569, 2A) ──▶ i.MX RT core
                                                                                                              └──▶ 1.5V (LDO, PHY IO)
```

- Total available from 802.3af: ~12.95 W after cable loss
- SoC + PHYs: ~4.5 W typical, 6 W peak
- BLE module: ~0.3 W
- Remaining ~6 W available for PoE passthrough to target device (if Class 1–2)