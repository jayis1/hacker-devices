# CoilGrip — Wireless Power Transfer (Qi) Manipulation, MITM & Covert Channel Platform

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

```
   ╔══════════════════════════════════════════════════════════════╗
   ║   ██████╗██╗██████╗ ██╗  ██╗ █████╗ ███████╗████████╗██████╗ ██╗ ██████╗ ███████╗██████╗ ██╗██████╗ █████╗ ███████╗ ██████╗ ██████╗ ███████╗██████╗   ║
   ║  ██╔════╝██║██╔══██╗██║  ██║██╔══██╗██╔════╝╚══██╔══╝██╔══██╗██║██╔═══██╗██╔════╝██╔══██╗██║██╔══██╗██╔══██╗██╔════╝██╔══██╗██╔══██╗╚══════╝██╔══██╗  ║
   ║  ██║     ██║██████╔╝███████║███████║█████╗     ██║   ██████╔╝██║██║   ██║█████╗  ██║  ██║██║██████╔╝██║  ██║█████╗  ██████╔╝██║  ██║ █████╗  ██║  ██║  ║
   ║  ██║     ██║██╔═══╝ ██╔══██║██╔══██║██╔══╝     ██║   ██╔═══╝ ██║██║   ██║██╔══╝  ██║  ██║██║██╔══██╗██║  ██║██╔══╝  ██╔══██╗██║  ██║ ██╔══╝  ██║  ██║  ║
   ║  ╚██████╗██║██║     ██║  ██║██║  ██║███████╗   ██║   ██║     ██║╚██████╔╝███████╗██████╔╝██║██║  ██║██████╔╝███████╗██████╔╝██████╔╝███████╗██████╔╝  ║
   ║   ╚═════╝╚═╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═╝     ╚═╝ ╚═════╝ ╚══════╝╚═════╝ ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚═════╝ ╚═════╝ ╚══════╝╚═════╝   ║
   ║   Wireless Power Transfer (Qi) Manipulation · Contactless MITM · Power-Glitch · Covert Channel   ║
   ╚══════════════════════════════════════════════════════════════╝

   Author:  jayis1
   Version: 1.0
   Date:    2026-06-29
```

> **⚠️ LEGAL DISCLAIMER:** CoilGrip is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Manipulating the Qi wireless-charging interface of devices you do not own, inducing power faults in third-party hardware, or using a charging port as an unapproved covert exfiltration channel may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), product-safety regulations (FCC Part 15 / 18, IEC 62368-1, Qi specification licensing), and data-protection regulations in your jurisdiction. Deliberately defeating Foreign Object Detection (FOD) or driving a Qi transmitter outside its certified envelope can cause hazardous heating and fire — **never operate CoilGrip unattended or on devices without explicit authorization.** The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**CoilGrip** is a credit-card-thin, battery-assisted hardware implant that wedges itself **between any Qi-certified wireless charger and any Qi-compatible receiver** (smartphone, earbud case, smartwatch, IoT sensor, industrial Qi receiver). It implements *both* sides of the Qi inductive-power protocol and uses that position to attack the otherwise-trust boundary of the charging interface. The device is controlled over encrypted BLE 5.0 from an operator's mobile phone and can operate untethered for the duration of an engagement.

Wireless charging is a pervasive and largely unexamined attack surface. Modern Qi chargers and receivers negotiate power delivery with a packet protocol carried by **load modulation** on the 110–205 kHz power carrier. That same physical layer doubles as a bidirectional data channel that CoilGrip can observe, manipulate, and abuse. CoilGrip turns the charging pad into a research console.

### What CoilGrip does

| Mode | Function | Red-team utility |
|------|----------|------------------|
| **MITM** | Transparently relays power between an upstream Qi charger and a downstream Qi receiver while intercepting and rewriting the negotiation packets. | Drop a target's charge rate to a trickle (DoS), force a fast-charge profile on a device that rejects it, or replay a captured negotiation to spoof a known charger identity. |
| **Power Profiler** | Samples the target's instantaneous draw from the Qi field at up to 256 kS/s and applies spectral/time-series analysis to fingerprint the device, OS state, and activity (screen on/off, app launch, boot). | Identify a target by its charge signature; detect when a screen unlocks or an app launches without ever touching the device. |
| **Contactless Glitch** | Withholds or pulses the transferred power with sub-microsecond timing to induce brown-outs / reset faults at a chosen phase of the target's execution (e.g. secure-boot signature check). | Fault-injection attacks delivered **through the case** — no need to open the target or attach a glitcher to the VCC rail. |
| **Covert Channel** | Modulates the Qi load to exfiltrate data from a compromised (consented) device to CoilGrip, or to inject data into a Qi-aware implant. Demonstrates the air-gap covert channel risk of the charging interface. | Data exfiltration that looks like ordinary charging activity; cross-domain leakage testing for classified/high-assurance environments. |
| **FOD Spoof** | Manipulates the Foreign-Object-Detection calibration that Qi chargers use to decide it is safe to energize. | Safety-research: demonstrate that a malicious receiver can trick a charger into energizing an unsafe load, or hide a foreign object. **Authorization required — thermal hazard.** |
| **Sniffer** | Passive capture of Qi negotiation traffic between an unmodified charger and unmodified receiver (placed adjacent, no inline insertion). | Protocol fuzzing, reverse engineering of proprietary extended-power-profile (EPP) handshakes, charger firmware behaviour analysis. |

### Why CoilGrip is novel

Existing Qi/WPT security research (academic papers such as *Wisp*, *VoltKey*, *MobiCom '22 "Wireless Charging for Authentication"*) treats the Qi link either as a sensor or as a side channel observed off to the side. No fielded, general-purpose hardware tool:

1. **Inline-MITMs the Qi link.** CoilGrip is a true bidirectional interposer — it presents a complete Qi receiver to the charger and a complete Qi transmitter to the target, so negotiation packets can be observed *and rewritten* in real time.
2. **Delivers fault injection through the case.** Every published power-glitch attack requires shunt insertion on the target's VCC rail. CoilGrip glitches the target by modulating the *transferred* Qi power, which requires only physical proximity — a contactless glitcher.
3. **Treats Qi as a C2 channel.** CoilGrip can both read target-emitted load modulation and inject its own, so it functions as a transceiver over the charging interface — useful as an air-gap covert-channel test set.
4. **Runs fully untethered.** A Cortex-M7 + iCE40 FPGA compute the profiling, glitch timing and protocol handling on-board; the operator drives it from a phone over BLE and walks away.

---

## 2. Attack Surface and Threat Model

### The Qi protocol, briefly

A Qi system consists of a **Power Transmitter (PT)** — the charging pad — and a **Power Receiver (PR)** — the device being charged. The PT drives an LC tank at 110–205 kHz, transferring power inductively. The PR communicates back to the PT by **load modulation**: switching a capacitor in/out of its tank, which the PT detects as an amplitude/phase shift on its own coil current. This back-channel carries packets (header, data, checksum) at 2 kbaud. The PT can also communicate to the PR by **amplitude shift keying** of the carrier (10% modulation depth). Together these form the Qi packet layer used for:

- **Start of power transfer** and PR detection (analogue ping → digital ping).
- **Power Signal Report** (PR tells PT how much power it needs).
- **Control Error Packet** (PR's closed-loop power demand: increase/decrease/halt).
- **Received Power Packet** (PR reports actual received power; PT uses this for FOD).
- **Extended Power Profile (EPP)** negotiation (5 W BPP up to 15 W / 30 W EPP, with charger identification and proprietary authentication).
- **Configuration / identification packets** (PR manufacturer, model).

### What CoilGrip exposes

- **Charger identity spoofing.** EPP handshakes carry charger-identification data that some receivers use to gate "fast charge". CoilGrip can capture and replay a fast-charge identity to coerce a target into drawing more power than a given pad is certified for (a denial-of-service / thermal-stress primitive).
- **Negotiation MITM.** CoilGrip can rewrite the Control Error / Power Signal packets to silently starve the target (DoS masquerading as "slow charging"), to force an over-voltage profile, or to inject re-negotiations that reset the link on a schedule.
- **Power-side fingerprinting.** A device's instantaneous power draw while charging is a fingerprint of its state. Screen-on adds ~2–4 W; app launch adds transient spikes; a phone unlocking with biometrics produces a characteristic ~50 ms plateau. CoilGrip samples at 256 kS/s and applies a 256-point FFT + envelope detector to classify these states from the Qi field alone.
- **Contactless fault injection.** By abruptly dropping the transferred power, CoilGrip can brown-out the target's PMIC at a chosen execution phase. Many PMICs feed the SoC core rail directly from the Qi receiver's 5 V output through a buck regulator; a fast enough drop propagates as a brown-out before the target's brown-out detector asserts. The iCE40 FPGA delivers glitch timing with ±1 µs precision referenced to a trigger from a Qi control packet, a periodic timer, or an external GPIO.
- **Covert channel.** A compromised target (with the operator's consent) can toggle its Qi load-modulation capacitor on a per-bit basis to exfiltrate data at ~1–2 kbit/s to CoilGrip, entirely indistinguishable from ordinary charging negotiation on a spectrum analyzer. Conversely CoilGrip can inject commands to a Qi-aware malicious implant. This is the same physical layer the target already uses to charge — no new radio.
- **FOD bypass / spoofing.** Qi chargers calibrate FOD against the PR's *Received Power Packet*. CoilGrip can fabricate these packets to claim it is consuming less power than it really is, hiding a foreign object from the charger — a **safety-research** primitive (thermal hazard: do not energize without authorization and thermal monitoring).

### Threat model

| Asset | Adversary capability | CoilGrip role |
|-------|----------------------|----------------|
| Device identity / OS state of a charging phone | Physical proximity to a Qi charger (≤ 4 mm) | Power-profile fingerprinting |
| Fast-charge authorization on a target | Capture of a legitimate EPP handshake | Replay of charger identity (MITM mode) |
| Target boot/secure-boot integrity | Contactless power control during power-up | Contactless glitch at secure-boot signature check |
| Air-gapped data confidentiality | Compromised endpoint + Qi receiver | Load-modulation covert channel exfil |
| Charger safety (FOD) | Inline receiver MITM | FOD calibration manipulation (safety research) |
| Qi protocol robustness | Packet capture + fuzz | Sniffer + crafted-packet injection |

### Out of scope

CoilGrip does **not** attack the 60+ W proprietary fast-charge overlays (Mi Turbo, VOOC, Quick Charge inductive) beyond observing their negotiation, nor does it operate above the Qi-certified 30 W EPP envelope. It does not perform RF attacks (covered by sibling devices) — its entire interaction is via the magnetic near-field of the Qi link.

---

## 3. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32H743ZIT6, Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM |
| **FPGA co-processor** | Lattice iCE40-UP5K, 5.3 k LUTs, 1 Mb SPRAM — glitch-timing & protocol gating |
| **Qi TX side** | Custom class-D H-bridge driver (TPS28225 dual N-FET driver) + 100 µH planar coil, MCU-generated 140 kHz PWM via HRTIM |
| **Qi RX side** | TI BQ51013B Qi 1.1 receiver, 5 V out, I²C telemetry; or discrete synchronous rectifier (alt) |
| **Upstream interface** | NXP MWCT1011 Qi 1.2 TX controller (SPI) — drives CoilGrip as a receiver to the upstream charger (advanced mode) |
| **Load modulation** | ADG849 analog switch + 22 nF modulation cap, FPGA-timed |
| **Current sensing** | INA240A1 (high-side, 40 V, 200 kHz BW) + 10 mΩ shunt; 16-bit ADS131M04 ADC (4-ch, 32 kSps simultaneous) for power profiling |
| **Voltage sensing** | On-target 5 V rail + upstream coil amplitude via envelope detector (ADL5511) |
| **BLE** | nRF52840-M.2 module, BLE 5.0, AES-256-CTR C2 channel |
| **USB** | USB-C CDC (console / firmware update / power) |
| **Storage** | microSD (UHS-I), capture & trace logging; 16 MB SPI NOR (config, profiles) |
| **Display** | 0.96" OLED (SSD1306) — status, mode, glitch parameters |
| **Trigger I/O** | 4× GPIO trigger inputs/outputs (sync to external event or DUT) |
| **Power** | 1S 2000 mAh LiPo (internal, assists FOD behaviour + untethered ops), MCP73871 charger, USB-C or upstream Qi to charge CoilGrip itself |
| **Form factor** | 54 × 85 × 6 mm (credit-card footprint), 38 g, milled aluminium shield on coil side |
| **Thermal** | TMP117 sensor on TX coil + software FOD interlock; auto-shutdown > 60 °C coil |

### Architecture block diagram

```
                          ┌──────────────────────────────────────────────────────────────┐
                          │                       COILGRIP BOARD                          │
   Upstream Qi            │                                                              │
   Charger (PT)           │   ┌────────────┐   SPI   ┌────────────┐   SPI   ┌──────────┐  │
    ─── coil ─────────────┼──▶│ MWCT1011   │────────│ STM32H743  │────────│ iCE40-UP │  │
                          │   │ (Qi RX/TX  │   I²C  │  Cortex-M7 │   SPI   │  FPGA    │  │
                          │   │  ctrl)     │────────│            │────────│ glitch / │  │
                          │   └─────┬──────┘        │            │        │ protocol │  │
                          │         │5V             │  HRTIM PWM │        │ gating   │  │
                          │         ▼               └─────┬──────┘        └────┬─────┘  │
                          │   ┌────────────┐   I²C  ┌──────┴─────┐              │        │
                          │   │ BQ51013B   │────────│  INA240 +  │  ADS131M04   │        │
                          │   │ (Qi RX)    │        │  shunt     │──── current  │        │
                          │   └─────┬──────┘        │  sense     │    sensing   │        │
                          │         │5V             └──────┬─────┘              │        │
                          │         │                      │                    │        │
                          │         ▼                      ▼                    ▼        │
                          │   ┌──────────────────────────────────────────────────────┐  │
                          │   │  Power mux + load-mod switch (ADG849 + 22nF cap)      │  │
                          │   └──────────────────────┬───────────────────────────────┘  │
                          │                          │ 5V out (Qi-compliant)           │
   Downstream Qi          │                          ▼                                 │
   Target (PR)   ─── coil ────────────────────────────────────────────────────────────┘
```

Power flows **down** (charger → CoilGrip → target). Control/telemetry flows **up** (MCU ↔ MWCT1011/BQ51013B/FPGA). The load-modulation switch bridges the target's Qi back-channel into the FPGA for packet capture and glitch injection.

---

## 4. Firmware Design

The firmware is a cooperative RTOS-free state machine driven by the HRTIM high-resolution timer for Qi PWM and the ADS131M04 ADC ISR for profiling. See `firmware/` for the full source. Highlights:

- **`main.c`** — top-level orchestrator: mode dispatch (SNIFFER, MITM, PROFILE, GLITCH, EXFIL, FOD), BLE command loop, OLED status.
- **`drivers/qi_tx.c`** — Qi transmitter driver: HRTIM-configured 140 kHz full-bridge PWM, ping/digital-ping state machine, ASK modulation of carrier to send PT→PR packets, FOD calibration table.
- **`drivers/qi_rx.c`** — Qi receiver driver: BQ51013B I²C telemetry (rectified voltage, output current, temperature, control-error loop state), and load-modulation packet transmission via ADG849.
- **`drivers/power_profiler.c`** — ADS131M04 simultaneous sampling, 256-point sliding-window FFT, envelope detector, state classifier (idle / screen-on / app-launch / boot / unlock), device fingerprint DB on SD.
- **`drivers/glitch_engine.c`** — FPGA-backed precision power-drop engine: glitch window from 1 µs to 65 ms, trigger sources (timer, Qi packet match, external GPIO, ADC threshold), repeat/ramp, post-glitch log.
- **`drivers/load_modulator.c`** — Qi backscatter packet encoder/decoder (Qi 1.1 packet format, 11-bit header, CRC-8), used by both EXFIL and MITM.
- **`drivers/fod.c`** — FOD calibration manipulation + safety interlock (TMP117 thermal shutdown, power-clamp).
- **`drivers/ble_uart.c`** — AES-256-CTR encrypted BLE C2 protocol (Nordic UART Service).
- **`drivers/usb_cdc.c`** — USB-CDC console, command-line interface, log streaming.
- **`drivers/oled.c`** — SSD1306 status display.
- **`drivers/sdcard.c`** — FAT32 capture/trace/profile logging.
- **`drivers/config.c`** — config persistence in SPI NOR.
- **`drivers/board_init.c`** — clocks, GPIO, HRTIM, NVIC, peripheral bring-up.

### Key design decisions

1. **No RTOS, by choice.** The Qi timing and glitch path must be deterministic to the microsecond; a kernel would add jitter. The firmware uses a priority-ordered main loop with short ISRs for the ADC and HRTIM. This mirrors how real glitchers (e.g. ChipShouter) are built.
2. **FPGA for timing, MCU for policy.** The iCE40 holds the glitch sequencer and the load-modulation packet framer so that timing is cycle-accurate to the 12 MHz Qi-derived clock, while the MCU decides *what* to do. This split keeps the MCU free for BLE, profiling, and logging without disturbing the glitch path.
3. **MWCT1011 for the upstream TX control.** Rather than re-implementing a certified Qi 1.2 transmitter in firmware (certification-bound and fragile), CoilGrip uses the NXP dedicated Qi TX controller for its *own* transmitter to the downstream target. The MCU configures it over SPI and can override the FOD calibration — this is the manipulation vector. The MCU's HRTIM PWM is reserved for the load-modulation path and for the optional discrete TX in sniff mode.
4. **BQ51013B for the downstream RX.** A certified Qi receiver IC handles the rectifier, output regulation, and I²C telemetry for the upstream side. CoilGrip taps its load-modulation line into the FPGA.
5. **Dual safety interlocks.** The TMP117 monitors coil temperature and the firmware enforces a hard power clamp and auto-shutdown above 60 °C; FOD manipulation is gated behind an explicit operator "safety-research" unlock with a logged acknowledgement.

---

## 5. Application / Software Interface

The companion app (`app/`) is a React Native application (Expo-managed) that provides encrypted BLE control of CoilGrip. Screens:

- **Dashboard** — connection status, coil temperature, current power state, mode selector.
- **MitmControl** — upstream/downstream negotiation viewer, packet rewrite rules, charger-identity replay, DoS "trickle" mode.
- **PowerProfiler** — live current trace, FFT waterfall, classified device state feed, fingerprint DB management.
- **GlitchControl** — glitch parameter editor (width, offset, trigger source, repeat/ramp), live oscilloscope-style preview, run button with confirm.
- **CovertChannel** — exfil rate monitor, packet log, bit-error-rate display, inject mode.
- **Sniffer** — raw Qi packet capture viewer, export to PCAP-like format on SD.
- **Settings** — BLE pairing, encryption key exchange, safety-research unlock, firmware update, log management.

The BLE protocol uses a 16-byte AES-256-CTR session key derived from an ECDH P-256 handshake on first pairing; all commands and telemetry are framed with a sequence number and a 32-bit CRC.

---

## 6. Use Cases

### Red team: contactless secure-boot bypass

A target IoT lock has a Qi-charging dock and a secure-boot signature check on power-up. The operator drops CoilGrip into the dock, places the lock on it, and uses **GlitchControl** to repeatedly cycle the charging field with a 3 µs drop timed ~4.2 ms after the digital ping (empirically aligned with the signature-verify window). The brown-out skips a branch, the bootloader accepts an unsigned image, and the operator drops a persistence payload.

### Red team: device identification by charging signature

The operator wants to confirm which of several candidate phones in a meeting room is the target's, without touching them. CoilGrip in **Profile** mode is placed under a charging pad; it fingerprints each device's charge envelope and matches against the on-board database, reporting model and OS state in real time.

### Security researcher: Qi protocol fuzzing

A vendor's Qi charger is suspected of mis-handling malformed Control Error packets. CoilGrip in **MITM** mode injects out-of-range values into the PR→PT stream while monitoring the charger's response; crashes or unsafe power ramps are captured to SD for a vulnerability report.

### Air-gap tester: covert-channel exfil

A high-assurance facility permits charging phones on Qi pads but forbids radio. The operator's consented test endpoint toggles its load-modulation cap to exfil a token at 1.5 kbit/s; CoilGrip, inline on the pad, logs the channel and reports BER to demonstrate the air-gap leakage risk.

### Safety researcher: FOD demonstration

With explicit authorization and thermal monitoring, CoilGrip demonstrates that a manipulated receiver can hide a foreign object from a charger's FOD, providing evidence for a safety-disclosure report.

---

## 7. Bill of Materials (summary)

| Ref | Part | Function | Est. cost (USD) |
|-----|------|----------|-----------------|
| U1 | STM32H743ZIT6 | MCU | 14.50 |
| U2 | iCE40-UP5K-SG48 | FPGA | 6.20 |
| U3 | MWCT1011 | Upstream Qi TX ctrl | 3.80 |
| U4 | BQ51013B | Downstream Qi RX | 3.10 |
| U5 | ADS131M04 | 4-ch simultaneous ADC | 4.40 |
| U6 | INA240A1 | Current sense amp | 1.90 |
| U7 | ADG849 | Load-mod switch | 0.80 |
| U8 | nRF52840-M.2 | BLE | 7.50 |
| U9 | ADL5511 | Envelope detector | 2.30 |
| U10 | TMP117 | Thermal sensor | 1.20 |
| U11 | TPS28225 | H-bridge driver | 1.40 |
| U12 | W25Q128 | 16 MB SPI NOR | 0.90 |
| Misc | coils, passives, connectors, LiPo, OLED | — | 9.50 |
| **Total** | | | **~57.50** |

---

## 8. File Layout

```
coil-grip/
├── README.md                  (this file)
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── board_init.c
│       ├── qi_tx.c
│       ├── qi_rx.c
│       ├── power_profiler.c
│       ├── glitch_engine.c
│       ├── load_modulator.c
│       ├── fod.c
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
    ├── package.json
    ├── App.js
    ├── components/
    │   └── DeviceContext.js
    └── screens/
        ├── DashboardScreen.js
        ├── MitmControlScreen.js
        ├── PowerProfilerScreen.js
        ├── GlitchControlScreen.js
        ├── CovertChannelScreen.js
        ├── SnifferScreen.js
        └── SettingsScreen.js
```

---

## 9. Legal & Ethical Notes (reiterated)

CoilGrip can cause real thermal and electrical damage if misused. The FOD and over-power primitives in particular can heat foreign objects to ignition. **Only operate CoilGrip on devices and chargers you own or have explicit written authorization to test, never unattended, and always with the thermal interlock enabled.** Defeating FOD on a charger you do not own may violate product-safety regulations and is not a field operation — it is a lab demonstration for a safety disclosure. The author (**jayis1**) provides this design for authorized research and education only.

---

*Author: jayis1 · Copyright © 2026 jayis1 · Released under GPL-2.0*