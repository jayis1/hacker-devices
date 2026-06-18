# TRACE-REAPER — Portable Power-Side-Channel Analyzer & AES Key Recovery Appliance

> **Author:** jayis1
> **License:** GPL-2.0
> **Status:** Research hardware design — firmware + PCB + companion app
> **Disclaimer:** For authorized security research and penetration testing only. Use exclusively on hardware you own or have explicit written authorization to assess. Side-channel analysis of devices you do not own may be illegal in your jurisdiction. The author and contributors accept no liability for misuse.

---

## 1. Overview

**TRACE-REAPER** is a battery-powered, pocket-sized appliance that performs
**power-side-channel analysis (SCA)** of cryptographic hardware in the field.
Most power-analysis workflows demand a bench full of equipment — a
high-speed oscilloscope, a shunt/amplifier front-end, a host PC running
Python notebooks, and hours of capture. TRACE-REAPER collapses all of that
into one self-contained unit the size of a chunky USB stick.

It samples the instantaneous power consumption of a target chip (smartcard,
microcontroller, secure element, SoC) through either a **shunt resistor**
in the target's VCC/GND rail or a **near-field EM probe** placed over the
die, acquires millions of traces at up to **250 MSPS**, and runs
**Correlation Power Analysis (CPA)** *on the device itself*, accelerated by
an onboard Lattice / Xilinx Spartan-7 class FPGA. Recovered key bytes are
surfaced live to a companion mobile app over BLE, together with the live
power trace, the per-byte correlation waterfall, and the ranked candidate
tables.

Typical recovery time for a full 16-byte AES-128 key against an
unprotected software implementation is on the order of **a few thousand
traces and under a minute of correlation** — entirely offline, with no
laptop in the loop.

### What makes it novel

* **On-device CPA.** No host PC required. The FPGA does the
  sample×hypothesis dot-products in hard real time; the MCU orchestrates and
  exposes results over BLE. This is the form-factor innovation: previous
  portable SCA tools (ChipWhisperer Lite, etc.) still stream raw samples to
  a laptop. TRACE-REAPER closes the loop on the appliance.
* **Dual acquisition front-end.** Active-shunt (current) **and** passive
  near-field EM, switchable from the app, sharing the same analog chain.
* **Trigger-flexible.** External trigger input, analog threshold trigger on
  the power trace itself, and a "triggerless" rolling-capture mode that
  auto-aligns traces by cross-correlation against a reference template.
* **Form factor.** 82 × 24 × 11 mm aluminum-cased wand, USB-C charging/data,
  internal 600 mAh LiPo giving ~4 h of continuous acquisition.
* **Honest signal integrity.** 14-bit ENOB at 250 MSPS, AC-coupled
  differential front-end, onboard references, proper analog grounding — not
  a toy ADC stapled to an MCU.

---

## 2. Attack Surface & Threat Model

### Targeted class

TRACE-REAPER attacks the **leakage of secret-dependent intermediate values**
inside symmetric cryptographic cores. The canonical victim is AES-128 in
ECB/CBC running in software on a microcontroller whose power rail is
observable, but the same workflow extends to:

* AES-256 (more hypotheses, longer run, same principles)
* PRESENT / lightweight ciphers (good for IoT nodes)
* DES / 3DES on legacy smartcards
* RSA square-and-multiply timing/power leakage (single-trace and
  multi-trace variants)
* ECC scalar multiplication leakage (a harder, research-grade target)

It does **not** attack:

* Hardware with first-order masking/hidden-variable countermeasures
  (those need higher-order CPA or template attacks — out of scope for v1
  firmware, though the capture pipeline can still record the raw traces for
  offline higher-order work).
* Devices whose power consumption is fully decorrelated from data by active
  noise injection.

### Attack surface of the device itself

Because TRACE-REAPER physically contacts and injects nothing but a shunt
into the *target's* power rail (passive sense), the primary attacker-facing
risks are:

1. **BLE link security.** Results are pushed over BLE; the link **must** be
   pairing-encrypted (LE Secure Connections, passkey). Firmware refuses to
   transmit recovered-key material over an unauthenticated link. (See
   `ble_bridge`.)
2. **Captured-trace exfiltration.** Raw traces may contain re-identifiable
   leakage of the target's secrets; they are stored in a per-session
   encrypted flash partition keyed to the operator's passkey.
3. **Physical tamper.** A tamper-lacing mesh and an accelerometer-based
   tamper detect zeroize keys and trace buffers if the case is opened while
   armed.
4. **USB attack surface.** The USB-C interface enumerates as a
   vendor-specific class **and** a read-only mass-storage capture dump
   mode; mass-storage is only enabled when the operator explicitly toggles
   it from the app, to avoid a "plug in, get owned" vector.

### Trust boundaries

```
[ Target chip ] --(shunt / EM probe)--> [ TRACE-REAPER analog front-end ]
                                              |
                                              v
                          [ ADC -> FPGA correlation engine ]
                                              |
                                              v
                          [ STM32H7 orchestrator + BLE ]
                                              |
                                              v
                          [ Operator's phone (companion app) ]
```

Nothing leaves the device until the operator explicitly arms a session,
authenticates over BLE, and starts acquisition. The phone is treated as
semi-trusted (results are delivered, raw traces are not, unless the
operator requests a dump over the authenticated link).

---

## 3. Hardware Specifications

| Block | Part (reference) | Notes |
|---|---|---|
| MCU / orchestrator | STM32H743VI | 480 MHz Cortex-M7, FPU, 2 MB flash, 1 MB SRAM, HSEM for FPGA IPC |
| Correlation FPGA | Lattice iCE40UP5K or Xilinx Spartan-7 (XC7S6) | iCE40 chosen for open toolchain (Yosys/nextpnr); Spartan-7 if more DSP slices needed |
| ADC | TI ADC08B250 / AD9236 (12-bit, 250 MSPS) or LTC2292 (dual 14-bit 80 MSPS) | Reference design targets 250 MSPS, 12-bit; configurable down to 20 MSPS for low-power targets |
| Analog front-end | AD8138 differential ADC driver + LTC6409 low-noise amp + ADG601 trigger mux | AC-coupled, 50 Ω input, ±1 V range, switchable gain 0/14/28 dB |
| EM probe input | Mini coax SMA + onboard near-field probe tip (H-field loop) | 1 mm / 5 mm interchangeable tips |
| Trigger | External BNC + analog-level + template-match | Programmable hysteresis, 10 ns jitter |
| BLE | Cypress/Infineon CYW20819 or Nordic nRF52840 as companion module | LE Secure Connections, passkey pairing |
| Storage | 8 MB external QSPI NOR (W25Q64) + microSD socket | Trace ring buffer on QSPI, full captures to SD |
| USB-C | USB 2.0 FS + PD sink-only | Charging and vendor-class control |
| Power | 600 mAh LiPo, TI BQ25895 charger, TPS62177 rails | 4 h continuous, hot-charge over USB-C |
| Tamper | Accelerometer (LIS2DH12) + case-seam conductive mesh | Triggers key zeroization |
| User UI | 3 tactile buttons + 5 RGB status LEDs + mono OLED 0.96" | Minimal local control; full UI in app |
| Form factor | 82 × 24 × 11 mm milled aluminum wand, anodized | SMA on one end, probe-tip latching on the other |

### Power budget

Active acquisition at 250 MSPS pulls ~600 mA peak from the 3.7 V cell —
the ADC and the FPGA are the hungry parts. The firmware supports a
"low-power" 40 MSPS mode that drops consumption to ~220 mA and extends
battery to ~9 h, suitable for slow software AES on 8-bit targets.

### Signal integrity

* Differential analog path, single ground point at the shunt, star ground
  to the aluminum case (which acts as a Faraday shield once the operator
  holds it).
* ADC clock from a dedicated low-jitter XO (LMK1C1102 fanout), not the MCU
  PLL — this matters for repeatable trace alignment.
* FPGA correlation engine clock-locked to the ADC sample clock so the
  sample stream and the hypothesis multiplication are coherent.

---

## 4. Architecture

```
                     +-------------------------------------------+
                     |              STM32H743 (MCU)              |
                     |  orchestrator · BLE stack · CPA host glue  |
                     |  trace manager · tamper · storage · UI    |
                     +-----+----------------+--------------------+
                           | SPI (control)  | HSEM/IRQ (events)
                           v                v
        +------------------+                +-------------------+
        |   iCE40UP5K FPGA |                |  CYW20819 BLE     |
        |  - sample DMA    |               |  LE Secure Conn   |
        |  - mean/variance |               |  GATT server      |
        |  - hypothesis    |               +-------------------+
        |    dot-products  |
        |  - candidate sort |
        +---------+--------+
                  | LVDS/parallel sample bus
                  v
        +-------------------+        +---------------------+
        |  ADC 250 MSPS      | <---- | analog front-end    |
        |  12-bit, 256M FIFO |        | AD8138/ADG601/LTC   |
        +---------+----------+        +----------+----------+
                  |                              |
                  +------------------------------+  <-- external trigger
                                                 |      shunt / EM probe
```

### Block roles

* **Analog front-end:** selects shunt vs EM input, applies gain, AC-couples
  to the ADC differential input. Provides an analog-comparator trigger with
  programmable threshold and hysteresis.
* **ADC:** streams 12-bit samples at 250 MSPS into the FPGA sample FIFO.
  The FPGA downsamples / decimates as configured.
* **FPGA correlation engine:** for each trace it (a) extracts the aligned
  window around the trigger, (b) computes per-sample mean and variance,
  (c) for each of 256 hypothesis values × 16 key bytes, accumulates the
  Pearson correlation against the HW/HD model across all captured traces.
  This is the bulk of the compute and the reason it's in the FPGA.
* **MCU orchestrator:** runs the RTOS, drives the BLE GATT server, manages
  storage, exposes the command protocol to the app, and aggregates the
  FPGA's correlation tables into "best guess per byte" + confidence.
* **BLE module:** carries the command protocol and pushes live trace
  thumbnails and correlation waterfalls at ~5 Hz.
* **Storage:** trace ring buffer in QSPI NOR (fast, finite); full captures
  optionally flushed to microSD; per-session encrypted.

### Data flow for one acquisition run

1. Operator configures a "session" in the app: target cipher (AES-128),
   number of traces (e.g. 5,000), sample window (e.g. 2,000 samples around
   trigger), trigger source, leakage model (HW of S-box output), known
   plaintext pattern (fixed vs random).
2. App pushes config to the MCU over authenticated BLE.
3. MCU arms the analog front-end, waits for triggers, and on each trigger
   the FPGA captures one window into the trace buffer and folds it into the
   running correlation accumulators.
4. Every ~200 ms the MCU snapshots the FPGA's correlation tables, picks the
   top candidate per byte, and pushes a BLE notification with the
   per-byte best-guess + correlation coefficient.
5. When the requested number of traces is reached (or the operator stops),
   the MCU finalizes, computes a global confidence, marks the key as
   "recovered" if all bytes exceed a configurable threshold (e.g. ρ > 0.5),
   and writes the session result to encrypted storage.

---

## 5. Firmware

The firmware is an STM32H7 bare-metal C program (no external RTOS dependency
in v1; a cooperative scheduler built on SysTick). It is organized as:

```
firmware/
  main.c              Scheduler, command dispatch, top-level state machine
  registers.h         MCU register map constants
  board.h             Pin/peripheral assignments, constants, enums
  Makefile             arm-none-eabi-gcc build
  drivers/
    adc_frontend.c/.h   Analog front-end + trigger configuration
    fpga_corr.c/.h      FPGA correlation engine control + IPC
    trace_buf.c/.h      QSPI-backed trace ring buffer
    ble_bridge.c/.h     BLE GATT server + link security enforcement
    crypto_model.c/.h   HW/HD leakage models, S-box tables
    storage.c/.h        Encrypted session storage on QSPI + SD
    tamper.c/.h         Accelerometer + case-mesh tamper, zeroize
    ui.c/.h             Buttons, LEDs, OLED local UI
```

### Key design decisions

* **FPGA-first processing.** The MCU never sees raw samples during a CPA
  run — only the correlation accumulators and the final tables. This keeps
  the MCU free to service BLE and storage without dropping traces, and it
  is what makes on-device CPA feasible at 250 MSPS.
* **Pearson correlation via running sums.** The FPGA maintains, per
  (key-byte, hypothesis), the running sum of samples, sum of samples², sum
  of hypotheses, sum of hypotheses², and sum of products. From these five
  integers the MCU computes ρ at snapshot time in O(1) per cell. This avoids
  storing all 5,000 × 2,000 samples.
* **Trigger alignment by cross-correlation.** In triggerless mode the FPGA
  keeps the last 64 samples of the previous trace as a reference template and
  aligns the incoming trace by maximizing the cross-correlation lag. This
  rescues runs where an external trigger is unavailable.
* **Leakage models.** `crypto_model.c` provides Hamming-Weight and
  Hamming-Distance models for the AES S-box output, S-box input, MixColumns
  output, and DES S-box output. The model is selected per session.
* **Security-first BLE.** The GATT server refuses to expose the
  `recovered_key` characteristic unless the link is encrypted and bonded
  via LE Secure Connections with a 6-digit passkey entered on the device's
  OLED (not the phone) — defending against active MITM.
* **Tamper zeroization.** On tamper event the firmware wipes the QSPI
  trace buffer, the SD capture, and the in-RAM correlation tables, then
  halts. The operator must re-pair.

### Build

```
cd firmware
make            # builds trace-reaper.elf using arm-none-eabi-gcc
make flash      # OpenOCD flash to STM32H743 via ST-LINK
```

The Makefile targets `arm-none-eabi-gcc` with `-mcpu=cortex-m7 -mfpu=fpv5-d16
-mfloat-abi=hard`. The FPGA bitstream is built separately with
`Yosys/nextpnr-ice40` (out of scope for this repo's source set, but the
correlation-engine register interface is documented in `fpga_corr.h`).

---

## 6. Companion App

A React Native app (`app/`) provides the operator UI:

* **Dashboard** — device status, battery, current session, quick start.
* **Config** — set target cipher, leakage model, trace count, window,
  trigger source, gain.
* **LiveTrace** — scrolling live power trace + per-byte correlation
  waterfall + best-guess key display, updating at ~5 Hz over BLE.
* **Results** — final recovered key (hex), confidence per byte, export,
  optional raw-trace dump request.
* **Storage** — list of past sessions, decrypt-on-open with operator
  passkey.

The app talks to the device over a small binary GATT protocol
(`utils/protocol.js`) framing commands as `{ cmd, payload }` and receiving
notifications as framed responses.

---

## 7. Use Cases

### Red team / pentest

* **Smartcard & HSM assessment.** Demonstrate that a "secure" payment or
  access-control smartcard leaks its AES key under CPA within minutes,
  quantifying the risk and the need for DPA-resistant middleware.
* **IoT firmware key extraction.** Recover the AES key from a low-end
  IoT MCU that performs software crypto, then use it to decrypt captured
  over-the-air traffic offline.
* **Supply-chain verification.** Spot-check that a vendor's "DPA-hardened"
  claim actually holds — if TRACE-REAPER recovers the key in 5,000 traces,
  the countermeasure is not effective.

### Security researchers

* **Countermeasure evaluation.** Capture traces against a masking
  prototype and re-run higher-order CPA offline (raw traces exportable).
* **EM vs current leakage comparison.** Switch input from shunt to EM probe
  mid-session to compare leakage strength per probe position.
* **New-cipher profiling.** Add a new leakage model in `crypto_model.c`,
  point the session at it, and characterize an unpublished cipher's leakage.

### Educators

* **Live classroom demos.** Recover an AES key in front of a class in under
  a minute with no oscilloscope on the bench — a visceral illustration of
  why side-channel hardening matters.

---

## 8. Legal & Ethical Disclaimer

TRACE-REAPER is provided for **authorized security research, penetration
testing, and education only**. Operating it against hardware you do not own
or have not been explicitly authorized to test may violate computer-misuse,
wiretap, or smartcard-specific legislation in your jurisdiction. Side-channel
analysis is, in many legal systems, treated analogously to unauthorized
access even though it is passive.

By using this design or any derivative of it you represent that you have
written authorization for the target, that you will handle any recovered
secret material according to your authorization's scope and retention
policy, and that you will not export or retransmit recovered keys outside
that scope. The author (**jayis1**) and contributors disclaim all liability
for misuse.

When in doubt: **don't**.