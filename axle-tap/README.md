# AxleTap — Automotive Ethernet (100/1000BASE-T1) Tap, MITM & gPTP Grandmaster Spoofing Platform

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

```
   ╔══════════════════════════════════════════════════════════════════════╗
   ║      █████╗ ██╗   ██╗███████╗██╗  ██╗   ██████╗ ████████╗██╗  ██╗  █████╗  ║
   ║     ██╔══██╗██║   ██║██╔════╝██║  ██║   ██╔══██╗╚══██╔══╝██║  ██║ ██╔══██╗ ║
   ║     ███████║██║   ██║███████╗███████║   ██║  ██║   ██║   ███████║███████║ ║
   ║     ██╔══██║██║   ██║╚════██║██╔══██║   ██║  ██║   ██║   ██╔══██║██╔══██║ ║
   ║     ██║  ██║╚██████╔╝███████║██║  ██║   ██████╔╝   ██║   ██║  ██║██║  ██║ ║
   ║     ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝   ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ║
   ║   Automotive Ethernet Tap · Inline MITM · gPTP Grandmaster Spoofing         ║
   ║   TSN Manipulation · SOME/IP Fuzzing · ADAS Bus Attack Surface             ║
   ╚══════════════════════════════════════════════════════════════════════╝

   Author:  jayis1
   Version: 1.0
   Date:    2026-07-22
```

> **⚠️ LEGAL DISCLAIMER:** AxleTap is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on vehicles, robotics, and industrial systems you own or have explicit written permission to assess. Interception, injection, or manipulation of in-vehicle networks, ADAS sensor feeds, or automotive Ethernet traffic on vehicles you do not own may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA), the Vehicle Safety Act (49 U.S.C. § 30122 — tampering with motor vehicle safety systems), the EPCA anti-tampering provisions for emissions control, EU UNECE R155/R156 (cybersecurity / software update regulations), ISO/SAE 21434 obligations, and data-protection laws in your jurisdiction. Spoofing a gPTP grandmaster on a live vehicle can cause ADAS misbehavior and create a real-world safety hazard. **Never deploy AxleTap on a vehicle operating on public roads, on any vehicle without explicit authorization, or in a manner that could affect the safety of occupants or third parties.** The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment.** This documentation is provided for educational and authorized research purposes only.

**Author / Creator:** jayis1
**License:** Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT
**Status:** Design complete — ready for fabrication

---

## 1. Overview

The **AxleTap** is a portable, pocket-sized **Automotive Ethernet** interception, manipulation, and time-attack platform. It targets the **100BASE-T1 / 1000BASE-T1** single-twisted-pair full-duplex Ethernet links (IEEE 802.3bw / 802.3bp) that have become the high-bandwidth backbone of modern vehicles — connecting ADAS sensor fusion ECUs, central compute / zonal controllers, infotainment SoCs, and increasingly the autonomous driving stack. It also targets the **Time-Sensitive Networking (TSN)** extensions (IEEE 802.1AS / 802.1Qbv / 802.1CB) layered on top: gPTP time synchronization, time-aware shapers, and frame replication.

AxleTap sits **inline** on an automotive Ethernet link (an OBD-II/vehicle connector break-out cable, or a bare-rocker "TAP-12" inline harness clamping the twisted pair). From that position it can:

- **Passively tap** both directions of a 100BASE-T1 / 1000BASE-T1 link at line rate, mirroring both directions to a USB-CDC virtual Ethernet / PCAP-stream, turning any in-vehicle link into a Wireshark-grade capture point.
- **Act as an inline MITM**: terminate the link on two automotive PHY ports, bridge the two directions in a cut-through store-and-forward engine, and selectively inject, drop, modify, or clone Ethernet frames in flight. Enables SOME/IP / DDS / AVB/TSN stream manipulation, ARP/DHCP spoofing, and L2 unicast/broadcast injection.
- **Spoof a gPTP grandmaster**: become the best clock (highest priority1 / lowest clockIdentity) on the automotive Ethernet time domain, causing all ECUs that depend on IEEE 802.1AS time (sensor fusion, time-triggered scheduling, AVB media clocks) to synchronize to AxleTap's forged clock. This enables time-slipping attacks that desync ADAS sensor frames, defeat 802.1Qbv time-aware shaper schedules, or freeze AVB media playback.
- **Manipulate TSN streams**: forge 802.1CB sequence numbers to trigger frame-replication discard, inject out-of-window frames into a 802.1Qbv schedule to cause scheduled-gate collisions, and spoof SR-class reservations (SR-A / SR-B) to starve best-effort traffic.
- **Fuzz SOME/IP / SOME/IP-SD**: discover services via SOME/IP-SD Offer messages, then generate malformed SOME/IP payloads, header-truncation, or oversized-segment attacks against in-vehicle service consumers.
- **Profile the vehicle's Ethernet topology**: discover which ECUs speak which protocols (SOME/IP, DoIP, AVB, DDS, ARP), map L2 MAC topology, fingerprint the gPTP grandmaster, and decode the 802.1Qbv gate-control list.
- **Exfiltrate** captured traffic over an encrypted BLE backhaul to the operator's mobile device when the USB host is not connected (covert battery mode).

The key insight that makes AxleTap **genuinely novel** for red teamers: **Automotive Ethernet is rapidly replacing CAN in the high-value parts of the vehicle** — the ADAS stack, the central compute, the autonomous driving pipeline — but almost all existing pentest hardware in the open-source ecosystem targets CAN. The OBD-II CAN bus is the well-trodden, low-speed attack surface that every researcher already knows. The 100BASE-T1/1000BASE-T1 link between, say, a front camera ECU and the central ADAS compute is the *new* attack surface — running at 100 Mbps or 1 Gbps over a single unshielded twisted pair, with a radically different physical layer (PAM3 / 4B/5B symbol coding, master/slave PHY link-up, IEEE 802.1AS time sync) that no existing open-source tap handles. There is no Fluke / Intrepid / Vector hardware equivalent available as open source; the commercial gear (Vector VN5650, Intrepid CSA, Techtonic tools) costs $15k–$50k. AxleTap fills that gap.

### Why automotive Ethernet (and why now)

Modern vehicles (2018+), autonomous shuttles, and modern industrial robots increasingly replace CAN with **100BASE-T1** (100 Mbps, single pair) and **1000BASE-T1** (1 Gbps, single pair) for these reasons:

| Link | Where it runs | What it carries |
|------|---------------|-----------------|
| 100BASE-T1 (802.3bw) | ADAS cameras, radar, lidar ↔ sensor-fusion ECU; zonal gateway ↔ zonal gateway | SOME/IP service traffic, AVB audio/video, DoIP diagnostics |
| 1000BASE-T1 (802.3bp) | Central compute ↔ ADAS domain; central compute ↔ autonomous driving SoC; lidar ↔ compute | High-rate sensor streams, uncompressed video, DDS RTPS |
| 10BASE-T1S (802.3cg) | Low-rate sensor ↔ ECU "LAST MILE" multipoint | Sensor data, calibration, mixed-signal I/O |

The critical attack surface is **gPTP (IEEE 802.1AS)**: all AVB/TSN time-triggered scheduling depends on a common time. Spoofing the grandmaster lets an attacker desync the entire AVB/TSN time domain — causing ADAS sensor frames to be discarded, time-aware shaper schedules to fire at the wrong time, or AVB media to drift. This is the **time layer** of automotive Ethernet, and it has *never* had an open-source attack tool.

### Key features

- **Dual 100/1000BASE-T1 ports** (two Marvell 88Q2XXX-class automotive PHYs) — full-duplex link rate support for IEEE 802.3bw (100 Mbps) and 802.3bp (1000 Mbps) over a single unshielded twisted pair each.
- **Inline cut-through bridge**: <300 ns store-and-forward latency (sub-microsecond, below 802.1Qbv cycle time) so that time-triggered traffic still arrives in-window during MITM.
- **Passive tap mode**: PHY-level mirror of both directions to a USB-CDC virtual Ethernet / PCAP-stream without modifying the link.
- **gPTP grandmaster spoofing**: full IEEE 802.1AS / 1588v2 PTP stack with forged best-master-clock selection (priority1=0, clockClass=6, clockAccuracy=0x20), enabling time-slipping and freeze attacks against the AVB/TSN domain.
- **TSN stream manipulation**: 802.1CB sequence-number forgery, 802.1Qbv schedule injection, SR-class reservation spoofing.
- **SOME/IP fuzzer**: service discovery (SOME/IP-SD), malformed payload generation, header-truncation, oversized-segment.
- **USB-CDC virtual Ethernet** for the operator's laptop — full PCAP capture with native Wireshark/tcpdump compatibility.
- **BLE 5.0 backhaul** for covert exfiltration / remote trigger when no USB host is attached.
- **SD-card capture** for long-duration unattended recording of in-vehicle Ethernet traffic (up to 256 GB).
- **OBD-II break-out harness** (Vehicle Interface Adapter, sold separately) with automotive-grade 100BASE-T1 OBD-II pins (SAE J1962.3 — under review) or inline rocker clamps.
- **Power**: passive — draws from the inline link's available power budget where supported, plus a 12 V automotive input and an internal 1000 mAh Li-Po for battery mode.
- **Hardware interlocks**: arming switch + keyed enable for transmit/inject functions; receive-only default state.

---

## 2. Attack Surface and Threat Model

### 2.1 Attack surface

AxleTap attacks four layers of the automotive Ethernet stack:

**L1 — Physical / PHY layer (100/1000BASE-T1).**
The PHY link uses master/slave negotiation, PAM3 signaling, and 4B/5B block coding. An attacker who can terminate the link on both sides can re-establish link at a degraded rate (force 100BASE-T1 downgrade on a 1000BASE-T1 link) or inject a PHY-level fault to force re-link, which resets the gPTP best-master-clock election — an opening for grandmaster spoofing.

**L2 — Ethernet MAC + 802.1Q VLAN / 802.1Qbv TSN.**
VLAN-tagged frames carry the SR-class (SR-A = VID 1, SR-B = VID 2) used by AVB. 802.1Qbv gate-control lists schedule traffic into time slots. An attacker can forge 802.1CB sequence numbers (triggering frame-replication discard at the receiving stream splitter), inject out-of-window frames (causing a schedule to fire on the wrong cycle), or starve best-effort traffic by spoofing SR-class reservations.

**L3 — Network / IP.**
Standard IPv4/IPv6 with ARP, DHCP, ICMP. Vehicle-internal DHCP servers (often a zonal gateway) assign addresses. An attacker can ARP-spoof to MITM the L3 path, DHCP-spoof to inject a rogue gateway, or send ICMP to disrupt diagnostics (DoIP runs on TCP 13400).

**L4+ — Transport & application: SOME/IP, DDS, DoIP, AVB.**
SOME/IP (Scalable service-Oriented MiddlewarE over IP, AUTOSAR) is the dominant in-vehicle service framework. SOME/IP-SD (Service Discovery) uses UDP 30490. DoIP (Diagnostics over IP, ISO 13400) uses TCP 13400. AVB streams use 802.1AS time. An attacker can:
  - Discover all SOME/IP services via SD Offer messages.
  - Inject forged SOME/IP-SD Offer / Subscribe messages to disrupt service binding.
  - Fuzz SOME/IP consumers with malformed headers, oversized payloads, or fragment-level attacks.
  - Send DoIP routing-activation messages to establish a diagnostic session, then issue UDS commands over the diagnostic session.
  - Desync AVB media clocks via gPTP grandmaster spoofing.

**Time layer — gPTP / IEEE 802.1AS.**
The common time used by AVB/TSN. gPTP runs on the multicast MAC 01-80-C2-00-00-0E. A forged grandmaster with priority1=0 will win the BMC election and become the time source for the entire domain, enabling time-slip and freeze attacks.

### 2.2 Threat model

**Asset:** The in-vehicle Ethernet segment and its time domain (the ADAS / autonomous driving pipeline).

**Adversary capabilities:**
- **Physical access** to the vehicle's Ethernet link (via an OBD-II break-out, a service bay connector, an inline clamp on the twisted pair, or a tampered wiring harness). This is the same trust assumption as any OBD-II attack tool.
- **No cryptographic keys** required to attack gPTP (802.1AS does not mandate authentication by default — a known automotive Ethernet weakness).
- **No pairing** required to attack SOME/IP-SD (discovery is multicast).

**Adversary goals:**
1. **Reconnaissance** — map the vehicle's Ethernet topology, services, and time domain without affecting operations (passive tap).
2. **Time attack** — desync the AVB/TSN time domain to degrade ADAS sensor fusion or freeze AVB media.
3. **Injection** — inject forged frames (SOME/IP, DoIP, 802.1CB) to manipulate in-vehicle services.
4. **Exfiltration** — capture long-duration traffic to a SD card or BLE backhaul for offline analysis.

**Out of scope:**
- Attacks that require breaking authenticated SOME/IP (TP, transport-layer security for SOME/IP — where deployed).
- Attacks on the SoC's secure boot / runtime (separate research domain).
- Wireless-only attacks (AxleTap requires physical link access).

### 2.3 Safety considerations

**gPTP grandmaster spoofing is a safety-critical operation.** Many ADAS / autonomous driving systems use gPTP time to gate sensor fusion windows and to schedule 802.1Qbv traffic. Desyncing the time domain can cause:
- Sensor frames to be discarded (out-of-window delivery).
- Time-triggered actuation to fire at the wrong time.
- AVB media playback to drift or freeze.

For this reason AxleTap's **transmit/inject** function is hardware-gated by a physical arming switch and a keyed enable jumper. The firmware boots in **receive-only mode** by default and refuses to transmit any frame on the link port unless the arming switch is closed AND a software arm command has been received over USB or BLE. The companion app shows a persistent red banner when the device is armed.

**Never deploy AxleTap on a vehicle operating on public roads.** Use only on a stationary vehicle on private property with explicit authorization.

---

## 3. Hardware Specifications

### 3.1 Block diagram

```
                       ┌─────────────────────────────────────────────────┐
                       │              AxleTap — Inline MITM Engine         │
                       │                                                 │
   LINK A  ─── UT1A ───┤─ 100/1000BASE-T1 PHY A ──┐                     │
   (ECU side)           │                          │                     │
                       │              ┌────────────┴────────────┐         │
                       │              │   Cut-through Bridge    │         │
                       │              │   (STM32H7 + MAC fabric) │         │
                       │              └────────────┬────────────┘         │
                       │                          │                     │
   LINK B  ─── UT1B ───┤─ 100/1000BASE-T1 PHY B ──┘                     │
   (compute side)      │                                                 │
                       │   ┌──────────────┬──────────────┬──────────┐    │
                       │   │ gPTP Engine  │ TSN Engine   │ Fuzzer   │    │
                       │   │ (802.1AS)   │ (802.1CB/Qbv)│ (SOME/IP)│    │
                       │   └──────┬──────┴──────┬───────┴────┬─────┘    │
                       │          │             │            │           │
                       │   ┌──────┴─────────────┴────────────┴────┐    │
                       │   │       STM32H723ZGT6 (Cortex-M7)       │    │
                       │   │   550 MHz · 1 MB SRAM · 2 MB Flash     │    │
                       │   └──────┬──────────────────────┬─────────┘    │
                       │          │                      │              │
                       │   ┌──────┴──────┐         ┌──────┴──────┐     │
                       │   │ USB-CDC     │         │ BLE 5.0     │     │
                       │   │ (USB-C)     │         │ (nRF52820)  │     │
                       │   │ + SD-Card   │         │             │     │
                       │   └──────┬──────┘         └──────┬──────┘     │
                       └──────────┼──────────────────────┼─────────────┘
                                  │                      │
                              USB-C / Laptop         Mobile App (BLE)
                       ┌──────────┴──────────┐
                       │  Power Mgmt          │
                       │  12 V automotive     │
                       │  + 1000 mAh Li-Po    │
                       │  + ARM/JTAG/Kill SW  │
                       └─────────────────────┘
```

### 3.2 Component selection

| Subsystem | Part | Rationale |
|-----------|------|-----------|
| **Main MCU** | STMicro STM32H723ZGT6 | Cortex-M7 @ 550 MHz, 1 MB SRAM (288 KB ITCM + 1 MB DTCM/AXI), 2 MB Flash. Two on-chip Ethernet MACs (one used for the cut-through bridge, one for USB-CDC virtual Ethernet), hardware crypto (AES/SHA/HMAC) for BLE backhaul encryption, FMC for SD-card. Chosen for: high SRAM (frame buffers), dual MAC, automotive-grade (AEC-Q100 grade 3 available). |
| **Automotive PHY A/B** | Marvell 88Q2112 (1000BASE-T1) or 88Q2XXX family | AEC-Q100 grade 1, 1000BASE-T1 / 100BASE-T1 dual-mode via MDIO configuration, RGMII / MII interface to STM32H7 MAC. Single twisted pair, master/slave negotiation, PAM3 + 4B/5B. Industrial-temp. Used as the two link ports of the inline bridge. |
| **Switch fabric / cut-through** | Two STM32H7 MACs + DMA double-buffered descriptors | Cut-through bridging at sub-microsecond latency via DMA-driven ring buffers, two MAC ports configured as a bridge. For 1000 Mbps, a L2 switch chip (Marvell 88E6141 / Microchip KSZ9477) is provisioned as an optional BOM alternative for line-rate cut-through. |
| **BLE backhaul** | Nordic nRF52820 | BLE 5.0, ARM Cortex-M4, SPI to main MCU. Used for covert exfiltration and remote arm. Encrypted with AES-CCM using a per-session key derived from a shared operator passphrase. |
| **USB interface** | STM32H7 USB OTG-HS (USB-C connector) | CDC-ECM virtual Ethernet + bulk PCAP-stream endpoints. Powered through USB-C (PD up to 5 V/3 A). |
| **SD card** | MicroSD slot on FMC / SDIO | Up to 256 GB, FAT32, raw PCAP circular buffer. Long-duration unattended capture. |
| **Power** | 12 V automotive input + 1000 mAh Li-Po | 12 V from vehicle (OBD-II pin 16 / inline harness), stepped down to 5 V/3.3 V via TPS62177. Li-Po for battery/covert mode. MCP73831 charger. |
| **Interlocks** | Hardware arming switch + keyed enable jumper + red status LED | Mechanical switch in series with the link PHY reset-enable line: open = receive-only, closed = armed. Keyed jumper disables transmit permanently for capture-only operations. Red LED indicates armed state. |
| **Form factor** | 78 × 32 mm PCB (USB stick form) + inline harness | Matches the inline-link form factor of a small automotive Ethernet tap; fits behind a vehicle trim panel. |

### 3.3 I/O and connectivity

| Port | Function | Connector |
|------|----------|-----------|
| LINK A (UT1A) | 100/1000BASE-T1 to ECU side | Inline harness clamp (UTP, 100Ω) |
| LINK B (UT1B) | 100/1000BASE-T1 to compute side | Inline harness clamp (UTP, 100Ω) |
| USB-C | CDC-ECM virtual Ethernet + PCAP-stream + power | USB-C receptacle |
| BLE 5.0 | Covert exfiltration / remote arm | PCB antenna, +4 dBm |
| SD card | Long-duration capture | MicroSD slot |
| 12 V input | Automotive power | OBD-II pin 16 or inline harness |
| Arm switch | Enable transmit | Slide switch (red) |
| Kill key | Disable transmit permanently | Jumper |
| Status LEDs | Link / Arm / Cap / BLE | 4 × 0603 LED |

---

## 4. Architecture

### 4.1 Bridge architecture

AxleTap's core is a **two-port inline bridge** implemented as two automotive Ethernet PHYs (PHY A and PHY B) connected to two Ethernet MACs on the STM32H723. The two MACs run a cut-through bridge engine in firmware (DMA-driven ring buffers), forwarding frames from A→B and B→A with minimal store-and-forward latency.

The bridge operates in three modes:

1. **Passive tap (default)**: PHY A and PHY B are each connected to one side of the inline link, but the bridge engine does not forward; instead, each PHY's receive path is mirrored to the USB-CDC virtual Ethernet for capture. The original link is unbroken because AxleTap's two PHYs link up with the two sides independently and the engine does not re-inject frames onto the link. (In true passive mode, the link is cut and re-established through AxleTap; AxleTap bridges it transparently. In pure-tap mode without MITM, AxleTap re-injects frames bidirectionally but does not modify them — equivalent to a L2 repeater with a mirror port.)

2. **Inline bridge (MITM)**: The bridge engine forwards A→B and B→A, but the operator's injection ruleset can drop, modify, clone, or inject frames in flight. Latency budget: <300 ns store-and-forward via DMA double-buffered descriptors at 550 MHz.

3. **Grandmaster spoof (gPTP only)**: The bridge engine forwards all traffic unchanged except for gPTP (multicast MAC 01-80-C2-00-00-0E) frames, which are intercepted and replaced with AxleTap's forged Pdelay_Resp / Sync / Follow_Up messages. This turns AxleTap into the time source for the entire AVB/TSN domain without disturbing the data path.

### 4.2 gPTP grandmaster spoofing

AxleTap runs a full IEEE 802.1AS / 1588v2 PTP stack in firmware. To win the BMC election it advertises:
- `priority1 = 0` (highest possible)
- `clockClass = 6` (primary reference — application-specific)
- `clockAccuracy = 0x20` (≤ 1 µs)
- `clockIdentity = 00:00:00:FF:FE:00:00:01` (forged EUI-48)

On startup, AxleTap listens for existing Pdelay_Resp / Sync messages to learn the current grandmaster and link delay. It then starts sending its own Sync / Follow_Up / Pdelay_Resp messages at the standard intervals (Sync 125 ms, Pdelay_Resp 1 s), winning the BMC election by priority. Once it is the grandmaster, all ECUs in the domain synchronize their local clocks to AxleTap's time, enabling:
- **Time-slip**: gradually shift the time base to desync sensor-fusion windows.
- **Time-freeze**: hold the time constant to freeze AVB media and time-triggered schedules.
- **Time-jump**: step the time to break 802.1Qbv gate-control-list assumptions.

### 4.3 TSN stream manipulation

For 802.1CB (Frame Replication and Elimination), AxleTap intercepts tagged frames and rewrites the sequence number field, causing the receiving stream splitter to discard "duplicate" frames (actually unique frames with forged sequence numbers).

For 802.1Qbv (Time-Aware Shaper), AxleTap injects frames into time windows that are not scheduled for the matching SR-class, causing gate collisions and schedule disruption.

### 4.4 SOME/IP fuzzer

AxleTap runs a SOME/IP-SD discovery engine that listens for Offer messages and builds a service map. The fuzzer can then generate:
- Malformed SOME/IP headers (bad length, bad message ID, bad interface version).
- Oversized payloads (segment-level attacks against MTU handling).
- Truncated segments.
- Forged SOME/IP-SD Offer / Subscribe messages to disrupt service binding.

### 4.5 Capture pipeline

```
PHY A RX ──┐
           ├─→ DMA ring ──→ STM32H7 MAC ──→ cut-through bridge ──→ PHY B TX
PHY B RX ──┘                                          │
                                                      │ mirror
                                                      ↓
                                          USB-CDC bulk PCAP-stream ──→ Laptop (Wireshark)
                                                      │
                                                      ↓
                                          SD card (circular PCAP) ──→ offline analysis
                                                      │
                                                      ↓ (BLE covert mode)
                                          nRF52820 AES-CCM ──→ Mobile app
```

---

## 5. Firmware Details and Design Decisions

### 5.1 Architecture

The firmware is a bare-metal C application on the STM32H723. No RTOS is used for the bridge path (latency-critical); a small cooperative scheduler handles the slower tasks (USB, BLE, SD, gPTP, fuzzer). The bridge path runs in ITCM with DMA descriptors in DTCM for zero-copy forwarding.

Key design decisions:

- **No RTOS for the bridge path.** An RTOS introduces context-switch jitter that would violate the 802.1Qbv schedule. The bridge runs from a single priority-15 interrupt handler, ITCM-resident, with the DMA ring double-buffered. All other tasks (USB, BLE, SD, gPTP, fuzzer) run in a cooperative round-robin scheduler at priority 1–7.

- **DMA double-buffered descriptors.** Two descriptor rings per MAC, ping-pong'd to keep latency under one 802.1Qbv cycle (typically 125 µs).

- **gPTP in ITCM.** The PTP stack is the latency-sensitive part of the time attack; placing it in ITCM gives deterministic instruction timing.

- **Hardware interlock in firmware.** The arm-switch state is read via a GPIO that is physically in series with the PHY TX-enable line. Even if firmware is compromised, the link cannot transmit unless the arm switch is closed. The kill-key jumper is read by firmware and overrides any software arm command: if the kill-key is set, the firmware never asserts TX-enable and refuses injection commands.

- **Cut-through vs store-and-forward.** AxleTap uses cut-through (forward as soon as the destination MAC is received) to minimize latency. The full frame is still DMA'd into the buffer for the injection ruleset; the ruleset is evaluated in parallel with cut-through forwarding, and a "drop" decision can pull the frame before the end of forwarding. (For the present design, store-and-forward is implemented — sub-microsecond is achievable; see the firmware for the dual descriptor approach.)

### 5.2 Files

- `firmware/main.c` — Top-level init, bridge engine, scheduler, USB-CDC, CLI.
- `firmware/registers.h` — STM32H723 register definitions.
- `firmware/board.h` — Pin/peripheral map and macros.
- `firmware/drivers/phy_driver.c/.h` — Marvell 88Q2112 PHY driver (MDIO config, link-up, mode selection).
- `firmware/drivers/bridge_driver.c/.h` — Cut-through bridge engine (DMA ring, forward, drop, modify, clone).
- `firmware/drivers/gptp_driver.c/.h` — IEEE 802.1AS PTP stack (BMC election, Sync/Follow_Up/Pdelay_Resp, time attack).
- `firmware/drivers/tsn_driver.c/.h` — 802.1CB sequence forgery + 802.1Qbv schedule injection.
- `firmware/drivers/someip_driver.c/.h` — SOME/IP-SD discovery + fuzzer.
- `firmware/drivers/ble_bridge.c/.h` — nRF52820 SPI bridge + AES-CCM encryption for covert exfil.
- `firmware/drivers/pcap_driver.c/.h` — PCAP writer (USB-CDC stream + SD-card circular buffer).
- `firmware/Makefile` — arm-none-eabi-gcc build, STM32CubeProgrammer flash.

See the firmware files in `firmware/` for the full implementation.

---

## 6. Application / Software Interface

The companion app is a React Native app (iOS / Android) that connects to AxleTap over BLE 5.0. It provides:

- **Dashboard**: link status (A/B), rate, arm state, capture state, SD usage, battery.
- **Capture screen**: live frame counter, protocol distribution (SOME/IP, DoIP, ARP, gPTP, AVB), PCAP download over BLE.
- **gPTP screen**: current grandmaster identity, BMC election status, time-attack controls (slip rate, freeze, jump).
- **TSN screen**: 802.1CB sequence table, 802.1Qbv gate-control-list view, injection controls.
- **SOME/IP screen**: discovered service map, fuzzer controls (target service, payload mode).
- **Console**: raw CLI over BLE (mirror of USB-CDC CLI).

See `app/` for the full implementation.

### 6.1 USB-CDC CLI

Over USB, AxleTap exposes a CDC-ACM serial console with a command-line interface:

```
axle> help
  status              - show link / arm / capture status
  arm                 - enable transmit (requires arm switch closed)
  disarm              - disable transmit
  tap on|off          - passive tap mode
  bridge on|off       - inline MITM bridge
  gptp gm             - spoof grandmaster
  gptp slip <ppb>     - time slip at <ppb> rate
  gptp freeze         - hold time constant
  gptp reset          - return to passive
  tsn seqforge on|off - 802.1CB sequence forgery
  tsn qbv inject      - inject out-of-window frame
  someip discover     - run SOME/IP-SD discovery
  someip fuzz <svc> <mode> - fuzz service
  pcap start|stop     - start/stop capture
  pcap dump           - dump SD-card PCAP to USB
  ble on|off          - enable BLE backhaul
```

---

## 7. Use Cases

### 7.1 Red team — reconnaissance

Plug AxleTap inline on a vehicle's ADAS Ethernet link (via OBD-II break-out or inline clamp). In passive tap mode, capture all traffic to a laptop (Wireshark) or to the SD card. Map:
- Which ECUs speak SOME/IP, DoIP, AVB.
- The gPTP grandmaster identity.
- The 802.1Qbv gate-control list.
- MAC-to-MAC topology (via LLDP if present, or inferred from traffic).

Output: a topology map of the vehicle's Ethernet segment — the starting point for any further attack.

### 7.2 Red team — time attack

After reconnaissance, identify the gPTP grandmaster. Arm AxleTap (with explicit safety authorization, stationary vehicle only) and issue `gptp gm` to spoof the grandmaster. Then issue `gptp slip -1000` to slip the time at -1000 ppb. Observe:
- ADAS sensor frames being discarded as out-of-window.
- AVB media drifting or freezing.
- 802.1Qbv schedules firing at the wrong time.

This is a high-impact demonstration of the automotive Ethernet time-layer attack surface — one that is invisible to CAN-focused security teams.

### 7.3 Red team — SOME/IP service fuzzing

After discovery, target a SOME/IP service (e.g., a sensor service) and issue `someip fuzz 0x1234 header-trunc`. Observe whether the consumer crashes, hangs, or responds with malformed error frames. This maps the robustness of the in-vehicle service framework.

### 7.4 Security researcher — 802.1CB robustness testing

Use AxleTap's TSN engine to forge 802.1CB sequence numbers and inject duplicate/out-of-sequence frames. Observe how the receiving stream splitter handles forged sequence numbers. This validates the resilience of frame replication and elimination in the target TSN stack.

### 7.5 Security researcher — gPTP authentication gap

AxleTap demonstrates that IEEE 802.1AS, as deployed in most production vehicles, does not authenticate PTP messages — any device that wins the BMC election becomes the time source. This is a known gap that AxleTap makes concretely exploitable, motivating the adoption of 802.1AS-Rev authentication or 1588 MACsec.

### 7.6 Long-duration capture

Deploy AxleTap in passive tap mode with a 256 GB SD card for unattended long-duration capture of in-vehicle Ethernet traffic. Retrieve the PCAP later for offline analysis (Wireshark with the SOME/IP / 802.1AS dissectors).

### 7.7 Covert exfiltration

In a physical-red-team scenario where a USB host is not available, AxleTap captures to SD card and exfiltrates a compressed summary (frame counts, protocol distribution, service map) over encrypted BLE to the operator's mobile app.

---

## 8. Limitations and Future Work

- **Line-rate cut-through at 1000 Mbps**: the firmware uses store-and-forward with DMA double-buffering; sub-microsecond is achievable but a L2 switch chip (Marvell 88E6141) is the BOM alternative for true line-rate cut-through at 1 Gbps.
- **SOME/IP-TP (transport protocol) fragmentation attacks**: not yet implemented; the fuzzer targets unicast/single-pdu SOME/IP.
- **DoIP authentication**: DoIP over TLS (DoIP over TLS, ISO 13400-2:2019 Addendum) is not bypassed; AxleTap only operates on unauthenticated DoIP.
- **gPTP authentication (802.1AS-Rev)**: AxleTap's grandmaster spoof assumes no authentication. Future work: integrate 802.1AS-Rev / 1588 MACsec bypass where keys are available.

---

## 9. Legal / Ethical Disclaimer

**This device is for authorized security research and penetration testing only.** Deploying AxleTap on any vehicle, robot, or industrial system you do not own or do not have explicit written authorization to test may be illegal under:
- 18 U.S.C. § 1030 (Computer Fraud and Abuse Act)
- 49 U.S.C. § 30122 (Vehicle Safety Act — tampering with motor vehicle safety systems)
- EU UNECE R155 / R156 (vehicle cybersecurity / software update regulations)
- ISO/SAE 21434 (road vehicle cybersecurity engineering)
- State and national wiretap, computer-misuse, and anti-tampering statutes.

**Never deploy AxleTap on a vehicle operating on public roads or on any vehicle without explicit written authorization.** gPTP grandmaster spoofing can cause ADAS / autonomous driving misbehavior and is a real-world safety hazard.

The author (**jayis1**) assumes no liability for misuse. Always obtain proper written authorization before deployment. This documentation is provided for educational and authorized research purposes only.

---

*Author: jayis1 · License: Hardware CERN-OHL-S v2 · Firmware GPL-2.0 · App MIT · 2026-07-22*