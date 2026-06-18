# GNSS-Phantom — Portable GNSS/GPS Signal Spoofing & Simulation Platform

```
   ╔═══════════════════════════════════════════════════════════════╗
   ║    GNSS-Phantom GPX-1                                         ║
   ║    GPS L1 / Galileo E1 / GLONASS L1 / BeiDou B1 Spoofer        ║
   ║    Author: jayis1                              ║
   ╚═══════════════════════════════════════════════════════════════╝
```

**Author:** jayis1  
**Version:** 1.0.0  
**Date:** 2026-06-18  
**License:** Proprietary — Authorized Security Research Use Only

---

## ⚖️ LEGAL & ETHICAL DISCLAIMER

**GNSS-Phantom is designed SOLELY for authorized security research, penetration testing, and red-team engagements conducted in controlled, RF-shielded environments.**

- **Unauthorized GNSS spoofing is ILLEGAL** in most jurisdictions (US: 18 U.S.C. § 1367; FCC Part 47; EU: ECC/DEC/(02)01; UK: Wireless Telegraphy Act 2006).
- **It is DANGEROUS** — GNSS signals are used by aviation, maritime navigation, autonomous vehicles, critical infrastructure timing, and emergency services. Spoofing can cause physical harm, property damage, and loss of life.
- **Users are entirely responsible** for obtaining all required legal authorizations, regulatory permits, and RF transmission licenses before operating this device.
- **This device MUST be operated only inside an authorized RF-shielded test chamber** or in a licensed test range where GNSS signal transmission is permitted.
- **The author (jayis1) and contributors disclaim all liability** for misuse of this design.

**By using this design, you acknowledge that you have obtained all necessary legal authorizations and assume full legal responsibility for operation.**

---

## 1. Overview

**GNSS-Phantom** is a pocket-sized, battery-powered GNSS/GPS signal spoofing and simulation platform designed for authorized security researchers, red teamers, and penetration testers. Unlike existing commercial GNSS simulators (which cost $10,000+), GNSS-Phantom is a low-cost, open-design tool that generates authentic L1-band GNSS signals for testing the resilience of GPS-dependent systems.

### What it does

GNSS-Phantom synthesizes GPS L1 C/A (1575.42 MHz), Galileo E1 (1575.42 MHz), GLONASS L1 (1602 MHz), and BeiDou B1 (1561.098 MHz) signals using direct digital synthesis via Si4463 RF transceivers. It can:

- **Simulate** authentic multi-constellation GNSS satellite constellations with proper C/A codes, navigation messages, and ephemeris data
- **Spoof** a target receiver's perceived position to an arbitrary lat/lon/alt
- **Replay** pre-recorded GNSS scenarios from external flash
- **Generate** dynamic trajectories with velocity simulation
- **Conduct** security assessments of GPS-dependent systems (drones, autonomous vehicles, asset trackers, timing systems)

### Why it's needed

GNSS dependency is ubiquitous — drones, autonomous vehicles, marine navigation, cell tower timing (PNT), financial transaction timestamps, critical infrastructure SCADA, IoT asset tracking, and military systems all rely on GNSS. Yet GNSS signals are unauthenticated and extremely weak (~ -130 dBm at the Earth's surface), making them trivially vulnerable to spoofing. Security researchers need affordable tools to:

1. Test the resilience of GPS receivers against spoofing attacks
2. Evaluate anti-spoofing countermeasures (signal authentication, multi-antenna CRPA, IMU cross-check)
3. Red-team assessments of drone delivery systems, autonomous vehicle fleets, and critical infrastructure
4. Demonstrate GNSS vulnerabilities for executive awareness programs

---

## 2. Attack Surface & Threat Model

### Attack Surface

GNSS-Phantom targets the **L1 frequency band (1559–1610 MHz)**, specifically:

| Signal | Frequency | Modulation | Code Rate | Data Rate |
|--------|-----------|------------|-----------|-----------|
| GPS L1 C/A | 1575.42 MHz | BPSK(1) | 1.023 Mcps | 50 bps |
| Galileo E1 | 1575.42 MHz | CBOC(6,1,1/11) | 1.023 Mcps | 250 bps |
| GLONASS L1 | 1602.00 MHz | BPSK(0.5) | 0.511 Mcps | 50 bps |
| BeiDou B1 | 1561.098 MHz | BPSK(2) | 2.046 Mcps | 50 bps |

### Threat Model

| Threat Actor | Capability | Motivation |
|-------------|------------|------------|
| Red Team Operator | Spoof position of target GPS receiver | Authorized security assessment |
| Security Researcher | Test anti-spoofing defenses | Academic/publication |
| Penetration Tester | Demonstrate vulnerability to client | Client engagement |
| Drone Hijacker (illegal) | Divert drone from intended flight path | Malicious (out of scope) |
| Vehicle Hijacker (illegal) | Misdirect autonomous vehicle | Malicious (out of scope) |

### Defense Testing Targets

GNSS-Phantom can evaluate:
- **Drone navigation systems** — DJI, ArduPilot, PX4 receivers
- **Marine AIS/GPS** — ship navigation systems
- **Vehicle telematics** — fleet management, GPS tracking
- **PTP/NTP timing** — critical infrastructure timing
- **IoT asset trackers** — logistics tracking devices
- **Smartphone GPS** — consumer location services
- **Surveying equipment** — RTK GNSS receivers

### Safety Interlocks

The device implements a **multi-layer safety interlock** to prevent accidental or unauthorized transmission:

1. **Physical safety switch** (SPST toggle) — must be in LIVE position
2. **Fire button hold** — must hold for 3 seconds continuously
3. **C2 ARM command** — must be received from the authenticated companion app
4. **Auto-disarm timeout** — 5-minute inactivity auto-disarm
5. **Status LED** — red LED indicates armed/transmitting state

---

## 3. Hardware Specifications

### Main Specifications

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32H743VIT6 (Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM) |
| **RF Transceiver 1** | Si4463-B1B (GPS/Galileo L1, 142-1050 MHz + 2.4 GHz) |
| **RF Transceiver 2** | Si4463-B1B (GLONASS/BeiDou L1) |
| **LNA** | SKY65404-21 (L1 band, 20 dB gain) |
| **RF Switch** | HMC253 (SP2T, DC-2.5 GHz) |
| **BLE Module** | nRF52840-M.2 (BLE 5.0, 1 Mbps, 2 Mbps, LR) |
| **TCXO** | TG5032CE-26N (26 MHz, 0.5 ppm stability) |
| **Fuel Gauge** | MAX17048 (I²C, LiPo battery monitoring) |
| **Charger** | TP4056 (USB-C charging, 1A max) |
| **Battery** | 3.7V LiPo 18650 (2000 mAh, ~4 hours operation) |
| **Flash** | 16 MB QSPI NOR (scenario/ephemeris storage) |
| **Connectivity** | BLE 5.0 (C2), USB-C (DFU/recovery), SMA (RF out) |
| **Sensors** | Battery voltage ADC, temperature (internal MCU) |
| **Indicators** | 4× LEDs (status, spoof, RX, power) |
| **Controls** | 3× buttons (mode, fire, safety) |
| **Form Factor** | 70 × 45 × 15 mm (pocket-sized) |
| **Weight** | ~60 g (with battery) |
| **BOM Cost** | ~$85 at qty 100 |

### RF Performance

| Parameter | Value |
|-----------|-------|
| **Frequency Range** | 1559–1610 MHz (L1 band) |
| **Max Output Power** | +20 dBm (100 mW) at SMA |
| **Min Output Power** | -30 dBm (1 µW) |
| **Power Adjustment** | 1 dB steps, BLE configurable |
| **Phase Noise** | <-90 dBc/Hz @ 10 kHz offset |
| **Spurious Emissions** | < -40 dBc (filtered) |
| **Frequency Accuracy** | ±0.5 ppm (TCXO) |
| **Code Rate** | 1.023 Mcps (GPS C/A) |
| **Modulation** | BPSK (direct synthesis) |
| **Max Simultaneous SVs** | 12 per constellation |
| **Nav Data Rate** | 50 bps (GPS/GLO), 250 bps (Galileo) |

### Power Budget

| Component | Current (TX) | Current (Idle) |
|-----------|-------------|----------------|
| STM32H743 @ 480 MHz | 80 mA | 35 mA |
| Si4463 ×2 (TX) | 85 mA × 2 = 170 mA | 3 mA × 2 |
| LNA + Switch | 15 mA | 0 mA |
| nRF52840 BLE | 8 mA | 0.1 mA |
| TCXO | 2 mA | 0 mA |
| LEDs | 5 mA | 2 mA |
| **Total (TX)** | **~280 mA** | **~42 mA** |
| **Battery Life (TX)** | **~7 hours** | **~48 hours idle** |

---

## 4. Architecture & Block Diagram

```
   ┌──────────────────────────────────────────────────────────────────┐
   │                     GNSS-Phantom GPX-1                           │
   │                                                                  │
   │  ┌──────────┐    ┌──────────┐    ┌─────────────────────┐       │
   │  │  USB-C   │───▶│ TP4056   │───▶│  LiPo 18650 2000mAh │       │
   │  │ Charging │    │ Charger  │    │  3.7V               │       │
   │  └──────────┘    └──────────┘    └─────────┬───────────┘       │
   │                                             │                   │
   │  ┌──────────┐                     ┌────────▼─────────┐         │
   │  │ MAX17048 │◀─── I²C ────────────▶│  STM32H743VIT6   │         │
   │  │ Fuel Ga. │                     │  Cortex-M7 480MHz │         │
   │  └──────────┘                     │                   │         │
   │                                     │  ┌─────────────┐ │         │
   │  ┌──────────┐    USART2            │  │ GNSS Engine │ │         │
   │  │ nRF52840 │◀─── C2 ─────────────▶│  │ C/A Gen     │ │         │
   │  │ BLE 5.0  │                     │  │ Nav Data    │ │         │
   │  └──────────┘                     │  └──────┬──────┘ │         │
   │                                     │         │        │         │
   │  ┌──────────┐    SPI2               └─────────┼────────┘         │
   │  │ 26 MHz   │─── REF ──────────────┐           │                 │
   │  │ TCXO     │                      │           │                 │
   │  └──────────┘                      ▼           ▼                 │
   │  ┌──────────┐    ┌──────────┐  ┌──────────┐  ┌──────────┐       │
   │  │ Si4463   │◀──│  LNA     │  │ Si4463   │  │ RF Switch│       │
   │  │ GPS/Gal  │──▶│ SKY65404 │  │ GLO/B1   │  │ HMC253   │       │
   │  └──────────┘    └──────────┘  └──────────┘  └────┬─────┘       │
   │                                                      │             │
   │                                              ┌───────▼──────┐     │
   │                                              │  SMA RF Out   │     │
   │                                              │  (Antenna)    │     │
   │                                              └──────────────┘     │
   └──────────────────────────────────────────────────────────────────┘
```

### Data Flow

```
Companion App (React Native)
    │ BLE 5.0
    ▼
nRF52840 (BLE Bridge) ── UART (230400 baud) ──▶ STM32H743
                                                    │
                                    ┌───────────────┼───────────────┐
                                    ▼               ▼               ▼
                              GNSS Engine      SPI2 (to RF)    Safety Logic
                              (C/A + Nav)     (1.023 Mbps)    (Interlocks)
                                    │               │               │
                                    └───────┬───────┘               │
                                            ▼                       │
                                     Si4463 ×2 (TX)                  │
                                     (L1 BPSK)                       │
                                            │                       │
                                            ▼                       │
                                     LNA → Switch → SMA ◀───────────┘
```

---

## 5. Firmware Architecture

### Design Decisions

1. **Direct Synthesis vs. Upconversion:** We use the Si4463's direct TX mode to synthesize BPSK-modulated C/A code + nav data directly at L1 frequency. This eliminates the need for a separate baseband DAC + mixer, reducing cost and complexity at the expense of spectral purity.

2. **Two Si4463 Transceivers:** GPS/Galileo share L1 (1575.42 MHz) while GLONASS uses 1602 MHz and BeiDou uses 1561.098 MHz. Two transceivers allow simultaneous multi-constellation spoofing without retuning.

3. **26 MHz TCXO Shared:** The TCXO feeds both the STM32H743 HSE input and both Si4463 XI inputs, ensuring coherent timing between the MCU sample generation and RF synthesis.

4. **C/A Code Generation:** The firmware implements two 10-bit LFSRs (G1 and G2) per GPS ICD-200, with G2 phase selection for all 32 PRN codes. The code is generated on-the-fly (not stored) to save flash.

5. **Navigation Data:** A simplified but structurally valid GPS navigation message is generated with proper TLM/HOW words, parity, and ephemeris-like data. Real ephemeris can be loaded via BLE from the companion app.

6. **Safety Interlocks:** Three independent safety mechanisms (physical switch, fire-button hold, C2 arm command) must all be satisfied before transmission begins. Auto-disarm after 5 minutes prevents accidental prolonged operation.

7. **BLE C2 Protocol:** A custom SLIP-framed, CRC-16/CCITT-protected protocol over Nordic UART Service provides reliable command/response between the app and device.

### File Structure

```
firmware/
├── Makefile                    # ARM GCC cross-compile build
├── main.c                      # Entry point: clock, GPIO, SPI, UART, main loop
├── board.h                     # Hardware pin assignments, constants, C2 opcodes
├── registers.h                 # Full MMIO register map (STM32H743 peripherals)
├── usb_descriptors.h           # USB CDC descriptors for recovery/DFU mode
└── drivers/
    ├── si4463.h                # Si4463 RF transceiver API
    ├── si4463.c                # SPI driver, freq synth, TX FIFO management
    ├── gnss_engine.h           # GNSS spoofing signal generator API
    ├── gnss_engine.c           # C/A code LFSR, nav data, BPSK sample gen
    ├── c2_protocol.h           # BLE C2 command/response protocol API
    └── c2_protocol.c           # CRC-16, SLIP framing, DLE escaping
```

### Key Algorithms

#### C/A Code Generation (GPS ICD-200)

```
G1: x¹⁰ + x³ + 1          (10-bit LFSR, taps at bit 10 and bit 3)
G2: x¹⁰ + x⁹ + x⁸ + x⁶ + x³ + x² + 1  (taps at bits 2,3,6,8,9,10)

PRN_n(t) = G1(t) XOR G2(t + delay[n])

delay[] = {5, 6, 7, 8, 17, 18, 139, 140, 141, 5, 6, 15, 62, 155, 156, 157,
           118, 24, 25, 26, 57, 92, 132, 140, 15, 42, 89, 99, 124, 126, 127, 28}
```

#### Navigation Message

```
Frame: 1500 bits (30 sec @ 50 bps)
  ├── 5 Subframes × 300 bits (6 sec each)
  │   ├── 10 Words × 30 bits
  │   │   ├── Preamble: 10001011 (8 bits)
  │   │   ├── Data: 24 bits (TLM/HOW/ephemeris)
  │   │   └── Parity: 6 bits (Hamming code)
  │   ├── Subframe 1: Clock correction, week number, SV health
  │   ├── Subframe 2: Ephemeris (precise orbit)
  │   └── Subframe 3: Ephemeris (continued)
```

#### BPSK Modulation

```
TX bit = C/A_code_chip(t) XOR navigation_data_bit(t)

Each nav bit spans 20 ms = 20460 chips
Output rate: 1.023 Mbps packed into SPI FIFO
```

---

## 6. Companion Application

### Architecture

The companion app is built in **React Native** for cross-platform (Android/iOS) support. It communicates with the device via BLE 5.0 using the Nordic UART Service (NUS).

### Screens

| Screen | Function |
|--------|----------|
| **Dashboard** | Connection status, engine state, battery, safety interlock, arm/disarm/start/stop controls |
| **Satellites** | Add/remove simulated SVs, set constellation, PRN, power offset |
| **Trajectory** | Set spoofed lat/lon/alt, GPS week/TOW, location presets |
| **RF Settings** | Carrier frequency, TX power slider, RF chain info |

### Protocol

The app implements the same C2 protocol as the firmware (SLIP framing, CRC-16/CCITT, DLE escaping), ensuring reliable communication over lossy BLE links.

### File Structure

```
app/
├── package.json               # React Native dependencies
├── App.js                      # Navigation setup, BLE connection manager
├── screens/
│   ├── DashboardScreen.js      # Main dashboard with safety interlock
│   ├── SatelliteScreen.js      # SV configuration (constellation, PRN, power)
│   ├── TrajectoryScreen.js     # Position/time simulation
│   └── RFSettingsScreen.js     # RF frequency and power settings
├── components/
│   ├── StatusIndicator.js      # Reusable LED-style status indicator
│   └── SafetyInterlock.js      # Multi-step safety confirmation dialog
└── utils/
    ├── protocol.js             # C2 protocol encode/decode, command builders
    └── ble.js                  # BLE manager (scan, connect, send, receive)
```

---

## 7. Use Cases

### Red Team Operations

1. **Drone Diversion Testing:** Demonstrate that a target drone's GPS receiver can be spoofed to change its perceived position, causing it to deviate from its intended flight path. This reveals the vulnerability to clients and motivates investment in anti-spoofing countermeasures.

2. **Vehicle Telematics Spoofing:** Test fleet management systems' GPS tracking to show that vehicle positions can be faked, enabling theft or route manipulation. This is critical for logistics companies assessing their security posture.

3. **Geofencing Bypass:** Evaluate whether geofenced systems (e.g., drone no-fly zones, speed-limited electric scooters, rental car geofences) can be bypassed by spoofing the GPS position.

### Security Research

4. **Anti-Spoofing Evaluation:** Test receiver-level anti-spoofing countermeasures such as:
   - Signal power monitoring (spoofing signals are typically stronger than authentic)
   - Multi-antenna CRPA (controlled reception pattern antenna) null steering
   - IMU/GNSS cross-checking (detect inconsistency between inertial and GPS position)
   - Authentication-enhanced signals (Galileo OSNMA, GPS Chimera)

5. **Timing Attack Research:** GNSS timing is used for PTP/NTP synchronization in telecom, finance, and power grid. Spoofing the timing signal can desynchronize critical infrastructure. GNSS-Phantom enables controlled timing offset injection for resilience testing.

6. **Academic Publication:** Researchers can use GNSS-Phantom to publish reproducible studies on GNSS vulnerabilities and countermeasures, advancing the field of PNT (Positioning, Navigation, and Timing) security.

### Penetration Testing

7. **Asset Tracker Assessment:** Test GPS tracking devices used in logistics, wildlife monitoring, and high-value asset tracking. Show that the reported position can be manipulated.

8. **Smart City Infrastructure:** Evaluate GNSS-dependent smart city systems (traffic monitoring, emergency vehicle dispatch, timing for synchronized traffic lights).

9. **Maritime Navigation:** Test ship AIS/GPS systems for vulnerability to position spoofing — relevant for port security and maritime critical infrastructure.

---

## 8. Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Build firmware
cd gnss-phantom/firmware
make CROSS_COMPILE=arm-none-eabi- clean all

# Flash via OpenOCD + ST-Link V3
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
    -c "program gnss-phantom.hex verify reset exit"
```

### Companion App

```bash
cd gnss-phantom/app
npm install
npx react-native run-android   # or run-ios
```

### Operation

1. Power on the device (hold MODE button during power-on for USB recovery mode)
2. Open the companion app and scan for "GNSS-Phantom" BLE device
3. Connect to the device
4. Configure satellites (constellation, PRN, power)
5. Set spoofed trajectory (lat/lon/alt, GPS time)
6. Set RF parameters (frequency, TX power)
7. **Engage physical safety switch** (LIVE position)
8. **Hold FIRE button for 3 seconds** to arm
9. Confirm safety interlock in app (4-step confirmation)
10. Press **START SPOOFING** in the app
11. Monitor status (state, battery, frame count) on dashboard
12. Press **STOP SPOOFING** or **DISARM** to cease transmission

---

## 9. KiCad Design Files

The `kicad/` directory contains:

| File | Description |
|------|-------------|
| `gnss-phantom.kicad_pro` | KiCad 8 project with net classes (Default, RF, Power) |
| `gnss-phantom.kicad_sch` | Full schematic: MCU, 2× Si4463, nRF52840, TCXO, LNA, RF switch, fuel gauge, charger, USB-C, SMA, LEDs, decoupling |
| `gnss-phantom.kicad_pcb` | 4-layer PCB (70×45mm): RO4350B/FR4 hybrid stackup, GND/+3V3 planes, component placement, routing |

### PCB Stackup

```
Layer 1 (F.Cu):   RF signals + component pads (35 µm copper)
Layer 2 (In1.Cu): Solid GND plane (35 µm copper)
Layer 3 (In2.Cu): +3V3 power plane (35 µm copper)
Layer 4 (B.Cu):   Routing + GND flood (35 µm copper)

Dielectric 1: Rogers RO4350B, 0.2 mm (RF substrate)
Dielectric 2: FR4 core, 0.71 mm
Dielectric 3: Rogers RO4350B, 0.2 mm
Finish: ENIG
```

---

## 10. Bill of Materials (Summary)

| Ref | Component | MPN | Qty | Est. Cost |
|-----|-----------|-----|-----|-----------|
| U1 | STM32H743VIT6 | STM32H743VIT6 | 1 | $18.00 |
| U2, U3 | Si4463 RF Transceiver | Si4463-B1B | 2 | $8.50 |
| U4 | nRF52840-M.2 Module | nRF52840-M.2 | 1 | $12.00 |
| U5 | MAX17048 Fuel Gauge | MAX17048G+T10 | 1 | $2.50 |
| U6 | HMC253 RF Switch | HMC253AGQS24 | 1 | $8.00 |
| U7 | SKY65404 LNA | SKY65404-21 | 1 | $3.50 |
| U8 | TP4056 Charger | TP4056 | 1 | $0.50 |
| Y1 | 26 MHz TCXO | TG5032CE-26N | 1 | $3.00 |
| J1 | SMA Edge Mount | 132289 | 1 | $2.00 |
| J2 | USB-C Receptacle | USB4105 | 1 | $1.50 |
| — | 18650 LiPo Battery | — | 1 | $5.00 |
| — | Passives (R, C, L) | — | ~40 | $5.00 |
| — | PCB (4-layer hybrid) | — | 1 | $15.00 |
| **Total** | | | | **~$85.00** |

---

## 11. Comparison to Existing Devices

| Feature | GNSS-Phantom | Commercial GNSS Sim (e.g., Spirent GSS) | HackRF One |
|---------|-------------|----------------------------------------|------------|
| Price | ~$85 | $10,000+ | ~$300 |
| GNSS Spoofing | ✅ (native) | ✅ (professional) | ⚠ (needs software) |
| Multi-Constellation | ✅ (GPS/Gal/GLONASS/BeiDou) | ✅ | ⚠ (needs S/W) |
| Portability | Pocket-sized (70×45mm) | Benchtop | USB dongle |
| Battery | ✅ (4-7 hours) | ❌ (mains) | ❌ (USB powered) |
| BLE C2 | ✅ | ❌ | ❌ |
| Safety Interlocks | ✅ (triple) | ✅ | ❌ |
| C/A Code Gen | ✅ (hardware LFSR) | ✅ | ⚠ (software) |
| Nav Data Gen | ✅ | ✅ | ⚠ (software) |
| Form Factor | Standalone | Lab equipment | SDR |

---

## 12. Limitations & Future Work

### Current Limitations

1. **Single-band:** Only L1 frequency — L2/L5 spoofing not supported
2. **Galileo CBOC:** Simplified BOC modulation (full CBOC would need higher sample rate)
3. **Spectral Purity:** Direct synthesis has higher spurious emissions compared to professional vector signal generators
4. **No RX Mode:** The current design is TX-only (RX mode for calibration would need firmware additions)
5. **Ephemeris Source:** Uses simplified ephemeris — real ephemeris must be loaded from external source (e.g., NASA CDDIS)

### Future Enhancements

1. **L2/L5 Support:** Add a second RF chain for L2 (1227.6 MHz) and L5 (1176.45 MHz)
2. **CRPA Spoofing:** Multi-antenna output for testing CRPA null-steering countermeasures
3. **Real-Time Ephemeris:** Integrate a GNSS receiver chip to extract real ephemeris from authentic signals
4. **Scenario Recording:** Add RX mode to capture authentic GNSS signals for replay attacks
5. **Encrypted C2:** AES-256 encrypted BLE C2 channel to prevent unauthorized control
6. **SD Card Logging:** Log all transmissions for audit trail compliance

---

## Author

**jayis1** — Security Researcher & Hardware Developer  
© 2026 jayis1. All rights reserved.

---

*"GNSS is the most critical yet most vulnerable infrastructure you've never thought about." — jayis1*