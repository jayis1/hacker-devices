# WFP-100 — Portable WiFi Pentesting Dongle

A pocket-sized USB-C dongle for 802.11ax (WiFi 6E) wireless network security auditing. Provides raw frame capture, injection, deauthentication, PMKID harvesting, and Evil Twin AP emulation — all self-contained with battery backup and zero host drivers required.

**Everything is open: KiCad schematics, C firmware, and the React Native companion app.**

## Specifications

| Parameter | Value |
|---|---|
| SoC | StarFive JH7110 (4×U74 + S7 @ 1.5 GHz) |
| WiFi | Intel AX210NGW (802.11ax, 2×2 MIMO, tri-band) |
| RAM | 2 GB LPDDR4X @ 3200 MT/s |
| Storage | 16 GB eMMC 5.1 |
| Boot Flash | 16 MB QSPI (GD25LQ128E) |
| PMIC | X-Powers AXP2101 |
| TPM | Microchip AT97SC3204T (SPI) |
| Battery | 1000 mAh LiPo (4+ hrs active capture) |
| Host Interface | USB-C 3.2 Gen1 (CDC-ECM + CDC-ACM) |
| Antennas | 2× external RP-SMA (2.4/5/6 GHz) |
| Form Factor | 85mm × 35mm × 12mm USB-C stick |
| BOM Cost | ~$51 @ 1K units |

## Directory Structure

```
wifi-pentest-dongle/
├── README.md
├── phase1_conceptual_architecture.md   # System architecture, block diagram, constraints
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md     # 6-layer HDI stackup, impedance, thermal
├── phase4_software_stack.md            # Boot strategy, MMIO, drivers, device tree
├── kicad/                              # KiCad 7 design files
│   ├── wifi-pentest-dongle.kicad_pro
│   ├── wifi-pentest-dongle.kicad_sch   # Full schematic (SoC, DDR, WiFi, PMIC, USB-C, etc.)
│   └── wifi-pentest-dongle.kicad_pcb   # 85×35mm board outline, component placement
├── firmware/                           # C firmware (SPL + Linux drivers)
│   ├── Makefile
│   ├── main.c                          # Board init (PMIC → clocks → GPIO → boot)
│   ├── board.h                         # Pin definitions
│   ├── registers.h                     # MMIO register map
│   ├── usb_descriptors.h               # USB CDC-ECM + CDC-ACM descriptors
│   └── drivers/
│       ├── ax210_pcie.h/.c             # Intel AX210 monitor mode + injection driver
│       ├── usb_gadget.h/.c             # USB composite gadget (ECM + ACM)
│       └── axp2101.h/.c               # AXP2101 PMIC I2C driver
└── app/                                # React Native companion app
    ├── App.js                          # Main navigation
    ├── package.json
    ├── screens/
    │   ├── DeviceScreen.js             # Connection & status
    │   ├── CaptureScreen.js            # Live packet capture
    │   └── SettingsScreen.js           # Channel, band, filter config
    ├── components/
    │   ├── StatusCard.js               # Device status card
    │   └── PacketList.js              # Scrollable packet list
    └── utils/
        └── protocol.js                # Binary wire protocol (CRC16, commands, parsing)
```

## Quick Start

### Firmware Build
```bash
export CROSS_COMPILE=riscv64-unknown-elf-
make -C firmware/ spl          # Build U-Boot SPL
make -C firmware/ drivers     # Build kernel modules
```

### App Build
```bash
cd app/
npm install
npx react-native run-android
```

## Licenses

- **Hardware** (KiCad, PCB): CERN-OHL-S v2
- **Firmware** (C drivers, SPL): GPL-2.0
- **Companion App** (React Native): MIT