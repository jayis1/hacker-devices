# ACOUSTIC-PHANTOM — Portable Acoustic Side-Channel Emanation Capture & Analysis Appliance

```
   ╔══════════════════════════════════════════════════════════════════╗
   ║                                                                  ║
   ║    █████╗  ██████╗ ███████╗███████╗██╗   ██╗██████╗ ██╗██████╗   ║
   ║   ██╔══██╗██╔════██╗██╔════╝██╔════╝██║   ██║██╔══██╗██║██╔══██╗  ║
   ║   ███████║██║   ██║█████╗  █████╗  ██║   ██║██████╔╝██║██║  ██║  ║
   ║   ██╔══██║██║   ██║██╔══╝  ██╔══╝  ██║   ██║██╔══██╗██║██║  ██║  ║
   ║   ██║  ██║╚██████╔╝██║     ██║     ╚██████╔╝██║  ██║██║██████╔╝  ║
   ║   ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝      ╚═════╝ ╚═╝  ╚═╝╚═╝╚═════╝   ║
   ║           Portable Acoustic Side-Channel Analysis Platform       ║
   ║           Author: jayis1                                         ║
   ╚══════════════════════════════════════════════════════════════════╝
```

> **Author:** jayis1
> **License:** Hardware — CERN-OHL-S v2, Firmware — GPL-2.0, App — MIT
> **Status:** Research hardware design — firmware + PCB + companion app
> **Version:** 1.0.0

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** ACOUSTIC-PHANTOM is designed **exclusively** for authorized security research, penetration testing under explicit written contract, and academic side-channel analysis of devices you own or have written permission to assess. Acoustic eavesdropping of devices, persons, or premises you do not own or have authorization to test may violate wiretap statutes (e.g., 18 U.S.C. § 2511), computer fraud and abuse laws (18 U.S.C. § 1030), and privacy regulations in your jurisdiction. Unauthorized interception of acoustic emanations from equipment you do not own is illegal in most jurisdictions. The author (**jayis1**) assumes no liability for any misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## Table of Contents

1. [Overview](#1-overview)
2. [What Makes ACOUSTIC-PHANTOM Novel](#2-what-makes-acoustic-phantom-novel)
3. [Attack Surface & Threat Model](#3-attack-surface--threat-model)
4. [Hardware Specifications](#4-hardware-specifications)
5. [Architecture & Block Diagram](#5-architecture--block-diagram)
6. [Theory of Operation — Acoustic Side Channels](#6-theory-of-operation--acoustic-side-channels)
7. [Firmware Details & Design Decisions](#7-firmware-details--design-decisions)
8. [Application/Software Interface](#8-applicationsoftware-interface)
9. [Use Cases for Red Teams, Security Researchers & Pentesters](#9-use-cases-for-red-teams-security-researchers--pentesters)
10. [Legal & Ethical Disclaimer](#10-legal--ethical-disclaimer)

---

## 1. Overview

**ACOUSTIC-PHANTOM** is a battery-powered, pocket-sized appliance that
performs **acoustic side-channel emanation analysis** of target hardware in
the field. Every electronic device with moving parts or varying electrical
loads emits sound — keystrokes on a mechanical keyboard, the click of a
relay, the whine of a switching-mode power supply under varying CPU load,
the seek chirps of a spinning hard drive, the dot-impact sounds of a
printer, the buzzing of capacitors and inductors during cryptographic
operations. These acoustic emanations carry information about the internal
state and activity of the device, and with the right capture and analysis
pipeline, they can be turned into a side-channel that leaks secrets.

ACOUSTIC-PHANTOM collapses the entire acoustic side-channel research
workflow — traditionally requiring a laboratory measurement microphone, a
portable recorder, and a host PC running Python with scipy/tensorflow —
into one self-contained unit roughly the size of a thick pen. It captures
audio through a **4-element MEMS microphone array** for far-field
beamforming and source localization, plus a **contact piezoelectric
transducer** for direct surface acoustic coupling to the target chassis.
The captured audio is processed on-device by an **STM32H7** running a
real-time DSP pipeline and a **TensorFlow Lite for Microcontrollers**
neural network inference engine that classifies acoustic events and
attempts recovery of typed keystrokes, file access patterns, and
cryptographic operation signatures.

Results — classified keystroke sequences, detected event timelines,
spectrograms, and confidence scores — are streamed live to a companion
mobile app over encrypted BLE, together with the raw audio waveform for
operator review.

### Typical attack scenarios

| Scenario | Emanation Source | What Leaks |
|----------|------------------|------------|
| **Keyboard acoustic eavesdropping** | Mechanical/IBM-style key click | Typed text, passwords, commands |
| **HDD seek acoustic analysis** | Spinning disk head actuator | File access patterns, accessed filenames |
| **Printer forensics** | Dot-matrix / thermal print head | Printed content (text reconstruction) |
| **SMPS coil whine cryptanalysis** | Inductor/transformer vibration | CPU load states, crypto operation timing |
| **Relay click analysis** | Electromechanical relays | State machine transitions, access control logic |
| **Capacitor singing** | Ceramic capacitor piezo effect | Voltage regulator load transients |

### Key specifications at a glance

| Parameter | Value |
|-----------|-------|
| MCU | STM32H743VIT6 (Cortex-M7 @ 480 MHz, 2 MB flash, 1 MB SRAM) |
| Audio DSP | On-chip (Cortex-M7 DSP extensions + CMSIS-DSP) |
| Microphone array | 4× SPH0641LU4H-1 MEMS (I²S PDM, 48 kHz / 16-bit) |
| Contact sensor | Piezoelectric disc + charge amplifier (10 Hz–20 kHz) |
| ML inference | TensorFlow Lite for Microcontrollers (int8 quantized) |
| Wireless | BLE 5.0 (CYW20819 module), encrypted GATT profile |
| Storage | W25Q128 128 Mbit QSPI NOR + microSD (FAT32) |
| Display | 1.3" SSD1306 OLED (128×64, I²C) |
| Battery | 800 mAh LiPo, ~6 h continuous capture |
| Charging | USB-C (BQ25895, 3A fast charge) |
| Form factor | 120 × 22 × 14 mm aluminum pen-style enclosure |
| Weight | ~42 g (without contact probe) |

---

## 2. What Makes ACOUSTIC-PHANTOM Novel

The acoustic side-channel has been studied in academia for over 15 years —
from the landmark keyboard acoustic eavesdropping work (Asonov & Agrawal,
2004) to HDD acoustic emanation analysis (Guri et al., 2016) and printer
forensics. Yet every published attack workflow demands a lab-grade
measurement microphone, a recording device, and a powerful PC for the
machine-learning analysis. No portable, self-contained, field-deployable
hardware appliance has existed — until now.

ACOUSTIC-PHANTOM's innovations:

1. **On-device ML inference.** The STM32H7 runs TensorFlow Lite for
   Microcontrollers with quantized int8 models for keystroke
   classification, event detection, and anomaly scoring. No laptop
   required — the neural network runs on the appliance itself, in hard
   real time, classifying each acoustic event as it arrives.

2. **Dual acquisition front-end.** Far-field **MEMS microphone array**
   with beamforming for directional pickup at distances up to ~2 m, plus
   a **contact piezoelectric transducer** that couples directly to the
   target's chassis for high-SNR capture of internal vibrations
   (inductor whine, relay clicks, seek sounds) that are barely audible
   in air.

3. **Beamforming source localization.** The 4-mic array uses delay-and-sum
   beamforming to steer a directional pickup pattern toward the target,
   rejecting ambient noise. The operator sees the estimated source
   bearing on the app, enabling precise aiming during covert
   capture.

4. **Protocol-aware event timeline.** Rather than dumping raw audio,
   ACOUSTIC-PHANTOM decodes acoustic events into a structured timeline:
   `{timestamp, event_type, confidence, features}`. The operator sees
   "KEYSTROKE @ 12.34s → 'A' (0.87 conf)", "RELAY_CLICK @ 12.51s",
   "CRYPTO_BURST @ 13.02s (duration 340 ms)" — not just a waveform.

5. **Multi-modal attack support.** One device, five attack classes.
   Switch attack profiles from the app: Keyboard, HDD, Printer, SMPS-Crypto,
   Relay. Each profile loads a different ML model and DSP configuration
   optimized for the target emanation frequency range and temporal
   characteristics.

6. **Tamper-evident design.** Accelerometer-based tamper detection
   wipes the capture buffer and crypto keys on physical disturbance. The
   device has no persistent key material — BLE session keys are
   ephemeral and derived via ECDH on each connection.

---

## 3. Attack Surface & Threat Model

### 3.1 Targeted attack classes

ACOUSTIC-PHANTOM targets **acoustic emanations** from mechanical and
electromechanical components within or attached to electronic devices. The
emanations are an unintended information channel — the device manufacturer
did not design the component to leak state information, but it does so
through acoustic radiation.

#### Keyboard acoustic eavesdropping

Mechanical keyboards (including IBM Model M, Cherry MX-based boards, and
laptop chiclet keyboards with distinct tactile mechanisms) produce
key-specific acoustic signatures on each actuation. The sound of a key
press depends on the key's position on the keyboard (which affects the
resonant frequency of the keycap/switch assembly), the switch type, and
the force profile. ACOUSTIC-PHANTOM captures each click, extracts a
spectral envelope feature vector, and feeds it to a pre-trained
classification model that maps the acoustic signature to a key identity.

**Recovery potential:** With a trained model for a specific keyboard,
per-key classification accuracy of 70–90% has been demonstrated in
literature. Language-model post-processing (n-gram or LLM-based)
improves effective text recovery to >95% for English prose. Passwords
and random strings are harder (no language model help) but still leak
timing patterns (inter-keystroke intervals reveal rhythm-based
information).

#### HDD seek acoustic analysis

Spinning hard drives produce audible seek sounds — the actuator arm
moving the read/write head across platters generates a short chirp whose
frequency profile correlates with the seek distance. By capturing and
classifying seek sounds, an attacker can infer which disk regions are
being accessed, potentially revealing file access patterns, directory
traversal, or the size of files being read. This is particularly
relevant against air-gapped systems where direct disk monitoring is not
possible but the drive's acoustic emanations propagate through air or
the chassis.

#### Printer forensics

Dot-matrix and some thermal/inkjet printers produce sound correlated
with the printed content. Dot-matrix print head impact sounds can be
reverse-engineered to reconstruct the printed text. Thermal printers
produce stepper-motor sounds correlated with feed distance, enabling
partial content inference (receipt lengths, bar code timing).

#### SMPS coil whine / capacitor singing

Switching-mode power supplies use inductors and transformers that vibrate
at the switching frequency (typically 100 kHz–2 MHz) due to
magnetostriction and piezoelectric effects in ceramic capacitors. While
the fundamental is ultrasonic, sub-harmonics and mechanical resonances
often produce audible whine whose amplitude and frequency shift with
load. During cryptographic operations, the CPU's power draw changes
characteristically (e.g., AES rounds cause periodic load spikes),
modulating the coil whine. ACOUSTIC-PHANTOM can detect these modulation
patterns to identify when crypto operations occur and, in some cases,
infer the operation type and duration — a metadata leak that can aid
timing attacks or confirm the presence of encryption.

#### Relay click analysis

Electromechanical relays produce a sharp click on state change. In
access control systems, industrial controllers, and some IoT devices,
relay clicks directly encode state transitions. By timestamping clicks
and correlating with known state machine logic, an attacker can infer
the device's internal state sequence without accessing any digital
interface.

### 3.2 What ACOUSTIC-PHANTOM does NOT attack

* **Software-only side channels** (cache timing, branch prediction).
  These require electrical/proximity access, not acoustic.
* **Electromagnetic emanations** (TEMPEST). That is a separate attack
  class handled by near-field EM probes (see: trace-reaper).
* **Ultrasonic covert channels between cooperating devices** (see:
  silent-symphony). ACOUSTIC-PHANTOM is a passive receiver/analyzer, not
  a covert channel transmitter.
* **Devices with no acoustic emanations** (solid-state drives with no
  moving parts, devices in sealed sound-isolated enclosures).

### 3.3 Attack surface of the device itself

| Surface | Risk | Mitigation |
|---------|------|------------|
| BLE pairing | Rogue connection | ECDH P-256 + numeric comparison; no static key |
| Firmware update | Malicious FW | Signed images (Ed25519), bootloader verifies signature |
| Physical capture | Key/buffer recovery | Tamper switch → zeroize SRAM + wipe flash buffers |
| microSD extraction | Captured audio leak | Optional AES-256-CTR encryption of stored captures |
| Acoustic counter-detection | Target hears the device | MEMS mics are silent; device has no speakers; piezo is passive |

---

## 4. Hardware Specifications

### 4.1 MCU & processing

| Component | Part | Role |
|-----------|------|------|
| MCU | STM32H743VIT6 | Cortex-M7 @ 480 MHz, double-precision FPU, 2 MB flash, 1 MB SRAM, L1 cache (16 KB I + 16 KB D). Runs the RTOS, DSP pipeline, TFLM inference, BLE stack, and storage. |
| DSP | CMSIS-DSP (on-chip) | FFT, FIR filter, correlation, MFCC extraction using Cortex-M7 DSP extensions (SIMD). |
| ML accelerator | TFLM int8 | TensorFlow Lite for Microcontrollers, quantized models, on-chip inference at 10–50 inferences/sec depending on model. |

### 4.2 Audio acquisition

| Component | Part | Role |
|-----------|------|------|
| MEMS mic array | 4× SPH0641LU4H-1 | I²S PDM microphones, 48 kHz, 16-bit, SNR 65 dB, omni-directional. Arranged in a 40 mm linear array for delay-and-sum beamforming. |
| Contact sensor | Piezoelectric disc (PZT-5H, 20 mm) + AD8233 charge amplifier | Direct surface coupling, 10 Hz–20 kHz bandwidth, for capturing internal vibrations through the target chassis. |
| Audio codec | WM8904 | 24-bit stereo CODEC for the contact piezo channel and reference ambient mic. I²S interface to the MCU. |
| Anti-aliasing | Passive RC (3rd-order, 24 kHz cutoff) | On each mic channel before the PDM decimator. |

### 4.3 Connectivity

| Component | Part | Role |
|-----------|------|------|
| BLE | CYW20819 module | BLE 5.0, encrypted GATT profile for app communication. UART interface to MCU. |
| USB | USB-C (USB 2.0 FS) | Firmware update, serial console, battery charging. |
| microSD | Hirose DM3D-SF | FAT32 storage for raw audio captures and exported datasets. |

### 4.4 Storage

| Component | Part | Role |
|-----------|------|------|
| NOR flash | W25Q128JV (128 Mbit, QSPI) | ML model storage, firmware backup, configuration, capture metadata. |
| microSD | Removable | Raw audio capture dumps (WAV format, optional AES-256-CTR encrypted). |

### 4.5 Power

| Component | Part | Role |
|-----------|------|------|
| Charger | BQ25895RTW | USB-C charging, 3 A, I²C programmable. |
| Battery | 800 mAh LiPo (3.7 V) | ~6 h continuous capture, ~12 h idle. |
| PMIC | TPS63031 (buck-boost) | 3.3 V system rail from 2.5–5.5 V input. |
| Fuel gauge | BQ27421 | Battery state-of-charge via I²C. |

### 4.6 Sensors & UI

| Component | Part | Role |
|-----------|------|------|
| Accelerometer | LIS2DH12 | Tamper detection, device orientation. |
| Display | 1.3" SSD1306 OLED (128×64, I²C) | Status, battery, active profile, event count. |
| LEDs | 3× WS2812B | Power/armed/capture status indicators. |
| Buttons | 3× tactile | POWER, PROFILE, ARM/DISARM. |

### 4.7 Form factor

| Dimension | Value |
|-----------|-------|
| Enclosure | 120 × 22 × 14 mm, 6061-T6 aluminum tube, anodized black |
| Weight | ~42 g (without probe) |
| Contact probe | Detachable, 150 mm flexible gooseneck with 20 mm PZT disc tip |
| Connectors | USB-C (bottom), microSD (side, under cover), probe jack (top, 3.5 mm) |
| Operating temp | 0 °C to 45 °C |
| Storage temp | -20 °C to 60 °C |

---

## 5. Architecture & Block Diagram

```
                            ┌──────────────────────────────────────────────┐
                            │              ACOUSTIC-PHANTOM                 │
                            │                                              │
   ┌─────────┐   I²S PDM   │  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐         │
   │ MEMS 0  ├─────────────┼─▶│     │  │     │  │     │  │     │         │
   │ MEMS 1  ├─────────────┼─▶│ PDM │  │     │  │     │  │     │         │
   │ MEMS 2  ├─────────────┼─▶│ Dec │  │ FIR │  │ Beam │  │ DSP │         │
   │ MEMS 3  ├─────────────┼─▶│ im. │  │ Fil │  │ form │  │ Pipe│         │
   └─────────┘             │  │     │  │ ter │  │     │  │     │         │
                           │  └─────┘  └─────┘  └──┬──┘  └──┬──┘         │
   ┌─────────┐  Charge     │  ┌─────┐  ┌─────┐     │        │            │
   │ PZT     ├─ Amp ───────┼─▶│WM   │  │     │     │        │            │
   │ Contact │  (AD8233)   │  │8904 │  │ AGC │──────┘        │            │
   └─────────┘             │  │     │  │     │               │            │
                           │  └─────┘  └─────┘               │            │
                           │                                 │            │
                           │                    ┌────────────▼──────────┐ │
                           │                    │   STM32H743VIT6       │ │
                           │                    │   Cortex-M7 @ 480MHz  │ │
                           │                    │                       │ │
                           │                    │  ┌─────────────────┐  │ │
                           │                    │  │  FreeRTOS       │  │ │
                           │                    │  │  ├─ Capture     │  │ │
                           │                    │  │  ├─ DSP         │  │ │
                           │                    │  │  ├─ TFLM Infer  │  │ │
                           │                    │  │  ├─ BLE Bridge  │  │ │
                           │                    │  │  ├─ Storage     │  │ │
                           │                    │  │  └─ UI          │  │ │
                           │                    │  └─────────────────┘  │ │
                           │                    │                       │ │
                           │                    │  ┌───────┐ ┌────────┐ │ │
                           │                    │  │ QSPI  │ │ SDMMC  │ │ │
                           │                    │  │ NOR   │ │ microSD│ │ │
                           │                    │  └───────┘ └────────┘ │ │
                           │                    └───────────┬──────────┘ │
                           │                                │            │
                           │                    ┌───────────▼──────────┐ │
                           │                    │  CYW20819 BLE 5.0    │ │
                           │                    │  ECDH P-256 + AES    │ │
                           │                    └───────────┬──────────┘ │
                           └────────────────────────────────┼────────────┘
                                                            │ BLE 5.0
                                                   ┌────────▼────────┐
                                                   │  Companion App  │
                                                   │  (React Native) │
                                                   │  iOS / Android  │
                                                   └─────────────────┘
```

### Signal flow summary

1. **MEMS array path:** 4× PDM microphones → I²S → PDM-to-PCM decimation
   (hardware) → per-channel FIR bandpass → delay-and-sum beamformer →
   beamformed mono output at 48 kHz / 16-bit.

2. **Contact piezo path:** PZT disc → AD8233 charge amplifier → WM8904
   CODEC → I²S → AGC → mono output at 48 kHz / 16-bit.

3. **DSP pipeline:** Both channels enter the real-time DSP pipeline:
   framing (25 ms, 10 ms hop) → Hann window → 512-point FFT → power
   spectrogram → MFCC extraction (for keyboard model) or spectral
   envelope (for HDD/printer models) or AM demodulation (for SMPS
   coil whine).

4. **Event detection:** Energy-based onset detector flags acoustic
   events. Each event is windowed (±150 ms context) and the feature
   vector is passed to the appropriate TFLM model for classification.

5. **Output:** Classified events are formatted as timeline entries and
   pushed to the BLE GATT characteristic for the app. Raw audio is
   optionally written to microSD.

---

## 6. Theory of Operation — Acoustic Side Channels

### 6.1 Keyboard acoustic eavesdropping

Each key on a mechanical keyboard produces a sound on actuation. The
sound is dominated by the resonance of the keycap + switch stem +
switch housing assembly, which varies with key position (keys near the
edge of the PCB resonate differently from center keys) and switch type.
The acoustic signature is captured as a 300 ms window centered on the
onset detection point.

**Feature extraction:** 13 MFCC coefficients + delta + delta-delta (39
features total), computed over 25 ms frames with 10 ms hop. The MFCC
vector captures the spectral envelope that distinguishes keys.

**Classification model:** A compact 3-layer fully-connected neural
network (256 → 128 → N_keys) quantized to int8. For a standard 104-key
keyboard, N=104. The model is trained per-keyboard-type: the operator
records a calibration set (pressing each key 20× in known order) on the
target keyboard, and the device fine-tunes the last layer on-device
using logistic regression (no backprop needed — just a linear classifier
on top of frozen features). This calibration step is critical: it
adapts to the specific keyboard's acoustics and the recording
environment.

**Post-processing:** Inter-keystroke timing is recorded. An optional
n-gram language model (stored in QSPI flash, ~500 KB) can be applied
to improve text recovery from noisy per-key predictions.

### 6.2 HDD seek acoustic analysis

A spinning HDD's actuator arm moves the read/write head across platters
to seek to different tracks. The seek produces a brief chirp (5–20 ms)
whose frequency sweep correlates with seek distance. Short seeks
(adjacent tracks) produce high-frequency clicks; long seeks (across the
platter) produce lower-frequency sweeps.

**Feature extraction:** Spectrogram (512-point FFT, 5 ms frames) of the
seek event window. The frequency sweep trajectory is extracted as a
1-D contour.

**Classification model:** A 1-D CNN (3 conv layers + 2 dense layers)
classifies the seek into distance buckets (short / medium / long /
full-stroke). This reveals access patterns: a sequence of short seeks
suggests sequential file reads; long seeks suggest random access or
file system traversal.

### 6.3 Printer forensics

Dot-matrix printers produce impact sounds for each dot fired. The
timing of impacts encodes the character pattern. By capturing the
print audio and detecting individual dot impacts, the printed text can
be reconstructed.

**Feature extraction:** Envelope detection + onset detection for
individual dot impacts. Inter-dot timing gives the character width;
impact intensity gives the dot column pattern.

**Classification model:** A temporal model maps dot-impact timing
sequences to characters. For 9-pin and 24-pin printers, the pin firing
pattern is well-structured and recoverable.

### 6.4 SMPS coil whine / crypto detection

Switching power supplies produce audible whine whose amplitude
modulates with load. During cryptographic operations (AES, RSA), the
CPU draws periodic current, modulating the SMPS whine at the
operation's characteristic frequency.

**Feature extraction:** AM demodulation of the coil whine carrier
(typically 200–800 Hz audible component). The demodulated envelope is
analyzed for periodic patterns matching known crypto operation
signatures (e.g., AES-128 runs 10 rounds with characteristic 4.5 ms
spacing on a 48 MHz MCU).

**Detection model:** An anomaly detector (running mean/variance +
threshold) flags sustained periodic patterns. The detected pattern's
frequency and duration are reported as metadata: "Sustained periodic
load, 220 Hz modulation, 340 ms duration — consistent with AES
operation."

### 6.5 Relay click analysis

Each relay state change produces a sharp broadband click (~1 ms
duration). ACOUSTIC-PHANTOM detects clicks via energy onset detection,
timestamps them, and reports the inter-click interval sequence. The
operator correlates the click timeline with the target device's known
state machine logic to infer state transitions.

---

## 7. Firmware Details & Design Decisions

### 7.1 Architecture

The firmware is bare-metal C with a minimal cooperative scheduler (no
full RTOS — the task set is fixed and small). This reduces jitter in
the audio capture path, which is critical for beamforming phase
accuracy. The scheduler runs at 1 kHz tick and dispatches:

1. **Capture task** (highest priority, interrupt-driven): Moves audio
   frames from I²S DMA buffers into the processing queue. Strictly
   bounded: never blocks, never allocates.

2. **DSP task:** Runs the framing → FFT → MFCC / spectral envelope
   pipeline on the most recent frame. Outputs feature vectors to the
   event detector.

3. **Inference task:** When the event detector flags an event, runs the
   TFLM model on the event window. Produces a classification result.

4. **BLE task:** Formats results and pushes them over GATT
   notifications. Throttled to 50 Hz to avoid overwhelming the BLE
   link.

5. **Storage task:** Writes raw audio to microSD if enabled. Uses
   double-buffered DMA writes to avoid blocking.

6. **UI task:** Updates the OLED display (10 Hz refresh).

### 7.2 Key design decisions

**Why bare-metal over FreeRTOS?** The audio capture path needs
guaranteed <10 μs jitter on the DMA interrupt. FreeRTOS's scheduler
adds unpredictable latency due to task switching and priority
inversion. A cooperative scheduler with interrupt-driven capture
gives deterministic timing. The trade-off is more manual state
management, which is acceptable for a fixed-function appliance.

**Why MFCC for keyboard classification?** MFCCs are the standard
feature for acoustic classification — they compactly represent the
spectral envelope (which distinguishes keys) in ~39 dimensions.
Alternatives (raw spectrograms, wavelet packets) require larger models
and more compute. MFCC + a compact MLP achieves good accuracy with a
model small enough (<100 KB) to fit in the STM32H7's flash alongside
the firmware.

**Why per-keyboard calibration?** The acoustic signature of a key
depends on the specific keyboard's construction, the recording
environment, and the microphone distance. A universal model would need
to generalize across hundreds of keyboard types — infeasible on-device.
Instead, the device ships with a generic feature extractor and a
per-keyboard calibration step that trains a linear classifier on-site.
This is a one-shot adaptation: the operator presses each key 20× in
guided sequence, the device computes class means and a shared covariance,
and stores the classifier weights in QSPI flash. The whole calibration
takes ~5 minutes.

**Why QSPI NOR for models?** TFLM models are read-only during inference.
QSPI NOR provides fast random access (up to 133 MHz quad-SPI) with
execute-in-place (XIP) capability. The model tensors are memory-mapped
directly — no copy into SRAM needed. This allows models up to 1 MB
while keeping the 1 MB SRAM free for audio buffers and working memory.

**Why two front-ends (MEMS + PZT)?** Far-field MEMS mics are essential
for keyboard eavesdropping at a distance. Contact piezo is essential
for HDD and SMPS coil whine — those emanations are strong through the
chassis but weak in air. Having both front-ends makes the device
versatile across all attack classes.

### 7.3 Memory map

```
  0x0800_0000 - 0x0801_FFFF  Firmware (128 KB)
  0x0802_0000 - 0x081F_FFFF  TFLM models, n-gram LM, calibration data (1.75 MB)
  0x9000_0000 - 0x9FFF_FFFF  QSPI NOR (W25Q128, 16 MB, XIP-mapped)
  0x2400_0000 - 0x2407_FFFF  SRAM audio buffers (512 KB, DMA + beamforming)
  0x2408_0000 - 0x240F_FFFF  SRAM DSP + inference working memory (512 KB)
  0x3000_0000 - 0x3001_FFFF  SRAM BLE stack + stack/heap (128 KB)
```

### 7.4 BLE protocol

The BLE GATT profile exposes these characteristics:

| UUID | Name | Direction | Description |
|------|------|-----------|-------------|
| 0xFF01 | Command | App→Device | Attack profile selection, arm/disarm, config |
| 0xFF02 | Event Stream | Device→App | Classified event notifications (notify) |
| 0xFF03 | Audio Stream | Device→App | Downsampled waveform (8 kHz mono, notify) |
| 0xFF04 | Status | Device→App | Battery, state, capture count, beam bearing |
| 0xFF05 | Calibration | App→Device | Start calibration, record labeled samples |
| 0xFF06 | File Transfer | Bidirectional | microSD file listing + download |

All characteristics (except Status, which is read-only) are encrypted
using the BLE link's AES-CCM encryption, established via LE Secure
Connections (ECDH P-256). No data is sent over unencrypted BLE.

---

## 8. Application/Software Interface

The companion app is built in React Native for iOS and Android. It
provides five screens:

1. **Dashboard:** Connection status, battery, active profile, live event
   count, beam direction indicator.

2. **Live Event Feed:** Real-time stream of classified events with
   timestamps, type, confidence, and decoded value. Color-coded by
   event type. Tap an event to see its feature vector and audio
   snippet.

3. **Spectrogram View:** Real-time scrolling spectrogram of the active
   audio channel (beamformed MEMS or contact piezo). Pinch to zoom in
   frequency/time.

4. **Calibration:** Guided per-keyboard calibration wizard. Records
   labeled samples, shows per-key accuracy, exports calibration data.

5. **Capture Manager:** Browse microSD captures, download over BLE,
   export to phone storage, optional AES decryption.

### App architecture

```
app/
├── App.js                 — Navigation + BLE context provider
├── package.json           — React Native project manifest
├── screens/
│   ├── DashboardScreen.js — Status overview + profile selector
│   ├── EventFeedScreen.js — Live classified event stream
│   ├── SpectrogramScreen.js — Real-time spectrogram
│   ├── CalibrationScreen.js — Guided keyboard calibration wizard
│   └── CaptureManagerScreen.js — microSD file browser + download
├── components/
│   ├── EventCard.js       — Single event display card
│   ├── SpectrogramCanvas.js — Canvas-based spectrogram renderer
│   └── BeamIndicator.js   — Source bearing compass widget
└── utils/
    ├── ble.js             — BLE manager + GATT protocol
    ├── protocol.js        — Binary protocol codec (pack/unpack)
    └── crypto.js          — AES-256-CTR decrypt for stored captures
```

---

## 9. Use Cases for Red Teams, Security Researchers & Pentesters

### 9.1 Red team: Covert credential harvesting

A red team operator places ACOUSTIC-PHANTOM within 1–2 m of a target
workstation (e.g., on a desk, in a plant pot, clipped to a shelf). The
MEMS array beamforms toward the keyboard. Over a workday, the device
captures every keystroke, classifies them using a pre-calibrated model,
and exfiltrates the recovered text over BLE to the operator's phone
outside the building. Passwords typed during the session are captured
along with surrounding text context that helps disambiguate.

### 9.2 Security researcher: Keyboard side-channel evaluation

A researcher evaluating a new keyboard's side-channel resistance uses
ACOUSTIC-PHANTOM to measure per-key distinguishability. The calibration
screen shows a confusion matrix — if keys cluster into
indistinguishable groups, the keyboard has good side-channel
resistance. If most keys are distinguishable with >80% accuracy, the
keyboard is vulnerable. This informs keyboard design for
side-channel-resistant input devices.

### 9.3 Penetration tester: Air-gapped system activity inference

Against an air-gapped system with a spinning HDD, ACOUSTIC-PHANTOM's
contact probe is attached to the drive enclosure (or the system
chassis). The seek acoustic profile reveals file access patterns: a
burst of short seeks = sequential file read; long seeks = random
access or directory traversal. This metadata helps infer what the
air-gapped system is doing without any digital interface access.

### 9.4 Hardware security researcher: Crypto operation detection

A researcher studying power-side-channel countermeasures uses
ACOUSTIC-PHANTOM's contact probe on a device's chassis to detect
SMPS coil whine modulation during crypto operations. If the coil whine
modulation reveals AES round timing, the device leaks crypto metadata
acoustically — even if traditional power-side-channel countermeasures
(masking, hiding) are in place. This is a novel attack surface that
most side-channel countermeasure designers do not consider.

### 9.5 Physical penetration tester: Access control relay analysis

A pentester assessing an access control system (e.g., a door controller
with electromechanical relays) places ACOUSTIC-PHANTOM near the
controller. The relay click timeline reveals the state machine
sequence: "relay A closed → 2.3s delay → relay B closed → 0.1s → relay
A opened" = unlock → door open delay → relock. This helps reverse-
engineer the controller's logic without accessing its firmware.

### 9.6 Academic: Printer content recovery

A researcher studying printer forensics uses ACOUSTIC-PHANTOM to
capture dot-matrix printer audio and reconstruct printed content. This
demonstrates the information leakage of physical printing processes and
informs secure printing practices in sensitive environments.

---

## 10. Legal & Ethical Disclaimer

**ACOUSTIC-PHANTOM is a security research tool intended exclusively for
authorized use.** Acoustic side-channel analysis of devices, equipment,
or persons without explicit written authorization may violate:

* **Wiretap statutes** (e.g., 18 U.S.C. § 2511 in the United States) —
  acoustic interception may constitute illegal eavesdropping.
* **Computer fraud and abuse laws** (e.g., 18 U.S.C. § 1030) — using
  captured information to access systems without authorization is a
  federal crime.
* **Privacy and data protection regulations** (GDPR, CCPA, and others)
  — capturing acoustic data that reveals personal information (e.g.,
  typed text) without consent may violate privacy law.
* **Industry-specific regulations** (HIPAA, PCI-DSS, SOX) — acoustic
  capture near regulated environments may violate compliance
  requirements.

**The author (jayis1) assumes no liability for any misuse of this
design.** Users are solely responsible for ensuring they have proper
authorization before deploying ACOUSTIC-PHANTOM in any environment.
This includes:

1. Written authorization from the device/equipment owner.
2. Awareness of and compliance with all applicable local, state, and
   federal laws.
3. Proper handling and secure deletion of any captured data.
4. Notifying and obtaining consent from any persons whose acoustic
   emanations (e.g., keystrokes) may be captured.

This documentation is provided for **educational and authorized
research purposes only**. The design, firmware, and companion app are
open-source to enable transparency, reproducibility, and defensive
research — not to facilitate unauthorized surveillance.

**If you are unsure whether your use case is authorized, assume it is
not.**

---

*Designed and authored by jayis1. Released under CERN-OHL-S v2
(hardware), GPL-2.0 (firmware), and MIT (app).*