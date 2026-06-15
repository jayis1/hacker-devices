# USB DMA Phantom — Thunderbolt/USB4 DMA Attack Platform

```
    ╔══════════════════════════════════════════════════╗
    ║   ██╗   ██╗██████╗ ██╗      █████╗ ███████╗     ║
    ║   ██║   ██║██╔══██╗██║     ██╔══██╗██╔════╝     ║
    ║   ██║   ██║██████╔╝██║     ███████║███████╗     ║
    ║   ██║   ██║██╔═══╝ ██║     ██╔══██║╚════██║     ║
    ║   ╚██████╔╝██║     ███████╗██║  ██║███████║     ║
    ║    ╚═════╝ ╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝     ║
    ║                                                    ║
    ║   Thunderbolt/USB4 DMA Security Research Platform   ║
    ╚════════════════════════════════════════════════════╝
```

## Overview

**USB DMA Phantom** is a pocket-sized Thunderbolt/USB4 Direct Memory Access (DMA) attack platform designed for authorized security researchers and red team operators. It presents as a legitimate PCIe peripheral over Thunderbolt/USB4 to gain DMA access to a target host's physical memory, enabling credential extraction, kernel exploitation, and IOMMU bypass — all controlled via an encrypted BLE companion app.

Unlike HID-layer BadUSB devices, USB DMA Phantom operates at the PCIe level, exploiting the fundamental trust model where Thunderbolt and USB4 hosts grant peripheral DMA access before IOMMU initialization.

## Key Capabilities

| Capability | Description |
|-----------|-------------|
| **DMA Read** | Read arbitrary host physical memory (credentials, keys, processes) |
| **DMA Write** | Write to host physical memory (shellcode injection, patching) |
| **Memory Scan** | Search 4+ GB of host memory for patterns (signatures, keys) |
| **Shellcode Inject** | Inject and execute code in kernel or user-mode address space |
| **IOMMU Bypass** | Exploit misconfigured/unpatched IOMMU on Thunderbolt hosts |
| **VID/PID Spoofing** | Emulate legitimate PCIe devices (Realtek NIC, Intel GPU, etc.) |
| **Payload Library** | Pre-loaded attack payloads stored in 16 MB SPI flash |
| **Encrypted C2** | AES-256-CTR encrypted BLE 5.0 command & control |
| **USB CDC** | USB 2.0 virtual COM port for configuration and firmware updates |
| **OTA Firmware** | Firmware updates via USB DFU or BLE OTA |
| **Passive Sniff** | Monitor PCIe TLP traffic without injecting (stealth mode) |

## Specifications

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32F423CHU6, Cortex-M4F @ 120 MHz, 1.5 MB Flash, 256 KB RAM |
| **PCIe Bridge** | TI XIO2001IZWR, 1-lane Gen2, DMA-capable |
| **USB-C Mux** | TI HD3SS460, Thunderbolt/USB4/DP alternate mode |
| **BLE Radio** | nRF52832-QFAA, BLE 5.0, 64 KB RAM |
| **Storage** | W25Q128JVSIM 16 MB SPI NOR flash + microSD card slot |
| **Encryption** | AES-256-CTR (hardware-accelerated on STM32F423) |
| **Power** | Bus-powered via USB-C (5V, 900 mA) |
| **Connectivity** | USB-C 2.0 + Thunderbolt 3/4 + USB4 + BLE 5.0 |
| **Form Factor** | 65 × 30 × 12 mm (stick form) |
| **BOM Cost** | ~$40 at qty 100, ~$85 at qty 1 |
| **Boot Time** | < 1 s to operational (PCIe link: ~2 s worst case) |

## Supported Attack Targets

| Target | Vector | Impact |
|--------|--------|--------|
| Windows 10/11 (Thunderbolt) | PCIe DMA | Full physical memory R/W |
| Linux (iommu=off) | PCIe DMA | Credential extraction |
| macOS FileVault 2 | PCIe DMA before login | Volume key extraction |
| BitLocker (TPM-only) | PCIe DMA before OS IOMMU | Volume key extraction |
| Unpatched Thunderbolt | CVE-2019-0090 etc. | IOMMU bypass → code exec |
| USB4 hosts | PCIe tunnel over USB4 | Same as Thunderbolt |

## Directory Structure

```
usb-dma-phantom/
├── README.md                              ← This file
├── phase1_conceptual_architecture.md      ← System design, block diagram, data flow
├── phase2_component_selection_schematics.md ← BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md        ← Stackup, routing, thermal, isolation
├── phase4_software_stack.md               ← Boot, MMIO, drivers, device tree
├── kicad/
│   ├── device.kicad_pro                   ← KiCad project with net classes & rules
│   ├── device.kicad_sch                  ← Full schematic
│   └── device.kicad_pcb                  ← Board outline, placement, stackup
├── firmware/
│   ├── Makefile                           ← Cross-compile for ARM Cortex-M4F
│   ├── main.c                            ← Main firmware entry point
│   ├── board.h                           ← Board definitions & pin assignments
│   ├── registers.h                        ← STM32F423 peripheral register map
│   ├── usb_descriptors.h                 ← USB CDC ACM + DFU descriptors
│   └── drivers/
│       ├── spi4.h                        ← SPI4 driver (flash + XIO2001)
│       ├── spi4.c                         ← SPI4 implementation with DMA
│       ├── w25q128.h                     ← W25Q128 flash driver
│       ├── w25q128.c                      ← Flash operations with HMAC verification
│       ├── xio2001.h                     ← XIO2001 PCIe bridge driver
│       └── xio2001.c                      ← DMA engine, VID/PID config, TLP sniff
└── app/
    ├── package.json                       ← React Native dependencies
    ├── App.js                             ← Navigation shell with dark theme
    ├── screens/
    │   ├── HomeScreen.js                  ← Dashboard, device status, quick actions
    │   ├── DmaControlScreen.js            ← Real-time DMA read/write/scan/inject
    │   ├── PayloadManagerScreen.js         ← Upload, manage, execute payloads
    │   └── SettingsScreen.js              ← VID/PID, stealth, firmware update
    ├── components/
    │   └── StatusBadge.js                 ← Reusable connection status indicator
    └── utils/
        └── protocol.js                    ← BLE C2 protocol, AES-256 encryption, framing
```

## Quick Start

### Firmware Build

```bash
# Install ARM GCC toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi make

# Build firmware
cd firmware/
make clean && make all

# Flash via SWD (OpenOCD)
make flash

# Flash via USB DFU
make dfu-flash
```

### Companion App

```bash
# Install dependencies
cd app/
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios

# Build release APK
cd android && ./gradlew assembleRelease
```

### Usage

1. **Connect** the USB DMA Phantom to a target host's USB-C/Thunderbolt port
2. **Pair** via the companion app over BLE ("DMA-Phantom-XXXX")
3. **Select** operating mode (Stealth DMA / Interactive / Config / Sniffer)
4. **Execute** DMA operations from the app or via USB CDC terminal

### USB CDC Command Interface

```
# Connect via USB CDC (115200 baud, 8N1)
minicom -D /dev/ttyACM0

# DMA commands (binary protocol, magic 0xD4)
# Read 256 bytes from physical address 0x1000:
# D4 01 00 00 00 00 00 00 00 00 10 00 00 01 00

# Write 4 bytes to address 0x2000:
# D4 02 01 00 00 00 00 00 00 20 00 00 04 00 DE AD BE EF
```

## Attack Workflow Example

```
1. Insert USB DMA Phantom into target's Thunderbolt port
2. Device enumerates as "Realtek RTL8111 NIC" (configurable VID/PID)
3. Target OS grants PCIe DMA access to the "network adapter"
4. Companion app connects via BLE (AES-256-CTR encrypted)
5. User selects "Credential Dump" payload from library
6. XIO2001 DMA engine reads target's SAM/LSASS memory region
7. Extracted credentials displayed in app, stored on microSD
8. User disconnects — device leaves no persistent software on target
```

## Security Considerations

- **Encryption**: All BLE C2 traffic is AES-256-CTR encrypted with session keys derived via ECDH
- **Authentication**: BLE pairing requires physical button press on device
- **Firmware protection**: STM32F423 readout protection Level 2, SWD disabled in production
- **Payload integrity**: HMAC-SHA256 verification of stored payloads before execution
- **Tamper evidence**: Secure boot via STM32F423 OTFDEC; tamper-evident epoxy over flash

⚠️ **LEGAL NOTICE**: This device is designed for AUTHORIZED security research and penetration testing ONLY. Unauthorized use against systems you do not own or have explicit written permission to test is illegal in most jurisdictions. Always follow responsible disclosure practices and comply with local laws.

## License

- **Hardware**: CERN-OHL-S v2 (Strongly Reciprocal)
- **Firmware**: MIT License
- **App**: MIT License
- **Documentation**: CC-BY-SA 4.0

## Acknowledgments

- [PCILeech](https://github.com/ufrisk/pcileech) — DMA attack research inspiration
- [Thunderclap](https://thunderclap.io/) — Thunderbolt security research
- [STM32](https://www.st.com/) — MCU and development tools
- [Texas Instruments](https://www.ti.com/) — XIO2001 PCIe bridge and HD3SS460 mux
- [Nordic Semiconductor](https://www.nordicsemi.com/) — nRF52832 BLE SoC