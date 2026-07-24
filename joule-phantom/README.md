# Joule-Phantom — Smart-Battery SMBus/PMBus MITM Implant

```
   ╔══════════════════════════════════════════════════════════════════╗
   ║   ███████╗ ██████╗  ██████╗ ██╗  ██╗██╗   ██╗███████╗███╗   ███╗  ║
   ║   ██╔════╝██╔═══██╗██╔═══██╗██║ ██╔╝██║   ██║██╔════╝████╗ ████║  ║
   ║   ███████╗██║   ██║██║   ██║█████╔╝ ██║   ██║█████╗  ██╔████╗██║  ║
   ║   ╚════██║██║   ██║██║   ██║██╔═██╗ ██║   ██║██╔══╝  ██║╔═██║██║  ║
   ║   ███████║╚██████╔╝╚██████╔╝██║  ██╗╚██████╔╝██║     ██║  ╚█╔╝██║  ║
   ║   ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝   ╚═╝╚═╝  ║
   ║   Inline Smart-Battery SMBus/PMBus MITM & Spoofing Implant        ║
   ╚══════════════════════════════════════════════════════════════════╝
```

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange) ![HW-License](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> **Author:** jayis1
> **License:** GPL-2.0 (firmware & app) / CERN-OHL-S v2 (hardware)
> **Status:** Complete research hardware design — firmware + KiCad PCB + companion app

---

## ⚠️ LEGAL & ETHICAL DISCLAIMER

This device, firmware, and companion application are designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Unauthorized interception, modification, or injection of data on battery management buses in devices you do not own may violate computer-fraud and abuse statutes (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), consumer-product-safety regulations, and transport regulations (UN38.3, IATA DGR §6.3.2 — lithium battery marking/testing requirements).

**Manipulating lithium battery telemetry carries fire and explosion risk.** Spoofing state-of-charge, temperature, or charging parameters can cause a battery management system to overcharge, over-discharge, or bypass thermal protection, potentially leading to thermal runaway. **The author (jayis1) assumes no liability for misuse, property damage, or personal injury.** Always obtain proper written authorization before deployment, and never deploy on passenger-carrying aircraft, consumer devices in public use, or any equipment where battery failure poses safety risk. This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**Joule-Phantom** is a pocket-sized, parasitically-powered inline Man-in-the-Middle (MITM) implant that sits between a host system (laptop, drone, IoT controller, medical device) and its smart battery pack. It transparently bridges the **SMBus/PMBus** link — the I²C-derived protocol used by nearly all laptop, drone, and industrial smart batteries conforming to the **Smart Battery Data Specification (SBS, rev 1.1)** and **PMBus rev 1.3** — while capturing, modifying, and injecting telemetry frames in real time.

Unlike existing devices in this repository that target Ethernet (axle-tap, shadow-tap), CAN bus (can-creeper), debug ports (ghostbus, forge-probe), or side channels (thermal-phantom, sideprobe), **Joule-Phantom is the first to attack the smart-battery management bus** — an attack surface that is ubiquitous, critically safety-relevant, and almost entirely unexplored in published red-team tooling.

### Why the Smart-Battery Bus?

Modern lithium battery packs are not dumb energy reservoirs. They contain a **Battery Management System (BMS)** IC (commonly a TI BQ-series, Renesas RAJ, or Maxim MAX17xx) that continuously reports state-of-charge, voltage, current, temperature, cycle count, and chemistry to the host via SMBus. The host's **Embedded Controller (EC)** or **Charger IC** uses this telemetry to make safety decisions: when to stop charging, when to throttle the CPU, when to force an emergency shutdown, and whether the battery is authentic.

An attacker who can intercept and modify this telemetry gains:

- **Persistence invisibility** — the SMBus runs continuously from boot; no driver loads needed.
- **Safety-system manipulation** — force host shutdown, prevent charging, or disable thermal throttling.
- **Authentication bypass** — many laptop battery authentication schemes (Lenovo, Dell, HP) run challenge-response over SMBus `ManufacturerAccess`; replay/spoof defeats aftermarket-battery blocking.
- **Firmware extraction** — BMS firmware update modes are often triggered via SMBus commands; an inline device can capture vendor proprietary flashing protocols.
- **Covert channel** — data can be exfiltrated through battery telemetry changes visible to any software reading `/sys/class/power_supply/` on Linux or the Windows ACPI battery interface.

---

## 2. Attack Surface & Threat Model

### Attack Surface

| Surface | Protocol | Connector | Prevalence |
|---------|----------|-----------|------------|
| Laptop smart batteries | SBS over SMBus (100 kHz I²C) | 6–9 pin flex / blade | Universal in modern laptops |
| Drone / UAV packs | SBS + vendor extensions | XT30/XT60 + signal wire | DJI, Skydio, industrial UAVs |
| PMBus charger ICs | PMBus (up to 1 MHz) | Various | Server PSU, industrial power |
| Medical device batteries | SBS subset | Proprietary | Infusion pumps, monitors |
| IoT / industrial packs | SBS or direct I²C BMS | JST/ Molex | Smart sensors, field units |

### Threat Model

**Adversary capability:** Physical access to the battery connector for ≥ 30 seconds (time to insert the flex interposer). No soldering required — Joule-Phantom is a drop-in flex-PCB interposer matching common 6-pin battery connector footprints.

**Adversary goals:**
1. **Telemetry spoofing** — Make the host believe the battery is full (persist malicious state), empty (force shutdown / data loss), or overheating (disable charging).
2. **Authentication bypass** — Clone a genuine battery's identity (ManufacturerName, SerialNumber, challenge-response) to defeat vendor lock-in / aftermarket detection.
3. **Charging manipulation** — Set ChargingCurrent / ChargingVoltage to 0 (denial of charge) or to unsafe values (potential thermal runaway on packs without independent hardware protection).
4. **Firmware extraction** — Capture proprietary BMS firmware update protocols over SMBus for reverse engineering.
5. **Covert exfiltration** — Encode data in battery telemetry (e.g., modulating ReportedSoC by ±1% to encode bits) readable by any unprivileged process on the host.
6. **Denial of service** — Jam all SMBus transactions to make the host believe no battery is present.

**Defender limitations:**
- SMBus is unencrypted by specification; no MAC, no signing, no encryption.
- Most hosts trust battery telemetry unconditionally for safety decisions.
- Battery authentication, when present, is a single challenge-response over `ManufacturerAccess` — replayable.
- The EC typically has no way to detect an inline SMBus device (no bus topology discovery).
- SMBus runs at 10–100 kHz; even cheap MCUs can fully bridge it with zero latency impact.

---

## 3. Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **Main MCU** | STM32L432KCU6 — Cortex-M4 @ 80 MHz, 256 KB Flash, 64 KB SRAM, QFN-32 (5×5 mm) |
| **BLE Co-processor** | ESP32-C3-MINI-1 — RISC-V, BLE 5.0, -20 dBm, UART-bridged GATT service |
| **Host-side SMBus** | I²C1 @ 100 kHz, 10 kΩ pull-ups, TXS0102 level shifter (1.2–3.6 V ↔ 3.3 V) |
| **Battery-side SMBus** | I²C2 @ 100 kHz, 10 kΩ pull-ups, TXS0102 level shifter |
| **Voltage translation** | TXS0102DCT dual-channel auto-bidirectional (SMB_CLK + SMB_DAT) |
| **Sensors** | On-board NTC thermistor (ADC), VBAT voltage divider (1:4, ADC) |
| **Glitch output** | GPIO pulse to battery EN/CHG pin, 0–500 µs, hardware-timed |
| **Connectors** | 2× 6-pin FFC (TE 84953-6) — host side + battery side, 0.5 mm pitch |
| **Power** | Parasitic 3.3 V from battery SMBus pull-up rail; optional CR1220 coin cell backup |
| **Status LED** | Single 0603 blue LED, covert-mode (off by default, 4 s heartbeat when active) |
| **Crystal** | 32.768 kHz LSE for RTC timestamping of captured frames |
| **Form factor** | Flex PCB interposer, 90×70 mm, 0.2 mm polyimide, 4-layer (F.Cu / GND / PWR / B.Cu) |
| **Weight** | ~2.1 g (flex PCB + components, excluding coin cell) |
| **Operating temp** | 0 °C to +45 °C (constrained by battery environment) |
| **Compliance markers** | Silkscreen: "AUTHOR: jayis1", "CERN-OHL-S v2" |

### Block Diagram

```
  ┌──────────┐                     ┌─────────────────────────────┐                     ┌──────────┐
  │   HOST   │   SMBus / PMBus     │       Joule-Phantom          │   SMBus / PMBus     │  BATTERY │
  │  Laptop  │◄─────── I2C1 ──────►│                              │◄─────── I2C2 ──────►│   Pack   │
  │  Drone   │   SMB_CLK / SMB_DAT │   STM32L432KCU6 (Cortex-M4)  │   SMB_CLK / SMB_DAT │   (BMS)  │
  │  IoT EC  │   SMBALERT#         │                              │   SMBALERT#         │          │
  └──────────┘   BATT_PRSNT#       │   ┌───────────────────┐      │   BATT_PRSNT#       └──────────┘
                                  │   │  MITM Rule Engine  │      │
                                  │   │  (32 rules, mask)  │      │
                                  │   └─────────┬─────────┘      │
                                  │             │                │
                                  │   ┌─────────▼─────────┐      │
                                  │   │  Capture Ring     │      │
                                  │   │  (128 frames)     │      │
                                  │   └─────────┬─────────┘      │
                                  │             │ UART 115200    │
                                  │   ┌─────────▼─────────┐      │
                                  │   │  ESP32-C3-MINI-1  │      │
                                  │   │  BLE 5.0 GATT     │      │
                                  │   └─────────┬─────────┘      │
                                  └─────────────┼────────────────┘
                                                │ BLE 2.4 GHz
                                                ▼
                                  ┌─────────────────────────┐
                                  │  Operator App           │
                                  │  (React Native)         │
                                  │  Dashboard / MITM /     │
                                  │  Capture / Inject       │
                                  └─────────────────────────┘
```

---

## 4. Architecture

### 4.1 Dual-Port SMBus Bridge

The core of Joule-Phantom is a **dual-port I²C/SMBus bridge**. The STM32L432 has two independent I²C peripherals:

- **I²C1 (HOST port)** — configured in **slave mode**, listening at the SBS default battery address `0x0B`. The host's EC/charger sees a normal battery responding at the expected address.
- **I²C2 (BATT port)** — configured in **master mode**, forwarding the host's requests to the real battery pack.

When the host issues an SMBus read (e.g., "give me StateOfCharge"), the flow is:

1. Host sends START + 0x0B + WRITE + command byte (e.g., 0x0D) → I²C1 slave receives.
2. Host sends REPEATED-START + 0x0B + READ → I²C1 slave TX ISR fires.
3. Firmware forwards the command to the real battery via I²C2 master (`smbus_read_word`).
4. Real battery responds with 2 bytes (e.g., `0x64 0x00` = 100%).
5. **MITM rule engine** inspects the response; if a rule matches command 0x0D, it may replace the bytes (e.g., `0x01 0x00` = 1%).
6. Firmware feeds the (possibly modified) response bytes to I²C1 slave TX.
7. Host receives the spoofed data.
8. The complete frame (original + modified) is pushed to the capture ring buffer.

This architecture introduces **zero observable latency** at SMBus speeds (100 kHz → 10 µs/bit; the bridge round-trip is <50 µs, well within the 35 ms SMBus timeout).

### 4.2 MITM Rule Engine

The rule engine supports up to **32 concurrent rules**, each with:

- **Command match** — 8-bit command code with 8-bit mask (e.g., `cmd=0x0D, mask=0xFF` matches exactly SoC; `cmd=0x00, mask=0xF0` matches any ManufacturerAccess subcommand).
- **Action** — one of: `SPOOF` (replace response bytes), `BLOCK` (NACK the transaction), `INJECT` (fire a fake host-originated write), `LOG_ONLY` (capture but pass through), `GLITCH` (fire a glitch pulse during the transaction).
- **Spoof payload** — up to 32 replacement bytes for SPOOF action.

Rules are installed at runtime via BLE commands from the operator app, or via pre-built **spoof profiles** (see §4.4).

### 4.3 Capture & Exfiltration

All transactions — host-originated, battery-originated, and alert signals — are captured into a 128-entry ring buffer with timestamps (1 ms resolution from TIM2). Each entry records: timestamp, port, direction, slave address, command code, write payload, read payload, and flags (NACK / TIMEOUT / MODIFIED / INJECTED / PEC_ERR).

Captured frames are streamed over BLE to the operator app in real time. The BLE framing protocol (`ble_link`) uses a length-prefixed, CRC-8-protected format with a 128-byte MTU, supporting opcodes for: HELLO, STATUS, FRAME, RULE_ADD, RULE_CLR, RULE_LIST, INJECT, GLITCH, SET_MODE, AUTH_BYPASS, ACK, NACK.

### 4.4 Spoof Profiles

Pre-built MITM rule sets for common attack scenarios:

| Profile | Effect | SBS Commands Modified |
|---------|--------|-----------------------|
| **Full Charge** | Host sees 100% SoC, 12.6 V, fully-charged status | 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x16 |
| **Empty** | Host sees 1% SoC, 3.0 V, critical-low status → forced shutdown | 0x0D, 0x0E, 0x0F, 0x11, 0x12, 0x16 |
| **Over-Temp** | Host sees 60 °C, over-temp alarm → stops charging | 0x05, 0x14, 0x15, 0x16 |
| **Over-Current** | Host sees 8 A discharge, over-current alarm → throttles / shuts down | 0x07, 0x08, 0x11, 0x16 |
| **No Charge** | ChargingCurrent=0, ChargingVoltage=0 → battery never charges | 0x14, 0x15, 0x16 |
| **Auth Bypass** | Spoofs ManufacturerAccess ACK to defeat challenge-response auth | 0x00 |
| **Clone** | Replays captured ManufacturerName/Serial/Date/DesignCapacity | 0x1C, 0x1B, 0x18 |

### 4.5 Safety Interlocks

Joule-Phantom includes hardware and firmware safety interlocks to prevent accidental damage during authorized testing:

- **Voltage ceiling** — ADC monitors real battery voltage (via divider); if > 13.0 V, all spoof rules are cleared and mode reverts to Pass-Through.
- **Temperature ceiling** — NTC thermistor monitored; if > ~60 °C, all spoof rules cleared.
- **Glitch pulse limit** — hardware-timed, capped at 500 µs maximum.
- **Default mode** — device boots in Pass-Through (capture-only); active spoofing requires explicit operator command.

---

## 5. Firmware

### 5.1 Design Decisions

- **Bare-metal C, no HAL/CMSIS dependency** — `registers.h` provides a minimal register map; this keeps the firmware self-contained, auditable, and portable to other STM32 families with minor register address changes.
- **STM32L432KCU6** chosen for: ultra-low-power (critical for parasitic operation), dual I²C peripherals, QFN-32 footprint fits on a flex interposer, Cortex-M4 with FPU for any future DSP on captured waveforms.
- **Interrupt-driven I²C slave** — the host-side slave listener uses I²C1 address-match + TX/RX interrupts to handle host-initiated transactions with minimal latency.
- **Ring-buffer capture** — lock-free single-producer (ISR) / single-consumer (main loop) design avoids critical sections.
- **BLE co-processor** — the ESP32-C3 handles BLE protocol complexity (GATT, pairing, MTU negotiation) while the STM32 handles real-time SMBus bridging. They communicate over a simple framed UART protocol. This separation ensures SMBus timing is never disturbed by BLE stack jitter.
- **CRC-8 on all BLE frames** — protects against RF corruption of command/control data.

### 5.2 File Layout

```
firmware/
├── Makefile              — arm-none-eabi-gcc build, no external deps
├── board.h               — pin assignments, SBS/PMBus constants, limits
├── registers.h           — STM32L432 peripheral register map
├── stm32l432_flash.ld    — linker script (256K Flash / 64K RAM)
├── main.c                — init, main loop, safety interlocks, ISRs
└── drivers/
    ├── smbus_port.h/c    — dual-port SMBus driver + MITM rule engine + capture ring
    ├── ble_link.h/c      — BLE UART framing protocol + command dispatch
    └── spoof_engine.h/c  — pre-built spoof profiles (full-charge, empty, etc.)
```

### 5.3 Build

```bash
cd joule-phantom/firmware
make            # produces joule-phantom.elf / .hex / .bin
make flash      # openocd via ST-Link
```

**Toolchain:** `arm-none-eabi-gcc ≥ 10.3`, `openocd ≥ 0.11`.

### 5.4 Firmware Statistics

| File | Lines | Purpose |
|------|-------|---------|
| `main.c` | ~290 | Init, main loop, safety, ISRs |
| `drivers/smbus_port.c` | ~310 | Dual-port I²C, MITM engine, capture ring |
| `drivers/ble_link.c` | ~270 | BLE UART protocol, command dispatch |
| `drivers/spoof_engine.c` | ~130 | Spoof profiles |
| `board.h` | ~180 | Pin/constant definitions |
| `registers.h` | ~250 | Register map |
| **Total C** | **~1430** | |

---

## 6. Companion Application

A React Native app (iOS / Android) connects to Joule-Phantom over BLE 5.0 and provides five screens:

| Screen | Function |
|--------|----------|
| **Dashboard** | Live telemetry (mode, battery present, rule count, temp, voltage), spoof-profile quick-launch buttons |
| **MITM Rules** | Add/edit/delete custom spoofing/blocking/glitch rules keyed on SBS command code; view active rule list |
| **Capture** | Real-time scrolling frame log with filters (ALL / HOST / BATT / MODIFIED / INJECTED), colour-coded flags |
| **Inject** | Manual SMBus command injection console with preset library (read SoC, read voltage, write 0V charge, etc.) |
| **Settings** | Operating mode selector, auth-bypass toggle, rule clear, legal disclaimer, device info |

### App File Layout

```
app/
├── package.json
├── App.js                 — Tab navigator, BLE lifecycle, state management
├── services/
│   └── BleService.js      — BLE manager, frame parser, command sender
└── screens/
    ├── DashboardScreen.js — Telemetry + spoof profiles
    ├── MitmScreen.js      — Rule editor
    ├── CaptureScreen.js   — Frame log viewer
    ├── InjectScreen.js    — Injection console
    └── SettingsScreen.js  — Mode / actions / about / disclaimer
```

**Dependencies:** `react-native-ble-plx` (BLE), `@react-navigation/*` (navigation), `react-native-base64` (BLE payload encoding).

---

## 7. Use Cases

### 7.1 Red Team: Laptop Battery Telemetry Manipulation

**Scenario:** Red team needs to demonstrate that a targeted laptop can be forced to shut down abruptly, causing data loss in a running encryption process.

**Steps:**
1. Insert Joule-Phantom flex interposer between laptop motherboard and battery pack connector (requires partial disassembly — typical for ThinkPad/Dell service access).
2. App connects over BLE; set mode to MITM.
3. Activate "Empty" spoof profile → host EC sees 1% SoC + 3.0V + critical alarm → forces hibernation/shutdown within seconds.
4. Capture log documents the exact frames modified for the engagement report.

### 7.2 Security Research: Battery Authentication Bypass

**Scenario:** Researcher is studying a laptop vendor's battery authentication scheme (e.g., Lenovo 45N1xxx battery DRM).

**Steps:**
1. Insert Joule-Phantom; capture all `ManufacturerAccess` (0x00) exchanges during boot with a genuine battery.
2. Analyze the challenge-response protocol from the capture log.
3. Install an "Auth Bypass" rule that spoofs ACK responses to all ManufacturerAccess challenges.
4. Test with an unauthenticated (third-party) battery + Joule-Phantom → if the host accepts it, the auth scheme is defeated by replay.

### 7.3 Penetration Testing: Drone Battery DoS

**Scenario:** Pentester needs to demonstrate that a commercial drone can be grounded by manipulating its smart battery telemetry.

**Steps:**
1. Insert Joule-Phantom between drone FC and battery pack (typically XT30 + signal wire).
2. Set mode to JAM → all SMBus transactions NACK'd → FC believes no battery is present → drone refuses to arm.
3. Alternatively, use "Over-Temp" profile → FC triggers thermal landing protocol.

### 7.4 Research: BMS Firmware Extraction

**Scenario:** Researcher wants to extract and reverse-engineer a BMS IC's firmware for vulnerability research.

**Steps:**
1. Insert Joule-Phantom; trigger BMS firmware update mode via host vendor tool.
2. Capture all SMBus write-block transactions to ManufacturerAccess / ManufacturerData.
3. The capture log contains the complete firmware image as transmitted over SMBus, with timestamps for protocol sequencing.

### 7.5 Covert Channel: Data Exfiltration via Battery Telemetry

**Scenario:** Red team has code execution on a target laptop but egress is blocked by DLP/network monitoring.

**Steps:**
1. Pre-position Joule-Phantom on the target's battery.
2. Malware on host writes data to battery telemetry by issuing SMBus writes (e.g., modulating `AtRate` register).
3. Joule-Phantom captures the modulated telemetry and exfiltrates over BLE to the operator.
4. No network traffic is generated; the channel is invisible to network-based DLP.

---

## 8. Repository Structure

```
joule-phantom/
├── README.md                          — this file
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── stm32l432_flash.ld
│   ├── main.c
│   └── drivers/
│       ├── smbus_port.h
│       ├── smbus_port.c
│       ├── ble_link.h
│       ├── ble_link.c
│       ├── spoof_engine.h
│       └── spoof_engine.c
├── kicad/
│   ├── joule-phantom.kicad_pro
│   ├── joule-phantom.kicad_sch
│   └── joule-phantom.kicad_pcb
└── app/
    ├── package.json
    ├── App.js
    ├── services/
    │   └── BleService.js
    └── screens/
        ├── DashboardScreen.js
        ├── MitmScreen.js
        ├── CaptureScreen.js
        ├── InjectScreen.js
        └── SettingsScreen.js
```

---

## 9. Bill of Materials (Summary)

| Ref | Part | Package | Qty | Notes |
|-----|------|---------|-----|-------|
| U1 | STM32L432KCU6 | QFN-32 5×5 mm | 1 | Main MCU |
| U2 | ESP32-C3-MINI-1 | LGA module | 1 | BLE co-processor |
| U3, U4 | TXS0102DCT | SOT-23-8 | 2 | SMBus level shifters |
| J1, J2 | TE 84953-6 (or equiv.) | FFC 6-pin 0.5 mm | 2 | Host / battery connectors |
| Y1 | 32.768 kHz crystal | SMD 3215 | 1 | RTC timestamping |
| D1 | LED blue 0603 | 0603 | 1 | Covert status |
| R1–R4 | 10 kΩ 0402 | 0402 | 4 | SMBus pull-ups |
| R5 | 470 Ω 0402 | 0402 | 1 | LED resistor |
| C1, C2, C4 | 100 nF 0402 | 0402 | 3 | Decoupling |
| C3 | 10 µF 0603 | 0603 | 1 | Bulk cap |

**Estimated BOM cost:** ~$12 USD at unit quantity (ESP32-C3 module is the cost driver).

---

## 10. Limitations & Future Work

- **Block-write spoofing** — the current MITM engine fully supports word (2-byte) spoofing; block (up to 32-byte) spoofing for string commands (ManufacturerName, DeviceName) is partially implemented and requires a cache-lookup path in the bridge logic. Future firmware will add full block-spoof rules.
- **PEC (Packet Error Checking)** — SMBus PEC is parsed but not fully re-computed after spoofing; a future revision will re-compute PEC on modified frames to avoid host detection via PEC mismatch.
- **Connector adaptors** — the current design uses a generic 6-pin 0.5 mm FFC footprint; real-world deployment requires model-specific interposer tails (ThinkPad, Dell, HP, MacBook each use different connector pinouts and mechanical keying).
- **Power analysis** — the on-board ADC could be used for rough current-side-channel analysis of the BMS IC during cryptographic operations; this is a future firmware extension.
- **1-wire / HDQ support** — some older battery packs use Dallas 1-wire / HDQ instead of SMBus; a future hardware revision could add a 1-wire transceiver.

---

## 11. Credits

- **Author / Creator:** jayis1
- **Hardware License:** CERN-OHL-S v2
- **Software License:** GPL-2.0
- **Inspiration:** The SBS specification (rev 1.1, Battery Smart 1998), PMBus rev 1.3, and the observation that despite near-universal deployment, smart-battery buses remain an overlooked attack surface in mainstream red-team tooling.

---

*Joule-Phantom — because every joule of trust can be phantom.*