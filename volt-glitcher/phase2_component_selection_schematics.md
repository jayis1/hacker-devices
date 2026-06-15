# Phase 2: Component Selection & Schematics — VoltGlitcher Pro

## Detailed Component Justification and Circuit Design

**Document Version:** 1.0  
**Date:** 2026-01-15  
**Author:** Hacker Devices  
**Status:** Final

---

## 1. MCU Selection: STM32F407VGT6

### 1.1 Selection Criteria

| Requirement | STM32F407VGT6 | Alternative: RP2040 | Alternative: SAMD51 |
|-------------|---------------|--------------------|--------------------|
| Core speed | 168 MHz | 133 MHz | 120 MHz |
| USB | OTG FS (12 Mbps) | USB 1.1 (device) | USB 2.0 FS |
| ADC | 12-bit, 3 MSPS | 12-bit, 500 KSPS | 12-bit, 1 MSPS |
| DMA | 2× 16-stream | 4 ch | 32 ch |
| I2C | 3× (400 kHz) | 2× | 6× |
| SPI | 3× (42 MHz) | 2× | 12× |
| Flash | 1 MB | 16 KB (ext flash) | 512 KB |
| RAM | 192 KB | 264 KB | 192 KB |
| Package | LQFP-100 | QFN-56 | TQFP-100/64 |

**Decision:** STM32F407VGT6 chosen for:
- Best-in-class ADC speed (3 MSPS) for monitoring glitch transients
- Full-speed USB OTG with VBUS sensing
- 3 independent SPI peripherals (FPGA, SD card, flash)
- Large flash for firmware complexity
- Well-understood toolchain (arm-none-eabi-gcc, OpenOCD)

### 1.2 Pin Assignment Summary

**Power pins:**
- VDD: 3.3V on pins 11, 19, 28, 50, 75, 100
- VSS: GND on pins 10, 18, 27, 49, 74, 99
- VDDA: 3.3V via LC filter on pin 22
- VSSA: GND via star point on pin 21
- VBAT: 3.3V with 100 µF backup cap on pin 6

**Critical path pins (glitch timing):**
- PC6: Q1 gate drive output (must be very high speed, 120 MHz I/O toggle)
- PC7: Gate driver enable (must toggle in <10 ns with PC6)
- PC9: Q2 EM gate (high speed)
- PB14: Q3 clock gate (high speed, inverted logic for P-ch)

**ADC pins:**
- PA0: ADC1_IN0 — Target VCC sense (voltage divider 10k/10k)
- PA5: ADC1_IN5 — Glitch output voltage (direct)
- PA6: ADC1_IN6 — Shunt current (INA210 output)
- PA7: ADC1_IN7 — EM coil voltage

**FPGA interface (SPI1):**
- PA4: SPI1_NSS (manual CS, not hardware — allows byte-level control)
- PA5: SPI1_SCK
- PB4: SPI1_MISO (shared with JTAG TDO — multiplexed)
- PB5: SPI1_MOSI

---

## 2. FPGA Selection: Lattice iCE40HX1K-TQ144

### 2.1 Selection Criteria

| Requirement | iCE40HX1K | Gowin GW1N-4 | ECP5-12K |
|-------------|-----------|-------------|----------|
| Logic cells | 1280 LCs | 5296 LUTs | 12960 LUTs |
| BRAM | 64 Kb | 72 Kb | 576 Kb |
| PLLs | 2 | 2 | 2 |
| Max freq | 273 MHz | 370 MHz | 383 MHz |
| Core voltage | 1.2V | 1.2V | 1.1V |
| Package | TQFP-144 | QFN-48/56 | TQFP-144 |
| Config interface | SPI | SPI | SPI/JTAG |
| Toolchain | Fully open (Yosys/nextpnr) | Proprietary | Partially open |
| Cost | ~$5 | ~$3 | ~$10 |

**Decision:** iCE40HX1K chosen for:
- Fully open-source toolchain (Yosys + nextpnr-ice40 = no vendor lock-in)
- Sufficient logic for glitch timing + pattern match + waveform gen
- 2 PLLs for independent clock domains
- Well-documented configuration protocol
- TQFP-144 package for easy hand-soldering

### 2.2 FPGA Resource Budget

| Module | LCs Used | BRAM Used | PLLs | Notes |
|--------|---------|-----------|------|-------|
| SPI register interface | 45 | 0 | 0 | Command parser + register file |
| Delay chain (2 channels) | 120 | 0 | 0 | Carry-chain based, 50 ps resolution |
| UART pattern matcher | 80 | 0 | 0 | 4-byte shift register comparator |
| JTAG TAP tracker | 60 | 0 | 0 | IEEE 1149.1 state machine |
| Waveform generator | 150 | 32 Kb | 0 | BRAM playback + DAC interface |
| Glitch output controller | 40 | 0 | 0 | MUX for VCC/EM/CLK outputs |
| PLL reconfig | 0 | 0 | 1 | Dynamic frequency adjustment |
| Temperature sensor IP | 30 | 0 | 0 | Ring-oscillator based |
| Fan PWM controller | 20 | 0 | 0 | 25 kHz PWM |
| **Total** | **545 / 1280** | **32 / 64 Kb** | **1 / 2** | **42% LC utilization** |

### 2.3 FPGA Pin Assignment

**Configuration pins:**
- Pin 1: CRESET_B (active-low config reset)
- Pin 2: CDONE (config done output)
- Pin 3-6: SPI config interface (SCK, SI, SO, SS_B)

**I/O Bank 0 (VCC_IO = 3.3V) — Glitch outputs:**
- IOB_0a (pin 23): → UCC27511 IN (VCC shunt gate drive)
- IOB_0b (pin 24): → Q2 gate (EM coil driver)
- IOB_1a (pin 25): → Q3 gate (clock glitch)
- IOB_1b (pin 26): → DAC_CS (EM amplitude control, external DAC)

**I/O Bank 1 (VCC_IO = 3.3V) — Trigger inputs:**
- IOT_37 (pin 67): ← TRIG0 (GPIO, from opto-isolator)
- IOT_38 (pin 68): ← TRIG1 (UART RX from target)
- IOT_39 (pin 69): ← JTAG TCK
- IOT_40 (pin 70): ← JTAG TMS
- IOT_41 (pin 71): ← JTAG TDI
- IOT_42 (pin 72): ← JTAG TDO

**I/O Bank 2 (VCC_IO = 3.3V) — MCU interface:**
- IOL_2a (pin 93): ↔ FPGA_INT (FPGA→MCU interrupt)
- IOL_2b (pin 94): ← FPGA_WAKE (MCU→FPGA)
- IOL_3a (pin 95): ← FPGA_SUSPEND (MCU→FPGA)

**PLL pins:**
- PLL_CLKIN (pin 15): ← 12 MHz external oscillator or target clock
- PLL_CLKOUT (pin 16): → internal (glitch timing reference)

---

## 3. Glitch Output Stage Components

### 3.1 VCC Shunt MOSFET: SiS426DN (Q1)

**Key specifications:**
- Type: N-channel enhancement mode
- Vds: 20V
- Id: 13A (continuous)
- Rds(on): 6 mΩ @ Vgs = 4.5V
- Vgs(th): 1.0V (typ), 1.4V (max)
- Qg: 12 nC (total gate charge)
- Package: SuperSOT-6 (Vishay)

**Selection rationale:**
- Ultra-low Rds(on) minimizes voltage drop during shunt
- 13A continuous current handles target supply transients
- Low gate charge (12 nC) enables fast switching
- SuperSOT-6 package has excellent thermal characteristics (RθJA = 40°C/W)

**Thermal analysis:**
- Worst case: Target at 3.3V, MOSFET ON for 100 ns at 10A
- Energy per glitch: 3.3V × 10A × 100 ns = 3.3 µJ
- At 1000 glitches/second: 3.3 mW average → negligible heating

**Gate resistor selection:**
- Rg = 2.2 Ω between gate driver and MOSFET gate
- Limits gate current to ~2A (from 4.5V gate drive)
- dV/dt at gate limited to prevent Miller plateau oscillation

### 3.2 Gate Driver: UCC27511DBVR (U6)

**Key specifications:**
- Type: Low-side, single-channel gate driver
- Peak source current: 4A
- Peak sink current: 6A
- Propagation delay: 30 ns (typ)
- Rise time: 15 ns (into 1.8 nF load)
- Input threshold: CMOS/TTL compatible
- Under-voltage lockout (UVLO): 3.5V
- Package: SOT-23-6 (DBV)

**Selection rationale:**
- 4A peak drive current ensures fast MOSFET turn-on
- 30 ns propagation delay is acceptable for glitch timing
- Built-in UVLO prevents partial gate drive
- Fault output detects MOSFET desaturation

**Circuit details:**
- IN: Connected to FPGA IOB_0a and MCU PC6 (OR-tied)
- EN: Connected to MCU PC7 (software enable)
- FAULT: Connected to MCU PC8 (interrupt on fault)
- VDD: 5.0V from USB VBUS (via 100 nF + 1 µF decoupling)
- OUT: Connected to Q1 gate via 2.2 Ω gate resistor

### 3.3 EM Coil MOSFET: IRLML6344TRPBF (Q2)

**Key specifications:**
- Type: N-channel enhancement mode
- Vds: 30V
- Id: 1.2A (continuous), 5A (pulsed)
- Rds(on): 60 mΩ @ Vgs = 2.5V
- Vgs(th): 1.0V (typ)
- Qg: 4.4 nC
- Package: SOT-23

**Selection rationale:**
- Logic-level gate drive (2.5V Vgs sufficient)
- Small SOT-23 package for compact layout
- 5A pulsed current sufficient for EM coil
- Low gate charge for fast switching

**Coil specifications:**
- Wire: 0.5mm enameled copper (AWG 24)
- Turns: 15 (typical for 3mm coil)
- Inductance: ~200 nH
- Resistance: ~0.5 Ω
- Peak current at 5V: ~2A (limited by MOSFET Rds + coil R)

### 3.4 Clock Glitch MOSFET: BSS84P (Q3)

**Key specifications:**
- Type: P-channel enhancement mode
- Vds: -50V
- Id: -0.13A (continuous)
- Rds(on): 10 Ω @ Vgs = -4.5V
- Vgs(th): -1.0V (typ), -2.0V (max)
- Package: SOT-23

**Selection rationale:**
- P-channel for clock stretching (pulls clock low)
- 10 Ω Rds(on) provides moderate clock stretching
- Not too low Rds(on) — we want to stretch, not short
- SOT-23 for minimal board area

**Clock glitch circuit design:**
```
TARGET_CLK ──── 100Ω series ──── TARGET_CLK_OUT
                    │
                    └── Q3 drain (BSS84P)
                         source → 3.3V pull-up
                         gate → FPGA (active-low assertion)
```

When Q3 gate is driven low (P-ch ON), the clock line is pulled toward 3.3V through Q3's Rds(on), effectively stretching the clock high period. The 100Ω series resistor limits current and provides impedance matching.

---

## 4. Current Sensing: INA210NA (U7)

### 4.1 Specifications

- Gain: 500 V/V (fixed)
- Shunt resistor: 0.1 mΩ (internal to evaluation, external on board)
- Bandwidth: 30 kHz
- CMRR: 110 dB
- Offset voltage: 35 µV (max)
- Supply current: 260 µA
- Package: DFN-8 (2×2 mm)

### 4.2 Circuit Design

```
VCC_TARGET ─── R_shunt(0.1mΩ) ─── VCC_TO_MCU
    │                                   │
    └── INA210 IN+              INA210 IN- ──┘
                  │
                  OUT → ADC1_IN6 (PA6)
                  REF → 1.25V reference
```

**Measurement range:**
- Vref = 1.25V (internal bandgap reference)
- Vout = Vref + (Ishunt × Rshunt × Gain)
- Vout = 1.25 + (Ishunt × 0.0001 × 500)
- Vout = 1.25 + (Ishunt × 0.05)
- At Ishunt = 5A: Vout = 1.25 + 0.25 = 1.50V → ADC = 1.50V × 4095/3.3 = 1861
- At Ishunt = -5A: Vout = 1.25 - 0.25 = 1.00V → ADC = 1241
- Resolution: 5A / (1861-1241) ≈ 8 mA per ADC count

---

## 5. Voltage Regulation

### 5.1 U2: TLV62569DBVR — 5V → 1.2V Buck Converter

**Design parameters:**
- Input: 4.5-5.5V (USB VBUS)
- Output: 1.2V / 3A
- Switching frequency: 1.5 MHz
- Inductor: 1.5 µH / 5A (e.g., Würth 744309015)
- Output caps: 22 µF X5R 0805 × 2

**Efficiency:** ~90% at 1.2V/2A load

### 5.2 U3: TLV62569DBVR — 5V → 3.3V Buck Converter

**Design parameters:**
- Input: 4.5-5.5V
- Output: 3.3V / 3A
- Same component values as U2 (different feedback divider)
- Feedback: R1=100kΩ (top), R2=43.2kΩ (bottom) → Vout = 1.212V × (1 + 100/43.2) ≈ 3.3V

### 5.3 U4/U5: TPS7A7001DDAR — LDO Regulators

**U4 (1.8V) design:**
- Input: 3.3V from U3
- Output: 1.8V / 2A
- Dropout: 300 mV at 2A
- Noise: 3.5 µVrms (10Hz-100kHz)

**U5 (2.5V) design:**
- Input: 3.3V from U3
- Output: 2.5V / 2A
- Dropout: 300 mV at 2A
- Used for FPGA PLL supply (low-noise critical)

---

## 6. Opto-Isolator: TLP291-4 (U8)

**Specifications:**
- 4-channel opto-isolator
- CTR: 50-300% (at IF = 5mA)
- Isolation voltage: 3750 Vrms
- Bandwidth: DC (no frequency limitation for digital signals)
- Package: TSSOP-16

**Design for TRIG0:**
- Input side: Target GPIO → 1kΩ resistor → LED anode → LED cathode → GND_target
- Output side: Phototransistor collector → 10kΩ pull-up → 3.3V, emitter → GND_glitcher
- Signal inversion: Yes (active-low output → use Schmitt trigger inverter if needed)

---

## 7. EEPROM: CAT24C256WI-GT3 (U9)

**Specifications:**
- Density: 256 Kbit (32 KB)
- Interface: I2C, 400 kHz Fast Mode
- Page size: 64 bytes
- Write cycle: 5 ms (byte), 5 ms (page)
- Endurance: 1,000,000 write cycles
- Data retention: 100 years
- Package: SOIC-8

**Memory map:**
| Offset | Size | Usage |
|--------|------|-------|
| 0x0000 | 1024 | 16 profiles × 64 bytes each |
| 0x0400 | 16 | Device settings structure |
| 0x0410 | 32 | Calibration data |
| 0x0430 | 64 | FPGA bitstream hash |
| 0x0470 | 32144 | Reserved / waveform cache |

---

## 8. Voltage/Current Monitor: INA219AIDCNR (U10)

**Purpose:** Continuous monitoring of target supply voltage and current, independent of MCU ADC. Provides digital readings via I2C for higher accuracy.

**Specifications:**
- 16-bit ADC
- Programmable gain: ±40mV to ±320mV full-scale
- Shunt voltage accuracy: ±0.5%
- Bus voltage accuracy: ±0.5%
- I2C interface: 400 kHz
- Package: SOT-23-6

**Configuration:**
- Shunt resistor: 0.1 Ω (100 mΩ) on target VCC line
- PGA gain: ×8 (±320mV range)
- Bus voltage range: 0-26V
- Resolution: 0.1 mA (at 0.1Ω shunt, ×8 gain)

---

## 9. USB Connector & ESD Protection

### 9.1 USB Micro-B: Molex 105017-0001

**Features:**
- USB 2.0 Micro-B receptacle
- SMT, right-angle
- Shield connected to PCB ground plane
- VBUS pin for power sensing

### 9.2 ESD Protection: PRTR5V0U2X (D5)

- Bi-directional TVS diode array
- Protects D+ and D- lines
- Clamping voltage: 6V
- Capacitance: 1 pF (USB signal integrity)

---

## 10. Crystals & Clock Generation

### 10.1 Y1: 8 MHz HSE Crystal

- Package: 5032 (5.0 × 3.2 mm)
- Frequency: 8.000 MHz
- Tolerance: ±30 ppm
- Load capacitance: 20 pF (C1 = C2 = 33 pF with stray ~13 pF)
- ESR: <60 Ω
- Drive level: 500 µW

### 10.2 Y2: 32.768 kHz LSE Crystal

- Package: 3215 (3.2 × 1.5 mm)
- Frequency: 32.768 kHz
- Tolerance: ±20 ppm
- Load capacitance: 12.5 pF (C3 = C4 = 22 pF)
- ESR: <50 kΩ

### 10.3 FPGA Clock: 12 MHz Oscillator

- ECS-120-8-36CKM-TR
- 3.3V CMOS output
- Frequency stability: ±50 ppm
- Used as PLL reference for glitch timing

---

## 11. SD Card Interface

- Connector: Hirose DM3BT-DSF-PEJS (microSD)
- Interface: SPI2 (SDIO mode not used — fewer pins)
- CS: PB12, SCK: PB13, MISO: PB14 (shared with clock glitch!), MOSI: PB15 (shared!)
- Card detect: PC4 (active-low)
- Note: SPI2 pins are multiplexed with clock glitch output. Clock glitch must be disabled when accessing SD card.

---

## 12. Bill of Materials Summary

| Ref | Part | Qty | Package | Unit Cost |
|-----|------|-----|---------|-----------|
| U1 | STM32F407VGT6 | 1 | LQFP-100 | $8.50 |
| U11 | iCE40HX1K-TQ144 | 1 | TQFP-144 | $5.20 |
| U2 | TLV62569DBVR | 1 | SOT-23-6 | $0.85 |
| U3 | TLV62569DBVR | 1 | SOT-23-6 | $0.85 |
| U4 | TPS7A7001DDAR | 1 | DDPAK-7 | $2.30 |
| U5 | TPS7A7001DDAR | 1 | DDPAK-7 | $2.30 |
| U6 | UCC27511DBVR | 1 | SOT-23-6 | $0.95 |
| U7 | INA210NA | 1 | DFN-8 | $1.80 |
| U8 | TLP291-4 | 1 | TSSOP-16 | $1.20 |
| U9 | CAT24C256WI-GT3 | 1 | SOIC-8 | $0.65 |
| U10 | INA219AIDCNR | 1 | SOT-23-6 | $1.50 |
| Q1 | SiS426DN | 1 | SuperSOT-6 | $0.90 |
| Q2 | IRLML6344TRPBF | 1 | SOT-23 | $0.25 |
| Q3 | BSS84P | 1 | SOT-23 | $0.15 |
| Y1 | 8 MHz crystal | 1 | 5032 | $0.30 |
| Y2 | 32.768 kHz crystal | 1 | 3215 | $0.25 |
| J1 | USB Micro-B | 1 | SMT | $0.45 |
| J6 | microSD socket | 1 | SMT | $0.80 |
| — | Passives (R/C/L) | ~120 | 0201-0805 | ~$3.00 |
| — | PCB (4-layer) | 1 | 120×80mm | $5.00 |
| | **Total** | | | **~$37.00** |

---

## 13. References

1. SiS426DN datasheet: https://www.vishay.com/docs/68854/sis426dn.pdf
2. UCC27511 datasheet: https://www.ti.com/lit/ds/symlink/ucc27511.pdf
3. INA210 datasheet: https://www.ti.com/lit/ds/symlink/ina210.pdf
4. iCE40HX1K datasheet: https://www.latticesemi.com/view-document?document_id=40955
5. STM32F407 datasheet: https://www.st.com/resource/en/datasheet/stm32f407vg.pdf
6. TLV62569 datasheet: https://www.ti.com/lit/ds/symlink/tlv62569.pdf
7. TPS7A7001 datasheet: https://www.ti.com/lit/ds/symlink/tps7a7001.pdf