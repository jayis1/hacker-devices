# CC-Stiletto

**USB-C Power Delivery Configuration Channel Attack Tool**

*Author: jayis1*
*Copyright (c) 2026 jayis1. All rights reserved.*

---

## Legal & Ethical Disclaimer

CC-Stiletto is a professional security research instrument designed exclusively for
authorized penetration testing, red team engagements, and defensive research on
hardware and firmware you own or have explicit written authorization to test.
Misuse — including attacking chargers, laptops, phones, or any equipment you do not
own or have permission to evaluate — may violate computer fraud, product safety, and
consumer protection statutes in your jurisdiction. USB-C Power Delivery operates at up
to 48 V / 5 A (240 W) under the Extended Power Range specification; deliberate
mis-negotiation of these levels can cause thermal damage, fire, or injury. The author
(jayis1) and contributors disclaim all liability for damage, injury, or legal
consequences arising from use of this design. **Use only on equipment you own or are
authorized to test, and only with appropriate electrical safety precautions.**

---

## 1. Purpose & Overview

CC-Stiletto is a pocket-sized hardware implant and research tool that attacks the
**Configuration Channel (CC)** of USB Type-C. The CC line is the single most critical —
and most under-examined — attack surface in modern USB-C: it is the bidirectional,
half-duplex, BMC-encoded (Biphase Mark Coding) wire over which two USB-C ports
negotiate voltage, current limits, data role (host vs. device), power role (source vs.
sink), and accessory modes via the **USB Power Delivery (PD)** protocol (up to PD 3.1 /
EPR).

Every modern laptop, phone, tablet, monitor, dock, and charger negotiates power and
data roles over CC before any USB traffic flows. CC-Stiletto inserts itself on the CC
line as a **transparent man-in-the-middle** (or as a standalone spoofing endpoint),
giving a red team the ability to:

1. **Sniff** every PD message exchanged between a source and sink, including the
   structured VDM (Vendor Defined Messages) that identify vendor identity, supported
   voltages, and alternate-mode entry sequences.
2. **Inject** arbitrary PD packets — spoofing Source_Capabilities, Request, BIST,
   Get_Source_Cap, FRS (Fast Role Swap) signals, and Hard Reset — to force a target port
   partner into states the manufacturer never intended.
3. **Glitch** the power delivery path by rapidly renegotiating voltage: forcing a sink
   to 20 V then back to 5 V in the middle of a current draw can cause brown-out or
   over-voltage faults in poorly protected devices — a hardware-level fault injection
   primitive delivered over a protocol that almost no endpoint validates.
4. **Hijack the data role** by issuing an unsolicited `DR_Swap` or `PR_Swap`, turning a
   device-mode victim into a host-mode adversary — a vector to turn a "dumb charger"
   scenario into a host that can enumerate the attached victim's USB descriptors.
5. **Dead-battery attack** — many devices implement the "dead battery" provision of the
   USB-C spec, sourcing VCONN and defaulting to 5 V even before the MCU firmware boots.
   CC-Stiletto can emulate a dead-battery sink to extract 5 V / VCONN from an unknown
   port for power harvesting, or present as a dead-battery source to coerce a target into
   a vulnerable bootstrap state.
6. **VCONN hijack** — take over the VCONN supply to power an embedded payload while the
   host thinks it is powering a passive cable.

The tool is built around an **STM32G474** (a 170 MHz Cortex-M4F with built-in USB-C
physical-layer support via two internal comparators and HRTIM for glitch timing) paired
with an **FUSB302B** USB-C PD transceiver for the BMC PHY. An onboard **LM5180** isolated
flyback and a bank of load-switch MOSFETs let CC-Stiletto actually source or sink
negotiated power, so it is not a purely passive listener — it can close the loop and
deliver or draw the voltage it spoofed.

### Why CC is an overlooked attack surface

Most USB security attention focuses on the data lines (D+/D-, USB 2.0 / 3.x / SS,
Alt-Mode DP/HDMI). But on USB-C the data lines are inert until the CC negotiation
completes. The CC negotiation itself is:

- **Unauthenticated.** PD has no cryptographic handshake. Any connected device can claim
  any capabilities.
- **Firmware-driven.** Negotiation runs in MCU firmware, often a separate PD controller
  or the main SoC's embedded controller — a different (and frequently weaker) code base
  than the OS USB stack.
- **Stateful.** A target can be coerced into `PD_STATE_TRANSITION_*` states that expose
  race windows for fault injection.
- **Power-coupled.** A successful spoof directly controls the voltage applied to the
  target's VBUS rail — a rare attacker-controlled analog primitive.

CC-Stiletto is the first purpose-built open tool to systematically exploit this layer.

---

## 2. Attack Surface & Threat Model

| Asset | Exposure |
|---|---|
| VBUS rail voltage | Attacker-controlled via spoofed `Source_Capabilities` + `Request`. Can drive 5/9/15/20/28/36/48 V onto a victim that expected 5 V. |
| Current limit | Spoofed `Request` can push a source past its real current rating, triggering thermal events in undersized chargers. |
| Data role | Unsolicited `DR_Swap` flips host/device roles; a "charger" can become a host enumerating the victim. |
| Power role / FRS | `FRS_Signal` can force a source to instant-become-sink, dumping VBUS back into an unprepared rail. |
| VCONN | Hijacking VCONN powers embedded payloads the host did not authorize. |
| Vendor VDMs | Proprietary structured VDMs (e.g. enter DisplayPort Alt-Mode) often carry unvalidated fields; fuzzable. |
| Dead-battery bootstrap | Devices in dead-battery state run a minimal PD controller with no input validation. |
| SOP' / SOP'' packets | Cable-marker packets on SOP' are frequently unauthenticated and can spoof an e-marker cable's identity to defeat over-current protections tied to e-marker advertised current rating. |

**Adversary model.** CC-Stiletto assumes physical access to the USB-C port under test
(or to a cable in-line between victim and source). The attacker can fully control the
CC and VBUS lines but cannot modify victim firmware a priori. Goals range from
information disclosure (vendor PD profile disclosure, e-marker identity) through denial
of service (force safe state / Hard Reset loops) to persistent code execution (data-role
swap followed by malicious USB enumeration).

**Out of scope.** Attacking the high-speed data lanes themselves (USB 3.x / DP Alt-Mode
physical layer) is handled by other tools; CC-Stiletto focuses on the PD negotiation and
power delivery control plane.

---

## 3. Hardware Specifications

### Microcontroller
- **STM32G474CEU6** — ARM Cortex-M4F, 170 MHz, 512 KB flash, 128 KB SRAM, CCM SRAM.
  - Selected for: integrated USB-C comparators (COMP1/COMP2 on CC lines for direct
    BMC sampling without an external PHY in low-cost mode), high-resolution timer
    (HRTIM) with 184 ps resolution for glitch timing, USB 2.0 FS device peripheral for
    the control interface, and dual DACs for VBUS voltage reference generation.

### USB-C PD PHY
- **FUSB302BMPX** — ON Semiconductor USB-C PD transceiver.
  - BMC encode/decode over CC, CC orientation detection, VCONN pass-through, SOP/SOP'/SOP'' support, I2C control.

### Power path
- **LM5180** — 1 W isolated flyback, generating an isolated 5 V rail from VBUS for the logic side so CC-Stiletto survives whatever VBUS voltage it negotiates.
- **TPS25940** — 5 A eFuse on the VBUS source path with adjustable current limit via DAC (1.5–5 A programmable).
- **IPK95N04L** — 40 V N-channel MOSFET bank (4× parallel) for VBUS sink/source switching, rated 60 A pulse.
- **MAX5370** — 4-channel 8-bit DAC for VBUS voltage reference, eFuse current limit, and PD PHY reference.

### Connectivity
- **USB 2.0 Full-Speed** — STM32 native USB device for control/status console to the companion app (CDC-ACM profile).
- **CC pass-through** — two FUSB302B PHYs: one for the "source side" and one for the "sink side", enabling true transparent MITM with independent CC monitoring on each side.
- **VBUS pass-through** — low-resistance bus (≈8 mΩ) with bidirectional current sense (INA226) on both legs.

### Sensors
- **INA226** ×2 — bidirectional VBUS voltage/current monitor (36 V, 16-bit), one on each leg.
- **NTC thermistor** — VBUS MOSFET bank temperature for thermal foldback.
- **Optical isolation** — ISO1540 I2C isolator between the VBUS-referenced power circuitry and the MCU logic.

### Form factor
- 52 mm × 18 mm × 7 mm USB-A-sized dongle with a USB-C plug on one end and a USB-C receptacle on the other — an in-line "cable" form factor indistinguishable at a glance from a short extension.
- Single status RGB LED, single tactile button (mode select / emergency Hard Reset), pogo-pin debug header on the underside.

### Power
- Self-powered from VBUS (3.0–20 V native, up to 48 V via the LM5180 flyback). Internal 100 µF bulk cap for transient hold-up during role swaps.

---

## 4. Architecture

```
 +----------------+        +---------------------+        +----------------+
 |  Victim / DUT  |<--VBUS->|    CC-Stiletto      |<--VBUS->|  Source / Host |
 |  USB-C port    |<--CC---->  (in-line MITM)     |<--CC---->  (charger/laptop)|
 +----------------+        +---------------------+        +----------------+
         ^                          ^                          ^
         |                          |                          |
   FUSB302B #2                 STM32G474                   FUSB302B #1
   (sink-side PHY)             (firmware core)             (source-side PHY)
         |                          |                          |
         +----------- I2C ----------+----------- I2C -----------+
                                    |
                          ISO1540 I2C isolator
                                    |
                       +------------+-------------+
                       |            |             |
                  INA226 #2    MAX5370 DAC    INA226 #1
                  (sink leg)   (Vbus ref +     (source leg)
                               eFuse limit)
                       |
                  TPS25940 eFuse
                       |
                  IPK95N04L ×4 MOSFET bank  <-- VBUS pass-through + glitch
                       |
                  LM5180 flyback --> 5 V isolated logic rail
                       |
                  USB FS device --> companion app
```

### Block diagram (logical data flow)

```
    CC-line BMC  ──►  FUSB302B PHY  ──►  I2C RX FIFO  ──►  PD parser  ──►  policy engine
                       (1+2)                                  │                │
                                                              │                ▼
                                                              │      ┌──────────────────┐
                                                              │      │ attack modules:  │
                                                              │      │ sniff / inject / │
                                                              │      │ spoof / glitch / │
                                                              │      │ role-swap / vconn│
                                                              │      └──────────────────┘
                                                              │                │
                                                              ▼                ▼
                                                       PD message builder ──► I2C TX ──► FUSB302B ──► CC line
                                                              │
                                                       HRTIM glitch trigger ──► MOSFET bank ──► VBUS
```

The firmware is organized as a **policy engine** on top of a thin PD protocol stack.
Each attack mode is a self-contained policy module that subscribes to PD events and
emits PD messages / hardware triggers. This separation lets new attack primitives be
added without touching the PHY drivers.

---

## 5. Firmware

The firmware (in `firmware/`) is bare-metal C targeting the STM32G474 with the
official STM32Cube headers (no HAL dependency — direct register access via the
`registers.h` definitions for auditability and minimal attack surface in the tool
itself).

### Files
- `main.c` — system initialization, USB-CDC console, main loop dispatching PD events to the active attack policy.
- `pd_phy.c` / `pd_phy.h` — FUSB302B driver (I2C register access, CC orientation, BMC FIFO read/write, SOP/SOP'/SOP'' framing).
- `pd_stack.c` / `pd_stack.h` — PD message encoder/decoder, header CRC, GoodCRC generation, message-id tracking, state machine skeleton.
- `policy_engine.c` / `policy_engine.h` — attack policy dispatcher: SNIFF, INJECT, SPOOF_VOLTAGE, GLITCH, ROLE_HIJACK, VCONN_HIJACK, DEAD_BATTERY.
- `power_path.c` / `power_path.h` — DAC programming, eFuse current limit, MOSFET bank / HRTIM glitch pulse generator, INA226 telemetry.
- `console.c` / `console.h` — text command interface over USB-CDC for the companion app and manual scripting.
- `board.h` — pin map, peripheral assignments, clock tree constants.
- `registers.h` — peripheral register base addresses and bit definitions used directly by the drivers.
- `Makefile` — arm-none-eabi-gcc build with `-Os -ffreestanding`, links at 0x08000000, produces `.bin` / `.elf` / `.hex`.

### Key design decisions

1. **Dual PHY, true MITM.** Two FUSB302B devices let the tool act as a transparent
   relay while rewriting PD frames in-flight — essential for voltage-glitch attacks
   where the source and sink must remain in agreement while CC-Stiletto alters the
   negotiated voltage mid-transaction.
2. **HRTIM-driven glitch.** The STM32G474's high-resolution timer generates
   sub-microsecond MOSFET switching pulses (down to ≈184 ps granularity) for
   power-glitch fault injection. This is the same timing primitive used by dedicated
   glitchers (e.g. ChipWhisperer) but delivered over the PD control plane — the victim
   requests the glitch by negotiating with CC-Stiletto.
3. **No HAL.** Direct register access keeps the tool's own attack surface minimal and
   makes timing deterministic — a HAL interrupt-driven I2C stack would jitter the
   glitch timing window.
4. **Policy modules are hot-pluggable.** The active policy is a function pointer table
   selected at runtime; switching modes does not require a reboot.
5. **Safety interlocks.** A hardware over-temperature comparator can force-cut the
   MOSFET bank independently of firmware, so a firmware crash cannot cause a sustained
   overcurrent.

---

## 6. Application / Software Interface

The companion app (`app/`) is a React Native (Expo) application providing:

- **Live PD sniffer view** — scrolling timeline of decoded PD messages with role/sop
  tags, hex dump, and known-message highlighting (Source_Capabilities, Request, VDM,
  Hard Reset, FR_Swap, etc.).
- **Capability editor** — compose arbitrary `Source_Capabilities` and `Request` packets
  via a GUI (PDO voltage/current selectors, Fixed/Variable/Battery/Augmented PDO
  types, EPR modes) and queue them for injection.
- **Glitch sequencer** — visual timeline to script voltage-step glitches (e.g. 20 V for
  1.2 ms → 5 V for 800 µs → 20 V) with one-tap arm/disarm.
- **Role-swap fuzzer** — automated `DR_Swap`/`PR_Swap`/`FRS` fuzzing with random
  inter-message timing.
- **Telemetry dashboard** — VBUS voltage/current on both legs, MOSFET temperature, eFuse
  state, active policy.
- **Capture export** — dump a session to PCAP-style JSON for offline analysis.

Communication with the device is over USB-CDC at 1 Mbaud using a simple line-based JSON
command protocol (`{"cmd":"sniff","sub":"start"}`, `{"evt":"pd","sop":0,"hdr":...}`).

---

## 7. Use Cases

### Red team
- **Malicious "charging station" drop.** Deploy CC-Stiletto as a public USB-C charging
  port. When a victim plugs in, CC-Stiletto negotiates as expected, then issues an
  unsolicited `DR_Swap`, becoming the USB host and enumerating the victim's device
  descriptors / attempting known USB attack payloads — turning a perceived power-only
  interaction into a data interaction.
- **In-line cable implant.** Place CC-Stiletto between a target laptop and its charger.
  Transparently relay PD, then on command glitch VBUS to fault the laptop during a
  sensitive boot or peripheral-init operation.
- **Charger characterization.** Discover a proprietary charger's real current limits and
  thermal response by issuing escalating `Request` objects — useful pre-engagement recon
  for follow-on hardware attacks.

### Security researchers
- **PD fuzzer.** Systematically fuzz a target's PD controller with malformed headers,
  bad message-ids, oversized PDO arrays, and out-of-spec SOP' packets to find parser
  bugs in the embedded PD stack.
- **E-marker spoofing.** Emulate SOP' packets claiming arbitrary e-marker cable
  identity and current rating, testing whether the source validates cable identity
  before granting high current / EPR voltages.
- **Vendor VDM reverse engineering.** Capture structured VDMs exchanged between a
  vendor laptop and dock to reverse-engineer proprietary Alt-Mode entry sequences.

### Penetration testers
- **Dead-battery extraction.** Demonstrate that an unpowered device exposes a live PD
  controller accepting unauthenticated messages — a persistence vector for delivering a
  malicious PD payload before the OS boots.
- **Compliance check.** Verify a client's USB-C ports implement the spec's required
  Hard Reset / Over-Temperature / Over-Current responses — a hardware security control
  that is rarely audited.

---

## 8. Bill of Materials (excerpt)

| Ref | Part | Function |
|-----|------|----------|
| U1 | STM32G474CEU6 | MCU / policy engine |
| U2,U3 | FUSB302BMPX | USB-C PD PHY (×2 for MITM) |
| U4 | LM5180-1W | isolated flyback, VBUS→5 V logic rail |
| U5 | TPS25940 | 5 A programmable eFuse |
| U6 | MAX5370 | 4-ch 8-bit DAC (vbus ref, ilimit, refs) |
| U7,U8 | INA226 | bidirectional V/I monitor (×2 legs) |
| U9 | ISO1540 | I2C digital isolator |
| Q1–Q4 | IPK95N04L | VBUS MOSFET bank |
| J1 | USB-C plug | to victim / DUT |
| J2 | USB-C receptacle | to source / host |
| J3 | pogo debug | SWD + UART |

---

## 9. Repository Layout

```
cc-stiletto/
├── README.md            this document
├── firmware/            STM32G474 bare-metal C firmware
│   ├── main.c
│   ├── pd_phy.c / .h    FUSB302B driver
│   ├── pd_stack.c / .h  PD message encode/decode + CRC
│   ├── policy_engine.c / .h
│   ├── power_path.c / .h
│   ├── console.c / .h
│   ├── board.h
│   ├── registers.h
│   └── Makefile
├── kicad/               KiCad 7 schematic + PCB + project
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/                 React Native (Expo) companion app
    ├── App.tsx
    ├── package.json
    ├── tsconfig.json
    └── src/  (screens, components, protocol)
```

---

## 10. Authoring & License

All firmware, hardware, and software in this directory were authored by **jayis1** and
are released for security research and educational use. Commercial deployment against
equipment you do not own is prohibited. See file headers for per-file attribution.

*Designed by jayis1 — 2026.*