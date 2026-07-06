# LumenTap

🔦 **Laser-Doppler Acoustic Eavesdropping Tool for Security Research**

---

> **Author:** jayis1
> **License:** MIT
> **Status:** Complete (Firmware, KiCad, App)
> **Date:** 2026-07-06

---

## ⚖️ Legal & Ethical Disclaimer

LumenTap is a **defensive and research-only** tool intended for authorized
security assessments, physical red-team engagements conducted under signed
scope, and academic study of acoustic/ optical side-channel threats.

Pointing a laser at a window, person, vehicle, or any surface you do not own
or have **written authorization** to test is illegal in most jurisdictions.
Even low-power Class 1M/2M lasers can cause eye injury when viewed through
optics. LumenTap ships with hardware interlocks, a keyed arming switch, and a
Class 1 software-limited emission mode. **Bypassing these interlocks or
operating the device outside of an authorized engagement is the sole
responsibility of the operator.** The author (jayis1) and contributors
disclaim all liability for misuse.

By building, flashing, or operating LumenTap you confirm that:
1. You have written authorization to test the target environment.
2. You have reviewed local laws on laser safety, surveillance, and
   interception of communications.
3. You will operate only in Class 1 (eye-safe) emission mode unless a
   qualified laser safety officer has authorized higher power.

---

## 1. Overview

LumenTap is an open-source **laser microphone** — a device that reconstructs
audio from the micro-vibrations of a remote reflective surface (most commonly
a window pane) by illuminating it with a collimated laser beam and measuring
the Doppler / amplitude modulation of the returned light. A high-speed
photodiode and low-noise transimpedance amplifier convert the optical return
into an electrical signal, which is digitized by an STM32H7 MCU and processed
by an on-board DSP pipeline (bandpass filtering, FM/AM demodulation,
AGC, decimation) and streamed to a host over **USB Audio Class 2.0**.

Unlike hobbyist "laser listener" projects that output raw noisy audio through
a 3.5 mm jack, LumenTap is a calibrated instrument:

- ** coherent detection** via a Mach-Zehnder interferometer topology option
  for sub-micron window displacement sensitivity.
- **On-device DSP** — 24-bit fixed-point FIR/IIR filters running on the
  STM32H7's Cortex-M7 with double-issue DSP instructions, freeing the host
  from real-time signal processing.
- **USB Audio Class 2.0** streaming at up to 192 kHz / 24-bit, so the device
  appears as a standard microphone to any OS — no custom drivers required.
- **SD-Card logging** of raw and demodulated audio for post-engagement
  forensic review.
- **Hardware interlocks**: keyed arming switch, ambient-light safety sensor,
  and a Class-1 software emission cap (≤ 0.39 mW visible, IEC 60825-1).
- **React Native companion app** for live waveform/spectrogram visualization,
  filter tuning, recording management, and engagement logging.

### Why a laser microphone belongs in a security research toolkit

Acoustic emanations are an established side-channel class. Window-glass
vibration eavesdropping has been documented in the TEMPEST literature since
the 1960s and remains a realistic threat model for:
- **SCIF / secure facility assessments** — quantifying how much speech leaks
  through perimeter glazing.
- **Red-team physical engagements** — demonstrating that an attacker across
  the street can recover conversation content without entering the building.
- **Counter-surveillance training** — teaching defenders what a laser
  bounce-back looks like and how to detect it (retro-reflective detection,
  laser glare).
- **Glass / facade security testing** — measuring the acoustic attenuation of
  laminated, double-glazed, and acoustic-grade glass specimens in a lab.

LumenTap gives researchers a reproducible, calibrated, open platform to study
this channel instead of relying on $20k commercial units or dangerous ad-hoc
rigs.

---

## 2. Attack Surface & Threat Model

| Asset (target) | Channel | Attacker position | Recoverable info |
|----------------|---------|-------------------|------------------|
| Interior conversation | Window-glass vibration ← room air pressure | Line-of-sight, 5–100 m | Speech content, speaker identity (with ML) |
| Glass-enclosed device (printer, keyboard) | Surface vibration from mechanical components | Line-of-sight | Keystroke timing, print pattern |
| Vehicle cabin | Side-window vibration | Line-of-sight, roadside | Conversation, hands-free audio |
| Solid-state relay / coil hum | Equipment enclosure vibration | Line-of-sight | Load switching events |

**Assumptions & limitations (honest):**
- Requires **line of sight** to a specular surface.
- Performance degrades sharply with double-glazing (the two panes couple
  out-of-phase at certain frequencies) and with curved/rough surfaces.
- Ambient acoustic noise (traffic, wind) couples into the glass and adds to
  the signal; the DSP pipeline includes spectral subtraction and a
  single-channel Wiener filter to suppress stationary noise.
- The Class 1 emission cap limits range to roughly 20–30 m on a clean
  single-pane target. Range scales with optical power, which is intentionally
  capped for safety.

**Defender countermeasures (which LumenTap helps validate):**
- Acoustic-grade laminated glass / double-glazing with desiccant gap.
- Window vibration dampers and exterior shutters.
- White-noise generators on the glass ("window agitators").
- Retro-reflective optical detectors that alarm on incident laser light.
- Interior blinds / curtains that break the air-to-glass coupling.

---

## 3. Hardware Specifications

| Subsystem | Part | Purpose |
|-----------|------|---------|
| MCU | **STM32H743VI** (LQFP-100, 480 MHz Cortex-M7, 2 MB flash, 1 MB RAM, 16-bit ADC @ 3.6 MSPS, HW DSP/FPU) | DSP pipeline, USB Audio, control |
| Laser source | **PLT5 510R** 520 nm laser diode (visible green, Class 1 capped) | Illumination of target |
| Laser driver | **iC-HK** constant-current laser driver with enable/shutdown | Current-limited, interlocked drive |
| Photodiode | **Hamamatsu S1223** (Si PIN, 320–1100 nm, 30 MHz bandwidth) | Return-light detection |
| TIA | **OPA380** (low-noise, 90 MHz GBP, transimpedance) | Photodiode → voltage |
| 2nd stage amp | **OPA1632** fully-differential amp | ADC differential drive |
| Audio codec (optional) | **CS4271** (24-bit, 192 kHz) | High-quality analog out / alt ADC |
| ADC (primary) | STM32H7 internal 16-bit ADC1 @ 192 kS/s | Optical return digitization |
| SD card | microSD via SDMMC1 (4-bit, 50 MHz) | Raw + demod logging |
| USB | USB 2.0 HS PHY on STM32H7 (480 Mbps) | UAC2 audio + CDC control |
| Optics | 520 nm narrowband BP filter + 25.4 mm collimating lens | Improve SNR, limit return |
| Safety | TSL257 ambient-light sensor + keyed arming switch + HW interlock | Emission safety |
| Power | USB-C (5 V), on-board 3.3/1.8 LDOs, battery backup option | Bus-powered |
| Form factor | 110 × 60 × 35 mm milled-aluminium enclosure, 1/4"-20 tripod mount | Field deployable |

### Power tree
```
USB-C 5V ──► MP1584 buck ─► 3.3V (MCU, logic, codec)
                          └► 1.8V (H7 VDDA1, analog)
                          └► 5.0V (laser driver, TIA rail)
```
The 5.0V analog rail is locally regulated from 3.3V via an LT3045
ultra-low-noise LDO (0.8 µVRMS) to keep laser driver noise out of the TIA.

---

## 4. Architecture & Block Diagram

```
                 ┌─────────────────────────── LumenTap ───────────────────────────┐
                 │                                                                │
   520 nm LD ──► collimator ──── free space ───► target window ───► return light   │
   (iC-HK)         │                                              │               │
      ▲            │                                              ▼               │
      │            │                                     BP filter + lens         │
   PWM              │                                              │               │
   enable           │                                              ▼               │
      │            │                                          S1223 PD            │
      │            │                                              │               │
      │            │                                          OPA380 TIA          │
      │            │                                              │               │
      │            │                                          OPA1632 FDA         │
      │            │                                              │               │
   Keyed ──► interlock logic ◄─── TSL257 ──► ADC1 (diff) ────────┘               │
   switch       │  ambient                            │                            │
      ▲         │                                    │ 192 kS/s 16-bit            │
      │         │                                    ▼                            │
   operator     │                          ┌─────────────────────┐                │
                │                          │  STM32H743VI        │                │
                │                          │  Cortex-M7 @ 480MHz │                │
                │                          │                     │                │
                │                          │  DSP pipeline:      │                │
                │                          │   - DC block        │                │
                │                          │   - BPF 20Hz-16kHz  │                │
                │                          │   - FM demod (quad) │                │
                │                          │   - AGC             │                │
                │                          │   - decimate →48k   │                │
                │                          │                     │                │
                │                          │  USB UAC2 audio     │────► Host PC / │
                │                          │  USB CDC control    │     Phone app  │
                │                          │  SDMMC1 logging     │────► microSD   │
                │                          └─────────────────────┘                │
                │                                   │                              │
                └───────────────────────────────────┘                              │
                                                                                 │
                 └────────────────────────────────────────────────────────────────┘
```

### Firmware signal flow

```
ADC1 (192 kS/s, 16-bit, DMA double-buffer)
   │
   ▼
[arm_fir_dc_block]   → remove DC / low-frequency drift from ambient light
   │
   ▼
[arm_biquad_cascade] → 8th-order bandpass 20 Hz–16 kHz (voice band)
   │
   ▼
[fm_demod_quadrature] → Hilbert transform → atan2 of (I,Q) → instantaneous freq
   │                      (recovers window velocity → displacement → audio)
   ▼
[wiener_noise_suppress] → spectral subtraction of stationary background
   │
   ▼
[arm_agc]            → gain riding, 60 dB range, 10 ms attack / 500 ms release
   │
   ▼
[decimate_4x]        → 192 kHz → 48 kHz, 24-bit packed for UAC2
   │
   ▼
USB UAC2 ISO endpoint (16-channel feedback) + SDMMC1 ring buffer
```

---

## 5. Firmware Details & Design Decisions

The firmware is bare-metal C for the STM32H743VI, built with
`arm-none-eabi-gcc` and the CMSIS-DSP library. No RTOS is used — the
real-time DSP path runs in ADC-half-transfer DMA ISRs with deterministic
timing, while the control plane (USB CDC, SD logging) runs in the main loop
and low-priority ISRs.

### Key design decisions

1. **No RTOS.** A 192 kHz / 16-bit audio path with a multi-stage DSP chain
   demands hard real-time deadlines (520 µs per block of 128 samples). An
   RTOS adds jitter to ISR latency. Instead the firmware uses a
   single-threaded super-loop with DMA-driven ISRs and a lock-free ring
   buffer for the SD-card logger.

2. **Quadrature FM demodulation for coherent mode.** In interferometric
   (coherent) detection, window displacement appears as phase modulation of
   the carrier. We approximate I/Q via a 64-tap Hilbert FIR (CMSIS-DSP
   `arm_fir_f32`) and demodulate with `atan2(I,Q)`. In direct-detection
   (amplitude) mode, we skip the Hilbert stage and use envelope detection.

3. **16-bit ADC @ 192 kS/s.** The H7's 16-bit ADC achieves ~14.5 ENOB at
   192 kS/s with oversampling disabled — sufficient for the ~60 dB SNR we
   need. Going to the external CS4271 codec adds 2-3 bits but doubles BOM
   cost; it's supported but optional.

4. **USB Audio Class 2.0.** UAC2 is natively supported on Linux, macOS, and
   iOS 13+, meaning LumenTap enumerates as a standard microphone with no
   driver. The firmware implements a UAC2 audio streaming interface
   (24-bit, 48 kHz, mono) plus a CDC-ACM control port for filter/laser
   commands.

5. **SD logging in parallel.** Raw ADC blocks and post-Demod audio are
   written to a FAT32 ring buffer via FatFs, allowing hours of capture
   without host connection. A header file written at session start records
   gain, filter cutoffs, laser power, and GPS-derived UTC (if available).

6. **Safety interlocks.** The laser enable line is AND-gated with (a) the
   keyed arming switch (HW), (b) the ambient-light sensor reading (SW, blocks
   emission if a human eye is likely in the beam path — heuristic), and
   (c) a software Class-1 power cap enforced in the PWM duty register. Any
   watchdog timeout also drops the laser.

### Build

```bash
cd firmware
make            # arm-none-eabi-gcc, CMSIS-DSP
make flash      # openocd -f interface/stlink.cfg -f target/stm32h7x.cfg
```

See `firmware/README` inside the folder for register-level detail.

---

## 6. Application / Software Interface

The companion app (`app/`) is a React Native (Expo) application that connects
to LumenTap over USB-OTG on Android / iOS. It provides:

- **LiveScreen** — real-time oscilloscope + spectrogram of the demodulated
  audio stream, plus laser power, gain, and SNR readouts.
- **TuneScreen** — adjust DSP parameters (bandpass cutoffs, AGC attack/release,
  noise-suppression depth, demod mode: AM/FM) and push them to the device over
  the CDC control endpoint.
- **RecordScreen** — session management: start/stop SD recording, tag
  captures, browse and play back recorded files pulled from the device.
- **SafetyScreen** — interlock status, ambient-light reading, emission class
  indicator, and a manual laser-enable kill switch.

The app speaks the LumenTap Wire Protocol (JSON-over-CDC, documented in
`app/utils/protocol.js`). Audio comes in via the platform's native USB Audio
HAL (the device is a standard UAC2 mic), so the app uses the OS audio API
rather than parsing raw USB isochronous packets.

---

## 7. Use Cases

### Red team
- Demonstrate that a target office's perimeter glazing leaks intelligible
  speech to an operator across the street, as part of a physical security
  assessment report.
- Capture audio from a vehicle parked outside a facility to show that
  in-cabin conversations (e.g., a smoking-area chat) are recoverable.

### Security researcher
- Measure the acoustic insertion loss of different glazing types
  (single, laminated, double, acoustic-grade) and publish comparative data.
- Train / test ML models for laser-microphone speech enhancement using
  the SD-logged raw + demod dataset.
- Study counter-surveillance: characterize retro-reflective detector
  response and window-agitator effectiveness.

### Counter-surveillance / defensive
- Detect the presence of an incident laser beam on a facility's windows
  using LumenTap in "passive receive" mode (laser disabled, photodiode only)
  as a sweep receiver.

### Pentester / physical
- Include an acoustic-leak finding in a physical pentest report backed by a
  recorded, timestamped audio capture chain-of-custody on the SD card.

---

## 8. Repository Structure

```
lumen-tap/
├── README.md
├── phase1_conceptual_architecture.md
├── phase2_component_selection_schematics.md
├── phase3_pcb_blueprints_layout.md
├── phase4_software_stack.md
├── firmware/
│   ├── Makefile
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── usb_descriptors.h
│   └── drivers/
│       ├── laser.c / laser.h
│       ├── dsp.c / dsp.h
│       ├── audio.c / audio.h
│       └── sdlog.c / sdlog.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── package.json
    ├── App.js
    ├── screens/
    │   ├── LiveScreen.js
    │   ├── TuneScreen.js
    │   ├── RecordScreen.js
    │   └── SafetyScreen.js
    ├── components/
    │   ├── Scope.js
    │   └── ParamSlider.js
    └── utils/
        └── protocol.js
```

---

## 9. Bill of Materials (summary)

| Ref | Part | Pkg | Qty | Est. USD |
|-----|------|-----|-----|----------|
| U1  | STM32H743VIT6 | LQFP-100 | 1 | 18.50 |
| U2  | iC-HK laser driver | QFN-16 | 1 | 6.20 |
| U3  | OPA380AIDR | SO-8 | 1 | 5.40 |
| U4  | OPA1632FDA | SSOP-8 | 1 | 4.10 |
| U5  | LT3045IDE#PBF | DFN-12 | 1 | 6.80 |
| U6  | MP1584EN buck | SOIC-8 | 1 | 1.20 |
| U7  | TSL257 ambient light | chipLED | 1 | 1.90 |
| D1  | PLT5 510R 520 nm LD | TO-18 | 1 | 8.30 |
| D2  | S1223 Hamamatsu PD | TO-5 | 1 | 12.60 |
| J1  | USB-C 16-pin | SMT | 1 | 1.10 |
| J2  | microSD socket | SMT | 1 | 1.40 |
| Misc | passives, lens, enclosure | — | — | ~12.00 |
| | | | **Total** | **~80.50** |

Under the $100 BOM target.

---

## 10. Safety Notes (read before building)

- The default firmware ships with **Class 1** emission (≤ 0.39 mW at 520 nm)
  enforced in the PWM duty register. Do not raise `LTM_LASER_PWR_MAX` without
  a laser safety officer's approval.
- The keyed arming switch is a **hardware** interlock in series with the
  laser driver enable. Do not bypass it.
- The TSL257 ambient-light sensor blocks emission when ambient IR/visible
  exceeds a threshold consistent with an observer in the beam path; this is a
  heuristic, not a guarantee. Always assume the beam is hazardous.
- Never point LumenTap at people, vehicles in motion, or aircraft.

---

## License

MIT © jayis1. See `LICENSE` in repo root. All hardware schematics, PCB
layouts, firmware, and app source are released under MIT.