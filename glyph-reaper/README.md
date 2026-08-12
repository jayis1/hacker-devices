# GLYPH-REAPER — Covert Embedded Display Bus Interception & Frame Reconstruction Implant

```
   ╔══════════════════════════════════════════════════════════════════════════╗
   ║   ██████╗ ███████╗ ██████╗ ██╗     ███████╗██████╗ ██╗  ██████╗███████╗   ║
   ║  ██╔════╝ ██╔════╝██╔════██╗██║     ██╔════╝██╔══██╗██║██╔════╝██╔════╝   ║
   ║  ██║  ███╗█████╗  ██║   ██║██║     █████╗  ██████╔╝██║██║     █████╗      ║
   ║  ██║   ██║██╔══╝  ██║   ██║██║     ██╔══╝  ██╔══██╗██║██║     ██╔══╝      ║
   ║  ╚██████╔╝███████╗╚██████╔╝███████╗███████╗██║  ██║██║╚██████╗███████╗     ║
   ║   ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝╚══════╝     ║
   ║                                                                          ║
   ║   Covert Embedded Display Bus Interception — Capture, Reconstruct,       ║
   ║   Exfiltrate Screen Content from MIPI DSI / RGB / SPI / LVDS Panels      ║
   ║   Below the OS. At the Pixel Clock.                                      ║
   ║   Author: jayis1                                                         ║
   ╚══════════════════════════════════════════════════════════════════════════╝
```

![Status](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange) ![HW-License](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> **Author:** jayis1
> **License:** Hardware — CERN-OHL-S v2, Firmware — GPL-2.0, App — MIT
> **Status:** Research hardware design — firmware + PCB + companion app
> **Version:** 1.0.0
> **Date:** 2026-08-12

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** GLYPH-REAPER is designed **exclusively** for authorized security research, penetration testing under explicit written contract, and academic study of embedded display pipeline security on devices you own or have written authorization to assess. Intercepting display content from devices, screens, or systems you do not own or have authorization to test may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), trade-secret protection laws, and privacy regulations in your jurisdiction. Capturing screen content that contains personal data, credentials, financial information, or proprietary information from devices you do not own can constitute unauthorized access to protected computing resources and interception of electronic communications. The author (**jayis1**) assumes no liability for any misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## Table of Contents

1. [Overview](#1-overview)
2. [What Makes GLYPH-REAPER Novel](#2-what-makes-glyph-reaper-novel)
3. [Attack Surface & Threat Model](#3-attack-surface--threat-model)
4. [Hardware Specifications](#4-hardware-specifications)
5. [Architecture & Block Diagram](#5-architecture--block-diagram)
6. [Firmware Design](#6-firmware-design)
7. [Application/Software Interface](#7-applicationsoftware-interface)
8. [Use Cases](#8-use-cases)
9. [Bill of Materials](#9-bill-of-materials)
10. [Legal & Ethical Considerations](#10-legal--ethical-considerations)

---

## 1. Overview

**GLYPH-REAPER** is a coin-cell-powered, covert inline interception implant that physically taps the **embedded display bus** between a target device's application processor (AP) or SoC and its display panel. Unlike external video capture tools (HDMI/DisplayPort recorders), GLYPH-REAPER operates at the raw pixel level on *internal* display interfaces — MIPI DSI, RGB parallel, SPI LCD, and LVDS — which carry the actual framebuffer content before any encryption, DRM, or content protection is applied.

By sitting physically inline on these low-level display buses, GLYPH-REAPER captures every pixel, every frame, and every transition in real time. It reconstructs full screen content in onboard PSRAM, performs delta compression, and streams the reconstructed display output over an encrypted BLE 5.2 or USB-CDC backhaul to an operator's mobile device. The companion app provides live screen mirroring, frame-by-frame analysis, OCR text extraction, and automated credential/secret detection.

### Key Capabilities

- **Multi-protocol display bus capture**: MIPI DSI (1–4 lanes, up to 1.5 Gbps/lane), RGB parallel (8/16/18/24-bit, up to 100 MHz pixel clock), SPI LCD (6800/8080 modes, up to 50 MHz), and LVDS (single/dual link, up to 135 MHz PCLK)
- **Real-time frame reconstruction**: Full-resolution frame buffer in 8 MB PSRAM with up to 30 FPS capture rate
- **Delta compression engine**: Hardware-accelerated RLE + delta encoding reduces bandwidth by 80-95% for static or slow-changing content
- **OCR text extraction**: On-device Tensilola DSP-based character recognition extracts text strings (credentials, OTPs, menu items) from captured frames
- **Credential pattern detection**: Automated scanning for password fields, API keys, QR codes, barcode content, and secret tokens
- **Covert form factor**: 22mm × 18mm flexible PCB designed to fit within display FPC stackup, 0.4mm Z-height
- **Ultra-low power**: 72-hour continuous capture on a single CR2032 coin cell, or indefinite from target's display power rail
- **Encrypted exfiltration**: AES-256-GCM over BLE 5.2 or USB-CDC with forward secrecy
- **Passive tap mode**: Zero-intrusion monitoring with no electrical impact on the target display bus (high-impedance buffering)

---

## 2. What Makes GLYPH-REAPER Novel

GLYPH-REAPER occupies a unique and previously unaddressed niche in the security research tool ecosystem:

### No Existing Tool for Internal Display Buses

While HDMI capture devices (like the existing HDMI-Siphon in this repo) target *external* display interfaces, there is no publicly available security research tool for intercepting *internal* embedded display buses. These buses — MIPI DSI, RGB parallel, SPI LCD, LVDS — are found in virtually every IoT device, smart appliance, automotive head unit, industrial HMI, medical device display, POS terminal, and mobile device. The attack surface is enormous and completely unprotected.

### Below-OS, Below-Driver, At-the-Wire Access

Display content on embedded systems passes through the display bus in raw, unencrypted form. Unlike the OS-level screen capture APIs (which may require permissions, trigger security indicators, or be blocked by DRM), the physical display bus carries the final rendered output with no access control. GLYPH-REAPER taps this bus directly, seeing exactly what the user sees — including content that the OS may prevent software from capturing.

### Multi-Protocol Coverage

No single commercial or research tool supports all four major embedded display protocols. GLYPH-REAPER's FPGA-based capture front-end can be dynamically reconfigured to sniff any of MIPI DSI, RGB parallel, SPI LCD, or LVDS by loading the appropriate bitstream, making it a universal embedded display interception tool.

### On-Device Intelligence

Rather than dumbly streaming raw pixels (which would require enormous bandwidth), GLYPH-REAPER performs on-device processing:
- Delta frame compression to reduce bandwidth by 80-95%
- OCR text extraction to exfiltrate only the text content
- Pattern matching for credentials, QR codes, and secrets
- Motion detection to trigger capture only when content changes

This makes it practical to exfiltrate display content over low-bandwidth BLE while still capturing all critical information.

### Covert Physical Design

The flexible PCB form factor is designed to be embedded within the FPC (Flexible Printed Circuit) stackup between a device's main board and its display panel. With a 0.4mm Z-height and the ability to draw power from the target's display power rail (3.3V VDD), GLYPH-REAPER can be permanently installed during a supply-chain interdiction or during a brief physical access window, with no external indicators of its presence.

---

## 3. Attack Surface & Threat Model

### Target Environments

| Target Class | Display Protocol | Typical Resolution | Attack Value |
|---|---|---|---|
| IoT smart home displays | SPI LCD / RGB parallel | 240×240 – 480×320 | PINs, arm/disarm codes, configuration |
| POS terminals | RGB parallel / MIPI DSI | 320×480 – 720×1280 | Card data, transaction amounts, admin PINs |
| Industrial HMI panels | LVDS / RGB parallel | 800×480 – 1920×1080 | Process data, safety states, operator credentials |
| Automotive head units | LVDS / MIPI DSI | 800×480 – 1920×720 | Navigation, phone mirroring, diagnostic data |
| Medical device displays | LVDS / RGB parallel | 800×600 – 1920×1080 | Patient data, medication info, admin settings |
| Wearable devices | SPI LCD / MIPI DSI | 240×240 – 454×454 | Health data, notifications, payment tokens |
| Access control panels | RGB parallel / SPI LCD | 320×240 – 800×480 | Badge data, PINs, door states, audit logs |
| Vending/kiosk displays | RGB parallel / LVDS | 480×272 – 1920×1080 | Transaction data, service menus, cash counts |

### Attack Vectors

1. **Supply-Chain Interdiction**: GLYPH-REAPER is installed in the display FPC chain during manufacturing or transit, before the device reaches the end user. The device operates covertly for the entire product lifetime.

2. **Brief Physical Access**: During a maintenance window, repair, or temporary access, the attacker opens the target device, inserts GLYPH-REAPER inline on the display FPC, and closes it. Installation takes under 2 minutes with proper tooling.

3. **Malicious Replacement Display**: A replacement display panel is pre-built with GLYPH-REAPER integrated. When the target device's original display is replaced (repair, upgrade, warranty service), the compromised display is installed.

4. **Development/Testing Platform**: During security research on a device the researcher owns, GLYPH-REAPER is used to monitor the display output during fuzzing, penetration testing, or reverse engineering to capture all visual feedback, error messages, and debug output.

### Threat Model

| Property | Capability |
|---|---|
| **Persistence** | Permanent hardware implant — survives firmware updates, factory resets, and OS reinstalls |
| **Stealth** | No software footprint on target. No CPU cycles consumed. No memory mapped. Invisible to all software-based detection |
| **Privilege** | Operates below all OS privilege levels. No kernel access needed. Not affected by secure boot, TEE, or DRM |
| **Detection difficulty** | Requires physical inspection of display FPC stackup. Visual inspection may miss flexible PCB embedded in FPC |
| **Data access** | All rendered display content: credentials, OTPs, QR codes, screen layouts, UI state, error messages, debug output |
| **Exfiltration** | BLE 5.2 (up to 100m line-of-sight) or USB-CDC. AES-256-GCM encrypted. Delta-compressed for low bandwidth |

### Assumptions

- The attacker has (or can arrange) brief physical access to the target device, or can interdict the supply chain
- The target device uses one of the four supported display protocols (MIPI DSI, RGB, SPI, LVDS)
- The target's display FPC is accessible (not potted in epoxy or within a sealed tamper-evident enclosure)
- The operator can position a BLE receiver within ~100m for real-time exfiltration, or the device can store frames locally for later retrieval

---

## 4. Hardware Specifications

### MCU / SoC

| Component | Part | Specification |
|---|---|---|
| Application MCU | nRF52840-QIAA-R7 | Cortex-M4F @ 64 MHz, 1 MB Flash, 256 KB RAM. BLE 5.2 controller, USB 2.0 device. Manages capture pipeline, compression, exfiltration |
| Capture FPGA | Lattice iCE40UP5K-SG48 | 5.3K LUTs, 16 KB SRAM, 8× 1K SPRAM, DSP blocks. Reconfigurable display protocol decoder. Handles high-speed pixel clock capture |
| DSP co-processor | ESP32-S3-MINI-1 | Dual-core Xtensa LX7 @ 240 MHz, 512 KB SRAM, vector instructions. OCR inference, delta compression, pattern matching |

### Memory

| Component | Part | Specification |
|---|---|---|
| Frame PSRAM | APS6404L-3SQR-SN | 8 MB QSPI PSRAM, 133 MHz. Stores full-resolution reconstructed frame buffer |
| Configuration Flash | W25Q128JVSIQ | 16 MB QSPI NOR Flash. Stores FPGA bitstreams, OCR model, captured frame history |
| SD Card | MicroSD slot (optional) | Up to 32 GB. Extended frame capture storage for offline retrieval |

### Display Bus Interface

| Protocol | Interface | Max Speed | Lane Configuration |
|---|---|---|---|
| MIPI DSI | 4× differential pairs (CLK + D0-D3) | 1.5 Gbps/lane | 1, 2, or 4 lanes. D-PHY level shifting |
| RGB Parallel | 24-bit data + PCLK + HSYNC + VSYNC + DE | 100 MHz PCLK | 8/16/18/24-bit color modes |
| SPI LCD | SPI (MOSI + SCK + CS + DC + WR + RD) | 50 MHz | 3-wire or 4-wire SPI, 6800/8080 parallel |
| LVDS | 4× differential pairs (CLK + 3 data) | 135 MHz PCLK | Single or dual link, 18-bit or 24-bit |

### Connectivity

| Interface | Specification |
|---|---|
| BLE 5.2 | Nordic S140 SoftDevice. 2 Mbps PHY, DLE, AES-256-GCM application-layer encryption |
| USB 2.0 CDC | USB-C connector, 12 Mbps. Enumerates as virtual serial port. Also used for charging and firmware updates |
| SWD | 4-pin debug header (0.5mm pitch, test points only) for firmware development |

### Sensors

| Sensor | Purpose |
|---|---|
| nRF52840 internal TEMP | Monitor implant temperature (overheating detection) |
| ADC battery sense | Monitor coin cell or harvested power voltage |
| Accelerometer (LIS2DH12) | Detect device movement/tampering, trigger capture on orientation change |

### Power

| Source | Specification |
|---|---|
| Primary: CR2032 coin cell | 220 mAh, 3V. 72 hours continuous capture, 30 days passive standby |
| Harvested: Target VDD (3.3V) | Draws from target's display power rail via FPC. Indefinite operation. <5mA draw |
| Rechargeable: LIR2450 (optional) | 120 mAh, 3.6V. USB-C rechargeable |
| Power management | TPS62740 step-down converter (90% efficiency), load switch between coin cell and harvested power |

### Form Factor

| Dimension | Value |
|---|---|
| PCB size | 22mm × 18mm flexible PCB (polyimide) |
| Z-height | 0.4mm (including components) |
| Weight | 0.8g (without coin cell) |
| FPC connectors | 0.5mm pitch FFC connectors, matching common display FPC widths (4-lane MIPI: 0.5mm 41-pin) |
| Operating temperature | -20°C to +70°C |
| Storage temperature | -40°C to +85°C |

---

## 5. Architecture & Block Diagram

```
                    TARGET DEVICE
    ┌─────────────────────────────────────┐
    │  Application Processor / SoC        │
    │  ┌───────────────────────────────┐  │
    │  │  Display Controller           │  │
    │  │  (MIPI DSI / RGB / SPI / LVDS)│  │
    │  └───────────┬───────────────────┘  │
    │              │ Display Bus          │
    └──────────────┼──────────────────────┘
                   │ (intercepted inline)
    ╔══════════════╪═══════════════════════════════════════════════╗
    ║  GLYPH-REAPER IMPLANT                                       ║
    ║                                                               ║
    ║  ┌─────────────────────────────────────────────────────┐     ║
    ║  │  Display Bus Input Front-End                        │     ║
    ║  │  ┌───────────┐  ┌───────────┐  ┌──────────────┐   │     ║
    ║  │  │ MIPI DSI  │  │ RGB Par.  │  │ SPI / LVDS   │   │     ║
    ║  │  │ RX (D-PHY)│  │ Level Sh. │  │ Level Sh.    │   │     ║
    ║  │  └─────┬─────┘  └─────┬─────┘  └──────┬───────┘   │     ║
    ║  │        └──────────┬───┴───────────────┘            │     ║
    ║  │                   ▼                                 │     ║
    ║  │  ┌─────────────────────────────────┐                │     ║
    ║  │  │  iCE40UP5K FPGA                 │                │     ║
    ║  │  │  - Protocol decoder bitstream   │                │     ║
    ║  │  │  - Pixel clock recovery         │                │     ║
    ║  │  │  - HSYNC/VSYNC/DE detection     │                │     ║
    ║  │  │  - RGB→YUV422 conversion        │                │     ║
    ║  │  │  - Row/column buffering         │                │     ║
    ║  │  └──────────┬──────────────────────┘                │     ║
    ║  │             │ Raw pixel stream (parallel)           │     ║
    ║  │             ▼                                       │     ║
    ║  │  ┌─────────────────────────────────┐                │     ║
    ║  │  │  ESP32-S3 DSP Co-Processor      │                │     ║
    ║  │  │  - Delta frame compression      │                │     ║
    ║  │  │  - RLE encoding                 │                │     ║
    ║  │  │  - OCR text extraction          │                │     ║
    ║  │  │  - Credential pattern matching  │                │     ║
    ║  │  └──────────┬──────────────────────┘                │     ║
    ║  │             │ Compressed frame data                 │     ║
    ║  │             ▼                                       │     ║
    ║  │  ┌─────────────────────────────────┐                │     ║
    ║  │  │  8 MB PSRAM (Frame Buffer)      │                │     ║
    ║  │  │  - Current frame                │                │     ║
    ║  │  │  - Previous frame (delta ref)   │                │     ║
    ║  │  │  - OCR text buffer              │                │     ║
    ║  │  └──────────┬──────────────────────┘                │     ║
    ║  │             │                                       │     ║
    ║  │  ┌──────────▼──────────────────────┐                │     ║
    ║  │  │  nRF52840 Application MCU       │                │     ║
    ║  │  │  - BLE 5.2 exfiltration         │                │     ║
    ║  │  │  - USB-CDC interface            │                │     ║
    ║  │  │  - AES-256-GCM encryption       │                │     ║
    ║  │  │  - Frame scheduling             │                │     ║
    ║  │  │  - Power management             │                │     ║
    ║  │  │  - Command & control            │                │     ║
    ║  │  └──────────┬──────────────────────┘                │     ║
    ║  │             │                                       │     ║
    ║  └─────────────┼───────────────────────────────────────┘     ║
    ║                │                                               ║
    ║    ┌───────────┴───────────┐    ┌──────────────────┐         ║
    ║    │  BLE 5.2 Antenna      │    │  USB-C Connector │         ║
    ║    │  (PCB trace antenna)  │    │  (optional)      │         ║
    ║    └───────────────────────┘    └──────────────────┘         ║
    ╚══════════════════════════════════════════════════════════════╝
                   │ BLE / USB
                   ▼
    ┌──────────────────────────────┐
    │  OPERATOR DEVICE             │
    │  GLYPH-REAPER Companion App  │
    │  - Live screen mirroring     │
    │  - Frame capture & analysis  │
    │  - OCR text extraction       │
    │  - Credential detection      │
    │  - Frame timeline scrubing   │
    └──────────────────────────────┘
```

### Data Flow Summary

1. **Capture**: The FPGA captures raw pixel data from the target's display bus, performing protocol-specific decoding (DSI packet parsing, RGB sync detection, SPI command extraction, LVDS deserialization)
2. **Buffer**: Raw pixels are written to PSRAM in a frame buffer, with HSYNC/VSYNC used to delimit frame boundaries
3. **Process**: The ESP32-S3 DSP reads the current frame, compares against the previous frame, and produces a delta-compressed representation. It also runs OCR and pattern matching on the frame content
4. **Exfiltrate**: The nRF52840 MCU encrypts the compressed frame data with AES-256-GCM and transmits via BLE 5.2 or USB-CDC
5. **Display**: The companion app receives, decrypts, decompresses, and renders the frame on the operator's device

---

## 6. Firmware Design

### Firmware Architecture

The firmware is divided across three processing elements, each with distinct responsibilities:

#### nRF52840 Application MCU (main.c + drivers)

- **System initialization**: Clocks, GPIO, power management, watchdog
- **BLE 5.2 stack**: S140 SoftDevice, custom GATT service for frame streaming and command/control
- **USB CDC-ACM**: Virtual serial port for wired exfiltration and configuration
- **AES-256-GCM encryption**: Hardware-accelerated crypto for all exfiltrated data
- **Frame scheduler**: Priority-based queuing of frame data, OCR results, and status updates
- **Power management**: Adaptive capture rate based on battery level, sleep modes during display inactivity
- **Command protocol**: Binary command interface for the companion app (start/stop capture, set protocol, query status, retrieve frames)

#### iCE40UP5K FPGA (display_capture.v)

- **Protocol-specific bitstreams**: Four bitstreams stored in Flash, one per supported protocol
- **MIPI DSI decoder**: D-PHY lane management, DSI packet parser, pixel extraction from RGB888/YUV422 payloads, short/long packet handling
- **RGB parallel decoder**: PCLK-synchronized sampling, HSYNC/VSYNC state machine, DE-based active region detection, configurable bit depth (8/16/18/24)
- **SPI LCD decoder**: SPI transaction snooping, command/data distinction (DC pin), display command set parsing (ILI9341/ST7789/SSD1306), framebuffer write extraction
- **LVDS decoder**: Clock recovery, channel-lock deserialization, 7:1 or 10:1 de-mapping, pixel reconstruction
- **Output**: Parallel 16-bit YUV422 pixel stream + sync signals to ESP32-S3 via GPIO

#### ESP32-S3 DSP (dsp_main.c)

- **Delta compression**: Frame-to-frame difference computation, RLE encoding of changed regions, region-of-interest bounding boxes
- **OCR engine**: Lightweight CNN-based text detector (Tesseract-derived, ~200KB model) running on ESP32-S3 vector instructions
- **Pattern matching**: Regex-like pattern detector for credential formats (API keys, JWT tokens, passwords, OTP codes, URLs)
- **QR/barcode decoder**: ZXing-port for QR code and barcode content extraction from captured frames
- **SD card storage**: Optional local frame storage for offline retrieval when BLE/USB is unavailable

### Design Decisions

1. **FPGA for capture front-end**: Display buses operate at high speeds (up to 1.5 Gbps for MIPI DSI) that require hardware-level signal processing. A small, low-cost FPGA (iCE40UP5K at ~$3) provides the reconfigurability to support multiple protocols without a multi-chip solution. The FPGA also handles the critical timing of pixel clock recovery and sync detection that would be impossible with software on a microcontroller.

2. **ESP32-S3 as DSP co-processor**: The delta compression and OCR workloads benefit from the ESP32-S3's vector instructions and dual-core architecture. Offloading this from the nRF52840 (which handles BLE and USB) allows each chip to operate at maximum efficiency. The ESP32-S3 was chosen over alternatives for its excellent ML inference capabilities at low power.

3. **nRF52840 for connectivity**: The nRF52840 is the established choice for BLE 5.2 with USB support. Its SoftDevice provides a certified BLE stack, and its CryptoCell-310 hardware accelerator handles AES-256-GCM without CPU overhead.

4. **8 MB PSRAM for frame buffer**: A full 720×1280 RGB565 frame requires 1.8 MB. With two frame buffers (current + previous for delta computation) plus OCR workspace, 8 MB provides ample headroom. QSPI PSRAM at 133 MHz provides sufficient bandwidth for 30 FPS operation.

5. **Flexible PCB form factor**: The polyimide flexible PCB is designed to fit within the tight Z-height constraints of display FPC stackups (typically 0.3-0.5mm total). This is essential for covert installation within modern devices where space is extremely constrained.

6. **Dual power source**: The primary CR2032 coin cell enables operation even when the target device is powered off (for brief periods). Harvesting from the target's 3.3V display rail provides indefinite operation. The TPS62740 buck converter ensures efficient power delivery regardless of source.

7. **Delta compression as primary bandwidth reduction**: BLE 5.2 at 2 Mbps PHY provides ~1.5 Mbps application throughput. A 720×1280 RGB565 frame at 30 FPS is 442 Mbps — a 300× reduction is needed. Delta compression achieves 80-95% reduction for typical UI content (which changes slowly), and OCR-only mode achieves >99% reduction by transmitting only extracted text.

---

## 7. Application/Software Interface

### Companion App (React Native)

The GLYPH-REAPER companion app runs on iOS and Android and provides the operator with full control over the implant and real-time display of captured content.

#### Screens

1. **Live View Screen**: Real-time display of the captured screen content, rendered at the target's native resolution. Supports pinch-to-zoom, pan, and screenshot capture. Shows capture FPS, bandwidth usage, and encryption status.

2. **Capture Screen**: Configuration of capture parameters — display protocol selection, resolution, color depth, capture rate, trigger modes (continuous, on-change, on-motion). Start/stop/pause controls and capture statistics.

3. **Text Extraction Screen**: Shows OCR-extracted text from the captured display in real time. Organized by frame with timestamps. Includes credential detection alerts highlighting detected patterns (API keys, passwords, OTPs, URLs, email addresses). Search and filter capabilities.

4. **Frame Analysis Screen**: Frame-by-frame timeline scrubbing. Select any captured frame to view at full resolution with pixel inspector. Diff view between any two frames. Histogram and color analysis tools.

5. **Settings Screen**: BLE/USB connection management, encryption key configuration, firmware update, power management settings, SD card management, export options (frame export to PNG/JPEG, text export to TXT/JSON, full session export to ZIP).

### Communication Protocol

The companion app communicates with GLYPH-REAPER over a binary protocol encapsulated in BLE GATT notifications or USB-CDC:

| Message Type | Direction | Purpose |
|---|---|---|
| `CMD_START_CAPTURE` | App → Device | Begin capturing with specified protocol and settings |
| `CMD_STOP_CAPTURE` | App → Device | Stop capturing |
| `CMD_SET_PROTOCOL` | App → Device | Switch display protocol (MIPI DSI / RGB / SPI / LVDS) |
| `CMD_SET_RESOLUTION` | App → Device | Configure expected resolution and color depth |
| `CMD_SET_TRIGGER_MODE` | App → Device | Set trigger: continuous, on-change, on-motion, on-schedule |
| `CMD_GET_STATUS` | App → Device | Query device status (uptime, battery, capture stats) |
| `CMD_GET_FRAME` | App → Device | Request specific frame by index |
| `CMD_SET_OCR_MODE` | App → Device | Enable/disable OCR, set language, set pattern matchers |
| `CMD_EXPORT_FRAMES` | App → Device | Request bulk frame export from SD card |
| `CMD_FIRMWARE_UPDATE` | App → Device | Initiate OTA firmware update |
| `MSG_FRAME_DATA` | Device → App | Compressed frame data with metadata |
| `MSG_OCR_TEXT` | Device → App | Extracted text from current frame |
| `MSG_CREDENTIAL_ALERT` | Device → App | Detected credential/secret pattern with location |
| `MSG_STATUS_UPDATE` | Device → App | Periodic status (battery, temperature, capture stats) |
| `MSG_ERROR` | Device → App | Error notification with code and description |

---

## 8. Use Cases

### For Red Teams

1. **Supply-Chain Implant**: During a red team engagement with supply-chain access, implant GLYPH-REAPER into target devices (POS terminals, access control panels, industrial HMIs) before deployment. Monitor all display content remotely via BLE for the duration of the engagement.

2. **Brief Access Installation**: With 2 minutes of physical access to a target device (during a meeting, while a user is away), install GLYPH-REAPER on the display FPC. Capture credentials, configuration screens, and sensitive data displayed on the target.

3. **Credential Harvesting**: Monitor login screens, PIN entry pads, and configuration interfaces on IoT devices, kiosks, and industrial control panels. The OCR engine automatically extracts typed credentials and the pattern matcher flags them for the operator.

4. **UI State Monitoring**: Track the operational state of target devices by monitoring their display content — detect when a device is in admin mode, when it's displaying error messages with system information, or when it shows diagnostic screens.

### For Security Researchers

1. **Firmware Reverse Engineering**: During firmware analysis of an embedded device, use GLYPH-REAPER to capture all display output during fuzzing and testing. Correlate displayed error messages with firmware behavior to understand the device's internal state machine.

2. **Display Protocol Analysis**: Use GLYPH-REAPER as a protocol analyzer for MIPI DSI, RGB, SPI LCD, and LVDS. Examine the raw display bus traffic to understand panel initialization sequences, command sets, and timing parameters.

3. **DRM/Content Protection Bypass Research**: Study how display content protection (HDCP for DSI, panel-level encryption) is implemented on embedded devices. GLYPH-REAPER captures content before any link-level encryption is applied.

4. **Side-Channel Display Analysis**: Research whether display content can be inferred from bus traffic patterns, power consumption, or EMI emissions. GLYPH-REAPER provides ground-truth display content for comparison.

### For Penetration Testers

1. **Physical Security Assessment**: During a physical pentest, install GLYPH-REAPER on access control panels, alarm keypads, or security monitoring displays to capture PINs, badge data, and system status information.

2. **POS Terminal Testing**: Assess the security of point-of-sale systems by capturing display content containing transaction data, card information (last 4 digits, card type), and merchant configuration screens.

3. **ICS/SCADA HMI Assessment**: Monitor industrial human-machine interface displays to capture process data, operator actions, alarm states, and configuration changes. Use this information to understand the industrial process and identify vulnerabilities.

4. **Automotive Security Testing**: Intercept infotainment and instrument cluster displays in vehicles to capture navigation data, phone mirroring content (CarPlay/Android Auto), diagnostic information, and vehicle configuration.

5. **Medical Device Security**: Assess medical device displays for patient data exposure, medication information, and device configuration. Useful for HIPAA compliance testing and FDA pre-market security assessment.

---

## 9. Bill of Materials

| Ref | Part | Qty | Unit Cost | Description |
|---|---|---|---|---|
| U1 | nRF52840-QIAA-R7 | 1 | $5.20 | Application MCU, BLE 5.2 |
| U2 | iCE40UP5K-SG48 | 1 | $3.10 | Capture FPGA |
| U3 | ESP32-S3-MINI-1 | 1 | $2.80 | DSP co-processor |
| U4 | APS6404L-3SQR-SN | 1 | $0.90 | 8 MB QSPI PSRAM |
| U5 | W25Q128JVSIQ | 1 | $0.80 | 16 MB QSPI NOR Flash |
| U6 | TPS62740DSCT | 1 | $1.20 | Step-down DC/DC converter |
| U7 | LIS2DH12TR | 1 | $0.70 | 3-axis accelerometer |
| U8 | SN65LVDS387 | 1 | $1.40 | LVDS receiver (optional) |
| J1 | Hirose DF40C-41DP-0.4V(51) | 1 | $1.80 | 41-pin FFC connector (MIPI DSI) |
| J2 | Hirose DF40C-31DP-0.4V(51) | 1 | $1.50 | 31-pin FFC connector (RGB/SPI) |
| J3 | Amphenol 10104110 | 1 | $0.60 | USB-C connector |
| BAT1 | CR2032 holder | 1 | $0.30 | Coin cell holder |
| Y1 | 32 MHz crystal | 1 | $0.20 | nRF52840 HF crystal |
| Y2 | 32.768 kHz crystal | 1 | $0.15 | nRF52840 LF crystal |
| Y3 | 40 MHz crystal | 1 | $0.20 | ESP32-S3 crystal |
| — | Passives (R, C, L) | ~40 | $0.80 | Resistors, caps, inductors |
| — | PCB (flexible polyimide) | 1 | $2.50 | 22×18mm flex PCB, 4-layer |
| — | Antenna (PCB trace) | 1 | $0.00 | Integrated BLE antenna |
| | **Total** | | **~$23.85** | Target BOM <$25 |

---

## 10. Legal & Ethical Considerations

GLYPH-REAPER is a dual-use security research tool. The author (**jayis1**) provides this design for educational and authorized research purposes only.

### Authorized Use

- Security research on devices you own or have written permission to test
- Penetration testing under a signed scope-of-work agreement
- Academic research on embedded display security
- Internal security assessments within your own organization
- Product security evaluation during development

### Unauthorized Use (Prohibited)

- Intercepting display content from devices you do not own or lack authorization to test
- Capturing personal data, credentials, or proprietary information without consent
- Supply-chain interdiction of devices you do not own
- Any use that violates wiretap laws, computer fraud laws, or privacy regulations
- Deployment on devices used by persons without their knowledge and consent

### Compliance Notes

- In the US: Intercepting electronic communications without authorization may violate 18 U.S.C. § 2511 (Wiretap Act) and 18 U.S.C. § 1030 (CFAA)
- In the EU: Unauthorized data interception may violate GDPR (Art. 5, Art. 6) and the ePrivacy Directive (Art. 5)
- Display content may contain personal data (names, health info, financial data) subject to privacy regulations
- Export of this technology may be subject to EAR/Wassenaar dual-use controls

**The author (jayis1) assumes no liability for any misuse of this design. Always obtain proper written authorization before deployment.**

---

*GLYPH-REAPER — Seeing what the screen sees. Below the OS. At the pixel clock.*
*Author: jayis1 · Version 1.0.0 · 2026-08-12*