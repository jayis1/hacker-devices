# Epoch Witness — Firmware-Generation Monotonicity Evidence Bench

Author and creator: jayis1. Hardware revision A; protocol EW1. Firmware and application: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, an executable desktop application, and routed carrier CAD. No physical board has been fabricated, programmed, electrically qualified, or timing-calibrated for this release. This is an observer of diagnostic indications, not an anti-rollback enforcement device, trusted attestation module, or certified security boundary.

## Purpose, novelty, and practical value

Epoch Witness checks a specific firmware-update contract: after a target announces a firmware generation, execution should only follow a completed announcement that does not decrease the highest generation already observed. The instrument listens to two generation bits, a commit indication, and an execution indication. It checks setup, stable hold, bounded commit duration, monotonic generation progression, and the ordering between commit completion and execution. It does not alter storage, supply power to the target, upload firmware, or attempt a boot bypass.

The motivating failure is an A/B update implementation whose recovery path accidentally selects an older security generation after a newer image has already been accepted. Logs produced by that implementation may omit the recovery branch or describe what the updater intended rather than what the execution gate actually selected. A separate observer can preserve a small, comprehensible record of the exported signals and expose contradictions without capturing code, credentials, personal information, or cryptographic keys. This is useful during owner-controlled update and recovery regression testing, especially when a firmware team can add slow diagnostic signals to a laboratory build.

Originality here is repository-local, not a patent claim. The existing directories and README purposes were inventoried before this concept was selected. BootROM Banshee explores boot behavior with storage interception and overlays; this instrument has no storage interface and cannot rewrite a manifest. Reset Quarantine observes reset settling, Grant Ledger observes consumption of authorization, Entropy Sentry observes source health, and Retry Ledger observes authentication failures and lockout. None of those supplies this generation-announcement state machine with a high-water mark surviving observation gaps. The passive carrier, board drivers, framing pattern, and desktop transport are deliberately reused from the repository's receive-only platform. The new contribution is the policy and evidence model, not a claim that four resistor inputs are a new electrical invention.

Two bits deliberately represent four ordered security generations, numbered zero through three. They are not the bottom bits of an arbitrary version number. A target adapter must map a bounded test campaign to these four ordered values without collisions or wraparound. Generation three is the upper end, never followed by zero as an ordinary rollover. More generations require a different interface and protocol. Keeping this limit explicit makes the test practical for small regression matrices while avoiding a misleading claim to observe a full hardware fuse counter.

## Attack surface, trust assumptions, and non-goals

The assessed surface is the target's update-selection and execution-authorization boundary. Relevant failures include announcing a lower generation, changing generation while commit is asserted, emitting an incomplete commit, executing before commit finishes, and executing a different generation from the one announced. An authorized adversarial test operator may request ordinary update or recovery flows on an isolated owned device; this instrument only observes their diagnostic outcomes.

The target's signal implementation remains trusted as an indication of internal behavior. Malicious firmware can lie on all four lines. A high execution indication does not prove instruction fetch from a particular flash region, and a low indication does not prove that execution stopped. Correlate findings with an independent trace or functional test before claiming a security exploit. A disconnected lead may look low because of the pulldown. The instrument cannot distinguish a broken wire from a legitimate low level and has no tamper loop or galvanic isolation.

The workstation, development board, programming interface, harness, and operator are trusted. The serial CRC detects accidental corruption, not forgery. An attacker can construct valid EW1 records, replace the observer firmware, or edit an exported JSON report. There is no device certificate, signed log, trusted wall clock, secure boot, persistent monotonic storage, or cryptographic authentication in this design. Protect physical access and serial permissions; do not run the app with elevated privileges unless the operating environment absolutely requires it. A local new-session action is not authentication of a physical instrument.

No wireless interface, attack transmitter, credential collector, autonomous exploit engine, persistent target modification, or hidden network service exists. The observer cannot prevent a bad boot. Its only transmitter is the MCU-to-workstation serial path. It is unsuitable for safety-critical equipment, production access control, or a claim of compliance based solely on its counters.

## Hardware specifications and bill of materials

The complete assembly consists of an external ST NUCLEO-F401RE, the passive carrier described by the native KiCad files, a labelled harness, and a Linux workstation. The STM32F401RET6 provides a Cortex-M4 core, 512 KiB flash, and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock and one-millisecond SysTick; it does not enable a PLL or require an external crystal. The NUCLEO supplies regulation, decoupling, reset circuitry, SWD programming, and the ST-LINK USB virtual serial bridge. Those are module facilities, not fictional parts omitted from the carrier schematic.

The carrier is 80 by 70 mm, two copper layers, and 1.6 mm nominal thickness. It contains fourteen through-hole components, nine nets, a closed edge outline, and routed connections. Tracks are 0.3 mm; a ground via connects the layer transition. There are no mounting holes, battery circuits, antennas, active sensors, isolated supplies, or target power outputs. Provide an insulated support and strain relief rather than letting the underside touch conductive equipment.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE development board |
| R1, R3, R5, R7 | 4 | 100 kΩ, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 straight through-hole header, 2.54 mm pitch |
| J2 | 1 | 1x4 straight through-hole header, 2.54 mm pitch |
| Harness | 1 set | Short insulated signal/ground leads, individually labelled |

The resistor footprint has 7.62 mm lead spacing and a DIN0207 body outline. Select actual purchased parts against these dimensions before ordering. The connector footprint is a standard vertical single-row pin header; mating height and enclosure clearance remain assembly checks. There is no stock or price guarantee.

J1 pins one and two are both ground: neither is a supply input. J2 pins one through four connect to NUCLEO CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0 respectively. J3 pin one is E0_IN, J4 pin one E1_IN, J5 pin one COMMIT_IN, and J6 pin one EXECUTE_IN. Pin two of each target connector is ground. The `_IN` names refer to the target side of each series resistor; names without that suffix are the MCU-side node. Connect NUCLEO ground to J1. Do not confuse this harness with a directly stackable shield.

Only common-ground, push-pull zero-to-3.3 V diagnostic outputs are supported. Each input sees a nominal 101 kΩ DC path to ground through its resistor pair. The series resistor does not make this input tolerant of 5 V, negative voltages, industrial transients, or unknown ground references. There are no dedicated ESD clamps or overvoltage disconnects. Exclude mains-related wiring, live field buses, relay coils, and signals whose electrical properties have not been measured. An independently designed and qualified front end is required for any other domain.

Power the development board through its documented ST-LINK USB connector after inspecting its power-selection jumpers. Reserve a conservative 5 V, 500 mA bench supply budget for the board and bridge; this is not measured consumption or a USB-compliance claim. Disconnect target signals before removing observer power to avoid back-power through MCU input structures. The carrier provides no reverse-polarity protection, isolated ground, or hot-plug guarantee. Electrical loading, startup current, shutdown behavior, thermal performance, and ESD tolerance remain bench validation tasks.

## Architecture and block diagram

```text
Owned target's slow diagnostic interface
 E0 --------1k-----+---- PA0 / A0   (J3 -> R2 -> J2.1)
 E1 --------1k-----+---- PA1 / A1   (J4 -> R4 -> J2.2)
 COMMIT ----1k-----+---- PA4 / A2   (J5 -> R6 -> J2.3)
 EXECUTE ---1k-----+---- PB0 / A3   (J6 -> R8 -> J2.4)
                  | each node has its own 100k pulldown
                 GND --- J1 --- NUCLEO GND --- USB host GND

GPIO sample -> independent qualification -> announcement state machine
                                      -> high-water mark + sticky findings
                                      -> bounded event queue + EW1 encoder
                                      -> nonblocking USART2 TX / PA2
                                      -> ST-LINK virtual COM -> Linux app
                                                         Epochs / Events / Integrity
```

PA0, PA1, and PA4 share GPIOA; PB0 is read from GPIOB immediately afterward. These are not simultaneous hardware latches. Do not infer submillisecond physical ordering from a qualified vector. The instrument is intentionally a slow diagnostic observer, not a tap for a processor-speed bus. Use an independent logic analyzer when qualifying signal timing and the target adapter.

## Exact firmware behavior and design decisions

Every input is independently qualified after a candidate value remains unchanged for three subsequent observed one-millisecond intervals. Thus a changed input needs its first observation plus three unchanged observations. Whole-vector debounce is avoided: execution chatter cannot indefinitely postpone commit recognition. Narrow pulses can be missed or rejected. Use generous laboratory margins, such as twenty-millisecond setup and high/low periods, rather than attempting to hit the sampling boundary.

The states are UNKNOWN, READY, and CAPTURE. Startup begins UNKNOWN without a historical generation. Once all inputs qualify and both raw-candidate and qualified COMMIT and EXECUTE remain low for twenty evaluated ticks, the engine enters READY. Generation bits need not be zero. This prospective baseline does not prove anything about earlier boots. Starting with COMMIT or EXECUTE asserted records startup activity and does not invent an edge. Arrange a documented target baseline before the experiment.

A qualified COMMIT rising edge in READY starts CAPTURE, invalidating any previous current announcement. The pending generation is the qualified binary value E1:E0. Setup requires at least ten stable evaluated ticks before that qualified edge. Raw generation disagreement breaks setup, even if it never becomes a qualified level. Setup is defined relative to qualified commit timing, not a hardware-measured setup time at the physical edge; account for qualification delay in the target fixture.

During CAPTURE, both candidate and qualified generation must match the pending value. A raw generation excursion invalidates the capture even if qualification would otherwise discard it. Commit must remain qualified high for at least ten and at most one thousand observed milliseconds. A falling edge at age one thousand is accepted if all other conditions hold. Age one thousand and one times out, invalidates the announcement, and returns to UNKNOWN until a fresh idle baseline is observed. A shorter pulse records a width fault. Holding COMMIT high cannot repeatedly announce generations.

On a well-formed falling edge, a first generation or a value equal to or above the high-water mark is accepted. Equal generations are permitted to model ordinary reboot of the same security release. A lower generation records a downgrade and cannot make current validity true. Generation progression is ordinary unsigned ordering from zero to three, never modulo arithmetic. The high-water mark advances only on accepted complete announcements; malformed high-valued announcements cannot poison it upward.

Execution is checked before commit completion changes validity. Consequently execution qualifying in the same sample as completion is recorded as premature, even if the announcement then completes correctly. Execution requires READY, current validity, COMMIT low, and matching current generation in both candidate and qualified inputs. A raw generation change invalidates current validity immediately once acquisition is qualified; changing back does not restore it. A new complete announcement is required. Sticky findings remain visible even after a later valid announcement, so the accepted counter is not a count of wholly safe trials.

A sampling jump other than one millisecond invalidates acquisition continuity, increments gap and missed-tick counters, clears current validity and qualification, and returns to UNKNOWN. It preserves the high-water mark and cumulative findings. A later idle baseline cannot erase a downgrade reference or repair missing evidence. Time itself wraps modulo 32 bits, while counters and elapsed ages saturate. No unobserved interval is counted as safe execution time. The event sequence wraps modulo 32 bits and counts attempted events; a full thirty-two-entry queue drops new events and increments a visible loss counter rather than blocking sampling.

The scheduler emits one boot record, periodic policy announcements approximately every second, status approximately every hundred milliseconds, and events in spare transmitter time. Policy records support late attachment without pretending a reboot occurred. The UART copies into a bounded buffer, advances only when TXE is ready, and abandons stalled transmission after one hundred milliseconds. There are no receive commands. A watchdog uses bounded initialization waits and main-loop feeding; its LSI-derived timeout is not a calibrated timing reference. A reset loses the RAM high-water mark, so the observer must remain powered across target resets and the operator must restart and document the campaign after an observer reset.

## Protocol and companion application

EW1 records have the form `EW1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC is four uppercase hexadecimal digits of CRC-16/CCITT-FALSE: polynomial 0x1021, initial 0xffff, no reflection, no final XOR, covering bytes before the asterisk. Values are unsigned 32-bit integers. The maximum accepted line is below 384 bytes. A full `EW1,` marker resynchronizes after a truncated line. CRC is never described as authentication.

B carries reset flags. P carries `10,10,1000,3,20`, representing setup, minimum commit, maximum commit, qualification, and baseline durations. E carries sequence, device time, kind, and value. Event kinds one through seven are ready segment, capture begin, accepted generation, downgrade, invalid capture, newly sticky fault bits, and missed acquisition ticks. Generation events contain a value zero through three. Fault bits zero through seven indicate setup, hold, width, timeout, rollback, unannounced execution, acquisition gap, and startup activity.

S carries, in order: time, phase, qualified, levels, valid, have_highest, highest, current, capture_age, faults, accepted, downgrades, invalid, execute_ms, gaps, missed, segments, dropped, and tx_fail. The current field retains the last accepted value even while validity is false. Capture age can retain the last attempt's age outside CAPTURE and must not be interpreted as an active timer then. Execution duration counts observed prohibited qualified-high ticks, not precise electrical pulse width.

The local Linux companion uses Python's standard library and Tk. This is appropriate for a tethered laboratory bench and avoids a mobile deployment dependency. Start `python3 app/audit.py`, select the exact serial terminal, and choose New live session. Epochs displays the phase, high-water mark, current validity, all counters, and named findings. Events displays every retained event up to a bounded history of 256 entries. Integrity shows provenance, connection, compatibility, freshness, parser errors, sequence losses, and freeze reasons. Disconnect retains the evidence; beginning another session replaces it, so export first.

The parser validates counts, CRC, numeric ranges, event values, and consistency of valid status with the generation and commit lines. It rejects impossible exposure increments and freezes on regressing counters, disappearing sticky findings, decreasing high-water marks, contradictory duplicate status or event records, incompatible policy, or an observed reboot after evidence. Identical duplicate status does not refresh freshness. The display becomes stale after one second without advancing accepted device time. Repeated policy cannot unfreeze a suspected reset.

Reset inference is necessarily incomplete. Without a persistent boot identity, a lost boot record followed by forward time and nondecreasing counters can look uninterrupted. Host receipt freshness does not establish physical acquisition continuity. Queued event times may precede the latest status without indicating a reset. Use short supervised trials, preserve reset notes, and begin a new session after transport discontinuities. The app does not claim continuous trusted capture through USB loss or adversarial substitution.

Replay accepts regular files only and reads at most two MiB plus one byte before rejecting an oversized capture. FIFOs and character devices are refused rather than blocking the UI. Export creates a new mode-0600 JSON file and refuses existing paths, including symlinks. Reports include author, protocol, source, connection, freshness, errors, freeze state, counters, and events. They are neither signed nor encrypted. There is no automatic cloud upload or background database; retain and delete reports according to the engagement's evidence policy.

## Builds, reproducibility, and validation

Run from this directory:

```sh
make -C firmware all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 tests/integration.py
/usr/bin/python3 tools/check_hardware.py
```

The embedded build requires GNU Arm Embedded GCC and newlib. It links actual Cortex-M4 startup and the STM32F401 memory map, not just a host substitute. Host tests require GCC with address and undefined-behavior sanitizers. The GUI requires Linux termios, Python Tk, and Xvfb for headless integration tests. CAD generation and checking require KiCad 9, its standard footprint libraries, and pcbnew. There are no downloaded app packages or dependency installation scripts.

The synthetic fixture uses the production policy and encoder. Generate a capture with `firmware/build/fixture > capture.ew1`, then replay with `python3 app/audit.py --replay capture.ew1`; add `--export new-report.json` for headless export. This produces test data, not a physical measurement. The fixture demonstrates an accepted generation two followed by a rejected generation one and prohibited execution. Unit tests exercise malformed timing, simultaneous completion/execution, preserved high-water mark across gaps, wrap, saturation, queue loss, encoder bounds, and noisy-input invariants. The real GUI event loop is exercised against a Linux pseudo-terminal, including hangup, all three screens, export, replay, and clearing stale summaries on reconnect.

The hardware generator creates native schematic symbols, wires, labels, footprints, routed tracks, and a connectivity manifest. The checker runs ERC and DRC, exports a fresh schematic netlist, and compares every pin/net, component value, and full footprint identity with the board and manifest. Passive-network checks do not validate the external module or actual harness. See VERIFICATION.md for executed outcomes; successful software and CAD checks are not a substitute for physical assembly review.

## Research workflows, bring-up, and authorized use

A product-security team can map four deliberately ordered test releases to the generation inputs and compare update/recovery behavior across revisions. A red team under written scope can ask the owner to exercise a documented older-image recovery flow while this observer records whether execution follows a downgrade. A penetration tester can attach a compact evidence report to a finding without collecting firmware content or credentials. All cases require the target owner to validate the meaning of the diagnostic signals and independently confirm functional consequences.

Before connecting valuable equipment, review the purchased module revision and harness, measure continuity, confirm common ground and voltage, and verify pin polarity with an oscilloscope. Bring up the observer disconnected from the target, then exercise each channel using a benign laboratory source. Confirm that inputs remain receive-only through reset and watchdog recovery. Compare observed timing with a calibrated reference, test power sequencing and USB removal, and inspect loading and ground currents. Recheck component dimensions and layout in KiCad before fabrication. Mechanical protection, EMI, ESD, oscillator tolerance, and temperature behavior are outstanding qualification items.

Manufacturer references are ST UM1724 for Nucleo-64/MB1136, RM0368 for STM32F401 registers, and DS9716 for STM32F401xD/xE electrical specifications. A cached UM1724 copy was used to cross-check CN8 mapping and the default ST-LINK USART2 path; fresh ST document downloads failed during this run. The design reuses the repository's STM32F401 register implementation. Verify the current manufacturer revisions, actual solder bridges, and absolute maximum ratings before physical use. No vendor documents are bundled or credited as original work.

Use only on equipment you own or have explicit written authorization to assess. Never connect to a live safety, medical, vehicle, industrial-control, or occupied-access system on the strength of this design alone. Observation can still damage electronics through bad wiring, ground differences, or back-powering. Establish scope, recovery procedures, and an evidence-retention agreement before a trial. This release provides a reproducible research prototype design, not permission to bypass access controls or a guarantee of security, electrical safety, or regulatory compliance.
