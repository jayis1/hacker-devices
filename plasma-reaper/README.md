# PlasmaReaper — Multi-Vector Fault Injection Toolkit

```
   ╔══════════════════════════════════════════════════════════════════════╗
   ║   ██████╗ ██████╗  █████╗ ██╗███╗   ██╗ ██████╗  ██████╗██████╗     ║
   ║  ██╔════╝██╔══██╗██╔══██╗██║████╗  ██║██╔════╝██╔═══╝██╔══██╗    ║
   ║  ██║     ██████╔╝███████║██║██╔██╗ ██║██║     ██║   ██████╔╝    ║
   ║  ██║     ██╔══██╗██╔══██║██║██║╚██╗██║██║     ██║   ██╔══██╗    ║
   ║  ╚██████╗██║  ██║██║  ██║██║██║ ╚████║╚██████╗╚██████╗██║  ██║    ║
   ║   ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═════╝╚═╝  ╚═╝    ║
   ║         Multi-Vector Glitch Injection Toolkit                        ║
   ║         Author: jayis1                                               ║
   ╚══════════════════════════════════════════════════════════════════════╝
```

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange) ![HW-License](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> **Author:** jayis1
> **License:** GPL-2.0 (firmware & app) / CERN-OHL-S v2 (hardware)
> **Status:** Complete research hardware design — firmware + KiCad PCB + companion app

---

## ⚠️ LEGAL & ETHICAL DISCLAIMER

PlasmaReaper is designed **exclusively** for authorized security research, penetration testing under explicit written contract, hardware security evaluations of devices you own or have written permission to assess, and academic fault-injection research. Unauthorized glitching, voltage manipulation, or electromagnetic injection against devices you do not own may violate computer-fraud and abuse statutes (e.g., 18 U.S.C. § 1030 CFAA), the Digital Millennium Copyright Act (DMCA) anti-circumvention provisions (17 U.S.C. § 1201), and tampering laws in your jurisdiction. Fault injection against safety-critical systems (medical devices, automotive ECUs, avionics, industrial controllers) can cause physical damage, malfunction, and personal injury. **The author (jayis1) assumes no liability for misuse, property damage, or personal injury.** Always obtain proper written authorization before deployment, and never use this device on operational safety-critical equipment. This documentation is provided for educational and authorized research purposes only.

---

## Table of Contents

1. [Overview](#1-overview)
2. [What Makes PlasmaReaper Novel](#2-what-makes-plasmareaper-novel)
3. [Attack Surface & Threat Model](#3-attack-surface--threat-model)
4. [Hardware Specifications](#4-hardware-specifications)
5. [Architecture & Block Diagram](#5-architecture--block-diagram)
6. [Theory of Operation — Multi-Vector Fault Injection](#6-theory-of-operation--multi-vector-fault-injection)
7. [Firmware Details & Design Decisions](#7-firmware-details--design-decisions)
8. [Application/Software Interface](#8-applicationsoftware-interface)
9. [Use Cases for Red Teams, Security Researchers & Pentesters](#9-use-cases-for-red-teams-security-researchers--pentesters)
10. [Bill of Materials](#10-bill-of-materials)
11. [Legal & Ethical Disclaimer](#11-legal--ethical-disclaimer)

---

## 1. Overview

**PlasmaReaper** is a portable, USB-powered, multi-vector fault injection
toolkit designed for hardware security researchers and red teams conducting
authorized embedded security assessments. Unlike single-domain glitchers
that support only voltage glitching or only clock glitching, PlasmaReaper
integrates **three independent fault injection domains** into one compact
device:

1. **Power glitching** — transient voltage droops on the target's VCC rail
   via a MOSFET shunt circuit with sub-100 ns edge transitions.
2. **Clock glitching** — injection of extra or skipped clock edges into the
   target's external clock line via a fast multiplexer.
3. **Electromagnetic (EM) fault injection** — localized, high-current EM
   pulses through a hand-wound injection probe positioned over the target
   die, inducing transient currents in internal logic.

All three vectors share a unified **trigger pipeline** with sub-nanosecond
jitter, sourced from: GPIO edge triggers, serial trigger words, power
envelope threshold crossings, or a free-running timer. The device captures
target response over UART and reports success/failure back to the operator
in real time, enabling fully automated sweep campaigns.

The core controller is an **STMicroelectronics STM32H723** Cortex-M7 MCU
running at 550 MHz, chosen for its high-resolution HRTIM (High-Resolution
Timer) peripheral capable of 184 ps timing resolution — critical for
precise glitch placement. An onboard **Lattice iCE40UP5K FPGA** handles
the fast trigger logic and glitch-shape generation, allowing glitch
sequences with deterministic sub-cycle timing that would be impossible
from the MCU alone. A **TI LM53603** buck regulator provides the clean
analog supply rail, and a **MOSFET-based shunt** with selectable series
resistance performs the actual voltage glitching.

The companion React Native app communicates over USB CDC (Android) or
BLE (iOS), providing a visual glitch-parameter editor, sweep configurator,
live oscilloscope-style preview of the last glitch waveform (sampled by the
internal ADC), and a results dashboard showing success rates across the
parameter space as a heat-map.

### Key Capabilities

- **Three glitch vectors** (power, clock, EM) independently or jointly controllable.
- **184 ps timing resolution** via STM32H723 HRTIM for precise glitch placement.
- **FPGA-assisted trigger** with deterministic sub-10 ns trigger-to-glitch latency.
- **Multi-source triggering**: GPIO edge, UART trigger word, power envelope
  threshold, manual, or timer.
- **Automated parameter sweeps** with 2-D heat-map visualization (e.g., glitch
  offset × glitch width, or glitch voltage × glitch duration).
- **Target response capture** via UART — the firmware recognizes expected
  success/failure strings and classifies outcomes automatically.
- **Glitch waveform capture** — internal 12-bit ADC samples the VCC rail
  during each glitch for post-shot verification and debugging.
- **Battery-backed RTC** for timestamped campaign logs.
- **Open and scriptable** — the entire device is controllable from a
  Python library (`plasmareaper.py`) for integration into custom research
  pipelines.

---

## 2. What Makes PlasmaReaper Novel

Most commercially available and DIY fault-injection tools fall into one
of three categories, each limited to a single vector:

| Tool class | Vector | Limitation |
|---|---|---|
| Voltage glitcher (e.g., ChipWhisperer-Lite, PicoGlitcher) | Power only | Cannot inject clock or EM faults |
| Clock glitcher (e.g., Glitcher boards from Riscure) | Clock only | Requires access to the target's external clock |
| EMFI benchtop (e.g., Riscure EM Probe, Torkel) | EM only | Bulky, expensive, requires positioning stage |

**PlasmaReaper is the first open design to combine all three vectors in
one portable, USB-powered tool.** This matters because real-world targets
respond differently to each vector:

- **Power glitching** is most effective on targets where the attacker has
  direct VCC access (e.g., a debug pin, an exposed regulator input, or
  a battery contact) but is blocked by on-die brown-out detectors when
  the glitch depth is too large.
- **Clock glitching** is most effective on targets with external clock
  sources (crystals, oscillators, or clock lines routed on the PCB), and
  is the technique of choice for skipping instructions in pipelined
  CPUs where a missed clock edge causes a NOP-like effect.
- **EM fault injection** requires no electrical contact — the injection
  probe is held near the die surface — making it the only option for
  targets in hardened enclosures or chip-on-board packages where VCC
  and clock lines are inaccessible.

By integrating all three, PlasmaReaper allows a researcher to quickly
characterize a target's susceptibility to each vector in a single session,
without re-cabling or switching tools. The unified trigger pipeline and
parameter model mean that a sweep defined for one vector can be re-targeted
to another with a single parameter change — the firmware abstracts the
physical glitch generation behind a common `glitch_params_t` structure.

### Additional novel features:

- **Joint vector injection**: PlasmaReaper can fire power + clock glitches
  simultaneously (or with a controlled inter-vector delay), creating
  compound faults that are more likely to bypass protection than either
  vector alone. This is a research-grade capability previously available
  only in six-figure commercial tools.
- **FPGA-assisted glitch shape generation**: the iCE40UP5K synthesizes
  arbitrary glitch shapes (square, triangular, double-pulse, ramp) with
  deterministic timing, free from MCU interrupt jitter.
- **Closed-loop glitch verification**: after each shot, the internal ADC
  samples the VCC rail and the firmware classifies the glitch as "clean",
  "overshoot", or "no-effect", allowing the sweep controller to skip
  ineffective parameter combinations automatically.
- **Target response classification**: the UART monitor recognizes up to
  16 configurable success/failure patterns (regex-lite) and tags each
  shot with its outcome, enabling fully autonomous exploitation sweeps.

---

## 3. Attack Surface & Threat Model

### 3.1 Target Surfaces

PlasmaReaper is designed to attack the following surfaces on embedded targets:

| Surface | Access requirement | Vector used |
|---|---|---|
| VCC rail (direct) | Probe on target VCC | Power glitch |
| VCC rail (indirect) | Probe on regulator input or battery contact | Power glitch |
| External clock line | Probe on target OSC-IN or CLK-IN | Clock glitch |
| Internal clock (crystal) | None (EM probe over die) | EM fault injection |
| Boot ROM execution | Trigger on reset deassertion | Power / clock / EM |
| Secure-boot verification | Trigger on boot start, glitch during signature check | Power / clock / EM |
| Readout protection bypass | Trigger on debug unlock command | Power / clock / EM |
| Crypto operation | Trigger on operation start (side-channel sync) | Power / clock / EM |

### 3.2 Threat Model

**Attacker capabilities assumed:**

- Physical access to the target device for the duration of the engagement.
- Ability to probe target VCC, GND, clock, UART, and reset lines (for
  power/clock glitching); or ability to position an EM probe near the
  target die (for EMFI, which may require decapsulation of the chip
  package for best results, though some EMFI is possible through plastic
  packages).
- Ability to monitor target UART or another response channel to detect
  successful glitches.

**Attacker goals:**

- Bypass secure-boot signature checks to boot unsigned code.
- Bypass readout protection (RDP) to dump flash contents.
- Skip authentication checks (e.g., PIN verification, debug-unlock
  challenge-response).
- Induce faults in cryptographic operations (DFA — Differential Fault
  Analysis) to recover secret keys.
- Skip loop counters or bound checks to bypass secure-state-machine
  transitions.

**Defender mitigations that PlasmaReaper is designed to overcome:**

- Brown-out detectors (BOD): swept glitch depth allows finding the
  narrow window below the BOD threshold but above the glitch-too-deep
  threshold where the target faults without resetting.
- Watchdog timers: the glitch is fired in a single shot, completing
  before the watchdog can react.
- Clock-monitoring PLLs: the clock glitcher can inject extra edges
  within a single cycle, faster than the PLL's lock-detection window.
- Glitch detection circuits: the multi-vector approach allows finding
  vectors that the detector is not monitoring (e.g., a chip with good
  power-glitch detection may be vulnerable to EMFI).

### 3.3 What PlasmaReaper is NOT

- PlasmaReaper is not a laser fault injection tool. Optical/laser FI
  requires decapsulation and precision optical positioning stages that
  are beyond the scope of a portable USB tool.
- PlasmaReaper is not a side-channel analysis tool. It does not capture
  power traces for CPA/DPA attacks (see the `sideprobe` device in this
  repository for that). However, PlasmaReaper's trigger input can be
  driven by an external side-channel trigger, allowing the two devices
  to be combined for synchronized fault injection during a specific
  cryptographic round.

---

## 4. Hardware Specifications

### 4.1 Core Controller

| Component | Part | Role |
|---|---|---|
| MCU | STMicroelectronics STM32H723ZGT6 | Main controller, sweep logic, UART monitor, ADC sampling, USB CDC, BLE |
| FPGA | Lattice iCE40UP5K-SG48 | Fast trigger logic, glitch shape generation, deterministic timing |
| FPGA flash | Macronics MX25L3233F | Stores iCE40 bitstream |

### 4.2 Glitch Output Stages

**Power glitch stage:**
- Shunt MOSFET: Vishay SiS413DN (N-channel, 40 V, 36 A, 18 mΩ)
- Series pass MOSFET: Infineon BSC060N10NS3G (N-channel, 100 V, 100 A, 6 mΩ)
- Selectable series resistance: 0.5 Ω to 50 Ω in 8 logarithmic steps
  (analog switch mux: TI TMUX1108)
- Glitch depth: adjustable from 0 V (full shunt) to VCC (no glitch) in
  256 steps via a DAC (TI DAC5311, 10-bit, 500 kSps)
- Glitch width: 50 ns to 10 ms (HRTIM controlled)
- Glitch repeat: 1 to 65535 shots per trigger, with inter-shot delay

**Clock glitch stage:**
- Fast mux: TI TS3A5018 (4:1 mux, 1.8 GHz bandwidth, sub-ns switching)
- Glitch shape: extra edge injection (short high pulse between two lows)
  or edge suppression (short low between two highs)
- Clock source: onboard 100 MHz SiT9102 oscillator (programmable via I²C)
  or passthrough from target's own clock
- Glitch placement: 0 to 2^32 cycles from trigger, 184 ps resolution

**EM fault injection stage:**
- Pulse generator: Diodes Inc. ZXGD3004E1 gate driver, 4 A peak
- Injection probe: hand-wound 20-turn coil on Fair-Rite 43 material core
  (ferrite), ~200 nH, 5 mm tip diameter
- Pulse voltage: 12 V to 60 V (boost converter: TI LM5155)
- Pulse width: 10 ns to 500 ns
- Pulse current: up to 30 A peak in the coil
- Repetition: single shot to 100 Hz (thermal limited)

### 4.3 Trigger Inputs

| Source | Connector | Latency to glitch |
|---|---|---|
| GPIO edge (active) | U.FL | < 10 ns (FPGA path) |
| UART trigger word | UART RX (3.3 V) | < 1 µs (MCU detection) |
| Power envelope threshold | Analog SMA | < 50 ns (comparator + FPGA) |
| Manual (button) | Onboard | < 100 µs (MCU) |
| Timer (free-running) | Internal | < 184 ps jitter (HRTIM) |
| External sync (from sideprobe) | U.FL | < 10 ns (FPGA path) |

### 4.4 Monitoring & Feedback

- **VCC rail ADC**: STM32H723 internal 12-bit ADC, 3.6 MSps, samples
  the glitch waveform for post-shot verification.
- **Target UART monitor**: USART3 with DMA, continuously captures target
  output for response classification.
- **Glitch-shot counter**: 32-bit, persists in RTC backup domain.
- **Status OLED**: 0.96" SSD1306, 128×64, shows current mode, shot count,
  last outcome, and sweep progress.

### 4.5 Connectivity & Power

| Interface | Part | Purpose |
|---|---|---|
| USB 2.0 FS CDC | STM32H723 USB | Host communication (Android/Linux) |
| BLE 5.0 | STM32WB55 module (AT command mode over USART2) | iOS communication, wireless triggering |
| UART console | USART1 | Debug log, 3.3 V, 115200 to 3 Mbps |
| SD card | MicroSD slot (SDIO) | Campaign log storage |
| Power input | USB-C 5 V / 2 A | Primary power |
| Battery | 18650 Li-ion in holder | Backup for EMFI field work (~3 hours) |

### 4.6 Form Factor

- PCB: 100 mm × 60 mm, 4-layer, 1.6 mm FR4
- Enclosure: 3D-printed PLA, 110 × 70 × 25 mm
- Weight: ~85 g (without battery), ~145 g (with 18650)
- Operating temperature: 0 °C to 40 °C
- The EM injection probe is on a 30 cm coaxial fly-lead with a BNC
  connector, allowing free positioning over the target.

---

## 5. Architecture & Block Diagram

```
                        ┌───────────────────────────────────────────────┐
                        │              STM32H723 (Cortex-M7)            │
                        │                                               │
                        │  ┌───────────┐  ┌───────────┐  ┌───────────┐ │
                        │  │ Sweep     │  │ UART      │  │ USB CDC   │ │
                        │  │ Controller│  │ Monitor   │  │ + BLE     │ │
                        │  └─────┬─────┘  └─────┬─────┘  └─────┬─────┘ │
                        │        │              │              │       │
                        │  ┌─────▼─────┐  ┌─────▼─────┐  ┌─────▼─────┐ │
                        │  │ HRTIM     │  │ ADC       │  │ SDIO      │ │
                        │  │ Glitch    │  │ Waveform  │  │ Logging   │ │
                        │  │ Timing    │  │ Capture   │  │           │ │
                        │  └─────┬─────┘  └───────────┘  └───────────┘ │
                        │        │                                     │
                        │  ┌─────▼─────────────────────────────────┐   │
                        │  │ SPI → iCE40UP5K FPGA                   │   │
                        │  │  ┌──────────────┐  ┌───────────────┐  │   │
                        │  │  │ Trigger      │  │ Glitch Shape  │  │   │
                        │  │  │ Arbitration  │──│ Generator     │  │   │
                        │  │  │ < 10 ns      │  │ (arbitrary)   │  │   │
                        │  │  └──────┬───────┘  └───────┬───────┘  │   │
                        │  └─────────┼─────────────────┼──────────┘   │
                        └────────────┼─────────────────┼──────────────┘
                                     │                 │
                        ┌────────────▼───┐   ┌─────────▼──────────┐
                        │  Trigger Mux   │   │  Glitch Output Mux │
                        │  - GPIO U.FL   │   │  - Power shunt     │
                        │  - UART word   │   │  - Clock mux       │
                        │  - Power env   │   │  - EM pulse driver │
                        │  - Manual      │   │                    │
                        │  - Timer       │   │                    │
                        │  - Ext sync    │   │                    │
                        └────────────────┘   └────────────────────┘
                                                    │
              ┌─────────────────┬───────────────────┼──────────────────┐
              ▼                 ▼                   ▼                  ▼
        ┌──────────┐     ┌──────────┐       ┌────────────┐      ┌──────────┐
        │ Power    │     │ Clock    │       │ EM Pulse   │      │ ADC      │
        │ Glitch   │     │ Glitch   │       │ Driver     │      │ Monitor  │
        │ Shunt    │     │ Mux      │       │ + Coil     │      │ (VCC)    │
        │ MOSFET   │     │          │       │            │      │          │
        └────┬─────┘     └────┬─────┘       └─────┬──────┘      └──────────┘
             │                │                   │
             ▼                ▼                   ▼
        Target VCC      Target CLK            Target die
                                            (EM probe)
```

### 5.1 Data Flow Summary

1. The host (phone or PC) sends a glitch parameter set over USB CDC or BLE.
2. The MCU's sweep controller programs the HRTIM and FPGA with the timing
   parameters and arms the trigger.
3. When a trigger event arrives (GPIO edge, UART word, power threshold,
   or timer expiry), the FPGA generates the glitch shape with deterministic
   timing and routes it to the selected output stage(s).
4. The power shunt MOSFET briefly pulls the target VCC down, the clock
   mux injects/suppresses an edge, and/or the EM pulse driver fires the
   coil — individually or in combination.
5. Simultaneously, the ADC samples the VCC rail to capture the glitch
   waveform for verification.
6. The UART monitor captures the target's response and classifies it
   against the configured success/failure patterns.
7. The outcome (success, failure, no-response, glitch-invalid) is logged
   to SD card and reported back to the host for heat-map visualization.
8. If a sweep is in progress, the controller advances to the next
   parameter combination and re-arms.

---

## 6. Theory of Operation — Multi-Vector Fault Injection

### 6.1 Power Glitching

A power glitch is a brief, intentional reduction in the target's supply
voltage. The goal is to cause the target's internal logic to malfunction
without triggering a full reset. The effect depends on:

- **Glitch depth** (how low VCC goes): too shallow and nothing happens;
  too deep and the target's brown-out detector (BOD) triggers a clean
  reset. The useful window is typically between 30% and 70% of nominal
  VCC, and is target-specific.
- **Glitch width** (how long VCC is held low): too short and the logic
  recovers; too long and the target resets. The useful window is
  typically between 50 ns and 1 µs for most MCUs.
- **Glitch timing** (when in the target's execution the glitch occurs):
  must be precisely aligned to the instruction or operation to be
  faulted. This is where the 184 ps HRTIM resolution and the multi-source
  trigger pipeline are critical.

PlasmaReaper implements the power glitch as follows: the series pass
MOSFET is held on (connecting target VCC to the supply) normally. During
the glitch, the FPGA fires the shunt MOSFET gate, pulling target VCC
toward GND through the selected series resistance. The DAC sets the
shunt gate voltage, which controls the effective glitch depth. The HRTIM
controls the glitch width.

### 6.2 Clock Glitching

A clock glitch is an extra or missing edge injected into the target's
clock signal. On pipelined CPUs, an extra rising edge can cause the
pipeline to advance twice in one cycle, effectively skipping one
instruction. A missing edge can cause the pipeline to stall, skipping
the expected instruction at that point.

PlasmaReaper uses a fast 4:1 mux (TS3A5018) to switch the target's clock
source between:

1. The target's normal clock (passthrough).
2. A forced high (edge suppression).
3. A forced low (edge injection point).
4. The onboard 100 MHz oscillator (for clock replacement).

The FPGA controls the mux select lines with sub-nanosecond timing to
insert or remove a single half-cycle within the target's clock stream.
The glitch position is specified in units of clock cycles from the
trigger, with 184 ps fine resolution.

### 6.3 Electromagnetic Fault Injection

EMFI induces faults by injecting a high-current, fast-edge pulse into a
coil positioned near the target's die. The resulting magnetic field
induces transient currents in the die's metal layers, flipping internal
logic states. Unlike power and clock glitching, EMFI requires no
electrical contact with the target — only proximity to the die.

PlasmaReaper's EM stage uses a ZXGD3004E1 gate driver to dump the boost
capacitor (charged to 12–60 V) through the injection coil in ~10–500 ns.
The resulting peak current can reach 30 A, producing a sharp magnetic
field pulse. The coil is on a flexible fly-lead for manual positioning
over the target.

EMFI is the most variable of the three vectors — results depend heavily
on probe position, orientation, and the target's package. PlasmaReaper
supports automated pulse-voltage and pulse-width sweeps, but probe
positioning remains manual. The firmware logs the pulse parameters for
each shot so the operator can correlate outcomes with physical position
in their notes.

### 6.4 Joint Vector Injection

PlasmaReaper can fire multiple vectors in a single shot with a
configurable inter-vector delay (0 to 2^32 ns, 184 ps resolution). For
example, a power glitch can be fired 200 ns after an EM pulse, creating
a compound fault that exploits the target's momentary vulnerability
after the EM-induced transient. This is a research-grade capability that
is, to the author's knowledge, not available in any other open tool.

### 6.5 Trigger Pipeline

The trigger pipeline is the heart of PlasmaReaper's precision. All
trigger sources are routed to the FPGA, which arbitrates them and
generates the glitch enable signal with sub-10 ns latency:

- **GPIO edge**: the FPGA's input synchronizer detects the edge and
  fires the glitch in the next clock cycle (< 10 ns).
- **UART trigger word**: the MCU monitors the target UART; when the
  configured byte sequence is detected, it asserts a GPIO to the FPGA,
  which fires the glitch. Total latency < 1 µs.
- **Power envelope threshold**: an analog comparator (TLV3501, 4.5 ns
  propagation delay) monitors the target's current draw (via a sense
  resistor and current-sense amp). When the current crosses the
  threshold (indicating, e.g., the start of a crypto operation), the
  comparator output goes to the FPGA, which fires the glitch in < 50 ns.
- **Timer**: the HRTIM free-runs and fires at the programmed offset from
  the last arm. This is used for targets where the execution timing is
  deterministic (e.g., boot ROM).
- **External sync**: a second U.FL input allows an external device (e.g.,
  the `sideprobe` SCA platform) to trigger glitches synchronized to a
  side-channel event.

---

## 7. Firmware Details & Design Decisions

### 7.1 Architecture

The firmware is structured as a single-threaded super-loop with
interrupt-driven I/O, running on bare metal (no RTOS) to preserve
deterministic timing. The critical paths — HRTIM programming, ADC
sampling, and FPGA SPI — are handled in interrupt context with minimal
work in the main loop.

### 7.2 Key Modules

| Module | File | Responsibility |
|---|---|---|
| Main control loop | `main.c` | Initialization, super-loop, sweep orchestration |
| HRTIM glitch driver | `drivers/hrtim_glitch.c` | Programs HRTIM for power/clock glitch timing |
| FPGA interface | `drivers/fpga_if.c` | SPI communication with iCE40, bitstream load, trigger config |
| Power glitch driver | `drivers/power_glitch.c` | DAC setup, shunt MOSFET control, series resistance selection |
| Clock glitch driver | `drivers/clock_glitch.c` | Mux control, clock source selection, glitch shape |
| EM pulse driver | `drivers/em_pulse.c` | Boost converter control, pulse generation, thermal protection |
| Trigger driver | `drivers/trigger.c` | Trigger source selection, arming, UART word matching |
| UART monitor | `drivers/uart_monitor.c` | DMA-based UART capture, pattern matching, response classification |
| ADC waveform capture | `drivers/adc_capture.c` | ADC setup, DMA capture of VCC rail during glitch |
| USB CDC | `drivers/usb_cdc.c` | USB communication with host, command protocol |
| BLE interface | `drivers/ble_if.c` | USART communication with STM32WB55 module, AT commands |
| SD card logging | `drivers/sdcard.c` | FATFS campaign log storage |
| OLED display | `drivers/oled.c` | SSD1306 status display |
| Command protocol | `drivers/protocol.c` | Host command parsing and response formatting |

### 7.3 Glitch Parameter Model

All glitch operations are abstracted behind a common `glitch_params_t`
structure (defined in `board.h`). This allows the sweep controller to
treat all three vectors uniformly:

```c
typedef struct {
    uint8_t  vector_mask;       // POWER | CLOCK | EM bit flags
    uint32_t trigger_offset_ns; // delay from trigger to glitch start
    uint32_t glitch_width_ns;   // glitch duration
    uint16_t power_depth_mv;    // power glitch depth in mV (0 = full shunt)
    uint8_t  power_series_r;    // series resistance index (0-7)
    uint8_t  clock_shape;       // 0 = extra edge, 1 = suppress edge
    uint32_t clock_cycle_offset;// glitch position in clock cycles
    uint16_t em_pulse_mv;       // EM pulse voltage in mV
    uint16_t em_pulse_width_ns; // EM pulse width
    uint32_t inter_vector_ns;   // delay between vectors for joint injection
    uint16_t repeat_count;      // number of shots per trigger
    uint32_t repeat_delay_ns;   // delay between repeats
} glitch_params_t;
```

### 7.4 Sweep Controller

The sweep controller implements a nested-loop sweep over two parameters
(typically glitch_offset × glitch_width, or glitch_depth × glitch_width),
with an optional third parameter stepped at the outer level. For each
parameter combination, the controller:

1. Programs the glitch parameters.
2. Arms the trigger.
3. Waits for the trigger to fire (or times out).
4. Reads the ADC waveform and classifies the glitch quality.
5. Reads the UART monitor and classifies the target response.
6. Logs the result to SD card and reports to the host.
7. Advances to the next parameter combination.

The controller supports both exhaustive sweeps (every combination in a
grid) and adaptive sweeps (skip regions with low success rates based on
early results — a simple hill-climbing heuristic).

### 7.5 Design Decisions

- **Bare metal over RTOS**: an RTOS would introduce scheduler jitter
  in the glitch timing path. The super-loop with interrupt-driven I/O
  keeps the critical path deterministic.
- **STM32H723 over STM32F4**: the HRTIM peripheral with 184 ps
  resolution is the key enabler. The F4's standard timers top out at
  ~10 ns resolution, which is insufficient for precise glitch placement
  on fast targets.
- **iCE40UP5K FPGA**: the FPGA handles the trigger-to-glitch path,
  which requires sub-10 ns latency. The MCU alone cannot guarantee this
  due to interrupt latency. The iCE40 was chosen for its open toolchain
  (Project IceStorm / Yosys) and low cost.
- **STM32WB55 for BLE**: the H723 lacks integrated BLE. Using a
  separate WB55 module in AT-command mode keeps the BLE stack off the
  critical path entirely.
- **Separate analog and digital supply rails**: the glitch output stage
  draws high transient currents that would corrupt the MCU's supply if
  shared. A dedicated LM53603 buck regulator feeds the glitch stage,
  with star-grounding to the target GND.
- **No isolation on the trigger input**: the trigger inputs are 3.3 V
  CMOS, direct-coupled. This minimizes latency but means the operator
  must ensure level compatibility. A note in the README warns about
  this; a future revision may add level shifters.

---

## 8. Application/Software Interface

### 8.1 Companion App (React Native)

The companion app (`app/`) is a React Native application targeting
Android (USB CDC) and iOS (BLE). It provides:

- **Connection screen**: scan for BLE devices or connect over USB.
- **Manual glitch screen**: set all glitch parameters manually and fire
  a single shot. Shows the captured VCC waveform after the shot.
- **Sweep configurator**: define 2-D parameter sweeps (X and Y axes,
  ranges, step sizes) and launch the sweep. Shows a live heat-map of
  results as the sweep progresses.
- **Results dashboard**: view completed sweeps, with heat-map
  visualization and per-cell detail (parameters, outcome, waveform).
- **Log viewer**: browse the SD card campaign log.
- **Settings**: configure success/failure patterns, trigger source,
  target UART baud rate, and BLE pairing.

### 8.2 Python Library

A Python library (`plasmareaper.py`, documented in the README but
provided as part of the app package) wraps the USB CDC command protocol
for scripting:

```python
from plasmareaper import PlasmaReaper

pr = PlasmaReaper(port="/dev/ttyACM0")
pr.connect()

# Single shot
params = pr.make_params(
    vector="power",
    offset_ns=1000,
    width_ns=200,
    depth_mv=900,
    series_r=3,
)
result = pr.fire(params)
print(result.outcome, result.waveform)

# Sweep
for offset in range(500, 5000, 100):
    for width in range(50, 500, 10):
        params = pr.make_params(vector="power", offset_ns=offset,
                                width_ns=width, depth_mv=900, series_r=3)
        result = pr.fire(params)
        if result.outcome == "success":
            print(f"HIT at offset={offset}, width={width}")
            break
```

### 8.3 Command Protocol

The USB CDC / BLE command protocol uses a simple binary framing:

| Byte | Field |
|---|---|
| 0 | Start byte (0xA5) |
| 1 | Command code |
| 2-3 | Payload length (little-endian) |
| 4..n | Payload |
| n+1 | CRC-8 |
| n+2 | End byte (0x5A) |

Command codes include: `CMD_ARM`, `CMD_FIRE`, `CMD_SET_PARAMS`,
`CMD_GET_RESULT`, `CMD_START_SWEEP`, `CMD_STOP_SWEEP`, `CMD_GET_WAVEFORM`,
`CMD_SET_TRIGGER`, `CMD_SET_PATTERNS`, `CMD_GET_STATUS`, `CMD_LOAD_FPGA`,
`CMD_ERASE_LOG`, `CMD_GET_LOG_ENTRY`.

---

## 9. Use Cases for Red Teams, Security Researchers & Pentesters

### 9.1 Secure-Boot Bypass

**Scenario**: a target device verifies a signed boot image before
executing it. The red team wants to boot a modified image.

**Approach**: connect PlasmaReaper to the target's VCC and reset lines.
Configure a trigger on the reset deassertion edge with a sweep over
glitch offset (0 to 100 µs) and glitch width (50 to 500 ns). The UART
monitor watches for the boot prompt of the unsigned image (success) or
the expected "signature failed" message (failure). The heat-map reveals
the parameter window where the signature check is bypassed.

### 9.2 Flash Readout Protection Bypass

**Scenario**: a target MCU has readout protection (RDP) enabled,
blocking debug access to flash. The researcher wants to dump the
firmware.

**Approach**: connect to the debug interface and issue the readout
command. Trigger a power glitch during the RDP check (typically a few
cycles after the command is sent). If the glitch causes the check to
skip, the readout proceeds. Sweep glitch depth and width to find the
window.

### 9.3 DFA on Hardware Crypto Engine

**Scenario**: a target uses a hardware AES engine to decrypt traffic.
The researcher wants to recover the key via Differential Fault Analysis.

**Approach**: use the power-envelope trigger to detect the start of the
AES operation (current draw spikes). Fire an EM pulse at a swept delay
after the trigger to fault a specific round. Capture the faulty
ciphertext over UART. Repeat for many parameter combinations and feed
the results into a DFA solver to recover the key.

### 9.4 Debug Unlock on Automotive ECU

**Scenario**: an automotive ECU has a security unlock challenge-response.
The red team wants to bypass the challenge and access the debug port.

**Approach**: connect to the ECU's debug interface. Issue the unlock
command and trigger a clock glitch during the challenge verification.
The clock glitch can skip the comparison instruction, causing the
verification to pass regardless of the challenge response.

### 9.5 Combined Side-Channel + Fault Attack

**Scenario**: a target uses a masked AES implementation that resists
standard power analysis. The researcher wants to combine a fault
injection with side-channel measurement to break the masking.

**Approach**: connect the `sideprobe` SCA platform to measure the
target's power trace. Route its trigger output to PlasmaReaper's
external sync input. PlasmaReaper fires a power glitch at a precise
offset after the side-channel trigger, desynchronizing the mask and
making the power trace amenable to standard CPA.

### 9.6 Loop Counter Skip

**Scenario**: a target has a loop that decrements a counter and exits
when it reaches zero. The researcher wants to skip the last iteration
to cause an off-by-one buffer overread.

**Approach**: use the UART trigger word to detect the loop entry, then
fire a power glitch at the calculated offset to skip the decrement
instruction. The target exits the loop one iteration early.

### 9.7 EMFI Through Plastic Package

**Scenario**: a target chip is in a plastic QFP package with no
accessible VCC or clock pins. The researcher has only physical access
to the package surface.

**Approach**: position the EM probe over the die area (estimated from
the package markings). Sweep EM pulse voltage and width while monitoring
the target UART for anomalous output. Move the probe in a grid pattern,
logging results for each position. The heat-map over position × voltage
reveals the sweet spot for fault injection.

---

## 10. Bill of Materials

| Ref | Part | Qty | Note |
|---|---|---|---|
| U1 | STM32H723ZGT6 | 1 | Main MCU |
| U2 | iCE40UP5K-SG48 | 1 | Trigger/glitch FPGA |
| U3 | STM32WB55CGU6 | 1 | BLE module |
| U4 | MX25L3233F | 1 | FPGA bitstream flash |
| U5 | LM53603 | 1 | Buck regulator (glitch stage supply) |
| U6 | LM5155 | 1 | Boost converter (EM pulse) |
| U7 | ZXGD3004E1 | 1 | EM pulse gate driver |
| U8 | DAC5311 | 1 | Glitch depth DAC |
| U9 | TS3A5018 | 1 | Clock glitch mux |
| U10 | TMUX1108 | 1 | Series resistance mux |
| U11 | TLV3501 | 1 | Power envelope comparator |
| Q1 | SiS413DN | 1 | Shunt MOSFET |
| Q2 | BSC060N10NS3G | 1 | Series pass MOSFET |
| DISP1 | SSD1306 0.96" OLED | 1 | Status display |
| J1 | USB-C 16-pin | 1 | Power + USB |
| J2 | MicroSD slot | 1 | Logging |
| J3 | U.FL | 2 | Trigger input, ext sync |
| J4 | SMA | 1 | Power envelope input |
| J5 | BNC | 1 | EM probe output |
| J6 | 0.1" header (2×5) | 1 | Target connection (VCC, GND, CLK, RST, UART) |
| BAT1 | 18650 holder | 1 | Battery backup |
| L1 | 100 µH, 2 A | 1 | Buck inductor |
| L2 | 22 µH, 3 A | 1 | Boost inductor |
| C_boost | 10 µF, 100 V | 1 | EM pulse capacitor |
| Misc | R, C, diodes | ~40 | Passives |

---

## 11. Legal & Ethical Disclaimer

PlasmaReaper is designed **exclusively** for authorized security research,
penetration testing under explicit written contract, hardware security
evaluations of devices you own or have written permission to assess, and
academic fault-injection research.

**Unauthorized fault injection against devices you do not own may
violate:**

- The Computer Fraud and Abuse Act (18 U.S.C. § 1030)
- The Digital Millennium Copyright Act (DMCA) anti-circumvention
  provisions (17 U.S.C. § 1201)
- State and federal tampering statutes
- Wiretap statutes if the target processes communications
- Export control regulations (EAR Category 5 Part 2) if the device is
  exported from the United States

**Fault injection against safety-critical systems — including medical
devices, automotive ECUs, avionics, industrial controllers, and
infrastructure PLCs — can cause physical damage, malfunction, and
personal injury.** Never use PlasmaReaper on operational safety-critical
equipment.

The author (**jayis1**) assumes no liability for misuse, property damage,
financial loss, or personal injury resulting from the use of this design.
Always obtain proper written authorization before deployment, and conduct
testing only in controlled laboratory conditions.

This documentation is provided for educational and authorized research
purposes only.

---

*Author: jayis1 · Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT*
*PlasmaReaper v1.0.0 — Multi-Vector Fault Injection Toolkit*