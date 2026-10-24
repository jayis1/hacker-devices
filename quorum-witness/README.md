# Quorum Witness — Three-Voter Permission Integrity Bench

Author and creator: jayis1

Hardware revision: A

Protocol: QW1

Status: software-built research prototype; physical validation outstanding

Firmware and application: GPL-2.0-only; hardware generator utilities: MIT

## Purpose and overview

Quorum Witness observes a narrow but consequential security boundary: three independent permission votes and the final enable decision that is supposed to implement their majority. A product may claim that compromising one subsystem cannot authorize maintenance access, enable a diagnostic path, or release a protected operation. That architectural claim is difficult to assess from a single application log. This device provides an independent, electrically receive-only record of whether the externally exposed decision agrees with the observed two-out-of-three voting rule.

The instrument reads VOTE_A, VOTE_B, VOTE_C, and ENABLE on an owned low-voltage development fixture. It identifies an enable asserted without a majority, a withheld enable despite a majority, and prolonged disagreement by an individual voter. It separates output-policy failures from dissent that a correctly implemented majority can tolerate. Its useful result is evidence for a design review, not a claim that the observed votes were authentic or that the enabled function was actually accessed.

This is a new concept within this repository, not a patentability or global novelty claim. The existing Handoff Audit checks mutually exclusive ownership and break-before-make timing. Grant Ledger checks single-use authorization consumption. Lease Watch checks heartbeat freshness. Quorum Witness instead evaluates a three-party majority and attributes persistent minority episodes to specific voters. It deliberately reuses jayis1's passive four-input carrier topology, startup, UART transport, and console foundations; those reusable parts are not represented as new inventions. The majority engine, dissent semantics, protocol schema, screens, and regression vectors define this device's distinct behavior.

## Legal, ethical, and operational limits

Use only on hardware you own or have explicit written permission to assess. Obtain approval for the exact diagnostic pins, ground connection, observer power source, test vectors, retained records, and interruption procedure. Restrict initial operation to an isolated development fixture with no connection to production actuators. This is not a safety instrument, certified voting relay, protective interlock, or emergency shutdown controller. Never place it in a life-safety, medical, vehicle, occupied-building, or industrial protection loop.

There is no RF transmitter, packet injection, target command interface, credential collection, credential guessing, exploitation payload, persistence mechanism, or target-power control. The firmware configures all four target-facing pins as inputs. The only transmit path is an unrelated USART output toward the Nucleo board's ST-LINK interface. The companion application opens its serial descriptor read-only and has no command screen. These constraints reduce accidental influence but do not make electrical attachment harmless: shared grounds, resistive loading, back-powering, and assembly mistakes remain real risks.

An authorized red team should use this instrument during an approved laboratory evaluation of trust separation. Generate scenarios through legitimate fixture controls or reviewed target test firmware. Do not repurpose the observer to force permission votes or attach it covertly. The author, jayis1, provides this reference without warranty of fitness, safety, regulatory compliance, or manufacturing readiness. The operator is responsible for consent, law, safety planning, and accurate reporting.

## Attack surface and threat model

The measured asset is the integrity of a diagnostic majority-to-enable contract. The assumed adversarial failure is one voter stuck high, stuck low, or changing incorrectly; an implementation fault in the combining logic; or an output that does not follow the decision. Two cooperating malicious voters are outside what majority observation can distinguish from legitimate consensus. The observer cannot prove that three wires represent three independently secured authorities. Three outputs from one compromised process can look perfectly consistent.

The target-facing trust boundary is four single-ended, nominal 3.3 V logic outputs with a shared ground. Inputs must already be exposed as suitable diagnostic signals. Do not connect bus lines, differential pairs, relay coils, switch contacts carrying voltage, or unknown test points. There is no isolation and no dedicated overvoltage or ESD barrier. A malicious or miswired source can destroy the observer or influence the host through common ground. The series resistors are impedance elements, not permission to exceed MCU electrical limits.

The host-facing boundary is the ST-LINK virtual serial port. A malicious host with access to the programmer can replace firmware; a malicious serial source can fabricate valid-looking records. CRC checks detect corruption, not forgery. Port names and USB identifiers do not authenticate a device. There are no signing keys, secure boot claims, encrypted exports, or tamper-evident logs. Keep the instrument and workstation physically controlled, record firmware provenance, and corroborate significant findings with an independent scope trace.

The app treats incoming bytes as untrusted. It limits framing memory, field counts, integer ranges, event values, replay size, and retained history. Output files are created exclusively with private permissions. No upload or analytics endpoint exists. The app displays transport freshness and continuity warnings rather than translating a clean-looking snapshot into a universal security verdict. A disconnected wire is biased low and cannot be distinguished from a legitimate low vote without additional fixture-side diagnostics.

## Hardware specifications and bill of materials

The compute module is an external ST NUCLEO-F401RE development board, MB1136 family, containing the STM32F401RET6 in LQFP64. Firmware uses the reset internal 16 MHz HSI clock, Cortex-M4 Thumb instruction set with software floating-point ABI, 512 KiB flash, and 96 KiB RAM. No FPGA, radio, battery, display, analog sensor, or expansion bus is required. The companion computer provides the display and evidence storage.

The supplied PCB is a passive, two-layer, 1.6 mm FR-4 carrier, 80 by 70 mm, wired to the Nucleo rather than represented as a pin-compatible shield. It contains fourteen actual components: five two-pin 2.54 mm headers, one four-pin 2.54 mm header, four 100 kOhm pull-down resistors, and four 1 kOhm series resistors. Resistors use axial DIN0207 footprints with 7.62 mm pitch; specify 1% tolerance and a suitable ordinary quarter-watt rating for assembly. Connector footprints are straight through-hole pin headers. Provide insulated harnesses and strain relief outside the bare board; mounting holes and an enclosure are not included.

J1 has ground on both pins. J2 pins one through four connect to Nucleo A0/PA0, A1/PA1, A2/PA4, and A3/PB0 respectively. J3.1 is VOTE_A_IN, J4.1 VOTE_B_IN, J5.1 VOTE_C_IN, and J6.1 ENABLE_IN. Pin two of every target header is ground. R1, R3, R5, and R7 are 100 kOhm pull-downs on the MCU side. R2, R4, R6, and R8 are 1 kOhm series elements between the target header and MCU-side net. The board carries no supply rail and does not generate 3.3 V for the target.

At a nominal 3.3 V high level, each series/pull-down path draws about 32.7 microamps and places about 3.267 V on the unloaded MCU-side node. This estimate ignores leakage, tolerance, source impedance, and cable capacitance; check actual input margins against the MCU datasheet and target output specification. The carrier's resistor dissipation is very small under intended conditions, but that calculation says nothing about fault survival. Reserve a conservative USB source budget of 500 mA for the complete development board while respecting the board manual's actual power configuration. Total current and thermal behavior have not been measured.

Power the Nucleo through its intended ST-LINK USB connector. Confirm board revision, Arduino labels, and USART bridge routing against ST UM1724 before assembly. The usual virtual-COM route uses PA2/USART2_TX and the default ST-LINK solder-bridge arrangement; inspect the physical board rather than assuming it is unchanged. Start from hardware reset with the HSI reset clock. Ground the fixture and observer before signal attachment, power the observer before enabling target outputs, and remove signals before removing observer power. Never rely on the observer's input protection structures to absorb a powered target's output.

## Architecture and block diagram

```text
Owned 3.3 V diagnostic fixture       Passive carrier             External MCU
VOTE_A --> J3.1 --- R2 1k ---+--- J2.1 ---------------------> A0 / PA0
VOTE_B --> J4.1 --- R4 1k ---+--- J2.2 ---------------------> A1 / PA1
VOTE_C --> J5.1 --- R6 1k ---+--- J2.3 ---------------------> A2 / PA4
ENABLE --> J6.1 --- R8 1k ---+--- J2.4 ---------------------> A3 / PB0
                            | each MCU-side node
                       R1/R3/R5/R7 100k
                            |
Ground ---------------------+--- J1 ----------------------> Nucleo GND

GPIO samples -> three-sample vector qualification -> majority/dissent engine
             -> bounded event queue + periodic policy/status records
             -> USART2 TX on PA2 -> ST-LINK USB virtual serial
             -> Linux overview / event history / health screens
             -> operator-selected private JSON evidence snapshot
```

All four bits are qualified as one vector. GPIOA is read before GPIOB, so ENABLE is not captured atomically with all votes. Inter-port skew, interrupt latency, oscillator tolerance, and physical input delay have not been characterized. Use deliberately paced diagnostic levels, initially held for at least ten milliseconds and considerably longer when evaluating the episode thresholds. This is not a high-speed logic analyzer and cannot certify absence of short authorization glitches.

## Firmware behavior and decisions

Production firmware consists of C11 source, explicit startup vectors, a linker memory map, board/register definitions, GPIO and UART drivers, the quorum engine, and a bounded text encoder. It has no dynamic allocation, command parser, filesystem, or network stack. Every target-facing pin remains an input. USART2 is configured for transmit only at 115200 baud, eight data bits, no parity, and one stop bit. SysTick provides the nominal millisecond time base. A watchdog is fed only by the main loop; oscillator and watchdog-configuration readiness waits are bounded.

The qualification rule requires three matching vectors on consecutive distinct millisecond observations. Repeated calls within one tick do not advance qualification. A change to any input restarts qualification for the entire vector. Consequently a chattering input can delay recognition of every other input, and brief changes can be intentionally rejected. The device reports the last qualified vector, not instantaneous raw voltage. These semantics must be included when interpreting timing results.

For qualified votes A, B, and C, majority is high if at least two are high. Dissent is a three-bit mask identifying inputs opposite that majority. With three Boolean inputs at most one bit is set. ENABLE high with majority low starts an unsafe episode; ENABLE low with majority high starts a denied episode. Each uninterrupted mismatch reaching fifty milliseconds increments its counter once and sets its active alarm bit. Agreement clears that active mismatch timer and alarm, but historical counters and maximum duration remain.

Each minority identity has an independent dissent timer. One hundred milliseconds of continuous qualified dissent increments that voter's count once. A correct output can therefore coexist with a dissent warning: redundancy is containing a fault, rather than necessarily producing an incorrect authorization. Rotating dissent from voter A to voter B does not create one continuous faulty voter. Conversely, an output mismatch with unchanged direction continues across vote changes that leave the majority unchanged. This distinction prevents unrelated input transitions from concealing a prolonged wrong output.

Each tick advances the preceding qualified interval before accepting a newly qualified vector. Recovery exactly at an episode threshold therefore does not erase the just-completed interval. Durations and evidence counters saturate at unsigned 32-bit maximum; timestamps and event sequence numbers wrap intentionally. The maximum mismatch is the longest observed continuous mismatch of either direction, not a sum. Per-voter maxima similarly record longest continuous dissent, not total time in the minority.

A sampling interval other than one millisecond invalidates continuity, increments gaps and missed ticks, and cancels active interval timers. That first post-gap sample does not count toward requalification. Three subsequent consecutive observations establish a new baseline, even if the vector is not all-low. No pre-gap elapsed time is credited to the new interval. Existing counts, maxima, and the last qualified levels remain available. A persistent condition can count again after a gap; those are separate observed episodes, not proof that the physical condition cleared between them. Always inspect the health counters with the fault totals.

The queue holds thirty-two events. When full it drops new events and increments dropped; sequence allocation still advances. Status snapshots have priority every hundred milliseconds, policy announcements every second, and remaining opportunities drain queued events. The transmitter copies at most 384 bytes into its own buffer and never blocks acquisition waiting for readiness. A stalled transfer is abandoned after one hundred milliseconds and increments tx_fail. Partial packets may appear on the host; the parser resynchronizes at the next complete QW1 marker.

Reset clears volatile evidence. The independent watchdog has a nominal approximately two-second period with significant internal-oscillator tolerance. There is no persistent boot identifier, replay protection, or nonvolatile evidence ledger. A full 32-bit timer-cycle stall cannot be reconstructed reliably; the watchdog is intended to reset an ordinary stalled loop much earlier. Compilation and host tests do not establish watchdog behavior on silicon.

## Companion application and wire interface

The companion is an appropriate Linux terminal application using standard-library curses, termios, select, and JSON. It needs no package installation. Run `python3 app/audit.py --port /dev/ttyACM0 --export assessment.json` from this folder. Select the actual port explicitly. Automatic discovery and authenticated pairing are not implemented. Permission errors are reported; no privilege escalation or automatic serial-permission modification occurs. Disconnect preserves the displayed record and disables LIVE. Exit and restart to reconnect or select another device.

Press 1 for the quorum overview, 2 for event history, 3 for health, e to export, and q to exit. The overview shows each qualified input, majority, dissent mask, active alarms, episode totals, current durations, and maxima. Event history shows sequence, device milliseconds, kind, and value. Health displays parser errors, observed boot sessions, connection and replay state, sampling gaps, missed ticks, dropped events, and UART failures. There are no active controls because the instrument is an observer.

LIVE means a connected transport, supported policy, no suspected restart, and a valid status received within one second. It does not mean that qualification is valid, samples were lossless, input voltages were correct, or the target is secure. Policy announcements allow late attachment even when the initial boot record was lost. An unsupported policy blocks new status and events until a supported policy arrives. An unannounced backward modular status timestamp freezes evidence as a suspected restart; policy alone cannot clear that warning. Queued event timestamps are deliberately excluded from this heuristic. Some resets remain indistinguishable if their first observed timestamp moves forward.

Records are ASCII `QW1,type,fields*CCCC` followed by LF. CRC-16/CCITT-FALSE covers the bytes before the asterisk, with initial value 0xffff and polynomial 0x1021. The checksum has four uppercase hexadecimal digits. All numeric fields are unsigned decimal and bounded to 32 bits. B contains reset flags, mismatch threshold 50, dissent threshold 100, and qualification count 3. P repeats only those three policy constants without clearing the current session.

S carries twenty-three fields in this exact order: time, levels, valid, majority, dissent, alarms, unsafe, denied, dissent_a, dissent_b, dissent_c, mismatch_ms, max_mismatch, gaps, missed, dropped, tx_fail, dissent_ms_a, dissent_ms_b, dissent_ms_c, max_dissent_a, max_dissent_b, max_dissent_c. Levels assign bits zero through three to A, B, C, and ENABLE. Alarms assign bits zero and one to unsafe and denied, and bits two through four to dissent A, B, and C. Invalid qualification means displayed logic fields are historical and current timers have been cleared.

E carries sequence, time, kind, and value. Kind 1 is qualified levels; 2 unsafe episode; 3 denied episode; 4, 5, and 6 dissent A, B, and C; 7 sampling gap; and 8 qualification established. Episode values are cumulative counts, gap values are missed ticks, and readiness value is one. Event history is bounded to 256 rows. Sequence values help investigate losses but are not cryptographic evidence of completeness.

Replay files are limited to sixteen MiB and always remain labeled not live. Use `python3 app/audit.py --replay capture.txt --headless --export replay.json` for offline processing. Exports include jayis1 authorship, source, receipt age, connection and replay provenance, policy knowledge, restart warning, latest status, boot metadata, parser errors, and bounded event history. Exclusive creation refuses existing destinations and final-path symlinks; file mode is 0600. Choose a trusted export directory. Exiting discards unexported memory. Filesystem deletion is not guaranteed secure erasure; apply your engagement's retention procedure.

## Builds, design checks, and acceptance

Run `make -C firmware clean all test`. This cross-links the actual Cortex-M4 image and executes native policy, encoder, UART-register-model, and main-loop tests. The intended flash address is 0x08000000; use ST-LINK with a physically controlled programming workflow followed by hardware reset. No hardware was programmed during this software verification. The host UART test maps ordinary process memory at register addresses; it validates software branches, not peripheral electrical behavior.

Run `python3 -m unittest discover -s tests -p test_app.py` and `python3 tests/integration.py`. Unit tests cover malformed frames, bounds, freshness, restart inference, policy incompatibility, retained history, screens, and exclusive export. Integration uses frames from the real C encoder, replays them through the actual CLI, and drives the real curses application through separate serial and terminal pseudo-terminals. Synthetic fixture data is explicitly not a capture from physical hardware.

Generate the CAD with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9 and its standard footprint libraries. Run `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC, and a fresh schematic-netlist comparison against PCB pads, values, footprint identities, and the manifest. The schematic contains embedded locally authored resistor and connector symbols; the PCB has actual routes, vias, an outline, and nine electrical nets. The external Nucleo is represented by its harness connector rather than an invented bare-MCU implementation. Passive-symbol ERC cannot validate external supplies, pin voltages, USB wiring, or target grounding.

Before fabrication, visually review the schematic and board, verify footprint dimensions against purchased components, and define enclosure access and strain relief. After assembly, test continuity, resistor values, shorts, ground offset, source loading, input thresholds, and back-power behavior on a sacrificial fixture. Then characterize actual sample timing, minimum detectable pulse, qualification latency, serial interruption, watchdog reset, and sustained fault sequences. ESD, EMC, calibration, thermal limits, regulatory suitability, and environmental reliability remain outstanding. These limitations do not disappear because the digital checks pass.

## Practical research workflows and interpretation

A product-security engineer can expose three independently implemented maintenance-permission votes and the selected access-enable indication on a development build. Start with all-low, exercise each single vote, then all pairs and unanimous permission. A correct majority output should reject every single vote while accepting each pair. Deliberately keep one voter opposite the other two through the target's approved test interface. The expected result is dissent attributed to that voter without an output mismatch when the voter-combining logic is correct.

A penetration tester can compare baseline and patched builds after discovering that one subsystem's permission incorrectly bypasses a majority gate. Preserve the fixture revision, firmware commit, pin mapping, observation duration, loss counters, and independent trace alongside the exported snapshot. The useful claim is that a specific diagnostic contract held or failed under stated stimuli. Do not extrapolate from a quiet counter to absence of exploitable races, authenticity of votes, or resistance to two compromised authorities.

An authorized red team can use the bench to make architectural assumptions visible during a collaborative review instead of collecting credentials or manipulating production access. Dissent can identify a shared implementation mistake or a diagnostic polarity error just as readily as an adversarial component. Investigate it rather than labeling a component malicious. Repeating the same paced vectors after remediation creates a reproducible regression check with very little sensitive data retained.

Authoritative electrical references are ST UM1724 for the MB1136 Nucleo-64 board, ST RM0368 for STM32F401 register behavior, and the STM32F401xD/xE datasheet for operating limits. Review editions matching the actual board and silicon. This repository's schematic, firmware, and tests define the prototype contract; they do not replace manufacturer electrical specifications or physical engineering qualification.
