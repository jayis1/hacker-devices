# Dependency Diamond — Parallel Prerequisite Completion Witness

Author and creator: jayis1. Revision A. Wire protocol DD1.
Firmware, application, and tests: GPL-2.0-only. Hardware utilities: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. This directory contains target-linked C firmware, a functional Linux desktop companion, and native routed KiCad carrier files. VERIFICATION.md records executed software and CAD checks. Fabrication, flashing a physical MCU, electrical qualification, timing calibration, and deployment have not been performed. This is an observation instrument, not a scheduler, access controller, safety interlock, or certification that a target is secure.

## Purpose and overview

Dependency Diamond asks whether a target starts dependent work only after the relevant prerequisite jobs actually finish. The supported contract is a small directed acyclic graph: root job zero must finish before either sibling job one or two starts; both siblings must finish before sink job three starts. Siblings can start in either order, finish in either order, run sequentially, or overlap. The observer records active and completed identities separately. Counting four starts and four completions alone would conceal a premature dependency admission.

A motivating example is a laboratory secure-update pipeline. Job zero authenticates a manifest. Jobs one and two independently validate two payload regions. Job three makes the candidate image selectable. A scheduler might accidentally treat submission of payload validation as completion, allowing the selectable state before both validators have finished. The device observes diagnostic START and FINISH strobes tagged with the four job identities. It does not inspect the image, recover secrets, alter the bootloader, inject faults, or bypass verification. The diagnostic mapping is reviewed with the target owner before any measurement has meaning.

A valid sequence is START zero, FINISH zero, START two, START one, FINISH one, FINISH two, START three, FINISH three. Another valid sequence finishes one before starting two. Both express the same dependency graph. Starting one while zero remains active is a violation even though zero has already started. Starting three after only one sibling finishes is also a violation. A second START for a completed job within the same graph is not reuse: membership persists until the sink completes and the whole graph closes.

The utility is independent bookkeeping at an explicitly instrumented boundary. Firmware logs can report successful aggregate counts while omitting the relationship between a prerequisite and its consumer. A small separate MCU can retain the first observed inconsistency even when the target logger is blocked or later rewrites its summary. This independence is limited by the honesty and fidelity of the diagnostic adapter. The instrument observes the exported contract, not invisible internal execution.

## Repository-relative originality

Repository directories were listed after pulling the default branch. Existing descriptions were searched for diamond and dependency-graph contracts. The closest inspected designs are Prefix Retire, Barrier Mask, and Slot Alias. Prefix Retire permits arbitrary readiness order but enforces a single numeric retirement prefix. Dependency Diamond imposes no ordering between siblings, and checks permission to start work against completion of predecessor identities. Barrier Mask accumulates participant arrivals for one collective release; this design tracks individual start/finish lifecycles and intermediate fan-out before fan-in. Slot Alias permits resource reuse immediately after release and has no prerequisite graph; completed jobs here cannot restart until the graph round closes.

The new contribution is the dependency policy, its active/completed identity model, invariant checks, and exhaustive event-order experiments. The passive resistor carrier, Nucleo interface, STM32 support code, scheduler, CRC framing, and desktop transport intentionally reuse existing work by jayis1. Reused electrical circuitry is not represented as a new circuit invention. Originality is relative to the inspected repository; no worldwide prior-art search or patentability assertion is made. The directory is self-contained and does not import another device folder at runtime.

## Legal, ethical, and operational boundaries

Use only on equipment you own or have explicit written authorization to assess. Permission must cover diagnostic instrumentation, physical attachment, approved workloads, possible interruptions, and evidence retention. Permission to test a web application does not authorize attachment to the appliance's internal electrical signals. Begin on a controlled low-energy development fixture with hazardous outputs disconnected. Never attach this unisolated prototype to medical equipment, vehicles in service, occupied access controls, industrial protection loops, mains-related wiring, or unidentified field signals.

An authorized red team can ask an owner to reproduce a suspected prerequisite race with ordinary development workloads. A penetration tester can compare the same workload before and after a scheduler fix. A security researcher can explore which legal interleavings an implementation supports without acquiring payload contents. These activities do not require covert deployment, credential collection, autonomous exploitation, persistence, target packet injection, or disruptive radio transmission. None of those capabilities is implemented.

Keep the device visible, label the harness, and agree on a stop procedure. Receive-only firmware does not prevent ground faults, loading effects, connector errors, or back-power. If connecting the observer changes target behavior, stop and investigate before treating its readings as representative. A diagnostic finding is evidence for engineering investigation, not automatic proof of an exploitable vulnerability. An absence of findings is not a security certificate.

## Attack surface and threat model

The assessed surface is the target's transition from queued or active work to dependency-satisfied work. Candidate defects include treating dispatch as completion, using an any-of test instead of an all-of join, stale completion bits surviving into another graph, duplicate start callbacks, wrong tag routing, and missing completion notifications. The device classifies a FINISH without a currently active job as orphan completion. A duplicate FINISH therefore also becomes an orphan. Duplicate START has precedence over unmet prerequisites when a tag already exists in the graph.

The adapter must expose the actual agreed boundaries. For example, FINISH should represent a validator's completed decision rather than placing validation on a queue. START three should represent the effective consumer admission rather than a user-interface message. A malicious or incorrectly instrumented target can lie on all four wires. Corroborate findings with adapter source review, independent waveform capture, and a functional test of the protected operation. The observer does not prove that validation succeeded, that data were erased, or that storage became durable.

Trusted components are the operator, workstation, external Nucleo module, firmware image, programming interface, and harness. Serial frames and replay files are untrusted parser inputs. CRC detects accidental corruption, not forgery. There is no authenticated pairing, secure element, signed log, persistent boot identity, encrypted export, trusted wall clock, secure updater, cloud service, radio, or network listener. Protect ST-LINK and serial permissions. Replacement firmware could drive the nominal input pins, so this is not a certified physical data diode.

Records contain tags, bitmaps, counters, and integrity metadata rather than target contents. Nevertheless, paths and filenames can identify laboratory assets. Export only into access-controlled engagement storage, follow the agreed retention period, and do not confuse file deletion with secure erasure. RAM evidence disappears on observer reset. A disconnected signal may appear low through its pulldown, so cable loss cannot always be distinguished from normal inactivity.

## Hardware specifications and bill of materials

The compute module is an external ST NUCLEO-F401RE, MB1136 family, carrying an STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware assumes the nominal reset-default 16 MHz HSI clock. It uses one-millisecond SysTick and USART2 at 115200 baud, eight data bits, no parity, and one stop bit. There is no RTOS, heap allocation, vendor SDK dependency, PLL configuration, or floating-point requirement. The Nucleo provides regulation, decoupling, reset, SWD programming, and the ST-LINK USB virtual serial connection.

The carrier is an 80 by 70 mm two-layer board with nominal 1.6 mm thickness, 0.3 mm tracks, a closed rectangular outline, and a ground-layer transition via. Fourteen through-hole components occupy thirty electrical pads on nine named nets. The board uses harness connections rather than stacking as a pin-compatible shield. No mounting holes or enclosure CAD are supplied; provide insulating support, strain relief, and a cover appropriate to the bench environment.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical through-hole header, 2.54 mm pitch |
| J2 | 1 | 1x4 vertical through-hole header, 2.54 mm pitch |
| Harness and support | 1 set | Labelled insulated leads and nonconductive base |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Equivalent resistors must preserve value, tolerance, body clearance, lead spacing, and suitability for the intended environment. Equivalent headers must preserve pin numbering and mating geometry. A different MCU module is not a drop-in substitution: revalidate power, pin mapping, reset clocks, serial routing, register definitions, and programmer access. Pricing, stock, assembly yield, and mechanical fit have not been measured. There is no FPGA, radio, battery, charger, analog sensor, level translator, active protection circuit, or target power output.

## Wiring, power, and electrical limits

| Function | Target header | MCU harness | Nucleo label / GPIO | Logical bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3 pin 1 | J2 pin 1 | CN8 A0 / PA0 | 0 |
| TAG1 | J4 pin 1 | J2 pin 2 | CN8 A1 / PA1 | 1 |
| START | J5 pin 1 | J2 pin 3 | CN8 A2 / PA4 | 2 |
| FINISH | J6 pin 1 | J2 pin 4 | CN8 A3 / PB0 | 3 |
| Ground | J3–J6 pin 2 | J1 both pins | Nucleo GND | none |

J1 is ground only; neither pin accepts a supply. J2 carries four signals and no ground or power. Names ending `_IN` are target-side nodes before the series resistor; unsuffixed nets are MCU-side nodes with pulldowns. Verify pin-one orientation and continuity rather than relying on wire colors. The carrier schematic deliberately represents the external development board through its harness, not a fictitious onboard MCU.

Supported signals are known common-ground push-pull zero-to-3.3 V logic. Each channel presents a nominal 101 kOhm resistor path to ground before leakage and source impedance. The 1 kOhm series resistor is not a qualified overvoltage barrier. There is no isolation, dedicated ESD clamp, fuse, reverse-current protection, or hot-plug guarantee. Five-volt, negative, 1.8 V, differential, relay-coil, and industrial field signals are outside the contract.

Power the Nucleo through its documented ST-LINK USB connector after inspecting board revision and jumper settings. Reserve a conservative 5 V, 500 mA source budget for planning; this is not measured consumption or USB certification. USB ground is connected to target ground. Establish reference compatibility, power the observer before enabling target signals, and remove those signals before removing observer power. Otherwise input protection structures can back-power an unpowered MCU. Loading, input thresholds, leakage, ground offsets, and power sequencing require physical qualification.

## Architecture and block diagram

```text
Owned diagnostic adapter          Passive carrier             External Nucleo
TAG0 ----------------------------- 1k --+---------------------- PA0 / A0
TAG1 ----------------------------- 1k --+---------------------- PA1 / A1
START ---------------------------- 1k --+---------------------- PA4 / A2
FINISH --------------------------- 1k --+---------------------- PB0 / A3
                                        | each MCU-side node
                                       100k
                                        |
Common ground ------------------------- GND ------------------- GND / USB

                 +--> job 1 --+
job 0 completed -+           +--> job 3 --> next empty graph
                 +--> job 2 --+

GPIO -> continuity guard -> bundled tag / independent strobe qualification
     -> prior setup and overlap guards -> active and completed identity sets
     -> prerequisite checks -> transactional counters -> DD1 encoder
     -> USART2 PA2 TX -> ST-LINK USB -> Ledger / History / Integrity screens
```

GPIOA is read before GPIOB; this is not an atomic cross-port latch. The device cannot resolve processor-speed races, arbitrarily short pulses, or physical ordering inside a sample interval. Use deliberately slow diagnostic notifications. Start with twenty milliseconds of tag setup and twenty milliseconds each of strobe-high and strobe-low intervals. Validate waveforms independently; qualification does not calibrate oscillator tolerance, interrupt latency, wiring delay, or inter-port skew.

## Exact firmware measurement contract

A graph round contains exactly jobs zero through three. Tags name unique jobs within that graph, not hashed identifiers or reusable resource classes. The fixed prerequisite masks are zero for job zero, bit zero for each sibling, and bits one plus two for the sink. Cross-round overlap is unsupported. No deadline is imposed: a stalled graph remains incomplete rather than falsely passing. The operator can observe an active bitmap indefinitely without any guarantee of eventual completion.

Establish a real empty target graph before restarting the observer. WAIT becomes TRACK after all inputs qualify and both raw and qualified strobes are low. Tags need not be zero. Quiet wires do not prove that the target has no pending job. Mid-graph attachment is unsupported. A strobe held high during startup does not invent a new event when qualification completes; prospective tracking begins only after a quiet baseline.

The two tag bits qualify as one bundle, preventing separately delayed bits from synthesizing an unobserved identity. START and FINISH qualify independently, so unrelated data chatter cannot indefinitely hide a sustained operation. Qualification requires the first candidate observation plus three unchanged one-millisecond observations. Repeated calls in the same tick do nothing. Short pulses can be missed or rejected, and retained qualified levels are not raw waveforms.

Each operation is a qualified rising edge. Five stable evaluated tag intervals must already exist before that edge. Raw and qualified tag must agree; the current raw operation strobe must remain high. The opposite strobe must be low in previous qualified, current qualified, and current raw samples. Simultaneous rises reject both attempts. Opposite raw activity at the endpoint is ambiguous even before it qualifies. A sampled raw tag glitch breaks setup even if it never becomes a qualified tag.

An accepted START first checks that its tag is neither active nor completed. It then requires every predecessor bit to be completed, not merely active. Success sets the active bit and increments launched. An accepted FINISH requires an active bit, clears it, sets the completed bit, and increments completed. When all four bits are completed, rounds increments and the completed set clears atomically. The sink necessarily completes last under this graph, but sibling order remains unconstrained.

All accepted and rejected qualified operation edges after baseline contribute to attempts. A faulting sample contributes one or two rejected attempts. Any policy error permanently halts correlation, retaining the graph state. Silence does not recover it. Export evidence, restore the actual target baseline, and deliberately restart the observer for another run. Any advancing sampling delta other than one similarly records a gap and missed intervals, then halts. Ordinary unsigned timer wrap is supported.

Every sample updates a copy of the ledger. If any counter update would overflow, the complete sample is rolled back and the prior state is halted with exhaustion. This includes completion of the sink: overflow cannot half-close a graph. Once halted, only time and saturating transmitter-failure diagnostics change. Necessary identities are launched = four times rounds plus population(active OR done), completed = four times rounds plus population(done), and attempts = launched plus completed plus rejected. Active and done must be disjoint, and each seen job's predecessors must appear in done. These constraints are checked in both encoder and companion, but are not authentication or a complete proof of physical realizability.

## Runtime, protocol, and companion application

The main loop samples before servicing output. UART submission copies one bounded frame, and polling writes at most one ready byte without waiting. A packet unfinished after one hundred nominal milliseconds is abandoned and tx_fail increases. The watchdog startup waits are bounded by SysTick; successful startup is fed by main-loop progress. An initialization failure halts evidence before first status. Actual watchdog behavior, including stopped clocks and brownouts, remains a physical acceptance task. Target GPIO remains input-only and USART receive is disabled.

DD1 uses ASCII `DD1,<kind>,<decimal fields>*<CRC>` followed by LF. The four uppercase hexadecimal CRC digits encode CRC-16/CCITT-FALSE over all bytes preceding the asterisk, polynomial 0x1021, initial 0xffff, no reflection or final XOR. B carries reset flags once. P carries `3,1,5,4`: qualification intervals, nominal sample milliseconds, prior setup intervals, and graph size. The DD1 identifier binds the fixed diamond topology; a different graph requires a different protocol. Policy repeats approximately each second for late host attachment without erasing evidence. S repeats approximately every hundred milliseconds.

S fields are `now levels valid halted faults phase active done stable observed rounds attempts launched completed rejected gaps missed tx_fail`. All are unsigned 32-bit. Phase is WAIT zero or TRACK one. Fault bits are unmet prerequisite=1, orphan completion=2, duplicate start=4, interface ambiguity=8, acquisition gap=16, exhaustion=32, and watchdog startup=64. Only the first terminal reason is retained. Active and done are bitmaps, while completed is a cumulative count. Valid describes qualification, not a successful security assessment.

Run `python3 app/console.py` on Linux with Tk. Enter the exact intended serial path and select New live. There is no scanning, command channel, permission escalation, or cloud upload. Ledger displays decoded state, counters, and named findings. History retains up to 256 accepted snapshots. Integrity displays provenance, connection, policy compatibility, receipt age, parser errors, reset observations, and freeze reason. Disconnect preserves evidence but removes live status. Export before intentionally replacing a session. A failed connection preserves previous evidence, and opening a second port is refused while the first is owned.

Live means compatible policy, connection, two advancing status timestamps, and recent device progress. It does not mean a safe graph: halted evidence can still be transported live. Duplicates do not renew freshness. Counter rollback, disappearing findings, contradictory duplicates, changed halted fields, unsupported policy, or impossible membership changes freeze acceptance. Reception pauses exceeding one second and excessive host/device elapsed-time divergence also freeze collection instead of relabelling backlog as current. Drift allowance is capped; long runs with uncalibrated clocks can conservatively freeze.

Replay accepts regular files no larger than two MiB, rejects FIFOs and devices, bounds framing memory, resynchronizes on the full DD1 marker, and counts incomplete tails at EOF. Replay is never labelled live. Private JSON export creates a new mode-0600 file, refuses overwrite and final-component symlinks, and includes author jayis1, source, provenance, integrity metadata, latest state, and retained history. It is not encrypted or signed. Without persistent boot identity, a lost boot frame and forward-looking counters can conceal a reset; operator notes and independent traces remain necessary.

## Build, test, and physical acceptance

Tools required are GNU Make, Arm GNU GCC with newlib/nosys, host GCC, Python 3 with Tk, Xvfb for headless GUI tests, and KiCad 9 CLI/Python bindings with standard footprint libraries. No pip or npm packages are required. From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

The build emits an actual Cortex-M4 ELF, binary, and map under firmware/build. The linker uses flash at 0x08000000 and reserves stack headroom. On a correctly identified, physically controlled board, a supervised OpenOCD programming procedure can use `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program firmware/build/dependency-diamond.elf verify reset exit"`. This is guidance, not an executed flashing claim. Recover through ST-LINK under reset according to the board manual. The companion does not update firmware.

Host tests enumerate all permutations of the eight unique start/finish events against an independent prerequisite oracle, stopping at the first illegal operation. Additional cases exercise duplicate starts, sibling overlap, raw endpoint ambiguity, held strobes, tag chatter, timer wrap, acquisition loss, and transactional exhaustion at sink completion. The real C encoder generates a synthetic fixture for parser, replay, private-export, and actual Tk event-loop tests using Linux pseudo-terminals. Synthetic data are not physical measurements. Native CAD checks execute ERC, DRC, and fresh netlist export and compare component values, footprints, and every pin/net against PCB pads and the manifest.

Before fabrication review schematic appearance, copper clearance, connector orientation, actual parts, and mechanical support. After assembly verify continuity, resistor values, input loading, thresholds, common ground, power sequencing, and GPIO direction through reset. Correlate qualified events with a calibrated logic analyzer. Test unplug, brownout, watchdog recovery, missing notifications, repeated graph rounds, and all supported sibling interleavings. Manufacturing readiness, ESD, EMC, environmental reliability, calibrated timing, and regulatory suitability remain unverified.

Manufacturer references are ST UM1724 for the Nucleo-64 MB1136 platform, RM0368 for STM32F401 registers, and DS9716 for electrical characteristics. This revision reuses the existing repository pin map and register platform; fresh manufacturer-document verification was not performed. Check current documentation and the actual board revision before connection. Preserve the distinction between design intent, executed software/CAD checks, and physical acceptance throughout any report.
