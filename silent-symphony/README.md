# SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator

**A portable, self-contained hardware platform for ultrasonic covert channel research, air-gap exfiltration testing, and RF-silent tactical communication for security professionals.**

```
╔═══════════════════════════════════════════════════════╗
║  ___ _ _   ___   ___  ___  _  _  ___  _   _  ___     ║
║ / __| | | | _ \\ / _ \\| _ \\| \\| |/ __|| | | ||_  )    ║
║ \\__ \\ |_| |  _/ \\_,_ |   /| .` |\\__ \\| |_| | / /     ║
║ |___/\\___/ |_|   /_/ |_|_\\|_|\\_||___/ \\___/ /___|    ║
║ Ultrasonic Covert Channel Communicator               ║
╚═══════════════════════════════════════════════════════╝
```

Author: **jayis1**
License: Hardware — CERN-OHL-S v2, Firmware — GPL-2.0, App — MIT

---

## Table of Contents

1. [Overview & Problem Statement](#overview--problem-statement)
2. [Attack Surface & Threat Model](#attack-surface--threat-model)
3. [Hardware Specifications](#hardware-specifications)
4. [Architecture & Block Diagram](#architecture--block-diagram)
5. [Theory of Operation — Ultrasonic Covert Channels](#theory-of-operation--ultrasonic-covert-channels)
6. [Firmware Details & Design Decisions](#firmware-details--design-decisions)
7. [Modulation Schemes & Protocol](#modulation-schemes--protocol)
8. [Application/Software Interface](#applicationsoftware-interface)
9. [Use Cases for Red Teams & Pentesters](#use-cases-for-red-teams--pentesters)
10. [Legal & Ethical Disclaimer](#legal--ethical-disclaimer)

---

## Overview & Problem Statement

Air-gapped networks — systems physically disconnected from the internet — have long been considered the gold standard for securing classified, industrial, and critical infrastructure data. Yet over the past decade, academic research has repeatedly demonstrated that air-gap isolation can be defeated through **covert channels** that exploit side-effects of physical hardware: electromagnetic radiation (TEMPEST), power line modulation, thermal emissions, and critically — **ultrasonic acoustic waves**.

The SILENT-SYMPHONY is purpose-built for researching, testing, and demonstrating these ultrasonic covert channels. It is the first open-source hardware platform that combines both **transmit** and **receive** capabilities for ultrasonic covert channel research in a single, battery-powered, handheld form factor.

Unlike general-purpose SDRs (which are bulky, expensive, and require host computers), or software-only solutions (which are limited by consumer hardware audio bandwidth), SILENT-SYMPHONY provides:

- **Dedicated ultrasonic transducers** optimized for 18–45 kHz operation
- **Real-time DSP** on an STM32H743 Cortex-M7 with hardware floating point
- **Standalone operation** — no laptop required during field use
- **Multiple modulation schemes** (FSK, OOK, DSSS-like spread spectrum)
- **Configurable power levels** from whisper-covert to full-range
- **Secure BLE link** for stealthy remote control via the companion app
- **8 MB capture buffer** for recording and later analysis of ultrasonic signals

The device brings to life attack vectors that have been demonstrated in peer-reviewed research:

| Research Group | Year | Vector | Range | Data Rate |
|---|---|---|---|---|
| Ben-Gurion University (Mordechai Guri) | 2016–2024 | Ultrasonic between air-gapped computers | 20 m | 1–20 bps |
| Fraunhofer FKIE | 2019 | Ultrasonic covert channel via speakers | 15 m | 3 bps |
| UCL & Cambridge | 2021 | Inaudible voice command injection | 5 m | N/A (commands) |
| Georgia Tech | 2022 | Ultrasonic cross-device tracking | 10 m | Beacon only |

---

## Attack Surface & Threat Model

### Attacker Capabilities

The SILENT-SYMPHONY is designed to model an **advanced adversary** who:

- Has physical access to a facility for placement or retrieval of the device
- Can operate in RF-hostile environments where radios are detected by spectrum monitoring
- Targets air-gapped systems with standard consumer speakers and microphones
- May operate in Faraday-shielded environments (ultrasonics penetrate where RF cannot)
- Has brief windows for device placement (minutes, not hours)

### Attack Scenarios

| Scenario | Description | Device Role |
|---|---|---|
| **Air-Gap Exfiltration** | Data is modulated as ultrasonic tones from a compromised system's speakers, captured by SILENT-SYMPHONY up to 20 m away | Receiver |
| **Covert Drop** | Device is placed in a secure area; operator sends data via BLE → device transmits ultrasonically to a nearby receiver | Transmitter |
| **Acoustic Beaconing** | Device emits periodic ultrasonic chirps for asset tracking without RF signature | Beacon |
| **Voice Interface Injection** | Device transmits inaudible ultrasonic commands that a voice assistant's microphone demodulates as valid voice commands (DolphinAttack vector) | Transmitter |
| **Facility Acoustic Profiling** | Device emits ultrasonic pulses and measures reflections to map room geometry undetected | Radar/Sonar |
| **Cross-Device Tracking** | Device emits unique ultrasonic fingerprints that advertising microphones in retail/office environments demodulate | Beacon |

### Defensive Measures the Device Helps Test

- Ultrasonic jamming and filtering on sensitive systems
- Acoustic enclosures and speaker isolation
- Microphone physical disconnect switches
- Ultrasonic detector deployment strategies
- Envelope detection and spectral analysis for forensics

---

## Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32H743VIT6 — ARM Cortex-M7 @ 480 MHz, DP-FPU, 2 MB Flash, 1 MB SRAM |
| **Ultrasonic Transmitter** | 2× Murata MA40S4S piezo tweeters (resonant: 40 kHz, usable: 20–45 kHz, 120 dB SPL @ 10V) |
| **Ultrasonic Receiver** | 2× Knowles SPU0410LR5H-QB-7 MEMS microphones (10 Hz–45 kHz ±3 dB) |
| **Amplifier (Tx)** | MAX98357A class-D amplifier (3W into 4Ω, I²S input) |
| **Pre-amplifier (Rx)** | AD8694 quad rail-to-rail op-amp (10 MHz GBW, low noise 8 nV/√Hz) |
| **ADC** | STM32H743 SAR1 (3× simultaneous 12-bit @ 3.6 MSPS, 16-channel) |
| **DAC** | STM32H743 DAC1 (2× 12-bit @ 1 MSPS) |
| **Audio Codec** | WM8731S I²S codec (24-bit DAC/ADC, 8–96 kHz sample rate) — bridges I²S to microphone pre-amp chain |
| **BLE Module** | nRF52840 (ARM Cortex-M4F, BLE 5.2, NFC-A) via UART + SPI passthrough |
| **Storage** | 16 MB W25Q128JV QSPI flash (firmware + capture buffer), 8 MB IS66WVH8M8DBLI PSRAM |
| **Power Management** | TP4056 LiPo charger + SGM6603-5.0YN5G boost converter (5V rail), RT9013-3.3 LDO |
| **Battery** | 800 mAh LiPo pouch cell (3.7 V) — ~4 hours active, ~24 hours standby |
| **Host Interface** | USB-C 2.0 FS (CDC-ACM virtual serial + DFU for firmware updates) |
| **Expansion** | 10-pin 1.27mm JTAG/SWD header, 6-pin UART breakout, 2× GPIO pins |
| **Antennas** | nRF52840 integrated PCB trace antenna (BLE) |
| **Indicators** | RGB LED (status), 3× yellow LEDs (Tx/Rx/Beacon), 1× red LED (fault) |
| **Buttons** | Power button, Mode selector (3-position rotary), Capture button, Reset button |
| **Enclosure** | 3D-printed PLA/polycarbonate (70×40×15 mm) with 2× ultrasonic horn openings |
| **BOM Cost** | ~$62 @ 1K units |

---

## Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        SILENT-SYMPHONY                              │
│  ┌──────────┐  ┌─────────────────────────────────────────────────┐  │
│  │  Battery  │  │               STM32H743VIT6                    │  │
│  │  800mAh   │  │  ┌────────────────────┐  ┌─────────────────┐  │  │
│  │   LIPo    │  │  │  Arm Cortex-M7     │  │    Flash 2MB    │  │  │
│  │   3.7V    │  │  │  480 MHz w/ DP-FPU │  │    SRAM 1MB     │  │  │
│  └────┬──────┘  │  └─────────┬──────────┘  └────────┬────────┘  │  │
│       │         │            │                       │            │  │
│  ┌────┴──────┐  │  ┌────────┴───────────────────────┴────────┐  │  │
│  │  TP4056   │  │  │         Internal Buses                   │  │  │
│  │  Charger  │  │  │  AXI @ 400MHz  |  APB1 @ 120MHz          │  │  │
│  └────┬──────┘  │  │  APB2 @ 120MHz  |  AHB @ 240MHz          │  │  │
│       │         │  └────────┬──────────┬──────────┬──────────┘  │  │
│  ┌────┴──────┐  │           │          │          │              │  │
│  │  SGM6603  │  │     ┌────┴────┐ ┌───┴────┐ ┌───┴────┐        │  │
│  │  5V Boost │  │     │  I2S1   │ │ I2S3   │ │  QUAD  │        │  │
│  └────┬──────┘  │     │  Tx DAC │ │ Rx ADC │ │  SPI   │        │  │
│       │         │     └────┬────┘ └───┬────┘ └───┬────┘        │  │
│       │         │          │          │          │              │  │
│  ┌────┴──────┐  │     ┌────┴────┐ ┌───┴────┐ ┌───┴──────┐      │  │
│  │  RT9013   │  │     │WM8731S  │ │WM8731S │ │ W25Q128  │      │  │
│  │  3.3V LDO │  │     │ CODEC   │ │ CODEC  │ │ QSPI     │      │  │
│  │  + 1.8V   │  │     │  DAC->  │ │  ADC<- │ │ 16MB     │      │  │
│  └───────────┘  │     └────┬────┘ └───┬────┘ └──────────┘      │  │
│                 │          │          │                          │  │
│           ┌─────┴────┐    │    ┌──────┴──────────┐               │  │
│           │ nRF52840  │    │    │  AD8694 Pre-Amp │               │  │
│           │ BLE 5.2   │    │    │  × 2 channels   │               │  │
│           │ Module     │    │    │  + Bandpass     │               │  │
│           └─────┬─────┘    │    └──────┬──────────┘               │  │
│                 │          │           │                          │  │
│      ┌──────────┴──┐  ┌───┴────────┐  │                          │  │
│      │ PCB Trace   │  │ MAX98357A  │  │                          │  │
│      │ BLE Antenna │  │ Class-D    │  │                          │  │
│      └─────────────┘  │ Amp 3.2W   │  │                          │  │
│                       └───┬────────┘  │                          │  │
│                           │           │                          │  │
│                    ┌──────┴──────┐ ┌──┴──────────────┐            │  │
│                    │ Murata      │ │ Knowles SPU0410 │            │  │
│                    │ MA40S4S     │ │ MEMS Mic × 2    │            │  │
│                    │ Piezo × 2   │ │ + Pre-Amp       │            │  │
│                    │ 20-45kHz    │ │ 10-45kHz        │            │  │
│                    └─────────────┘ └─────────────────┘            │  │
│                                                                     │  │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │  Power Tree                                                    │  │
│  │  ┌──────┐   ┌───────┐   ┌────────┐   ┌──────────────┐       │  │
│  │  │LiPo  ├──►│TP4056 ├──►│SGM6603 ├──►│5V Rail       │       │  │
│  │  │3.7V  │   │4.2V   │   │5.0V    │   │(MAX98357A)   │       │  │
│  │  └──────┘   └───────┘   └───┬────┘   └──────────────┘       │  │
│  │                              ├───────►│RT9013-3.3  ├──►3.3V rail │  │
│  │                              │        └────────────┘       │  │
│  │                              ├───────►│RT9013-1.8  ├──►1.8V rail │  │
│  │                                       └────────────┘       │  │
│  └────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────┘
```

### Signal Chain — Transmitter

```
USB/BLE Command → STM32H7 Message Encode → FSK/OOK Modulator
    → I²S DAC → WM8731S → I²S Output → MAX98357A Class-D Amp
    → Murata MA40S4S Piezo Speaker → 18-45 kHz Acoustic Wave
```

### Signal Chain — Receiver

```
Knowles SPU0410 MEMS Mic → AD8694 Bandpass Filter & Pre-Amp (×20)
    → WM8731S ADC → I²S → STM32H7 Digital Down-Convert
    → FSK/OOK Demodulator → Error Correction → Message → USB/BLE Output
```

---

## Theory of Operation — Ultrasonic Covert Channels

### Why Ultrasonics?

Ultrasonic frequencies (above ~17 kHz) are inaudible to most human hearing but can be produced by standard consumer speakers and captured by standard MEMS microphones found in laptops, phones, and smart speakers. Research has shown:

- **Consumer laptop speakers** produce usable output up to 24 kHz
- **Smartphone microphones** capture signals up to 24 kHz (48 kHz sample rate)
- **Professional equipment** extends to 48 kHz (96 kHz sample rate)
- **Ultrasonic waves** penetrate thin walls and are not stopped by Faraday cages
- **Standard acoustic monitoring** in secure facilities rarely filters above 16 kHz

### Modulation Schemes

#### FSK (Frequency Shift Keying) — Primary Mode

Two ultrasonic tones carry binary data:
- **Mark (1):** 21.0 kHz (tone A)
- **Space (0):** 19.5 kHz (tone B)
- **Baud rate:** 1–50 symbols/second, user-configurable
- **Bit encoding:** NRZ or Manchester (selectable)
- **Sync preamble:** 10 cycles at 22 kHz + 4-bit Barker code
- **Error detection:** CRC-16-IBM per 32-byte frame

#### OOK (On-Off Keying) — Low-Complexity Mode

- **Mark (1):** 20 kHz carrier ON (configurable duration)
- **Space (0):** Carrier OFF (same duration)
- **Simpler to demodulate** but less robust against noise
- **Best for whisper mode** at very low power

#### DSSS-like Spread Spectrum — Whisper Mode

- Carrier frequency hops across 19–23 kHz in pseudo-random sequence
- 7-chip Barker sequence per symbol for processing gain
- **Estimated SNR improvement:** ~8.5 dB
- **Data rate:** 1–5 bps
- **Range:** ~3 m with high reliability

### Protocol Frame Format

```
┌─────────┬─────────┬──────────┬──────────┬─────────┬──────────────┐
│ Preamble│ Sync    │ Length   │ Payload  │ CRC-16  │ Padding      │
│ 22 kHz  │ Barker  │ (1 byte) │ (0-64 B) │ (2 B)   │ (to 4 B)     │
│ 10 cyc   │ 4-bit   │          │          │         │              │
└─────────┴─────────┴──────────┴──────────┴─────────┴──────────────┘
```

**Total frame size:** 3–72 bytes (variable)
**Transmission time at 10 bps:** 2.4–57.6 seconds
**Transmission time at 50 bps:** 0.48–11.5 seconds

### Coexistence & Collision Avoidance

- **Channel sensing:** Device listens for 500 ms before transmitting
- **Random backoff:** 100–500 ms jitter window
- **ACK/NACK protocol:** Reliable mode requires recipient ACK
- **Frequency agility:** On persistent collision, device shifts ±500 Hz

---

## Firmware Details & Design Decisions

### Firmware Architecture

The firmware follows a **layered, interrupt-driven architecture**:

```
┌────────────────────────────────────────────────────┐
│  Application Layer                                 │
│  ┌──────────┐ ┌──────────┐ ┌────────────────────┐  │
│  │ Command  │ │ Message  │ │  Capture Buffer    │  │
│  │ Parser   │ │ Queue    │ │  Manager           │  │
│  └────┬─────┘ └────┬─────┘ └─────────┬──────────┘  │
├───────┴────────────┴──────────────────┴────────────┤
│  Protocol Layer                                    │
│  ┌────────────────┐ ┌────────────────────────────┐  │
│  │ FSK Modulator  │ │ FSK Demodulator            │  │
│  │ /Demodulator   │ │ (Goertzel + PLL tracking)  │  │
│  └────────┬───────┘ └────────┬───────────────────┘  │
│  ┌────────┴──────────────────┴────────────────┐     │
│  │  OOK Modulator/Demodulator (fallback)       │     │
│  └─────────────────────────────────────────────┘     │
├─────────────────────────────────────────────────────┤
│  DSP Layer (CMSIS-DSP + custom)                     │
│  ┌──────────┐ ┌──────────┐ ┌─────────────────────┐ │
│  │ Goertzel │ │ PLL      │ │ FIR/IIR Filters     │ │
│  │ Detector │ │ Tracking │ │ (Bandpass 18-45kHz) │ │
│  └──────────┘ └──────────┘ └─────────────────────┘ │
├─────────────────────────────────────────────────────┤
│  HAL Layer (CMSIS-HAL + custom)                     │
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐      │
│  │I2S │ │I2S │ │QSPI│ │DMA │ │TIM │ │UART│      │
│  │Tx  │ │Rx  │ │    │ │    │ │    │ │    │      │
│  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘      │
└─────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **STM32H743 over ESP32 or Cortex-M4:** The STM32H7's Cortex-M7 with double-precision FPU at 480 MHz provides the DSP horsepower needed for real-time Goertzel algorithm processing across multiple frequency bins. ESP32's FFT hardware is convenient but limiting for custom Goertzel designs. The 1 MB of SRAM allows double-buffered I²S capture.

2. **Dual I²S buses (I2S1 Tx, I2S3 Rx):** Separating transmit and receive audio paths avoids timing conflicts and allows independent sample rates. Tx runs at 96 kHz (needed for 45 kHz ultrasonic output per Nyquist), Rx at 192 kHz (for oversampling and better demodulation SNR).

3. **WM8731S codec as I²S bridge:** Rather than routing microphone pre-amp directly to the STM32 ADC (which has limited resolution for ultrasonic signals), the WM8731S provides 24-bit sigma-delta conversion with programmable filters, giving superior SNR for weak ultrasonic signals.

4. **Goertzel algorithm over FFT:** For the target application, we only need to detect 2–4 specific frequency bins per demodulation. Goertzel is O(N) per bin vs. O(N log N) for FFT, and uses less RAM. At 192 kHz with 1024-sample windows, we get ~187 Hz frequency resolution — sufficient for FSK discrimination.

5. **External QSPI flash for capture buffer:** 16 MB gives ~43 seconds of 192 kHz 16-bit mono audio, or ~340 seconds at the effective demodulated data rate. This allows unattended operation where the device captures, stores, and forwards data on the next BLE sync.

6. **nRF52840 for BLE rather than integrated STM32 BLE:** The STM32H743 does not include BLE. Adding the nRF52840 as a co-processor provides a proven BLE stack, separates radio interference from audio processing, and allows the nRF to handle sleep/standby power management independently.

### Sample Rates & Buffer Management

| Stream | Sample Rate | Bit Depth | Buffer Size | DMA Mode |
|---|---|---|---|---|
| I²S Tx (playback) | 96 kHz | 16-bit | 512 samples × 2 (ping-pong) | Double-buffer, circular |
| I²S Rx (capture) | 192 kHz | 16-bit | 1024 samples × 2 (ping-pong) | Double-buffer, circular |
| ADC (monitoring) | 50 kHz | 12-bit | 256 samples | Single, triggered |
| DAC (debug output) | 48 kHz | 12-bit | 256 samples | Single, triggered |

### Interrupt Priorities

| Interrupt | Priority | Purpose |
|---|---|---|
| I²S Rx DMA Half-Transfer | 0 (highest) | Process first half of capture buffer |
| I²S Rx DMA Transfer Complete | 0 | Process second half of capture buffer |
| I²S Tx DMA Half-Transfer | 1 | Fill next transmit buffer |
| I²S Tx DMA Transfer Complete | 1 | Fill next transmit buffer |
| TIM3 (Demodulation Timer) | 2 | Symbol timing for demodulator |
| TIM4 (Modulation Timer) | 2 | Symbol timing for modulator |
| UART5 (BLE module) | 3 | Command/response with nRF52840 |
| USB LP | 4 | USB CDC-ACM data |

---

## Modulation Schemes & Protocol

### FSK Demodulation via Goertzel Algorithm

The Goertzel algorithm computes the energy at specific frequencies without a full FFT:

```
Q0 = s[n] + 2·cos(2π·k/N)·Q1 − Q2
where k = N·f_target / f_sample
```

For each received I²S buffer (1024 samples @ 192 kHz):

1. Compute Goertzel energy at f_mark (21.0 kHz) and f_space (19.5 kHz)
2. Select the tone with higher energy → bit decision
3. After 2 samples per bit at our rate... actually at N=1024 and 192 kHz sample rate, each measurement window is ~5.3 ms. At 50 bps (20 ms/bit), we do ~3.7 measurements per bit — majority vote for robustness.
4. Track PLL phase error to maintain symbol synchronization

### OOK Demodulation

- Envelope detection via sliding RMS window (128 samples @ 192 kHz ~ 0.67 ms)
- Threshold comparison with adaptive baseline (20-sample rolling average of noise floor)
- Hysteresis: 3 dB above threshold for ON, 6 dB below for OFF (prevents flutter)

### Error Mitigation

| Technique | Benefit |
|---|---|
| Manchester encoding | DC balance, clock recovery, 3 dB coding gain |
| CRC-16-IBM per frame | 99.99% detection of errors in 64-byte payload |
| 3-of-5 majority voting per symbol | √³ reduction in single-bit errors |
| Automatic Repeat Query (ARQ) | Retransmission on CRC failure |
| Interleaving (32-bit depth) | Burst error resiliency |
| Adaptive rate fallback | 50→25→10→5 bps on repeated failures |

---

## Application/Software Interface

### BLE Command Protocol

The companion app communicates with SILENT-SYMPHONY over BLE 5.2 using the nRF52840 as a UART-to-BLE bridge. The protocol is packet-based:

| Byte 0 | Bytes 1-2 | Bytes 3-4 | Bytes 5... | Last 2 |
|---|---|---|---|---|
| Start (0xAA) | Length (LE) | Command ID | Payload | CRC-16 |

### Command Set

| Command ID | Name | Direction | Description |
|---|---|---|---|
| 0x01 | GET_STATUS | → Device | Request device status |
| 0x02 | STATUS_RESP | → App | Battery %, mode, signal quality, storage used |
| 0x03 | SET_MODE | → Device | Switch between: IDLE, TX, RX, BEACON, SCAN |
| 0x04 | SET_FSK_PARAMS | → Device | Configure mark/space frequencies, baud rate, power |
| 0x05 | SET_OOK_PARAMS | → Device | Configure carrier freq, bit duration, power |
| 0x06 | TX_MESSAGE | → Device | Send a message payload (1-64 bytes) |
| 0x07 | TX_INTERLEAVED | → Device | Send with interleaving for burst resilience |
| 0x10 | RX_START | → Device | Begin capture mode |
| 0x11 | RX_STOP | → Device | Stop capture |
| 0x12 | RX_DATA | → App | Captured/demodulated data |
| 0x13 | RX_SPECTRUM | → App | 256-point power spectrum of current environment |
| 0x14 | RX_SIGNAL_QUALITY | → App | SNR estimate from preamble, freq offset estimate |
| 0x20 | BEACON_SET | → Device | Configure beacon interval, message, power |
| 0x21 | BEACON_DATA | → Device | Set beacon message payload |
| 0x30 | SET_POWER | → Device | Set Tx power: LOW (1V), MED (3.3V), HIGH (5V), WHISPER |
| 0x31 | SET_GAIN | → Device | Set Rx pre-amp gain: LOW(×1), MED(×10), HIGH(×50), AUTO |
| 0x40 | STORE_CAPTURE | → Device | Save current RX buffer to QSPI flash |
| 0x41 | RETRIEVE_CAPTURE | → Device → App | Download stored capture |
| 0x42 | ERASE_STORAGE | → Device | Format flash storage |
| 0x50 | UPDATE_FIRMWARE | → Device | Begin DFU over BLE |
| 0xFF | RESET | → Device | System reset |

### Mobile App Features

The React Native companion app (source in `app/`) provides:

- **Dashboard:** Real-time device status, battery level, storage usage, signal quality
- **Transmit:** Message composition, mode selection (FSK/OOK/Whisper), parameter tuning, live spectrum
- **Capture:** Real-time waterfall/spectrogram of the ultrasonic environment, signal detection alerts, capture-to-storage
- **Analyze:** Review stored captures, decode/export as hex/text, calculate effective data rates
- **Beacon:** Configure beacon interval and message, detect other beacons, triangulation aid

---

## Use Cases for Red Teams & Pentesters

### Case 1: Air-Gap Exfiltration Test

**Scenario:** A client's sensitive R&D network is air-gapped from the internet. Data exfiltration from the air-gapped network via network or USB is blocked. Can data be exfiltrated acoustically?

**Setup:**
1. Place SILENT-SYMPHONY in receiver mode within 5–15 m of the target air-gapped computer
2. On the target system, a compromised peripheral or malicious binary modulates sensitive data as ultrasonic tones via the system's own speakers (requires prior implant or physical access)
3. SILENT-SYMPHONY captures the ultrasonic signal, demodulates it, and stores the decoded data
4. Operator retrieves the data by walking near the device with the BLE companion app, or physically retrieving the device and connecting via USB

**Deliverable:** Proof-of-concept demonstration showing that even with no network connectivity, data can cross the air gap via ultrasonic waves — with exact bitrate, SNR measurements, and range characterization.

### Case 2: RF-Silent Covert Communication

**Scenario:** A red team operation requires communication between team members in a facility where RF emissions are monitored by spectrum analysis (government building, SCIF, data center).

**Setup:**
1. Each team member carries a SILENT-SYMPHONY in covert (belt/bag) mode
2. Short messages are transmitted between devices using whisper-mode FSK at 1–5 bps
3. The ultrasonic signal is further masked by ambient facility noise
4. No RF emissions occur — standard spectrum monitoring cannot detect the communication

**Deliverable:** Demonstrated covert communication channel that bypasses RF spectrum monitoring, with documented range and reliability metrics for the specific facility type.

### Case 3: Acoustic Injection (DolphinAttack Variant)

**Scenario:** A smart speaker (Amazon Echo, Google Home) in a secure meeting room processes voice commands. Can ultrasonic carriers modulate standard voice commands that the device interprets as human speech?

**Setup:**
1. SILENT-SYMPHONY is placed in transmit mode with a pre-configured command message ("unlock door", "send email", "start recording")
2. The command is modulated as an ultrasonic carrier using baseband modulation that exploits non-linearity in the microphone pre-amplifier
3. The smart speaker's MEMS microphone demodulates the signal as audible speech
4. The command is executed by the smart device

**Deliverable:** Demonstrated inaudible voice command injection against target smart devices, with device-specific carrier frequency optimization and power requirements.

### Case 4: Facility Acoustic Covert Channel Mapping

**Scenario:** Before a physical penetration test, the team needs to understand room geometry, wall composition, and potential hiding spots without being seen.

**Setup:**
1. SILENT-SYMPHONY in SCAN mode emits ultrasonic chirps (20–45 kHz swept)
2. Onboard AD8694 pre-amps capture reflections from walls, furniture, and obstacles
3. Time-of-flight measurements produce a coarse room map
4. The acoustic profile is stored for later analysis via the companion app

**Deliverable:** Acoustic profile and estimated room geometry, identifying potential blind spots and hiding locations for device placement.

### Case 5: Ultrasonic Beacon for Asset/Personnel Tracking

**Scenario:** During a multi-day red team engagement, a tracking device needs to be placed inside a facility to confirm regular access patterns without RF beaconing.

**Setup:**
1. SILENT-SYMPHONY is configured as a beacon (periodic ultrasonic ping at 45-second intervals)
2. The beacon is tuned to a frequency above 22 kHz (beyond consumer microphone cutoff for most laptops, but detectable by the device)
3. A second SILENT-SYMPHONY in SCAN mode placed outside the facility detects the periodic beacon
4. Access patterns are inferred from beacon detection times

**Deliverable:** Long-duration (24+ hour) beacon deployment with periodic confirmation of device presence, zero RF emissions.

---

## Directory Structure

```
silent-symphony/
├── README.md                     ← This file
├── firmware/
│   ├── main.c                    ← Entry point, scheduler, command dispatch
│   ├── board.h                   ← Pin definitions, peripheral mapping
│   ├── registers.h               ← STM32H743 MMIO register map
│   ├── Makefile                  ← Cross-compilation, flash, debug targets
│   └── drivers/
│       ├── ultrasonic_driver.h   ← Ultrasonic Tx/Rx control API
│       ├── ultrasonic_driver.c   ← FSK/OOK modulation & demodulation
│       ├── amp_driver.h          ← MAX98357A + AD8694 control API
│       ├── amp_driver.c          ← Amplifier gain, power control
│       ├── ble_bridge.h          ← nRF52840 UART protocol
│       ├── ble_bridge.c          ← BLE command/response bridge
│       ├── codec_driver.h        ← WM8731S I²S initialization
│       ├── codec_driver.c        ← Codec config, sample rate, filter
│       ├── capture_buffer.h      ← QSPI flash + PSRAM buffer manager
│       └── capture_buffer.c      ← DMA-driven capture to storage
├── kicad/
│   ├── silent-symphony.kicad_pro  ← KiCad project file
│   ├── silent-symphony.kicad_sch  ← Full schematic
│   └── silent-symphony.kicad_pcb  ← PCB layout (70×40mm, 4-layer)
├── app/
│   ├── App.js                    ← Main navigation (Tab Navigator)
│   ├── package.json              ← Dependencies
│   ├── screens/
│   │   ├── DashboardScreen.js    ← Device status, battery, mode
│   │   ├── TransmitScreen.js     ← Message composer & Tx controls
│   │   ├── CaptureScreen.js      ← Spectrum waterfall & capture
│   │   └── AnalyzeScreen.js      ← Stored capture review & decode
│   ├── components/
│   │   ├── StatusBar.js          ← Connection status indicator
│   │   ├── FreqChart.js          ← Canvas-based waterfall/spectrum
│   │   └── MessageLog.js         ← Scrollable message history
│   └── utils/
│       └── protocol.js           ← BLE packet encode/decode, CRC-16
```

---

## Legal & Ethical Disclaimer

**THIS DEVICE IS FOR AUTHORIZED SECURITY RESEARCH AND PENETRATION TESTING ONLY.**

The SILENT-SYMPHONY is a tool for:
- Legitimate security assessments conducted under written authorization
- Academic research into acoustic covert channels
- Defense testing and vulnerability research
- Educational demonstrations in controlled environments

**WARNING:** Use of this device to intercept, exfiltrate, or covertly communicate without explicit authorization may violate:
- Wiretap laws (18 U.S.C. § 2510 et seq. and international equivalents)
- Computer Fraud and Abuse Act (CFAA) or similar legislation
- National security regulations regarding air-gapped system testing
- Facility-specific security policies and non-disclosure agreements

The author (jayis1) assumes no liability for misuse. Operators are responsible for ensuring all testing is conducted with proper authorization and within legal boundaries.

**Responsible Disclosure:** If you discover that ultrasonic covert channels are exploitable in a deployed system, please follow responsible disclosure practices and notify the vendor before publishing.

---

## Licenses

| Component | License |
|---|---|
| Hardware (KiCad schematics & PCB) | CERN-OHL-S v2 |
| Firmware (C source) | GPL-2.0 |
| Companion App (React Native) | MIT |
| Documentation | CC-BY-SA 4.0 |

---

*Design and implementation by jayis1. This device is based on published academic research into ultrasonic covert channels (Ben-Gurion University, Fraunhofer FKIE, et al.) and implements those techniques in open-source hardware for the first time.*