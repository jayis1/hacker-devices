# MagLance — Targeted Magnetic Field Injection Platform

```
   ╔══════════════════════════════════════════════════════════════════╗
   ║   ███╗   ███╗ █████╗ ██╗██╗   ██████╗ ██╗  ██╗██╗   ██╗███████╗  ║
   ║   ████╗ ████║██╔══██╗██║██║  ██╔══██╗██║  ██║██║   ██║██╔════╝  ║
   ║   ██╔████╔██║███████║██║██║  ██║  ██║██║  ██║██║   ██║███████╗  ║
   ║   ██║╚██╔╝██║██╔══██║██║██║  ██║  ██║██║  ██║╚██╗ ██╔╝╚════██║  ║
   ║   ██║ ╚═╝ ██║██║  ██║██║██║  ██████╔╝╚██████╔╝ ╚████╔╝ ███████║  ║
   ║   ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═════╝  ╚═════╝   ╚═══╝  ╚══════╝  ║
   ║   Targeted Magnetic Field Injection · MRAM Manipulation            ║
   ║   Hall-Sensor Spoofing · Magnetic Side-Channel Sensing            ║
   ╚══════════════════════════════════════════════════════════════════╝
```

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange) ![HW-License](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** MagLance is designed **exclusively** for authorized security research, penetration testing under written contract, hardware vulnerability assessment of devices you own, and academic/educational environments. Manipulating magnetic sensors, MRAM, or safety-critical Hall-effect systems on devices you do not own or have not been explicitly authorized to test is illegal in most jurisdictions and may cause permanent damage to the target or create physical safety hazards (e.g., motor commutation failure, current-sensor misreadings in EV battery management). The author (**jayis1**) assumes no liability for any misuse, injury, or damage. High magnetic fields can erase magnetic storage media, disrupt pacemakers and medical devices, and damage magnetically sensitive components. Always operate in controlled environments with appropriate safety protocols, away from magnetic media and medical devices. **You are responsible for your own safety and legality.** This is a research design, not a finished product.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Attack Surface & Threat Model](#2-attack-surface--threat-model)
3. [Hardware Specifications](#3-hardware-specifications)
4. [Architecture & Block Diagram](#4-architecture--block-diagram)
5. [Firmware Design](#5-firmware-design)
6. [Application Interface](#6-application-interface)
7. [Use Cases](#7-use-cases)
8. [Safety Considerations](#8-safety-considerations)
9. [License & Credits](#9-license--credits)

---

## 1. Overview

**MagLance** is an open-source, handheld magnetic field injection platform for hardware security research. It generates precisely controlled, localized pulsed and DC magnetic fields capable of manipulating magnetoresistive memory (MRAM, STT-MRAM, FeRAM), spoofing Hall-effect sensors, bypassing magnetic reed switches and tamper detectors, and sensing magnetic side-channel emissions from nearby current-carrying conductors.

Unlike broad electromagnetic fault injection (EMFI) tools that use high-voltage spark-gap probes to induce transient faults in nearby ICs, MagLance operates in a fundamentally different regime: it produces **controlled, directional, quasi-static magnetic fields** using precision current-driven coil assemblies. This allows it to:

- **Write** individual MRAM/STT-MRAM bits by exceeding the switching threshold of specific magnetic tunnel junctions (MTJs)
- **Spoof** Hall-effect sensors (used extensively in automotive BLDC motor commutation, EV current sensing, and position encoders) by presenting an external field that overrides the sensed field
- **Bypass** magnetic reed switches and Hall-effect tamper switches used in security enclosures, ATMs, and alarm systems
- **Disrupt** magnetic isolators (e.g., digital isolators using giant magnetoresistance — GMR) used in galvanic isolation barriers
- **Sense** magnetic side-channel emissions from nearby current flows through walls and enclosures, enabling non-invasive current waveform reconstruction
- **Profile** the magnetic susceptibility of a target using the integrated 3-axis magnetometer array before, during, and after injection

The device is built around an **STM32G474** mixed-signal MCU — chosen for its high-resolution timer (HRTIM) capable of 184 ps pulse resolution, CORDIC accelerator for real-time trigonometric computations, and FMAC (filter math accelerator) for real-time signal processing of magnetometer data. The coil driver uses a high-current bipolar H-bridge architecture with a **gate driver + discrete MOSFET** topology, enabling field reversal (north/south polarity switching) and pulse widths from 100 ns to continuous DC.

### Why Magnetic Field Injection?

Magnetic field injection attacks are an emerging and underexplored class of hardware attacks. As the industry moves toward:

- **MRAM/STT-MRAM** as persistent memory in IoT and automotive (replacing flash/eeprom)
- **Hall-effect sensors** in safety-critical automotive, industrial, and robotics applications
- **Magnetic digital isolators** replacing optocouplers in power electronics
- **Magnetic reed switches** in tamper-detection for ATMs, POS terminals, and secure enclosures

...the attack surface created by external magnetic fields becomes increasingly relevant. MagLance is the first open-source tool purpose-built to research this attack surface in a controlled, reproducible manner.

---

## 2. Attack Surface & Threat Model

### 2.1 Target Classes

| Target Class | Example Components | Attack Mode | Impact |
|---|---|---|---|
| **MRAM / STT-MRAM** | Everspin MR4A08B, Avalanche persistent memory, embedded MRAM in MCUs | Bit-flip via localized field exceeding MTJ switching threshold | Persistent memory corruption, security bit clearing, key modification |
| **Hall-effect current sensors** | Allegro ACS712/ACS724, Infineon TLE4972, Melexis MLX91208 | External field injection biases current reading | EV battery management spoofing, motor controller disruption, overcurrent bypass |
| **Hall-effect position/speed sensors** | Allegro A3144, Honeywell SS41, automotive wheel speed sensors | Field injection mimics or masks target field | ABS/ESC disruption, motor commutation failure, tachometer spoofing |
| **Magnetic reed switches** | Hermetically sealed reed switches in security enclosures, ATMs, POS terminals | External magnet holds/breaks switch state | Tamper bypass, door-open suppression, alarm disable |
| **Magnetic digital isolators** | NVE IsoLoop, magnetic couplers in motor drives, solar inverters | Field injection corrupts isolated signal | Galvanic isolation bypass, signal injection across isolation barrier |
| **GMR/TMR magnetic sensors** | NVE GMR sensors, TMR angle sensors in automotive | Field injection corrupts angle/position reading | Throttle position spoofing, steering angle manipulation |
| **Magnetic stripe media** | Credit cards, hotel keys, access cards | Controlled erasure or partial overwrite | Data destruction, magstripe cloning assistance |

### 2.2 Threat Model

**Adversary capability:** Physical proximity (1–50 mm) to the target device. The attacker needs to position the coil assembly near the target component. This is feasible during:

- Physical access engagements (red team with device access)
- Supply-chain interdiction (pre-deployment modification)
- Laboratory security evaluation (authorized testing)
- Forensic analysis (controlled environment)

**Adversary objective:** One or more of:
- Corrupt persistent memory to bypass security controls
- Spoof sensor readings to cause malfunction or bypass safety limits
- Bypass physical tamper detection
- Extract information via magnetic side-channel emissions
- Evaluate magnetic susceptibility of a design under test

**Adversary constraints:** The attacker must know or discover the approximate location of the target component. MagLance's integrated magnetometer array assists with this by passively mapping the local magnetic field landscape before injection.

### 2.3 Attack Feasibility Notes

- **MRAM bit-flip:** Requires 1–10 mT at the MTJ location. MagLance can generate 5–50 mT at 5 mm distance with the standard coil tip. MRAM switching thresholds vary by technology generation; older toggle-MRAM is more susceptible than modern STT-MRAM with spin-transfer torque write assist.
- **Hall sensor spoofing:** Hall sensors typically measure ±10–100 mT. MagLance can easily generate fields in this range. The attack is trivially feasible — the challenge is calibration to produce a specific spoofed reading.
- **Reed switch bypass:** Reed switches typically activate at 1–5 mT. MagLance can hold or release reed switches from up to 30 mm with the standard coil.
- **Magnetic side-channel sensing:** Detects fields from 0.1 µT to 1 mT. Can sense current changes of ~10 mA at 10 mm through thin enclosure walls.

---

## 3. Hardware Specifications

### 3.1 Core Components

| Component | Part | Purpose |
|---|---|---|
| **MCU** | STM32G474VET6 (LQFP-100) | Main controller: HRTIM pulse generation, CORDIC math, USB CDC, BLE UART, profile management |
| **Coil Driver** | Discrete H-bridge: 4× IPB019N08N3 (80V, 100A, 1.9mΩ) + 2× UCC21520 gate drivers | Bipolar current drive to coil assembly; supports pulse and DC modes |
| **Coil Assembly** | Interchangeable tip with hand-wound copper coil on ferrite core | Field generation; standard tip = 200 turns, 2 mm air gap, ferrite rod |
| **Magnetometer Array** | 3× PNI RM3100 (SPI, 3-axis) | Field mapping and measurement; positioned at 0 mm, 15 mm, 30 mm from coil face |
| **Current Sense** | ACS724ELCTR-30AB (±30A Hall current sensor) | Real-time coil current monitoring for closed-loop field control |
| **BLE Module** | nRF52840-M.2 module | Wireless control and telemetry; AES-256 encrypted C2 channel |
| **Power Management** | TPS61235PRWLR boost (5V/5A) + BQ25895 charger | LiPo charging and 5V rail for coil driver logic |
| **Battery** | 3.7V 18650 LiPo (3000 mAh) | Portable power; supports ~200 high-power pulses or 30 min DC mode |
| **USB** | USB-C (USB 2.0 Full Speed, CDC) | Firmware updates, configuration, serial console |
| **Display** | 0.96" OLED (SSD1306, I2C) | Status, field strength, current draw, profile name |
| **User Input** | 3 push buttons + rotary encoder | Mode selection, parameter adjustment, fire trigger |
| **Storage** | 24LC256 (256 kbit I2C EEPROM) | 16 profile slots with parameters and names |
| **Safety** | ADS1115 ADC (16-bit) for temperature/voltage monitoring + hardware fuse | Overcurrent cutoff, thermal protection, undervoltage lockout |

### 3.2 Performance Parameters

| Parameter | Value |
|---|---|
| **Peak magnetic field** | 50 mT at 5 mm (standard tip), 120 mT at 1 mm (focused tip) |
| **Pulse width range** | 100 ns to 100 ms (HRTIM controlled) |
| **Pulse resolution** | 184 ps (HRTIM, 32× interpolation) |
| **Field polarity** | Bipolar (north/south), software-selectable |
| **Maximum coil current** | 80 A peak (pulsed), 8 A continuous (DC mode) |
| **Magnetometer range** | ±800 µT (RM3100), 3-axis, 1 µT resolution |
| **Refresh rate** | Up to 600 Hz (magnetometer), 500 kHz (current sense via ADC) |
| **Profiles** | 16 stored in EEPROM, each with 8 parameters |
| **Connectivity** | USB-C CDC, BLE 5.0 (nRF52840) |
| **Battery life** | ~200 high-power pulses, ~30 min DC mode, ~8 hr standby |
| **Form factor** | 150 × 50 × 30 mm wand (excluding coil tip) |
| **Weight** | ~220 g (with battery), ~80 g (without) |

### 3.3 Interchangeable Coil Tips

| Tip | Turns | Core | Field @ 5mm | Use Case |
|---|---|---|---|---|
| **Standard** | 200 | Ferrite rod, 5mm dia | 50 mT | General purpose, MRAM, Hall sensors |
| **Focused** | 400 | Ferrite cone, 2mm tip | 120 mT @ 1mm | High-field targeting, small MTJ arrays |
| **Wide** | 50 | Air core, 20mm dia | 15 mT | Reed switches, large area coverage |
| **Sense** | N/A | N/A (passive only) | N/A | Magnetic side-channel sensing only |

---

## 4. Architecture & Block Diagram

```
  ┌─────────────────────────────────────────────────────────────────┐
  │                        MagLance Top-Level                        │
  │                                                                  │
  │  ┌──────────┐    USB-C     ┌─────────────────────────────────┐  │
  │  │  Host PC │◄────────────►│         STM32G474VET6           │  │
  │  │  /Phone  │   BLE 5.0    │                                 │  │
  │  │  (App)   │◄────────────►│  ┌─────────┐  ┌────────────┐  │  │
  │  └──────────┘   nRF52840   │  │ HRTIM   │  │  CORDIC    │  │  │
  │                            │  │ (184ps) │  │  + FMAC    │  │  │
  │                            │  └────┬────┘  └─────┬──────┘  │  │
  │                            │       │              │        │  │
  │                            │  ┌────▼──────────────▼────┐   │  │
  │                            │  │   Coil Driver State     │   │  │
  │                            │  │   Machine & Safety      │   │  │
  │                            │  └────┬────────────────────┘   │  │
  │                            └───────┼────────────────────────┘  │
  │                                    │                           │
  │           ┌────────────────────────┼────────────────────┐     │
  │           │           H-Bridge Driver Section            │     │
  │           │                                            │     │
  │           │   ┌──────────┐    ┌─────┐     ┌─────┐      │     │
  │           │   │UCC21520 #1│──►│Q1 Q2│     │Q3 Q4│◄──┐ │     │
  │           │   └──────────┘    └──┬──┘     └──┬──┘   │ │     │
  │           │   ┌──────────┐       │           │      │ │     │
  │           │   │UCC21520 #2│──────┼───────────┼──────┘ │     │
  │           │   └──────────┘       │           │        │     │
  │           │                ┌─────▼───────────▼────┐   │     │
  │           │                │   Coil Assembly      │   │     │
  │           │                │   (Interchangeable)  │   │     │
  │           │                └─────────┬───────────┘   │     │
  │           │                          │Current        │     │
  │           │              ┌───────────▼──────────┐   │     │
  │           │              │  ACS724 Current Sense │   │     │
  │           │              └───────────┬──────────┘   │     │
  │           └──────────────────────────┼──────────────┘     │
  │                                      │ Feedback            │
  │  ┌───────────────────────────────────┼─────────────────┐  │
  │  │         Sensing Section           │                  │  │
  │  │                                    │                  │  │
  │  │  ┌──────────┐  ┌──────────┐  ┌────▼─────┐           │  │
  │  │  │RM3100 #1 │  │RM3100 #2 │  │RM3100 #3 │           │  │
  │  │  │(0mm)     │  │(15mm)    │  │(30mm)    │           │  │
  │  │  └────┬─────┘  └────┬─────┘  └────┬─────┘           │  │
  │  │       │             │             │                  │  │
  │  │       └─────────────┴─────────────┘                  │  │
  │  │                     │ SPI                            │  │
  │  │               ┌──────▼──────┐                        │  │
  │  │               │  STM32G474  │                        │  │
  │  │               │  FMAC filter│                        │  │
  │  │               └─────────────┘                        │  │
  │  └────────────────────────────────────────────────────┘  │
  │                                                          │
  │  ┌────────────────────────────────────────────────────┐  │
  │  │              Power & Safety Section                 │  │
  │  │                                                     │  │
  │  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌─────────┐ │  │
  │  │  │ 18650  │─►│BQ25895 │─►│TPS61235│─►│ 5V Rail │ │  │
  │  │  │ LiPo   │  │Charger │  │  Boost │  │  (5V/5A)│ │  │
  │  │  └────────┘  └────────┘  └────────┘  └────┬────┘ │  │
  │  │                                              │      │  │
  │  │  ┌─────────┐  ┌─────────┐  ┌──────────┐    │      │  │
  │  │  │ADS1115  │  │Thermal  │  │ 15A Fuse │◄───┘      │  │
  │  │  │(Monitor)│  │ NTC x2  │  │ + TVS    │           │  │
  │  │  └─────────┘  └─────────┘  └──────────┘           │  │
  │  └────────────────────────────────────────────────────┘  │
  │                                                          │
  │  ┌────────────────────────────────────────────────────┐  │
  │  │              User Interface Section                 │  │
  │  │  ┌─────────┐  ┌──────────┐  ┌────────────────────┐ │  │
  │  │  │SSD1306  │  │ 3x Btn + │  │ 24LC256 EEPROM     │ │  │
  │  │  │ OLED    │  │ Rotary   │  │ (16 profiles)       │ │  │
  │  │  │ Display │  │ Encoder  │  │                    │ │  │
  │  │  └─────────┘  └──────────┘  └────────────────────┘ │  │
  │  └────────────────────────────────────────────────────┘  │
  └─────────────────────────────────────────────────────────┘
```

### 4.1 Signal Flow

**Injection path:** MCU HRTIM → gate driver → H-bridge MOSFETs → coil → magnetic field at target. Current through the coil is measured by the ACS724 and fed back to the MCU ADC at 500 kHz for closed-loop field strength control.

**Sensing path:** 3× RM3100 magnetometers → SPI → MCU FMAC (filtering/denoising) → CORDIC (vector magnitude, angle calculation) → USB/BLE telemetry to host. The three sensors at different distances enable gradient-based source localization.

**Safety path:** ADS1115 monitors battery voltage, coil driver supply voltage, and two NTC thermistors (one on the H-bridge PCB, one on the coil). If any parameter exceeds safe limits, the HRTIM is immediately disabled via hardware comparator interrupt, and the gate drivers are pulled low by a dedicated safety GPIO.

---

## 5. Firmware Design

### 5.1 Architecture

The firmware is written in C for the STM32G474 using bare-metal register access (no HAL dependency for core functionality). It is organized into the following modules:

| Module | File | Responsibility |
|---|---|---|
| **Main control loop** | `main.c` | System initialization, state machine, user input handling, mode dispatch |
| **Board configuration** | `board.h` | Pin assignments, clock config, peripheral mapping |
| **Register definitions** | `registers.h` | STM32G474 register addresses and bit definitions |
| **Coil driver** | `coil_driver.c/h` | HRTIM configuration, pulse generation, H-bridge control, current feedback |
| **Magnetometer** | `magnetometer.c/h` | RM3100 SPI driver, 3-axis field measurement, gradient calculation |
| **Profile manager** | `profile_manager.c/h` | EEPROM storage and retrieval of 16 attack profiles |
| **USB interface** | `usb_iface.c/h` | USB CDC serial protocol, command parser, telemetry streaming |
| **BLE interface** | (via UART to nRF52840) | ASCII command protocol forwarded from USB CDC |

### 5.2 Operating Modes

1. **PULSE mode:** Single or multi-pulse injection with configurable width (100 ns – 100 ms), polarity, amplitude (current setpoint), and inter-pulse delay. Used for MRAM bit-flip, reed switch toggle, and transient Hall sensor spoofing.

2. **DC mode:** Continuous DC field for sustained Hall sensor spoofing or reed switch holding. Current limited to 8 A continuous with thermal monitoring. Auto-shutdown on overtemperature.

3. **SWEEP mode:** Frequency-swept AC field (1 Hz – 100 kHz) for magnetic isolator disruption and resonance hunting. The sweep can be linear or logarithmic, with configurable dwell time per frequency step.

4. **SENSE mode:** Passive-only mode. All three RM3100 sensors sample at maximum rate, data is streamed to the host for real-time magnetic field mapping and side-channel current waveform reconstruction. No coil output.

5. **PROFILE mode:** Recall and execute a stored profile. Profiles contain: mode, width, polarity, current, pulse count, delay, sweep parameters, and a human-readable name.

6. **CALIBRATE mode:** Closed-loop calibration routine. The coil is driven at a known current, and the RM3100 array measures the resulting field. This characterizes the coil-to-field transfer function for each tip, enabling accurate field strength targeting.

### 5.3 Key Design Decisions

- **HRTIM for pulse generation:** The STM32G474's HRTIM provides 184 ps timing resolution — essential for precise MRAM switching threshold characterization. Standard timers (16 ns resolution at 170 MHz) are insufficient for sub-nanosecond field pulse control.

- **Bipolar H-bridge:** Using a full H-bridge (rather than a single low-side switch) allows bidirectional current flow, enabling both north and south polarity fields without physically repositioning the coil. This is critical for toggle-MRAM writing, which requires alternating field polarity.

- **Closed-loop current control:** The ACS724 current sensor feeds back to the MCU ADC at 500 kHz. The firmware implements a PI controller that adjusts the HRTIM duty cycle in real-time to maintain the target current despite coil heating (which changes resistance) and battery voltage sag. This ensures repeatable field strength across operating conditions.

- **3-axis gradient sensing:** Three RM3100 sensors at different distances enable magnetic source localization via gradient analysis. The CORDIC accelerator computes the vector magnitude and angle in real-time, and the FMAC performs spatial filtering to reduce noise.

- **Bare-metal register access:** Direct register manipulation (rather than STM32 HAL) provides full control over HRTIM timing registers and minimizes interrupt latency for safety-critical overcurrent shutdown.

- **Modular coil tips:** The firmware stores calibration data for multiple coil tips in EEPROM. Tip identification is via a 1-wire EEPROM in the coil connector (DS2431), allowing automatic calibration loading when a tip is connected.

### 5.4 Safety System

The safety system operates at three levels:

1. **Hardware level:** A 15A fast-blow fuse in series with the coil. A TVS diode clamps inductive kickback. Gate driver enable pins are tied to a hardware comparator that monitors coil current — if current exceeds 100 A (fault condition), the gate drivers are disabled within <1 µs, independent of firmware.

2. **Firmware level:** Real-time monitoring of: coil current (ACS724), H-bridge temperature (NTC1), coil temperature (NTC2), battery voltage, and boost converter output voltage. Any out-of-range condition triggers immediate shutdown with an error code displayed on the OLED.

3. **User level:** A physical enable switch (one of the 3 push buttons) must be held during injection. Releasing the switch immediately cuts coil output. A keylock mechanism prevents accidental triggering.

---

## 6. Application Interface

### 6.1 Companion App

MagLance ships with a React Native companion app (`app/`) for iOS and Android. The app connects via BLE 5.0 to the nRF52840 module and provides:

- **Dashboard:** Real-time display of 3-axis field strength from all three RM3100 sensors, coil current, battery level, temperature, and active profile
- **Pulse control:** Configure and fire single/multi-pulse sequences with visual timing diagram
- **DC control:** Slider for continuous field strength with polarity toggle
- **Sweep control:** Frequency sweep configuration with live spectrum analyzer (FFT of magnetometer data)
- **Sense mode:** Real-time 3D magnetic field visualization, gradient display, and current waveform reconstruction
- **Profile manager:** Create, edit, save, and load profiles; assign custom names and icons
- **Calibration wizard:** Step-by-step coil tip calibration with automatic transfer function generation
- **Session logging:** All actions and sensor data are logged to the phone's storage for post-engagement analysis
- **Safety monitor:** Prominent safety status display with configurable alert thresholds

### 6.2 Serial Protocol (USB CDC / BLE UART)

The device exposes a simple ASCII command protocol over both USB CDC and BLE UART:

```
Command format: <COMMAND> <PARAM1> <PARAM2> ...\n
Response format: OK <data>\n  or  ERR <code> <message>\n

Commands:
  PULSE <width_ns> <current_ma> <polarity> <count> <delay_us>
  DC <current_ma> <polarity>
  SWEEP <start_hz> <stop_hz> <steps> <dwell_ms> <current_ma>
  SENSE <rate_hz> <duration_s>
  STOP
  PROFILE LOAD <slot>
  PROFILE SAVE <slot> <name>
  PROFILE GET <slot>
  CAL <tip_id>
  GET FIELD          -> "OK <x0> <y0> <z0> <x1> <y1> <z1> <x2> <y2> <z2>"
  GET STATUS         -> "OK <mode> <current> <vbat> <temp1> <temp2> <profile>"
  GET TIP            -> "OK <tip_id> <tip_name> <cal_a> <cal_b>"
  SET TIP <tip_id>
  HELP
```

### 6.3 Data Streaming

In SENSE mode, the device streams binary magnetometer frames at up to 600 Hz:

```
Frame (20 bytes):
  [0-1]   sync word 0xAA55
  [2-3]  sensor 0 X (int16, µT * 100)
  [4-5]  sensor 0 Y
  [6-7]  sensor 0 Z
  [8-9]  sensor 1 X
  [10-11] sensor 1 Y
  [12-13] sensor 1 Z
  [14-15] sensor 2 X
  [16-17] sensor 2 Y
  [18-19] sensor 2 Z
```

---

## 7. Use Cases

### 7.1 For Red Teams

- **Tamper bypass:** Hold magnetic reed switches in their "closed" state while opening a security enclosure, suppressing the tamper signal. This allows access to internal components without triggering alarms.
- **Sensor spoofing during physical engagement:** Spoof Hall-effect current sensors in EV charging stations to bypass overcurrent protection or billing mechanisms.
- **Persistent memory manipulation:** Clear security bits or modify configuration in MRAM-based devices without leaving forensic traces of electrical access (no soldering, no debug port access — purely magnetic).
- **Covert current sensing:** Use the SENSE mode to monitor power consumption patterns of a target device through its enclosure, identifying activity states and potentially extracting timing-based cryptographic information.

### 7.2 For Security Researchers

- **MRAM susceptibility characterization:** Systematically map the magnetic field thresholds required to flip bits in various MRAM/STT-MRAM technologies. This produces valuable data for designing magnetic shielding in MRAM-based secure elements.
- **Hall sensor attack surface evaluation:** Test automotive and industrial Hall sensors against external field injection to evaluate the effectiveness of magnetic shielding and differential sensor designs.
- **Magnetic isolator evaluation:** Evaluate magnetic digital isolators for susceptibility to external field injection, determining whether isolation barriers can be breached magnetically.
- **Magnetic side-channel analysis:** Use the integrated magnetometer array to measure the magnetic emissions of cryptographic devices, potentially correlating with power side-channel traces for combined attacks.
- **Reed switch security assessment:** Evaluate the bypass resistance of reed switch-based tamper detection in ATMs, POS terminals, and secure enclosures.

### 7.3 For Penetration Testers

- **Physical security testing:** Evaluate magnetic reed switch-based door/window sensors in alarm systems. Determine the standoff distance and field strength required for bypass.
- **Automotive security:** Test Hall-effect wheel speed sensors, throttle position sensors, and current sensors for susceptibility to external magnetic manipulation. Evaluate whether safety-critical readings can be spoofed.
- **IoT device testing:** Many IoT devices use MRAM for configuration storage and Hall sensors for tamper detection. MagLance enables non-invasive testing of these mechanisms.
- **Payment terminal testing:** Evaluate the magnetic tamper detection in POS terminals and ATMs. Some terminals use magnetic switches to detect physical opening — MagLance can test bypass resistance.

---

## 8. Safety Considerations

### 8.1 Magnetic Field Safety

- MagLance can generate fields up to 120 mT at close range. While this is below the threshold for biological effects (which begin around 1 T for static fields), it can:
  - **Erase magnetic stripe cards** (credit cards, hotel keys) within 50 mm
  - **Damage mechanical hard drives** within 100 mm
  - **Disrupt magnetically sensitive medical devices** (pacemakers, insulin pumps) — maintain at least 1 m distance
  - **Magnetize ferromagnetic objects** placed in the field

### 8.2 Electrical Safety

- The H-bridge can source up to 80 A from the battery. The 15A fuse provides hard protection against catastrophic faults.
- The coil can reach 80°C during sustained operation. Thermal monitoring will shut down the system before this occurs, but the coil tip may be warm to the touch.
- The battery is a high-current 18650 cell. A battery protection PCB (BMS) provides overcharge, over-discharge, and short-circuit protection.

### 8.3 Operational Safety

- Always operate behind a non-magnetic barrier if the target could produce shrapnel from magnetic attraction
- Never point the coil at magnetic storage media or medical devices
- Use the lowest effective field strength and shortest pulse width
- Allow the coil to cool between high-power pulses (the firmware enforces a thermal cooldown period)
- The physical enable switch must be held during injection — this is a deliberate dead-man's switch

---

## 9. License & Credits

**Author:** jayis1

**Firmware & Software:** GPL-2.0
**Hardware (KiCad designs):** CERN-OHL-S v2
**Documentation:** CC-BY-SA 4.0

MagLance is an original design by jayis1. All firmware, hardware designs, application code, and documentation in this repository are the work of jayis1.

This project uses the following open-source components:
- STM32G474 (STMicroelectronics) — MCU
- nRF52840 (Nordic Semiconductor) — BLE module
- RM3100 (PNI Corporation) — magnetometer
- React Native (Meta) — companion app framework

### Contributing

This is a research design. Issues, pull requests, and field-test reports are welcome. If you build MagLance and conduct susceptibility testing on a device, please share your results (redacted as appropriate) to build a community knowledge base of magnetic field vulnerability data.

### Citation

If you use MagLance in academic research, please cite:

```
@misc{maglance,
  author = {jayis1},
  title = {MagLance: Targeted Magnetic Field Injection Platform},
  year = {2026},
  url = {https://github.com/jayis1/hacker-devices/tree/main/maglance}
}
```

---

*MagLance — the first open-source tool for systematic magnetic field vulnerability research.*