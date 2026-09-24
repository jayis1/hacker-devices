# FlexRay Sentinel — Passive Dual-Channel Schedule and Trust Analyzer

Author and creator: jayis1
Version: 0.1.0
License: MIT firmware/application; CERN-OHL-S-2.0 hardware design
Status: engineering reference design; firmware host build verified; fabrication, EMC, FlexRay conformance, KiCad ERC, and KiCad DRC still require laboratory validation

## Legal and ethical disclaimer

FlexRay Sentinel is intended only for defensive engineering, authorized security research, owned-asset diagnostics, controlled automotive test benches, and penetration tests conducted under written rules of engagement. FlexRay networks can carry steering, braking, propulsion, restraint, and other safety-critical messages. Never connect this device to a vehicle on a public road, to a moving vehicle, or to equipment whose unexpected behavior could injure a person or damage property. Do not monitor systems that you do not own or lack explicit permission to assess. Follow local computer-access, privacy, vehicle-safety, export-control, and electromagnetic-compatibility law. The design deliberately defaults to receive-only operation and does not implement arbitrary frame transmission. The author, jayis1, provides this design without warranty and accepts no liability for misuse. Keep experimental work on an isolated bench with emergency power removal and an independent safety observer.

## Purpose and overview

FlexRay Sentinel is a compact, isolated, two-channel observer for the deterministic FlexRay networks found in safety-critical vehicles, industrial motion platforms, and legacy x-by-wire research systems. It records both FlexRay physical channels with a common clock, reconstructs frame metadata, learns the cluster schedule, and identifies deviations that ordinary packet capture misses: a slot arriving at the wrong point in the communication cycle, a payload length changing without a configuration update, a normally redundant frame disappearing from one channel, an unexpected startup or sync frame, or a new slot appearing after a baseline has been frozen.

The practical problem is not simply decoding a frame. FlexRay is time-triggered. A legitimate-looking payload can still be suspicious because it arrives in an unauthorized slot, cycle, channel, or timing window. General-purpose logic analyzers expose edges but do not understand the cluster schedule. Automotive interface boxes commonly provide capture, but many hide timing detail behind proprietary software and are priced for production validation laboratories. CAN tools do not transfer: CAN arbitration is event-driven, while FlexRay divides each communication cycle into static and dynamic segments governed by a cluster configuration. FlexRay Sentinel focuses on this semantic gap. It treats timing and schedule membership as security evidence.

The instrument is passive by design. Two high-impedance receive paths connect to channels A and B through automotive-qualified protection and FlexRay receivers. A Lattice iCE40UP5K FPGA timestamps decoded symbol events against an 80 MHz disciplined counter and streams bounded records to an STM32H743 microcontroller. The microcontroller validates frame and header CRCs, learns slot profiles, scores anomalies, stores privacy-preserving evidence, and exposes a USB interface to the companion application. A normally closed bypass relay is not used to bridge traffic; instead, it disconnects the monitoring stubs when the device loses power or detects an internal fault. No transmitter pins are routed from the MCU or FPGA to the bus connectors on revision A0.

This receive-only architecture is the primary novelty claim: the device correlates dual-channel physical observations with an explicit learned FlexRay schedule, while enforcing a hardware boundary that prevents the analysis stack from transmitting onto the observed network. That combination makes it suitable for proving that an assessment sensor cannot become an accidental injector.

## Who benefits and what workflow improves

A red team can place FlexRay Sentinel on a laboratory harness before an exercise, learn the normal schedule during a clean drive-cycle simulation, freeze the baseline, and then record anomalies while another authorized tool exercises gateways and diagnostic paths. The Sentinel supplies independent evidence of whether the exercise changed safety-bus behavior. It does not need to be the offensive tool.

A blue team or vehicle security operations group can use it as a portable validation sensor when investigating intermittent network faults or suspected unauthorized ECU replacement. A new ECU revision may preserve application payloads but alter sync behavior, redundancy, or slot timing. Those changes become visible without reverse-engineering every signal bit.

A security researcher can compare firmware versions, gateway configurations, or fault-injection experiments with repeatable schedule fingerprints. A functional-safety engineer can use the same evidence to distinguish bus-level degradation from application-level failures. An educator can demonstrate static slots, cycle counters, startup frames, CRC behavior, and redundant channels on an isolated cluster without giving students an unrestricted transmitter.

Compared with a generic oscilloscope, the device produces searchable frame and schedule evidence. Compared with a CAN interface, it understands FlexRay timing. Compared with a commercial development interface, it is open, inexpensive, privacy-conscious, and physically receive-only. Estimated one-off build cost is USD 145–210 depending on isolated power and connector choices; the dominant costs are the two automotive FlexRay receivers, isolated power module, FPGA, four-layer PCB, and rugged connectors.

## Attack surface

The monitored environment presents several distinct attack surfaces.

1. Physical access to an exposed harness lets an adversary attach an unauthorized node, replace an ECU, disturb termination, or create channel-specific attenuation.
2. A compromised gateway can emit syntactically valid frames in reserved or unused slots, modify payload length, alter cycle repetition, or abuse startup and synchronization behavior.
3. A malicious or faulty ECU can drift within a slot until it reaches a receiver tolerance boundary, producing intermittent failures that are difficult to reproduce.
4. Channel redundancy can be degraded selectively. Traffic may remain available on channel A while channel B is suppressed, delayed, or electrically distorted, reducing fault tolerance without causing an immediate outage.
5. Diagnostic or bootloader transitions can legitimately change schedules. An attacker may imitate those transitions, so context and authorization matter.
6. The monitoring device itself can be attacked through malformed capture records, oversized payloads, USB messages, hostile configuration values, storage exhaustion, or physical tampering.
7. Supply-chain substitution is relevant: receiver variants, oscillator quality, isolation ratings, and connector pinouts materially affect measurement fidelity and safety.

FlexRay Sentinel does not claim to identify the human or ECU responsible for every anomaly. It produces timestamped, integrity-checked observations that support an investigation. A new slot may be an intrusion, a software update, a diagnostic session, or a configuration mistake. The operator must correlate events with test plans and vehicle state.

## Threat model

The trusted computing base contains the immutable boot root, signed firmware, FPGA bitstream digest, configuration parser, capture decoder, anomaly engine, and evidence signer. The host phone or laptop is not trusted to alter historical evidence. The host may request a new baseline or configuration, but changes are staged until the physical confirmation button is pressed. USB is treated as hostile input. Payload lengths are checked before copying, message CRCs are validated, and profiles use fixed-capacity tables rather than unbounded allocation.

The principal adversary can control one attached ECU or gateway and knows the expected schedule. The adversary may emit valid CRCs and plausible payloads. Detection therefore uses timing, cycle membership, channel placement, frame flags, length, and stable payload fingerprints. A sophisticated adversary that exactly reproduces every learned property is outside pure bus-observation detection; higher-layer signal semantics or ECU attestation would be required.

The adversary may also have temporary physical access to the Sentinel. Tamper evidence is provided by enclosure seals and a secure-element-backed evidence key, but revision A0 is not a certified tamper-resistant product. Extracting firmware through invasive methods is outside scope. Denial of service against the observed bus is detectable but not preventable because the device has no transmit path and must not become a safety controller.

Privacy is part of the model. Raw vehicle payloads may encode location, occupant, or operational data. Default evidence records retain frame metadata and a keyed fingerprint instead of payload bytes. Raw payload storage is an explicit, physically confirmed option for authorized laboratory work. Evidence exports record whether raw storage was enabled.

## Hardware specifications

| Subsystem | Design choice |
|---|---|
| Main controller | STM32H743VIT6, Cortex-M7 at up to 400 MHz, 2 MiB flash, 1 MiB RAM |
| Capture FPGA | Lattice iCE40UP5K-SG48, dual receive pipelines and 80 MHz timestamp counter |
| FlexRay receivers | 2 × NXP TJA1083 receive paths, one per channel; transmit data held disabled and not routed to control logic |
| Isolation | ISO7741 digital isolation between bus-side receivers and logic domain |
| Clock | 20 MHz TCXO feeding FPGA PLL; 80 MHz capture clock, nominal 12.5 ns tick |
| Storage | 32 MiB QSPI NOR for ring evidence; optional microSD for authorized raw capture |
| Trust anchor | ATECC608B for device identity and export signatures |
| Host connectivity | USB-C USB 2.0 high-speed device, CDC plus framed vendor protocol |
| User interface | RGB status LED, baseline button, recessed privacy button, piezo warning |
| Power | USB-C 5 V, isolated 5 V-to-5 V bus-side module, 3.3 V and 1.2 V regulators |
| Protection | TVS arrays, common-mode chokes, resettable fuse, reverse protection, receiver current limiting |
| Connectors | Two keyed 4-pin channel inputs plus USB-C; adapter harnesses remain vehicle-specific |
| Form factor | 92 mm × 55 mm four-layer PCB in a 105 mm × 68 mm × 26 mm enclosure |
| Typical consumption | 2.3 W active capture, 0.6 W safe idle |

The TJA1083 choice is coherent across documentation and KiCad references. It provides FlexRay electrical receive functionality suitable for 10 Mbit/s links. Revision A0 exposes separate channel connectors rather than pretending one connector standard fits every vehicle. Adapter cables carry only channel pair and reference; operators must verify pinout, common-mode range, and termination before attachment.

The FPGA performs deterministic edge timing because software interrupt latency is not an acceptable measurement clock. It does not parse application signals. Each capture record contains channel, frame flags, cycle, slot, length, timestamp, payload, header CRC, and frame CRC. The MCU rejects records that exceed 254 payload bytes or identify impossible channels and slots.

Power domains are separated so a fault in USB-connected equipment is less likely to create a current path into the monitored network. Isolation does not make an unsafe bench safe by itself. The design must be reviewed against the target harness, isolation voltage, grounding plan, and measurement category before use.

## Architecture and block diagram

```text
 Channel A pair -> protection -> TJA1083 RX -> isolation --+
                                                          |
 Channel B pair -> protection -> TJA1083 RX -> isolation --+--> iCE40UP5K
                                                               | dual symbol decoder
 20 MHz TCXO -------------------------------------------------> | 80 MHz timestamp
                                                               | bounded FIFO
                                                               v
                                                         STM32H743
                                                  +------+-----+-------+
                                                  | decoder / CRC      |
                                                  | schedule baseline  |
                                                  | anomaly scoring    |
                                                  | evidence signer    |
                                                  +---+-----------+----+
                                                      |           |
                                                QSPI / microSD   USB-C
                                                      |           |
                                                      +---- companion app

 Physical buttons -> baseline/privacy confirmation
 Fault supervisor -> receivers disabled and monitor stubs disconnected
 ATECC608B -> evidence identity and export signature
```

The data flow is deliberately one way at the bus boundary. Channel receivers feed the isolation barrier and FPGA. No application command reaches a bus transmitter. The FPGA-to-MCU SPI link is bounded by record length and FIFO depth. The MCU stores compact events in a ring, and the application reads snapshots. If USB backpressure occurs, the evidence writer takes priority over live display. A loss counter records dropped live frames.

## Firmware details and design decisions

The `firmware/` tree contains a host-verifiable implementation of the record codec and anomaly detector. `drivers/flexray.c` provides big-endian record framing, CRC-11 header checking, CRC-24 frame checking, payload fingerprinting, and strict field validation. `drivers/detector.c` implements a fixed-capacity hashed slot table, online timing means, cycle masks, expected channel and length, baseline freezing, bounded anomaly scores, confidence estimates, and JSON baseline export. `main.c` includes deterministic self-tests and a demonstration stream. `board.h` is the single source for limits and pin assignments. `registers.h` defines the MCU/FPGA register contract.

The host build is intentional. It lets contributors run `make check` with an ordinary C11 compiler before a board-support package exists. Hardware adaptation should replace the host capture source with an SPI DMA driver but retain the tested decoder and detector APIs. Dynamic allocation is avoided. All arrays have explicit maxima. This controls fragmentation and makes worst-case memory use reviewable.

The anomaly score is explainable rather than machine-learned. A length change carries more weight than a first observation in a new cycle. Timing drift, channel movement, changed payload fingerprint, and duplicate slot observations add independent evidence. An event is emitted when the configured threshold is reached. Operators can inspect why a score fired instead of trusting an opaque classifier.

Timing learning uses an online mean, permitting a baseline without storing every sample. Production firmware should add robust dispersion bounds, oscillator calibration, and cluster-specific macrotick conversion. Payload fingerprints are not authentication and are not collision-proof; their purpose is to detect common changes without retaining sensitive raw bytes. A production build should key the fingerprint with a secure-element-derived secret.

The firmware has four conceptual modes. Safe mode leaves receivers disabled during startup checks. Observe mode captures against a frozen baseline. Learn mode updates profiles. Replay-lab is reserved for replaying previously recorded data internally through the decoder; it does not transmit to FlexRay. A watchdog, capture-overflow counter, signed configuration object, and evidence journal are required before field deployment.

### Failure and degradation behavior

If the MCU crashes, the hardware supervisor disables receiver stubs and signals red. If the FPGA FIFO fills, the device increments a loss counter and preserves event records rather than pretending the capture is complete. If one FlexRay channel disappears, the other remains observable and a channel-asymmetry event is raised. If storage fills, old ordinary frames are overwritten, but signed anomaly summaries remain until exported or physically cleared. If USB disconnects, capture continues. If the TCXO or FPGA PLL loses lock, timing scores are suspended and records are marked clock-invalid. If the secure element fails, unsigned local capture may continue, but the application labels exports as untrusted.

These choices ensure the monitor fails visibly and does not claim evidence quality it lacks. Most importantly, no software failure can enable bus transmission because the route is absent from the board.

## Companion application and software interface

The `app/` directory contains a React Native interface authored by jayis1. It provides three functional views. Status shows both channels, current cycle, learned slot count, anomaly count, and capture loss. Events filters the evidence timeline by score and displays slot, cycle, channel, type, and explanation. Baseline explains the active learning policy and stages freeze or privacy changes for physical confirmation.

The pure JavaScript protocol module is separately testable with Node. Messages contain a fixed magic value, version, type, sequence, payload length, payload, and CRC-16. Decoders reject short messages, bad magic, impossible lengths, and bad CRCs. Event JSON is range checked before display. The app does not offer frame injection controls because the hardware does not transmit.

A production transport layer can use USB on Android and desktop, with iOS access through a supported accessory bridge. The current repository supplies application screens and the stable framing functions but not platform signing credentials or generated native build directories. `node_modules`, Expo state, and native build outputs are excluded from version control.

The evidence export format should include device identifier, firmware digest, FPGA digest, baseline digest, capture start and end time, clock-health flags, dropped-record counters, policy state, and a secure-element signature. PCAP-NG export can represent decoded frames in custom blocks, while JSON is better for schedule profiles and anomaly explanations.

## Security research and penetration-testing use cases

### Gateway compromise validation

Place Sentinel on an isolated FlexRay cluster, learn a baseline, and conduct an authorized gateway assessment. If gateway compromise causes new static slots, altered repetition patterns, or one-channel-only behavior, Sentinel provides independent timestamps. This separates proof of bus impact from logs generated by the system under test.

### ECU replacement and supply-chain comparison

Record profiles from known-good and suspect ECUs under the same bench sequence. Compare startup frames, sync participation, slot lengths, cycle masks, and timing means. Differences do not prove malicious intent, but they identify where deeper firmware or hardware analysis should focus.

### Fault-injection observation

During voltage, clock, electromagnetic, or software fault injection, monitor whether a target produces CRC failures, delayed slots, duplicate transmissions, or loss of redundancy. The device gives the fault campaign a structured bus outcome instead of only pass/fail application behavior.

### Incident-response capture

An intermittent safety-bus fault may occur only after hours of thermal or vibration testing. Sentinel can retain low-volume schedule metadata and anomaly evidence without storing every private payload. Investigators receive a timeline with capture-loss disclosure and clock-health state.

### Configuration regression testing

Run the same simulated drive cycle before and after an ECU software release. Freeze the approved schedule as a test artifact. Unexpected slots, lengths, or channel changes become reviewable regression failures even when functional tests still pass.

### Training and protocol education

Use a small isolated FlexRay development cluster to visualize cycles, static slots, startup frames, sync frames, and redundancy. Students can corrupt captured records in software and watch CRC and anomaly tests reject them, without receiving controls that transmit to a real bus.

## Build and verification

Host firmware verification:

```sh
cd firmware
make clean
make check
```

Application protocol verification:

```sh
cd app
npm test
```

The KiCad sources are structured engineering reference files with named components, footprints, nets, placement, traces, zones, and board outline. They have not been opened by `kicad-cli` in this environment because that tool is not installed. Therefore they are not represented as ERC-, DRC-, signal-integrity-, thermal-, EMC-, isolation-, or fabrication-verified. Before fabrication, open them in the declared KiCad version, update symbols from installed libraries, run ERC and DRC, inspect differential routing and stubs, verify creepage, confirm TJA1083 reference circuitry against the current NXP data sheet, generate manufacturing outputs, and obtain review from an automotive hardware engineer.

The firmware host target verifies C logic but is not a complete STM32 startup image. Board bring-up still needs CMSIS startup, linker script, clock tree, SPI DMA, USB, QSPI, secure-element, filesystem, boot verification, and FPGA configuration code. This distinction is explicit so a passing host build is not mistaken for a fabrication-ready safety product.

## Responsible design notes

FlexRay Sentinel contains no hardcoded target identifiers, credentials, exploit payloads, or live attack automation. Receive-only hardware is the default and only revision-A0 bus capability. Configuration changes require local confirmation. Payload retention is disabled by default. The project favors forensic utility over disruption and should never be expanded into a jammer or uncontrolled safety-bus injector.

Any future transmit-capable daughterboard should be a separate, conspicuously keyed laboratory accessory with galvanic separation, two-person enablement, time-limited authorization, and a load-safe test harness. It must not silently convert this monitor into an offensive implant.

## Directory structure

```text
flexray-sentinel/
├── README.md
├── DESIGN.md
├── firmware/
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── Makefile
│   └── drivers/
│       ├── flexray.c / flexray.h
│       └── detector.c / detector.h
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── App.js
    ├── protocol.mjs
    ├── test.mjs
    └── package.json
```

## Author

Every design decision, hardware source, firmware file, application file, and document in this device directory is credited to jayis1. Contributions should preserve that creator attribution while recording individual changes through normal version-control history.
