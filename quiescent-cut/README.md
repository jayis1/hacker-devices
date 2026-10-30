# Quiescent Cut — Retirement-Cohort Reclamation Witness

Author and creator: jayis1. Hardware revision A. Wire protocol QC1. Firmware, application, and tests: GPL-2.0-only. Hardware utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. This folder contains target-linked firmware, an executable Linux desktop companion, and native routed KiCad carrier files. Software execution and CAD checks are recorded in VERIFICATION.md. No physical fabrication, MCU programming, electrical qualification, calibrated timing, or deployment is claimed. This is an observation instrument, not a memory manager, safety controller, or proof that reclamation is secure.

## Purpose and practical value

Quiescent Cut investigates a specific security-sensitive lifecycle: an object is removed from publication, readers that could already hold that object finish their read sections, and only afterward is the object reclaimed. A premature free can become a use-after-free vulnerability. A test that merely waits until every reader is idle can conceal a different implementation error: it unnecessarily waits for new readers that could never have obtained the retired object. This instrument separates the reader population captured at retirement from subsequent readers.

An authorized development target exposes four slow diagnostic indications: READER_A, READER_B, RETIRE, and RECLAIM. Each reader line represents one independently tracked read-side lane. RETIRE marks removal of one object from publication. RECLAIM marks the actual decision to recycle that object. The observer captures which readers are present at the retirement cut, remembers which of those readers still owe a quiescent observation, and classifies the eventual reclaim edge against that remaining obligation. A new read section after a lane has become quiescent does not restore its old obligation.

The intended use is a deliberately slowed, owner-instrumented demonstration or regression fixture, not attachment to arbitrary processor memory buses. The observer cannot see pointers, actual allocator state, CPU caches, memory barriers, grace-period implementation internals, or whether a reader really accesses an object. A diagnostic signal that lies about publication or read-side lifetime can make an invalid algorithm appear correct. Source review and independent trace must establish the meaning of every wire.

Dedicated hardware is useful when the suspected defect and the target logger share the same scheduler, lock, or reporting queue. The observer has a separate MCU and reports a small cumulative ledger without receiving application payloads. This reduces reliance on target logs, but does not create a trusted attestation service. The benefit is a narrowly specified, reproducible external consistency check.

## Repository-local originality

Existing device directories and README purposes were inventoried before implementation. The closest contracts were read in full. Revocation Drain waits for an admission freeze and continuous BUSY quiet before acknowledgment. Barrier Mask accumulates tagged notifications from a fixed set of four participants. Grant Ledger consumes a single authorization credit. Burst Budget replenishes credits according to elapsed time. Veto Memory retains preparation withdrawal or abort within a transaction. None implements a retirement-time snapshot of two reader lanes whose obligations can disappear independently while later readers remain active.

The distinctive experiment is to retire with both lanes active, let lane A exit, restart A as a new reader, let B exit, and reclaim while A remains active. Quiescent Cut accepts that ordering because only the old cohort matters. A blanket idle detector would reject it. Conversely, dropping the RETIRE strobe does not cancel the old cohort, and a second retirement before the first object is resolved loses correlation rather than replacing inconvenient obligations.

Originality is claimed relative to the reviewed repository, not as a worldwide patent or prior-art assertion. The carrier topology, STM32 startup and peripheral support, serial framing implementation, and desktop transport deliberately reuse existing work authored by jayis1. Reuse is explicit rather than presenting a renamed resistor network as a new electrical invention. All necessary files are local to this folder; building the device does not depend on another device directory.

## Legal and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Approval must cover firmware instrumentation, physical attachment, workloads, permitted interruptions, and evidence retention. Permission to test an application does not automatically authorize connecting an unisolated board to unknown test points. Keep the instrument visible, label the harness, and stop if attachment changes target behavior.

Use a low-energy development fixture with hazardous actuators disconnected. Do not attach to deployed door controls, medical equipment, operating machinery, vehicles in service, mains-related circuitry, or unknown industrial wiring. Receive-only software is not electrical isolation, a safety guarantee, or permission for covert collection. The author, jayis1, does not authorize unauthorized interception, destructive testing, or exploitation of third-party systems.

For red-team work, this device supports an agreed laboratory demonstration of unsafe reclamation indications on a sacrificial target. For penetration testers, it supplies supporting evidence for a source-level investigation and remediation retest. It does not generate exploit traffic, read credentials, extract memory, install persistence, or issue target commands. Do not assign a vulnerability severity solely from a diagnostic counter without corroborating the protected operation.

## Attack surface and threat model

The assessment surface is the integration between publication removal, read-side lifetime tracking, and actual recycling. Relevant defects include freeing before an old reader exits, accidentally clearing obligations when RETIRE deasserts, replacing one pending retirement with another, and treating a simultaneous final reader exit as proof that reclamation happened afterward. The instrument records these as sampled contract observations, not proven physical ordering at CPU timescales.

Trusted elements are the operator, wiring, approved target adapter, installed observer image, development module, programming interface, and workstation. Potentially hostile inputs include malformed serial traffic and replay recordings. The companion validates framing, checksum, unsigned bounds, conservation identities, chronology, and sticky findings before presenting records. These checks reject selected inconsistent data; they do not authenticate a source or prove that every accepted summary has a realizable underlying waveform.

CRC-16 detects accidental corruption, not malicious forgery. There is no encrypted transport, pairing credential, secure element, signed evidence, persistent boot identifier, secure bootloader, anti-rollback enforcement, network endpoint, or automatic updater. ST-LINK access can replace firmware. A malicious USB device can impersonate the stream. Protect the bench physically and preserve firmware hashes independently where chain of custody matters.

The supplied firmware leaves all target-facing pins as inputs. Its only transmitter is USART2 toward the external module's ST-LINK USB bridge. The app opens the serial device read-only, and there is no inbound command parser. Nevertheless, programmable GPIO is not a physical data diode: replacement firmware could drive those pins. Shared USB ground, resistor loading, incorrect voltages, and power sequencing can disturb the target even with correct firmware.

## Hardware specification and bill of materials

The assembly comprises an external ST NUCLEO-F401RE development board, the supplied passive carrier, short insulated leads, a nonconductive support, and a Linux workstation. The STM32F401RET6 provides Cortex-M4 execution, 512 KiB flash, and 96 KiB SRAM. Firmware uses reset-default nominal 16 MHz HSI, a nominal one-millisecond SysTick, and USART2 at 115200 baud, eight data bits, no parity, one stop bit. No PLL, RTOS, dynamic allocation, vendor SDK, or floating-point unit is required.

The purchased Nucleo module supplies MCU regulation, decoupling, reset, SWD programming, USB power handling, and the ST-LINK virtual COM connection. These facilities are intentionally external; the carrier is not advertised as a complete standalone MCU board. The carrier has no FPGA, radio, antenna, active sensor, battery, charger, storage card, isolated power supply, or level translator.

The board is 80 by 70 mm, two-layer, nominally 1.6 mm thick, with 0.3 mm routed tracks and a closed outline. Fourteen through-hole components connect thirty electrical pads across nine named nets. It is cable-connected, not a mechanically mating Nucleo shield. There are no mounting holes or enclosure CAD. Provide insulating support, underside protection, and strain relief before use.

| Reference | Quantity | Requirement |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Straight 1x2, 2.54 mm through-hole header |
| J2 | 1 | Straight 1x4, 2.54 mm through-hole header |
| Harness/support | 1 set | Labelled insulated leads, nonconductive support |

The resistors use KiCad's DIN0207 axial footprint with 7.62 mm lead spacing; headers use standard vertical PinHeader footprints. Check purchased body dimensions, drill fit, connector mating clearances, and square pin-one pads before ordering. Availability, price, assembly yield, and physical fit have not been validated.

## Pin map and electrical boundaries

| Signal | Target connector | MCU harness | Nucleo point | Bit |
| --- | --- | --- | --- | --- |
| READER_A | J3 pin 1 | J2 pin 1 | CN8 A0 / PA0 | 0 |
| READER_B | J4 pin 1 | J2 pin 2 | CN8 A1 / PA1 | 1 |
| RETIRE | J5 pin 1 | J2 pin 3 | CN8 A2 / PA4 | 2 |
| RECLAIM | J6 pin 1 | J2 pin 4 | CN8 A3 / PB0 | 3 |
| GND | J3–J6 pin 2 | J1 both pins | Nucleo GND | none |

All signals are active high. J1 is ground only; never attach a supply to either pin. J2 has four signals, not power. Net names ending `_IN` are target-side nodes before the series resistor; unsuffixed names are MCU-side nodes with individual pulldowns. Confirm numbering by continuity rather than cable appearance.

Supported sources are nominal zero-to-3.3 V common-ground push-pull diagnostic outputs. Each channel has a 1 kOhm series resistor and 100 kOhm pulldown. The resistance path to ground is nominally 101 kOhm before leakage and source impedance. There is no qualified ESD suppressor, overvoltage clamp, fuse, reverse-current blocker, galvanic isolation, or hot-plug protection. The series resistor is not a safety barrier. Do not attach 5 V, 1.8 V, negative-voltage serial, differential buses, relay coils, or 24 V industrial signals.

Power the Nucleo through its documented ST-LINK USB arrangement. Reserve a conservative 5 V, 500 mA source budget subject to the module's jumper configuration and USB limits; this is not measured consumption. Establish compatible ground before attaching signals, power the observer first, and remove signals before powering it down. Otherwise target outputs can back-power the MCU through protection structures. Verify actual solder bridges and board revision before relying on USART2-to-ST-LINK routing.

## Architecture and block diagram

```text
Authorized target adapter       Passive carrier            External Nucleo
READER_A ----------------------- 1k --+--------------------- PA0 / A0
READER_B ----------------------- 1k --+--------------------- PA1 / A1
RETIRE ------------------------- 1k --+--------------------- PA4 / A2
RECLAIM ------------------------ 1k --+--------------------- PB0 / A3
                                     | each input node
                                    100k
                                     |
Common ground ---------------------- GND ------------------ GND / USB host

GPIO sample -> continuity guard -> independent channel qualifiers
            -> retirement cohort capture -> owed-reader mask
            -> reclaim classification -> conserved cumulative ledger
            -> QC1 bounded encoder -> nonblocking USART2 PA2 TX
            -> ST-LINK USB serial -> Ledger / History / Integrity screens
                                 -> explicit private JSON export
```

GPIOA is read before GPIOB; these are not simultaneously latched physical observations. Inter-port skew is unmeasured. Events within a sample interval cannot be ordered reliably, and short pulses can be rejected or missed. Use deliberately slowed diagnostic indications with at least ten milliseconds high and low dwell for initial experiments, and validate the adapter with an independent analyzer. This is not a high-speed memory-bus tap.

## Exact firmware measurement contract

Each channel qualifies independently. A candidate change requires its first observation plus three further unchanged nominal millisecond intervals. Chatter on an unrelated lane does not suppress a sustained strobe. Duplicate calls in the same tick do not advance qualification. A nonzero timestamp difference other than one means acquisition continuity is lost; the device records the gap and missed ticks, abandons an active round, and halts correlation permanently for this run. Quiet signals cannot reconstruct a lost reader cohort.

At startup the phase is WAIT. The operator must arrange the real target between retirements, then start the observer. Once all inputs qualify and both raw and qualified RETIRE and RECLAIM are low, the phase becomes READY. Reader lines may already be high; those readers are legitimate members of a later cut. Mid-retirement attachment is unsupported. Activity before READY is outside coverage and not retroactively counted.

A qualified RETIRE rising edge in READY opens one round. The captured cohort is the union of previous qualified, current qualified, and current raw reader bits. This deliberately conservative boundary includes a reader leaving on the same observation and a raw reader assertion not yet qualified. The owed mask initially equals that cohort. A RETIRE pulse is an event, not a continuing permission level: lowering it cannot cancel old obligations.

On subsequent active observations, a captured lane becomes quiescent only when its qualified and raw bits are both low. Clearing that owed bit increments released once. A later high on that lane does not re-add the obligation. The target adapter must ensure that a new read-side section cannot acquire the retired object after publication removal. If that assumption is false, acceptance is meaningless; the observer cannot inspect pointers or infer acquire/release memory ordering.

Each qualified RECLAIM rising edge after initial baseline increments reclaims and receives one category. In ACTIVE it is accepted only if the owed mask was already empty in the previous evaluated observation. Otherwise it is early. The observation that clears the final owed bit is not itself sufficient for a simultaneous reclaim. A reclaim tied to retirement is early even with an empty cohort. This conservative endpoint rule avoids claiming favorable order for events observed together.

An active reclaim, whether accepted or early, closes that one round and enters DRAIN. Any remaining owed readers are counted as discarded evidence, not as successfully quiesced readers. A later correct-looking event cannot repair an early reclaim. When both strobes return raw and qualified low, a new READY baseline is established; reader lines need not become idle. Reclaim outside ACTIVE is orphaned, including repeated reclaim after a completed round. Held-high strobes count only once, not once per sample.

A new RETIRE rising edge outside READY, after the initial WAIT coverage boundary, is overlapping retirement. It increments overlap and permanently halts correlation. A simultaneous reclaim is counted as orphan before halting. This is a one-object-at-a-time diagnostic interface: guessing which object a later reclaim belongs to would fabricate evidence. There is deliberately no timeout. A reader can hold an obligation indefinitely, and the app displays the still-active round rather than treating elapsed time as permission.

All accounting uses unsigned 32-bit counters. A sample is evaluated on a temporary state copy. If any increment exhausts its counter, the whole speculative sample is rolled back and the previously committed active round is abandoned before terminal halt. Ordinary millisecond timestamp wrap is supported; arbitrary backward time is a discontinuity. UART failure counts saturate separately. No state persists across observer reset and there is no remote clear command.

Three conservation identities are checked by firmware and companion: rounds equals accepted plus early plus abandoned plus active; reclaims equals accepted plus early plus orphan; captured equals released plus discarded plus population of owed. Active is one only in ACTIVE phase. These identities separate reader obligations, object lifecycles, and operation indications. They do not prove that memory was actually freed safely.

## Runtime and wire interface

The main loop samples before servicing telemetry. UART submission copies a bounded frame; each poll sends at most one byte when ready. A stalled frame is abandoned after one hundred nominal milliseconds. No host read or acknowledgment is required. Watchdog initialization waits are bounded by SysTick; failure produces terminal fault before first status. A stopped clock and physical watchdog behavior still require silicon testing.

QC1 frames are ASCII `QC1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. The four uppercase CRC digits use CRC-16/CCITT-FALSE: polynomial 0x1021, initial 0xffff, no reflection or final XOR, over bytes before the asterisk. B carries the reset-register flags once at boot. P carries `3,1,2,1`: qualification intervals, nominal sample milliseconds, reader lanes, and policy revision. P repeats approximately every second so late attachment can establish compatibility without declaring a new boot. S is scheduled approximately every hundred milliseconds.

S fields, in order, are:

`now levels valid halted faults phase cohort owed observed baselines rounds accepted early orphan abandoned reclaims overlap captured released discarded gaps missed tx_fail`

Phases zero through three mean WAIT, READY, ACTIVE, DRAIN. Input and qualification masks use the pin-table bit order. Fault bits zero through five mean early reclaim, orphan reclaim, overlapping retirement, acquisition gap, counter exhaustion, watchdog startup failure. Faults are sticky. Overlap, gap, exhaustion, and watchdog failure halt the run; early and orphan findings do not. HALTED freezes policy evidence while time and transmitter diagnostics may still advance. Telemetry progress must not be confused with healthy acquisition.

## Companion application

Run `python3 app/console.py` on Linux with Tk installed. The dependency-free desktop provides Ledger, History, and Integrity screens. Choose the exact serial terminal, normally `/dev/ttyACM0`, then New live. No automatic probing, pairing, permission changes, or target commands occur. A second connection is rejected until disconnect. Opening failure preserves the previous evidence. A successful replacement intentionally starts a new host session, so export first if old evidence matters.

Ledger shows masks, phase, counts, and named findings. History retains up to 256 accepted snapshots, not every reader edge. Integrity displays provenance, connection state, policy compatibility, parser errors, reset flags, freshness, and freeze reasons. Disconnect preserves retained evidence but removes live status. Linux hangup flags are handled even where a disconnected terminal returns empty reads rather than raising an exception.

Freshness requires a connected live source, compatible policy, two advancing status timestamps, and receipt within one second. Duplicate timestamps do not renew freshness. Contradictory duplicates, cumulative rollback, disappearing sticky faults, changed halted evidence, or a new boot after evidence freeze acceptance. Long reception pauses and cumulative host/device clock divergence also freeze rather than presenting queued old records as current. Drift tolerance is a half-second buffering allowance plus a capped half-second oscillator allowance; this can conservatively reject a long uncalibrated session.

Replay is always labelled replay, never live. Only regular files up to two MiB are accepted, opened nonblocking before checking type so a FIFO cannot stall the interface. Framing resynchronizes on the full QC1 marker. Truncated tails count as parser errors, and a replay with no compatible status fails. A corrupt record does not erase earlier retained evidence. These controls bound accidental and hostile input handling, not the truth of a maliciously forged recording.

Export creates a new JSON file with mode 0600, refuses overwrite and final-path symlinks, and includes author jayis1, provenance, connection and freshness status, parser errors, reset metadata, warnings, latest snapshot, and history. There is no automatic upload or hidden database. Reports are unsigned and unencrypted; local permissions do not protect against an administrator or compromised workstation. Follow the engagement's retention and disposal rules.

Some observer resets remain indistinguishable if the boot packet is lost and all later counters and times happen to progress. There is no persistent identity or attested continuous capture. Reconnect after transport disruption and establish the real target baseline before restarting an observer. Retained acceptance counts describe diagnostic consistency, never immunity from use-after-free.

## Build and verification

Required tools are GNU Make, Arm GNU GCC with newlib/nosys support, host GCC, Python 3 with Tk, Xvfb for headless GUI execution, and KiCad 9 CLI/Python bindings with standard footprints. No pip or npm dependency installation is necessary. Run from this directory:

```sh
make -C firmware all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

The firmware build produces ELF, binary, and map files under firmware/build for Cortex-M4 Thumb soft-float, flash at 0x08000000, and the specified RAM. The linker reserves stack headroom. Programming through ST-LINK is a separate supervised physical step; do not use debugger initialization that silently changes the reset-clock contract. `firmware/build/fixture` writes synthetic production-encoder traffic suitable for replay. It is test data, not a hardware measurement.

Host tests cover cohort combinations, both reader-release orders, new readers, simultaneous boundaries, orphan reclaim, overlapping retirement, acquisition gaps, counter exhaustion, timer wrap, bounded encoding, UART timeout, and watchdog startup. App tests use real Tk event loops and Linux pseudo-terminals with the real C fixture, checking screens, private export, error paths, disconnect, transport restoration, and replay. Passing these tests does not demonstrate electrical behavior or successful MCU execution.

The hardware checker runs ERC and DRC, exports a fresh schematic netlist, and compares every reference, value, full footprint identifier, pin, pad, and net against the PCB and manifest. Symbols are embedded and footprints are actually placed and routed. Passive ERC cannot validate the external power supply, harness orientation, threshold margins, or module internals. Review rendered schematic and copper, component dimensions, soldering access, and support before fabrication.

## Research workflows and physical acceptance

For a product-security regression, instrument two real read-side lanes and one retirement/reclaim pair. First test an empty cohort. Then retire with both readers active, release them in each order, and repeat with a post-cut reader left high. Add an intentionally premature reclaim in the owned fixture to verify that the observer and independent trace agree. Preserve adapter source, exact target revision, observer image, wiring, workload schedule, and exported integrity qualifications.

For an authorized red-team demonstration, use an isolated training target that signals reclaim before an old reader ends. The observer exposes that ordering without collecting object contents or attempting memory corruption. A penetration tester can repeat the same paced experiment after remediation. A clean sample trace supports only the tested diagnostic contract; it cannot establish full concurrency correctness under arbitrary workloads.

Before physical use, inspect assembly, measure continuity and resistor values, check shared-ground potential, confirm input-only behavior through resets, measure loading and logic thresholds, and compare sample timing against a calibrated analyzer. Exercise observer power loss, target power loss, USB removal, short pulses, asynchronous transitions, watchdog recovery, and stalled host reception. Fabrication, flashing, electrical bring-up, ESD, EMC, environmental reliability, oscillator calibration, and regulatory qualification remain outstanding.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136, RM0368 for STM32F401 registers, and DS9716 for STM32F401xD/xE electrical characteristics. The implementation reuses the repository's established module mapping and register support; fresh manufacturer-document verification was not performed. Check current documents matching the physical board before assembly. Author of this design, firmware, application, and documentation: jayis1.
