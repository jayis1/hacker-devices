# WattPhantom — USB-C Power Delivery Manipulation & Covert Channel Platform

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

```
   ╔══════════════════════════════════════════════════════════════════════╗
   ║      █████╗ █████╗ ███████╗██╗   ██╗  ██████╗ ████████╗██╗  ██╗███████╗ ║
   ║      ██╔══██╗██╔══██╗██╔════╝██║   ██║  ██╔══██╗╚══██╔══╝██║  ██║██╔════╝ ║
   ║      ██████╔╝███████║███████╗███████║  ██║  ██║   ██║   ███████║███████╗ ║
   ║      ██╔══██╗██╔══██║╚════██║██╔══██║  ██║  ██║   ██║   ██╔══██║██╔════╝ ║
   ║      ██║  ██║██║  ██║███████║██║  ██║  ██████╔╝   ██║   ██║  ██║███████╗ ║
   ║      ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝  ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚══════╝ ║
   ║   USB-C Power Delivery Attacker · CC-Line Covert Channel · Fingerprint   ║
   ║   Power Contract Spoofing · Overvoltage · Power DoS · MITM                ║
   ╚══════════════════════════════════════════════════════════════════════╝

   Author:  jayis1
   Version: 1.0
   Date:    2026-08-03
```

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Manipulating USB-C Power Delivery contracts on devices you do not own may cause permanent hardware damage, violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), product safety regulations (e.g., IEC 62368-1, USB-IF compliance specifications), and anti-tampering statutes. Forcing overvoltage conditions on unauthorized equipment may constitute property destruction or sabotage. Using CC-line covert channels to exfiltrate data from systems you do not own or are not authorized to test may violate wiretapping, data-protection, and computer-crime laws. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

---

## Overview

**WattPhantom** is a pocket-sized, battery-operated inline USB-C device for **Power Delivery (PD) protocol exploitation, CC-line covert channel communication, and device charging-fingerprint analysis**. It physically sits between a USB-C power source (charger, host, hub) and a USB-C target (phone, laptop, IoT device, peripheral), giving the operator full bidirectional control over the PD negotiation, the VBUS power path, and the USB data path — all commanded from a mobile app over encrypted BLE.

| Capability | What it does | Attack surface |
|------------|--------------|----------------|
| **PD Contract Spoofing** | Emulates source, sink, or dual-role port; advertises arbitrary PDOs (Power Data Objects) including non-compliant voltage/current combinations | Forces target to accept dangerous VBUS voltages (up to 48V with PD 3.1 EPR) |
| **Overvoltage Attack** | Negotiates a high-voltage PDO then switches VBUS to a higher voltage mid-contract | Destroys unprotected targets; causes thermal runaway in charging ICs |
| **Power DoS** | Selectively denies, interrupts, or oscillates power contracts | Renders target non-functional; drains battery; causes boot loops |
| **CC-Line Covert Channel** | Exfiltrates data via PD Vendor Defined Messages (VDMs) on the Configuration Channel | Bypasses USB data blocks; works on locked-down/MDM-enforced devices; invisible to USB monitoring |
| **Charging Fingerprinting** | Profiles the exact PDO requests, timing, and current draw patterns of a target | Identifies device model, OS version, charging state, and running apps |
| **USB Data MITM** | Passes through USB 2.0/3.2 data while independently manipulating PD | Intercepts USB traffic while the target believes it has a legitimate power contract |
| **Role Swap Attack** | Triggers unexpected PR_Swap/DR_Swap mid-connection | Can reverse data direction, trigger host-mode exploits on OTG-capable targets |
| **Hard Reset Exploitation** | Injects PD Hard Reset at critical moments | Causes target power-state confusion; forces re-negotiation with attacker-controlled parameters |

### Why WattPhantom is novel

USB-C Power Delivery is a complex negotiated protocol where two devices communicate over a single wire (the Configuration Channel, CC) using BMC (Biphase Mark Coding) at 600 kbps to agree on voltage (5V–48V), current (up to 5A), and data role. The security of the entire USB-C ecosystem rests on the assumption that both ends of the cable are honest — but the protocol has **no authentication, no encryption, and no integrity checks**. WattPhantom exploits this:

1. **No existing tool** combines PD protocol manipulation with a covert channel and device fingerprinting in a single pocket device. Research attacks like Tencent's "BadPower" (2020) demonstrated that malicious chargers can damage devices, but there is no open-source hardware tool for red teams to test these attack classes.
2. **The CC-line covert channel is unique.** Most data-exfiltration defenses focus on USB data lines, WiFi, and Bluetooth. WattPhantom encodes data into PD VDM (Vendor Defined Message) packets on the CC wire — a channel that no USB monitoring tool, EDR solution, or network DLP system inspects. A compromised system can exfiltrate data through its charging port even when USB data is disabled by policy.
3. **Charging fingerprinting enables physical tracking.** Every USB-C device has a unique PD negotiation fingerprint: which PDOs it requests, the timing between messages, the exact current profile during charging. WattPhantom captures this fingerprint, enabling red teams to identify devices, detect cloning, and track individuals across locations via their charging behavior.
4. **Inline MITM is physically invisible.** WattPhantom is designed as a USB-C "extension" — it looks like a short cable dongle (~55×22×12mm). The target sees a legitimate charger on one end; the charger sees a legitimate device on the other. The operator controls everything in between.

---

## Attack Surface and Threat Model

### What WattPhantom Exposes

USB-C Power Delivery (defined in USB PD Rev 3.1, IEC 62680-1-3) is a negotiated power protocol. Two devices on either end of a USB-C cable communicate over the CC (Configuration Channel) pin using BMC at 600 kbps to exchange structured 32-bit messages (Header + up to 7×32-bit Data Objects). These messages define:

- **Source Capabilities (Source_Capabilities message)**: The charger advertises what voltages/currents it can provide as PDOs.
- **Request (Request message)**: The target selects a PDO and requests it.
- **Accept / Ready / PS_RDY**: The handshake sequence that brings VBUS to the requested voltage.
- **VDMs (Vendor Defined Messages)**: Extensible messages for cable discovery, alternate mode negotiation, and vendor-specific data — **unauthenticated and unencrypted**.
- **PR_Swap / DR_Swap**: Role-swap messages that change power and data direction.
- **Hard Reset / Soft Reset**: Reset messages that force re-negotiation.
- **BIST**: Built-in self-test mode.
- **Alert, Get_Status, Get_Source_Info**: Status query messages.

WattPhantom attacks the following vulnerabilities in this protocol:

| Vulnerability | CVSS-like | Description |
|--------------|-----------|-------------|
| **Unauthenticated PD negotiation** | Critical | Any device on either end of a USB-C cable can claim any power capability. No cryptographic identity verification exists. |
| **Unauthenticated VDM channel** | High | VDMs carry arbitrary vendor data on the CC line with no integrity check. Perfect covert channel. |
| **Mid-contract voltage switching** | Critical | After a contract is accepted, a malicious source can silently raise VBUS above the negotiated level. Targets with inadequate OVP (overvoltage protection) will be damaged. |
| **Power denial / oscillation** | Medium | Repeatedly sending Hard Reset or cycling contracts prevents the target from maintaining a stable power state. |
| **Role swap exploitation** | High | Triggering unexpected PR_Swap on an OTG device can reverse the power/data direction, potentially exposing host-mode vulnerabilities. |
| **Timing side-channel in PD** | Medium | The inter-message timing, PDO selection order, and request timing reveal device identity. |
| **VBUS current profiling** | Medium | Real-time current draw during charging reveals device activity (screen on/off, CPU load, app execution). |

### Threat Model

| Asset | Adversary Capability | WattPhantom Role |
|-------|---------------------|------------------|
| Target device hardware integrity | Physical access to USB-C charging port | Overvoltage attack via spoofed PDO |
| Target device availability | Physical access to charging port | Power DoS via contract oscillation |
| Data exfiltration from locked-down device | Compromised software on target + USB-C access | CC-line VDM covert channel |
| Device identity / tracking | Physical access to charging port (or compromised public charger) | PD fingerprinting |
| USB data confidentiality | Inline position on USB-C cable | USB data MITM while manipulating PD |
| OTG host-mode security | Physical access to OTG-capable target | Role swap attack to trigger host mode |

**Preconditions:**
- **Inline attacks** require physical access to insert WattPhantom between the target and its charger/host. This is possible at public charging stations, hotel USB ports, shared workspaces, or any scenario where the attacker controls the charging infrastructure.
- **Covert channel** requires compromised software on the target that can encode data into PD VDMs via the USB-C controller driver. This is post-exploitation — the covert channel is the exfiltration path, not the initial access.
- **Overvoltage attacks** require a target with inadequate OVP (overvoltage protection). Most modern devices have OVP, but many cheap IoT devices, accessories, and older designs do not.
- **Fingerprinting** can be done passively (inline) without the target detecting anything abnormal.

**Out of scope:**
- WattPhantom does **not** perform USB data injection (use BadUSB-Injector for that).
- It does **not** perform USB DMA attacks (use USB-DMA-Phantom).
- It does **not** perform voltage glitching on target power rails (use Volt-Glitcher).
- It does **not** break USB-C cable authentication (E-marker verification), though it can interact with cable e-markers via SOP' messages.

---

## Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32G474QBT6 (ARM Cortex-M4F, 170 MHz, 128 KB Flash, 32 KB SRAM) |
| **PD Controller** | On-chip UCPD (USB-C Power Delivery) peripheral + FUSB302B external BMC transceivers (×2) |
| **Power Path** | 2× TPS25982 eFuse (5–48V, 5A, programmable OVP/OCP) |
| **VBUS Switching** | 2× SISS740DN 100V N-channel MOSFETs with gate drivers |
| **Current/Voltage Monitor** | INA226 (16-bit, 36V, bi-directional, I²C) on each VBUS path |
| **BLE** | nRF52840 module (u-blox NINA-B302) over UART |
| **Display** | 0.96" OLED (SSD1306, 128×64, I²C) |
| **USB-C Ports** | 2× USB4320-03-C (16-pin, USB-C receptacles) — Source end and Sink end |
| **CC Switching** | 2× TMUX2512 (4:1 analog mux) for CC1/CC2 routing |
| **USB Data Switch** | FSUSB42UMX (USB 2.0 high-speed switch, 480 Mbps) |
| **Battery** | 1000 mAh LiPo (3.7V) with MCP73831 charger + MAX17048 fuel gauge |
| **Power Input** | USB-C (sink side) or internal battery |
| **Form Factor** | 55 × 22 × 12 mm (USB-C dongle/extension form) |
| **PCB** | 6-layer, 0.8mm, controlled impedance for USB 2.0 |
| **Operating Temp** | 0–60°C (commodity components; not rated for automotive) |
| **Weight** | ~25 g (including battery) |

### Key Design Decisions

1. **STM32G474 with built-in UCPD**: The STM32G474 includes a USB-C Power Delivery physical layer (UCPD) that handles BMC encoding/decoding, CRC, and message framing in hardware. This eliminates the need for an external PD controller IC and reduces latency for protocol manipulation. However, we pair it with FUSB302B transceivers for the CC line because the STM32G474's UCPD only supports one CC channel — WattPhantom needs two (one for each USB-C port).

2. **TPS25982 eFuses on VBUS paths**: The eFuses provide hardware-limited overvoltage protection (OVP) and overcurrent protection (OCP) that the operator can configure. This prevents WattPhantom itself from being destroyed by a hostile source, and provides a safety margin when testing overvoltage attacks (the operator can set a ceiling).

3. **INA226 on both VBUS paths**: Real-time current and voltage monitoring on both the source and sink sides enables differential power analysis of the target's charging behavior, contract compliance verification, and covert-channel signal detection (VDM-triggered current fluctuations).

4. **TMUX2512 CC switching**: The analog muxes allow WattPhantom to connect/disconnect, cross-connect, or isolate the CC1 and CC2 lines between the two USB-C ports. This enables true MITM (passing CC through transparently while monitoring), or complete isolation (controlling each port independently).

5. **FSUSB42 USB data switch**: Allows USB 2.0 data to pass through transparently (MITM mode) or be blocked/isolated. The switch supports high-speed (480 Mbps) with minimal signal degradation.

6. **NINA-B302 BLE module**: Provides a secure BLE 5.0 backhaul for operator control. The nRF52840 handles the BLE stack independently, freeing the STM32G474 for deterministic PD protocol timing.

---

## Architecture and Block Diagram

```
                    ┌─────────────────────────────────────────────────────────────┐
                    │                     WATTPHANTOM BOARD                       │
                    │                                                             │
   USB-C SOURCE ───┤──┬──VBUS──┬──[eFuse1]──┬──[INA226-1]──┬──VBUS─┬──────────────┤── USB-C SINK
   (Charger/Host)    │  │       │  TPS25982  │              │       │              │ (Target)
                     │  │       └────────────┘              │       │              │
                     │  │                                   │       │              │
                     │  ├──CC1──[TMUX2512]──────┬───────────┤──CC1──┤              │
                     │  │       (4:1 mux)        │           │       │              │
                     │  ├──CC2──[TMUX2512]──────┤───────────┤──CC2──┤              │
                     │  │       (4:1 mux)        │           │       │              │
                     │  │                        │           │       │              │
                     │  │  ┌──FUSB302B──────┐    │           │  ┌──FUSB302B──────┐ │
                     │  │  │ CC PHY (src)   │    │           │  │ CC PHY (snk)   │ │
                     │  │  │ BMC↔I²C        │    │           │  │ BMC↔I²C        │ │
                     │  │  └───────┬────────┘    │           │  └───────┬────────┘ │
                     │  │          │ I²C         │           │          │ I²C       │
                     │  │  ┌───────▼────────────▼───────────▼──────────▼──────┐   │
                     │  │  │              STM32G474QBT6                        │   │
                     │  │  │         (Cortex-M4F @ 170 MHz)                     │   │
                     │  │  │  ┌──────────────────────────────────────────────┐  │   │
                     │  │  │  │ UCPD Peripheral (HW PD engine)              │  │   │
                     │  │  │  │ - BMC encode/decode (shared with FUSB302B)  │  │   │
                     │  │  │  │ - CRC, framing, GoodCRC auto-reply          │  │   │
                     │  │  │  │ - SOP/SOP'/SOP" message support             │  │   │
                     │  │  │  └──────────────────────────────────────────────┘  │   │
                     │  │  │  ┌────────────┐ ┌────────────┐ ┌────────────────┐ │   │
                     │  │  │  │ PD Engine   │ │ Covert Ch. │ │ Fingerprinter  │ │   │
                     │  │  │  │ (state mach)│ │ (VDM codec)│ │ (PDO profiler) │ │   │
                     │  │  │  └────────────┘ └────────────┘ └────────────────┘ │   │
                     │  │  │  ┌────────────┐ ┌────────────┐ ┌────────────────┐ │   │
                     │  │  │  │ I²C Master  │ │ SPI (OLED) │ │ UART (BLE)     │ │   │
                     │  │  │  └────────────┘ └────────────┘ └───────────────┘ │   │
                     │  │  └───────────────────┬───────────────────────────────┘   │
                     │  │                      │ UART                               │
                     │  │              ┌───────▼────────┐                           │
                     │  │              │ NINA-B302      │                           │
                     │  │              │ (nRF52840 BLE) │──── BLE 5.0 ──── Phone    │
                     │  │              └────────────────┘                           │
                     │  │  ┌────────────┐ ┌────────────┐ ┌────────────────┐        │
                     │  │  │ SSD1306 OLED│ │ MAX17048   │ │ MCP73831       │        │
                     │  │  │ (I²C)       │ │ Fuel Gauge │ │ LiPo Charger   │        │
                     │  │  └────────────┘ └────────────┘ └────────────────┘        │
                     │  │                                                            │
                     │  └──USB D+/D-──[FSUSB42]───────────────────USB D+/D-────────┤
                     │                 (USB 2.0 switch)                            │
                     └─────────────────────────────────────────────────────────────┘
```

### Data Flow

1. **Passive monitoring (MITM)**: CC lines pass through the TMUX2512 muxes transparently. The FUSB302B transceivers tap the CC lines, sending BMC-decoded PD messages to the STM32G474 via I²C. The MCU logs, analyzes, and forwards to BLE. VBUS passes through the eFuse and INA226 uninterrupted. USB data passes through the FSUSB42 switch uninterrupted.

2. **Active PD manipulation**: The TMUX2512 muxes disconnect the CC pass-through, isolating each port. The STM32G474 now controls PD independently on each port — it can advertise different PDOs to the sink than what the source offered, or request different contracts from the source than what the sink needs. VBUS is still connected, but the eFuses and MOSFETs can modulate it.

3. **Overvoltage attack**: The source port negotiates a normal contract (e.g., 5V). The sink port advertises a high-voltage PDO (e.g., 20V). The target requests 20V. WattPhantom raises VBUS to 20V on the sink side while the source side still provides 5V — an internal boost converter (or the source's own capability if it supports 20V) provides the higher voltage. For attacks beyond the source's capability, WattPhantom's internal boost converter (not shown, controlled by the MCU) can generate up to 48V from the 5V input.

4. **Covert channel**: Compromised software on the target encodes data into PD VDMs via the USB-C controller driver. The VDMs travel over the CC line to WattPhantom, which decodes them and forwards the data over BLE to the operator. No USB data connection is needed — the covert channel works over power-only connections.

5. **Fingerprinting**: During passive monitoring, the MCU timestamps every PD message, records PDO selections, measures inter-message delays, and profiles VBUS current draw at 1 kHz. The resulting fingerprint vector is compared against a database of known device profiles.

---

## Firmware Details and Design Decisions

The firmware is written in C for the STM32G474 (Cortex-M4F) and is RTOS-free — a deterministic super-loop design that ensures precise PD protocol timing. The critical timing constraint is the PD specification's tReceiverResponse (15 ms) and tSenderResponse (30 ms) windows; the MCU must respond to PD messages within these windows.

### Firmware Architecture

```
firmware/
├── main.c              — Super-loop: command dispatcher, state machine, BLE/USB poll
├── registers.h         — STM32G474 register map (no CMSIS dependency)
├── board.h             — Pin assignments, peripheral mappings, constants
├── Makefile            — arm-none-eabi-gcc build
└── drivers/
    ├── board_init.c    — Clock, GPIO, I²C, SPI, UART, UCPD peripheral init
    ├── pd_controller.c — PD protocol state machine, PDO management, contract negotiation
    ├── cc_phy.c        — FUSB302B CC line transceiver driver (BMC, CRC, framing)
    ├── vbus_switch.c   — eFuse and MOSFET control, VBUS voltage/current regulation
    ├── current_monitor.c— INA226 I²C driver, real-time power profiling
    ├── covert_channel.c— VDM codec, data encoding/decoding, BLE relay
    ├── fingerprint.c   — PD fingerprinting engine, profile database, matching
    ├── ble_uart.c       — nRF52840 BLE UART bridge
    ├── oled.c          — SSD1306 OLED display driver
    └── usb_cdc.c       — USB CDC virtual serial for debug/log
```

### Key Design Decisions

1. **No RTOS**: The PD protocol has strict timing (15–30 ms response windows). An RTOS introduces scheduler jitter that could cause PD timeouts. The super-loop design with interrupt-driven I²C and UART ensures deterministic response.

2. **FUSB302B as CC PHY**: Although the STM32G474 has a built-in UCPD peripheral, we use the FUSB302B as the external CC transceiver because it supports independent CC1/CC2 monitoring on both ports and has built-in BMC encoding. The MCU communicates with the FUSB302B over I²C at 1 MHz.

3. **PD state machine**: A full PD state machine implementation (Source, Sink, and Dual-Role) with support for SOP, SOP', and SOP" messages. The state machine handles contract negotiation, role swaps, hard/soft resets, VDMs, and BIST.

4. **VDM codec**: The covert channel uses Unstructured VDMs (VDM Header + arbitrary data objects) to encode arbitrary data. The VDM command field and data objects are used as a bi-directional data channel with 24 bits of payload per VDM data object (32-bit object minus 8-bit VDM header overhead). At 600 kbps BMC with 4-byte VDMs, the raw covert channel bandwidth is ~3 KB/s.

5. **Fingerprinting engine**: Captures a feature vector from each PD negotiation: {PDO bitmask, request order, inter-message timing histogram, current profile during PS_RDY, total negotiation time}. A cosine-similarity match against a database of known profiles identifies the device.

6. **Safety interlocks**: The firmware has hardware (eFuse OVP/OCP) and software (VBUS voltage ceiling, current limit, thermal monitor) safety interlocks. The operator must explicitly enable "attack mode" to bypass safety limits, and the device reverts to safe mode on BLE disconnect.

---

## Application/Software Interface

The companion app is a React Native application that provides:

| Screen | Function |
|--------|----------|
| **Connection** | BLE scan and connect to WattPhantom |
| **Dashboard** | Real-time status: source/sink contracts, VBUS voltage/current, CC state, battery |
| **PD Manipulator** | Configure spoofed PDOs, trigger role swaps, hard resets, contract oscillation |
| **Covert Channel** | Monitor CC-line VDM covert channel, view decoded data, configure VDM codec |
| **Fingerprint** | Capture and display device PD fingerprint, match against database |
| **Power Monitor** | Real-time VBUS voltage/current graphs, power profiling |
| **Settings** | Safety limits, BLE config, firmware update, data export |

The app communicates with WattPhantom over a BLE UART service (Nordic UART Service-style transparent serial). Commands are ASCII text with binary attachments for bulk data.

### Command Protocol

```
STATUS                          → "OK src=5V/3A snk=9V/2A cc=CC1 vbus=5.02V/0.31A batt=87"
SET SRC_PDO <index>             → Select source PDO to advertise to sink
SET SNK_PDO <index>             → Select PDO to request from source
SET VBUS <voltage_mv> <current_ma> → Force VBUS to specific voltage/current
HARD_RESET <port>               → Send PD Hard Reset on src or snk port
PR_SWAP                        → Trigger Power Role Swap
DR_SWAP                        → Trigger Data Role Swap
CONTRACT_OSCILLATE <period_ms>  → Oscillate power contract every N ms
COVERT_TX <hex_data>            → Send data over CC-line VDM covert channel
COVERT_RX_START                 → Begin monitoring for incoming VDM data
FINGERPRINT_CAPTURE             → Capture PD fingerprint of connected target
FINGERPRINT_MATCH               → Match captured fingerprint against database
POWER_PROFILE_START <duration_s>→ Begin VBUS power profiling
POWER_PROFILE_STOP              → Stop profiling, return data
SET_SAFETY <ovp_mv> <ocp_ma>    → Configure safety limits
ATTACK_MODE <on|off>            → Enable/disable attack mode (bypasses safety defaults)
```

---

## Use Cases

### For Red Teams

1. **Malicious charging station emulation**: Deploy WattPhantom as a "free charging station" at a target location. When a victim plugs in, WattPhantom fingerprints their device, profiles their charging behavior, and optionally deploys overvoltage or power DoS attacks.

2. **Covert data exfiltration**: After compromising a target device (e.g., via a phishing payload that installs a VDM encoder driver), use the CC-line covert channel to exfiltrate data through the charging port. This bypasses USB data monitoring, network DLP, and even air-gaps (the data exits via the power cable).

3. **Device identification and tracking**: At a physical penetration test site, use WattPhantom to fingerprint all USB-C devices found at desks, conference rooms, and charging stations. Match fingerprints against a database to identify device models, detect company-issued vs. personal devices, and track individuals.

4. **Power-based social engineering**: Use power DoS to force a target device to drain its battery during a critical moment (e.g., during a presentation, before a security check), creating an opportunity for physical access.

5. **Inline USB MITM**: Insert WattPhantom between a target and its trusted workstation. The target sees its normal charger; WattPhantom intercepts USB data while independently controlling PD.

### For Security Researchers

1. **PD protocol fuzzing**: Systematically generate malformed PD messages to test the robustness of target devices' PD controllers. Discover parsing bugs, buffer overflows, and state machine inconsistencies.

2. **OVP testing**: Verify that a device under test properly rejects overvoltage conditions. WattPhantom can sweep VBUS from 5V to 48V and log which devices survive and which fail.

3. **Charging behavior analysis**: Profile how different devices negotiate PD contracts under various conditions (low battery, full battery, screen on/off, CPU load). This data is valuable for power management research and device characterization.

4. **Covert channel characterization**: Measure the bandwidth, latency, and detectability of the CC-line VDM covert channel across different target devices and operating systems.

5. **Cable authentication research**: Interact with USB-C cable e-markers via SOP' messages to study cable authentication mechanisms and identify counterfeit cables.

### For Penetration Testers

1. **Physical access scenario**: During a physical pentest, replace a target's charger with a WattPhantom-equipped charging station. Gain device fingerprints, power profiles, and potentially exfiltrate data via the covert channel.

2. **IoT device testing**: Many IoT devices use USB-C for power. Test their PD implementation for vulnerabilities: do they accept non-compliant voltages? Do they handle Hard Reset gracefully? Can their charging ICs be exploited?

3. **Supply chain verification**: Use WattPhantom to verify that USB-C chargers and devices meet their claimed PD specifications. Detect counterfeit chargers that advertise capabilities they cannot deliver.

4. **Compliance testing**: Verify that a product's USB-C implementation complies with USB PD 3.1 and IEC 62680-1-3 specifications. Identify non-compliant behavior that could cause interoperability issues or safety hazards.

---

## Bill of Materials (Summary)

| Ref | Part | Manufacturer | MPN | Qty | Purpose |
|-----|------|--------------|-----|-----|---------|
| U1 | STM32G474QBT6 | STMicroelectronics | STM32G474QBT6 | 1 | Main MCU (Cortex-M4F, UCPD) |
| U2 | FUSB302BMPX | onsemi | FUSB302BMPX | 2 | CC line BMC transceiver (one per port) |
| U3 | TPS25982ARPQ | Texas Instruments | TPS25982ARPQ | 2 | VBUS eFuse (OVP/OCP) |
| U4 | INA226AIDGSR | Texas Instruments | INA226AIDGSR | 2 | VBUS voltage/current monitor |
| U5 | NINA-B302 | u-blox | NINA-B302-00B | 1 | BLE 5.0 module (nRF52840) |
| U6 | SSD1306 | Solomon Systek | SSD1306 | 1 | 0.96" OLED display |
| U7 | TMUX2512 | Texas Instruments | TMUX2512RUMR | 2 | CC line 4:1 analog mux |
| U8 | FSUSB42UMX | onsemi | FSUSB42UMX | 1 | USB 2.0 high-speed switch |
| U9 | MAX17048G+T10 | Maxim/Analog | MAX17048G+T10 | 1 | LiPo fuel gauge |
| U10 | MCP73831T-2ACI/OT | Microchip | MCP73831T-2ACI/OT | 1 | LiPo charger |
| Q1 | SISS740DN-T1-GE3 | Vishay | SISS740DN-T1-GE3 | 2 | VBUS N-channel MOSFET |
| J1 | USB4320-03-C | Amphenol | USB4320-03-C | 2 | USB-C 16-pin receptacle |
| BT1 | LiPo 1000mAh | — | — | 1 | 3.7V 1000mAh battery |

---

## Legal & Ethical Notice

**Author: jayis1**

This design is provided for **authorized security research and education only**. The USB-C Power Delivery protocol vulnerabilities documented here are known to the USB-IF and have been discussed in academic and industry security conferences. WattPhantom is a testing tool for red teams and security researchers to verify that their devices and infrastructure are resilient to PD-based attacks.

**You must not use WattPhantom to:**
- Damage or destroy equipment you do not own
- Exfiltrate data from systems you are not authorized to test
- Intercept communications without legal authorization
- Deploy malicious charging stations in public spaces
- Circumvent product safety features on consumer devices

**You must:**
- Obtain written authorization before testing any device you do not own
- Comply with all applicable laws (CFAA, ECPA, product safety, etc.)
- Respect USB-IF specifications and safety standards
- Report vulnerabilities to manufacturers responsibly

The author (jayis1) provides this design "as is" without warranty and assumes no liability for misuse.

---

*WattPhantom v1.0 — Author: jayis1 — License: Hardware CERN-OHL-S v2 · Firmware GPL-2.0 · App MIT*