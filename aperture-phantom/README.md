# AperturePhantom — MIPI CSI-2 Camera Interposer for Adversarial Computer-Vision Research

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Interception, capture, or injection of image data on devices you do not own may violate computer-fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), surveillance and biometric-data protection regulations (e.g., GDPR Art. 9, Illinois BIPA), and transportation safety statutes in your jurisdiction. Capturing imagery of persons without consent may additionally violate privacy and recording laws. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment, including authorization to capture any imagery that may contain identifiable persons.** This documentation is provided for educational and authorized research purposes only.

---

## Overview

**AperturePhantom** is a pocket-sized, battery-operated hardware interposer for **MIPI CSI-2 camera interface security research**. It sits physically in-line between a target's image sensor (or camera module) and its host SoC / application processor, on the 22-pin / 15-pin / 30-pin FPC ribbon that carries the CSI-2 D-PHY differential pairs, I²CCCI control bus, and power/clock lines. From this position it can:

1. **Passively capture** every frame the sensor produces — raw Bayer, YUV, or RGB — at wire speed, streaming to onboard SDRAM and SD card, with no observable perturbation of the host's image pipeline.
2. **Inject** synthetic frames toward the host SoC while suppressing or replacing the sensor's real output, so that the host's computer-vision stack (face recognition, object detection, ADAS, license-plate reader, presence sensing) processes attacker-chosen imagery instead of reality.
3. **Replay** previously recorded scenes (the "freeze-frame attack") so a camera appears to show a benign, authorized view while the physical scene has changed — e.g., holding the last authorized face frame, or a static empty corridor.
4. **Forge** the I²CCCI control channel to reconfigure the sensor on the fly (resolution, exposure, ROI, test-pattern), and to read back sensor-embedded metadata and OTP calibration/serial data.
5. **Fuzz** the host's CSI-2 receiver / image-signal-processor (ISP) driver by emitting malformed packets, short lines, bad virtual channels, illegal data types, and oversized payloads to find kernel driver parsing bugs.

The device is controlled from a phone over encrypted BLE or from a laptop over USB CDC. The companion app provides a live frame viewer, an injection-script editor, a replay library, a sensor I²CCCI console, and a CSI-2 protocol fuzzer.

### Why this is novel

Existing camera-security work is almost entirely software-side: attacking the CV model with digital adversarial examples, or analyzing the host application. **The analog, pre-ISP optical-to-digital link has been largely ignored** because tapping a 1–4 lane, 0.8–2.5 Gbps/lane MIPI D-PHY link is hard. AperturePhantom is, to the author's knowledge, the first open, purpose-built interposer that makes the CSI-2 link itself the attack surface. Compared to generic FPGA dev boards:

- It is **form-factor correct**: real FPC connectors matching the standard camera ribbons (22-pin 0.5 mm for Raspberry-Pi-class modules, 30-pin for phone modules, MIPI-CSI-HS universal breakouts), so it drops in with no soldering.
- It is **protocol-aware**: it speaks CSI-2 packet framing, virtual channels, data types (RAW8/10/12/14, YUV422 8/10-bit, RGB565/888, User Defined 8-bit), and line/frame start/end short packets, so it can decode and forge valid streams rather than blind bit-pipes.
- It is **self-contained and covert**: battery + onboard storage, no tethered laptop required for operation; the interposer is thin enough to sit inside a phone's camera flex recess or behind a module.
- It is **CV-aware at the link layer**: it can conditionally inject (e.g., "only when a face-detection trigger frame is seen", emulated by simple template matching against stored reference frames) to mount low-and-slow attacks that pass casual operator review.

The threat this concretely demonstrates: **a computer-vision system is only as trustworthy as the link between its sensor and its brain.** If an adversary can reach that physical link — and on IoT cameras, door-station modules, laptops, drones, and automotive camera harnesses, the FPC is routinely accessible — they own the system's perception of reality without touching the (often hardened) application processor at all.

---

## Attack Surface and Threat Model

### What AperturePhantom Exposes

MIPI CSI-2 is the dominant camera interface in phones, tablets, laptops, single-board computers, IoT cameras, drones, automotive vision modules, and increasingly industrial vision sensors. The 22/30-pin FPC is often unshielded, unlabeled, and physically exposed on test jigs, dev kits, and even production hardware (behind a bezel or module shield). AperturePhantom turns that exposed link into a full read/write surface:

| Capability | Technique | Security Impact |
|------------|-----------|-----------------|
| **Frame capture (surveillance)** | Pass through sensor→host while copying to SDRAM/SD | Exfiltrate all imagery the camera sees — faces, license plates, documents, PIN entry on keypads viewed by camera — even on hosts with locked-down application storage |
| **Frame injection (perception forgery)** | Replace sensor frames with synthetic/replayed frames | Defeat face recognition (present an authorized face), bypass presence/occupancy sensing, spoof ADAS lane/object detection, hide an intruder from a CCTV analytics pipeline |
| **Freeze-frame replay** | Hold the last "good" frame indefinitely | Make a monitored scene appear static and safe while the real scene changes; classic for physical-access bypass against camera-verified unlock |
| **Sensor reconfiguration** | I²CCCI master writes to sensor registers | Force long exposure (leak screen contents via optical), force test pattern, change ROI to exclude a region, disable, or brick the sensor to cause a fail-open behavior |
| **Sensor OTP / serial extraction** | I²CCCI reads from sensor EEPROM pages | Clone module identity; bypass per-module serial binding; harvest calibration data |
| **CSI-2 receiver fuzzing** | Emit malformed packets/lines/VCs from the FPGA | Find kernel ISP-driver bugs: buffer overflows, OOB reads, NULL derefs, DoS, potentially code execution in the camera/ISP kernel subsystem |
| **Covert channel exfiltration** | Encode captured data as imperceptible pixel perturbations on the injected stream | Move stolen data out through the host's own network stack once the CV app re-encodes and uploads the forged frames |
| **Timing attacks** | Inject only on trigger (template match) | Low-and-slow attacks that pass human review of recorded footage |

### Threat Model

| Asset | Adversary Capability | AperturePhantom Role |
|-------|---------------------|---------------------|
| Camera imagery / biometrics | Physical access to camera FPC | Passive tap & capture |
| CV decision integrity (unlock, detection) | In-line placement on camera link | Frame injection & replay |
| Sensor identity / calibration | I²CCCI bus access | OTP read & clone |
| Host kernel/ISP driver | Ability to emit CSI-2 toward host receiver | Protocol fuzzing for code-exec |
| Physical-access bypass (camera-verified door) | Momentary access to interpose + pre-recorded authorized frame | Freeze-frame replay |

**Out of scope:** wireless network attacks on the host (covered by other tools), defeating the CV *model* with digital perturbations applied after capture (that's software adversarial-ML work; AperturePhantom is about controlling the pre-model bitstream itself), and optical-domain attacks on the lens/sensor (laser blinding, etc. — see PhotonStrike).

### Assumptions & Limits

- The interposer requires **physical access to the camera FPC** for the duration of the operation (though it can be left in place covertly).
- High-end mobile SoCs sometimes route CSI-2 inside the package stack (PoP) or on buried PCB layers; those are out of reach. The vast majority of IoT/SBC/automotive/door-station modules expose a standard FPC.
- D-PHY link rates above 2.5 Gbps/lane (D-PHY v2.1 / C-PHY) are not supported by the selected FPGA transceivers in this revision.

---

## Hardware Specifications

| Parameter | Value |
|---|---|
| **Control MCU** | STMicro STM32H743VIT6 (Cortex-M7 @ 480 MHz, 2 MB flash, 1 MB SRAM, FMC, SDMMC, SPI, I²C, USB OTG-HS, DMA2D chroma artifact accelerator) |
| **MIPI bridge FPGA** | Lattice CrossLink-NX LIFMD-25 (non-volatile, native MIPI D-PHY TX/RX, 25 k LUTs, 18.5 Kb FIFOs, 2× 12.5 Gbps SerDes) — handles 1–4 lane CSI-2 at 0.8–2.5 Gbps/lane |
| **Frame SDRAM** | 128 Mbit (16 MB) IS42S16160J SDRAM on FMC; stores ~3 raw 1080p Bayer frames or ~24 RAW640x480 frames |
| **Bulk storage** | MicroSD (UHS-I, 4-bit SDMMC, FAT32 via FatFs) — capture/replay library |
| **Sensor control bus** | I²CCCI master (SCL/SDA, 100/400/1000 kHz, 7/10-bit addr) — 2 channels to drive either side of the link independently |
| **Host backhaul** | USB 2.0 OTG-HS CDC + BLE 5 (Silicon Labs EFR32BG22 module, AES-CCM encrypted GATT link) |
| **Connectors (target)** | 22-pin 0.5 mm FPC (Raspberry-Pi / SBC standard, both sides), 30-pin 0.4 mm FPC (phone module), each on a high-speed Hirose/Amphenol bottom-contact seat |
| **Power** | 1-cell LiPo (3.7 V, 600 mAh), MP2667 charger via USB-C, TPS62162 3.3 V rail; pass-through VBAT/clock to sensor with current monitoring |
| **Sensors (onboard)** | INA219 current/monitor on the sensor power rail (detect sensor power-down / tamper), DS28E07 secure element for interposer authentication |
| **Form factor** | 35 × 20 mm, 1.6 mm PCB, total build ≤ 2.2 mm with FPC sockets — fits behind standard camera modules |
| **Indicators** | Single RGB LED (status), vibration motor driver (silent alerts) |
| **User input** | Capacitive touch pad (3 zones) for armed/capture/inject without phone |

### Power Budget

| Rail | Draw (active capture+BLE) | Draw (idle passthrough) |
|---|---|---|
| FPGA + D-PHY | ~380 mW | ~120 mW (passthrough) |
| STM32H7 | ~110 mW | ~25 mW (sleep) |
| BLE + SD | ~60 mW | <1 mW |
| **Total** | ~550 mW → ~7 h on 600 mAh | ~150 mW → ~24 h passive |

---

## Architecture and Block Diagram

```
                 ┌──────────────────────── CAMERA MODULE / SENSOR ────────────────────────┐
                 │   IMX219 / OV5647 / OV5640 / sensor + I²CCCI slave                    │
                 └───────────┬──────────────────────────────────────┬─────────────────────┘
                  CSI-2 D-PHY lanes (1-4) │  I²CCCI (SCL/SDA) │ PWR/CLK
       ┌──────────────────────────────────┼──────────────────────┼──────────────────────┐
       │            ┌─────────────────────┴──────────────────────┴────────────────────┐ │
       │            │                     APERTURE-PHANTOM INTERPOSER                  │ │
       │            │                                                                      │ │
       │            │   ┌──────────────┐   ┌──────────────────────┐   ┌──────────────┐  │ │
       └─ D-PHY ───►│   │ CrossLink-NX │──►│   capture  ──► DMA   │──►│  STM32H743   │  │ │
                   │   │  (FPGA bridge│   │   FIFO               │   │   control MCU│  │ │
       ◄─ D-PHY ───│   │  RX+TX PHYs) │◄──│   inject  ◄─ DMA      │◄──│  (orchestr.) │  │ │
                   │   └──────┬───────┘   └──────────┬───────────┘   └──────┬───────┘  │ │
                   │          │ SPI cmd/status       │ FMC SDRAM bus            │ USB   │ │
                   │          └──────────────────────┘                          │ BLE   │ │
                   │                                                              ▼       │ │
                   │                                16 MB SDRAM  ◄── FMC ─────────┤       │ │
                   │                                MicroSD ◄── SDMMC ───────────┤       │ │
                   │                                I²CCCI masters (x2) ─────────►│ side-A│ │
                   │                                                                ►│ side-B│ │
                   │   ┌────────────────────────────────────────────────────────────┐       │ │
                   │   │ power pass-through + INA219 monitor + DS28E07 auth         │       │ │
                   │   └────────────────────────────────────────────────────────────┘       │ │
                   └───────────┬──────────────────────────────────────┬─────────────────────┘
                  CSI-2 D-PHY lanes (1-4) │  I²CCCI (SCL/SDA) │ PWR/CLK
                 ┌──────────────────────────────────┴──────────────────┴────────────────────┐
                 │            HOST SoC / APPLICATION PROCESSOR  (CSI-2 RX + ISP + CV)         │
                 └──────────────────────────────────────────────────────────────────────────┘

   Backhaul:  USB CDC ──► laptop      BLE ──► phone (AperturePhantom app)
```

**Data flow for capture:** Sensor → FPGA RX PHY → CSI-2 packet decode → FPGA line/frame FIFO → FMC DMA → STM32H7 → SDRAM staging → SD card (FatFs) and/or BLE/USB stream to app.

**Data flow for injection:** App/script/SD → STM32H7 → FMC DMA → FPGA inject FIFO → CSI-2 packet encode → FPGA TX PHY → Host CSI-2 receiver (sensor is gated off or kept alive for I²CCCI continuity).

**Passthrough mode:** FPGA routes sensor→host with a parallel copy tap; no latency added (sub-100 ps skew), so the host's link training and exposure-control loop are undisturbed.

---

## Firmware Details and Design Decisions

The firmware runs on the STM32H743 control MCU. The CrossLink-NX FPGA runs gateware (described in `firmware/fpga_gateware.md`) that provides a SPI command/status interface to the MCU and presents the CSI-2 link as two FIFOs (capture FIFO and inject FIFO) plus a status register file (lane state, line/word counts, CRC errors).

Key design decisions:

- **MCU/FPGA split.** The MCU handles everything *except* the high-speed serial itself: command protocol, frame buffering, replay scheduling, the I²CCCI sensor bus, storage, BLE/USB backhaul, the script engine, and trigger matching. The FPGA is treated as a fast I/O engine with a FIFO interface. This keeps the firmware portable and auditable in C, while the bit-level PHY work lives in vendor IP we don't reimplement.
- **DMA-first.** All frame movement uses MDMA/BDMA over FMC and SDMMC; the CPU never copies pixel data. The DMA2D chroma-ARM accelerator is used for the adversarial-patch overlay engine (blit a small region into a captured frame before re-injection).
- **FatFs on SDMMC** for the capture/replay library; frames stored as raw headered files (`APF` magic, width/height/dt/payload), enabling frame-exact replay.
- **Script engine** (a tiny bytecode interpreted on the MCU) for autonomous, phone-less operation: sequences of `WAIT ms`, `CAPTURE n`, `INJECT slot`, `REPLAY file`, `SENSOR_WRITE addr val`, `IF_TRIGGER slot`, `LOOP`, `BLE_NOTIFY`. Stored in flash; selected via touch pad or BLE.
- **Trigger matching** uses a downsampled, 16-bin-per-channel histogram difference between the live frame and a stored reference; if Δ exceeds a threshold, a trigger fires (cheap, runs in the MCU SRAM, no full template matching needed). This powers the "inject only when a face-shaped blob enters the ROI" low-and-slow mode.
- **Encrypted BLE** (AES-CCM, key from DS28E07 secure-element key agreement) so a captured-frames stream to the phone is not plaintext over the air.
- **Sensor continuity on inject:** when injecting, the I²CCCI side-A master keeps issuing the sensor's expected register-read responses toward the host where applicable, and the FPGA keeps the sensor's clock alive, so a host that polls the sensor doesn't notice the swap. A "mute sensor" path gates the sensor's D-PHY TX via a sensor register write (most sensors support a soft TX_DISABLE).
- **Power/tamper awareness:** the INA219 monitors the sensor VANA rail; an unexpected drop (host powered the sensor off) triggers an alert and graceful mode change.

See `firmware/` for the full C implementation (`main.c`, `registers.h`, `board.h`, `Makefile`, and drivers in `firmware/drivers/`), and `firmware/fpga_gateware.md` for the FPGA-side contract.

---

## Application / Software Interface

The companion app (React Native, in `app/`) talks to the device over BLE GATT or USB CDC using a binary framing protocol (`app/utils/protocol.js`). Screens:

- **ConnectionScreen** — scan, pair, authenticate via DS28E07 challenge, show link status (lanes, rate, data type).
- **DashboardScreen** — armed/disarmed, current mode (PASSTHROUGH / CAPTURE / INJECT / REPLAY / FUZZ), battery, SD free space, sensor power state.
- **LiveViewScreen** — downscaled live preview of captured frames (sensor-side view) over BLE; snapshot button.
- **ReplayLibraryScreen** — list recorded scenes on SD; pick one, set loop count, schedule.
- **InjectEditorScreen** — compose an injection: pick a stored frame or draw a patch, set target ROI, alpha, timing; push to a device "slot".
- **ScriptEditorScreen** — edit and upload the on-device bytecode script (trigger-gated inject, freeze-frame, fuzz sequence).
- **SensorConsoleScreen** — I²CCCI register read/write console for the attached sensor (auto-detect common sensors: IMX219, OV5647, OV5640, OV5645, IMX477).
- **FuzzerScreen** — pick CSI-2 fuzz strategies (short lines, bad DT, bad VC, oversize payload, CRC errors) and run them against the host receiver.

The app does **not** itself run any CV model; it is a control & capture interface. Post-capture analysis of captured frames is left to the operator's offline tooling (the device stores raw Bayer, convertible to standard formats offline).

---

## Use Cases

### Red team
- **Camera-verified physical access bypass.** Pre-record an authorized occupant's face at the door-station camera, interpose AperturePhantom, and replay the frozen frame so the door's CV-based unlock sees a "present authorized person" while the real scene is the red teamer.
- **Surveillance exfiltration.** On a deployed IoT camera with locked-down app storage, tap the CSI-2 link and stream all imagery to the operator — bypassing the host's encryption-at-rest entirely.
- **Perception denial in ADAS research.** Inject an empty-road frame into a test-bench ADAS camera feed to demonstrate that lane-keep / AEB can be blinded at the link layer (authorized test bench only).

### Security researchers
- **ISP/CSI-2 driver fuzzing.** Systematically probe the host kernel's CSI-2 receiver and ISP driver for parsing bugs with malformed packets.
- **Sensor-identity cloning research.** Read sensor OTP/serial pages and study per-module binding schemes.
- **Covert-channel characterization.** Use the pixel-perturbation exfil channel to measure how much data a CV app's re-encode/upload pipeline will launder undetected.

### Penetration testers
- **Biometric liveness bypass studies.** Demonstrate to a client that their liveness detection trusts a link they don't control, and recommend link-side attestation (authenticated sensor ↔ SoC channel).
- **Camera-module supply-chain checks.** Verify a module's reported serial/calibration against a known-good manifest by reading OTP over I²CCCI.

---

## Ethical & Legal Notes (reiterated)

This tool is for authorized research on systems you own or are explicitly authorized to test. Capturing imagery of people may require their consent under applicable law. Do not deploy on public-facing cameras, road vehicles in traffic, or any device you do not own without written authorization. The design is published to let defenders build link-side attestation and to give researchers a reproducible platform — not to facilitate unauthorized surveillance.

Author: **jayis1** · License: GPL-2.0 · Status: design reference