# Coincidence Guard — Bilateral Consent Timing Evidence Bench

Author and creator: jayis1. Hardware revision A; protocol CG1. Firmware and application: GPL-2.0-only. Hardware generator and checker: MIT. Carrier hardware: CERN-OHL-S-2.0.

Status: engineering prototype design with executable firmware, companion application, and routed CAD. Physical assembly, programming, timing calibration, and electrical qualification have not been performed. This is an observation instrument, not a safety controller, access controller, certified two-hand control, or proof of independent human consent.

## Purpose, originality, and useful questions

Coincidence Guard observes two consent indications, the resulting permission indication, and an inhibit indication. It answers a temporal question that an ordinary truth table cannot answer: did both consent inputs become asserted within a bounded interval after an observed all-low baseline, and did permission remain within that accepted pair? A held input must not be reused across a release of the other input. Inhibit invalidates a pair and requires a fresh baseline. The device reports violations and uncertainty without driving the equipment under examination.

The practical setting is a development bench for privileged maintenance consoles, dual-operator release fixtures, laboratory key ceremonies, or local administrative approval hardware. A product might advertise two-party approval while its controller actually accepts an indefinitely held first button followed by a second button hours later. Another might preserve permission after one consent is withdrawn. This instrument makes those observable policy differences explicit, reproducible, and exportable. It does not identify who pressed a button, inspect cryptographic keys, or establish that two people are present.

Repository inventory preceded this design. Quorum Witness evaluates three voters against a majority output; it does not require fresh paired edges within a window. Grant Ledger studies consumption of a single authorization. Lease Watch relates heartbeat freshness to enable. Handoff Audit checks break-before-make domain transitions, while Erase Witness checks a sanitization handshake. Coincidence Guard instead implements a bilateral timing and anti-tie-down contract with a separate inhibit. Its novelty is this repository-local combination of observable contract, bounded evidence, and inexpensive bench hardware, not a claim that coincidence circuits or two-hand controls are new inventions worldwide.

The passive resistor carrier and low-level NUCLEO infrastructure deliberately reuse jayis1's established repository topology. A novel instrument does not require inventing a new USB bridge or resistor footprint. The state machine, input meaning, protocol policy, screen labels, and test vectors are specific to this device. It is a standalone device subfolder and does not alter a shared hardware tree or any other repository.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Agree on diagnostic wiring, shutdown authority, retained evidence, timing policy, and scope with the owner before connecting anything. Never attach this prototype to a deployed safety circuit, industrial machine control, mains wiring, vehicle interlock, medical equipment, or any circuit whose malfunction could harm people. A connector marked permission is an observation input, not a control output. No amount of software testing substitutes for electrical approval of the actual installation.

The threat under study is a controller or integration defect that grants a privileged function from stale, noncoincident, withdrawn, or inhibited approvals. The attacker model includes someone able to hold one local consent input, operate a second approval path later, or exploit a controller's release ordering. The instrument itself does not generate those stimuli. Researchers must use an owned test fixture or approved diagnostic facilities and keep hazardous actuators disconnected.

Trust boundaries exist between the observed product and diagnostic signals, between those signals and the common-ground carrier, between the MCU and ST-LINK serial bridge, and between the bridge and the host application. A compromised product can lie on diagnostic pins. A malicious host or substituted serial device can forge records. CRC detects accidental corruption, not an adversary. No secure element, signature, authenticated session, trusted wall clock, immutable log, or independent identity sensor is present. Physical custody and independent corroboration are necessary when evidence matters.

The shipped firmware only reads target-facing pins. This is a software configuration property, not galvanic isolation or an irreversible hardware data diode: reprogrammed MCU firmware could drive those GPIOs through the series resistors. Do not claim malicious-firmware resistance. The application opens serial read-only and implements no target commands, configuration upload, radio functions, credential capture, persistence, or remote management. There is no arming action because the instrument has no intended active target function; connecting it still loads the signals electrically.

## Hardware specifications and bill of materials

The processor is an external NUCLEO-F401RE development board, MB1136 family, carrying an STM32F401RET6-class LQFP64 MCU with 512 KiB flash and 96 KiB SRAM. Firmware uses its reset 16 MHz HSI clock and Cortex-M4 core, not the board's maximum clock. ST-LINK/V2-1 supplies programming and the USB virtual serial bridge. The custom PCB contains only connectors and resistors; it does not duplicate the development board's regulator, MCU, decoupling, reset circuit, or USB design.

| Assembly | Quantity | Specification |
| --- | ---: | --- |
| NUCLEO-F401RE | 1 | External processor and USB bridge; verify actual board revision |
| R1, R3, R5, R7 | 4 | 100 kilohm, 1%, axial DIN0207, 7.62 mm pitch, at least 0.125 W |
| R2, R4, R6, R8 | 4 | 1 kilohm, 1%, same axial footprint and power rating |
| J1, J3, J4, J5, J6 | 5 | Straight 1x2 male 2.54 mm headers, matching insulated cable sockets |
| J2 | 1 | Straight 1x4 male 2.54 mm header |
| Carrier PCB | 1 | Two layers, 80 by 70 mm, nominal 1.6 mm FR4 |
| Harness | 1 set | Short insulated leads, labeled conductors, strain relief outside PCB |

J1 pins 1 and 2 are both ground. Neither is a supply pin. Connect one to a NUCLEO ground. J2 pins 1 through 4 connect respectively to NUCLEO CN8 A0/PA0, A1/PA1, A2/PA4, A3/PB0. The external connectors J3, J4, J5, J6 each use pin 1 for their signal and pin 2 for ground. They represent CONSENT_A, CONSENT_B, PERMIT, INHIBIT respectively. All four logical inputs are active high. Verify header orientation against the square pad before wiring; do not rely on cable color.

Each external signal reaches its GPIO through 1 kilohm. A 100 kilohm resistor pulls the GPIO side to ground. This provides a defined disconnected-low tendency and modest series impedance, not open-wire detection or overvoltage protection. A disconnected inhibit wire can therefore appear uninhibited. A disconnected consent can look like withdrawal; neither result proves what the product did. Check the harness independently and retain wiring photographs with the assessment.

Only ground-referenced 0 to 3.3 V push-pull diagnostic outputs are allowed. Exclude 5 V, negative voltages, relay coils, open-drain signals without a separately engineered pull-up, and unknown field wiring. There are no TVS diodes, isolation components, fuses, buffers, or level translators. The series resistor must not be described as an absolute-maximum-rating solution. Keep the powered target disconnected whenever the NUCLEO is unpowered to avoid injection through input protection structures. Disconnect signal leads before removing USB power.

The carrier has no supply rail and no battery. Power the NUCLEO only through its intended ST-LINK USB connection for this reference setup; do not combine USB and external supply arrangements without consulting the board manual. Target and USB grounds are common. Ground-potential differences can damage equipment or invalidate measurements. USB power consumption is dominated by the development board and must be measured on the actual assembly; this release specifies no unmeasured runtime or current budget. Radio, FPGA, analog sensor, wireless charging, and battery charging hardware are absent.

## Architecture and block diagram

```text
Owned fixture diagnostic outputs (3.3 V only, short common-ground harness)
 A ----------1k----+---- PA0 / A0
 B ----------1k----+---- PA1 / A1
 PERMIT -----1k----+---- PA4 / A2
 INHIBIT ----1k----+---- PB0 / A3
                  | each GPIO node has its own 100k pulldown
                 GND -------- NUCLEO ground -------- USB host ground

NUCLEO-F401RE:
 HSI -> SysTick -> sampled-vector qualifier -> consent state machine
                                             | counters and event queue
                                             v
                              bounded CRC encoder -> USART2 PA2 TX
                                                        |
                                                  ST-LINK USB VCP
                                                        |
 Linux read-only serial -> bounded parser -> Overview / Events / Health
                                             -> exclusive private JSON export
```

There is no connection from host commands to target pins. GPIOA and GPIOB are read sequentially, not with simultaneous hardware capture. Millisecond sampling and whole-vector qualification are appropriate for slow consent indications, not high-speed race analysis. Clock drift, port-read skew, cables, signal edge shape, and operating-system USB latency prevent interpreting receipt times as precise target edge times.

## Exact observable consent contract

Firmware samples at nominal one-millisecond intervals. A complete four-bit input vector must appear on three consecutive acquired ticks before becoming qualified. Changes in any input restart qualification of the whole vector. A pulse between samples can be missed, and changes shorter than qualification may intentionally be rejected. State names describe this observation model, not physical guarantees.

UNKNOWN is the initial state and the state following a missed acquisition interval. An all-low qualified vector establishes READY. Any other initial vector locks the monitor; attaching with buttons already held cannot manufacture a fresh approval. INHIBIT high always leads to LOCKED. Leaving LOCKED requires all four signals low together for a qualified baseline. Permission remaining high prevents rearming, even if both consents were released.

From READY, A alone enters WAIT_A and B alone enters WAIT_B. The name records which input arrived first. If both consents qualify together, the monitor accepts a pair with zero measured skew. Otherwise the second consent must qualify while the first remains high, no later than 250 measured milliseconds after the first qualified edge. Exactly 250 is accepted; 251 is expired. Timeout is checked even when inputs do not change. A partly qualified second edge does not extend the deadline.

An exchange from A-only to B-only, or the reverse, without an all-low qualified vector is rejected. This is the anti-tie-down rule: a controller cannot reuse an old held input merely because the other one toggled. Timeout enters LOCKED and increments expired. A malformed waiting sequence increments rejected. Removing both consents and permission together with inhibit low creates a new baseline rather than forcing an unnecessary extra state transition.

PAIRED persists only while both consents remain asserted and inhibit remains low. Permission may rise, fall, and rise while the same pair remains held; that is deliberately one continuous consent session, not a single-use grant. Withdrawing either consent or asserting inhibit revokes the pair and increments revoked. Revoked counts every termination of an established pair, including normal release; it is not itself a count of attacks. The pairs counter measures accepted consent pairs and does not assert that their permission timing was safe.

Permission is checked against both the previous accepted state and the incoming qualified vector. If permission and a new pair appear together, unsafe increments because observation does not prove that permission followed approval. The pair itself is still counted. Permission concurrent with withdrawal or inhibit also counts unsafe immediately on qualification. Unsafe counts observed episode beginnings, not every millisecond of an asserted violation; it clears its episode latch after a later qualified vector no longer exhibits unauthorized permission. A held violation across an acquisition gap may be counted again because continuity is unknown.

An acquisition jump larger than one tick increments gaps and missed, invalidates qualification, clears pair history, and requires a new all-low baseline. It never assumes that the unobserved interval contained stable consent. Last qualified levels remain available with valid false. Counters and state ages saturate at the unsigned 32-bit maximum; uptime and event sequence wrap. Ordinary uptime wrap is supported by modular subtraction. An outage spanning a full counter cycle cannot be reconstructed. Skew retains the last accepted pair's measurement until another pair, not the age of the current approval.

## Firmware structure and recovery

firmware/consent.c contains the deterministic acquisition, qualification, transition, and evidence logic. wire.c formats bounded ASCII records without heap allocation or printf. drivers/board.c owns GPIO input setup, SysTick, USART configuration, reset flags, and watchdog initialization. drivers/serial.c transmits one bounded copied packet nonblockingly. startup.c and linker.ld provide the actual Cortex-M4 reset path and memory placement. registers.h names the STM32 register addresses used by those drivers; board.h fixes the carrier contract.

The sole interrupt is SysTick incrementing a volatile tick counter; queue and state-machine ownership remains in the main loop. An event queue stores 32 entries. Full queues drop new events, increment dropped, and still advance sequence numbers. Status is nominally scheduled every 100 ms, policy every second, and events use remaining opportunities. Status therefore remains available during heavy event activity, although transport failure can still prevent delivery. The application does not infer event completeness from a quiet screen.

UART uses 115200 baud, eight data bits, no parity, one stop bit, transmit only. A stalled packet is abandoned after 100 ms and increments tx_fail. No incoming command parser exists. The watchdog uses the independent low-speed oscillator with a conservative reload; exact timeout depends on oscillator tolerance and must be measured. If watchdog setup fails, firmware does not start producing measurements. Reset discards RAM evidence and returns to UNKNOWN; the instrument cannot retain observations through power loss.

There is no field updater or signed boot chain. Program the ELF or binary only with a physically controlled ST-LINK workflow, then perform a hardware reset so the documented clock assumptions hold. Arbitrary debugger jumps from software with changed clocks are unsupported. Do not treat CRC, USB ownership, or the boot announcement as firmware authentication. Production hardening would require a separate threat model and hardware validation, not simply enabling an app password.

## Companion application and protocol

The companion is a Linux desktop-terminal application using Python standard-library curses, termios, select, and JSON. Run `python3 app/audit.py --port /dev/ttyACM0 --export assessment.json` from this directory, choosing the actual port explicitly. Keys 1, 2, and 3 select Overview, Events, and Health. The e key exports; q exits. No package manager or cloud account is needed. Permission errors are reported instead of changing device-node permissions automatically.

The Overview shows qualified inputs, state, age, last-pair skew, and policy counters. Events shows the latest sixteen of up to 256 retained records. Health shows acquisition and transport loss plus parser errors. LIVE means a connected transport, compatible policy, and a recent status less than one second old with no suspected reset. It does not mean safe permission, valid acquisition, a trustworthy target, or a gap-free log. Replay can never be LIVE. Unplug preserves visible evidence but marks the connection inactive; restart the application explicitly to reconnect.

Records have the form `CG1,type,decimal-fields*CCCC` followed by LF. CCCC is uppercase CRC-16/CCITT-FALSE over bytes before the asterisk, polynomial 0x1021 and initial value 0xffff. B carries reset flags, window 250, and qualification 3. P repeats window and qualification without clearing history. Repetition lets a late subscriber recover policy knowledge after losing the boot record. Unsupported policies freeze status ingestion until a supported announcement arrives.

S fields are time, levels, valid, state, age, skew_ms, pairs, rejected, unsafe, expired, revoked, gaps, missed, dropped, tx_fail. State codes are UNKNOWN=0, READY=1, WAIT_A=2, WAIT_B=3, PAIRED=4, LOCKED=5. E carries sequence, device time, kind, value. Event kinds are levels=1, state=2, unsafe=3, rejected=4, expired=5, revoked=6, pairs=7, missed ticks=8. All values are unsigned decimal integers; boolean and enum fields receive additional bounds checks.

The parser validates CRC, field count, version, numeric range, and event kind. It bounds incomplete records and resynchronizes on a full CG1 marker after a truncated transfer. Event timestamps can legitimately precede the most recent status because of queueing. Reset inference therefore uses ordered status timestamps and cumulative counters, not delayed event time. Regression freezes evidence until an explicit boot or new application session. A policy record alone cannot clear that suspicion. Some resets remain indistinguishable if both timestamp and counters happen to move forward; no persistent boot identifier is implemented.

For offline processing use `python3 app/audit.py --replay capture.txt --headless --export replay.json`. Replay is limited to sixteen MiB. Export exclusively creates a mode-0600 JSON file and refuses existing paths or final-component symlinks. Select a trusted parent directory. The report includes author jayis1, source provenance, replay flag, connection, freshness, receipt age, reset suspicion, policy compatibility, parser errors, counters, and bounded events. No raw capture, telemetry, or automatic upload occurs. Unexported evidence disappears on exit, and exported evidence must be deleted under the engagement's retention policy. Files are neither encrypted nor signed.

## Build and validation workflow

Run `make -C firmware clean all test` for target linking and host regression tests. Run `python3 -m unittest discover -s tests -p test_app.py -v` and `python3 tests/integration.py` for parser, export, serial lifecycle, and interactive application verification. Integration uses real C-encoder output but synthetic inputs; separate pseudo-terminals exercise the serial and curses paths. It is not a claim that a USB board was connected.

Use KiCad 9 with its standard footprint library. `/usr/bin/python3 tools/generate_hardware.py` generates native schematic, PCB, project, local symbols, connectivity manifest, and XML netlist. `/usr/bin/python3 tools/check_hardware.py` runs ERC and DRC and compares every schematic reference, value, footprint, and pin/net to actual PCB pads and the manifest. The carrier has routed copper, vias, a closed outline, and actual passive component symbols. The NUCLEO remains an external assembly, explicitly represented by its harness interface rather than a fictitious on-carrier MCU.

Inspect VERIFICATION.md for executed checks and tool versions. CAD checks do not prove external voltage safety, mechanical suitability, correct purchased parts, or manufacturing readiness. Before assembly independently review footprints, mounting and strain relief, clearances, soldering access, and ground connections. Before target use measure input loading, verify no back-power paths in permitted sequencing, check thresholds, clock tolerance, acquisition jitter, disconnect behavior, watchdog reset, and long-duration counter behavior. ESD, EMC, regulatory and environmental validation remain outstanding. No fabrication-ready or physical-performance claim is made.

## Research examples and reporting limits

A product-security engineer can compare an intended bilateral approval policy with diagnostic output during approved bench scripts. Test a normal pair with generous timing margins, the exact window boundaries in simulation, a tied-down first input, a hand exchange without baseline, inhibit during permission, and consent withdrawal. Use an independent logic analyzer on real hardware. If the controller emits permission in the same qualified vector as the second input, classify ordering as unproven rather than assuming the observer resolved submillisecond causality.

A penetration tester can use these observations to verify remediation of stale-approval reuse without collecting credentials or modifying protected storage. Record firmware revisions, fixture version, mapping of diagnostic pins to actual enforcement, cable arrangement, and policy assumptions. A legitimate finding states that a specified observable contract was violated under a reproducible stimulus, not that the instrument proved user identity or defeated a cryptographic system.

An authorized red team working with the product owner can demonstrate integration assumptions in a collaborative laboratory exercise: two independent UI paths may share one stale latch, inhibit may not revoke permission, or reconnect may preserve old approvals. The bench makes those differences discussable without covert deployment. A nonzero error count can also arise from wiring, qualification, timing mismatch, or a dishonest diagnostic mapping. Missing evidence is inconclusive, never a pass. Preserve all uncertainty fields in reports and repeat ambiguous tests with better instrumentation.

## Manufacturer references

Consult ST UM1724, STM32 Nucleo-64 MB1136 user manual, especially Arduino connector mapping and USART solder bridges. CN8 A0 through A3 are the four harness inputs; PA2 is routed to ST-LINK virtual COM with the standard bridge arrangement. Consult RM0368 for STM32F401 peripheral registers and the STM32F401xD/xE datasheet for electrical limits. The ST cmsis-device-f4 STM32F401xE header provides an additional register-map cross-check. These references describe the external processor assembly; they do not certify this carrier or the fixture connected to it.
