# BLE Phantom — Dual BLE Security Research Platform

```
    ╔══════════════════════════════════════════╗
    ║   ██████╗ ██╗      █████╗ ███╗   ██╗     ║
    ║   ██╔══██╗██║     ██╔══██╗████╗  ██║     ║
    ║   ██████╔╝██║     ███████║██╔██╗ ██║     ║
    ║   ██╔═══╝ ██║     ██╔══██║██║╚██╗██║     ║
    ║   ██║     ███████╗██║  ██║██║ ╚████║     ║
    ║   ╚═╝     ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝     ║
    ║                                            ║
    ║   Dual BLE 5.x Security Research Platform  ║
    ╚══════════════════════════════════════════╝
```

## Overview

BLE Phantom is a pocket-sized dual-radio Bluetooth Low Energy security research platform that enables comprehensive offensive and defensive analysis of BLE 5.x ecosystems. With two independent nRF52832 BLE radios controlled by an STM32F401 Cortex-M4F MCU, it provides simultaneous sniffing, spoofing, and active man-in-the-middle capabilities — all communicating with a companion app over USB CDC.

**Key Capabilities:**
- 🔍 **Dual-radio BLE sniffing** — Simultaneous capture on different channels
- ⚡ **Active MITM relay** — Intercept and modify BLE traffic in real time
- 🎭 **Address spoofing** — Rapid rotation of random/identity addresses
- 🔑 **Pairing capture** — Record LE Secure Connections & Legacy exchanges
- 📡 **GATT fuzzing** — Automated mutation of read/write operations
- 🔄 **Replay attacks** — Record and replay advertising/data PDUs
- 📊 **Channel analysis** — RSSI monitoring and channel occupancy mapping

## Specifications

| Parameter | Value |
|-----------|-------|
| MCU | STM32F401CCU6 (Cortex-M4F, 84 MHz, 256 KB Flash, 64 KB SRAM) |
| BLE Radio A | nRF52832-QFAA (BLE 5.x, +4 dBm, −96 dBm sensitivity) |
| BLE Radio B | nRF52832-QFAA (BLE 5.x, +4 dBm, −96 dBm sensitivity) |
| Interface | USB 2.0 Full-Speed CDC (virtual serial) |
| Power | USB bus-powered (5 V, 500 mA) or 3.7 V LiPo (600 mAh) |
| Form Factor | 80 × 40 mm USB-A dongle |
| BOM Cost | ~$14 (qty 100) |
| Weight | ~15 g |

## Directory Structure

```
ble-phantom/
├── README.md
├── phase1_conceptual_architecture.md    # System design, block diagram, data flow
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists
├── phase3_pcb_blueprints_layout.md      # PCB stackup, routing, thermal
├── phase4_software_stack.md             # Boot, drivers, firmware architecture
├── kicad/
│   ├── device.kicad_pro                 # KiCad project file
│   ├── device.kicad_sch                 # Full schematic
│   └── device.kicad_pcb                 # Board layout
├── firmware/
│   ├── Makefile                         # Cross-compile build system
│   ├── main.c                           # Main firmware with event loop
│   ├── board.h                          # Pin definitions and hardware config
│   ├── registers.h                      # STM32F401 MMIO register definitions
│   ├── usb_descriptors.h                # USB CDC device descriptors
│   ├── stm32f401.ld                     # Linker script
│   └── drivers/
│       ├── spi_radio.h                  # SPI radio driver header
│       ├── spi_radio.c                  # SPI radio driver implementation
│       ├── usb_cdc.h                    # USB CDC driver header
│       └── usb_cdc.c                    # USB CDC driver implementation
└── app/
    ├── package.json                     # React Native project config
    ├── App.js                           # Main app with tab navigation
    ├── screens/
    │   ├── DashboardScreen.js           # Device status and mode control
    │   ├── SnifferScreen.js             # Packet capture and display
    │   ├── MITMScreen.js                # MITM relay configuration
    │   └── SettingsScreen.js            # Device configuration
    ├── components/
    │   └── ConnectionContext.js          # USB serial connection context
    └── utils/
        └── protocol.js                  # Binary protocol definitions and parsers
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Build firmware
cd ble-phantom/firmware
make clean && make

# Flash via ST-Link
make flash

# Or flash via USB DFU
make dfu
```

### Companion App

```bash
# Install dependencies
cd ble-phantom/app
npm install

# Run on Android (USB OTG connected to device)
npx react-native run-android

# Run on iOS (USB Lightning adapter)
npx react-native run-ios
```

### nRF52832 Radio Firmware

Each nRF52832 requires custom SPI-slave firmware. Build using nRF5 SDK v17.x:

```bash
cd nrf_radio_fw/
make radio_a  # Build for Radio A
make radio_b  # Build for Radio B

# Flash via SWD (nRF Connect Programmer or OpenOCD)
openocd -f interface/stlink-v2.cfg -f target/nrf52.cfg \
  -c "program radio_a.hex verify reset exit"
```

## Architecture

```
┌──────────────────────────────────────────────────────┐
│  Host App (React Native)                            │
│  ┌──────────┐ ┌──────────┐ ┌───────┐ ┌──────────┐  │
│  │Dashboard │ │ Sniffer  │ │ MITM  │ │ Settings │  │
│  └────┬─────┘ └────┬─────┘ └───┬───┘ └────┬─────┘  │
│       └────────────┼───────────┼──────────┘         │
│                    │           │                     │
│            ┌───────┴───────────┴───────┐            │
│            │   USB CDC Serial Bridge   │            │
│            └───────────┬───────────────┘            │
└────────────────────────┼───────────────────────────┘
                         │ USB
┌────────────────────────┼───────────────────────────┐
│  BLE Phantom Hardware  │                           │
│               ┌────────┴────────┐                  │
│               │   STM32F401CCU6  │                  │
│               │   (Main MCU)     │                  │
│               └──┬──────────┬────┘                  │
│          SPI0    │          │    SPI1                │
│          ┌───────┴──┐   ┌──┴───────┐               │
│          │ nRF52832  │   │ nRF52832 │               │
│          │ Radio A   │   │ Radio B  │               │
│          └────┬──────┘   └──────┬───┘               │
│               │ Ant A           │ Ant B            │
│               └────────────────┘                   │
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌───────┐            │
│  │MCP73831  │  │TLV1117   │  │ LEDs  │            │
│  │(Charger) │  │(3V3 LDO) │  │×4     │            │
│  └──────────┘  └──────────┘  └───────┘            │
└─────────────────────────────────────────────────────┘
```

## Operating Modes

| Mode | Radio A | Radio B | Description |
|------|---------|---------|-------------|
| Idle | Off | Off | Low power standby |
| Sniff | Scanning | Off | Passive BLE packet capture |
| Scan | Scanning | Scanning | Dual-channel discovery |
| Advertise | Advertising | Scanning | Emulate peripheral + listen |
| Connect | Connected | Off | Connect to target device |
| MITM | Connected | Connected | Dual-radio relay between devices |
| Replay | TX | Off | Replay captured packets |

## Protocol

Communication between the host app and BLE Phantom uses a binary protocol over USB CDC:

```
Frame: [0xAA][0x55][CMD][LEN_H][LEN_L][PAYLOAD...][CRC_H][CRC_L][0x55][0xAA]
```

See `app/utils/protocol.js` for complete protocol definitions.

## Security Research Use Cases

1. **BLE Sniffing** — Capture all advertising, scan request/response, and data channel traffic on any BLE channel
2. **Connection Hijacking** — Establish parallel connections to both sides of a BLE link and relay traffic
3. **GATT Fuzzing** — Systematically test GATT services with malformed read/write requests
4. **Privacy Analysis** — Track address rotation and extract IRKs from pairing exchanges
5. **Replay Attacks** — Record and replay BLE PDUs to test for replay vulnerability
6. **OTA Firmware Interception** — Capture and analyze DFU/OTA update traffic
7. **Jamming Detection** — Monitor RSSI patterns to detect or characterize jamming

## Legal Disclaimer

BLE Phantom is designed for **authorized security research and education only**. Unauthorized interception of Bluetooth communications may violate local laws (e.g., CFAA, ECPA, Computer Misuse Act). Always obtain proper authorization before testing. The authors assume no liability for misuse.

## License

Hardware design files: **CERN-OHL-S v2** (Strongly Reciprocal)
Firmware: **GPL-3.0-or-later**
Companion app: **MIT License**

## Bill of Materials (Summary)

| Component | Part Number | Qty | Unit Price |
|-----------|------------|-----|-----------|
| MCU | STM32F401CCU6 | 1 | $2.80 |
| BLE Radio | nRF52832-QFAA | 2 | $3.50 |
| LiPo Charger | MCP73831T-2ACI/OT | 1 | $0.45 |
| LDO | TLV1117LV33DCYR | 1 | $0.50 |
| USB ESD | PRTR5V0U2X,215 | 1 | $0.20 |
| Antenna | 2450AT18A100E | 2 | $0.30 |
| **Total** | | | **~$14** |