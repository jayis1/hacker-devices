# ZIGBEE-PHANTOM — IEEE 802.15.4 / Zigbee Network Intrusion & Key Extraction Platform

![Device](https://img.shields.io/badge/status-design-green) ![License](https://img.shields.io/badge/license-GPL--2.0-blue) ![Author](https://img.shields.io/badge/author-jayis1-orange)

> **⚠️ LEGAL & ETHICAL DISCLAIMER:** This device is designed **exclusively** for authorized security research, penetration testing with explicit written consent, and red-team operations on networks and infrastructure you own or have explicit written permission to assess. Intercepting, injecting, jamming, or manipulating Zigbee/802.15.4 communications on networks you do not own may violate computer fraud and abuse laws (e.g., 18 U.S.C. § 1030 CFAA, ECPA), wiretap statutes, and radio-communication regulations in your jurisdiction. Unauthorized extraction or use of cryptographic network keys may constitute a criminal offense. The author (**jayis1**) assumes no liability for misuse. **Always obtain proper written authorization before deployment, and ensure compliance with local radio-emission regulations (FCC Part 15, ETSI EN 300 440, etc.).** This documentation is provided for educational and authorized research purposes only. KillerBee-class tooling, while powerful, must be used responsibly.

**Author / Creator:** jayis1
**License:** Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT
**Status:** Design complete — ready for fabrication

---

## Table of Contents

1. [Overview & Problem Statement](#overview--problem-statement)
2. [Attack Surface & Threat Model](#attack-surface--threat-model)
3. [Hardware Specifications](#hardware-specifications)
4. [Architecture & Block Diagram](#architecture--block-diagram)
5. [Theory of Operation — 802.15.4 / Zigbee Protocol Attacks](#theory-of-operation--802154--zigbee-protocol-attacks)
6. [Firmware Details & Design Decisions](#firmware-details--design-decisions)
7. [Application/Software Interface](#applicationsoftware-interface)
8. [Use Cases for Red Teams, Security Researchers & Pentesters](#use-cases-for-red-teams-security-researchers--pentesters)
9. [Bill of Materials](#bill-of-materials)
10. [Legal & Ethical Disclaimer](#legal--ethical-disclaimer)

---

## Overview & Problem Statement

Zigbee — the mesh networking protocol built on IEEE 802.15.4 — is the invisible backbone of the smart world. It runs inside Philips Hue bulbs, Samsung SmartThings hubs, Amazon Echo devices (which include a built-in Zigbee coordinator), Yale and Schlage smart locks, IKEA Trådfri lighting, Aqara and Xiaomi sensors, Honeywell and Siemens building-automation controllers, hospital patient-monitoring infrastructure, and countless industrial sensor meshes. Over 500 million Zigbee devices shipped in 2023 alone. Yet despite this ubiquity, Zigbee security remains critically under-examined in real-world deployments, and open hardware tooling for attacking it is nearly nonexistent outside of academic research (KillerBee, ATZB2530 sticks, and discontinued ApiMote boards).

The **ZIGBEE-PHANTOM** is a purpose-built, portable, battery-operated hardware platform for 802.15.4 / Zigbee security research. It is the first open-source device that unifies four critical attack capabilities into one pocketable tool:

1. **Promiscuous sniffing** — full 802.15.4 PHY/MAC capture with channel-energy scanning, frame decryption (when the network key is known or recovered), and PCAP export.
2. **Key transport interception & recovery** — captures Zigbee 3.0 (Zigbee Pro R21+) `Transport-Key` commands, extracts the `Trust Center Link Key` (the Zigbee Alliance install code derived key), and brute-forces weak install codes.
3. **Rogue coordinator emulation** — impersonates a Zigbee Trust Center or Router, issues forged `NWK Update` / `Network Rejoin` / `Transport-Key` commands, and performs `Rejoin Request` flooding to force key renegotiation or steal the network key.
4. **Selective jamming & reactive de-auth** — detects target frames (e.g., a specific PAN ID or source EUI64) and injects a corrupted ACK or a high-duty-cycle noise burst to disrupt the link, enabling a targeted denial-of-service without jamming the entire band.

The device bridges captured traffic over an encrypted BLE link to a companion mobile app for real-time analysis, and over USB-CDC for full KillerBee-compatible PCAP streaming to a host running standard toolchains (`killerbee`, `zigbee-wireshark dissector`, `zbstumbler`, `zbdump`).

### Why this device, and why now

Zigbee 3.0 introduced mandatory install-code-based trust-center link keys and removed the well-known `ZigbeeAlliance09` key from the default flow — but deployment reality lags far behind the spec. Millions of devices in the field still use the legacy well-known key, or use install codes short enough (4–8 bytes) to fall to offline brute force. Coordinators from major vendors accept rejoins without fresh key negotiation, enabling key-replay attacks. Smart-lock vendors routinely ship with debug `Transport-Key` commands enabled in production firmware. Yet the only tooling to exploit any of this is a decade-old KillerBee framework running on a discontinued Atmel RZUSBSTICK. ZIGBEE-PHANTOM brings modern, programmable, open hardware to this attack surface.

---

## Attack Surface & Threat Model

### Target protocols

| Layer | Standard | Attack relevance |
|-------|----------|------------------|
| PHY | IEEE 802.15.4-2020 (O-QPSK DSSS, 2.4 GHz, 250 kbps, 16 channels) | Energy detection, selective jamming, channel hopping surveillance |
| MAC | IEEE 802.15.4 MAC (beacon/non-beacon, CSMA/CA, AES-CCM\* security) | Frame injection, ACK spoofing, replay, deauth |
| NWK | Zigbee NWK (mesh routing, route discovery) | Route poisoning, mesh-in-the-middle, orphan scan abuse |
| APS | Zigbee APS (application profiles, clusters, ZCL) | Cluster-command injection (e.g., `OnOff` toggle, `Level` manipulation) |
| ZDO | Zigbee Device Object (trust center, join/rejoin) | Rogue coordinator, key transport interception, rejoin flooding |

### Threat model

**Operator (assessor):** Armed with ZIGBEE-PHANTOM, a laptop or phone, and an authorization scope. Goal: recover Zigbee network keys, map device topology, demonstrate injection impact, and exfiltrate via BLE or USB.

**Target environment:** A facility deploying Zigbee for lighting, access control, environmental monitoring, or industrial telemetry. The mesh may span dozens to hundreds of nodes across multiple floors, anchored by one or more coordinators (Trust Centers). Devices may use Zigbee 3.0 install codes, legacy `ZigbeeAlliance09`, or vendor-default keys.

**Defender (assumed):** May deploy Zigbee sniffers (rare), WIDS for 802.15.4 (very rare), or rely on the coordinator's join log. ZIGBEE-PHANTOM is designed to minimize detectable footprint: passive sniffing is receive-only and undetectable by the mesh; active injection uses low-duty-cycle reactive modes rather than continuous-band jamming.

### Attack primitives supported

| Primitive | Description |
|-----------|-------------|
| Passive sniff | Receive all 802.15.4 frames on a channel; auto-follow PAN hopping |
| Energy scan | Per-channel RSSI sweep for mesh discovery across all 16 channels |
| Key recovery | Capture `Transport-Key` / `NwkKeyDescriptor` frames; offline brute install codes |
| Rogue coordinator | Forge `NWK Beacon`, `Transport-Key`, `NWK Update-ID` commands |
| Rejoin flooding | Broadcast forged `Rejoin Request` to force key renegotiation |
| Selective jam | Match a frame filter; emit corrupted ACK or noise burst |
| Relay | Capture a frame on channel A, re-inject on channel B (cross-channel mesh relay) |
| ZCL injection | Forge `OnOff` / `Level` / `DoorLock` cluster commands against joined devices |

---

## Hardware Specifications

### Core radio

- **Primary SoC:** Texas Instruments **CC2652R1F** — ARM Cortex-M4F @ 48 MHz, 128 KB SRAM, 512 KB Flash, integrated 802.15.4 PHY/MAC, BLE 5.0. The CC2652 is the de-facto modern Zigbee radio (used in the official TI CC2652 LaunchPad, SONOFF Zigbee 3.0 dongle, and Home Assistant SkyConnect). Its RF core (a dedicated Cortex-M0) runs the 802.15.4 MAC independently, freeing the M4F for attack logic.
- **RF Front-End:** Nordic **nRF21540** — 802.15.4/Sigfox/Bluetooth range extender, +20 dBm output power (vs. the CC2652's native +5 dBm), LNA with 14 dB gain, selectable gain via GPIO. Boosts effective range from ~30 m to ~300 m LOS — enough to engage a target mesh from outside a building.
- **Diversity antennas:** Two PCB trace antennas (0 dipoles) with an SP3T RF switch (SKY13330) for antenna diversity selection — critical for reliable capture in multipath-rich industrial environments.

### Backhaul & UI

- **Companion MCU:** Espressif **ESP32-S3-MINI-1** — dual-core Xtensa LX7 @ 240 MHz, Wi-Fi 4 + BLE 5.0, 8 MB external PSRAM. Handles the operator backhaul: encrypted BLE to mobile app, and USB-CDC to a host laptop. Runs a full KillerBee-compatible pcap-streaming protocol so the device is a drop-in replacement for the RZUSBSTICK in existing toolchains.
- **Display:** 1.3-inch **SH1106** OLED (128×64, SPI) — shows current channel, RSSI, frame counters, capture stats, and mode status.
- **Input:** 5-way tactical joystick (analog + button) — mode selection, channel change, injection trigger. Designed for gloved operation in a server-room / field scenario.
- **Status LEDs:** 4× — RX (green), TX (red), KEY (amber, lights when a transport-key frame is captured), ERROR (blue).

### Storage

- **MicroSD socket** — up to 32 GB, SPI mode, FAT32. Stores full PCAP captures and recovered key material for post-engagement analysis. A single 16 GB card holds ~200 hours of continuous 802.15.4 capture at full line rate.

### Power

- **Battery:** 1200 mAh LiPo (3.7 V nominal) — ~5 h active sniff + BLE streaming, ~12 h sniff-only (backhaul off).
- **Charging:** TP4056 LiPo charger via USB-C (5 V), MCP73831 backup charger, gas-gauge via MAX17048.
- **Power modes:** ACTIVE (full sniff + backhaul), SNIFF-ONLY (backhaul off, ~25 mA), LOW-POWER-WAIT (radio in RX duty-cycle, ~3 mA, wakes on energy detect), OFF (load-switched).

### Form factor

- PCB: 85 × 54 mm (ISO/IEC 7810 ID-1 card size, credit-card footprint) — fits in a standard card wallet for covert carry.
- Enclosure: 3D-printed PETG shell, 92 × 60 × 18 mm, with SMA bulkhead connectors for two external antennas.
- Weight: ~65 g (without external antennas).

### Connectivity

| Interface | Purpose |
|-----------|---------|
| USB-C (5 V power + CDC data) | Host backhaul, charging, firmware updates (DFU over USB) |
| BLE 5.0 (ESP32-S3) | Mobile app backhaul, encrypted |
| Wi-Fi 4 (ESP32-S3) | Optional — OTA firmware, remote command via VPN |
| SPI (internal) | CC2652 ↔ ESP32-S3 high-speed frame transfer |
| MicroSD SPI | Capture storage |
| SMA ×2 | External 2.4 GHz antennas (diversity) |

---

## Architecture & Block Diagram

```
╔═══════════════════════════════════════════════════════════════════════════╗
║                         ZIGBEE-PHANTOM BLOCK DIAGRAM                      ║
╠═══════════════════════════════════════════════════════════════════════════╣
║                                                                            ║
║   ┌──────────┐   SPI+IRQ   ┌──────────────────┐    RF (2.4 GHz)            ║
║   │  ESP32-S3 │────────────▶│   CC2652R1F     │─────┬────────┐             ║
║   │  companion│◀───────────│  802.15.4 SoC    │     │        │             ║
║   │  MCU      │  +GPIO EN   │  (Cortex-M4F +  │     ▼        ▼             ║
║   │           │             │   RF Cortex-M0) │   ┌──────┐ ┌──────┐        ║
║   │ BLE5/WiFi │             └──────────────────┘   │nRF21540│ │nRF21540│     ║
║   │ CDC-USB   │                      ▲             │ FEM   │ │ FEM  │      ║
║   │ SD host   │                      │             └───┬───┘ └──┬───┘      ║
║   └─────┬─────┘                      │                 │        │          ║
║         │  SPI                        │GPIO             ▼        ▼          ║
║         ▼                            │           ┌─────────┐ ┌─────────┐   ║
║   ┌──────────┐                 ┌──────┴────┐      │  SMA-1  │ │  SMA-2  │   ║
║   │ MicroSD  │                 │ 5-way joy │     │ antenna │ │ antenna │   ║
║   │  (FAT32) │                 │  stick UI │     └─────────┘ └─────────┘   ║
║   └──────────┘                 └──────┬────┘                              ║
║                                       │                                    ║
║   ┌──────────┐     I2C         ┌──────┴────┐                               ║
║   │ SH1106    │◀──────────────│  MAX17048 │  battery gas gauge             ║
║   │ OLED 1.3" │                │  fuel     │                                ║
║   └──────────┘                └───────────┘                                ║
║                                                                            ║
║   ┌──────────────────────┐    ┌──────────┐                                ║
║   │ TP4056 LiPo charger  │◀───│ USB-C    │  5 V + data                   ║
║   │  + load switch        │    │ connector│                                ║
║   └──────────────────────┘    └──────────┘                                ║
║           │                                                                 ║
║           ▼                                                                 ║
║   ┌──────────────────────┐                                                ║
║   │ 1200 mAh LiPo battery │                                                ║
║   └──────────────────────┘                                                ║
║                                                                            ║
╠═══════════════════════════════════════════════════════════════════════════╣
║   Status LEDs: [RX green] [TX red] [KEY amber] [ERR blue]                  ║
╚═══════════════════════════════════════════════════════════════════════════╝
```

### Data flow

1. The **CC2652 RF core** (Cortex-M0) runs the 802.15.4 PHY/MAC in promiscuous mode. It produces frame descriptors (timestamp, RSSI, LQI, channel, CRC status, payload) via the radio's command API (`RF_cmdIeeeRx`, `RF_cmdIeeeTx`).
2. The **CC2652 application core** (Cortex-M4F) drains the RF queue, runs the attack logic (filtering, key extraction, injection scheduling), and forwards frames to the ESP32-S3 over a 20 MHz SPI slave link with an IRQ line for flow control.
3. The **ESP32-S3** formats frames as KillerBee-style pcap records, streams them over USB-CDC to a host, writes them to MicroSD, and/or compresses and encrypts them for BLE backhaul to the mobile app.
4. The **mobile app** decodes the 802.15.4 frames, displays the mesh topology graph, shows captured key material, and lets the operator trigger injection modes and channel changes.

---

## Theory of Operation — 802.15.4 / Zigbee Protocol Attacks

### PHY and MAC capture

The CC2652 RF core is commanded into promiscuous RX (`RF_cmdIeeeRx`) with all auto-ACK and auto-filter disabled. Every received frame — valid CRC or not, matching PAN ID or not — is handed to the application core. Each frame is annotated with:
- A 32-bit microsecond timestamp (from the CC2652's `AON_RTC` — always-on real-time counter, accurate to 1 µs).
- RSSI in dBm (signed 8-bit).
- LQI (link-quality indicator, 0–255).
- Channel (0–26, with 2405 + 5×ch MHz).
- Frame type (beacon, data, ACK, MAC command, LLDN, multipurpose).
- Security-level flag.

### Channel energy scan

The device sweeps all 16 802.15.4 channels (11–26), dwelling 100 ms per channel and recording the `Rssi` field from `RF_cmdIeeeRx`. This produces a 16-bin energy heatmap. Channels with sustained activity above a configurable RSSI threshold are flagged as "mesh active" — the device auto-locks onto the strongest channel for deep capture.

### Key recovery — the core attack

Zigbee 3.0 networks derive the **Trust Center Link Key (TCLK)** from a 16-byte **install code** using AES-MMO (Matyas-Meyer-Oseas) hashing. When a device joins, the Trust Center sends a `Transport-Key` APS command carrying the **network key**, encrypted under the TCLK. If the attacker knows or can brute-force the install code, they derive the TCLK, decrypt the `Transport-Key`, and recover the network key — after which all NWK-encrypted traffic on the mesh is decryptable.

ZIGBEE-PHANTOM captures every `Transport-Key` frame (APS command ID 0x05) and stores:
- The encrypted network key blob.
- The source EUI64 (Trust Center) and destination EUI64 (joining device).
- The APS frame counter and key sequence.

The companion app then runs:
1. **Install-code brute force** — iterates install codes (4, 8, 12, or 16 bytes) through AES-MMO to derive candidate TCLKs, attempts AES-CCM\* decrypt of the `Transport-Key` payload. A 4-byte install code (used by some legacy vendors) falls in seconds; an 8-byte code falls in hours on the ESP32-S3's hardware AES.
2. **Known-key dictionary** — checks the capture against a built-in dictionary of ~200 known default TCLKs (including the legacy `ZigbeeAlliance09` = `5A:69:67:42:65:65:41:6C:6C:69:61:6E:63:65:30:39`, plus vendor defaults extracted from public firmware dumps).
3. **Offline pcap analysis** — exports the full capture as a `.pcap` for offline analysis with Wireshark's Zigbee dissector and the killerbee `zbdump`/`zbdsniff` toolchain.

### Rogue coordinator emulation

The device forges a Zigbee **NWK Beacon** (MAC beacon frame) advertising:
- A spoofed PAN ID (default: matches the target PAN recovered during sniffing).
- A spoofed Extended PAN ID.
- A `ProtocolVersion` (2 = Zigbee Pro) and `StackProfile` (Zigbee 3.0 = 0x03).
- A `RouterCapacity` / `DeviceCapacity` flag indicating "join permitted."
- A `TxOffset` that desynchronizes beacon-following devices.

When an end device or router receives the forged beacon and sees a stronger signal than the real coordinator (enabled by the nRF21540's +20 dBm amplifier), it issues a `Join Request`. ZIGBEE-PHANTOM responds with a forged `Transport-Key` carrying a chosen network key — at which point the device joins the rogue mesh and the attacker controls its ZCL clusters (e.g., toggling a smart lock).

### Selective jamming

Rather than continuous-band jamming (which is trivially detected and illegal in most jurisdictions), ZIGBEE-PHANTOM implements **reactive, frame-filtered jamming**:
- The RF core receives normally.
- The application core inspects each frame header against a filter (PAN ID, source EUI64, frame type).
- On match, the device transmits a maximum-length (~127 byte) 802.15.4 frame with intentionally corrupted FCS, beginning transmit during the inter-frame gap of the target's expected ACK window. This causes the target to miss its ACK and retransmit — a targeted, low-duty-cycle denial-of-service that looks like RF interference rather than active jamming.

### Cross-channel relay

Two CC2652 radios (one on channel A, one on channel B) are operated simultaneously (the CC2652 supports concurrent multi-mode operation via its RF core's multi-instance command API). A frame captured on channel A is re-injected on channel B — effectively extending a mesh's reach across channels, or bridging two otherwise-isolated meshes. This demonstrates the "mesh-in-the-middle" risk in deployments that use channel separation as a soft isolation boundary.

---

## Firmware Details & Design Decisions

### Architecture

The firmware runs on the **CC2652R1F application core** (Cortex-M4F, 48 MHz) and the **ESP32-S3** (Cortex-M4F-class). The two communicate over SPI.

**CC2652 firmware** (in `firmware/`):
- `main.c` — top-level loop, mode state machine (SNIFF / KEY-HUNT / ROGUE-COORD / JAM / RELAY), joystick input handling, OLED status.
- `drivers/cc2652_rf.c` / `cc2652_rf.h` — RF core command API wrapper (`RF_open`, `RF_cmdIeeeRx`, `RF_cmdIeeeTx`, channel/freq config, promiscuous mode setup).
- `drivers/zigbee_mac.c` / `zigbee_mac.h` — 802.15.4 MAC frame parser/builder (frame types, addressing modes, security header, FCS).
- `drivers/zigbee_aps.c` / `zigbee_aps.h` — Zigbee APS layer parser, `Transport-Key` command dissector, APS decrypt (AES-CCM\*).
- `drivers/crypto.c` / `crypto.h` — AES-MMO install-code-to-TCLK derivation, AES-CCM\* NWK/APS decrypt, hardware AES acceleration via the CC2652's `AESC` module.
- `drivers/flash_storage.c` — capture ring buffer in external SPI flash (W25Q128, 16 MB) for capture-before-backhaul-connect buffering.
- `drivers/sd_card.c` — MicroSD FAT32 write for pcap files.
- `drivers/oled.c` — SH1106 SPI driver.
- `drivers/joystick.c` — 5-way joystick ADC driver.
- `board.h` — pin assignments.
- `registers.h` — CC2652 register map (RFC, AESC, AON_RTC, GPIO, I2C, SPI).
- `Makefile` — GCC ARM toolchain build.

**ESP32-S3 firmware** (companion, in `app/` as a bridging daemon — the companion mobile app talks to it): the ESP32-S3 runs a KillerBee-compatible CDC daemon that streams pcap records over USB and/or BLE to the operator. It also serves as the Wi-Fi/BLE bridge for the mobile app.

### Key design decisions

1. **CC2652 + ESP32-S3 dual-MCU** — splitting the 802.15.4 radio from the backhaul keeps the RF core deterministic and the attack logic free from Wi-Fi/BLE timing jitter. A single SoC (e.g., ESP32-C6 with both Wi-Fi and 802.15.4) was considered, but the CC2652's dedicated RF core is materially better at promiscuous capture and the dual-MCU architecture allows the ESP32-S3 to be updated independently of the radio firmware.

2. **nRF21540 FEM** — the +15 dB power boost is critical for rogue-coordinator attacks: the forged beacon must appear stronger than the legitimate coordinator. The FEM's LNA also improves passive sniff sensitivity by ~10 dB, enabling capture from outside a target building.

3. **Antenna diversity** — two SMA-connectorized antennas with an RF switch let the operator place a directional Yagi on SMA-1 and an omnidirectional whip on SMA-2, switching per-frame for the best RSSI. This is invaluable in multi-floor industrial deployments.

4. **KillerBee compatibility** — the USB-CDC interface presents as a standard CDC-ACM serial port and emits the classic KillerBee frame format (channel, RSSI, LQI, timestamp, payload). This means existing tools — `zbstumbler`, `zbdump`, `zbdsniff`, `killerbee Wireshark plugin` — work unmodified. This dramatically lowers the barrier to adoption for researchers already familiar with the KillerBee toolchain.

5. **MicroSD PCAP capture** — every captured frame is written to a FAT32 MicroSD as a standard 802.15.4 pcap (`LINKTYPE_IEEE802_15_4_WITHFCS = 195`). The card is mountable on any host for immediate Wireshark analysis. This is a substantial operational improvement over the original KillerBee, which required a live USB connection.

6. **Install-code brute force on-device** — the CC2652's hardware AES (`AESC` module) accelerates the AES-MMO hash, enabling on-device install-code brute force at ~100 k candidates/sec for 4-byte codes. An 8-byte code space is too large for on-device brute force, so the app offers a "dictionary" mode (curated list of ~200 known vendor install codes) and an "export for offline GPU" mode.

7. **Reactive jamming, not continuous** — the firmware deliberately does not implement continuous-band jamming (which would be a poor operational choice and a legal/ethical hazard). Instead, frame-filtered reactive jamming targets specific devices and appears as RF interference to defenders.

8. **Power discipline** — the SNIFF-ONLY power mode drops backhaul and runs the CC2652 in duty-cycled RX (1 ms wake / 100 ms sleep), dropping current to ~3 mA. This enables ~12-hour covert capture in a drop-and-leave scenario.

---

## Application/Software Interface

### Mobile app (React Native)

The companion app (`app/`) is a React Native application targeting iOS and Android. It connects to ZIGBEE-PHANTOM over encrypted BLE 5.0 (Nordic UART Service-style GATT profile with AES-CTR encryption on top of the link). Screens:

1. **Dashboard** — device status (battery, mode, channel, RSSI), live frame counter, KEY-capture alert.
2. **Channel Scan** — 16-channel energy heatmap, auto-lock channel.
3. **Mesh Topology** — interactive graph of discovered devices (coordinators, routers, end devices), with EUI64, PAN ID, and last-seen timestamp.
4. **Capture View** — live scrollable frame list with decode (frame type, addressing, APS cluster, security level). Filter by frame type / device / PAN.
5. **Key Recovery** — shows captured `Transport-Key` frames, runs install-code brute force, displays recovered TCLK + network key in hex. Export to clipboard / email / SD.
6. **Injection** — pick a target device and a ZCL command (`OnOff Toggle`, `Level Move`, `DoorLock Unlock`), or launch Rogue Coordinator / Rejoin Flood. All injection actions require a hold-to-confirm gesture.
7. **Settings** — channel, PAN ID filter, RSSI threshold, jam filter, capture-to-SD toggle, power mode, firmware update (OTA via Wi-Fi).

### Host interface (USB-CDC)

When connected to a laptop over USB-C, ZIGBEE-PHANTOM presents as `/dev/ttyACM0` (Linux/macOS) or `COMx` (Windows) and speaks a serial-line KillerBee protocol:

```
[ZBPHY]  CH=15 RSSI=-67 LQI=210 FC=0x1234 LEN=42
payload hex...
[ZBPHY]  CH=15 RSSI=-71 LQI=180 FC=0x1235 LEN=18
payload hex...
```

It also streams a binary pcap when the host opens the port in "capture" mode (`AT+PCAP=1`). This is byte-compatible with the original KillerBee `zbdump` output, so `wireshark -k -i /dev/ttyACM0` works out of the box.

### API (for automation)

The mobile app exposes a REST-like API over BLE characteristics for scripted red-team automation. Example: a red-team orchestration script can issue `SET_CHANNEL 15`, `START_SNIFF`, `WAIT_KEY`, `EXTRACT_KEY`, `INJECT_ZCL <device> <cluster> <cmd>` as a sequence.

---

## Use Cases for Red Teams, Security Researchers & Pentesters

### 1. Smart-home mesh key recovery

Drop ZIGBEE-PHANTOM near a target residence using Philips Hue, SmartThings, or Echo-Zigbee. Sniff until a device joins (or trigger a join by power-cycling a battery-powered sensor). Capture the `Transport-Key` frame, brute the install code (many Hue devices still ship with the well-known `ZigbeeAlliance09` TCLK), recover the network key, and decrypt all subsequent mesh traffic — including smart-lock open/close events.

### 2. Industrial building automation audit

In a commercial building with Zigbee-based HVAC and lighting (Siemens, Honeywell), perform a passive 12-hour capture from a covert drop. Map the mesh topology, identify the Trust Center, and report weak-key findings to the facility owner as a remediation finding.

### 3. Rogue coordinator against smart locks

Demonstrate to a client that their Zigbee smart-lock mesh accepts a rogue coordinator: forge a beacon with a stronger signal (via the nRF21540 FEM) than the legitimate coordinator, capture the lock's `Join Request`, issue a forged `Transport-Key`, and toggle the lock's `DoorLock` cluster. This is a high-impact red-team finding for any access-control deployment.

### 4. Selective denial-of-service against a specific sensor

Target a single malfunctioning or suspect sensor (e.g., a motion detector in a sensitive area) with frame-filtered reactive jamming. The sensor misses its ACKs and retransmits, appearing "noisy" to the coordinator without affecting the rest of the mesh — a surgical, low-collateral-impact DoS.

### 5. Cross-channel mesh relay for facility bridging

Demonstrate that two Zigbee meshes on different channels (e.g., channel 15 for lighting, channel 20 for access control) can be bridged by an attacker relay. ZIGBEE-PHANTOM captures on channel 15, re-injects on channel 20, proving that channel separation is not a security boundary.

### 6. Academic protocol research

Security researchers use ZIGBEE-PHANTOM to study new Zigbee 3.0 features (install-code rotation, distributed trust center), develop novel key-recovery attacks, and publish responsibly-disclosed CVEs against coordinator firmware. The open firmware and KillerBee compatibility make it a drop-in research platform.

### 7. Zigbee penetration-testing training

As a teaching tool, ZIGBEE-PHANTOM lets students capture and decrypt real Zigbee traffic in a lab, observe the consequences of weak install codes, and practice injection in a controlled mesh. The mobile app's visualization accelerates learning compared to pure CLI tools.

---

## Bill of Materials

| Ref | Part | Description | Qty |
|-----|------|-------------|-----|
| U1 | CC2652R1F | TI 802.15.4 + BLE SoC, QFN48 | 1 |
| U2 | ESP32-S3-MINI-1 | Espressif Wi-Fi4/BLE5 module | 1 |
| U3 | nRF21540 | Nordic 802.15.4 range extender FEM | 1 |
| U4 | W25Q128JVSIQ | 16 MB SPI flash (capture buffer) | 1 |
| U5 | TP4056 | LiPo charge controller | 1 |
| U6 | MAX17048 | LiPo fuel gauge, I2C | 1 |
| U7 | AP2112K-3.3 | 3.3 V LDO | 1 |
| U8 | AP2112K-1.8 | 1.8 V LDO (CC2652 VDDR) | 1 |
| U9 | SKY13330-397LF | SP3T RF switch (antenna diversity) | 1 |
| DS1 | SH1106 | 1.3" OLED, 128×64, SPI | 1 |
| J1 | USB-C 16-pin | USB 2.0 + 5 V power | 1 |
| J2 | MicroSD socket | Push-push, SPI | 1 |
| J3, J4 | SMA bulkhead | 2.4 GHz antenna connectors | 2 |
| SW1 | 5-way joystick | Analog + center button | 1 |
| LED1-4 | 0603 LED | RX/TX/KEY/ERR status | 4 |
| BAT1 | 1200 mAh LiPo | 3.7 V, JST-PH connector | 1 |
| Y1 | 24 MHz crystal | CC2652 LF clock | 1 |
| Y2 | 32.768 kHz crystal | CC2652 / RTC | 1 |

---

## Legal & Ethical Disclaimer

This device is intended **solely** for authorized security research, penetration testing with explicit written consent, and red-team operations on infrastructure you own or have explicit written permission to assess. Interception, injection, key extraction, jamming, or manipulation of Zigbee/802.15.4 communications on networks you do not own may violate:

- **18 U.S.C. § 1030** (Computer Fraud and Abuse Act)
- **18 U.S.C. § 2511** (Wiretap Act)
- **47 U.S.C. § 301 / FCC Part 15** (unauthorized radio transmission)
- **EU Directive 2013/40/EU** (attacks on information systems)
- **UK Computer Misuse Act 1990**
- Equivalent statutes in your jurisdiction

**The author (jayis1) assumes no liability for misuse.** Always obtain written authorization before deployment. Ensure compliance with local radio-emission regulations. This documentation and design are provided for educational and authorized research purposes only. Responsible disclosure of any vulnerabilities discovered is expected.

---

*Designed by jayis1. Released under CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app).*