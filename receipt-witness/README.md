# Receipt Witness — Ordered Audit-Receipt and Backpressure Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol RW1. Firmware and companion application: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design, with target-linked STM32 firmware, a functional Linux desktop application, and routed native KiCad carrier files. No physical assembly, programming, electrical qualification, or timing calibration has been performed. This instrument observes diagnostic indications; it neither creates durable audit storage nor proves that an underlying audit record exists.

## Purpose and repository-local originality

Receipt Witness investigates a security failure that can remain invisible in ordinary application logs: a protected operation completes, but the asynchronous audit path loses or indefinitely postpones its receipt. Logging APIs often acknowledge enqueueing long before storage becomes durable. A queue-full flag may exist without actually preventing protected operations. An independent observer can distinguish these milestones and preserve evidence when the target's own logging subsystem is the component under test.

The instrument listens to four owner-provided, slow diagnostic outputs: ACTION, APPEND, DURABLE, and BLOCKED. It correlates actions with append acknowledgments and durable receipts in first-in, first-out order, using a bounded queue of eight operations. It checks a fifty-millisecond append deadline, a two-hundred-millisecond durability deadline measured from append, orphan acknowledgments, outstanding-operation overflow, and actions observed during blocked admission. The result is a compact ledger of admitted actions, correlated appends, completed receipts, abandoned correlations, maximum accepted latencies, and sticky findings. It never reads the log contents, extracts secrets, sends a target command, or changes admission policy.

Repository directories and existing device descriptions were inspected before selecting this concept. Grant Ledger accounts for consumption of single-use authorization; Erase Witness examines sanitization ordering; Epoch Witness examines firmware-generation monotonicity; Retry Ledger measures authentication retry budgets. Those designs do not implement an outstanding audit-receipt FIFO with separate enqueue and durability deadlines. Receipt Witness deliberately reuses jayis1's passive four-input carrier, register definitions, startup code, board driver, and bounded UART pattern. Its new contribution is the audit-conservation policy and evidence interpretation, not a claim that the resistor network itself is electrically novel. Originality is relative to this repository, not a worldwide prior-art or patentability claim.

## Authorized use and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Authorization must cover the exact test points, diagnostic firmware instrumentation, induced test conditions, and capture retention. Keep the bench instrument visible and labelled. A general penetration-testing agreement does not authorize arbitrary physical attachment or changes to a production logging service. Never attach this unisolated carrier to occupied access-control equipment, medical machinery, vehicle controls, mains-connected test points, high-energy equipment, or a live industrial bus.

The observer is receive-only, but physical attachment can still cause disruption through ground loops, electrical loading, miswiring, or back-power. Start with a sacrificial development fixture and disconnect hazardous outputs. Stop if attaching the instrument changes the target's behavior. Findings require corroboration with target source review, a logic analyzer, and the owner's logging contract. A missing receipt indication is not automatically an exploitable vulnerability, and a clean capture is not certification of complete or tamper-resistant auditing. The author, jayis1, does not authorize unauthorized access, interference, or covert collection.

## Threat model and trust boundaries

The assessed boundary is between a protected operation's reported completion, the logger's reported acceptance, and its reported durable completion. Relevant implementation defects include dropping audit entries under load, acknowledging durability prematurely, continuing protected operations while admission is blocked, duplicating completion indications, and losing outstanding work during a logging restart. An authorized lab test may exercise normal queue-pressure and recovery paths on an isolated target. This device observes the resulting indications; it does not generate the workload or induce a fault.

The diagnostic adapter must emit exactly one APPEND and one DURABLE pulse per ACTION, in action order. Batched flush completion must be expanded by the target adapter into one appropriately spaced receipt per action. An arbitrary storage flush strobe cannot be connected directly and interpreted as one receipt. Out-of-order acknowledgments and multiple simultaneous actions are unsupported. There are no transaction identifiers on the four-wire interface; the observer cannot detect deliberate substitution of one operation's receipt for another. A malicious target can lie on all four wires. The meaning of each diagnostic output must therefore be reviewed independently.

The workstation, harness, instrument image, development board, and operator are trusted. Captured bytes and replay files are untrusted parser inputs. CRC detects accidental corruption but does not authenticate a sender, prevent replay, or sign an export. There is no secure element, persistent boot identifier, signed updater, cryptographic attestation, or tamper-proof clock. ST-LINK programming and serial permissions are local physical trust boundaries. No network service, telemetry upload, radio, credential store, or target-side receive command exists. A compromised workstation can modify reports regardless of private file permissions.

## Hardware specifications and bill of materials

The complete assembly uses an external ST NUCLEO-F401RE development board, a passive through-hole carrier, a labelled harness, and a Linux workstation. The STM32F401RET6 provides a Cortex-M4 core, 512 KiB flash, and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock without a PLL and configures a one-millisecond SysTick. USART2 TX runs at 115200 baud, eight data bits, no parity, one stop bit. The Nucleo's ST-LINK supplies programming and USB virtual serial. MCU regulation, decoupling, reset, SWD, and USB circuitry are provided by that separately purchased module, not omitted fictional components on the carrier.

The carrier measures 80 by 70 mm, with two copper layers and nominal 1.6 mm board thickness. It has fourteen components, thirty electrical pads, nine named nets, a closed outline, 0.3 mm tracks, and a ground-layer transition via. There are no batteries, charging circuits, antennas, active sensors, isolation components, or target power outputs. Use an insulating support and strain relief; mounting holes are not provided. This is a cable-connected carrier, not a mechanically compatible stacking shield.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kΩ, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm through-hole header |
| J2 | 1 | 1x4 vertical 2.54 mm through-hole header |
| Harness and support | 1 set | Insulated labelled leads and nonconductive carrier support |

The axial resistor footprint has 7.62 mm lead spacing. Match purchased resistor dimensions and connector mating height against the footprints before ordering. No stock, vendor availability, or price claim is made. Substitutions must preserve resistance, tolerance, lead geometry, and the supported voltage domain.

J1 pins one and two are both ground; neither is a supply connection. Connect it to Nucleo ground. J2 pins one through four go to Nucleo CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0. J3 pin one is ACTION_IN, J4 pin one APPEND_IN, J5 pin one DURABLE_IN, and J6 pin one BLOCKED_IN. Pin two on each target connector is ground. Unsuffixed net names identify MCU-side nodes after the series resistors. Verify connector numbering and harness continuity before power-on; silkscreen does not replace a continuity test.

Only push-pull zero-to-3.3 V diagnostic signals sharing a verified common ground are supported. Each channel presents approximately 101 kΩ to ground through the resistor pair. The 1 kΩ series resistor does not establish 5 V tolerance, transient immunity, or safe negative-voltage operation. Dedicated ESD clamps, galvanic isolation, reverse-polarity protection, and overvoltage disconnection are absent. Do not connect unknown field wiring, relay coils, differential buses, or signals with an unverified reference potential.

Power the Nucleo through its documented ST-LINK USB connection, with its power-selection jumpers checked against the board manual. Reserve a conservative 5 V, 500 mA workstation or bench supply allocation; that is a planning allowance, not measured current or USB certification. Disconnect the target leads before depowering the observer to avoid input-diode back-power. Ground potential, startup current, signal rise time under load, thermal behavior, and ESD resilience require physical measurements. The passive board consumes no regulated supply, but its resistors and shared ground still load the target.

## Architecture and block diagram

```text
Owned target diagnostic outputs         Passive carrier         External NUCLEO
 ACTION  ------------------------------ 1k --+----------------- PA0 / A0
 APPEND  ------------------------------ 1k --+----------------- PA1 / A1
 DURABLE ------------------------------ 1k --+----------------- PA4 / A2
 BLOCKED ------------------------------ 1k --+----------------- PB0 / A3
                                            | each has 100k to ground
 Ground ----------------------------------- GND --------------- GND

GPIO samples -> independent qualification -> ordered pending FIFO
                                        -> deadline and blocked checks
                                        -> cumulative evidence ledger
                                        -> RW1 CRC encoder -> USART2 PA2 TX
                                        -> ST-LINK USB -> Linux desktop
                                                       Ledger / History / Integrity
```

PA0, PA1, and PA4 are read from GPIOA; PB0 is read from GPIOB immediately afterward. These are not simultaneous hardware latches. The design does not infer processor-speed edge ordering or capture short pulses. It is intentionally suitable for a slow diagnostic adapter whose indications are stretched and serialized. A logic analyzer remains necessary to qualify that adapter and investigate boundary races.

## Firmware behavior and design decisions

All four inputs are independently qualified. A candidate change is first observed, then must remain unchanged for three subsequent sampled milliseconds before the qualified level changes. Unrelated chatter cannot indefinitely defer another channel's qualification. Pulses shorter than this interval may be rejected or missed; an absent count does not prove the underlying event never happened. Use comfortably separated diagnostic pulses, for example ten milliseconds high and ten milliseconds low, while respecting the policy deadlines.

Startup and correlation loss enter an unready state. A new observation segment requires all raw and qualified inputs to be low, with all four channels known, for twenty evaluated ticks. Any activity interrupts the idle baseline. Activity on ACTION, APPEND, or DURABLE while unready records a sticky outside-segment finding; it does not fabricate prior FIFO state. BLOCKED must also become low before a segment begins. This is a prospective baseline, not evidence about operations before attachment. Arrange it deliberately in the target fixture.

Within a ready segment, a qualified rising ACTION increments the operation count and creates one pending FIFO entry. The raw or qualified BLOCKED level at that evaluated edge records operation-while-blocked, but does not discard the operation: the app can still show whether its receipt subsequently arrived. Holding ACTION high does not create additional operations. A new action that exceeds eight outstanding operations invalidates correlation, abandons every outstanding operation including the new one, and requires another baseline.

A qualified rising APPEND marks the oldest pending entry that has not yet been appended. Its age must be no greater than fifty sampled milliseconds from the action. ACTION and APPEND qualifying together are permitted and create a zero-sampled-delay append. This is a diagnostic convention, not proof of physical simultaneity or a measured zero storage delay. APPEND without an eligible operation records an orphan, abandons outstanding correlation, and returns to baseline acquisition.

A qualified rising DURABLE consumes only the oldest pending entry, which must already have an append from a previous evaluated tick. DURABLE is processed before the current tick's action and append, so it cannot acknowledge a new append on the same tick. It can consume an older appended operation while a new action and append replenish the FIFO. Its append age must be no greater than two hundred sampled milliseconds. A receipt cannot skip an older unappended action. Every pulse acknowledges exactly one operation; a single level cannot acknowledge a batch.

Ages advance before edge processing. The append deadline is inclusive at fifty and the durability deadline inclusive at two hundred; age fifty-one or two-hundred-one is late. Any overdue pending entry invalidates the whole correlation segment, because later acknowledgments cannot be matched reliably after a missing deadline. Pending operations become abandoned, rather than retrospectively successful if a late receipt arrives. An ACTION observed on the same evaluated tick as a timeout or orphan durable indication is still counted and abandoned, preserving the conservation ledger. Its blocked finding is preserved as well. The firmware does not claim to count all activity after the segment becomes unready.

Sampling discontinuity is explicit. Any tick delta other than one records a gap, accumulates missing ticks when representable, abandons pending operations, resets input qualification, and requires a fresh baseline. It never treats an unobserved period as continuous idle or deadline compliance. Qualified levels remain available as historical status but are not eligible for correlation until reacquired. Time wraps modulo 32 bits; elapsed entry ages and cumulative counters saturate instead of wrapping. Tests cover ordinary timer wrap and arithmetic saturation.

Before saturation, the main conservation equation is actions = receipts + abandoned + pending. Appended totals include appends associated with subsequently abandoned operations. Consequently appended is not simply receipts plus currently appended entries; the unexplained difference must fall between zero and abandoned. Maximum latency fields describe only successfully accepted appends or receipts, not the worst physical delay including timeouts. Sticky faults survive recovery segments, and a segment counter distinguishes renewed baselines. Reboot loses all RAM evidence. Keep the observer powered throughout a target campaign and export before intentionally restarting it.

The UART driver copies a bounded frame, advances only when TXE is available, and abandons stalled output after one hundred milliseconds. Main-loop acquisition never waits for the serial transmitter. The watchdog is initialized with bounded waits and fed from the loop. A failure to initialize it suppresses measurement output. A boot record is sent once, policy repeats approximately every second, and cumulative status repeats approximately every hundred milliseconds. Loss of an intermediate status does not erase cumulative counts, but transitions between statuses cannot be reconstructed into precise individual event times.

## Application interface and protocol

The Linux companion is a standard-library Python/Tk desktop application. Launch `python3 app/audit.py` from this directory. Enter the exact serial terminal, commonly `/dev/ttyACM0`, and choose New live session. Serial is opened read-only and no commands are sent to the instrument. The first screen shows the ledger and named findings. History retains at most 256 accepted status rows. Integrity shows provenance, connection, policy compatibility, parser errors, data age, stale state, reset flags, and any reason evidence has been frozen.

Disconnect retains evidence and its live provenance. Starting another successful live connection or loading a replay replaces the model, so export first. Replay accepts only an opened regular file no larger than one MiB; FIFO and device-file reads are rejected without blocking. JSON export creates a new file with mode 0600 and refuses to overwrite an existing file. Export is an explicit operator action and includes the same integrity qualifications as the display. The application has no automatic cloud sync or background retention. Filesystem permissions are not encryption or secure erasure.

Frames are ASCII `RW1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC is four uppercase hexadecimal digits using CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xffff, no reflection or final XOR, covering bytes preceding the asterisk. B contains one reset-flags field. P contains `8,50,200,3,20`, describing capacity, append deadline, durability deadline, qualification interval, and idle baseline. S contains time, ready, levels, count, waiting, faults, actions, appended, receipts, abandoned, gaps, missed, segments, max_append, max_durable, and tx_fail in that order. Integers are unsigned 32-bit. Levels bits zero through three are ACTION, APPEND, DURABLE, and BLOCKED.

Fault bits zero through seven are blocked action, orphan append, orphan durable receipt, late append, late durability, FIFO overflow, acquisition gap, and activity outside an established segment. Waiting is the number of outstanding operations that have not yet appended. Count includes all outstanding operations, whether appended or not. Ready means correlation is established; it does not mean the run has no findings. A previously sticky fault remains visible through later successful work.

The parser checks CRC, field count, integer ranges, queue bounds, accepted latency bounds, and conservation equations before displaying a row. It freezes evidence on counter regression, disappearing sticky faults, backwards or ambiguous device time, contradictory duplicate timestamps, incompatible policy, or a boot record following evidence. Identical duplicate status does not refresh freshness. Data becomes stale after one second without an advancing accepted status. A valid periodic policy permits late attachment but never unfreezes evidence. Without persistent boot identity some resets can remain indistinguishable from forward traffic; the app does not claim perfect restart detection. Serial hangup closes the connection while preserving the model. Drivers that retain readable bytes at hangup are drained first; the operating system can still discard unconsumed bytes on unplug.

## Build, tests, and reproducibility

From this directory run `make -C firmware all test`. The build uses Arm GNU GCC, Cortex-M4 Thumb soft-float settings, an explicit linker script, and no downloaded MCU SDK. It emits an ELF, map, and raw binary below `firmware/build/`. Host tests exercise the production policy, encoder, and UART driver with sanitizers. Linux memory-mapped UART tests validate software behavior only, not silicon timing. Run `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v` for parser, export, replay-boundary, and real Tk event-loop tests using pseudo-terminals and a fixture emitted by the actual C encoder. Fixtures are synthetic, not laboratory captures.

Regenerate CAD with `/usr/bin/python3 tools/generate_hardware.py`, using the installed KiCad pcbnew module and standard footprint libraries. Then run `/usr/bin/python3 tools/check_hardware.py`. The checker executes schematic ERC and board DRC, exports a fresh XML netlist, and compares every reference, pin, value, full footprint library identity, and net against the PCB and manifest. The saved netlist is checked independently against the regenerated export. Passing these checks establishes internal CAD consistency, not electrical safety, mechanical fit, manufacturability approval, or the truthfulness of the external target.

See VERIFICATION.md for the actual tool versions and outcomes from this release. Required laboratory work remains: inspect the harness, check continuity and grounds, verify voltage domains with a meter, program a reviewed image using ST-LINK, verify oscillator and UART timing, exercise known good and bad diagnostic sequences, measure loading and back-power, and compare the displayed ledger with an independent analyzer. Do not order or deploy the board based solely on software tests.

## Practical research campaigns

A product-security engineer can instrument a development logger to distinguish queue admission from storage completion, then run a normal burst workload under explicit test authorization. The instrument reveals whether backpressure actually gates protected operations or only sets a cosmetic flag. A red team working with the product owner can use the same evidence to assess audit-loss risk during an approved maintenance or recovery exercise, without collecting identities or audit payloads. A penetration tester can attach the bench to a vendor-provided test build and document the exact logging guarantees that failed, rather than extrapolating from a successful API return.

For each campaign record the target image, adapter implementation, pulse mapping, observer firmware identity, harness revision, expected operation order, and acceptance thresholds. Begin with a known idle segment and a single action/append/receipt sequence. Confirm the counter meaning before increasing load. Include an intentionally missing append, delayed durable pulse, blocked action, duplicate acknowledgment, and observer disconnect in the fixture's validation plan. Export evidence before starting another session. Retain only what the engagement requires, and describe any sampling gap or ambiguous reset prominently in the report.

The instrument's usefulness comes from separating claims: a target said an operation occurred, then said it entered the audit path, then said it became durable. It can expose contradictions in those statements under a declared temporal contract. It cannot prove storage persistence after power loss, prevent malicious deletion, validate log contents, establish receipt authenticity, or replace a full audit-system security review. These limitations are architectural and remain true even when every included software and CAD test passes.
