# TeslaPhantom — Electromagnetic Fault Injection & Magnetic Side-Channel Analysis Platform

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **Author: jayis1** · SPDX-License-Identifier: GPL-2.0

```
┌──────────────────────────────────────────────────────────────────────┐
│                       TESLA-PHANTOM                                  │
│                                                                      │
│   ┌─────────┐    ┌──────────┐    ┌───────────┐    ┌────────────┐    │
│   │ HV Pulse│───►│ EM Probe │───►│  Target   │───►│ Fluxgate   │    │
│   │ Marx Bd │    │  Coil    │    │   IC      │    │ Magnetometer│   │
│   └────┬────┘    └──────────┘    └───────────┘    └──────┬─────┘    │
│        ▲              ▲                    ▲              │          │
│        │              │                    │              ▼          │
│   ┌────┴────┐   ┌─────┴──────┐    ┌───────┴──────┐  ┌──────────┐   │
│   │ iCE40   │   │ 3-Axis     │    │ Trigger SMA  │  │ AD7606   │   │
│   │ FPGA    │   │ Stepper XYZ│    │ (Ext / Int)  │  │ 16-bit   │   │
│   │ Timing  │   │ Positioning│    │              │  │ ADC      │   │
│   └────┬────┘   └────────────┘    └──────────────┘  └────┬─────┘   │
│        │               ▲                               │          │
│        └───────────────┴───────────────────────────────┘          │
│                              STM32H743                              │
│                      (480 MHz Cortex-M7 MCU)                       │
└──────────────────────────────────────────────────────────────────────┘
```

## 1. Overview

**TeslaPhantom** is a dual-mode security research instrument that combines
**Electromagnetic Fault Injection (EMFI)** with **passive Magnetic
Side-Channel Analysis (M-SCA)** and **automated 3-axis probe positioning**
into a single portable platform. It is designed for red teams, security
researchers, and penetration testers who need to evaluate the resilience
of cryptographic hardware (smart cards, secure elements, microcontrollers,
HSMs, IoT chips) against electromagnetic fault attacks and magnetic
emanation side-channel leakage.

### What makes it novel

Existing fault injection tools (voltage glitchers, clock glitchers, laser
FI) require either direct electrical contact with the target's power/clock
lines or expensive optical setups. EMFI is a **contactless** technique: a
high-voltage pulse is discharged through an inductive coil positioned near
the target IC's die, generating a localized magnetic field spike that
induces transient fault currents in the silicon. This bypasses decoupling
capacitor filtering and does not require modifying the target's PCB.

TeslaPhantom is the first open design to combine:

1. **Active EMFI** — A Marx-bank high-voltage pulse generator (~400 V)
   driving a miniature probe coil, with FPGA-controlled pulse timing
   (sub-nanosecond jitter, adjustable pulse width 5–200 ns).
2. **Passive M-SCA** — An integrated fluxgate magnetometer (DRV425) and
   16-bit ADC (AD7606) capture magnetic emanations from the target during
   cryptographic operations, enabling correlation power analysis in the
   magnetic domain.
3. **Automated 3-axis positioning** — Three NEMA-8 stepper motors with
   TMC2209 drivers move the probe coil in X/Y/Z with 20 µm resolution,
   enabling automated **fault injection cartography** — scanning the entire
   die surface to map fault-sensitive regions.
4. **Dual trigger architecture** — The iCE40 FPGA can trigger pulses from
   an external SMA signal (target's GPIO toggle at start of crypto
   operation), from an internal timer with programmable delay, or from a
   pattern-matched magnetic signature detected in real time.

## 2. Attack Surface & Threat Model

### Attack surface

| Surface | Method | Range | Target layer |
|---------|--------|-------|--------------|
| IC die (contactless) | EMFI pulse | 0.1–5 mm from die | Silicon substrate |
| Magnetic emanation | Passive M-SCA | 1–50 mm | Power/ground loop currents |
| Crypto timing | Trigger correlation | Wired (SMA) | Target firmware |
| Position-scan mapping | 3-axis cartography | 20 µm steps | Full die area |

### Threat model

TeslaPhantom models an adversary with **physical proximity** to the target
device (within touching distance) but **no direct electrical contact** with
the target's power, clock, or debug lines. This is a realistic threat
model for:

- **Smart cards / payment cards**: Attacker holds the card, positions
  probe over the chip, injects faults during PIN verification or key
  generation.
- **Secure elements in IoT devices**: Attacker opens the device enclosure,
  positions probe over the secure element, performs fault injection to
  bypass secure boot or extract keys.
- **Automotive ECUs**: Attacker accesses the ECU board, uses EMFI to
  bypass immobilizer authentication or extract firmware from a locked MCU.
- **HSMs / TPMs**: Attacker with facility access probes the secure
  processor to induce faults in RSA/ECC computations for fault-attack
  key recovery (e.g., Bellcore attack on RSA-CRT).

### What it is NOT

TeslaPhantom is **not** a remote attack tool. It requires physical
proximity and, for best results, visual access to the target IC. It does
not exploit software vulnerabilities. It is a **hardware-level evaluation
tool** for assessing cryptographic implementations against physical
attacks.

## 3. Hardware Specifications

### MCU & FPGA

| Component | Part | Role |
|-----------|------|------|
| Main MCU | STM32H743VIT6 (Cortex-M7, 480 MHz, 2 MB Flash, 1 MB SRAM) | System control, state machine, BLE, USB, SD, UI |
| Timing FPGA | Lattice iCE40UP5K-SG48 | Pulse timing generation, trigger logic, ADC sample scheduling |
| External SRAM | IS66WV51216BLL-55BLI (1 MB, 55 ns) | Capture buffer for long SCA traces |

### High-voltage pulse generator

| Parameter | Value |
|-----------|-------|
| Topology | 4-stage Marx bank (ceramic capacitors, avalanche BJTs) |
| Max voltage | 400 V (adjustable 50–400 V via DAC-controlled charge pump) |
| Pulse width | 5–200 ns (FPGA-tuned, 1 ns resolution) |
| Rise time | < 2 ns |
| Pulse energy | 0.1–8 mJ (adjustable) |
| Repetition rate | Single shot to 10 Hz (thermal limited) |
| Probe coil | Hand-wound 5-turn, 3 mm diameter, #34 AWG enameled copper |

### Magnetic side-channel sensor

| Component | Part | Specs |
|-----------|------|-------|
| Fluxgate magnetometer | Texas Instruments DRV425 | ±1.3 mT range, 80 nT/√Hz noise, 47 kHz bandwidth |
| Signal conditioning | AD8331 VGA (variable gain amplifier) | 0–48 dB gain, SPI-controlled |
| Anti-alias filter | LTC1564-7 programmable LPF | 8th-order, 1 kHz–150 kHz cutoff |
| ADC | Analog Devices AD7606C-6 | 16-bit, 6 channels, 800 kSPS simultaneous sampling |

### 3-axis positioning system

| Axis | Motor | Driver | Resolution | Travel |
|------|-------|--------|------------|--------|
| X (horizontal) | NEMA-8 stepper | TMC2209 (UART) | 20 µm (16× microstep) | 50 mm |
| Y (horizontal) | NEMA-8 stepper | TMC2209 (UART) | 20 µm | 50 mm |
| Z (vertical) | NEMA-8 stepper | TMC2209 (UART) | 20 µm | 20 mm |
| Limit switches | Optical end-stops (TCST2103) | GPIO EXTI | — | Per axis |

### Connectivity & storage

| Interface | Component | Purpose |
|-----------|-----------|---------|
| BLE 5.0 | u-blox NINA-B306 | Wireless control & data streaming |
| USB 2.0 FS | STM32 USB-CDC | Serial console, firmware updates, data download |
| microSD | SPI microSD socket | Trace logging, scan session persistence |
| OLED | SSD1306 128×64 I²C | Status display (armed, position, voltage, battery) |

### Power

| Rail | Source | Voltage | Load |
|------|--------|---------|------|
| VUSB | USB-C 5V | 5.0 V | Charging + system |
| VBAT | Li-Po 1800 mAh | 3.7 V nominal | Backup / portable operation |
| +3V3 | TPS62843 buck | 3.3 V | MCU, BLE, OLED, sensors |
| +1V8 | TPS7A47 LDO | 1.8 V | FPGA core, ADC VDRIVE |
| +5V | TPS61070 boost | 5.0 V | ADC analog, VGA |
| HV | Custom charge pump | 50–400 V | Marx bank charge |

### Form factor

- **PCB**: 4-layer, 80 mm × 55 mm (main board) + 30 mm × 20 mm (probe head daughterboard)
- **Enclosure**: Custom 3D-printed PLA enclosure, 95 × 70 × 35 mm
- **Probe arm**: Carbon fiber tube, 60 mm length, mounting 3-axis gantry
- **Weight**: ~180 g (including battery)

## 4. Architecture & Block Diagram

### System architecture

```
                         ┌─────────────────────────────────────────┐
                         │            STM32H743 (MCU)              │
                         │                                         │
   BLE ──UART3──►       │  ┌──────────┐  ┌──────────────────┐    │
   (NINA-B306)          │  │ State    │  │ Scan Controller  │    │
                         │  │ Machine  │──│ (X/Y/Z sweep,    │    │
   USB ──USB-FS──►      │  │ (FSM)    │  │  cartography)    │    │
   (CDC)                 │  └────┬─────┘  └────────┬─────────┘    │
                         │       │                 │              │
   OLED ──I2C1──►        │  ┌────▼─────┐  ┌────────▼─────────┐    │
   (SSD1306)             │  │ Command  │  │ Positioning       │    │
                         │  │ Parser   │  │ Controller        │    │
   microSD ──SPI4──►     │  │ (BLE/USB)│  │ (TMC2209 UART)    │    │
                         │  └────┬─────┘  └────────┬─────────┘    │
   Trigger ──EXTI──►     │       │                 │              │
   (SMA)                 │  ┌────▼─────┐  ┌────────▼─────────┐    │
                         │  │ HV       │  │ SCA Capture       │    │
                         │  │ Controller│  │ Engine            │    │
                         │  │ (DAC→    │  │ (ADC DMA→SRAM→SD) │    │
                         │  │  charge) │  │                   │    │
                         │  └────┬─────┘  └────────┬─────────┘    │
                         └───────┼─────────────────┼──────────────┘
                                 │                 │
                    ┌────────────▼───┐    ┌────────▼────────┐
                    │   iCE40 FPGA   │    │   AD7606 ADC    │
                    │                │    │   (16-bit,      │
                    │ • Pulse timing │    │    800 kSPS)    │
                    │ • Trigger logic│    └────────┬────────┘
                    │ • ADC sched    │             │
                    │ • Marx trigger │    ┌────────▼────────┐
                    └───────┬────────┘    │  AD8331 VGA     │
                            │             │  + LTC1564 LPF  │
                   ┌────────▼──────┐      └────────┬────────┘
                   │  Marx Bank    │               │
                   │  (4-stage,    │      ┌────────▼────────┐
                   │   400V max)   │      │  DRV425         │
                   └────────┬──────┘      │  Fluxgate Mag   │
                            │             └─────────────────┘
                   ┌────────▼──────┐
                   │  EM Probe     │
                   │  Coil (3mm)   │
                   └───────────────┘
```

### Signal flow — EMFI mode

1. MCU commands FPGA with pulse parameters (width, delay, count).
2. MCU sets HV charge voltage via DAC → charge pump → Marx bank charges.
3. Trigger arrives (external SMA, internal timer, or magnetic signature).
4. FPGA fires Marx bank trigger → avalanche BJTs conduct → capacitors
   discharge in series through probe coil → ~400 V transient → magnetic
   field spike → induces fault current in target IC.
5. MCU waits for target response (via trigger line or BLE-reported
   status), logs result to SD.

### Signal flow — M-SCA mode

1. MCU commands ADC to capture N samples at 800 kSPS.
2. DRV425 fluxgate sensor picks up magnetic emanation from target.
3. VGA amplifies (auto-ranged), LPF anti-aliases, ADC digitizes.
4. DMA streams samples to external SRAM.
5. MCU processes trace (optional on-device DSP: bandpass, envelope,
   correlation) and logs to SD or streams over BLE/USB.

### Signal flow — Cartography scan

1. MCU moves probe to (x₀, y₀) via stepper motors.
2. At each grid point, executes configured EMFI pulse sequence.
3. Records target response (fault/no-fault/crash) at each point.
4. Moves to next grid point, repeats until scan area covered.
5. Generates 2D fault sensitivity map → exports to companion app.

## 5. Firmware Design & Decisions

### Target platform

- **MCU**: STM32H743VIT6 (Cortex-M7 @ 480 MHz, LQFP-100)
- **FPGA**: Lattice iCE40UP5K (SG48, 5280 LUTs)
- **Toolchain**: arm-none-eabi-gcc 13.x, bare-metal, no HAL — direct
  register access for deterministic timing.
- **FPGA toolchain**: Project IceStorm (open-source) / yosys + nextpnr.

### State machine

```
        ┌──────┐
        │ BOOT │
        └──┬───┘
           │ init all peripherals
        ┌──▼───┐
┌───────│ IDLE │◄──────────────────────────┐
│       └──┬───┘                            │
│          │ arm command                    │
│       ┌──▼───┐                            │
│  ┌───►│ ARMED│                            │
│  │    └──┬───┘                            │
│  │       │ trigger                        │
│  │    ┌──▼─────┐    pulse done     ┌────┐│
│  │    │ PULSE  │──────────────────►│LOG ││
│  │    └────────┘                   └──┬─┘│
│  │       │ fault detected?            │  │
│  └───────┤ yes (retry/adjust)         │  │
│          │ no                          │  │
│       ┌──▼──────┐                     │  │
│       │ ANALYZE │─────────────────────┘  │
│       └─────────┘                        │
│                                         │
│  SCA mode: IDLE → ARMED → CAPTURE → LOG → IDLE
│  Scan mode: IDLE → SCAN (loop: MOVE→PULSE→LOG) → IDLE
```

### Key design decisions

1. **Bare-metal, no HAL**: The STM32 HAL adds unpredictable latency.
   Fault injection requires deterministic timing (jitter < 1 ns is handled
   by the FPGA, but the MCU must not introduce multi-µs stalls). Direct
   register access ensures the MCU responds within a known worst-case
   bound.

2. **FPGA offloads all timing-critical paths**: The MCU is too slow and
   non-deterministic for sub-nanosecond pulse timing. The iCE40 FPGA
   generates the actual Marx trigger signal with a programmable delay
   (relative to the trigger source) and pulse width, using a 200 MHz
   internal clock (5 ns resolution) with a delay-line interpolator for
   sub-clock resolution.

3. **External SRAM for capture buffer**: The AD7606 at 800 kSPS × 16-bit
   × 6 channels generates 9.6 MB/s. The STM32H743's internal SRAM (1 MB)
   cannot hold a meaningful trace. External 1 MB SRAM via FMC provides
   ~130 ms capture windows; longer traces are streamed to SD.

4. **Marx bank over spark gap**: A Marx bank using avalanche BJTs
   (2N5551) is chosen over a spark gap because it produces faster rise
   times (< 2 ns vs. > 10 ns), is voltage-controllable (not dependent on
   gap distance), and is more repeatable. The trade-off is lower peak
   voltage (400 V vs. kV for spark gaps), which is sufficient for
   short-range EMFI at 0.5–2 mm.

5. **TMC2209 with StallGuard4**: The stepper drivers support
   sensorless homing (StallGuard4), eliminating the need for limit
   switches on the low-force NEMA-8 motors. However, optical end-stops
   are included as a fallback for reliable homing.

6. **Dual trigger architecture**: Supporting both external (SMA from
   target) and internal (timer) triggers, plus a novel **magnetic
   signature trigger** (FPGA pattern-matches on ADC data), allows
   attacking targets where no trigger signal is available.

### File structure

```
firmware/
├── Makefile              # Build system (arm-none-eabi-gcc)
├── linker.ld             # Linker script for STM32H743
├── board.h               # Board-level definitions, pin assignments, types
├── registers.h           # STM32H7 register definitions (GPIO, SPI, etc.)
├── main.c                # Main state machine and application logic
└── drivers/
    ├── board_init.c      # Clock, GPIO, FMC, QSPI, EXTI initialization
    ├── fpga_spi.c        # SPI communication with iCE40 FPGA
    ├── hv_pulse.c        # High-voltage charge pump & Marx bank control
    ├── stepper.c         # 3-axis TMC2209 stepper motor control
    ├── fluxgate.c        # DRV425 fluxgate magnetometer reading
    ├── adc_capture.c     # AD7606 ADC configuration and DMA capture
    ├── sca_engine.c      # Side-channel analysis trace processing
    ├── trigger.c         # Trigger routing (ext/int/magnetic signature)
    ├── scan_controller.c # Automated cartography scan logic
    ├── ble_uart.c        # BLE module communication (NINA-B306)
    ├── usb_cdc.c         # USB CDC serial interface
    ├── oled.c            # SSD1306 OLED display driver
    ├── sdcard.c          # microSD card logging
    └── config.c          # Persistent configuration storage
```

## 6. Application / Software Interface

### Companion app (React Native)

A React Native companion app provides wireless control over BLE:

| Screen | Function |
|--------|----------|
| **Dashboard** | Device status (state, battery, HV voltage, probe position, temperature) |
| **EMFI Control** | Set pulse voltage, width, delay, count; arm/disarm; single shot |
| **SCA Capture** | Configure sample count, rate, gain; capture and view magnetic trace |
| **Scan Setup** | Define grid area, step size, pulse params per point; start/stop scan |
| **Scan Map** | 2D heat map of fault sensitivity across scanned die area |
| **Trace Analysis** | View captured SCA traces, apply filters, compute correlation |
| **Settings** | BLE pairing, trigger source, safety interlocks, firmware update |

### BLE protocol

Custom GATT service (UUID `0000TES0-0000-1000-8000-00805F9B34FB`):

| Characteristic | UUID suffix | Direction | Purpose |
|----------------|-------------|-----------|---------|
| Command | `0001` | Phone→Device | JSON command string |
| Status | `0002` | Device→Phone | JSON status (notify) |
| Trace Data | `0003` | Device→Phone | Binary SCA trace chunks (notify) |
| Scan Map | `0004` | Device→Phone | Binary fault map data (notify) |

Commands are JSON strings, e.g.:
```json
{"cmd":"emfi_pulse","voltage":300,"width_ns":50,"delay_ns":1000,"count":1}
{"cmd":"sca_capture","samples":16384,"rate_khz":800,"gain_db":30}
{"cmd":"scan_start","x0":0,"y0":0,"x1":10,"y1":10,"step_mm":0.5}
{"cmd":"move","x":5.0,"y":3.0,"z":0.5,"speed":"slow"}
```

### USB CDC interface

When connected via USB, the device exposes a virtual serial port
(115200 8N1) with a text command interface. Same JSON commands are
accepted, plus binary trace download mode (XMODEM).

## 7. Use Cases

### Red team operations

- **Secure boot bypass**: Position probe over target MCU's boot ROM area,
  inject fault during signature verification check → bypass secure boot →
  boot unsigned firmware.
- **Key extraction via RSA-CRT fault**: Inject fault during RSA-CRT
  computation → one faulty signature → gcd(faulty, correct) reveals
  prime factor → private key recovered (Bellcore attack).
- **PIN bypass**: Inject fault during PIN comparison loop → skip branch →
  access secure element without valid PIN.
- **Firmware dump from locked MCU**: Fault the read-protection check on
  STM32/ESP32 → dump flash via SWD/JTAG or bootloader.

### Security research

- **Fault sensitivity mapping**: Use cartography scan to map which die
  regions are most susceptible to EMFI — useful for IC designers
  evaluating hardening countermeasures (duplication, error detection
  codes, sensors).
- **Magnetic SCA evaluation**: Capture magnetic emanations during AES
  operations → perform CPA (Correlation Power Analysis) in the magnetic
  domain → assess whether the target leaks key material via magnetic
  emanation.
- **Countermeasure testing**: Test effectiveness of fault detection
  schemes (e.g., duplicated computation, parity checks, randomized
  execution) against precise EMFI with known timing and location.

### Penetration testing

- **Smart card assessment**: Evaluate payment cards, ID cards, and access
  cards for EMFI vulnerability as part of a physical security audit.
- **IoT device evaluation**: Assess IoT secure elements and
  microcontrollers for fault injection resistance as part of a
  hardware security assessment.
- **Automotive security**: Test ECU immobilizer authentication and
  secure OTA update mechanisms for EMFI weaknesses.

## 8. Safety & Legal / Ethical Disclaimer

> **⚠️ WARNING — HIGH VOLTAGE**
>
> This device generates up to **400 V** pulsed voltage. The Marx bank
> and charge pump circuitry can deliver a painful and potentially
> dangerous electric shock. Always discharge the Marx bank before
> handling the probe. Use the safety interlock (remove probe cap →
> automatic discharge). Never touch the probe tip while the device is
> armed.

> **⚖️ LEGAL / ETHICAL DISCLAIMER**
>
> TeslaPhantom is a **security research tool** designed for use by
> authorized security researchers, red teams, and penetration testers
> **only**. It must be used exclusively on devices and systems that you
> own or have **explicit written authorization** to test.
>
> Unauthorized use of this device to attack, bypass, or extract
> cryptographic keys from systems you do not own or have permission to
  test is **illegal** in most jurisdictions and may constitute a
> criminal offense under computer fraud, unauthorized access, and
> intellectual property laws.
>
> The author (**jayis1**) and contributors disclaim all liability for
> misuse of this design. This tool is provided for educational and
> authorized security research purposes only. Users are solely
> responsible for compliance with all applicable laws and regulations.
>
> **Use responsibly. Use legally. Use only with authorization.**

## 9. Bill of Materials (Key Components)

| Ref | Part | Package | Qty | Notes |
|-----|------|---------|-----|-------|
| U1 | STM32H743VIT6 | LQFP-100 | 1 | Main MCU |
| U2 | iCE40UP5K-SG48 | QFN-48 | 1 | Timing FPGA |
| U3 | AD7606C-6 | LQFP-64 | 1 | SCA ADC |
| U4 | AD8331 | SOIC-16 | 1 | VGA for fluxgate |
| U5 | LTC1564-7 | SSOP-16 | 1 | Programmable LPF |
| U6 | DRV425 | WSON-12 | 1 | Fluxgate magnetometer |
| U7 | NINA-B306 | module | 1 | BLE 5.0 |
| U8 | TMC2209 | QFN-28 | 3 | Stepper drivers |
| U9 | TPS62843 | SOT-563 | 1 | 3.3V buck |
| U10 | TPS7A47 | TSSOP-20 | 1 | 1.8V LDO |
| U11 | TPS61070 | SOT-23-6 | 1 | 5V boost |
| U12 | IS66WV51216BLL | TSOP-48 | 1 | 1 MB SRAM |
| U13 | SSD1306 | module | 1 | OLED display |
| Q1–Q4 | 2N5551 | TO-92 | 4 | Marx bank avalanche switches |
| D1–D4 | 1N4148 | SOD-123 | 4 | Marx bank charging diodes |
| C1–C4 | 1 nF 630V C0G | 1206 | 4 | Marx bank capacitors |
| M1–M3 | NEMA-8 stepper | — | 3 | 3-axis positioning |
| SW1–SW3 | TCST2103 opto | — | 3 | Limit switches |

## 10. Repository Structure

```
tesla-phantom/
├── README.md              # This file
├── firmware/
│   ├── Makefile
│   ├── linker.ld
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── board_init.c
│       ├── fpga_spi.c
│       ├── hv_pulse.c
│       ├── stepper.c
│       ├── fluxgate.c
│       ├── adc_capture.c
│       ├── sca_engine.c
│       ├── trigger.c
│       ├── scan_controller.c
│       ├── ble_uart.c
│       ├── usb_cdc.c
│       ├── oled.c
│       ├── sdcard.c
│       └── config.c
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── App.js
    ├── package.json
    ├── components/
    │   └── DeviceContext.js
    └── screens/
        ├── DashboardScreen.js
        ├── EmfiControlScreen.js
        ├── ScaCaptureScreen.js
        ├── ScanSetupScreen.js
        ├── ScanMapScreen.js
        ├── TraceAnalysisScreen.js
        └── SettingsScreen.js
```

---

*Copyright (C) 2026 jayis1. Licensed under GPL-2.0.*
*Author: jayis1*