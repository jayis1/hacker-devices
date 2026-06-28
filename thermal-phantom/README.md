# Thermal Phantom

**A Precision Thermal Fault Injection & Side-Channel Analysis Device**

*Author: jayis1*  
*Copyright © 2026 jayis1. All rights reserved.*

---

## ⚠️ Legal & Ethical Disclaimer

**This device is intended for AUTHORIZED security research, penetration testing, and academic study ONLY.**  
Use of Thermal Phantom to attack hardware without explicit written authorization from the device owner is ILLEGAL in most jurisdictions. Thermal fault injection can permanently damage or destroy integrated circuits. The author (jayis1) assumes no liability for misuse, damage, or illegal activity. By using this design, you accept full responsibility for compliance with all applicable laws.

---

## 1. Overview

**Thermal Phantom** is a compact, precision thermal fault injection device designed for security researchers and red teams. It leverages controlled localized heating and cooling of integrated circuits to induce fault conditions for bypassing security mechanisms, extracting cryptographic keys, and analyzing hardware resilience to thermal stress.

Unlike voltage glitching or electromagnetic fault injection (EMFI), thermal fault injection (TFI) exploits the fact that semiconductor timing, threshold voltages, and logical behavior change with temperature. By rapidly heating or cooling a target chip's specific region — sometimes down to individual logic blocks — an attacker can:

- Bypass secure boot checks by inducing timing violations
- Cause bit flips in cryptographic operations (differential fault analysis)
- Disrupt RNG entropy sources
- Bypass readout protections by altering flash/SRAM retention behavior
- Trigger debug or factory test modes that are temperature-gated

Thermal Phantom combines a high-power Peltier (TEC) module, precision IR temperature sensing, laser-targeted heating, and a real-time control loop capable of ±0.1°C accuracy at the silicon surface. It operates either as a standalone tool with a trigger input, or under full software control via USB or BLE.

### What Makes It Novel

No existing commercial or open-source tool provides combined **heating AND cooling** fault injection with closed-loop thermal regulation and sub-millisecond timing. Existing thermal attack research uses crude hot-air stations or spray dusters — ThermPhantom brings **lab-grade precision** to a pocket-sized device.

---

## 2. Attack Surface & Threat Model

### 2.1 Attack Vectors

| Vector | Description | Target Components |
|--------|-------------|-------------------|
| **Thermal Glitching** | Rapid temperature transitions to induce timing faults during critical instructions | Secure boot ROM, crypto accelerators, eFuse verification logic |
| **Cold Boot Persistence** | Cooling SRAM/DRAM to extend data retention after power-off for memory forensics | DDR modules, SRAM, cache memory |
| **DFA (Differential Fault Analysis)** | Inducing bit errors mid-crypto computation to recover keys via fault analysis | AES, RSA, ECC hardware accelerators |
| **RNG Bias** | Temperature manipulation to bias random number generators | TRNGs, entropy pools |
| **Flash Retention Manipulation** | Heating to alter flash read thresholds, potentially bypassing readout protection | NOR/NAND flash, eMMC |
| **Thermal Trojan Trigger** | Detecting hardware trojans that activate at specific temperatures | Any IC suspected of containing trojan logic |

### 2.2 Threat Model

- **Asset:** Cryptographic keys, secure boot secrets, proprietary firmware, fuse data
- **Attacker Capability:** Physical access to target device for 5-60 minutes; ability to power-cycle target
- **Attack Difficulty:** Medium-High (requires understanding of target thermal behavior, but ThermPhantom automates the control loop)
- **Detection:** Thermal attacks are difficult to detect via software attestation because they don't modify firmware — they alter physics. Some TPMs include temperature sensors, but most SoCs do not log thermal anomalies during boot.
- **Mitigation (Defender Perspective):** Thermal sensors on critical blocks, redundant computation, temperature-locked fuses, conformal coating

### 2.3 Realistic Attack Scenarios

1. **Secure Boot Bypass:** Target SoC verifies RSA signature during boot at ambient temperature. Attacker heats the crypto accelerator block to 95°C, causing a verification timing fault that skips the check. Boot proceeds with unsigned firmware.

2. **DRAM Cold Boot:** Target device holds encryption keys in RAM. Attacker cools the DRAM chip to -20°C using Peltier module, powers off, and quickly dumps memory contents. Keys persist for extended periods at low temperatures.

3. **eFuse Readout Bypass:** Target MCU has flash readout protection (RDP) active. Heating the flash controller to 110°C alters read threshold voltages, causing the RDP check to misread the fuse as unprotected.

---

## 3. Hardware Specifications

### 3.1 Core Components

| Component | Part | Purpose |
|-----------|------|---------|
| **MCU** | STM32H743 (Cortex-M7 @ 480 MHz) | Main control loop, thermal PID, trigger I/O, comms |
| **Peltier (TEC)** | TEC1-12706 (15W, 12V) | Reversible heating/cooling of target |
| **TEC Driver** | DRV8873 (H-bridge, 10A) | Bidirectional current control for heat/cool |
| **IR Temperature Sensor** | MLX90614-DCI (medical grade, ±0.2°C) | Non-contact silicon surface temp |
| **Contact Probe Temp** | DS18B20 (±0.5°C) | Contact backup temp on Peltier cold plate |
| **Laser Heater** | 450nm 1W laser diode (focused) | Ultra-fast localized spot heating (<1ms response) |
| **Laser Driver** | LM359 constant-current driver | Precise laser pulse control |
| **BLE Module** | nRF52840 dongle (UART-connected) | Wireless control & data exfil |
| **USB Interface** | FT4232H (USB 2.0 Hi-Speed) | Firmware upload, data capture, serial console |
| **Power** | 4S LiPo (14.8V, 2200 mAh) + TPS54360 buck | Portable operation + bench power compatible |
| **Trigger I/O** | SMA connectors x2 | External trigger in/out for synchronization |
| **Display** | 0.96" OLED (SSD1306, I2C) | Status display, temperature readout |
| **Form Factor** | 100 x 60 x 25mm handheld | Aluminum enclosure, thermal insulation |

### 3.2 Performance Characteristics

- **Temperature Range:** -10°C to +120°C at silicon surface (Peltier), up to +300°C spot (laser)
- **Cooling Rate:** 2°C/second (Peltier active)
- **Heating Rate:** 3°C/second (Peltier), 50°C/second (laser spot)
- **Accuracy:** ±0.1°C (closed-loop, steady state)
- **Response Time:** <5ms (laser), <100ms (Peltier transition)
- **Trigger Latency:** <10µs (hardware trigger path)
- **Battery Life:** ~45 minutes active Peltier, ~8 hours monitoring only
- **Communication:** USB 2.0 (480 Mbps), BLE 5.0, UART debug

### 3.3 Safety Systems

- Hardware overtemperature cutoff at 130°C (dedicated comparator, independent of MCU)
- Overcurrent protection on TEC driver (10.5A limit)
- Laser interlock (mechanical shutter, fail-safe closed)
- Battery protection (BMS with overcharge/discharge/balance)
- Emergency stop button (hardwired power cutoff)

---

## 4. Architecture & Block Diagram

```
 ┌─────────────────────────────────────────────────────────────┐
 │                     THERMAL PHANTOM                         │
 │                                                             │
 │  ┌──────────┐    I2C     ┌──────────────┐    SPI           │
 │  │ MLX90614 │──────────▶│              │────────▶┌──────┐ │
 │  │ IR Temp  │            │  STM32H743   │          │ OLED │ │
 │  └──────────┘           │  Controller  │          └──────┘ │
 │                         │              │                    │
 │  ┌──────────┐    1-Wire │  - PID Loop  │    UART           │
 │  │ DS18B20  │──────────▶│  - Scheduler │────────▶┌──────┐ │
 │  │ Contact  │           │  - Trigger   │          │nRF52 │ │
 │  └──────────┘           │    Engine    │          │ BLE  │ │
 │                         │              │          └──────┘ │
 │  ┌──────────┐    PWM    │              │    USB            │
 │  │DRV8873   │◀─────────│              │────────▶FT4232H   │
 │  │H-Bridge  │           └──────────────┘                   │
 │  └────┬─────┘                  │                            │
 │       │                        │ GPIO                       │
 │       ▼                        ▼                            │
 │  ┌──────────┐           ┌──────────┐                       │
 │  │  TEC     │           │ Trigger  │   SMA x2              │
 │  │ Peltier  │           │ I/O Mux  │─────▶ Ext Trig Out    │
 │  └──────────┘           └──────────┘◀───── Ext Trig In     │
 │                                                             │
 │  ┌──────────┐    GPIO    ┌──────────┐    GPIO               │
 │  │ LM359    │◀─────────│ Laser    │────────▶ Safety Shutter│
 │  │ Driver   │           │ Control  │                        │
 │  └────┬─────┘           └──────────┘                        │
 │       ▼                                                     │
 │  ┌──────────┐    ┌───────────────┐    ┌─────────────────┐  │
 │  │ 450nm    │    │ Overtemp      │    │ LiPo 4S BMS     │  │
 │  │ Laser    │    │ Cutoff (HW)   │    │ 14.8V 2200mAh   │  │
 │  └──────────┘    └───────────────┘    └─────────────────┘  │
 │                                                             │
 └─────────────────────────────────────────────────────────────┘
                          │
                    ┌─────┴─────┐
                    │  TARGET    │
                    │  DEVICE    │
                    │  (IC)      │
                    └───────────┘
```

### 4.1 Control Loop Architecture

The firmware implements a cascaded PID control strategy:

1. **Outer Loop (100ms):** MLX90614 IR sensor reads target surface temp → PID calculates setpoint error → adjusts TEC target
2. **Inner Loop (1ms):** DS18B20 cold-plate temp → fast PID adjusts TEC PWM duty cycle
3. **Laser Loop (100µs):** Hardware timer-based pulse control, triggered by external event or software
4. **Safety Loop (continuous):** Hardware comparator monitors temperature; if >130°C, cuts TEC power via MOSFET gate

---

## 5. Firmware Design

### 5.1 Design Philosophy

- **Real-time first:** All thermal control happens in interrupt-driven ISRs, never in main loop polling
- **Deterministic timing:** Trigger path bypasses MCU software entirely via hardware timer chaining
- **Safety in hardware:** Overtemperature and overcurrent are handled by analog comparators, not firmware
- **Modular drivers:** Each peripheral has its own driver file; main.c orchestrates only
- **Portable:** HAL abstracted via `board.h`/`registers.h`, portable to other STM32 variants

### 5.2 Key Firmware Components

| File | Purpose | Lines |
|------|---------|-------|
| `main.c` | System init, state machine, command dispatch | ~150 |
| `thermal_ctl.c` | Dual PID loop, temperature profiles | ~200 |
| `tec_driver.c` | H-bridge PWM, direction control, soft-start | ~130 |
| `laser_driver.c` | Laser pulse generation, safety shutter | ~110 |
| `temp_sensor.c` | MLX90614 + DS18B20 abstraction | ~140 |
| `trigger.c` | Hardware trigger I/O, edge detection, arm/disarm | ~120 |
| `comm.c` | USB CDC + BLE UART protocol | ~180 |
| `safety.c` | Overtemp monitor, battery monitor, watchdog | ~100 |
| `cli.c` | Serial command-line interface | ~160 |
| `board.h` | Pin mappings, clock config, constants | ~80 |
| `registers.h` | Register definitions, macros | ~60 |
| `Makefile` | Build system | ~40 |

### 5.3 Command Protocol

Thermal Phantom uses a simple binary command protocol over USB CDC and BLE UART:

```
[SYNC:0xAA][CMD:1B][LEN:2B][DATA:N][CRC:2B]
```

Key commands:
- `0x01 SET_TEMP` — Set target temperature (int16, 0.1°C units)
- `0x02 SET_MODE` — Heat / Cool / Laser / Idle
- `0x03 ARM_TRIGGER` — Arm external trigger with profile
- `0x04 FIRE_LASER` — Fire laser pulse (duration, power)
- `0x05 READ_TEMP` — Read all temperature sensors
- `0x06 LOAD_PROFILE` — Load thermal profile (sequence of steps)
- `0x07 START_PROFILE` — Execute loaded profile
- `0x08 STOP` — Emergency stop all outputs
- `0x09 GET_STATUS` — Read system status (temps, battery, state)
- `0x0A SET_PID` — Tune PID parameters (Kp, Ki, Kd)
```

---

## 6. Application / Software Interface

### 6.1 Mobile App (React Native)

The companion app provides:

- **Dashboard:** Live temperature graphs, TEC status, battery level, safety state
- **Profile Editor:** Create and edit multi-step thermal profiles (e.g., "Cool to -5°C, hold 30s, trigger, heat to 80°C in 2s")
- **Trigger Config:** Set trigger source (manual, external, auto), edge, delay
- **Laser Control:** Configure laser power, pulse duration, safety interlock status
- **Log Viewer:** Real-time event log with timestamped temperature data
- **Device Settings:** PID tuning, BLE pairing, firmware update

### 6.2 CLI Interface

For headless/scripted operation, the USB CDC serial interface provides a text CLI:

```
thermal> set_temp 85.0
thermal> set_mode heat
thermal> arm_trigger ext rising delay=100us
thermal> read_temp
  IR: 24.3°C  Plate: 23.8°C  Target: 24.1°C
thermal> fire_laser power=80% duration=50ms
thermal> load_profile cold_boot.json
thermal> start_profile
thermal> stop
```

---

## 7. Use Cases

### 7.1 Red Team Operations

- **Physical key extraction from TPMs:** Cool TPM SRAM during power glitch to extract intermediate values
- **Secure boot bypass on IoT devices:** Heat boot ROM verification logic to induce faults
- **Embedded device unlocking:** Temperature-gated debug interfaces on automotive ECUs
- **Hardware trojan detection:** Characterize device behavior across thermal range

### 7.2 Security Research

- **DFA on hardware crypto:** Induce precise faults in AES/RSA operations for differential analysis
- **RNG evaluation:** Test entropy quality under thermal stress
- **Flash security analysis:** Evaluate RDP/OTP behavior under thermal manipulation
- **Side-channel correlation:** Combine thermal profiles with power traces

### 7.3 Penetration Testing

- **Physical security testing of data center equipment:** Evaluate server DRAM cold boot resilience
- **Smart card / HSM testing:** Thermal fault injection on secure elements
- **IoT device assessment:** Test firmware verification under thermal stress
- **Supply chain verification:** Detect counterfeit or trojanized components via thermal fingerprinting

---

## 8. Repository Structure

```
thermal-phantom/
├── README.md                  ← This file
├── firmware/
│   ├── main.c                 ← System init, state machine
│   ├── thermal_ctl.c          ← PID thermal control
│   ├── thermal_ctl.h
│   ├── tec_driver.c           ← Peltier H-bridge driver
│   ├── tec_driver.h
│   ├── laser_driver.c         ← Laser pulse control
│   ├── laser_driver.h
│   ├── temp_sensor.c          ← MLX90614 + DS18B20
│   ├── temp_sensor.h
│   ├── trigger.c              ← Trigger I/O engine
│   ├── trigger.h
│   ├── comm.c                 ← USB/BLE communication
│   ├── comm.h
│   ├── safety.c               ← Safety monitoring
│   ├── safety.h
│   ├── cli.c                  ← Serial CLI
│   ├── cli.h
│   ├── board.h                ← Pin & peripheral mapping
│   ├── registers.h            ← Register definitions
│   └── Makefile
├── kicad/
│   ├── thermal-phantom.kicad_sch   ← Schematic
│   ├── thermal-phantom.kicad_pcb   ← PCB layout
│   └── thermal-phantom.kicad_pro   ← Project file
└── app/
    ├── App.tsx                ← Entry point
    ├── package.json
    └── src/
        ├── screens/
        │   ├── DashboardScreen.tsx
        │   ├── ProfileEditorScreen.tsx
        │   ├── TriggerConfigScreen.tsx
        │   ├── LaserControlScreen.tsx
        │   └── LogViewerScreen.tsx
        └── components/
            ├── TemperatureGauge.tsx
            └── StatusBadge.tsx
```

---

## 9. Bill of Materials (Summary)

| Ref | Part | Qty | Est. Cost |
|-----|------|-----|-----------|
| U1 | STM32H743VIT6 | 1 | $18.00 |
| U2 | nRF52840 USB dongle | 1 | $12.00 |
| U3 | MLX90614-DCI | 1 | $15.00 |
| U4 | DRV8873 H-bridge | 1 | $4.50 |
| U5 | FT4232H | 1 | $6.00 |
| U6 | DS18B20 | 1 | $2.00 |
| U7 | LM359 LED driver | 1 | $3.00 |
| TEC1 | TEC1-12706 Peltier | 1 | $8.00 |
| LD1 | 450nm 1W laser diode | 1 | $10.00 |
| DISP1 | SSD1306 0.96" OLED | 1 | $3.00 |
| PWR1 | TPS54360 buck converter | 1 | $4.00 |
| BAT1 | 4S LiPo 14.8V 2200mAh | 1 | $25.00 |
| BMS1 | 4S BMS protection board | 1 | $8.00 |
| MISC | Passives, connectors, enclosure | — | $20.00 |
| **Total** | | | **~$138.50** |

---

## 10. Comparison to Existing Tools

| Feature | Thermal Phantom | Hot Air Station | CO2 Duster | EMFI Tool |
|----------|----------------|-----------------|------------|-----------|
| Heating | ✓ (Peltier + Laser) | ✓ (broad area) | ✗ | ✗ |
| Cooling | ✓ (Peltier) | ✗ | ✓ (crude) | ✗ |
| Precision | ±0.1°C | ±2°C | Uncontrolled | N/A |
| Spot heating | ✓ (Laser, <1mm) | ✗ (>10mm) | ✗ | ✓ (coil) |
| Closed-loop | ✓ | ✗ | ✗ | ✗ |
| Trigger sync | ✓ (<10µs) | ✗ | ✗ | ✓ |
| Portable | ✓ | ✗ | ✓ | ✗ |
| Cost | ~$140 | ~$50 | ~$10 | >$15,000 |

---

## 11. Limitations & Future Work

- **Peltier thermal mass:** Transition from hot to cold takes ~3 seconds due to TEC thermal mass. Future versions could use dual TECs or microfluidic switching.
- **Laser safety:** 1W 450nm is Class 4. Must use with safety goggles and interlocked enclosure. Future: switch to IR laser with visible aiming laser.
- **Target surface access:** Requires physical access to bare IC die or package surface. BGA packages need decapping.
- **Profile automation:** Current CLI is manual. Future: Python automation library with Jupyter notebook integration.

---

## 12. License

Hardware design: CERN-OHL-S v2  
Firmware: GPLv3  
App: MIT  

All authored by **jayis1**.

---

*Thermal Phantom — where heat becomes a weapon of precision.*  
*jayis1, 2026*