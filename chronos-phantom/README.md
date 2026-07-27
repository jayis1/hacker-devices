# Chronos-Phantom — IEEE 1588 PTP / NTP Time-Synchronization Attack Platform

```
   ╔══════════════════════════════════════════════════════════════════════╗
   ║  ██████╗██╗  ████████╗   ██╗ ██████╗ ██╗   ██╗███████╗███████╗███████╗  ║
   ║  ██╔════╝██║ ██╔══════╝  ███║██╔═══██╗██║   ██║██╔════╝██╔════╝██╔════╝  ║
   ║  ██║     ██║ ██║  ███╗   ╚██║██║   ██║██║   ██║███████╗███████╗█████╗    ║
   ║  ██║     ██║ ██║   ██║   ██╔╝██║   ██║██║   ██║╚════██║╚════██║██╔══╝    ║
   ║  ╚██████╗██║ ╚██████╔╝  ██╔╝╚██████╔╝╚██████╔╝███████║███████║██║      ║
   ║   ╚═════╝╚═╝  ╚═════╝  ╚═╝  ╚═════╝  ╚═════╝ ╚══════╝╚══════╝╚═╝      ║
   ║  Precision Time-Synchronization Spoofing, Skew & Desync Implant          ║
   ╚══════════════════════════════════════════════════════════════════════╝
```

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange) ![HW-License](https://img.shields.io/badge/hardware-CERN--OHL--S%20v2-red)

> **Author:** jayis1
> **License:** GPL-2.0 (firmware & app) / CERN-OHL-S v2 (hardware)
> **Status:** Complete research hardware design — firmware + KiCad PCB + companion app

---

## ⚠️ LEGAL & ETHICAL DISCLAIMER

This device, firmware, and companion application are designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on systems you own or have explicit permission to assess. Unauthorized manipulation of time-synchronization infrastructure (PTP grandmasters, NTP stratum servers, GNSS-disciplined clocks) in networks you do not own may violate computer-fraud and abuse statutes (e.g., 18 U.S.C. § 1030 CFAA), wiretap statutes (18 U.S.C. § 2511), critical-infrastructure protection laws (e.g., the U.S. Critical Infrastructure Protection Act, NERC CIP standards), and financial-services regulations (FINRA, MiFID II timestamping requirements, SEC Rule 613 — Consolidated Audit Trail).

**Manipulating time in industrial, financial, or critical-infrastructure environments can cause physical damage, data corruption, and safety-system malfunction.** Desynchronizing power-grid protection relays (which use IEEE C37.238 PTP for sample alignment) can cause false trips or failure to trip during faults, risking equipment damage and blackouts. Manipulating financial-trade timestamps can trigger regulatory violations and market-integrity investigations. **The author (jayis1) assumes no liability for misuse, property damage, financial loss, or personal injury.** Always obtain proper written authorization before deployment, and never deploy on operational power-grid timing, live financial-trading infrastructure, or any safety-critical distributed system. This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**Chronos-Phantom** is a portable, pocket-sized network implant that targets **precision time-synchronization protocols** — primarily **IEEE 1588 Precision Time Protocol (PTP)** and **Network Time Protocol (NTP)** — to spoof, skew, desynchronize, or stealthily monitor the clocks of distributed systems. It is the first device in this repository to attack the **time layer** of networked infrastructure, an attack surface that is ubiquitous, critically safety-relevant in industrial environments, and almost entirely absent from published red-team tooling.

Unlike existing devices that target Ethernet traffic generically (axle-tap, shadow-tap), CAN bus (can-creeper), GNSS timing (gnss-phantom), or wireless radios (ble-phantom, lora-phantom, zigbee-phantom), **Chronos-Phantom specifically attacks the network time-synchronization layer** — the protocols that make distributed systems agree on "what time it is" and that underpin sample alignment in power grids, timestamp integrity in financial trading, log correlation in security systems, and Kerberos/NTLM authentication validity.

### Why Time Synchronization?

Modern distributed systems depend on accurate time in ways that are invisible until they break:

- **Power-grid protection** — Phasor Measurement Units (PMUs) and Sampled Values (IEC 61850-9-2LE) rely on IEEE C37.238 PTP profile with sub-microsecond alignment. A 1 ms skew can cause differential protection to misoperate, tripping breakers spuriously or failing during faults.
- **Financial trading** — MiFID II, SEC Rule 613, and FINRA require microsecond-accurate timestamps. Clock manipulation can create "future-dated" trades, break audit trails, or enable front-running schemes.
- **Authentication** — Kerberos tickets, TOTP/HOTP tokens, and NTLM session timestamps have tight tolerance windows (typically 5 minutes for Kerberos, 30 seconds for TOTP). A controlled time skew can extend or invalidate authentication.
- **Log integrity** — Security SIEM correlation, forensic timelines, and legal evidence all depend on synchronized clocks. Desynchronization breaks event correlation and creates "evidence gaps."
- **Industrial control** — PLCs, SCADA gateways, and distributed I/O use PTP for coordinated actuation. Time skew causes process instability, sequence-of-events recording errors, and inter-controller handshake failures.
- **Distributed databases** — Cassandra, Spanner, CockroachDB, and Cockroach-style systems use time for consistency. Clock skew violates linearizability, causes write conflicts, and corrupts MVCC snapshots.

An attacker who can manipulate time gains:

- **Silent persistence** — time manipulation is rarely monitored; most NIDS/IPS do not inspect PTP frames.
- **Authentication bypass** — extend Kerberos ticket lifetime by skewing the KDC's clock forward, or replay expired tickets by skewing the target backward.
- **Forensic disruption** — desynchronize log timestamps to break incident-reconstruction timelines.
- **Safety-system manipulation** — cause protection relays to misoperate by breaking sample alignment.
- **Covert channel** — encode data in PTP `correctionField` or NTP `root dispersion` bits, exfiltrating through time-protocol frames that pass firewalls unchallenged.

---

## 2. Attack Surface & Threat Model

### Attack Surface

| Surface | Protocol | Transport | Prevalence |
|---------|----------|-----------|------------|
| Industrial PTP | IEEE 1588v2/v3 (C37.238, IEC 61588) | Multicast UDP / L2 Ethernet | Power substations, factories, smart grid |
| Telecom PTP | ITU-T G.8275.1/.2 PTP telecom profiles | L2 / L3 Ethernet | 5G fronthaul, mobile backhaul |
| Financial PTP | PTPv2 with hardware timestamping | Unicast / multicast Ethernet | HFT exchanges, trading floors |
| Enterprise NTP | NTPv3/v4 (stratum 1-15) | UDP port 123 | Universal in enterprise IT |
| Secure NTP | NTS (Network Time Security, RFC 8915) | TLS + UDP | Emerging in critical infrastructure |
| White Rabbit | CERN WR PTP extension | Synchronous Ethernet | Scientific, particle accelerators |
| PTP over 802.1AS | Audio/Video bridging (gPTP) | Ethernet L2 | Pro AV, automotive Ethernet |

### Threat Model

**Adversary capability:** Network access to the PTP/NTP domain (either inline tap insertion or VLAN/segment access). For inline operation, Chronos-Phantom is a drop-in Ethernet pass-through (like axle-tap/shadow-tap) that bridges the link while modifying PTP/NTP frames in flight. For non-inline operation, it acts as a rogue PTP Grandmaster or NTP server on the segment.

**Adversary goals:**
1. **Grandmaster spoofing** — Announce as PTP Grandmaster (BMCA winner) with higher `priority1` / `clockClass` than the legitimate GM, causing all slaves to lock to Chronos-Phantom's clock.
2. **Time skew injection** — Inject a controlled offset (e.g., +5 minutes) into PTP `correctionField` or NTP `transmitTimestamp`, shifting slaves' clocks gradually or suddenly.
3. **Slow-drift attack** — Introduce sub-microsecond-per-second drift to slowly desynchronize PMUs past the 1 ms protection-relay threshold over minutes-to-hours, evading abrupt-change detection.
4. **Desynchronization denial-of-service** — Send alternating `Announce` messages with conflicting GM identities to keep slaves in an unstable BMCA election state, preventing lock.
5. **Kerberos/NTLM window manipulation** — Skew the KDC clock forward by 25 minutes to extend ticket validity, or backward to allow replay of expired tickets.
6. **PTP frame capture** — Passively capture all PTP frames on the segment to map the timing topology, identify GMs, slaves, and transparent clocks.
7. **NTP amplification** — Act as a rogue NTP server with monlist-style amplification to reflect DDoS traffic (legacy attack — included for completeness).
8. **Covert exfiltration** — Encode exfiltrated data in PTP `correctionField` (64-bit, slaves ignore large values as "path asymmetry") or NTP `root delay`/`root dispersion` extension fields.
9. **White Rabbit attack** — Target the PTP-over-SyncE extension used in scientific and some telecom networks by manipulating the sync Ethernet frequency reference.

**Defender limitations:**
- PTP is unauthenticated by default; the optional `TLV: AUTHENTICATION` is almost never deployed.
- BMCA (Best Master Clock Algorithm) is a "trust the loudest" protocol — highest priority wins, no verification.
- NTP authentication (symmetric keys, autokey) is widely unused; NTS is emerging but not deployed in most enterprises.
- Most NIDS/IPS do not inspect PTP frames (EtherType 0x88F7) — they are treated as unknown Layer 2 traffic.
- PTP hardware timestamping in PHYs trusts the GM implicitly; the PHY does not validate GM identity.
- Time-skew detection requires external reference (GNSS or independent NTP stratum-1) that is often absent in isolated industrial networks.
- Gradual drift (sub-ppm) is indistinguishable from legitimate oscillator drift without a second independent reference.

---

## 3. Hardware Specifications

### MCU & Processing

| Component | Part | Role |
|-----------|------|------|
| Main MCU | STM32H753 (ARM Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB RAM) | PTP frame processing, BMCA engine, NTP responder, skew/drift math, BLE control |
| PTP PHY | Microchip KSZ9131 (10/100/1000BASE-T with IEEE 1588v2 hardware timestamping) | Hardware timestamping of PTP frames at the MII/RGMII boundary |
| BLE/Comms | ESP32-C3 (RISC-V, BLE 5.0) | Operator control link, companion app interface |
| Local clock | Morion MV89 TCXO (10 MHz, ±0.5 ppb stability) | Stable local reference for skew/drift generation; disciplinable via 1-PPS input |
| GNSS input (optional) | u-blox NEO-M9N (1-PPS + NMEA) | External time reference for holdover, legitimacy, or "trusted GM" impersonation |
| Power management | TPS63031 buck-boost (3.3 V rail from 1.8-5.5 V input) | PoE pass-through or USB-C power |
| PoE pass-through | Silicon Labs Si3402 (802.3af PoE PD) | Allows inline insertion on PoE-powered links (industrial PTP often runs PoE) |

### Radios & Connectivity

| Radio | Standard | Range | Purpose |
|-------|----------|-------|---------|
| BLE 5.0 | Bluetooth Low Energy | ~30 m | Operator control from companion app |
| Ethernet (×2) | 10/100/1000BASE-T | Wired | Inline bridge (target ↔ network) with PTP hardware timestamping |
| GNSS | GPS/Galileo/GLONASS | Global | Optional external time reference |
| USB-C | USB 2.0 | Wired | Configuration, power, firmware updates |

### Sensors & Indicators

| Sensor/Indicator | Part | Function |
|------------------|------|----------|
| Status LEDs | 4× 0603 SMD | Link A, Link B, PTP lock, BLE active |
| IMU | LSM6DSO (6-axis) | Tamper detection — detect device movement/physical discovery |
| Temperature | Internal MCU die sensor | TCXO temperature compensation |
| Push button | Tactile SMD | Mode selection / BLE pairing |

### Power

| Parameter | Value |
|-----------|-------|
| Input voltage | 5 V USB-C or 48 V PoE |
| Operating current | ~250 mA @ 3.3 V (active PTP) |
| Standby current | ~15 mA (BLE-only, bridges pass-through) |
| Battery | 500 mAh LiPo (backup, ~4 hours untethered) |
| Power source priority | PoE > USB-C > LiPo backup |

### Form Factor

| Dimension | Value |
|-----------|-------|
| PCB size | 65 × 30 mm (4-layer, 1.6 mm) |
| Enclosure | Anodized aluminum, 70 × 35 × 12 mm |
| Weight | ~35 g (with battery) |
| Ethernet connectors | 2× RJ45 (magnetics integrated) |
| Connector | USB-C (config), SMA (1-PPS/GNSS external) |

### Block Diagram

```
 ┌──────────────────────────────────────────────────────────────────┐
 │                        CHRONOS-PHANTOM                            │
 │                                                                  │
 │  ┌─────────┐    RGMII     ┌───────────────┐    RGMII   ┌────────┐│
 │  │  RJ45   │◄────────────►│   KSZ9131     │◄──────────►│  RJ45  ││
 │  │ PORT A  │  (PTP TS)    │  PTP PHY #1   │  (PTP TS)  │ PORT B ││
 │  │ (host)  │              │  + TS engine  │            │(net)   ││
 │  └────┬────┘              └───────┬───────┘            └────────┘│
 │       │ RMII                     │ RMII                          │
 │       │              ┌───────────┴────────┐                       │
 │       │              │   ETH MUX / Bridge  │                       │
 │       │              │   (in FPGA or MCU)  │                       │
 │       └─────────────►│  L2 frame inspector │◄─────────────────────┘
 │                       └─────────┬──────────┘
 │                                 │
 │                       ┌─────────▼──────────┐
 │                       │    STM32H753       │
 │                       │  Cortex-M7 @480MHz │
 │                       │                    │
 │                       │  • PTP BMCA engine │
 │                       │  • NTP responder   │
 │                       │  • Skew/drift math  │
 │                       │  • Frame mod engine │
 │                       │  • Covert codec     │
 │                       └─────┬────────┬─────┘
 │                             │ SPI    │ UART
 │                       ┌─────▼───┐ ┌──▼───────┐
 │                       │  ESP32  │ │  MV89   │
 │                       │  -C3    │ │  TCXO   │
 │                       │  BLE   │ │ 10MHz   │
 │                       └────┬────┘ └────┬───┘
 │                            │           │ 1PPS
 │                       ┌────▼────┐ ┌───▼──────┐
 │                       │ USB-C   │ │ NEO-M9N  │
 │                       │ config  │ │  GNSS    │
 │                       └─────────┘ └──────────┘
 └──────────────────────────────────────────────────────────────────┘
```

---

## 4. Architecture

### System Architecture

Chronos-Phantom operates in three principal modes, selectable from the companion app or via a long-press of the mode button:

1. **Inline MITM Mode** — The two RJ45 ports form a transparent L2 bridge. PTP/NTP frames passing through are intercepted, timestamped, optionally modified, and re-injected. This mode requires physical insertion into the target link (like axle-tap/shadow-tap) but gives full frame-level control. Non-PTP traffic passes through unmodified with sub-microsecond added latency (cut-through via ETH MUX).

2. **Rogue Grandmaster Mode** — Chronos-Phantom connects to the target network via Port B only and announces itself as a PTP Grandmaster with configurable `priority1`, `clockClass`, `clockAccuracy`, and `clockVariance`. By setting `priority1=0` and `clockClass=6` (primary reference clock), it wins the BMCA election against most legitimate GMs (which typically use `priority1=128`, `clockClass=248`). All slaves on the segment then lock to Chronos-Phantom's clock, which can be freely skewed.

3. **Passive Sniffer Mode** — Both ports listen (Port A in promiscuous mode, no inline insertion required if connected via a SPAN/mirror port or passive tap). All PTP/NTP frames are captured, timestamped, and logged for topology mapping and reconnaissance.

### PTP Frame Processing Pipeline

```
  ┌──────────┐    ┌──────────┐    ┌─────────────┐    ┌──────────────┐    ┌──────────┐
  │ RX frame │───►│ Eth hdr  │───►│ PTP header  │───►│ Field modify │───►│ TX frame │
  │ (PHY TS) │    │ classify │    │ parse/TS    │    │ (skew/covert)│    │ (PHY TS) │
  └──────────┘    └──────────┘    └─────────────┘    └──────────────┘    └──────────┘
                        │              │                     │
                        ▼              ▼                     ▼
                   Drop non-PTP?   Sync/Delay_Req        correctionField
                   (pass-through)  Announce/Resp          adjust
                                   Follow_Up/Pdelay
```

### BMCA Election Manipulation

The Best Master Clock Algorithm (defined in IEEE 1588-2008 §9.2.2) elects the Grandmaster by comparing, in order:

1. `priority1` (lower wins)
2. `clockClass` (lower wins)
3. `clockAccuracy` (lower wins)
4. `clockVariance` (lower wins; more negative = better)
5. `priority2` (lower wins)
6. `clockIdentity` (numerically lower wins)

Chronos-Phantom's BMCA engine crafts `Announce` messages that win against any observed legitimate GM by:
- Observing the current GM's `Announce` fields (passive sniff before attack).
- Setting `priority1` to one less than the observed GM (or 0 for guaranteed win).
- Setting `clockClass=6` (claims primary reference — GNSS/atomic).
- Setting `clockAccuracy=0x20` (sub-nanosecond, matching GNSS-disciplined clocks).
- Setting `clockVariance=0x8000` (very stable).

### Skew & Drift Generation

Chronos-Phantom can apply time manipulation in several profiles:

| Profile | Offset | Rate | Detection evasion | Use case |
|---------|--------|------|-------------------|----------|
| **Step** | Instantaneous (e.g., +5 min) | N/A | None — easily detected | Kerberos replay |
| **Ramp** | Linear over time (e.g., +1 ms/s) | Constant | Mild — detected by rate-of-change monitors | Slow desync |
| **Stealth** | Sub-ppm drift (e.g., 0.1 ppm) | Very low | Strong — indistinguishable from oscillator drift | PMU desync over hours |
| **Jitter** | Pseudo-random ±100 µs | Variable | Moderate — looks like network jitter | Disrupt phase-sensitive apps |
| **Sawtooth** | Periodic ramp-and-reset | Cyclic | Moderate — creates periodic alignment errors | PMU differential trip |

The skew is applied by modifying the `correctionField` (64-bit signed ns) in `Sync`/`Follow_Up` messages, or by adjusting the `originTimestamp`/`receiveTimestamp` in NTP response frames. For hardware-timestamped PTP, the `correctionField` is the correct place — slaves add it to the GM timestamp, so a positive `correctionField` shifts the perceived time forward.

### Covert Channel Codec

Chronos-Phantom implements a covert data channel over PTP/NTP frames:

- **PTP correctionField encoding** — The `correctionField` is nominally used for transparent-clock path-delay correction (residence time). Slaves add it to timestamps. Values up to ±1 s are plausible for large networks. Chronos-Phantom encodes 8 bits per frame in the low bits of `correctionField`, with the high bits kept plausible. A receiving Chronos-Phantom (or a software agent) decodes the stream. Throughput: ~8 bits per PTP frame, ~1 frame/sec → 8 bps (enough for key exfiltration).
- **NTP root delay/dispersion** — The 32-bit `rootDelay` and `rootDispersion` fields in NTP response headers are rarely inspected by clients (they affect reachability but not time computation). 16 bits/frame can be encoded here. Throughput: ~16 bps at 1 response/sec.

---

## 5. Firmware Details & Design Decisions

### Firmware Architecture

The firmware runs on the STM32H753 (Cortex-M7) and is organized into:

- **`main.c`** — System initialization, main loop, mode dispatcher.
- **`board.h`** — Pin assignments, peripheral mapping, board configuration.
- **`registers.h`** — STM32H753 register definitions (RCC, ETH, SPI, USART, GPIO, PTP auxiliary snapshot).
- **`ptp_engine.c/h`** — PTP frame parser, BMCA engine, grandmaster spoofing, skew injection, correctionField manipulation.
- **`ntp_engine.c/h`** — NTPv3/v4 responder, skew injection, root-delay/dispersion covert encoding.
- **`eth_bridge.c/h`** — L2 transparent bridge (inline mode), cut-through forwarding, PTP frame interception.
- **`tcxo_drvr.c/h`** — TCXO frequency control (DAC-tuned), 1-PPS disciplining loop (if GNSS present).
- **`skew_gen.c/h`** — Skew profile generator (step, ramp, stealth, jitter, sawtooth).
- **`covert_codec.c/h`** — Covert channel encode/decode over PTP/NTP fields.
- **`ble_link.c/h`** — UART link to ESP32-C3 for app communication, command dispatch.
- **`imux_tamper.c/h`** — IMU-based tamper detection, zeroize on physical disturbance.
- **`Makefile`** — ARM GCC toolchain build.

### Design Decisions

1. **STM32H753 over a dual-MCU design** — The H7's Cortex-M7 @ 480 MHz with hardware FPU is fast enough for full PTP frame processing in software (the PTP header is only 34 bytes), and its on-chip Ethernet MAC with PTP timestamping support eliminates the need for an external FPGA. The KSZ9131 PHY provides the PHY-layer timestamping with ±1 ns resolution.

2. **Hardware timestamping in PHY, not MAC** — The KSZ9131 performs timestamping at the MII/RGMII boundary, which is more accurate than MAC-level timestamping (avoids MAC queue latency jitter). The H7's ETH peripheral captures the PTP timestamp via auxiliary snapshot triggers.

3. **TCXO with DAC tuning** — The MV89 is a voltage-controlled TCXO (VC-TCXO). The MCU drives a DAC to fine-tune its frequency, enabling sub-ppm drift injection without external hardware. When GNSS 1-PPS is available, a software PLL disciplines the TCXO, giving a "legitimate" reference for grandmaster spoofing that passes slave-side sanity checks.

4. **ESP32-C3 for BLE, not the H7 directly** — The STM32H7 has no BLE radio. The ESP32-C3 is a minimal RISC-V BLE 5.0 SoC that communicates with the H7 over UART at 2 Mbps. It runs a simple GATT server for app control, keeping all security logic on the H7.

5. **Cut-through bridge** — In inline mode, non-PTP frames are forwarded cut-through (the frame begins retransmission before it is fully received), keeping added latency under 500 ns. Only PTP frames (EtherType 0x88F7) and NTP frames (UDP 123) are fully buffered for inspection/modification.

6. **Tamper-response** — The IMU detects physical movement. If the device is picked up, rotated, or jarred (configurable threshold), firmware zeros the key material, clears the covert-channel buffers, and reverts to a benign "transparent bridge" mode. This prevents analysis if discovered.

7. **No persistent storage of captured frames** — Captured PTP/NTP frames are kept in RAM only, streamed over BLE to the app. If the device loses power or is tampered with, no captured data remains. This is a deliberate anti-forensic design choice for red-team use.

---

## 6. Application / Software Interface

The companion app (React Native, iOS/Android) provides:

### Screens

1. **Dashboard** — Device status (mode, link A/B, PTP lock, GNSS fix, battery, temperature).
2. **Topology View** — Live PTP topology map: Grandmaster, Transparent Clocks, Ordinary/Slave clocks, with their `clockClass`, `accuracy`, `priority`, `offsetFromMaster`.
3. **Attack Control** — Select attack mode (Inline MITM, Rogue GM, Passive), configure skew profile (step/ramp/stealth/jitter/sawtooth), set offset value, start/stop.
4. **BMCA Spoofer** — Observe current GM, set spoofed `priority1`, `clockClass`, `accuracy`, `variance`, `priority2`, `identity`. "Win BMCA" button.
5. **Covert Channel** — Send/receive covert-encoded data over PTP/NTP. Shows byte-level hex stream and decoded ASCII.
6. **Capture Log** — Live stream of captured PTP/NTP frames with timestamp, source/dest MAC, message type, correctionField, and offset.
7. **Settings** — BLE pairing, tamper sensitivity, GNSS disciplining on/off, firmware update.

### App-Device Protocol

The app communicates over BLE GATT with a custom service (UUID `6e400001-b5a3-f393-e0a9-e50e24dcca9e`, Nordic UART Service convention):

- TX characteristic (app → device): command packets (binary, length-prefixed).
- RX characteristic (device → app): response/event packets.

Commands include: `CMD_GET_STATUS`, `CMD_SET_MODE`, `CMD_SET_SKEW_PROFILE`, `CMD_BMCA_SPOOF`, `CMD_COVERT_SEND`, `CMD_COVERT_RECV_START`, `CMD_CAPTURE_START`, `CMD_CAPTURE_STOP`, `CMD_GNSS_DISCIPLINE`, `CMD_TAMPER_THRESHOLD`, `CMD_FIRMWARE_UPDATE`.

---

## 7. Use Cases

### For Red Teams

1. **Kerberos ticket extension** — Deploy Chronos-Phantom inline on the Domain Controller's network segment. Skew the DC's clock forward by 10 minutes. Now Kerberos tickets issued with a 10-minute lifetime are valid for 20 minutes of real time, extending the window for lateral movement with stolen TGTs. Reverse the skew when done to avoid detection.

2. **Log timeline disruption** — During an engagement, desynchronize SIEM collector clocks by ±30 seconds. This breaks event correlation in the SIEM, creating gaps in the attack timeline and making forensic reconstruction harder for the defenders.

3. **Covert C2 channel** — Encode C2 traffic in PTP `correctionField` values. The frames pass through network devices that have no PTP inspection capability. A software agent on a compromised host reads the local PTP slave's observed `correctionField` (via the PTP management interface) to receive C2 commands — a fully network-based, time-protocol-hidden channel.

### For Security Researchers

4. **PTP authentication research** — Study the BMCA election behavior with adversarial `Announce` messages. Test how slaves react to conflicting GMs, `Announce` rate changes, and `correctionField` overflow.

5. **Time-skew tolerance mapping** — Map how much skew various applications tolerate before failing: Kerberos (typically ±5 min), TOTP (±30 sec), PMU differential protection (±1 ms), NTP `tock`/`tock` interleave, distributed database linearizability.

6. **NTPv4 / NTS fuzzing** — Use Chronos-Phantom as a rogue NTP server to fuzz NTP clients with malformed response fields, testing client-side parsing robustness (buffer overflows in legacy `ntpd`, `chrony`, `w32time`).

7. **Industrial timing security assessment** — In a lab ICS environment, assess whether protection relays detect PTP GM spoofing. Many relays have no GM authentication, so this test reveals a critical gap.

### For Penetration Testers

8. **Network timing posture assessment** — Deploy Chronos-Phantom in passive mode to map the PTP topology, identify the GM, and check whether PTP authentication TLVs are present. Report the absence of PTP authentication as a finding.

9. **NTP amplification check** — Test whether the client's NTP servers respond to `monlist` requests (CVE-2013-5211 amplification), by sending a `MODE=7` request from Port B and measuring response size.

10. **Time-based authentication bypass testing** — Test whether the target's TOTP/HOTP implementation accepts codes outside the ±1 step window when the clock is skewed. Some implementations have a "grace window" that an attacker can exploit.

---

## 8. Comparison to Existing Devices

| Existing device | What it attacks | Chronos-Phantom difference |
|-----------------|-----------------|---------------------------|
| gnss-phantom | GNSS/GPS RF timing | Chronos-Phantom attacks the *network* time layer (PTP/NTP), not RF. Complementary — GNSS spoofing affects PTP slaves that use GNSS as GM reference; Chronos-Phantom attacks the PTP protocol directly. |
| axle-tap / shadow-tap | Generic Ethernet tap | Those capture/forward all Ethernet. Chronos-Phantom is PTP/NTP-protocol-aware — it parses, modifies, and generates PTP frames, not just mirrors L2 traffic. |
| can-creeper | CAN bus | Different bus entirely. PTP runs over Ethernet. |
| badusb-injector / forge-probe | USB / debug ports | Time protocols run over Ethernet, not USB. |
| silent-symphony | Acoustic side channel | Different physical layer. |

Chronos-Phantom is the **only device in the repository that attacks time-synchronization protocols** — a distinct, critical, and under-explored attack surface.

---

## 9. Bill of Materials (Summary)

| Ref | Part | Package | Qty | Notes |
|-----|------|---------|-----|-------|
| U1 | STM32H753VIT6 | LQFP-100 | 1 | Main MCU |
| U2 | KSZ9131RNI | QFN-48 | 1 | PTP PHY |
| U3 | ESP32-C3FH4 | QFN-32 | 1 | BLE |
| U4 | MV89 (VC-TCXO) | DIP-14 | 1 | 10 MHz ±0.5 ppb |
| U5 | NEO-M9N | LCC-36 | 1 | GNSS (optional) |
| U6 | TPS63031 | SON-10 | 1 | Buck-boost |
| U7 | Si3402-B | QFN-32 | 1 | PoE PD |
| U8 | LSM6DSO | LGA-14 | 1 | IMU |
| J1, J2 | RJ45 magjacks | SMD | 2 | Ethernet ports |
| J3 | USB-C 16-pin | SMD | 1 | Config/power |
| J4 | SMA | PCB edge | 1 | 1-PPS/GNSS |
| BT1 | 502030 LiPo | Wire | 1 | 500 mAh |

Full BOM in `kicad/chronos-phantom.csv`.

---

## 10. Limitations & Future Work

### Current Limitations

- **Single-segment attack** — Chronos-Phantom attacks one PTP domain (one VLAN/segment). Multi-segment attacks require multiple devices or a compromised switch.
- **No White Rabbit support** — White Rabbit requires Synchronous Ethernet (physical-layer frequency sync) and is not yet implemented. Future firmware could add SyncE support via the KSZ9131's clock output.
- **PTP authentication not bypassed** — If the target deploys PTP authentication TLVs (rare), Chronos-Phantom cannot forge authenticated `Announce` messages without the key. However, it can still capture and replay authenticated frames (replay attack).
- **BLE range** — 30 m BLE range may be limiting in large industrial environments. A future version could add a sub-GHz radio (like lora-phantom) for long-range control.

### Future Work

- **White Rabbit / SyncE** — Add Synchronous Ethernet support to attack WR networks.
- **PTP authentication key extraction** — Side-channel attack on the GM to extract the PTP authentication key (combine with sideprobe-style power analysis).
- **Multi-port transparent clock emulation** — Emulate a PTP Transparent Clock to modify residence times across multi-hop paths.
- **gPTP (802.1AS) support** — Add Automotive Ethernet / Pro AV timing attack capability.

---

## 11. Repository Structure

```
chronos-phantom/
├── README.md                  ← this file
├── firmware/
│   ├── main.c                  ← system init, main loop, mode dispatcher
│   ├── board.h                 ← pin/peripheral assignments
│   ├── registers.h             ← STM32H753 register definitions
│   ├── ptp_engine.c             ← PTP parser, BMCA, GM spoof, skew
│   ├── ptp_engine.h
│   ├── ntp_engine.c             ← NTP responder, skew, covert encode
│   ├── ntp_engine.h
│   ├── eth_bridge.c             ← L2 transparent bridge
│   ├── eth_bridge.h
│   ├── tcxo_drvr.c              ← TCXO frequency control, PLL
│   ├── tcxo_drvr.h
│   ├── skew_gen.c               ← skew profile generator
│   ├── skew_gen.h
│   ├── covert_codec.c           ← covert channel codec
│   ├── covert_codec.h
│   ├── ble_link.c               ← BLE UART link
│   ├── ble_link.h
│   ├── imux_tamper.c            ← IMU tamper detection
│   ├── imux_tamper.h
│   └── Makefile
├── kicad/
│   ├── chronos-phantom.kicad_sch  ← schematic
│   ├── chronos-phantom.kicad_pcb  ← PCB layout
│   └── chronos-phantom.kicad_pro  ← project file
└── app/
    ├── App.tsx                 ← main app entry
    ├── src/
    │   ├── screens/
    │   │   ├── DashboardScreen.tsx
    │   │   ├── TopologyScreen.tsx
    │   │   ├── AttackControlScreen.tsx
    │   │   ├── BmcaSpooferScreen.tsx
    │   │   ├── CovertChannelScreen.tsx
    │   │   ├── CaptureLogScreen.tsx
    │   │   └── SettingsScreen.tsx
    │   ├── ble/
    │   │   ├── BleManager.ts
    │   │   └── protocol.ts
    │   ├── components/
    │   │   ├── StatusCard.tsx
    │   │   ├── SkewProfilePicker.tsx
    │   │   └── FrameLogTable.tsx
    │   └── types.ts
    ├── package.json
    ├── tsconfig.json
    └── babel.config.js
```

---

## 12. License & Credits

- **Firmware & App:** GPL-2.0
- **Hardware (KiCad):** CERN-OHL-S v2
- **Author:** jayis1

All firmware, hardware, and application code in this directory is authored by **jayis1** and released under the above licenses. No part of this work is attributed to Nous Research or any other entity.

---

*Chronos-Phantom — Time is the attack surface.* — jayis1