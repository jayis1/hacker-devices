# Lock Rank — Two-Client Ranked-Lock Acquisition Witness

Author and creator: jayis1. Hardware revision A. Wire protocol LR1. Firmware, companion application, and tests: GPL-2.0-only. Hardware generation and checking utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked firmware, a working Linux desktop companion, and routed native KiCad files. Executed software and CAD checks are recorded in VERIFICATION.md. Fabrication, MCU programming, electrical interoperability, timing calibration, and physical deployment have not been performed. This is an observer, not a lock manager, safety interlock, or certificate of deadlock freedom.

## Purpose and overview

Lock Rank investigates an availability boundary that mutual exclusion alone does not establish: when a client acquires multiple shared resources, does it respect an agreed global acquisition order? A maintenance task and an audit task may individually hold valid locks while acquiring them in opposite orders. Under a particular interleaving this can obstruct recovery or audit progress. Looking only for simultaneous ownership of the same resource misses that ordering error. A separate diagnostic observer can preserve a lower-rank acquisition made while the same client still holds a higher-rank resource.

The instrument watches two clients, A and B, and two ranked resources, LOW and HIGH. An owner-controlled adapter exports four slow ownership levels, not requests: A_LOW, A_HIGH, B_LOW, and B_HIGH. Each acquisition is classified as solo, ascending, descending, or simultaneous unordered. Same-rank cross-client ownership is recorded independently as a collision. The resulting evidence distinguishes a rank inversion from exclusion failure, allows either lock to be released first, and does not demand that every operation acquire both resources.

For example, an embedded appliance might have a low-ranked configuration mutex and a high-ranked audit-buffer mutex. Its ordinary maintenance path takes configuration then audit. An error-reporting branch might already hold audit and attempt to take configuration. A laboratory build exposes actual lock ownership before and after these paths. Lock Rank records the descending acquisition if it completes, without reading either protected buffer or collecting credentials. It cannot see a blocked acquisition request, so a deadlock that stops both requests before either completes may produce no rank finding. That limitation is central: this device observes completed acquisition order, not a complete wait-for graph.

## Originality relative to this repository

Existing directories and README purposes were inventoried before design. Searches for lock rank, lock order, and deadlock did not identify another ranked-lock witness. Exclusion Ledger records six pairwise collision relationships among four owners of one shared resource. Lock Rank instead assigns two inputs to each client and checks historical acquisition direction across two distinct resources. Simultaneous A_LOW and B_HIGH is ordinary independent ownership here, not a collision.

Context Stack checks tagged nested entry and last-in-first-out return. Lock Rank imposes no last-in-first-out release rule; releasing LOW before HIGH is legal, although subsequently reacquiring LOW while HIGH remains held is descending. Contention Turn and Admission Priority check selection between requesters of one resource; this device has neither request wires nor a fairness preference. The new contribution is the explicitly bounded rank contract, independent client baselines, conservative exchange classification, and companion evidence validation. Global lock ordering is established engineering; no worldwide novelty or patentability claim is made.

The passive carrier layout, startup code, register definitions, serial transport, main-loop scheduler, and desktop transport reuse existing jayis1-authored platform work from contention-turn. Reuse is intentional and disclosed. The rank engine, LR1 schema, rank-specific tests, and documentation supply the distinct measurement function. Library footprints remain standard KiCad assets; project authorship does not claim authorship of upstream libraries or manufacturer documentation.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. The written scope should name the target, development firmware, diagnostic adapter, permitted workload, attachment points, stop procedure, and evidence-retention period. Begin with a disconnected laboratory fixture whose hazardous outputs and production dependencies are disabled. Never connect this unisolated prototype to mains wiring, occupied access-control equipment, vehicles, medical systems, machinery, or unknown field voltages.

The measured attack surface is resource-management ordering in security-sensitive maintenance, recovery, and audit paths. A red team may ask the owner to exercise an approved error-handling workload that exposes an inversion. A security researcher may compare a proposed fix against the same workload. A penetration tester may attach independent evidence to an availability finding after confirming the real functional consequence through separately authorized testing. The instrument itself does not trigger the workload, acquire a target lock, inject a bus transaction, or modify target firmware.

The diagnostic adapter, observer image, workstation, harness, and operator are trusted. A malicious target can falsify all four indications. An attacker with SWD access can replace the observer image. A hostile workstation can forge or delete exports. CRC is accidental-corruption detection, not authentication, encryption, signed provenance, or anti-replay protection. There is no persistent boot identity, trusted wall clock, secure element, verified update service, radio, network listener, or cloud account. Physical programming remains a local, trusted maintenance operation.

Receive-only behavior is enforced by the supplied GPIO configuration, not an irreversible data diode. Replacement firmware could drive a lead. Even correct firmware can disturb a target through loading, grounding, miswiring, or back-power. Stop if attachment changes target behavior. The author, jayis1, makes no safety certification, regulatory compliance, or production-readiness claim. Authorization does not replace electrical qualification.

## Hardware specifications and bill of materials

The complete assembly uses an external ST NUCLEO-F401RE development board, this passive carrier, short labelled insulated leads, a nonconductive support, and a Linux workstation. The module contains an STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, one-millisecond SysTick, and USART2 transmission at 115200 baud, eight data bits, no parity, one stop bit. PA2 sends telemetry to the module's ST-LINK virtual COM bridge. The USART receiver is disabled.

The external development board supplies regulation, decoupling, reset, USB conversion, and SWD programming. These are not imaginary components omitted from a custom MCU schematic. The custom board is deliberately only a passive harness carrier. It has no radio, antenna, FPGA, battery, charger, active sensor, level translator, isolated supply, storage card, or actuator. It is not a mechanically stackable Arduino shield. Reserve a conservative USB source allowance of 5 V at 500 mA for the complete Nucleo assembly; this is a supply budget, not measured consumption.

| Reference | Quantity | Specification / purchasing candidate |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, axial DIN0207; Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, axial DIN0207; Yageo MFR-25FBF52-1K |
| J1, J3, J4, J5, J6 | 5 | 1x2 2.54 mm vertical header; Samtec TSW-102-07-G-S |
| J2 | 1 | 1x4 2.54 mm vertical header; Samtec TSW-104-07-G-S |
| Harness/support | 1 set | Labelled short wires, insulated base, strain relief |

Purchasing candidates are not live stock or price guarantees. Check supplier drawings, resistor body dimensions, lead diameters, header mating access, and actual board revision before ordering. The PCB measures 80 by 70 mm, with two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks, and a closed outline. Fourteen components, thirty pads, and nine nets appear in the schematic, board, and connectivity manifest. Resistors use 7.62 mm lead spacing. No mounting holes or enclosure CAD are included; underside insulation and harness support must be supplied.

## Pin mapping and electrical envelope

| Input | Target connector | MCU connector | Nucleo / MCU pin | Bit |
| --- | --- | --- | --- | --- |
| A_LOW | J3.1 | J2.1 | A0 / PA0 | 0 |
| A_HIGH | J4.1 | J2.2 | A1 / PA1 | 1 |
| B_LOW | J5.1 | J2.3 | A2 / PA4 | 2 |
| B_HIGH | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only; neither pin supplies power. J2 has four signals and no ground contact. Each target signal passes through a 1 kOhm resistor to an MCU-side node with a 100 kOhm pulldown. Net names ending `_IN` identify target-side nodes. Check connector pin numbers and continuity rather than inferring polarity from physical orientation or wire color.

Only common-ground, nominal zero-to-3.3 V push-pull diagnostic outputs are supported. Five-volt outputs, 1.8 V domains, negative voltages, relay coils, differential buses, and unknown electrical environments are excluded. There is no isolation, surge protection, qualified ESD clamp, fuse, or hot-plug guarantee. The series resistor is not an overvoltage protection system. A disconnected lead reads low through its pulldown and cannot be distinguished from a genuinely released lock.

The static resistive path is 101 kOhm from each asserted target output to ground. Loading, input leakage, cable capacitance, ground offsets, and rise time must be measured on the assembled fixture. Power the observer before applying target signals and disconnect signals before depowering the observer, avoiding back-power through MCU protection structures. Keep leads short. Validate low/high voltage margins against the STM32 datasheet rather than assuming a software test establishes electrical compatibility.

## Architecture and block diagram

```text
Owner-controlled 3.3 V diagnostic ownership outputs
 A_LOW   A_HIGH   B_LOW   B_HIGH
    |       |       |       |
 J3..J6: 1k series, 100k MCU-side pulldown, shared ground
    |       |       |       |
 J2 --> external NUCLEO-F401RE PA0, PA1, PA4, PB0
                |
       four independent qualifiers
                |
       per-client empty baselines --> acquisition classes
       per-rank ownership pairs   --> overlap ledgers
                |
       transactional bounded counters
                |
       LR1 status + CRC16, USART2 TX only
                |
       ST-LINK USB virtual COM --> Linux desktop
                |
       Ledger / History / Integrity --> private JSON export
```

GPIOA and GPIOB are read separately. There is no claim of simultaneous electrical sampling across ports. The target adapter must deliberately slow transitions and hold each relevant state for at least ten nominal milliseconds as a conservative bench setup recommendation. Four equal samples establish a level: the first candidate plus three confirmations. This is not a high-speed logic analyzer and does not capture glitches shorter than its qualification interval. Timing claims refer to sampled, qualified observations, not exact electrical edges.

## Measurement contract and firmware decisions

Each client becomes ready only after its own two inputs are qualified low together. Initial attachment with a held lock does not invent an acquisition. A client can become ready while the other remains unqualified or busy. After readiness, each low-to-high qualified ownership edge increments starts. A held high input is not another acquisition. The counters partition all observed starts: starts equals solo plus ascending plus descending plus ambiguous, separately for each client.

A HIGH acquisition is ascending only when LOW was held in both the preceding and current qualified observations. A LOW acquisition is descending when HIGH was held in either observation. Therefore a direct HIGH-to-LOW exchange is conservatively descending: simultaneous diagnostic release and acquisition do not prove release happened first. Acquiring HIGH alone from empty is legal and classified solo, because rank ordering permits skipping lower ranks. Releasing either resource does not itself create a finding or change previous acquisition classes.

If both locks rise in one qualified observation, both starts are classified ambiguous. Neither is declared ordered or descending. The ambiguous counter therefore grows in pairs. It records missing ordering evidence, not a proven implementation inversion. Independently, simultaneous A_LOW and B_LOW is a LOW collision; simultaneous A_HIGH and B_HIGH is a HIGH collision. Opposite ranks held by different clients are not collisions. Collision segment counts and qualified overlap-tick totals remain visible even before acquisition readiness.

A sampling delta other than one millisecond invalidates qualification, client readiness, and current collision segments. The firmware records one gap and the number of missed nominal ticks, then requires fresh qualification and a new empty baseline for each client. Historical counts and sticky findings remain. A long physical collision interrupted by a sampling gap can become two observed segments; segment totals are not unique incident counts. Repeated calls at the same timestamp are ignored, and normal unsigned timestamp wrap is supported.

All tick changes occur on a speculative structure copy. Counter overflow discards the entire tick and enters a terminal halted state with the previous conserved counts. It clears active qualification and readiness, sets the exhaustion flag, and thereafter updates only time and transport-failure diagnostics. This prevents two simultaneous acquisitions from being half recorded. Watchdog initialization failure also halts before the first telemetry frame. The independent watchdog uses LSI and bounded setup waits; reset causes are captured for reporting, not interpreted as permission to restore a target baseline.

The target is bare-metal Cortex-M4 C11 with explicit peripheral addresses in registers.h, a real vector table, initialized data copying, zeroed BSS, and a linker script at flash address 0x08000000. There is no downloaded SDK or dynamic allocation. The serial driver copies into a bounded buffer, emits at most one byte per service call, and abandons a stalled transmission after one hundred nominal milliseconds. Acquisition runs before serial service, so slow telemetry cannot intentionally block sampling; missed sampling still remains observable.

## Protocol and companion application

LR1 uses ASCII frames beginning `LR1,`, followed by B, P, or S and decimal unsigned fields. An asterisk introduces four uppercase hexadecimal CRC16-CCITT-FALSE digits, followed by LF. The CRC covers bytes before the asterisk, starts at FFFF, uses polynomial 1021, and has no reflection or final xor. Boot B contains reset flags. Policy P contains `3,1,2,1`: qualification confirmations, nominal tick milliseconds, client count, and policy revision. Boot is sent once; policy repeats approximately every second to support late attachment without erasing retained evidence.

Status S fields, in order, are now, levels, valid, halted, faults, gaps, missed, ready; A's observed, baselines, starts, solo, ascending, descending, ambiguous; the same seven B fields; low_overlaps, high_overlaps, low_ticks, high_ticks, active, tx_fail. Ready is a two-bit client mask; active is a two-bit rank collision mask. Fault bits from least significant upward are descending, ambiguous, collision, gap, exhausted, and watchdog unavailable. Status transmission is scheduled approximately every hundred milliseconds, subject to boot/policy priority and transport occupancy.

The Linux application uses standard-library Tk, termios, and select, with no pip or JavaScript dependency installation. Run `python3 app/console.py` from this directory. Enter the ST-LINK tty path and choose New live. Ledger shows the decoded counters and findings. History retains up to 256 snapshots. Integrity shows source, provenance, parser errors, policy compatibility, freshness, reset observations, and connection state. Disconnect preserves evidence but removes live status. Replay reads a regular file bounded to two MiB. Export JSON creates a new file with mode 0600 and refuses to overwrite or follow a destination symlink.

The parser checks syntax, integer widths, CRC, field conservation, sticky flags, coverage bounds, and monotonic counter progression. A contradictory duplicate timestamp, rollback, later boot record, or changed halted evidence freezes interpretation until a new session. Repeated timestamps do not refresh liveness. Two advancing compatible snapshots are required before calling live evidence fresh. Reception pauses beyond one second and cumulative host/device timing divergence conservatively require reconnecting and flushing queued input. Oscillator drift can trigger this conservative rule during long sessions. Without persistent boot identity, some resets remain indistinguishable from valid forward traffic.

## Build, test, and operational workflow

From this directory, run `make -C firmware all test`. GNU Arm Embedded GCC and newlib provide the Cortex-M4 link and binary conversion; host GCC runs the sanitizer-backed state tests and serial register model. Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_app.py' -v` for application tests with a real Tk event loop and Linux pseudo-terminal. The fixture executable emits synthetic records using the production C encoder. These fixtures are not measured hardware results.

Regenerate CAD with `/usr/bin/python3 tools/generate_hardware.py` in an environment containing KiCad 9 and pcbnew. Run `/usr/bin/python3 tools/check_hardware.py` to require clean ERC/DRC, no unconnected items, and exact reference/pin/net/value/footprint agreement across the schematic, PCB, saved XML netlist, and manifest. Run `python3 tools/check_release.py` for README length, substantive C count, attribution, and whitespace checks. Build products, Python caches, and local KiCad preferences are ignored rather than committed.

Before a real trial, inspect the assembled board for shorts, verify every harness pin, measure signal voltages and grounds, and confirm the correct Nucleo solder-bridge configuration. UM1724 describes ST-LINK virtual COM routing with SB13/SB14 enabled and SB62/SB63 disabled; check the actual board revision. Program the ELF through the module's local SWD interface only after independently reviewing the image. Observe an empty baseline, then use an owner-provided slow fixture to exercise solo HIGH, LOW-then-HIGH, HIGH-then-LOW, simultaneous acquisition, flexible release order, and same-rank cross-client overlap. Compare physical waveforms to reported transitions before attaching any development target.

For an authorized regression study, retain the target build identity, adapter source, input mapping, stimulus description, observer commit, export, and any continuity warnings together. Do not interpret zero descending counts as proof of deadlock freedom: blocked requests, uninstrumented clients, additional resources, recursive acquisitions, short pulses, false diagnostic outputs, and activity before baseline are outside coverage. No persistent evidence is stored on the MCU; power loss clears its counters. Export before shutting down and handle research records according to the agreed retention policy.

## Manufacturer references and remaining acceptance work

The pin map and default serial routing are based on ST UM1724, especially the NUCLEO-F401RE connector table and USART communication section: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . Memory capacities, default HSI clock, and electrical limits are described in https://www.st.com/resource/en/datasheet/stm32f401re.pdf . Peripheral definitions should be checked against RM0368 revision 6: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf . Consult applicable device errata before physical deployment.

Software tests and clean CAD rules establish reproducible design checks, not a manufactured product. Outstanding acceptance includes procurement fit, assembly, continuity, actual MCU programming, GPIO thresholds, signal integrity, loading, ground compatibility, clock tolerance, watchdog recovery, unplug behavior with the physical USB bridge, enclosure support, and operator training. Passive-only ERC cannot validate an external power source or the diagnostic truth of target firmware. Keep these limitations attached to every demonstration and research report.
