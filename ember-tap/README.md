# Ember-Tap — USB-C Power Delivery Fuzzer & Midspan Implant

> **Author:** jayis1  
> **License:** Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT  
> **Status:** ✅ Complete (conceptual → schematic → PCB → firmware → app)

---

## ⚖️ Legal & Ethical Disclaimer

Ember-Tap is a **defensive and authorized-research tool only**. It is designed for
security researchers, red teams operating under signed Rules of Engagement, and
product developers hardening their USB-C / Power Delivery stacks.

**You MUST NOT** use Ember-Tap against devices, infrastructure, or individuals
without explicit written authorization from the asset owner. Unauthorized
manipulation of USB-PD negotiations can damage hardware (overvoltage on VBUS),
trigger lithium-battery safety events, and is illegal in most jurisdictions
under computer-fraud, product-tampering, and critical-infrastructure statutes.

By building or operating Ember-Tap you accept full responsibility for any
damage or legal consequence. The author (jayis1) provides this design **as-is,
without warranty**, and disclaims all liability for misuse.

If in doubt: **don't plug it in.**

---

## 1. What Ember-Tap Is

Ember-Tap is a pocket-sized **USB-C inline midspan** that sits between a power
source (charger, hub, host port) and a sink (phone, laptop, peripheral, battery
pack). Physically it looks like a short dongle: a USB-C plug on one end, a
USB-C receptacle on the other, and a tiny status OLED on the top face.

Unlike a passive pass-through, Ember-Tap actively participates in the **CC-line
Power Delivery negotiation** while leaving the high-speed SuperSpeed data lanes
electrically transparent. This gives the operator a man-in-the-middle position
on the PD contract itself — the most security-relevant and least-audited
control plane in the modern USB ecosystem.

### Core capabilities

| Mode | Description |
|---|---|
| **PD Sniff** | Passive capture of SOP/SOP'/SOP'' PD frames on CC1/CC2, decoded to a PCAP-style log over USB CDC. |
| **Contract Spoof** | Act as a source to a sink, advertising arbitrary PDO/VDO profiles — e.g. trick a device into drawing 20 V when it expected 5 V, or advertise a fake Fixed PDO to provoke parser bugs. |
| **Sink Masquerade** | Act as a sink to a source, requesting voltages/currents outside the real device's ratings to probe source-side soft limits and OVP behavior. |
| **Fuzz Engine** | Stateful PD-message fuzzer: mutates Header fields, message type, numobj, PDO contents, and chunking; replays malformed SOP frames with configurable timing. |
| **Hard Reset / DoS** | Injects Hard Reset, Soft Reset, and Get Source Cap messages to test device recovery and watchdog behavior. |
| **VBUS Glitch** | Coordinates with an onboard load-switch + TVS to momentarily droop or crowbar VBUS during a data transfer, testing sink-side brown-out handling. |
| **Dead Battery** | Emulates the "dead battery" Rp condition to force sources into a 5 V / current-limited boot state even when the sink is fully charged — useful for bypassing charge-gate logic. |
| **DFP/UFP Role Swap** | Forces role swaps and VCONN source handoff to probe state-machine bugs in dual-role ports. |
| **Data Tap** | Optional USB 2.0 (D+/D-) pass-through tap via the MCU's USB PHY for passive enumeration capture (no SuperSpeed mux — those pins are wire-OR'd). |

### Why this matters

USB-PD is now the universal power contract for laptops, phones, docks, e-bikes,
drones, and EVs (via J1772 → USB-C adapters). The spec is 700+ pages, the
state machines are intricate, and implementations are notoriously buggy:

* **Sink-side overvoltage:** a crafted Source Capabilities message can coerce a
  sink MOSFET gate driver to accept 20 V before its OVP comparator trips,
  destroying the device.
* **Source-side overcurrent:** requesting 5 A on a charger rated for 3 A can
  trigger thermal runaway in cheap chargers lacking I-sense.
* **Auth bypass:** ignoring or fuzzing the 16-byte Authenticated Message
  (SOP' to cable) lets an attacker use uncertified cables for high-power
  contracts.
* **Role-swap races:** forcing rapid DFP/UFP swaps has bricked docks in the
  wild.

There is **no** cheap, open, pocketable tool for systematically probing these.
Ember-Tap fills that gap.

---

## 2. Attack Surface & Threat Model

### 2.1 Assets under test

| Asset | Why it's sensitive |
|---|---|
| Sink PMIC / charge controller | Overvoltage → catastrophic MOSFET failure, battery thermal event. |
| Source charger firmware | Overcurrent → fire; PD controller MCU → firmware extraction via debug. |
| Cable e-marker (SOP') | Cloned/counterfeit cables passing as 5 A-capable. |
| Dock / hub mux | Role-swap races → data corruption, HDMI flicker, port bricking. |
| Host OS USB stack | Malformed PD messages reaching the EC → kernel panic on some laptops. |

### 2.2 Attack vectors Ember-Tap exercises

1. **Protocol fuzzing** of the PD message layer (header bitfields, object count,
   extended messages, chunking).
2. **Contract manipulation** — advertising/requesting PDOs outside the partner's
   real capabilities.
3. **Timing attacks** — tSenderResponse, tTypeCSendSourceCap, tPSTransition
   violations to trigger watchdog resets.
4. **VBUS power glitching** — synchronized droop/crowbar during data phases.
5. **Dead-battery spoofing** — Rp/Rd resistor manipulation on CC lines.
6. **Authentication fuzzing** — malformed SOP' Authenticated Messages.

### 2.3 Threat model & operator safety

Ember-Tap is the attacker (red-team tool). The defender is the device under
test. The operator must treat VBUS as potentially lethal to the DUT:

* Onboard **TVS diode** (SMBJ24A) + **polyfuse** (2 A) protect the operator's
  upstream port.
* **Hardware interlock:** the VBUS glitch FET is gated by a comparator that
  disables crowbar above 20 V to avoid arcing on low-rated connectors.
* **Current limit:** a load-switch (TPS25982) enforces a hard 5 A ceiling
  regardless of the negotiated contract.
* All high-voltage VBUS nodes are on a separate PCB partition with 2 mm creepage.

### 2.4 Out of scope

* SuperSpeed (USB 3.x) data manipulation — requires a redriver/retimer beyond
  the BOM target; wire-OR'd pass-through only.
* Thunderbolt / USB4 sideband — not captured.
* Active MUX of SBU1/2 (analog audio) — pass-through only.

---

## 3. Hardware Specifications

### 3.1 Bill of Materials (key parts)

| Ref | Part | Function | Package | ~$ |
|---|---|---|---|---|
| U1 | **STM32G474CBT6** | Main MCU, Cortex-M4 @ 170 MHz, 128 KB Flash, USB FS PHY | LQFP-48 | 5.20 |
| U2 | **FUSB302B** | USB-PD BMC transceiver, CC1/CC2, SOP/SOP'/SOP'' | WLCSP-12 / CSP | 1.40 |
| U3 | **TPS25982** | 5 A eFuse load switch, I²C current monitor, OVP | VQFN-16 | 2.10 |
| U4 | **TPS61040** | Boost for OLED + FUSB302 VCONN (5 V → 12 V, 50 mA) | SOT-23-6 | 0.90 |
| U5 | **SSD1306** | 0.96" OLED, 128×64, I²C | module | 2.50 |
| U6 | **SN65220** | USB 2.0 EMI filter + ESD, D+/D- | SON-8 | 0.60 |
| Q1 | **TPS2549** logic + **SI4410BDY** | VBUS glitch/load-dump N-FET | PowerPAK-8 | 1.10 |
| D1 | **SMBJ24A** | VBUS TVS, 24 V standoff | SMB | 0.40 |
| F1 | **MF-MSMF200** | 2 A polyfuse on VBUS-in | SMD 1812 | 0.50 |
| J1 | **USB4175** | USB-C plug, 24-pin, SMT | mid-mount | 1.80 |
| J2 | **USB4177** | USB-C receptacle, 24-pin, SMT | SMT | 1.70 |
| misc | passives, crystals, test points | — | — | ~3.00 |
| **Total BOM** | | | | **≈ $21** |

Well under the $100 budget.

### 3.2 MCU / radios / connectivity / power / form factor

| Attribute | Value |
|---|---|
| **MCU** | STM32G474, Cortex-M4F @ 170 MHz, 128 KB Flash, 32 KB SRAM |
| **PD PHY** | FUSB302B (BMC, CC1/CC2, hardware GoodCRC) |
| **Radios** | None (wired only) |
| **Sensors** | I²C current/voltage via TPS25982; onboard thermistor near glitch FET |
| **Host I/O** | USB 2.0 Full-Speed CDC (enumerates as `/dev/ttyACMx`) |
| **Display** | SSD1306 0.96" OLED, I²C @ 400 kHz |
| **Input** | 3 tactile buttons (UP/DOWN/SELECT) + 1 hardware-kill |
| **Power** | Parasitic from upstream VBUS (5–20 V), onboard buck → 3.3 V; ~35 mA draw |
| **Form factor** | 52 × 22 × 9 mm dongle, aluminum-shielded, OLED window |
| **Weight** | ~18 g |

### 3.3 Power domains

```
J1 VBUS (5–20 V) ── F1 polyfuse ── D1 TVS ── TPS25982 eFuse ──┬── J2 VBUS (negotiated)
                                                              └── buck (3.3 V) ── MCU/OLED/FUSB
```

The buck is a simple **MP4570** (4–36 V in, 3.3 V/1 A out) to handle the full
PD voltage range. The 3.3 V rail also feeds the FUSB302 and OLED.

---

## 4. Architecture & Block Diagram

```
        ┌──────────────────────────────────────────────────────────────────┐
        │                          EMBER-TAP                               │
        │                                                                  │
  USB-C │  CC1 ────────────────────────────────────────────────────────┐  │
  PLUG  │  CC2 ──────────────────────────────────────────────────────┐ │  │
  (J1)  │  VBUS ─ F1 ─ D1 ─ TPS25982 eFuse (I²C) ── Q1 glitch ─────┐│ │  │
        │  D+  ── SN65220 ──┐                                        ││ │  │
        │  D-  ── SN65220 ──┤                                        ││ │  │
        │  SBU  (pass)      │                                        ││ │  │
        │  SS   (pass)      │                                        ││ │  │
        └───────────────────┼────────────────────────────────────────┘│ │  │
                            │                                         │ │  │
                            │        ┌────────────────────┐            │ │  │
                            │        │   STM32G474        │            │ │  │
                            │        │  Cortex-M4 170MHz  │            │ │  │
                            │        │                    │            │ │  │
                            │  D+/D- │  USB FS PHY ◄──────┤ (CDC)      │ │  │
                            │        │                    │            │ │  │
                            │  I²C1  │  ┌──── FUSB302B ───┤ CC1/CC2    │─┘ │  │
                            │        │  │   (PD BMC)       │            │   │
                            │  I²C1  │  ├──── TPS25982 ────┤ I/V mon    ├───┘
                            │        │  │   (eFuse)         │            │
                            │  I²C2  │  ├──── SSD1306 ──────┤ OLED       │
                            │        │  │                   │            │
                            │  GPIO  │  ├──── 3× buttons ───┤ UI         │
                            │        │  ├──── Q1 gate ──────┤ VBUS glitch│
                            │        │  └──── KILL ─────────┤ HW interlock│
                            │        └────────────────────┘            │    │
                            └──────────────────────────────────────────┘    │
        ┌──────────────────────────────────────────────────────────────────┘
        │
  USB-C │  CC1 ────────────────────────────── (to DUT)
  RECEPT│  CC2 ────────────────────────────── (to DUT)
  (J2)  │  VBUS ────────────────────────────── (negotiated voltage)
        │  D+/D- pass-through
        │  SBU pass-through
        │  SS1/SS2 pass-through (wire-OR, no mux)
        └─────────────────────────────────────────────────────
```

### 4.1 Data flow

1. **CC lines** are NOT pass-through. They terminate at the FUSB302B. The MCU
   originates all PD messages toward the DUT (sink) and toward the source,
   acting as a true MITM on the PD layer. GoodCRC is handled in hardware by
   the FUSB302.

2. **VBUS** passes through the eFuse + optional glitch FET. Voltage/current
   is monitored in real time via I²C.

3. **D+/D-** pass through an EMI filter to the MCU's USB PHY for CDC + optional
   USB-2 passive tap. When the MCU is in "transparent" USB mode, it bridges
   D+/D- from J1 to J2 via a USB 2.0 mux (TS3USB221A, not shown in main BOM
   but included in the full schematic) so the DUT still enumerates with the
   host while Ember-Tap observes.

4. **SuperSpeed** pairs are wire-OR'd (direct PCB trace pass-through) with
  appropriate AC-coupling caps. No active mux — keeps BOM low and preserves
  signal integrity for 5/10 Gbps links.

---

## 5. Firmware

### 5.1 Overview

The firmware is bare-metal C targeting the STM32G474 with a custom
cooperative scheduler (no OS). It is organized as:

```
firmware/
├── main.c                  — scheduler, state machine, CLI dispatch
├── registers.h             — STM32G474 peripheral register definitions
├── board.h                 — pin map, clock config, board constants
├── drivers/
│   ├── pd_engine.c/.h      — FUSB302 driver + PD state machine + fuzzer
│   ├── power_monitor.c/.h  — TPS25982 I²C driver, VBUS glitch control
│   ├── usb_cdc.c/.h        — USB 2.0 FS CDC device + descriptors
│   ├── display.c/.h        — SSD1306 I²C driver, UI renderer
│   └── buttons.c/.h        — debounced GPIO button handler
└── Makefile                — arm-none-eabi-gcc build
```

### 5.2 Key design decisions

* **No HAL.** Direct register access via `registers.h` for deterministic
  timing — the PD fuzzer needs sub-millisecond control over tSenderResponse.
* **Hardware GoodCRC.** The FUSB302 auto-replies GoodCRC for accepted SOP
  frames, so the MCU only deals with application-layer PD messages.
* **Ring-buffers** for PD frame capture (4 KB) and CDC TX (2 KB) avoid
  dynamic allocation.
* **Stateful fuzzer** understands the PD contract FSM: it won't send a
  Request before receiving Source Cap, etc. Mutations are applied to the
  *next valid message in the FSM sequence*, maximizing crash coverage vs.
  random fuzzing.
* **VBUS glitching** is gated by a hardware comparator (on the TPS25982's
  FLTB pin) — firmware cannot crowbar VBUS above the configured OVP threshold.

### 5.3 USB-CDC command interface

The device enumerates as a virtual serial port (`/dev/ttyACM0`). Commands:

| Command | Action |
|---|---|
| `sniff on/off` | Passive PD capture to the serial stream |
| `spoof src <vdo_json>` | Act as source, advertise custom PDOs |
| `sink req <mv> <ma>` | Request a specific voltage/current from real source |
| `fuzz start <count> <seed>` | Begin stateful PD fuzzing campaign |
| `fuzz profile <name>` | Load a mutation profile (header, pdo, timing, chunk) |
| `hardreset` | Send Hard Reset SOP |
| `roleswap` | Force DFP↔UFP role swap |
| `vbus glitch <us> <type>` | Droop/crowbar VBUS for N µs |
| `deadbattery on/off` | Toggle Rp to emulate dead-battery sink |
| `log dump` | Dump captured PD frames as hex |
| `status` | Current mode, VBUS mV/mA, temperature |

---

## 6. Companion App

A React Native app (`app/`) provides a touch UI for operators who prefer a
phone over a serial terminal. It connects to Ember-Tap over **Bluetooth**
(optional HC-05 module on UART3, not in the base BOM) or via a **USB-C OTG
serial** bridge. Screens:

* **Dashboard** — live VBUS voltage/current graph, PD contract summary,
  current mode.
* **Sniffer** — scrolling decoded PD message list (type, PDOs, flags).
* **Fuzzer** — campaign config (count, seed, mutation profile), start/stop,
  live crash counter.
* **Spoof** — PDO editor (drag to set V/I), "advertise" button.
* **Glitch** — VBUS glitch parameter sheet with armed/safe toggle.
* **Logs** — export captured PCAP-style file to phone storage.

See `app/README` inline for build instructions.

---

## 7. Use Cases

### Red team

* **Implant drop:** leave Ember-Tap inline with a target's phone charger;
  it negotiates 5 V passthrough (invisible) until a trigger command flips it
  into spoof mode, coercing 20 V into an unprotected phone — a destructive
  denial-of-service disguised as a charger.
* **Dock bricking:** rapid role-swap + Hard Reset storms against a target
  dock to brick its PD controller mid-engagement.
* **Charger fingerprinting:** the PD Source Cap response is often unique per
  charger model; Ember-Tap logs it for physical-device attribution.

### Security researcher

* **Sink parser fuzzing:** feed mutated Source Cap messages to a new PMIC
  evaluation board to find OOB-write / overflow bugs in its PD controller
  firmware.
* **Cable e-marker audit:** send SOP' Get Identity to uncertified cables and
  record whether they (incorrectly) respond with VDOs.
* **Spec-conformance:** verify a DUT's tSenderResponse and tPSTransition
  timing against the USB-PD 3.1 spec.

### Pentester

* **Charging-station attack:** plug into a public USB-C charging station,
  Ember-Tap negotiates a safe 5 V with the station while feeding the client
  device a crafted contract — demonstrates the need for USB data-blockers
  that also cover the CC layer, not just D+/D-.

### Product developer (defender)

* **OVP validation:** confirm your sink's OVP comparator trips within spec
  when Ember-Tap advertises 20 V after agreeing on 5 V (the classic "PD
  contract bait-and-switch").
* **Brown-out test:** use VBUS glitch mode to verify your device survives
  momentary droops during a firmware-update write.

---

## 8. Repository Layout

```
ember-tap/
├── README.md                       ← you are here
├── firmware/
│   ├── main.c
│   ├── registers.h
│   ├── board.h
│   ├── drivers/
│   │   ├── pd_engine.c
│   │   ├── pd_engine.h
│   │   ├── power_monitor.c
│   │   ├── power_monitor.h
│   │   ├── usb_cdc.c
│   │   ├── usb_cdc.h
│   │   ├── display.c
│   │   ├── display.h
│   │   ├── buttons.c
│   │   └── buttons.h
│   └── Makefile
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── App.js
    ├── package.json
    ├── screens/
    │   ├── DashboardScreen.js
    │   ├── SnifferScreen.js
    │   ├── FuzzerScreen.js
    │   ├── SpoofScreen.js
    │   ├── GlitchScreen.js
    │   └── LogsScreen.js
    ├── components/
    │   ├── PDMessage.js
    │   └── VBUSGauge.js
    └── utils/
        └── protocol.js
```

---

## 9. Building & Flashing

### Firmware

```bash
cd ember-tap/firmware
make            # arm-none-eabi-gcc, produces build/embertap.elf + .bin
make flash      # openocd via ST-Link (SWD)
```

### KiCad

Open `kicad/device.kicad_pro` in KiCad 7+. Schematic and PCB are fully
placed and DRC-clean.

### App

```bash
cd ember-tap/app
npm install
npx react-native run-android    # or run-ios
```

---

## 10. Credit & License

Designed and authored by **jayis1**. All firmware, schematics, and app code
credit jayis1 as the creator. Released under the licenses in the top-level
repo `README.md`.

*Hardware (KiCad):* CERN-OHL-S v2  
*Firmware (C):* GPL-2.0  
*App (JS):* MIT  

© jayis1. See `LICENSE` files in each subdirectory.