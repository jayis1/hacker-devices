# Spectre-Sniffer — Portable Electromagnetic Side-Channel Analysis & Tempest Surveillance Device

**Author: jayis1**  
**Version: 1.0.0**  
**License: Proprietary — Authorized Security Research Use Only**

---

## 1. Overview

The **Spectre-Sniffer** is a portable, handheld electromagnetic (EM) side-channel analysis and Tempest surveillance platform designed for security researchers, hardware penetration testers, and red team operators. It captures and processes unintentional electromagnetic emissions from electronic devices to extract sensitive information — including cryptographic keys, display content, keystroke timing, and processor activity — without any physical or electrical contact with the target.

Named after the Spectre vulnerability family (which exploited CPU side channels), the Spectre-Sniffer extends the concept from software to hardware: instead of exploiting microarchitectural side channels in software, it exploits the **physical electromagnetic side channels** that every electronic device inevitably radiates.

The device combines a wideband RF front-end (10 MHz – 8 GHz), a high-speed 16-bit ADC (130 MSPS), an FPGA for real-time signal processing, and a dual-core ESP32-S3 for application logic and wireless exfiltration. It fits in a rugged handheld form factor with a 2.8" touchscreen, making it deployable in field operations, on-site assessments, and laboratory environments alike.

### 1.1 Key Capabilities

- **Tempest / van Eck Phreaking**: Reconstruct video display content from unintended EM leakage at distances up to 10 meters
- **Cryptographic Side-Channel Extraction**: Capture EM traces during crypto operations (AES, RSA, ECC) and perform correlation power analysis (CPA) or differential power analysis (DPA) to recover secret keys
- **Keystroke Timing Analysis**: Detect and classify keystroke EM signatures from wired and wireless keyboards
- **Processor Activity Profiling**: Identify code execution patterns, loop structures, and branch activity from CPU EM emissions
- **Hidden Electronics Detection**: Locate active electronics behind walls, in enclosures, or within devices using EM field sensing
- **Power Supply Noise Analysis**: Extract data-modulated signals from power supply EMI
- **Signal Recording & Replay**: Capture raw IQ data to SD card for offline analysis with SDR tools (GNU Radio, MATLAB)

### 1.2 Target Audience

- **Red Team Operators**: Physical penetration testing, covert surveillance, and information gathering
- **Hardware Security Researchers**: Side-channel vulnerability assessment, cryptographic implementation testing
- **Certification Labs**: EMC/EMI testing, TEMPEST certification pre-screening
- **Embedded Security Engineers**: Product security validation, leakage assessment
- **Law Enforcement (Authorized)**: Forensic analysis of electronic devices

---

## 2. Attack Surface & Threat Model

### 2.1 Target Devices

| Target Type | EM Leakage Mechanism | Information Extracted |
|---|---|---|
| LCD/LED Displays | Pixel clock harmonics, LVDS/eDP cable radiation | Screen content reconstruction |
| Smartphones | CPU clock harmonics, baseband processor emissions | Keystrokes, app activity, crypto operations |
| Laptop/Desktop Computers | VGA/DVI/HDMI cable radiation, CPU power rail modulation | Display content, keystroke timing, encryption keys |
| Smart Cards / HSMs | Power consumption modulation during crypto ops | AES/RSA/ECC secret keys |
| IoT Devices | Unshielded MCU emissions, wireless SoC radiation | Firmware execution traces, encryption keys |
| Network Equipment | Switch ASIC emissions, PHY clock leakage | Traffic patterns, protocol timing |
| Keyboards (PS/2, USB) | Matrix scan emissions, cable radiation | Keystroke content and timing |

### 2.2 Attacker Capabilities (Assumed)

- Physical proximity to target: 0.1 cm (contact probe) to 10 m (directional antenna)
- No physical modification or electrical connection to target required
- Ability to operate covertly (passive receiver only — no emissions from the sniffer itself when in passive mode)
- Recording duration: minutes to hours (limited by SD card capacity and battery)
- Real-time processing for display reconstruction; post-processing for crypto analysis

### 2.3 Threat Model

**Attacker Goal**: Extract confidential information from a target device by analyzing its unintentional electromagnetic emanations.

**Attacker Position**: The attacker is within RF range of the target device. They may be in an adjacent room, outside a window, or at the same desk. The sniffer is concealed in a bag, briefcase, or custom enclosure.

**Defender Capabilities**: The defender may have:
- Shielded enclosures / Faraday cages
- TEMPEST-certified equipment (MIL-STD-461, NATO SDIP-27)
- EM-suppressing display filters
- Cryptographic implementations with masking and randomization
- Physical access controls preventing probe placement

**Attack Scenarios**:
1. **Covert Screen Capture**: Place sniffer in a backpack near a monitor in a public space (coffee shop, airport lounge). Reconstruct screen content in real time.
2. **Key Extraction**: Position the H-field probe near a smart card reader during authentication. Capture EM traces and recover the secret key via DPA.
3. **Keystroke Logging**: Place the E-field probe near a keyboard cable. Classify keystroke EM signatures to recover typed text.
4. **Firmware Reverse Engineering**: Monitor CPU EM emissions during boot to identify code structure, loop counts, and memory access patterns.

### 2.4 Legal & Ethical Disclaimer

> **WARNING**: The Spectre-Sniffer is designed exclusively for authorized security research, penetration testing with written permission, and educational purposes. Intercepting electromagnetic emissions from devices you do not own or do not have explicit permission to test may violate local, state, and federal laws including but not limited to wiretapping statutes, computer fraud laws, and telecommunications regulations. The author (jayis1) assumes no liability for misuse. **Always obtain written authorization before testing.**

---

## 3. Hardware Specifications

### 3.1 Core Components

| Component | Part Number | Specification | Purpose |
|---|---|---|---|
| **MCU** | ESP32-S3-WROOM-1 | Dual-core Xtensa LX7 @ 240 MHz, 512 KB SRAM, 16 MB Flash | Application processor, WiFi, UI, storage |
| **FPGA** | Lattice iCE40UP5K-UWG30 | 5280 LUTs, 120 KB BRAM, 4 PLLs, DSP | Real-time signal processing, FFT, filtering |
| **ADC** | LTC2208CUP-14 | 16-bit, 130 MSPS, 700 MHz bandwidth, LVDS output | Wideband digitization of EM signals |
| **LNA** | Mini-Circuits PSA4-5043+ | 0.4–8 GHz, 20 dB gain, 0.9 dB NF | Wideband low-noise amplification |
| **Downconverter** | MAX2682 | 400 MHz–2.5 GHz, 15 dB conversion gain | Tunable downconversion for high-band signals |
| **Tunable Filter** | PE82305 | 10–3000 MHz, 3 dB bandwidth 5% of center | Pre-select filtering to avoid aliasing |
| **Display** | ILI9341 2.8" TFT | 320×240, 18-bit color, SPI + touch | User interface and spectrum visualization |
| **Storage** | microSD (SPI mode) | Up to 512 GB | Raw IQ capture, configuration, firmware updates |
| **Battery** | 18650 Li-Ion (3.7V, 3500 mAh) | Protected cell, with TP4056 charger + DW01 protection | Field operation (4–6 hours typical) |
| **Power Management** | MCP73871 + TPS63020 | 3.3V/1.8V/1.2V rails, 95% efficiency | Regulated power from battery or USB-C |
| **USB Interface** | CP2102N | USB 2.0 to UART + USB-C connector | Programming, data transfer, charging |
| **RTC** | DS3231 | ±2 ppm accuracy, I²C | Timestamping captured data |
| **Probe Interface** | SMA (2x) + U.FL (2x) | 50 Ω impedance | H-field probe, E-field probe, external antenna |

### 3.2 Probe Options

| Probe | Type | Frequency Range | Application |
|---|---|---|---|
| **H-Probe (Near-Field)** | Shielded loop, 10 mm diameter | 1 MHz – 3 GHz | Magnetic field coupling for IC-level probing |
| **E-Probe (Near-Field)** | 5 mm monopole tip | 1 MHz – 8 GHz | Electric field coupling for cable/PCB emissions |
| **Log-Periodic Antenna** | 800 MHz – 6 GHz | Far-field directional | Tempest reception at distance |
| **Active Differential Probe** | Dual H-field loops | 100 kHz – 1 GHz | Differential EM sensing, common-mode rejection |

### 3.3 Form Factor

- **Dimensions**: 180 mm × 80 mm × 35 mm (handheld)
- **Weight**: 320 g (with battery and probes)
- **Enclosure**: 3D-printed ABS with EMI gaskets on seam
- **IP Rating**: IP54 (splash resistant with gaskets)
- **Operating Temperature**: -10°C to +50°C

### 3.4 Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Spectre-Sniffer Block Diagram                 │
│                                                                     │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────────┐ │
│  │ H-Probe   │───▶│ LNA      │───▶│ Tunable  │───▶│ Downconverter│ │
│  │ (SMA)     │    │ PSA4-5043│    │ Filter   │    │ MAX2682      │ │
│  └──────────┘    └──────────┘    │ PE82305  │    └──────┬───────┘ │
│                                  └──────────┘           │          │
│  ┌──────────┐    ┌──────────┐                           │          │
│  │ E-Probe  │───▶│ LNA      │───▶  Optional bypass ─────┘          │
│  │ (SMA)    │    │ (2nd ch) │    for <400 MHz direct               │
│  └──────────┘    └──────────┘                           │          │
│                                                         ▼          │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    LTC2208 ADC (16-bit, 130 MSPS)            │  │
│  │  LVDS data out ────────────────────────────────────────────▶ │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                         │          │
│                                                         ▼          │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              Lattice iCE40UP5K FPGA                          │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────────────┐  │  │
│  │  │ ADC     │  │ FIR     │  │ FFT     │  │ Trigger/      │  │  │
│  │  │ Capture │──▶│ Decimate│──▶│ 1024-pt │──▶│ Buffer       │  │  │
│  │  └─────────┘  └─────────┘  └─────────┘  └───────┬───────┘  │  │
│  │                                                   │          │  │
│  │  ┌────────────────────────────────────────────────┘          │  │
│  │  │  SPI / Parallel Bridge to ESP32-S3                        │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                         │          │
│                                                         ▼          │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              ESP32-S3 (Application Processor)                │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐  │  │
│  │  │ Signal   │  │ Tempest  │  │ Crypto   │  │ WiFi/BLE  │  │  │
│  │  │ Analysis │  │ Decode   │  │ DPA/CPA  │  │ Exfil     │  │  │
│  │  └──────────┘  └──────────┘  └──────────┘  └───────────┘  │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐                 │  │
│  │  │ UI / LCD │  │ SD Card  │  │ RTC      │                 │  │
│  │  │ (SPI)    │  │ (SPI)    │  │ (I²C)    │                 │  │
│  │  └──────────┘  └──────────┘  └──────────┘                 │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                         │          │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Power Management (MCP73871 + TPS63020)                       │  │
│  │  18650 → 3.3V/1.8V/1.2V rails, USB-C charging               │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 4. Firmware Architecture

### 4.1 Design Philosophy

The firmware is split into two processing domains:

1. **FPGA (Verilog)**: Real-time, deterministic signal processing — ADC capture, decimation, FFT, triggering, and buffering. The FPGA handles all time-critical operations that cannot tolerate jitter or latency from a microcontroller.

2. **ESP32-S3 (C)**: Application-level processing — signal analysis algorithms, Tempest decoding, DPA/CPA computation, user interface, file I/O, and wireless communication. The ESP32 runs FreeRTOS for task management.

### 4.2 FPGA Firmware (Verilog)

| Module | Description |
|---|---|
| `adc_capture` | LVDS deserialization of LTC2208 data, 16-bit samples at 130 MSPS |
| `fir_decimate` | Configurable FIR decimation filter (decimation factor 2–256) |
| `fft_1024` | 1024-point real FFT with Hanning window, 16-bit fixed-point |
| `trigger_unit` | Configurable trigger on signal level, edge, or pattern |
| `buffer_ctrl` | Dual-port ping-pong buffer management for continuous capture |
| `spi_bridge` | SPI slave interface to ESP32-S3 for configuration and data transfer |

### 4.3 ESP32-S3 Firmware (C)

| Module | Description |
|---|---|
| `main.c` | System initialization, task creation, main loop |
| `drivers/adc.c` | ADC configuration and data readout via FPGA bridge |
| `drivers/fpga.c` | FPGA configuration bitstream loading and register access |
| `drivers/display.c` | ILI9341 TFT driver with touch input |
| `drivers/sdcard.c` | FAT32 filesystem via SPI (FatFs library) |
| `drivers/rf_frontend.c` | LNA gain control, filter tuning, downconverter config |
| `drivers/rtc.c` | DS3231 RTC driver for timestamping |
| `signal/fft_processing.c` | FFT data post-processing, peak detection, averaging |
| `signal/tempest_decode.c` | Video signal reconstruction from EM leakage |
| `signal/dpa_engine.c` | Differential Power Analysis engine for key recovery |
| `signal/cpa_engine.c` | Correlation Power Analysis engine |
| `signal/keystroke_analyzer.c` | Keystroke EM signature classification |
| `ui/main_screen.c` | Main spectrum display and control interface |
| `ui/tempest_view.c` | Tempest screen reconstruction view |
| `ui/crypto_analyzer.c` | Cryptographic side-channel analysis view |
| `ui/menu.c` | Settings and configuration menu |
| `net/wifi_exfil.c` | WiFi-based data exfiltration and remote control |
| `net/ble_exfil.c` | BLE beacon for covert status reporting |
| `utils/circular_buffer.c` | Lock-free circular buffer for ADC data |
| `utils/config_manager.c` | JSON configuration read/write to SD card |

### 4.4 Data Flow

```
Antenna → LNA → Tunable Filter → [Downconverter] → ADC (130 MSPS)
                                                      │
                                                      ▼
                                              FPGA Capture
                                              (LVDS deser, FIFO)
                                                      │
                                                      ▼
                                              FIR Decimation
                                              (2x–256x decimate)
                                                      │
                                                      ▼
                                         ┌────────────┴────────────┐
                                         ▼                        ▼
                                    FFT Engine              Raw IQ Buffer
                                    (1024-pt,                (to ESP32)
                                     Hanning)
                                         │                        │
                                         ▼                        ▼
                                    Peak Detect              SD Card Write
                                    Averaging                (capture mode)
                                         │
                                         ▼
                                    ESP32-S3
                                    ┌────┴────┐
                                    ▼         ▼
                              Tempest      Crypto
                              Decode       Analysis
                                    │         │
                                    ▼         ▼
                              Display      Key Output
                                            (screen/WiFi)
```

---

## 5. Application / Software Interface

### 5.1 Companion App (React Native)

The Spectre-Sniffer companion app provides remote control, data visualization, and post-processing capabilities on a smartphone or tablet.

**Screens**:

1. **Dashboard**: Device status, battery level, signal strength, recording indicator
2. **Spectrum Analyzer**: Real-time waterfall and FFT display streamed from device over WiFi
3. **Tempest View**: Reconstructed screen image from captured EM leakage
4. **Crypto Analyzer**: Trace visualization, DPA/CPA correlation plots, key candidate display
5. **Capture Manager**: Browse, download, and delete captures from SD card
6. **Settings**: Probe calibration, filter configuration, trigger settings, WiFi/BLE config

### 5.2 REST API (ESP32-S3 HTTP Server)

| Endpoint | Method | Description |
|---|---|---|
| `/api/status` | GET | Device status, battery, temperature, signal level |
| `/api/capture/start` | POST | Start IQ capture with parameters (freq, bandwidth, duration) |
| `/api/capture/stop` | POST | Stop current capture |
| `/api/capture/list` | GET | List captures on SD card |
| `/api/capture/download/<id>` | GET | Download capture file |
| `/api/spectrum` | GET | Current FFT data (JSON array) |
| `/api/spectrum/stream` | GET | WebSocket stream of FFT data |
| `/api/tempest/start` | POST | Start Tempest decoding |
| `/api/tempest/image` | GET | Current reconstructed image (PNG) |
| `/api/crypto/traces` | GET | Captured crypto traces |
| `/api/crypto/analyze` | POST | Run DPA/CPA analysis on captured traces |
| `/api/config` | GET/PUT | Device configuration |
| `/api/calibrate` | POST | Run probe calibration routine |

### 5.3 Command-Line Interface (USB Serial)

A text-based CLI is available over the USB-C serial port for headless operation:

```
spectre> help
Available commands:
  capture start <freq_hz> <bw_hz> <duration_s>  - Start IQ capture
  capture stop                                   - Stop capture
  capture list                                   - List captures
  tempest start <freq_hz>                        - Start Tempest decode
  tempest stop                                   - Stop Tempest decode
  crypto capture <n_traces>                      - Capture crypto traces
  crypto analyze <algorithm>                     - Run DPA/CPA analysis
  spectrum                                       - Show current spectrum
  config get/set <key> [value]                   - Get/set configuration
  cal probe                                      - Calibrate connected probe
  status                                         - Show device status
  wifi scan                                      - Scan WiFi networks
  wifi connect <ssid> <password>                 - Connect to WiFi
  reboot                                         - Reboot device
```

---

## 6. Use Cases

### 6.1 Red Team — Covert Screen Capture (Tempest Attack)

**Scenario**: A red team operator needs to capture the screen of a target's monitor in a secure facility without physical access.

**Setup**: The operator places the Spectre-Sniffer in a backpack or briefcase with the log-periodic antenna pointing toward the target monitor. The device is configured to scan for the monitor's pixel clock harmonic (typically 30–150 kHz for LCD, or the VGA/DVI pixel clock in the 25–400 MHz range).

**Operation**: The sniffer's FPGA locks onto the pixel clock fundamental, and the Tempest decoder reconstructs the raster scan. The reconstructed image is displayed on the sniffer's screen and simultaneously streamed via WiFi to the operator's smartphone.

**Result**: The operator obtains a readable reconstruction of the target screen from up to 10 meters away, through a window or thin wall.

### 6.2 Hardware Security — Cryptographic Key Extraction

**Scenario**: A security researcher is evaluating a smart card reader for side-channel vulnerability.

**Setup**: The H-field probe is positioned directly above the smart card's crypto coprocessor (0.5–2 mm distance). The sniffer is configured to trigger on the power-up sequence of the card reader.

**Operation**: The researcher initiates 10,000 authentication attempts with known plaintext. For each attempt, the sniffer captures a 10 µs EM trace window around the AES encryption operation. The DPA engine processes the traces, sorting them by a hypothetical key byte hypothesis and computing differential traces.

**Result**: After processing, the DPA engine reveals the correct AES key bytes as statistically significant spikes in the differential traces. The key is displayed on screen and saved to the SD card.

### 6.3 Penetration Testing — Keystroke Recovery

**Scenario**: A penetration tester is assessing the physical security of a keyboard in a shared office space.

**Setup**: The E-field probe is clipped near the keyboard cable. The sniffer is configured for the 1–50 MHz range where keyboard matrix scan emissions are strongest.

**Operation**: The keystroke analyzer captures EM signatures and classifies them using a pre-trained template library. Each keystroke is logged with a timestamp and confidence score.

**Result**: The tester recovers typed passwords, confidential emails, and other sensitive input with >85% accuracy after calibration.

### 6.4 Forensic Analysis — Hidden Device Detection

**Scenario**: A security auditor is sweeping a conference room for hidden electronics (bugs, cameras, tracking devices).

**Setup**: The sniffer is set to broadband scan mode (10 MHz – 8 GHz) with the H-field probe. The device sweeps across frequency bands while the operator walks the perimeter of the room.

**Operation**: The FPGA continuously computes FFTs and the ESP32 detects anomalous spectral peaks that don't correspond to known ambient signals. Peaks are geotagged (via manual position logging) and displayed on a heat map.

**Result**: The auditor identifies a hidden Bluetooth beacon behind a picture frame and a GSM-based audio bug inside a power outlet.

### 6.5 Supply Chain Security — Electronics Verification

**Scenario**: An organization receives a shipment of network switches and wants to verify they haven't been tampered with.

**Setup**: The sniffer is placed next to each switch during power-on. The EM signature (spectral fingerprint) is captured and compared against a known-good baseline.

**Operation**: The FPGA captures the first 5 seconds of boot-up EM emissions. The ESP32 computes a spectral fingerprint and compares it to the stored reference using cross-correlation.

**Result**: A switch with a hardware trojan (extra IC, modified PCB trace) shows a >15% deviation in its EM fingerprint, flagging it for further inspection.

---

## 7. Performance Specifications

| Parameter | Value |
|---|---|
| Frequency Range | 10 MHz – 8 GHz (direct to 400 MHz, downconverted above) |
| Instantaneous Bandwidth | 65 MHz (Nyquist-limited at 130 MSPS) |
| ADC Resolution | 16 bits (effective 12.3 bits at 130 MSPS) |
| Noise Floor (with LNA) | -168 dBm/Hz (typical) |
| Max Capture Duration | Limited by SD card (512 GB ≈ 4.5 hours at 130 MSPS) |
| FFT Rate | 126,000 FFTs/sec (1024-point) |
| Tempest Refresh Rate | 1–15 fps (dependent on resolution and SNR) |
| DPA Trace Memory | 65,536 traces × 1024 samples (in external PSRAM) |
| Battery Life | 4 hours (active capture), 8 hours (standby monitoring) |
| WiFi Range | 50 m (line of sight) |
| BLE Range | 10 m |

---

## 8. Bill of Materials (Estimated)

| Component | Qty | Unit Cost | Total |
|---|---|---|---|
| ESP32-S3-WROOM-1 | 1 | $8.50 | $8.50 |
| Lattice iCE40UP5K-UWG30 | 1 | $12.00 | $12.00 |
| LTC2208CUP-14 | 1 | $85.00 | $85.00 |
| PSA4-5043+ LNA | 2 | $15.00 | $30.00 |
| MAX2682 Downconverter | 1 | $4.50 | $4.50 |
| PE82305 Tunable Filter | 1 | $35.00 | $35.00 |
| ILI9341 2.8" TFT | 1 | $12.00 | $12.00 |
| 18650 Battery (3500 mAh) | 1 | $8.00 | $8.00 |
| MCP73871 + TPS63020 | 1 | $6.00 | $6.00 |
| CP2102N | 1 | $2.50 | $2.50 |
| DS3231 RTC | 1 | $3.00 | $3.00 |
| SMA Connectors | 4 | $1.00 | $4.00 |
| U.FL Connectors | 2 | $0.50 | $1.00 |
| PCB (4-layer, ENIG) | 1 | $25.00 | $25.00 |
| Passive Components | 1 | $15.00 | $15.00 |
| 3D-Printed Enclosure | 1 | $10.00 | $10.00 |
| H-Field Probe (DIY) | 1 | $5.00 | $5.00 |
| E-Field Probe (DIY) | 1 | $3.00 | $3.00 |
| **Total** | | | **~$270.00** |

---

## 9. References & Further Reading

- *"Compromising Emanations: Eavesdropping Risks of Computer Displays"* — W. van Eck, 1985
- *"Electromagnetic Side-Channel Analysis of Cryptographic Devices"* — D. Genkin, L. Pachmanov, I. Pipman, E. Tromer
- *"Differential Power Analysis"* — P. Kocher, J. Jaffe, B. Jun, 1999
- *"TEMPEST: A Signal Problem"* — NSA/CSS, NACSIM 5000
- *"EM Side-Channel Attacks on Commercial Contactless Smartcards Using Low-Cost Equipment"* — T. Plos, M. Hutter
- *"A Practical Guide to Side-Channel Analysis of Cryptographic Hardware"* — F. Regazzoni, P. Kocher

---

*Spectre-Sniffer — Design by jayis1. For authorized security research only.*
