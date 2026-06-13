# Hacker Devices — Open Hardware for Security Research

A collection of fully open-source hardware designs for legitimate security research and pentesting. Every device is complete end-to-end: engineering docs, KiCad schematics & PCB layouts, production C/Python firmware, and React Native companion apps. Build it, flash it, run the app, start researching.

## What's Inside Every Device

No half-finished reference designs. Each device folder contains the full stack:

| Category | Files | Description |
|---|---|---|
| **Engineering Docs** | `phase1_conceptual_architecture.md` → `phase4_software_stack.md` | 4-phase design process: requirements, BOM & netlists, PCB layout rules, software stack |
| **KiCad Schematics** | `kicad/*.kicad_pro`, `*.kicad_sch`, `*.kicad_pcb` | Full schematic with real component symbols, net labels, power flags, and board outline with placed footprints |
| **C/Python Firmware** | `firmware/main.c`, `registers.h`, `board.h`, `drivers/`, `Makefile` | Production-quality MCU firmware — init code, drivers with DMA, USB descriptors, wire protocol |
| **React Native App** | `app/App.js`, `screens/`, `components/`, `utils/protocol.js`, `package.json` | Companion mobile app for device control, configuration, real-time status, and data capture |

## Design Methodology

Every device follows a rigorous 4-phase process:

| Phase | Document | Contents |
|---|---|---|
| **Phase 1** | `phase1_conceptual_architecture.md` | Purpose, performance targets, constraints, block diagram, bus topology, power domains |
| **Phase 2** | `phase2_component_selection_schematics.md` | BOM with real part numbers, pinouts/MUX tables, decoupling networks, netlists, impedance matching |
| **Phase 3** | `phase3_pcb_blueprints_layout.md` | PCB stackup, routing rules, length matching, via strategy, thermal management, DFM notes |
| **Phase 4** | `phase4_software_stack.md` | MMIO/register defs, clock & GPIO init (production C), critical device driver, device tree, bootloader |

## Devices

| # | Device | Status | Description |
|---|---|---|---|
| 1 | [`wifi-pentest-dongle/`](wifi-pentest-dongle/) | ✅ Complete | 802.11ax monitor/inject, deauth, PMKID capture |
| 2 | [`rf-transceiver-tool/`](rf-transceiver-tool/) | ✅ Complete | Sub-GHz + 2.4GHz multi-protocol (CC1101 + nRF24) |
| 3 | *badusb-injector* | ⏳ Queued | HID emulation payload injector, multi-profile |
| 4 | *nfc-rfid-cloner* | ⏳ Queued | 13.56MHz + 125kHz, Mifare/EM4100 |
| 5 | *network-tap-mitm* | ⏳ Queued | 2× GbE transparent bridge, packet capture |
| 6 | *can-bus-analyzer* | ⏳ Queued | OBD-II + CAN-FD, automotive pentest |
| 7 | *sdr-keyfob-replay* | ⏳ Queued | 433/315MHz keyfob capture & replay |
| 8 | *ble-sniffer-spoofer* | ⏳ Queued | nRF52, advertisement inject, GATT fuzz |
| 9 | *imsi-catcher-detector* | ⏳ Queued | LTE/4G SDR analysis |
| 10 | *biometric-bypass-toolkit* | ⏳ Queued | Fingerprint emulation, RFID clone for access control |

## Constraints

- **BOM target:** Under $100 per device
- **Form factor:** Portable, USB-powered or battery-operated
- **All designs use real, purchasable components** with exact part numbers
- **KiCad schematics are valid KiCad 7+ format** — open them directly
- **Firmware is production-quality C** — not stubs or pseudocode
- **React Native apps have working UI** with proper state management and wire protocol matching the firmware

## License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| Python tools & scripts | MIT |
| React Native companion apps | MIT |