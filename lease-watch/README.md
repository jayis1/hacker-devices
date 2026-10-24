# Lease Watch — Heartbeat-to-Enable Trust Contract Auditor

Author and creator: jayis1.

Revision A engineering prototype. Firmware and application: GPL-2.0-only. Hardware generation and checking scripts: MIT. Hardware design: CERN-OHL-S-2.0. This repository contains a target-linked firmware implementation, a Linux console, and a routed carrier design. It does not contain evidence of an assembled or electrically qualified prototype. See VERIFICATION.md for the distinction between executed checks and outstanding laboratory work.

## Purpose and originality

Lease Watch investigates a small but important security assumption: an output should remain enabled only while the controller that owns it is demonstrably alive, out of reset, and ready. Many embedded products produce a heartbeat for monitoring, but continue driving an enable latch after the task responsible for safety or authorization has stopped. A heartbeat can also originate in an interrupt that survives a deadlocked application. An independent observer makes those assumptions visible rather than trusting the controller's own log.

The instrument receives four accessible, common-ground, 3.3 V logic signals: heartbeat, enable, reset-not, and ready. It timestamps qualified transitions, applies a fixed heartbeat lease, and reports enabled operation without a recent qualified rising heartbeat, enabled operation during reset, and enabled operation before readiness. It has no target-transmit driver, no radio, no target command channel, and no mechanism for injecting a fault. An operator supplies any deliberate application pause through the target's own authorized development environment.

The repository was inventoried before choosing this concept. Gray Audit reconstructs quadrature motion and identifies impossible Gray transitions. Latch Ledger investigates disconnected mechanical tamper contacts. SENT Sentinel interprets sensor frames, while the timing and fault-injection designs address other trust boundaries. Lease Watch instead checks a cross-signal temporal authorization contract. It reuses the reviewed NUCLEO register driver and carrier construction approach, not the quadrature or contact algorithms. Originality here means a distinct function within this repository, not a claim of worldwide invention or patent novelty.

## Authorized use and exclusions

Use this design only on equipment you own or have explicit written permission to inspect. Scope the exact test points, firmware revision, permitted operating states, observers, and evidence retention period before connection. Do not attach it to production access controls, operating machinery, road vehicles, medical equipment, mains circuits, or any system whose unexpected behavior could injure someone. Disconnect actuators and use a representative laboratory load before testing recovery behavior.

This is an observer, not a protective interlock. It cannot stop an unsafe output, remove power, or enforce authorization. A green-looking screen is not a safety certification. Neither the firmware nor the host application provides a safety integrity level. Hardware inspection and an independent scope remain necessary. The absence of a transmitter reduces opportunities for accidental target commands but does not make electrical attachment risk-free.

No payload extraction, credentials, network scanning, persistence, covert deployment, or interference functions are implemented. The application records signal state and timing metadata rather than application memory or serial-console contents. Those records may still reveal operational patterns; treat them as engagement evidence with restricted access.

## Threat model and interpretation

The assessed component may have an enable output that fails to expire when its heartbeat stops. A reset path may leave an external latch asserted. A compromised or defective ready signal may precede actual initialization. Lease Watch can observe these contradictions only if the chosen test points truthfully represent the relevant states. If compromised firmware generates a plausible heartbeat while bypassing authorization internally, this instrument cannot prove that the underlying application is healthy.

Trust boundaries are the target-to-carrier wiring, carrier-to-MCU GPIO, firmware-to-ST-LINK serial stream, and host parser-to-export file. Inputs are treated as electrical observations, not trusted declarations of safety. Host frames have strict size, type, integer-range, policy, and checksum validation. CRC detects accidental corruption; it does not authenticate a device or prevent forgery by an adversary who controls the workstation or serial adapter.

The host must be trusted. An attacker with debugger access can replace firmware. There is no secure boot, cryptographic attestation, encrypted transport, or protected evidence signing key. This limitation is deliberate: there is no remote control endpoint to authenticate, and adding a shared secret would not make an exposed development board trustworthy. Preserve firmware hashes and operator notes separately if the engagement requires chain of custody.

## Hardware specification

The processing module is an external NUCLEO-F401RE, using STM32F401RET6 with 512 KiB flash and 96 KiB SRAM. Revision A firmware uses the reset-default internal 16 MHz HSI clock and a 1 kHz SysTick. It uses no vendor SDK, RTOS, dynamic allocation, DMA, external oscillator configuration, or network dependency. ST-LINK's virtual COM port carries output at 115200 baud, eight data bits, no parity, and one stop bit.

The supplied PCB is an 80 by 70 mm, two-layer, 1.6 mm thick passive carrier. It is connected to the NUCLEO by short labeled jumper wires; it is not a mechanically mating shield. All carrier components are through-hole for convenient inspection and replacement. The external development board supplies its own regulated power, decoupling, debugger, and USB interface. Those components are not misleadingly represented as being on this passive PCB.

There are no batteries, radios, analog sensors, or FPGA. Power is through the NUCLEO's ST-LINK USB connector. Reserve a conservative 500 mA host USB budget for the development assembly; actual current draw and transient behavior have not been measured. The carrier has no supply rail and draws only signal loading current. J1 is ground only: both pins are GND. Never attach a supply to J1.

Each input passes through a 1 kΩ resistor into the MCU-side net, which has a 100 kΩ pulldown to common ground. At a nominal 3.3 V high, the resistive load is about 33 microamps and the node is about 3.27 V; exact resistor tolerances and GPIO leakage change those figures. This is not a precision level translator. The acceptable working interface is nominal 3.3 V push-pull CMOS from an already powered target, with no negative excursions or overshoot beyond MCU limits. The design makes no claim of 5 V, 1.8 V, open-collector, RS-232, automotive, or industrial 24 V compatibility.

The series resistor is not overvoltage protection. There is no galvanic isolation, TVS suppression, fuse, reverse-current blocking, or independent fail-safe input buffer. An energized target can back-power an unpowered MCU through protection structures. Power the NUCLEO first, confirm common-ground compatibility, connect ground before signals, remove signals before powering the NUCLEO down, and never leave target signals attached to an unpowered instrument. Use short bench leads, preferably below 20 cm, and validate loading with a scope. These operational limits do not replace a future protected-input revision.

## Connector mapping and bill of materials

| Reference | Function or value | Footprint / connection |
| --- | --- | --- |
| J1 | Both pins ground | 1x2, 2.54 mm vertical header |
| J2 | MCU input harness | 1x4, 2.54 mm vertical header |
| J3 | Heartbeat input / ground | Pin 1 HB_IN, pin 2 GND |
| J4 | Enable input / ground | Pin 1 ENABLE_IN, pin 2 GND |
| J5 | Reset-not input / ground | Pin 1 RESET_N_IN, pin 2 GND |
| J6 | Ready input / ground | Pin 1 READY_IN, pin 2 GND |
| R1, R3, R5, R7 | 100 kΩ pulldown, 1%, 0.25 W | Axial DIN0207, 7.62 mm pitch |
| R2, R4, R6, R8 | 1 kΩ series, 1%, 0.25 W | Axial DIN0207, 7.62 mm pitch |
| External module | NUCLEO-F401RE / MB1136 | Independently USB powered |

J2 pin 1 goes to Arduino A0, PA0, heartbeat. Pin 2 goes to A1, PA1, enable. Pin 3 goes to A2, PA4, reset-not. Pin 4 goes to A3, PB0, ready. Connect a NUCLEO ground to J1. Use the silkscreen reference and square pin-one pad rather than guessing orientation from a cable photograph. The numerical bit order in firmware is HB bit 0, ENABLE bit 1, RESET_N bit 2, READY bit 3.

The NUCLEO pin mapping and default ST-LINK USART routing follow ST UM1724. SB13 and SB14 normally enable the virtual COM connection; SB62 and SB63 normally remain off for that use. Confirm your board revision and solder bridges before assembly. PA2 is the firmware's USART2 transmit pin. PA3 is not configured as a receiver. GPIO register definitions are a small explicit subset of STM32F401 RM0368; another MCU with similar header labels is not automatically compatible.

## Architecture and block diagram

```text
Authorized 3V3 bench target                Passive carrier A
HB ---------> J3 --- 1k --- HB -----------> NUCLEO A0 / PA0
ENABLE -----> J4 --- 1k --- ENABLE -------> NUCLEO A1 / PA1
RESET_N ----> J5 --- 1k --- RESET_N ------> NUCLEO A2 / PA4
READY ------> J6 --- 1k --- READY --------> NUCLEO A3 / PB0
                             | each
                            100k
                             |
Common GND ------------------+-----------> NUCLEO GND

GPIO observations -> 3-sample qualifier -> heartbeat lease / alarm engine
                  -> bounded event queue + periodic status
                  -> USART2 TX -> ST-LINK USB COM -> Linux console
                                                   | overview / events / health
                                                   + private JSON snapshot
```

PA0, PA1, and PA4 are read together from one GPIO register; PB0 is read subsequently. This introduces unmeasured inter-port skew. Combined with asynchronous sampling and qualification, it prevents claims about sub-millisecond ordering. For tightly coupled reset and ready transitions, inspect a logic analyzer trace rather than interpreting a one-sample distinction as a proven vulnerability.

## Firmware behavior and decisions

The main loop samples the four inputs at most once per millisecond. Multiple loop iterations at the same tick do not advance qualification. A new level needs three consecutive observed samples. This rejects a single-sample spike; it does not guarantee rejection of every physical glitch. Pulses shorter than the sampling interval can be missed entirely. Use heartbeat high and low times of at least 10 ms, and periods comfortably below the fixed 250 ms lease, for the intended laboratory workload.

A qualified rising heartbeat renews the lease only when reset-not is high. The initial power-on level is never counted as a rising edge. Reset has precedence over a simultaneous qualified heartbeat and invalidates earlier heartbeat evidence. After reset, a genuinely new rising transition is required. ENABLE and READY are active high; RESET_N is active low. Other polarities require a reviewed firmware change, not an undocumented cable inversion.

Five alarm bits are defined: bit zero means enabled without a valid heartbeat; bit one means enabled with heartbeat age at least 250 ms; bit two means enabled during reset; bit three means enabled before readiness; bit four means sampling continuity is uncertain. Uncertainty is reported even with enable low. Each bit has a saturating entry counter. The counter measures alarm episodes, not milliseconds and not unique root causes. Multiple simultaneous bits may describe the same incident.

A skipped tick increments gap and missed-sample counters, invalidates heartbeat evidence, resets qualification, and retains the last qualified levels instead of trusting the first raw value after the gap. This prevents a single low glitch from clearing an enabled alarm. Recovery requires qualification again; heartbeat validity additionally requires observing a qualified low followed by a qualified high. Unknown time is never presented as continuous proof of health. The maximum enabled-stale duration may include an uncertain interval; it is an investigation aid rather than a certified lower bound on physical failure duration.

Heartbeat age and stale duration are accumulated with saturating arithmetic. They do not become young again after a full 32-bit timer cycle. Absolute event timestamps still wrap modulo 32 bits. Min, max, and last heartbeat periods are measured between accepted edges within valid continuity, and UINT32_MAX is the unset minimum or unavailable age sentinel. Counters saturate, while event sequence numbers intentionally wrap. Reset clears all volatile observations; there is no flash journal or power-loss persistence.

The event queue contains 32 entries and drops new events when full. The dropped counter remains in periodic status, and sequence allocation occurs even for a dropped event. UART transmission is nonblocking, uses an owned buffer, and abandons a stuck transfer after 100 ms. Transmission failures are counted. Status has priority every 100 ms, so heavy transition traffic cannot permanently starve the health view. A partial abandoned line is rejected by the host checksum parser, which resynchronizes at the next newline.

The independent watchdog starts before the monitoring loop. Its nominal timeout is roughly two seconds, but the internal low-speed oscillator is not a precision time source. If startup cannot confirm the oscillator and watchdog configuration within bounded waits, the firmware stays silent. A halted main loop should therefore stop producing fresh host status and eventually reset, subject to physical hardware validation. No assertion of target health depends on continuing to receive old event records.

## Companion application and wire protocol

The companion is a real Linux terminal application using Python's standard-library curses, termios, and select interfaces. It provides an overview screen, an event-history screen, and a health/counter screen. Press 1, 2, or 3 to switch views, e to export, and q to exit. The chosen export filename is exclusive: an existing file is not overwritten. Select a new filename when starting a new assessment.

Open the virtual COM port with `python3 app/lease.py --port /dev/ttyACM0 --export assessment.json` from this device directory. The port is opened read-only, raw, 115200 baud. No commands are sent. A disconnect leaves the last observation visible but marks it not live; restart the application to reconnect. Freshness requires an open serial connection and a valid status received within one second. Event traffic alone cannot refresh that indicator.

Frames are ASCII `LW1,type,decimal-fields*CCCC` followed by newline. CCCC is uppercase CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xFFFF, calculated over the bytes before the asterisk. B records contain reset flags, lease milliseconds, and qualification sample count. E records contain sequence, device time, event kind, and value. Event kinds are level change 1, alarm mask 2, sampling gap 3, and accepted heartbeat count 4.

S records contain time, levels, alarms, heartbeat age, heartbeat count, last period, minimum period, maximum period, gap count, missed samples, dropped events, transmit failures, maximum enabled-stale duration, and five alarm entry counters in bit order. There are exactly eighteen status numbers. The parser accepts only the documented protocol and policy, bounds decimal fields to unsigned 32-bit values, rejects oversized lines, and limits replay input to 16 MiB. Unsupported versions do not silently become compatible.

Exports contain author, source, connection state, freshness, host receipt age, session count, status, boot metadata, parse errors, and the latest 256 events. This bounded history is a snapshot, not a complete acquisition file. A boot record resets the displayed session. Without a boot record, backwards modular status time suggests a restart; queued event timestamps are deliberately excluded from this heuristic. Restart inference is not cryptographic identity and can miss resets whose timestamps happen to advance.

Replay mode uses `python3 app/lease.py --replay capture.txt --headless --export replay.json`. Replays are always labeled not live. JSON files are created with mode 0600 and never uploaded. Delete them using the workstation's approved retention procedure; application exit clears only in-memory state. The supplied synthetic fixture is generated by the actual C engine and encoder, but it is not a measurement taken from physical hardware.

## Build, inspect, and test

From this directory run `make -C firmware all test` to produce the Cortex-M4 ELF and binary and execute host tests. The build supplies startup code, a flash/RAM linker script, board definitions, and register definitions. It uses GCC with warnings as errors and no downloaded SDK. Program the resulting binary at flash address 0x08000000 using your existing authorized ST-LINK workflow, then perform a hardware reset into the documented reset-clock state. Debugger scripts that change clocks before entry violate that contract.

Run `python3 -m unittest discover -s tests -p test_app.py` and `python3 tests/integration.py` for parser, export, replay, pseudo-terminal serial, and real curses-screen checks. The integration test creates separate UI and serial PTYs, feeds production C packets, exercises navigation and export refusal, disconnects the stream, and exits the application. PTYs validate host plumbing, not electrical acquisition accuracy.

Use `/usr/bin/python3 tools/generate_hardware.py` to regenerate CAD with KiCad's Python bindings, then `/usr/bin/python3 tools/check_hardware.py`. The checker runs ERC and DRC with violations treated as failure, exports a fresh schematic XML netlist, and compares every reference, value, footprint, pin, and net with PCB pads and the connectivity manifest. Passive symbols mean ERC cannot validate the external MCU's power or input voltage. Inspect the schematic and PCB visually and verify purchased component dimensions before ordering.

## Practical assessment workflows

For a red-team laboratory exercise, connect to the authorized development target and observe its nominal heartbeat before pausing the application task through its own debugger. If enable remains high after expiration, export the finding and record where the heartbeat originates. Do not describe a task pause as exploitation or claim that the observer caused the failure. Repeat with the final software release and demonstrate remediation by the target independently deasserting enable.

For product security research, compare startup and reset ordering across firmware revisions. Observe whether readiness is meaningful, whether reset invalidates the prior lease, and whether an external output latch remains asserted. For penetration-test evidence, pair the exported snapshot with scope captures, connection photographs, firmware version, authorization scope, and relevant target logs. This yields a reproducible observation rather than a screen labeled safe or unsafe without context.

Before any physical deployment, validate the input loading, threshold margins, actual sample interval, oscillator error, watchdog recovery, cable disconnect behavior, power sequencing, and USB failure behavior on a sacrificial bench target. Fabrication, assembly, programming, electrical bring-up, ESD, EMC, and regulatory testing remain outstanding. The software tests and CAD checks are useful engineering gates; none substitute for those measurements.
