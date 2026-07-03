# RadarPhantom — Portable mmWave FMCW Radar Phantom-Target Injector

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Interfering with automotive radar (ADAS) on vehicles you do not own, on public roadways, or in any manner that could affect the safety of unconsenting third parties may violate telecommunications regulations (e.g., 47 U.S.C. § 333 FCC rules, ETSI EN 302 858 in the EU), vehicle safety laws, and computer-fraud / critical-infrastructure statutes in your jurisdiction. Transmitting in the 76–81 GHz allocated radiolocation band without appropriate experimental authorization is illegal in most countries. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization, operate only in shielded test ranges or anechoic chambers, and never deploy near public roads.** This documentation is provided for educational and authorized research purposes only.

---

## Overview

**RadarPhantom** is a battery-operated, handheld hardware tool that injects **synthetic phantom targets** into the field of view of an automotive FMCW (Frequency-Modulated Continuous-Wave) radar — the type used by virtually every modern ADAS suite for Adaptive Cruise Control, Autonomous Emergency Braking, Blind-Spot Monitoring, and Lane-Change Assist. It is, in essence, a miniaturized **Digital Radio Frequency Memory (DRFM) repeater** for the 77–81 GHz automotive radar band: it captures the victim radar's own transmitted chirp, holds it in a digital delay buffer, optionally applies a Doppler frequency shift to emulate radial velocity, scales the amplitude to emulate a chosen radar cross-section (RCS), and re-radiates the modified signal back toward the victim sensor. To the victim radar's signal-processing chain, the replayed waveform appears to have reflected off a real physical object at a programmable range, with a programmable velocity, and with a programmable RCS — a **phantom target** that does not physically exist.

| Parameter | Capability |
|-----------|------------|
| **Operating band** | 76–81 GHz automotive FMCW (UWB SRR) |
| **Technique** | DRFM repeater jammer (coherent retransmit) |
| **Phantom range** | 0.5 m – 250 m (programmable round-trip delay) |
| **Phantom velocity** | ±300 km/h (programmable Doppler shift, ±250 kHz) |
| **Phantom RCS** | −40 dBsm to +20 dBsm (programmable gain) |
| **Target patterns** | Single point, multi-point cluster (vehicle silhouette), sweep (approach/depart), micro-Doppler (pedestrian gait) |
| **Control interface** | BLE 5.0, USB-C CDC, on-device OLED + joystick |
| **Battery** | 2× 18650 Li-ion, ~90 min active |
| **Form factor** | Handheld wand, 210 × 55 × 35 mm, 380 g |

### What RadarPhantom is for

RadarPhantom lets a red team or ADAS-research lab answer questions that no software-only or bench-simulation tool can:

- **Will the AEB falsely brake for a ghost obstacle?** Inject a phantom vehicle at 30 m closing at 120 km/h and observe whether the victim's planner triggers an emergency stop.
- **Is the ACC lock-on spoofable?** Inject a phantom at a stable range/velocity and check whether the victim's cruise controller acquires and follows the ghost instead of the real lead car.
- **Does the radar's tracker reject a physically-impossible target?** Inject a phantom that teleports (range discontinuity) or moves at Mach-1 and see whether the Kalman-filter tracker accepts or flags it.
- **Is the micro-Doppler classifier fooled?** Inject a pedestrian-gait micro-Doppler signature onto a phantom and see whether the victim classifies it as a VRU (vulnerable road user).
- **Robustness regression in CI:** as part of an ADAS release pipeline, play a library of known phantom scenarios against each radar ECU revision and gate the release on tracker-rejection metrics.

### Why RadarPhantom is novel

Existing radar-target simulators fall into two camps:

1. **Benchtop radar target simulators** (e.g., Keysight UXA + analog delay line, Rohde & Schwarz AREG) are rack-mount, cost $200k+, require an anechoic chamber and a host PC, and emulate one static target. They are calibration instruments, not red-team tools.
2. **Open-source research prototypes** are single-board SDR + horn antenna rigs that require a laptop, an external 77 GHz transceiver eval board (e.g., TI AWR2944BOOST), and a MATLAB script to configure each scenario — fine for a paper, useless in a parking-lot engagement.

RadarPhantom is the first **self-contained, pocket-sized, battery-powered DRFM repeater** with an on-device OLED UI, a BLE companion app, a scriptable scenario engine, and a built-in scenario library designed for offensive ADAS assessment. It is to automotive radar what a WiFi pentest dongle is to 802.11: a purpose-built, portable, operator-friendly tool for authorized adversarial testing.

---

## Attack Surface and Threat Model

### Threat model

| Asset | What the adversary (red team) wants |
|-------|-------------------------------------|
| **ADAS ECU** | Cause false-positive AEB/ACC/BSD events or false-negative (mask a real threat) to degrade safety functions |
| **Autonomous driving stack** | Confuse the perception/fusion layer with phantom returns that survive into the object list |
| **Tracker / Kalman filter** | Inject targets that the tracker accepts as real and acts upon (brake, steer, warn) |
| **Micro-Doppler classifier** | Mislabel a phantom as a pedestrian / cyclist to manipulate right-of-way behavior |

### Attack techniques implemented

| Technique | How RadarPhantom implements it |
|-----------|-------------------------------|
| **Range-gate pull-off (RGPO)** | DRFM repeats the victim chirp at an increasing delay so the phantom "walks" away from the real target range gate, then drops the real return. |
| **Velocity-gate pull-off (VGPO)** | Doppler NCO ramps the phantom's apparent velocity away from a real target's Doppler, stealing the tracking filter. |
| **Multiple false targets (MFT)** | FPGA stores the captured chirp in a multi-tap delay buffer; each tap re-radiates with its own delay/Doppler/RCS — producing a cluster of ghost returns (e.g., a synthetic "vehicle" silhouette across 4 range bins). |
| **Spot jamming (noise)** | Optional non-coherent noise mode saturates the victim's receiver front-end to deny detection (range/velocity denial). |
| **Micro-Doppler injection** | Doppler NCO is modulated with a periodic gait signature (leg-swing harmonics) so the phantom's Doppler spectrum resembles a walking pedestrian. |
| **Flash / glint spoof** | Amplitude modulator applies specular-glint statistics (Rice/Rayleigh fading) to mimic real-target RCS fluctuation. |

### Trust boundary

RadarPhantom operates entirely in the **physical RF layer** between the radar sensor and the environment. It sends no data over CAN/Ethernet/OBD-II; it does not touch the vehicle's network. The attack surface is the **radar sensor's front-end and downstream signal-processing assumptions** (that returns are real, that range = delay×c/2, that velocity = Doppler×λ/2, that the tracker's gating will reject physically-impossible kinematics). RadarPhantom tests whether those assumptions hold.

---

## Hardware Specifications

### Block-level bill of materials

| Subsystem | Part | Role |
|-----------|------|------|
| **RF front-end (RX)** | Custom 77–81 GHz receiver: WR-12 horn → LNA (HMC1043) → sub-harmonic mixer (LO 38–40.5 GHz from a PLL) → IF 1–4 GHz | Capture victim chirp, downconvert to a manageable IF |
| **RF front-end (TX)** | IF → upconversion mixer (same LO) → PA (HMC998) → WR-12 horn | Re-radiate the modified phantom return |
| **LO synthesizer** | ADF41520 24 GHz fractional-N PLL ×3 to 72–81 GHz (or a fixed 38 GHz ×2 for sub-harmonic mixing) | Coherent LO for both RX and TX mixers |
| **ADC** | AD9213 12-bit 10 GSPS (or MAX19711 12-bit 1.8 GSPS in the band-limited path) | Digitize the IF for the DRFM buffer |
| **DRFM core** | Lattice iCE40UP5K FPGA (5K LUTs, 16 DSP, 1 Mb SPRAM) + 2× ISSI IS66WVH32M8 SRAM (32 Mbit each) | Delay buffer, Doppler NCO, amplitude scaler, multi-tap target generator |
| **DAC** | AD9789 14-bit 2.5 GSPS | Reconstruct the IF for upconversion |
| **Control MCU** | STMicro STM32H743VIT6 (Cortex-M7, 480 MHz, 2 MB Flash, 1 MB RAM) | Scenario engine, BLE/USB, OLED, joystick, SD, power management |
| **BLE** | STM32H743 built-in or nRF52840 companion | Operator control link |
| **Display** | 1.3" OLED (SH1107, 128×96) + 5-way joystick | On-device UI |
| **Storage** | Micro-SD socket (SPI mode) | Scenario scripts, capture logs |
| **Power** | 2× 18650 (7.4 V, ~3000 mAh) → 5 V/3 A buck → 3.3 V/1.5 A LDO; 3.3 V → ±5 V charge pump for RF bias | ~90 min active, ~6 h standby |
| **USB** | USB-C CDC (STM32 USART3 ↔ CP2102N or native FS OTG) | Config, scenario upload, log download |
| **Form factor** | Handheld wand: two WR-12 horn antennas on the business end, OLED + joystick on the operator end, 210 × 55 × 35 mm, ~380 g | Ruggedized, single-hand operation |

### Power budget

| Rail | Load | Source |
|------|------|--------|
| 3.3 V digital | MCU + FPGA + OLED + BLE: ~280 mA | 3.3 V LDO from 5 V buck |
| 5 V analog | ADC + DAC + IF amps: ~420 mA | 5 V buck from 7.4 V |
| ±5 V RF bias | LO PLL + mixers + LNA + PA: ~310 mA | Charge pump from 5 V |
| **Total** | ~1.0 A @ 7.4 V ≈ 7.4 W | 2× 18650, ~90 min |

### Form factor notes

The wand shape keeps the two horn antennas (RX and TX) at the same end with ~30 mm separation for good transmit/receive isolation (the FPGA's DRFM buffer also provides a ~150 ns minimum delay that rejects direct leakage). The OLED + joystick sit at the operator end so the antennas point away from the operator, reducing self-exposure and mimicking a laser-pointer grip.

---

## Architecture and Block Diagram

```
                 ┌───────────── RadarPhantom wand (76–81 GHz) ─────────────┐
                 │                                                         │
   victim        │   ┌──── RX horn ────┐               ┌──── TX horn ────┐  │  phantom
   radar  ───────┼──▶│ LNA  77-81 GHz  │  IF 1-4 GHz   │  PA  77-81 GHz  │──┼──────▶ victim
   chirp         │   │  ↓ mixer        │   ← upmix →   │  ↑ mixer        │  │   return
                 │   └────────┬────────┘               └────────▲────────┘  │
                 │            │ IF                              │ IF         │
                 │            ▼                                 │            │
                 │   ┌─────────────┐  digital  ┌───────────┐  │            │
                 │   │  AD9213 ADC  │──────────▶│ iCE40UP5K │──┼──────────┐ │
                 │   │  12b 10 GSPS │           │  DRFM core│  │          │ │
                 │   └─────────────┘           │ • delay   │  ▼          │ │
                 │            ▲                │ • Doppler │ ┌─────────┐ │ │
                 │            │  LO 38-40.5GHz │ • gain   │ │AD9789  │ │ │
                 │   ┌────────┴────────┐       │ • multi- │ │14b DAC │ │ │
                 │   │ ADF41520 PLL ×2  │──────▶│  tap     │ └─────────┘ │ │
                 │   │  (sub-harm LO)  │       └─────┬────┘              │ │
                 │   └────────┬────────┘             │ SPI cfg           │ │
                 │            │                       ▼                   │ │
                 │            │              ┌────────────────┐            │ │
                 │            │              │  STM32H743VIT6 │            │ │
                 │            │              │  (Cortex-M7)   │            │ │
                 │            │              │  scenario eng. │            │ │
                 │            │              └──┬──┬──┬──┬────┘            │ │
                 │            │                 │  │  │  │                 │ │
                 │            │            ┌────┘  │  │  └─────┐           │ │
                 │            │            ▼       ▼  ▼        ▼          │ │
                 │            │       ┌──────┐ ┌──────┐ ┌──────┐ ┌───────┐  │ │
                 │            │       │ OLED │ │BLE 5│ │USB-C │ │ µSD   │  │ │
                 │            │       │+stick│ │ nRF │ │ CDC  │ │ log   │  │ │
                 │            │       └──────┘ └──────┘ └──────┘ └───────┘  │ │
                 │            └────────────────────────────────────────────┘ │
                 └──────────────────────────────────────────────────────────┘
                                   2× 18650 (7.4 V, ~3000 mAh)
```

### Signal-flow narrative

1. **Capture.** The victim radar sweeps a 76–81 GHz FMCW chirp. RadarPhantom's RX horn captures it; the LNA boosts it; a sub-harmonic mixer (driven by a 38–40.5 GHz LO derived from the ADF41520 PLL) downconverts it to a 1–4 GHz IF.
2. **Digitize.** The AD9213 ADC samples the IF at 10 GSPS (Nyquist for the IF band). Samples stream into the iCE40UP5K FPGA.
3. **DRFM processing.** The FPGA writes the captured chirp into the SRAM-backed delay buffer. A numerically controlled oscillator (NCO) applies a Doppler frequency offset. An amplitude scaler applies the target RCS. For multi-target mode, several delay taps are read out and summed.
4. **Reconstruct.** The AD9789 DAC converts the processed IF back to analog.
5. **Re-radiate.** The upconversion mixer (same coherent LO) shifts the IF back to 77–81 GHz; the PA and TX horn re-radiate the phantom return toward the victim.
6. **Scenario control.** The STM32H743 runs the scenario engine: it sets the delay (range), Doppler (velocity), amplitude (RCS), and pattern (single / sweep / multi / micro-Doppler) according to a script or live operator input over BLE/USB/joystick. The OLED shows the current phantom parameters and battery state.

---

## Firmware Details and Design Decisions

### Design decisions

- **DRFM over analog delay line.** A purely analog delay line (e.g., a fiber-optic delay) can only produce one static range and cannot apply Doppler. A DRFM (digital buffer + NCO) gives programmable range, velocity, RCS, and multi-target generation in one pipeline. The trade-off is bandwidth: the FPGA + ADC/DAC chain must handle the full 4 GHz IF bandwidth, which the AD9213/AD9789 do at reduced ENOB — acceptable because the victim radar's processing gain recovers the signal.
- **Sub-harmonic mixing.** Generating a 77–81 GHz LO directly is hard; a 38–40.5 GHz LO with a sub-harmonic mixer is achievable with a 24 GHz PLL + ×3 (or ×2 of a 38 GHz fundamental). This also eases PLL phase-noise requirements.
- **iCE40UP5K as the DRFM core.** Fully open-source toolchain (Yosys + nextpnr), 5K LUTs are enough for a 4-tap delay buffer + NCO + scaler, and the on-chip SPRAM holds the NCO phase accumulator. External SRAM holds the delay buffer for long-range (250 m ≈ 1.67 µs ≈ 167 ksamples at 100 MSPS after decimation).
- **STM32H743 for control, not DSP.** Keeping the DRFM in the FPGA guarantees deterministic, sub-microsecond delay control; the Cortex-M7 handles the asynchronous control plane (UI, comms, scenario scripting, power) where jitter does not matter.
- **Scenario engine.** Firmware implements a tiny stack-based scenario VM (see `scenario.c`) so complex multi-target, time-varying attacks (RGPO walk-off, pedestrian gait) can be scripted in a few hundred bytes and stored on SD. This mirrors how the SideProbe device scripts capture campaigns.

### File map

| File | Responsibility | Approx. lines |
|------|----------------|---------------|
| `firmware/main.c` | Boot, main loop, command dispatcher, OLED menu | ~220 |
| `firmware/board.h` | Pin map, peripheral assignments, constants | ~120 |
| `firmware/registers.h` | STM32H743 register bases + bit definitions | ~150 |
| `firmware/drivers/board_init.c` | Clocks, GPIO, power rails, watchdog | ~160 |
| `firmware/drivers/drfm.c` | FPGA SPI protocol: delay/Doppler/gain/tap programming | ~190 |
| `firmware/drivers/scenario.c` | Scenario VM + built-in scenario library | ~210 |
| `firmware/drivers/radar_cfg.c` | Victim-band auto-detect (sweep parameters sniff) | ~150 |
| `firmware/drivers/lo_pll.c` | ADF41520 PLL programming & LO retune | ~140 |
| `firmware/drivers/ble_uart.c` | BLE 5.0 control link (UART to nRF52840) | ~110 |
| `firmware/drivers/usb_cdc.c` | USB-C CDC command interface | ~100 |
| `firmware/drivers/oled.c` | SH1107 OLED driver + menu rendering | ~150 |
| `firmware/drivers/sdcard.c` | SD SPI mode: scenario load, log write | ~120 |
| `firmware/drivers/power.c` | Battery monitoring, rail sequencing, low-battery cutoff | ~100 |
| `firmware/drivers/joystick.c` | 5-way joystick debounce + event queue | ~80 |
| `firmware/Makefile` | arm-none-eabi-gcc build | ~60 |

Total firmware: **~2060 lines** of C.

### Scenario VM

The scenario engine is a tiny bytecode interpreter (opcodes: `SET_RANGE`, `SET_VEL`, `SET_RCS`, `WAIT`, `LOOP`, `RAMP`, `MULTITAP`, `MICRODOPPLER`, `NOISE`, `END`). A scenario file on SD is a flat byte stream; the STM32H7 executes it at 1 kHz tick, programming the FPGA each tick. Built-in scenarios (in `scenario.c`) cover: *static phantom vehicle*, *closing sweep*, *RGPO walk-off*, *pedestrian micro-Doppler*, *multi-target cluster*. Operators can author new scenarios in a compact text format (see `scenario.c` comment block) or via the companion app.

---

## Application / Software Interface

### On-device UI

A 4-line OLED menu navigated with the 5-way joystick:

```
RadarPhantom  v1.0  [78%]
─────────────────────────
MODE: STATIC
RNG:  30.0 m   VEL:  0 km/h
RCS: +10 dBsm  TAPS: 1
─────────────────────────
[UP/DN] param  [OK] menu
[L/R]  value   [<]  stop
```

Menu items: `Mode`, `Range`, `Velocity`, `RCS`, `Taps`, `Scenario…`, `Band sniff`, `Log`, `Power off`.

### Companion app (React Native)

`app/src/screens/` provides:

- **ConnectionScreen** — BLE scan/connect to RadarPhantom, RSSI, firmware version.
- **DashboardScreen** — live phantom parameters (range/vel/RCS), battery, band-sniff status, quick presets.
- **ScenarioEditorScreen** — visual scenario builder (range/velocity ramps, micro-Doppler), save to device SD.
- **TargetScreen** — multi-tap cluster editor (arrange a synthetic vehicle silhouette across range/Doppler bins).
- **BandSniffScreen** — shows the detected victim band (start/stop freq, chirp rate, duty cycle) from `radar_cfg.c`.
- **LogScreen** — download/view on-device capture logs.
- **SettingsScreen** — LO calibration, RF power level, safety interlocks, firmware update.

A `DeviceContext` (BLE manager) shares the connection across screens; `DeviceProtocol` packs/unpacks the binary command frames described in `ble_uart.c`.

### Command protocol

Binary frames over BLE/USB, little-endian:

| Opcode (u8) | Payload | Response |
|-------------|---------|----------|
| `0x01 PING` | — | `0x81` + version (u16) + battery (u8) |
| `0x02 SET_RANGE` | range_cm (u32) | `0x82` + ack |
| `0x03 SET_VEL` | vel_mmps (s32) | `0x83` + ack |
| `0x04 SET_RCS` | rcs_q0d8 (s16) | `0x84` + ack |
| `0x05 SET_TAPS` | n_taps (u8) + array | `0x85` + ack |
| `0x06 LOAD_SCENARIO` | slot (u8) | `0x86` + ack |
| `0x07 SNIFF` | timeout_s (u8) | `0x87` + band info |
| `0x08 ARM` / `0x09 DISARM` | — | ack |
| `0x0A LOG_DUMP` | — | streamed frames |
| `0x0B SET_POWER` | level (u8) | ack |

Frames are CRC-8 protected; the `ARM` opcode is a two-step (arm + confirm) interlock to prevent accidental RF emission.

---

## Use Cases

### Red team / physical pentest

- **Parking-lot AEB trigger test (authorized).** With the vehicle owner's consent and in a controlled lot, place RadarPhantom 20 m in front of a stationary target vehicle. Inject a phantom at 30 m closing at 80 km/h and observe whether the victim's AEB arms and whether the instrument cluster shows a collision warning. This tests the AEB false-positive surface without the complexity of a real closing target.
- **ACC lock-on hijack.** On a test track with a real lead vehicle, inject a phantom at a slightly shorter range and matching velocity; see whether the victim's ACC switches its lock-on to the phantom and slows unnecessarily.
- **Tracker sanity regression.** Inject a phantom that jumps 50 m between frames (physically impossible). A robust tracker should reject it; a weak one will track it. This is a direct measurable for ADAS firmware QA.

### Security research / lab

- **Micro-Doppler classifier evasion.** Inject a phantom with pedestrian micro-Doppler and observe whether the victim's classifier labels it VRU — research whether classifiers can be spoofed into granting a phantom right-of-way.
- **Sensor-fusion conflict.** Inject a phantom that only the radar sees (no camera/lidar return) and study how the victim's sensor-fusion layer reconciles the radar-only target with the absence of corroborating detections.
- **RGPO/VGPO benchmark.** Measure each radar ECU vendor's resistance to range/velocity gate pull-off as a quantitative vendor-comparison metric.

### Penetration tester / product-security

- **ADAS product-security assessment.** As part of a vehicle cyber-security assessment (e.g., ISO/SAE 21434 TARA), RadarPhantom provides the physical-layer stimulus to exercise the radar's threat-detection and fail-safe behavior, which a purely network-based pentest cannot reach.
- **Regulatory / homologation support.** Document radar robustness against intentional interference for type-approval evidence packages.

---

## Repository Layout

```
radar-phantom/
├── README.md                 (this file)
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── board_init.c
│       ├── drfm.c
│       ├── scenario.c
│       ├── radar_cfg.c
│       ├── lo_pll.c
│       ├── ble_uart.c
│       ├── usb_cdc.c
│       ├── oled.c
│       ├── sdcard.c
│       ├── power.c
│       └── joystick.c
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── App.js
    ├── package.json
    └── src/
        ├── navigation/
        │   └── AppNavigator.js
        ├── components/
        │   ├── DeviceContext.js
        │   └── DeviceProtocol.js
        └── screens/
            ├── ConnectionScreen.js
            ├── DashboardScreen.js
            ├── ScenarioEditorScreen.js
            ├── TargetScreen.js
            ├── BandSniffScreen.js
            ├── LogScreen.js
            └── SettingsScreen.js
```

---

## Author

**jayis1** — original device concept, firmware, hardware design, and companion application. Released under GPL-2.0 (firmware/hardware) and MIT (app). See legal disclaimer above; for authorized research only.