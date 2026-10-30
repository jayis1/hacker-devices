# Prefix Retire — Out-of-Order Completion / In-Order Visibility Witness

Author and creator: jayis1. Hardware revision A. Wire protocol PR1.
Firmware, application, and tests: GPL-2.0-only. Hardware tools: MIT. Passive carrier: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with executable C firmware, a Linux desktop companion, and native routed KiCad carrier files. See VERIFICATION.md for checks actually executed. Fabrication, programming a physical MCU, electrical qualification, calibrated timing, and deployment have not been performed. This is an observation instrument, not an ordering controller, trusted execution monitor, safety interlock, or certificate of security.

## Purpose and overview

Prefix Retire observes an integration boundary that ordinary completion counters cannot express: independent work may finish in arbitrary order, but externally visible results must retire in a predetermined order. A target development adapter exposes a two-bit ordinal, a READY strobe, and a RETIRE strobe. Four operations form a batch. READY notifications can arrive in any permutation and may be interleaved with retirement. RETIRE must name ordinal zero, then one, then two, then three, and each ordinal must already have a recorded readiness notification. The instrument retains readiness even after retirement until the entire batch finishes.

A motivating example is an embedded security gateway that verifies four ordered policy updates in parallel. Update three can finish verification before update zero. That is not itself a defect. Making update three externally visible before zero, however, can expose a configuration state that the ordered policy log never authorized. A separate observer can distinguish legitimate parallel completion from premature visibility. The device does not read policy contents, validate signatures, intercept a bus, change a target configuration, or execute an exploit.

Consider READY notifications for three, one, zero, and two, followed by RETIRE notifications for zero, one, two, and three. This is a successful batch. READY three followed by RETIRE three is a non-prefix retirement, even though three is ready. RETIRE zero without READY zero is a readiness failure. READY zero, RETIRE zero, then another READY zero is a duplicate within the same batch, not reuse of a freed slot. Only after the accepted retirement of three does the next batch start with empty readiness and next ordinal zero.

The practical value is a small independent ledger that survives misleading target log summaries. A log saying four tasks completed and four results were published does not establish either their correspondence or their publication order. The instrument counts accepted readiness, accepted retirement, rejected observations, completed batches, and evidence loss separately. Its deliberately small state space supports exhaustive order experiments and comprehensible remediation retests.

## Originality and comparison with existing devices

Repository directories and design titles were inventoried before selecting this concept. The closest contracts inspected were Slot Alias, Replay Sieve, Receipt Witness, and Epoch Witness. Slot Alias tracks exclusive resource occupancy and explicitly permits arbitrary release order. Prefix Retire permits arbitrary readiness order but requires ordered visibility, and it does not make an already retired ordinal reusable within its batch. Replay Sieve detects duplicate identities within externally delimited sessions but does not track an ordered retirement prefix. Receipt Witness associates anonymous FIFO acknowledgments with action order and does not permit tagged out-of-order completion. Epoch Witness compares security generations rather than completion and retirement of concurrent batch members.

The distinctive contribution is this two-stage, tagged, prefix-retirement measurement contract, its invariants, and its regression experiments. The resistor carrier, STM32 register definitions, startup, UART transport, checksum pattern, and desktop transport intentionally reuse existing work by jayis1. Reuse is identified rather than presented as a novel electrical circuit. Originality is repository-relative; no worldwide prior-art search, patentability assessment, or claim of inventing reorder buffers is made. The folder is self-contained and does not import sibling projects at runtime.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Authorization must cover diagnostic firmware, physical attachment, workload generation, possible service interruptions, and evidence retention. A general application penetration-testing agreement is not permission to attach an unisolated circuit to an appliance. Begin on a low-energy development bench with hazardous outputs disconnected and visible, labelled leads. Never connect to deployed door controls, medical equipment, vehicles in service, industrial protection systems, mains-related wiring, or unknown field signals.

The assessed attack surface is the integration between asynchronous work completion and the gate that exposes ordered results. Relevant defects include publishing a later result first, publishing before verification completes, duplicate completion delivery, misassigned ordinal tags, a lost notification, and accidental batch-state reset. An authorized red team can ask an owner to expose a suspected reordering race using ordinary test workloads. Penetration testers can compare before-and-after traces for a fix. Security researchers can evaluate an implementation's declared ordering contract without receiving secrets or modifying its traffic.

The diagnostic adapter must bind READY to the agreed internal completion event and RETIRE to the actual visibility boundary. A UI status variable is not automatically a reliable proxy for a hardware access gate. A malicious target can lie on every wire; the observer cannot prove signature verification, durable storage, actual instruction execution, or absence of secret-dependent effects. Corroborate findings with adapter source review, independent waveform capture, and an application-level functional check. A low count of faults is not a compliance certificate.

The workstation, firmware image, external MCU board, wiring, programming interface, and operator are trusted. Serial frames and replay files are untrusted parser input. CRC detects accidental corruption, not intentional forgery. There is no secure element, authenticated pairing, signed log, trusted wall clock, encrypted export, secure updater, persistent boot identifier, or network listener. Protect ST-LINK access and serial permissions. No credential collection, radio transmission, autonomous attack mode, hidden implant behavior, or target command channel is implemented.

Physical attachment can disturb a target through loading, common-ground faults, or back-power even when firmware is receive-only. Stop if attachment changes behavior. A disconnected input can look low because of its pulldown; the circuit cannot distinguish that from a legitimate inactive line. Data minimization limits records to ordinals, masks, counts, and integrity information, but serial paths and report filenames can still identify lab assets. Store reports privately under the engagement's retention policy. Deleting a file does not guarantee secure erasure.

## Hardware specifications and bill of materials

The compute module is an external ST NUCLEO-F401RE, MB1136 family, carrying an STM32F401RET6 in LQFP64. The Cortex-M4 provides 512 KiB flash and 96 KiB SRAM. Firmware uses the nominal reset-default 16 MHz HSI clock, one-millisecond SysTick, and USART2 at 115200 baud, eight data bits, no parity, one stop bit. There is no RTOS, vendor SDK, PLL setup, heap allocation, or floating-point requirement. The external module provides supply regulation, decoupling, reset, SWD programming, and ST-LINK USB virtual serial facilities.

The carrier is an 80 by 70 mm, two-layer, nominal 1.6 mm FR-4 board. Fourteen through-hole components connect nine named nets through thirty electrical pads. Tracks are 0.3 mm wide and a through via connects a ground-layer transition. A closed rectangular outline is provided. There are no mounting holes or enclosure files; supply insulating support and strain relief. This board connects through harnesses rather than stacking as a pin-compatible shield.

| References | Quantity | Part specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical through-hole header, 2.54 mm pitch |
| J2 | 1 | 1x4 vertical through-hole header, 2.54 mm pitch |
| Harness and support | 1 set | Short labelled insulated leads and nonconductive support |

Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Check actual purchased dimensions, mating connector access, drill fit, and pin-one orientation before fabrication. Availability, pricing, assembly fit, and manufacturing yield are not verified. There are no radios, FPGA, battery, charger, analog sensors, level translators, active protection components, or target power outputs. The carrier does not replace the Nucleo development module.

## Pin map, voltage domain, and power

| Function | Target connector | MCU harness | Nucleo label / MCU | Logical bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3 pin 1 | J2 pin 1 | CN8 A0 / PA0 | 0 |
| TAG1 | J4 pin 1 | J2 pin 2 | CN8 A1 / PA1 | 1 |
| READY | J5 pin 1 | J2 pin 3 | CN8 A2 / PA4 | 2 |
| RETIRE | J6 pin 1 | J2 pin 4 | CN8 A3 / PB0 | 3 |
| Ground | J3–J6 pin 2 | J1 both pins | Nucleo GND | none |

J1 is ground only; neither pin accepts a supply. J2 carries four signals and no power or ground. Net names ending `_IN` refer to the target side of a series resistor, while unsuffixed names refer to the MCU-side node and pulldown. Verify continuity and connector orientation rather than trusting cable color.

Supported inputs are known common-ground push-pull zero-to-3.3 V logic only. The nominal resistor path to ground is 101 kOhm per channel before leakage and source impedance. The 1 kOhm resistor is not a qualified overvoltage barrier. There is no galvanic isolation, dedicated ESD clamp, fuse, polarity protection, hot-plug guarantee, or reverse-current blocker. Five-volt, negative, differential, industrial, relay-coil, and unverified ground-domain signals are excluded. A separately qualified front end is required for other electrical environments.

Power the Nucleo through its documented ST-LINK USB connector after checking jumper settings and board revision. Reserve a conservative 5 V, 500 mA source budget for the development-board assembly; this is not measured consumption or a USB-compliance result. Power the observer before enabling target signals and disconnect target signals before removing observer power. MCU input protection structures can otherwise back-power an unpowered board. USB ground is connected to target ground, so verify reference compatibility first.

## Architecture and block diagram

```text
Owned target adapter              Passive carrier             External Nucleo
TAG0 ----------------------------- 1k --+---------------------- PA0 / A0
TAG1 ----------------------------- 1k --+---------------------- PA1 / A1
READY ---------------------------- 1k --+---------------------- PA4 / A2
RETIRE --------------------------- 1k --+---------------------- PB0 / A3
                                        | each MCU-side node
                                       100k
                                        |
Common ground ------------------------- GND ------------------- GND / USB

GPIO -> continuity guard -> bundled ordinal + independent strobe qualifiers
     -> prior setup and overlap checks -> readiness bitmap + next ordinal
     -> transactional counters -> bounded PR1 encoder -> USART2 PA2 TX
     -> ST-LINK USB -> Linux Ledger / History / Integrity -> private JSON
```

GPIOA is sampled before GPIOB, not with an atomic cross-port latch. The design cannot resolve processor-speed races, arbitrarily short pulses, or physical edge order within one sampling interval. The target must expose deliberately slow diagnostic notifications. Initial tests should provide twenty milliseconds of tag setup and twenty milliseconds each of strobe-high and strobe-low intervals. Verify actual waveforms independently; software qualification does not calibrate oscillator tolerance, interrupt latency, wiring delay, or inter-port skew.

## Exact measurement contract and firmware decisions

Each batch names exactly four ordered operations, zero through three. Tags are ordinals in that batch, not the low bits of arbitrary IDs, hashes, reusable resource slots, or monotonically wrapping security generations. Every batch must produce one READY and one RETIRE for each ordinal. The next batch may not overlap the current batch. Arbitrary tagged completion is supported within a batch; unlimited outstanding work and cross-batch pipelining are not.

The operator must establish the real beginning of a target batch before restarting the observer. Startup phase WAIT becomes TRACK only after all inputs qualify and raw plus qualified strobes are both low. Tags need not be zero. Quiet wires cannot prove that no earlier operation exists, so mid-batch attachment is unsupported. Starting with a held strobe does not invent an edge. A later quiet interval establishes prospective tracking but cannot reconstruct pre-attachment history.

TAG1:TAG0 qualifies as one two-bit bundle, preventing independently delayed bits from manufacturing an intermediate ordinal. READY and RETIRE qualify separately so unrelated chatter does not hide a sustained strobe. A candidate requires its first observation plus three unchanged nominal millisecond observations. Calls repeated within the same tick do nothing. Short pulses can be missed or rejected. Once qualified, a level remains retained until a new value qualifies.

An operation occurs on a qualified rising strobe. Five stable evaluated tag intervals must already have elapsed before that observation. Raw and qualified tag must agree, the current raw strobe must remain high, and the opposite strobe must be low in previous qualified, current qualified, and current raw samples. Simultaneous rising strobes reject both attempts. Rising one strobe while the other remains high is also rejected. A raw glitch can break setup even if it never becomes a qualified level. These are conservative diagnostic-domain rules rather than measurements of electrical setup time.

A valid READY sets the ordinal's readiness bit and increments prepared. Existing readiness is a duplicate error, including when that ordinal already retired. Bits remain set until all four retire. A valid RETIRE first requires the tag to equal next. A different tag yields the primary non-prefix finding even if that tag was also unready. If the tag is next but its bit is absent, retirement-before-ready is the finding. Successful retirement increments retired and next. Retirement of three completes the batch, increments rounds, and atomically clears readiness and next for the following batch.

Every qualified attempt after baseline contributes to attempts, whether accepted or rejected. Rejected classifies the one or two operation edges on the faulting sample. All policy errors permanently halt correlation. There is no optimistic recovery from silence, and the app cannot clear a finding. The retained readiness set and prefix remain visible. Reestablish the target's batch boundary and explicitly restart the instrument for a new run, exporting old evidence first.

Any advancing timestamp delta other than one records an acquisition gap and missed intervals, then halts. Ordinary unsigned timer wrap is supported. Counter changes are made on a copy of the ledger. If any update would overflow, the complete sample is rolled back and the prior ledger is halted with exhaustion. The discarded faulting sample is not represented as partly accepted. Halted evidence is immutable except time and saturated transmitter diagnostics. RAM evidence is lost on reset; no persistent journal is implied.

Necessary conservation identities are prepared = four times rounds plus readiness population, retired = four times rounds plus next, and attempts = prepared plus retired plus rejected. All ordinal bits below next must be ready. These constraints are checked in the firmware encoder and companion. They catch inconsistent evidence but are not cryptographic authentication or a complete proof that every possible record sequence corresponds to a physical waveform.

## Runtime, protocol, and application

The main loop samples before servicing output. UART submission copies one bounded packet; each poll writes at most one ready byte and never waits for transmission. An incomplete packet is abandoned after one hundred nominal milliseconds and tx_fail increases. The watchdog has bounded initialization waits and is fed by main-loop progress. Initialization failure halts the ledger before first status. Actual watchdog timing and stopped-clock behavior remain physical tests. Target-facing GPIO remains input-only; USART receive is disabled. Replacement firmware could drive those pins, so this is not a certified physical data diode.

PR1 is ASCII `PR1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC consists of four uppercase hexadecimal digits of CRC-16/CCITT-FALSE over bytes before the asterisk: polynomial 0x1021, initial 0xffff, no reflection or final XOR. B carries reset flags once at startup. P carries `3,1,5,4`, meaning qualification intervals, nominal sample milliseconds, required prior setup intervals, and batch size. Policy repeats approximately every second for late host attachment without pretending the device reset. S repeats approximately every hundred milliseconds.

S fields are `now levels valid halted faults phase ready next stable observed rounds attempts prepared retired rejected gaps missed tx_fail`. Values are unsigned 32-bit. Phase zero is WAIT and one is TRACK. Fault bits are non-prefix retirement=1, retirement before ready=2, duplicate readiness=4, interface/setup ambiguity=8, acquisition gap=16, exhaustion=32, watchdog startup=64. Only the first terminal reason is retained. Valid is a qualification mask, not a successful-batch indicator. Ready is a bitmap, while next is a numeric ordinal.

Run `python3 app/console.py` on Linux with Tk. Enter the exact serial port and choose New live. There is no automatic probing or command interface. Ledger displays counts, current state, and named findings. History retains up to 256 accepted snapshots. Integrity displays provenance, connection state, compatibility, receipt age, parser errors, reset flags, and freeze reason. Disconnect preserves evidence but removes live status. Successful new connection or replay replaces the session; failed connection preserves it. Export before replacement.

Live requires compatible policy, connection, two advancing statuses, and a recent advancing receipt. It describes transport progress, not safe target behavior: a halted ledger can still be live telemetry. Duplicate timestamps do not renew freshness. Counter rollback, disappearing findings, changed halted fields, unsupported policy, and inconsistent bitmap or prefix changes freeze acceptance. Host pauses over one second and excessive cumulative host/device progress divergence also freeze live collection rather than presenting queued old evidence as current. Drift allowance is capped at one second including buffering; long uncalibrated runs can conservatively freeze.

Replay accepts regular files up to two MiB and is always labelled replay. Nonblocking opening and file-type checking reject FIFOs and devices. Framing resynchronizes on the complete PR1 marker, bounds partial input, and counts incomplete EOF tails. Private JSON export creates a new mode-0600 file, refuses overwrite and final-component symlinks, and includes author jayis1, provenance, integrity metadata, latest state, and retained history. It is unsigned and unencrypted. No cloud upload, database, or automatic background retention is implemented.

## Build, verification, recovery, and physical acceptance

Required tools are GNU Make, Arm GNU GCC with newlib/nosys, host GCC, Python 3 with Tk, Xvfb for headless GUI checks, and KiCad 9 CLI/Python bindings with standard footprint libraries. No pip or npm dependencies are required. From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

The target build emits an actual Cortex-M4 ELF, binary, and map under firmware/build. Flash starts at 0x08000000 with explicit startup and reserved stack headroom. On a physically controlled, correctly identified Nucleo, a supervised OpenOCD workflow can use `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program firmware/build/prefix-retire.elf verify reset exit"`. This command is guidance, not an executed flashing result. Recover a faulty image through ST-LINK under reset according to the board manual; the app does not perform updates.

The fixture executable passes synthetic GPIO samples through the production engine and encoder. Tests then process those records through replay and Linux pseudo-terminals into the actual Tk event loop. Simulated transport is explicitly not physical USB or target acquisition. CAD checking executes ERC, DRC, and fresh schematic-netlist export, then compares every component value, footprint identity, and pin/net against PCB pads and the manifest. Passive-symbol ERC cannot validate an external power source or wiring harness.

Before fabrication inspect schematic and copper, actual component dimensions, connector clearance, and support. After assembly check continuity, resistor values, ground offset, input thresholds, loading, GPIO direction through reset, and back-power paths. Compare sample timing and glitch rejection against a calibrated analyzer. Exercise unplug, brownout, watchdog recovery, deliberately missing notifications, all readiness permutations, invalid retirement permutations, and long-run counter behavior. ESD, EMC, environmental reliability, manufacturing readiness, and regulatory suitability are unverified.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136, RM0368 for STM32F401 registers, and DS9716 for electrical characteristics. This release reuses the established repository board map; fresh manufacturer-document verification was not performed. Check current documents and the actual module revision before connecting anything. A lost boot frame followed by forward-looking counters can conceal a reset because there is no persistent boot identity. Maintain operator notes, keep the observer powered across target experiments, and start a documented new run after disruption rather than claiming uninterrupted trusted capture.
