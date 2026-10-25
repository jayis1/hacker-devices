# Entropy Sentry — Random-Source Health and Release Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol ES1. Firmware and application: GPL-2.0-only. Hardware tooling: MIT. Carrier hardware: CERN-OHL-S-2.0.

Status: engineering prototype design with linked STM32 firmware, an executable Linux companion application, and routed carrier CAD. Physical fabrication, programming, electrical qualification, timing calibration, and source-specific statistical validation have not been performed. This is an observation instrument, not an entropy source, a randomness certificate, or an enforcement device.

## Purpose, novelty, and practical value

Entropy Sentry examines a security boundary that ordinary random-number test programs do not observe: whether a device continues releasing random material after its exposed diagnostic source stream shows a gross health failure. A software randomness suite can identify a biased recording, but usually cannot independently correlate that failure with the target's alarm indication and release permission. This instrument observes four low-speed diagnostic wires, performs two deliberately simple health checks, and preserves aggregate failure and release evidence without exporting the individual source bits.

The intended fixture exposes BIT, CLOCK, ALARM, and RELEASE as separate 3.3 V signals. BIT is a laboratory diagnostic sample, CLOCK marks its sampling edge, ALARM is the target's own health-failure indication, and RELEASE describes the target's externally observable permission to release generated material. A researcher can introduce a stuck or biased source through the target's documented laboratory test mode and ask whether the alarm appears and whether release remains asserted. Entropy Sentry does not generate the stimulus, change the target, read its secrets, or deliver random bytes to any consumer.

Repository inventory found protocol interposers, side-channel instruments, microphone statistics, and several permission-sequence witnesses, but no dedicated random-source health-to-release observer. PDM Trust Probe summarizes a microphone bus; this device instead processes a clocked diagnostic bit sequence with persistent health-test failures and correlates those failures with random-material release. Reset Quarantine examines readiness after restart, and Lease Watch examines heartbeat continuity. Neither implements this statistical source-health contract. Originality is claimed for the repository-local combination and workflow, not as a worldwide patent or prior-art determination.

The passive carrier topology and low-level NUCLEO support are intentionally reused from jayis1's existing receive-only designs. Reusing a simple electrical interface is preferable to inventing an unnecessary analog front end. The health engine, ES1 protocol, tailored screens, and regression tests implement the new function. No claim is made that renaming resistors creates a new circuit invention.

## Authorized use and threat model

Use only on equipment you own or are explicitly authorized in writing to assess. Random-number generators may underpin device identity, cryptographic keys, authentication, and regulated products. Obtain approval for diagnostic-mode changes and any source-failure simulation. Never connect to deployed medical, transportation, industrial safety, or other safety-critical equipment. Do not tap production key-generation signals or use this design to recover random seeds, credentials, or private communications. The author, jayis1, provides a research design rather than a certified test instrument; the operator remains responsible for electrical safety, privacy, retention, and applicable law.

The target may have a stuck noise source, biased conditioning path, missing health alarm, or policy error that allows release after a failure. A compromised target may simply present an honest-looking diagnostic stream while releasing unrelated data. That attack is outside the instrument's trust boundary: four diagnostic wires cannot authenticate the origin of a sample or prove that an indication corresponds to real enforcement. Verify the relationship between these wires and the implementation being assessed using independent design review and permitted instrumentation.

The instrument itself trusts its firmware, physical harness, development board, host computer, and local operator. Host compromise can forge exported reports, and a substituted USB serial device can generate valid-looking ES1 records. CRC catches accidental corruption, not malicious forgery. There is no authenticated pairing, encrypted transport, signed evidence, secure boot, or protected update service. Physical debug access can replace the firmware. Controlled storage and inventory of the instrument, firmware hashes recorded separately by the operator, and a trusted reporting workstation are required for meaningful evidence.

There are no radios, packet transmitters, target command channels, or target-facing GPIO outputs in the supplied firmware. USART2 transmits only toward ST-LINK's virtual COM port. The companion opens the selected serial device read-only and offers no target command UI. Receive-only describes intended firmware behavior; it is not a hardware-enforced one-way barrier against malicious replacement firmware, since the MCU's pins remain physically programmable. Raw BIT and CLOCK states are not included in telemetry, but aggregate statistics still disclose some characteristics of the test stream. Do not connect secret-bearing operational streams merely because individual bits are not exported.

## Hardware specifications and assembly

The complete instrument comprises an external NUCLEO-F401RE development board, this passive wired carrier, labeled harness leads, and a Linux workstation. The MCU is STM32F401RET6, Cortex-M4, with 512 KiB flash and 96 KiB SRAM. Firmware runs from the reset-default nominal 16 MHz HSI clock without a PLL. SysTick provides nominal one-millisecond observations. The development board supplies power regulation, MCU decoupling, reset, debug/programming access, and the ST-LINK USB serial bridge; those circuits are not falsely represented as components on the passive carrier.

The carrier is an 80 by 70 mm, two-layer, 1.6 mm board. It contains fourteen through-hole components: eight resistors and six headers. There is no battery, charger, regulator, radio, antenna, isolated supply, active sensor, or external memory. It is a wired bench instrument, not a field-rated probe. The PCB uses a closed outline, routed 0.3 mm copper, a through via, and footprints from KiCad's standard libraries. It has no mounting holes; use an insulated support with external strain relief and do not leave it resting on conductive equipment.

| Reference | Quantity | Part and function |
| --- | --- | --- |
| External module | 1 | NUCLEO-F401RE, purchased separately |
| R1, R3, R5, R7 | 4 | 100 kilohm, 1 percent, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kilohm, 1 percent, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | Straight 1x2, 2.54 mm pitch header |
| J2 | 1 | Straight 1x4, 2.54 mm pitch header |
| Harness | 1 set | Short, insulated, labeled leads; ground and signal pairs |

J1 pins 1 and 2 are both GND. Neither pin is a supply input. J2 pins 1 through 4 connect respectively to NUCLEO CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0. J3 observes BIT, J4 CLOCK, J5 ALARM, and J6 RELEASE. On each target header pin 1 is signal and pin 2 is ground. All indications are active high. CLOCK is a diagnostic sample strobe, not the MCU oscillator, and RELEASE is a diagnostic indication, not an output driven by this instrument.

Only common-ground, push-pull, zero-to-3.3 V outputs are supported. Each input has a 1 kilohm series resistor and a 100 kilohm pulldown at the MCU node. The high-state load is approximately 33 microamps per channel, excluding input leakage; that estimate is not a measurement. There are no TVS devices, buffers, clamps, fuses, or galvanic isolation. Do not connect 5 V logic, negative signals, relay coils, field wiring, unknown buses, or sources with a ground offset. Series resistance does not make an overvoltage connection safe.

Power the development board through its intended ST-LINK USB connector with the board's documented power jumpers. Do not combine supplies without reviewing the actual board manual. Disconnect target signals before removing instrument power to avoid back-powering through MCU protection structures. The carrier has no supply rail and draws its resistor-network current from diagnostic outputs. Total assembly current, USB startup behavior, thermal margin, and power-off leakage remain unmeasured. There is therefore no battery-runtime claim or complete measured power budget.

## Architecture and block diagram

```text
Authorized fixture, slow 3.3 V push-pull diagnostic signals
 BIT ------1k----+---- PA0 / A0
 CLOCK ----1k----+---- PA1 / A1
 ALARM ----1k----+---- PA4 / A2
 RELEASE --1k----+---- PB0 / A3
                | each node has its own 100k pulldown
               GND -------- NUCLEO GND -------- USB host GND

External NUCLEO-F401RE
  HSI -> 1 ms SysTick -> edge/setup/continuity checks
                          | accepted bits, never raw telemetry
                          v
                 repetition + reference-proportion tests
                          | sticky failures + qualification
                          v
                   alarm/release correlation
                          | counters + bounded event queue
                          v
                   ES1 CRC encoder -> USART2 PA2 -> ST-LINK VCP
                                                      |
                        Linux parser -> Health tests / Events / Integrity
                                                      |
                                           private exclusive JSON export
```

GPIOA and GPIOB are read sequentially, not latched simultaneously. BIT and CLOCK share GPIOA, while RELEASE is read from GPIOB. Do not infer submillisecond ordering or precise alarm latency from these readings. Host receipt time includes transport and scheduler delays and is not a target edge timestamp. Use an independent logic analyzer to qualify the fixture and any timing-sensitive conclusion.

## Exact health-test contract

The diagnostic interface must provide rising CLOCK edges at intervals from 10 through 100 observed milliseconds, inclusive. High and low phases should each last at least five milliseconds. BIT must remain unchanged for at least two observed milliseconds before the rising edge; design the fixture with additional timing margin rather than operating at that boundary. There is no debouncing and no high-speed capture. Pulses entirely between samples are invisible. A 1 MHz RNG clock cannot be connected and expected to work; expose a deliberately slowed diagnostic stream without claiming it represents every operational source sample.

A high CLOCK on initial attachment is not counted as an edge. The first acquired vector establishes a baseline. A subsequent low-to-high observation is a candidate sample. A too-short BIT setup interval latches fault bit 4; a rising-edge interval shorter than 10 milliseconds latches fault bit 8. Ambiguous samples are excluded from statistical windows. These timing faults do not diagnose entropy; they diagnose an invalid observation contract.

The repetition test counts consecutive equal accepted bits. A run of 31 is below the cutoff; a run of 32 or more latches fault bit 1. max_run retains the longest observed run. The proportion test uses nonoverlapping windows of 64 accepted bits. The first bit in each window becomes its reference value, and matches includes that first bit. Reaching 49 matches latches fault bit 2 immediately, even before window completion. Completing a window records last_matches and increments windows. A count of 48 is below the cutoff. Window boundaries do not reset the repetition test.

These constants are intentionally explicit engineering policies, not approved NIST SP 800-90B parameters. They have not been selected from a source-specific min-entropy estimate or false-positive budget. Passing both checks does not demonstrate unpredictability, independence, resistance to adversarial control, or compliance. An alternating deterministic sequence passes these checks; the synthetic passing fixture deliberately uses that sequence to make this limitation obvious. This device is useful for catching gross failures and studying response policy, not for accepting a cryptographic source.

Qualification is false at boot and becomes true only after one complete window with no sticky fault. Every fault remains latched until the MCU is physically reset or reprogrammed; later passing windows cannot erase it. Source alarm assertions alone do not latch an instrument fault, but release while ALARM is asserted is unsafe. Qualification does not imply that the target itself has asserted readiness or that release is otherwise permitted.

Release is evaluated using both prior qualification and the result of the current observation. RELEASE concurrent with the first passing window's completion still increments unsafe evidence because its later ordering was not established. A later release with qualification true, no fault, and ALARM low is not counted unsafe. unsafe_episodes counts beginnings of observed unsafe intervals, and unsafe_ms counts acquired one-millisecond observations in those intervals. unannounced_ms counts observations with an instrument fault and target ALARM low. Neither counter is a calibrated physical duration or proof that the target should implement exactly the same health thresholds.

A sampling gap invalidates statistical continuity, clears the partial window and current run, increments gaps, and accumulates missed ticks. It preserves sticky faults and cumulative evidence. No guessed bits fill the missing interval. A CLOCK pause beyond 100 milliseconds similarly invalidates continuity and increments stalls once for that pause. A stable BIT remains known to be stable during a clock-only stall, so a rising edge exactly on the first overdue observation can start a new window without inventing a setup failure. After a gap or stall, a fresh complete passing window is required; existing sticky failures still prohibit qualification.

All cumulative evidence counters saturate at the unsigned 32-bit maximum. Uptime and event sequence use modular unsigned arithmetic. Ordinary uptime wrap is supported; missing an entire uptime cycle is not reconstructable. Event storage holds 32 entries and drops new entries when full, incrementing dropped and advancing the sequence. Events are supporting evidence, not a guaranteed lossless record. An unplugged BIT lead can resemble a stuck source, and an unplugged ALARM lead can resemble an unannounced failure. Verify wiring before interpreting either.

## Firmware design and recovery

health.c implements the measurement contract with no heap allocation. health.h defines limits, event layout, and persistent RAM state. wire.c provides bounded decimal encoding and CRC without printf. main.c schedules acquisition, status, periodic policy, and event transmission. drivers/board.c configures GPIO, HSI-dependent SysTick and UART parameters, reset-cause capture, and the independent watchdog. drivers/serial.c owns one copied transmission buffer and never waits for UART readiness in a blocking loop. startup.c and linker.ld provide the embedded vector table, memory initialization, flash mapping, and stack reservation.

Only the SysTick interrupt updates the clock. The main loop exclusively owns health state and the event queue. Status is scheduled every 100 milliseconds and policy every second. Events use remaining transmission opportunities. UART operates transmit-only at nominal 115200 baud, eight data bits, no parity, and one stop bit. A transmission exceeding 100 milliseconds is abandoned, tx_fail increments, and the host must resynchronize after the truncated frame. No serial command parser exists.

Watchdog startup has finite oscillator/status waits. Startup failure emits no plausible measurements. The watchdog's physical timeout depends on the independent oscillator and has not been measured. Reset loses RAM evidence, clears health latches, and starts unqualified. Boot records carry reset flags but no persistent unique session identifier. There is no flash logger, power-loss journal, firmware-update protocol, signing key, or secure recovery claim. Program through physically controlled ST-LINK/SWD using the linked image at 0x08000000, then perform a hardware reset. Starting from an arbitrarily modified debugger clock configuration is unsupported.

## Companion application and wire interface

The appropriate companion for this bench instrument is a Linux terminal application using only Python's standard library. Run `python3 app/audit.py --port /dev/ttyACM0 --export assessment.json` from the device directory after selecting the actual serial port. Normal serial-device permissions are required; the application never changes them or asks for root. Press 1 for Health tests, 2 for Events, 3 for Integrity, e to export, and q to exit. There is no network listener, cloud dependency, automatic upload, or active operation to replay on reconnection.

Health tests shows qualification, decoded fault meanings, ALARM/RELEASE, window progress, maximum run, accepted samples, windows, and correlation counters. Events shows recent sequence/time/kind/value records. Integrity shows parse errors, transport state, replay provenance, acquisition gaps, missed ticks, stalls, queue losses, and transmitter failures. LIVE means connected, compatible policy known, and a progressing status received less than one second ago. It does not mean healthy or secure. Duplicate statuses cannot renew freshness. Disconnect retains data but removes live status; reconnect requires explicitly restarting the application.

ES1 frames have the form `ES1,type,unsigned-decimal-fields*CCCC` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, uses polynomial 0x1021 and initial 0xffff, and is rendered in uppercase hexadecimal. B contains one reset-flags field. P contains the fixed policy `32,64,49,10,100,2`. Policy repeats for late attachment and never clears suspected restart. S fields, in order, are time, observed, qualified, faults, window_used, last_matches, max_run, samples, windows, failures, unsafe_episodes, unsafe_ms, unannounced_ms, gaps, missed, stalls, dropped, and tx_fail. observed bit 0 is ALARM and bit 1 is RELEASE; BIT and CLOCK are absent.

E contains sequence, time, kind, and value. Event kind 1 announces a newly latched fault mask bit, 2 a completed window's match count, 3 an unsafe episode's observed ALARM/RELEASE pair, 4 missed acquisition ticks, and 5 cumulative stalls. The app retains at most 256 events and displays the latest sixteen. It does not automatically reconcile sequence gaps; inspect sequences and dropped/tx_fail counters when assessing completeness. Events delayed behind status are not evidence of timestamp rollback by themselves.

The parser checks version, checksum syntax, CRC, field count, decimal syntax, unsigned range, and enum bounds. Its incomplete frame buffer is bounded, and a complete ES1 marker permits recovery when a timed-out frame is followed immediately by a valid one. Ordered status time regression, cumulative-counter regression, disappearing sticky fault bits, or changed data at an identical timestamp freezes the old evidence as a suspected restart. An in-session B record also freezes existing evidence. No automatic policy or boot announcement clears that freeze. Without a persistent boot identifier, some restarts remain indistinguishable from advancing traffic.

Offline replay uses `python3 app/audit.py --replay capture.txt --headless --export replay.json`. Replay is limited to sixteen MiB and never appears LIVE. Export exclusively creates a mode-0600 JSON file and refuses overwrite or a final-component symlink. Choose a trusted parent directory. Reports credit jayis1 and include source, replay provenance, connection/freshness, receipt age, policy state, restart suspicion, boot flags, parser errors, status, and retained events. Export is not encrypted or signed; follow the engagement's access and retention policy.

## Build, validation, and research workflow

Run `make -C firmware clean all test` to build the Cortex-M4 binary and execute host regressions. Run `python3 -m unittest discover -s tests -p test_app.py -v` and `python3 tests/integration.py` for parser and real terminal workflows. The integration fixture uses the actual C encoder and synthetic bit patterns; it is not a hardware capture. It exercises offline replay, late attachment, separate serial/UI pseudo-terminals, screens, private export, overwrite refusal, disconnection, and exit.

KiCad 9 with its standard footprint library and Python pcbnew bindings is required for CAD tooling. `/usr/bin/python3 tools/generate_hardware.py` regenerates the native schematic, local symbols, PCB, project, manifest, and XML netlist. `/usr/bin/python3 tools/check_hardware.py` requires zero ERC/DRC violations and compares schematic reference/pin/net, value, and footprint mappings with actual PCB pads and the saved manifest. Passive-only ERC cannot validate the external module's supply, a swapped harness, or protection absent by design.

For an authorized assessment, first verify voltage and ground with appropriate instruments. Validate the diagnostic interface against an independent logic analyzer. Establish a known alternating fixture, then test runs just below and at 32, proportion windows with 48 and 49 matches, too-fast strobes, setup violations, clock pauses, and disconnected leads. Observe target ALARM and RELEASE before and after a permitted simulated failure. Repeat after a remediation using the same firmware, thresholds, fixture, and capture method. Report what the wires showed rather than asserting the security of the entire generator.

A product-security researcher can compare an OEM's advertised health response with exposed diagnostic behavior. A penetration tester can document release continuing after a gross source failure without collecting keys. A collaborative red team can challenge the assumption that a health alarm also revokes release, while keeping stimulus generation and production systems outside this instrument. Before fabrication or deployment, review connector orientation, purchased footprints, insulation, ground paths, loading, USB power sequencing, oscillator tolerance, watchdog recovery, and ESD exposure. See VERIFICATION.md for executed results and remaining physical work.

## References and scope of evidence

ST UM1724, STM32 Nucleo-64 boards MB1136, Table 16 documents CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0 for NUCLEO-F401RE. Its USART section describes the ST-LINK PA2/PA3 virtual-COM connection; verify solder bridges on the purchased board. RM0368 documents the STM32F401 register map, and the STM32F401xD/xE datasheet defines electrical limits. Manufacturer documents describe their hardware, not certification of this carrier. NIST SP 800-90B provides broader entropy-source assessment context; this instrument neither implements that full assessment nor claims its compliance.

Manufacturer manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
