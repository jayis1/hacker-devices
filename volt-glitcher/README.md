# Volt-Glitcher: Precision Fault Injection Tool

The **Volt-Glitcher** is an open-source hardware security research tool designed for high-precision voltage and clock glitching. It enables researchers to perform Fault Injection (FI) attacks to bypass security features in microcontrollers and SoCs.

## Features
- **Sub-nanosecond Precision**: FPGA-based timing logic for glitch pulses down to 1ns.
- **High-Speed Triggering**: Match patterns on UART, SPI, or GPIO with <50ns latency.
- **Power Trace Visualization**: Built-in 100MSPS ADC for real-time power analysis.
- **Dual-Control**: Manage via USB-C (CLI/Python) or Bluetooth LE (Mobile App).
- **Portable**: Credit-card sized form factor with integrated LiPo charging.

## Directory Structure
```
volt-glitcher/
├── README.md                              # This file
├── phase1_conceptual_architecture.md      # System design and data flow
├── phase2_component_selection_schematics.md # BOM and netlists
├── phase3_pcb_blueprints_layout.md        # PCB stackup and layout rules
├── phase4_software_stack.md               # Firmware and FPGA build info
├── kicad/                                 # KiCad 7.0 design files
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
├── firmware/                              # STM32H7 Firmware (C)
│   ├── Makefile
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   └── drivers/
│       ├── fpga.c
│       └── glitch.c
└── app/                                   # React Native Companion App
    ├── package.json
    ├── App.js
    ├── screens/
    └── components/
```

## Quick Start

### Firmware
Build and flash the firmware using the ARM GCC toolchain and OpenOCD:
```bash
cd firmware/
make
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program volt-glitcher.elf verify reset exit"
```

### Companion App
Install dependencies and run on Android/iOS:
```bash
cd app/
npm install
npx react-native run-android
```

## Specifications
| Parameter | Value |
|-----------|-------|
| MCU | STM32H723VGT6 (480MHz) |
| FPGA | Lattice iCE40UP5K |
| ADC | 8-bit, 100 MSPS |
| Glitch Width | 1ns - 1ms |
| Voltage Range | 0.8V - 5.0V |
| Peak Current | 2A |

## License
- Hardware: CERN OHL-S v2.0
- Firmware/Software: MIT License

## Credits
Designed by the Hacker Devices team.
