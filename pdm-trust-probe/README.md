# PDM Trust Probe — Privacy-Preserving Digital Microphone Integrity Analyzer

Author: jayis1

Hardware revision: A0

Firmware protocol: PTP/1

License: CERN-OHL-S-2.0 hardware; MIT firmware and companion application

> LEGAL, ETHICAL, AND PRIVACY NOTICE: PDM Trust Probe is intended only for equipment owned by the operator or covered by explicit written authorization. Microphone buses can carry private speech, workplace conversations, biometric information, and regulated data. Obtain consent, follow applicable interception and privacy law, and use isolated laboratory fixtures. The default design immediately reduces bitstreams to non-reconstructable window statistics and does not store or export raw audio. Do not modify it for covert recording, surveillance, access-control bypass, or testing people who have not consented. jayis1 provides this research reference without a warranty of safety, fitness, regulatory compliance, or manufacturing readiness.

## Purpose and overview

PDM Trust Probe is an inline integrity instrument for pulse-density modulation microphone links. PDM microphones are found in laptops, conference appliances, headsets, smart speakers, intercoms, vehicle cabins, industrial voice terminals, and embedded products. A codec or processor supplies a clock, and one or more microphones return one-bit density streams. Product teams often treat that short PCB trace as a trustworthy sensor connection. It is rarely logged, authenticated, or monitored once the product leaves manufacturing.

That trust assumption creates a useful security-research target. A substituted microphone can be stuck, mirrored from another channel, electrically degraded, or replaced by a tiny digital source. Compromised firmware can change the microphone clock, select the wrong stereo edge, or silently disable a channel. Connector corrosion and cracked solder joints can produce intermittent patterns that look like application bugs. A malicious interposer could feed synthetic samples while leaving the operating system, codec driver, and application convinced that a genuine microphone is present. Traditional audio tools show the sound after decimation, filtering, automatic gain control, beamforming, and operating-system processing; they do not prove what happened on the physical PDM wires.

The Probe sits between a host codec and as many as four PDM data lines. A normally closed analog path preserves the original clock and data wiring while unpowered. High-impedance comparators feed an iCE40UP5K FPGA, which measures clock rate, one density, transition density, edge alignment, channel correlation, stuck intervals, and electrical contention. The FPGA exports only fixed-size statistical windows to an STM32H563 controller. The controller applies learned envelopes, records bounded integrity events, and serves the local USB companion application. Raw one-bit microphone samples are not placed in external flash, sent over USB, or retained after each FPGA window is reduced.

For authorized fault-injection testing, a separate active path can emit one of four fixed non-speech patterns: alternating bits, silence-density midpoint, all-zero, or all-one. It cannot accept arbitrary sample uploads. Intervention requires a physical ARM button, an expiring software lease, and a duration no longer than 250 milliseconds. This allows engineers to verify failover, mute indicators, channel-health telemetry, and application recovery without turning the instrument into a general audio injector.

The concept is distinct from the repository's acoustic side-channel tools and general bus analyzers. Acoustic Phantom captures environmental sound for analysis; PDM Trust Probe protects the electrical trust boundary between a digital microphone and its consumer. It is not an audio recorder, beamformer, voice assistant, RF device, or universal logic analyzer. Its primary artifact is integrity evidence: when a channel changed, which metric violated policy, and whether the physical clock and data behavior remained inside a baseline.

## Requirements, scope, and non-goals

Revision A0 targets single-ended 1.8 V and 3.3 V PDM links from 512 kHz through 4.8 MHz, with one shared clock and up to four data inputs. It supports microphones sampled on either clock edge and boards that multiplex two logical channels on one wire. The reference firmware uses a host simulation so protocol, queue, calibration, and policy logic can be compiled and tested without the target MCU SDK. An embedded port must replace the board functions and FPGA transport with the STM32 HAL or a reviewed register-level implementation.

The Probe must add less than 8 pF to a passively observed line, must not source target I/O voltage, and must return to a transparent bypass state after reset, watchdog expiry, USB disconnect, or loss of its own power. Capture is useful without active insertion. The operator first measures target I/O voltage, verifies ground reference, and observes the bus in bypass mode. Active tests are limited to fixtures and spare systems that can tolerate malformed microphone input.

A0 is not production-validated. It has not received signal-integrity simulation, formal privacy certification, environmental testing, EMC testing, KiCad ERC/DRC on this machine, or physical bring-up. The KiCad files are concrete engineering source with component footprints, nets, an outline, and routed representative signals, but they remain a review artifact. Do not order boards until an electrical engineer validates the target-voltage protection, analog-switch bandwidth, comparator thresholds, connector pinout, FPGA configuration, ESD strategy, and manufacturing outputs.

Non-goals include speech reconstruction, wake-word detection, speaker identification, acoustic content export, arbitrary waveform injection, bypassing hardware privacy switches, attacking public devices, hiding the instrument, and certifying that a microphone is authentic. Electrical statistics can detect many substitutions and failures, but they are not cryptographic identity. A sophisticated emulator may reproduce a learned envelope. For high-assurance products, combine the Probe with component provenance, secure boot, physical inspection, and authenticated sensors.

## Attack surface and threat model

### Assets and security claims

The protected assets are microphone availability, channel identity, evidence integrity, user privacy, and the correctness of host behavior during sensor faults. The design claims that its normal capture path exports statistics rather than reconstructable audio; active output cannot contain uploaded speech; commands are bounded and replay-checked; and power or software failure returns the board toward passive bypass. It does not claim to stop a physical attacker, authenticate microphone silicon, or prove that no speech can be inferred from every possible statistic. Researchers should assess whether their chosen window length and metric set leak information in their jurisdiction and application.

### Trust boundaries

The host clock is untrusted because compromised firmware or a failing codec may change its frequency, duty cycle, or enable timing. Every target data line is untrusted because it may be driven by a genuine microphone, a counterfeit part, a damaged assembly, another channel through a short, or an intentional emulator. The FPGA is inside the measurement boundary but must treat asynchronous edges carefully and contain metastability. The MCU trusts only fixed-width FPGA records with checked lengths and counters. USB commands are hostile inputs. The companion browser is outside the safety boundary and cannot directly drive pins; it requests operations that firmware independently authorizes.

The operator is also part of the threat model. An authorized engineer may select an unsafe voltage, connect to a mains-referenced product, mistake a logical channel mapping, or export evidence that identifies a product or test subject. The hardware therefore senses but never powers target VIO, prints active state on a red LED, uses a recessed physical button, and stores no default audio payload. Exports identify metrics and firmware version, not USB serial numbers or audio.

### Adversaries and failure cases

A supply-chain attacker may replace one microphone with a programmed source that emits plausible density. A compromised embedded controller may disable its privacy indication while leaving microphones clocked. A manufacturing defect may swap left and right channels, mirror a stream, or leave a data line open. A damaged flex cable may produce burst errors correlated with motion. An application may fail open when it receives all-zero or all-one PDM. A malicious USB client may send oversized frames, repeat sequence numbers, extend an arm lease, or request a long active pattern. A local user may remove power during intervention.

The Probe detects stuck-high and stuck-low windows, improbable density, abnormal transition rate, duplicate statistical signatures across channels, clock drift, clock absence, FIFO overflow, active-path contention, and unexpected bypass state. It does not label the semantic content of audio. Correlation findings are evidence for investigation rather than proof of compromise because quiet rooms, identical test tones, and tightly matched microphone arrays can legitimately look similar.

### Defensive controls and residual risk

All queues are fixed-size. Overflow drops the oldest analysis event, increments a counter, and never changes target output. Policy updates validate ordered bounds and supported clock ranges. USB sequence numbers must increase. The ARM exchange uses a nonce to prevent accidental replay, but the included challenge transform is not cryptographic authentication; production deployments should use the ATECC608B and a signed session transcript. The physical button must remain asserted. A lease expires after sixty seconds, and each pattern is at most 250 milliseconds. Arbitrary bitstreams are absent from the protocol.

Residual risks include comparator loading, switch distortion, ground loops, FPGA timing defects, and inference from long-term density statistics. An emulator can mimic basic statistics. Fixed patterns can still upset poorly designed audio software. A relay may bounce. The bypass path does not protect against an incorrectly wired connector. The host simulation validates portable logic, not STM32 peripheral configuration or FPGA RTL. These limits are why A0 is a lab reference rather than a fabrication-ready product.

## Hardware specification

| Function | Selected part or characteristic | Rationale |
|---|---|---|
| Control MCU | STM32H563ZIT6, Cortex-M33, 2 MiB flash, 640 KiB SRAM | USB device, TrustZone option, high-speed SPI, timers, CRC, mature toolchain |
| Capture FPGA | Lattice iCE40UP5K-SG48 | Deterministic multi-channel edge capture, small static power, open tooling option |
| Passive receivers | SN74AXC4T245 plus threshold-safe front end | Four channels, target-referenced input, controlled loading |
| Clock receiver | SN74LVC1T45 with series damping | Separates host clock measurement from target data |
| Active switch | TMUX1574 four-channel 2:1 switch | Selects original or fixed FPGA pattern only when armed |
| Fail-safe bypass | Omron G6K-2F-Y relay for clock and dual data plus default analog pass path | De-energized continuity; additional channels use direct passive taps in A0 |
| Voltage sensing | ADS7042 ADC and protected divider | Confirms 1.8 V or 3.3 V target domain without powering it |
| Current and fault monitor | INA226 on management rail; fast contention comparator | Detects board faults and drive collisions |
| Secure identity | ATECC608B | Optional production pairing and signed evidence manifest |
| Local storage | 16 MiB MX25L128 QSPI NOR | Bounded event journal; never stores sample windows |
| Host connection | USB-C full-speed CDC/WebUSB | Local operation without radio or cloud service |
| Controls | recessed ARM button, BYPASS switch, red/green LEDs | Explicit physical authorization and visible state |
| Target connectors | two 10-pin 0.5 mm FFC connectors and 0.1-inch test header | Inline fixtures and accessible bench probing |
| Power | USB 5 V, TPS62172 3.3 V, TPS62840 1.2 V FPGA rail | Efficient isolated management power; target VIO sensed only |
| Board | 130 mm × 72 mm A0 bench board, four layers | Short PDM routes, continuous reference plane, labeled test points, room for probes |

The board uses TVS protection at USB and low-capacitance ESD parts on target lines. Each PDM trace has an optional 22–47 ohm series resistor and a no-load shunt footprint for tuning. The target connector exposes CLK, DATA0 through DATA3, VIO sense, two grounds, fixture detect, and shield. PDM Trust Probe never sources VIO. An external USB isolator is required when the target ground is floating or its safety classification is unknown.

## Architecture and block diagram

```text
        authorized host codec                       target microphones
      CLK DATA0 DATA1 DATA2 DATA3                  CLK DATA0..DATA3
          |     |     |     |                         |      |
          +-----+-----+-----+---- inline FFC ----------+------+
                        |
                 +------v-------+     de-energized state
                 | passive taps |-------------------- bypass
                 +------+-------+
                        |
          +-------------v----------------+
          | target-referenced receivers  |
          +------+-----------------------+
                 | four data + clock
          +------v-----------------------+
          | iCE40UP5K measurement fabric |
          | counters / edge timing /     |
          | correlation / bounded FIFO   |
          +------+---------------+-------+
                 | statistics    | four fixed patterns
                 | SPI           v
          +------v---------+  +--+----------------+
          | STM32H563      |  | TMUX active path |
          | policy / USB / |  | physical arm gate|
          | event journal  |  +-------------------+
          +---+--------+---+
              |        |
          QSPI events  ATECC608B
              |
            USB-C <------> local PDM Trust Console
```

There are two data planes. The passive measurement plane observes edges and cannot drive the target. The intervention plane is separately gated by the FPGA watchdog, MCU lease, physical button, and analog-switch enable. The MCU never performs per-bit forwarding; that would introduce nondeterministic latency and unsafe failure behavior. It configures a compact FPGA policy and receives summary records. If either processor stops servicing its watchdog, the active enable drops and the original path remains selected.

A measurement window contains a timestamp, clock count, per-channel one count, per-channel transition count, channel mask, and anomaly flags. At 2,048 clocks, it is small enough for responsive detection but contains far less information than the original bit sequence. Production privacy review may choose larger windows or coarser quantization. Firmware converts each window into events only when policy is violated and maintains aggregate health for display.

## Firmware architecture and design decisions

The `firmware/` directory is portable C11. `main.c` provides a deterministic host simulation, board contract, tests, and service loop. `drivers/pdm_capture.c` models the bounded FPGA window queue and calculates statistics. `drivers/analyzer.c` applies density, transition, duplicate, and clock policies. `drivers/diagnostics.c` accumulates calibration envelopes, derives conservative policy bounds, produces channel-health scores, and creates a non-secret configuration fingerprint. `drivers/protocol.c` validates the PTP/1 management frames, monotonic sequence numbers, arm challenge, lease, and pattern duration.

No module allocates memory dynamically. All lengths are checked before copying. The oldest event is discarded on queue saturation so the target path is unaffected. Shared ISR data in the embedded port must be volatile or exchanged through DMA descriptors with explicit ownership. The host build deliberately avoids vendor headers, allowing GCC or Clang to compile the safety-critical parsers and state machines with warnings treated as errors.

The reference challenge transform proves state-machine behavior but is not a message authentication code. A production port should ask ATECC608B to verify a signed arm request bound to device identity, firmware measurement, monotonic counter, operation, and lease. Firmware update should use an immutable signed first stage, rollback protection, readout protection, and a recovery strap. Debug access remains available on A0 and must be locked or physically controlled in a deployed instrument.

The FPGA register map exposes identity, control, status, measured clock, edge-error count, FIFO level/data, enabled-channel mask, gate duration, fixed pattern choice, word count, and watchdog. RTL is outside this reference commit; the register contract and hardware connections define its integration boundary. Before claiming embedded readiness, implement and simulate clock-domain crossing, input synchronization, FIFO overflow, watchdog reset, and analog-switch enable timing.

Build and test the host firmware:

```sh
make -C firmware clean all test
```

The self-test checks clock limits, sample-window accounting, stuck-line detection, calibration policy generation, health scoring, fingerprints, physical arm enforcement, duration bounds, lease expiry, and replay rejection. A hardware port should add tests with recorded logic-analyzer fixtures and a HIL jig that toggles real PDM voltage domains.

## PTP/1 application interface

PTP/1 is a little-endian USB framing protocol. Requests begin with magic `PTP1`, a 32-bit monotonically increasing sequence, a 16-bit payload length, protocol version, command, and up to 244 payload bytes. Commands query identity/status, start or stop statistics, set a policy, arm a session, run a fixed pattern, read events, and reset. Unknown versions, duplicate sequences, incorrect lengths, unsupported fields, and expired leases are rejected.

The Vite/React companion application is a local console with four real views. Dashboard shows connection, bypass or observation state, measured clock, privacy mode, and violation count. Channels shows density and transition meters with a health explanation. Events presents timestamped findings and exports a JSON evidence report. Policy exposes the clock/density envelope and fixed-pattern duration while making the physical-arm requirement visible. A mock-device flow lets reviewers build and exercise every screen without hardware.

The app includes a tested codec for frames and fixed event records, a WebUSB transport that opens, claims, and writes to a selected local device, and a deterministic mock transport for hardware-free review. Start/stop and the bounded ARM-plus-pattern flow send real PTP/1 frames. It has no cloud endpoint, analytics package, login, microphone API, or audio playback component. Export is explicit and generated locally. The demonstration data is visibly mock data. Production transport work still needs assigned USB identifiers, response reads, capabilities negotiation, timeouts, cancellation, and disconnect recovery without retrying active commands.

Build and test the application:

```sh
cd app
npm install
npm test
npm run build
```

## Use cases

Security researchers can evaluate whether an embedded product notices an unplugged, stuck, duplicated, or electrically substituted microphone. They can compare the physical PDM clock with operating-system claims, characterize boot-time and suspend/resume behavior, and test whether mute controls truly stop the microphone clock or merely discard audio later in software. Supply-chain teams can record a statistical baseline from known-good assemblies and triage outliers without collecting staff conversations.

Red teams with explicit authorization can demonstrate the weakness of unauthenticated sensor links by inserting the Probe into a spare conference appliance or development board. The bounded fixed patterns test whether applications fail closed, show a degraded-sensor indication, or silently treat invalid input as real. Because arbitrary audio injection is unavailable, the exercise stays focused on trust and response rather than impersonation or covert recording.

Product-security teams can reproduce intermittent field failures. The event timestamp can be correlated with power, vibration, thermal chamber, firmware, and UI logs. Manufacturing engineers can find swapped stereo edges, mirrored data lines, wrong clock configurations, and assembly opens. Privacy engineers can verify which power states keep digital microphones active and whether a hardware mute switch interrupts the physical link.

Penetration testers can include sensor-path integrity in an authorized assessment report. Evidence can show clock frequency, policy revision, channel number, anomaly metric, device firmware version, and dropped-event count. It cannot show what a person said. That distinction supports useful security testing while reducing collection of unrelated private content.

## Build, flash, test, and recovery

Start with a continuity check while the board is unpowered. Confirm the intended host-to-microphone pins and target voltage using the product schematic and a current-limited bench setup. Power the Probe from an isolated USB source, leave BYPASS asserted, and verify the green LED. Connect the companion app in mock mode first. On target hardware, compare the measured clock against an oscilloscope before enabling event capture.

Calibration requires at least eight windows under representative benign conditions. It should include quiet and expected acoustic activity, boot, suspend, resume, and legitimate mute transitions. Review generated bounds rather than applying them blindly. Store a baseline only after confirming channel mapping. Begin active tests on a disposable fixture at the 25 ms duration and one channel. Observe the host's response and target current. Never use all-one or all-zero tests on unknown production equipment.

Recovery is intentionally simple. Release ARM, move BYPASS to passive, disconnect the target, and power-cycle the Probe. Holding BYPASS during reset prevents intervention. Factory reset should erase policies, pairing material, and event metadata but not alter the target. A corrupted application cannot override the relay and FPGA watchdog. If passive continuity is absent with the board unpowered, stop and repair the fixture rather than attempting a software workaround.

## Manufacturing and validation notes

Use controlled-impedance guidance only after measuring the target stackup; most short PDM routes are not strict transmission lines, but edge rates still make return paths and stubs important. Keep receiver taps short, place series resistors near drivers, keep switch and connector capacitance in the simulation, and provide labeled test points for clock, every data channel, VIO, relay enable, FPGA watchdog, and ground. Place ESD devices close to connectors and avoid sharing their discharge path with comparator references.

Before fabrication, regenerate the schematic and PCB with a supported KiCad release, run ERC and DRC, inspect every symbol-to-footprint mapping, review the bill of materials for lifecycle and voltage ratings, and perform an independent privacy and safety review. The included `kicad/DESIGN_NOTES.md` records the unverified status. Bring-up should use a PDM signal generator, current-limited supplies, a scope with low-capacitance probes, and test firmware that cannot enable the active path.

## Security limitations, maintenance, and roadmap

Statistics are not identity. A capable emulator can match density and transition envelopes, and channel correlation can false-positive on shared test tones. Future work may add challenge sensors, board-specific timing fingerprints, and signed baselines, but none should be marketed as cryptographic authentication without rigorous analysis. Long-window statistical streams may leak activity patterns, so retention and export must be minimized.

Maintenance should keep the host build warning-free, expand malformed-frame tests, pin app dependencies, and document embedded toolchain versions. Hardware revisions require a pin-map review shared by schematic, PCB, `board.h`, FPGA constraints, and README. Protocol changes increment the version and preserve safe rejection of unsupported clients. Evidence schema changes must remain backward readable.

The roadmap includes reviewed FPGA RTL, STM32H563 USB/SPI implementation, hardware-in-loop fixtures, measured input capacitance, signal-integrity characterization across both voltage domains, signed firmware, secure arm authorization, a reproducible BOM, and independent KiCad review. Until those items are complete, A0 must be described as a host-tested reference design, not certified instrumentation.

## License and provenance

The original concept, documentation, firmware, application, and hardware source in this directory are authored by jayis1. Hardware design files are offered under CERN-OHL-S-2.0; firmware and application source are offered under MIT unless a file states otherwise. Component names and trademarks belong to their owners. This independent device directory does not modify or depend on a unified systems tree, SoC invention framework, or shared command-line platform.
