# SATA Phantom — SATA Bus Interposer for Security Research

**Author:** jayis1  
**License:** Proprietary — Authorized Security Research Use Only  
**Version:** 1.0.0  

> **⚠️ LEGAL DISCLAIMER**  
> This device is designed exclusively for authorized security research, penetration testing with written consent, and red-team operations on systems you own or have explicit permission to assess. Unauthorized interception or modification of storage bus traffic may violate computer fraud and abuse laws, data protection regulations, and privacy statutes in your jurisdiction. The author (jayis1) assumes no liability for misuse. You are responsible for complying with all applicable laws.

---

## Table of Contents

1. [Overview](#overview)  
2. [Attack Surface & Threat Model](#attack-surface--threat-model)  
3. [Hardware Specifications](#hardware-specifications)  
4. [Architecture & Block Diagram](#architecture--block-diagram)  
5. [Firmware Design](#firmware-design)  
6. [Application Interface](#application-interface)  
7. [Use Cases](#use-cases)  
8. [Operation Modes](#operation-modes)  
9. [Detection & Countermeasures](#detection--countermeasures)  
10. [Building & Flashing](#building--flashing)  

---

## Overview

The **SATA Phantom** is a hardware security research tool that operates as a **transparent man-in-the-middle interposer on the Serial ATA (SATA) bus**. Physically installed between a SATA storage device (HDD, SSD, SSHD) and the host's SATA controller, it captures, inspects, modifies, drops, or injects SATA frames in real-time without the host or drive being aware of its presence.

Unlike software-based disk monitoring (which requires kernel-level access and triggers EDR alerts), the SATA Phantom operates at the physical and link layer, making it **undetectable by any software running on the target host**. It can selectively corrupt write data, inject malicious sectors during read operations, clone drives transparently, and exfiltrate disk contents over a secondary covert channel (ESP32 WiFi/Bluetooth).

The device leverages the unique capabilities of the **Gowin GW1N-LV1 FPGA** for ultra-low-latency SATA frame forwarding combined with an **ESP32-S3** coprocessor for analysis, command-and-control, and exfiltration. The FPGA provides wire-speed frame processing with minimal added latency (~2–5 μs per frame), while the ESP32 implements higher-level protocol analysis and the RF control plane.

---

## Attack Surface & Threat Model

### Attacker Profile
- **Red team** with physical access to a target machine for <60 seconds
- **Insider threat** with brief unsupervised access to a server room
- **Supply chain interloper** embedding interposers during manufacturing/repair
- **Evil maid** who can briefly open a laptop or desktop case

### Attack Surface Covered
| Layer | Attack |
|-------|--------|
| **Physical** | Inline interposer between SATA cable/connector |
| **Link (OOB)** | OOB signal manipulation, COMRESET injection, speed negotiation downgrade |
| **Transport** | Frame-level MITM — read/write DMA transactions |
| **Application** | Sector-level data injection, corruption, exfiltration |
| **Covert** | RF side-channel (WiFi/BT) for C2 and data extraction |

### Assets at Risk
- Full disk contents (raw block-level access)
- Bootloader and kernel images (persistent firmware implants)
- Encrypted volume headers (LUKS, BitLocker, FileVault metadata)
- File system metadata for forensic manipulation

### Threat Model
```
[HOST SATA CONTROLLER] <--> [SATA Phantom] <--> [SATA DRIVE]

           |--- USB-C (debug/configuration)
           |--- ESP32 WiFi/BT (covert C2 channel)
           |--- e-Paper status display
```

The host interacts with the drive normally. The SATA Phantom:
- **Cannot be detected** by any software on the host (it is electrically transparent)
- **Passes all normal traffic** with sub-microsecond added latency
- **Selectively triggers** on configurable patterns (LBA ranges, opcodes, data patterns)
- **Operates standalone** for months on battery in deep-sleep, or indefinitely when powered from the SATA bus

---

## Hardware Specifications

### Core Components

| Component | Part | Function |
|-----------|------|----------|
| **FPGA** | Gowin GW1N-LV1 (QFN48) | SATA PHY + link-layer frame processing, transparent bridge |
| **MCU** | ESP32-S3-WROOM-1 | Protocol analysis, policy engine, C2, exfiltration |
| **SATA PHY** | TI SN75LVCP601 (×2) | High-speed differential signal redriver + mux for the interposer |
| **Oscillator** | SiTime SiT9120 25.000 MHz | Reference clock for SATA Gen1/2/3 operation |
| **Flash** | W25Q64JV (64 Mbit) | FPGA configuration storage + firmware images |
| **PSU** | TPS62065 (3.3V) + TPS74801T (1.2V FPGA core) | Power from SATA 5V/12V rails or external USB-C |
| **Battery** | 100 mAh LiPo (CP-402035) | Backup power for stealth exfiltration after host shutdown |
| **Display** | 1.54" e-Paper (GDEH0154D67) | Status, mode, and alerts (zero-power when static) |
| **Covert** | ESP32-S3 built-in 2.4 GHz WiFi + BLE 5.0 | C2 tunnel and exfiltration |
| **Debug** | USB-C (CP2102N) | Serial console, firmware update, configuration |

### SATA Interface
- **Standard**: SATA 3.0 (6 Gbps), backward compatible to SATA 1.5 Gbps / 3.0 Gbps
- **Connectors**: SATA 22-pin receptacle (host side) + SATA 22-pin plug (drive side)
- **Signals**: TX±, RX± (differential pairs), +5V, +12V, GND, staggered pinout for hot-plug
- **Features**: OOB signal detection (COMINIT/COMWAKE/COMRESET), speed negotiation passthrough
- **Latency**: < 5 μs per frame added (all Gen3 speeds)

### Power
| Source | Voltage | Current (active) | Current (sleep) |
|--------|---------|-------------------|------------------|
| SATA 5V rail | 5.0 V ± 5% | < 150 mA | < 500 μA |
| SATA 12V rail | 12.0 V ± 10% | < 50 mA | < 200 μA |
| USB-C | 5.0 V | < 200 mA | — |
| Internal LiPo | 3.7 V | < 120 mA (buck-boost) | < 50 μA |

### Form Factor
- **Dimensions**: 48 mm × 28 mm × 8 mm (fits inside a standard SATA cable shroud)
- **PCB**: 4-layer, 0.8 mm thickness, ENIG finish, controlled impedance (100 Ω ± 10%)
- **Connectors**: SATA 22-pin male on one edge, SATA 22-pin female on opposite edge
- **Weight**: ~12 g (including battery)
- **Mounting**: Sleeve/overmold design that looks identical to a short SATA extension cable

---

## Architecture & Block Diagram

```
┌──────────────────────────────────────────────────────────────────────┐
│                        SATA PHANTOM                                  │
│                                                                      │
│  ┌─────────────┐    ┌──────────────────────┐    ┌─────────────┐      │
│  │   HOST SATA  │◄──►│  SN75LVCP601 (×2)   │◄──►│  DRIVE SATA  │      │
│  │  CONNECTOR   │    │  Redriver/MUX        │    │  CONNECTOR   │      │
│  └─────────────┘    └──────────┬───────────┘    └─────────────┘      │
│                                │                                      │
│                                ▼                                      │
│                   ┌────────────────────────┐                         │
│                   │                        │                         │
│                   │   GOWIN GW1N-LV1 FPGA  │                         │
│                   │  ┌──────────────────┐  │                         │
│                   │  │ SATA PHY (Gen3)  │  │                         │
│                   │  ├──────────────────┤  │                         │
│                   │  │ Link Layer FSM   │  │                         │
│                   │  ├──────────────────┤  │                         │
│                   │  │ Frame Inspector  │  │                         │
│                   │  ├──────────────────┤  │                         │
│                   │  │ Filter/Trigger   │  │                         │
│                   │  ├──────────────────┤  │                         │
│                   │  │ Injection FIFO   │  │                         │
│                   │  ├──────────────────┤  │                         │
│                   │  │ Scratch Buffer   │  │                         │
│                   │  │ (8 KB SRAM)      │  │                         │
│                   │  └──────────────────┘  │                         │
│                   │                        │                         │
│                   │   SPI Slave Interface  │◄────┐                   │
│                   └────────────────────────┘     │                   │
│                                                   │                   │
│                   ┌────────────────────────┐     │                   │
│                   │                        │     │                   │
│                   │    ESP32-S3 MCU        │     │                   │
│                   │  ┌──────────────────┐  │     │                   │
│                   │  │ Policy Engine    │  │────┘                   │
│                   │  ├──────────────────┤  │                        │
│                   │  │ Protocol Parser  │  │                        │
│                   │  ├──────────────────┤  │                        │
│                   │  │ Exfiltration Mgr │  │                        │
│                   │  ├──────────────────┤  │                        │
│                   │  │ C2 Handler       │  │                        │
│                   │  │ (WiFi / BLE)     │  │                        │
│                   │  ├──────────────────┤  │                        │
│                   │  │ Crypto Engine    │  │                        │
│                   │  │ (AES-256-GCM)    │  │                        │
│                   │  └──────────────────┘  │                        │
│                   │                        │                        │
│                   │  UART / SPI / I2C      │                        │
│                   │  (e-Paper, debug)      │                        │
│                   └────────────────────────┘                        │
│                                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────────┐    │
│  │  TPS62065    │  │  TPS74801T   │  │  CP2102N USB-UART       │    │
│  │  3.3V Buck   │  │  1.2V LDO    │  │  (debug console)        │    │
│  └──────────────┘  └──────────────┘  └─────────────────────────┘    │
│                                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────────┐    │
│  │  W25Q64JV    │  │  100 mAh     │  │  1.54" e-Paper Display  │    │
│  │  64 Mb Flash │  │  LiPo Battery│  │  (GDEH0154D67)          │    │
│  └──────────────┘  └──────────────┘  └─────────────────────────┘    │
└──────────────────────────────────────────────────────────────────────┘
```

### Signal Flow

1. **Host → Drive (Write Path)**: Host SATA TX→ FPGA RX→ inspect→ (modify or pass)→ FPGA TX→ Drive RX
2. **Drive → Host (Read Path)**: Drive SATA TX→ FPGA RX→ inspect→ (modify or pass)→ FPGA TX→ Host RX
3. **Control**: ESP32 programs filter/trigger rules into FPGA via SPI. FPGA raises interrupts for matched frames.
4. **Exfiltration**: ESP32 receives matched frames from FPGA, optionally encrypts them, and sends via WiFi/BLE to operator.
5. **Injection**: ESP32 writes crafted frames into FPGA injection FIFO. FPGA merges them into the stream at the next idle opportunity.

---

## Firmware Design

The firmware is split across two processing domains:

### FPGA Firmware (Verilog/VHDL — synthesized to bitstream)

The FPGA implements the **critical real-time path**:

| Module | Description |
|--------|-------------|
| `sata_phy_gen3` | High-speed SERDES for SATA Gen1/2/3 (1.5/3.0/6.0 Gbps). Handles OOB signal detection (COMINIT, COMWAKE, COMRESET), 8b/10b encoding/decoding, clock recovery from incoming stream |
| `link_layer_fsm` | SATA link-layer state machine: PHY Ready, power management, ALIGN primitives, speed negotiation |
| `transport_layer` | Frame-level handling. Extracts FIS (Frame Information Structure) type, identifies DMA Setup, Data FIS, Command FIS |
| `frame_inspector` | Parallel comparison engine. Matches against up to 16 configurable rules: LBA range, opcode range, data pattern match |
| `filter_trigger` | On match: pass, drop, corrupt (flip bits), raise interrupt to ESP32, or copy to scratch buffer |
| `injection_fifo` | 512-byte circular buffer. Merges injected frames into the stream during gap intervals |
| `scratch_buffer` | 8 KB dual-port SRAM for temporary frame capture |
| `spi_slave` | SPI slave interface to ESP32 for rule programming, status reading, injection push |
| `clock_manager` | PLL for generating SATA bit clocks from 25 MHz reference |

Key design decisions:
- **Store-and-forward vs. cut-through**: Uses **cut-through** for normal traffic (minimal latency) and falls back to store-and-forward only for matched frames that require inspection or modification
- **Frame alignment**: Aligns to Dword boundaries using K28.5 comma characters per SATA specification
- **Scrambling**: Supports SATA Gen2/3 scrambler/descrambler (LFSR with polynomial x^16 + x^15 + x^13 + x^4 + 1)
- **BIST**: Built-in self-test generates PRBS patterns for link integrity verification

### MCU Firmware (C — ESP32-S3 / ESP-IDF)

The ESP32-S3 runs **FreeRTOS** with the following tasks:

| Task | Priority | Stack | Description |
|------|----------|-------|-------------|
| `ctrl_task` | High (5) | 4096 | SPI command interface to FPGA. Programs rules, reads status, manages injection FIFO |
| `policy_engine` | Medium (3) | 8192 | Interprets complex match patterns not in hardware. Manages stateful rules (e.g., "match 3 reads to same LBA then inject") |
| `proto_parser` | Medium (3) | 4096 | Parses captured SATA FIS at protocol level. Identifies NCQ commands, TRIM, secure erase attempts |
| `exfil_task` | Low (1) | 12288 | Manages exfiltration queue. Encrypts with AES-256-GCM, sends via WiFi (TCP/TLS or UDP with XOR obfuscation to avoid IPS detection) |
| `c2_task` | Low (1) | 8192 | WiFi AP + mDNS + WebSocket server for operator C2. Also BLE GATT server as fallback |
| `display_task` | Low (1) | 2048 | Updates e-Paper display with status, errors, mode |
| `power_mgmt` | High (4) | 2048 | Battery monitoring, deep-sleep entry/exit on SATA activity detection, power rail sequencing |

---

## Application Interface

### Companion App (React Native)

The SATA Phantom companion app provides the operator's control interface.

#### Screens

1. **Discovery Screen** — Scans for SATA Phantom devices on the local network via mDNS/BLE. Shows device status, battery level, current mode.

2. **Dashboard Screen** — Real-time monitoring of SATA traffic:
   - Total frames captured
   - Current transfer speed (negotiated Gen level)
   - Read/Write ratio
   - Matched/alerts count
   - Battery level and power source
   - e-Paper mirror

3. **Rules Engine Screen** — Create and deploy match/action rules:
   - LBA range sliders
   - Opcode selectors (READ DMA EXT, WRITE DMA EXT, SECURITY ERASE, etc.)
   - Action: Monitor / Log / Intercept / Modify / Drop
   - Data pattern hex editor for content matching
   - Rule priority ordering (drag-and-drop)

4. **Injector Screen** — Craft and inject SATA frames:
   - Raw FIS builder with hex editor
   - Pre-built templates: "Bootloader Redirect", "Sector Cloner", "TRIM Blocker"
   - Schedule injection for "next write to LBA range X"

5. **Exfiltration Screen** — Browse and download captured data:
   - File listing of captured sectors
   - Stream mode (real-time sector display)
   - Encrypted download over TLS
   - Stealth modes: packet timing obfuscation, random MAC rotation

6. **Console Screen** — Serial-over-WiFi terminal to the SATA Phantom's debug UART.

7. **Settings Screen** — WiFi config, encryption keys, firmware update (OTA), stealth parameters.

#### API Endpoints (REST, served by ESP32)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/v1/status` | GET | Device status, Gen level, uptime, battery |
| `/api/v1/rules` | GET/POST/DELETE | CRUD for FPGA match rules |
| `/api/v1/capture` | GET | Stream captured frames (WebSocket upgrade) |
| `/api/v1/inject` | POST | Inject a SATA frame |
| `/api/v1/exfil/list` | GET | List exfiltrated data chunks |
| `/api/v1/exfil/download/{id}` | GET | Download encrypted exfil data |
| `/api/v1/config` | GET/PUT | Device configuration |
| `/api/v1/update` | POST | OTA firmware update |
| `/api/v1/scan` | GET | Trigger SATA bus scan (device enumeration) |

---

## Use Cases

### 1. Covert Disk Forensics (Read-Side Attack)
**Scenario**: A red team gains brief physical access to a server. They install the SATA Phantom between the OS drive and motherboard. Over the next 24 hours, the Phantom captures all disk reads — the OS boots, accesses files, runs databases. The Phantom selectively logs sectors containing filesystem metadata, partition tables, and recently-modified files. The operator retrieves a complete disk map and key file contents without the host ever knowing.

**Technique**: The FPGA is programmed with LBA range rules logging partition table (LBA 0–63), MFT/GPT, and a sampling of the most-frequently-accessed sectors. ESP32 encrypts captured sectors and pushes them via WiFi in 1 KB chunks during low-activity periods.

### 2. Persistent Firmware Implant (Write-Side Attack)
**Scenario**: The target drive contains a UEFI bootloader. The SATA Phantom intercepts writes to the EFI system partition (ESP). When a legitimate bootloader update occurs, the Phantom modifies the written data in-flight to inject a bootkit payload. The drive itself believes it wrote the legitimate bootloader. The bootkit survives OS reinstallation since the ESP isn't typically wiped.

**Technique**: Rule = "WRITE DMA EXT to LBA range 0x1000–0x2000 (typical ESP), data pattern 'BOOTLOADER_SIG'". On match, FPGA replaces the sector payload with a crafted malicious bootloader while preserving the original checksum if possible.

### 3. Disk Cloning with Stealth Covert Channel
**Scenario**: During a physical penetration test, the operator needs to clone a drive without removing it from the chassis. The SATA Phantom captures all reads from the source drive and writes them to a secondary drive on the same SATA bus, or streams them out over WiFi at low bandwidth over hours/days.

**Technique**: The Phantom captures reads with no modification to the forwarded data. Captured sectors are queued in W25Q64 flash and exfiltrated via WiFi in background. On 1 Mbps WiFi with compression, a 256 GB SSD can be exfiltrated in ~6–14 days in stealth mode (nightly bursts).

### 4. Anti-Forensics: Evidence Manipulation
**Scenario**: During the post-compromise cleanup phase, the red team needs to remove traces. The SATA Phantom intercepts specific sectors on write — when the target runs a secure erase or TRIM command, the Phantom substitutes dummy data or replays old sector content from its buffer. This allows the operator to "hide" evidence by making the drive report it as erased while the Phantom preserves the actual data.

### 5. Encryption Key Harvesting
**Scenario**: Full-disk encryption (LUKS/BitLocker/FileVault) stores volume headers and key material in known sectors. The Phantom captures the first access to these sectors after power-on (when the decryption key is read). The operator later analyzes the captured data to recover key material.

### 6. Performance Degradation Attack
**Scenario**: The Phantom randomly flips a bit in every 1000th write sector. This causes silent data corruption that manifests as random application crashes and file system errors. The target troubleshoots for weeks before discovering the hardware-level corruption.

---

## Operation Modes

| Mode | Description | LED Color | Power |
|------|-------------|-----------|-------|
| **Transparent** | Pass all traffic, no inspection (safe/default) | Green blink | 50 mA |
| **Monitor** | Inspect traffic, raise alerts on matched rules, no modification | Blue blink | 80 mA |
| **Active** | Monitor + modify/inject traffic per rules | Red pulse | 120 mA |
| **Exfil** | Active + exfiltrate captured data over RF | White pulse | 150 mA |
| **Deep Sleep** | RTC-only, wake on SATA OOB signal or timer | Off | 50 μA |
| **USB Config** | Enumeration as USB CDC device for firmware update | Magenta | 60 mA |

---

## Detection & Countermeasures

### How the Target Could Detect SATA Phantom

| Method | Effectiveness | Mitigation in SATA Phantom |
|--------|--------------|----------------------------|
| SATA link timing analysis | Low — Phantom adds < 5 μs, within normal jitter | Optimized cut-through path |
| Physical inspection | Medium — requires opening chassis and examining cables | Cable-shroud form factor |
| Power draw measurement | Low-medium — Phantom draws < 150 mA, often within normal SATA power variance | Power from SATA rail, minimal overhead |
| RF spectrum analysis | Medium — WiFi/BT emissions detectable with spectrum analyzer | Burst mode on random channels, low power |
| SMART attribute monitoring | Low — Phantom doesn't affect SMART data (passthrough) | N/A — SMART commands forwarded transparently |
| CRC error injection detection | Medium — if Phantom corrupts data without fixing CRC | Phantom recomputes CRC on modified frames |
| Bus analyzer on separate SATA port | High — requires hardware-level SATA analyzer | Only mitigatable by physical stealth |

### Counter-Detection Features
- **CRC Recalculation**: When the Phantom modifies frame data, it recalculates the CRC-32/4 and replaces it. The drive/host never sees CRC errors.
- **Zero-Jitter Mode**: Adds constant delay to all frames (not just modified ones), making it impossible to detect modification via timing side-channel.
- **Random MAC Rotation**: When exfiltrating over WiFi, the ESP32 changes its MAC address every transmission burst.
- **Covert Timing**: Exfiltration uses randomized inter-packet delays (100–5000 ms) that blend with ambient network noise.
- **Supply Voltage Smoothing**: On-board LC filter and LDOs smooth any current draw transients that could be detected on the SATA power rail.

---

## Building & Flashing

### Prerequisites

- **Gowin IDE** (for FPGA synthesis): Download from Gowin Semiconductor
- **ESP-IDF v5.0+**: `pip install esptool` and install ESP-IDF toolchain
- **KiCad 7.0+**: For PCB design files
- **Node.js 18+**: For React Native companion app
- **Arm GCC toolchain**: `gcc-arm-none-eabi` for verification builds

### Quick Start

```bash
# Clone the repository
git clone https://github.com/jayis1/hacker-devices.git
cd hacker-devices/sata-phantom

# Build FPGA bitstream
cd firmware/fpga
make all          # Synthesizes, places, routes, generates .fs
cd ../..

# Build ESP32 firmware
cd firmware/esp32
idf.py build
cd ../..

# Build companion app
cd app
npm install
npx react-native run-android  # or run-ios
cd ..

# Flash (connect SATA Phantom via USB-C)
cd firmware/esp32
idf.py -p /dev/ttyUSB0 flash
cd ../fpga
openFPGALoader -b sata_phantom bitstream.fs
```

---

## File Structure

```
sata-phantom/
├── README.md                    # This file
├── firmware/
│   ├── fpga/
│   │   ├── rtl/
│   │   │   ├── sata_phy_top.v      # Top-level SATA PHY
│   │   │   ├── sata_link_layer.v   # Link layer FSM
│   │   │   ├── sata_transport.v    # Transport layer
│   │   │   ├── frame_inspector.v   # Pattern matching engine
│   │   │   ├── filter_trigger.v    # Action mux
│   │   │   ├── injection_fifo.v    # Frame injection buffer
│   │   │   ├── scratch_buffer.v    # Capture scratch RAM
│   │   │   ├── spi_slave.v         # SPI control interface
│   │   │   └── clock_manager.v     # PLL and clock tree
│   │   ├── constraints/
│   │   │   └── sata_phantom.sdc    # Timing constraints
│   │   └── Makefile
│   └── esp32/
│       ├── main/
│       │   ├── main.c              # Entry point, task creation
│       │   ├── ctrl_task.c         # FPGA control interface
│       │   ├── policy_engine.c     # High-level policy engine
│       │   ├── proto_parser.c      # FIS protocol parser
│       │   ├── exfil_task.c        # Exfiltration manager
│       │   ├── c2_server.c         # C2 WebSocket server
│       │   ├── display_task.c      # e-Paper display
│       │   ├── power_mgmt.c        # Power management
│       │   ├── crypto_engine.c     # AES-256-GCM encryption
│       │   └── board.h             # Board pin definitions
│       ├── drivers/
│       │   ├── spi_fpga.c          # SPI driver for FPGA
│       │   ├── epd_driver.c        # e-Paper display driver
│       │   ├── battery_mon.c       # Battery gauge (MAX17048)
│       │   └── registers.h         # FPGA register map
│       ├── sdkconfig               # ESP-IDF config
│       ├── CMakeLists.txt
│       └── partitions.csv
├── kicad/
│   ├── sata-phantom.kicad_pro      # KiCad project
│   ├── sata-phantom.kicad_sch      # Schematic
│   ├── sata-phantom.kicad_pcb      # PCB layout
│   ├── libraries/
│   │   ├── gowin-gw1n.pretty/     # Gowin FPGA footprint
│   │   ├── sn75lvcp601.pretty/    # SATA redriver
│   │   └── sata-connector.pretty/ # SATA 22-pin connectors
│   └── 3d_models/
│       └── sata-phantom.step       # 3D step model
└── app/
    ├── package.json
    ├── App.js                      # Root component
    ├── src/
    │   ├── navigation/
    │   │   └── AppNavigator.js     # Navigation stack
    │   ├── screens/
    │   │   ├── DiscoveryScreen.js  # Device discovery (mDNS/BLE)
    │   │   ├── DashboardScreen.js  # Real-time traffic dashboard
    │   │   ├── RulesScreen.js      # Rules engine UI
    │   │   ├── InjectorScreen.js   # Frame injection
    │   │   ├── ExfilScreen.js      # Exfiltration browser
    │   │   ├── ConsoleScreen.js    # Serial console
    │   │   └── SettingsScreen.js   # Configuration
    │   ├── components/
    │   │   ├── TrafficChart.js     # Real-time frame chart
    │   │   ├── RuleCard.js         # Drag-and-drop rule card
    │   │   ├── HexEditor.js        # Hex editor component
    │   │   ├── DeviceStatus.js     # Status indicator
    │   │   └── ExfilFileRow.js     # Exfiltrated file row
    │   └── services/
    │       ├── api.js              # REST API client
    │       ├── websocket.js        # WebSocket stream handler
    │       ├── mdns.js             # mDNS discovery
    │       └── crypto.js           # Encryption helpers
    └── __tests__/
        └── api.test.js
```

---

## References

- [Serial ATA Specification, Rev 3.5](https://sata-io.org/)
- Gowin GW1N-1 Datasheet, Gowin Semiconductor
- ESP32-S3 Technical Reference Manual, Espressif Systems
- TI SN75LVCP601 — SATA Redriver Datasheet
- "A Practical Guide to SATA Interposer Attacks" — jayis1, 2025

---

*Copyright © 2025 jayis1. All rights reserved. Authorized security research use only.*
