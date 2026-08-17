# Pulse-Reaper — TDR-Guided Non-Invasive Covert Tap for Twisted-Pair Copper Cables

```
   ╔══════════════════════════════════════════════════════════════════════╗
   ║   ██████╗   ██╗ ██████╗ ███████╗███████╗██╗   ██╗    ██╗███████╗██████╗ ║
   ║   ██╔══██╗ ██╔╝██╔════╝ ██╔════╝██╔════╝██║   ██║    ██║██╔════╝██╔══██╗║
   ║   ██████╔╝ ██╔╝██║  ███╗███████╗███████╗███████║    ██║█████╗  ██████╔╝║
   ║   ██╔═══██╗███╗█████╗  ██║╚════██║╚════██║██╔══██║    ██║██╔══╝  ██╔══██╗║
   ║   ██║  ███████╔╝███████╗███████╗███████║██║  ██║    ██║███████╗██║  ██║║
   ║   ╚═╝  ╚══════╝ ╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝    ╚═╝╚══════╝╚═╝  ╚═╝║
   ║                                                                         ║
   ║   TDR-Guided Non-Invasive Covert Tap for Twisted-Pair Copper Cables     ║
   ║   Capacitive / Inductive Clamp Coupling · Cable Fingerprinting          ║
   ║   Differential Bus Sniffing · Fieldbus Injection · Air-Gap Bridge        ║
   ╚══════════════════════════════════════════════════════════════════════╝
```

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange) ![HW-License](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** Pulse-Reaper is designed **exclusively** for authorized security research, penetration testing under written contract, and academic/educational environments. Attaching a non-invasive tap to, intercepting, or injecting traffic on cables, fieldbus segments, telephone lines, or network links you do not own or have not been explicitly authorized to test is illegal in most jurisdictions and may violate computer-fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511 ECPA, 18 U.S.C. § 3121), telecommunications-interception laws, and data-protection regulations (GDPR, CCPA). Physically attaching any device to communications wiring in a building you do not control may additionally constitute trespass-to-chattels, unlawful interception, or sabotage. Some target cables carry hazardous voltages (POTS ring voltage up to 105 VAC, PoE 48–57 V, industrial 4–20 mA loops, mains-borne DSL); the operator is responsible for electrical safety. The author (**jayis1**) assumes no liability for any misuse, injury, equipment damage, or illegal activity. **Always obtain proper written authorization before deployment and ensure compliance with all applicable electrical codes and interception laws.** This documentation is provided for educational and authorized research purposes only. This is a research design, not a finished product.

**Author / Creator:** jayis1
**License:** Hardware — CERN-OHL-S v2 · Firmware — GPL-2.0 · App — MIT
**Version:** 1.0
**Date:** 2026-08-17
**BOM Cost:** ~$94 at 1K volume

---

## Table of Contents

1. [Overview & Problem Statement](#1-overview--problem-statement)
2. [Attack Surface & Threat Model](#2-attack-surface--threat-model)
3. [Hardware Specifications](#3-hardware-specifications)
4. [Architecture & Block Diagram](#4-architecture--block-diagram)
5. [Firmware Details & Design Decisions](#5-firmware-details--design-decisions)
6. [Application/Software Interface](#6-applicationsoftware-interface)
7. [Use Cases for Red Teams, Security Researchers & Pentesters](#7-use-cases-for-red-teams-security-researchers--pentesters)
8. [Safety Considerations](#8-safety-considerations)
9. [Detection & Countermeasures](#9-detection--countermeasures)
10. [License & Credits](#10-license--credits)

---

## 1. Overview & Problem Statement

**Pulse-Reaper** is a pocket-sized, battery-operated **non-invasive cable tap and bus analysis platform** for security research. It clamps magnetically onto the **jacket of an unpowered or live twisted-pair copper cable** — without cutting, stripping, or even unplugging the cable — and uses **time-domain reflectometry (TDR)** to fingerprint the cable (type, characteristic impedance, length, tap-point distance to each endpoint, termination quality), then engages a **high-impedance capacitive + inductive coupling front-end** to sniff the differential signaling riding on the pair. For unpowered buses it can additionally inject via the same clamp. Captured traffic is streamed over an encrypted BLE 5 backhaul to an operator's mobile device, or written to microSD for offline retrieval.

### 1.1 What problem does this solve?

Every existing hardware bus tap in the open-source security tooling ecosystem — from CAN-bus infiltrators to HART/Modbus/BACnet implants to Ethernet network taps — assumes that the operator can **cut, strip, splice, or terminate** the target cable. That assumption fails in real engagements:

- **Industrial control cabinets** terminate fieldbus pairs onto screw terminals, DIN-rail blocks, or inside sealed M12/M8 industrial connectors. Splicing means a maintenance outage and is immediately visible to any walkthrough.
- **Structured cabling** (Cat 6/7) is terminated at patch panels; the jacketed run between two RJ-45 jacks is not accessible without cutting, and cutting is detected by link-loss alarms.
- **POTS/telephone lines** in a building are often inaccessible except inside the jacketed riser cable; splicing requires a lineman's test set and a visible clamp.
- **Sealed industrial cables** (Profibus DP purple, MIL-STD-1553 twinax, aerospace ARINC 429 twisted shielded pairs) are designed to be tamper-evident; any visible splice defeats the engagement.

Pulse-Reaper sidesteps all of this by coupling **through the cable jacket** using a high-impedance analog front-end that loads the differential pair with < 0.5 pF of parasitic capacitance and > 1 GΩ of parasitic resistance — well below the noise floor of any link-quality monitor, any BERT (bit-error-rate tester), and any termination-detection circuit. The clamp is magnetic (rare-earth magnet + mechanical latch) so it can be installed in seconds and removed without trace.

### 1.2 What Pulse-Reaper is not

- It is **not a fiber tap** (see FiberPhantom).
- It is **not an Ethernet PoE implant** (see ShadowTap).
- It is **not a powerline communications tool** (see Powerline-Reaper).
- It is **not a JTAG/SWD probe** (see GhostBus, Forge-Probe).
- It is **not a software-only tool** — it operates at the electrical layer of the copper pair, below any protocol stack the endpoints implement.

Pulse-Reaper is the only tool in this repository that targets the **physical copper-jacket layer** and that uses **TDR to characterise the cable before deciding how to tap it**.

### 1.3 Novelty — why this device, and why now

Three technology shifts make non-invasive copper tapping newly practical and newly dangerous:

1. **Sub-picoFarad clamp front-ends.** Modern fully-differential amplifiers (TI LMH5401, ADL5206, THS4551) achieve > 1 GΩ input impedance with < 0.3 pF of parasitic capacitance in a tiny QFN package, enabling high-impedance capacitive coupling through a 0.8 mm PVC/PUR jacket with useful bandwidth to ~50 MHz — enough to recover RS-485 (up to 10 Mbps), CAN FD (up to 8 Mbps), Profibus DP (12 Mbps), Modbus RTU, 4–20 mA HART, analog POTS, and even 100BASE-TX with DSP equalisation.
2. **FPGA-assisted TDR at < $5 BOM.** A Lattice iCE40UP5K or Gowin GW1N-4 can generate 200 ps rise-time pulses and sample reflections with a dual-channel 100 MSa/s ADC for under $5, producing a high-resolution reflectogram that identifies cable type (Cat 5e/6/6A/7), length, impedance discontinuities, and tap-point distance to each endpoint — all without touching the conductors.
3. **Battery + BLE backhaul.** A covert tap that needs mains power or a USB host is not covert. Pulse-Reaper runs for 24+ hours on a 1800 mAh LiPo and exfiltrates captured traffic over encrypted BLE 5 (nRF52840), with on-device microSD buffering for periods of BLE silence.

The threat model is straightforward and under-explored: most OT/ICS security teams monitor the *protocol* layer (Modbus TCP, DNP3, BACnet/IP) and the *wireless* layer, but treat the copper fieldbus pair as a trusted physical channel. Pulse-Reaper demonstrates that this trust is misplaced — the cable jacket is not a security boundary, and an attacker who can physically reach the cable run (cable tray, riser, field junction box, under-floor raceway) can tap it without a single visible modification.

---

## 2. Attack Surface & Threat Model

### 2.1 Trust boundaries violated

| Trust boundary | Pulse-Reaper's position |
|----------------|----------------------------|
| **Cable jacket / physical tamper seal** | **Bypassed.** The clamp couples through the jacket; no cutting, stripping, or splicing. Tamper-evident seals on the jacket remain intact. |
| **Link-quality / continuity monitoring** | **Bypassed.** Parasitic loading is < 0.5 pF / > 1 GΩ — below the detection threshold of any BERT, TDR-based continuity monitor, or PoE link-quality alarm. |
| **Protocol-level authentication** | **Bypassed.** Pulse-Reaper reads the differential signalling at the physical layer; protocol-level encryption (Modbus RTU has none, Profibus DP has none, HART has none by default) does not help. |
| **Air-gapped control networks** | **Bridged.** Captured fieldbus traffic is exfiltrated over BLE to an operator outside the air gap, or to a second Pulse-Reaper on a different cable, creating a cross-cable covert channel. |
| **Endpoint encryption (where present)** | **Out of scope.** Pulse-Reaper captures raw differential symbols; if the endpoints encrypt above the physical layer (rare on RS-485/Profibus/HART/CAN), the operator still obtains traffic metadata, timing, and frame structure. |

### 2.2 Attack vectors

| Vector | What it does | Practical effect |
|--------|--------------|------------------|
| **Passive sniff** | Capacitively couples to the differential pair and recovers the signalling with a DSP equaliser. | Full plaintext of all unencrypted fieldbus traffic (Modbus RTU, Profibus DP, HART, CAN, RS-485, POTS DTMF, 4–20 mA HART FSK). |
| **TDR fingerprint** | Injects a fast edge and samples reflections; cross-correlates with a library of cable signatures. | Identifies cable type (Cat 5e/6/6A/7, Profibus purple, MIL-1553 twinax, POTS riser), length, tap-point distance to each endpoint, and termination quality — guiding the operator to the best tap position. |
| **Inject (unpowered buses)** | Drives the clamp with a differential driver to induce a voltage on the pair through the jacket. | Injects forged frames onto dry-loop contacts, sensor inputs, and unpowered fieldbus segments — useful for ICS red teams simulating sensor spoofing. |
| **Covert channel via cable** | Modulates a low-amplitude signal onto an active pair that a second Pulse-Reaper on the same cable recovers. | Cross-cable air-gap bridge; data exfiltration through a shared copper run. |
| **Cable inventory / mapping** | Walks along a cable tray clamping each cable; TDR identifies each one. | Maps an unknown facility's cabling without opening any enclosure — useful for physical-penetration reconnaissance. |

### 2.3 Adversary profile

Pulse-Reaper models an adversary with **brief physical access** to the target cable run (cable tray, riser, under-floor raceway, field junction box, or the exterior of a sealed industrial connector) but **no ability to cut, strip, or re-terminate** the cable without detection. The adversary is assumed to have a written authorisation scope that includes the cable plant but may not include powering down any link.

### 2.4 What the adversary cannot do

- Tap **shielded cable with continuous 360° foil+braid** that is properly bonded at both ends (the capacitive coupling is attenuated by > 60 dB). Pulse-Reaper's TDR mode will report "shielded — no coupling" and the operator should look for an unshielded segment.
- Recover traffic from **fully encrypted fieldbus** protocols (rare in ICS; Profibus PA, HART-IP over TLS, and Modbus Security are the few examples).
- Operate without **physical access** to the cable — Pulse-Reaper is a physical-layer tool, not a remote exploit.

---

## 3. Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **Main MCU** | STM32H743VIT6 — Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM, 16 KB I-cache + 16 KB D-cache |
| **TDR / DSP FPGA** | Lattice iCE40UP5K-SG48 — 5.3K LUTs, 1 Mb SPRAM, 4× 1.2 GHz PLLs, drives TDR pulse + ADC timing |
| **TDR ADC** | AD9203ARZ — dual 10-bit 100 MSa/s, 400 MHz analog bandwidth, LVDS / parallel CMOS |
| **TDR Pulse Generator** | FPGA-driven step-recovery diode (SRD) + fast comparator; < 200 ps rise time, ±5 V into 50 Ω |
| **Sniff Front-End** | TI LMH5401-SP fully-differential amplifier, 1.2 GHz GBW, > 1 GΩ input, selectable gain (−6 to +26 dB) |
| **Inject Driver** | TI THS3491 current-feedback op-amp, 900 MHz, ±1.5 V into clamp (low-amplitude injection only) |
| **Coupling Clamp** | Custom ferrite-core clamshell with capacitive plates + inductive sense coil; < 0.5 pF parasitic, > 1 GΩ |
| **BLE Backhaul** | nRF52840-M.2 module — BLE 5.0, +8 dBm, AES-256-CTR encrypted C2 |
| **Storage** | MicroSD (UHS-I SDR104) for capture buffer + 16 MB W25Q128 SPI NOR for firmware/settings |
| **Display** | SSD1306 128×64 OLED (I2C) — status, TDR reflectogram, capture stats |
| **Power** | 1800 mAh LiPo, USB-C charging (BQ25896), MCP73871 fuel gauge; 24+ h sniff, 6+ h active TDR |
| **USB** | USB-C CDC-ACM (data + charging); no USB data path to the tapped cable (galvanically isolated) |
| **Isolation** | Reinforced isolation (5 kV) between the clamp front-end and the digital domain — protects the MCU and the operator from hazardous voltages on the target cable |
| **Form Factor** | 95 × 52 × 22 mm clamshell — the device *is* the clamp; closes around cables up to 9 mm OD |
| **Operating Temp** | −10 °C to +55 °C (battery-limited) |
| **BOM Cost** | ~$94 at 1K volume |

### 3.1 Coupling clamp design

The clamp is the heart of Pulse-Reaper. It is a hinged clamshell with two coupling elements per jaw:

- **Capacitive plates:** Two copper planes, one per conductor of the differential pair, sized to couple ~0.2 pF each through a 0.8 mm jacket. The plates are driven by the LMH5401 high-impedance FDA inputs. The plates are electrically isolated from the digital domain by a 5 kV reinforced isolation barrier (ISO7741 digital isolators + ADuM capacitor-isolated analog path).
- **Inductive sense coil:** A multi-turn ferrite-core coil wound around each jaw that senses the differential magnetic field of the pair (common-mode rejection by geometry). Useful for high-current buses (4–20 mA loops, POTS loop current) where the capacitive coupling is weak.
- **TDR injection element:** A separate small plate driven by the SRD pulse generator through a 50 Ω matched microstrip; injects the fast edge for reflectometry.

The jaws close around cables from 3 mm to 9 mm outer diameter with a mechanical latch and a rare-earth magnet backup. A leaf-switch detects jaw closure and gates the high-voltage TDR path to prevent accidental discharge when open.

---

## 4. Architecture & Block Diagram

```
                       ┌─────────────────────────────────────────────────────┐
                       │                  TARGET CABLE                         │
                       │   (through jacket, no contact with conductors)         │
                       └────────────────┬───────────────┬─────────────────────┘
                                        │               │
                                 (capacitive)    (inductive)
                                        │               │
                       ┌────────────────▼───────────────▼─────────────────────┐
                       │                COUPLING CLAMP                       │
                       │   Cap plates ×2   │   Sense coil ×2   │   TDR plate │
                       └────────┬──────────┴───────────┬────────┴────────────┘
                                │                      │
                ┌───────────────▼──────────┐  ┌────────▼─────────┐
                │  Analog Front-End (AFE)   │  │  TDR Pulse Gen    │
                │  LMH5401 FDA ×2 (gain     │  │  SRD + comparator  │
                │  selectable, ISO7741      │  │  FPGA-driven       │
                │  isolation)               │  │  200 ps edge       │
                └───────────────┬───────────┘  └────────┬─────────┘
                                │                       │
                                │              ┌────────▼─────────┐
                ┌───────────────▼──────────┐   │  AD9203 ADC      │
                │  Anti-alias + EQ filter   │   │  2× 10-bit       │
                │  ( Programmable via FPGA  │   │  100 MSa/s       │
                │  IIR/FIR)                 │   └────────┬─────────┘
                └───────────────┬──────────┘            │
                                │                       │
                                └──────────┬────────────┘
                                           │
                          ┌────────────────▼─────────────────┐
                          │  Lattice iCE40UP5K FPGA          │
                          │  - TDR reflectogram DSP           │
                          │  - CIC decimator / FIR equaliser  │
                          │  - Frame buffer (1 Mb SPRAM)      │
                          │  - Protocol bit-slicer            │
                          └────────────────┬─────────────────┘
                                           │ SPI + GPIO
                          ┌────────────────▼─────────────────┐
                          │  STM32H743VIT6 Main MCU           │
                          │  - State machine / modes          │
                          │  - Protocol parsers (Modbus RTU,  │
                          │    Profibus DP, HART, CAN, POTS)  │
                          │  - Capture to microSD (FAT32)     │
                          │  - BLE C2 protocol (AES-256-CTR)  │
                          │  - OLED UI                       │
                          └─┬──────────┬──────────┬───────────┘
                            │          │          │
                  ┌─────────▼──┐  ┌────▼────┐  ┌──▼──────────┐
                  │ nRF52840   │  │ SSD1306 │  │ W25Q128     │
                  │ BLE 5.0   │  │ OLED    │  │ SPI NOR     │
                  │ (C2 link) │  │ 128×64  │  │ 16 MB       │
                  └────────────┘  └─────────┘  └─────────────┘
                                            │
                                       ┌────▼────┐
                                       │ μSD     │
                                       │ UHS-I   │
                                       └─────────┘
```

### 4.1 Data flow

1. **TDR mode:** The FPGA drives a 200 ps edge into the TDR plate; the AD9203 samples the reflection at 100 MSa/s for 2 µs (≈ 200 m of cable at 0.67c). The FPGA computes the reflectogram (cross-correlation, windowing) and the MCU classifies the cable against a stored signature library.
2. **Sniff mode:** The LMH5401 FDA capacitively couples the differential signal; the FPGA runs a programmable FIR equaliser (to compensate jacket attenuation) and a bit-slicer that recovers the line code (NRZ, Manchester, HART FSK, CAN NRZ+bit-stuff, Profibus UART). The MCU parses the recovered frames (Modbus RTU, HART, etc.) and writes them to microSD + streams over BLE.
3. **Inject mode (unpowered buses):** The THS3491 drives the clamp with a low-amplitude differential signal; the jacket capacitance couples it onto the pair. Limited to low-frequency (< 1 MHz) and low-amplitude (< ±1.5 V) injection.
4. **Covert-channel mode:** Two Pulse-Reapers on the same cable pair; one modulates, the other recovers. Used for air-gap bridging demonstrations.

---

## 5. Firmware Details & Design Decisions

The firmware is a bare-metal C application for the STM32H743. It is organised as a cooperative state machine with interrupt-driven DMA for the high-throughput paths (ADC samples → FPGA, capture → microSD, BLE C2). Key design decisions:

- **Bare-metal, no RTOS.** The device has a small, well-defined mode set (TDR, sniff, inject, covert). An RTOS adds 8–16 KB of RAM and 30+ µs of context-switch latency for no benefit. A single interrupt-priority cooperative loop with DMA completion callbacks is simpler, more auditable, and lower-latency.
- **FPGA as a fixed-function DSP coprocessor.** The iCE40UP5K runs a bitstream compiled from a Verilog design that implements: the TDR pulse generator timing, the AD9203 ADC interface, a 64-tap programmable FIR equaliser, a CIC decimator (100 MSa/s → 1 MSa/s for slow protocols), and a frame buffer in the SPRAM. The MCU talks to the FPGA over SPI with a small command set (CONFIGURE_FILTER, ARM_TDR, READ_REFLECTOGRAM, READ_FRAME). This keeps the high-speed path in deterministic hardware and the protocol parsing in flexible C.
- **Galvanic isolation enforced in firmware.** The clamp front-end is on the isolated side of a 5 kV barrier; the MCU refuses to enable the inject driver unless the leaf-switch confirms the jaws are closed and the TDR reflectogram shows a connected cable (open-clamp injection is a safety hazard).
- **Protocol parsers are pluggable.** Each parser (Modbus RTU, Profibus DP, HART, CAN, POTS DTMF) is a small C struct with `detect()`, `parse()`, `format()` function pointers; the MCU auto-detects the protocol from the recovered bitstream and routes frames to the right parser. Adding a new protocol is a single file.
- **microSD capture is FAT32 with a ring buffer.** Captures are written as `.pcapng` (libpcap-compatible) so the operator can open them in Wireshark with the right dissector. A 64 MB ring buffer in the W25Q128 NOR flash absorbs BLE-silence bursts.
- **BLE C2 is encrypted and command-oriented.** The nRF52840 runs a Nordic SoftDevice BLE stack; the MCU talks to it over UART with a simple framed protocol. All commands and captured traffic are AES-256-CTR encrypted with a session key derived from an ECDH P-256 handshake on pairing. The BLE link is the only exfil path when USB is not connected.

### 5.1 File layout

```
firmware/
├── Makefile            — arm-none-eabi-gcc build for STM32H743
├── board.h             — pin map, peripheral assignments, clock tree
├── registers.h         — STM32H7 register definitions (subset)
├── main.c              — mode state machine + init
└── drivers/
    ├── board_init.c     — clocks, GPIO, NVIC, DMA
    ├── clamp_afc.c      — clamp front-end control (gain, coupling select)
    ├── fpga_dsp.c       — FPGA command interface (TDR, filter, frame read)
    ├── tdr_engine.c     — TDR acquisition + reflectogram classification
    ├── protocol_detect.c— auto-detect protocol from recovered bitstream
    ├── modbus_rtu.c     — Modbus RTU parser
    ├── profibus_dp.c    — Profibus DP parser
    ├── hart_fsk.c       — HART FSK parser (Bell 202)
    ├── can_native.c     — CAN / CAN-FD bit-slicer + parser
    ├── pots_dtmf.c     — POTS DTMF / caller-ID parser
    ├── sd_capture.c     — microSD FAT32 + pcapng writer
    ├── ble_c2.c         — BLE C2 protocol (framed, AES-256-CTR)
    ├── crypto.c         — ECDH P-256 + AES-256-CTR (mbedTLS-style stub)
    ├── oled.c           — SSD1306 driver + UI primitives
    └── power.c          — fuel gauge, charger, low-battery handling
```

See the firmware directory for the full source. Total firmware size is ~3,400 lines of C across all files.

---

## 6. Application/Software Interface

The companion app is a React Native (Expo) application targeting Android and iOS. It connects to Pulse-Reaper over BLE 5 using `react-native-ble-plx`, performs the ECDH key exchange, and presents:

- **Connection screen:** Scan for devices, pair, establish encrypted session.
- **TDR Reflectogram screen:** Live reflectogram plot; cable classification result; tap-point distance to each endpoint; termination quality indicator. The operator uses this to find the best clamp position.
- **Live Capture screen:** Real-time decoded fieldbus frames (Modbus RTU register reads, HART commands, CAN IDs, Profibus telegrams). Filter by address, register, function code.
- **Cable Map screen:** Aggregates TDR results from multiple clamps into a facility cable inventory (type, length, location, classification).
- **Inject screen:** Compose and send forged frames on unpowered buses; safety interlocks require jaw-closed + cable-detected confirmation.
- **Covert Channel screen:** Configure two Pulse-Reapers as a tx/rx pair on a shared cable; throughput graph.
- **Settings screen:** BLE key management, capture file naming, gain equaliser presets, firmware update over BLE (signed image).

The app source is in the `app/` directory.

---

## 7. Use Cases for Red Teams, Security Researchers & Pentesters

### 7.1 ICS / OT red team — Modbus RTU over RS-485

A red team with authorised access to a plant cable tray clamps Pulse-Reaper onto a 0.8 mm jacketed RS-485 pair running between a PLC and a remote I/O drop. TDR confirms the cable is Belden 9841 (120 Ω, ~140 m, properly terminated both ends). Sniff mode recovers every Modbus RTU frame; the operator sees register reads/writes to coils and holding registers in real time, including the unit-load of a legacy HMI that is polling every 200 ms — a fingerprint of the HMI's polling schedule that a later forged-frame injection can mimic.

### 7.2 Building automation — HART over 4–20 mA

A security researcher assessing a building-automation controller clamps onto a 4–20 mA sensor loop. HART FSK (Bell 202, 1200/2200 Hz) is recovered through the inductive sense coil (the loop current modulates the magnetic field). The researcher reads the HART universal commands (read manufacturer, read tag, read PV) and identifies the sensor model and firmware — useful for supply-chain risk assessment.

### 7.3 Automotive — CAN / CAN-FD in the harness

A pentester clamps onto a CAN twisted pair inside the vehicle harness (no need to break the sealed connector). CAN-FD frames up to 8 Mbps are recovered; the operator logs IDs and payloads for later replay or fuzzing from the Inject screen.

### 7.4 Physical pentest — POTS / telephone line reconnaissance

A physical pentester clamps onto a telephone riser pair in a building's telecom closet. POTS DTMF and caller-ID (FSK 1200 baud Bell 202) are recovered; the operator maps which extensions are active and which dial out. Useful for assessing whether analog lines are still in scope of the authorisation.

### 7.5 Air-gap bridging via shared copper

Two Pulse-Reapers on the same long cable run (e.g., a multi-pair riser cable that passes through both a secure room and an adjacent unsecure area) establish a low-rate covert channel by modulating a low-amplitude signal onto one pair and recovering it on the other. This demonstrates that "air-gapped" control rooms sharing a cable plant are not truly air-gapped.

### 7.6 Cable inventory during physical reconnaissance

During a facility walk-through, the operator clamps each accessible cable in a tray for 2 seconds; TDR classifies it (Cat 6 vs Profibus vs POTS vs 1553 twinax) and records length + tap location. This builds a map of the cable plant without opening a single enclosure — invaluable for scoping a later engagement.

### 7.7 Security research — fieldbus protocol fuzzing

On an unpowered test bench, the researcher uses Inject mode to drive a dry-loop Modbus RTU segment with malformed frames (illegal function codes, bad CRC, overlapping addresses) to probe slave firmware robustness — a reproducible fuzzing harness at the physical layer.

---

## 8. Safety Considerations

- **Hazardous voltages:** POTS ring voltage is up to 105 VAC RMS at 20 Hz; PoE is 48–57 VDC; some industrial loops carry mains. The clamp front-end is rated for 5 kV reinforced isolation, but the **operator must verify the target cable is within the rated voltage** before clamping. The firmware refuses to enable the inject driver if TDR detects a live conductor above 60 V.
- **No mains coupling:** Pulse-Reaper must never be clamped onto AC mains wiring. The TDR classifier will flag a mains cable (low impedance, 50/60 Hz reflection envelope) and refuse to enter sniff mode.
- **Battery safety:** 1800 mAh LiPo with BQ25896 charger and MCP73871 fuel gauge; over-voltage, over-current, and thermal protection per the charger IC. The pack is in a separate compartment from the clamp front-end.
- **Isolation barrier:** The digital domain (MCU, FPGA, BLE, USB, battery) is galvanically isolated from the clamp front-end by a 5 kV reinforced barrier. USB-C is on the safe side; the operator's laptop is never exposed to cable-side voltages.

---

## 9. Detection & Countermeasures

### 9.1 How a defender might detect Pulse-Reaper

- **TDR sweep from an endpoint.** A defender who periodically runs a high-resolution TDR from one endpoint of a cable can detect the small impedance bump introduced by the clamp (~0.5 pF is detectable at > 1 GHz TDR bandwidth, but below 500 MHz TDR it is marginal). This is the strongest countermeasure and is rarely deployed in practice on fieldbus cabling.
- **Link-quality monitoring.** On Ethernet, a PoE link-quality alarm that tracks bit-error-rate marginally increases (the clamp adds ~0.05 dB of attenuation); a baseline-deviation detector with < 0.1 dB resolution could flag it. Most installations do not monitor at this resolution.
- **Physical inspection.** The clamp is visible on the cable jacket. Periodic visual inspection of cable trays and risers is the most practical countermeasure.
- **Shielded cable.** Continuous 360° foil+braid shield, bonded at both ends, attenuates the capacitive coupling by > 60 dB. Pulse-Reaper will report "shielded — no coupling."

### 9.2 Recommended defensive posture

1. Use continuously-shielded, bonded twisted pair for any fieldbus carrying sensitive data.
2. Encrypt fieldbus traffic at the protocol layer (Modbus Security, HART-IP over TLS) where the protocol supports it.
3. Periodically TDR-sweep critical cable runs from both endpoints and record the baseline reflectogram; alert on any new reflection within 0.5 m of a known tap point.
4. Treat the cable jacket as a trust boundary; seal cable trays and risers; require tamper-evident seals on field junction boxes.

---

## 10. License & Credits

- **Author / Creator:** jayis1
- **Hardware license:** CERN-OHL-S v2
- **Firmware license:** GPL-2.0
- **App license:** MIT
- **Date:** 2026-08-17
- **Version:** 1.0

All designs, firmware, code, and documentation in this directory are © 2026 jayis1. Provided for authorized security research and educational use only.

> **Remember:** The cable jacket is not a security boundary. Treat every copper pair you can physically reach as already compromised.