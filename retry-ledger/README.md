# Retry Ledger — Authentication Retry-Budget and Lockout Evidence Bench

Author and creator: jayis1. Hardware revision A; protocol RL1. Firmware and companion application: GPL-2.0-only. Hardware scripts: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unvalidated physical prototype design with target-linked firmware, a functional Linux desktop application, and routed carrier CAD. Fabrication, programming, electrical qualification, oscillator calibration, and physical target operation have not been performed. This instrument observes diagnostic indications. It neither authenticates users nor enforces lockout and must never be used as a safety or access controller.

## Purpose and repository-local originality

Retry Ledger asks a specific security question: after a target reports enough unsuccessful authentication decisions, does its lockout indication arrive promptly and remain asserted, and does privileged activity continue while the retry budget is exhausted? Many security reviews inspect the login algorithm but miss the integration between an authentication task, a lockout timer, and a peripheral permission latch. A second processor can observe those boundaries without collecting passwords, identifiers, challenges, responses, or application payloads.

An authorized development target exposes four slow 3.3 V diagnostic signals: FAIL, SUCCESS, LOCK, and ACCESS. FAIL pulses when an unsuccessful authentication decision is completed. SUCCESS pulses when an accepted authentication decision is completed. LOCK stays high while the target claims that its retry lockout is effective. ACCESS indicates privileged activity. The instrument counts qualified rising outcome edges, tracks a prospective three-strike budget, and looks for access and lock-indication contradictions during a five-second observation interval. It never generates authentication attempts and does not include a brute-force client, credential capture, exploit delivery, target write path, radio, or remote administration service.

The repository was inventoried before implementation. The closest existing designs are Grant Ledger, which consumes single-use permission credit; Lease Watch, which relates heartbeat freshness to enabled outputs; Reset Quarantine, which measures settling after reset; and Entropy Sentry, which correlates random-source health with release. None maintains a cumulative failed-authentication budget, gives failure precedence over conflicting outcome indications, or evaluates lockout assertion grace and minimum hold time after exhaustion. Retry Ledger's novelty is this repository-local policy and evidence workflow, not a worldwide prior-art or patent claim. The passive electrical carrier and low-level NUCLEO board support are intentionally reused from designs already authored by jayis1. A different resistor layout would not improve the research question.

## Authorized use, attack surface, and threat model

Use only with equipment you own or are explicitly authorized in writing to assess. Agree on physical test points, diagnostic firmware modifications, allowed test accounts, interruptions, evidence retention, and stop conditions before connecting anything. Never attach this prototype to deployed door controls, medical equipment, vehicles, industrial actuators, emergency systems, or other safety-critical equipment. Use a sacrificial development target with hazardous outputs disconnected. Permission to test a web application is not permission to modify the associated hardware.

The intended adversarial condition is a defective target implementation: an off-by-one retry counter, a successful response that unexpectedly resets a locked budget, an early lockout timer expiration, or a peripheral that continues accepting privileged work after software says it is locked. The instrument treats the diagnostic mapping as an explicit assumption. A malicious target can lie on all four wires, omit failures, or assert LOCK without disabling anything. A compromised observer can also fabricate reports. Findings are candidates for corroboration with source review and independent instrumentation, not cryptographic proof of authentication or exploitability.

The trust boundary includes the harness, external development board, installed firmware, USB bridge, host operating system, local application, and operator. The serial CRC detects accidental corruption, not malicious forgery. RL1 has no authentication, pairing keys, encryption, signed evidence, secure boot, persistent boot identity, or signed update path. Physical SWD access can replace firmware. A hostile USB serial endpoint can supply syntactically valid records. Operate on a trusted, physically controlled bench; preserve firmware hashes separately if evidence handling requires them.

Receive-only describes the supplied firmware's target-facing behavior, not a hardware-enforced unidirectional barrier. MCU GPIOs remain programmable by replacement firmware. USART2 transmits only to the development board's ST-LINK virtual serial bridge. The companion opens the selected terminal read-only and never writes target commands. Even passive connections can disturb the target through loading, grounding, power sequencing, or wiring errors. Disconnect immediately if unexpected heating, resets, or voltage changes occur.

## Hardware specifications and component selection

The complete assembly is a NUCLEO-F401RE development board plus a passive, wired carrier and a Linux workstation. The external MCU is STM32F401RET6, Cortex-M4, with 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, no PLL, and a nominal one-millisecond SysTick. The NUCLEO provides the regulator, decoupling, reset circuit, ST-LINK programming interface, and USB serial bridge. These circuits are external module facilities, not nonexistent components on the carrier schematic.

The carrier is an 80 by 70 mm, two-layer, 1.6 mm PCB with fourteen through-hole components and nine nets. Copper tracks are 0.3 mm wide; the design uses a through via and a closed board outline. There are no mounting holes: provide an insulated mechanical support and strain relief instead of leaving the board on conductive equipment. Radios, antennas, batteries, battery chargers, isolated supplies, active sensors, external storage, and target power outputs are absent by design.

| Reference | Quantity | Specification and role |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, STM32F401RET6 board |
| R1, R3, R5, R7 | 4 | 100 kΩ, 1%, axial DIN0207, input-node pulldown |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial DIN0207, series input resistor |
| J1, J3–J6 | 5 | Straight 1x2, 2.54 mm through-hole header |
| J2 | 1 | Straight 1x4, 2.54 mm through-hole header |
| Harness | 1 set | Short insulated leads with labels and shared-ground pairs |

For a sourcing example, the resistors can be Yageo MFR-25FBF52-100K and MFR-25FBF52-1K, subject to checking their current dimensional drawings against the DIN0207 footprint. Header examples are Samtec TSW-102-07-G-S and TSW-104-07-G-S; check mating length and enclosure clearance before ordering. These examples are not an availability guarantee or an independently qualified purchasing list.

J1 pins 1 and 2 are both GND; neither is a supply input. J2 pins 1, 2, 3, and 4 connect respectively to NUCLEO CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0. J3 is FAIL, J4 SUCCESS, J5 LOCK, and J6 ACCESS. Each target header uses pin 1 for signal and pin 2 for GND. The four target signals are active high. Signal names without `_IN` are MCU-side nodes; names with that suffix are target-side connections before the series resistors.

Only common-ground, push-pull, zero-to-3.3 V diagnostic outputs are supported. The series resistor and pulldown form an approximately 101 kΩ load at DC; nominal high-state current is about 33 µA per asserted channel. They do not provide galvanic isolation, comprehensive ESD protection, overvoltage protection, or immunity to ground loops. Do not connect 5 V logic, negative voltages, relay coils, field buses, mains-related signals, unknown interfaces, or separate earth references without an independently designed and qualified front end.

Power the NUCLEO through its documented ST-LINK USB connector and verify power-selection jumpers on the actual board revision. Do not combine external supplies casually. Disconnect target signals before removing instrument power, because MCU input structures can back-power an unpowered board. Four nominal 3.3 V asserted inputs dissipate approximately 0.43 mW total in the resistor networks; the development board and USB bridge dominate system consumption. Reserve a conservative 5 V, 500 mA bench USB supply budget, but do not interpret this as a measured draw or proof of USB enumeration compliance. Startup, peak draw, leakage, regulator temperature, and total assembly consumption remain laboratory measurements to perform. No battery-runtime or thermal-qualification claim is made.

## Architecture and block diagram

```text
Authorized target: slow diagnostic outputs, no payload collection
 FAIL ------1k----+---- PA0 / A0      J3 -> R2 -> J2 pin 1
 SUCCESS ---1k----+---- PA1 / A1      J4 -> R4 -> J2 pin 2
 LOCK ------1k----+---- PA4 / A2      J5 -> R6 -> J2 pin 3
 ACCESS ----1k----+---- PB0 / A3      J6 -> R8 -> J2 pin 4
                 | each node has a separate 100k pulldown
                GND ------- NUCLEO GND ------- USB host GND

NUCLEO: GPIO reads -> independent input qualification
                         -> budget / lockout state machine
                         -> sticky findings and bounded event queue
                         -> RL1 CRC framing -> nonblocking USART2 PA2
                                                    |
                                          ST-LINK virtual COM
                                                    |
Linux desktop: Budget | Events | Integrity -> private JSON export
```

GPIOA and GPIOB are sampled sequentially rather than simultaneously latched. ACCESS resides on GPIOB; the other inputs share GPIOA. Do not infer submillisecond ordering or a physical edge's exact time from a qualified observation. Host receipt time includes serial and operating-system delay and is never a substitute for device observation time. Use an independent logic analyzer when qualifying the fixture or investigating a boundary condition.

## Exact policy and firmware decisions

Each channel is qualified independently after its candidate level remains unchanged for three observed one-millisecond intervals following a change. A newly changed level therefore needs the initial observation plus three subsequent unchanged samples. Unrelated input chatter does not postpone another channel's qualification. Narrow pulses may be rejected or missed entirely. Design fixture outcome pulses and low intervals to last at least ten milliseconds; this is a deliberately slow diagnostic interface, not a direct tap on a processor-speed authentication bus.

There are four phases: UNKNOWN, TRACKING, LOCKOUT, and RECOVERY. Initial acquisition starts UNKNOWN. Once every channel has qualified and the qualified and current candidate vectors remain all-low for twenty evaluated ticks, the engine opens a prospective observation segment with zero strikes. This baseline does not prove that the target has no historical failures. Arrange a documented target-state reset before starting the experiment and record it separately. Otherwise report only behavior within the newly observed segment.

In TRACKING, each qualified FAIL rising edge increments the strike count and lifetime failure count. Holding FAIL high does not repeatedly consume retries. A qualified SUCCESS rising edge resets strikes only while tracking, while FAIL is low, and while LOCK is low. Overlapping FAIL and SUCCESS levels set a conflict finding; failure takes precedence. Success never clears sticky findings or lifetime counters. A lock assertion outside an exhausted budget does not itself create a lockout cycle, but ACCESS while LOCK is high remains prohibited and SUCCESS while LOCK is high is rejected.

The third counted failure enters LOCKOUT immediately at its qualified observation time. The policy requires LOCK to be qualified high by age twenty observed milliseconds. Absence at that age or later sets the missing-lock finding and accumulates observed missing-lock ticks. Once LOCK has been seen during this cycle, a qualified low before completion sets early-release evidence, including a release qualifying on the exact expiry tick. That conservative endpoint rule avoids treating a same-tick release as proof of later ordering. The interval runs for five thousand observed milliseconds from budget exhaustion, not from delayed lock assertion.

Additional failures during LOCKOUT or RECOVERY increment a separate blocked-failure counter without restarting the timer. The name describes the policy phase, not proof that the target actually blocked the authentication request. Success during those phases sets a finding and cannot reset strikes. ACCESS high during either phase, UNKNOWN, or any qualified LOCK high sets access evidence. Access is evaluated after processing failure edges but before lockout expiration, so access coinciding with the third failure or the expiry sample is not silently accepted. A cycle count means budget exhaustion occurred; it is not a count of successful secure lockouts.

At expiry the engine enters RECOVERY. It requires the same all-low twenty-tick baseline before reopening TRACKING. A held success, asserted lock, repeated failures, or active access therefore prevents recovery. There is no software command that clears the evidence. Starting a new app session clears only that application's retained host view; resetting the MCU restarts device counters and must be documented by the operator.

Any sample-time jump other than one millisecond invalidates continuity, increments gap and missed-tick counters, resets qualification, and returns to UNKNOWN. It does not pretend that unseen time was safe or count missing time toward the lockout. Lifetime findings remain sticky. A later baseline opens a new prospective segment; it cannot repair the earlier gap. A naturally wrapping unsigned millisecond timer is supported, but arbitrary backward jumps are treated as gaps. Cumulative counters saturate rather than wrap. Event sequence numbers wrap modulo 32 bits. A bounded thirty-two-entry queue discards new events when full and increments a loss counter without blocking acquisition.

The main loop emits a boot record once, a policy record immediately and approximately every second, status every hundred milliseconds, and queued events when the transmitter is otherwise free. Policy repetition supports late attachment without falsely announcing a reboot. UART transmission copies frames into a bounded buffer, advances only when TXE is ready, and abandons a stalled frame after one hundred milliseconds. The next complete marker permits host resynchronization. The independent watchdog is configured with bounded initialization waits and fed by the main loop. Its timeout depends on the uncalibrated LSI oscillator; it is a recovery measure, not a timing reference or a certification claim.

## Protocol and companion application

RL1 uses ASCII records: `RL1,<kind>,<unsigned fields>*<four uppercase CRC hex digits>` followed by LF. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xffff, no reflection or final XOR, over the bytes before the asterisk. All numeric fields are unsigned decimal 32-bit values. Maximum accepted record size is below 384 bytes. The app validates syntax, field counts, ranges, CRC, phase constraints, monotonic cumulative counters, sticky masks, and exposure increments bounded by device-time advancement.

B contains reset flags. P contains `3,5000,20,3,20`: strike limit, lockout duration, assertion grace, channel qualification duration, and baseline duration. E contains sequence, device milliseconds, kind, and value. Event kinds 1 through 8 mean segment opened, failure, accepted success, budget exhausted, lock seen, interval expired, newly sticky fault bits, and acquisition gap. S contains, in order: time, phase, qualified, levels, strikes, lock age, faults, failures, successes, cycles, blocked failures, access milliseconds, missing-lock milliseconds, gaps, missed ticks, segments, dropped events, and transmitter failures. The fault mask uses bits 0 through 6 for missing lock, early release, prohibited access, conflicting outcomes, locked success, acquisition gap, and unknown activity.

The application uses Python's standard library and Tk rather than a mobile framework because a local Linux serial console is appropriate for a bench instrument. Launch `python3 app/audit.py` from this directory. Select the exact terminal path and choose New live session. The Budget screen displays phase, strikes, interval age, counters, and named sticky findings. Events shows the bounded event history rather than only the newest record. Integrity displays source provenance, connection status, parser errors, event sequence gaps, compatibility, freshness, and freeze reasons. Disconnect preserves evidence and its original live/replay label; a new live session intentionally clears the old view, so export first.

The interface becomes stale after one second without advancing accepted status time. Repeated identical frames do not refresh freshness. Conflicting same-time records, regressions, disappearance of sticky findings, incompatible policy, or an observed boot after evidence freeze the session. Export remains available. There is no persistent boot identity: if a boot frame is lost and a later post-reset record has forward time and nondecreasing counters, the restart can be indistinguishable from uninterrupted telemetry. This limitation is deliberate and prominent: RL1 does not certify continuous capture across resets, USB loss, or adversarial interference. Keep trials short, physically supervise resets, and begin a new capture after any transport discontinuity. Event timestamps may precede a recent status because queue delivery is delayed; they are not incorrectly used as uptime regressions.

Replay accepts only regular files, reads at most two MiB plus one byte, and rejects oversized inputs. A FIFO or character device is not a replay source. Export creates a new JSON file with mode 0600 and refuses existing paths, including symlinks. The report includes author, protocol, source, connection state, freshness, warnings, parser loss information, current counters, and retained events. It is neither signed nor encrypted. Retain only agreed evidence and delete reports with ordinary operating-system tools; no hidden cloud storage, automatic uploads, or background database exists.

## Build, tests, and reproducible examination

From the device directory run:

```sh
make -C firmware all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 tests/integration.py
/usr/bin/python3 tools/check_hardware.py
```

The target build requires GNU Arm Embedded GCC and newlib. Host tests require GCC with AddressSanitizer and UndefinedBehaviorSanitizer. The desktop requires Python 3 with Tk and Linux termios; the headless GUI test additionally requires Xvfb. Hardware generation and checking require KiCad 9, its standard footprints, and its Python pcbnew module. There are no downloaded application dependencies or package-install scripts. The firmware links directly against the STM32F401 memory map and includes startup code and its linker script; this is not merely a host-only simulator.

`tools/generate_hardware.py` deterministically regenerates the carrier schematic, embedded/local symbol library, project, PCB, and connectivity manifest. The checker runs ERC and DRC, exports a fresh schematic netlist, and compares every reference/pin/net, value, and footprint identifier to the actual PCB pads and the manifest. A parseable file alone is not sufficient. Inspect the rendered schematic and the PCB in KiCad before fabricating. Automated passive-network checks do not validate the external NUCLEO, target voltage, harness, isolation, supply configuration, or a real solder joint.

The C fixture emits synthetic records using the production engine and encoder. Use `firmware/build/fixture > capture.rl1`, then `python3 app/audit.py --replay capture.rl1`, or add `--export report.json` for a headless report. These are generated test scenarios, not physical measurements. The test suite covers budget exhaustion, successful reset, overlapping outcomes, held inputs, early release, missing lock, prohibited access, independent-channel chatter, gaps, timer wrap, counter saturation, queue loss, serial timeout/recovery, CRC, parser bounds, and actual encoder compatibility. See VERIFICATION.md for executed outcomes and tool versions.

## Practical research workflows and remaining qualification

A product security team can instrument an owned authentication component and compare successive firmware revisions using the same scripted laboratory outcomes. A red team can demonstrate a policy integration weakness without collecting live credentials: the target owner drives documented failed logins while the observer records whether privileged test activity continues during lockout. A penetration tester can attach an evidence snapshot to a finding about inconsistent retry enforcement, while clearly distinguishing observed diagnostic indications from verified access to a protected resource. None of these uses requires transmitting or automatically attacking a third-party system.

Before a real trial, inspect the unpowered harness, measure continuity, confirm target ground and voltage, and verify the NUCLEO board revision and connector assignment. Independently check pulse widths and polarity with an oscilloscope. Bring up the observer disconnected from the target first, then use a benign laboratory signal source to validate each channel. Compare nominal millisecond timing against a calibrated reference over temperature and supply variation. Verify startup and shutdown sequencing, USB unplug behavior, watchdog reset behavior, ESD precautions, enclosure clearance, and any effect of input loading. Review the electrical design with another qualified engineer before using valuable equipment.

Manufacturer references for this implementation are ST's UM1724 Nucleo-64 board manual, RM0368 STM32F401 reference manual, and DS9716 STM32F401xD/xE datasheet, available from the NUCLEO-F401RE and STM32F401RE product documentation pages at st.com. Confirm the current document revision and actual purchased board revision before assembly; no downloaded datasheet is bundled or falsely attributed. Remaining physical work is not concealed by a successful compiler, a long README, or zero CAD-rule violations. The released artifact is a coherent research prototype design with software and CAD evidence, not a production-ready security instrument.
