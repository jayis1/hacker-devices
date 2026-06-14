# Phase 1 — Conceptual Architecture: CAN Bus Infiltrator

## 1. System Purpose

The **CAN Bus Infiltrator** is a pocket-sized automotive security research platform that connects to a vehicle's OBD-II diagnostic port and provides real-time CAN bus monitoring, message injection, fuzzing, and replay capabilities. It targets penetration testers, automotive security researchers, and red teamers evaluating vehicle network security.

### Core Capabilities
- **Passive Sniffing**: Monitor all traffic on up to 2 CAN buses (high-speed CAN-H / CAN-L) simultaneously with hardware timestamping.
- **Active Injection**: Craft and transmit arbitrary CAN frames (standard 11-bit and extended 29-bit IDs) with precise timing control.
- **Fuzzing Engine**: On-device stateful CAN fuzzer with mutation strategies (bit-flip, byte-random, field-aware, arithmetic).
- **Capture & Replay**: Record CAN sessions to microSD, replay with configurable loop count and inter-frame delay fidelity.
- **Protocol Decoding**: Real-time UDS (ISO 14229), OBD-II (ISO 15031), and SAE J1939 protocol dissection.
- **Wireless Control**: BLE 5.0 link to companion mobile app for remote operation without physical proximity.
- **USB-C Dual Role**: Acts as USB-CDC device for direct PC control; also used for firmware updates and power.

## 2. Attack Surface Analysis

| Attack Vector | Direction | Description |
|---|---|---|
| OBD-II Port (CAN-H/CAN-L) | In/Out | Primary bus interface — sniff, inject, fuzz |
| USB-C Data | In/Out | Host control, DFU firmware updates, power |
| BLE 5.0 | In/Out | Wireless command/control from mobile app |
| microSD | In | Offline replay scripts, session logs |
| SWD Debug | In | JTAG/SWD port for development & recovery |

### Threat Model
- The device is a **research tool** — it does not exploit ECU vulnerabilities directly but provides the bus access layer.
- Physical access to OBD-II port is required for bus connection.
- BLE pairing uses passkey authentication; bonding stored in secure element.
- USB CDC interface requires no authentication (development convenience).
- Firmware DFU via USB is signed with Ed25519 (public key embedded in bootloader).

## 3. Performance Targets

| Parameter | Target |
|---|---|
| CAN bus speed | Up to 1 Mbit/s (CAN 2.0B) |
| Simultaneous buses | 2 (dual-channel) |
| Frame capture rate | ≥ 10,000 frames/s per channel |
| Injection latency | ≤ 50 µs from trigger to bus |
| Timestamp resolution | 1 µs (hardware-stamped) |
| BLE throughput | ≥ 200 kbps sustained |
| USB CDC throughput | ≥ 1 Mbps sustained |
| microSD write speed | ≥ 2 MB/s (Class 10) |
| Power consumption | ≤ 500 mA @ 5 V (2.5 W) |
| Boot time | ≤ 2 s to operational |
| Battery | None — powered by vehicle OBD-II (8–16 V) or USB-C (5 V) |

## 4. Design Constraints

| Constraint | Value | Rationale |
|---|---|---|
| BOM cost | < $45 @ 100 units | Affordable research tool |
| PCB size | ≤ 65 mm × 35 mm (credit-card-width) | Fits behind OBD-II dongle shell |
| PCB layers | 4 | Cost/complexity balance |
| Operating temp | –40 °C to +85 °C | Automotive under-dash environment |
| ESD protection | ±8 kV contact on all external interfaces | ISO 10605 automotive spec |
| Galvanic isolation | 2.5 kV on CAN bus (digital isolator) | Protect host MCU from vehicle faults |
| Software | Bare-metal C, no OS | Deterministic timing, small footprint |
| Firmware update | Signed DFU over USB or BLE | Anti-rollback protection |

## 5. High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     CAN Bus Infiltrator                         │
│                                                                 │
│  ┌──────────┐    ┌──────────────┐    ┌────────────────────┐     │
│  │ OBD-II   │    │  CAN         │    │  STM32F407VGT6     │     │
│  │ Connector│───▶│ Transceiver  │───▶│  (Cortex-M4 @168MHz)│     │
│  │ (Pin 6/14│    │ TJA1050 (×2) │    │                     │     │
│  │  CAN-H/L) │    │ + ISO1050   │    │  ┌──────────────┐  │     │
│  └──────────┘    │ galvanic iso │    │  │ CAN bxCAN    │  │     │
│                   └──────┬───────┘    │  │ (2 × CAN2.0B)│  │     │
│                          │            │  └──────────────┘  │     │
│                          │            │  ┌──────────────┐  │     │
│  ┌──────────┐            │            │  │ SDIO (microSD)│  │     │
│  │ microSD  │◀───────────┼────────────┤  └──────────────┘  │     │
│  │ Slot     │            │            │  ┌──────────────┐  │     │
│  └──────────┘            │            │  │ USB OTG FS   │  │     │
│                          │            │  └──────┬───────┘  │     │
│  ┌──────────┐            │            │         │          │     │
│  │ USB-C    │◀───────────┼────────────┤         │          │     │
│  │ Port     │            │            │  ┌──────┴───────┐  │     │
│  └──────────┘            │            │  │ BLE nRF52840 │  │     │
│                          │            │  │ (co-processor)│  │     │
│  ┌──────────┐            │            │  └──────┬───────┘  │     │
│  │ LED Bar  │◀───────────┼────────────┤         │          │     │
│  │ (4×RGB)  │            │            │  ┌──────┴───────┐  │     │
│  └──────────┘            │            │  │ SPI to nRF   │  │     │
│                          │            │  └──────────────┘  │     │
│  ┌──────────┐            │            └────────────────────┘     │
│  │ SWD      │◀───────────┘                                       │
│  │ Debug     │                                                    │
│  └──────────┘                                                    │
│                                                                 │
│  Power Tree:                                                    │
│  OBD-II (12 V) ──▶ LM2596 ──▶ 3.3 V rail                      │
│  USB-C (5 V)   ──▶ LDO    ──▶ 3.3 V rail (OR'd)               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 6. Data Flow

### 6.1 Sniffing Path
```
CAN Bus → TJA1050 → ISO1050 (isolator) → STM32 bxCAN → Ring Buffer
  → SDIO (microSD log)  |  → SPI (BLE stream)  |  → USB CDC (PC stream)
```

### 6.2 Injection Path
```
Mobile App → BLE → nRF52840 → SPI → STM32 → bxCAN → ISO1050 → TJA1050 → CAN Bus
USB Terminal → USB CDC → STM32 → bxCAN → ISO1050 → TJA1050 → CAN Bus
microSD Script → SDIO → STM32 → bxCAN → ISO1050 → TJA1050 → CAN Bus
```

### 6.3 Fuzzer Path
```
STM32 Fuzzer Engine → bxCAN → ISO1050 → TJA1050 → CAN Bus
   ↑ feedback: bxCAN RX captures responses → mutation engine → next frame
```

## 7. Bus Topology

| Bus | Master | Slaves | Protocol | Speed |
|---|---|---|---|---|
| bxCAN #1 | STM32 (bxCAN) | TJA1050 CH1 | CAN 2.0B | Up to 1 Mbps |
| bxCAN #2 | STM32 (bxCAN) | TJA1050 CH2 | CAN 2.0B | Up to 1 Mbps |
| SPI1 | STM32 (master) | nRF52840 (slave) | SPI Mode 0 | 8 Mbps |
| SDIO | STM32 (master) | microSD card | SDIO 4-bit | 25 MHz |
| USB OTG FS | STM32 (device) | USB Host (PC) | USB 2.0 FS | 12 Mbps |
| I2C1 | STM32 (master) | AT24C02 (EEPROM) | I²C | 400 kHz |
| SWD | External debugger | STM32 | SWD | 4 MHz |

### Memory Map
- **STM32F407VGT6**: 1 MB Flash, 192 KB SRAM (128 KB + 64 KB CCM)
- **nRF52840**: 1 MB Flash, 256 KB RAM (runs BLE stack independently)
- **AT24C02**: 2 Kb (256 bytes) I²C EEPROM — stores device config & pairing keys
- **microSD**: Up to 32 GB FAT32 — session logs & replay scripts

## 8. Mechanical & Form Factor

- PCB: 65 mm × 35 mm, 4-layer, 1.6 mm thickness
- Enclosure: Custom 3D-printed OBD-II dongle shell (snap-fit)
- OBD-II connector: Right-angle 16-pin male (Molex 345000216)
- All components on top side; bottom side has ground pour and test points
- 4× RGB LEDs (WS2812B-mini) on PCB edge for status indication
- microSD slot on PCB edge (bottom-facing, spring-eject)
- USB-C on PCB edge (top-facing)
- SWD header: 0.1" 5-pin (GND, VCC, SWDIO, SWCLK, RESET)