# Context Stack — Nested Security-Context Return Witness

Author and creator: jayis1. Revision A. Protocol CS1. Firmware and application: GPL-2.0-only. Hardware utilities: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design. The repository includes target-linked STM32 firmware, a working Linux desktop companion, and native routed KiCad files. Software execution and CAD checks are recorded in VERIFICATION.md. Fabrication, programming, electrical characterization, calibrated timing, and physical deployment have not been performed. This instrument observes diagnostic indications; it does not enforce control-flow integrity or establish which instructions actually executed.

## Purpose and practical value

Context Stack watches nested entry and return indications from a security-sensitive component. A laboratory adapter supplies a two-bit context tag and separate PUSH and POP strobes. Each entry places its tag on a bounded stack. Each return must name the most recently entered, still outstanding context. The observer reports mismatched returns, returns without entries, excessive nesting, overlapping operations, unstable tags, and sampling discontinuities. It retains cumulative evidence independently of the target's own event logger.

The motivating defect is a security-context restoration bug. Consider a trusted service that temporarily enters a maintenance scope, then enters a narrower diagnostic scope, and finally unwinds those scopes. Returning the outer scope before the inner scope can leave permissions, resource ownership, or cleanup state inconsistent. Ordinary logs may contain every entry and return yet fail to show that their nesting is wrong. This device exposes a small, explicit last-in-first-out contract that can be checked without collecting credentials, executable addresses, memory contents, or user payloads.

The device is useful when a product team can instrument a slow laboratory build. It is not a passive tap on an arbitrary processor instruction stream. Two tag bits represent four operator-defined classes; they do not encode return addresses or unique invocation identifiers. Repeated tags are permitted because reentrant calls can legitimately enter the same class twice. That choice also means exchanging two equal-tag invocations is invisible. Reports must preserve the tag mapping and this ambiguity rather than claiming complete call-stack reconstruction.

## Repository-relative originality

The existing repository directories and README purpose statements were inventoried before implementation. The closest contracts were examined: Receipt Witness correlates asynchronous audit acknowledgments in FIFO order; Epoch Witness checks monotonic generation announcements and execution ordering; Parity Latch checks codeword parity and transfer setup/hold. None implements a bounded LIFO context stack whose return tag must match the outstanding top and whose correlation is permanently invalidated after an ambiguous operation.

Originality is relative to the inspected repository, not a worldwide invention or patentability claim. Stack checking is an established technique. The contribution here is a practical receive-only diagnostic instrument, its explicitly limited tag model, conserved entry ledger, and desktop evidence interface. The passive carrier, startup, register definitions, UART driver, bounded framing, and desktop transport reuse jayis1's existing platform. Reuse is intentional and does not change attribution.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Written scope should identify the target, diagnostic adapter, attachment points, approved workloads, permitted downtime, and evidence-retention policy. Initial trials belong on an isolated development fixture with hazardous actuators and production dependencies disconnected. Do not attach this prototype to occupied access-control equipment, vehicles, medical devices, industrial protection systems, mains-related circuitry, or unknown field wiring.

Relevant defects include out-of-order scope cleanup, duplicated return notifications, missing entry notifications, excessive recursion, incorrectly reused diagnostic strobes, and changing a tag too close to an operation indication. An authorized red-team exercise can ask the owner to reproduce a suspected restoration flaw through documented local test paths. The observer does not create the workload, exploit the flaw, modify permissions, inject signals, or prevent a defective return. A penetration-testing finding requires independent confirmation of the functional consequence.

The target adapter must accurately represent its internal transitions. A malicious target can lie on all four wires, and a disconnected wire can appear low through its pulldown. The workstation, harness, MCU image, programming interface, and operator are trusted. Neither the CRC nor file permissions authenticate evidence. There is no signed bootloader, secure updater, persistent boot identity, trusted wall clock, cryptographic attestation, encrypted storage, radio, network listener, or cloud upload.

Receive-only operation is a firmware property, not a certified physical data diode. Replacement MCU firmware could drive these GPIOs. Even correct firmware can disturb a target through loading, ground loops, back-power, or wiring mistakes. Stop if attachment changes target behavior. The design is not an interlock or safety instrument, and the author, jayis1, does not authorize misuse or guarantee security, electrical safety, or regulatory compliance.

## Hardware specification

The assembly consists of an external ST NUCLEO-F401RE development board, the passive carrier in kicad/, short labelled harness leads, an insulating support, and a Linux workstation. The STM32F401RE Cortex-M4 supplies 512 KiB flash and 96 KiB SRAM. The firmware uses the reset-default nominal 16 MHz HSI clock and one-millisecond SysTick. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit, through the development board's ST-LINK virtual serial interface.

The external Nucleo supplies MCU regulation, decoupling, reset, SWD programming, and USB conversion. These are not fictional parts omitted from an otherwise self-contained board. The carrier contains no MCU, radio, FPGA, battery, charger, active sensor, SD card, level translator, or isolated supply. It is a cable-connected carrier, not a mechanically stackable shield.

The PCB is 80 by 70 mm with two copper layers, nominal 1.6 mm thickness, 0.3 mm routed tracks, one ground-layer transition, and a closed outline. Fourteen components, thirty electrical pads, and nine nets are represented in both schematic and PCB. Standard KiCad through-hole resistor and pin-header footprints are used. There are no mounting holes or enclosure files; provide nonconductive support and strain relief before use.

| References | Quantity | Component |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm through-hole header |
| J2 | 1 | 1x4 vertical 2.54 mm through-hole header |
| Harness/support | 1 set | Insulated leads, labels, strain relief, insulating base |

Resistor footprints use 7.62 mm lead spacing. Check purchased body dimensions, connector mating access, drill fit, and support geometry before fabrication. Vendor stock, price, and component availability have not been verified.

## Wiring, power, and electrical limits

| Function | Target header | MCU-side header | Nucleo label / GPIO | Bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| PUSH | J5.1 | J2.3 | A2 / PA4 | 2 |
| POP | J6.1 | J2.4 | A3 / PB0 | 3 |
| Common ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only. Neither pin supplies power. J2 contains four signals, not power or ground. Names ending `_IN` identify target-side nodes; unsuffixed signal names identify MCU-side nodes after the series resistor. Confirm pin numbering with continuity measurements before connecting a target. A header resembling another connector is not evidence of compatible wiring.

Only common-ground nominal zero-to-3.3 V push-pull diagnostic outputs are supported. Each channel presents a nominal 101 kOhm path to ground before MCU leakage and source impedance. The series resistor is not qualified overvoltage or ESD protection. There is no galvanic isolation, dedicated clamp, reverse-current blocker, fuse, or hot-plug guarantee. Five-volt signals, negative voltages, 1.8 V logic, differential buses, relay coils, and unknown field signals are outside the contract.

Power the Nucleo through its documented ST-LINK USB arrangement. Reserve a conservative 5 V, 500 mA source allowance while respecting actual jumper settings and USB limits; this is not measured current. USB ground reaches target ground through the Nucleo and J1. Establish compatible ground and power the observer before enabling target signals. Remove target signals before removing observer power to avoid back-power through MCU protection structures. Ground offsets, loading, rise times, power sequencing, leakage, and thermal behavior require bench measurement.

## Architecture

```text
Owned target diagnostic adapter       Passive carrier             NUCLEO
TAG0 ------------------------------- 1k --+--------------------- PA0
TAG1 ------------------------------- 1k --+--------------------- PA1
PUSH ------------------------------- 1k --+--------------------- PA4
POP -------------------------------- 1k --+--------------------- PB0
                                          | each node
                                         100k
                                          |
Common ground --------------------------- GND ----------------- GND / USB

GPIO sample -> independent qualifiers -> setup check -> bounded LIFO engine
                                                    -> conserved counters
                                                    -> CS1 CRC snapshots
                                                    -> USART2 PA2 TX
                                                    -> ST-LINK USB
                                                    -> Nesting / History / Integrity
```

GPIOA is read before GPIOB. These are not atomic hardware-latched samples across both ports. The design cannot resolve processor-speed ordering or capture arbitrarily short pulses. Use a slow diagnostic adapter and independently verify its waveform with a calibrated analyzer. Initial experiments should use at least twenty milliseconds of tag setup and twenty milliseconds each of strobe high and low time, not boundary-hugging waveforms.

## Exact firmware contract

All inputs qualify independently. A changed candidate must remain unchanged for three subsequent one-millisecond observations before becoming qualified. Thus a transition needs its initial observation plus three matching intervals. Chatter on one input cannot indefinitely defer another. Pulses shorter than the qualification interval may be rejected or entirely missed; absence of a count is not proof that no physical transition occurred.

Startup assumes an empty target context stack only by operator agreement. Reset the target to a documented empty baseline and restart the observer before the campaign. All four inputs must qualify, then both raw and qualified PUSH and POP must remain low for twenty evaluated ticks. Tags need not be zero. This quiet baseline enables READY but does not prove that no earlier target context exists. Attaching midway through execution is unsupported even if the wires happen to be quiet.

Once ready, each qualified PUSH rising edge increments pushes and saves the qualified two-bit tag. Holding PUSH high does not generate repeated entries. Each qualified POP rising edge increments pops and compares the tag against the most recent outstanding entry. A match removes exactly one entry and increments matched. Matching an outer entry while an inner entry remains produces a mismatch rather than searching down the stack. Equal-tag nesting is legal and indistinguishable by identity.

Depth is limited to eight. A ninth push is counted as an attempted entry, records overflow, and abandons all outstanding entries including that attempt. A pop on an empty stack records underflow. Simultaneous rising strobes, or a rising strobe while the opposite qualified strobe remains high, record collision. Both attempted operations are counted when both rise together. Collision takes precedence over setup and stack checks because the ordering itself is unknown.

Before an operation, the qualified tag must have accumulated five observed stable intervals, with raw tag equal to qualified tag. A raw excursion resets setup even when it is too short to qualify. A qualified tag change resets setup. This is setup relative to the qualified operation edge, not the physical edge; qualification delay can make the physical margin shorter. There is no post-edge hold obligation. Tags may change after an accepted operation if the next operation earns fresh setup. This observer is not a silicon timing analyzer.

Any operation fault permanently invalidates correlation for that observer run. Outstanding entries become abandoned and depth becomes zero. Quiet inputs never restore READY after a fault: silence does not establish that the target unwound an unknown stack. Reestablish the actual target baseline and deliberately restart the observer for another run. Export evidence first. Subsequent input levels and sampling diagnostics remain observable, but further operation edges are not counted as if their stack were known.

A tick jump other than one millisecond records an acquisition gap and missing ticks, abandons outstanding entries, and permanently invalidates correlation. Repeated calls in the same tick are ignored. Normal unsigned timer wrap is supported. The firmware does not count unobserved time as safe or fabricate missed operations. Counters use checked arithmetic. Each sample applies to a temporary engine copy; exhaustion discards the entire speculative update, abandons the previously committed pending stack, and enters a terminal halted state. The overflow-causing sample is not included in the retained totals.

The principal invariant is pushes = matched + abandoned + depth. Pops is not equal to matched because invalid or ambiguous returns are counted separately. Peak is the largest accepted depth, never nine for an overflow attempt. Top is zero when depth is zero and otherwise retains the outstanding top tag. Fault bits remain sticky. All evidence is volatile RAM and disappears on reset; this is not a persistent journal.

## Firmware transport and application

The allocation-free C implementation separates startup, board registers, acquisition, policy, serialization, and UART service. The main loop does not wait for transmission. The UART copies a bounded frame, sends one available byte per poll, and abandons a transmission stalled for one hundred milliseconds. tx_fail counts failed submissions and timeouts, saturating independently. The watchdog uses bounded initialization waits and main-loop feeding. Watchdog initialization failure halts acquisition before the first status.

CS1 frames are ASCII `CS1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial 0xffff, without reflection or final XOR. Four uppercase hexadecimal digits cover bytes before the asterisk. B contains reset flags. P contains `8,5,3,20`: capacity, setup, qualification, baseline. Policy repeats approximately every second so late serial attachment can establish compatibility without pretending a reboot occurred. Status repeats approximately every hundred milliseconds.

S fields, in order, are `now levels valid ready halted faults depth top pushes pops matched abandoned mismatch underflow overflow collision unstable gaps missed peak tx_fail`. All values are unsigned 32-bit. Levels and valid are four-bit masks in the wiring-table order. Fault bits zero through seven are mismatch, underflow, overflow, collision, unstable setup, acquisition gap, counter exhaustion, and watchdog startup. The encoder refuses internally inconsistent states. CRC is corruption detection only, not authentication or a signature.

Launch `python3 app/console.py` from this directory. The standard-library Python/Tk app has Nesting, History, and Integrity screens. Enter the exact serial path, commonly `/dev/ttyACM0`, and select New live. Serial is opened read-only; no target command or firmware setting is sent. Disconnect retains evidence. A successful new session replaces the model, while a failed connection attempt preserves the old one. There is no automatic port probing or hidden network discovery.

History retains at most 256 snapshots, not every GPIO edge. Integrity reports source, provenance, compatibility, parser errors, reset flags, freshness, and freeze reasons. LIVE requires a connected compatible session with advancing accepted device time. Duplicate snapshots do not renew freshness. Counter rollback, disappearing sticky faults, contradictory duplicate timestamps, unexpected reboot records, and changing halted evidence freeze the model. Long reception pauses and cumulative host/device clock divergence conservatively detect buffered stale data. These heuristics cannot prove session identity without a persistent boot identifier.

Replay accepts only regular files no larger than two MiB and rejects FIFOs and device files without blocking. Framing resynchronizes on a complete CS1 marker; a partial EOF tail records an error and an empty recording is rejected. JSON export creates a new mode-0600 file and refuses an existing destination or final-path symlink. Exports include author jayis1, provenance, integrity qualifications, latest status, and bounded history, including metadata for empty sessions. Files are neither signed nor encrypted. Choose a trusted destination and follow the engagement's retention policy.

## Build and validation

Required tools are GNU Make, Arm GNU GCC with newlib/nosys support, native GCC, Python with Tk, Xvfb for headless GUI testing, and KiCad 9 CLI/Python bindings with standard footprints. No mobile SDK or downloaded Python packages are needed. From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
```

The target build emits `firmware/build/context-stack.elf`, map, and binary for Cortex-M4 Thumb soft-float with flash at 0x08000000 and 96 KiB RAM. The linker reserves at least 4 KiB stack headroom. Program only through a supervised ST-LINK procedure appropriate to the actual board. Programming and silicon execution are not claimed here.

Tests exercise nesting, wrong returns, underflow, overflow, simultaneous operations, lost samples, setup rejection, reentrancy, wrap, counter exhaustion, randomized invariants, UART bounds, and timeout recovery. The synthetic fixture uses the production C encoder; `firmware/build/fixture > synthetic.cs1` creates a replay file, not a physical measurement. App tests exercise real Tk event loops with Linux pseudo-terminals, replay, all screens, hangup, private export, overwrite refusal, malformed frames, and impossible checksum-valid snapshots.

The CAD checker runs ERC and DRC with violations treated as errors and exports a fresh schematic netlist. It compares every reference/pin/net, component value, and full footprint identifier against PCB pads and the manifest. Symbols are embedded and supplied in Witness.kicad_sym. Zero passive-network rule violations cannot validate an external supply, harness polarity, voltage compatibility, or physical safety. Review the rendered design and actual purchased parts before ordering.

## Research workflow and remaining physical work

A product-security engineer can map four permission scopes to tags and run a documented nested workload, comparing captures before and after a cleanup fix. An authorized red team can preserve an out-of-order diagnostic return as a reproducible integration finding without collecting secrets. A penetration tester can attach the tag mapping, observer image identity, exported counters, and corroborating target logs to a scoped report. A regression engineer can replay synthetic or approved recorded sessions through the same parser used for live observations.

Before any valuable target is attached, inspect the assembled carrier unpowered, check continuity and shorts, measure resistor values, and confirm the exact Nucleo revision and solder bridges. Use a benign laboratory source to exercise each input and compare timing against a calibrated analyzer. Verify target pins remain inputs through boot and watchdog recovery. Measure ground offset, input loading, threshold margin, back-power, cable effects, USB disconnect behavior, and thermal limits. ESD, EMC, enclosure suitability, environmental robustness, and regulatory compliance remain unverified.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136, RM0368 for STM32F401 registers, and DS9716 for STM32F401xD/xE electrical characteristics. Verify current revisions and actual module configuration before wiring. The implementation reuses the repository's established PA0/PA1/PA4/PB0 mapping and PA2 USART2 path; current manufacturer documents were not retrieved successfully during this run. No vendor document is bundled or claimed as original work. A clean software run means only that no violation was observed under this bounded sampled contract, not that the target is secure.
