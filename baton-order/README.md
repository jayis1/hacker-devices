# Baton Order — Four-Domain Cyclic Ownership Witness

Author and creator: jayis1. Hardware revision A. Protocol BO1. Firmware and companion application: GPL-2.0-only. Hardware generation and verification utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design. Target firmware builds, software tests, and native KiCad checks are recorded in VERIFICATION.md. Fabrication, MCU programming, electrical measurements, oscillator calibration, and physical deployment have not been performed. This instrument is an observer, not an arbiter, access controller, isolation barrier, safety interlock, or certificate of correct security behavior.

## Purpose and practical value

Baton Order checks a security scheduling property that mutual exclusion alone cannot establish: when four domains share a resource under a strict cyclic policy, does ownership actually advance in the declared order? A system can grant only one domain at a time and still repeatedly admit one privileged client, skip an audit client, or run a cleanup sequence backwards. Four owner-provided diagnostic outputs represent actual ownership. The instrument builds a directed four-by-four transition matrix and classifies each observed transfer as forward, skipped, reverse, or repeated admission.

The intended cycle is domain 0, domain 1, domain 2, domain 3, then domain 0 again. The physical connector assignment defines those identities. A typical owner-controlled fixture could expose application processing, audit collection, sanitization, and maintenance as four scheduled service windows. Repeated admission to application processing without the intervening windows is a different finding from simultaneous ownership. A security researcher can compare the transition matrix before and after an arbiter fix without collecting any application content or credentials.

This is a deliberately narrow instrument. It observes slow diagnostic indications, not CPU instructions or internal bus transactions. It cannot prove that sanitization occurred, establish that an audit record was stored, or identify a particular exploit. It reports violations of a declared indication-level contract. That contract must be appropriate for the target: an ordinary demand-driven round-robin scheduler may legitimately skip an inactive requester. Such a target does not satisfy this instrument's strict-cycle premise unless the owner exports a separate sequence of scheduled windows that includes empty turns.

## Originality and related designs

The repository directories and README purposes were inspected before selection, including the detailed contracts of Exclusion Ledger, Starvation Window, and Handoff Audit. Exclusion Ledger measures simultaneous grant relationships and overlap durations. Starvation Window measures pending requests, waiting age, and peer bypasses for two clients. Handoff Audit compares two command and feedback paths for break-before-make behavior. None of those primarily records a four-owner directed transition matrix under a strict cyclic order, with distinct repeated admission and phase reacquisition semantics.

Baton Order intentionally reuses the existing jayis1-authored passive carrier topology, explicit STM32 registers, startup, UART transport, main-loop scheduler, and desktop serial infrastructure. Reusing tested infrastructure avoids pretending that another resistor network is a new electrical invention. Its new contribution is the sequence evidence contract, transition accounting engine, matrix validation, tests, and research workflow. Cyclic scheduling is established engineering; originality here is relative to this repository, not a worldwide prior-art search, patentability opinion, or commercial uniqueness guarantee.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. The authorization should identify the asset, diagnostic firmware, wiring points, permitted workload, operator, stop conditions, and retention period. Start on an isolated bench with consequential outputs disconnected. Do not connect this unisolated prototype to occupied access-control systems, moving vehicles, medical devices, emergency infrastructure, machinery, mains wiring, or unknown-voltage field interfaces. A disclaimer is not a substitute for competent electrical review.

The investigated attack surface is the ownership sequencing boundary in a shared security resource. Relevant failures include duplicated grant state after retry, an incorrect next-owner index, a privileged path that bypasses a scheduled audit turn, and rollback of the arbiter state during recovery. An authorized red team can exercise approved workloads and use the resulting matrix to narrow a report. The instrument does not generate requests, alter grants, inject faults, or exploit a detected condition. It has no radio, network server, target-facing command parser, credential recorder, or autonomous attack feature.

Trust boundaries remain important. The target diagnostic adapter must truthfully expose actual ownership rather than merely repeat intended software state. A malicious target can falsify all four inputs. A compromised workstation can modify exports. ST-LINK access can replace firmware. CRC detects accidental corruption, not forgery, replay, or tampering. There is no secure element, persistent boot identity, trusted wall clock, cryptographic attestation, encrypted evidence store, or authenticated update mechanism.

The supplied firmware configures all target signals as inputs. This is not irreversible electrical receive-only enforcement: alternative firmware could configure GPIO outputs. Common ground, input loading, miswiring, shorts, and back-power can disturb the target even without intentional output. Stop immediately if attachment changes behavior. The creator jayis1 makes no safety, regulatory, production-readiness, or security certification claim. Maintain independent protections throughout testing.

## Hardware specifications and bill of materials

The complete instrument uses an external ST NUCLEO-F401RE development board, a passive carrier, short labelled leads, insulating support, and a Linux workstation. The module's STM32F401RET6 Cortex-M4 has 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI oscillator, one-millisecond SysTick, and USART2 at 115200 baud, eight data bits, no parity, and one stop bit. PA2 transmits telemetry to the module's ST-LINK virtual COM bridge. The receiver is disabled.

The Nucleo provides the MCU, regulator, decoupling, USB bridge, reset, and SWD interface. These are external purchased-module functions, not imaginary parts on the custom PCB. The carrier contains no active semiconductor, FPGA, radio, antenna, battery, charger, memory card, analog sensor, level translator, actuator, or target power supply. It is harness-connected and is not a mechanically stackable Arduino shield.

| Reference | Quantity | Specification / purchasing candidate |
| --- | --- | --- |
| External board | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W axial; Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W axial; Yageo MFR-25FBF52-1K |
| J1, J3, J4, J5, J6 | 5 | 1x2 2.54 mm header; Samtec TSW-102-07-G-S |
| J2 | 1 | 1x4 2.54 mm header; Samtec TSW-104-07-G-S |
| Support and leads | 1 set | Insulating bench fixture and labelled harness |

Candidates are not availability guarantees. Check part drawings, lead diameters, body size, and plating before ordering. Resistor footprints use DIN0207 bodies and 7.62 mm pitch. The two-layer carrier is 80 by 70 mm, nominally 1.6 mm thick, with 0.3 mm tracks and a closed outline. There are fourteen placed components, thirty electrical pads, and nine named nets. No mounting holes or enclosure CAD is supplied. An insulating fixture must prevent underside contact and provide strain relief.

### Pin map and electrical limits

| Meaning | Target header signal | MCU header pin | Nucleo label / MCU pin | Bit |
| --- | --- | --- | --- | --- |
| OWNER0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| OWNER1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| OWNER2 | J5.1 | J2.3 | A2 / PA4 | 2 |
| OWNER3 | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only; neither pin supplies power. J2 has four signal pins and no ground pin. Each target signal passes through a 1 kOhm series resistor to its MCU-side node. A 100 kOhm resistor pulls that node to ground. Names ending `_IN` denote target-side nodes; plain OWNER names denote MCU-side nodes. Verify continuity and pin numbering rather than relying on connector orientation or wire color.

Only nominal zero-to-3.3 V common-ground push-pull diagnostic outputs are supported. There is no galvanic isolation, active buffering, field-voltage protection, ESD qualification, surge clamp, fuse, reverse-current blocker, or hot-plug guarantee. A series resistor is not a general overvoltage protection circuit. Five-volt signals, negative voltages, 1.8 V domains, differential buses, relay coils, and unknown industrial wiring are excluded. Keep leads short and verify compatible grounds before connection.

The nominal DC path from an asserted input to ground is 101 kOhm. At 3.3 V, the arithmetic load estimate is about 32.67 microamps per channel, excluding leakage and transient charging. Reserve a conservative 5 V, 500 mA USB supply allowance for the development-board assembly; actual consumption depends on jumpers, ST-LINK, the board revision, and USB limits. Power the observer before applying target signals, and remove target signals before depowering it. Otherwise MCU protection structures can back-power the observer.

ST's [UM1724 Nucleo-64 manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf), Table 16, identifies the F401RE A0–A3 mappings used here. Section 7.10 describes USART2 virtual COM routing: the normal ST-LINK path uses SB13/SB14 on and SB62/SB63 off. Verify the actual module revision and solder bridges before attachment. Peripheral definitions follow STM32F401 RM0368; electrical thresholds must be checked against the STM32F401xD/xE datasheet. Reset entry is required: this firmware does not normalize an arbitrary bootloader clock configuration.

## Architecture and block diagram

```text
Owned target                   Passive carrier             External Nucleo
 OWNER0 ---------------------- 1k --+--------------------- PA0 / A0
 OWNER1 ---------------------- 1k --+--------------------- PA1 / A1
 OWNER2 ---------------------- 1k --+--------------------- PA4 / A2
 OWNER3 ---------------------- 1k --+--------------------- PB0 / A3
                                   | each node
                                  100k
                                   |
 common ground ------------------- GND ------------------ GND / USB

GPIOA then GPIOB -> independent qualification -> known-high collision check
  -> all-zero baseline -> singleton identity -> directed transition matrix
  -> cyclic-distance classes -> transactional cumulative evidence
  -> checked BO1 snapshots -> USART2 TX -> ST-LINK -> workstation USB
  -> Ledger / History / Integrity screens -> explicit private JSON export
```

GPIOA and GPIOB are sampled consecutively, not latched simultaneously. Relative port-read skew and input qualification can change apparent edge ordering. This is not a metastability detector or high-speed bus analyzer. Begin with diagnostic plateaus and idle windows of at least twenty milliseconds, then compare the complete observation chain with an independent logic analyzer. Events shorter than the acquisition/qualification envelope can be missed. A clean matrix is never proof that no physical violation occurred.

## Firmware measurement contract

The firmware is C11 with explicit startup vectors, linker layout, GPIO and watchdog drivers, bounded UART service, the order engine, and a checked serializer. It uses no heap, operating system, filesystem, flash log, or received-command channel. Flash starts at 0x08000000. The linker reserves stack headroom. All evidence is volatile and disappears on observer reset. The reset-register snapshot is transmitted but is not a unique boot identifier.

Each channel independently qualifies after its candidate level remains unchanged for three subsequent one-millisecond observations. A newly observed candidate therefore needs its first sample plus three matching intervals. Once qualified, its previous qualified level remains visible while another candidate settles. This measures qualified indications rather than instantaneous electrical levels. A sampled one-tick low excursion can be rejected and does not necessarily constitute release and readmission.

At startup, phase is unknown. A fully qualified all-zero vector establishes readiness. The first subsequent singleton becomes an anchor: it increments anchors and records its owner but does not create a transfer. The starting identity is unconstrained; a baseline cannot prove which owner should run first. While ready, a change from one singleton to another records one directed transfer, even if no all-zero sample occurred between them. Break-before-make dead time is not this device's policy.

A held singleton creates no further admission. A fully qualified zero interval records idle while retaining the previous owner. When a singleton returns after idle, an admission occurs even if it has the same identity. Thus 0-owner, idle, 0-owner is a repeated admission, whereas continuously held 0-owner is not. Idle has no maximum duration and does not erase the expected cyclic successor. There is no timeout-based starvation claim.

For a transfer from owner i to owner j, matrix cell `4*i+j` increments. Cyclic distance `(j+4-i)%4` gives the classification: zero is repeat, one is forward, two is skip, and three is reverse. A reverse step in this four-owner system is also a forward distance of three; the name describes the declared strict cycle, not physical direction. Every classified transfer updates both a matrix cell and exactly one aggregate. A violation does not halt future classification: its destination becomes the reference for the next transfer. The matrix preserves which relation violated policy rather than only a generic alarm total.

Two or more independently qualified high inputs create a collision episode even if an unrelated channel has not yet qualified. This prevents unrelated chatter from hiding known multiple ownership. Collision sets a sticky finding, increments collisions once per observed multi-high segment, and loses sequence phase. Moving directly between different multi-high masks remains the same collision episode. Returning to a singleton clears the current collision indicator but cannot restore phase; an all-zero baseline and a new anchor are required. Consequently no transfer is fabricated across ambiguous ownership.

Repeated calls with the same timestamp do nothing. A nonzero timestamp delta other than one records an acquisition gap and the number of skipped nominal ticks. It invalidates all qualification, clears current phase and collision state, and preserves historical counts. A continuing collision after fresh qualification may count as another observed segment. Collisions therefore count observed episodes, not guaranteed unique physical incidents. Ordinary 32-bit tick wrap is supported; an entire unseen timer cycle cannot be detected from the modulo clock alone.

Counter updates are transactional. If any updated counter would overflow, the entire speculative sample is discarded and prior evidence enters terminal HALTED with exhaustion set. Last owner becomes the sentinel four, readiness and qualification clear, and established totals remain unchanged. Watchdog initialization failure also enters HALTED before the first status. Acquisition fields then freeze; timestamp and saturating transmission-failure diagnostics may continue. The serializer rejects contradictory matrix totals, missing anchors, impossible flags, and invalid state combinations instead of emitting them as plausible evidence.

## Protocol and companion software

The main loop samples before servicing UART. Each UART service writes at most one ready byte, so a stalled transmitter cannot impose an unbounded busy wait. A frame submission copies into fixed storage; transmission exceeding one hundred milliseconds is abandoned and counted. Boot is announced once. Policy repeats approximately every second, allowing late attachment without treating a policy announcement as reset. Status snapshots are offered approximately every hundred milliseconds. These are cumulative observations, not an event-complete trace.

Frames are ASCII `BO1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection, and no final XOR over bytes preceding the asterisk. The checksum has four uppercase hexadecimal digits. B carries the reset-register value. P carries `3,1,4,16`: qualification intervals, nominal sample period, owner count, and matrix-cell count. Other policies do not establish compatibility.

S fields are `now,levels,valid,halted,faults,observed,gaps,missed,anchors,forward,skip,reverse,repeat,collisions,last,ready,idle,collision_active`, followed by sixteen row-major matrix cells, then `tx_fail`. All numbers fit unsigned 32 bits. Fault bits zero through six mean repeat, skip, reverse, collision, gap, exhaustion, and watchdog unavailable. The app names the cells m0 through m15. Observed counts fully qualified sample vectors, not admissions or guaranteed wall-time coverage. Anchors and transfers must not be confused with completed complete rotations.

Run `python3 app/console.py` on Linux with Tk installed. Enter the exact serial path, normally `/dev/ttyACM0`, and choose New live. The Ledger screen shows counters, named findings, and the directed matrix. History retains up to 256 snapshots. Integrity displays source, live/replay provenance, compatibility, connection state, evidence age, parser errors, reset flags, and any freeze reason. Disconnect retains evidence. A successful new connection or replay replaces the session; export important evidence first. Failed connection setup preserves the old model.

The application opens serial read-only and sends no target command. LIVE requires compatible connected telemetry, at least two advancing timestamps, and recent progress. Duplicate timestamps do not refresh freshness. Contradictory duplicates, backwards time, decreasing counters, disappearing sticky flags, changes after terminal halt, and impossible counter growth freeze evidence. Prolonged reception pauses and cumulative host/device time divergence also freeze the session, preventing queued stale records from appearing current. These are necessary consistency checks, not cryptographic authentication or an exhaustive proof that every accepted stream is realizable.

The cumulative host/device divergence allowance is 0.5 seconds plus five percent of session elapsed time, with that added allowance capped at 0.5 seconds. Thus the total budget never exceeds one second. This deliberately trades long-running oscillator tolerance for bounded accumulated lag: an honest but drifting HSI clock can eventually require reconnect. The observer cannot distinguish clock error from serial buffering, nor detect an initial fixed delay before the first accepted snapshot. LIVE means progressing transport under these bounds, not authenticated real-time delivery.

Replay is explicitly non-live. Only regular files no larger than two MiB are accepted; files are opened nonblocking and checked before allocation, so selecting a FIFO cannot block the interface. Framing storage is bounded and resynchronizes on the complete BO1 marker. A partial tail at EOF increments errors. A recording without any compatible accepted status is rejected. The test fixture is generated by the actual C engine and serializer, but remains synthetic software evidence, not a physical capture.

JSON export creates a new file with mode 0600, refuses overwrite, and rejects a final-path symlink. Reports retain author jayis1, provenance, connection and freshness state, parser errors, reset information, freeze reasons, the latest snapshot, and bounded history. Even an empty report retains session metadata. Permissions do not encrypt the file. Use a trusted destination, control access under the engagement agreement, and do not equate ordinary deletion with secure erasure.

## Building and verification

Required tools are GNU Make, ARM GNU GCC with newlib/nosys support, native GCC, Python 3 with Tk, KiCad 9 CLI/Python bindings and standard footprints, plus Xvfb for unattended GUI tests. No third-party Python package, mobile SDK, cloud service, or runtime network dependency is required. From this device directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
```

Outputs include firmware/build/baton-order.elf and baton-order.bin. Programming requires supervised ST-LINK access followed by reset; no programming was performed for this release. Host tests exercise every directed transfer and input mask, repeated admissions, ambiguous ownership, baseline recovery, qualification, acquisition gaps, tick wrap, transactional exhaustion, and a deterministic 10,000-turn matrix oracle. UART tests model registers in a Linux process. Scheduler tests execute the production main loop with modeled board functions, including lost boot transmission and watchdog startup failure.

Application tests consume real C-encoder output through the parser and through Linux pseudo-terminals using the actual Tk event loop. They exercise three screens, paced live reception, hangup, retained provenance, replay, private export, overwrite refusal, replay bounds, malformed framing, checksum-valid impossible records, duplicate timestamps, rollback, and stale/backlogged transport. They establish software behavior, not USB compatibility with a physical Nucleo.

Hardware checks run ERC and DRC with violations fatal, export a fresh schematic netlist, and compare reference, value, footprint identity, pin number, and net across the schematic, PCB pads, and saved manifest. The PCB is routed, not an unrouted placement sketch. Passive-symbol ERC cannot validate the external power source, determine acceptable target voltages, or certify electrical safety. Review rendered labels and purchased component dimensions before fabrication.

## Research workflows and remaining physical acceptance

For a vendor assessment, first document why a strict four-turn cycle is required. Record the mapping between logical owners, diagnostic firmware, and connector labels. Establish an all-zero baseline, run an approved known-good sequence, and export it. Then run the suspect workload without changing the mapping. Compare matrix cells rather than just forward totals. A repeated admission can identify a retry path worth reviewing; a skipped turn can identify omitted audit or sanitization scheduling. Neither alone establishes data disclosure or arbitrary execution.

For red-team reporting, retain written authorization and explain exactly which diagnostic guarantee failed. Include adapter source, target firmware identity, observer build identity, wiring, sample limitations, acquisition gaps, and the evidence export. A penetration tester can use the result to request a targeted vendor explanation without claiming that an indication proves the underlying resource operation occurred. A clean run is limited to the observed workload and acquisition envelope.

Before connecting a target, check the assembled board unpowered for continuity and shorts. Bring up the Nucleo alone, verify input configuration, inspect power sequencing, measure loading and ground offset, and compare paced known-good, repeat, skip, reverse, and collision patterns against an independent analyzer. Exercise reconnect during ownership, reset while an owner is held, an acquisition interruption, and every connector permutation used in the procedure. Keep consequential outputs disconnected during initial experiments.

Physical thresholds, cable loading, oscillator accuracy, GPIO read skew, watchdog recovery, USB behavior, back-power, ESD, EMC, thermal limits, enclosure suitability, and regulatory behavior remain unverified. Passing software and CAD checks does not authorize deployment or fabrication without review. Original design, firmware, application, tests, and documentation author: jayis1.
