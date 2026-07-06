# Phase 1: Conceptual Architecture — LumenTap

## Laser-Doppler Acoustic Eavesdropping Tool

**Document Version:** 1.0
**Date:** 2026-07-06
**Author:** jayis1
**Status:** Final

---

## 1. Executive Summary

LumenTap is a calibrated, open-source laser microphone for security
research. It illuminates a remote reflective surface (typically a window
pane) with a Class-1-capped 520 nm laser, captures the Doppler- and
amplitude-modulated return light with a high-speed Hamamatsu S1223
photodiode and an OPA380 transimpedance amplifier, digitises the result
with an STM32H743VI's 16-bit ADC at 192 kS/s, processes it through an
on-device DSP pipeline (DC block, bandpass, quadrature FM demod,
Wiener noise suppression, AGC, decimation), and streams the recovered
audio to a host as a USB Audio Class 2.0 microphone — no custom drivers.

### Key Differentiators

| Feature | Hobbyist rig | Commercial laser mic | **LumenTap** |
|---------|--------------|----------------------|--------------|
| Detection | Direct AM only | Direct or coherent | **AM + quadrature FM** |
| DSP | None (raw) | Proprietary black box | **Open 6-stage pipeline on Cortex-M7** |
| Host interface | 3.5 mm jack | Analog XLR | **USB UAC2 (driverless)** |
| Logging | External recorder | Internal HDD | **microSD raw + demod ring** |
| Safety | None | Limited | **HW keyed interlock + ambient sensor + Class-1 cap** |
| Open source | No | No | **Fully open (MIT), jayis1** |

---

## 2. System Architecture

```
┌────── LumenTap ──────────────────────────────────────────────────┐
│                                                                  │
│  PLT5 510R ── collimator ─── free space ──► target window        │
│  (iC-HK)         │                              │               │
│     ▲            │                              ▼               │
│  PWM/en          │                       BP filter + lens       │
│     │            │                              │               │
│  Interlock       │                           S1223 PD           │
│  logic           │                              │               │
│     │            │                           OPA380 TIA          │
│  Arm key +       │                              │               │
│  TSL257          │                           OPA1632 FDA         │
│                  │                              │               │
│                  │                       ADC1 (diff, 192k)      │
│                  │                              │               │
│                  │                       ┌──────┴───────┐       │
│                  │                       │ STM32H743VI  │       │
│                  │                       │ Cortex-M7    │       │
│                  └──── interlock ◄──────┤  DSP pipeline│       │
│                                          │  UAC2 + CDC  │──USB──► Host
│                                          │  SDMMC1      │──SD───► microSD
│                                          └──────────────┘       │
└──────────────────────────────────────────────────────────────────┘
```

### Bus topology

| Bus | Master | Slaves | Speed |
|-----|--------|--------|-------|
| SPI1 | STM32 | (reserved for FPGA in future revision) | 50 MHz |
| I2C1 | STM32 | TSL257 (0x29), CS4271 (0x4A optional) | 100 kHz |
| SDMMC1 | STM32 | microSD card | 50 MHz, 4-bit |
| USB OTG-HS (FS mode) | STM32 (device) | Host PC / phone | 12 Mbps FS |
| ADC1 + DMA1_S0 | DMA | — | 192 kS/s, 16-bit |
| TIM1_CH1 PWM | STM32 | iC-HK PWMI | 100 kHz |

### Power domains

| Rail | Source | Loads | Noise target |
|------|--------|-------|--------------|
| VBUS_5V | USB-C | → MP1584 buck | n/a |
| VCC_3V3 | MP1584 | MCU, I2C, SD, codec | < 1 mV ripple |
| VCC_1V8 | LDO from 3V3 | MCU VDDA1 (analog) | < 100 µV |
| VCC_5V_CLEAN | LT3045 from 3V3 | laser driver, TIA | < 0.8 µVRMS |

The clean 5 V rail is generated from 3V3 (not directly from USB 5V) via
an LT3045 ultra-low-noise LDO so that USB bus noise cannot modulate the
laser driver current — this is the single most important analog design
decision in the device.

---

## 3. Performance Targets

| Parameter | Target | Basis |
|-----------|--------|-------|
| Audio bandwidth | 20 Hz – 16 kHz | voice band |
| Output sample rate | 48 kHz / 24-bit | UAC2 standard |
| ADC sample rate | 192 kS/s / 16-bit | oversample 4× for decimator |
| System SNR (single-pane target, 10 m) | ≥ 18 dB | intelligible speech |
| Range (Class 1, single pane) | 20–30 m | eye-safety limited |
| Laser power (default) | ≤ 0.39 mW @ 520 nm | IEC 60825-1 Class 1 |
| On-board DSP latency | < 2 ms per block | 128-sample blocks @ 192k |
| SD write throughput | ≥ 400 KB/s | raw + demod stream |
| USB enumeration | UAC2 + CDC composite | Linux/macOS/iOS driverless |

---

## 4. Constraints & Assumptions

- **Line of sight required.** No NLoS recovery.
- **Single-pane glass works best.** Double-glazing introduces coupled-panes
  notches; the Wiener suppressor partially mitigates but cannot fully
  recover.
- **Class 1 cap is enforced in firmware** (PWM duty ≤ 39/255 ≈ 0.39 mW
  with the chosen diode). Raising this requires explicit operator
  override and is logged.
- **No RTOS.** The 192 kHz / 128-sample block leaves 666 µs of CPU per
  block; the CMSIS-DSP chain consumes ~120 µs on a 480 MHz M7, leaving
  ample headroom without RTOS jitter.
- **BOM ≤ $100.** See README §9.

---

## 5. Threat Model Summary (full version in README §2)

| Target | Channel | Range | Yield |
|--------|---------|-------|-------|
| Office window | glass vibration | 5–30 m | intelligible speech |
| Vehicle side window | glass vibration | 5–15 m | conversation, hands-free |
| Equipment enclosure | surface vibration | 1–10 m | switching events |

**Countermeasures LumenTap helps validate:** laminated/acoustic glass,
double-glazing, window agitators, retro-reflective optical detectors,
interior blinds.

— *jayis1*