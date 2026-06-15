# VoltGlitcher Pro

⚡ **High-Precision Voltage Glitch Injection Tool for Hardware Security Research**

---

## Overview

The VoltGlitcher Pro is an open-source, FPGA-assisted fault injection platform designed for hardware security researchers to test the resilience of MCUs, SoCs, and other integrated circuits against voltage glitching, electromagnetic fault injection, and clock glitching attacks.

Unlike simple Arduino-based glitchers with microsecond-level timing, the VoltGlitcher Pro achieves **sub-nanosecond precision** through a dual-processor architecture pairing an STM32F407VGT6 MCU with a Lattice iCE40HX1K FPGA co-processor.

## Key Features

- **Three glitch modes:** Voltage glitch (VCC shunt), EM glitch (coil driver), Clock glitch (clock stretch)
- **Four trigger inputs:** GPIO (opto-isolated), UART pattern match, JTAG TAP state, Manual button
- **Sub-ns timing:** FPGA carry-chain delay lines with 50 ps resolution
- **Arbitrary waveforms:** Upload custom pulse shapes to FPGA BRAM
- **16 EEPROM profiles:** Save and recall glitch configurations
- **Companion app:** React Native app for iOS/Android with real-time monitoring and statistical analysis
- **Safety features:** Hardware overcurrent cutoff, gate driver fault detection, temperature monitoring
- **Fully open source:** MIT licensed — firmware, FPGA bitstream, app, and PCB design

## Hardware

| Component | Part | Purpose |
|-----------|------|---------|
| MCU | STM32F407VGT6 (LQFP-100) | USB interface, ADC monitoring, safety control |
| FPGA | Lattice iCE40HX1K-TQ144 | Sub-ns timing, pattern matching, waveform gen |
| Shunt MOSFET | SiS426DN (N-ch, 6mΩ) | VCC glitch output |
| EM MOSFET | IRLML6344TRPBF | EM coil driver |
| Clock MOSFET | BSS84P (P-ch) | Clock glitch (stretch) |
| Gate Driver | UCC27511DBVR | 4A peak drive for Q1 |
| Current Sense | INA210NA | Shunt current monitoring |
| Voltage Regs | TLV62569 + TPS7A7001 | 1.2V/1.8V/2.5V/3.3V power tree |
| EEPROM | CAT24C256WI-GT3 | 32KB profile storage |
| Opto-isolator | TLP291-4 | Trigger input isolation |

## Repository Structure

```
volt-glitcher/
├── firmware/           # STM32F407 bare-metal firmware
│   ├── main.c          # Main application and ISRs
│   ├── board.h         # Pin definitions and board API
│   ├── registers.h     # MCU registers + FPGA register map
│   ├── usb_descriptors.h  # USB descriptors and protocol structs
│   ├── Makefile        # Build system (arm-none-eabi-gcc)
│   └── drivers/
│       ├── fpga.c/h    # FPGA configuration and register access
│       └── glitch.c/h # Glitch engine, profiles, calibration
├── kicad/             # KiCad 8 schematic and PCB
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
├── app/               # React Native companion app
│   ├── App.js          # Main app with navigation
│   ├── components/
│   │   └── GlitchConfig.js  # Parameter configuration UI
│   ├── screens/
│   │   ├── ControlScreen.js  # ARM/FIRE/monitor dashboard
│   │   └── AnalysisScreen.js # Statistics and charts
│   ├── utils/
│   │   └── protocol.js  # USB binary protocol implementation
│   └── package.json
├── phase1_conceptual_architecture.md
├── phase2_component_selection_schematics.md
├── phase3_pcb_blueprints_layout.md
├── phase4_software_stack.md
└── README.md          # This file
```

## Quick Start

### Firmware Build
```bash
cd firmware
make clean && make
make flash-stlink  # or: make flash (OpenOCD)
```

### Companion App
```bash
cd app
npm install
npx react-native run-android
```

### Basic Usage
1. Connect VoltGlitcher Pro to target via J3 (glitch output) and J4 (trigger input)
2. Connect USB cable to host computer
3. Open companion app → Connect → Configure glitch parameters → ARM
4. When trigger condition is met, glitch fires automatically
5. Monitor results in Analysis tab; mark successful glitches

## Safety Warning

⚠️ **Voltage glitching involves deliberately short-circuiting target power rails.** This can damage both the target device and the VoltGlitcher Pro. Always:
- Use a current-limited power supply for the target
- Start with narrow pulse widths and increase gradually
- Never leave the device armed and unattended
- Disconnect the glitch output when not in use

## License

MIT License — Copyright (c) 2026 Hacker Devices