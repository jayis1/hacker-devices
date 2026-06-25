# UWB-PHANTOM — Ultra-Wideband Ranging & Digital-Key Manipulation Tool

> **Author:** jayis1
> **License:** SPDX-License-Identifier: GPL-3.0-or-later
> **Status:** Hardware reference design + firmware + companion app

> **⚠️ LEGAL DISCLAIMER:** This device is designed **exclusively** for authorized
> security research, penetration testing with explicit written consent, and red-team
> operations on systems you own or have explicit permission to assess. Interception,
> replay, relay, or manipulation of UWB ranging exchanges on devices you do not own
> (including but not limited to vehicles, key fobs, smartphones, asset trackers, and
> access-control systems) may violate computer-fraud and abuse statutes (e.g.,
> 18 U.S.C. § 1030 CFAA, ECPA), the U.S. Motor Vehicle Theft Law Enforcement Act,
> wireless / radio regulations (FCC Part 15 subpart F for U-NII-5/6, ETSI EN 302 065
> in the EU for SRD UWB), and data-protection law in your jurisdiction. The author
> (**jayis1**) assumes no liability for misuse. **Always obtain proper written
> authorization before deployment, and verify UWB emission compliance locally.**
> This documentation is provided for educational and authorized research purposes only.

---

## 1. Overview

**UWB-PHANTOM** is a portable, battery-powered ultra-wideband (UWB) security research
appliance built around the **Qorvo/Decawave DW3110** IEEE 802.15.4z transceiver and an
**ESP32-S3** host MCU. It is purpose-built to interrogate, capture, relay, and stress the
secure-ranging channel that modern digital keys, asset trackers, and presence systems
rely on for distance bounding.

UWB secure ranging is the trust root for an exploding class of systems:

- **CCC Digital Key** (Apple CarKey, BMW Digital Key, Hyundai/Kia/Genesis Digital Key 2.0)
  — the phone or fob is granted access only if a UWB two-way-ranging (TWR) measurement
  proves the credential is *physically close* to the vehicle.
- **Apple Find My / AirTag** — UWB precision finding is used to locate tags and, on iOS
  17+, to alert a phone that an *unknown* tracker is following it.
- **Smart-home presence** — UWB anchors that map which room a person is in (Aqara,
  SwitchBot, Ubisys, Nuki).
- **Access control** — wave-to-open hotel/office doors and next-gen badges.
- **Payment / POS anti-relay** — contactless payment terminals use UWB to defeat relay
  attacks on the NFC channel.

The entire security model of these systems hinges on a single assumption: **UWB
time-of-flight is physically un-relayable** because light/firmware cannot make a far
device appear near. UWB-PHANTOM is the appliance for stress-testing that assumption
under a controlled, instrumented, and repeatable bench.

### What it does

| Capability | Description |
|---|---|
| **TWR ranging** | Act as a UWB anchor or tag, perform single-sided / double-sided TWR, log distance over time. |
| **STS secure ranging** | Full IEEE 802.15.4z Scrambled Timestamp Sequence (STS) modes — static, dynamic, and advanced — with programmable key/IV. Used to verify that a target enforces STS. |
| **Passive sniffer** | Non-transmitting capture of UWB frames in the 6.5 GHz / 8 GHz bands; decode 802.15.4z MAC, dump STS quality, frame counters, ranging timestamps. |
| **Relay engine** | Two-radio relay that forwards 802.15.4z challenges/responses between a prover and verifier across arbitrary distance — the canonical car-key relay-attack test bench. Includes configurable delay, jitter, and retransmission behaviour. |
| **Distance-shrinking** | Manipulate timestamps inside the relay path so a far prover *appears* 0–10 cm from the verifier. Demonstrates the residual risk in implementations that skip STS or use weak STS. |
| **Tracker scanner** | Detect and locate UWB emitters (AirTags, compatible third-party Find My tags, custom anchors) by their ranging signature and emission schedule. |
| **STS auditor** | Probe a target's STS handling: detect downgrades, replay-acceptance, missing STS enforcement, frame-counter reuse. |
| **BLE orchestration** | Pair with the companion app over BLE GATT for live control, log streaming, and CSV / PCAP export over USB-C. |
| **Power autonomy** | 1000 mAh LiPo, ~6 h sniffing, ~3 h active relay, USB-C PD charging, fuel-gauge telemetry. |

### Form factor

Pocket-sized: 85 × 54 × 14 mm (about the size of two stacked credit cards), 78 g with
battery. Passive-matrix 1.3" OLED, three tactile buttons, USB-C, UWB antenna on the short
edge. Wearable lanyard slot.

---

## 2. Attack Surface & Threat Model

### Systems in scope

The attack surface UWB-PHANTOM exercises is the **802.15.4z secure-ranging layer**
between:

- **Prover** — the credential holder (phone, fob, tag).
- **Verifier** — the relying party (vehicle, door, tracker-locator, payment terminal).
- **Channel** — the 500 MHz-wide UWB impulses exchanged for time-of-flight.

### Why UWB is a *trust root* and therefore a *target*

Modern digital-key standards replaced "the fob transmitted the right rolling code"
with "the fob proved, cryptographically and physically, that it is within N centimetres
of the verifier". The physical proof is a **time-of-flight** measurement:

    d = (t_round - t_reply) * c / 2

Because light is fast (~30 cm/ns), an attacker cannot, in theory, make a far prover
appear close — the round-trip time exposes the extra wire/relay distance. The 802.15.4z
amendment reinforces this with the **Scrambled Timestamp Sequence (STS)**: encrypted
impulse patterns that an attacker cannot predict and therefore cannot pre-emit to mask
relay latency.

The threat model UWB-PHANTOM probes is **where this guarantee breaks down in
practice**.

### Threats instrumented

| ID | Threat | How UWB-PHANTOM exercises it |
|---|---|---|
| T-01 | **No STS** — verifier accepts ranging without STS, so a relay adds latency but no cryptographic check | Relay engine + distance-shrinker produce a "near" distance with arbitrary underlying wire length. |
| T-02 | **Weak STS / static STS** — STS key/IV reused, allowing prediction and pre-computed impulse injection | STS auditor replays captured STS with controlled mutations. |
| T-03 | **STS downgrade** — verifier will fall back to non-STS if prover claims "no STS support" | STS auditor pretends to be a legacy prover and records whether the verifier continues. |
| T-04 | **Frame-counter replay** — no anti-replay on frame counters | Sniffer captures counters; relay replays them at a chosen time. |
| T-05 | **Distance-shortening by timestamp manipulation** — relay path adjusts captured timestamps | Distance-shrinker in relay mode rewrites timestamps before retransmission. |
| T-06 | **Tracker stalking** — unknown UWB tag is following a person | Tracker scanner detects and triangulates emitters by ranging signature. |
| T-07 | **Anchor spoofing** — fake anchors confuse indoor positioning | TWR engine impersonates anchors at chosen coordinates. |
| T-08 | **Side-channel: STS rejection timing** — verifier's "STS invalid" path has different latency than "STS valid" | Sniffer records inter-frame timing to fingerprint verifier firmware. |

### Out of scope

- Breaking the symmetric/asymmetric cryptography of the credential layer itself (that is
  a SE/Secure Element problem; use a separate tool such as `forge-probe`).
- Active RF jamming of the UWB band (we transmit only on channels we are licensed to
  test, with STS disabled when in pure sniff mode).
- Any denial-of-service against a vehicle in motion.

### Trust assumptions

The operator has **written authorisation** for every target verifier and prover. The
device logs every transmission with timestamp, channel, and target identifier, and
refuses to enter transmit mode until an "authorised target" entry is set in the companion
app.

---

## 3. Hardware Specifications

| Subsystem | Part | Notes |
|---|---|---|
| Host MCU | **Espressif ESP32-S3-WROOM-1-N16R8** | 16 MB flash, 8 MB PSRAM, dual-core Xtensa LX7 @ 240 MHz, integrated Wi-Fi 4 + BLE 5.0 (LE + LE Coded). Hosts the companion-app GATT server and runs the DW3110 driver. |
| UWB transceiver | **Qorvo DW3110** (Decawave DW3000 family) | IEEE 802.15.4a/z; channels 5 (6489.6 MHz) and 9 (7988.8 MHz); 4.992 GHz / 1.3 GHz PRF; AES-128 STS engine; SPI host interface. |
| UWB antenna | **Taoglas Knights 6.5–8 GHz UWB chip antenna** | Omnidirectional, 0 dBi typ; matched to both channels. |
| Display | **SSD1306 1.3" 128×64 OLED** (I²C) | Status, mode, ranging telemetry. |
| Input | 3 × tactile buttons (mode / up / down) | |
| Storage | MicroSD (SPI) — optional | PCAP and CSV capture dump. |
| USB | **USB-C, CH224K PD negotiator** | 5 V sink, UART CDC for console, USB Mass Storage for SD. |
| Power | **1000 mAh LiPo**, **MAX17048 fuel gauge** | ~6 h sniff, ~3 h relay. |
| LEDs | RGB status LED (WS2812 mini) + UWB TX LED | |
| Form factor | 85 × 54 × 14 mm PCB in 3D-printed PETG shell | |
| Temperature | 0 – 45 °C operating | |
| Mass | 78 g with battery | |

### Block diagram (text)

```
        +-------------------+      SPI (40 MHz)       +------------------+
        |   ESP32-S3-WROOM  |<------------------------>|   DW3110 UWB     |
        |   host MCU        |                          |   transceiver   |
        |  16M flash/8M PS  |                          |  IEEE 802.15.4z |
        |  BLE 5 / Wi-Fi 4  |                          |   AES-128 STS   |
        +---------+---------+                          +--------+---------+
                  | I²C / GPIO                                   | UWB
                  |                                              v
   +--------------+--------------+                       +-----------------+
   | SSD1306 OLED  | MAX17048 FG |                       |   UWB antenna   |
   | 128x64        |   (I2c)     |                       |  6.5-8 GHz chip |
   +---------------+-------------+                       +-----------------+
                  |
   +--------------+--------------+
   | 3 buttons     | WS2812 LED  |  USB-C --- CH224K PD --- LiPo 1000mAh
   | (mode/up/down)| (status)    |           |
   +---------------+-------------+           +-> CP2102N USB-UART <-> ESP32-S3
                                                   |
                                                   +-> MicroSD (SPI)
```

---

## 4. Architecture & Firmware Design

### Mode state machine

The firmware is a single super-loop driven by a mode FSM:

```
                +-------+      BLE       +-----------+
   companion -->|  IDLE  |<------------->|  CONFIG  |
   app          +---+----+   cmd         +-----------+
                    |
        +-----------+-----------+-----------+----------+
        v           v           v           v          v
    +------+   +--------+   +-------+   +--------+  +--------+
    | TWR  |   | SNIFF  |   | STS   |   | RELAY  |  | TRACK  |
    |anchor|   | (no TX)|   |audit  |   |2-radio |  | scan   |
    +------+   +--------+   +-------+   +--------+  +--------+
```

- **IDLE** — display summary, battery, no TX.
- **CONFIG** — companion-app-driven: set channel, PRF, STS key/IV, authorised-target
  list, capture destination.
- **TWR** — single/double-sided TWR, anchor or tag role, distance streamed to display
  and BLE.
- **SNIFF** — pure RX, frames written to a ring buffer and exported to PCAP on SD.
- **STS-AUDIT** — act as prover against a target verifier with controlled STS behaviour.
- **RELAY** — two logical radios (the DW3110 plus an optional second DW3110 on the
  expansion header) forward frames; delay/jitter/distance-shrink configurable.
- **TRACK-SCAN** — sweep channels and emission schedules, emit detection events.

### Drivers

- `dw3110.c` — SPI host driver; init, channel config, frame TX/RX, STS key load,
  timestamp extraction, antenna-delay calibration.
- `uwb_sniffer.c` — frame filtering, MAC parser, PCAP writer.
- `relay.c` — dual-radio relay path, timestamp manipulation, jitter injection.
- `display.c` — SSD1306 framebuffer, status widgets.
- `ble_comms.c` — GATT service, characteristic protocol, log streaming.

### Build

Cross-compiled with `xtensa-esp32s3-elf-gcc` and **ESP-IDF v5.2**. See `firmware/Makefile`.
`make flash` writes via USB-C CDC; `make monitor` opens the console at 115200 8N1.

### Design decisions

1. **DW3110 over DW1000.** The DW1000 (802.15.4a) has no STS engine; any relay attack
   trivially defeats it. UWB-PHANTOM targets the *current* 4z stack so research mirrors
   what ships today.
2. **Single-radio relay by default, dual-radio expansion.** A single DW3110 can time-
   share between prover-side and verifier-side because each frame is short (~2 ms). A
   second DW3110 on the expansion header gives full duplex and minimum latency for the
   most aggressive distance-shrinking tests.
3. **STS key is operator-loaded, never stored in flash.** On power-down the key zeroises.
4. **No transmission without an authorised target entry.** The BLE app must push a
   signed target record; the firmware verifies an HMAC-SHA256 over the target EUI-64
   before allowing TX mode. This is a tamper-resistant (not tamper-proof) guard against
   casual misuse.
5. **PCAP-compatible capture.** The sniffer writes a LinkType for IEEE 802.15.4 with a
   UWB-PHY metadata TLV, consumable by Wireshark with the 802.15.4 dissector.
6. **Antenna delay self-calibration.** On first boot the device performs a two-device
   loopback (requires a second UWB-PHANTOM) to compute and store antenna delay.

---

## 5. Application / Software Interface

### Companion app (`app/`, React Native)

React Native + TypeScript, Expo workflow, BLE plugin `@jaysrc/uwb-phantom-ble`.

Screens:

- **Connect** — scan for `UWB-PHANTOM-*` BLE advertisements, pair with PIN.
- **Dashboard** — battery, mode, channel, current distance plot.
- **TWR** — choose anchor/tag role, PRF, slot time; live distance chart and CSV export.
- **Sniffer** — channel picker, capture length, file target; in-app frame list.
- **STS Auditor** — pick target EUI, choose audit suite (no-STS, static-STS, downgrade,
  counter-replay); run, view results.
- **Relay** — prover-side and verifier-side channel, delay/jitter sliders, distance-shrink
  target (0–50 cm), armed/disarmed big-button.
- **Tracker Scanner** — live list of detected UWB emitters with RSSI and distance,
  map when multiple UWB-PHANTOMs are co-located.
- **Targets** — authorised-target list (EUI, vehicle make/model, signed HMAC).
- **Settings** — antenna delay, SD card format, firmware version, legal ack checkbox.

### BLE GATT service

```
Service  : 0000FEA5-0000-1000-8000-00805F9B34FB  ("UWB-Phantom")
  Char   : cmd     (write)      — JSON command
  Char   : event   (notify)     — JSON event stream
  Char   : log    (notify)      — raw PCAP chunks
  Char   : target (read/write)  — authorised-target blob (HMAC-signed)
```

### Console (USB-CDC)

A line-oriented REPL exposes the same commands as the BLE `cmd` characteristic, useful
for headless operation: `mode sniff`, `channel 9`, `sts key <hex>`, `relay arm 0.05`,
`track scan 30`, `dump`.

---

## 6. Use Cases

### Red team: relay attack on a digital car key (with owner consent)

1. Operator places one UWB-PHANTOM near the owner's phone/fob (prover-side), another
   near the vehicle door (verifier-side). Both are paired to the companion app.
2. Vehicle issues a 4z ranging challenge. Prover-side UWB-PHANTOM captures it and
   forwards over BLE/Wi-Fi to the verifier-side unit, which re-emits toward the car.
3. The car measures a 5 cm distance; door unlocks.
4. The audit log records the entire exchange, including whether STS was enforced,
   enabling a clean write-up of the residual risk.

### Researcher: STS downgrade audit

Point UWB-PHANTOM at a candidate verifier, run the **STS Auditor** suite. The report
shows whether the verifier ever accepted a frame without STS, accepted a static STS,
or downgraded when the prover claimed "no STS support". Findings feed a CVE or
vendor disclosure.

### Pentester: tracker stalking detection

Carry UWB-PHANTOM in tracker-scan mode for 30 minutes. It detects and triangulates
UWB emitters following the operator (AirTags, compatible third-party Find My tags,
custom anchors), and exports a timestamped track for evidence.

### Defender: anchor-spoofing red-team

Place fake UWB anchors at chosen coordinates to demonstrate that an indoor-positioning
deployment lacks anchor authentication. Drives the requirement for signed anchor
advertisements.

### Educator: UWB protocol teaching

Pure sniff mode produces Wireshark-openable PCAPs of real 802.15.4z traffic, valuable
for teaching the protocol, STS, and ranging math.

---

## 7. Limitations & Future Work

- Single-radio relay has a floor on distance-shrink determined by the radio turn-around
  time (~600 µs ≈ 90 m of "absorbed" light-distance). Dual-radio expansion removes this.
- The DW3110 AES-128 STS engine is fixed-function; advanced STS modes beyond
  802.15.4z-2020 are not supported.
- No support for FiRa MAC layer profiles beyond basic ranging; future firmware may add
  FiRa 2.0 OOB MAC.
- Antenna-delay calibration requires a reference unit.

---

## 8. Bill of Materials (excerpt)

| Ref | Part | Pkg | Source |
|---|---|---|---|
| U1 | ESP32-S3-WROOM-1-N16R8 | module | Mouser |
| U2 | Qorvo DW3110 | QFN-48 | direct / distributor |
| U3 | MAX17048G+T10 | TDFN-8 | Maxim |
| U4 | CH224K | SOP-8 | WCH |
| U5 | CP2102N | QFN-28 | Silicon Labs |
| U6 | W2812B-mini | 3535 | Worldsemi |
| ANT1 | Taoglas.6.5-8 GHz UWB chip | SMD | Taoglas |
| DISP1 | 1.3" SSD1306 OLED | module | generic |
| BT1 | 1000 mAh LiPo, 3.7 V | custom | Panasonic/Panasonic-equivalent |
| J1 | USB-C 16-pin | SMD | Amphenol 12401548E4#2A |
| SW1-3 | tactile switch, 6×6 mm | THT | C&K PTS636 |

---

## 9. Credit

- Hardware, firmware, app, and documentation: **jayis1**.
- Built on prior public research into UWB relay attacks on digital car keys and the
  IEEE 802.15.4z specification; the device is a research appliance, not a finished
  consumer product.

> This project is released under GPL-3.0-or-later. See `LICENSE`. All use is at the
> operator's own risk and responsibility. **Authorized use only.**