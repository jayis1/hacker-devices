# Veto Memory — Irreversible Transaction-Veto Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol VM1. Firmware and companion application: GPL-2.0-only. Hardware utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked firmware, an executable Linux desktop companion, and routed native KiCad carrier files. Executed checks are recorded in VERIFICATION.md. No fabrication, MCU programming, electrical qualification, timing calibration, or physical target experiment is claimed. This is an observer, not a transaction coordinator, safety interlock, or proof of durable storage.

## Purpose and overview

Veto Memory checks a subtle transaction boundary: after either participant withdraws its preparation, or an abort is observed, can the target still announce a commit merely because the preparation wires later look healthy? A combinational permission check can miss this historical obligation. This instrument retains a veto throughout the current diagnostic round and compares a later commit against that retained history. It separates a commit with insufficient prior preparation from a commit after an irrevocable veto, and separately records repeated commits before the round returns to idle.

The target owner's diagnostic adapter exports four deliberately slow, nominal 3.3 V signals: PREP_A, PREP_B, COMMIT, and ABORT. Preparation levels indicate that two named participants currently consent to the operation. COMMIT is an indication of the coordinator's decision, not an instruction from this instrument. ABORT means the round must not commit. Once a preparation has been observed high, a qualified withdrawal before the first commit also vetoes that round. Restoring the preparation level does not restore permission. Only a full all-low boundary starts a new round.

An example is an embedded appliance applying a security configuration across a policy engine and an audit subsystem. Both participants prepare their changes. The audit subsystem then withdraws because its local durable write failed. A retry path reasserts its preparation, while the coordinator mistakenly retains an old commit request. Looking only at the final pair of prepare levels can make that commit appear valid. Veto Memory preserves the intervening withdrawal so that a laboratory report can distinguish a clean fresh transaction from an unsafe reuse of a vetoed one.

The useful output is a bounded external consistency record. It does not inspect configuration contents, collect credentials, read storage, send commands, induce faults, or prevent the operation. The owner must confirm that diagnostic signals accurately represent the real transaction. Important findings require source review and independent functional evidence; a wire named COMMIT is not proof that any persistent state changed.

## Originality and platform reuse

Repository directory names and README titles were inventoried before selection. A search for veto, withdrawn preparation, and two-phase commit identified no matching measurement contract. Quorum Witness checks a simultaneous majority vote, not a remembered per-round veto. Barrier Mask collects distinct arrival identities before one collective release, rather than tracking the withdrawal and restoration of two preparation levels. Lock Rank checks ordering across resource acquisitions, not a coordinator decision after abort. The new contribution here is historical veto retention, explicit empty-boundary semantics, commit-class precedence, and the tests that distinguish a restored level from restored authorization.

The external Nucleo module, passive four-input carrier topology, startup code, register mapping, nonblocking serial driver, bounded CRC encoder foundation, scheduler, and desktop transport reuse jayis1-authored work from lock-rank. Reuse is intentional, disclosed, and does not represent the resistor network as a new invention. Veto Memory supplies its own measurement engine, VM1 schema, cross-record contract validation, synthetic fixtures, and transaction-specific tests. Originality is claimed relative to this repository, not worldwide novelty or patentability. Standard KiCad footprint libraries and manufacturer documents retain their upstream provenance; project authorship does not claim ownership of those external works.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written authorization to assess. Scope should identify the target development build, diagnostic adapter, attachment points, workloads, permitted disruption, operator, stop procedure, and retention policy. Begin on an isolated laboratory fixture with hazardous outputs disabled. Never attach this unisolated prototype to mains wiring, occupied access-control systems, vehicles, medical equipment, machinery, or unknown industrial field voltages. The author, jayis1, makes no safety certification, regulatory approval, manufacturing-readiness guarantee, or warranty of security.

The measured attack surface is transaction bookkeeping during error recovery: stale consent, forgotten aborts, restored preparation being mistaken for a new round, simultaneous prepare and commit, and duplicate commit requests. An authorized red team may ask the owner to exercise approved recovery workloads that expose these mistakes. A penetration tester may compare a baseline and patched build using identical diagnostic traces. A product-security researcher may use the ledger to decide where to investigate inconsistent coordinator state. None of these workflows authorizes covert deployment, unauthorized access, or destructive testing.

The operator, harness, observer firmware, Nucleo board, programmer, and workstation are trusted. A malicious target can falsify every diagnostic level. A malicious serial device can fabricate checksum-valid records. A compromised workstation can edit exports. CRC detects accidental corruption, not authenticity, replay, or origin. There is no secure element, signed bootloader, authenticated pairing, encrypted telemetry, persistent boot identifier, trusted wall clock, cloud service, network listener, radio, or automatic update facility. Physical SWD programming is a locally controlled maintenance operation; restrict access to the programmer and record the image used for each campaign.

Receive-only behavior is established by the supplied GPIO configuration, not an irreversible physical data diode. Replacement firmware could drive a lead. Correct firmware can still disturb the target through grounding, input loading, miswiring, or back-power. A disconnected lead reads low and cannot be distinguished from a genuine withdrawal or idle level. Stop immediately if attachment changes target operation. Keep the fixture visible, labelled, mechanically supported, and within its electrical envelope.

## Hardware specifications and bill of materials

The complete device consists of an external ST NUCLEO-F401RE development board, this passive carrier, short labelled insulated leads, an insulating support, and a Linux workstation. The STM32F401RET6 Cortex-M4 has 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, one-millisecond SysTick, and USART2 transmission at 115200 baud with eight data bits, no parity, and one stop bit. PA2 transmits toward the Nucleo ST-LINK virtual serial bridge. USART reception is disabled.

The Nucleo provides regulation, decoupling, reset, USB conversion, programming access, and the MCU. These are real external module functions, not omitted parts of an imaginary bare-chip PCB. The custom board is only a passive harness carrier, connected by wires rather than stacked as a pin-compatible shield. It has no FPGA, radio, antenna, battery, charger, active sensor, level translator, isolated supply, or removable storage. Reserve a conservative 5 V USB source allowance of 500 mA for the complete module assembly, subject to actual jumper settings and USB constraints. This is a power-source budget, not measured consumption.

| Item | Quantity | Specification / candidate |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, axial DIN0207; Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, axial DIN0207; Yageo MFR-25FBF52-1K |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header; Samtec TSW-102-07-G-S |
| J2 | 1 | 1x4 vertical 2.54 mm header; Samtec TSW-104-07-G-S |
| Harness and support | 1 set | Labelled insulated leads, strain relief, nonconductive base |

Purchasing candidates are not verified availability or stock claims. Compare actual supplier drawings, body dimensions, lead diameters, and mating access before ordering. Resistor footprints use 7.62 mm lead spacing. The PCB is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm tracks and a closed outline. Its fourteen components expose thirty pads across nine electrical nets. Mounting holes and enclosure CAD are not included; external support and underside insulation are required.

## Pin mapping and electrical envelope

| Function | Target header | MCU header | Nucleo / MCU | Bit |
| --- | --- | --- | --- | --- |
| PREP_A | J3.1 | J2.1 | A0 / PA0 | 0 |
| PREP_B | J4.1 | J2.2 | A1 / PA1 | 1 |
| COMMIT | J5.1 | J2.3 | A2 / PA4 | 2 |
| ABORT | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only, with both pins connected together. Neither pin supplies power. J2 contains four signal contacts and no ground or power contact. Each target signal passes through 1 kOhm to an MCU-side node with a 100 kOhm pulldown. Net names ending `_IN` denote target-side nodes. Check connector numbering and continuity rather than guessing polarity from orientation or wire color.

Only common-ground, nominal zero-to-3.3 V push-pull diagnostic outputs are supported. Five-volt outputs, 1.8 V logic, negative voltage, differential pairs, relay coils, mains-derived interfaces, and unknown test points are excluded. The series resistor is not qualified overvoltage protection. There is no dedicated ESD clamp, isolation barrier, reverse-current blocker, fuse, or hot-plug guarantee. Static resistive loading presents a nominal 101 kOhm path to ground; source impedance, leakage, tolerance, and capacitance alter real input margins.

Power the observer before enabling target signals and remove target signals before depowering it. Connect only compatible grounds. USB ground reaches the target through Nucleo and J1, so an apparently passive observer can create an unwanted ground path. Verify thresholds, cable capacitance, rise time, loading, ground offsets, and back-power behavior on a sacrificial fixture before attaching valuable equipment. Do not rely on MCU protection structures to absorb a powered target output.

## Architecture and block diagram

```text
Owner-controlled transaction diagnostics
 PREP_A   PREP_B   COMMIT   ABORT
    |        |        |       |
 J3..J6: 1k series, 100k MCU-side pulldown, shared ground
    |        |        |       |
 J2 --> external NUCLEO-F401RE PA0, PA1, PA4, PB0
                     |
             independent qualifiers
                     |
       WAIT -> READY -> ACTIVE -> DRAIN
                   |       |
       remembered withdrawal / abort veto
                   |       |
        commit classification + conserved round counts
                     |
         bounded VM1 status + CRC16, USART2 TX
                     |
        ST-LINK USB virtual COM -> Linux desktop
                     |
           Ledger / History / Integrity
                     |
             private local JSON export
```

GPIOA is read before GPIOB; acquisition across the ports is not electrically simultaneous. There is no claim of capturing processor-speed races, narrow pulses, or exact edge timing. The adapter must deliberately stretch diagnostic levels. Initially hold every relevant state for at least ten nominal milliseconds and validate the resulting waveform with an independent logic analyzer. This is a slow contract witness, not a replacement for high-speed trace equipment.

## Exact measurement contract

Each channel qualifies independently after the initial candidate sample and three unchanged one-millisecond intervals. Four equal samples establish a new level. A pulse rejected by qualification does not necessarily contribute a finding. Independent qualification prevents unrelated chatter from postponing a sustained commit forever, but also means qualified states are histories, not an instantaneous four-bit electrical sample. The additional raw checks below are conservative endpoint guards, not full raw-edge capture.

WAIT is the initial state. A fully qualified all-low vector with raw inputs also low establishes READY and increments baselines. Initial attachment with any input already high does not invent a transaction or a commit. The owner must ensure this empty boundary really means that the target has no pending round. All-low glitches that qualify can falsely divide one physical transaction into two observed rounds, so correct adapter design and pacing are prerequisites.

From READY, the first nonzero qualified vector begins ACTIVE and increments rounds. A direct commit from idle therefore opens a round and is classified early; it is not silently ignored. A prepare level, abort alone, or any other nonzero vector can begin a round. There is intentionally no transaction timeout: the instrument records the observed contract without imposing an invented maximum preparation latency. An indefinitely active round remains active until commit, all-low cancellation, acquisition loss, or observer reset.

While ACTIVE, seen retains which preparations have been observed high. A previously seen preparation changing from qualified high to low latches veto and increments withdrawals for that participant. Both preparations can withdraw in the same observation. Reassertion never clears veto. A qualified ABORT high also latches veto. veto_rounds increments once per round when the latch first sets, not every tick while it remains high. A raw ABORT high at a qualified commit endpoint also latches veto, even if it has not yet independently qualified. A short raw abort occurring only between commit endpoints and never qualifying may be missed.

COMMIT rising edges are classified in this precedence order. A latched veto produces vetoed. Otherwise, both preparations must be high in the previous qualified observation, the current qualified observation, and the current raw sample; failure produces early. If those conditions hold, accepted increments. A simultaneous second preparation and commit is early because the prior observation did not establish readiness. A raw preparation drop at the commit endpoint is early if it has not qualified into a withdrawal; a qualified withdrawal takes the stronger vetoed classification. These are explicit sampled-order rules, not claims of physically resolving simultaneous events.

The first commit ends ACTIVE and enters DRAIN, regardless of classification. While DRAIN, another qualified COMMIT rising edge increments repeated and commits, without opening a new round or changing the earlier classification. Holding COMMIT high never creates additional commits. Preparation changes after the first decision are not classified as precommit withdrawals. The original veto and seen masks remain visible until the boundary clears them.

A fully qualified all-low vector with raw inputs low returns DRAIN to READY. The same empty condition during ACTIVE cancels the round, increments cancelled, and establishes another baseline. Empty cancellation takes precedence over withdrawal accounting: simultaneous removal of all indications ends the round rather than inventing a withdrawal finding for a round that no longer exists. The next nonzero qualified vector is then a genuinely new observed round. The adapter must not encode an in-round pause as a full empty boundary.

The conservation equations are `rounds = accepted + early + vetoed + cancelled + abandoned + active`, where active is one only in ACTIVE, and `commits = accepted + early + vetoed + repeated`. A gap in acquisition permanently halts this run, clears qualification, and abandons a previously active round. It does not silently forget a veto and resume. Reestablish the actual target idle state, export retained evidence, and deliberately restart the observer. Ordinary unsigned millisecond wrap is supported; repeated calls at the same timestamp do nothing.

Every sample updates a temporary ledger. If a cumulative counter would overflow, the entire speculative sample is discarded and the previous active round is abandoned exactly once before a terminal exhaustion halt. The serial-failure counter saturates separately. Startup watchdog failure also halts before the first status. Halted snapshots may advance device time and serial diagnostics, but measurement evidence remains frozen. Observer reset loses all volatile evidence; no power-loss-safe journal or unique persistent boot identity is implemented.

## Firmware organization and protocol

Production firmware is allocation-free C11: main.c provides scheduling, engine.c implements qualification and the veto ledger, wire.c encodes bounded records, drivers/board.c configures the MCU, drivers/serial.c services nonblocking transmission, and drivers/crc16.c implements error detection. board.h, registers.h, startup.c, linker.ld, and Makefile supply the actual target build. UART service sends at most one ready byte per poll and abandons a transfer stalled for one hundred milliseconds. Acquisition never waits for a complete serial frame.

VM1 records are ASCII `VM1,type,fields*CCCC` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, with initial 0xffff, polynomial 0x1021, no reflection, and no final XOR. CCCC is four uppercase hexadecimal digits. B carries reset flags once at boot. P carries `3,1,2,1`: qualification intervals, nominal sample milliseconds, participant count, and policy revision. P repeats approximately every second without clearing evidence, supporting late attachment after a missed boot record. S is offered approximately every hundred milliseconds.

S contains these unsigned 32-bit fields in exact order: `now levels valid halted faults phase veto seen observed baselines rounds accepted early vetoed cancelled abandoned commits repeated veto_rounds withdrawals gaps missed tx_fail`. phases are WAIT=0, READY=1, ACTIVE=2, DRAIN=3; halted is separate. Fault bits zero through five mean early commit, commit after veto, repeated commit, acquisition gap, counter exhaustion, and watchdog startup failure. observed counts evaluated fully qualified ticks, not complete electrical coverage. Both encoder and companion reject necessary conservation and state contradictions, but those checks do not prove every possible historical trace was realizable.

## Companion application and data handling

Run `python3 app/console.py` from this directory on Linux with Tk installed. The application has Ledger, History, and Integrity screens. Enter an explicit serial path, normally `/dev/ttyACM0`, then select New live. There is no probing, command transmission, privilege escalation, automatic permission repair, or device-authentication claim. A second connection is refused until disconnect. Opening failure preserves the previous evidence. Disconnect retains measurements but removes live status.

Ledger displays counters, qualified masks, phase, remembered veto, and sticky findings. History retains at most 256 snapshots rather than every GPIO edge. Integrity exposes source, origin, compatibility, parser errors, reset flags, elapsed receipt age, connection state, and any freeze reason. LIVE means a compatible transport is making recent progress, not that acquisition is active, wiring is correct, or a transaction is secure. A halted device can still send live telemetry; inspect halted and phase separately.

Two advancing accepted snapshots are needed to show live progress. Duplicate timestamps do not renew freshness. Contradictory duplicates, backwards time, cumulative-counter rollback, disappearing sticky findings, a new boot after evidence, and changing halted measurements freeze acceptance. A veto cannot disappear within a round without a baseline or halt. Reception pauses longer than one second and excessive cumulative host/device timing divergence require reconnect rather than treating queued old data as fresh. Without a persistent boot identifier some resets can remain indistinguishable from forward traffic; this is a documented limitation, not authenticated session continuity.

Replay accepts only regular files of at most two MiB, rejects FIFOs and device files without waiting, and stays visibly nonlive. Framing resynchronizes at the complete VM1 marker, bounds line storage, verifies checksums and integer ranges, and reports unfinished tails. Export creates a new mode-0600 JSON file with author jayis1, source, provenance, integrity state, latest snapshot, and bounded history. Existing paths and final-path symlinks are refused. Exports are local, unsigned, and unencrypted; use a trusted directory and the engagement's retention policy. Deleting a file does not guarantee secure erasure.

## Build, verification, and bring-up

Required development tools are GNU Make, Arm GNU GCC with newlib/nosys support, native GCC, Python with Tk, Xvfb for headless GUI tests, and KiCad 9 CLI/Python bindings with its standard footprints. No downloaded app package or mobile SDK is required. Run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

The target build emits ELF, binary, and map files under firmware/build. The linker starts flash at 0x08000000 and reserves stack headroom within 96 KiB RAM. Program only through a supervised local ST-LINK workflow appropriate to the physical board, followed by hardware reset. No programming or execution on silicon is claimed in this release. Running `firmware/build/fixture > synthetic.vm1` produces explicitly synthetic production-encoder traffic, not a physical capture.

Host tests cover clean commit, transient abort retention, withdrawal and reassertion, repeated commit, cancellation, all prepare/commit masks, raw endpoint guards, late attachment, gaps, timer wrap, overflow, randomized walks, bounded encoding, CRC, UART timeout, and main-loop policy announcements. App tests consume those real C-encoder fixtures, exercise Tk screens through a pseudo-terminal, verify disconnect and replay, and test private exports and hostile frames. The CAD checker runs ERC and DRC, exports a fresh schematic netlist, and compares every reference, value, footprint identity, pin, and net against actual PCB pads and the connectivity manifest.

Before fabrication, inspect schematic and layout, verify the actual purchased footprint dimensions, and plan support and insulation. After assembly, check shorts, continuity, resistor values, ground compatibility, loading, voltage margins, and powered/unpowered behavior. Then use a benign source to exercise each channel, verify the input-only configuration through reset, measure qualification timing, disconnect USB, and confirm watchdog recovery. ESD, EMC, regulatory suitability, thermal behavior, oscillator calibration, and sustained real-target operation remain unverified. Zero CAD violations do not establish those properties.

## Manufacturer references and research workflow

ST UM1724, STM32 Nucleo-64 boards MB1136, identifies the NUCLEO-F401RE connector map in Table 16 and the USART2 virtual COM routing in section 7.10 of revision 17. Search-index text was checked for PA0/A0, PA1/A1, PA4/A2, PB0/A3 and the normal SB13/SB14 enabled, SB62/SB63 disabled virtual COM configuration; direct PDF retrieval failed during this run. Confirm the actual board revision and bridge settings before assembly. ST RM0368 defines STM32F401 peripheral registers; the STM32F401xD/xE datasheet defines electrical limits. Existing repository register support is reused; fresh full-document electrical qualification is not claimed.

References: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf ; https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf ; https://www.st.com/resource/en/datasheet/stm32f401re.pdf . Manufacturer materials are external references, not files authored by this project.

For a practical assessment, begin with a known-idle owner fixture and record wiring, target build, observer image, and test scope. Exercise clean prepare/commit first. Then repeat with a qualified preparation withdrawal followed by restoration while the other preparation stays high. Finally repeat with a transient qualified ABORT, leaving a preparation high so the round does not return to empty. Preserve both exported evidence and an independent waveform. Expected veto findings indicate a diagnostic contract failure, not automatically an exploitable vulnerability. Compare the same vectors after remediation and report gaps, missing coverage, and adapter limitations alongside the counter differences.
