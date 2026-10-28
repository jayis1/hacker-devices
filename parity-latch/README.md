# Parity Latch — Coded Diagnostic-Transfer Integrity Bench

Author and creator: jayis1. Revision A. Protocol PL1. Firmware and application: GPL-2.0-only. Hardware tooling: MIT. Passive carrier: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design with target-linked firmware, executable desktop software, and native routed KiCad files. Physical fabrication, MCU programming, calibrated timing, electrical qualification, and deployment are not claimed. See VERIFICATION.md for executed checks and outstanding laboratory work.

## Purpose and practical value

Parity Latch observes a deliberately slow, parallel diagnostic interface carrying two data bits, one odd-parity bit, and a strobe. It asks whether a target presents an internally consistent codeword with enough observed setup and hold time around each qualified strobe. The primary application is validation of security-sensitive state transfer between components: a maintenance-mode selector, a boot-policy adapter, or a laboratory authorization mailbox can expose its selected state through this compact interface. The observer never changes that state and never supplies a strobe.

A parity check alone is not enough. A receiver might sample a valid codeword before its data settles, or the sender might alter the word before the receiving component has finished latching it. Parity Latch combines three independently reported conditions: odd-parity validity at the sampled strobe, a prospective stable setup interval, and an observed hold interval after that strobe. It keeps transfers pending until the hold obligation completes. A transfer with correct parity but a setup or hold failure is not added to the good-codeword histogram.

The device collects no credential, payload stream, key, executable image, network address, or personal identifier. Two data bits identify four operator-defined laboratory states, not a protocol for exporting arbitrary secret content. An operator can use the counters and codeword distribution to compare original and corrected firmware on an owned fixture. The useful result is a reproducible integration defect with an explicit sampling contract, not an assertion that a system was compromised.

## Repository-relative originality

The repository directories and README purposes were inventoried before selecting this concept. The nearest measurement contracts were read in detail. Dualrail Witness treats two complementary rails as one permission and checks enable/fault behavior; it does not observe a parity-coded multibit transfer. Epoch Witness enforces generation monotonicity and commit/execution order; it has no parity bit and is deliberately insensitive to valid decreases only in a different, ordered-generation sense. Handoff Audit measures command/feedback disagreement and break-before-make timing, not a codeword captured at a strobe. Clock Ratio measures counts between enabled clock anchors. None implements this odd-parity codeword histogram with pending post-strobe hold qualification and separately conserved abandoned transfers.

This is repository-local originality, not a global prior-art or patentability claim. Parity checking and setup/hold measurement are established engineering techniques. The new contribution is their bounded, auditable instrument contract for slow security-state adapters. The passive resistor carrier, STM32 startup and UART driver, framing primitives, and local desktop transport deliberately reuse jayis1's existing platform patterns. A new device does not require pretending that a connector or a CRC algorithm has just been invented.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written authorization to assess. Obtain permission for the exact diagnostic test points, any target firmware instrumentation, ground connection, approved workload, possible interruption, and retained evidence. Restrict initial work to isolated development fixtures with hazardous actuators and production dependencies disconnected. Do not connect this prototype to occupied access controls, vehicles, medical systems, mains circuitry, industrial protection equipment, unknown field wiring, or independent hazardous voltage domains.

The assessed attack surface is the integrity of a state-transfer boundary. Relevant defects include a stuck parity wire, inconsistent data/parity update ordering, premature strobing, raw data movement during the hold interval, and a second transfer launched before the first transfer's observation window ends. The supplied firmware does not inject any of these faults. Approved target test firmware or an owner-controlled laboratory generator must create the experimental conditions. There is no exploitation engine, stealth mode, credential collection, persistence mechanism, wireless transmitter, or target control interface.

The observer trusts the operator, physical harness, installed observer image, development board, and workstation. Target indications may be dishonest: correctly encoded GPIO values do not prove that an internal access path has the same value. A disconnected wire looks low through its pulldown and cannot be distinguished from an intentional low. Odd parity detects an odd number of flipped bits within the three-bit word; even-numbered corruption can remain valid. It provides no authentication, confidentiality, freshness, replay resistance, or guarantee of independent fault detection.

The serial CRC is likewise only an accidental-corruption check. A malicious USB device or edited capture can produce valid records. There is no signed bootloader, secure updater, device certificate, authenticated pairing, persistent boot identity, encrypted log, or cryptographic evidence chain. Program through physically controlled ST-LINK and retain image provenance through the laboratory's separate process. Do not relabel these reports as attestations.

All target-facing pins remain inputs in the supplied firmware; the USART receiver is disabled. Only PA2 transmits, toward ST-LINK and the workstation. This is not a physical data diode: replacement firmware could change GPIO direction. Ground loops, loading, wrong voltage, power sequencing, and wiring errors can still disturb the target. Stop immediately if attaching the observer changes target behavior. The author, jayis1, does not authorize misuse or promise fitness for a safety function.

## Hardware specifications and bill of materials

The complete instrument consists of an external ST NUCLEO-F401RE, the passive carrier defined here, a short labelled harness, an insulating support, and a Linux workstation. The STM32F401RE Cortex-M4 supplies 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, nominal one-millisecond SysTick, and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. Odd parity in the observed diagnostic word is separate from the UART framing, which is ordinary 8N1.

There is no FPGA, radio, analog sensor, battery, charger, SD card, target power output, or active level shifter. The external Nucleo supplies regulation, MCU decoupling, reset circuitry, SWD programming, and ST-LINK USB serial conversion. Those facilities are not represented as fictional parts on the passive carrier schematic. The carrier is connected by individual harness leads, not mechanically stacked as a shield.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External processor module | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical through-hole header, 2.54 mm pitch |
| J2 | 1 | 1x4 vertical through-hole header, 2.54 mm pitch |
| Harness and support | 1 set | Short insulated leads, labels, strain relief, insulating base |

The board is 80 by 70 mm, nominally 1.6 mm thick, with two copper layers, 0.3 mm tracks, a routed ground transition, and a closed outline. Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Standard KiCad vertical pin-header footprints provide plated holes and visible pin-one identification. Fourteen components and nine electrical nets are represented in both schematic and PCB. There are no mounting holes or enclosure files. Verify purchased part dimensions, drill fit, mating access, and support arrangements before fabrication; supplier availability has not been checked.

## Pin mapping, voltage, and power

| Signal | Target pin | MCU harness | Nucleo label / MCU | Wire bit |
| --- | --- | --- | --- | --- |
| D0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| D1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| PARITY | J5.1 | J2.3 | A2 / PA4 | 2 |
| STROBE | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only; neither pin provides power. J2 contains signals only. CAD names ending `_IN` identify the target side of the series resistor. Plain names identify the MCU-side node, where the 100 kOhm pulldown connects. The carrier does not need a supply rail. Do not mistake the ground header for a power connector or infer that J2 can be attached to any arbitrary four-position header.

Use only common-ground, nominal zero-to-3.3 V push-pull diagnostic outputs. Each channel has a nominal 101 kOhm resistive path to ground, before MCU leakage and source impedance are considered. The series resistor is not overvoltage protection. There is no isolation barrier, qualified ESD network, fuse, reverse-current blocker, or level translation. Five-volt logic, negative serial voltages, differential buses, relay coils, 1.8 V domains, and field signals are outside the supported contract.

Power the Nucleo using its documented ST-LINK USB arrangement. Reserve a conservative 5 V, 500 mA source budget while respecting the actual module's jumper configuration and USB limits; this is not a measured consumption figure. Establish compatible ground and power the observer before enabling target signals. Remove target signals before removing observer power to avoid back-powering through input protection structures. Ground-offset, loading, leakage, threshold margin, cable capacitance, and startup behavior need physical measurement.

The mapping follows the NUCLEO-F401RE Arduino analog connector arrangement documented in ST UM1724. Verify the exact board revision and USART solder bridges against the manual before wiring. The normal ST-LINK virtual-COM arrangement uses SB13/SB14 enabled and SB62/SB63 disabled. The register definitions follow STM32F401 RM0368. A different Nucleo model or MCU variant is not a drop-in substitute.

## Architecture and block diagram

```text
Authorized slow diagnostic adapter       Passive carrier A
D0      --> J3.1 --- R2 1k ---+--- J2.1 ---> PA0 / A0
D1      --> J4.1 --- R4 1k ---+--- J2.2 ---> PA1 / A1
PARITY  --> J5.1 --- R6 1k ---+--- J2.3 ---> PA4 / A2
STROBE  --> J6.1 --- R8 1k ---+--- J2.4 ---> PB0 / A3
                            | each MCU-side node
                           100k
                            |
Common ground --------------+--- J1 ------> Nucleo / USB ground

GPIO reads -> independent qualifiers -> raw-aware setup/hold engine
                                   -> pending / completed / abandoned ledger
                                   -> accepted codeword histogram
                                   -> bounded CRC snapshots
                                   -> USART2 TX -> ST-LINK USB
                                   -> Transfers / History / Integrity screens
                                   -> operator-selected private JSON export
```

GPIOA is read before GPIOB. STROBE is therefore not hardware-latched at precisely the same instant as the data and parity pins. Software sampling, inter-port skew, oscillator error, and interrupt latency have not been calibrated. This is not a processor-bus analyzer or nanosecond setup/hold instrument. Use generously paced laboratory vectors, initially with at least twenty milliseconds of setup, high time, and low time, and verify actual timing with a calibrated analyzer.

## Exact firmware measurement contract

The implementation is C11 with explicit startup vectors, a linker memory map, register definitions, GPIO/watchdog support, bounded UART transport, a transfer engine, and checked serialization. There is no heap allocation, RTOS, flash writing, persistent journal, filesystem, network stack, or firmware command parser. The image starts at 0x08000000 and reserves at least 4 KiB stack headroom. All accumulated evidence is volatile and disappears when the observer resets.

Each input qualifies independently after a candidate remains unchanged for three subsequent one-millisecond observations. A changed input therefore needs its first observation plus three matching intervals. Calls repeated within one tick are ignored. Chatter on one input does not delay another's qualification. Measurement begins only when all four inputs have qualified; startup with strobe high does not invent a rising edge. A qualified low strobe is needed before a later rising edge can start a transfer.

The three low bits form the codeword. Odd parity means their XOR is one. Payload values zero, one, two, and three therefore require parity bits one, zero, zero, and one respectively; valid three-bit words are four, one, two, and seven. The captured field retains the most recently started word even after its transfer completes. It is historical when active is zero, not a claim that the current pins still carry that word.

Setup is a capped five-millisecond run of observed intervals whose raw data/parity bits equal the already-qualified word. A raw excursion resets setup even when too brief to qualify. Setup includes the interval ending at the qualified strobe observation; it is not referenced to the physical strobe edge. Because the strobe itself is qualified, the physical setup can be shorter than this software measure. Do not report this as a silicon timing specification. A word changing in the same qualified observation as the strobe fails setup.

A qualified rising strobe starts one pending transfer, captures the word, and checks parity and setup. It remains pending for ten subsequent observed milliseconds. During this interval both raw and qualified data/parity must equal the captured word. A one-sample raw excursion records one hold failure even if debounce rejects it. A change on the exact tenth endpoint is conservatively a hold failure. Strobe falling is permitted during the hold window; the obligation is to preserve the word, not to preserve strobe high.

At the tenth endpoint, a noncolliding pending transfer completes. If any parity, setup, or hold condition failed, bad increases. Otherwise good increases and exactly one payload histogram bin increases. Failure counters are once per transfer per condition and can overlap: adding them does not yield the bad total. Failures may also belong to a transfer subsequently abandoned by a gap or collision. Good means only that the sampled contract passed; it is not proof of authentic authorization or correct behavior inside the target receiver.

A second qualified rising strobe while a transfer is pending is a collision. Both the pending transfer and the new strobe are counted as abandoned; neither contributes to completed or the histogram. This conservative rule also wins if the second strobe occurs at the exact hold-completion endpoint. A fresh qualified low is then required. Holding strobe high cannot produce repeated transfers. The engine always preserves starts = completed + aborted + active and completed = good + bad; the four bins sum to good.

A sample jump other than one millisecond is an acquisition gap. It increments gap and missed-tick diagnostics, abandons a pending transfer, clears qualification and setup continuity, and requires reacquisition. Previously recorded counts and sticky findings remain. Unknown time never earns setup or hold credit. An already-high post-gap strobe is not an observed edge. Gap counts describe observed discontinuities, not necessarily distinct physical failures.

Acquisition updates are transactional. If any counter would overflow, the incomplete update is discarded, the prior pending transfer is abandoned if necessary, and the engine enters terminal HALTED with an exhaustion finding. This prevents half-counted transfers. Time continues modulo 32 bits; transport-failure counts saturate. HALTED acquisition evidence does not resume automatically. A failed watchdog initialization also halts before the first status. A reset is required to start a new measurement session, and that reset clears volatile evidence.

## Runtime, protocol, and companion application

The main loop samples before servicing UART. Serial submission copies one bounded frame; each poll sends at most one byte when TX is ready. A blocked transmission is abandoned after one hundred milliseconds and counted. The scheduler sends one boot record, policy initially and roughly every second, and cumulative status roughly every hundred milliseconds. There is no per-edge event stream or complete capture log. The independent watchdog is fed by the main loop; its physical recovery and LSI tolerance remain bench tests.

PL1 records are ASCII `PL1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial 0xffff, no reflection or final XOR, and covers all bytes before the asterisk. The checksum is four uppercase hexadecimal digits. B carries one reset-flags integer. P carries `5,10,3`, representing setup, hold, and qualification intervals. Periodic policy permits late attachment without pretending that the observer rebooted.

S contains these ordered fields: `now, levels, valid, active, halted, faults, captured, age, starts, completed, good, bad, aborted, parity, setup, hold, collisions, gaps, missed, bin0, bin1, bin2, bin3, tx_fail`. Numeric values are bounded to unsigned 32-bit range; age is zero when inactive and below ten when active. Fault bits zero through six mean parity, setup, hold, collision, acquisition gap, exhaustion, and watchdog startup. Levels and valid are four-bit masks using the pin table.

Run `python3 app/console.py` from this directory. The local desktop uses Python Tk, termios, poll, and JSON with no third-party package installation. Enter the exact serial path, normally `/dev/ttyACM0`, and select New live. Transfers displays every measurement and named finding; History retains up to 256 snapshots; Integrity displays provenance, compatibility, parser errors, reset flags, freshness, and any freeze reason. No screen can send target commands. Disconnect preserves evidence, while a successful new session intentionally replaces it; export first.

Replay accepts only regular files up to two MiB, opened nonblocking and checked before allocation. FIFOs and device files are rejected. Replay is always labelled replay, never live. Framing is bounded and resynchronizes on the complete PL1 marker after a truncated record. A partial EOF tail counts as a parser error; an empty recording is rejected. Fixtures generated by the C encoder are synthetic software input, not measurements from fabricated hardware.

LIVE requires a connected compatible session and advancing accepted device time within one second. Duplicate timestamps do not renew freshness. Contradictory duplicates, counter rollback, disappearing sticky findings, and mutations of halted acquisition evidence freeze the retained session. A boot received after evidence also freezes it. Policy never clears a freeze. Long reception pauses and cumulative host/device progress differences trigger conservative backlog protection. These checks cannot establish authenticated session continuity: a lost boot and forward-moving counters can make some resets indistinguishable.

JSON export uses exclusive creation, mode 0600, and refusal of existing destinations or final-path symlinks. It includes author jayis1, source, live/replay provenance, connection, compatibility, error count, freeze reason, reset flags, receipt age, latest status, and bounded history. Even an empty export retains session metadata. There is no cloud upload, hidden database, automatic recovery file, encryption, or signing. Select a trusted destination and follow the engagement's retention policy; deleting a file is not guaranteed secure erasure.

## Build and verification

Required tools are GNU Make, ARM GNU GCC with newlib/nosys support, native GCC, Python 3 with Tk, KiCad 9 CLI and Python bindings, installed standard KiCad footprints, and Xvfb for automated desktop tests. From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
```

The target build produces `firmware/build/parity-latch.elf` and `.bin`. Use a supervised ST-LINK programming workflow and hardware reset into the documented reset clock state. No programming trial is claimed. Native tests exercise the actual engine and encoder under sanitizers, and the UART tests use modeled memory-mapped registers rather than real USART silicon. The desktop test feeds production-C frames through Linux pseudo-terminals and operates a real Tk event loop, including the three screens and hangup behavior.

The CAD checker runs ERC and DRC with violations treated as errors, exports a fresh schematic netlist, and compares reference/pin/net mappings, values, and footprint identities against actual PCB pads and the connectivity manifest. Symbols are embedded and supplied in Witness.kicad_sym. The PCB has real routed tracks and a closed outline. Passive-symbol ERC cannot validate an external power source, target voltage, harness polarity, isolation, or electrical safety. Rendering and footprint review remain necessary even when automated checks pass.

## Laboratory acceptance and research workflows

Before fabrication, review the rendered schematic and board with the actual selected components. Assemble on an insulating support, check continuity and shorts unpowered, measure resistor values, and bring up the Nucleo without a target. Verify that target-facing pins remain inputs during reset and execution. Check loading, threshold margin, power sequencing, back-power, ground offset, and cable effects. Then correlate paced diagnostic vectors against an independent analyzer, including short rejected pulses and a raw excursion within the hold interval.

A product-security engineer can instrument a maintenance-state mailbox, demonstrate correct parity and stable transfer timing, then deliberately exercise an approved faulty sender build. An authorized red team can document a state-transfer integration weakness without collecting secrets or obtaining unauthorized access. A penetration tester can preserve the fixture mapping, firmware version, counters, and corroborating scope traces in a report. A regression engineer can compare the same four-state workload before and after remediation. In each case, distinguish diagnostic evidence from proof that a receiver actually acted on a corrupt word.

Physical watchdog recovery, USB behavior with a real board, timing calibration, environmental limits, ESD, EMC, enclosure suitability, and regulatory compliance remain unverified. This instrument is not an interlock, certified fault detector, isolation tester, or permission controller. A clean run means no violation was reported under the sampled contract during that run, not that all races or fault patterns are absent.

Manufacturer references: ST UM1724, Nucleo-64 MB1136 user manual, and ST RM0368, STM32F401 peripheral reference manual. Review editions matching the actual board and silicon:

- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

Original design, firmware, application, and documentation author: jayis1.
