# OneWire Cartographer

**Dual-segment 1-Wire/iButton topology, timing, and trust-boundary analyzer**

**Author and creator:** jayis1  
**Hardware, firmware, application, and documentation:** jayis1  
**License:** MIT  
**Status:** engineering reference design, revision 1.0

> **Authorized use only.** OneWire Cartographer is a laboratory instrument for systems you own or for which you have explicit written permission to assess. Electronic access-control identifiers, equipment service buses, battery authentication links, and industrial sensors may be safety- or security-critical. Do not intercept, emulate, modify, or disrupt a deployed bus without authorization, a rollback plan, and an assessment of physical safety. The operator is responsible for law, policy, privacy, and system availability.

## Purpose and overview

OneWire Cartographer is an inline, fail-open instrument for understanding security assumptions hidden inside Dallas/Maxim 1-Wire and iButton installations. A 1-Wire bus looks simple—one open-drain data conductor plus ground—but that simplicity often hides a complicated trust boundary. Door readers accept serial-number tokens. Printer and medical consumables expose identity EEPROMs. Battery packs publish telemetry and authentication material. Environmental probes share long multidrop cables. Service tools discover devices using ROM search. Product teams frequently validate only the eight-byte ROM CRC and then treat a device as authentic.

Ordinary logic analyzers show edges but do not explain which side caused them, whether a pulse crossed an interposer, how the analog waveform changed, or whether an observed token is electrically consistent with previous sightings. Typical 1-Wire masters can enumerate devices but actively alter the bus and cannot watch an existing master. OneWire Cartographer fills that gap. It divides the target into a **master segment** and a **device segment**, samples both simultaneously, correlates digital events with line voltage, inventories passively observed ROMs, and calculates timing fingerprints. A normally closed relay preserves the original path when power is absent. In monitor mode, a low-resistance analog switch joins the two segments while independent comparators and ADC channels observe each side.

The device is not merely a protocol sniffer. Its novel feature is a *causal map* of the bus. By comparing edge arrival, low-pulse duration, rise time, presence timing, and the segment on which an edge first appears, the analyzer distinguishes master activity, slave presence, wiring distortion, contention, and interposer-induced changes. The resulting map helps a researcher answer questions such as: Is identity trusted solely by ROM? Does a reader reject a token whose presence pulse has unfamiliar timing? Are two nominally identical sensors electrically distinguishable? Does an unauthorized branch appear after installation? Is a weak pull-up causing intermittent authentication? Can a defensive gateway isolate a suspicious device without taking the whole bus down?

Controlled transmission exists only for isolated bench work. A physical ARM switch, a time-bounded host challenge, rate limiting, voltage checks, and a latched fault policy gate every driven reset or data slot. Passive capture never requires arming. The design deliberately avoids autonomous credential collection, unattended replay, or covert radios. USB-C is the only management interface.

## What makes it different

The core contribution is dual-ended attribution rather than conventional single-node decoding. Two fast comparator channels timestamp crossings at 8 MHz. Two ADC channels retain line-level context. Firmware pairs those measurements into normalized events and records which segment originated a transition. The companion console displays a topology inventory, timing confidence, anomalous slot widths, voltage excursions, and differences between the master and device waveforms.

A conventional analyzer connected to DQ sees the wired-AND result. OneWire Cartographer can temporarily open its analog bridge in an authorized laboratory session, leaving each side with a controlled pull-up, to determine whether a low state came from the master or a slave. It can then return to transparent monitoring. The normally closed mechanical bypass means firmware failure, watchdog reset, unplugged USB, or loss of power restores continuity. This architecture is useful both offensively, when validating weak identity assumptions, and defensively, when designing tamper detection and bus guardians.

## Attack surface and threat model

### Assets under study

The primary assets are device identity, commands and responses, sensor values, authentication exchanges, bus availability, and evidence integrity. In an access-control deployment, the protected decision may be “unlock for this ROM.” In a battery system it may be “permit charging for this pack.” In a consumable ecosystem it may be “accept this cartridge.” Captured traces can themselves contain identifiers and must be handled as assessment evidence.

### Adversaries modeled

1. **Physical tapper:** gains temporary access to DQ and ground, records traffic, and attaches a parallel slave.
2. **Identifier cloner:** presents a programmable 1-Wire device with the ROM of an enrolled token.
3. **Inline manipulator:** delays, suppresses, or substitutes slots between a legitimate master and slave.
4. **Malicious or counterfeit peripheral:** answers normal discovery but abuses timing, parasite power, or unexpected function commands.
5. **Fault injector:** forces DQ low, overdrives the line, or creates marginal rise times to induce fail-open behavior.
6. **Compromised assessment host:** attempts to make the instrument transmit without local operator consent.
7. **Accidental operator error:** connects excessive voltage, drives a live multidrop installation, or selects the wrong segment.

### Surfaces examined

The device observes reset and presence sequences, Search ROM (`0xF0`), Alarm Search (`0xEC`), Read ROM (`0x33`), Match ROM (`0x55`), Skip ROM (`0xCC`), family-specific function bytes, strong-pull-up intervals, line idle voltage, rise/fall behavior, slot width, reset width, and inter-segment propagation. It highlights ROM CRC failure, changing inventories, impossible edge order, simultaneous pull-down, prolonged low states, overvoltage, and deviations from a learned timing envelope.

Many systems expose additional surfaces outside DQ: debug headers on the reader, USB on the instrument, companion-app storage, exported evidence, power injection, connector pin-order mistakes, and firmware update. This design scopes USB commands explicitly, bounds frame size, validates thresholds, and defaults to a passive mode. Production builds should add signed DFU, readout protection, encrypted evidence storage if sensitive identifiers are retained, and a per-unit secure-element key.

### Trust boundaries and exclusions

The target master, target peripherals, Cartographer firmware, and analysis workstation are separate trust zones. No target-provided byte is trusted as a length, index, or command. The firmware event queue is fixed-size; overflow increments a counter and discards oldest analysis events without changing the bus. The bridge hardware, not application software, owns the fail-open property.

The reference design does not claim galvanic isolation between target ground and USB ground. Use an external USB isolator for floating or industrial systems, and never attach it to mains-referenced conductors. It is designed for 3.0 V to 5.5 V 1-Wire signaling, not automotive single-wire CAN, LIN, SWIM, DALI, mains power-line communication, or unknown high-energy wiring. It does not defeat cryptographic iButton protocols. It helps characterize implementation and integration weaknesses; it does not make a secure authenticator clonable.

## Hardware specification

### Processing and capture

- **MCU:** STM32G474RET6, Arm Cortex-M4F at up to 170 MHz, 512 KiB flash, 128 KiB SRAM, USB device, high-resolution timers, comparators, and fast ADCs.
- **Sampling:** paired timer capture at 8 MHz nominal, giving 125 ns timestamp granularity; ADC snapshots associate millivolts with each threshold edge.
- **Front end:** TLV3502 dual high-speed comparator with hysteresis derived from precision dividers. Input series resistance, Schottky clamps, and low-capacitance TVS protection limit loading and transients.
- **Segment switch:** TS5A23157 dual SPDT analog switch used as a low-resistance, break-before-make electronic bridge for controlled isolation.
- **Fail-open bypass:** Omron G6K-2F-Y normally closed relay. “Fail-open” refers to assessment behavior: when Cartographer loses power, the original target circuit is reconnected rather than left interrupted.
- **Drive stages:** two 2N7002 open-drain MOSFETs, one per segment, with default-off gate pulldowns. A separate current-limited high-side switch provides optional strong pull-up under firmware timeout control.
- **Protection:** resettable fuse on target auxiliary power, 5.6 V TVS, 220-ohm current-limiting sense paths, reverse-current blocking, and analog measurement ahead of drive authorization.

### Connectivity and controls

- USB-C full-speed device for power, CDC-style event streaming, commands, and firmware update.
- Three-pin keyed connectors on each target side: limited auxiliary voltage, DQ, ground.
- Recessed physical ARM slide switch and amber armed LED.
- Green status LED, red latched-fault LED, trigger input, and trigger output for oscilloscope correlation.
- No Wi-Fi, BLE, cellular, or long-range radio. This reduces attack surface and prevents accidental remote operation.

### Power and form factor

USB-C supplies 5 V. A low-noise 3.3 V regulator powers digital and analog logic. The target bus is sensed with high impedance and is not powered by USB unless the operator fits the `JP_TARGET_POWER` jumper. When fitted, target supply passes through a 100 mA current limiter. Typical monitor consumption is approximately 110 mA before attached targets. The intended PCB is four layers, 110 mm by 78 mm, 1.6 mm FR-4, with solid internal ground and power planes. The enclosure is a 125 mm by 88 mm by 28 mm bench pod with guarded side connectors and a recessed arm switch.

The board keeps the two DQ traces symmetrical around the comparator and bridge. Connector-to-relay paths are short and wide; sensitive ADC traces remain inside a ground guard. USB routing uses a 90-ohm differential pair. The PCB file includes real footprints, named nets, routed representative segments, board outline, and author/authorized-use silkscreen. It is an engineering starting point; fabrication still requires ERC, DRC, stack-up review, impedance confirmation, footprint verification against selected manufacturer parts, and bench validation.

## Architecture

```text
                 USB-C
                   │
             ┌─────▼──────┐
             │ STM32G474  │◄──── ARM switch / trigger input
             │ decoder,   │────► status, fault, trigger output
             │ policy, USB│
             └──┬──────┬──┘
                │      │ timer capture + ADC
       ┌────────▼──┐ ┌─▼─────────┐
       │ master    │ │ device    │
       │ comparator│ │ comparator│
       └─────▲─────┘ └────▲──────┘
             │             │
 MASTER ─────┼──[ analog bridge ]──┼───── DEVICE
 connector   │             │       connector
             ├─ open-drain ┤
             │  drivers    │
             └──[NC safety relay]──┘
```

The acquisition path never depends on protocol decode. Comparator interrupt/DMA capture writes timestamped edges into a ring buffer. The analyzer consumes those records later, so a malformed or unsupported function command cannot block edge capture. A normalized event has type, port, timestamp, duration, voltage, value, and flags. Higher-level ROM inventory is derived from byte events but raw edges remain the source of truth.

The control plane accepts fixed headers containing magic, protocol version, command, payload length, and sequence. Informational and capture commands are passive. Drive-related commands cross a policy boundary. The policy checks physical ARM state, session expiry, fault latch, action rate, requested mode, and live voltage before touching a MOSFET. Any detected contention releases both drivers and returns the mechanical relay to its normally closed continuity state.

## Firmware

The `firmware/` implementation is portable C11 and contains more than 500 lines of substantive C across the application and drivers. `board.h` documents production pin assignments and platform services. `registers.h` defines the host protocol and capture aperture. `drivers/onewire_phy.c` owns edge buffering, voltage thresholds, open-drain timing, reset generation, and Dallas CRC-8. `drivers/analyzer.c` classifies reset pulses and slots, assembles bytes, inventories ROMs, and computes a compact timing fingerprint. `drivers/policy.c` enforces local consent, five-minute session expiry, 24-action-per-second limit, and fail-open fault handling. `main.c` binds the modules, parses host frames, streams normalized events, and supplies a deterministic host simulator.

Design decisions emphasize predictable failure. There is no dynamic allocation. ISR-facing indexes are fixed-width and rings have explicit overflow behavior. Time comparisons use signed subtraction so 32-bit wrap remains safe for bounded intervals. Writes are least-significant-bit first as required by 1-Wire. Bus safety is sampled twice before drive. Threshold configuration requires at least 400 mV hysteresis separation and rejects values outside the protected input range.

The host build allows logic to be tested without an STM32 toolchain:

```bash
cd firmware
make clean test
```

For a hardware port, replace the host implementations of `board_micros`, ADC, GPIO, USB, delay, and watchdog with STM32 HAL or register-level equivalents; remove `OWC_HOST_SIM`; configure timer DMA for both comparator outputs; and map the memory aperture in `registers.h`. Production firmware should enable independent watchdog, brownout reset, option-byte readout protection, signed update verification, and a boot-time relay self-test.

## Companion application and interface

The `app/` directory contains a functional React/Vite Web Serial console rather than static mockups. It can request the USB device by the project VID/PID, stream fixed 16-byte normalized events, bound its in-memory timeline, send framed commands, and disconnect cleanly. If no hardware is present, clearly labeled demo records make the interface reviewable without pretending that a capture occurred.

Four working views are included:

- **Live:** timeline of reset, presence, bit, byte, ROM, timing, voltage, contention, and policy events. Metrics show mode, event count, and anomaly count.
- **Topology:** observed ROM inventory with family interpretation, CRC validity, observation count, and timing jitter.
- **Lab:** controlled-drive interface. It remains disabled until a connected device accepts an arm-session request; the hardware switch remains authoritative.
- **Export:** downloads the current evidence as structured JSON carrying device and author metadata.

The browser transport uses Web Serial, available in Chromium-derived browsers on HTTPS or localhost. Frames are little-endian and payloads are capped at 244 bytes. Protocol unit tests verify serialization, event parsing, challenge transformation, ROM formatting, and oversize rejection. Run `npm install`, `npm test`, and `npm run build` in `app/`. A production evidence workflow should add capture hashes, operator/case fields, UTC synchronization, signed device attestations, and automatic redaction of identifiers before reports leave the authorized team.

## Practical use cases

### Red teams

A red team can install Cartographer during an approved physical access-control assessment and passively determine whether the reader uses only Read ROM or performs a cryptographic challenge. The team can compare multiple issued tokens, identify enrollment and retry behavior, and document whether duplicate identifiers are detected. In an isolated replica—not the live door—the armed mode can generate carefully bounded reset and byte sequences to verify parser behavior and fail-secure handling. Dual-port attribution proves whether odd timing originated in the reader, token, or interconnect.

For embedded products, the tool can document a consumable or battery trust chain. A tester can identify family codes, function commands, polling cadence, strong-pull-up usage, and recovery after disconnect. This informs risk without immediately desoldering parts or modifying firmware. Timing fingerprints may reveal that a counterfeit peripheral has a valid copied ROM but materially different presence behavior; that signal can motivate a stronger challenge-response control rather than being treated as authentication by itself.

### Security researchers

Researchers can characterize undocumented 1-Wire variants, collect rise-time distributions over cable length, compare parasite-powered and externally powered devices, and study multidrop arbitration. Trigger output aligns decoded events with a high-bandwidth oscilloscope. Exported normalized events are compact enough for notebooks while preserving raw timing and voltage context. The two-sided architecture is especially useful for evaluating defensive inline filters, because it measures the filter’s propagation and waveform impact rather than assuming transparency.

### Penetration testers and defenders

A penetration tester can inventory exposed maintenance buses in building automation, industrial panels, kiosks, laboratory equipment, and appliances without transmitting discovery commands. Defenders can baseline known ROM populations and timing, then investigate an unexpected branch or intermittent device. Product engineers can regression-test that malformed reset width, stuck-low faults, duplicate ROMs, and sudden device removal produce safe application behavior. Installation technicians can locate weak pull-ups, excessive capacitance, grounding problems, and connectors whose resistance changes presence detection.

## Safe operating workflow

1. Obtain written scope and identify whether loss of the bus can create a safety or availability problem.
2. Verify voltage and ground with an independent meter. Do not assume a single conductor is 1-Wire because the connector resembles one.
3. Keep target-power jumper open. Connect USB through an isolator when target grounding is uncertain.
4. Power Cartographer first and confirm its relay is in bypass and fault LED is clear.
5. Disconnect target power, insert master and device connectors, inspect pin order, then restore target power.
6. Begin passive capture. Record baseline traffic before changing any threshold or bridge state.
7. Export evidence with case context. Treat ROM identifiers as potentially sensitive access data.
8. Use Lab mode only on an isolated bench replica. Move the physical ARM switch deliberately and allow the session to expire after the test.
9. If a voltage, short, or contention fault latches, remove target power and diagnose externally before clearing it.
10. At completion, remove Cartographer, restore the original wiring, and verify normal operation with the system owner.

## Repository layout

```text
onewire-cartographer/
├── README.md
├── firmware/
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── Makefile
│   └── drivers/
│       ├── onewire_phy.c/.h
│       ├── analyzer.c/.h
│       └── policy.c/.h
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── index.html
    ├── package.json
    ├── vite.config.js
    ├── src/
    │   ├── main.jsx
    │   ├── protocol.js
    │   ├── transport.js
    │   └── style.css
    └── test/protocol.test.js
```

## Limitations and next engineering steps

The checked-in design is compile-ready reference firmware and an electrically meaningful KiCad starting point, not a certified finished product. Comparator thresholds and protection leakage must be measured across temperature. Analog-switch on-resistance and relay capacitance must be validated against long buses and parasite power. The PCB needs full KiCad ERC/DRC in the exact library version used for fabrication, manufacturer-specific footprints, decoupling placement, test points, mounting holes, and controlled-impedance USB review. EMC, ESD, USB compliance, fault-energy, and enclosure flammability testing remain mandatory before field deployment.

Firmware still needs the production STM32 hardware abstraction, DMA ISR glue, USB descriptors, secure boot integration, and exhaustive fuzzing of USB frames. Timing classification should become adaptive per family while retaining conservative hard bounds. The application should persist no identifiers by default, and evidence signing should use a per-device key rather than browser state. These limitations are explicit so the reference design is not mistaken for validated commercial test equipment.

## Authorship

OneWire Cartographer was conceived, designed, documented, and authored by **jayis1**. All source code, hardware metadata, silkscreen attribution, protocol tests, and application metadata in this directory credit jayis1.