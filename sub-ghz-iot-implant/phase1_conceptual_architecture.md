# Sub-GHz IoT Gateway Implant — Phase 1: Conceptual Architecture

## 1. System Purpose

The **Sub-GHz IoT Gateway Implant** (codename: **SUBSTATION**) is a portable, pocket-sized multi-protocol Sub-GHz attack platform targeting Zigbee, Z-Wave, proprietary Sub-GHz IoT protocols (433/868/915 MHz), and BLE backhaul for operator control. It enables security researchers and red teamers to:

- **Passively sniff** Zigbee (IEEE 802.15.4), Z-Wave, and raw Sub-GHz RF traffic
- **Replay and inject** captured frames with timestamp-accurate timing
- **Active MITM** Zigbee and Z-Wave networks by impersonating coordinators/routers
- **Brute-force** rolling-code Sub-GHz protocols (garage doors, key fobs)
- **Enumerate** nearby IoT networks with RSSI heatmapping
- **Exfiltrate** captured data over BLE to a companion mobile app

The device operates as a USB-powered or battery-operated implant that can be covertly deployed or hand-carried during physical pentests.

## 2. Attack Surface & Threat Model

### Primary Targets
| Protocol | Frequency | Modulation | Typical Targets |
|----------|-----------|------------|-----------------|
| Zigbee (IEEE 802.15.4) | 2405–2480 MHz (also Sub-GHz: 868/915) | O-QPSK | Smart home hubs, lights, locks, sensors |
| Z-Wave | 868.4 MHz (EU) / 908.4 MHz (US) | GFSK/FSK | Door locks, sensors, switches |
| Sub-GHz OOK/ASK | 315/433/868/915 MHz | OOK/ASK | Garage doors, key fobs, weather stations |
| Sub-GHz FSK/GFSK | 433/868/915 MHz | FSK/GFSK | Tire pressure monitors, remote switches |
| BLE (backhaul) | 2402–2480 MHz | GFSK | Operator control channel (not attack target) |

### Attack Modes
1. **Passive Sniff**: Receive-only, no RF footprint, capture all Sub-GHz traffic
2. **Active Replay**: Re-transmit captured frames (rolling code, protocol frames)
3. **Active MITM**: Interpose between coordinator and end devices (Zigbee/Z-Wave)
4. **Fuzz/Inject**: Generate malformed protocol frames for vulnerability research
5. **Rolling Code Analysis**: Capture multiple rolling-code transmissions, predict next code

### Threat Model (from target's perspective)
- Attacker can passively capture all unencrypted IoT traffic
- Attacker can replay captured frames to trigger actions (open garage, unlock door)
- Attacker can impersonate trusted devices after key extraction
- Attacker can deny service via jamming or deauthentication

## 3. Performance Targets

| Parameter | Target |
|-----------|--------|
| Frequency range | 281–1660 MHz (CC1352P) + 2.4 GHz BLE |
| RX sensitivity (Sub-GHz) | -110 dBm (868 MHz, 50 kbps) |
| TX output power | Up to +20 dBm (CC1352P PA) |
| Capture buffer | 64 MB SDRAM for packet captures |
| Packet throughput | ≥ 2000 pkts/sec capture rate |
| BLE throughput | ≥ 250 kbps sustained to app |
| Boot time | < 800 ms from power-on to operational |
| Battery life (sniff mode) | ≥ 12 hours on 1000 mAh LiPo |
| Battery life (active TX) | ≥ 4 hours continuous |
| USB backhaul speed | ≥ 8 Mbps (USB 2.0 FS) |
| Form factor | 85 mm × 54 mm (credit card footprint) |
| BOM cost target | < $85 (1000-unit volume) |

## 4. Design Constraints

1. **Regulatory**: Device is a research tool; TX must respect local regulations. Firmware enforces configurable frequency/power limits per region.
2. **Covert operation**: Silent RF emission in sniff mode; no indicators on device casing.
3. **Dual power**: USB-C for lab use, LiPo for field deployment.
4. **No external antenna required**: PCB trace antenna + optional SMA pigtail for external antenna.
5. **Single-chip SoC preferred**: CC1352P integrates MCU + Sub-GHz + 2.4 GHz radio, reducing BOM.
6. **Open firmware**: All firmware is open-source for audit and modification.

## 5. Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        SUBSTATION Top-Level Block                  │
│                                                                     │
│  ┌──────────────┐    ┌──────────────────────────────────────┐     │
│  │  USB-C       │    │          CC1352P SoC                 │     │
│  │  Connector   │◄──►│  ┌──────────┐  ┌──────────────────┐  │     │
│  │  (Power +    │    │  │ ARM      │  │ Multi-Protocol   │  │     │
│  │   Data)      │    │  │ Cortex-  │  │ Radio Engine     │  │     │
│  └──────────────┘    │  │ M4F      │  │ - Sub-GHz TX/RX  │  │     │
│                       │  │ @ 48 MHz │  │ - 2.4 GHz BLE   │  │     │
│  ┌──────────────┐    │  └────┬─────┘  └────────┬─────────┘  │     │
│  │  LiPo        │    │       │                  │            │     │
│  │  Charger +   │◄──►│  ┌────┴──────────────────┴─────┐    │     │
│  │  1000 mAh    │    │  │  Peripherals & Memory Bus    │    │     │
│  └──────────────┘    │  │  ┌────────┐  ┌────────────┐  │    │     │
│                       │  │  │ 64 MB  │  │   SPI      │  │    │     │
│  ┌──────────────┐    │  │  │ SDRAM  │  │   Flash    │  │    │     │
│  │  SMA / U.FL  │    │  │  │ IS66   │  │   16 MB    │  │    │     │
│  │  Antenna     │◄──►│  │  │ WVS256  │  │   MX25    │  │    │     │
│  │  Connector   │    │  │  └────────┘  │   LW1636  │  │    │     │
│  └──────────────┘    │  │  ┌────────┐  └────────────┘  │    │     │
│                       │  │  │ 128 Kb │                 │    │     │
│  ┌──────────────┐    │  │  │ SRAM   │                 │    │     │
│  │  RGB LED     │    │  │  │ (int)  │                 │    │     │
│  │  (Status)    │◄──►│  │  └────────┘                 │    │     │
│  └──────────────┘    │  │  ┌────────┐                  │    │     │
│                       │  │  │ Crypto │                  │    │     │
│  ┌──────────────┐    │  │  │ ATECC  │                  │    │     │
│  │  MicroSD     │    │  │  │ 608A   │                  │    │     │
│  │  Slot        │◄──►│  │  └────────┘                  │    │     │
│  └──────────────┘    │  └──────────────────────────────┘    │     │
│                       └──────────────────────────────────────┘     │
│                                                                     │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐         │
│  │  Power Mgmt  │    │  Level Shift │    │  ESD Clamp   │         │
│  │  TLV62569    │    │  TXB0108     │    │  TPD4E05U06  │         │
│  │  3.3V + 1.2V│    │  (SD Card IO) │    │  (USB/SD)    │         │
│  └──────────────┘    └──────────────┘    └──────────────┘         │
└─────────────────────────────────────────────────────────────────────┘
```

## 6. Data Flow

### 6.1 Packet Capture Flow (Sub-GHz RX → BLE → App)
```
Antenna → CC1352P RF Frontend → Radio CPU (LNA → Mixer → ADC → Demod)
  → IEEE 802.15.4 / Z-Wave / Raw Decoder → Packet Buffer (SRAM)
  → DMA → SDRAM Ring Buffer (64 MB)
  → Packet Filter / Classifier → BLE GATT Notification
  → Companion App (over BLE)
```

### 6.2 Replay Flow (App → BLE → Sub-GHz TX)
```
Companion App → BLE Write Characteristic → CC1352P BLE Stack
  → Command Parser → Packet Constructor (re-encode)
  → Radio CPU (Mod → DAC → PA → Antenna)
```

### 6.3 MITM Flow (Bidirectional Interposition)
```
Target Coordinator → Antenna → CC1352P RX → Frame Parser
  → Impersonation Engine → CC1352P TX → Target End Device
  (simultaneously: Target End Device → RX → Impersonation → TX → Coordinator)
```

### 6.4 USB Capture Download Flow
```
CC1352P USB Controller → USB-C Connector → Host PC
  (Snoop/PCAP format over bulk endpoint)
```

## 7. Bus Topology

```
                    CC1352P (Main SoC)
                    ├── SSI0 (SPI) ──────── IS66WVS256 (SDRAM, 64 MB)
                    │     CLK = SSI0_CLK
                    │     MOSI = SSI0_TX
                    │     MISO = SSI0_RX
                    │     CS  = SSI0_FSS
                    ├── SSI1 (SPI) ──────── MX25LW1636 (Flash, 16 MB)
                    │     CLK = SSI1_CLK
                    │     MOSI = SSI1_TX
                    │     MISO = SSI1_RX
                    │     CS  = GPIO_12
                    ├── I²C (SSI2 alt) ──── ATECC608A (Crypto)
                    │     SDA = SSI2_DATA
                    │     SCL = SSI2_CLK
                    ├── UART0 ───────────── Debug Console (115200 8N1)
                    ├── UART1 ───────────── Expansion Header
                    ├── SPI (SSI3) ──────── MicroSD Card
                    │     via TXB0108 level shifter (3.3V ↔ SD_IO)
                    ├── GPIO ────────────── RGB LED (WS2812B)
                    ├── GPIO ────────────── Buttons (2: MODE, ACTION)
                    ├── USB ─────────────── USB-C Connector (FS)
                    └── RF Paths ──────────
                          ├── Sub-GHz PA → SMA/U.FL (868/915 MHz)
                          └── 2.4 GHz → PCB Trace Antenna (BLE)
```

### Address Map
| Region | Base Address | Size | Purpose |
|--------|-------------|------|---------|
| Flash (internal) | 0x0000_0000 | 1 MB | Application firmware |
| SRAM (internal) | 0x2000_0000 | 128 KB | Stack, heap, buffers |
| SDRAM (external) | 0x6000_0000 | 64 MB | Packet capture ring buffer |
| SPI Flash (external) | 0x7000_0000 | 16 MB | Stored captures, config |
| Peripherals | 0x4000_0000 | 64 KB | MMIO registers |

### Interrupt Priority Assignment
| Priority | IRQ | Purpose |
|----------|-----|---------|
| 0 (highest) | RF Core Hardware | Packet RX/TX completion |
| 1 | SSI0 (SDRAM DMA) | Ring buffer DMA done |
| 2 | SSI1 (Flash) | Flash write complete |
| 3 | I²C (ATECC) | Crypto operation done |
| 4 | UART0 | Debug console |
| 5 | GPIO (Buttons) | User input |
| 6 | USB | Host communication |
| 7 (lowest) | Timer (RTC) | Housekeeping, watchdog |

## 8. Power Architecture

```
USB-C (5V) ──┬── TLV62569 (Buck) ── 3.3V Main Rail ── CC1352P, Flash, SD, Crypto
              │       (2A max)
              ├── MCP73871 (LiPo Charger) ── LiPo 3.7V 1000mAh
              │       (charge from USB)
              └── TLV62569 (Buck) ── 1.2V Core Rail ── CC1352P Core
                      (500mA max)

Battery Path:
LiPo (3.7V) ── TPS63020 (Buck-Boost) ── 3.3V Main Rail
                      (2A max, >95% eff)
```

### Power Budget
| Rail | Voltage | Max Current | Source |
|------|---------|-------------|--------|
| VDD_MAIN | 3.3V | 2A | TLV62569 or TPS63020 |
| VDD_CORE | 1.2V | 500mA | TLV62569 |
| VDD_PA | 3.3V | 1A (TX burst) | Direct from VDD_MAIN |
| VDD_RF | 1.7V | 200mA | Internal LDO (CC1352P) |
| VDD_SD | 3.3V | 300mA | VDD_MAIN via level shifter |
| VDD_BLE | 3.3V | 50mA | Internal (CC1352P) |

### Battery Life Estimates
- **Sniff mode (RX only)**: CC1352P ~10 mA + SDRAM ~15 mA = ~25 mA → 40 hrs (theoretical), 12+ hrs (conservative with BLE active)
- **Active TX (20 dBm)**: CC1352P ~130 mA peak → 4+ hrs (conservative with duty cycling)
- **Standby (BLE advertising)**: ~2 mA → 500+ hrs

## 9. Mechanical & Environmental

| Parameter | Specification |
|-----------|--------------|
| PCB dimensions | 85 mm × 54 mm × 1.6 mm |
| Enclosure | 3D-printed PETG, 90 × 58 × 15 mm |
| Weight (w/ battery) | < 65 g |
| Operating temperature | -10°C to +50°C |
| Storage temperature | -20°C to +70°C |
| Humidity | 0–90% RH non-condensing |
| Connectors | USB-C (data+charge), SMA (external antenna), MicroSD |
| User interface | 2 tactile buttons, 1 RGB LED (WS2812B) |

## 10. Security Architecture

- **ATECC608A** provides hardware root of trust:
  - Secure boot: firmware signature verification using ATECC ECDSA
  - BLE pairing: ATECC-stored keys for authenticated BLE connections
  - Anti-rollback: ATECC monotonic counter prevents firmware downgrade
- **Encrypted SDRAM**: AES-128-CTR encryption of captured packets in external RAM (key in CC1352P internal key store)
- **Secure USB**: CDC-ECM with device authentication before pcap download
- **Configurable TX limits**: Region database in SPI flash enforces legal frequency/power constraints