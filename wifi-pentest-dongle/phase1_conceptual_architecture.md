# WFP-100 — Portable WiFi Pentesting Dongle
## Phase 1: Conceptual Architecture & Requirements

---

## 1.1 System Core Purpose

The **WFP-100** (WiFi Pentester, 1st Generation) is a pocket-sized USB-C dongle for 802.11ax wireless network security auditing. It provides raw 802.11 frame capture (monitor mode), frame injection, deauthentication, PMKID harvesting, and Evil Twin access-point emulation — all in a self-contained, battery-backed package that requires zero host drivers.

**Primary Function:** Capture, analyze, and inject 802.11 frames across 2.4 GHz, 5 GHz, and 6 GHz bands for authorized penetration testing, with onboard processing for packet filtering and WPA handshake extraction.

---

## 1.2 Target Performance Metrics

| Parameter | Target | Notes |
|---|---|---|
| **WiFi Standard** | 802.11ax (WiFi 6E) | 2.4/5/6 GHz tri-band |
| **Monitor Mode Throughput** | ≥500 Mbps capture | Raw radiotap frames to host |
| **Injection Rate** | ≥1500 frames/sec | Management + data frames |
| **Channel Hop Latency** | ≤5 ms | Full-band scan < 2 sec |
| **CPU Performance** | ≥1.5 GHz quad-core RISC-V | Packet forging + crypto |
| **Memory** | 2 GB LPDDR4X | Capture buffers + Scapy |
| **Storage** | 16 GB eMMC 5.1 | PCAP storage, wordlists |
| **Battery Life** | ≥4 hrs active capture | 1000 mAh LiPo |
| **USB Throughput** | ≥400 Mbps CDC-ECM | To host via USB-C |
| **Form Factor** | 85mm × 35mm × 12mm | USB-C stick, pocket-portable |
| **BOM Cost** | ≤$75 @ 1K units | Affordable pentesting tool |
| **Operating Temp** | 0°C to +60°C | Portable/consumer grade |

---

## 1.3 Design Constraints

| Constraint | Value | Rationale |
|---|---|---|
| **Thermal** | Passive only, ≤3W sustained TDP | Pocket form factor, no fan |
| **Power Supply** | USB-C 5V/1.5A or 3.7V LiPo | Host-powered or battery |
| **Host Interface** | USB-C 3.2 Gen1 | Single cable for data + power |
| **Regulatory** | FCC Part 15 (unlicensed), FCC §15.247 | Intentional radiator certification required |
| **Security** | Secure Boot (eFuse OTP), TPM over I2C | Prevent firmware tampering |
| **Antenna** | 2× external RP-SMA (2.4/5/6 GHz) | Detachable, directional antenna support |
| **Software** | Mainline Linux 6.6+, iwlwifi driver | No proprietary WiFi blobs required for monitor mode |

---

## 1.4 High-Level Block Diagram

```
┌──────────────────────────────────────────────────────────────────────┐
│                      WFP-100 SYSTEM ARCHITECTURE                     │
│                                                                      │
│  ┌──────────────┐     AXI4 Bus (128-bit)     ┌────────────────────┐ │
│  │  SoC:        │◄──────────────────────────►│  LPDDR4X 2GB      │ │
│  │  StarFive    │   12.8 GB/s @ 3200 MT/s   │  MT53E256M32D2DS  │ │
│  │  JH7110      │                             └────────────────────┘ │
│  │              │                                                    │
│  │  4× U74      │   PCIe Gen3 x1              ┌────────────────────┐ │
│  │  1× S7 (L2)  │◄──────────────────────────►│  WiFi: AX210      │ │
│  │  @ 1.5 GHz   │   8 GT/s                    │  Intel AX210NGW   │ │
│  │              │                              │  802.11ax 2×2     │ │
│  │  ┌────────┐ │                              │  M.2 2230 E-key   │ │
│  │  │GPU:    │ │                              └───────┬────────────┘ │
│  │  │IMG     │ │                                     │               │
│  │  │BXE-2-32│ │                              ┌──────┴──────┐        │
│  │  └────────┘ │                              │ 2× RP-SMA  │        │
│  └──────┬───────┘                              │ Antennas   │        │
│         │                                       └─────────────┘       │
│  ┌──────┴────────────────────────────────────────────────────────┐  │
│  │                   PERIPHERAL SUBSYSTEM                         │  │
│  │  ┌──────────┐  ┌────────┐  ┌───────┐  ┌──────┐  ┌────────┐ │  │
│  │  │ USB 3.2  │  │ eMMC   │  │ SPI   │  │ I2C  │  │ UART   │ │  │
│  │  │ CDC-ECM  │  │ 16GB   │  │ (Flash│  │ (RTC,│  │ (Debug│ │  │
│  │  │ CDC-ACM  │  │ 5.1    │  │  TPM) │  │  TPM)│  │  Cons) │ │  │
│  │  └──────────┘  └────────┘  └───────┘  └──────┘  └────────┘ │  │
│  │  ┌──────────┐  ┌────────────────────────────────────────┐    │  │
│  │  │ microSD  │  │ GPIO (LEDs, Button, WiFi Kill, RST)   │    │  │
│  │  │ (SDIO)  │  │  3× LED, 1× Button, 1× WiFi Kill      │    │  │
│  │  └──────────┘  └────────────────────────────────────────┘    │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                      │
│  ┌───────────────────────────────────────────────────────────────┐   │
│  │                   POWER MANAGEMENT                            │   │
│  │  PMIC: X-Powers AXP2101        Charger: BQ25895 (USB-C)     │   │
│  │  Rails: 3.3V, 1.8V, 1.5V DDR, 0.9V core                    │   │
│  │  Battery: 1000 mAh LiPo (3.7V nominal)                      │   │
│  │  RTC: NXP PCF8563 (I2C, coin-cell backup)                   │   │
│  └───────────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 1.5 Data Flow & Bus Topology

### 1.5.1 Primary Bus Architecture

| Bus | Protocol | Width | Clock | Bandwidth | Purpose |
|---|---|---|---|---|---|
| **AXI Bus 0** | AXI4 | 128-bit | 3200 MHz DDR | 12.8 GB/s | CPU ↔ DRAM |
| **AXI Bus 1** | AXI4-Lite | 64-bit | 500 MHz | 4.0 GB/s | GPU ↔ DRAM (frame buffer) |
| **PCIe Gen3 x1** | PCIe 3.0 | 1 lane | 8 GT/s | ~985 MB/s | AX210 WiFi |
| **AHB** | AHB-Lite | 32-bit | 200 MHz | 0.8 GB/s | Peripheral register access |
| **APB** | APB4 | 32-bit | 50 MHz | — | Low-speed peripheral config |

### 1.5.2 WiFi Packet Data Flow

```
Antennas (2× RP-SMA)
     │
     ▼
┌─────────────┐    PCIe Gen3 x1 TLP    ┌─────────────────┐
│ Intel AX210 │◄───────────────────────►│  JH7110 SoC    │
│ 802.11ax    │   DMA (8K descriptors)  │                 │
│ 2×2 MIMO    │                         │  ┌───────────┐ │
│             │   Radiotap + MAC hdr    │  │  CPU      │ │
│ Monitor TX: │◄────────────────────────│  │  U74 ×4   │ │
│  Frame inj  │                         │  │  1.5 GHz  │ │
│ Monitor RX: │───────────────────────►│  └─────┬─────┘ │
│  Raw 802.11 │   DMA → DRAM ring buf   │        │       │
└─────────────┘                          └────────┼───────┘
                                                  │
                                          ┌───────┼───────────────────┐
                                          │       │                   │
                                          ▼       ▼                   ▼
                                   ┌──────────┐ ┌──────────┐   ┌──────────┐
                                   │ DRAM     │ │ eMMC     │   │ USB-C    │
                                   │ Ring buf │ │ PCAP log │   │ CDC-ECM  │
                                   │ 2 MB     │ │ (host dl)│   │ → Host   │
                                   └──────────┘ └──────────┘   └──────────┘
```

### 1.5.3 USB-C Composite Gadget Data Flow

```
Host PC ◄──── USB-C 3.2 Gen1 ────► JH7110 USB DWC3 Controller
                                        │
                        ┌───────────────┼───────────────┐
                        │               │               │
                   CDC-ECM (RNDIS)  CDC-ACM      Mass Storage
                   (Ethernet)       (Serial)     (eMMC share)
                   172.16.42.1      115200 8N1   /pcap share
                        │               │               │
                        ▼               ▼               ▼
                   Linux network    ash shell       PCAP files
                   bridge to WiFi   debug console   for offline
```

### 1.5.4 Interrupt Architecture

| Priority | Source | IRQ # | Destination | Notes |
|---|---|---|---|---|
| Highest (0) | PCIe AX210 | IRQ 48 | U74 Core 0 | WiFi DMA completion |
| High (1) | USB DWC3 | IRQ 52 | U74 Core 0 | Host data transfer |
| High (2) | eMMC SDIO | IRQ 38 | U74 Core 1 | Storage I/O |
| Medium (3) | SPI (TPM) | IRQ 55 | U74 Core 2 | Security events |
| Medium (4) | I2C (PMIC/RTC) | IRQ 57 | U74 Core 2 | Power management |
| Low (5) | GPIO (buttons) | IRQ 61 | S7 (L2) | User input |
| Lowest (6) | RTC Tick | IRQ 62 | S7 (L2) | Timekeeping |

### 1.5.5 Power Domains

| Domain | Voltage | Rail Source | Consumers |
|---|---|---|---|
| **VDD_CORE** | 0.9V | AXP2101 DCDC1 | U74 cores, GPU, interconnect |
| **VDD_DDR** | 1.5V | AXP2101 DCDC2 | LPDDR4X VDD2 |
| **VDD_DDR_IO** | 1.8V | AXP2101 DCDC3 | LPDDR4X VDDQ |
| **VDD_SOC** | 1.8V | AXP2101 LDO1 | SoC peripheral IO, eMMC |
| **VDD_3V3** | 3.3V | AXP2101 LDO2 | USB PHY, GPIO, SPI |
| **VDD_PCIE** | 3.3V | AXP2101 LDO3 | PCIe slot (AX210) |
| **VDD_WIFI_IO** | 1.8V | AXP2101 LDO4 | AX210 PCIe refclk IO |
| **VDD_RTC** | 3.0V | Coin cell CR1220 | PCF8563 RTC backup |

---

## 1.6 Assumptions & Rationale

1. **StarFive JH7110** chosen for: mainline Linux support (kernel 6.6+), PCIe Gen3 x1 for AX210, quad-core RISC-V performance, and open-source bootloader (U-Boot + OpenSBI). No NDA required unlike Qualcomm/MediaTek.

2. **Intel AX210NGW** (M.2 2230 E-key) is the only WiFi adapter with full 802.11ax monitor/injection support in mainline Linux. The `iwlwifi` driver supports monitor mode, frame injection, and 6 GHz operation (after regulatory domain initialization).

3. **PCIe (not USB)** interconnect for WiFi: USB-attached WiFi adapters lack the DMA performance and raw frame injection capability required for pentesting. PCIe allows direct DMA of radiotap frames into DRAM ring buffers.

4. **2 GB LPDDR4X** is the sweet spot: enough for Linux networking stack + Scapy + hostapd + capture buffers, while keeping BOM under target.

5. **USB-C composite gadget** (CDC-ECM + CDC-ACM) means zero host drivers — works on Windows, macOS, and Linux out of the box.

6. **Onboard LiPo battery** enables disconnected walk-around audits — capture PMKID handshakes in the field without a laptop attached. Data stored to eMMC for later retrieval.

7. **No HDMI/display** — all interaction via USB serial console or web UI served over CDC-ECM network. Minimizes BOM and power.

8. **TPM (AT97SC3204T)** via SPI for secure boot measurement and key storage — prevents firmware tampering and provides hardware-rooted attestation for audit logs.

---

## 1.7 Pentesting Capability Matrix

| Attack Type | Tool | Description |
|---|---|---|
| **Monitor Mode** | `iw dev wlan0 set monitor control` | Capture all 802.11 frames on channel |
| **Deauth Flood** | `aireplay-ng -0 10 -a <BSSID>` | Disconnect clients from AP |
| **PMKID Capture** | `hcxdumptool` | Single-frame attack on WPA2/WPA3 |
| **Handshake Capture** | `airodump-ng` | Capture 4-way WPA handshake |
| **Evil Twin** | `hostapd-wpe` | Rogue AP with credential harvesting |
| **Channel Hop** | Custom script | Fast channel scanning across 2.4/5/6 GHz |
| **Frame Injection** | `aireplay-ng`, custom Scapy | Arbitrary 802.11 frame transmission |
| **Karma Attack** | `hostapd` + KARMA patches | Probe response spoofing |
| **WPA3 Dragonfly** | `hcxtools` | Dragonfly exchange capture & analysis |
| **Beacon Flood** | `mdk4` | Fake AP beacon generation |

---

## 1.8 Mechanical Concept

```
          ┌─────────────────────────────────────────────────┐
          │  WFP-100 Enclosure (85mm × 35mm × 12mm)        │
          │                                                   │
  RP-SMA ─┤○  ○─ RP-SMA            ┌──────────────┐        │
  Ant 1   │  Ant 2                  │  Intel AX210  │        │
          │                         │  M.2 2230     │        │
          │                         └──────┬───────┘        │
          │                                │ PCIe           │
          │    ┌───────────────────────┐   │                │
          │    │   JH7110 SoC          │◄──┘                │
          │    │   + LPDDR4X (PoP)     │                    │
          │    └───────────┬───────────┘                    │
          │                │                                 │
          │   ┌────────────┴────────────┐                   │
          │   │ eMMC │ microSD │ TPM    │                   │
          │   └─────────────────────────┘                    │
          │                                                    │
          │   [●PWR] [●ACT] [●MON]     [BTN: Mode]          │
          │    LED    LED     LED                                │
          │                                                   │
  USB-C ──┤                                                   │
          └─────────────────────────────────────────────────┘
```

LED Indicators:
- **PWR** (Red): Solid = powered, Blinking = low battery
- **ACT** (Green): Blinking = CPU active, Solid = idle
- **MON** (Blue): Solid = monitor mode, Blinking = injecting, Off = station mode

Button: Cycle mode (Station → Monitor → Inject → AP → Station)