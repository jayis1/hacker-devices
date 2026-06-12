# Hacker Devices — Full-Cycle Hardware Designs for Security Research

A collection of open-source hardware designs for legitimate security research and pentesting. Each device is a self-contained subfolder with its own complete 4-phase engineering documentation.

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
| 1 | [`wifi-pentest-dongle/`](wifi-pentest-dongle/) | 🔄 In Progress | 802.11ax monitor/inject, deauth, PMKID capture |
| 2 | *rf-transceiver-tool* | ⏳ Queued | Sub-GHz + 2.4GHz multi-protocol (CC1101 + nRF24) |
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
- **Code is production-quality** — not stubs or pseudocode

## License

Hardware: CERN-OHL-S v2  
Software: GPL-2.0 (kernel drivers), MIT (userspace tools)