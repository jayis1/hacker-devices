# DALI Sentinel — Inline DALI-2 Lighting-Bus Security Auditor

**Creator and author:** jayis1  
**Status:** engineering reference design; prototype validation required  
**License:** see the repository license

> **Authorized use only.** DALI Sentinel is intended for systems you own or have explicit written permission to assess. Lighting can be life-safety infrastructure: changing levels, disabling emergency luminaires, or interrupting a bus can cause injury, panic, regulatory violations, and property damage. Production buses must remain in passive mode unless a documented test plan, rollback procedure, safety observer, and facility authorization permit active testing. Never use this device to interfere with emergency lighting, occupied egress routes, hospitals, transportation systems, or public infrastructure.

## Purpose and overview

DALI Sentinel is a two-ended, fail-safe inline instrument for assessing the security and operational resilience of IEC 62386 Digital Addressable Lighting Interface networks. It records forward and backward frames with precise timing, identifies commissioning and control behavior, detects suspicious or unsafe sequences, and permits tightly constrained fault-injection experiments on an isolated laboratory segment. The device focuses on a neglected boundary: the two-wire lighting control bus between application controllers, sensors, gateways, control gear, and emergency-lighting equipment.

DALI is robust and simple, but classic deployments were designed around physical wiring trust rather than message authentication. A controller can broadcast an arc-power level. Commissioning commands can randomize and readdress gear. Queries can reveal topology and status. Gateways connect that trusted field bus to IP, wireless, cloud, and building-management domains. A compromised gateway, an exposed ceiling cable, a malicious replacement sensor, or an installer tool left connected can therefore affect a large lighting zone without exploiting cryptography. DALI-2 improves interoperability and device definitions, not cable-level authenticity.

Existing DALI commissioning tools usually help installers find and configure luminaires. Generic oscilloscopes show voltage but do not explain protocol state. DALI Sentinel combines an electrically correct high-impedance observer, two independently switchable bus endpoints, a protocol-aware event recorder, and an evidence-oriented application. It answers questions such as:

* Which source initiated commissioning, and did it happen outside a maintenance window?
* Are broadcast OFF, RECALL MAX LEVEL, or direct-arc commands unusually frequent?
* Does a gateway enforce authorization, rate limits, and command allowlists?
* Can malformed timing or repeated collisions wedge control gear?
* Are short addresses duplicated, unstable, or silently reassigned?
* Is emergency-gear traffic isolated and supervised appropriately?
* What changed between the approved baseline and an incident capture?

The instrument has three modes. **Observe** connects only protected sensing circuitry and never drives the field bus. **Inline bridge** closes a hardware bypass relay so the existing cable behaves as a continuous pair while both sides are observed. **Lab isolate** opens that bypass and routes controller-side and gear-side traffic through independently controlled DALI PHYs. Lab isolate requires a local arm button, a fitted authorization jumper, a configured voltage/current limit, and an expiring session. On reset, watchdog expiry, firmware fault, low battery, or loss of host connection, the latching policy returns to the normally closed bypass.

DALI Sentinel is not an internet-connected implant and does not autonomously attack targets. It deliberately excludes wireless command execution on a live bus. USB is the only control transport; optional BLE is telemetry-only and can be depopulated. Captures include raw edge timing, decoded frame fields, direction confidence, collision indicators, voltage minima, policy findings, and operator annotations. The result is suitable for engineering diagnosis and assessment evidence rather than merely a scrolling packet log.

## Attack surface and threat model

### Assets and trust boundaries

The protected assets are lighting availability, safe illumination levels, emergency-lighting behavior, topology and addressing, energy schedules, occupancy-derived information, commissioning state, and confidence in building automation. The primary trust boundary is the exposed two-wire bus. Secondary boundaries are DALI-to-IP gateways, ceiling sensors, room controllers, USB maintenance hosts, firmware update packages, and exported capture files.

The assumed attacker may have temporary physical access to a cable, ceiling void, sensor base, controller cabinet, or service connector; may compromise a networked lighting gateway; or may substitute a peripheral. The attacker can observe traffic, pull the bus low, transmit valid-looking frames, cause collisions, and exploit permissive commissioning. The design does not assume the attacker can break a locked enclosure, extract MCU readout-protected firmware, defeat signed updates, and manipulate both isolated sides simultaneously. Those stronger attackers remain relevant but require separate controls.

### Abuse cases measured

**Unauthenticated command injection.** Standard DALI signaling has no sender identity or message authentication. A correctly timed forward frame may be accepted based solely on address and opcode. Sentinel flags high-impact broadcasts and can verify, on a bench, whether a gateway prevents an unauthorized upstream principal from generating them.

**Commissioning takeover.** INITIALISE, RANDOMISE, SEARCHADDR, COMPARE, PROGRAM SHORT ADDRESS, WITHDRAW, and TERMINATE form a stateful commissioning flow. Unexpected sequences can erase an operational topology or create persistent address changes. The analyzer tracks commissioning state per observed initiator side, checks sequence timing, and marks incomplete or out-of-window sessions.

**Denial of service.** Holding either wire low, transmitting continuously, forcing collisions, or generating malformed half-bit timing can consume the bus. The analog monitor records low-duty ratio, stuck-low duration, frame rate, and recovery. Active tests are hard-limited to a lab-isolated endpoint and stop automatically on current, temperature, or deadline violations.

**Address collision and ambiguity.** Multiple control gear devices may answer one query after bad commissioning or replacement. Backward-frame collisions are inferred from edge violations and analog current/voltage signatures. Sentinel does not claim to identify a physical luminaire solely from a collision; it records the evidence and provides guided isolation steps.

**Gateway pivot.** An IP or wireless gateway can bridge a remote compromise into a physically trusted lighting segment. Sentinel correlates controller-side frames with host-side test actions and computes response latency. It helps determine whether upstream access control translates into meaningful field-bus policy.

**Occupancy leakage.** Queries, sensor events, and scene changes can reveal room use. Captures are sensitive. The application encrypts exports when a passphrase is supplied, redacts operator labels by default, and never uploads data automatically.

**Malicious maintenance host.** USB commands are bounded, framed, and state checked. The host cannot enter active mode unless the physical controls agree. Firmware rejects unknown commands, stale nonces, excessive rates, and active requests when bypass feedback disagrees with the commanded state.

### Explicit limitations

Sentinel is not a standards-compliance certification fixture. Direction inference on a shared DALI pair is probabilistic unless inline-isolated. Analog collision classification requires calibration against the installation. The current revision decodes 16-bit forward and 8-bit backward frames and common 24-bit DALI-2 control-device traffic, but not every vendor extension. It does not bypass locked cabinets, discover building IP credentials, decrypt proprietary gateway traffic, or guarantee that a command is safe. Emergency-lighting tests require specialist procedures and applicable local regulation.

## Hardware specification

| Subsystem | Selected part / implementation | Rationale |
|---|---|---|
| Real-time MCU | STM32G474RET6, Cortex-M4F at 170 MHz, 512 KiB flash, 128 KiB SRAM | Fast comparators/timers, deterministic capture, USB FS, hardware CRC |
| Isolation | 2 × ISO7721 digital isolators plus isolated 5 V modules | Independent controller and gear domains |
| DALI receive | Dual comparator channel per side, 22 V-tolerant divider, hysteresis, TVS and RC filtering | High-impedance voltage and edge sensing |
| DALI transmit | Current-limited optocoupler/MOSFET sink per side | DALI-compatible low assertion without driving a high level |
| Analog evidence | ADS131M04 24-bit simultaneous ADC at 8 kS/s | Bus voltage, sink current, and collision signature capture |
| Bypass | Normally closed DPST signal relay with welded-contact feedback | Field continuity when unpowered or faulted |
| Storage | Industrial microSD, FAT/exFAT capture blocks with per-block CRC | Long captures and removable evidence |
| Host | USB-C device, USB CDC plus vendor bulk endpoint | Deterministic control and high-rate capture export |
| Optional radio | nRF52840 module, telemetry-only build option | Read-only mobile status without field actuation |
| Security | ATECC608B secure element | Device identity and signed manifest verification |
| User controls | Arm button, mode encoder, authorization jumper, RGB status LED, buzzer | Physical consent and unmistakable active-state indication |
| Display | 1.54-inch 240×240 memory LCD | Low-power live bus state and warnings |
| Power | USB-C 5 V or protected 1-cell 2000 mAh LiPo; isolated rails | Portable capture; no power drawn from DALI |
| Protection | SMBJ33CA TVS, resettable fuse, 100 kΩ sense paths, reinforced creepage | Survive wiring transients and installation mistakes |
| Form factor | 118 × 72 × 24 mm enclosure; pluggable terminal blocks | Service-tool dimensions and clear side labeling |

The device never powers the installation bus. DALI nominal voltage is supplied by an installation power supply, and total current is constrained by the system design. Each Sentinel side presents a receive impedance above the minimum design target and a disabled transmitter at boot. The ADC uses separate protected dividers; it is evidence instrumentation, not the digital decision path. A low asserted by the tool is produced by an isolated sink with analog current limiting. Back-to-back MOSFETs prevent phantom conduction when domains are unpowered.

The PCB is a four-layer board: signal and components, uninterrupted ground reference, split power plane, and low-speed signal. Isolation slots separate field A, logic, and field B. The bypass pair is routed directly between terminal blocks and kept away from USB and radio circuitry. Test points expose only protected logic-domain replicas. The enclosure labels `CTRL`, `GEAR`, `BUS`, and `OBSERVE DEFAULT`; interchangeable connectors are intentionally avoided.

## Architecture and block diagram

```text
 Controller / gateway cable                              Gear / sensor cable
       DA+ DA-                                                  DA+ DA-
          |                                                        |
  [TVS + divider + comparator A]                         [TVS + divider + comparator B]
          |                                                        |
  [isolated sink PHY A]                                  [isolated sink PHY B]
          |                                                        |
          +----------- normally-closed DPST bypass relay ----------+
          |                                                        |
     ISO7721 A                                                ISO7721 B
          \                                                        /
           +------ timer capture / direction / collision engine ---+
                                      |
                              STM32G474 trust core
                    +-----------------+------------------+
                    |                 |                  |
              ADS131M04 ADC       microSD logger     ATECC608B
                    |                 |                  |
                    +---------- evidence pipeline -------+
                                      |
                    USB-C CDC/bulk control and export
                                      |
                      DALI Sentinel companion app
```

Timer channels timestamp both edges from each comparator at 2 MHz, giving 0.5 µs resolution. A frame decoder compares edge intervals against the nominal 416.7 µs half-bit and 833.3 µs bit. It derives Manchester symbols without assuming perfect installation timing. Separate adaptive thresholds account for oscillator error but remain bounded so noise cannot train the decoder into accepting arbitrary waveforms. ADC windows around each transition preserve voltage evidence for offline review.

The safety supervisor is independent of protocol decoding. It samples bypass feedback, authorization jumper, arm button, isolated-rail power-good signals, sink current, enclosure temperature, and an active deadline. Any violation disables both sinks before opening or closing other paths. Firmware cannot extend a session indefinitely: an operator must release and re-press ARM after the maximum ten-minute lab window.

## Firmware

Firmware is freestanding C11 organized around small modules. `main.c` owns lifecycle, event processing, command handling, and a deterministic host self-test. `drivers/dali_phy.c` owns edge capture, Manchester decoding, transmit scheduling, and electrical safety state. `drivers/analyzer.c` tracks rates, commissioning state, high-impact commands, duplicate replies, stuck-low conditions, and findings. `drivers/storage.c` writes CRC-protected capture blocks through a replaceable block-device callback. `board.h` defines pin ownership and limits; `registers.h` documents the STM32 register subset without a vendor HAL.

The capture path never allocates memory. Interrupt context pushes compact edge records into a single-producer/single-consumer ring. Main context drains records, runs decoders, and emits immutable `dali_frame_t` objects. Overflow creates an explicit loss event; it never silently continues an apparently complete capture. Every frame stores start time, duration, raw bits, bit count, side, framing quality, collision score, and voltage minimum.

Observe mode is the boot and firmware-update default. Inline mode merely verifies and reports the closed mechanical bypass. Lab isolate follows a guarded transition: validate jumper, require a fresh arm edge, verify both bus voltages, disable sinks, open bypass, verify relay feedback, then permit a selected endpoint. The command parser cannot skip those states. Transmit requests carry an expiry, maximum repetition count, side, and purpose code. Broadcast high-impact commands are denied unless a separate policy flag was enabled while physically armed.

The decoder recognizes the forward-frame address selector, direct arc levels, special commands, queries, and backward replies. It retains unknown opcodes rather than inventing semantics. DALI-2 24-bit frames are captured with instance/device fields for later expansion. Commissioning analysis is sequence based: RANDOMISE without INITIALISE, programming without comparison, an excessive search loop, or commissioning outside the configured maintenance window each produces a distinct finding.

Captures use a binary block format with magic `DALS`, version, sequence, monotonic timestamp, payload length, and CRC-32C. A clean shutdown writes a signed manifest containing device identity, firmware version, block count, and aggregate digest. Power loss may omit the manifest but does not invalidate previously complete blocks. CSV and JSON conversion happens in the app so field firmware remains small and deterministic.

### Build and test

```bash
cd firmware
make clean && make
./build/dali-sentinel --self-test
```

The default host build exercises decoders, policy, storage CRC, ring overflow accounting, and safety transitions. For target integration, set `TARGET=stm32g474` and provide the documented startup/linker files and ARM toolchain. Hardware acceptance additionally requires calibrated DALI loads, isolation hipot testing, relay fault injection, receive-threshold sweeps, and comparison with a certified protocol analyzer.

## Companion application and interface

The React Native TypeScript app is local-first. It communicates through an injected transport interface so Android USB, desktop serial, and replay-file adapters can share the same parser. No cloud SDK is included. The main screens are:

* **Connect** — device identity, firmware version, secure-element status, bypass feedback, and capture storage.
* **Live** — frames per second, bus voltage, low-duty ratio, side attribution, collisions, and a filtered timeline.
* **Findings** — severity-ranked commissioning, availability, broadcast, addressing, and integrity observations with evidence links.
* **Topology** — observed short addresses, device types, response history, and confidence; it does not pretend passive observation is a complete inventory.
* **Lab** — guarded test plans, endpoint selection, repetition and expiry limits. Controls remain disabled unless the device reports local authorization.
* **Export** — JSON/CSV evidence, redaction settings, capture hash, operator notes, and encrypted archive option.

USB messages use COBS-delimited binary envelopes: version, message type, sequence, payload length, payload, and CRC-32C. Status is periodic and read-only. Mutating requests include the current session nonce and receive an explicit accepted or denied result. The UI displays denial reasons directly. The app never changes mode simply because a screen was opened.

A finding is intentionally explainable. For example, `COMMISSIONING_OUTSIDE_WINDOW` links to INITIALISE and subsequent special-command frames, includes timestamps and side confidence, and states the configured window. `BUS_LOW_DUTY_HIGH` reports its one-minute denominator. This avoids opaque “risk scores” that cannot support remediation.

## Practical use cases

### Red teams

A red team can evaluate whether compromise of a building-automation gateway translates into unrestricted lighting control. With the Sentinel passively attached, the team records a baseline and then executes only approved upstream actions. The correlated capture demonstrates exactly which DALI commands reached the bus, whether broadcasts were possible, and whether the controller produced alarms. In a lab replica, the team can measure response to replay, excessive queries, collision patterns, and interrupted commissioning without endangering occupants.

For physical assessments, the instrument can document that an accessible ceiling cable exposes an unauthenticated control plane. The objective is evidence and impact bounding, not surprise darkness. A safe demonstration may address a dedicated test luminaire during a maintenance window while a safety observer confirms that emergency and egress systems are isolated.

### Security researchers

Researchers can compare gateway products, control gear, and sensors under timing mutations. Raw edges and analog snapshots help distinguish a parser discrepancy from electrical failure. The two-ended architecture reveals causality: what the controller side sent, what the gear side observed, and which backward reply followed. Protocol extensions can be added as analyzer tables without changing the safety supervisor.

Sentinel is also useful for fuzz-harness development. A corpus can be built from authorized captures, minimized on a disconnected bench, replayed under bounded current and rate, and annotated with resets or abnormal status replies. Results should be disclosed responsibly to vendors and standards stakeholders.

### Penetration testers and defenders

Testers can add the lighting bus to an OT/building assessment with concrete checks: exposed wiring, gateway authorization, commissioning controls, broadcast handling, segmentation, maintenance logging, duplicate addresses, and recovery after bus faults. Findings map to mitigations such as physical cable protection, gateway network segmentation, strict upstream roles, disabled remote commissioning, alerting on commissioning opcodes, rate limiting, inventory control, and separation of emergency systems.

Defenders can leave Sentinel in observe-only mode during troubleshooting or a controlled incident response window. A baseline capture identifies ordinary scene and query rates. A later deviation can show repeated broadcasts, unexpected commissioning, a stuck-low peripheral, or a failing power supply. Because the bypass is normally closed and observation is high impedance, removal does not require reprogramming the installation.

Installers gain diagnostic value as well: polarity-independent DALI signaling can still suffer voltage drop, excess supply current, poor topology, duplicate addresses, and marginal edges. Voltage and edge histograms help separate cybersecurity concerns from wiring defects. The report should preserve that distinction; malformed frames are not automatically malicious.

## Safe operating procedure

1. Obtain written scope and identify emergency, egress, medical, and occupied-zone dependencies.
2. Review drawings and confirm the selected pair and nominal bus voltage with an approved meter.
3. Power Sentinel from USB or battery before connection; confirm **OBSERVE** and closed-bypass indication.
4. Connect with the installation unchanged and record a baseline. Do not infer side solely from cable labels; verify traffic behavior.
5. Export and hash evidence before any approved active test.
6. Perform active work only on an isolated bench or explicitly approved non-life-safety test zone. Fit the authorization jumper and use a short expiry.
7. Keep a safety observer and immediate physical rollback available.
8. Return to observe mode, verify normal control, remove the instrument, and document all changes.
9. Protect captures as building-security data and erase removable media according to the engagement plan.

## Design status and manufacturing notes

This repository is a design package, not a certified product. The KiCad files contain a connected reference schematic and routed-outline PCB concept with representative footprints and named nets. Before fabrication, an engineer must complete ERC/DRC, validate footprints against current datasheets, calculate creepage for the intended installation category, review DALI loading and isolation, add manufacturing tolerances, and obtain any required EMC, safety, radio, and battery certifications. The BOM should use industrial-temperature components where ceiling or cabinet temperatures demand it.

Critical validation includes unpowered transparency, relay welded-contact detection, no-drive boot behavior, watchdog rollback, receive loading, transient immunity, sink current limiting, thermal cutoff, capture timestamp accuracy, ADC calibration, microSD power-loss behavior, and malformed-frame corpus testing. Emergency-lighting interoperability must be tested only by qualified personnel under the governing standards.

## Repository layout

```text
dali-sentinel/
├── README.md
├── firmware/
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── Makefile
│   └── drivers/
│       ├── dali_phy.c / dali_phy.h
│       ├── analyzer.c / analyzer.h
│       └── storage.c / storage.h
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── App.tsx
    ├── package.json
    ├── tsconfig.json
    └── test/protocol.test.mjs
```

DALI Sentinel was conceived and authored by **jayis1** for authorized security research, resilient building engineering, and accountable field-bus assessment.
