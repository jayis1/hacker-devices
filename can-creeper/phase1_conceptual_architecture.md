# Phase 1: Conceptual Architecture — CAN Creeper

## 1.0 Executive Summary

CAN Creeper is a pocket-sized, battery-powered CAN bus diagnostic, passive-sniffing, and active-injection tool designed for automotive security research, ECU reverse engineering, and red-team physical penetration testing. It supports both classical CAN 2.0B (11/29-bit IDs, up to 1 Mbps) and CAN FD (flexible data rate, up to 8 Mbps arbitration / 15 Mbps data phase) on two independent channels simultaneously. The device can operate as a passive man-in-the-middle tap (spliced inline between an ECU and the bus), an OBD-II plug-in sniffer, or an active frame injector capable of crafting arbitrary CAN/CAN FD frames, ISO-TP segmented messages, and UDS diagnostic requests.

Control and data exfiltration occur over Bluetooth 5.2 LE using the Nordic UART Service (NUS) with a custom binary wire protocol, or over USB 2.0 Full Speed CDC-ACM for wired operation. A companion React Native mobile app provides real-time frame logging with hardware-timestamped precision, DBC file parsing, signal graphing, and scripted injection sequences.

## 1.1 System Purpose and Attack Surface

### Primary Use Cases

| Use Case | Description | Threat Model |
|---|---|---|
| Passive Bus Sniffing | Tap into CAN-H/CAN-L of a vehicle bus (OBD-II port or direct splice) and log all frames with 1 µs timestamps | Reconnaissance, protocol reverse engineering |
| Active Frame Injection | Craft and transmit arbitrary CAN/CAN FD frames at programmable rates to trigger ECU responses | Fuzzing, replay attacks, diagnostic manipulation |
| ISO-TP / UDS Probing | Send multi-frame ISO 15765-2 segmented messages and UDS (ISO 14229) service requests | ECU firmware extraction, security-access brute force, DID reading |
| Man-in-the-Middle | Insert between two bus segments, selectively forward, modify, or drop frames | Gateway bypass, message spoofing, arbitration-ID remapping |
| CAN FD Discovery | Probe ECUs for CAN FD capability, negotiate BRS bit-rate switch, capture high-speed data phases | Next-gen vehicle assessment |
| Bus Load Analysis | Measure bus utilization, error frame counts, and identify malfunctioning nodes | Diagnostics, denial-of-service detection |

### Attack Surface

- **Physical access**: OBD-II port (standardized pin 6=CAN-H, pin 14=CAN-L), direct wire splice, or DB9/CANopen connectors
- **Protocol layers**: CAN 2.0B frames, CAN FD frames, ISO-TP (ISO 15765-2), UDS (ISO 14229-1), OBD-II PID (SAE J1979)
- **Wireless attack vector**: BLE GATT/NUS for remote control up to ~30m line-of-sight
- **USB attack vector**: CDC-ACM serial emulation for scripted automation from a laptop

## 1.2 Performance Targets

| Parameter | Target | Notes |
|---|---|---|
| CAN channels | 2 independent | Simultaneous sniff on both, inject on either |
| CAN 2.0B bitrate | 125 kbps – 1 Mbps | Standard automotive rates |
| CAN FD arbitration rate | Up to 8 Mbps | ISO 11898-1:2015 |
| CAN FD data rate | Up to 15 Mbps | BRS-enabled phase |
| Timestamp resolution | 1 µs (32-bit counter) | Free-running 1 MHz timer, wraps every ~71 minutes |
| Frame buffer depth | 16,384 frames per channel | Stored in external PSRAM |
| BLE throughput | ~80 KB/s (NUS) | Sufficient for ~8000 frames/sec at 10 bytes/frame |
| USB throughput | ~1 MB/s (CDC-ACM) | Full-speed USB 2.0 |
| Injection rate | Up to 100% bus load | Programmable inter-frame spacing |
| Power consumption | <150 mA active, <50 µA sleep | 3.7V LiPo, ~8 hours active on 2000 mAh |
| BOM cost target | <$75 USD | Qty 1 pricing from LCSC/Mouser |

## 1.3 Block Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                        CAN Creeper System                         │
│                                                                   │
│  ┌──────────┐   ┌──────────┐   ┌──────────────────────────────┐  │
│  │ 3.7V LiPo │──▶│  TP4056  │──▶│  TPS63070 Buck-Boost → 3.3V │  │
│  │ 2000mAh   │   │ Charger  │   │  + TPS7A4701 LDO → 1.8V     │  │
│  └──────────┘   │ + DW01   │   │  + AP2112K LDO → 5V (CAN)   │  │
│                 │ Protect  │   └──────────────┬───────────────┘  │
│                 └──────────┘                  │                   │
│                                               ▼                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │              nRF52840 SoC (Cortex-M4F @ 64 MHz)            │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │  │
│  │  │ ARM M4F  │  │ 256 KB   │  │  1 MB    │  │  BLE 5.2 │   │  │
│  │  │   Core   │  │   RAM    │  │  Flash   │  │  Radio   │   │  │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │  │
│  │  │ SPI0     │  │ SPI1     │  │ TWIM0    │  │ USBD     │   │  │
│  │  │ (CAN0)   │  │ (CAN1)   │  │ (OLED)   │  │ (CDC)    │   │  │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │  │
│  └───────┼─────────────┼─────────────┼─────────────┼──────────┘  │
│          │             │             │             │              │
│          ▼             ▼             ▼             ▼              │
│  ┌──────────────┐ ┌──────────────┐ ┌────────┐ ┌──────────┐      │
│  │ MCP2518FD    │ │ MCP2518FD    │ │ SSD1306│ │ USB-C    │      │
│  │ CAN FD Ctrl  │ │ CAN FD Ctrl  │ │ 128x64 │ │ Recept.  │      │
│  │ + TJA1445    │ │ + TJA1445    │ │ OLED   │ │ (CDC)    │      │
│  │ Transceiver  │ │ Transceiver  │ │ Display│ └──────────┘      │
│  └──────┬───────┘ └──────┬───────┘ └────────┘                   │
│         │                │                                       │
│         ▼                ▼                                       │
│  ┌──────────┐     ┌──────────┐                                   │
│  │ CAN0     │     │ CAN1     │                                   │
│  │ DB9 +    │     │ OBD-II   │                                   │
│  │ Terminals│     │ J1962    │                                   │
│  └──────────┘     └──────────┘                                   │
│                                                                   │
│  ┌──────────────────────┐                                        │
│  │  APS6404L PSRAM      │  8 MB QSPI PSRAM for frame buffering   │
│  │  (QSPI @ 80 MHz)     │                                        │
│  └──────────────────────┘                                        │
│                                                                   │
│  ┌──────────────────────┐                                        │
│  │  W25Q128JV Flash     │  16 MB NOR flash for DBC storage,      │
│  │  (QSPI @ 80 MHz)     │  firmware OTA, injection scripts        │
│  └──────────────────────┘                                        │
└──────────────────────────────────────────────────────────────────┘
```

## 1.4 Data Flow Architecture

### Sniffing Data Path (Channel 0 Example)

```
CAN Bus ──▶ TJA1445AT ──▶ MCP2518FD (SPI slave) ──▶ nRF52840 SPI0 ──▶
  DMA to PSRAM ring buffer ──▶ Frame parser (SW) ──▶
    ┌─ BLE NUS TX characteristic (wireless)
    └─ USB CDC-ACM TX (wired)
```

1. Differential CAN-H/CAN-L signals enter TJA1445AT transceiver
2. TJA1445 converts to single-ended RXD, outputs to MCP2518FD
3. MCP2518FD performs CAN FD protocol decoding, stores frame in internal 31-FIFO
4. nRF52840 reads frame via SPI at 8 MHz using EasyDMA
5. Frame metadata (timestamp, channel, flags) prepended in software
6. Frame pushed to APS6404L PSRAM ring buffer via QSPI
7. Frame serialized per wire protocol, queued for BLE NUS notification or USB CDC TX

### Injection Data Path

```
BLE NUS RX / USB CDC RX ──▶ Protocol parser ──▶ Frame validator ──▶
  Injection scheduler ──▶ MCP2518FD TX FIFO (SPI) ──▶ TJA1445AT ──▶ CAN Bus
```

1. Frame received from app via BLE NUS write or USB CDC
2. Binary protocol parser validates CRC, extracts CAN ID, DLC, data, flags
3. Injection scheduler checks bus state (via MCP2518FD TEC/REC error counters)
4. Frame written to MCP2518FD TX FIFO via SPI
5. MCP2518FD arbitrates for bus access, transmits via TJA1445AT

### MITM Data Path

```
CAN Bus A ──▶ TJA1445AT #0 ──▶ MCP2518FD #0 ──▶ nRF52840 ──▶
  Filter/Modify Logic ──▶ MCP2518FD #1 ──▶ TJA1445AT #1 ──▶ CAN Bus B
```

## 1.5 Bus Topology

### Internal Buses

| Bus | Master | Slaves | Speed | Notes |
|---|---|---|---|---|
| SPI0 | nRF52840 | MCP2518FD #0 | 8 MHz | Mode 0,0; CS on P0.17 |
| SPI1 | nRF52840 | MCP2518FD #1 | 8 MHz | Mode 0,0; CS on P0.22 |
| QSPI | nRF52840 | APS6404L, W25Q128JV | 80 MHz | Dual-device on separate CS; PSRAM CS=P0.13, Flash CS=P0.19 |
| TWIM0 (I²C) | nRF52840 | SSD1306 OLED | 400 kHz | Address 0x3C; SDA=P0.26, SCL=P0.27 |
| USBD | nRF52840 | USB-C receptacle | 12 Mbps | Full-speed USB 2.0, CDC-ACM class |

### External Interfaces

| Interface | Connector | Signals | Notes |
|---|---|---|---|
| CAN Channel 0 | DB9 female + screw terminals | CAN-H, CAN-L, GND | Software-selectable 120Ω termination via GPIO-controlled FET |
| CAN Channel 1 | OBD-II J1962 male | CAN-H (pin 6), CAN-L (pin 14), GND (pin 4), VBAT (pin 16, for detection only) | 120Ω termination always off (vehicle provides) |
| USB | USB-C receptacle | D+, D-, VBUS, GND, CC1, CC2 | 5V power + CDC-ACM data |
| Battery | JST-PH 2.0mm | VBAT+, VBAT- | 3.7V LiPo |
| Debug | Tag-Connect 6-pin | SWDIO, SWCLK, VTG, GND, nRESET | TC2050-IDC-NL footprint |

## 1.6 Power Architecture

```
USB-C VBUS (5V) ──▶ TP4056 ──▶ LiPo (3.7-4.2V) ──▶ DW01-P + FS8205A protection
                           │
                           ▼
                    TPS63070 Buck-Boost
                    (2.0V–16V in → 3.3V out @ 2A)
                           │
                    ┌──────┼──────┐
                    ▼      ▼      ▼
                 3.3V    1.8V    5V_CAN
                 Rail    Rail    Rail
                 (main)  (QSPI)  (TJA1445)
                          │
                   TPS7A4701   AP2112K-5.0
                   LDO         LDO
```

### Power Modes

| Mode | nRF52840 | CAN Transceivers | PSRAM | BLE | Current |
|---|---|---|---|---|---|
| Active Sniffing | ON (64 MHz) | Normal mode | Active | Advertising/Connected | ~45 mA |
| Active Injection | ON (64 MHz) | Normal mode | Active | Connected | ~55 mA |
| Idle (Connected) | ON (64 MHz) | Standby | Standby | Connected | ~8 mA |
| Sleep | SYSTEM_ON (sleep) | Sleep | Power-down | Disconnected (sleep) | ~35 µA |
| Off | Power switch open | — | — | — | 0 µA |

## 1.7 Clock Architecture

```
┌─────────────────────────────────────────────┐
│  32 MHz Crystal (NX3225SA-32.000000MHZ)     │
│  ──▶ nRF52840 HFCLK (64 MHz PLL)            │
│      ┌─▶ CPU @ 64 MHz                       │
│      ├─▶ SPIM0/1 @ 8 MHz (÷8)              │
│      ├─▶ QSPI @ 80 MHz (HFCLK/div)         │
│      └─▶ USBD @ 48 MHz (HFCLK/div)         │
│                                              │
│  32.768 kHz Crystal (FC-135)                 │
│  ──▶ nRF52840 LFCLK                         │
│      ┌─▶ RTC0 (1 µs timestamp counter)      │
│      ├─▶ BLE SoftDevice LFCLK               │
│      └─▶ WDT (watchdog)                     │
│                                              │
│  40 MHz Crystal (for MCP2518FD #0)          │
│  40 MHz Crystal (for MCP2518FD #1)          │
└─────────────────────────────────────────────┘
```

## 1.8 Memory Map

| Region | Start Address | Size | Device | Purpose |
|---|---|---|---|---|
| CODE_FLASH | 0x00000000 | 1 MB | nRF52840 internal | Firmware + SoftDevice |
| PSRAM | 0x00800000 | 8 MB | APS6404L (QSPI) | Frame ring buffers (2× 16K frames) |
| NOR_FLASH | 0x01000000 | 16 MB | W25Q128JV (QSPI) | DBC files, scripts, OTA images |
| UICR | 0x10001000 | — | nRF52840 | Customer configuration |
| FICR | 0x10000000 | — | nRF52840 | Factory info (device ID) |

### PSRAM Ring Buffer Layout

```
Offset 0x000000: Channel 0 Frame Buffer (16,384 entries × 32 bytes = 512 KB)
  Each entry:
    [0-3]   Timestamp (µs, 32-bit)
    [4-7]   CAN ID + Flags (32-bit: ID[28:0], IDE[29], RTR[30], FD[31])
    [8]     DLC (4-bit) + BRS (1-bit) + ESI (1-bit) + reserved
    [9-15]  Reserved
    [16-31] Data bytes 0-15 (CAN FD up to 64 bytes, but we store max 16 for ring efficiency)
    [32-47] Extended data bytes 16-31 (only for CAN FD with DLC > 8, stored in next slot)
Offset 0x080000: Channel 1 Frame Buffer (16,384 entries × 32 bytes = 512 KB)
Offset 0x100000: Injection Script Storage (1 MB)
Offset 0x200000: DBC File Cache (2 MB)
Offset 0x400000: Reserved / Scratch (4 MB)
```

## 1.9 Firmware Architecture Overview

```
┌──────────────────────────────────────────────┐
│              Application Layer               │
│  ┌──────────┐ ┌──────────┐ ┌──────────────┐ │
│  │ Frame    │ │ Injection│ │ Protocol     │ │
│  │ Logger   │ │ Scheduler│ │ Handler      │ │
│  └────┬─────┘ └────┬─────┘ └──────┬───────┘ │
├───────┼─────────────┼─────────────┼──────────┤
│       ▼             ▼             ▼          │
│  ┌────────────────────────────────────────┐  │
│  │         Hardware Abstraction Layer     │  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐  │  │
│  │  │ CAN  │ │ QSPI │ │ BLE  │ │ USB  │  │  │
│  │  │ FD   │ │ PSRAM│ │ NUS  │ │ CDC  │  │  │
│  │  │ Drv  │ │ Drv  │ │ Drv  │ │ Drv  │  │  │
│  │  └──────┘ └──────┘ └──────┘ └──────┘  │  │
│  └────────────────────────────────────────┘  │
├──────────────────────────────────────────────┤
│  nRF5 SDK 17.1.0 + SoftDevice S140 7.3.0    │
│  ARM Cortex-M4F @ 64 MHz                     │
└──────────────────────────────────────────────┘
```

## 1.10 Security Considerations

- **Firmware integrity**: Signed images verified by nRF52840's immutable bootloader (BPROT/ACL)
- **BLE pairing**: LE Secure Connections with numeric comparison (MITM protection)
- **USB access**: No mass storage — CDC-ACM only, no file system exposure
- **CAN bus isolation**: TJA1445 provides galvanic isolation between CAN bus and device logic; no direct electrical path from CAN-H/CAN-L to MCU
- **Injection guard**: Hardware-enforced minimum inter-frame spacing (2 bit times) to prevent bus monopolization
- **Physical tamper**: Exposed SWD disabled in production via APPROTECT fuse

## 1.11 Form Factor

- PCB dimensions: 65 mm × 35 mm (credit-card sized)
- 2-layer PCB, 1.6 mm FR-4
- Enclosure: 3D-printed ABS, snap-fit, with cutouts for OLED, USB-C, DB9, OBD-II pigtail
- Weight: ~35g without battery, ~55g with 2000 mAh LiPo
