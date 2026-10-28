# Exclusion Ledger — Four-Domain Pairwise Grant-Collision Witness

Author and creator: jayis1. Hardware revision A. Protocol EL1. Firmware and application: GPL-2.0-only. Hardware generation/checking utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design. This directory provides target-linked embedded firmware, a working Linux desktop application, and routed native KiCad carrier files. See VERIFICATION.md for executed software and CAD checks. Fabrication, MCU programming, electrical qualification, calibrated timing, and physical deployment have not been performed. This is an observation instrument, not an access controller, isolation barrier, safety interlock, or certification of secure arbitration.

## Purpose and overview

Exclusion Ledger investigates a basic confidentiality and integrity assumption in a shared security resource: only one security domain should hold its grant at a time. Four owner-provided diagnostic indications expose the actual grants to four domains. The instrument records a separate ledger for each of the six possible pairs. Each ledger includes observed overlap entries, cumulative qualified overlap duration, longest observed segment, current segment age, and whether that pair is currently overlapping. This makes a specific conflicting pair visible rather than reducing every collision to a single alarm.

Consider a laboratory controller with application, recovery, maintenance, and audit clients sharing a protected resource. A transition bug might briefly preserve application access while enabling maintenance access. Another defect might leave recovery granted indefinitely while the audit client is admitted. A single overlap counter cannot distinguish these patterns. The pair ledger lets a product-security engineer compare which domain relationships changed after an arbitration fix, whether a collision is recurring or continuously held, and whether observation gaps limit the interpretation.

The device observes deliberately slow diagnostic outputs, not internal processor bus cycles. It cannot prove that an access occurred, identify instructions, inspect memory, recover credentials, or enforce exclusion. Diagnostic outputs must be mapped to the real grant state by the equipment owner. If they merely repeat software's intended state, the measurement may miss the very implementation defect being investigated. Independent functional tests and source review remain necessary.

## Repository-local originality

Existing device directories and README purposes were inventoried before implementation. A repository search for mutual exclusion and overlapping grants identified Starvation Window as a close neighbor; its detailed measurement contract was reviewed. That device studies two requesters, bounded waiting, peer bypasses, and secondary two-owner switching. Exclusion Ledger instead spends all four inputs on simultaneous grant relationships and builds six independently qualified overlap ledgers. It has no request inputs, fairness deadline, bypass metric, or owner handoff requirement.

Toggle Mailbox observes a two-phase request/acknowledgment transaction and payload lifetime. Slot Alias observes collisions of tagged outstanding identities. Neither supplies a four-domain pair incidence matrix with per-pair segment duration. Gray Audit interprets quadrature input sequences and is also a different measurement. The new contribution here is the multi-domain overlap evidence contract, independent pair eligibility, transactional accumulation, and its research workflow. Pairwise exclusion itself is established engineering; repository-local originality is not a worldwide prior-art or patentability assertion.

The carrier routing, STM32 register definitions, startup, UART service, main-loop scheduler pattern, and desktop transport reuse existing jayis1-authored work. Reusing a reviewed passive interface is intentional. The measurement engine, EL1 contract, domain-specific tests, and documentation define this device rather than simply renaming an existing policy.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written authorization to assess. Record the target, development firmware, diagnostic adapter source, connection points, permitted workload, operator, stop procedure, and evidence-retention period before attachment. Begin with an isolated bench fixture whose hazardous outputs are disconnected. Do not connect this unisolated prototype to occupied access-control systems, vehicles, medical equipment, emergency infrastructure, machinery, mains wiring, or unknown-voltage signals.

The measured attack surface is shared-resource grant arbitration between domains that should not operate concurrently. Relevant failures include a stale grant during context transfer, incomplete reset of one client, independently issued grants from competing control paths, and a cleanup race that leaves two permissions asserted. An authorized red team can request approved owner-controlled workloads that exercise these conditions. The device neither generates those workloads nor exploits the observed collisions. Its firmware has no received-command parser, target-facing output operation, radio, network service, or credential capture feature.

The target's diagnostic adapter, observer firmware, workstation, and harness are trusted components of the measurement. A malicious target can falsify all four indications. An attacker controlling ST-LINK can replace the firmware. A compromised workstation can fabricate or delete exported evidence. CRC detects ordinary corruption; it is not authentication, encryption, signed provenance, or anti-replay protection. There is no secure element, immutable boot identity, trusted wall clock, secure update mechanism, cloud storage, or remote management interface.

Receive-only behavior is enforced by this firmware configuration, not by an irreversible electrical data diode. Reprogrammed GPIO pins could drive the leads. Common ground, loading, shorts, miswiring, or back-power can disturb the target even with the intended image. A legal disclaimer does not make an unsafe connection safe. Stop immediately if attachment changes behavior. Author jayis1 makes no production-readiness, safety certification, or regulatory-compliance claim.

## Hardware specifications

The complete instrument consists of an external ST NUCLEO-F401RE development board, the passive carrier, labelled short insulated leads, nonconductive support, and a Linux workstation. The module's STM32F401RET6 Cortex-M4 provides 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI oscillator, one-millisecond SysTick, and USART2 transmission at 115200 baud, eight data bits, no parity, and one stop bit. PA2 sends telemetry to the module's ST-LINK virtual COM bridge.

The external development board provides regulation, MCU decoupling, reset, USB conversion, and SWD programming. These functions are not fictional components on the custom PCB. The carrier has no active IC, FPGA, radio, antenna, battery, charger, analog sensor, memory card, level translator, target power source, or actuator. It is connected by a harness; it is not a mechanically stackable Arduino shield.

| References | Quantity | Specification / purchasing candidate |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W axial; Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W axial; Yageo MFR-25FBF52-1K |
| J1, J3–J6 | 5 | 1x2 2.54 mm header; Samtec TSW-102-07-G-S |
| J2 | 1 | 1x4 2.54 mm header; Samtec TSW-104-07-G-S |
| Support and leads | 1 set | Insulated bench support and labelled wiring |

Purchasing candidates are not stock or availability guarantees. Check current drawings, body sizes, plating, and lead diameters before ordering. Native KiCad footprints use DIN0207 resistor bodies with 7.62 mm spacing and vertical through-hole headers. The carrier measures 80 by 70 mm, nominally 1.6 mm thick, with two copper layers, 0.3 mm tracks, and a closed outline. There are fourteen components, thirty electrical pads, and nine named nets. No mounting holes or enclosure CAD is included; provide an insulating fixture that prevents underside contact and harness strain.

### Pin map and electrical envelope

| Meaning | Target connector | MCU connector | Nucleo label / MCU pin | State bit |
| --- | --- | --- | --- | --- |
| GRANT0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| GRANT1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| GRANT2 | J5.1 | J2.3 | A2 / PA4 | 2 |
| GRANT3 | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only; neither pin is a supply. J2 has four signals and no ground contact. Each target connector passes through a 1 kOhm series resistor to an MCU-side node, with a 100 kOhm pulldown from that node to ground. Net names ending `_IN` are target-side nodes. Plain GRANT names are MCU-side nodes. Check pin numbers and continuity; do not infer wiring from connector orientation or wire color.

Only nominal zero-to-3.3 V common-ground push-pull diagnostic outputs are supported. No galvanic isolation, active input buffer, ESD qualification, surge clamp, fuse, reverse-current blocker, hot-plug guarantee, or field-voltage protection is supplied. A series resistor is not an overvoltage protection system. Five-volt outputs, negative voltages, 1.8 V domains, unknown industrial wiring, differential buses, relay coils, and mains are excluded. Keep leads short and establish compatible grounds before connecting signals.

The nominal resistive path to ground is 101 kOhm. At 3.3 V, calculated current is approximately 32.67 microamps per asserted channel, or 130.69 microamps with all four asserted, excluding input leakage and transient charging. These are arithmetic estimates, not measured consumption. Reserve a conservative 5 V, 500 mA USB source allowance for the Nucleo assembly, corresponding to 2.5 W; actual current depends on the module, jumpers, bridge, and USB limits. The passive carrier draws no separate supply current. Power the observer before target signals, and disconnect target signals before depowering it to avoid back-power through MCU protection structures.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136 boards, the STM32F401xD/xE datasheet, and RM0368 for peripheral registers. Check the actual board revision, Arduino A0–A3 mapping, ST-LINK USART bridges, and reset clock state before wiring. The source uses explicit register definitions rather than a downloaded SDK. Electrical thresholds, cable loading, power sequencing, ground offset, and oscillator tolerance remain physical acceptance tasks.

## Architecture and block diagram

```text
Owned target diagnostic adapter      Passive carrier       External Nucleo
  GRANT0 ---------------------------- 1k --+--------------- PA0 / A0
  GRANT1 ---------------------------- 1k --+--------------- PA1 / A1
  GRANT2 ---------------------------- 1k --+--------------- PA4 / A2
  GRANT3 ---------------------------- 1k --+--------------- PB0 / A3
                                         | each node
                                        100k
                                         |
  Common ground ------------------------ GND ------------- GND / USB

GPIOA then GPIOB reads -> independent channel qualification
  -> six pair predicates -> per-pair entry / duration / longest segment
  -> transactional counters + gap diagnostics + terminal exhaustion
  -> bounded EL1 CRC snapshots -> PA2 -> ST-LINK virtual COM -> USB
  -> Linux Ledger / History / Integrity screens -> explicit private JSON
```

GPIOA and GPIOB are read consecutively, not latched simultaneously. Events close to the sampling boundary can therefore be ordered differently from physical events. This is not a high-speed bus analyzer, metastability detector, or calibrated pulse-width instrument. Start with diagnostic plateaus of at least twenty milliseconds and compare them against a separate logic analyzer. Short events can be rejected or entirely missed. No amount of clean software testing establishes physical acquisition accuracy.

## Exact firmware measurement contract

Firmware is C11, with startup vectors, linker layout, GPIO and watchdog drivers, bounded UART service, measurement engine, and checked serializer. It uses no heap, operating system, filesystem, flash log, or command receiver. The image begins at 0x08000000; the linker reserves stack headroom. Measurements are volatile and reset with the observer. There is no persistent record of earlier boots.

Each input qualifies independently after the candidate remains unchanged for three subsequent one-millisecond observations. A newly observed level therefore requires its first observation plus three matching intervals. Qualification is independent: chatter on GRANT2 cannot prevent an established GRANT0/GRANT1 collision from accumulating. After first qualification, the last qualified value is held while another candidate settles. The instrument measures overlap of qualified indications, not raw electrical overlap. A sampled one-tick low excursion can be rejected without splitting a qualified overlap segment.

A pair is eligible as soon as both of its channels are qualified. There is no all-low baseline requirement because simultaneous high grants are directly observable at attachment. If attachment occurs during a collision, entries counts one observed segment, not the unknown physical start time. Pair order is 01, 02, 03, 12, 13, 23. Three high grants activate three pair ledgers; four high grants activate all six. These are pairwise relationships, not six independent physical incidents. Never sum pair durations and label the sum as elapsed wall time.

The first overlapping qualified observation increments entries and starts age at zero. Each subsequent consecutive overlapping observation increments both age and cumulative duration by one nominal millisecond. Maximum is the largest segment age observed, including an ongoing segment. The first nonoverlapping qualified observation clears active and age without adding a trailing interval. Consequently a segment consisting of a single qualified observation has an entry but zero duration and maximum. This convention counts only intervals bracketed by overlapping observations and avoids guessing at the departure interval.

Each pair sets a sticky finding bit on its first observed overlap. Findings do not halt acquisition: this instrument is meant to compare multiple collisions and pairs over a campaign. A return to exclusive operation clears current active flags but never clears historical entries, durations, maxima, or findings. The observed field counts fully qualified four-input sample vectors, not elapsed time or per-pair coverage. It can remain zero while a qualified pair accumulates evidence and another channel has never settled. Do not use observed as a denominator for a particular pair's percentage exposure.

Repeated calls within one tick are ignored. A nonzero timestamp difference other than one indicates an acquisition gap. Firmware increments gaps and missed by the skipped tick count, invalidates all qualification, and closes current segments without extending their durations across the unknown interval. Last qualified levels remain visible but valid becomes zero until requalification. A continuing physical overlap may create another observed segment after the gap. Thus entries counts observed segments, not guaranteed unique physical incidents. Sticky findings and established cumulative totals survive.

All affected fields update transactionally. If any cumulative counter or current age cannot advance without unsigned overflow, the speculative sample is discarded and the prior ledger enters terminal HALTED. Its active flags and ages are cleared, qualification is invalidated, and the exhaustion bit is set. Previously accumulated totals and maxima remain. Watchdog startup failure also halts before the first status. HALTED acquisition fields freeze; timestamp and saturating transport-failure diagnostics may continue. Ordinary consecutive 32-bit tick wrap is supported, but a full unseen timer cycle cannot be identified from a modulo timestamp alone.

## Protocol and companion application

The main loop samples before serving UART. A frame submission copies into a fixed buffer; each service call writes at most one ready byte. A stalled frame is abandoned after one hundred milliseconds and counted. Boot is announced once. Policy is announced initially and approximately every second, supporting late attachment without pretending the device rebooted. Status is offered approximately every hundred milliseconds. These are cumulative snapshots, not a complete per-edge recording.

Frames are ASCII `EL1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection, and no final XOR over bytes before the asterisk. The checksum is four uppercase hexadecimal digits. B contains the reset-register snapshot. P contains `3,1,4,6`: qualification intervals, sample period in milliseconds, input count, and pair count. Unsupported policies do not establish compatibility.

S begins with `now,levels,valid,halted,faults,observed,gaps,missed`. Six groups follow in pair order, each containing `entries,duration,maximum,age,active`. The last field is tx_fail. Every numeric value fits unsigned 32 bits. Fault bits zero through five identify pair findings; bit six is acquisition gap, bit seven exhaustion, and bit eight watchdog unavailable. The app names group fields entries0 through entries5 and similarly for the other metrics. A bit value of one is historical evidence, not necessarily a current overlap.

Run `python3 app/console.py` on Linux with Tk installed. Enter the exact device path, normally `/dev/ttyACM0`, and select New live. Ledger displays measurements and named pair findings. History retains up to 256 snapshots. Integrity displays source, live/replay provenance, compatibility, connection, age, parser errors, reset flags, and any freeze reason. Disconnect retains evidence. A successful new connection or replay replaces the session; export important evidence first. A failed connection preserves the existing model. No target command is transmitted, and the serial descriptor is opened read-only.

LIVE means compatible telemetry is progressing, not that acquisition is complete or a target is safe. It requires a connected session, at least two advancing accepted snapshots, and recent reception. Duplicate timestamps do not refresh freshness. Contradictory duplicates, backwards time, cumulative-counter rollback, disappearing findings, mutations after halt, and implausible duration growth freeze retained evidence. A reception pause or cumulative host/device timing divergence also freezes the session rather than treating queued old frames as current. These checks cannot identify every reset or fabricated stream; they are necessary consistency bounds, not a complete realizability proof or authentication system.

Replay is explicitly non-live. It accepts only regular files no larger than two MiB, opened nonblocking and checked before allocation. FIFOs and devices are refused. The parser bounds frame storage and resynchronizes on the complete EL1 marker. A partial tail at EOF increments parser errors; a recording without any compatible accepted status is rejected. Included fixture output comes from the production C encoder but is synthetic software evidence, not a physical capture.

JSON export creates a new private file with mode 0600, refuses overwrite, and rejects a final-path symlink. It retains author jayis1, provenance, parser errors, connection, freshness, reset information, freeze reasons, latest snapshot, and bounded history. Even empty exports retain session metadata. File permissions are not encryption. Choose a trusted destination and apply the engagement retention policy; deleting a file is not guaranteed secure erasure.

## Build, test, and hardware validation

Required tools are GNU Make, ARM GNU GCC with newlib/nosys support, native GCC, Python 3 with Tk, KiCad 9 CLI and Python bindings, standard KiCad footprints, and Xvfb for automated desktop tests. No mobile SDK, third-party Python package, or network dependency is required by the companion. From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
```

Target outputs are firmware/build/exclusion-ledger.elf and .bin. Programming requires supervised ST-LINK access and reset into the documented clock state; programming has not been performed. Host tests run the real engine and serializer with sanitizers, exhaust all input masks, compare random sequences with a separately implemented pair oracle, and exercise gaps, qualification independence, tick wrap, and exhaustion. UART tests model registers in a Linux process rather than exercising real silicon. Scheduler tests cover recurring policy after lost boot transmission and watchdog startup failure.

The app tests feed real C-encoder frames through Linux pseudo-terminals and a real Tk event loop. They exercise screens, disconnect, replay, private export, overwrite refusal, malformed and impossible records, duplicate timestamps, rollback, and stale/backlogged transport. KiCad checks run ERC and DRC with violations fatal, export a fresh schematic netlist, and compare every component reference, pin, value, footprint identity, and net against actual PCB pads and the saved manifest. Passive-symbol ERC cannot validate the external power source or certify input voltages.

## Research workflows and remaining acceptance

A security researcher can observe an owner-controlled four-client arbitration fixture under normal load, record that only one qualified grant is present, then compare the same approved concurrency workload after a suspected cleanup change. A red team can provide narrow evidence identifying which domain pair violated the agreed diagnostic contract without collecting application contents. A penetration tester can use the pair matrix to ask a vendor for a targeted explanation rather than presenting a generic collision count as proof of compromise. Reports should include adapter implementation, firmware identities, wiring, signal meanings, sampling limitations, and all acquisition gaps.

Before fabrication, review the schematic rendering, purchased part dimensions, and mechanical support. After fabrication, check continuity and shorts unpowered, then bring up the Nucleo without a target. Verify inputs remain inputs, measure loading and ground offset, inspect power sequencing, and compare paced known-good and conflicting grant patterns against a calibrated analyzer. Exercise all six pairs, three-way and four-way overlap, chatter on an unrelated channel, reconnect during an active collision, a sampling interruption, and observer reset. Keep the fixture's consequential outputs disconnected throughout initial work.

Physical oscillator accuracy, watchdog recovery, USB behavior with a real module, thresholds, back-power, ESD, EMC, temperature limits, enclosure suitability, and regulatory behavior remain unverified. The design supplies executable software and inspectable engineering source, not a claim that an unbuilt instrument is safe to deploy.

Original design, firmware, software, and documentation author: jayis1.
