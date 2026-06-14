# CAN Bus Infiltrator

> 🔧 A pocket-sized automotive CAN bus security research platform for real-time sniffing, injection, fuzzing, and replay of CAN frames via OBD-II.

![License](https://img.shields.io/badge/license-CERN--OHL--S--2-blue)
![BOM](https://img.shields.io/badge/BOM-%2445-green)
![MCU](https://img.shields.io/badge/MCU-STM32F407VGT6-orange)
![BLE](https://img.shields.io/badge/BLE-nRF52840-blue)

## 🚗 Overview

The **CAN Bus Infiltrator** is a dual-channel CAN bus security research tool that connects to a vehicle's OBD-II diagnostic port. It provides real-time monitoring, frame injection, intelligent fuzzing, and session capture/replay — all controlled wirelessly via BLE or tethered via USB-C.

### Key Features

| Feature | Description |
|---|---|
| **Dual CAN Channels** | Monitor HS-CAN and MS-CAN simultaneously (OBD-II pins 6/14 & 3/11) |
| **Galvanic Isolation** | 2.5 kV digital isolators (ISO1050) protect the MCU from vehicle electrical faults |
| **Passive Sniffing** | Listen-only mode captures all traffic with 1 µs hardware timestamps |
| **Frame Injection** | Craft and send arbitrary CAN frames (11-bit and 29-bit IDs) |
| **Intelligent Fuzzing** | Random, bit-flip, arithmetic, and byte-mutation fuzzing strategies |
| **Capture & Replay** | Record sessions to microSD, replay with faithful timing |
| **Protocol Decoding** | Real-time OBD-II, UDS, and SAE J1939 dissection |
| **BLE 5.0 Control** | Wireless command/control from the companion mobile app |
| **USB-C CDC** | Direct PC control at ≥1 Mbps for high-throughput logging |
| **4× RGB LEDs** | Visual status: green=idle, blue=sniffing, red=injection, orange=fuzzing |

## 📊 Specifications

| Parameter | Value |
|---|---|
| MCU | STM32F407VGT6 (Cortex-M4 @ 168 MHz, 1 MB Flash, 192 KB RAM) |
| BLE SoC | nRF52840 (BLE 5.0, 1 MB Flash, 256 KB RAM) |
| CAN Transceivers | 2× TJA1050 (ISO 11898-2 compliant, up to 1 Mbps) |
| Digital Isolators | 2× ISO1050 (2.5 kV galvanic isolation) |
| Max CAN Speed | 1 Mbps per channel |
| Capture Rate | ≥ 10,000 frames/s per channel |
| Injection Latency | ≤ 50 µs from trigger to bus |
| Timestamp Resolution | 1 µs (TIM2 hardware) |
| BLE Throughput | ≥ 200 kbps |
| USB Throughput | ≥ 1 Mbps |
| Power | OBD-II (8–16 V) or USB-C (5 V), ≤ 500 mA @ 5 V |
| PCB Size | 65 mm × 35 mm × 1.6 mm, 4-layer |
| BOM Cost | ~$29.55 @ 100 units (under $45 at 10 units) |
| Operating Temp | –40 °C to +85 °C |

## 📁 Directory Structure

```
can-bus-infiltrator/
├── README.md
├── phase1_conceptual_architecture.md    # System purpose, block diagram, data flow
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md       # PCB stackup, routing, thermal, isolation
├── phase4_software_stack.md              # Boot, MMIO, drivers, firmware, device tree
├── kicad/
│   ├── device.kicad_pro                  # KiCad project with net classes & rules
│   ├── device.kicad_sch                 # Full schematic (all ICs, passives, connectors)
│   └── device.kicad_pcb                 # Board outline, placement, stackup
├── firmware/
│   ├── Makefile                          # ARM cross-compile toolchain
│   ├── main.c                            # Main application loop, fuzzer, command processor
│   ├── board.h                           # Pin definitions, macros, helper functions
│   ├── registers.h                       # STM32F407 peripheral register definitions
│   ├── usb_descriptors.h                 # USB CDC device descriptors (VID 0x1209)
│   └── drivers/
│       ├── can.h / can.c                 # Dual-channel bxCAN driver
│       ├── ble_spi.h / ble_spi.c         # SPI interface to nRF52840 BLE module
│       ├── sdcard.h / sdcard.c            # SDIO microSD card driver with CAN logging
│       ├── i2c_eeprom.h / i2c_eeprom.c  # AT24C02 EEPROM config storage
│       └── ws2812b.h / ws2812b.c         # WS2812B RGB LED driver
└── app/
    ├── package.json                      # React Native dependencies
    ├── App.js                            # Navigation (4-tab bottom nav)
    ├── screens/
    │   ├── SnifferScreen.js              # Real-time CAN frame viewer with filtering
    │   ├── InjectScreen.js               # CAN frame crafting & injection
    │   ├── FuzzerScreen.js               # Fuzzing mode selector & progress
    │   └── DeviceScreen.js               # BLE connection, settings, LED, DFU
    ├── components/
    │   └── FrameDecoder.js               # OBD-II / J1939 / UDS protocol decoder
    └── utils/
        └── protocol.js                    # Binary protocol encoder/decoder & BLE handler
```

## 🏁 Quick Start

### Firmware

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi

# Build firmware
cd firmware
make clean && make

# Flash via ST-Link
make flash

# Or flash via USB DFU
make dfu
```

### Companion App

```bash
cd app
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios
```

### Connecting to a Vehicle

1. Plug the CAN Bus Infiltrator into the vehicle's OBD-II port
2. The device powers on automatically (green LEDs)
3. Open the companion app and connect via BLE
4. Select "Sniffer" tab → choose channel and baud rate → tap "SNIFF"
5. CAN frames stream in real-time to your phone

### USB-CDC Control

```bash
# Connect via USB-C, device appears as /dev/ttyACM0
stty -F /dev/ttyACM0 115200 raw

# Start sniffing on CH1 at 500K
echo -ne '\xAA\x01\x00\x00\x02\x00\x00\x03' > /dev/ttyACM0

# Inject a frame
echo -ne '\xAA\x03\x01\x00\x0D\x7D\x00\x08\x02\x01\x00\x00\x00\x00\x00\x00\x00\x8C' > /dev/ttyACM0
```

## 🔌 OBD-II Pin Mapping

| Pin | Net | Function |
|---|---|---|
| 3 | CAN2_H | MS-CAN High |
| 4 | GND | Chassis ground |
| 5 | GND_ISO | Signal ground (isolated) |
| 6 | CAN1_H | HS-CAN High |
| 10 | CAN2_L | MS-CAN Low |
| 14 | CAN1_L | HS-CAN Low |
| 16 | VBAT | Battery (12 V nominal) |

## ⚡ Power Tree

```
OBD-II Pin 16 (12 V) ──▶ MP2307DN Buck (12V→5V) ──┬──▶ ISO1050 side (5V)
                                                       │
USB-C VBUS (5 V) ──▶ Schottky OR ──▶ RT9193 LDO (5V→3.3V) ──┬──▶ STM32 (3.3V)
                                                                  ├──▶ nRF52840 (3.3V)
                                                                  ├──▶ microSD (3.3V)
                                                                  └──▶ EEPROM (3.3V)
```

## 🛡️ Security Features

- **Signed firmware updates**: Ed25519 signature verification in bootloader
- **BLE passkey authentication**: Secure pairing with bond storage
- **Galvanic isolation**: 2.5 kV between vehicle CAN domain and digital domain
- **ESD protection**: ±8 kV contact discharge on all external interfaces (IEC 61000-4-2)
- **Read protection**: STM32 option bytes Level 1 (no debug access to flash)

## 📜 Licenses

| Component | License |
|---|---|
| Hardware (KiCad schematics, PCB layout) | CERN-OHL-S-2 |
| Firmware (C code) | MIT |
| Companion App (React Native) | MIT |
| Documentation (Phase 1–4) | CC-BY-SA-4.0 |

---

**⚠️ DISCLAIMER**: This device is a security research tool intended for authorized testing only. Unauthorized access to vehicle CAN buses may be illegal. Always obtain proper authorization before connecting to any vehicle's network. The authors assume no liability for misuse.