# Threshold Memory — Hysteretic Alarm-Policy Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol TH1. Firmware, application, and tests: GPL-2.0-only. Hardware utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked embedded firmware, a Linux desktop companion, and native routed KiCad files. See VERIFICATION.md for executed software and CAD checks. Fabrication, physical programming, electrical interoperability, calibrated timing, and deployment are not claimed. This instrument observes diagnostic indications; it is not an alarm controller, protective interlock, or certificate of security compliance.

## Purpose, practical value, and originality

Threshold Memory investigates a history-dependent alarm policy that an ordinary instantaneous comparator cannot validate. An authorized target exports a two-bit severity code, an actual alarm indication, and an observation-inhibit indication. Severity zero establishes expected alarm low. Severity three establishes expected alarm high. Severity one or two must retain the previously established expectation. Therefore an identical present input code can legitimately require opposite outputs depending on which extreme was seen most recently. The instrument reconstructs that history independently and reports missing alarms, spurious alarms, observed mismatch episodes, and coverage.

A practical example is an appliance that raises a security diagnostic after a confidence score crosses an upper boundary but should retain the alarm until the score crosses a lower recovery boundary. A defective implementation may compare against one threshold everywhere, drop alarm state during an ordinary mid-band update, or initialize the alarm from the wrong side after a maintenance inhibit. The observer makes these differences visible without recording the underlying user activity, credentials, payload, or detailed score. The four-code diagnostic adapter must be reviewed separately: the instrument checks the exported policy, not the truth of the original score.

The repository's device directories and README titles were inventoried before selection, and the closest retained-state designs were examined. Sticky Trip observes fresh authorized clear edges and a set-dominant latch. Grant Ledger accounts for single-use authorization credit. Lease Watch compares heartbeat freshness to an enable. Threshold Memory instead checks two-boundary hysteresis, automatic recovery at the lower boundary, and retained middle-band state with a finite output-response grace. It has no clear-command input, authority input, authorization transaction, or heartbeat lease. Originality is claimed within the inspected repository, not as worldwide novelty or patentability.

The passive carrier topology, STM32 register support, startup, CRC encoding, and desktop transport intentionally reuse jayis1-authored infrastructure from Backoff Ramp. Reusing reviewed electrical and transport components is preferable to inventing a different MCU for cosmetic uniqueness. The new device contribution is the hysteresis observation contract, bundled severity qualification, response-grace accounting, evidence invariants, dedicated tests, and laboratory workflow. Every delivered file credits jayis1 using comments or format-compatible metadata.

## Authorized-use disclaimer and threat model

Use this device only on equipment you own or have explicit written permission to assess. Agree on the diagnostic adapter, exact connection points, target firmware revision, allowed operating states, stopping procedure, and retention period before attaching wires. Red-team authorization must explicitly cover physical instrumentation; a general network assessment scope is insufficient. Use isolated development units with consequential outputs disconnected. Never connect this unisolated prototype to installed alarms, access-control installations, machinery, medical equipment, vehicles, mains circuits, or other life-safety systems.

The assessed attack surface is the boundary between a severity classification and its retained alarm state. The target may have incorrect state initialization, reordered updates, an accidental one-threshold implementation, or an alarm-output path that diverges from its internal variable. The observer receives diagnostic signals rather than driving the target. It implements no credential capture, exploit delivery, active fault injection, radio transmission, persistence, scanning, or remote control. Approved stimuli are applied through the target's own development workflow by the operator, not through this instrument.

A malicious target can lie on every diagnostic output. If severity and alarm are both derived from a single intended software variable, a test can become circular and conceal the real output defect. Derive the alarm indication as close as practical to the actual retained register or observable output, and document how severity bins are exported. No evidence record proves the correctness of a sensor, risk model, cryptographic decision, or physical intrusion detector upstream of that adapter.

The operator, wiring, programmed firmware, and workstation are trusted. CRC detects accidental corruption, not forgery. There is no secure boot, signed evidence, authenticated pairing, persistent boot identity, encrypted serial transport, or tamper-resistant key storage. A hostile host or substituted USB serial device can fabricate plausible telemetry. Keep programming access controlled and preserve operator notes separately if evidence handling requires chain of custody. Reports contain activity and timing metadata; even without payloads, treat them as confidential engagement material.

Receive-only firmware is not a hardware data diode. Reprogramming the MCU could make its input pins outputs, and common-ground connections, shorts, loading, and back-powering remain real hazards. The carrier has no isolation, active buffers, dedicated ESD suppression, or overvoltage protection. The legal notice does not replace electrical engineering review. Stop immediately if the target behaves unexpectedly, a connection heats, or a ground relationship is uncertain.

## Hardware specifications and bill of materials

The external processor module is an ST NUCLEO-F401RE, using STM32F401RET6, Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI operation and uses SysTick for a nominal one-millisecond tick. USART2 TX on PA2 emits 115200 baud, eight data bits, no parity, one stop bit. ST-LINK provides the USB virtual COM bridge and programming interface. Its receiver is not enabled by this firmware. The external module supplies its own regulator, decoupling, USB connector, reset circuitry, and debugger; those parts are not silently missing from the passive carrier.

The carrier is an 80 by 70 mm, two-layer PCB with nominal 1.6 mm thickness, through-hole components, a closed outline, and 0.3 mm tracks. It attaches using short labelled jumper leads, not as a mechanically mating shield. There is no battery, charger, radio, antenna, FPGA, analog sensor, flash journal, display, or actuator. Use an insulating support and external strain relief. No enclosure CAD or mounting holes are provided in revision A. The module and cables sit alongside the carrier on the bench.

| Reference | Quantity | Value or requirement |
| --- | --- | --- |
| External module | 1 | NUCLEO-F401RE, purchased MB1136 revision to be inspected |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pull-down |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3–J6 | 5 | Straight 1x2 header, 2.54 mm pitch |
| J2 | 1 | Straight 1x4 header, 2.54 mm pitch |
| Harness/support | 1 set | Insulated short leads, nonconductive tray, strain relief |

Resistor footprints are DIN0207 with 7.62 mm lead spacing. Candidate resistor families include Yageo MFR-25; candidate header families include Samtec TSW with matching pitch and row count. These are dimensional suggestions, not verified inventory or procurement guarantees. Check supplier drawings against the actual footprint before ordering. The CAD manifest records all fourteen components, thirty electrical pads, nine nets, values, and footprint identities for automated comparison.

| Raw bit | Signal | Target pin | MCU-side pin | Nucleo point |
| --- | --- | --- | --- | --- |
| 0 | LEVEL0, severity least significant bit | J3.1 | J2.1 | A0 / PA0 |
| 1 | LEVEL1, severity most significant bit | J4.1 | J2.2 | A1 / PA1 |
| 2 | ALARM, active high | J5.1 | J2.3 | A2 / PA4 |
| 3 | INHIBIT, active high | J6.1 | J2.4 | A3 / PB0 |
| none | Common ground | J3–J6.2 | J1 both pins | Nucleo GND |

J1 is ground only: neither pin is a supply. J2 carries four signal nets and no ground. Each target signal passes through 1 kOhm to an MCU-side node with a 100 kOhm pull-down. Nets ending `_IN` are target-side nodes. All inputs require common-ground nominal 0–3.3 V push-pull logic. Do not attach 5 V or 1.8 V logic, negative voltages, differential buses, relay coils, mains, automotive wiring, or unknown industrial signals. A disconnected lead tends low and is not reliably distinguished from a legitimate indication.

The resistor path is nominally 101 kOhm at a high input. Actual loading and threshold margin depend on resistor tolerance, target source impedance, input leakage, and cable capacitance. No measured electrical performance is claimed. Reserve a conservative 5 V, 500 mA USB-source allowance for the development assembly, subject to the board's documented power limits; consumption has not been measured. Power the observer first, establish compatible ground, then attach signals. Remove signals before depowering the observer to reduce back-power risk. Series resistance is not an isolation or surge barrier.

## Architecture and block diagram

```text
Authorized target       Passive carrier                    External Nucleo
LEVEL0 ---- J3.1 ------- 1k --+-- J2.1 ---------------------- PA0 / A0
LEVEL1 ---- J4.1 ------- 1k --+-- J2.2 ---------------------- PA1 / A1
ALARM ----- J5.1 ------- 1k --+-- J2.3 ---------------------- PA4 / A2
INHIBIT --- J6.1 ------- 1k --+-- J2.4 ---------------------- PB0 / A3
                            | each MCU-side node
                           100k
                            |
GND ------- J3..J6.2 ------- GND -- J1 --------------------- GND / USB

GPIO reads -> bundled severity + independent alarm/inhibit qualification
 -> retained threshold expectation -> ten-tick output settling allowance
 -> coverage and mismatch ledger -> transactional counter commit
 -> TH1 CRC records -> bounded USART TX -> ST-LINK virtual COM
 -> Linux Ledger / History / Integrity screens -> private JSON export
```

GPIOA is read before GPIOB; acquisition is not atomic across ports. The severity bus is entirely on GPIOA, but software sampling is still not a high-speed capture instrument. Start with twenty-millisecond or longer stable plateaus and generous timing margins. Corroborate the diagnostic waveform with an independent analyzer. This device does not measure analog thresholds, comparator hysteresis voltage, metastability, electrical pulse width, or fast glitch susceptibility.

## Exact firmware contract and design decisions

Severity is the integer formed by LEVEL1 and LEVEL0. The two bits qualify together as one bundle. ALARM and INHIBIT qualify separately. A new candidate sample followed by three matching successive one-millisecond observations establishes a value, so four samples are needed after a change. Repeated calls in one millisecond do not add qualification. Bundling severity avoids constructing an artificial threshold from independently delayed data bits; separate alarm qualification prevents alarm chatter from indefinitely postponing a severity threshold. Qualification cannot remove every artifact of a poorly designed adapter, so severity changes must still be stable and intentionally paced.

Previously qualified values remain accepted while a replacement candidate is qualifying. Raw glitches that never qualify do not invalidate that retained value. Initial comparison waits for all three groups to qualify. The valid mask reports this distinction. The device therefore checks qualified diagnostic behavior, not continuous raw electrical stability. A short ALARM pulse may be rejected, and an INHIBIT pulse shorter than qualification may be invisible. These are defined coverage limits rather than proof that those pulses never existed.

State zero means UNKNOWN. Initial middle-band severity one or two never establishes an expectation, even if ALARM is low. Qualified severity zero establishes state one, EXPECT LOW. Qualified severity three establishes state two, EXPECT HIGH. At either middle value, the previous known expectation persists. Crossing between middle values does not restart an allowance or alter expectation. Returning to the same extreme likewise does not repeatedly restart allowance. The supplied instrument therefore distinguishes real hysteresis from a combinational threshold.

Whenever the expected state actually changes, including UNKNOWN becoming known, the engine starts a ten-tick settling allowance and closes any current mismatch episode. The transition observation itself is not compared. Each subsequent observed tick decrements the allowance. Comparison begins on the tenth following tick, including an alarm update qualified on that same tick. This is a bounded response contract rather than a strict causality claim about sub-tick ordering. New expectation changes can start new allowances; rapidly changing thresholds can consequently yield little coverage and must not be presented as a clean test.

Qualified INHIBIT high makes state UNKNOWN, cancels settling, and closes the current episode. Historical evidence remains. On release, a still-qualified extreme can establish a new expectation; a middle value cannot. INHIBIT is a statement that the operator-defined diagnostic contract is out of scope, not an authenticated authorization or proof of a real reset. A target that asserts it continuously can suppress coverage. Inspect coverage alongside findings and use the authorized adapter specification to judge whether inhibition was legitimate.

Each comparison increments coverage and exactly one of good, bad_high, or bad_low. Here bad_high means expected high but observed low, and bad_low means expected low but observed high. These field names refer to the expected direction, not the observed voltage. Mismatch records whether the current comparison is bad. Episodes count transitions into mismatch, streak counts current consecutive bad comparisons, and longest retains the greatest streak. Unknown intervals, settling, gaps, or matched comparisons close a segment. A continuing physical fault split by a gap can produce multiple observed episodes; counts are not guaranteed distinct real-world incidents.

Rises and falls count entries into expected high and expected low, including acquisition from UNKNOWN. They are not counts of raw electrical transitions. All cumulative counters are unsigned 32-bit and never wrap silently. A sample is evaluated on a temporary ledger. Any exhausted counter rolls back the entire speculative sample, retains earlier history, clears active acquisition state, and enters terminal halt. Transmission-failure diagnostics saturate separately. This preserves the identity coverage equals good plus both mismatch totals even at exhaustion.

Skipping a nominal millisecond records an acquisition gap and missing ticks, discards qualification and expected memory, and requires fresh qualification. A newly observed extreme can recover memory; quiet middle-band data cannot. The lost interval never earns coverage. Ordinary unsigned timestamp wrap is handled, but an entirely unseen full clock cycle cannot be reconstructed. Watchdog initialization uses bounded waits; failure produces halted evidence before telemetry. The foreground UART driver sends at most one byte per call and abandons a blocked frame after one hundred milliseconds. Firmware uses no heap, RTOS, command receiver, filesystem, or network stack.

## Companion application and evidence interface

Run `python3 threshold-memory/app/console.py` on Linux with Python Tk installed. Enter the explicit serial path, normally a selected `/dev/ttyACM` device, and press New live. No automatic probing or transmission is performed. Disconnect before selecting another live port. The app opens read-only, configures 115200 8N1, flushes pending bytes, and restores original terminal settings on close where the device remains present. A failed connection does not discard previously retained evidence.

Ledger displays every measurement field, decoded fault names, and expected state. History retains up to 256 accepted snapshots. Integrity displays source, provenance, connection status, policy compatibility, parser errors, reset information, age, and freeze reason. Live status requires an open connection, compatible policy, two advancing snapshots, and recent progress. Live refers to transport progress, not valid acquisition: UNKNOWN or halted state must still be interpreted explicitly. Disconnect preserves evidence and provenance but removes live status.

Replay accepts regular files up to two MiB and rejects FIFOs, devices, and oversized data before unbounded allocation. Replayed evidence is never labelled live. A partial final frame is an error even if earlier complete snapshots were valid. Export JSON creates a new file with private permissions and refuses overwrite. It records author jayis1, source, origin, diagnostics, current snapshot, and history even when no completed useful test exists. There are no cloud uploads, listeners, analytics, or third-party Python dependencies.

Wire frames are ASCII `TH1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC16/CCITT-FALSE covers the bytes before the asterisk, uses polynomial 0x1021 and initial 0xffff, and is written as four uppercase hexadecimal digits. B carries MCU reset flags once per boot. P carries `3,10,0,3`: qualification intervals, settling ticks, low boundary, high boundary. Policy repeats approximately once per second for late attachment; it never resets host evidence. S contains these unsigned 32-bit fields in order:

```text
now levels valid halted faults gaps missed state settle mismatch coverage
 good bad_high bad_low episodes streak longest rises falls tx_fail
```

Fault bits one, two, four, eight, and sixteen mean missing high alarm, spurious high alarm, acquisition gap, exhausted counter, and unavailable watchdog. The encoder and app both check conservation, masks, state consistency, mismatch episode bounds, and fault-counter relationships. The host rejects counter rollback, disappearing sticky findings, contradictory duplicate timestamps, and impossible sparse transitions. These are necessary plausibility bounds, not cryptographic authenticity or a full trace realizability proof.

A boot received after evidence freezes the existing session rather than silently deleting it. A lost boot can sometimes be inferred from counter or timestamp rollback; a reset with forward-looking counters and time is inherently indistinguishable without persistent identity. Host/device progress is checked in both directions with a capped tolerance to detect queued backlog. A prolonged reception pause freezes evidence and requires explicit reconnect. Long uncalibrated sessions can conservatively fail this check due to oscillator drift; do not disable it merely to make a stale stream appear live.

## Build, checks, and authorized laboratory workflow

Required tools are GNU Arm Embedded GCC with newlib, host GCC, Python 3 with Tk, Xvfb for headless UI tests, and KiCad 9 with pcbnew. There is no downloaded firmware SDK or package installation hook. From the repository root:

```sh
make -C threshold-memory/firmware clean all test
xvfb-run -a /usr/bin/python3 -m unittest discover -s threshold-memory/tests -p test_app.py -v
/usr/bin/python3 threshold-memory/tools/check_hardware.py
python3 threshold-memory/tools/check_release.py
```

The target build links a real Cortex-M4 reset path and memory map and creates ignored ELF, BIN, and map files. Host tests execute the production engine and encoder, plus the UART register model. The fixture executable generates explicitly synthetic records through the real firmware engine; app tests feed those records through the production parser and a real Tk event loop with a Linux pseudo-terminal. This is not a claim of physical USB interoperability or a flashed board.

The hardware checker runs ERC and DRC, exports a fresh schematic XML netlist, and compares reference, pin, net, value, and footprint identity against PCB pads and the manifest. Regenerate deliberate CAD changes with `tools/generate_hardware.py`, then rerun checks and review a rendered schematic. Passive-component ERC cannot validate the external Nucleo regulator, board revision, or electrical compatibility. Clean routing checks do not establish fabrication readiness, ESD tolerance, or enclosure suitability.

For bring-up, inspect the unpowered assembly, verify connector orientation and ground continuity, and check for unexpected shorts. Review ST UM1724, RM0368, and the STM32F401xD/xE datasheet against the purchased board. Confirm Arduino analog pin mapping and ST-LINK virtual COM solder bridges before programming. Manufacturer entry points are https://www.st.com/en/evaluation-tools/nucleo-f401re.html and https://www.st.com/en/microcontrollers-microprocessors/stm32f401re.html. Program only through a supervised ST-LINK workflow at the linked flash origin.

Begin with an approved signal generator or instrumented development target, not a deployed alarm. Establish severity zero and alarm low, then severity three and alarm high, then return to severity one while holding alarm high. Compare that with an intentionally defective target that drops alarm in the middle band. Repeat from the low side to demonstrate that the same middle-band code has different expected outcomes. Measure the raw waveforms and qualified latency with an independent instrument. Keep the fixture source, target revision, calibration notes, wiring photographs, and exported evidence together.

A useful red-team report describes the violated declared retention policy and the exact observed limitations, not an unsupported claim of exploitability. Researchers can compare firmware revisions for accidental loss of hysteresis. Penetration testers can validate an approved maintenance-inhibit workflow and document whether it suppresses meaningful coverage. Remaining physical work includes fabrication, programming, loading measurements, timing calibration, brownout behavior, watchdog behavior, hot-plug review, mechanical fit, and regulatory assessment. None is replaced by passing software tests.
