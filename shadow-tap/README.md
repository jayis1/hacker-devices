# ShadowTap — PoE Stealth Network Tap & MITM Implant

**ShadowTap** is a palm-sized, PoE-powered inline Ethernet tap and Man-in-the-Middle (MITM) implant designed for authorized red-team engagements. It transparently bridges two Gigabit Ethernet links while enabling selective packet interception, modification, injection, and capture — all powered parasitically from the same 802.3af PoE link it taps.

## Specifications

| Parameter | Value |
|---|---|
| **SoC** | NXP i.MX RT1062, Cortex-M7 @ 600 MHz |
| **Ethernet** | 2× 88E1510 PHY, RGMII, 1000Base-T full-duplex |
| **Forwarding** | Line-rate 1 Gbps cut-through (< 500 ns latency) |
| **MITM Engine** | 32 rule slots, ARP/DNS/DCP/inject/drop/modify |
| **PoE** | 802.3af Class 3 (12.95 W), TPS2378 PD controller |
| **BLE** | nRF52840-M.2, BLE 5.0, AES-256-CTR encrypted C2 |
| **Storage** | microSD (PCAP capture), 16 MB SPI NOR flash |
| **Form Factor** | 60 × 40 × 15 mm |
| **BOM Cost** | ~$45 at qty 100 |
| **Boot** | < 2 s from PoE power-on to operational |

## Directory Structure

```
shadow-tap/
├── README.md
├── phase1_conceptual_architecture.md   # System design, block diagram, data flow
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md     # Stackup, routing, thermal, isolation
├── phase4_software_stack.md            # Boot, MMIO, drivers, device tree
├── kicad/
│   ├── shadow-tap.kicad_pro           # KiCad project with net classes & rules
│   ├── shadow-tap.kicad_sch           # Full schematic (SoC, PHYs, PoE, BLE, SD)
│   └── shadow-tap.kicad_pcb           # Board outline, placement, stackup
├── firmware/
│   ├── Makefile                        # Cross-compile for ARM Cortex-M7
│   ├── main.c                          # Entry point, ENET init, frame processing
│   ├── board.h                         # Hardware definitions & constants
│   ├── registers.h                     # Full MMIO register map (CCM, ENET, UART, SPI, SD, FLEXSPI)
│   ├── usb_descriptors.h              # USB descriptors for recovery mode
│   └── drivers/
│       ├── enet_driver.h              # Dual-ENET DMA driver API
│       ├── enet_driver.c              # Zero-copy DMA rings, cut-through, PHY MDIO
│       ├── ble_c2_driver.h            # BLE C2 protocol API
│       └── ble_c2_driver.c            # SLIP framing, CRC16, AES crypto, command dispatch
└── app/
    ├── package.json                    # React Native dependencies
    ├── App.js                          # Navigation, BLE connection manager
    ├── screens/
    │   ├── DashboardScreen.js          # Link status, mode toggle, quick actions
    │   ├── RulesScreen.js              # MITM rule CRUD with modal editor
    │   └── CaptureScreen.js            # PCAP start/stop/stream, saved captures
    ├── components/
    │   └── StatusIndicator.js          # Reusable LED-style status dot
    └── utils/
        └── protocol.js                 # BLE C2 protocol (SLIP, CRC16, NUS UART)
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Build firmware
cd firmware
make CROSS_COMPILE=arm-none-eabi- clean all

# Flash via OpenOCD (SWD)
openocd -f interface/cmsis-dap.cfg -f target/imxrt1062.cfg \
    -c "program shadow-tap.elf verify reset exit"
```

### Companion App

```bash
cd app
npm install
npx react-native run-android   # or run-ios
```

### Usage

1. **Insert inline**: Connect J1 (UPLINK) to switch, J2 (TARGET) to host
2. **Power on**: PoE provides power automatically; LEDs indicate link status
3. **Connect app**: Open ShadowTap app → Scan & Connect
4. **Configure rules**: Rules screen → Add Rule → select type (ARP, DNS, etc.)
5. **Enable MITM**: Toggle from TAP → MITM mode on dashboard
6. **Capture**: Start capture → frames saved to microSD as PCAP

## Architecture

### Transparent Tap (Default)
Frames arriving on either port are forwarded to the opposite port with minimal latency (cut-through after 64 bytes). The device does not advertise its MAC address or respond to LLDP/CDP unless configured.

### MITM Mode
Programmable rules match on frame fields (offset + mask + value). Matched frames can be:
- **Modified**: Byte replacement at arbitrary offsets (ARP/DNS payload rewrite)
- **Dropped**: Silently discard matching frames
- **Injected**: Craft and inject new frames (gratuitous ARP, DNS replies)

### BLE C2 Channel
Encrypted (AES-256-CTR) SLIP-framed UART link to nRF52840-M.2 module. The companion app sends commands (add/delete rules, start/stop capture, stream PCAP) and receives status updates and notifications.

## Key Components

| Ref | Part | Function |
|---|---|---|
| U1 | MIMXRT1062DVL6B | Cortex-M7 SoC, dual ENET, 600 MHz |
| U2, U3 | 88E1510-A0 | Marvell Gigabit PHY, RGMII |
| U4 | TPS2378DDA-R-P | PoE PD controller (802.3af Class 3) |
| U5 | TLV62569DBVR | 3.3V/3A buck converter |
| U6 | TLV62569DBVR | 1.2V/2A buck converter |
| U7 | IS25LP016D-JBLE | 16 MB SPI NOR flash (boot/config) |
| U8 | AT24C02C-SSHL-T | 2 Kb I²C EEPROM (device ID) |
| U9 | nRF52840-M2 | BLE 5.0 module (FCC pre-certified) |
| J1, J2 | LPJG16314A4NL | RJ45 MagJack w/ PoE center taps |

## Security Considerations

- **For authorized use only** — red team engagements with written scope
- BLE C2 uses AES-256-CTR encryption with key exchange over BLE SMP
- No wireless transmit on Ethernet — device is a transparent wire
- Default mode is passive tap; MITM must be explicitly enabled
- Device does not respond to network discovery protocols (LLDP, CDP, NDP) by default

## License

Hardware design: CERN-OHL-S v2 (Strongly Reciprocal)
Firmware: GPL-3.0
App: MIT