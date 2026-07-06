# Phase 2: Component Selection & Schematics — LumenTap

**Document Version:** 1.0
**Date:** 2026-07-06
**Author:** jayis1
**Status:** Final

---

## 1. MCU — STM32H743VIT6

| Requirement | STM32H743VIT6 | alt: STM32F407 | alt: RP2040 |
|-------------|---------------|----------------|-------------|
| Core | Cortex-M7 @ 480 MHz | M4 @ 168 MHz | M0+ @ 133 MHz |
| FPU/DSP | DP FPU + DSP instructions | SP FPU | none |
| Flash / RAM | 2 MB / 1 MB | 1 MB / 192 KB | 16 KB / 264 KB |
| ADC | 16-bit, 3.6 MSPS, diff | 12-bit, 2.4 MSPS | 12-bit, 500 kSPS |
| USB | OTG-HS (FS/HS) | OTG-FS | USB 1.1 |
| SDMMC | SDMMC1+2, 4-bit, 50 MHz | SDIO 4-bit | none |
| Package | LQFP-100 | LQFP-100 | QFN-56 |

**Decision:** STM32H743VIT6. The 16-bit ADC is the deciding factor —
14.5 ENOB at 192 kS/s gives ~84 dB dynamic range, which is essential
for the weak return signal. The M7 DSP instructions let us run the full
pipeline without external DSP.

### Pin assignment (abridged — full table in `board.h`)

| Pin | Function | AF |
|-----|----------|----|
| PA0/PA1 | ADC1_INP0/INN0 (diff) | ANALOG |
| PA8 | TIM1_CH1 → laser PWM | AF1 |
| PA9 | USB OTG-HS VBUS | — |
| PA10 | USB OTG-HS ID | — |
| PA15 | I2C1 SCL (TSL257) | AF4 |
| PB0/PB1 | status / fault LEDs | GPIO out |
| PB3 | arm key input (HW interlock) | GPIO in |
| PB4 | laser driver EN (HW gated) | GPIO out |
| PB7 | I2C1 SDA | AF4 |
| PB8 | user button | GPIO in |
| PC8..PC11 | SDMMC1 D0..D3 | AF9 |
| PC12 | SDMMC1 CK | AF9 |
| PD2 | SDMMC1 CMD | AF9 |
| PD11/PD12 | USB OTG-HS DM/DP (FS) | AF10 |

---

## 2. Optical Front-end

### 2.1 Laser diode — PLT5 510R (Osram)

- 520 nm visible green — best eye-safety margin vs 650 nm red at equal
  photodiode responsivity (S1223 peaks ~720 nm but is flat 320–1100 nm).
- TO-18 package, 5 mW max continuous.
- Driven by iC-HK constant-current driver with PWM input and enable.
- Class-1 cap (0.39 mW) enforced by limiting PWM duty in firmware AND
  by a hardware AND-gate between the keyed switch and the iC-HK EN pin.

### 2.2 Photodiode — Hamamatsu S1223

- Si PIN, 3.3 mm × 3.3 mm active area, 320–1100 nm.
- 30 MHz bandwidth (way more than needed — ensures no phase distortion
  of the audio-band modulation).
- Dark current 0.1 nA; NEP 7.5 × 10⁻¹⁴ W/√Hz.

### 2.3 Transimpedance amplifier — OPA380

- 90 MHz GBP, low-noise (3 nV/√Hz), current-feedback architecture
  designed for photodiode TIAs.
- Feedback resistor R_f = 10 kΩ → transimpedance gain 10 kV/A.
- Bandwidth = sqrt(GBP / (2π · R_f · C_total)) ≈ 1.2 MHz, ample for
  192 kS/s.

### 2.4 Second stage — OPA1632 fully-differential amplifier

- Converts the single-ended TIA output to a differential pair to drive
  the STM32 ADC in differential mode (improves SFDR by ~6 dB).
- Gain = 2 V/V.

### 2.5 Optical bandpass + lens

- 520 nm narrowband BP filter (FWHM 10 nm) in front of the photodiode
  rejects ambient illumination.
- 25.4 mm achromatic collimating lens for the laser; matched 25 mm
  collection lens for the return path.

---

## 3. Power Supply

| Rail | Regulator | Input | Output | Noise | Loads |
|------|-----------|-------|--------|-------|-------|
| VCC_5V (bus) | — | USB-C | 5.0 V | n/a | → buck |
| VCC_3V3 | MP1584 buck | 5V | 3.3V | < 5 mV | MCU, SD, I2C, codec |
| VCC_1V8 | TPS7A4701 LDO | 3V3 | 1.8V | < 100 µV | MCU VDDA1 |
| VCC_5V_CLEAN | LT3045 LDO | 3V3 | 5.0V | < 0.8 µVRMS | laser drv, TIA |

**Critical:** the clean 5 V rail is derived from 3V3 (not 5V bus) so
that USB bus ripple cannot reach the laser driver. The LT3045 was chosen
specifically for its 0.8 µVRMS noise and 1 µA quiescent — it is the
analog keystone of the design.

### Decoupling

| IC | Decoupling |
|----|------------|
| STM32H743 | 10 × 100 nF X7R + 1 × 4.7 µF MLCC on VDD; 1 µF + 10 nF on VDDA |
| OPA380 | 100 nF + 10 µF tantalum on V+ |
| iC-HK | 100 nF + 1 µF on VDD, 10 nF on laser outputs |
| LT3045 | 4.7 µF in, 10 µF out, 10 nF SET cap |

---

## 4. Safety Subsystem

### 4.1 Keyed arming switch (HW interlock)

A physical keyed SPST switch in series with PB4 (laser EN). The MCU
cannot drive EN high unless the key is closed AND PB3 reads high. This
is a hardware AND — not software — so a firmware crash cannot enable
the laser.

### 4.2 TSL257 ambient-light sensor

- I²C address 0x29.
- 50 ms integration, 1× gain.
- If reading exceeds 6000 counts (heuristic for an observer in the
  beam path), the firmware drops the laser and sets
  `g_safety.ambient_safe = 0`.
- This is a heuristic, not a guarantee. The operator must always assume
  the beam is hazardous.

### 4.3 Software Class-1 power cap

- `LTM_LASER_PWR_MAX = 39` (0..255 PWM ticks).
- With the chosen diode and driver current, this corresponds to
  ~0.39 mW average at 520 nm — Class 1 per IEC 60825-1.
- Bypassing requires setting `operator_override`, which the firmware
  logs in the SD session header.

---

## 5. USB Interface

- USB-C receptacle (2.0 signals only; CC pins pulled down for device).
- STM32 OTG-HS in FS mode (no ULPI PHY — saves a chip and BOM).
- Composite device: UAC2 audio streaming + CDC-ACM control.
- Descriptors in `usb_descriptors.h`. VID 0x1209 (pid.codes), PID 0x4C54
  ('LT').

---

## 6. SD Card Logging

- microSD socket (Hirose DM3D-SF) on SDMMC1, 4-bit, 50 MHz.
- Raw ADC blocks (16-bit) AND post-demod audio (24-bit) written in a
  tagged record format so post-engagement analysis can re-run the DSP.
- FAT32-friendly 512-byte sector alignment in `sdlog.c`.

— *jayis1*