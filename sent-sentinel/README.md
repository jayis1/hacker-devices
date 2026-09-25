# SENT Sentinel — Four-Channel Automotive Sensor Trust Analyzer

Author and creator: jayis1

Version: 0.1.0

License: MIT for firmware and application; CERN-OHL-S-2.0 for hardware design

Status: engineering reference design. The host firmware and application protocol tests are executable; physical hardware, automotive qualification, EMC, KiCad ERC, and KiCad DRC require laboratory validation.

## Legal and ethical disclaimer

SENT Sentinel is intended only for defensive engineering, authorized security research, owned-asset diagnostics, controlled automotive test benches, and penetration tests conducted under explicit written rules of engagement. Automotive sensors influence propulsion, braking, steering, restraint, thermal management, and industrial machinery. Never attach this device to a moving vehicle, a public-road vehicle, or equipment whose unexpected behavior could injure a person or damage property. Do not monitor systems you do not own or lack permission to assess. Follow applicable computer-access, privacy, vehicle-safety, workplace-safety, export-control, and electromagnetic-compatibility law.

The revision-A0 field interface is physically receive-only. It does not generate SENT pulses, drive sensor lines, alter sensor supply, or inject values. That limitation is deliberate and must not be defeated. Use an isolated bench supply, a fused harness, an emergency disconnect, and an independent safety observer. Verify every connector pin against current vehicle and sensor documentation before applying power. The author, jayis1, provides this design without warranty and accepts no liability for misuse or for fabrication based on an unreviewed reference design.

## Purpose and overview

SENT Sentinel is a compact four-channel instrument for evaluating trust in sensors that use SAE J2716 Single Edge Nibble Transmission (SENT). SENT is common in automotive pressure, position, temperature, pedal, throttle, and magnetic-angle sensors. It encodes a status nibble, data nibbles, and a CRC in the time between falling edges. It may also multiplex a slower diagnostic channel over consecutive frames. Ordinary oscilloscopes can show the waveform, and logic analyzers can measure pulses, but neither automatically answers the security question: is this sensor behaving like the known-good unit under the present physical conditions?

The device captures four SENT channels with hardware timer inputs, measures their pulse widths against a common clock, samples up to four associated analog reference or redundant sensor outputs, validates CRCs, reconstructs fast-channel values, and learns a bounded baseline. Once an operator freezes that baseline with a physical button, the firmware identifies tick-time changes, frame-period drift, status-nibble changes, value excursions, sequence discontinuities, stable-nibble fingerprint changes, and disagreement between digital values and analog evidence. It creates compact, explainable events rather than silently declaring a sensor malicious.

Its novelty is cross-domain correlation at the sensor boundary. A replay device can generate syntactically valid SENT frames with correct CRCs, but it may fail to track a redundant analog output, supply-current signature, paired sensor, or commanded bench stimulus. A replacement sensor may report plausible values while using a different tick period, status policy, frame rate, or slow-channel identity. SENT Sentinel combines timing, protocol, and independent analog evidence while remaining electrically incapable of transmission. The result is useful for supply-chain comparison, fault-injection campaigns, ECU input validation, incident response, and laboratory red-team exercises.

This is not another CAN interface. SENT normally connects one sensor output to one receiving ECU input and has no arbitration, addressing, or bidirectional diagnostic session. The attack surface is therefore close to the physical sensor and harness. SENT Sentinel focuses on that overlooked boundary: the few centimeters or meters between a measured phenomenon and the controller that trusts the measurement.

## Who benefits and what workflow improves

A red team can place the instrument beside a target ECU on an isolated harness, learn known-good sensor behavior, and then exercise authorized physical-access or supply-chain scenarios. The Sentinel acts as an independent witness. It can show that a test implant maintained CRC validity but broke the relationship between digital throttle position and a redundant analog track. Because it cannot inject, it does not become the offensive mechanism under test.

A vehicle security engineer can compare original and replacement sensors under the same programmable stimulus. Timing and identity fingerprints reveal differences that a simple min/max value check misses. A functional-safety engineer can investigate intermittent sensor faults and retain explicit evidence of CRC errors, pulse drift, status changes, and capture loss. A silicon researcher can observe how voltage, clock, temperature, electromagnetic, or optical fault injection changes the sensor output. Educators can demonstrate SENT framing and CRC behavior without distributing a transmitter.

Compared with an oscilloscope, SENT Sentinel runs long captures and produces searchable events. Compared with a generic logic analyzer, it understands sync calibration, nibble timing, CRC, frame identity, and learned tolerances. Compared with a commercial automotive interface, it is open, locally operated, privacy-conscious, and designed so the field connector has no output path.

## Attack surface

The monitored system and the instrument each present distinct attack surfaces:

1. A person with harness access can replace a sensor, splice an emulator into its output, alter ground or supply impedance, or bridge one channel to another.
2. A counterfeit or compromised sensor can emit valid CRCs while changing calibration, status semantics, slow-channel identity, update rate, or response to physical stimuli.
3. An implant can replay a previously recorded waveform. The result may look valid in isolation but disagree with a redundant analog track, paired sensor, actuator command, or bench stimulus.
4. Supply disturbance, ground offset, injected noise, temperature, and component aging can create timing or value anomalies that resemble an attack. Evidence therefore requires context.
5. A compromised ECU can bias sensor supply or load the signal line. SENT Sentinel observes resulting waveform changes but does not prevent the behavior.
6. The USB host can send malformed records, hostile configuration values, oversized messages, repeated baseline commands, or misleading timestamps.
7. Physical access to the Sentinel creates tamper, replacement, and firmware-extraction risks. The ATECC608B provides an evidence identity, not certified tamper resistance.
8. Storage exhaustion and capture overload can hide evidence unless loss is measured and reported.

The analyzer does not claim that every deviation is malicious. A new sensor revision, different temperature, startup transient, diagnostic mode, or legitimate calibration may change a baseline. Every event records the factors that contributed to its score so an investigator can distinguish protocol evidence from conclusions about intent.

## Threat model

The principal adversary can control one sensor or insert an inline emulator. The adversary knows the expected SENT data format and can calculate valid CRCs. They may replay values, alter timing within receiver tolerance, imitate status bits, or substitute a sensor with similar nominal behavior. SENT Sentinel therefore does not treat CRC validity as authenticity. It considers frame timing, tick calibration, sequence continuity, bounded identity features, value range, and independent analog correlation.

The trusted computing base comprises the receive-only input topology, immutable boot root planned for production, capture timer setup, decoder, bounded detector, physical-confirmation policy, evidence journal, and export signer. The phone or laptop is not trusted to rewrite historical events. A host may stage baseline changes, but the device applies them only after a local button press. Firmware uses fixed-capacity arrays, validates channel and length fields before indexing, and avoids unbounded allocation.

A powerful emulator that reproduces every measured digital and analog property can evade this instrument. Defending against that adversary requires authenticated sensors, challenge-response capability designed into the sensor, trusted physical stimulation, or additional side-channel measurements. Likewise, SENT Sentinel detects but cannot prevent denial of service. It is not a safety controller and must never be placed in a control loop.

Privacy matters because sensor streams can reveal speed, position, operator actions, or equipment state. The default evidence mode stores derived event metadata and keyed fingerprints rather than raw nibbles. Raw capture is an explicit laboratory option requiring physical confirmation. Exports state whether raw retention was enabled and whether any records were lost.

## Hardware specifications

| Subsystem | Design choice |
|---|---|
| Main controller | STM32G474VET6, Cortex-M4F at up to 170 MHz, high-resolution timers, multiple capture timers, USB device |
| SENT capture | Four falling-edge inputs routed through protected 74LVC14A Schmitt receive buffers to TIM2 and TIM4 capture channels |
| Analog correlation | Texas Instruments ADS8684IDBTR, four 16-bit channels with protected bipolar-capable inputs and SPI interface |
| Input protection | Series resistance, low-capacitance automotive TVS devices, RC filtering, and high-impedance taps sized per target harness review |
| Clock | 16 MHz temperature-compensated oscillator; timer clock at 170 MHz, nominal 5.88 ns count |
| Storage | Winbond W25Q128JV 16 MiB QSPI NOR ring journal |
| Trust anchor | Microchip ATECC608B for device identity and signed export digests |
| Host connectivity | USB-C, USB 2.0 full-speed device, CDC plus bounded vendor protocol |
| User interface | RGB status LED, baseline/confirm button, privacy button, audible fault indicator |
| Power | USB-C 5 V input, fused and reverse-protected; 3.3 V digital and low-noise analog rails |
| Connector | Keyed eight-pin field connector carrying four digital taps and four optional analog companions; separate ground reference studs |
| Form factor | 75 mm × 55 mm four-layer PCB in an approximately 88 mm × 66 mm × 25 mm enclosure |
| Typical consumption | Approximately 1.4 W during four-channel capture, excluding external probes |

The STM32G474 is appropriate because SENT edge decoding is a timer-capture problem rather than a radio or high-speed FPGA problem. At a 170 MHz timer rate, even a short 2 microsecond SENT tick spans hundreds of timer counts. DMA-backed capture can preserve edge timing without depending on interrupt latency. A production target port should use circular DMA buffers for TIM2 and TIM4 and timestamp buffer boundaries with a free-running 64-bit software epoch.

The ADS8684 is a real, orderable converter intended for industrial input ranges. It avoids pretending that the STM32 internal ADC can directly tolerate every automotive analog track. The exact divider, clamp, filter, and anti-alias network depends on the target range and must be calculated before fabrication. Analog inputs are observational only and must present sufficient impedance that they do not invalidate sensor diagnostics.

The 74LVC14A is shown as a compact receive buffer, but its absolute maximum and input thresholds do not make it an automotive line protector by itself. The schematic requires front-end dividers, clamps, TVS selection, and transient validation against the intended harness. An automotive-qualified buffer may be substituted during detailed design. No buffer output is routed back to a field pin.

## Architecture and block diagram

```text
 SENT 0 ---- protection ---- Schmitt RX ---- TIM2_CH1 capture --+
 SENT 1 ---- protection ---- Schmitt RX ---- TIM2_CH2 capture --|
 SENT 2 ---- protection ---- Schmitt RX ---- TIM4_CH1 capture --+--> STM32G474
 SENT 3 ---- protection ---- Schmitt RX ---- TIM4_CH2 capture --|    decoder
                                                               |    baseline
 Analog 0..3 -- high-Z protected network --> ADS8684 -- SPI ----+    anomaly engine
                                                                    evidence journal
 TCXO ---------------------------------------------------------> capture timebase

 STM32G474 --> QSPI NOR compact journal
 STM32G474 <-> ATECC608B evidence digest signature
 STM32G474 <-> USB-C bounded protocol <-> companion application
 Physical button -----------------------> baseline/privacy confirmation

 There is no digital or analog transmit path from the controller to the field connector.
```

The data path is intentionally asymmetric. Edge captures and ADC samples move inward. USB commands can alter display filters or stage a baseline operation, but cannot reach a field driver because no field driver exists. The firmware pairs analog samples with the nearest decoded frame, records uncertainty when the sampling deadline is missed, and suppresses analog-correlation scoring when the analog channel is unavailable rather than inventing confidence.

Each decoder calibrates its own tick duration from the 56-tick synchronization pulse. Subsequent pulse widths are rounded to a nibble only when their residual error is bounded. The decoder validates the status and data CRC before the detector receives a frame. CRC failures remain valuable counters and evidence but do not update a baseline.

## Firmware details and design decisions

The `firmware/` directory contains more than a placeholder board loop. `drivers/sent.c` implements tick calibration, pulse-to-nibble conversion, CRC-4 calculation, bounds checking, fast-value reconstruction, deterministic edge encoding for tests, and stable-field fingerprinting. `drivers/detector.c` implements fixed-capacity profiles, online means, timing tolerances in parts per million, range learning, analog correlation, counter continuity, explainable scores, and bounded JSON export. `main.c` provides nine deterministic self-tests plus a demonstration stream. `board.h` centralizes capacities, tolerances, modes, and pins. `registers.h` documents the intended STM32G474 peripheral addresses and bit contracts.

A normal C11 compiler builds the protocol and detector core on a host. This separates logic verification from board support. The host executable generates valid SENT edges, decodes them through the same API, learns a baseline, freezes it, and checks normal and anomalous observations. It tests bad sync, bad CRC, invalid inputs, analog disagreement, timing shifts, counter discontinuity, and baseline export. Dynamic allocation is not used.

The anomaly model is explainable. Timing shift contributes independently from a changed status nibble. A value outside the learned envelope contributes separately from analog disagreement. A sequence gap is weaker evidence than a large analog mismatch because capture loss can create apparent gaps. Scores saturate at 100. The configured threshold controls event creation, but the contributing flags remain visible.

Online means bound memory use, but they are not robust estimators. Production firmware should add variance, median-based filtering, per-operating-mode baselines, temperature context, wrap-safe sequence policy, and a calibration record signed by the secure element. Baselines should be versioned rather than overwritten. A production journal should use power-fail-safe records with monotonic sequence numbers, CRCs, and digest checkpoints.

Four modes are defined. Safe mode configures buffers and timers but does not accept a baseline. Learn mode updates profiles under local supervision. Observe mode freezes profile learning and emits evidence. Replay-lab mode feeds saved records through the decoder internally; it never produces field pulses. A hardware watchdog, brownout detection, clock-health monitor, DMA-overflow counter, and explicit degraded-state LED are required before field deployment.

Failure must be visible. If DMA overruns, the loss counter increments and subsequent evidence is marked incomplete. If the TCXO or timer clock is unhealthy, timing scoring is suspended. If the ADC is unavailable, digital analysis continues but analog confidence is absent. If QSPI storage fills, low-severity routine records may rotate, while signed anomaly summaries remain until physically cleared. If USB disconnects, capture continues. If signing fails, export remains possible but is labeled unsigned.

## Companion application and software interface

The `app/` directory contains a React Native companion authored by jayis1. It has three functional views rather than empty navigation placeholders. Status presents capture health, CRC rejects, loss counters, per-channel tick period, value, and analog measurement. Events filters the timeline by score and shows the factors behind each alert. Baseline displays the active policy and stages learning, freezing, or signed-summary export for physical confirmation.

`protocol.mjs` defines a bounded binary envelope with a magic value, version, message type, sequence, payload length, payload, and CRC-16. It rejects short records, unsupported versions, impossible lengths, and corrupted messages. Event JSON is range checked before display. The command builder accepts only passive workflow operations. There is deliberately no inject, replay-to-wire, pulse-generation, or arbitrary register-write command.

The protocol module is tested directly with Node without downloading React Native dependencies. A production transport can use Android USB host support or a desktop bridge. Platform-specific signing, native project directories, and USB permissions are not checked into this reference design. The application should remain local-first: it needs no cloud account and should export only when an operator asks.

## Security research and penetration-testing use cases

### Sensor substitution assessment

Mount known-good and candidate replacement sensors on the same fixture and apply a repeatable mechanical or pressure profile. Compare tick time, update period, status behavior, fast-channel range, stable fingerprint, and analog relationship. Differences identify where reverse engineering or supplier review should focus; they do not by themselves prove malicious intent.

### Authorized replay detection study

On an isolated ECU bench, use a separate authorized emulator to replay previously recorded sensor values while SENT Sentinel observes. The analyzer can reveal that valid digital values no longer track a live redundant analog path or commanded stimulus. This evaluates defensive detection without giving Sentinel transmission capability.

### Fault-injection observation

During voltage, clock, thermal, electromagnetic, laser, or software fault injection against a sensor, record whether synchronization, CRC, status, timing, and analog correlation change. The structured event stream is easier to compare across campaigns than screenshots from an oscilloscope.

### Harness and ground-fault investigation

Long-duration capture can correlate intermittent CRC failures or tick drift with supply and analog changes. A technician can separate protocol corruption from genuine physical movement, while capture-loss and clock-health fields prevent false certainty.

### ECU input validation

Place the device on a controlled hardware-in-the-loop setup while an ECU firmware release is exercised. Freeze an approved sensor profile and detect unexpected tolerance changes, status assumptions, or loss of correlation. This supports regression testing but does not replace functional-safety validation.

### Training

Students can generate synthetic edges in host mode, corrupt sync and CRC values, and inspect explainable detector scores. They learn that CRC provides error detection rather than source authentication and that independent physical evidence can strengthen a security conclusion.

## Build and verification

Build and execute firmware tests with:

```sh
cd firmware
make clean
make check
```

Run application protocol tests with:

```sh
cd app
npm test
```

The host target verifies portable decoder and detector logic. It is not a complete STM32 firmware image. Hardware bring-up still needs CMSIS startup, linker script, system clocks, timer/DMA configuration, ADS8684 SPI transport, USB device classes, QSPI journal, secure-element provisioning, authenticated boot, and production fault handling.

The KiCad sources contain named real components, footprints, nets, board placement, routing examples, a ground zone, and a board outline. They are engineering reference files, not fabrication outputs. `kicad-cli` was not available in the authoring environment, so no ERC or DRC pass is claimed. Before fabrication, open the project in the declared KiCad generation, resolve library compatibility, complete every power and decoupling unit, calculate the field protection network, run ERC and DRC, review creepage and ground strategy, validate input impedance, perform transient simulation, and obtain review from an automotive hardware engineer.

## Responsible design and limitations

SENT Sentinel contains no exploit payload, hardcoded target, credential collection, jamming, or field transmission. Its security value comes from trusted observation. Future contributors should preserve the physical one-way boundary. If active SENT generation is required for a laboratory, it should be a separate, conspicuously keyed fixture with its own authorization controls, not a hidden firmware feature in this monitor.

The design is not automotive-qualified and is not suitable for installation in a production vehicle. It has not passed ISO 7637 transient testing, CISPR 25 emissions testing, environmental qualification, functional-safety analysis, or cybersecurity certification. The enclosure, probe insulation, connector keying, and grounding plan all require target-specific review.

## Directory structure

```text
sent-sentinel/
├── README.md
├── firmware/
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── Makefile
│   └── drivers/
│       ├── sent.c / sent.h
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

Every design decision, hardware source, firmware file, application file, and document in this device directory is credited to jayis1. Version-control history should preserve that creator attribution.