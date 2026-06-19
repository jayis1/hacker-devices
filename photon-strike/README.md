# PhotonStrike — Portable Laser Fault Injection Platform

```
   ╔══════════════════════════════════════════════════════════════╗
   ║   ██████╗  ██████╗ ███╗   ██╗██████╗  ██████╗ ██╗   ██╗       ║
   ║   ██╔══██╗██╔═══██╗████╗  ██║██╔══██╗██╔═══██╗██║   ██║       ║
   ║   ██████╔╝██║   ██║██╔██╗ ██║██║  ██║██║   ██║██║   ██║       ║
   ║   ██╔══██╗██║   ██║██║╚██╗██║██║  ██║██║   ██║╚██╗ ██╔╝       ║
   ║   ██████╔╝╚██████╔╝██║ ╚████║██████╔╝╚██████╔╝ ╚████╔╝        ║
   ║   ╚═════╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝  ╚═════╝   ╚═══╝         ║
   ║   ████████╗██╗  ██╗███████╗██╗     ██╗                         ║
   ║   ██╔════╝██║  ██║██╔════╝██║     ██║                         ║
   ║   █████╗  ███████║██████╗ ██║     ██║                         ║
   ║   ██╔══╝  ██╔══██║██╔══╝ ██║     ██║                         ║
   ║   ██║     ██║  ██║███████╗███████╗███████╗                    ║
   ║   ╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝                    ║
   ║                                                              ║
   ║   Portable Laser Fault Injection Platform v1.0               ║
   ║   Author: jayis1                                             ║
   ╚══════════════════════════════════════════════════════════════╝
```

> **⚠️ Legal & Ethical Notice:** PhotonStrike is intended **exclusively** for authorized security research, penetration testing under written contract, hardware vulnerability assessment of devices you own, academic fault-injection research, and CTF / educational environments. Laser fault injection of semiconductors you do not own, or bypassing cryptographic protections on production systems without explicit written authorization, is illegal in most jurisdictions and may cause permanent damage to the target. The author (**jayis1**) assumes no liability for any misuse or damage. The laser subsystem emits **Class 3B / Class 4 near-infrared radiation** — direct or reflected exposure can cause **permanent eye and skin injury**. Always operate only with appropriate laser safety goggles rated for the emission wavelength, behind interlocked enclosures, and on targets you are explicitly authorized to test. This is a research design, not a finished product. **You are responsible for your own safety and legality.**

---

## Table of Contents

1. [Overview](#overview)
2. [What Makes PhotonStrike Unique](#what-makes-photonstrike-unique)
3. [Attack Surface & Threat Model](#attack-surface--threat-model)
4. [Hardware Specifications](#hardware-specifications)
5. [Architecture & Block Diagram](#architecture--block-diagram)
6. [Theory of Operation — Laser Fault Injection](#theory-of-operation--laser-fault-injection)
7. [Firmware Details & Design Decisions](#firmware-details--design-decisions)
8. [Fault Models & Analysis Pipeline](#fault-models--analysis-pipeline)
9. [Application/Software Interface](#applicationsoftware-interface)
10. [Use Cases for Red Teams & Security Researchers](#use-cases-for-red-teams--security-researchers)
11. [Building & Flashing](#building--flashing)
12. [Safety & Regulatory Notes](#safety--regulatory-notes)
13. [License & Credits](#license--credits)

---

## Overview

**PhotonStrike** is a pocket-sized, FPGA-assisted **laser fault injection (LFI)** platform
for semiconductor and embedded-system security research. Where the existing
`volt-glitcher` device disturbs a target through its power rails and
`spectre-sniffer` / `trace-reaper` *observe* side-channel leakage, PhotonStrike
goes one step further: it injects **single-bit to single-byte transient faults**
directly into the silicon of a running integrated circuit, at a precise
clock-cycle, by focusing short near-infrared laser pulses onto individual logic
cells through the backside of the die.

Laser fault injection is one of the most powerful — and least accessible —
techniques in the hardware security arsenal. A successful LFI attack can:

* **Flip a single SRAM bit or flip-flop state** at an exact instruction boundary,
  turning a `branch-if-equal` into a `branch-always` and bypassing a secure-boot
  signature check.
* **Corrupt one byte of an AES state** immediately before the final
  `MixColumns`/`ShiftRows`, enabling **Differential Fault Analysis (DFA)** to
  recover the last round key from a single correct/faulted ciphertext pair.
* **Skip a comparison instruction** in a secure element's PIN-verify routine,
  turning a wrong PIN into a correct one.
* **Break anti-rollback / monotonic counters** by zeroing the increment step.
* **Disable a fuse/efuse read** by corrupting the latch that captures the fuse
  value on readout.

Until now, LFI required a laboratory bench: a pulsed laser source, a
motorized XYZ stage, a trigger-sync box, a high-end oscilloscope, and a
host PC running a Python DFA pipeline. PhotonStrike collapses the laser
driver, the FPGA-grade trigger timing, the automated spatial scan, and
the on-device DFA key-recovery engine into one rugged, battery-operated
unit the size of a thick smartphone. An operator mounts PhotonStrike on
the backside of a decapsulated target (the die is thinned to ~100 µm and
illuminated from the substrate side at 1064 nm where silicon is
transparent), arms the trigger, and either manually probes a region of
interest or runs an automated **raster fault scan** that sweeps the laser
spot across a grid of (x, y, delay, pulse-width) combinations, capturing
correct/faulted output pairs and feeding them to the embedded DFA solver.

### Key Capabilities

| Capability | Detail |
|---|---|
| **Wavelength** | 1064 nm (backside, silicon-transparent) primary; optional 808 nm for thinned frontside |
| **Pulse energy** | ~0.5–4 nJ per pulse, software-adjustable via driver current and pulse width |
| **Pulse width** | 5 ns – 200 ns, FPGA carry-chain controlled, 250 ps resolution |
| **Spot size** | ~3 µm FWHM (backside, 0.45 NA aspheric objective through thinned Si) |
| **Trigger latency** | < 50 ns from GPIO edge to first photon; jitter < 1 ns |
| **Trigger sources** | GPIO edge, UART/SPI pattern match, power-trigger (shunt ADC threshold), manual, target clock phase |
| **Spatial scan** | Onboard MEMS mirror deflector + fine piezo Z-focus; up to 1000 × 1000 µm grid |
| **DFA engine** | On-device Piret-Quisquater AES-128 DFA solver in C; recovers last round key from ~1 faulted pair per byte |
| **Logging** | MicroSD (FAT32), CSV scan logs + binary trace capture |
| **Companion app** | BLE 5 control + live scan heatmap + DFA key readout |
| **Power** | 2 × 18650 Li-ion (2S, 7.4 V, ~3400 mAh), USB-C PD pass-through, ~6 h active scan |

---

## What Makes PhotonStrike Unique

| Feature | Bench LFI setups | PhotonStrike |
|---|---|---|
| **Portability** | Benchtop, mains powered, optics table | Pocket / handheld, battery, ~140 g |
| **Laser source** | Nd:YAG / OPO / Ti:Sapphire ($30k+) | Pulsed 1064 nm diode + driver, <$200 BOM |
| **Trigger timing** | Standalone pulse/delay generator | Integrated iCE40UP5K carry-chain, 250 ps |
| **Spatial scanning** | Motorized XYZ stage, ~10 s per point | MEMS mirror + piezo, ~2 ms per point |
| **DFA recovery** | Host PC, Python notebook | On-device C DFA solver, instant |
| **Operator UX** | Manual tuning | BLE app with live heatmap + key readout |
| **Cost target** | $50k–$250k | ~$800 BOM (excluding safety enclosure) |
| **Self-contained scan** | No | Yes — single button "scan & recover" |

PhotonStrike is **not** a replacement for a high-end Hamamatsu PLP-10 +
PicoQuant pulsed laser + Newport XPS stage. It deliberately trades
ultimate photon budget and spot size for **portability, automation, and
on-device analysis** — the things that matter when the goal is to
*break a target on-site during a red team engagement*, not to publish a
SEM-quality fault map in a journal.

---

## Attack Surface & Threat Model

### Targets PhotonStrike is designed to assess

1. **Secure elements / smartcards (EAL5+)** — AES, RSA, ECC coprocessors;
   PIN/counter protections; anti-tearing logic. Backside illumination after
   chemical decapsulation.
2. **Microcontroller secure boot** — STM32, NXP, Microchip, Renesas RL78,
   Espressif — corrupt the signature-verify branch to force "pass".
3. **TPM 2.0 & discrete crypto accelerators** — fault the DA counter or the
   HMAC bind step.
4. **Automotive ECUs** — break immobilizer challenge-response, fault the
   secure-boot signature check to load modified firmware.
5. **IoT SoCs** — fault the efuse/OTP read to misreport secure-boot fuses
   as unblown.
6. **Hardware wallets** — skip the PIN-compare step, corrupt the
   BIP-39 seed-display path.

### Fault models PhotonStrike implements

| Fault model | Mechanism | Effect |
|---|---|---|
| **Single-bit flip** | Tight focus, low energy, brief pulse | One SRAM cell / one flip-flop toggles |
| **Single-byte fault** | Slightly defocused, longer pulse | One byte of an AES state register corrupts |
| **Instruction skip** | Pulse on the PC incrementer / pipeline latch | One instruction dropped from execution |
| **Random byte** | Higher energy, wider spot | Whole byte randomizes (used for DFA on some ciphers) |
| **Stuck-at fault** | Sustained illumination during an operation | A latch holds a fixed value through the operation |

### Threat model framing (defender's perspective)

PhotonStrike is built for the **defender who must assume a physical
adversary**. A device that has not been tested against single-bit LFI cannot
claim resistance to it. PhotonStrike lets a security researcher:

* **Measure** a target's LFI sensitivity map (which die regions are
  fault-sensitive, at which delays, at which energies).
* **Verify** that countermeasures (latch hardening, parity, redundant
  computation, sensors, randomized clock) actually work under realistic
  attack parameters.
* **Demonstrate** a concrete exploit chain (e.g. "one 12 nJ pulse at
  PC+4 cycles after the verify call flips the branch outcome") that a
  product team can then reproduce and patch.

This is the same defensive rationale that justifies power-SCA tools like
`trace-reaper`: you cannot defend against an attack you cannot perform.

### Assumptions & preconditions

* The attacker has **physical access** to a decapsulated / thinned target.
  PhotonStrike does **not** decapsulate for you — chemical (acid) or
  mechanical (milling + plasma) decap is a separate prep step.
* The attacker can identify a **trigger signal** (a GPIO toggled by the
  firmware under attack, a UART marker, a power signature). PhotonStrike
  provides four trigger modalities to make this practical.
* The attacker knows or can recover the target's **clock frequency** for
  cycle-accurate delay programming (PhotonStrike can measure this).

---

## Hardware Specifications

### Core compute

* **MCU:** STMicroelectronics **STM32H757VI** (Cortex-M7 @ 480 MHz +
  Cortex-M4 @ 240 MHz, 2 MB Flash, 1 MB SRAM, BGA-100). The M7 runs the
  scan automaton, DFA solver, BLE stack and SD logging; the M4 is
  dedicated to real-time trigger arbitration and laser safety
  interlock monitoring.
* **FPGA co-processor:** Lattice **iCE40UP5K-SG48** (5.3k LUTs, 16 DSP,
  128 kB SPRAM, 8 kB BRAM). Implements the carry-chain nanosecond delay
  line, the pulse-width generator, the trigger pattern matcher, and the
  MEMS mirror scan sequencer. Configured over SPI from the STM32 at boot.

### Optical subsystem

* **Laser diode:** 1064 nm pulsed InGaAs/Quantum-well diode, ~1 W peak,
  ~30% duty cycle max, M² < 1.8, TO-220 or C-mount with TEC stabilization.
  *Example parts:* II-VI (now Coherent) P5-1064-1-C, or Hamamatsu
  PLP-10-106 (retrofittable for higher energy).
* **Laser driver:** Custom high-speed MOSFET current sink, 0.5–8 A
  programmable pulse current, < 5 ns rise/fall, opto-isolated gate from
  the FPGA. Hardware overcurrent clamp + thermal cutoff.
* **Beam path:** Diode → aspheric collimator (0.45 NA, 1064 nm AR) →
  beam expander (×3) → 2-axis MEMS mirror (Mirrorcle MEMS-2512) →
  0.45 NA long-working-distance objective → target die (backside).
* **Focusing:** Piezo-driven Z stage (60 µm travel, ~50 nm resolution)
  for focus; coarse manual XY on a kinematic mount for initial alignment.
* **Imaging (optional):** A 1280×800 monochrome CMOS sensor behind a
  dichroic beamsplitter provides backside die imaging for target
  navigation. The 1064 nm laser and the visible/near-IR image share the
  same objective path.

### Trigger & sensing

* **4 trigger channels:**
  1. **GPIO edge** — 2 × SMA, 5 V tolerant, 50 Ω, opto-isolated.
  2. **Pattern match** — UART/SPI/SWD snooped on a 3-pin header, matched
     in FPGA LUT RAM at up to 50 Msym/s.
  3. **Power trigger** — on-board 12-bit ADC sampling a shunt input at
     5 MSPS; threshold/edge/window programmed from the app.
  4. **Manual / software** — app button or physical arm button.
* **Target clock sampler:** A high-impedance probe input feeds an FPGA
  counter that measures target clock frequency to within 10 ppm; used to
  convert "N cycles after trigger" into a delay value.

### Connectivity

* **BLE 5.0:** Espressif **ESP32-C3-MINI** module on a UART bridge; the
  STM32 speaks a framed binary protocol over UART3 at 2 Mbps, the ESP32
  exposes GATT characteristics for scan control, status, and result
  streaming. Keeps the RF stack off the timing-critical MCU.
* **USB-C:** USB 2.0 Full-Speed for firmware flashing (DFU), CDC console,
  and host-side raw capture download. USB-C PD pulls up to 9 V for
  charging the 2S pack while idle.
* **MicroSD:** UHS-I slot on SDMMC1, FAT32, for scan logs and faulted-
  ciphertext capture (a long scan can produce tens of MB).

### Power

* **Battery:** 2 × 18650 Li-ion in a 2S holder, ~7.4 V, ~3400 mAh.
  Protected cells required. Internal BMS with balancing, fuel gauge
  (MAX17048) over I²C.
* **Regulation:** 5 V/3 A buck for logic, 3.3 V LDO for analog,
  programmable 0.5–8 V boost for the laser driver.
* **Endurance:** ~6 h active scanning, ~40 h standby.
* **Charge:** USB-C PD, 9 V/2 A → 4 h full charge.

### Form factor

* **PCB:** 4-layer, 80 × 45 mm, FR-4, 1.6 mm. The optical head is a
  separate 30 × 30 mm daughterboard on an FPC for pointing flexibility.
* **Enclosure:** Machined aluminum, 95 × 60 × 35 mm, anodized black,
  with a laser-interlock shutter and a key switch on the main power.
  The optical head protrudes through a sealed aperture.
* **Weight:** ~140 g (without batteries), ~230 g with cells.

---

## Architecture & Block Diagram

```
                         ┌────────────────────────────────────────────────┐
                         │                  PhotonStrike                   │
                         │                                                │
   USB-C ───────────────▶│ USB FS  ┌─────────────┐   SPI   ┌────────────┐ │
   (flash/console)       │         │ STM32H757VI │◀───────▶│ iCE40UP5K  │ │
                         │         │  M7 + M4    │         │  FPGA      │ │
   ESP32-C3 ─ UART3 ───▶│         │             │         │            │ │
   (BLE 5)               │         │  scan FSM   │         │ delay line │ │
                         │         │  DFA solver │         │ pulse gen  │ │
   MicroSD ─── SDMMC1 ──▶│         │  SD logger  │         │ pattern    │ │
                         │         │  BLE proto  │         │  matcher   │ │
   MEMS mirror ── I²C ──▶│         │  fuel gauge │         │ MEMS seq   │ │
   Piezo Z ──── DAC ────▶│         │  ADC trig   │         │ clk meas   │ │
                         │         └──────┬──────┘         └─────┬──────┘ │
                         │                │                      │        │
                         │         ┌──────▼──────┐         ┌──────▼──────┐ │
                         │         │ Laser Driver│◀────────│ Pulse Gate  │ │
                         │         │  0.5–8 A    │         │ (FPGA GPIO) │ │
                         │         └──────┬──────┘         └─────────────┘ │
                         │                │                                │
   GPIO trig (SMA) ─────▶│  ┌─────────────┴──────────────┐                  │
   Power trig (shunt) ──▶│  │   Trigger Arbiter (M4)     │                  │
   Pattern trig (header)▶│  │   → armed → wait → fire    │                  │
   Arm button ──────────▶│  └─────────────┬─────────────┘                  │
                         │                │                                │
                         │         ┌──────▼──────────────┐                 │
                         │         │  1064 nm Diode →     │                 │
                         │         │  Collimator → MEMS → │                 │
                         │         │  Objective → Target  │                 │
                         │         └─────────────────────┘                 │
                         └────────────────────────────────────────────────┘
                                                │
                                                ▼
                                  ┌─────────────────────────┐
                                  │  Decapped / thinned      │
                                  │  target die (backside)   │
                                  └─────────────────────────┘
```

### Data flow during a scan

1. **App** (over BLE) sends a scan descriptor: trigger config, (x,y) grid
   bounds, delay sweep range, pulse-width list, energy list, expected
   correct output, target clock estimate.
2. **M7** validates the descriptor against the safety envelope (max
   energy, max duty cycle, interlock state) and programs the FPGA.
3. **M4** arms the trigger arbiter and waits for the trigger event.
4. On trigger, the **FPGA** runs its carry-chain delay, then asserts the
   pulse gate for the programmed width; the **laser driver** fires.
5. The target produces a (possibly faulted) output; the operator/app
   reads it via the target's normal interface and sends it back to
   PhotonStrike (or PhotonStrike snoops it on a UART).
6. **M7** compares to the expected output, classifies the fault
   (no-change / single-bit / single-byte / random / crash), logs to SD,
   advances the MEMS mirror / delay, and repeats.
7. When a faulted ciphertext pair is captured for an AES DFA target, the
   **M7 DFA solver** runs the Piret-Quisquater recovery and streams the
   recovered last-round key byte-by-byte to the app.

---

## Theory of Operation — Laser Fault Injection

### Why backside, why 1064 nm

Silicon is opaque to visible light (bandgap 1.12 eV → absorption edge
~1100 nm). At **1064 nm** (just below the bandgap) silicon is
**transparent** to a depth of ~1 mm, so a backside-illuminated,
thinned (~100 µm) die lets photons reach the active transistor layer
with minimal absorption. Each absorbed photon can generate an
electron-hole pair in a depletion region; the resulting **photocurrent
transient** disturbs a node's voltage for a few nanoseconds, which can
flip a sensitive SRAM cell or latch. Frontside illumination (through
the metal stack) is blocked by many metal layers on modern processes,
hence backside is strongly preferred.

### The fault-injection mechanism

A focused ~3 µm spot deposits nJ-scale energy into a small volume of
silicon in nanoseconds. The resulting photocurrent:

* **Transiently pulls a node** above/below its switching threshold →
  bit flip in a 6T SRAM cell or a flip-flop.
* **Forces a combinational gate output** to a rail for the duration of
  the pulse → instruction skip if the gate is in the PC increment path.
* **Saturates a sense amplifier** → stuck-at value during a fuse read.

Because the disturbance is **transient** (no permanent damage at
controlled energy) and **localized** (single node), LFI produces the
cleanest single-bit / single-byte faults of any fault-injection
technique — far cleaner than voltage or EM glitching, which disturb
many nodes simultaneously.

### Timing precision

To hit a specific instruction or a specific AES round, the laser pulse
must arrive within a **single clock cycle** (often ~10 ns on a 100 MHz
target). PhotonStrike's FPGA carry-chain delay line synthesizes a
programmable delay with **~250 ps resolution** and < 1 ns jitter,
giving sub-cycle placement of the pulse relative to the trigger.

### DFA on AES-128 (Piret-Quisquater)

The flagship analysis built into PhotonStrike. Inject a **single-byte
fault** into the AES state *just before the last `MixColumns` of the
last round*. Given the correct ciphertext `C` and a faulted ciphertext
`C'`:

* The fault propagates through `ShiftRows` + `AddRoundKey` (no
  `MixColumns` in the last round) into exactly **4 bytes** of `C'`.
* For each candidate fault value `f ∈ {1..255}` and each of the 4
  affected columns, the algorithm solves for the corresponding 4
  last-round-key bytes that make the fault consistent.
* With one good faulted pair, the key space for those 4 bytes shrinks
  to ~2⁸ candidates; a second faulted pair in a different column
  recovers the next 4 bytes; four pairs total recover all 16 key
  bytes. PhotonStrike implements this in C and runs it in
  milliseconds.

This is the same attack that has broken AES in countless smartcards
and TPMs; PhotonStrike makes it field-deployable.

---

## Firmware Details & Design Decisions

The firmware (`firmware/`) is bare-metal C for the STM32H757, split
across the M7 (application) and M4 (real-time trigger + safety)
domains. No external RTOS is required for the scan automaton; a
cooperative main loop plus interrupt-driven DMA is sufficient and
keeps latency predictable.

### Files

| File | Role |
|---|---|
| `main.c` | M7 application: scan FSM, DFA solver, BLE protocol, SD logging, USB CDC |
| `m4_core.c` | M4 firmware: trigger arbiter, safety interlock monitor |
| `board.h` | Pin map, peripheral assignments, board revision constants |
| `registers.h` | STM32H7 register definitions used outside the HAL |
| `drivers/gpio.c` | GPIO config abstraction |
| `drivers/laser.c` | Laser driver: current set, pulse enable, safety, TEC control |
| `drivers/trigger.c` | Trigger arbiter: GPIO/pattern/power/manual, arm/fire/disarm |
| `drivers/fpga.c` | SPI config of the iCE40: delay, pulse width, pattern, MEMS seq |
| `drivers/mems.c` | MEMS mirror + piezo Z scan sequencer over I²C/DAC |
| `drivers/dfa.c` | Piret-Quisquater AES-128 DFA solver |
| `drivers/ble_uart.c` | Framed binary protocol to the ESP32-C3 BLE bridge |
| `drivers/sdmmc.c` | FAT32 SD logging |
| `drivers/usb_dev.c` | USB CDC console + DFU |
| `drivers/timing.c` | Cycle counters, delay helpers |
| `Makefile` | ARM GCC build for both cores |
| `stm32h757_flash.ld` | Linker script (M7 image, M4 image appended) |

### Key design decisions

1. **Two cores, strict separation.** The M4 runs only the trigger
   arbiter and the hardware safety interlock (laser shutter, overtemp,
   overcurrent, key-switch state). It cannot be blocked by M7
   activity (BLE, SD, DFA), so the laser can **always** be shut off
   within microseconds of a fault. The M7 is allowed to do "slow" work
   (logging, DFA, BLE) because it never gates the laser.
2. **FPGA owns the picosecond timing.** The STM32's peripheral timers
   top out at ~2 ns resolution at 480 MHz; that's not enough for
   sub-cycle placement on fast targets. The iCE40 carry-chain delay
   line gives ~250 ps, and the pulse-width generator is also on the
   FPGA so the M7 never touches the hot path.
3. **Safety is hardware, not firmware.** The laser driver has a
   hard-wired overcurrent clamp, a thermal cutoff thermistor, and a
   mechanical shutter that is held open only by an M4 GPIO that the
   M7 cannot assert. The key switch gates the laser driver's power
   rail directly. No firmware bug can override these.
4. **DFA in C, not Python.** Field operation means no host PC. The
   Piret-Quisquater solver is implemented in fixed C with no dynamic
   allocation, so it runs in milliseconds on the M7 and can be
   invoked per-faulted-pair as scans proceed.
5. **Scan descriptor is a compact binary struct.** Sent over BLE in a
   single GATT write; the app builds it from the UI. Keeps BLE
   throughput irrelevant to scan speed.
6. **Deterministic logging.** Every shot is logged with (x, y, delay,
   width, energy, trigger source, fault classification, faulted-output
   hash) to a CSV on SD, so a scan is fully reproducible and
   post-mortem analyzable offline.

---

## Fault Models & Analysis Pipeline

### On-device fault classifier

After each shot, the operator/app supplies the target's output (or
PhotonStrike snoops it). PhotonStrike classifies the outcome:

| Class | Criterion | Action |
|---|---|---|
| `NO_CHANGE` | output == expected | advance |
| `SINGLE_BIT` | Hamming distance == 1 | log, optionally feed DFA |
| `SINGLE_BYTE` | exactly one byte differs, rest identical | **prime DFA candidate** |
| `MULTI_BYTE` | 2+ bytes differ | log (may still be DFA-usable) |
| `CRASH` | no output / timeout | log, optionally reset target |
| `RESET` | target reset line asserted | log, re-arm |

### DFA pipeline (AES-128, Piret-Quisquater)

1. Operator provides the **expected (correct) ciphertext** for a known
   plaintext under the unknown key.
2. PhotonStrike runs a spatial+delay scan, classifying each shot.
3. Each `SINGLE_BYTE` (or clean `MULTI_BYTE` in a known column) is
   queued for the DFA solver.
4. The solver computes, for each affected column, the candidate
   last-round-key bytes; results accumulate as more faulted pairs
   arrive.
5. Once all 16 key bytes are uniquely determined, PhotonStrike emits
   the recovered key over BLE and logs it to SD.

### Beyond AES

The firmware's DFA module is structured so that additional solvers
(byte-fault DFA on AES-256 last two rounds, byte-fault DFA on
PRESENT, fault-skip DFA on RSA-CRT) can be added behind the same
classifier interface. AES-128 is the v1 reference implementation.

---

## Application/Software Interface

The companion app (`app/`) is a React Native (TypeScript) application
targeting iOS and Android, communicating with PhotonStrike over BLE
5 using `react-native-ble-plx`. Screens:

| Screen | Purpose |
|---|---|
| `HomeScreen` | Connection status, battery, laser safety state, quick arm/disarm |
| `ScanSetupScreen` | Build the scan descriptor: grid bounds, delay sweep, pulse widths, energy, trigger source |
| `TriggerScreen` | Configure each trigger modality (GPIO edge, pattern, power threshold, manual) |
| `ScanProgressScreen` | Live heatmap of (x,y) → fault class, current shot stats, ETA |
| `DFAScreen` | Expected ciphertext entry, live DFA candidate keys, recovered key display |
| `LogScreen` | Browse SD scan logs, export CSV over share sheet |
| `SafetyScreen` | Interlock status, laser enable, key-switch state, fault history, emergency stop |

The BLE service exposes these GATT characteristics:

* `PS_CHARACTERISTIC_SCAN` (write) — submit a scan descriptor.
* `PS_CHARACTERISTIC_STATUS` (notify) — live scan progress + fault events.
* `PS_CHARACTERISTIC_DFA` (notify) — DFA candidate / recovered key stream.
* `PS_CHARACTERISTIC_CONTROL` (write) — arm, disarm, abort, laser enable.
* `PS_CHARACTERISTIC_LOG` (read) — pull a named log file in chunks.

The app enforces a **two-person rule** on the `SafetyScreen`: enabling
the laser requires a long-press confirmation and shows a live
interlock checklist.

---

## Use Cases for Red Teams & Security Researchers

### 1. Secure-boot bypass on an STM32H7 target

The target verifies an RSA signature on external flash before jumping
to it. PhotonStrike, triggered on the GPIO that marks "verify
returned", scans a delay window around the `bne` instruction that
branches on the verify result. A single-bit fault on the branch's
condition latch turns the not-equal into equal; the target jumps to
the unsigned payload. Total on-device time: minutes once the trigger
is wired.

### 2. AES-128 key recovery from a secure element

A smartcard performs AES-128 encryption on command. PhotonStrike
triggers on the command-APDU edge, scans delays around the 9th round,
and captures correct + faulted ciphertext pairs. The on-device DFA
solver recovers the last round key; the app displays it. With the
last round key, the full AES key is derived by inverse key schedule.

### 3. Immobilizer challenge-response fault

An automotive immobilizer compares a challenge response with a
constant-time compare. PhotonStrike skips one iteration of the
compare loop, forcing "match". The ECU authorizes ignition.

### 4. Efuse secure-boot readout fault

A SoC reads its secure-boot efuses on every boot. PhotonStrike faults
the latch that captures the "secure boot enabled" fuse bit, causing
the SoC to behave as if secure boot is disabled and load unsigned
bootloader code. Used to demonstrate why fuse reads must be hardened.

### 5. Hardware-wallet PIN skip

A hardware wallet gates BIP-39 seed access on a PIN compare.
PhotonStrike faults the compare's result register, bypassing the
PIN. Demonstrates the need for wallet-side fault countermeasures
(redundant compare, randomized control flow).

### 6. Defensive: countermeasure regression testing

A vendor hardens their AES coprocessor with parity + dual-rail logic
and random delay insertion. PhotonStrike's spatial scan produces a
fault sensitivity map; the defender compares the pre- and post-hardening
maps to **verify** the countermeasures reduced fault success rate,
instead of merely assuming they did.

---

## Building & Flashing

### Firmware

```bash
cd photon-strike/firmware
make              # builds M7 + M4 images, combined .hex
make flash        # via USB DFU (STM32CubeProgrammer) or ST-Link
```

The Makefile targets `arm-none-eabi-gcc`. The M4 image is linked at
`0x080E0000` and loaded by the M7 boot stub into the M4's vector base
before releasing the M4 from reset.

### FPGA bitstream

The iCE40 bitstream (`firmware/fpga/photonstrike.bit`) is generated
from the open-source `yosys` + `nextpnr-ice40` flow (`make fpga`).
The STM32 loads it over SPI at boot; no external flash is required.

### Companion app

```bash
cd photon-strike/app
npm install
npx react-native run-android   # or run-ios
```

---

## Safety & Regulatory Notes

* **Laser class:** With the nominal 1064 nm diode at full energy and
  the objective in place, the accessible emission exceeds **Class 3B**
  and approaches **Class 4** limits. PhotonStrike **must** be operated
  inside an interlocked enclosure with 1064 nm-rated goggles (OD ≥ 6
  at 1064 nm). The firmware enforces an interlock input; if the
  interlock opens, the M4 cuts the laser within microseconds and
  latches a fault.
* **Target damage:** Excess energy can permanently damage a die.
  PhotonStrike defaults to a conservative energy ceiling and
  increments slowly; the operator can raise the ceiling only from the
  `SafetyScreen` with confirmation.
* **Regulatory:** Sale/use of Class 3B/4 lasers is regulated in most
  jurisdictions (FDA 21 CFR 1040 in the US, EN 60825-1 in the EU).
  This is a research design, **not** a certified product. Builders
  are responsible for compliance.
* **No guidance is given here for decapsulation.** Decapping a chip
  is a separate skill (acid / milling / plasma) with its own safety
  considerations; PhotonStrike assumes a prepared target.

---

## License & Credits

* **Hardware (KiCad):** CERN-OHL-S v2
* **Firmware (C):** GPL-2.0
* **Companion app (TypeScript):** MIT
* **FPGA bitstream source:** Apache-2.0

**Author:** jayis1
**Repository:** part of the `hacker-devices` collection.
**Version:** 1.0.0

PhotonStrike builds on decades of public laser fault injection
research (Rosseb, Piret-Quisquater, Selimhanow, Dehdashtian, etc.)
and packages it into a portable, automated, on-device-analysis
platform. The author thanks the open hardware security community for
the prior art that made this design possible.