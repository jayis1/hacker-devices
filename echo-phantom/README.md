# ECHO-PHANTOM — Covert I²S/TDM Audio Bus MITM Implant

```
   ╔════════════════════════════════════════════════════════════════════════════╗
   ║   ███████╗ ██████╗ ██████╗  ██████╗   ██╗   ██╗██████╗ ██╗   ██╗███████╗██████╗  ║
   ║   ██╔════╝██╔═══██╗██╔══██╗██╔═══██╗  ██║   ██║██╔══██╗██║   ██║██╔════╝██╔══██╗ ║
   ║   █████╗  ██║   ██║██████╔╝██║   ██║  ██║   ██║██████╔╝██║   ██║█████╗  ██████╔╝ ║
   ║   ██╔══╝  ██║   ██║██╔══██╗██║   ██║  ██║   ██║██╔══██╗██║   ██║██╔══╝  ██╔══██╗ ║
   ║   ██║     ╚██████╔╝██║  ██║╚██████╔╝  ╚██████╔╝██║  ██║╚██████╔╝███████╗██║  ██║ ║
   ║   ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚═════╝    ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝ ║
   ║                                                                              ║
   ║   Covert I²S / TDM Audio Bus MITM — Capture, Inject & Replay Audio Streams  ║
   ║   Below the OS. Below the Driver. At the Wire.                              ║
   ║   Author: jayis1                                                            ║
   ╚════════════════════════════════════════════════════════════════════════════╝
```

> **Author:** jayis1
> **License:** Hardware — CERN-OHL-S v2, Firmware — GPL-2.0, App — MIT
> **Status:** Research hardware design — firmware + PCB + companion app
> **Version:** 1.0.0
> **Date:** 2026-07-10

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** ECHO-PHANTOM is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and academic study of embedded audio pipeline security on devices you own or have explicit written authorization to assess. Intercepting audio from devices, persons, or premises you do not own or do not have authorization to test may violate wiretap statutes (e.g., 18 U.S.C. § 2511, 18 U.S.C. § 2515), computer fraud and abuse laws (18 U.S.C. § 1030), surveillance and eavesdropping laws in your jurisdiction, and biometric/voice-data protection regulations (e.g., GDPR Art. 9, Illinois BIPA for voiceprint). Injecting synthetic audio into a device's microphone stream can cause a target system to act on false voice commands, which may constitute unauthorized access to a protected computer. The author (**jayis1**) assumes no liability for any misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## Table of Contents

1. [Overview](#1-overview)
2. [What Makes ECHO-PHANTOM Novel](#2-what-makes-echo-phantom-novel)
3. [Attack Surface & Threat Model](#3-attack-surface--threat-model)
4. [Hardware Specifications](#4-hardware-specifications)
5. [Architecture & Block Diagram](#5-architecture--block-diagram)
6. [Theory of Operation — I²S/TDM Audio Bus Exploitation](#6-theory-of-operation--i²stdm-audio-bus-exploitation)
7. [Firmware Details & Design Decisions](#7-firmware-details--design-decisions)
8. [Protocol & Command Interface](#8-protocol--command-interface)
9. [Application/Software Interface](#9-applicationsoftware-interface)
10. [Use Cases for Red Teams, Security Researchers & Pentesters](#10-use-cases-for-red-teams-security-researchers--pentesters)
11. [Bill of Materials Summary](#11-bill-of-materials-summary)
12. [Limitations & Countermeasures](#12-limitations--countermeasures)
13. [Legal & Ethical Framework](#13-legal--ethical-framework)

---

## 1. Overview

**ECHO-PHANTOM** is a coin-cell-powered, covert man-in-the-middle (MITM) implant that physically taps the **I²S (Inter-IC Sound)** or **TDM (Time-Division Multiplexed)** audio bus between a target device's **microphone codec / MEMS microphone** and its host application processor (AP) or digital signal processor (DSP). By sitting inline on this low-level peripheral bus, ECHO-PHANTOM operates *below the operating system*, *below the audio driver*, and *below any application-level security controls* — capturing every audio sample the microphone produces as raw digital data, and optionally injecting synthetic audio to make the target system hear attacker-chosen sound.

The I²S/TDM audio bus is the physical layer that carries microphone data in virtually every modern voice-enabled device: smart speakers (Amazon Echo, Google Nest, Apple HomePod), voice assistants, automotive infotainment and hands-free systems, IoT devices with voice interfaces, smart TVs, conferencing systems, security intercoms, hearing aids, baby monitors, and industrial devices with voice commands. Despite being the primary data path for the most sensitive modality — audio of private conversations — this bus is almost never treated as a trust boundary. Security teams focus on Wi-Fi, Bluetooth, cloud APIs, and OS-level permissions, but the physical audio bus remains an unexamined attack surface that sits entirely outside software security controls.

### 1.1 Why this attack surface is unique

Modern voice-enabled devices implement "privacy" through software controls: a microphone mute button in an app, a firmware-level mute flag, a "wake word" detector, or a privacy LED indicator. All of these operate at the OS or application layer. None of them protect the raw I²S/TDM bus between the microphone hardware and the processor. An attacker who can physically access this bus — which on most devices is a simple 4-wire or 6-wire connection on a flex PCB or motherboard — can:

1. **Capture** every audio sample at the physical layer, bypassing all software privacy controls including OS-level mute, privacy LEDs, and wake-word gating.
2. **Inject** synthetic audio that the processor interprets as genuine microphone input, enabling voice command injection attacks (e.g., injecting inaudible voice commands to a smart speaker).
3. **Replay** previously recorded audio ("freeze-frame attack") so a device appears to hear a benign or authorized voice while the physical acoustic environment has changed.
4. **Modify** audio in real time — filtering, attenuating, or transforming the audio stream to alter what the system hears.

ECHO-Phantom is the first open research tool that targets this specific attack surface.

### 1.2 Key capabilities

| Capability | Description |
|---|---|
| **Passive audio capture** | Sniff all I²S/TDM audio traffic at wire speed, stream to onboard SD card or over encrypted BLE |
| **Active audio injection** | Replace microphone audio with attacker-synthesized PCM, inject voice commands |
| **Audio replay** | Play back previously recorded audio clips to the host processor |
| **Real-time audio modification** | DSP pipeline for filtering, attenuation, noise injection, pitch shifting |
| **Wake-word / voice command injection** | Inject crafted audio containing voice commands for Alexa, Google Assistant, Siri |
| **Multi-protocol support** | I²S (Philips, left-justified, right-justified), TDM (2/4/8/16 slots), PCM (long/short sync) |
| **Format detection** | Auto-detect sample rate (8–192 kHz), bit depth (16/24/32-bit), channel count |
| **Covert BLE backhaul** | Encrypted BLE 5.0 for remote control and audio streaming to operator's phone |
| **Long-duration operation** | Coin-cell + optional parasitic power from the target's own I²S lines |
| **Tiny form factor** | 18 × 12 × 2.4 mm — fits inside smart speaker enclosures, automotive head units |

---

## 2. What Makes ECHO-PHANTOM Novel

No existing security research tool targets the I²S/TDM audio bus as an attack surface. This is a significant gap because:

### 2.1 Below the OS, below the driver

All existing "audio interception" tools operate at the software level — they hook into ALSA, CoreAudio, WASAPI, or the Android audio HAL. They can be detected by software integrity checks, require root/privileged access, and are subject to OS-level privacy controls (mute, permission dialogs, LED indicators). ECHO-Phantom operates at the **physical wire level** — it intercepts the digital audio data before it reaches the processor's audio DMA controller. The OS has no knowledge of its presence. There is no software hook to detect, no permission to request, and no LED to trigger.

### 2.2 Active injection is the real threat

Passive capture is concerning but well-understood in the Tempest/surveillance world. What's genuinely novel is **active injection**: replacing the microphone's audio with attacker-chosen content. This enables:

- **Voice command injection below the OS** — inject inaudible (ultrasonic-modulated) or normal voice commands directly into the I²S stream. The target device's wake-word detector and ASR (automatic speech recognition) will process this as genuine microphone input. Unlike acoustic attacks (e.g., "Light Commands" using lasers), this requires no line of sight, no acoustic emission, and bypasses any software audio filtering.
- **Authentication bypass** — if a device uses voice biometrics for authentication, inject a pre-recorded voiceprint to unlock or authenticate.
- **False evidence** — make a device's audio recording contain attacker-chosen content while the physical microphone is muted.

### 2.3 Protocol-aware, not just raw capture

ECHO-Phantom doesn't just grab raw clock/data lines — it understands the I²S/TDM protocol. It detects word-select polarity, bit clock frequency, slot assignments, bit depth, and channel mapping. It can parse multi-channel TDM frames and selectively modify specific slots (e.g., only inject into the far-field microphone channel while leaving the near-field channel untouched).

### 2.4 Real-time DSP on the wire

The implant includes a dedicated DSP subsystem that can modify audio in real-time as it passes through. This enables subtle attacks — slightly attenuating certain frequencies, injecting inaudible ultrasonic markers, or applying a band-pass filter that makes the target's ASR system mishear specific words (an "audio adversarial perturbation" at the physical layer).

### 2.5 Comparison to existing devices

| Existing Device | Attack Surface | ECHO-Phantom Difference |
|---|---|---|
| Shadow-Tap | Ethernet (L2) | ECHO-Phantom targets audio bus (physical peripheral), not network |
| Tactile-Phantom | I²C touch controller | Similar MITM concept but on a completely different bus (audio, not touch) |
| Aperture-Phantom | MIPI CSI-2 camera | Similar MITM concept but on audio, not video; different protocol, different threat model |
| Silent-Symphony | Ultrasonic air-gap comms | Silent-Symphony *emits* ultrasonic audio; ECHO-Phantom *intercepts* the audio bus itself |
| Acoustic-Phantom | Acoustic side-channel capture | Acoustic-Phantom captures airborne sound with a microphone; ECHO-Phantom taps the digital audio bus |
| Spectre-Sniffer | EM side-channel | Spectre-Sniffer captures EM emanations; ECHO-Phantom intercepts the actual digital audio data path |

---

## 3. Attack Surface & Threat Model

### 3.1 Attack surface

The I²S/TDM audio bus is a **3-wire or 4-wire synchronous serial interface** that carries digitized audio between ICs:

| Signal | Function | Typical Voltage |
|---|---|---|
| **SCK / BCLK** | Bit clock — one edge per bit sample | 1.8V or 3.3V |
| **WS / LRCLK** | Word select — toggles left/right channel (I²S) or frame sync (TDM) | 1.8V or 3.3V |
| **SD / SDATA** | Serial data — two's-complement audio | 1.8V or 3.3V |
| **MCLK** (optional) | Master clock — typically 256× sample rate | 1.8V or 3.3V |

The bus carries **unencrypted, unauthenticated** audio data. There is no CRC, no authentication, no encryption — just raw PCM samples clocked from source to sink. This is by design: the I²S standard (originally specified by Philips in 1986) was intended for short on-board connections between a DAC and a DSP, with no security model whatsoever.

### 3.2 Attack vectors

| Attack | Description | Impact |
|---|---|---|
| **Passive capture** | Tap SD/BCLK/WS, reconstruct all audio | Exfiltration of all microphone audio, bypassing OS privacy controls |
| **Audio injection** | Drive SD with attacker PCM while suppressing mic output | Voice command injection, false audio evidence |
| **Audio replay** | Replay stored PCM clips | "Freeze-frame" audio — device hears stale authorized audio |
| **Audio modification** | Real-time DSP filter/transform | Adversarial audio perturbations, selective muting |
| **Format manipulation** | Change sample rate, bit depth, channel mapping | Cause buffer overflows, ASR misinterpretation |
| **Clock manipulation** | Jitter or modify BCLK frequency | Audio distortion, DoS, timing attacks on voice activity detection |
| **Mute bypass** | Capture audio even when OS has muted the microphone | Total privacy control bypass |

### 3.3 Threat model

| Property | Assessment |
|---|---|
| **Physical access required** | Yes — must access the I²S/TDM bus traces (flex cable, PCB test points, or connector) |
| **Detection difficulty** | Very low — no software-visible artifact; OS sees normal audio I/O |
| **Persistence** | Hardware implant — persists until physically removed |
| **Skill level** | Moderate — requires identifying I²S signals, soldering or clamping the implant |
| **Bypasses** | OS-level mute, privacy LEDs, wake-word gating, ASR input validation, app-level audio permissions |

### 3.4 Target devices

| Target | Audio Bus | Attack Value |
|---|---|---|
| Amazon Echo / Echo Dot | I²S from MEMS mic to Mediatek/SoC | Capture all voice, inject commands |
| Google Nest / Nest Mini | I²S from MEMS mic to SoC | Capture, inject "OK Google" commands |
| Apple HomePod / HomePod Mini | I²S from mic array to Apple audio chip | Capture, inject "Hey Siri" commands |
| Automotive head units | TDM (4–8 channel) from mic array to DSP/SoC | Capture in-cabin conversations, inject hands-free commands |
| Smart TVs with voice remote | I²S from mic to SoC | Capture, inject voice search commands |
| Video conferencing systems | I²S/TDM from mic array to DSP | Capture meeting audio, inject audio |
| Baby monitors / security intercoms | I²S from mic to MCU | Capture private spaces |
| Hearing aids / medical audio devices | I²S from mic to DSP | Capture private conversations |

---

## 4. Hardware Specifications

### 4.1 Core specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32H743VIT6 — Cortex-M7 @ 480 MHz, 2 MB flash, 1 MB SRAM |
| **Audio DSP** | On-chip Cortex-M7 DSP extensions (CMSIS-DSP) + hardware CRC |
| **I²S/TDM Interface** | 3× SPI/I²S peripherals (SAI1, SAI2, SAI3) — full-duplex I²S/TDM |
| **Audio codec (for injection)** | On-chip, no external codec needed; STM32 SAI generates I²S output |
| **Sample rates** | 8, 16, 22.05, 24, 32, 44.1, 48, 88.2, 96, 176.4, 192 kHz |
| **Bit depths** | 16, 24, 32-bit (auto-detected) |
| **TDM slots** | 2, 4, 8, 16 channels (TDM mode) |
| **Audio capture buffer** | 128 MB SDRAM (IS42S16160G) via FMC interface |
| **Storage** | MicroSD card slot (UHS-I, up to 2 TB) for WAV capture |
| **BLE** | nRF52840 module (Fanstel BT840F) — BLE 5.0, AES-256-CTR encrypted link |
| **USB** | USB 2.0 full-speed (CDC) for host control + audio streaming |
| **Power** | 3.7V LiPo (150 mAh, 3.4 × 2.5 × 4 mm) + parasitic power option |
| **Battery life** | ~12 hours passive capture, ~6 hours active injection |
| **Charging** | USB-C (5V, 500 mA) |
| **Form factor** | 18 × 12 × 2.4 mm (without battery) |
| **Operating temp** | -20°C to +70°C |
| **BOM cost** | ~$22 at qty 100 |

### 4.2 I/O pinout

The implant has two sides: a **target side** (facing the microphone/codec) and a **host side** (facing the AP/DSP).

| Signal | Target Side | Host Side | MCU Pin |
|---|---|---|---|
| BCLK (bit clock) | RX (from mic) | TX (regenerated) | PA8 / SAI1_CK_A |
| WS (word select) | RX (from mic) | TX (regenerated) | PA7 / SAI1_FS_A |
| SDATA (serial data) | RX (from mic) | TX (modified/injected) | PB3 / SAI1_SD_A |
| MCLK (master clock) | RX (optional) | TX (regenerated) | PC7 / SAI1_MCLK_A |
| SDATA (from host) | TX (to mic, for control) | RX (from AP) | PE6 / SAI1_SD_B |
| BLE UART | — | — | USART1 (PA9/PA10) |
| USB CDC | — | — | USB OTG1 (PA11/PA12) |
| SD card | — | — | SDMMC1 (4-bit) |
| SDRAM | — | — | FMC (D0-D15, A0-A12) |
| Status LED | — | — | PB0 (red — capture active) |
| Status LED 2 | — | — | PB1 (blue — injecting) |

### 4.3 Audio path architecture

```
 ┌─────────────┐    I²S/TDM     ┌═══════════════════════════════════════┐    I²S/TDM     ┌──────────────┐
 │  Microphone  │──────────────▶│          ECHO-PHANTOM Implant          │──────────────▶│  Host AP/DSP  │
 │  Codec /     │   BCLK, WS,  │                                       │   BCLK, WS,   │  (Echo SoC,  │
 │  MEMS Mic    │   SDATA, MCLK │  ┌─────────┐  ┌──────────┐  ┌────────┐│   SDATA, MCLK │  Cortex-A,   │
 │              │               │  │ Target   │  │  Audio   │  │ Host   ││               │  DSP, etc.)  │
 └─────────────┘               │  │ I/F (RX) │─▶│  DSP/MIX │─▶│ I/F    ││               └──────────────┘
                               │  │ SAI1_A   │  │  Engine  │  │ SAI1_B ││
                               │  └─────────┘  └──────────┘  └────────┘│
                               │       │             ▲             │   │
                               │       ▼             │             ▼   │
                               │  ┌─────────┐  ┌──────────┐  ┌────────┐│
                               │  │ Capture  │  │ Inject   │  │ Format ││
                               │  │ Buffer   │  │ Buffer   │  │ Detect ││
                               │  │ (SDRAM)  │  │ (Flash)  │  │ Engine ││
                               │  └─────────┘  └──────────┘  └────────┘│
                               │       │             │                  │
                               │       ▼             │                  │
                               │  ┌─────────────────────────┐           │
                               │  │   BLE C2 (nRF52840)      │           │
                               │  │   USB CDC (backup)       │           │
                               │  └─────────────────────────┘           │
                               └═══════════════════════════════════════┘
```

---

## 5. Architecture & Block Diagram

### 5.1 System block diagram

```
 ┌─────────────────────────────────────────────────────────────────────┐
 │                        ECHO-PHANTOM Top-Level                        │
 │                                                                     │
 │  ┌───────────────┐    ┌──────────────────────────────────────────┐  │
 │  │  Power Mgmt    │    │            STM32H743VIT6 MCU              │  │
 │  │  ┌──────────┐ │    │  ┌──────────┐  ┌──────────┐  ┌────────┐│  │
 │  │  │ LiPo     │ │    │  │  SAI1_A  │  │  SAI1_B  │  │  SAI2  ││  │
 │  │  │ Charger  │ │    │  │  (RX)    │  │  (TX)    │  │(aux RX)││  │
 │  │  │ MCP73831 │ │    │  └────┬─────┘  └────┬─────┘  └────────┘│  │
 │  │  └──────────┘ │    │       │              │                   │  │
 │  │  ┌──────────┐ │    │  ┌────▼──────────────▼─────┐            │  │
 │  │  │ LDO 3.3V │ │    │  │   Audio DMA Engine       │            │  │
 │  │  │ TLV75533 │ │    │  │  (BDMA + MDMA + DMA2D)   │            │  │
 │  │  └──────────┘ │    │  └────────────┬────────────┘            │  │
 │  │  ┌──────────┐ │    │               │                          │  │
 │  │  │ Vbus     │ │    │  ┌────────────▼────────────┐            │  │
 │  │  │ Detect   │ │    │  │   CMSIS-DSP Pipeline     │            │  │
 │  │  └──────────┘ │    │  │  (Capture/Inject/Modify) │            │  │
 │  └───────────────┘    │  └────────────┬────────────┘            │  │
 │                       │               │                          │  │
 │  ┌───────────────┐    │  ┌────────────▼──────────────────┐       │  │
 │  │  nRF52840 BLE │    │  │  FMC → 128 MB SDRAM            │       │  │
 │  │  (BT840F)     │────│  │  SDMMC1 → MicroSD (WAV capture)│       │  │
 │  │  UART C2 link │    │  │  USB OTG → CDC (host control)  │       │  │
 │  └───────────────┘    │  │  Flash → Inject clip storage   │       │  │
 │                       │  └────────────────────────────────┘       │  │
 │  ┌───────────────┐    └──────────────────────────────────────────┘  │
 │  │  Target Side   │                                                │
 │  │  FFC Connector │   ← 6-pin 0.5mm FFC to microphone/codec       │
 │  └───────────────┘                                                │
 │  ┌───────────────┐                                                │
 │  │  Host Side     │   ← 6-pin 0.5mm FFC to AP/DSP                 │
 │  │  FFC Connector │                                                │
 │  └───────────────┘                                                │
 └─────────────────────────────────────────────────────────────────────┘
```

### 5.2 Data flow — passive capture mode

```
Microphone → [Target I/F RX (SAI1_A)] → DMA → SDRAM capture buffer
                                                ↓
                                         WAV encoder → MicroSD
                                                ↓
                                         BLE stream → Operator phone

Host AP ← [Host I/F TX (SAI1_B)] ← passthrough from SAI1_A (zero-delay)
```

### 5.3 Data flow — active injection mode

```
Inject clip (Flash) → [Audio Mix Engine] → [Host I/F TX (SAI1_B)] → Host AP
                              ↑
Microphone → [Target I/F RX (SAI1_A)] → passthrough or suppression
```

### 5.4 Data flow — real-time modification mode

```
Microphone → [Target I/F RX (SAI1_A)] → [CMSIS-DSP Filter] → [Host I/F TX (SAI1_B)] → Host AP
                                              ↓
                                         SDRAM capture buffer (modified vs. original)
```

---

## 6. Theory of Operation — I²S/TDM Audio Bus Exploitation

### 6.1 I²S protocol primer

I²S (Inter-IC Sound) is a synchronous serial protocol with three signals:

- **BCLK (Bit Clock)**: one clock edge per bit of audio data. Frequency = sample_rate × bit_depth × channels.
- **WS (Word Select / LRCLK)**: toggles between left and right channel. WS = 0 for left, WS = 1 for right (in standard Philips mode). WS transitions one BCLK before the first bit of each word.
- **SDATA (Serial Data)**: two's-complement audio data, MSB first, transmitted on the BCLK edge following the WS transition.

Optional:
- **MCLK (Master Clock)**: typically 256× or 384× the sample rate. Used by some codecs for internal PLL.

### 6.2 TDM protocol primer

TDM (Time-Division Multiplexing) extends I²S to support more than 2 channels. Instead of a simple left/right WS toggle, TDM uses a pulse or frame-sync signal to mark the start of a frame containing N slots (typically 2, 4, 8, or 16 channels). Each slot carries one audio sample for one channel. BCLK frequency = sample_rate × bit_depth × slots.

### 6.3 How ECHO-Phantom intercepts

The implant sits **inline** on the I²S/TDM bus:

1. **Target I/F (SAI1_A)** receives the microphone's output (BCLK, WS, SDATA) as an I²S *slave*.
2. **Host I/F (SAI1_B)** regenerates BCLK and WS (derived from the captured clock) and outputs modified or passthrough SDATA to the host AP/DSP. SAI1_B is configured as an I²S *master* for clock generation but a *slave* for data timing.
3. The MCU's audio DMA engine transfers each received audio frame from SAI1_A's RX buffer into SDRAM (for capture) and simultaneously copies the frame (or a modified version) into SAI1_B's TX buffer.
4. The latency between target-side RX and host-side TX is exactly **one audio frame** (one WS period) — typically 10–22 µs for 48–96 kHz sample rates. This is inaudible and undetectable by the host.

### 6.4 Format auto-detection

On power-up (or on command), ECHO-Phantom's format detection engine monitors the target bus:

1. Measure BCLK frequency using a timer capture — this gives sample_rate × bit_depth × channels.
2. Measure WS frequency — this gives the sample rate.
3. Divide BCLK by WS to get bit_depth × channels.
4. Count WS transitions per BCLK period to determine left/right vs. TDM frame.
5. Determine bit depth by checking when SDATA transitions from MSB to the first zero-padding bits.
6. For TDM: count the number of slots per frame by analyzing the WS pulse width relative to BCLK.

The detected format is reported to the operator via BLE/USB, and the SAI peripherals are automatically configured to match.

### 6.5 Audio injection

To inject audio, the operator uploads a WAV file (or raw PCM) to the implant via BLE or USB. The firmware stores it in the on-chip flash (up to 2 MB — enough for ~10 seconds of 16-bit 48 kHz mono audio) or on the SD card for longer clips. When the injection command is issued:

1. The audio mix engine loads the inject clip into a ring buffer in SRAM.
2. For each audio frame received from SAI1_A, the engine either:
   - **Replaces** the mic audio entirely with the inject clip (full injection).
   - **Mixes** the inject clip with the mic audio at a configurable ratio (overlay).
   - **Suppresses** the mic audio and plays the clip (clean injection — mic is "muted" from the host's perspective).
3. The mixed/replaced audio is written to SAI1_B's TX buffer and sent to the host AP/DSP.
4. The host's ASR/wake-word engine processes this as genuine microphone input.

### 6.6 Ultrasonic voice command injection

A particularly powerful attack: the operator can craft an audio clip containing a voice command (e.g., "Alexa, unlock the front door") modulated onto an ultrasonic carrier (e.g., 20–40 kHz). If the target device's microphone and I²S codec have bandwidth beyond 20 kHz (many MEMS microphones do), the injected ultrasonic audio will be digitized and passed to the host ASR. The host's software may not filter ultrasonic frequencies (or may do so inadequately), and the ASR system may demodulate or process the command. By injecting at the I²S level (post-microphone, post-codec-ADC), ECHO-Phantom bypasses any analog front-end filtering that would block airborne ultrasonic attacks.

---

## 7. Firmware Details & Design Decisions

### 7.1 Architecture

The firmware is organized into these layers:

| Layer | Files | Responsibility |
|---|---|---|
| **Application** | `main.c` | Command loop, state machine, mode orchestration |
| **Audio I/O** | `drivers/sai_audio.c` | SAI1_A/B configuration, DMA setup, frame transfer |
| **Format detection** | `drivers/format_detect.c` | Auto-detect sample rate, bit depth, channel count |
| **Audio DSP** | `drivers/audio_dsp.c` | CMSIS-DSP filters, mixing, gain, format conversion |
| **Capture engine** | `drivers/capture.c` | SDRAM ring buffer, WAV file writing, SD card I/O |
| **Injection engine** | `drivers/inject.c` | Inject clip loading, ring buffer, playback state machine |
| **BLE C2** | `drivers/ble_c2.c` | UART protocol to nRF52840, encrypted command/response |
| **USB CDC** | `drivers/usb_cdc.c` | USB CDC virtual serial port for host control |
| **Board init** | `drivers/board_init.c` | Clock, GPIO, power, peripheral initialization |
| **SD card** | `drivers/sdcard.c` | SDMMC1 FATFS, WAV file creation/append |

### 7.2 Design decisions

**Why STM32H743?**
The STM32H743 has three Serial Audio Interfaces (SAI1, SAI2, SAI3), each supporting full-duplex I²S/TDM. This allows ECHO-Phantom to have a dedicated SAI for the target (mic) side and a separate SAI for the host (AP) side, with independent clock generation. The 480 MHz Cortex-M7 provides enough processing power for real-time CMSIS-DSP audio filtering at 192 kHz. The 2 MB flash stores inject clips, and the FMC interface connects to the 128 MB SDRAM capture buffer.

**Why not an FPGA?**
An FPGA would add complexity, power consumption, and cost. The STM32H743's SAI peripherals can handle I²S/TDM at up to 192 kHz / 32-bit with hardware DMA — no FPGA needed for the target use case. If sub-sample-precision timing manipulation were required (it isn't for audio), an FPGA would be justified.

**Why inline (MITM) rather than sniff-only?**
A sniff-only device would only capture audio. ECHO-Phantom's key novelty is *active injection* — replacing the mic audio with attacker content. This requires being inline on the bus, not just passively probing it.

**Why coin-cell + parasitic power?**
For covert deployment, the implant may need to operate without its own battery (e.g., inside a sealed smart speaker). The I²S MCLK line from the target often carries enough power (via a small energy-harvesting circuit) to run the MCU at reduced clock speed in capture-only mode. The LiPo battery provides full operation including injection.

**Why 128 MB SDRAM?**
At 48 kHz / 16-bit / mono, one minute of audio is 5.76 MB. 128 MB allows ~22 minutes of capture before the buffer wraps (or longer with SD card offloading). The SDRAM also serves as a staging buffer for large inject clips loaded from SD card.

### 7.3 Real-time constraints

The critical real-time path is: SAI1_A RX DMA → SRAM frame buffer → (optional DSP) → SAI1_B TX DMA. At 48 kHz, one audio frame is 20.83 µs. The DMA double-buffering ensures that while one buffer is being filled by SAI1_A, the other is being processed and transferred to SAI1_B. The total processing budget per frame is one frame period (20.83 µs at 48 kHz), which is ample for the CMSIS-DSP operations (typically < 5 µs per frame for a simple FIR filter).

### 7.4 Build system

The firmware builds with `arm-none-eabi-gcc` and a hand-written linker script (`linker.ld`). No external HAL library is used beyond CMSIS — all register definitions are in `registers.h` for transparency and educational value. The Makefile supports `make`, `make flash`, `make clean`, and `make size`.

---

## 8. Protocol & Command Interface

### 8.1 BLE command protocol

ECHO-Phantom communicates with the operator's phone over BLE 5.0 using a custom GATT service with two characteristics:

| UUID | Type | Purpose |
|---|---|---|
| `0x0001` | Write | Command (operator → device) |
| `0x0002` | Notify | Response / telemetry (device → operator) |
| `0x0003` | Notify | Audio stream (device → operator, compressed) |

The command packet format:

```
Byte 0:   Command code (CMD_*)
Byte 1:   Payload length (N)
Bytes 2-N+1: Payload
Byte N+2: CRC-8
```

| Command | Code | Payload | Response |
|---|---|---|---|
| `CMD_PING` | 0x01 | — | `ACK` + firmware version |
| `CMD_GET_STATUS` | 0x02 | — | mode, format, buffer fill, battery, uptime |
| `CMD_GET_FORMAT` | 0x03 | — | detected sample rate, bit depth, channels |
| `CMD_START_CAPTURE` | 0x04 | destination (SD/BLE/USB) | `ACK`, capture active |
| `CMD_STOP_CAPTURE` | 0x05 | — | `ACK`, frames captured, duration |
| `CMD_START_INJECT` | 0x06 | clip ID, mode (replace/mix/overlay), gain | `ACK`, injecting |
| `CMD_STOP_INJECT` | 0x07 | — | `ACK` |
| `CMD_UPLOAD_CLIP` | 0x08 | clip ID, total size, data chunk | `ACK` when complete |
| `CMD_SET_MODE` | 0x09 | mode (passthrough/capture/inject/modify) | `ACK` |
| `CMD_SET_FILTER` | 0x0A | filter type, coefficients | `ACK` |
| `CMD_STREAM_START` | 0x0B | quality (8/16/32 kbps) | `ACK`, audio notifications begin |
| `CMD_STREAM_STOP` | 0x0C | — | `ACK` |
| `CMD_ERASE_CLIPS` | 0x0D | — | `ACK` |
| `CMD_GET_BATTERY` | 0x0E | — | voltage, percentage, charging state |
| `CMD_FACTORY_RESET` | 0x0F | magic bytes | `ACK` + reboot |

### 8.2 USB CDC interface

When connected via USB, ECHO-Phantom appears as a virtual serial port (`/dev/ttyACM0` on Linux, `COMx` on Windows). The same command protocol is used over the serial port, with the addition of a binary audio streaming mode for high-speed capture download.

---

## 9. Application/Software Interface

The companion app is a React Native application for iOS and Android that provides:

### 9.1 Screens

| Screen | Function |
|---|---|
| **Dashboard** | Device status, battery, current mode, detected audio format, buffer fill |
| **Capture** | Start/stop capture, select destination (SD/BLE/USB), live audio waveform |
| **Inject** | Upload/manage inject clips, select injection mode, set gain, start/stop injection |
| **Modify** | Configure real-time DSP filters (LP, HP, BP, gain, noise injection) |
| **Stream** | Live audio monitoring — listen to captured audio in real time |
| **Clips** | Upload, manage, and preview inject clips |
| **Settings** | BLE pairing, encryption key, firmware update, factory reset |

### 9.2 Audio streaming

The app can receive a live audio stream from the device over BLE. Audio is compressed using a lightweight adaptive PCM codec (8/16/32 kbps selectable) and played through the phone's speaker or headphones. This allows the operator to listen to the captured microphone audio in real time.

### 9.3 Clip upload

Inject clips are uploaded from the phone to the device via BLE. The app supports:
- Recording a new clip using the phone's microphone.
- Selecting a WAV/MP3 file from the phone's storage.
- Generating synthetic voice commands (text-to-speech using the phone's TTS engine).

---

## 10. Use Cases for Red Teams, Security Researchers & Pentesters

### 10.1 Smart speaker privacy audit

**Scenario:** A red team is engaged to assess whether a smart speaker's microphone privacy controls are effective.

**Attack:**
1. Open the smart speaker enclosure and locate the I²S bus between the MEMS microphone and the SoC.
2. Attach ECHO-Phantom inline on the I²S bus.
3. From the operator's phone, enable "mute" on the smart speaker via its app.
4. Start passive capture — verify that ECHO-Phantom captures audio even when the speaker reports "muted."
5. Report to the client: "Your mute button is a software flag; audio is still captured at the hardware level."

**Value:** Demonstrates that software mute is insufficient; the physical audio bus must be gated.

### 10.2 Voice command injection against smart speaker

**Scenario:** Test whether a smart speaker can be made to execute commands without acoustic emission.

**Attack:**
1. Attach ECHO-Phantom inline on the I²S bus.
2. Upload a clip containing "Alexa, turn on all lights" via the app.
3. Trigger injection — the smart speaker's ASR processes the injected audio and executes the command.
4. No sound is emitted in the room — the attack is completely silent.

**Value:** Demonstrates that physical access to the audio bus enables completely silent voice command injection.

### 10.3 Automotive in-cabin audio capture

**Scenario:** Assess whether in-cabin conversations in a vehicle can be intercepted via the audio bus.

**Attack:**
1. Access the vehicle's head unit or telematics module.
2. Locate the TDM bus between the microphone array and the DSP.
3. Attach ECHO-Phantom on the TDM bus.
4. Capture all in-cabin audio, including phone calls and conversations.

**Value:** Demonstrates the risk of physical audio bus access in vehicles; informs countermeasure design (encrypted audio bus, physical tamper detection).

### 10.4 Voice authentication bypass

**Scenario:** A device uses voice biometrics for authentication. Can it be bypassed?

**Attack:**
1. Record the target user's voice (from prior authorized interactions or social engineering).
2. Upload the voice clip to ECHO-Phantom.
3. Attach the implant to the target device's I²S bus.
4. Inject the recorded voice — the device's voice biometric system matches the injected audio to the enrolled voiceprint and authenticates.

**Value:** Demonstrates that voice biometrics are vulnerable to physical-layer injection attacks; informs the need for liveness detection at the acoustic layer.

### 10.5 Security researcher — audio pipeline fuzzing

**Scenario:** A security researcher wants to find vulnerabilities in an IoT device's audio processing pipeline.

**Attack:**
1. Attach ECHO-Phantom inline.
2. Inject malformed audio frames — incorrect bit depth, extreme sample values, rapid format changes, very high sample rates.
3. Monitor the host for crashes, buffer overflows, or unexpected behavior.

**Value:** Fuzzing the audio input path at the physical layer — the raw I²S data — finds bugs that software-level audio fuzzing (which goes through the OS audio stack) cannot reach.

### 10.6 Supply chain implant detection

**Scenario:** A security team wants to test whether a supply chain adversary could implant a hardware audio tap.

**Attack:**
1. ECHO-Phantom is deployed inside a test smart speaker.
2. The team verifies that standard security scanning (X-ray of the device, firmware integrity checks, network traffic monitoring) fails to detect the implant.
3. Report: "Current detection methods do not cover hardware audio bus implants."

**Value:** Informs supply chain security practices — the need for physical audio bus inspection and tamper-evident packaging.

---

## 11. Bill of Materials Summary

| Component | Part Number | Qty | Cost (qty 100) | Function |
|---|---|---|---|---|
| MCU | STM32H743VIT6 | 1 | $9.50 | Core processor, SAI audio I/O, DSP |
| BLE module | Fanstel BT840F | 1 | $4.20 | BLE 5.0 wireless C2 |
| SDRAM | IS42S16160G-6TL | 1 | $1.80 | 128 MB audio capture buffer |
| LDO | TLV75533PDBVR | 1 | $0.35 | 3.3V regulation |
| Charger | MCP73831T-2ACI/OT | 1 | $0.60 | LiPo charging |
| Battery | 150 mAh LiPo | 1 | $2.10 | Power |
| FFC connector | Hirose FH12-6S-0.5SH | 2 | $1.40 | Target + host side connections |
| MicroSD slot | Molex 503182-1853 | 1 | $1.20 | Storage |
| USB-C connector | GCT USB4105 | 1 | $0.80 | Charging + USB CDC |
| Status LEDs | 0402 red + blue | 2 | $0.10 | Status indication |
| Passives | R, C, ferrite beads | ~20 | $0.85 | Support components |
| PCB | 4-layer, 18×12mm | 1 | $0.45 | Substrate |
| **Total** | | | **~$22.35** | |

---

## 12. Limitations & Countermeasures

### 12.1 Limitations

1. **Physical access required** — the implant must be soldered or clamped onto the I²S bus traces. This limits the attack to scenarios where physical access is possible.
2. **Voltage level compatibility** — the target bus must be 1.8V or 3.3V logic. Some devices use 0.9V or 1.2V audio bus signaling, which would require level-shifting.
3. **Bus speed** — the STM32H743's SAI supports up to ~12 MHz BCLK, covering all standard audio sample rates up to 192 kHz / 32-bit / 8-channel TDM. Extremely high-rate or non-standard audio buses may exceed this.
4. **Single bus** — the implant taps one I²S/TDM bus. Devices with multiple independent audio paths (e.g., separate mic arrays) would require multiple implants.
5. **Detection via physical inspection** — X-ray, visual inspection, or impedance measurement of the I²S bus can reveal the implant.

### 12.2 Defensive countermeasures

| Countermeasure | Effectiveness |
|---|---|
| **Encrypted audio bus** (I²S with AES-128 link encryption) | Full — implant cannot read or inject without the key |
| **Authenticated audio path** (HMAC on audio frames) | High — implant can capture but injection is detected |
| **Physical tamper detection** (tamper mesh on PCB, tamper-responsive enclosure) | High — implant installation detected |
| **SoC-integrated microphone** (mic and ADC in same package) | Moderate — eliminates external I²S bus but internal traces may still be accessible |
| **MEMS mic with built-in encryption** | Full — I²S carries encrypted data |
| **Hardware mute switch** (analog gate before ADC) | High — physically disconnects the mic from the bus |
| **Privacy LED wired to mic power** | Low — LED indicates mic is powered, but doesn't protect the bus |

---

## 13. Legal & Ethical Framework

ECHO-PHANTOM is a **defensive and research tool** intended for:

- **Authorized penetration testing** under a signed scope of work and rules of engagement.
- **Academic security research** on devices you own or have explicit written permission to assess.
- **Red team engagements** where the client has authorized physical access to target devices.
- **Hardware CTF competitions** and educational labs.
- **Security product evaluation** — testing whether a product's audio privacy controls are effective against hardware-level attacks.

**ECHO-PHANTOM must NOT be used to:**

- Intercept audio from devices, persons, or premises without explicit written authorization.
- Capture private conversations without the consent of all parties.
- Inject voice commands into devices you do not own or do not have authorization to test.
- Bypass voice biometric authentication on devices without authorization.
- Deploy as a covert surveillance device in any context.

The author (**jayis1**) assumes no liability for misuse. By building, flashing, or operating ECHO-PHANTOM, you confirm that you have proper authorization and are complying with all applicable laws.

---

*Designed by jayis1 — 2026*
*"The microphone hears everything. The OS hears what it's told. ECHO-Phantom sits between."*