# BACnet Phantom — Stealthy BACnet/IP & MS/TP Building-Automation Implant

**Author:** jayis1
**License:** CERN-OHL-W v2 (hardware) / GPLv3 (firmware & app)
**Status:** Open hardware design, security-research use only

> **Ethical / Legal Disclaimer:** This design is provided exclusively for authorized security research, penetration testing, and red-team engagements against building-automation systems you own or have written permission to test. BACnet networks control real-world HVAC, access control, lighting, and life-safety equipment. Unauthorized interception, modification, or disruption of a BACnet network may violate computer-fraud, critical-infrastructure, and public-safety statutes and can cause physical harm. You are responsible for all consequences of using this device. The author (jayis1) disclaims all liability. **Do not use on live life-safety circuits.**

---

## 1. Purpose & Overview

BACnet (ISO 16484-5, ANSI/ASHRAE 135) is the dominant building-automation protocol in commercial and industrial facilities. It rides the building's operational network in two main dialects: **BACnet/IP** (UDP/47808, "0xBAC0") over Ethernet/Wi-Fi, and **BACnet MS/TP** (Master-Slave/Token-Passing) over RS-485 at up to 115200 baud. BACnet controls HVAC plants, lighting, chilled-water and boiler systems, door access, occupancy sensors, and increasingly the energy-management dashboards that feed metering and carbon reporting.

BACnet is also, by historical design, almost entirely unauthenticated. Most production BACnet networks broadcast their entire object database (analog-input, analog-output, analog-value, binary-output, schedule, calendar, trend-log, program, life-safety-point) to anyone who sends a ReadProperty-Request. WriteProperty-Service is enabled by default. Device-object location and description fields routinely leak floor plans, room numbers, and equipment names. There is no transport security, no message integrity, and only an optional (almost universally un-deployed) DataLink layer keying. A 2019 `nmap` sweep of 14,000 internet-facing BACnet devices returned live object lists from a single ReadPropertyMultiple — that attack surface has only grown with cloud-pivot expansions.

The **BACnet Phantom** is a pocket-sized dual-stack implant that bridges the IP and RS-485 dialects while staying invisible to both. It passively maps the building network, dumps every readable object, replays captured setpoints, injects arbitrary writes, impersonates a router (BACnet/IP-to-MSTP or vice versa), and — uniquely — registers as a BBMD (BACnet/IP Broadcast Management Device) foreign-device to capture broadcast traffic across routed segments. All of this from a device the size of a key fob that draws 110 mA and runs for a full shift on a single charge.

The Phantom is **not** a protocol fuzzing hammer; it is a surgical in-band tool designed to look like another controller. It speaks proper BACnet (InvokeID arithmetic, segmentation, confirmed/unconfirmed service routing, I-Am / I-Ack timing, token-pass state-machine for MS/TP) so that building-management workstations log it as a legitimate device rather than an anomaly.

### Design goals

| Goal | Approach |
|---|---|
| Stealth | No broadcast emissions in passive mode; random MAC + instance; tap mode forwards traffic unchanged |
| Dual stack | BACnet/IP (UDP/0xBAC0) **and** MS/TP (RS-485) in one device with a router bridge |
| Field persistence | ~9 h battery; sub-100 mA cruise; passive POE option on Ethernet variant |
| Operational speed | Discovery + full object dump of a 200-device site in < 90 s |
| Safety | Write-protect jumper, external ACK-pending watchdog, life-safety-object blocklist |
| Red-team ergonomics | Phone-first companion app over BLE; no laptop needed for recon phase |

## 2. Attack Surface & Threat Model

**Target**: BACnet/IP and BACnet MS/TP networks in commercial/industrial buildings — the OT/ICS "soft underbelly" that connects HVAC, lighting, access control, and metering to the IT backbone.

**Attack surface**

1. **BACnet/IP (UDP/47808)** — unauthenticated broadcast discovery (`Who-Is/I-Am`), unauthenticated ReadProperty/WriteProperty, BBMD foreign-device registration that forwards broadcasts across subnets, unauthenticated router discovery.
2. **BACnet MS/TP (RS-485)** — 115200 baud token-pass ring; a device that seizes master status can read/write any slave and disrupt the token round-trip (DoS).
3. **Object database** — analog-value setpoints (zone temperature setpoints, duct pressure, hot-water demand), binary-output (fan/valve/pump enable), schedule objects (time-of-day programs), calendar objects, trend-log buffers.
4. **BBMD pivoting** — registering as a foreign device with an existing BBMD lets the Phantom receive broadcast copies from remote subnets, effectively turning a single internal drop into building-wide visibility.
5. **Router impersonation** — announcing a Routed NPDU (network-number 0) from a rogue BACnet/IP→MS/TP router lets the Phantom attract traffic destined for the MS/TP segment.

**Threat model**

* Adversary: authorized red-team operator with at least one network drop or one RS-485 tap point in the target building.
* Defender: passive network monitoring (BACnet-aware NIDS, Wireshark on the BACnet port, periodic device inventory).
* Assumptions: no application-layer keying deployed; `WriteProperty` permitted on target objects; BBMD present on at least one segment; operator has physical or logical access to a BACnet subnet.
* Non-goals: the Phantom does **not** break encryption — if BACnet/SC (Secure Connect, TLS 1.3) is deployed with proper keys, the Phantom degrades to passive sniffer only. It also does not defeat segmented, firewall-isolated "shadow BACnet" builds.

**Safety boundary**: the Phantom refuses to WriteProperty on objects of type `life-safety-point`, `life-safety-zone`, or `pulse-converter` unless the operator physically fits a hardware write-protect bypass jumper and acknowledges the destructive-write prompt in the companion app. This is a hardware-enforced interlock — firmware checks the GPIO and will hard-fail on any attempt to override.

## 3. Hardware Specifications

| Subsystem | Part | Notes |
|---|---|---|
| MCU | Espressif **ESP32-S3-WROOM-1-N16R8** | 16 MB flash, 8 MB PSRAM; dual-core Xtensa LX7 @ 240 MHz; AES/SHA/RSA accelerator used to validate downloaded payloads and BLE pairing |
| Ethernet MAC/PHY | **Wiznet W5500** (hard-wired TCP/IP, 10/100) | SPI @ 40 MHz; supports the broadcast-receive semantics BACnet/IP needs (raw UDP socket promiscuity) |
| RS-485 transceiver | **ISO1500 digital isolator + MAX3491 RS-485 driver** | 2500 V<sub>RMS</sub> isolation, failsafe bias, 115200 baud MS/TP; Phantom taps the segment without breaking the line |
| Power | **TP4056 LiPo charger** + **MP2315 buck** | 2000 mAh 1S LiPo gives ~9 h cruise; USB-C PD passthrough for in-line operation while charging |
| BLE | ESP32-S3 integrated (no external radio) | Companion-app control channel; never emits on BACnet/IP or Wi-Fi unless operator-initiated |
| Status | 0.96" **SSD1306 OLED** + RGB LED | Out-of-view operator feedback; OLED is a privacy feature (does not light up against walls) |
| USB-C | USB-C 2.0 (data + 5 V power) | On-the-field firmware + battery charging; data lines routed through a MAX1352 eFuse for OTG safety |
| Tamper | **ST TS3A4751 tamper loop** + reed switch | Cuts power on enclosure breach; firmware zeroizes object DB and MAC seed |
| Form factor | 52 × 26 × 12 mm PCB inside milled-aluminium tamper case | Key-fob scale; pocketable; aluminium shields BLE RF except through the deliberate window |
| Antennas | Internal chip antenna (BLE), PCB-trace antenna routed into the case window | BLE-only; no external RF emissions indicator |

### Power budget (cruise, passive discovery)

| Rail | Consumer | mA |
|---|---|---|
| 3V3 | ESP32-S3 (continuous, both cores) | 48 |
| 3V3 | W5500 (10M idle + 100M burst) | 32 |
| 3V3 | MAX3491 (RS-485 tap, listening) | 6 |
| 3V3 | SSD1306 (OLED, page-flip every 2 s) | 11 |
| 5V | USB-C PD isolator when present | 3 |
| Total | — | **~110 mA** |

At 110 mA a 2000 mAh / 3.7 V LiPo gives `2000 × 3.7 / (0.110 × 3.3)` ≈ **8.9 h** of operation, comfortably above the "one shift" target.

## 4. Architecture & Block Diagram

```
                                  ┌──────────────────────────────────────┐
                                  │        ESP32-S3 (jayis1 firmware)    │
                                  │                                       │
   USB-C ◄── eFuse ◄── 5V ─────►  │  ┌──────────┐    ┌──────────────┐   │
   (power+dfu)                    │  │ BACnet/IP │ ◄──┤ BACnet NPDU  │   │
                                  │  │  stack   │    │   router     │   │
                                  │  └────┬─────┘    └──────┬───────┘   │
                                  │       │                  │           │
   W5500 ◄── SPI (40 MHz) ─────►  │  ┌────▼─────┐    ┌──────▼──────┐   │
   (UDP/0xBAC0, 10/100)           │  │ discovery│    │  MS/TP FSM   │   │
                                  │  │  / dump  │    │  (RS-485)    │   │
                                  │  │  / replay│    └──────┬──────┘   │
                                  │  └────┬─────┘           │           │
                                  └───────┼─────────────────┼───────────┘
                                          │                 │
                                  ┌───────▼──────┐  ┌───────▼──────────┐
   BLE  ◄── ESP32-S3 ─────────────► │  app bridge  │  │  ISO1500 + MAX3491│
   (companion, no BACnet leak)    │  (NUS-like)  │  │  (RS-485 tap,    │
                                  └──────┬───────┘  │   isolated)      │
                                  ┌──────▼──────┐  └────────┬─────────┘
                                  │  SSD1306    │           │
                                  │  status     │  ┌────────▼─────────┐
                                  └────────────┘  │  RS-485 building │
                                                  │  segment (MS/TP)  │
                                                  └──────────────────┘
```

**Data flow**:

1. **Sniff mode** — BACnet/IP and MS/TP are monitored in parallel; NPDU (Network Protocol Data Unit) frames are normalized and pushed to a ring buffer. The companion app streams a filtered view over BLE.
2. **Discover mode** — Phantom emits `Who-Is` (or MS/TP `Who-Is` token-bus) only on operator command. Replies (`I-Am`) populate a device/object database stored in PSRAM and persisted to flash.
3. **Dump mode** — Phantom issues `ReadPropertyMultiple` against every discovered device for its full object list, then iterates `ReadProperty` for each object's `present-value`, `description`, `units`, `relinquish-default`, and `cov-increment`.
4. **Inject mode** — Phantom issues `WriteProperty-Request` for analog-value / binary-output objects (subject to the hardware interlock).
5. **Bridge mode** — Phantom advertises itself as a BACnet/IP↔MS/TP router (network-number pair), forwarding NPDU frames between the two domains; in this mode it can also impersonate a missing controller to keep workstations quiet.

## 5. Firmware Design

**Author:** jayis1
**Location:** [`firmware/`](firmware/)
**Language:** C99, ESP-IDF v5.1+ (FreeRTOS)

The firmware is organised around six FreeRTOS tasks running on the two ESP32-S3 cores:

| Task | Core | Priority | Function |
|---|---|---|---|
| `bacnet_ip_task` | 0 | 15 | UDP/0xBAC0 socket, confirmed/unconfirmed service dispatch, segmentation reassembly |
| `bacnet_mstp_task` | 1 | 18 | MS/TP token-passing state machine (idle, poll, use-token, done-with-token), 5 ms timer ticks |
| `router_task` | 0 | 12 | NPDU router between IP and MS/TP networks; maintains route table |
| `app_task` | 1 | 9 | BLE Nordic-UART-Service style command channel to companion app |
| `persist_task` | 0 | 5 | Flash-backed ring buffer of captured objects + replay scripts; wear-levelled |
| `watchdog_task` | 1 | 20 | External ACK watchdog: forces revert of any write that does not receive an ACK within N seconds; also enforces the life-safety blocklist |

### Key design decisions

* **Two cores, not one.** BACnet/IP and MS/TP timing budgets are mutually hostile — MS/TP needs deterministic 5 ms ticks for the token round, IP needs high-throughput segmentation reassembly. Pinning each stack to a core with `xTaskCreatePinnedToCore` keeps the token FSM deterministic.
* **No broadcast in passive mode.** The IP stack opens its UDP socket with `IP_HDRINCL` semantics and sets `SO_BROADCAST` only in discover/inject mode; passive sniffing uses raw L2 (via W5500's socket promiscuous mode) and never emits any frame, including ARP. The MAC address is randomised per session and derived from the device-instance seed in PSRAM so the operator can rotate it via the app.
* **MS/TP tap, not break.** The MAX3491 is wired with the failsafe bias sized for a true 1/8-unit-load tap; the Phantom presents a 1/4-unit load to a 32-unit segment, leaving 31 units for legitimate masters. It listens to the token ring without soliciting a master address until the operator issues `MSTP_JOIN`.
* **Hardware-enforced write interlock.** `WRITE_BYPASS_JUMPER` GPIO is read on every `WriteProperty` emission; a 0 means hard-fail. The jumper is a physical pin on the underside of the case that must be installed before power-up — it cannot be hot-plugged, so accidental writes during a teardown are impossible.
* **Acknowledgement watchdog.** Every `WriteProperty` the Phantom issues is registered with the external watchdog task. If a matching `SimpleACK` is not received within `ACK_TIMEOUT_MS` (default 1500 ms), the watchdog reverts the object to its pre-write `relinquish-default` and emits a BACnet `ReinitializeDevice` request with `warmstart` only if operator-confirmed. This prevents runaway writes from leaving equipment in an unsafe state.
* **Tamper zeroization.** A reed switch on the case lid is held low while the case is closed. On transition (lid opened unexpectedly), the firmware immediately: (1) wipes the object DB and seed from PSRAM, (2) issues `esp_partition_erase` on the capture flash region, (3) generates a new random MAC/instance, (4) reboots. The companion app is notified over BLE with a `TAMPER` event.

### Build

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

See [`firmware/Makefile`](firmware/Makefile) for the equivalent `make` wrapper around `idf.py`.

## 6. Application / Software Interface

**Author:** jayis1
**Location:** [`app/`](app/)
**Stack:** React Native 0.73 (Expo SDK 50) + `react-native-ble-plx` for BLE.

The companion app is a phone-first operator console that lets the red-teamer drive the Phantom from the floor without a laptop. Four screens:

1. **Connect** — BLE scan, list `BACNET-PHANTOM-*` peripherals, pair with bonding (display-only passkey).
2. **Recon** — live device/object map; tap a device to expand its object list; long-press to read a single property.
3. **Inject** — analog/binary WriteProperty console with slider + acknowledgement timer; the life-safety blocklist shows a red banner for protected object types.
4. **Settings** — rotate MAC/instance, enable passive sniff / discover / bridge modes, adjust ACK-timeout and COV-cadence, pull the on-board capture log, wipe logs.

A JSON-over-BLE line protocol (`{"op":"whois","low":0,"high":4194303}\n`) keeps the wire protocol auditable and the app code reviewable.

```bash
cd app
npm install
npx expo start
```

## 7. Use Cases

**Red team — building footprint.** Drop the Phantom at any RJ45 in the BACnet/IP segment. Passive sniff builds the full device/object inventory; `ReadPropertyMultiple` against schedules and calendars leaks time-of-day programs; the object `description`/`location` fields frequently leak the building floor plan (e.g. `2F-NORTH-VAV-12`).

**Red team — controller impersonation.** Power-cycle a target controller and seize its MS/TP master address. The Phantom picks up the orphaned token, keeping the workstation silent, then issues its own `WriteProperty` to setpoints to demonstrate lateral physical impact (HVAC setpoint change, lighting schedule override).

**Red team — BBMD pivot.** Register the Phantom as a foreign device on a discovered BBMD. The BBMD forwards broadcast traffic from remote subnets to the Phantom, giving one drop building-wide visibility without ever traversing a router in a way that NIDS would alert on.

**Security researcher — protocol fuzzing.** The `bacnet_ip_task` includes a fuzz hook that replays a `.pcap` of malformed NPDU frames; researchers use this to test BACnet stack robustness in BAS controllers.

**Compliance / blue team.** In read-only mode (jumper removed), the Phantom is a portable BACnet inventory tool for periodic device audits — enumerate, compare to asset DB, flag new device-instances or unauthorised writes since the last sweep.

**Incident response.** The watchdog task can be configured as a "write-guard" — the Phantom listens on the wire and emits `WriteProperty` reverts for any unprotected write it observes, effectively acting as a poor-man's BACnet IDS/IPS during an active incident.

## 8. Bill of Materials (selected)

| Ref | Part | Pkg | Notes |
|---|---|---|---|
| U1 | ESP32-S3-WROOM-1-N16R8 | M.2 keyfob module | 16 MB flash / 8 MB PSRAM |
| U2 | W5500 | QFN-48 | Hard-wired TCP/IP, raw UDP socket promiscuity |
| U3 | ISO1500 | SOIC-16 | 2500 VRMS digital isolator |
| U4 | MAX3491ESA+T | SOIC-8 | 3.3 V RS-485 transceiver, fail-safe bias |
| U5 | TP4056 | SOP-8 | LiPo charger, 1 A |
| U6 | MP2315 | QFN-20 | 3.3 V buck from 5 V |
| U7 | SSD1306 | OLED module | 0.96", I²C |
| U8 | MAX1352 | QFN-12 | USB-C eFuse / OTG safety |
| D1-D3 | LED-R/G/B | 0603 | Status |
| BAT1 | LiPo 2000 mAh | 1S | Internal |

Full BOM with manufacturer part numbers is in the KiCad project.

## 9. What This Device Is Not

* Not a Wi-Fi pentest tool — BLE only; no Wi-Fi radio on the OT network.
* Not a generic RS-485 bus monitor — BACnet MS/TP only; the token FSM is BACnet-specific.
* Not an offensive C2 — there is no exfiltration channel other than the operator's phone over BLE; the Phantom does not beacon to the internet.
* Not a life-safety override — the hardware interlock blocks `life-safety-*` object writes regardless of firmware.

---

*Designed by jayis1. Released for authorized security research and education only.*