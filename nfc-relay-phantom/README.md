# NFC Relay Phantom

> 🔮 A pocket-sized, battery-powered multi-protocol NFC/RFID security research platform for sniffing, cloning, emulating, and relay-attacking 13.56 MHz NFC and 125 kHz RFID systems.

![License](https://img.shields.io/badge/license-CERN--OHL--S--2-blue) ![License](https://img.shields.io/badge/firmware-GPL--2.0-green) ![License](https://img.shields.io/badge/app-MIT-yellow) ![Status](https://img.shields.io/badge/status-design-orange)

> **⚠️ Legal Notice:** This device is designed for **authorized security research and penetration testing only**. Unauthorized interception, cloning, or relay of access credentials is illegal in most jurisdictions. Always obtain proper authorization before testing any system you do not own.

---

## 📋 Specifications

| Feature | Detail |
|---------|--------|
| **NFC Protocols** | ISO 14443A/B (Mifare, DESFire), FeliCa (NFC-F), ISO 15693 (NFC-V) |
| **RFID Protocols** | EM4100, HID Prox II, AWID, ioProx, T5577 write |
| **NFC IC** | NXP PN5180A0HN/C2 — full NFC frontend controller |
| **125 kHz IC** | EM Micro EM4095 — 125 kHz RFID reader/writer |
| **Main MCU** | STM32L4S5VIT6 — Cortex-M4 @ 120 MHz, 2MB Flash, 640KB SRAM |
| **BLE Coprocessor** | Nordic NRF52832 — BLE 5.0, dedicated radio stack |
| **Display** | SSD1306 128×64 OLED (I2C) |
| **Storage** | W25Q128JVSIQ 16MB SPI NOR Flash (capture buffer) |
| **Battery** | 3.7V 1200mAh LiPo, USB-C charging (BQ25896) |
| **Connectivity** | BLE 5.0 + USB-C CDC-ACM |
| **Form Factor** | 85 × 54 × 8 mm (credit card size) |
| **BOM Cost** | ~$38 (1K qty) |
| **Battery Life** | 4+ hours active, 48+ hours standby |

---

## 🏗 Architecture

```
┌─────────────────────────────────────────────────────┐
│                   NFC Relay Phantom                   │
│                                                       │
│  ┌──────────────┐     ┌──────────────┐              │
│  │  STM32L4S5   │     │  NRF52832     │              │
│  │  (Main MCU)  │◄───►│  (BLE Radio) │◄──► Phone    │
│  │  120MHz M4   │UART │  64MHz M4F   │              │
│  └──────┬───────┘     └──────────────┘              │
│         │                                             │
│  ┌──────┴──────┐     ┌──────────────┐               │
│  │  PN5180     │     │  EM4095      │               │
│  │  13.56MHz   │     │  125kHz      │               │
│  │  NFC Frontend│    │  RFID Reader │               │
│  └──────┬──────┘     └──────┬──────┘               │
│         │                    │                        │
│  ┌──────┴──────┐     ┌──────┴──────┐               │
│  │ 13.56MHz    │     │ 125kHz      │               │
│  │ PCB Antenna │     │ PCB Antenna │               │
│  └─────────────┘     └─────────────┘               │
│                                                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐         │
│  │ SSD1306  │  │ W25Q128  │  │ BQ25896  │         │
│  │ OLED     │  │ 16MB Flash│  │ Charger  │         │
│  └──────────┘  └──────────┘  └──────────┘         │
└─────────────────────────────────────────────────────┘
```

---

## 🎯 Operation Modes

| Mode | NFC | 125 kHz | Description |
|------|-----|---------|-------------|
| **Reader** | ✅ | ✅ | Read and display tag UIDs, ATQA, SAK, facility codes |
| **Sniffer** | ✅ | — | Capture NFC frame-level traffic between tag and reader |
| **Card Emulate** | ✅ | — | Emulate an NFC card with custom UID/parameters |
| **Clone** | — | ✅ | Read 125 kHz tag and write to blank T5577 |
| **Relay** | ✅ | — | BLE tunnel between two Phantom devices for relay attacks |
| **Mifare Attack** | ✅ | — | Nested authentication, key brute-force (Darkside) |

---

## 📁 Directory Structure

```
nfc-relay-phantom/
├── phase1_conceptual_architecture.md    # System purpose, attack surface, block diagram
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md     # PCB stackup, routing, thermal, antennas
├── phase4_software_stack.md            # Boot, MMIO, drivers, device tree, build
├── kicad/
│   ├── device.kicad_pro                # KiCad project with net classes
│   ├── device.kicad_sch                # Full schematic with all ICs and passives
│   └── device.kicad_pcb                # Board outline, placement, layer stackup
├── firmware/
│   ├── Makefile                        # ARM GCC cross-compilation
│   ├── main.c                          # Main application with state machine
│   ├── board.h                         # Pin definitions, macros, config
│   ├── registers.h                     # MMIO register definitions
│   ├── usb_descriptors.h               # USB CDC-ACM + Mass Storage descriptors
│   └── drivers/
│       ├── spi_dma.h                   # SPI driver with DMA (PN5180 + W25Q128)
│       ├── spi_dma.c                   # SPI implementation
│       ├── pn5180.h                    # PN5180 NFC controller driver
│       ├── pn5180.c                    # PN5180 implementation
│       ├── em4095.h                    # EM4095 125 kHz RFID driver
│       └── em4095.c                    # EM4095 implementation
├── app/
│   ├── package.json                    # React Native dependencies
│   ├── App.js                          # Main app with tab navigation
│   ├── screens/
│   │   ├── HomeScreen.js               # Dashboard, battery, quick actions
│   │   ├── NFCScreen.js                # NFC reader/emulator/sniffer
│   │   ├── RFID125Screen.js            # 125 kHz read/clone
│   │   ├── RelayScreen.js              # BLE relay attack setup
│   │   └── SettingsScreen.js           # Device config, firmware update
│   ├── components/
│   │   └── TagCard.js                  # Reusable tag display component
│   └── utils/
│       └── protocol.js                 # BLE GATT protocol manager
└── README.md                           # This file
```

---

## 🚀 Quick Start

### Firmware Build

```bash
# Install ARM GCC toolchain
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# Build firmware
cd nfc-relay-phantom/firmware
make clean && make -j$(nproc)

# Flash via SWD (Tag-Connect J2)
openocd -f interface/cmsis-dap.cfg -f target/stm32l4s5.cfg \
    -c "program build/nfc_phantom.elf verify reset exit"

# Or flash via USB DFU (hold SW2 during reset)
dfu-util -a 0 -s 0x08000000:leave -D build/nfc_phantom.bin
```

### Companion App

```bash
cd nfc-relay-phantom/app
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios
```

### NRF52832 BLE Firmware

```bash
# Build NRF52 BLE stack (requires nRF5 SDK)
cd nfc-relay-phantom/firmware/nrf52_ble
make

# Flash via SWD
nrfjprog --program build/nrf52_ble.hex --sectorerase --reset

# Or OTA DFU via BLE
nrfutil dfu ble -s build/nrf52_ble.zip
```

---

## ⚡ Power Architecture

```
USB-C 5V ──► FB1 ──► BQ25896 ──► VBAT (3.7V LiPo)
                       │
                       └──► TPS62840 ──► VREG_3V3 (3.3V main rail)
                                          │
                                          ├──► STM32L4S5 (3.3V)
                                          ├──► PN5180 (3.3V digital/analog)
                                          ├──► W25Q128 (3.3V)
                                          ├──► SSD1306 (3.3V)
                                          ├──► EM4095 (3.3V logic)
                                          └──► TLV70033 ──► VREG_3V3_NRF
                                                              │
                                                              └──► NRF52832 (3.3V isolated)
```

---

## 🔒 Security Features

- **Firmware read protection** — STM32L4 Option Bytes RDP Level 1
- **BLE encryption** — AES-CCM authenticated pairing with passkey
- **Key storage** — Mifare keys in battery-backed SRAM2, wiped on tamper
- **OTA signing** — Ed25519 signed firmware images for Bank B updates
- **Relay encryption** — AES-128-GCM tunnel between paired Phantom devices
- **USB CDC** — No authentication required (physical access assumed)

---

## 📡 BLE GATT Services

| Service | UUID | Characteristics |
|---------|------|----------------|
| NFC Control | 0xFFF0 | 0xFFF1: Mode, 0xFFF2: Protocol, 0xFFF3: Start/Stop |
| NFC Data | 0xFFF4 | 0xFFF5: UID (Notify), 0xFFF6: Frame (Notify), 0xFFF7: Send (Write) |
| RFID 125 | 0xFFF8 | 0xFFF9: Card ID (Notify), 0xFFFA: Write T5577 (Write) |
| Relay | 0xFFFB | 0xFFFC: Data (Notify+Write), 0xFFFD: Control (R/W) |
| Battery | 0x180F | 0x2A19: Level (Notify) |

---

## 📜 Licenses

| Component | License |
|-----------|---------|
| Hardware (schematics, PCB) | CERN-OHL-S-2.0 |
| Firmware | GPL-2.0-or-later |
| Companion App | MIT |

---

## 🙏 Credits

- **STM32L4S5** — STMicroelectronics
- **NRF52832** — Nordic Semiconductor
- **PN5180** — NXP Semiconductors
- **EM4095** — EM Microelectronic
- **BQ25896** — Texas Instruments
- **W25Q128** — Winbond
- **SSD1306** — Solomon Systech

---

*Designed for authorized security research and penetration testing. Use responsibly.*