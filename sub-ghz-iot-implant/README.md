# Substation — Sub-GHz IoT Gateway Implant

A portable, pocket-sized multi-protocol Sub-GHz attack platform for security researchers and red teamers. Targets Zigbee, Z-Wave, proprietary Sub-GHz protocols (433/868/915 MHz), and provides BLE backhaul for operator control.

> **⚠️ Legal Notice:** This device is designed for authorized security research and penetration testing only. Transmitting on licensed frequencies without authorization is illegal in most jurisdictions. Always comply with local regulations.

## Specifications

| Feature | Specification |
|---------|--------------|
| **MCU** | TI CC1352P1F3RGZ (ARM Cortex-M4F @ 48 MHz) |
| **Sub-GHz Radio** | 281–1660 MHz, OOK/ASK/FSK/GFSK/O-QPSK, -110 dBm RX sensitivity, +20 dBm TX |
| **2.4 GHz Radio** | BLE 5.0 (operator backhaul), 0 dBm TX |
| **Memory** | 1 MB Flash, 128 KB SRAM (internal), 64 MB SDRAM (external), 16 MB SPI Flash |
| **Security** | ATECC608A hardware crypto (ECDSA, ECDH, SHA-256, secure boot) |
| **Connectivity** | USB-C (data + charge), BLE 5.0, UART debug |
| **Storage** | MicroSD slot, 16 MB SPI Flash |
| **Antenna** | Sub-GHz SMA (external), 2.4 GHz inverted-F PCB trace |
| **Power** | USB-C 5V or 1000 mAh LiPo (12+ hours sniff, 4+ hours active TX) |
| **Status** | WS2812B RGB LED, 2 tactile buttons |
| **Form Factor** | 85 mm × 54 mm (credit card footprint) |
| **BOM Cost** | ~$22.50 (1K volume), < $85 assembled |

## Attack Capabilities

- **Passive Sniff**: Capture all Sub-GHz traffic (Zigbee, Z-Wave, OOK/ASK, FSK)
- **Active Replay**: Re-transmit captured frames with precise timing
- **Zigbee MITM**: Impersonate coordinator/router, intercept and modify frames
- **Z-Wave Sniff/Inject**: Capture and inject Z-Wave frames
- **Rolling Code Analysis**: Capture and analyze rolling-code protocols
- **Fuzz/Inject**: Generate malformed protocol frames for vulnerability research
- **BLE Control**: Real-time packet streaming and device control via companion app

## Directory Structure

```
sub-ghz-iot-implant/
├── README.md                              # This file
├── phase1_conceptual_architecture.md      # System purpose, block diagram, data flow
├── phase2_component_selection_schematics.md # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md        # Stackup, routing, thermal, DFM
├── phase4_software_stack.md               # Boot, drivers, DMA, device tree, build
├── kicad/
│   ├── device.kicad_pro                   # KiCad project file
│   ├── device.kicad_sch                   # Full schematic
│   └── device.kicad_pcb                   # Board layout & stackup
├── firmware/
│   ├── Makefile                            # Cross-compile for ARM Cortex-M4F
│   ├── main.c                              # Main application & task scheduler
│   ├── board.h                             # Hardware definitions & pin mappings
│   ├── registers.h                         # MMIO register definitions
│   ├── usb_descriptors.h                   # USB CDC-ECM + CDC-ACM descriptors
│   └── drivers/
│       ├── radio_subghz.h                  # Sub-GHz radio driver header
│       ├── radio_subghz.c                  # Multi-protocol radio implementation
│       ├── sdram.h                          # SDRAM ring buffer driver header
│       ├── sdram.c                          # SDRAM controller with DMA
│       ├── ble_comms.h                      # BLE communication driver header
│       ├── ble_comms.c                      # BLE GATT service & streaming
│       ├── spi_flash.h                      # SPI Flash driver header
│       ├── spi_flash.c                      # MX25LW1636 flash operations
│       ├── atecc608a.h                      # ATECC608A crypto driver header
│       └── atecc608a.c                      # Secure boot & BLE auth
└── app/
    ├── package.json                         # React Native dependencies
    ├── App.js                               # Navigation & entry point
    ├── screens/
    │   ├── DeviceScreen.js                  # Connection status & device info
    │   ├── CaptureScreen.js                  # Live packet capture & filtering
    │   └── SettingsScreen.js                # Radio configuration & modes
    ├── components/
    │   ├── BleContext.js                    # BLE state management
    │   ├── StatusCard.js                    # Reusable card component
    │   └── PacketList.js                    # Packet list component
    └── utils/
        └── protocol.js                      # Protocol decoding & analysis
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Build firmware
cd firmware/
make TARGET=cc1352p

# Flash via OpenOCD + CMSIS-DAP
make flash

# Or flash via TI UniFlash
./uniflash.sh -cc1352p -program build/substation.hex
```

### Companion App

```bash
cd app/
npm install

# Android
npx react-native run-android

# iOS
npx react-native run-ios
```

### KiCad

```bash
cd kicad/
kicad-cli netlist device.kicad_sch -o device.net
kicad-cli drc device.kicad_pcb
kicad-cli pcb export gerbers device.kicad_pcb -o gerbers/
```

## Hardware Connections

### SWD Debug Header (0.1" pitch)

| Pin | Signal |
|-----|--------|
| 1 | VDD (3.3V) |
| 2 | SWD_CLK |
| 3 | GND |
| 4 | SWD_IO |
| 5 | GND |
| 6 | SWD_TDI |
| 7 | GND |
| 8 | SWD_TDO |
| 9 | GND |
| 10 | RESET |

### UART Debug (115200 8N1)

| Pin | Signal |
|-----|--------|
| TX | DIO_24 |
| RX | DIO_25 |

### Expansion Header (6-pin FPC)

| Pin | Signal |
|-----|--------|
| 1 | VDD (3.3V) |
| 2 | UART1_TX |
| 3 | UART1_RX |
| 4 | GPIO |
| 5 | GPIO |
| 6 | GND |

## Block Diagram

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Antenna   │    │  CC1352P    │    │   ATECC608A │
│   (SMA +    │◄──►│  SoC + RF   │◄──►│   Crypto    │
│   PCB BLE)  │    │  48 MHz M4F │    │   I²C       │
└─────────────┘    └──────┬──────┘    └─────────────┘
                          │
               ┌──────────┼──────────┐
               │          │          │
        ┌──────┴──┐  ┌───┴───┐  ┌──┴──────┐
        │64 MB    │  │16 MB  │  │MicroSD  │
        │SDRAM   │  │Flash  │  │Slot     │
        │(x2)    │  │MX25   │  │via TXB  │
        └─────────┘  └───────┘  └─────────┘
                          │
               ┌──────────┼──────────┐
               │          │          │
        ┌──────┴──┐  ┌───┴───┐  ┌──┴──┐
        │USB-C    │  │LiPo  │  │WS2812│
        │Data+Pwr │  │Charger│  │ LED │
        └─────────┘  └───────┘  └──────┘
```

## Power Budget

| Mode | Current | Duration (1000 mAh) |
|------|---------|---------------------|
| Sniff (RX only) | ~25 mA | 40 hrs (theoretical), 12+ hrs (conservative) |
| Active TX (20 dBm) | ~130 mA | 4+ hrs (with duty cycling) |
| BLE advertising | ~2 mA | 500+ hrs |
| Standby (deep sleep) | ~1 µA | Years |

## Licenses

- **Hardware**: CERN-OHL-S v2 (Strongly Reciprocal)
- **Firmware**: GNU GPL v3
- **Companion App**: MIT License
- **Documentation**: Creative Commons BY-SA 4.0

## Credits

Designed as part of the [Hacker Devices](https://github.com/jayis1/hacker-devices) project.