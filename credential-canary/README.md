# Credential Canary — Inline Wiegand/OSDP Security Auditor

**Author and creator:** jayis1  
**Version:** 1.0.0  
**License:** GPL-2.0-only  
**Status:** fabrication-ready reference design; firmware and companion application included

> **Legal and ethical notice:** Credential Canary is intended only for security work on access-control systems that you own or are explicitly authorized in writing to assess. Connecting equipment to a deployed door controller can expose personal data, interrupt life-safety behavior, unlock controlled space, or violate computer-misuse, interception, privacy, trespass, and building-safety laws. Never test an occupied site's egress, fire-door, elevator, medical, detention, or emergency system without the responsible safety authority and a rollback plan. Preserve free egress at all times. The author, **jayis1**, accepts no liability for unauthorized or unsafe use. The lab-test mode must be used on an isolated bench fixture unless the engagement owner has approved the exact test case.

## 1. Purpose and overview

Credential Canary is a portable, two-ended instrument for assessing the cable-level trust boundary between a physical access-control reader and its control panel. It supports the two interfaces found most often at that boundary: legacy Wiegand pulse signaling and bidirectional OSDP over RS-485. It can be attached as a high-impedance monitor, inserted as a transparent bridge, or connected to an isolated bench harness. It records timing and protocol evidence, identifies insecure deployment choices, and makes the difference between a badge-reader problem and a panel-policy problem visible to an assessor.

The novel part is not merely decoding badge numbers. Commodity logic analyzers already display pulses, and generic RS-485 adapters already display bytes. Credential Canary correlates **both sides** of the link. It timestamps what the reader emitted and what the panel received, measures transformations and latency, records supervision and secure-channel state, and applies a local policy engine. This allows a researcher to answer questions that are otherwise surprisingly difficult:

* Is a supposedly upgraded reader still falling back to clear Wiegand?
* Does an OSDP peripheral use Secure Channel, or merely advertise OSDP support?
* Does the panel accept a credential repeated within an implausibly short relay window?
* Does the panel detect a changed OSDP address, missing supervision traffic, malformed parity, or a disconnected reader?
* Is the installation's tamper conductor connected and acted upon?
* Are credentials exposed on the cable even though the upstream access-control server uses TLS?
* Did a field failure originate on the reader side, the panel side, or in cabling?

The tool has two galvanically separated OSDP ports and two independently conditioned Wiegand endpoints. A fail-safe solid-state bypass path is closed by default and remains transparent when the MCU is reset or unpowered. Active forwarding is never the boot default. Monitor mode samples without driving either field side. Bridge mode opens the passive bypass only after firmware self-test and allows validated frames to cross the isolation boundary. Lab-test mode exposes diagnostic controls but requires a physical authorization input in addition to the app command. This pairing prevents an accidental touchscreen action from turning a passive survey into an intrusive test.

Credential Canary is not a badge-cloning appliance and does not attempt to recover OSDP Secure Channel keys. Captured credential values are shown partially redacted by default in the app. Its practical purpose is to produce defensible evidence: protocol, timing, parity, secure-state, wiring, supervision, and tamper findings that can be placed in an assessment report and replayed against a safe simulator.

## 2. Why this device is useful

Physical access deployments often span several generations. A modern mobile credential reader may connect to a new intelligent controller over wiring inherited from a 1990s Wiegand installation. Sales documentation may call the installation “encrypted” because the credential technology or server connection uses cryptography, while the final reader cable still carries a clear, replayable number. Conversely, a site may have OSDP wiring but no Secure Channel, a default installation key, or permissive fallback behavior. Network scanners cannot observe this layer and controller logs normally omit electrical details.

Existing specialist OSDP tools generally act as a control panel or peripheral. Existing Wiegand tools tend to be one-sided readers, emulators, or pulse generators. Credential Canary's matched endpoints provide a causality record. Each event has a 1 µs timestamp, interface identity, sequence number, integrity flags, and bounded data field. Wiegand edges are retained as timing events and assembled into frames. OSDP frames are checked before policy analysis. The app shows a unified timeline and exports a JSON working copy with device identity, author metadata, status counters, and ordered events.

The design is useful for defensive commissioning too. Installers can verify D0/D1 polarity, pulse width, inter-pulse spacing, RS-485 A/B orientation, address consistency, checksums, CRC use, bus turnaround, and tamper continuity without bringing a full access-control server online. Security teams can establish a baseline during commissioning and compare it after maintenance.

## 3. Attack surface and threat model

### 3.1 Assets under examination

The primary assets are credential identifiers, access decisions, reader configuration, keypad input, biometric match results, controller trust assumptions, tamper signals, and service availability. Wiegand usually protects none of these at the cable layer. OSDP can protect sensitive messages with Secure Channel, but deployment quality determines whether that protection is effective.

The reader cable frequently leaves a protected controller enclosure and travels through walls to equipment accessible from a public side of a boundary. An adversary may remove a reader, reach conductors in a ceiling or junction box, substitute a peripheral, bridge traffic through another medium, or induce electrical faults. Credential Canary models an attacker with temporary physical access to that cable but no access-control database credentials. It also models an insider or installer capable of changing wiring or peripheral configuration.

### 3.2 Risks the instrument can reveal

**Clear credential exposure.** A Wiegand credential is represented by timed D0 and D1 pulses. Anyone with electrical access can observe it. The policy engine identifies common 26- and 34-bit formats, checks their parity, and records unknown lengths without pretending that unknown means invalid.

**Replay and relay acceptance.** The firmware keeps a bounded recent-credential table. A repeated payload inside a configurable short window is raised as a replay-window observation. This is not proof of attack—users can present a badge twice—but it provides a reproducible trigger for controlled anti-replay tests.

**OSDP without Secure Channel.** OSDP transport alone does not guarantee confidentiality or authenticity. Credential-bearing replies in clear frames are reported. The auditor also records CRC/checksum validity and whether the secure-block flag is present. It never labels an encrypted payload as decrypted.

**Address instability and peripheral substitution.** An address change on a side that has established a baseline is reported. This supports controlled tests of whether the panel notices substitution or unexpected multidrop behavior.

**Parser robustness.** On an isolated fixture, malformed lengths, integrity trailers, sequence values, and timeouts can test a simulator or authorized controller. The shipping firmware only parses and forwards valid frames in bridge mode; test generation is intentionally separated from passive collection and physically gated.

**Electrical denial of service.** Open, shorted, stuck-low, reversed, or unterminated lines can make a reader disappear. The hardware provides protected high-impedance sensing and independent sides so the assessor can localize the fault. It is not rated to interrupt emergency egress circuits.

**Tamper blindness.** The enclosure switch and field tamper input produce explicit events. This helps verify that the physical installation and panel policy both react to disturbance rather than merely having an unused wire.

### 3.3 Trust boundaries

The STM32G474 is the real-time trust anchor. It owns capture timestamps, frame validation, bypass control, and the policy ring buffer. The nRF52840 module is a transport coprocessor and is not allowed to directly drive field buses. A compromised phone can request a mode change, but the field output path still requires firmware state and, for lab mode, physical authorization. OSDP sides cross separate ADM2483 digital isolation barriers. USB is ESD protected and intended as the preferred evidentiary interface.

The companion app is an operator console, not an access-control decision engine. Exported JSON is a working copy, not a court-grade signed forensic image. A production evidence workflow should stream to a host that hashes and signs captures, document clock calibration, and preserve chain of custody.

### 3.4 Out of scope

Credential Canary does not defeat smart-card cryptography, recover biometric templates, attack an access-control server, open a door by itself, discover a site remotely, or guarantee code compliance. It does not make a live safety system safe to fuzz. Secure Channel cryptanalysis and credential manufacture are explicitly outside the design objective.

## 4. Hardware specification

| Subsystem | Selection | Design role |
|---|---|---|
| Main MCU | STM32G474CEU6, Cortex-M4F at 170 MHz, 512 KiB flash, 128 KiB SRAM | Edge timing, dual UART, policy engine, USB device, watchdog |
| Radio module | Raytac MDBT50Q-1MV2 / nRF52840 | BLE 5 authenticated transport; field buses cannot be driven directly |
| OSDP interfaces | 2 × ADM2483BRWZ isolated RS-485 transceivers | Independent reader-side and panel-side observation/bridge ports |
| Wiegand receive | LMV339 comparators, resistor clamps, TVS, selectable thresholds | Four protected high-impedance D0/D1 sensing channels |
| Wiegand forwarding | Open-drain MOSFET stages behind PhotoMOS isolation | Controlled pulse mirroring with no injected high level |
| Fail-safe path | TLP3543 PhotoMOS channels | Normally transparent reader-to-panel path during reset or power loss |
| Timing | 32-bit 1 MHz free-running timer, timer capture inputs | Microsecond event timestamps and pulse qualification |
| Storage | 1,024-event SRAM ring; optional SPI NOR footprint | Deterministic bounded collection without filesystem stalls |
| USB | USB-C device, USBLC6-2SC6 ESD protection | Power, CDC/vendor protocol, bench collection and firmware update |
| Power | 5 V USB or protected 9–24 V field input; isolated 3.3 V rails | Bench and installation-survey operation |
| User controls | Recessed authorization button, RGB LED, tamper switch | Physical mode consent and visible state |
| Connectors | Two keyed 6-position terminal blocks | Reader and panel: D0, D1, RS-485 A, RS-485 B, ground, sense/power |
| PCB | 100 × 60 mm, 4-layer recommended, 1.6 mm FR-4 | Ground return integrity and isolation keep-out |
| Enclosure | 112 × 68 × 24 mm polycarbonate | Clip points, labeled sides, recessed active-mode control |

The schematic names every important interface net. The PCB contains assigned footprints, copper nets, routed representative links, terminal blocks, USB-C, MCU, radio, and isolated transceivers. A four-layer production spin should use solid internal ground and power planes, maintain the ADM2483 isolation clearance, add stitching vias around USB, and keep the nRF module antenna keep-out free of copper and enclosure metal. The checked-in board is a reference layout, not a substitute for manufacturer-specific DFM, creepage, EMC, thermal, and safety review.

### 4.1 Electrical behavior

Wiegand inputs are never pulled up to the tool's internal rail while monitoring a powered target. Comparators observe the field pull-ups through high-value dividers and clamps. The output stages are open-drain because a Wiegand transmitter asserts a zero or one by pulling the corresponding conductor low. No push-pull output may be fitted. Pulse rejection below 15 µs suppresses contact and EMC spikes while retaining normal reader pulses.

RS-485 termination is switchable and off by default so inserting the device does not create a third termination. Bias is also off by default. The two transceivers permit side-specific receive and controlled retransmission rather than shorting two buses together. This is essential for measuring turnaround and for reverting to bypass if software health fails.

The bypass control is arranged so reset, watchdog expiry, brownout, and an unpowered unit restore the non-invasive path. Firmware has 100 ms to feed the independent watchdog. An active mode is abandoned after an internal health failure. LED meanings are fixed: green is passive/healthy, blue is armed capture, amber is dropped-event warning, and red is tamper or fatal fault.

## 5. Architecture

```text
       READER SIDE                                                PANEL SIDE
 ┌──────────────────┐                                      ┌──────────────────┐
 │ D0/D1 ─ protection ─ comparators ─┐          ┌─ isolated open-drain ─ D0/D1 │
 │ OSDP A/B ─ ADM2483 isolated PHY ──┼──────────┼─ ADM2483 isolated PHY ─ A/B │
 │ tamper/power sense ───────────────┘          └──────────── power/tamper    │
 └─────────┬────────┘                                      └────────┬─────────┘
           │        normally-transparent PhotoMOS bypass             │
           └───────────────────────┬─────────────────────────────────┘
                                   │
                         ┌─────────▼──────────┐
                         │ STM32G474          │
                         │ edge capture + DMA │
                         │ Wiegand assembler  │
                         │ OSDP parser/CRC    │
                         │ policy + ring      │
                         │ watchdog/interlock │
                         └──────┬───────┬─────┘
                                │       │
                          USB-C │       │ UART
                                │  ┌────▼─────────┐
                                └──┤ nRF52840 BLE │
                                   └────┬─────────┘
                                        │ authenticated BLE
                              ┌─────────▼──────────┐
                              │ React Native app   │
                              │ timeline/findings  │
                              │ modes/export       │
                              └────────────────────┘
```

Events flow from interrupt-level byte or edge intake to bounded protocol state machines. Completed frames enter the capture ring and policy observer. The command service drains events to USB/BLE using a CRC-protected binary envelope. Backpressure never blocks field parsing; if the ring fills, the firmware increments a drop counter and raises the amber condition rather than silently overwriting old evidence.

## 6. Firmware

The firmware is freestanding C11 and does not require a vendor HAL. `registers.h` defines the STM32 register subset used by the board layer, making peripheral ownership auditable. `board.c` initializes GPIO, timer, UARTs, watchdog, LED, bypass, USB transport hooks, and RS-485 direction. `protocol.c` contains independent Wiegand and OSDP state machines. `capture.c` implements the event ring and defensive rules. `command.c` implements a small CRC-protected control protocol. `main.c` owns lifecycle, status, tamper transitions, indicator state, and IRQ ingress.

Wiegand capture retains up to 64 bits. An edge that occurs too close to the previous edge is counted as a glitch and does not alter the frame. A 25 ms quiet period terminates a frame. Known 26- and 34-bit layouts receive parity checks; other lengths remain available for analysis. Credential bytes are stored only in volatile capture memory in this revision.

The OSDP decoder requires the `0x53` start-of-message byte, validates the little-endian declared length, bounds the frame at 256 bytes, supports checksum and CRC trailers, extracts control and secure-block state, and times out a partial frame after 5 ms. Only structurally decoded, integrity-valid frames can cross the software bridge. Production firmware should complete a formal review against the exact SIA OSDP revision used by the target environment.

Policy observations are intentionally explainable. Each alert includes a code and compact context rather than an opaque score. Current rules cover parity failure, repeated credential timing, clear sensitive OSDP replies, malformed integrity checks, address change, line fault placeholders, and enclosure tamper. Thresholds are starting points for a controlled assessment, not universal declarations of compromise.

The command envelope is:

```text
CA | command/type | payload_length | payload[0..58] | CRC16-IBM little-endian
```

Commands request information, status, capture start/stop, operating mode, evidence markers, and policy reset. Streamed event packets use type `0xF0`. Mode transitions to bridge or lab mode remain subject to hardware interlock policy.

Build with an Arm GNU Embedded toolchain:

```bash
cd firmware
make check             # host compiler syntax and warning check
make                   # credential-canary.elf and .bin
make size
make flash             # ST-Link/OpenOCD, only on the intended board
```

The linker script targets 512 KiB flash at `0x08000000` and 128 KiB SRAM at `0x20000000`. Before field use, add MCU option-byte policy, signed update verification, BLE bonding storage, USB descriptors, production clock calibration, and hardware-in-loop regression tests. Those productization items do not change the documented architecture but should not be skipped for a trusted assessment instrument.

## 7. Companion application

The `app/` directory contains a React Native/Expo development-build application. It scans only for the Credential Canary service UUID, connects, subscribes to streamed events, validates packet CRC, decodes status and events, and provides three connected views:

* **Overview** shows mode, capture state, alerts, drops, mode controls, session start/stop, and textual evidence markers.
* **Capture** filters a 2,000-event mobile window by Wiegand, OSDP, or alerts. Credential display is reduced to a trailing fragment to discourage unnecessary personal-data exposure.
* **Findings** lists policy alerts, allows an explicit baseline reset, and exports an ordered JSON working copy through the platform share sheet.

The disconnected view scans, lists device identifiers and RSSI, and makes transport limitations visible. Android Bluetooth scan/connect permissions and the iOS usage description are declared. Because `react-native-ble-plx` uses native code, run an Expo development build rather than Expo Go.

```bash
cd app
npm install
npm test
npx expo run:android     # or: npx expo run:ios
```

The pure JavaScript protocol module has tests for command round trips, corruption rejection, alert decoding, and status decoding. For production, add authenticated user roles, encrypted-at-rest exports, device certificate pinning, a privacy retention setting, PCAP-style export, and a desktop collector for long surveys.

## 8. Field workflow

1. Obtain written scope naming the building, controller, reader, date, acceptable interruption, and emergency contact.
2. Confirm that the tested opening is not relied upon for emergency egress or another life-safety function. Arrange a safety observer where required.
3. Photograph and label existing conductors. Record supply voltage and determine whether the interface is Wiegand, OSDP, or both.
4. Power Credential Canary from USB with field outputs in safe bypass. Confirm green state and zero drop count.
5. Connect ground/reference first, then passive sense conductors. For RS-485, leave termination and bias disabled unless the bus design specifically calls for them.
6. Use monitor mode to establish a baseline. Present dedicated test credentials rather than employee credentials whenever possible.
7. Record evidence markers before each approved test case. Observe parity, format length, Secure Channel, address, timing, supervision, and tamper results.
8. Use bridge or lab mode only when its behavior appears in the approved test plan. Keep a means of immediate physical rollback.
9. Return to safe bypass before disconnecting. Restore original wiring, function-test the opening with the site owner, and account for all adapters.
10. Export, minimize, encrypt, and retain capture data under the engagement's evidence and privacy rules.

## 9. Use cases

### Red teams

A red team can demonstrate the difference between perimeter appearance and cable-layer trust without touching the production access database. A controlled exercise might show that a modern reader emits a clear Wiegand value, that the same test credential is accepted after rapid repetition, or that removing a supervised OSDP peripheral causes no useful alarm. Correlated reader/panel timestamps help demonstrate relay feasibility while keeping the experiment measurable. The fail-safe default and evidence markers make the activity easier to coordinate with a white team.

### Penetration testers

A tester can include the reader-controller interface in an access-control assessment rather than limiting work to web applications and server networks. Credential Canary produces concrete findings: observed bit length, parity behavior, clear or protected OSDP messages, integrity failures, address stability, capture loss, and tamper transitions. Those findings map directly to remediation such as migrating from Wiegand, enabling OSDP Secure Channel with managed keys, removing fallback, supervising communication, protecting controller-side wiring, and monitoring enclosure tamper.

### Security researchers

Researchers can characterize proprietary formats, electrical tolerance, state-machine recovery, and interoperability on isolated fixtures. The source is small enough to instrument, and both protocol decoders have explicit bounds. A simulator can feed recorded timing into panel test equipment without placing a live door at risk. The two-ended topology is particularly useful for studying forwarding latency, timeout behavior, and downgrade transitions.

### Defenders and installers

Blue teams can baseline critical doors, detect undocumented Wiegand remnants, verify secure-channel rollout, and investigate intermittent faults. Installers can distinguish polarity, termination, or cable issues from controller configuration. A comparison capture before and after maintenance provides more useful assurance than a single successful badge presentation.

## 10. Design decisions and limitations

The STM32G474 was selected instead of a Linux SBC because deterministic timing, fast startup, low power, and a narrow software attack surface matter more than local graphics. It has enough timers, comparators, UART capability, USB, RAM, and performance for both protocols without an FPGA. The BLE radio is separate so radio firmware cannot directly toggle field outputs. Dual isolated RS-485 interfaces cost more than one shared transceiver but preserve the central two-sided measurement feature.

The first revision uses volatile capture by default. This is a privacy decision as much as a cost decision: badge data should not silently accumulate on a misplaced device. Long-term captures belong on an authenticated collector under explicit retention rules. An optional SPI NOR can hold signed firmware assets or a carefully encrypted queue.

The reference PCB still needs a normal hardware release process: schematic ERC, PCB DRC, isolation review, stack-up selection, impedance/EMC review, BOM lifecycle check, prototype bring-up, fault injection, watchdog verification, temperature testing, and compliance assessment. Terminal pin order must be checked against the actual installation; “A” and “B” naming is not consistent across RS-485 vendors. The design is not intrinsically safe, weatherproof, UL-listed, or approved as part of a fire or access-control system.

## 11. Repository layout

```text
credential-canary/
├── README.md
├── firmware/
│   ├── main.c, board.c, board.h, registers.h
│   ├── protocol.c, protocol.h
│   ├── capture.c, capture.h
│   ├── command.c, command.h
│   ├── startup.c, linker.ld, Makefile
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── App.js, app.json, babel.config.js, package.json
    ├── src/protocol.js
    └── test/protocol.test.mjs
```

All original design, firmware, application code, and documentation in this device directory credit **jayis1** as author and creator. Contributions should preserve that attribution and the authorized-use warning.
