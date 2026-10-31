# Owner Bind — Two-Client Resource-Binding Evidence Bench

Author and creator: jayis1. Hardware revision A. Wire protocol OB1.
Firmware, companion application, and tests: GPL-2.0-only. Hardware generation and checking tools: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. This directory includes target-linked C firmware, an executable Linux desktop companion, and routed native KiCad carrier sources. See VERIFICATION.md for checks actually performed. Fabrication, physical programming, electrical qualification, timing calibration, and field deployment remain outstanding. This is an observation instrument, not an access-control device, memory-protection unit, or safety interlock.

## Purpose and practical value

Owner Bind checks a small but important identity boundary: a resource must be released by the client to which it was assigned, and a client operating under a single-outstanding-resource policy must not acquire a second slot while retaining the first. An owner-controlled diagnostic adapter exposes SLOT, CLIENT, ALLOCATE, and RELEASE on four slow logic lines. There are two resource slots and two clients. Either client may acquire either free slot, but the active assignment must remain one-to-one. Either occupied slot may release first.

The motivating case is an embedded security appliance with two asynchronous work descriptors and two request domains. An error callback might use the right descriptor index but the wrong client context. A simple occupancy check accepts that release because the descriptor is occupied. Owner Bind preserves the assigned client and rejects the mismatched release. A separate integration defect allocates another descriptor to a client whose contract allows only one outstanding request. Different slot identifiers make that error invisible to a duplicate-slot detector; this instrument checks the client side of the relationship as well.

For example, allocate slot zero to client one, then allocate slot one to client zero. Both bindings are legitimate. Releasing slot zero as client zero is a wrong-client finding even though client zero owns a different resource. The original slot-zero/client-one binding remains in the frozen evidence. In a second experiment, allocate slot zero to client one and then slot one to client one. The second admission violates the single-outstanding-client contract. Slot one stays vacant in the retained accepted state, and the rejected operation is counted separately.

Same-client reuse is explicitly legal after release. Client zero may acquire slot zero, release it, acquire slot zero again, release it, then migrate to slot one. There is no alternating-client rule, numerical ordering, FIFO requirement, lock rank, fixed slot affinity, or requirement to exercise both clients. The device is useful only when a one-resource-per-client policy is actually part of the target specification. Targets that legitimately permit a client to own several resources need a different contract; a finding against this policy is not evidence that their design is wrong.

## Originality and platform reuse

The configured repository default branch was pulled and device directories and README titles were inventoried before implementation. The closest contracts were read directly. Slot Alias detects duplicate live slot identifiers but has no client identity. Slot Generation requires generation toggling between allocations and cannot represent legitimate repeated assignment to the same named client. Read Share models compatible readers and an exclusive writer of one resource, not a mapping between independent resource and client identities. Lock Rank permits one client to own two resources when acquired in the correct order; Owner Bind forbids that second concurrent ownership regardless of order.

The new contribution is the two-sided binding policy, wrong-client release and already-bound-client checks, client-labelled evidence, companion constraints, and array-oracle regressions covering reuse and migration. These are repository-relative distinctions, not a claim of worldwide invention, patentability, or a new mathematical matching algorithm. The passive carrier geometry, STM32 register support, bounded UART scheduler, CRC writer, and Linux transport deliberately reuse jayis1-authored Slot Generation platform work. This folder is self-contained; neither builds nor application execution import sibling device folders. Standard KiCad footprint geometry retains its upstream provenance.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Scope must cover internal attachment points, diagnostic firmware, approved workloads, possible interruption, ground connections, and evidence retention. A general network testing agreement does not automatically authorize opening an appliance or instrumenting its internal signals. Start with an isolated low-energy development fixture, disconnected hazardous outputs, visible labels, an agreed stop condition, and an owner-reviewed recovery procedure. Do not connect this unisolated prototype to production doors, medical equipment, vehicles, machinery, mains-related wiring, or unknown field voltages.

The assessed attack surface is ownership bookkeeping at the target's allocation and completion boundaries. Relevant defects include confused-client callbacks, cross-domain cleanup, duplicate live assignments, duplicate completion, missing diagnostic events, and adapters that change identity while asserting an operation. An authorized red team can request a controlled fault-path workload from the product owner. A penetration tester can retain before-and-after evidence for an independently confirmed authorization defect. Researchers can compare normal and cancellation paths without reading the protected work data. The instrument does not trigger those workloads, inject commands, corrupt memory, recover credentials, persist on a target, or provide covert access.

The adapter must report effective allocation and accepted release, not untrusted requests that the target correctly rejects. CLIENT must identify an actual client domain, not a hash of many independent callers. SLOT must identify one actual exclusive resource. If these meanings change during a run, the evidence loses its interpretation. Review adapter source and compare waveforms with target functional traces. A dishonest target can lie on every wire; this device is not an attestation system. A valid diagnostic binding does not prove that the target used the correct buffer or enforced the corresponding permissions internally.

Trusted components include the operator, harness, Nucleo board, observer firmware, programming interface, and workstation. CRC detects accidental corruption, not intentional forgery. There is no secure boot, authenticated updater, signed evidence, persistent boot identifier, trusted wall clock, encrypted export, radio, cloud service, or network listener. Someone with programming access can replace the input-only image with output-driving firmware. Receive-only describes the supplied image, not an irreversible hardware data diode. Common-ground attachment, loading, miswiring, or back-power can still disturb a target. Stop if connection changes behavior.

## Hardware specifications and assembly scope

The complete assembly uses an external ST NUCLEO-F401RE development board, the passive carrier provided here, short labelled leads, an insulating support, and a Linux workstation. The STM32F401RET6 Cortex-M4 has 512 KiB flash and 96 KiB SRAM. Firmware uses nominal reset-default 16 MHz HSI, one-millisecond SysTick, and USART2 transmission at 115200 baud with eight data bits, no parity, and one stop bit. PA2 routes telemetry toward the board's ST-LINK virtual COM bridge. Target-facing GPIO remains input-only and the UART receiver is disabled.

The Nucleo module supplies regulation, decoupling, reset, boot configuration, SWD programming, MCU, and USB connectivity. These are not claimed as components on the custom carrier. The carrier is an 80 by 70 mm cable-connected board, not a stacking shield. It has two copper layers, nominal 1.6 mm thickness, 0.3 mm routed tracks, fourteen through-hole components, thirty electrical pads, and nine named nets. There are no mounting holes or enclosure files. Provide insulated supports, strain relief, and a protective cover. No FPGA, battery, charger, antenna, analog sensor, isolated supply, target power output, or level translator is fitted.

| Item | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, actual revision checked before wiring |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Single-row 1x2 vertical 2.54 mm header |
| J2 | 1 | Single-row 1x4 vertical 2.54 mm header |
| Harness and support | 1 set | Short insulated leads, labels, nonconductive support |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Match purchased body dimensions, connector mating access, drill fit, and environmental ratings before assembly. Price, availability, yield, and connector fit have not been verified. The MCU module is purchased separately and is not represented as a fictional four-pin microcontroller symbol in the schematic. J2 represents only the four signal wires going to that external module.

| Signal | Target header pin 1 | J2 pin | External Nucleo / GPIO | Sample bit |
| --- | --- | --- | --- | --- |
| SLOT | J3 | 1 | CN8 pin 1, A0 / PA0 | 0 |
| CLIENT | J4 | 2 | CN8 pin 2, A1 / PA1 | 1 |
| ALLOCATE | J5 | 3 | CN8 pin 3, A2 / PA4 | 2 |
| RELEASE | J6 | 4 | CN8 pin 4, A3 / PB0 | 3 |

Pin two of each target header is ground. Both J1 pins are ground only; neither is a supply input. J2 has four signals and no ground or supply pin. Connect a separate Nucleo ground lead to J1. Nets ending `_IN` are on the target side of the series resistor; unsuffixed nets are MCU-side nodes. Confirm pin-one orientation and continuity rather than trusting wire color.

Only known common-ground zero-to-3.3 V push-pull diagnostic logic is supported. Each signal traverses 1 kOhm into a node with a 100 kOhm pulldown. The nominal resistive path to ground is 101 kOhm before leakage and source impedance. The series resistor is not a qualified overvoltage barrier. There is no dedicated ESD clamp, galvanic isolation, fuse, reverse-current protection, or hot-plug guarantee. Five-volt signals, negative voltages, 1.8 V logic, differential buses, relay coils, and unidentified field wiring are outside scope.

Power the Nucleo through its documented ST-LINK USB connection with revision, jumper, and solder-bridge settings checked. A conservative source-planning allowance is 5 V at 500 mA, not a measured consumption figure or USB compliance claim. USB ground reaches target ground. Power the observer before enabling target outputs, and remove target outputs before removing observer power. Otherwise MCU input structures can back-power the board. Manufacturer references are ST UM1724 for MB1136 module connections, RM0368 for STM32F401 registers, and the STM32F401RE datasheet for limits. Fresh UM1724 retrieval failed during this run; the reused map must be confirmed against the purchased module's manual before wiring.

## Architecture and block diagram

```text
Owned diagnostic adapter                 Passive carrier         External Nucleo
SLOT ----------------------------------- 1k --+------------------ PA0
CLIENT --------------------------------- 1k --+------------------ PA1
ALLOCATE ------------------------------- 1k --+------------------ PA4
RELEASE -------------------------------- 1k --+------------------ PB0
                                              | each channel
                                             100k
                                              |
Target ground ------------------------------- GND --------------- USB ground

GPIO -> continuity guard -> bundled slot/client and independent strobe qualification
     -> prior setup check -> two-sided binding policy -> transactional ledger
     -> OB1 status/CRC writer -> bounded USART2 TX -> ST-LINK USB
     -> Linux Ledger / History / Integrity screens -> private JSON evidence
```

GPIOA and GPIOB are read separately; there is no atomic cross-port latch. This is a slow diagnostic observer, not a processor-speed logic analyzer. Begin with twenty milliseconds identity setup, twenty milliseconds strobe high, and twenty milliseconds both strobes low for every operation. Verify independent waveforms before shortening those intervals. These margins are a fixture starting point, not calibrated electrical timing specifications. Qualification cannot establish inter-port skew, cable propagation, interrupt latency, oscillator tolerance, or the ordering of physical edges inside one sampling interval.

## Firmware contract and implementation decisions

Before starting the observer, independently establish that neither target resource has an owner. WAIT becomes TRACK after all inputs qualify and raw and qualified strobes are low. Quiet wires alone do not establish an empty target. Starting the observer during unknown existing ownership is invalid. Late desktop attachment to an already-running observer is different: retained state and periodic policy frames support that use without reconstructing ownership from silence.

SLOT and CLIENT qualify as one two-bit bundle. ALLOCATE and RELEASE qualify independently. A changed candidate requires three subsequent matching sample intervals to qualify. Five prior stable identity observations are required before an operation endpoint; setup cannot be earned on that endpoint. The raw identity must match the qualified identity, and the opposite strobe must be low in previous qualified, current qualified, and raw observations. Both simultaneous rising strobes are rejected together. A short pulse may be missed entirely; no comprehensive glitch-detection claim is made.

For allocation, an occupied requested slot is a duplicate-slot fault, regardless of the claimed client. Otherwise the other live slot is checked: if its client matches the requested client, the new allocation is a binding fault. If both checks pass, the slot becomes active and records its client. For release, a vacant slot is an orphan fault; an occupied slot requires a matching client. Only a matching release clears occupancy. Duplicate and orphan classifications precede the binding check so the finding describes the most direct contradiction. Historical client bits on vacant slots do not prohibit reuse or migration.

All sample updates happen on a speculative copy. Counter exhaustion discards the entire update and preserves prior accepted ownership. Any missed acquisition tick causes permanent halt, not a silently restarted qualification window. Duplicate calls at the same timestamp are ignored; ordinary unsigned timestamp wrap is supported. After halt, ownership, findings, and acquisition accounting remain frozen; device time and the saturating transport-failure diagnostic may advance. Export evidence, reestablish an empty target, and deliberately restart the observer to begin again. There is no remote clear command or automatic recovery.

The implementation conserves accepted allocations = accepted releases + current occupied slots. Attempts = allocations + releases + rejected edges. Allocations also equal closed nonempty episodes + one if an episode is active + allocations made while another slot was live. Peak occupancy is bounded by two. Field `order` is a legacy packed-evidence name, not ordering: bits zero and one retain the last client assigned to slots zero and one; bits two and three retain ever-assigned membership. Unseen slots have zero stored client. If both slots are active their client bits must differ. Released historical client bits may legitimately match.

`engine.c` contains the contract, `wire.c` validates and serializes snapshots, and `drivers/board.c` owns input GPIO, SysTick, UART setup, reset-cause capture, and bounded watchdog initialization. `drivers/serial.c` performs nonblocking bounded transmission. `startup.c` initializes data and BSS and supplies the reset vector. `board.h`, `registers.h`, and `linker.ld` define the real target interfaces and memory layout. There is no heap, RTOS, downloaded SDK dependency, or host-to-target command parser.

## Application and protocol

Frames are ASCII `OB1,<kind>,<decimal fields>*<four uppercase CRC digits>\n`. CRC-16/CCITT-FALSE covers bytes before the asterisk, using polynomial 0x1021, initial 0xffff, no reflection, and no final xor. Kind B contains reset flags. Kind P contains `3,1,5,2`: qualification intervals, nominal sample milliseconds, setup observations, and slot capacity. Policy repeats about once per second independently of the one-shot boot record. This distinguishes compatibility announcements from resets.

Status kind S contains, in order: `now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail`. Launched means accepted allocations, completed accepted releases, rounds closed nonempty episodes, and queued allocations accepted while another slot was live. Queued does not imply a FIFO. Fault bits are 1 binding mismatch or already-bound client, 2 orphan release, 4 duplicate slot, 8 interface ambiguity, 16 acquisition gap, 32 counter exhaustion, and 64 watchdog startup failure. The first terminal fault is retained.

Status transmission is attempted every hundred nominal milliseconds when the UART buffer is available. A stalled frame expires after a hundred milliseconds rather than blocking acquisition. Failed submissions and timeouts increment tx_fail. Snapshots are not an exhaustive event trace. Terminal levels reflect the fault observation, not subsequent live voltages. No incoming serial bytes are consumed by firmware and no application button controls the target.

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk installed. No third-party Python packages are required. Enter the exact virtual serial path and choose New live. The app opens read-only, configures 115200 8N1, refuses a second simultaneous connection, restores terminal attributes when possible, and detects hangup. Ledger shows active slot/client pairs, counters, phase, and findings. History retains up to 256 snapshots. Integrity shows provenance, compatibility, parser errors, reset information, freshness, and freeze reasons.

Two advancing timestamps are needed before live progress is displayed. Duplicate timestamps do not refresh evidence. Long reception pauses and excessive accumulated host/device drift freeze interpretation rather than presenting queued data as current. Conservative drift rejection can occur with an uncalibrated oscillator. Counter rollback, disappearing findings, changed halted evidence, or a later boot also freeze the session. Without persistent boot identity, some lost-boot resets with plausible forward counters remain indistinguishable.

The companion checks both single-frame conservation and lower bounds between sparse snapshots. A changed client requires enough new allocations and releases to explain replacement of the old binding. Ever-assigned history cannot disappear. These checks are necessary consistency conditions, not exhaustive hidden-event reconstruction or authentication. Replay accepts only regular files up to two MiB, bounds framing, resynchronizes on the complete marker, and counts partial EOF tails as errors. Replay is labelled non-live. Export creates a new mode-0600 JSON file, refuses overwriting and final-component symlinks, and includes jayis1 authorship, provenance, integrity metadata, latest state, and history. Storage is unsigned and unencrypted; use trusted directories and an agreed retention policy.

## Build, tests, and bring-up workflow

From this directory run `make -C firmware all test`. Requirements are GNU make, host GCC with address/undefined-behavior sanitizers, and arm-none-eabi GCC with newlib/nosys support. Outputs are `firmware/build/owner-bind.elf`, `.bin`, and `.map`. No test flashes hardware. Run `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v` for parser, contract, replay, export, and actual Tk event-loop testing through Linux pseudoterminals. Run `python3 -m compileall -q app` for byte compilation.

The C oracle represents ownership as an independent two-element array with vacant entries, explores both identities and operations, and tests reuse, migration, both release orders, wrong-client release, and second-slot client collision. The fixture emits actual encoder output into the real companion, but all such data is explicitly synthetic software evidence. It is not a measured target trace. Negative tests use checksum-valid impossible snapshots so CRC alone cannot mask missing semantic validation.

Native CAD generation requires KiCad 9 and its matching pcbnew Python module. Run `/usr/bin/python3 tools/generate_hardware.py` followed by `/usr/bin/python3 tools/check_hardware.py`. The checker runs ERC and DRC, exports a fresh netlist, and compares component references, values, full footprint identities, pin numbers, and net assignments against board pads and the manifest. The schematic uses real embedded component symbols and the PCB contains routed copper and a closed outline. Passive-only ERC does not validate the external module or physical protection.

For physical acceptance, review adapter semantics, continuity, component values, connector orientation, ground offsets, leakage, thresholds, and power sequencing first. Then establish an empty target, power the observer, and enable diagnostic signals. Exercise both clients on both slots, same-client reuse after release, client migration, and both legal departure orders. On a safe fixture deliberately introduce a wrong-client accepted release and confirm the rightful binding is retained. Repeat separately with double allocation by one client, duplicate slot, orphan release, and ambiguous strobes. Reset the actual target baseline between terminal tests.

Assembly inspection, programming, ST-LINK routing, watchdog recovery, oscillator calibration, waveform correlation, sustained serial load, ESD behavior, enclosure safety, and actual target loading remain laboratory gates. Compare with an independent logic analyzer and target functional logs before relying on observations. Until those gates pass, describe this as a software-build-tested and CAD-checked prototype, not production-qualified security instrumentation.
