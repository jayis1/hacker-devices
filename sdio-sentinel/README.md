# SDIO Sentinel

**Inline SD/SDIO trust firewall and transaction provenance monitor**

**Author and creator: jayis1**

**Hardware revision:** 1.0 design study

**Firmware / host protocol:** 1.0.0 / version 1

**License:** MIT

> **DESIGN STUDY — NOT YET VALIDATED ON PHYSICAL HARDWARE.** The host-simulation firmware and companion application are implemented and locally tested. The electrical design is a detailed prototype specification, but signal integrity, FPGA timing closure, KiCad ERC/DRC, thermal behavior, and interoperability with physical cards and hosts require engineering validation before fabrication or deployment.

## Purpose and overview

SDIO Sentinel is an inline hardware security instrument for studying the trust boundary between an SD or microSD host and a removable memory card or SDIO peripheral. It places two independent electrical endpoints between the host and card, observes commands in both directions, reconstructs card-state transitions, and records policy-relevant events without capturing ordinary sector contents. In its default state, a normally closed bypass path favors availability. Passive-observe mode forwards traffic while reporting timing, command, CRC, state-machine, and access-pattern anomalies. An explicitly armed policy mode can stop selected destructive or identity-changing commands in an authorized test fixture.

The problem is broader than removable storage. Embedded products commonly boot from SD media, import firmware from cards, accept configuration packages, or expose an SDIO slot internally for Wi-Fi and other peripherals. Security teams can inspect filesystem contents after an incident, but they rarely see the command exchange that happened before the operating system mounted the card. A malicious peripheral can abuse SDIO function access; a compromised host can rewrite boot blocks; unstable level shifting can create intermittent corruption; a cloned card can present an unexpected initialization sequence. SDIO Sentinel makes that pre-filesystem behavior observable.

The design is intentionally defensive and bounded. It is not a data-stealing interposer, a covert implant, or an automatic exploitation platform. Raw data blocks are processed only far enough to validate length and CRC and are discarded unless a bench developer deliberately enables a short-lived diagnostic build. The public application exports command metadata rather than sector data. There is no radio. Active enforcement requires both an exact software authorization statement and a 1.5-second physical ARM-button hold. Authorization expires after inactivity and after a maximum fifteen-minute session. Reset, MCU failure, or loss of FPGA heartbeat returns the system to transparent bypass.

This combination is distinct from the repository's eMMC acquisition platform, NVMe/SATA interposers, and general multi-bus tools. SDIO Sentinel's primary security question is: **can a team prove which SD/SDIO commands crossed the removable-media boundary, identify protocol-state anomalies, and enforce narrowly approved safety rules without collecting user content?** Its removable-card form factor, command-state model, boot-region policy, SDIO function-write detection, and privacy-preserving audit format serve that question directly.

## Intended users, scope, and non-goals

The intended users are embedded-device security researchers, product-security teams, digital-forensics laboratories, hardware red teams operating under written scope, and developers debugging SD interoperability. The intended environment is a bench fixture with a host extender cable and a test card. A team should know the target's voltage, maximum clock, boot behavior, and tolerance for added propagation delay before inserting any interposer.

Valid uses include verifying that a secure-boot appliance does not modify its recovery card; proving that a kiosk reads but does not write service media; detecting unexpected CMD52/CMD53 writes to an SDIO function; reproducing card reinitialization faults; or demonstrating that a bootloader's update path touches a prohibited block range. Active blocking is appropriate only when interruption cannot create an unsafe physical condition and the owner has approved the exact policy.

Non-goals are equally important. Revision 1 does not support UHS-II, PCIe/NVMe SD Express, 1.2 V signaling, cards above the 50 MHz SD High Speed profile, content reconstruction, filesystem parsing, password cracking, key extraction, covert capture, command fuzzing against uncontrolled equipment, denial of service, or bypass of access controls. The design does not assert compliance with the SD Association specification, FCC/CE rules, a safety standard, or evidentiary forensic standards. The command tables are based on public protocol behavior and must be reviewed against the applicable licensed specification before commercial work.

## Attack surface

An SD transaction crosses several distinct attack surfaces:

1. **Card initialization and identity.** CMD0, CMD8, ACMD41/CMD1, CID, CSD, relative-address assignment, bus-width selection, and voltage switching establish what the card claims to be. Repeated initialization, illegal ordering, or sudden capability changes can indicate a clone, brownout, signal-integrity defect, or malicious state machine.
2. **Block addressing.** SDSC cards use byte addressing while SDHC/SDXC cards use block addressing. Confusing those modes can turn a narrow rule into the wrong address range. The analyzer tracks capacity mode and normalizes protected ranges before policy evaluation.
3. **Boot and update regions.** Many products place a bootloader, kernel, recovery image, configuration database, or trust anchor near the beginning of media. CMD24/CMD25 writes to the first configurable region receive a high-risk finding and, in enforcement mode, require explicit physical authorization.
4. **Erase, protection, and lock commands.** CMD27 through CMD30, CMD32/CMD33/CMD38, and CMD42 can alter card configuration, write protection, large ranges, or lock state. These commands are unusual in ordinary read-mostly appliances and default to require-arm rules.
5. **SDIO function access.** CMD52 and CMD53 address registers or memory in an SDIO function rather than ordinary storage. The R/W bit and function number are policy inputs. Unexpected writes can reconfigure a network module or another peripheral before the host driver becomes visible to higher-level monitoring.
6. **Electrical and timing behavior.** Excessive clock rates, contention, invalid CRCs, timeouts, voltage-switch attempts, and rapid reset cycles can corrupt data or hide protocol manipulation. The FPGA measures them near the bus rather than relying on host logs.
7. **Management interface.** USB is a command surface. Malformed lengths, replayed packets, unsupported versions, and unauthorized mode changes must fail closed. The firmware uses a bounded packet decoder, CRC32, fixed maximum payload, and no dynamic allocation.
8. **Physical debug and update interfaces.** SWD, FPGA configuration, test pads, and the USB DFU path could replace trusted firmware. Production assembly should lock MCU readout, authenticate updates, disable unneeded debug, and protect the FPGA bitstream. Those production controls are specified but not implemented by the host-simulation build.
9. **Companion computer and exports.** The browser may be compromised, or an operator may accidentally export sensitive metadata. The app excludes raw data, makes export deliberate, and labels demo data. A real deployment should bind USB sessions to the physical device and encrypt any long-term audit archive.
10. **Supply chain.** Counterfeit cards, substituted FPGA/MCU parts, modified connectors, and compromised build tools can invalidate results. A serious forensic workflow needs incoming inspection, signed releases, reproducible builds, serialized calibration, and custody records.

## Threat model

### Assets

Protected assets are the target card's integrity and availability, the confidentiality of card contents, the correctness of the audit record, the authorized policy configuration, the MCU and FPGA firmware, and the availability of the host under test. The design values content minimization: it should answer “what command occurred?” without retaining “what user data was in the sector?”

### Adversaries

A malicious card may return crafted responses, manipulate busy timing, advertise inconsistent capabilities, or expose an unexpected SDIO function. A compromised host may issue destructive writes, reconfigure a peripheral, or attempt to disable monitoring. A local user may try to arm policy mode without engagement authorization. A physically capable adversary may replace firmware, probe test pads, force bypass, or tamper with the audit channel. Environmental faults such as contact bounce, marginal power, EMI, and clock distortion can resemble attacks and are treated as a separate but important adversary class.

### Trust boundaries

The host connector is untrusted. The target-card connector is untrusted. FPGA parsing is a narrow hardware trust boundary and must not accept unbounded lengths. The MCU trusts only the documented FPGA register ABI in `firmware/registers.h`. USB packets are untrusted until their magic, version, length, and CRC pass validation. The browser application is an operator interface, not a root of trust: it cannot synthesize the physical ARM condition. The mechanical bypass is independent of software and is the final availability boundary.

### Abuse cases and mitigations

An operator could use a generic interposer to copy private sectors. SDIO Sentinel mitigates this by omitting sector-content capture from the normal firmware, bounding frame payloads, redacting exports, and documenting a prohibition on unauthorized interception. An operator could create an overbroad block rule and brick a test target. The app validates command ranges and masks, active modes require dual confirmation, dangerous default rules require arm, authorization expires, and the hardware exposes a prominent bypass control. A hostile host could flood commands to exhaust logs. The audit store is a fixed-size ring, counters are saturating in the production design, and forwarding does not depend on USB throughput. A hostile USB client could send oversized frames. The decoder rejects lengths above 256 bytes and resynchronizes without heap allocation.

A malicious card could deliberately generate CRC errors to hide a write. CRC failures raise findings but do not erase the decoded command metadata; enforcement rules operate on bounded command fields. A compromised MCU could manipulate policy. The production roadmap requires signed firmware, readout protection, measured boot of the FPGA image, and a visible status signal driven by independent FPGA logic. A power failure during an active transaction could corrupt media. The bypass relay defaults to its transparent state, but any interposer still adds failure modes; backups and expendable test media are mandatory.

### Residual risk

No inline bridge can be completely transparent. Added capacitance, connector quality, trace skew, and FPGA turnaround timing may break a host-card pair that works directly. Rules derived from incomplete state can create false positives, especially after insertion mid-session. Physical bypass protects availability but cannot undo a partially forwarded write. An attacker with board-level access can likely defeat visible indicators or replace components. The tool supports research decisions; it does not prove that a system is secure.

## Hardware specification

| Subsystem | Selected design | Rationale |
|---|---|---|
| Protocol FPGA | Lattice ECP5 `LFE5U-25F-6BG256C` | Independent host/card I/O banks, sufficient logic and block RAM for command state, timestamp FIFO, CRC engines, and deterministic forwarding at up to 50 MHz |
| Control MCU | STMicroelectronics `STM32U575VIT6`, Cortex-M33 | USB 2.0 FS, TrustZone-capable core, hardware CRC, timers, low-power modes, ample flash/RAM, and a mature embedded tool ecosystem |
| Host interface | Full-size SD extender receptacle, Hirose DM3AT family footprint | Bench connection to an SD host through a short controlled fixture |
| Target interface | microSD push-push socket, Hirose DM3D family footprint | Common removable target-media format |
| USB | USB-C receptacle, USB 2.0 device only, USBLC6-2SC6 ESD protection | Powered management and local console with no radio exposure |
| Clock conditioning | 74LVC2G125-class clock gate/buffer | Controlled clock isolation during an approved fail-safe transition; final part must be timing-qualified |
| Bypass | Normally closed signal relay concept plus direct fixture option | Returns to transparent continuity when control power is absent; the exact multi-pole implementation requires prototype validation |
| Power | USB 5 V to 3.3 V rail; dedicated 1.1 V FPGA core regulator; target 3.3 V sensed, not back-powered | Separates management power from the target bus and prevents accidental card powering |
| Controls | Recessed ARM pushbutton, BYPASS slide control, red/green status LEDs | Requires physical presence and makes active state visible |
| Debug | Tag-Connect SWD and FPGA JTAG pads on assembly side | Development only; depopulate or lock for controlled releases |
| PCB | Four layers, approximately 125 × 75 mm, 1.6 mm FR-4 | Short bus routes over a continuous reference plane; bench rather than covert form factor |

The FPGA receives HOST_CLK, HOST_CMD, HOST_D0..D3 and independently drives CARD_CLK, CARD_CMD, CARD_D0..D3. Direction changes follow SD command and response phases. Each endpoint has series-damping footprints and ESD options close to its connector. The six critical traces are length-controlled within each direction group and routed over an uninterrupted ground plane. The clock is kept away from USB and regulator switch nodes. Revision 1 targets 3.3 V signaling and SD High Speed at 50 MHz; 1.8 V UHS modes are detected and rejected or bypassed because the level and timing architecture has not been qualified for them.

### Power budget

| State | MCU | FPGA/core and I/O | LEDs/misc. | Estimated USB input |
|---|---:|---:|---:|---:|
| Unpowered bypass | 0 mA | 0 mA | 0 mA | 0 mA |
| Safe powered bypass | 8 mA | 15 mA | 2 mA | about 25 mA at 5 V before conversion losses |
| Passive 25 MHz | 18 mA | 90 mA | 3 mA | about 125 mA |
| Passive 50 MHz | 24 mA | 165 mA | 3 mA | about 215 mA |
| Policy activity peak | 35 mA | 230 mA | 8 mA | design allowance of 320 mA |

These are engineering estimates, not measurements. The board is USB-powered and has no battery, reducing charging and storage hazards. It must not source target-card power. Sense resistors and ideal-diode isolation prevent the host and USB rails from back-feeding each other. Thermal testing must confirm FPGA junction margin inside an enclosure.

### Preliminary bill of materials

| Ref. | Part | Package / note |
|---|---|---|
| U1 | LFE5U-25F-6BG256C | caBGA-256; ECP5 FPGA |
| U2 | STM32U575VIT6 | LQFP-100; control MCU |
| U3 | 74LVC2G125 | VSON-8 clock isolation; validate propagation delay |
| U4 | USBLC6-2SC6 | SOT-23-6 USB ESD array |
| U5 | AP2112K-3.3 | SOT-23-5 management regulator; final thermal review required |
| U6 | 1.1 V synchronous buck, final selection pending | Must meet ECP5 transient and sequencing limits |
| J1 | Hirose DM3AT family SD connector | Full-size host fixture interface |
| J2 | Hirose DM3D family microSD connector | Target card socket |
| J3 | GCT USB4105 family | USB-C management connector |
| SW1 | C&K RS282G05A3 or alternate | Recessed physical arm control |
| K1 | Normally closed bypass network | Schematic expresses safety intent; contact count and SI require prototype selection |
| Y1 | 12 MHz ±20 ppm crystal | MCU USB clock source |
| Y2 | 25 MHz oscillator | FPGA reference and timestamp domain |

All manufacturer and distributor availability assumptions are preliminary. The KiCad files use standard-library footprints where possible. A hardware engineer must confirm land patterns, pin mapping, regulator sequencing, BGA escape, connector orientation, relay contact topology, and substitute availability before ordering boards.

## Architecture and block diagram

```text
                     management trust boundary
 USB-C ── ESD ── STM32U575 ── authenticated session policy
                    │ SPI + IRQ        │ bounded audit ring
                    ▼                  ▼
HOST SD ── ESD ── ECP5 dual-endpoint transaction bridge ── ESD ── TARGET microSD
   │          host parser │       │ card parser                 │
   │                      ├─ CRC / state / timing analyzer      │
   │                      └─ deterministic rule comparator      │
   └══════════ normally-closed, unpowered bypass ══════════════┘
                         ▲
                 physical ARM + BYPASS
```

The data plane and control plane are separated. FPGA logic owns cycle-level forwarding, CRC observation, direction control, and a bounded metadata FIFO. The MCU never sits in the critical timing path. It drains metadata, maintains longer-lived card state and counters, evaluates operator/session authorization, loads simple hardware rule comparators, and serves USB. A stalled USB link therefore cannot stall the card bus. If MCU heartbeats stop, hardware clears policy enable and returns to bypass.

The app protocol begins with the bytes `0x53 0x44`, protocol version, packet type, little-endian sequence, little-endian payload length, two reserved bytes, payload, and CRC32. Payloads are limited to 256 bytes. Status reports board identity `0x53445331`, uptime, observed frames, blocked transactions, anomaly count, clock, mode, card state, arm state, and card presence. Protocol version changes are explicit; unknown versions are rejected.

## Firmware details and design decisions

The `firmware/` directory contains portable C11 logic plus a host validation target. `main.c` is both the entry point and a deterministic self-test harness. `drivers/frame.c` implements SD command decoding, CRC7 and CRC16, card-state tracking, command names, risk scoring, boot-region normalization, rapid reinitialization detection, SDIO function access recognition, and privacy-preserving JSON metadata. `drivers/policy.c` implements fixed-size rules, exact scope confirmation, the physical-arm timer, inactivity and session expiry, safe mode transitions, destructive-command defaults, and a 64-entry audit ring. `drivers/transport.c` implements bounded framing and CRC32 without heap allocation. `board.h` fixes limits and logical pins; `registers.h` is the versioned MCU-to-FPGA register contract.

The host target deliberately compiles with `-std=c11 -Wall -Wextra -Werror -pedantic`. It does not pretend to be a complete STM32 board-support package. The portable command, policy, and transport code is compile-checked and tested; production integration still needs STM32Cube startup files, USB device descriptors, SPI/DMA glue, secure boot, watchdog/brownout configuration, and the FPGA gateware. This division prevents untested vendor register literals from being presented as finished hardware firmware.

Important decisions include fixed-size memory, explicit endianness, no custom cryptography, fail-safe transitions, and separating observation from intervention. CRCs detect accidental corruption but do not authenticate a USB peer. Production control sessions should use a vetted library and a device-bound key established during provisioning; inventing a cryptographic protocol is out of scope. Sensitive keys must never be stored in source, example configuration, logs, or exported traces.

The analyzer scores evidence rather than labeling every unusual command an attack. CRC errors score 18, invalid state transitions score 24, a boot-region write scores 45, lock/unlock scores 30, and excessive clock or rapid reinitialization add context. Severity thresholds are informational below 10, notice from 10, warning from 35, and critical from 70. Teams should tune these values using known-good equipment and preserve both raw metadata and the exact policy version when making conclusions.

### Firmware build and validation

```sh
cd firmware
make clean
make check
```

`make check` builds the host target, executes CRC reference vectors, command decoding, state analysis, boot-region findings, authorization expiry, policy behavior, status packet round trips, and corrupt-packet rejection, then runs a safe demonstration trace. The resulting executable is a simulator, not a flashable image. Production firmware should add hardware-in-loop tests for reset during writes, card removal, clock loss, FIFO overflow, USB disconnection, brownout, watchdog recovery, malformed FPGA metadata, and authorization timeout.

## Companion application

The `app/` directory is a responsive React/Vite console suitable for a laptop connected by USB. It includes four real workflows:

* **Overview** presents connection state, command count, anomaly and block totals, clock frequency, card state, physical arm status, and recent findings.
* **Live trace** renders command sequence, timestamp, command name, argument, score, and policy decision. Capture can be paused or cleared without changing device forwarding.
* **Policy** exposes safe bypass, passive observation, and enforcement modes. Active selection remains disabled until authorization is acknowledged, and the device still requires its physical ARM hold. New rules validate command range, action, and an eight-digit hexadecimal argument mask.
* **Audit export** creates a deliberate JSON download containing findings and decisions while stripping any `rawData` field.

A `DemoTransport` produces deterministic-looking local test events and exercises offline, connected, mode-change, policy, and export states without hardware. `WebSerialTransport` contains the permission and connection boundary for future physical integration; browsers prompt the user before exposing a serial port. The production protocol implementation must add packet streaming, reconnect logic, incompatible-firmware handling, device identity verification, and explicit permission-denied messaging.

Build and test the application with:

```sh
cd app
npm install --package-lock=false --no-audit --no-fund
npm test
npm run build
```

The tests verify little-endian status decoding, bounded rule validation, unknown-mode rendering, and removal of raw data during audit export. The build produces static assets in `app/dist/`, which is intentionally ignored. Google Fonts are a presentation enhancement; an offline deployment should bundle fonts locally or accept system-font fallback.

## Safe first-use journey

1. Back up the target card and use an expendable clone for initial work.
2. Confirm that the host uses 3.3 V SD signaling at no more than 50 MHz. Do not insert the device into UHS-II or SD Express equipment.
3. Leave the hardware BYPASS control selected. Connect USB power and confirm the safe-bypass LED pattern.
4. Connect the unpowered host fixture and target card with the orientation checked twice. Keep leads short.
5. Start the app in demo mode first so the operator understands controls and export behavior.
6. Connect the real USB device, select passive observation, and boot a known-good host/card pair. Record initialization and compare it with a direct connection.
7. Review CRC, timeout, state, and clock findings. If errors occur only through the interposer, stop and investigate signal integrity rather than treating them as an attack.
8. Define a narrow policy on paper. Obtain written approval. Only then acknowledge scope, hold the physical ARM button, and enter enforcement mode.
9. Watch the target for unsafe behavior. Return to bypass immediately if the host retries, hangs, or controls a safety-relevant function.
10. Export only the metadata needed for the engagement, document retention, then clear volatile records and disconnect.

## Failure journey and recovery

If USB disconnects, the FPGA continues forwarding and the bounded FIFO may drop old metadata; it must not block the bus because the app is absent. If the MCU heartbeat expires, hardware clears enforcement and asserts bypass. If the FPGA loses lock or reports contention, the MCU records the reason, turns on the red indicator, and requests bypass. If target power is outside the supported range, insertion must be refused. If a rule blocks a command and the host retries continuously, the session should rate-limit duplicate audit records and offer a single physical bypass action.

After any brownout, watchdog reset, or unexplained target corruption, remove the Sentinel, preserve the test card, verify the host with known-good media, and compare the audit metadata to independent logic-analyzer traces. Do not resume enforcement until the root cause is understood. A blocked write may leave a multi-block transaction incomplete; filesystem repair or reimaging can be necessary even though the tool behaved according to policy.

## Use cases

### Product-security and secure-boot validation

A manufacturer can boot an appliance through update, recovery, and factory-reset paths and prove which card blocks were written. The analyzer highlights changes to the protected early-media region and preserves a compact event sequence for regression review. It also reveals whether boot firmware falls back to vendor commands or reinitializes cards unexpectedly.

### SDIO peripheral trust assessment

A researcher can place an SDIO Wi-Fi or laboratory function card behind the bridge and inventory CMD52/CMD53 operations. Register-write policies help answer whether a driver changes security-relevant device state before higher-level monitoring starts. This is not a radio attack: the Sentinel observes the local wired SDIO boundary and should be used only with owned peripherals.

### Forensic provenance

A lab can document that a workstation issued read commands but no writes while triaging removable media. This does not replace a certified write blocker—the hardware is unvalidated—but it offers a research path toward command-level provenance with content-minimized logs. Any evidentiary claim requires calibration, validation, chain of custody, and independent accreditation.

### Red-team control validation

Under written authorization, a red team can test whether a kiosk, camera, printer, industrial panel, or embedded gateway accepts altered removable-media workflows. Rather than deploying a covert payload, the team uses transaction metadata to show when the target reads update areas, modifies media, or accesses an SDIO function. The physical activity indicator and audit trail keep the test visible and attributable.

### Reliability and manufacturing diagnostics

Hardware developers can correlate contact bounce, CRC faults, timeouts, and repeated initialization with a fixture, card batch, regulator, or firmware release. The same state model that identifies suspicious sequences is useful for ordinary interoperability defects. This defensive diagnostic use is expected to be the safest and most frequent deployment.

## Validation status and acceptance criteria

| Area | Status | Evidence / remaining work |
|---|---|---|
| Concept novelty | Reviewed against repository directory and README inventory | No existing SD/SDIO command firewall was found; eMMC acquisition and generic bus tools solve different primary problems |
| Portable firmware | **Locally validated** | C11 host build uses warnings as errors; self-tests and demo complete |
| Firmware size | **Validated** | More than 500 substantive C source lines across `main.c` and three driver modules |
| Companion app | **Locally validated** | Node tests pass and Vite production build completes |
| App physical transport | Partial | Permission boundary exists; binary streaming and real-device pairing remain |
| KiCad syntax | Structurally checked | Files contain real part references, footprints, nets, pads, and routed segments; KiCad CLI was unavailable for ERC/DRC |
| Electrical correctness | **Unvalidated** | Requires schematic review, regulator completion, BGA pin verification, SI simulation, prototype measurements, and safety testing |
| FPGA gateware | Not included in revision 1 | Register ABI and required behavior are specified; HDL implementation and timing closure are future work |
| Physical operation | **Unvalidated** | No board has been fabricated, assembled, or connected to a host/card |

Acceptance for a future prototype requires: verified symbols and footprints; zero unexplained ERC errors; PCB DRC completion; stack-up and impedance review; FPGA pin-bank voltage review; ECP5 timing closure at 50 MHz across voltage and temperature; power-sequencing measurements; USB and ESD review; 72-hour passive traffic testing with multiple hosts/cards; forced reset and removal tests; proof that loss of either processor returns to bypass; and independent review of all enforcement rules.

## Known limitations and future work

The repository does not include FPGA HDL, STM32 vendor startup code, manufacturing outputs, enclosure CAD, calibration procedure, or a certified bypass relay implementation. The schematic uses standard-library symbol identifiers and expresses connectivity intent, but a KiCad installation must resolve the exact libraries. The PCB is a component-and-net placement study with representative routing, not fabrication-ready artwork. BGA fanout, decoupling, core regulator, target-power sensing, series termination, ESD arrays on all card lines, test points, stack-up, and return-path analysis must be completed.

Protocol analysis currently models common base commands but does not fully decode application-specific commands, every response type, tuning data, combo-card behavior, or vendor extensions. Rules are intentionally simple masks. Future work can add named address regions, signed policy bundles, immutable engagement identifiers, calibrated timing histograms, PCAP-NG custom blocks for metadata, and integration with a logic analyzer. Content capture should remain absent by default.

The app uses demo transport for verified behavior. Real hardware support needs the USB framing implementation, device-bound authentication, cancellation-safe Web Serial reads, firmware compatibility checks, reconnection, and encrypted local storage if records persist. Mobile support is possible through a native USB wrapper, but a browser-first desktop application keeps permissions visible and reduces dependency complexity.

## Legal and ethical notice

**Use SDIO Sentinel only on hardware and media you own or for which you have explicit, written authorization.** Inserting an interposer can damage equipment, corrupt data, interrupt services, void warranties, violate contracts, or expose private information. Never use it to intercept another person's data, bypass access controls, deploy malware, disrupt safety-critical equipment, or conceal activity. Define scope, targets, time window, allowed commands, data handling, and recovery responsibilities before testing. Maintain backups and use expendable media.

Laws governing computer access, communications interception, privacy, export controls, evidence handling, and radio/electrical products vary by jurisdiction. This document is technical information, not legal advice. The operator is responsible for obtaining qualified legal and safety review. The author, jayis1, provides this design study without a warranty of fitness, security, non-infringement, regulatory compliance, or physical safety.

## Repository layout

```text
sdio-sentinel/
├── README.md
├── .gitignore
├── firmware/
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── Makefile
│   └── drivers/{frame,policy,transport}.{c,h}
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── package.json
    ├── index.html
    ├── vite.config.js
    ├── src/{App,main,protocol,transport,styles}.*
    └── test/protocol.test.js
```

SDIO Sentinel was conceived, designed, documented, and authored by **jayis1** for authorized hardware-security research.
