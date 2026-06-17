# HDMI-Siphon — Transparent HDMI Interposer for Video Security Research

**Author: jayis1**  
**Version: 1.0.0**  
**License: Proprietary — Authorized Security Research Use Only**

---

> ⚠️ **LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and educational purposes. Intercepting, recording, or modifying HDMI video signals without the consent of all parties involved violates wiretapping laws, copyright laws (including the DMCA anti-circumvention provisions), and computer fraud statutes in most jurisdictions. The author (jayis1) assumes no liability for misuse. **Always obtain proper authorization before use.**

---

## 1. Overview

The **HDMI-Siphon** is a novel, portable hardware device that functions as a **transparent HDMI interposer** for hardware security research, red team operations, and video surveillance assessments. It sits inline between an HDMI video source (computer, set-top box, surveillance DVR, video conferencing system, digital signage player) and a display, intercepting the full HDMI signal path without introducing perceptible latency or degradation.

Unlike software-based screen capture solutions, the HDMI-Siphon operates at the **physical signal layer** — the source device has no knowledge of its presence, cannot detect it via software, and cannot disable it through OS-level security controls. This makes it uniquely valuable for physical security assessments where the goal is to evaluate whether sensitive display content can be intercepted from the video signal path.

### 1.1 Why HDMI-Siphon?

Modern security research often focuses on network, wireless, and software attack surfaces, but the **physical video signal path** remains a critically underexplored vector. Video signals carry the most sensitive data visible on screen — passwords typed in terminal sessions, classified documents, surveillance footage, financial data, medical records, and authenticated sessions — yet the HDMI cable itself is almost never treated as a trust boundary.

The HDMI-Siphon addresses this gap by providing a research platform that can:

- **Demonstrate the exposure** of video signal interception in physical security assessments
- **Test display privacy controls** and verify that sensitive displays are not leaking content
- **Enable HDMI-CEC security analysis** — the CEC bus carries device commands and can be fuzzed or monitored
- **Provide forensic capture** of display content from unattended kiosks, digital signage, and embedded displays
- **Assess HDCP implementation** robustness and verify that protected content paths are properly enforced

### 1.2 Key Capabilities

| Capability | Description |
|---|---|
| **Transparent Passthrough** | Zero-latency HDMI relay with no visual degradation; source detects a valid display via spoofed EDID |
| **EDID Manipulation** | Arbitrary EDID injection — force any resolution, disable HDCP, present custom display capabilities |
| **HDMI Frame Capture** | Capture individual frames or video sequences to microSD at up to 1080p30 |
| **OSD / Overlay Injection** | Inject text, markers, or full overlay graphics into the video stream in real-time |
| **HDCP Stripping** | Capture HDCP-protected content by spoofing HDCP handshake and stripping encryption (authorized research only) |
| **HDMI-CEC Bus Monitor** | Sniff and inject CEC commands between devices — CEC keylogging, device control, power management |
| **Covert Recording Mode** | Battery-powered, no LEDs, captures on motion/pattern trigger to microSD |
| **WiFi Exfiltration** | Captured frames and logs transmitted via ESP32-S3 WiFi to operator device |
| **Display Clone** | Simultaneously output to both the real display and an encrypted WiFi stream |

---

## 2. Attack Surface & Threat Model

### 2.1 Threat Model

The HDMI-Siphon assumes the following threat scenario:

- **Attacker has physical access** to the HDMI cable path between source and display
- **Attacker can insert the interposer** in under 30 seconds (inline coupler form factor)
- **Target display may be unattended** (kiosk, digital signage, CCTV monitor, conference room)
- **Target may have HDCP enabled** on the source device
- **The interposer must be undetectable** — no added latency >1 frame, no visual artifacts, no electrical signature

### 2.2 Attack Surface Vectors

| Vector | Description | Risk Level |
|---|---|---|
| **Physical Cable Intercept** | Insertion into existing HDMI cable run between source and display | Critical |
| **Kiosk Display Capture** | Unattended kiosks, ATMs, and self-service terminals with exposed HDMI ports | High |
| **Conference Room Tap** | HDMI cables in shared meeting spaces, boardrooms, or presentation rooms | High |
| **Digital Signage** | Signage players with accessible HDMI connections in public spaces | Medium |
| **HDMI-CEC Keylogging** | CEC bus carries remote control keypresses, including smart TV and media player inputs | High |
| **HDCP Bypass** | Capturing protected video content by exploiting HDCP handshake weaknesses | Critical |
| **EDID Poisoning** | Crafting malicious EDID data to trigger buffer overflows or denial of service in source GPU driver | Medium |

### 2.3 Detection Vectors (what the device avoids)

- **No added input lag** — FPGA-based frame buffer provides sub-microsecond passthrough
- **No signal degradation** — HDMI redriver chip ensures clean signal retiming
- **No electrical signature** — powered from HDMI 5V line or external battery, no fan, no RF emissions from processing chain
- **Stealth mode** — all status LEDs can be disabled via hardware jumper or firmware config
- **Minimal form factor** — 65×45×15mm, identical to a standard HDMI coupler/adapter

---

## 3. Hardware Specifications

### 3.1 Block Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│  HDMI Source ──▶ HDMI RX (TFP401) ──▶ FPGA (iCE40UP5K) ──▶ HDMI TX (TFP410) ──▶ Display │
│                                          │                    │                        │
│                                          ▼                    ▼                        │
│                                    IS42S16160          EDID EEPROM                    │
│                                    SDRAM 32MB          (24LC02B)                      │
│                                          │                                             │
│                                          ▼                                             │
│                                    ESP32-S3 (Control, WiFi, BLE)                      │
│                                    │          │          │                            │
│                                    ▼          ▼          ▼                            │
│                                microSD     USB-C     Li-Po Batt                       │
│                                (capture)   (debug)   (300mAh)                         │
│                                                                                       │
│                          HDMI-CEC Bus Monitor (GPIO)                                  │
└──────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Component Selection

| Component | Part Number | Purpose |
|---|---|---|
| **HDMI RX** | Texas Instruments TFP401APZP | HDMI/DVI receiver, up to 165 MHz pixel clock (1080p60), 24-bit RGB output |
| **HDMI TX** | Texas Instruments TFP410PAP | HDMI/DVI transmitter, 165 MHz, 24-bit RGB input with DE/HSYNC/VSYNC |
| **FPGA** | Lattice iCE40UP5K-UWG30 | Ultra-low-power FPGA, 5280 LUTs, 120 kbit BRAM, PLL, SPI master |
| **MCU/WiFi** | Espressif ESP32-S3-WROOM-1 | Dual-core Xtensa LX7 @ 240 MHz, 512KB SRAM, WiFi 4, BLE 5.0 |
| **SDRAM** | ISSI IS42S16160J-6TL | 16M×16 (32 MB) SDRAM, 166 MHz, for frame buffering |
| **EDID EEPROM** | Microchip 24LC02BT-I/OT | 2 Kbit I²C EEPROM for EDID storage, spoofing, and passthrough |
| **Level Shifter** | TXS0108EPWR | 8-bit bidirectional voltage translator (FPGA 3.3V ↔ HDMI I²C 5V) |
| **I²C Mux** | TCA9548APWR | 8-channel I²C mux for EDID/CEC/configuration bus |
| **SD Card Slot** | Amphenol 101-00397-68 | microSD push-push, SPI mode |
| **USB-Serial** | WCH CH340C | USB-to-UART bridge for debug console |
| **Battery** | LP503048 3.7V Li-Po | 300 mAh rechargeable, ~2 hours covert operation |
| **Battery Charger** | MCP73831T-2ACI/OT | Li-Po charger with charge status LED |
| **Regulator** | XC6206P332MR | 3.3V LDO, 250 mA output |
| **Regulator** | XC6206P182MR | 1.8V LDO for FPGA core |
| **HDMI Connector** | HDMI-A Receptacle (x2) | Type A female, SMT, with shield grounding tabs |
| **CEC Level Shift** | BSS138 MOSFET | 3.3V ↔ 5V open-drain level shifter for CEC bus |

### 3.3 Pin Mapping

#### FPGA to SDRAM

| FPGA Pin | SDRAM Pin | Function |
|---|---|---|
| IO_0 | DQ0 | Data bit 0 |
| IO_1 | DQ1 | Data bit 1 |
| IO_2 | DQ2 | Data bit 2 |
| IO_3 | DQ3 | Data bit 3 |
| IO_4 | DQ4 | Data bit 4 |
| IO_5 | DQ5 | Data bit 5 |
| IO_6 | DQ6 | Data bit 6 |
| IO_7 | DQ7 | Data bit 7 |
| IO_8 | DQ8 | Data bit 8 |
| IO_9 | DQ9 | Data bit 9 |
| IO_10 | DQ10 | Data bit 10 |
| IO_11 | DQ11 | Data bit 11 |
| IO_12 | DQ12 | Data bit 12 |
| IO_13 | DQ13 | Data bit 13 |
| IO_14 | DQ14 | Data bit 14 |
| IO_15 | DQ15 | Data bit 15 |
| IO_16 | BA0 | Bank address 0 |
| IO_17 | BA1 | Bank address 1 |
| IO_18 | A0 | Row/col address 0 |
| IO_19 | A1 | Row/col address 1 |
| IO_20 | A2 | Row/col address 2 |
| IO_21 | A3 | Row/col address 3 |
| IO_22 | A4 | Row/col address 4 |
| IO_23 | A5 | Row/col address 5 |
| IO_24 | A6 | Row/col address 6 |
| IO_25 | A7 | Row/col address 7 |
| IO_26 | A8 | Row/col address 8 |
| IO_27 | A9 | Row/col address 9 |
| IO_28 | A10/AP | Row/col address 10 / Auto-precharge |
| IO_29 | A11 | Row/col address 11 |
| IO_30 | A12 | Row/col address 12 |
| IO_31 | CLK | SDRAM clock |
| IO_32 | CKE | Clock enable |
| IO_33 | CS# | Chip select |
| IO_34 | RAS# | Row address strobe |
| IO_35 | CAS# | Column address strobe |
| IO_36 | WE# | Write enable |
| IO_37 | DQMU | Data mask upper byte |
| IO_38 | DQML | Data mask lower byte |

#### FPGA to HDMI RX (TFP401)

| FPGA Pin | TFP401 Pin | Function |
|---|---|---|
| IO_39 | R0 | Red data bit 0 |
| IO_40 | R1 | Red data bit 1 |
| IO_41 | R2 | Red data bit 2 |
| IO_42 | R3 | Red data bit 3 |
| IO_43 | R4 | Red data bit 4 |
| IO_44 | R5 | Red data bit 5 |
| IO_45 | R6 | Red data bit 6 |
| IO_46 | R7 | Red data bit 7 |
| IO_47 | G0 | Green data bit 0 |
| IO_48 | G1 | Green data bit 1 |
| IO_49 | G2 | Green data bit 2 |
| IO_50 | G3 | Green data bit 3 |
| IO_51 | G4 | Green data bit 4 |
| IO_52 | G5 | Green data bit 5 |
| IO_53 | G6 | Green data bit 6 |
| IO_54 | G7 | Green data bit 7 |
| IO_55 | B0 | Blue data bit 0 |
| IO_56 | B1 | Blue data bit 1 |
| IO_57 | B2 | Blue data bit 2 |
| IO_58 | B3 | Blue data bit 3 |
| IO_59 | B4 | Blue data bit 4 |
| IO_60 | B5 | Blue data bit 5 |
| IO_61 | B6 | Blue data bit 6 |
| IO_62 | B7 | Blue data bit 7 |
| IO_63 | HSYNC | Horizontal sync |
| IO_64 | VSYNC | Vertical sync |
| IO_65 | DE | Data enable (blanking indicator) |
| IO_66 | PCLK | Pixel clock (up to 165 MHz) |

#### FPGA to HDMI TX (TFP410)

| FPGA Pin | TFP410 Pin | Function |
|---|---|---|
| IO_67 | R0 | Red data bit 0 |
| IO_68 | R1 | Red data bit 1 |
| IO_69 | R2 | Red data bit 2 |
| IO_70 | R3 | Red data bit 3 |
| IO_71 | R4 | Red data bit 4 |
| IO_72 | R5 | Red data bit 5 |
| IO_73 | R6 | Red data bit 6 |
| IO_74 | R7 | Red data bit 7 |
| IO_75 | G0 | Green data bit 0 |
| IO_76 | G1 | Green data bit 1 |
| IO_77 | G2 | Green data bit 2 |
| IO_78 | G3 | Green data bit 3 |
| IO_79 | G4 | Green data bit 4 |
| IO_80 | G5 | Green data bit 5 |
| IO_81 | G6 | Green data bit 6 |
| IO_82 | G7 | Green data bit 7 |
| IO_83 | B0 | Blue data bit 0 |
| IO_84 | B1 | Blue data bit 1 |
| IO_85 | B2 | Blue data bit 2 |
| IO_86 | B3 | Blue data bit 3 |
| IO_87 | B4 | Blue data bit 4 |
| IO_88 | B5 | Blue data bit 5 |
| IO_89 | B6 | Blue data bit 6 |
| IO_90 | B7 | Blue data bit 7 |
| IO_91 | HSYNC | Horizontal sync |
| IO_92 | VSYNC | Vertical sync |
| IO_93 | DE | Data enable |
| IO_94 | PCLK | Pixel clock output |

#### ESP32-S3 Connections

| ESP32-S3 Pin | Connected To | Function |
|---|---|---|
| GPIO1 | FPGA SPI_MOSI | FPGA configuration SPI |
| GPIO2 | FPGA SPI_MISO | FPGA configuration SPI |
| GPIO3 | FPGA SPI_SCK | FPGA configuration SPI |
| GPIO4 | FPGA SPI_SS | FPGA chip select |
| GPIO5 | FPGA DONE | FPGA configuration done indicator |
| GPIO6 | FPGA CRESET | FPGA configuration reset |
| GPIO7 | FPGA CDONE | FPGA configuration done |
| GPIO8 | I²C SDA (EDID mux) | EDID/DDC bus access |
| GPIO9 | I²C SCL (EDID mux) | EDID/DDC bus access |
| GPIO10 | SD_CS | microSD chip select |
| GPIO11 | SD_MOSI | microSD SPI MOSI |
| GPIO12 | SD_MISO | microSD SPI MISO |
| GPIO13 | SD_SCK | microSD SPI clock |
| GPIO14 | CEC_RX | HDMI CEC bus receive (open-drain) |
| GPIO15 | CEC_TX | HDMI CEC bus transmit (open-drain via MOSFET) |
| GPIO16 | FPGA_IRQ | FPGA interrupt to ESP32 |
| GPIO17 | FPGA_RDY | FPGA ready signal |
| GPIO18 | CH340 TXD | USB debug UART |
| GPIO19 | CH340 RXD | USB debug UART |
| GPIO20 | BAT_ADC | Battery voltage monitor (voltage divider) |
| GPIO21 | LED_STATUS | Status LED (PWM capable) |
| GPIO42 | BTN_CONFIG | Config/trigger button |
| GPIO43 | BTN_CAPTURE | Capture button |

### 3.4 Power Architecture

```
HDMI 5V ───▶ Schottky OR ──▶ 5V Rail ──▶ XC6206P332MR ──▶ 3.3V Rail
                │                           │
Li-Po 3.7V ────┤                     XC6206P182MR ──▶ 1.8V Rail
                │
           MCP73831 ──▶ USB-C 5V (charging)
```

| Rail | Voltage | Max Current | Domains |
|---|---|---|---|
| VCC_5V0 | 5.0 V | 500 mA | HDMI 5V, USB, charger |
| VCC_3V3 | 3.3 V | 250 mA | FPGA I/O, ESP32-S3, SD card, EDID EEPROM |
| VCC_1V8 | 1.8 V | 100 mA | FPGA core, SDRAM I/O |
| VCC_1V2_FPGA | 1.2 V | 50 mA | FPGA PLL (internal regulator) |

### 3.5 Form Factor & Mechanical

| Dimension | Value |
|---|---|
| PCB Size | 65 mm × 45 mm, 4-layer |
| Thickness | 15 mm (including HDMI connectors) |
| Weight | ~45 g |
| Enclosure | 3D-printed ABS, black mate finish |
| Connectors | 2× HDMI Type A female (input/output), 1× USB-C (debug/charge) |
| Indicators | 1× RGB LED (firmware controllable, stealth-disable by jumper) |
| Controls | 2× tactile switches (config, capture) |
| Operating Temp | 0°C to 60°C |

---

## 4. Firmware Design

### 4.1 Architecture

The firmware is split between two processors:

**FPGA (Lattice iCE40UP5K):** Handles all real-time pixel processing:
- Pixel clock domain crossing from RX to TX
- Frame buffer read/write via SDRAM controller
- OSD overlay mixing (alpha-blend a 640×480 overlay plane)
- Frame capture trigger (capture on rising edge of trigger signal)
- EDID pass-through or injection
- CEC bit-banging at 3.3V logic level
- Configurable pixel manipulation (color inversion, brightness shift, region blanking)

**ESP32-S3:** Handles control plane and data exfiltration:
- WiFi configuration portal (captive portal for first-time setup)
- WebSocket API for real-time control
- BLE GATT service for mobile app control
- SPIFPGA configuration loading at boot
- SD card file system (FAT32) for frame storage
- HTTP server for frame download
- Battery management and status reporting

### 4.2 FPGA Firmware Modules

| Module | Description |
|---|---|
| `hdmi_rx_capture` | Samples 24-bit RGB + syncs from TFP401 at pixel clock rate |
| `sdram_ctrl` | SDRAM controller for IS42S16160 — open-row, auto-refresh, burst read/write |
| `frame_buffer` | Double-buffered frame buffer (two 1920×1080×24-bit partitions = ~12 MB each) |
| `osd_overlay` | 640×480 alpha-blended overlay plane from BRAM character generator |
| `pixel_passthrough` | Direct pixel pipe from RX to TX with <2 pixel clocks latency |
| `cec_controller` | CEC bus state machine (bit-banged at ~3.6 kHz) |
| `edid_sniffer` | I²C slave that monitors EDID reads and can inject alternative data |
| `capture_engine` | State machine that triggers frame write to SDRAM on capture command |
| `spi_slave` | SPI slave interface for ESP32-S3 register read/write and frame transfer |

### 4.3 ESP32-S3 Firmware Modules

| Module | Description |
|---|---|
| `main.c` | Initialization, task scheduling, watchdog, power management |
| `spi_fpga.c` | FPGA configuration and register abstraction via SPI |
| `wifi_manager.c` | WiFi AP/STA mode, captive portal, OTA updates |
| `webserver.c` | HTTP + WebSocket server for control and frame download |
| `ble_gatt.c` | BLE GATT service with device control characteristics |
| `sd_card.c` | FAT32 filesystem, frame writing, directory management |
| `capture_ctrl.c` | Capture trigger logic, frame numbering, format conversion (RGB→JPEG via software) |
| `cec_monitor.c` | CEC bus decoding, logging, and injection |
| `edid_manager.c` | EDID read, store, inject, and restore |
| `battery.c` | ADC monitoring, fuel gauging, low-battery shutdown |
| `protocol.c` | JSON wire protocol for commands and status |

### 4.4 Wire Protocol

All command/response between app → ESP32-S3 uses JSON over WebSocket (WiFi) or BLE GATT notifications:

```json
// Capture a frame
{"cmd": "capture", "params": {"format": "jpeg", "quality": 90}}

// Response
{"status": "ok", "frame_id": 42, "size": 1843200, "timestamp": "2026-06-17T04:30:00Z"}

// Set EDID override
{"cmd": "edid_set", "params": {"edid_hex": "00FFFFFFFFFFFF00..."}}

// Query status
{"cmd": "status"}
{"status": "ok", "mode": "passthrough", "resolution": "1920x1080@60",
 "hdcp": false, "cec_active": true, "battery": 85, "storage_free": 14256384}

// Inject OSD text
{"cmd": "osd_text", "params": {"text": "RECORDING", "x": 100, "y": 50,
 "color": "#FF0000", "alpha": 128}}

// CEC command
{"cmd": "cec_send", "params": {"address": 0x40, "opcode": 0x44, "payload": "00"}}

// Start covert recording
{"cmd": "covert_start", "params": {"interval_ms": 1000, "trigger": "motion",
 "max_frames": 100}}
```

---

## 5. Application Interface

### 5.1 Companion Mobile App (React Native)

The HDMI-Siphon companion app provides real-time control and monitoring:

| Screen | Features |
|---|---|
| **Dashboard** | Live status: resolution, HDCP status, CEC activity, battery level, storage remaining |
| **Capture** | Trigger single capture, configure interval recording, motion detection with sensitivity slider |
| **Gallery** | Browse captured frames as thumbnails, download, delete, share |
| **OSD** | Drag-and-drop overlay text editor; position text anywhere on screen, choose color and alpha |
| **EDID Manager** | View current EDID, upload custom EDID, toggle HDCP disable, select from presets |
| **CEC Console** | Real-time log of CEC bus activity; send CEC commands manually |
| **Covert Mode** | Arm device for covert operation: configure trigger, max frames, LED disable |
| **Settings** | WiFi configuration, firmware update, factory reset, stealth mode toggle |

### 5.2 CLI Tool (Python)

A Python CLI tool (`hdmi-siphon-cli.py`) is provided for headless operation:

```bash
# Capture a frame
python hdmi-siphon-cli.py capture --output frame.jpg --quality 95

# Set EDID to 1080p without HDCP
python hdmi-siphon-cli.py edid --preset 1080p_nohdcp

# Start interval recording (1 fps for 60 frames)
python hdmi-siphon-cli.py record --interval 1 --count 60 --output ./capture/

# Monitor CEC bus
python hdmi-siphon-cli.py cec-monitor

# Status
python hdmi-siphon-cli.py status
```

---

## 6. Use Cases

### 6.1 Red Team Physical Assessments

**Scenario: Boardroom Display Tap**  
The red team gains brief physical access to an executive boardroom. The HDMI-Siphon is inserted inline between the conference room PC and the wall-mounted display in under 30 seconds. During the meeting, slides, financial documents, and strategy presentations are captured via motion-triggered frame capture to microSD. After the meeting, the device is retrieved and frames are exfiltrated via WiFi while still in the building.

### 6.2 Kiosk & ATM Security Assessment

**Scenario: ATM Screen Capture**  
An ATM or self-service kiosk often has an HDMI connection inside the chassis accessible through a service panel. The HDMI-Siphon is inserted during an authorized physical security assessment to verify that on-screen information (account balances, transaction data, session tokens) cannot be captured and reconstructed from the video signal.

### 6.3 Digital Signage Vulnerability Research

**Scenario: Signage Content Interception**  
Digital signage players in airports, malls, and transit stations typically output via HDMI to large displays. The HDMI-Siphon can assess whether signage content — including emergency notifications, security alerts, or operational data — is transmitted without encryption and could be intercepted, modified, or replaced by an attacker with physical HDMI access.

### 6.4 HDCP Implementation Testing

**Scenario: Protected Content Path Audit**  
Security researchers assess whether an organization's display infrastructure properly enforces HDCP on all display ports. The HDMI-Siphon's EDID manipulation can disable HDCP negotiation on the source, verifying whether content protection policies are correctly implemented in the GPU driver and media pipeline.

### 6.5 HDMI-CEC Security Analysis

**Scenario: Smart TV CEC Attack Surface**  
HDMI-CEC (Consumer Electronics Control) allows devices to control each other over the HDMI bus — a remote control keypress on a smart TV or set-top box is transmitted via CEC. The HDMI-Siphon monitors this bus to capture keystrokes, channel changes, and device commands. It can also inject malicious CEC commands to power off displays, switch inputs, or trigger device resets.

### 6.6 Covert Surveillance Detection

**Scenario: Sweeping for Video Interceptors**  
Defensive security teams use the HDMI-Siphon to simulate an attacker's interception capabilities during physical security sweeps. By understanding the attack surface, defenders can recommend mitigations: tamper-evident HDMI port covers, signal encryption (HDCP mandatory policies), physical access controls, and display port disablement in sensitive areas.

---

## 7. Bill of Materials

| Item | Part | Qty | Unit Price (100) | Total |
|---|---|---|---|---|
| HDMI Receiver | TFP401APZP | 1 | $4.50 | $4.50 |
| HDMI Transmitter | TFP410PAP | 1 | $4.20 | $4.20 |
| FPGA | iCE40UP5K-UWG30 | 1 | $7.80 | $7.80 |
| MCU Module | ESP32-S3-WROOM-1 | 1 | $3.50 | $3.50 |
| SDRAM | IS42S16160J-6TL | 1 | $2.80 | $2.80 |
| EDID EEPROM | 24LC02BT-I/OT | 1 | $0.35 | $0.35 |
| Level Shifter | TXS0108EPWR | 1 | $1.20 | $1.20 |
| I²C Mux | TCA9548APWR | 1 | $1.50 | $1.50 |
| microSD Slot | 101-00397-68 | 1 | $0.55 | $0.55 |
| USB-Serial | CH340C | 1 | $0.60 | $0.60 |
| Li-Po Battery | LP503048 300mAh | 1 | $2.50 | $2.50 |
| Battery Charger | MCP73831T | 1 | $0.40 | $0.40 |
| LDO 3.3V | XC6206P332MR | 1 | $0.30 | $0.30 |
| LDO 1.8V | XC6206P182MR | 1 | $0.30 | $0.30 |
| HDMI Connectors | HDMI-A recept (2x) | 2 | $0.80 | $1.60 |
| CEC Level Shifter | BSS138 | 2 | $0.10 | $0.20 |
| Caps/Resistors/Inductors | 0402 passives | ~40 | $0.02 | $0.80 |
| PCB Fabrication | 4-layer, ENIG, 65×45mm | 1 | $5.00 | $5.00 |
| **Total** | | | | **$38.10** |

---

## 8. Repository Structure

```
hdmi-siphon/
├── README.md                    # This document
├── phase1_conceptual_architecture.md
├── phase2_component_selection_schematics.md
├── phase3_pcb_blueprints_layout.md
├── phase4_software_stack.md
├── firmware/
│   ├── main.c                   # ESP32-S3 main firmware
│   ├── board.h                  # Pin definitions and board config
│   ├── registers.h              # FPGA register map and bitfield defines
│   ├── Makefile                 # Build system for ESP-IDF
│   └── drivers/
│       ├── spi_fpga.c           # SPI communication with FPGA
│       ├── spi_fpga.h
│       ├── sd_card.c            # microSD card driver
│       ├── sd_card.h
│       ├── capture_ctrl.c       # Capture control logic
│       ├── capture_ctrl.h
│       ├── cec_monitor.c        # CEC bus monitor and injector
│       ├── cec_monitor.h
│       ├── edid_manager.c       # EDID read/write/inject
│       ├── edid_manager.h
│       ├── wifi_manager.c       # WiFi and web server
│       ├── wifi_manager.h
│       ├── ble_gatt.c           # BLE GATT service
│       ├── ble_gatt.h
│       ├── protocol.c           # Wire protocol serialization
│       ├── protocol.h
│       ├── battery.c            # Battery monitoring
│       └── battery.h
├── kicad/
│   ├── hdmi-siphon.kicad_pro    # KiCad project file
│   ├── hdmi-siphon.kicad_sch     # Schematic
│   └── hdmi-siphon.kicad_pcb     # PCB layout
├── app/
│   ├── App.js                   # React Native entry point
│   ├── package.json             # Node dependencies
│   ├── screens/
│   │   ├── DashboardScreen.js   # Main status dashboard
│   │   ├── CaptureScreen.js     # Frame capture controls
│   │   ├── GalleryScreen.js     # Captured frame gallery
│   │   ├── OSDScreen.js         # OSD overlay editor
│   │   ├── EDIDScreen.js        # EDID management
│   │   ├── CECScreen.js         # CEC console
│   │   ├── CovertScreen.js      # Covert mode configuration
│   │   └── SettingsScreen.js    # Device settings
│   ├── components/
│   │   ├── StatusCard.js        # Status indicator card
│   │   ├── FrameViewer.js       # Frame image viewer
│   │   ├── CECLog.js            # CEC event log component
│   │   └── OSDCanvas.js         # OSD drag/drop editor canvas
│   └── utils/
│       └── protocol.js          # Wire protocol helper
└── hdmi-siphon-cli.py           # Python CLI tool
```

---

## 9. Build & Flash Instructions

### 9.1 Prerequisites

- **ESP-IDF v5.1+** for ESP32-S3 firmware
- **Lattice iCEcube2 or Project IceStorm** for FPGA bitstream
- **KiCad 7.0+** for PCB editing
- **Node.js 18+** and **React Native CLI** for companion app
- **Python 3.10+** for CLI tool

### 9.2 Building Firmware

```bash
# ESP32-S3 firmware
cd firmware/
idf.py set-target esp32s3
idf.py menuconfig     # Configure WiFi, BLE, SD card settings
idf.py build
idf.py flash -p /dev/ttyUSB0

# FPGA bitstream (requires IceStorm tools)
cd fpga/
yosys -p "synth_ice40 -blif hdmi_siphon.blif" hdmi_siphon.v
arachne-pnr -d 5k -P uwg30 -o hdmi_siphon.txt hdmi_siphon.blif
icepack hdmi_siphon.txt hdmi_siphon.bin
```

### 9.3 Building App

```bash
cd app/
npm install
npx react-native run-android   # or run-ios
```

---

## 10. Legal & Ethical Use

This device is intended for:
- **Authorized penetration tests** with written consent from the target organization
- **Security research** on devices you own or have permission to test
- **Educational demonstrations** in controlled environments
- **Defensive security assessments** by blue teams

This device is NOT intended for:
- Intercepting video without consent (violates wiretapping laws)
- Capturing copyrighted content without authorization (violates DMCA)
- Surveillance of individuals without their knowledge or consent
- Any use that violates applicable local, state, federal, or international law

**Author: jayis1** — all designs, firmware, and documentation are provided for legitimate security research only.

---
