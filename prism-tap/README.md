# Prism-Tap — MIPI CSI-2 / DSI Camera & Display Interface Tap & Injection Implant

```
   ╔════════════════════════════════════════════════════════════════════════╗
   ║  ██████╗ ██████╗  ██████╗ ██╗  ██╗██████╗  ██████╗ ██╗  ██╗███████╗    ║
   ║  ██╔══██╗██╔══██╗██╔═══██╗╚██╗██╔╝██╔══██╗██╔═══██╗██║ ██╔╝██╔════╝    ║
   ║  ██████╔╝██║  ██║██║   ██║ ╚███╔╝ ██████╔╝██║   ██║█████╔╝ █████╗      ║
   ║  ██╔══██╗██║  ██║██║   ██║ ██╔██╗ ██╔═══╝ ██║   ██║██╔═██╗ ██╔══╝      ║
   ║  ██║  ██║██████╔╝╚██████╔╝██╔╝ ██╗██║     ╚██████╔╝██║  ██╗███████╗    ║
   ║  ╚═╝  ╚═╝╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚═╝      ╚═════╝ ╚═╝  ╚═╝╚══════╝    ║
   ║                                                                        ║
   ║  MIPI CSI-2 / DSI Camera & Display Interface Tap & Injection Implant   ║
   ╚════════════════════════════════════════════════════════════════════════╝
```

**Author:** jayis1  
**Version:** 1.0  
**Date:** 2026-08-07  
**License:** GPL-2.0 / CERN-OHL-S v2  
**Status:** Research hardware design — firmware + PCB + companion app

---

## ⚠️ Legal & Ethical Disclaimer

**This device is designed exclusively for authorized security research, penetration testing, and red team operations.** Use of Prism-Tap against any device, camera module, display panel, or embedded system without explicit written authorization from the asset owner is illegal in most jurisdictions and may violate computer fraud and abuse statutes (e.g., 18 U.S.C. § 1030 CFAA), wiretap laws (18 U.S.C. § 2511), and privacy/surveillance regulations. Intercepting camera or display data from devices you do not own may constitute unauthorized interception of electronic communications. The author (**jayis1**) does not condone or support unauthorized surveillance, data exfiltration, or tampering with any device. You are solely responsible for ensuring you have proper authorization before deploying this device. This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**Prism-Tap** is a compact, battery-powered inline tap and man-in-the-middle (MITM) implant for **MIPI CSI-2 (Camera Serial Interface)** and **MIPI DSI (Display Serial Interface)** buses — the high-speed serial interfaces used by virtually all smartphone cameras, tablet displays, automotive vision systems, drone cameras, IoT camera modules, and an increasing number of laptop webcams and embedded displays.

Unlike traditional network taps or USB-based HID injectors, Prism-Tap targets the **physical layer between image sensors and application processors**, and between application processors and display panels. This is an attack surface that has received almost no attention in the offensive security community, despite being present in billions of devices worldwide. The MIPI D-PHY physical layer runs at hundreds of megatransfers per second (up to 2.5 Gbps per lane in CSI-2 v1.3), making it inaccessible to generic logic analyzers or microcontrollers without specialized PHY hardware.

Prism-Tap fills this gap. It bridges MIPI CSI-2 or DSI links through an onboard FPGA that performs real-time packet inspection, selective capture, frame injection, and protocol-aware manipulation — all while maintaining the timing constraints required by the host processor so the target device never detects the tap.

### Key Capabilities

| Capability | CSI-2 (Camera) | DSI (Display) |
|---|---|---|
| **Passive capture** | Raw Bayer/YUV frames from image sensor | Display frame buffer contents |
| **Frame injection** | Inject synthetic camera frames (spoofing) | Inject overlay content or full screen replacement |
| **Selective drop** | Drop specific frames to cause sensor blindness | Drop frames to blank/flicker display |
| **Replay attack** | Replay previously captured frames | Replay previously captured display content |
| **Timing manipulation** | Delay frames to disrupt ISP pipelines | Alter frame timing for flicker/jitter injection |
| **Metadata spoof** | Modify embedded data lines (exposure, gain) | Alter DSI packet headers and DCS commands |

### Why MIPI Interfaces Matter

MIPI CSI-2 and DSI are the dominant high-speed serial interfaces for cameras and displays in mobile and embedded devices:

- **Smartphones:** Every modern phone uses CSI-2 for its main selfie and rear cameras, and DSI for its OLED/LCD panel.
- **Tablets & Laptops:** Increasingly use MIPI for integrated webcams and touch displays.
- **Automotive:** ADAS cameras (rear-view, lane-keeping, autonomous driving) overwhelmingly use CSI-2.
- **Drones:** FPV and navigation cameras use CSI-2 to connect to flight controllers.
- **IoT:** Smart cameras, doorbells, and industrial vision systems use CSI-2.
- **Medical:** Endoscope and imaging modules use MIPI interfaces.

An inline tap on these interfaces enables:
- **Biometric data exfiltration:** Capturing raw fingerprint or face recognition camera data before it reaches the secure enclave.
- **Camera spoofing:** Injecting synthetic frames to bypass liveness detection or fool computer vision systems.
- **Display content theft:** Capturing sensitive information shown on screen.
- **Display injection:** Overlaying malicious content or injecting fake UI elements.
- **Automotive security:** Testing ADAS camera input integrity and injection resistance.
- **Forensic capture:** Capturing raw sensor data before any host-side processing or encryption.

---

## 2. Attack Surface & Threat Model

### Attack Surface

Prism-Tap operates at the **MIPI D-PHY physical layer**, sitting inline between a MIPI transmitter (sensor or AP) and a MIPI receiver (AP or display controller). The D-PHY uses differential signaling with a clock lane and 1–4 data lanes, each running at up to 2.5 Gbps (CSI-2 v1.3) or 4.5 Gbps (D-PHY v2.1). The interface uses both a high-speed (HS) mode and a low-power (LP) mode for control signaling.

```
 ┌─────────────┐     MIPI CSI-2 / DSI      ┌───────────┐     Re-driven MIPI     ┌──────────────┐
 │  Image      │  ──── D-PHY Lanes ──────▶│           │  ──── D-PHY Lanes ──▶│  Application  │
 │  Sensor or  │  ◀──── LP Control -------│ Prism-Tap │  ◀──── LP Control --│  Processor or │
 │  AP (Tx)    │                           │           │                      │  Display (Rx) │
 └─────────────┘                           └───────────┘                      └──────────────┘
                                                  │
                                           ┌──────┴──────┐
                                           │  FPGA Core  │ ← Packet inspection, capture,
                                           │  + Bridge   │   injection, timing manipulation
                                           └──────┬──────┘
                                                  │
                                         ┌────────┼────────┐
                                         ▼        ▼        ▼
                                    ┌─────┐ ┌─────┐ ┌──────┐
                                    │ BLE │ │ SD  │ │ USB-C│
                                    │ C2  │ │ Card│ │ Exfil│
                                    └─────┘ └─────┘ └──────┘
```

### Threat Model

**Assets at risk:**
1. **Raw biometric data** — Fingerprint sensors, iris scanners, and face-recognition cameras transmit raw, unprocessed biometric data over CSI-2 before any secure-enclave processing. This data is never exposed via software APIs, making a physical tap the only way to access it.
2. **Camera input integrity** — Computer vision and liveness-detection systems trust that CSI-2 data genuinely originates from the physical sensor. Frame injection breaks this trust boundary.
3. **Display output confidentiality** — Screen contents (passwords, messages, documents, MFA codes) transmitted over DSI are assumed to be confined to the physical display panel.
4. **ADAS sensor trust** — Automotive cameras feed safety-critical systems; injecting or suppressing frames can cause dangerous misbehavior.

**Adversary model:**
- **Physical access required:** Prism-Tap must be inserted inline on the MIPI bus, requiring physical access to the target device's internal camera/display FPC connectors or flex cable.
- **Stealth consideration:** The device is designed to be electrically transparent — it re-drives the MIPI signal with minimal latency so the host processor's CSI-2/DSI controller does not detect timing violations or link errors.
- **No software footprint:** Because the tap operates below the OS driver layer, no software artifact is left on the target. Host-side secure boot, full-disk encryption, and OS-level hardening provide no protection against this attack.

**Out of scope:**
- Remote/network-based attacks (Prism-Tap requires physical access)
- Attacks on MIPI I3C/SoundWire (different protocols, separate devices)
- Breaking MIPI D-PHY link encryption (CSI-2 v2.0 adds optional link encryption; Prism-Tap targets the vast majority of devices that do not use it)

---

## 3. Hardware Specifications

### System Architecture

Prism-Tap uses a three-chip architecture: a **MIPI bridge front-end** (receiver + transmitter), an **FPGA** for real-time packet processing, and an **MCU** for control, storage, and wireless C2.

| Component | Part | Role |
|---|---|---|
| **MCU** | STM32H730VB — Cortex-M7 @ 550 MHz, 128 KB SRAM, 1 MB Flash | Control, BLE stack, SD card, USB-C, command processing |
| **FPGA** | Lattice iCE40-UP5K — 5.3K LUTs, 16 PRAM, 4 DSP blocks | Real-time MIPI packet inspection, frame buffering, injection |
| **MIPI CSI-2 Rx** | Toshiba TC358746XBG — 1/2-lane CSI-2 input, parallel output | Receives camera/sensor MIPI stream |
| **MIPI CSI-2 Tx** | Toshiba TC358748XBG — parallel input, 1/2-lane CSI-2 output | Re-drives camera stream to AP |
| **MIPI DSI Rx** | Analog Devices ADV7480 — DSI input bridge | Receives display MIPI stream |
| **MIPI DSI Tx** | Synopsys DesignWare DSI Host (in FPGA soft-IP) or TC358762 | Re-drives display stream to panel |
| **BLE** | nRF52840-M.2 module — BLE 5.0, AES-256-CTR | Encrypted C2 channel |
| **Storage** | microSD card slot (UHS-I, 50 MB/s) — PCAP-like frame capture | Frame/log storage |
| **SPI NOR** | 32 MB W25Q256 — configuration + firmware backup | FPGA bitstream + MCU backup |
| **USB-C** | USB 2.0 HS (480 Mbps) via MCU PHY | Data exfiltration, firmware update |
| **Power** | 1200 mAh Li-Po, bq24074 charger, TPS62743 regulator | Battery operation + USB charging |
| **Display** | 0.96" OLED (SSD1306, I2C) | Status display |
| **Form Factor** | 45 × 25 × 8 mm PCB, flex cable pigtails | Inline tap form factor |

### Detailed Specs

| Parameter | Value |
|---|---|
| **MIPI CSI-2 lanes** | 1–2 lane Rx + 1–2 lane Tx (re-driven) |
| **MIPI DSI lanes** | 1–2 lane Rx + 1–2 lane Tx (re-driven) |
| **Max data rate** | 1.5 Gbps/lane (CSI-2 v1.3 compliant) |
| **Max resolution** | 4K @ 30fps (CSI-2) or 1080p @ 60fps (DSI) |
| **Tap latency** | < 200 ns (cut-through, FPGA pipeline) |
| **Capture modes** | Full frame, frame diff, metadata-only, selective ROI |
| **Injection modes** | Full frame replace, overlay, selective frame, timing manipulation |
| **BLE range** | 10 m line-of-sight (encrypted C2) |
| **Battery life** | 4–6 hours (active tap), 24+ hours (capture-only) |
| **Storage** | Up to 2 TB microSD (FAT32/exFAT) |
| **USB-C** | USB 2.0 HS, CDC-ACM virtual serial + mass storage |
| **Operating temp** | -10°C to +60°C |
| **Weight** | ~18 g (with battery) |

### Power Architecture

Prism-Tap is battery-powered for field deployment. In tap mode, the FPGA and both MIPI bridges are active, drawing approximately 180 mA at 3.3V. The MCU runs at reduced clock (80 MHz) to save power while maintaining BLE connectivity. A low-power "capture-only" mode disables the Tx bridge (one-directional tap) and reduces FPGA clock, extending battery life to 24+ hours. The device can also be USB-C powered for continuous bench operation.

### MIPI D-PHY Front-End

The D-PHY front-end is the critical analog section. Each MIPI lane consists of two differential pairs (Dp/Dn). The TC358746/748 bridge chips handle the D-PHY physical layer (termination, level shifting, clock recovery) and output a parallel pixel bus to the FPGA. The FPGA performs:

1. **Packet parsing** — CSI-2/DSI packet headers, payload CRC checks, short/long packet classification
2. **Frame buffering** — Ping-pong buffers in dual-port Block RAM for zero-loss capture
3. **Injection engine** — Replace frame buffer contents, insert overlay graphics, drop frames
4. **Timing passthrough** — Re-generate LP/HS signaling on the output bridge to maintain link integrity

---

## 4. Architecture & Block Diagram

```
 ┌──────────────────────────────────────────────────────────────────────────┐
 │                         PRISM-TAP BOARD                                  │
 │                                                                          │
 │   ┌────────┐   MIPI    ┌──────────┐  Parallel  ┌───────────┐  Parallel  │
 │   │ Camera │──CSI-2──▶│ TC358746 │──Bus(16b)─▶│           │──Bus(16b)─▶┌──────────┐
 │   │ Sensor │   D-PHY  │  (Rx)    │  + Syncs  │           │  + Syncs   │ TC358748 │
 │   └────────┘          └──────────┘           │  iCE40    │            │  (Tx)    │──CSI-2──▶ AP
 │                                                │  UP5K    │            └──────────┘
 │   ┌────────┐   MIPI    ┌──────────┐  Parallel  │  FPGA    │  Parallel  ┌──────────┐
 │   │ App    │──DSI────▶│ ADV7480  │──Bus(16b)─▶│          │──Bus(16b)─▶│TC358762  │──DSI────▶ Display
 │   │ Proc   │   D-PHY  │  (Rx)    │  + Syncs  │           │  + Syncs   │  (Tx)    │
 │   └────────┘          └──────────┘           └─────┬─────┘            └──────────┘
 │                                                    │ SPI
 │                                              ┌─────┴─────┐
 │                                              │ STM32H730 │
 │                                              │   (MCU)   │
 │                                              └──┬──┬──┬──┘
 │                       ┌────────────────────────┘  │  └───────────────┐
 │                  ┌────┴────┐  ┌────┴────┐  ┌──────┴──────┐  ┌───────┴────┐
 │                  │ nRF52840│  │ microSD │  │  USB-C     │  │  SSD1306   │
 │                  │  (BLE)  │  │  Slot   │  │  Connector │  │  OLED 0.96"│
 │                  └─────────┘  └─────────┘  └────────────┘  └────────────┘
 │                                                                          │
 │   ┌─────────────────────────────────────────────────────────────────┐   │
 │   │  Power: bq24074 Charger + TPS62743 Buck + 1200mAh Li-Po        │   │
 │   └─────────────────────────────────────────────────────────────────┘   │
 └──────────────────────────────────────────────────────────────────────────┘
```

### Data Flow

1. **Camera tap path:** Image sensor → TC358746 (CSI-2 Rx) → parallel bus → FPGA → parallel bus → TC358748 (CSI-2 Tx) → Application processor. The FPGA inspects every packet in real-time.

2. **Display tap path:** AP → ADV7480 (DSI Rx) → parallel bus → FPGA → parallel bus → TC358762 (DSI Tx) → Display panel.

3. **Capture path:** FPGA → SPI DMA → MCU → SD card (frames) and/or BLE (thumbnails, metadata) and/or USB-C (bulk frame export).

4. **Injection path:** MCU → SPI → FPGA frame buffer → re-transmitted via output bridge. Pre-loaded frames or real-time MCU-generated content.

5. **C2 path:** Operator app → BLE (AES-256-CTR) → MCU → command dispatch → FPGA/bridges.

---

## 5. Firmware Details & Design Decisions

### Firmware Architecture

The firmware runs on the STM32H730 MCU under bare-metal (no RTOS) with a cooperative scheduler. This avoids RTOS overhead while maintaining deterministic BLE timing via interrupt-prioritized ISRs.

**Design decisions:**
- **Bare-metal over FreeRTOS:** The BLE stack (SoftDevice equivalent via nRF52840's separate radio) runs on the nRF52840 module independently; the STM32 only needs to service SPI/SD/USB interrupts. A cooperative main loop with priority ISRs is simpler and more debuggable.
- **FPGA bitstream loaded from SPI NOR at boot:** The iCE40 is SRAM-based and must be configured at power-on. The MCU loads the bitstream from the 32 MB W25Q256 SPI NOR flash via SPI at boot.
- **Dual-mode capture:** The FPGA writes frames to its internal Block RAM in a ping-pong configuration. The MCU reads one buffer via SPI while the FPGA fills the other, enabling zero-loss capture at full frame rate.
- **Frame compression:** The MCU can optionally JPEG-encode frames (using the H7's hardware JPEG accelerator) before writing to SD, reducing storage requirements by ~10:1.
- **Encrypted C2:** BLE communication uses AES-256-CTR with a session key derived from a pre-shared key via ECDH (P-256). The nRF52840 handles the BLE/ECC crypto; the STM32 handles AES via its hardware crypto accelerator.

### Firmware Modules

| Module | File | Purpose |
|---|---|---|
| Main loop & scheduler | `main.c` | Boot, init, command dispatch, power management |
| Board config | `board.h` | Pin assignments, clock config, peripheral mapping |
| Register defs | `registers.h` | MCU register base addresses, bit definitions |
| MIPI bridge control | `drivers/mipi_bridge.c/h` | I2C config of TC358746/748/ADV7480/TC358762 |
| FPGA interface | `drivers/fpga_if.c/h` | SPI comms, bitstream load, register access, DMA |
| Frame capture | `drivers/frame_capture.c/h` | Frame buffer management, SD write, JPEG encode |
| Frame injection | `drivers/frame_inject.c/h` | Pre-loaded frames, real-time injection control |
| BLE C2 | `drivers/ble_if.c/h` | nRF52840 UART link, AES encrypt/decrypt, protocol |
| SD card | `drivers/sdcard.c/h` | FAT32 file system, frame file management |
| USB CDC | `drivers/usb_cdc.c/h` | Virtual serial for command + bulk frame exfil |
| OLED display | `drivers/oled.c/h` | Status display, simple menu rendering |
| Command protocol | `drivers/protocol.c/h` | Binary command/response framing over BLE/USB |
| Power management | `drivers/power.c/h` | Battery monitoring, mode switching, charger control |

### Frame File Format

Captured frames are stored on SD card in a custom `.ptf` (Prism-Tap Frame) container:

```
Offset  Size  Field
0x0000  16    Magic: "PRISMTAP-FRAME\0"
0x0010  4     Version (uint32_t = 1)
0x0014  4     Frame index (uint32_t)
0x0018  4     Timestamp (uint32_t, ms since boot)
0x001C  4     Width (uint16_t) + Height (uint16_t)
0x0020  4     Format (enum: RAW8, RAW10, RAW12, YUV422, RGB888, JPEG)
0x0024  4     Frame size (uint32_t, bytes)
0x0028  16    CRC-32 + reserved
0x0038  ...   Frame payload
```

---

## 6. Application / Software Interface

### Companion App (React Native)

Prism-Tap includes a React Native companion app for iOS and Android that provides:

- **Device connection** — BLE scan, pairing, encrypted session establishment
- **Live capture view** — Real-time camera/display frame preview (downscaled thumbnails via BLE)
- **Frame injection** — Select and inject pre-loaded frames or captured frames from gallery
- **Capture gallery** — Browse, export, and delete captured frames from SD card
- **Mode control** — Switch between tap modes (passive, active MITM, capture-only, inject-only)
- **Settings** — Configure frame format, capture interval, injection triggers, encryption keys
- **Exfiltration** — Bulk download captured frames via USB-C or BLE (batch)

### BLE Command Protocol

The app communicates with Prism-Tap over BLE using a binary command/response protocol:

| Command | Opcode | Description |
|---|---|---|
| `PING` | 0x01 | Heartbeat / latency check |
| `GET_STATUS` | 0x02 | Battery, mode, link status |
| `SET_MODE` | 0x03 | Set operating mode |
| `START_CAPTURE` | 0x10 | Begin frame capture to SD |
| `STOP_CAPTURE` | 0x11 | Stop capture |
| `GET_FRAME_THUMB` | 0x12 | Request downscaled frame thumbnail |
| `GET_FRAME_LIST` | 0x13 | List captured frames on SD |
| `LOAD_INJECT_FRAME` | 0x20 | Upload frame to FPGA for injection |
| `START_INJECT` | 0x21 | Begin injecting loaded frame |
| `STOP_INJECT` | 0x22 | Stop injection, resume passthrough |
| `SET_INJECT_MODE` | 0x23 | Full-replace / overlay / selective |
| `SET_TIMING_OFFSET` | 0x30 | Adjust frame timing (delay/jitter) |
| `DROP_FRAMES` | 0x31 | Drop next N frames |
| `GET_CONFIG` | 0x40 | Read bridge/FPGA configuration |
| `SET_CONFIG` | 0x41 | Write bridge/FPGA configuration |
| `ERASE_FRAMES` | 0x50 | Delete all captured frames |
| `EXPORT_FRAMES` | 0x51 | Bulk export via USB-C |
| `FW_UPDATE` | 0x60 | Firmware update mode |

---

## 7. Use Cases

### Red Team Operations

1. **Biometric data exfiltration:** Insert Prism-Tap inline on a target smartphone's camera FPC. Capture raw face-recognition or iris-scan frames before they reach the secure enclave. This data, never exposed via software APIs, can reveal biometric templates or be used for spoofing attacks.

2. **Camera spoofing at security checkpoints:** Inject synthetic frames into an access control system's camera to bypass liveness detection, face recognition, or license-plate readers. Pre-load a video sequence into the FPGA and trigger injection at the right moment.

3. **Automotive ADAS manipulation:** Tap into a vehicle's rear-view or lane-keeping camera bus. Inject crafted frames to cause the vision system to misclassify road conditions, or selectively drop frames to create momentary "blindness" in safety-critical systems.

4. **Drone camera hijacking:** Intercept a drone's FPV camera stream and inject synthetic imagery to mislead the operator or autonomous navigation system.

### Security Research

5. **ISP (Image Signal Processor) fuzzing:** Feed crafted raw Bayer frames into an AP's ISP to test for buffer overflows, integer overflows, or denial-of-service conditions in the image processing pipeline.

6. **Liveness detection bypass research:** Capture and replay frames from a real user's face to test the resilience of liveness detection algorithms against replay attacks at the physical sensor interface.

7. **Display content capture:** Tap a laptop's eDP/DSI display link to capture screen contents (including MFA codes, passwords, sensitive documents) that are transmitted to the panel in cleartext.

8. **Display injection:** Inject overlay content or full-screen replacement on a target's display to test UI-level trust boundaries or conduct phishing-style attacks at the hardware level.

### Penetration Testing

9. **Forensic image capture:** During physical access to a seized device, capture raw camera frames for forensic analysis without booting the OS (which might trigger remote wipe or tamper-evident logging).

10. **IoT camera assessment:** Test the security of smart cameras, doorbells, and industrial vision systems by intercepting and manipulating their camera input at the CSI-2 bus level.

11. **Trust boundary validation:** Verify whether a target system's secure boot and OS hardening actually protect against physical-layer attacks on its camera/display interfaces.

---

## 8. Bill of Materials (Summary)

| Ref | Part | Package | Qty | Notes |
|---|---|---|---|---|
| U1 | STM32H730VBT6 | LQFP-48 | 1 | Main MCU |
| U2 | Lattice iCE40-UP5K-SG48 | SG48 | 1 | FPGA |
| U3 | Toshiba TC358746XBG | BGA-84 | 1 | CSI-2 Rx bridge |
| U4 | Toshiba TC358748XBG | BGA-84 | 1 | CSI-2 Tx bridge |
| U5 | Analog Devices ADV7480 | BGA-32 | 1 | DSI Rx bridge |
| U6 | Toshiba TC358762XBG | BGA-108 | 1 | DSI Tx bridge |
| U7 | nRF52840-M2 | M.2 module | 1 | BLE 5.0 |
| U8 | W25Q256JVSIQ | SOIC-16 | 1 | 32 MB SPI NOR |
| U9 | SSD1306 OLED | COG 0.96" | 1 | Status display |
| U10 | bq24074 | VQFN-10 | 1 | Li-Po charger |
| U11 | TPS62743 | DSBGA-6 | 1 | 3.3V buck regulator |
| J1 | USB-C 16-pin | SMD | 1 | USB + power |
| J2 | microSD socket | SMD push-push | 1 | Storage |
| J3 | MIPI CSI-2 input FPC | 22-pin 0.5mm | 1 | Camera input |
| J4 | MIPI CSI-2 output FPC | 22-pin 0.5mm | 1 | Camera output |
| J5 | MIPI DSI input FPC | 30-pin 0.4mm | 1 | Display input |
| J6 | MIPI DSI output FPC | 30-pin 0.4mm | 1 | Display output |
| BAT1 | 1200 mAh Li-Po | Custom | 1 | 3.7V pouch cell |

---

## 9. Limitations & Future Work

### Current Limitations

- **2-lane maximum:** The current design supports 1–2 lane MIPI interfaces. Some high-end smartphones use 4-lane CSI-2 for high-resolution sensors; a future revision could use 4-lane bridge chips.
- **D-PHY v1.2 only:** The TC358746/748 support up to 1.5 Gbps/lane. D-PHY v2.0+ (2.5+ Gbps/lane) would require newer bridge silicon.
- **No C-PHY support:** Some newer sensors use MIPI C-PHY. A future variant could add C-PHY bridge chips.
- **Latency:** The cut-through path adds ~200 ns latency. While this is within MIPI timing margins for most devices, extremely latency-sensitive applications may detect the tap.

### Future Directions

- **4-lane support** with TC358749 or newer bridge chips
- **C-PHY variant** using dedicated C-PHY front-end
- **CSI-2 v2.0 link encryption** research (defeating optional packet encryption)
- **Onboard neural injection** — Use the FPGA for real-time adversarial patch generation in camera frames to fool ML-based vision systems
- **Automotive-grade version** with extended temperature range and ruggedized connectors

---

## 10. Repository Structure

```
prism-tap/
├── README.md                  — This document
├── firmware/
│   ├── Makefile               — Build system (arm-none-eabi-gcc)
│   ├── main.c                 — Main firmware, boot, scheduler, command dispatch
│   ├── board.h                — Pin assignments, clock config, hardware constants
│   ├── registers.h            — MCU register definitions
│   ├── linker.ld              — Linker script for STM32H730
│   └── drivers/
│       ├── mipi_bridge.c/h    — MIPI bridge chip I2C configuration
│       ├── fpga_if.c/h        — FPGA SPI interface, bitstream loading, DMA
│       ├── frame_capture.c/h  — Frame buffer management, SD write, JPEG
│       ├── frame_inject.c/h   — Frame injection engine control
│       ├── ble_if.c/h         — nRF52840 BLE C2, encryption
│       ├── sdcard.c/h         — FAT32 SD card storage
│       ├── usb_cdc.c/h        — USB CDC virtual serial + bulk transfer
│       ├── oled.c/h           — OLED status display
│       ├── protocol.c/h      — Binary command protocol
│       └── power.c/h          — Power management, battery, charger
├── kicad/
│   ├── device.kicad_sch       — Schematic
│   ├── device.kicad_pcb       — PCB layout
│   └── device.kicad_pro       — KiCad project file
└── app/
    ├── App.js                 — React Native entry point
    ├── package.json           — Dependencies
    ├── screens/
    │   ├── ConnectScreen.js   — BLE scan and device pairing
    │   ├── CaptureScreen.js   — Live frame capture view
    │   ├── InjectScreen.js    — Frame injection controls
    │   ├── GalleryScreen.js   — Captured frame gallery
    │   └── SettingsScreen.js  — Device configuration
    └── utils/
        ├── deviceContext.js   — BLE context provider
        └── protocol.js        — Command protocol implementation
```

---

## 11. License

- **Firmware & software:** GPL-2.0
- **Hardware (KiCad):** CERN-OHL-S v2 (Strongly Reciprocal)
- **Documentation:** CC-BY-SA 4.0

All authored by **jayis1**.

---

*Prism-Tap — because the camera sees everything, and we can see the camera.*