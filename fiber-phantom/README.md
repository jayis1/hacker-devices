# FiberPhantom — Covert Fiber Optic Network Tap & MITM Implant

**Author:** jayis1  
**Version:** 1.0  
**Date:** 2026-06-17  

---

## ⚠️ Legal & Ethical Disclaimer

**This device is designed exclusively for authorized security research, penetration testing, and red team operations.** Use of FiberPhantom against any network, fiber infrastructure, or communications link without explicit written authorization from the asset owner is illegal in most jurisdictions and may constitute a federal crime (e.g., 18 U.S.C. § 2511, § 1030; CFAA; ECPA; equivalent laws in other countries). The author (jayis1) does not condone or support unauthorized interception of communications. You are solely responsible for ensuring you have proper authorization before deploying this device. This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**FiberPhantom** is a covert, battery-powered fiber optic network tap and man-in-the-middle (MITM) implant designed for physical security assessments of fiber-optic infrastructure. Unlike traditional copper-based network taps, FiberPhantom targets optical fiber links — the backbone of data center interconnects, campus backbones, metro networks, and dark fiber runs that are typically assumed to be physically secure against passive eavesdropping.

The device supports three deployment modes that trade off stealth against capability:

| Mode | Invasiveness | Capability | Description |
|------|-------------|------------|-------------|
| **Bend-Tap** | Non-invasive | Read-only passive monitoring | Clips onto an unmodified fiber jacket; induces a controlled macro-bend to siphon ~0.5–2% of light. No service disruption, no cable modification. |
| **Inline-Couple** | Semi-invasive | Full-duplex tap + MITM injection | Splices the device inline using mechanical splices; integrated 2×2 fused coupler taps 10% to APD, allows VCSEL injection onto the through-path. |
| **SFP+ Pass-Through** | Invasive | Full transparent access | Plugs into an SFP+ cage on network equipment; appears as a standard optical transceiver but mirrors all traffic to the FPGA. |

FiberPhantom recovers the tapped optical signal using an Avalanche Photodiode (APD) with a transimpedance amplifier (TIA), performs Clock/Data Recovery (CDR) and framing in an onboard FPGA, and provides multiple exfiltration channels: local MicroSD capture, BLE command-and-control, and an optional 60 GHz millimeter-wave backhaul for high-bandwidth real-time streaming.

The companion mobile application (React Native) allows the operator to configure tap modes, set MITM rules, monitor capture statistics, and exfiltrate captured PCAP data over BLE — all from a safe standoff distance.

---

## 2. Attack Surface & Threat Model

### 2.1 What FiberPhantom Targets

Fiber optic links are the high-value backbone of modern network infrastructure. They carry:
- Data center east-west traffic (10G/25G/40G/100G Ethernet over single-mode or multimode fiber)
- Campus and metro backbone links (10GBASE-LR, 10GBASE-ER)
- Storage Area Networks (Fibre Channel 8G/16G/32G)
- Passive Optical Network (PON) uplinks (GPON/XGSPON)
- Dark fiber leased lines between data centers
- Industrial control system backhaul links (SCADA, ICS)

Security teams typically assume fiber is secure because:
1. It doesn't radiate electromagnetic signals (unlike copper)
2. Physical access to fiber runs requires data center cages, riser conduits, or manhole access — which are assumed to be physically secured
3. Tapping fiber is assumed to require expensive equipment and skilled splicing

**FiberPhantom challenges assumption #3.** It makes fiber tapping accessible with a purpose-built, compact, battery-operated device that can be deployed in seconds (bend-tap mode) or minutes (inline-couple mode) by a red team operator with brief physical access.

### 2.2 Threat Model

| Threat Actor | Capability | Example Scenario |
|-------------|-----------|-----------------|
| Red team operator | Physical access to fiber for <60 seconds | Bend-tap clip deployment in a data center cable tray during a walk-through |
| Red team operator | Physical access to fiber for 5–15 minutes | Inline-couple splice insertion during after-hours data center access |
| Red team operator | Access to network equipment with SFP+ ports | SFP+ pass-through insertion during hardware swap or maintenance window |
| Insider threat | Access to fiber patch panels or cable trays | Persistent tap installation on inter-rack links |

### 2.3 What FiberPhantom Can Achieve

- **Passive traffic capture**: Full packet capture of 1G/10G fiber Ethernet to MicroSD (PCAP format)
- **Protocol analysis**: On-device inspection of Ethernet, IP, TCP, UDP, VLAN, VXLAN, MPLS, Fibre Channel frames
- Credential extraction: On-device regex matching for plaintext credentials in captured streams
- **MITM frame modification**: Selective modification/injection of Ethernet frames on the through-path (inline-couple and SFP+ modes)
- **ARP/DNS spoofing**: Inject forged ARP responses or DNS replies to redirect traffic through attacker-controlled infrastructure
- **TLS downgrade attacks**: Inject TCP RST frames to force connection resets, potentially causing clients to fall back to unencrypted protocols
- **Network mapping**: Passive discovery of VLAN topology, IP addressing, MAC relationships, and traffic patterns

### 2.4 Detection Countermeasures

FiberPhantom's key advantage is stealth. However, sophisticated defenders can deploy:
- **Optical Time Domain Reflectometers (OTDR)**: Can detect anomalies (bend-induced loss, splice points) on the fiber. Bend-tap mode introduces ~0.3–0.5 dB insertion loss; inline-couple mode introduces ~1.5–2 dB (coupler + splices).
- **Link-loss monitoring**: Network equipment with DOM (Digital Optical Monitoring) can detect unexpected receive power drops.
- **Physical inspection**: Visual inspection of cable runs for non-standard clips or splices. FiberPhantom's form factor mimics standard fiber management clips to reduce visual detection.
- **Encrypted traffic**: TLS/IPsec-encrypted traffic resists credential extraction, though traffic analysis (flow patterns, metadata) remains possible.

---

## 3. Hardware Specifications

### 3.1 Processor Architecture

| Component | Part | Role |
|-----------|------|------|
| Main MCU | Raspberry Pi RP2040 (dual Cortex-M0+ @ 133 MHz) | Control plane, SPI master to FPGA, SD card I/O, power management, mode selection |
| FPGA | Lattice iCE40 UltraPlus UP5K (5.3K LUTs, 128 KB SPRAM) | Clock/data recovery, Ethernet framing, MITM rule engine, PCAP stream formatting |
| BLE/C2 Module | nRF52840 module (Cortex-M4F @ 64 MHz) | BLE 5.0 command-and-control link, standby communication, status reporting |
| 60 GHz Backhaul (optional) | Silicon Radar TRX_120_003 or equivalent | High-bandwidth covert exfiltration link (~1.5 Gbps) for real-time tap streaming |

### 3.2 Optical Front-End

| Component | Part | Role |
|-----------|------|------|
| Optical Tap (Inline mode) | 2×2 fused biconical taper coupler, 90:10 split, 1310nm/1550nm dual-band | Passive tap of through-path light + injection port for VCSEL |
| Optical Receiver | Excelitas C30659 or equivalent InGaAs APD + Maxim MAX3658 TIA | High-sensitivity optical-to-electrical conversion, ~-30 dBm sensitivity |
| Optical Transmitter (MITM injection) | 1310nm VCSEL (e.g., Finisar/II-VI) or 850nm VCSEL for multimode | Injects modulated light onto through-path via coupler input port |
| Bend Coupler (Bend-tap mode) | Custom molded polymer bend fixture with integrated micro-PD (Si PD for 850nm, InGaAs PD for 1310/1550nm) | Non-invasive light siphoning from fiber jacket |
| SFP+ Cage (SFP+ mode) | Standard SFP+ receptacle cage | Accepts the device as a pass-through SFP+ module |

### 3.3 Connectivity

| Interface | Usage |
|-----------|-------|
| BLE 5.0 (nRF52840) | Primary C2 channel, up to ~50m range, encrypted (ECDH key exchange, AES-CCM) |
| SPI (RP2040 ↔ FPGA) | 4-wire SPI @ 48 MHz for configuration and data transfer |
| UART (RP2040 ↔ nRF52840) | Control plane communication between MCU and BLE module |
| MicroSD (UHS-I) | Local packet capture storage, up to 512 GB |
| USB-C | Battery charging, firmware flashing, direct data exfiltration |
| 60 GHz mmWave (optional) | High-bandwidth covert backhaul, ~1.5 Gbps, line-of-sight up to ~200m |

### 3.4 Power

| Parameter | Value |
|-----------|-------|
| Battery | 1200 mAh LiPo (3.7V nominal) |
| Battery Life (tap-only mode) | ~8 hours continuous |
| Battery Life (MITM active mode) | ~4 hours continuous |
| Battery Life (idle/standby) | ~72 hours |
| Charging | USB-C PD (5V/2A), full charge in ~90 min |
| Power Consumption (active) | ~3.5W (tap mode), ~6.0W (MITM mode with VCSEL) |

### 3.5 Form Factor

- **Dimensions**: 82 mm × 34 mm × 14 mm (approx. size of a large USB flash drive)
- **Weight**: ~28g (with battery)
- **Enclosure**: Matte black anodized aluminum (EMI shielding + thermal), form factor mimics a standard fiber optic cable management clip
- **Deployment**: Magnetic clip-on for bend-tap mode; mechanical splice holders fold out for inline-couple mode; SFP+ edge connector exposed for pass-through mode
- **Status indicators**: Single multi-color LED (hidden behind diffuser, only visible from specific angle — stealth indicator)

---

## 4. Architecture & Block Diagram

```
                              ┌──────────────────────────────────────────────────────────┐
                              │                     FIBER LINK                            │
                              │  Network Side ◄──────────────────────────► Target Side  │
                              └──────┬───────────────────────────┬──────────────────────┘
                                     │ (through path)             │ (tap path)
                              ┌──────▼──────┐             ┌──────▼──────┐
                              │ 2×2 Fused   │             │   APD +      │
                              │ Coupler     │             │   TIA        │
                              │ (90:10)     │             │ (Receiver)  │
                              └──────┬──────┘             └──────┬──────┘
                                     │ (injection)               │ (analog signal)
                              ┌──────▼──────┐             ┌──────▼──────┐
                              │  VCSEL      │             │  Limiting   │
                              │ (Injector)  │             │  Amp + CDR  │
                              └──────┬──────┘             └──────┬──────┘
                                     │                           │
                                     │                    ┌──────▼──────┐
                              ┌──────▼──────┐             │  iCE40 UP5K │
                              │ RP2040      │◄──SPI──────►│  FPGA       │
                              │ (Main MCU)  │  48 MHz     │  - CDR      │
                              │             │             │  - Framer   │
                              │ - Mode Ctrl │             │  - MITM     │
                              │ - SD Card   │             │  - PCAP     │
                              │ - Power Mgmt│             │  Engine    │
                              └──────┬──────┘             └─────────────┘
                                     │ UART
                              ┌──────▼──────┐             ┌──────────────┐
                              │ nRF52840    │◄────BLE────►│  Operator    │
                              │ (BLE C2)    │    5.0      │  Mobile App  │
                              └──────┬──────┘             └──────────────┘
                                     │
                              ┌──────▼──────┐
                              │ 60 GHz      │ (optional)
                              │ Backhaul    │
                              └─────────────┘

  ┌─────────────────────────────────────────────────────────────────────┐
  │                    BEND-TAP MODE (non-invasive)                      │
  │  Fiber ──► [Bend Clip w/ micro-PD] ──► TIA ──► FPGA (read-only)     │
  │  No coupler, no VCSEL. Passive monitoring only.                     │
  └─────────────────────────────────────────────────────────────────────┘
```

### 4.1 Signal Flow

**Bend-Tap Mode (read-only):**
1. Operator clips FiberPhantom onto a fiber cable at a cable tray or patch panel
2. The bend fixture induces a controlled macro-bend (radius ~12mm), causing ~0.5–2% of light to leak through the jacket
3. The micro-PD at the bend point captures the leaked light; TIA amplifies the photocurrent
4. Limiting amplifier normalizes the signal; FPGA CDR recovers clock and data
5. FPGA performs Ethernet framing (8b/10b decode, line-rate recovery, packet boundary detection)
6. Valid frames are timestamped and written to MicroSD in PCAP format and/or streamed over BLE/60GHz

**Inline-Couple Mode (tap + MITM):**
1. Operator cuts the fiber and splices both ends into FiberPhantom's mechanical splice holders
2. The 2×2 fused coupler passes 90% of light straight through (network → target) with ~1.5 dB loss
3. 10% is tapped to the APD receiver for monitoring (same signal flow as bend-tap)
4. For MITM: the FPGA analyzes passing frames against a rule table; matching frames can be modified, dropped, or replaced
5. Modified frames are encoded and driven into the VCSEL, which injects light through the coupler's second input port, combining with the through-path signal
6. The target sees the modified optical signal

**SFP+ Pass-Through Mode:**
1. FiberPhantom is inserted into an SFP+ cage on a switch or media converter
2. It presents standard SFP+ I2C identification (mimics a legitimate transceiver's EEPROM)
3. Optical signal from the network enters via the SFP+ RX; the internal photodiode recovers data
4. The FPGA taps the recovered data (full visibility, no insertion loss)
5. The VCSEL re-transmits the (possibly modified) signal on the SFP+ TX to the target equipment
6. This mode provides the cleanest MITM capability — full electrical access to both directions

---

## 5. Firmware Details

### 5.1 Architecture

The firmware runs on the RP2040 (main MCU) and is responsible for:
- **Mode selection and initialization** — detects deployment mode based on hardware jumpers/straps
- **FPGA configuration** — loads bitstream into iCE40 UP5K over SPI at boot
- **MITM rule management** — receives rules from BLE C2, programs into FPGA rule RAM
- **SD card capture** — manages PCAP file writing, handles wear leveling
- **Power management** — monitors battery, controls sleep/wake, manages VCSEL power
- **BLE C2 protocol** — communicates with nRF52840 over UART for status reporting and command reception
- **APD bias control** — adjusts APD reverse bias voltage for optimal sensitivity vs. noise

### 5.2 Key Design Decisions

**Why RP2040 + iCE40 instead of a single SoC?**  
The RP2040's PIO (Programmable I/O) state machines are useful for SPI-to-FPGA bridging and SD card interfacing at high speed, but they cannot perform 10G CDR. The iCE40 UP5K handles the high-speed optical data path with deterministic timing. This separation keeps the control plane simple and the data plane fast.

**Why an external nRF52840 instead of RP2040's built-in BLE?**  
The RP2040 has no integrated radio. An external nRF52840 module provides a proven, certifiable BLE 5.0 stack with hardware AES acceleration, freeing the RP2040 for data path management.

**Why PCAP format for local capture?**  
PCAP (libpcap format) is the universal standard for packet capture. By writing PCAP files directly, captured data can be analyzed with Wireshark, tcpdump, or any standard packet analysis tool without conversion. The FPGA handles PCAP header generation and frame timestamping in hardware.

**APD bias control loop:**  
The APD requires a precise reverse bias voltage (typically 30–80V depending on the APD type) for optimal sensitivity. Too low: poor sensitivity. Too high: avalanche breakdown, excessive noise, potential APD damage. The firmware implements a closed-loop bias control that monitors the TIA output level and adjusts the bias via a DAC to maintain optimal SNR across varying input power levels.

### 5.3 File Structure

```
firmware/
├── Makefile              — ARM cross-compile build system
├── main.c                — Entry point, mode selection, main loop
├── board.h               — Pin assignments, peripheral config, constants
├── registers.h           — RP2040 register definitions
├── drivers/
│   ├── apd_driver.c/h    — APD bias control, signal level monitoring
│   ├── cdr_driver.c/h    — FPGA SPI interface, CDR control, data readback
│   ├── ble_c2_driver.c/h  — UART protocol to nRF52840 BLE module
│   ├── sd_driver.c/h     — MicroSD SPI driver, PCAP file writing
│   └── vcsel_driver.c/h  — VCSEL modulation control, injection timing
```

---

## 6. Application / Software Interface

### 6.1 Companion App

The FiberPhantom companion app is a React Native application that provides the operator with a mobile control interface over BLE 5.0.

**Screens:**
- **Dashboard** — Connection status, battery level, deployment mode, link rate, capture stats
- **Tap Config** — Configure tap mode, APD sensitivity, capture filters
- **MITM Rules** — Create/edit/delete frame modification rules (pattern match → action)
- **Capture** — Start/stop capture, view live frame count, download PCAP over BLE

### 6.2 BLE Protocol

The BLE C2 protocol uses a custom GATT service (UUID `6e400001-...`) with two characteristics:
- **TX characteristic** (`6e400002-...`): App → Device (commands)
- **RX characteristic** (`6e400003-...`): Device → App (status + notifications)

Commands are framed with a simple TLV (Type-Length-Value) protocol over BLE notification chunks (max 20 bytes payload per chunk, reassembled by sequence number).

### 6.3 PCAP Analysis

Captured PCAP files stored on the MicroSD can be:
1. Downloaded over BLE (slow, ~20 KB/s effective throughput)
2. Downloaded over USB-C when physically recovered
3. Streamed over 60 GHz backhaul (fast, ~1.5 Gbps)

---

## 7. Use Cases

### 7.1 Red Team: Data Center Fiber Tap

During an authorized physical red team engagement, the operator gains brief access to a data center colocation facility. They identify a fiber patch panel connecting two racks. Using **bend-tap mode**, they clip FiberPhantom onto an active 10GBASE-LR link, capture ~30 minutes of traffic to MicroSD, and remove the device. The captured PCAP reveals VLAN topology, internal IP ranges, unencrypted internal services, and potentially credentials transmitted in plaintext over HTTP/Telnet/FTP.

### 7.2 Red Team: MITM on Metro Fiber

An organization's two data centers are connected via a leased dark fiber pair. The red team identifies a manhole access point along the fiber route. During a night-time operation, they access the manhole, cut the fiber, and splice in FiberPhantom using **inline-couple mode**. The device performs selective DNS spoofing, injecting forged DNS responses for specific internal domain queries, redirecting target machines to attacker-controlled infrastructure while passing all other traffic transparently. The MITM rules are configured and updated over BLE from a vehicle parked within 30m.

### 7.3 Security Researcher: Fiber Protocol Fuzzing

A researcher studying Fibre Channel protocol implementations uses FiberPhantom in **SFP+ pass-through mode** inserted between a host bus adapter (HBA) and a storage array. The device captures all Fibre Channel frames for analysis. The researcher uses the MITM engine to inject malformed FC frames (oversized payloads, invalid Opcodes, corrupted CRCs) to test the robustness of the HBA's frame parser, identifying potential buffer overflow conditions.

### 7.4 Penetration Tester: Network Topology Discovery

During an internal network assessment, the tester encounters a fiber uplink between core and distribution switches that they cannot tap via traditional Ethernet methods (no SPAN port available, switches managed by a different team). Using FiberPhantom in **bend-tap mode**, they passively capture the uplink traffic, mapping the full Layer 2/Layer 3 topology, identifying VLAN trunk configurations, HSRP/VRRP configurations, and routing protocol (OSPF/BGP) neighbor relationships.

### 7.5 Insider Threat Simulation

To test an organization's fiber-link monitoring capabilities, a purple team deploys FiberPhantom in **inline-couple mode** on a production fiber link and monitors whether the SOC detects the ~1.8 dB insertion loss via OTDR or DOM alarms. This validates (or exposes gaps in) the organization's physical-layer monitoring and incident response procedures.

---

## 8. Bill of Materials (Summary)

| Ref | Component | Qty | Est. Cost |
|-----|-----------|-----|-----------|
| U1 | RP2040 (QFN-56) | 1 | $1.00 |
| U2 | iCE40 UP5K (SG48) | 1 | $5.50 |
| U3 | nRF52840 module | 1 | $4.00 |
| U4 | MAX3658 TIA (QSOP-16) | 1 | $8.00 |
| U5 | Excelitas C30659 InGaAs APD | 1 | $45.00 |
| U6 | 1310nm VCSEL (TO-46) | 1 | $12.00 |
| U7 | W25Q128JV SPI Flash (FPGA bitstream) | 1 | $1.20 |
| U8 | AP2112K-3.3 LDO | 1 | $0.30 |
| U9 | AP2112K-1.2 LDO | 1 | $0.30 |
| U10 | MCP73831 LiPo charger | 1 | $0.50 |
| U11 | DAC5311 (APD bias DAC) | 1 | $2.00 |
| U12 | TPS61085 boost converter (APD bias 65V) | 1 | $2.50 |
| OPT1 | 2×2 fused fiber coupler 90:10 (1310/1550nm) | 1 | $15.00 |
| OPT2 | Mechanical splice holders (LC) | 2 | $2.00 |
| J1 | USB-C connector | 1 | $0.80 |
| J2 | MicroSD socket | 1 | $0.60 |
| J3 | SFP+ edge connector (for SFP+ mode) | 1 | $3.00 |
| BAT1 | 1200 mAh LiPo | 1 | $4.00 |
| PCB | 4-layer FR4, 82×34mm | 1 | $5.00 |
| **Total** | | | **~$108.70** |

---

## 9. Limitations & Future Work

### Current Limitations
- **Bend-tap mode is read-only**: Macro-bend coupling can only siphon light out, not inject it in. MITM requires inline-couple or SFP+ mode.
- **10G maximum for CDR**: The iCE40 UP5K CDR handles up to ~10G. 25G/40G/100G links can only be captured in bend-tap mode using an external CDR; inline MITM is limited to 10G.
- **Insertion loss**: Inline-couple mode introduces ~1.8 dB loss. Links operating near the power budget margin may experience link degradation.
- **Single-wavelength**: The APD is optimized for 1310/1550nm. 850nm multimode links require a different PD (Si photodiode) — the bend-tap micro-PD is wavelength-specific.

### Future Enhancements
- **Coherent detection**: Add a local oscillator laser for coherent detection of phase-modulated signals (DP-QPSK, DP-16QAM used in 100G+ links)
- **DWDM tap**: Multi-channel WDM tap for capturing multiple wavelength channels simultaneously
- **On-device TLS fingerprinting**: Identify TLS client/server fingerprints (JA3/JA4) from captured traffic
- **AI-based credential extraction**: On-device ML model for extracting credentials from captured streams without full PCAP exfiltration

---

## 10. References

1. ITU-T G.652 — Characteristics of a single-mode optical fibre and cable
2. IEEE 802.3ae — 10 Gigabit Ethernet (10GBASE-LR/LW/ER/EW)
3. Maxim Integrated MAX3658 — Compact, 155Mbps to 3.2Gbps, 3.3V Transimpedance Amplifier
4. Lattice Semiconductor iCE40 UltraPlus Family Data Sheet
5. Raspberry Pi RP2040 Datasheet
6. NFC Forum — SFP+ MSA (Small Form Factor Pluggable Multi-Source Agreement)
7. PCAP format specification — libpcap/WinPcap file format

---

*Designed and authored by jayis1. For authorized security research use only.*