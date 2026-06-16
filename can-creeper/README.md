# CAN Creeper — Dual CAN FD Sniffer & Injector

A pocket-sized, battery-powered CAN bus diagnostic, passive-sniffing, and active-injection tool for automotive security research, ECU reverse engineering, and red-team physical penetration testing.

## Specifications

| Parameter | Value |
|---|---|
| **CAN Channels** | 2 independent (simultaneous sniff on both, inject on either) |
| **CAN 2.0B** | 125 kbps – 1 Mbps, 11/29-bit IDs |
| **CAN FD** | Up to 8 Mbps arbitration, 15 Mbps data phase |
| **Timestamp Resolution** | 1 µs (32-bit free-running counter) |
| **Frame Buffer** | 16,384 frames per channel (8 MB PSRAM) |
| **MCU** | nRF52840-QIAA-R7 (Cortex-M4F @ 64 MHz, 1 MB Flash, 256 KB RAM) |
| **CAN Controllers** | 2× MCP2518FD-I/SL (SPI, 31 FIFO each) |
| **CAN Transceivers** | 2× TJA1445AT/0Z (CAN FD up to 5 Mbps) |
| **Wireless** | BLE 5.2 (Nordic UART Service, up to 244-byte packets) |
| **Wired** | USB 2.0 Full Speed CDC-ACM |
| **Display** | 128×64 OLED (SSD1306, I²C) |
| **Storage** | 8 MB QSPI PSRAM (APS6404L) + 16 MB NOR Flash (W25Q128JV) |
| **Battery** | 3.7V LiPo 2000 mAh (~25 hours active sniffing) |
| **Charging** | TP4056 Li-Ion charger via USB-C |
| **Protection** | DW01-P + FS8205A battery protection, PESD1CAN-U ESD on CAN bus |
| **PCB** | 65 mm × 35 mm, 4-layer FR-4, ENIG finish |
| **BOM Cost** | ~$42.50 USD (Qty 1) |
| **Companion App** | React Native (iOS/Android) |

## Directory Structure

```
can-creeper/
├── README.md                           ← This file
├── phase1_conceptual_architecture.md   ← System architecture & block diagram
├── phase2_component_selection_schematics.md ← BOM, pinouts, netlists
├── phase3_pcb_blueprints_layout.md     ← PCB stackup, routing, thermal
├── phase4_software_stack.md            ← Boot, MMIO, drivers, build system
├── kicad/
│   ├── device.kicad_pro                ← Project file with net classes & DRC
│   ├── device.kicad_sch                ← Full schematic (300+ lines)
│   └── device.kicad_pcb                ← Board outline & component placement
├── firmware/
│   ├── Makefile                        ← Cross-compile for nRF52840
│   ├── linker.ld                       ← Linker script (in Makefile)
│   ├── main.c                          ← Full SPL init, main loop (300+ lines)
│   ├── board.h                         ← Complete pin definitions (150+ lines)
│   ├── registers.h                     ← Full MMIO register map (150+ lines)
│   └── drivers/
│       ├── can_fd_driver.h/.c          ← MCP2518FD SPI driver (200+ lines)
│       ├── psram_driver.h/.c           ← QSPI PSRAM + Flash driver (200+ lines)
│       ├── ble_nus_driver.h/.c         ← BLE NUS wrapper (150+ lines)
│       ├── usb_cdc_driver.h/.c         ← USB CDC-ACM driver (200+ lines)
│       ├── oled_driver.h/.c            ← SSD1306 OLED driver (150+ lines)
│       ├── protocol_handler.h/.c       ← Binary wire protocol (300+ lines)
│       └── injection_engine.h/.c       ← Scripted injection engine (200+ lines)
└── app/
    ├── package.json                    ← React Native dependencies
    ├── App.js                          ← Main app with navigation (100+ lines)
    ├── screens/
    │   ├── SnifferScreen.js            ← Real-time frame monitor (150+ lines)
    │   ├── InjectorScreen.js           ← Frame injection & UDS builder (150+ lines)
    │   ├── ScriptsScreen.js            ← Script management (150+ lines)
    │   └── SettingsScreen.js           ← Device configuration (150+ lines)
    ├── components/
    │   └── ConnectionIndicator.js      ← Status bar component (80+ lines)
    └── utils/
        └── protocol.js                 ← Binary protocol with CRC (300+ lines)
```

## Quick Start

### Hardware

1. Order PCBs from your preferred fab (JLCPCB, PCBWay, OSH Park) using the Gerbers generated from `kicad/`
2. Order components from the BOM in `phase2_component_selection_schematics.md`
3. Assemble (SMT stencil recommended for nRF52840 QFN-73)
4. Program firmware via SWD (Tag-Connect TC2050) or USB DFU

### Firmware

```bash
# Install toolchain
sudo apt install gcc-arm-none-eabi
# Download nRF5 SDK 17.1.0 from Nordic Semiconductor

# Build
cd firmware
make SDK_ROOT=/path/to/nrf5_sdk_17.1.0

# Flash via SWD (J-Link or nrfjprog)
make flash_swd

# Or flash via USB DFU
make flash
```

### Companion App

```bash
cd app
npm install
npx react-native start
# In another terminal:
npx react-native run-android  # or run-ios
```

## Protocol

The binary wire protocol uses a simple framed format with CRC-16/CCITT:

```
Byte 0:     Start byte (0xAA)
Byte 1:     Command ID
Byte 2-3:   Payload length (16-bit LE)
Byte 4-N:   Payload
Byte N+1-2: CRC-16/CCITT (over bytes 1 through N)
```

See `app/utils/protocol.js` and `firmware/drivers/protocol_handler.c` for full implementation.

## Key Features

- **Passive Sniffing**: Tap CAN-H/CAN-L with automatic bus detection, no ACK/error flag generation
- **Active Injection**: Craft arbitrary CAN/CAN FD frames at programmable rates
- **ISO-TP / UDS**: Send multi-frame ISO 15765-2 messages and UDS diagnostic requests
- **MITM Mode**: Insert between two bus segments, selectively forward/modify/drop frames
- **CAN FD Discovery**: Probe ECUs for CAN FD capability with BRS negotiation
- **DBC Decoding**: Upload DBC files for real-time signal decoding in the app
- **Scripted Injection**: JSON-based injection scripts for automated fuzzing and replay
- **Hardware Timestamps**: 1 µs precision on all captured frames
- **Dual Transport**: BLE 5.2 for wireless or USB CDC-ACM for wired control
- **OTA DFU**: Firmware updates over BLE or USB

## License

CAN Creeper is open source hardware and software.

- Hardware design files (KiCad): CERN Open Hardware License Version 2 - Strongly Reciprocal (CERN-OHL-S-2.0)
- Firmware (C): MIT License
- Companion App (React Native): MIT License

## Credits

Designed by Nous Research, 2026.

Built with:
- Nordic Semiconductor nRF52840 SoC and nRF5 SDK
- Microchip MCP2518FD CAN FD Controller
- NXP TJA1445 CAN FD Transceiver
- AP Memory APS6404L PSRAM
- Winbond W25Q128JV NOR Flash
- Solomon Systech SSD1306 OLED
- Texas Instruments TPS63070 and TPS7A4701 power management
