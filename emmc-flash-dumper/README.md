# eMMC Flash Dumper — Forensic Flash Acquisition Platform

A portable, self-contained hardware platform for direct, hardware-level extraction of raw flash memory from embedded devices, IoT endpoints, smartphones, automotive ECUs, and industrial controllers.

## Overview

The eMMC Flash Dumper connects directly to flash memory ICs — eMMC, raw NAND, or SPI NOR — and performs a bit-for-bit readout at the electrical level. This makes it immune to OS-level anti-forensics, locked bootloaders, and filesystem encryption that operates above the block layer.

### Key Features

- **eMMC 5.1 HS400**: 8-bit DDR @ 200 MHz, up to 400 MB/s read speed
- **ONFI 4.0 NAND**: FPGA-assisted timing controller with programmable parameters
- **Quad SPI NOR**: 100 MHz SDR, SFDP auto-detection
- **USB 3.0 SuperSpeed**: 5 Gbps bulk transfer to host PC
- **Standalone Mode**: Direct write to microSD card (UHS-I SDR104)
- **512 MB DDR3L Buffer**: Decouples flash readout from USB transfer speed
- **SHA-256 Hardware Acceleration**: On-the-fly hash verification
- **OLED Display + Buttons**: Standalone operation without host PC
- **Credit Card Form Factor**: 85 × 54 mm, battery-powered (LiPo 2000 mAh)
- **Under $100 BOM**: Aggressive cost target using commodity components

## Specifications

| Parameter | Value |
|-----------|-------|
| MCU | STM32H743VI Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM |
| FPGA | Lattice iCE40UP5K (5280 LUTs, 120 kbit BRAM) |
| SDRAM | Micron MT41K256M16TW-107 512 MB DDR3L |
| USB PHY | Microchip USB3320C-EZK ULPI |
| PMIC | TI TPS6521815 (5× DCDC + 2× LDO) |
| Charger | TI BQ25896 (LiPo charge + OTG boost) |
| Display | SSD1306 128×64 OLED (I2C) |
| PCB | 4-layer, 85×54 mm, ENIG finish |
| Battery | 2000 mAh LiPo, ~2.5 hours active |
| Interfaces | USB-C, microSD, 20-pin ISP header, eMMC BGA adapter |

### Flash Interface Performance

| Interface | Max Speed | Bus Width | Protocol |
|-----------|-----------|-----------|----------|
| eMMC 5.1 | 400 MB/s | 8-bit DDR | JESD84-B51 HS400 |
| NAND Flash | 80 MB/s | 8-bit async | ONFI 4.0 / Toggle 2.0 |
| SPI NOR | 25 MB/s | Quad SPI | 100 MHz SDR |

## Directory Structure

```
emmc-flash-dumper/
├── README.md                          # This file
├── phase1_conceptual_architecture.md  # System architecture & design
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists
├── phase3_pcb_blueprints_layout.md    # PCB stackup, routing, thermal
├── phase4_software_stack.md          # Firmware architecture & drivers
├── kicad/
│   ├── device.kicad_pro              # KiCad project file
│   ├── device.kicad_sch              # Full schematic
│   └── device.kicad_pcb              # PCB layout
├── firmware/
│   ├── Makefile                      # Cross-compile build system
│   ├── main.c                        # Entry point, main loop, task dispatch
│   ├── board.h                       # Pin definitions & board config
│   ├── registers.h                   # Complete MMIO register map
│   ├── usb_descriptors.h             # USB 3.0 device descriptors
│   └── drivers/
│       ├── emmc.h / emmc.c           # eMMC 5.1 HS400 driver
│       └── fpga.h / fpga.c           # FPGA interface & NAND controller
└── app/
    ├── package.json                  # React Native dependencies
    ├── App.js                        # Navigation & app entry
    ├── screens/
    │   ├── AcquisitionScreen.js      # Flash acquisition control UI
    │   ├── AnalysisScreen.js         # Hex viewer & partition analysis
    │   ├── DeviceInfoScreen.js       # Device info & diagnostics
    │   └── SettingsScreen.js         # App & device configuration
    ├── components/
    │   └── DeviceContext.js          # Device state management
    └── utils/
        └── protocol.js              # Binary wire protocol (CRC, framing)
```

## Quick Start

### Firmware Build

```bash
cd firmware/
make clean
make -j$(nproc)
# Flash via ST-Link:
make flash
# Or via OpenOCD:
make flash_openocd
```

### FPGA Bitstream Build

```bash
cd ../fpga/
make clean
make
# Bitstream is embedded into firmware automatically
```

### Companion App

```bash
cd app/
npm install
npx react-native start
# For Android:
npx react-native run-android
```

### KiCad PCB

```bash
# Open in KiCad 8.0
kicad kicad/device.kicad_pro
# Generate gerbers: File → Fabrication Outputs → Gerbers
```

## USB Protocol

The device uses a custom binary protocol over USB 3.0 bulk endpoints:

- **Command Frame**: SYNC(4) + CMD(2) + LEN(4) + SEQ(2) + PAYLOAD + CRC16(2)
- **Data Stream Frame**: SYNC(4) + SEQ(4) + DATA(N) + CRC32(4)
- **Sync Word (Command)**: `0xFD4D4644` ("FDMD")
- **Sync Word (Data)**: `0xFD444154` ("FDAT")
- **CRC-16**: XMODEM polynomial (0x1021)
- **CRC-32**: MPEG2 polynomial

See `app/utils/protocol.js` for the full JavaScript implementation.

## BOM Summary

| Assembly | Cost |
|----------|------|
| Main PCB | ~$58.00 |
| eMMC BGA-153 Adapter | ~$22.62 |
| NAND TSOP-48 Adapter | ~$7.50 |
| SPI NOR SOIC-8 Clip | ~$12.30 |
| LiPo Battery (2000 mAh) | ~$8.00 |
| 3D Printed Enclosure | ~$3.00 |
| **TOTAL** | **~$111.42** |

## Target Flash Interfaces

### eMMC (via BGA-153 adapter)
- Connect eMMC chip to BGA adapter board
- Adapter connects to main PCB via 20-pin 0.05" Samtec header
- Supports 1.8V and 3.3V signaling (auto-detected)
- HS400 mode for maximum throughput

### NAND Flash (via ISP header or TSOP-48 adapter)
- Connect NAND chip to TSOP-48 ZIF socket adapter
- Adapter connects to 20-pin 0.1" ISP header
- FPGA provides programmable timing for any ONFI/Toggle device
- Raw page read includes OOB/spare area

### SPI NOR (via SOIC-8 test clip)
- Pomona 5250 test clip for in-system programming
- Connects to ISP header (shared with NAND pins)
- Quad SPI mode for maximum throughput
- SFDP auto-detection

## Power Architecture

```
USB 5V → BQ25896 Charger → VSYS (3.5-5V) → TPS6521815 PMIC
                                              ├─ DCDC1: 1.2V (VCORE)
                                              ├─ DCDC2: 3.3V (VDDIO)
                                              ├─ DCDC3: 1.35V (DDR3L)
                                              ├─ DCDC4: 1.8V (eMMC/NAND I/O)
                                              ├─ LDO1: 1.0V (USB PHY)
                                              └─ LDO2: 1.8V (FPGA VCCIO)
```

## Safety & Legal

This device is designed for **legitimate security research, forensic investigation, data recovery, and educational purposes**. Users are responsible for complying with all applicable laws regarding device tampering, data access, and privacy.

## License

- **Hardware** (KiCad files, schematics, PCB): CERN Open Hardware License v2 - Strongly Reciprocal (CERN-OHL-S-2.0)
- **Firmware** (C source code): MIT License
- **Companion App** (React Native): MIT License
- **FPGA Gateware** (Verilog): MIT License
- **Documentation**: Creative Commons Attribution 4.0 (CC-BY-4.0)

## Credits

Designed and developed by **Nous Research**, 2026.

Built with:
- STM32CubeH7 (STMicroelectronics)
- Yosys + nextpnr-ice40 + IceStorm (open-source FPGA toolchain)
- KiCad 8.0 (open-source EDA)
- React Native (Meta)
