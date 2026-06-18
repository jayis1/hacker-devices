# GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant

**A pocket-sized, battery-powered covert hardware debug probe that auto-discovers and exploits SWD and JTAG interfaces on target embedded systems, enabling firmware extraction, runtime memory inspection, peripheral manipulation, and debug-port exploitation — all streamed over an encrypted BLE backhaul to an operator's mobile device.**

```
╔══════════════════════════════════════════════════════════════╗
║   ____ _   _    _    _   _  ____      _   ____ ___ _   _     ║
║  / ___| | | |  / \  | | | |/ ___|    / \ / ___|_ _| \ | |    ║
║ | |  _| |_| | / _ \ | | | | |  _    / _ \ |  _ | ||  \| |    ║
║ | |_| |  _  |/ ___ \| |_| | |_| |  / ___ \ |_| || || |\  |    ║
║  \____|_| |_/_/   \_\\___/ \____| /_/   \_\_____|___|_| \_|   ║
║   Covert SWD/JTAG Hardware Debug Implant                      ║
╚══════════════════════════════════════════════════════════════╝
```

**Author:** jayis1
**License:** Hardware — CERN-OHL-S v2 · Firmware — GPL-2.0 · App — MIT
**Version:** 1.0.0

---

## Table of Contents

1. [Overview & Problem Statement](#1-overview--problem-statement)
2. [Attack Surface & Threat Model](#2-attack-surface--threat-model)
3. [Hardware Specifications](#3-hardware-specifications)
4. [Architecture & Block Diagram](#4-architecture--block-diagram)
5. [Theory of Operation — SWD/JTAG Exploitation](#5-theory-of-operation--swdjtag-exploitation)
6. [Firmware Details & Design Decisions](#6-firmware-details--design-decisions)
7. [Protocol & Command Interface](#7-protocol--command-interface)
8. [Application/Software Interface](#8-applicationsoftware-interface)
9. [Use Cases for Red Teams & Pentesters](#9-use-cases-for-red-teams--pentesters)
10. [Legal & Ethical Disclaimer](#10-legal--ethical-disclaimer)

---

## 1. Overview & Problem Statement

Embedded systems — from IoT endpoints and smart-home hubs to automotive ECUs, industrial PLCs, and consumer electronics — are overwhelmingly deployed with **hardware debug ports** left physically accessible and frequently left enabled. The Serial Wire Debug (SWD) and JTAG interfaces specified by ARM and the IEEE 1149.1 standard were designed for factory programming, boundary-scan testing, and development-phase debugging. In production hardware, these same ports provide an extraordinarily powerful inroad for security researchers: direct, unmediated access to CPU debug logic, on-chip flash, SRAM, peripherals, and the system bus — bypassing every software-layer defense the target runs.

Despite this, **no purpose-built, field-deployable tool exists** that combines automated debug-port discovery, multi-protocol support, firmware extraction, runtime memory inspection, and a covert operator backhaul into a single, battery-powered, handheld unit. Researchers today cobble together desktop JTAG adapters (J-Link, ST-Link, Black Magic Probe), OpenOCD configurations, and a laptop — a setup that is conspicuous, tethered, and requires extensive manual protocol identification.

**GHOSTBUS** closes that gap. It is a self-contained covert implant that:

- **Auto-discovers** SWD and JTAG pinouts on unknown target boards via impedance-guided pin scanning.
- **Speaks both** ARM SWD (v5/v6, with Multi-Drop support) and IEEE 1149.1 JTAG (with chain enumeration).
- **Extracts firmware** from on-chip flash and external QSPI/SPI flash via the debug bus.
- **Inspects and modifies** runtime memory, registers, and peripheral state without halting the core (or with controlled halt).
- **Streams results** over an encrypted BLE 5 link to a companion mobile app, keeping the operator physically separated from the target.
- **Operates covertly** from a single LiPo cell for over 4 hours, with a magnetic-contact pogo-pin probe head that attaches in seconds.

GHOSTBUS is, in essence, a **portable, autonomous JTAG/SWD exploitation platform** — the hardware equivalent of a network implant, but for the silicon debug layer.

---

## 2. Attack Surface & Threat Model

### Target Attack Surface

GHOSTBUS targets the **hardware debug interface** attack surface, which exists on virtually every microcontroller-based device:

| Interface | Spec | Typical Pins | Capability |
|-----------|------|-------------|------------|
| ARM SWD | ARM Debug Interface v5/v6 | SWDIO, SWCLK, GND, VRef | CPU halt, memory access, flash programming, DWT/ITM trace |
| ARM SWD Multi-Drop | ARM ADIv6 + DPv2 | SWDIO, SWCLK, GND | Multi-core / multi-SoC on shared bus |
| JTAG (IEEE 1149.1) | IEEE 1149.1 | TCK, TMS, TDI, TDO, GND | Boundary scan, ICE debug, TAP-level access |
| cJTAG (compact JTAG) | IEEE 1149.7 | TCKC, TMSC | 2-pin compact JTAG |
| ARM ETM/ITM trace | ARM CoreSight | TRACECLK, TRACEDATA[0..3] | Real-time instruction/data trace |

### Threat Model

**Assets at risk (defender perspective):**
- Device firmware (intellectual property, embedded keys, algorithms)
- Runtime memory contents (cryptographic keys, session tokens, secrets)
- Secure boot configuration and eFuse/fuse-lock state
- Peripheral register state (radio config, sensor calibration, secure element access)
- Proprietary communication protocol implementations

**Adversary capabilities (GHOSTBUS operator):**
- **Physical proximity** to the target device (within pogo-pin contact range).
- **No prior knowledge** of the target's debug pinout (GHOSTBUS auto-discovers).
- **No network access** required — BLE backhaul is point-to-point to the operator's phone.
- **Ability to power-cycle** the target (optional, for cold-boot flash extraction).
- **No special host software** — all logic runs on-device; the phone is a display/control surface.

**Defender mitigations GHOSTBUS is designed to probe/defeat:**
- Debug port left enabled but unauthenticated (the most common case).
- Debug port "disabled" via a fuse/NVM bit that can be bypassed via voltage glitching or that was never actually blown (GHOSTBUS detects and reports the lock state).
- Readout protection (RDP/CRP) levels — GHOSTBUS enumerates the RDP level and, where a known vulnerability exists (e.g., STM32 RDP Level 1 downgrade, NXP FTFE unsecured fallback), can execute the applicable exploit.
- Password-protected debug ports (e.g., NXP LPC, some TI parts) — GHOSTBUS can brute-force or replay captured passwords.

**Out of scope:**
- Attacks requiring decapsulation or FIB editing of the target die.
- Side-channel attacks (power/EM/timing) — covered by companion tools (volt-glitcher, spectre-sniffer).
- Attacks on debug ports that are physically removed from the PCB (cut traces, no test pads).

---

## 3. Hardware Specifications

### Core Processing

| Component | Part | Role |
|-----------|------|------|
| Main MCU | **STM32H563ZIT6** (Cortex-M33, 250 MHz, 1 MB flash, 640 KB SRAM) | Debug-protocol engine, SWD/JTAG bit-banging, firmware extraction, crypto, BLE UART bridge |
| BLE Co-processor | **Nordic nRF52840** (Cortex-M4F, 64 MHz, 1 MB flash, 256 KB RAM) | BLE 5.0 long-range link, encrypted transport, operator command framing |
| Crypto accelerator | STM32H563 SAES + PKA | AES-GCM BLE transport, ECDH key exchange, target-side crypto offload |
| FPGA (optional, on PRO variant) | Lattice iCE40-UX1K | Hardware-assisted JTAG TAP state machine at >50 MHz, glitch-tolerant SWD |

### Debug Probe Front-End

- **8-channel universal probe head** — software-routable to SWD (SWDIO/SWCLK), JTAG (TCK/TMS/TDI/TDO), cJTAG, or power-sense.
- **Pogo-pin contacts** (1.0 mm pitch, spring-loaded gold-plated) on a magnetic mount for rapid attachment to test pads.
- **Adjustable voltage range**: 1.2 V – 5.0 V target VRef sense, with level-shifting buffers (TI TXU0104).
- **Programmable series resistors** (digital potentiometer, 0–10 kΩ per channel) for impedance matching and pull control.
- **Target power sense & optional target power injection** (3.3 V / 5 V, 500 mA, current-limited) for cold-boot extraction scenarios.

### Connectivity

| Link | Spec | Range | Use |
|------|------|-------|-----|
| BLE 5.0 | Long-range Coded PHY (125 kbps, +8 dBm) | Up to 300 m LOS | Operator backhaul, encrypted AES-GCM |
| USB-C | USB 2.0 Full-Speed (12 Mbps) | Tethered | Direct host mode, firmware update, charging |
| UART (aux) | 3.3 V, up to 3 Mbps | Board-level | Auxiliary serial console bridge to target |

### Power

- **Battery**: 1200 mAh LiPo (3.7 V nominal), integrated fuel gauge (MAX17048).
- **Runtime**: >4 hours active debug, >48 hours standby (BLE connected).
- **Charging**: USB-C PD (5 V/1 A), onboard MCP73831 LiPo charger.
- **Target power**: optional 3.3 V/500 mA injection via the probe head (current-limited, reverse-polarity protected).

### Form Factor

- **PCB**: 4-layer, 50 mm × 24 mm, ENIG finish, 0.4 mm trace/space.
- **Enclosure**: Anodized aluminum tube, 54 mm × 28 mm × 14 mm, with magnetic end-cap housing the pogo-pin probe head.
- **Weight**: 38 g (with battery), 19 g (without).
- **Indicators**: RGB LED (status), single tactile button (mode/activate), vibration motor (covert alert on discovery).
- **Operating temp**: -10 °C to +55 °C.

---

## 4. Architecture & Block Diagram

```
                        ┌─────────────────────────────────────────────────┐
                        │                 GHOSTBUS IMPLANT                  │
                        │                                                 │
   Operator Phone       │  ┌────────────┐    UART    ┌─────────────────┐  │
   (BLE 5.0)  <─────────┼──┤  nRF52840  │<─────────>│   STM32H563ZI    │  │
   Companion App        │  │  BLE 5.0   │  AES-GCM  │  Cortex-M33 250M │  │
                        │  │  Co-proc   │  transport │  Debug Engine    │  │
                        │  └─────┬──────┘            └────────┬────────┘  │
                        │        │                            │           │
                        │   ┌────┴─────┐              ┌────────┴────────┐  │
                        │   │ Antenna  │              │ Level Shifters  │  │
                        │   │ PCB     │              │ TXU0104 x2       │  │
                        │   └──────────┘              └────────┬────────┘  │
                        │                                       │           │
                        │                              ┌────────┴────────┐  │
                        │                              │  Pogo-Pin Head  │  │
                        │                              │  8x gold contacts│  │
                        │                              └────────┬────────┘  │
                        └──────────────────────────────────────┼───────────┘
                                                               │
                                                  ┌────────────┴────────────┐
                                                  │    TARGET DEVICE         │
                                                  │  (MCU / SoC / ECU)       │
                                                  │  SWD / JTAG / cJTAG      │
                                                  │  Test pads / header      │
                                                  └─────────────────────────┘
```

### Firmware Block Diagram (STM32H563)

```
   ┌──────────────────────────────────────────────────────────────┐
   │                      Application Layer                        │
   │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐  │
   │  │Discovery │ │  SWD     │ │  JTAG    │ │  Memory Inspector │  │
   │  │ Engine   │ │  Engine  │ │  Engine  │ │  / Flasher        │  │
   │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────────┬─────────┘  │
   │       │            │            │                │            │
   │  ┌────┴────────────┴────────────┴────────────────┴─────────┐ │
   │  │              Debug Protocol Abstraction                   │ │
   │  └────┬─────────────────────────────────────────┬──────────┘ │
   │       │                                         │            │
   │  ┌────┴──────┐  ┌────────────┐  ┌───────────┐  ┌┴──────────┐  │
   │  │SWD Bit-   │  │JTAG Bit-   │  │ Target     │  │ Crypto     │ │
   │  │bang GPIO  │  │bang GPIO  │  │ Power Ctrl │  │ (AES/ECDH) │ │
   │  └───────────┘  └───────────┘  └───────────┘  └───────────┘  │
   │                       │                                      │
   │              ┌────────┴────────┐                             │
   │              │  HAL / CMSIS    │                             │
   │              │  (STM32H5)      │                             │
   │              └─────────────────┘                             │
   └──────────────────────────────────────────────────────────────┘
```

---

## 5. Theory of Operation — SWD/JTAG Exploitation

### 5.1 Debug Port Discovery

The first challenge on an unknown target is identifying which PCB test pads correspond to SWD/JTAG signals, and which protocol the target speaks. GHOSTBUS performs **impedance-guided pin scanning**:

1. **Continuity map**: For each pair of the 8 probe channels, GHOSTBUS measures DC resistance (via a precision current source and ADC). Ground and VRef/VCC pins are identified by their low resistance to the known GND/VCC rail (when target is powered) or by voltage level (when powered).

2. **Pull characterization**: Each remaining channel is driven with a weak pull-up and weak pull-down in sequence, and the resulting settled voltage is read. SWDIO and TMS pins typically have onboard pull resistors, producing characteristic "mid" levels. SWCLK/TCK are often pulled down or floating.

3. **Protocol probing**: Candidate pin assignments are enumerated and tested:
   - For SWD: drive SWCLK with a slow clock (100 kHz), toggle SWDIO through the SWD line reset (50+ cycles of '1') and issue a `DP_IDCODE` read. A valid 32-bit IDCODE with correct parity indicates SWD is present on that pin pair.
   - For JTAG: drive TCK, walk the TAP through Test-Logic-Reset → Shift-DR, and read TDO. A valid IDCODE (with the mandatory 0b0001 LSB) in the first DR indicates JTAG.
   - For cJTAG: similar but on the 2-pin compact interface.

4. **Scoring**: Each candidate is scored by response validity, IDCODE match against the GHOSTBUS SoC-ID database (3000+ entries), and signal integrity (oversampling for edge jitter). The best-scoring pinout is reported to the operator.

### 5.2 SWD Exploitation

Once SWD is identified, GHOSTBUS establishes the Debug Port (DP) and Access Port (AP) chain:

- **DP initialization**: line reset, read `IDCODE`, write `ABORT` to clear sticky errors, power up the debug (`CDBGPWRUPREQ`) and system (`CSYSPWRUPREQ`) domains.
- **AP enumeration**: walk APSEL 0–255, reading `IDR` on each. Non-zero `IDR` indicates a present Access Port. GHOSTBUS classifies AHB-AP, APB-AP, and JTAG-AP types.
- **Memory access**: via the AHB-AP `CSW`/`TAR`/`DRW` registers, GHOSTBUS performs 8/16/32-bit reads and writes anywhere in the target's memory map. This includes flash (read), SRAM (read/write), and peripheral registers.
- **Core control**: via the Debug Fault Status Register and DHCSR, GHOSTBUS can halt, single-step, resume, and read/write core registers (R0–R15, xPSR).
- **Flash extraction**: GHOSTBUS reads the entire flash region (e.g., 0x08000000–0x080FFFFF on STM32) in 4 KB blocks, computing SHA-256 per block for integrity and streaming over BLE.

### 5.3 JTAG Exploitation

For JTAG targets, GHOSTBUS:

- Performs a TAP reset (TMS=1 for 5+ clocks) and reads the instruction register length by shifting in `SAMPLE/PRELOAD` and measuring the response.
- Reads `IDCODE` from each TAP in the chain (chain length auto-detected).
- Identifies each TAP by IDCODE against the database.
- Uses the TAP's debug instruction (vendor-specific, often `EXTEST`/`INTEST`/`SAMPLE`) to access the on-chip debug module, or bit-bangs a vendor debug protocol over the data register.
- Supports boundary-scan for pin-state extraction and force.

### 5.4 Readout Protection (RDP) Handling

GHOSTBUS reads the target's RDP/CRP level via the debug interface (where exposed) and reports it. Where a known downgrade or bypass vulnerability exists in the database, GHOSTBUS offers the operator a one-tap exploit:

- **STM32F1/F4 RDP Level 1 → Level 0 downgrade** (erases flash but enables full debug).
- **STM32L0 Option Byte manipulation** via the debug port.
- **NXP LPC password replay** (captured from a prior session).
- **Generic fuse-read** for parts exposing eFuse state over debug.

The operator confirms each exploit due to its destructive nature.

### 5.5 Covert BLE Backhaul

All target data is AES-256-GCM encrypted (session key from ECDH P-256 exchange at BLE pairing) and sent over BLE 5.0 Coded PHY. The phone app decrypts and displays. GHOSTBUS never stores extracted firmware in plaintext on its own flash — only an encrypted cache for retransmission on BLE link loss, keyed to the session, purged on power-down.

---

## 6. Firmware Details & Design Decisions

### 6.1 Architecture

The STM32H563 firmware is a bare-metal (no RTOS) cooperative scheduler design. This was chosen over an RTOS for three reasons:

1. **Deterministic timing**: SWD/JTAG bit-banging requires sub-microsecond jitter. An RTOS context switch (10s of µs) would corrupt the protocol. Bare-metal with interrupt-driven BLE UART buffering keeps the debug engine timing deterministic.
2. **Code-size**: the full firmware fits in <80 KB, leaving room for the SoC-ID database and exploit scripts.
3. **Power**: no RTOS tick means deeper sleep between debug operations; critical for battery life.

### 6.2 SWD Bit-Banging

SWD is bit-banged on GPIO with carefully tuned timing:

- `SWDIO` is bidirectional — driven as output during write phases, switched to input with a pull-up for read phases. The turnaround cycle (1 idle clock) is enforced.
- `SWCLK` is driven at a programmable rate: default 1 MHz (safe for unpowered-pull targets), up to 25 MHz for known-good targets via DMA-timed SPI output.
- Parity (even) is computed and checked on every 32-bit transfer.
- Line reset (>50 cycles of SWDIO=1) is issued before each DP operation to recover from desync.

### 6.3 JTAG Bit-Banging

JTAG uses 4 dedicated GPIOs (TCK, TMS, TDI, TDO). The TAP state machine is implemented as a state-indexed table (16 states) with next-state computed from TMS bit — this avoids branching and keeps TCK-to-TMS setup time under 20 ns. For high-speed TAP walking (>10 MHz), GHOSTBUS uses the SPI peripheral to clock TCK while DMA feeds TDI/TMS from a precomputed buffer.

### 6.4 Crypto

All BLE traffic is encrypted with AES-256-GCM (96-bit tag, 96-bit nonce from a monotonic counter). Session keys are established via ECDH P-256 at pairing using the STM32H563's PKA accelerator. The device's long-term identity key is stored in the STM32H563's TrustZone-protected OTP, and the private key never leaves secure memory.

### 6.5 SoC-ID Database

A compact (12 KB) lookup table maps DP-IDCODE / JTAG-IDCODE values to vendor, family, flash base, SRAM base, RDP register offset, and known exploit IDs. The table is stored in a separate firmware section for OTA updates as new parts are characterized.

### 6.6 BLE Co-processor Protocol

The nRF52840 runs a thin BLE GATT server with a custom service (UUID `8a7b1c2d-...`). Characteristics:

| Characteristic | UUID suffix | Direction | Purpose |
|----------------|-------------|-----------|---------|
| Command | `...01` | Phone → GhostBus | Operator commands (JSON-framed, AES-GCM) |
| Telemetry | `...02` | GhostBus → Phone | Status, progress, discovery results |
| Data | `...03` | GhostBus → Phone | Extracted firmware / memory dumps (chunked) |
| Auth | `...04` | Bidirectional | ECDH key exchange at pairing |

Chunked data uses a 20-byte header (session id, sequence, total length, SHA-256 of payload) followed by up to 180 bytes of ciphertext per BLE packet (MTU 247 → ~200 usable with GATT overhead).

---

## 7. Protocol & Command Interface

The phone app sends JSON commands (AES-GCM encrypted), e.g.:

```json
{
  "op": "discover",
  "channels": [0,1,2,3,4,5,6,7],
  "protocols": ["swd","jtag","cjtag"],
  "timeout_ms": 5000
}
```

GHOSTBUS responds with telemetry:

```json
{
  "event": "discovery_result",
  "protocol": "swd",
  "swdio_ch": 2,
  "swclk_ch": 0,
  "gnd_ch": 7,
  "vref_mv": 3300,
  "idcode": "0x2BA01477",
  "soc": "ARM Cortex-M4 (CoreSight-DP)",
  "aps": [
    {"apsel":0,"idr":"0x24770011","type":"AHB-AP","name":"STM32F4 AHB"},
    {"apsel":1,"idr":"0x00000000","type":"none"}
  ],
  "rdp_level": 1,
  "rdp_bypass_available": "stm32f4_rdp1_downgrade"
}
```

Supported operations: `discover`, `swd_read`, `swd_write`, `swd_halt`, `swd_resume`, `swd_step`, `swd_read_core_reg`, `swd_flash_extract`, `swd_erase`, `jtag_scan_chain`, `jtag_read_dr`, `jtag_write_dr`, `jtag_boundary_scan`, `power_target_on`, `power_target_off`, `rdp_exploit`, `status`, `battery`, `lock`.

---

## 8. Application/Software Interface

The companion app (React Native) provides four primary screens:

- **Dashboard**: connection status, battery, target SoC identification, RDP level, quick actions.
- **Discovery**: channel map visualization, live impedance scan, protocol auto-detect with confidence scoring.
- **Memory**: hex viewer/editor over the target memory map; flash extraction with progress and SHA-256 verification; register inspection with named-bitfield decoding for known peripherals.
- **Console**: raw command interface for advanced operators; session log; exploit script runner.

The app connects via BLE using `react-native-ble-plx`, handles AES-GCM decryption in JS, and stores extracted firmware (encrypted at rest with the phone's secure enclave) for offline analysis.

---

## 9. Use Cases for Red Teams & Pentesters

1. **Physical red-team drop device**: GHOSTBUS is small enough (38 g) to be pre-positioned against a target device's test pads during a physical access window; it then autonomously discovers the debug port, extracts firmware, and exfiltrates over BLE while the operator is elsewhere in the building.

2. **Firmware extraction for vulnerability research**: pull the full flash image from an IoT endpoint in seconds for offline reverse engineering, without the target ever knowing (debug access is non-invasive at the software level).

3. **Runtime secret extraction**: halt the target core, dump SRAM, and recover live cryptographic keys, session tokens, or Wi-Fi credentials held in RAM.

4. **Secure-boot assessment**: read the RDP/eFuse state and attempt known downgrade vulnerabilities to evaluate the effectiveness of the target's debug-locking strategy.

5. **Peripheral fuzzing**: via the AHB-AP, write random values to peripheral registers (radio, USB, sensor) to discover undocumented or vulnerable register states.

6. **Automotive ECU research**: connect to an ECU's debug pads (often accessible under the connector shell) and read calibration data, firmware, or inject runtime patches for dynamic analysis.

7. **Supply-chain integrity audit**: verify that a batch of devices has debug ports properly disabled, as a manufacturing-QC security gate.

8. **Covert post-exploitation persistence**: a GHOSTBUS left attached to an internal device can serve as a long-term, hardware-level backdoor that survives firmware updates and factory resets.

---

## 10. Legal & Ethical Disclaimer

GHOSTBUS is a **security research tool** intended exclusively for use by authorized security professionals, penetration testers, and researchers conducting assessments on systems they own or have explicit written authorization to test.

**Unauthorized access to computer systems, including via hardware debug interfaces, is illegal in most jurisdictions** (e.g., US CFAA 18 U.S.C. § 1030, UK Computer Misuse Act 1990, EU Directive 2013/40/EU). Attaching GHOSTBUS to a device you do not own or do not have permission to test may constitute a criminal offense.

By using GHOSTBUS you affirm that:
- You are testing systems you own or for which you have written authorization.
- You will not use GHOSTBUS to access, extract, or modify data belonging to third parties without consent.
- You understand that debug-port exploitation can cause permanent damage to the target (e.g., RDP downgrade erases flash) and accept that risk.
- You will comply with all applicable local, national, and international laws.

The author (**jayis1**) and contributors provide GHOSTBUS strictly as a research platform and accept no liability for misuse or for any damage caused by its use. The tool is provided "as is" without warranty of any kind.

**If you are unsure whether your use is authorized, do not use this tool.**

---

*GHOSTBUS — because the debug port is the universal backdoor.*
*© jayis1. Released under CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app).*