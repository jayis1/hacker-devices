# Handoff Audit — Break-Before-Make Security-Domain Evidence Bench

Author and creator: jayis1. Revision A engineering prototype.
Firmware and companion application: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

This project provides linked STM32 firmware, a functional Linux terminal application, and a routed KiCad carrier for an external NUCLEO-F401RE. It is an unbuilt engineering prototype, not a certified isolation tester or a fabrication-qualified instrument. Software execution and CAD checks are recorded in VERIFICATION.md; electrical measurements, assembly, programming, and physical operation remain unverified.

## Purpose and repository-relative originality

Handoff Audit asks whether a controller transfers ownership between two mutually exclusive security domains without briefly connecting both. Examples include a development-board storage selector, a laboratory debug-port ownership switch, or a bench multiplexer separating a maintenance controller from an application controller. A correct software decision does not necessarily mean the underlying gates have opened and closed in the intended order. Independently observing command and feedback signals helps distinguish an unsafe command sequence from a slow or stuck physical interface.

The instrument receives four low-speed 3.3 V logic signals: command A, command B, feedback A, and feedback B. It counts command-overlap episodes, feedback-overlap episodes, cross-domain transfers with insufficient observed dead time, and persistent command/feedback disagreement on either side. It does not switch the domains, inject traffic, decode credentials, emulate an identity, or attempt exploitation. Its output is evidence about the supplied diagnostic signals under a declared sampling contract, not proof of galvanic isolation or confidentiality.

The repository's device directories and README titles were inventoried before selection. The nearest designs are Grant Ledger, which consumes single-use authorization credit; Lease Watch, which checks heartbeat freshness against enabled outputs; Gray Audit, which examines quadrature transitions; and Latch Ledger, which observes mechanical tamper contacts. None of those implements paired command/feedback break-before-make measurements. Handoff Audit instead measures the all-off interval between different observed feedback paths and distinguishes command overlap from feedback overlap. The carrier topology, startup code, UART driver, and console infrastructure deliberately reuse jayis1's existing patterns. Novelty is relative to that inventory, not a worldwide prior-art, patentability, or commercial uniqueness claim.

## Authorized use and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Obtain permission for the exact test points, any target firmware instrumentation, physical connection, expected interruptions, and retention of timing evidence. A penetration-testing agreement does not automatically authorize modifying every circuit at a site. Keep this observer visible and labeled. Do not use it covertly, to intercept private content, or to bypass a deployed security control.

Restrict initial work to sacrificial development fixtures with hazardous actuators disconnected. Never connect this carrier to mains switches, medical systems, occupied access-control installations, vehicle controls, high-energy battery paths, or operating industrial machinery. It has shared ground, no isolation barrier, and no protection suitable for those environments. Even receive-only firmware can disturb equipment through loading, ground loops, incorrect sequencing, or a wiring mistake. Stop if the target changes behavior after attachment.

A finding should be corroborated with a scope, target source review, and the owner's specified switching requirements. An overlap in two diagnostic GPIOs is not automatically exploitable cross-domain access. Conversely, a clean run cannot establish that no private information crossed another unobserved interface. Report uncertainty, sampling gaps, and the limitations of the target's feedback instrumentation alongside any count. The author, jayis1, does not authorize misuse or guarantee suitability for a safety function.

## Threat model and trust boundaries

The trusted setup comprises the operator, reviewed firmware image, physical wiring, development board, and workstation. Potentially untrusted inputs include the target's four signals, serial bytes, and saved captures. A target can lie about its own internal state. Feedback derived from the same software variable as the command is weak evidence; use a genuinely independent, electrically compatible status output when available. Neither the firmware nor the application can discover whether feedback actually represents a switch contact, a gate output, or a fabricated indication.

The target pins remain inputs from initialization onward. Only PA2 transmits, toward the Nucleo's ST-LINK virtual COM port. USART reception is disabled, and there is no firmware command parser, radio, network service, credential store, flash journal, or remotely controlled output. Startup, USB disconnect, malformed host data, and application exit do not arm an active target interface. The application opens the serial device read-only and offers no write controls.

CRC detects accidental stream corruption; it is not authentication, a digital signature, or replay prevention. A malicious USB device or capture file can fabricate valid records. There is no pairing secret, signed bootloader, secure update mechanism, hardware attestation, or anti-rollback promise. Programming occurs through physically controlled ST-LINK. Review and hash images using the laboratory's normal process; do not describe the resulting record as cryptographic evidence. A compromised workstation can alter exports regardless of file permissions.

## Hardware specification and bill of materials

The external processor board is NUCLEO-F401RE, containing STM32F401RE, a Cortex-M4 with 512 KiB flash and 96 KiB SRAM. The firmware uses the reset-default 16 MHz HSI clock, SysTick at a nominal one millisecond, and USART2 TX at 115200 baud, eight data bits, no parity, one stop bit. There is no PLL initialization or downloaded SDK. ST-LINK provides USB programming and serial conversion. Regulation, MCU decoupling, USB termination, debug circuitry, and the processor itself belong to the separately purchased development board, not the passive carrier.

The carrier is an 80 by 70 mm two-layer PCB, 1.6 mm thick. It contains fourteen through-hole components: four 100 kΩ pull-down resistors, four 1 kΩ series resistors, five two-position headers, and one four-position header. Use 1% quarter-watt axial resistors with 7.62 mm lead spacing and 2.54 mm-pitch headers. There is no battery, charger, radio, FPGA, active level shifter, optocoupler, analog sensor, regulator, or connector for target power. No enclosure or mounting-hole design is supplied; use an insulating bench support and strain relief.

| Signal | Target connector | MCU-side connector | Nucleo label / processor pin |
|---|---|---|---|
| Command A | J3 pin 1 | J2 pin 1 | A0 / PA0 |
| Command B | J4 pin 1 | J2 pin 2 | A1 / PA1 |
| Feedback A | J5 pin 1 | J2 pin 3 | A2 / PA4 |
| Feedback B | J6 pin 1 | J2 pin 4 | A3 / PB0 |
| Ground | J3–J6 pin 2 | J1, both pins | GND |

R2, R4, R6, and R8 are series resistors from the target connectors to the MCU-side nets. R1, R3, R5, and R7 pull those MCU-side nets to ground. The net names ending in `_IN` are on the target connector side of the series resistor. J1 is ground only: neither pin supplies power. J2 carries only the four observed signals. Its ordering is a wiring contract, not a claim that the carrier physically stacks onto the Arduino headers.

Use common-ground 3.3 V CMOS diagnostic outputs. Do not attach 5 V TTL, negative-voltage serial, differential buses, raw coils, externally energized relay contacts, or outputs from independently isolated domains. This observer would join their grounds and could destroy the isolation being investigated. The phrase security domain describes a logical access boundary in a common-ground development fixture, not separate hazardous voltage domains.

At nominal 3.3 V the series/pull-down path draws approximately 33 microamps per high input. That is a circuit estimate, not a measured loading specification. Check target drive impedance, MCU leakage, cable capacitance, input thresholds, and margin before use. The pull-down cannot distinguish a disconnected cable from a legitimate low. Series resistance is not an overvoltage or ESD safety barrier. There is no dedicated clamp, fuse, or abuse-resistant protection network.

Power the Nucleo through its intended ST-LINK USB connection. Reserve a conservative 500 mA source budget while respecting the manufacturer's actual USB configuration and limits; actual current consumption has not been measured. Inspect board revision, Arduino pin labels, and USART solder bridges against ST UM1724. Its default virtual-COM arrangement uses SB13/SB14 enabled and SB62/SB63 disabled. Ground both systems first, power the observer before enabling target outputs, and disconnect signals before removing observer power. Input protection structures can otherwise back-power the MCU.

## Architecture and block diagram

```text
Owned development fixture                 Passive carrier
CMD_A --> J3.1 ---- 1k ----+---- J2.1 --> A0 / PA0
CMD_B --> J4.1 ---- 1k ----+---- J2.2 --> A1 / PA1
FB_A  --> J5.1 ---- 1k ----+---- J2.3 --> A2 / PA4
FB_B  --> J6.1 ---- 1k ----+---- J2.4 --> A3 / PB0
                         | each
                        100k
                         |
Ground ------------------+---- J1 ----> Nucleo GND

GPIO observation -> whole-vector qualification -> handoff engine
                 -> bounded events + periodic health/status/policy
                 -> USART2 TX -> ST-LINK USB serial
                 -> Linux overview / event / health screens
                 -> operator-selected private JSON snapshot
```

GPIOA is read before GPIOB, so feedback B is not captured at precisely the same instant as the other inputs. Inter-port skew, oscillator error, interrupt latency, and actual worst-case sample timing are unmeasured. This design is for deliberately paced diagnostic vectors, initially held at least ten milliseconds, not nanosecond switch characterization. A scope remains necessary for real transition timing and glitches shorter than the software qualification interval.

## Firmware behavior and design decisions

Production code is C11 with explicit vector startup, linker script, register definitions, board driver, bounded UART driver, event queue, policy engine, and encoder. The linker reserves 512 KiB flash and 96 KiB RAM and checks for at least 4 KiB stack headroom. No dynamic allocation occurs in the firmware. The timer, GPIO, USART2, RCC, and independent watchdog definitions follow the STM32F401 memory map; substitutions require review against ST RM0368 and the applicable device datasheet.

All four inputs are qualified as one vector. Three equal observations on successive millisecond ticks accept that vector. Calls repeated within the same tick do not advance the count. A changed candidate restarts qualification. One chattering line can therefore delay every other signal's recognition; this deliberate simplification is unsuitable for independently high-rate traffic. All timing and fault definitions below concern qualified vectors, not instantaneous electrical states.

At startup the engine is unsynchronized. Three all-low observations establish an idle baseline. A high initial level is displayed after qualification but never invented as a trustworthy transfer. A sampling gap increments the gap count and missed-tick count, cancels the previous path and temporal intervals, and again requires qualified all-low recovery. The last qualified vector remains visible until another qualifies. Counters already recorded remain intact. Gaps are unknown evidence, not a long safe break.

Command overlap is an episode in which both qualified command bits are high. Feedback overlap independently counts episodes in which both feedback bits are high. Holding an overlap continuously increments its counter only once. Leaving and re-entering that condition counts a new episode. These two measures separate what the controller requested from what the reported hardware did; they do not prove why a disagreement occurred.

The engine remembers the last singly active feedback path: one for A, two for B, zero for unknown. First activation after synchronization establishes the path without counting a transfer. Changing from one singly active path to the other increments transfers. If a qualified both-off interval intervened, its duration becomes last_dead; otherwise last_dead is zero. A duration below twenty milliseconds increments short_break. Exactly twenty is accepted by this particular policy. Returning to the same path does not count as a cross-domain transfer.

Feedback overlap cancels accumulated break evidence. A later both-off interval can start new evidence; merely exiting overlap into the other path yields a zero-dead-time transfer. Last_dead is the most recently measured cross-domain interval, not the age of the currently open switch. It remains historical until another transfer. A transfer count is an observed transition, not a success total; inspect short_break and overlap counters before interpreting it.

Each command is compared with its corresponding feedback after vector qualification. A mismatch starts its own timer, and reaching fifty milliseconds increments that side's mismatch counter once per continuous episode. Returning to agreement clears the timer and permits another episode later. Timer advancement occurs before processing a newly qualified vector at the same tick, so agreement arriving exactly at the threshold does not erase the preceding fifty milliseconds. These are integration-policy thresholds, not universal relay or security requirements.

Elapsed durations and ordinary counters saturate at unsigned 32-bit maximum. Millisecond timestamps and event sequences intentionally wrap. Saturating accumulation prevents an indefinitely old all-off interval from appearing young after a full timer cycle. As with any 32-bit tick observer, a stall lasting a complete wrap cannot be reconstructed reliably; the independent watchdog is intended to reset a stalled loop much sooner. Physical watchdog behavior remains a bench test.

The event ring holds thirty-two entries and drops new events on exhaustion, incrementing dropped. Sequence allocation advances even for lost events. Priority status snapshots are scheduled every hundred milliseconds; policy announcements recur every second; remaining transmit opportunities drain events. The UART owns a bounded 384-byte buffer, never blocks acquisition waiting for TX readiness, and abandons a stalled transfer after one hundred milliseconds. Transmit failures are counted. Partial lines are recoverable at the next complete protocol marker.

The independent watchdog uses the internal low-speed oscillator and a nominal roughly two-second timeout. Oscillator and watchdog-register readiness waits are bounded using SysTick. Failure to establish watchdog startup leaves the instrument silent. A physical clock fault or inaccurate watchdog period is not validated by host tests. Reset clears all volatile state. There is no persistent boot identity or nonvolatile evidence ledger.

## Companion application, screens, and transport

The app uses Python's Linux curses, termios, and select modules without external packages. From this directory run `python3 app/audit.py --port /dev/ttyACM0 --export assessment.json`. Select the port explicitly; automatic discovery and authenticated device identity are not implemented. Access errors are reported without pretending that a connection succeeded. Disconnect preserves visible evidence but disables the live indicator. Exit and restart the app to choose or reconnect a port.

Press 1 for the handoff overview, 2 for event history, 3 for health, e for private export, and q to exit. The overview shows qualified inputs, synchronization, last feedback path, transfer count, last dead time, and the five fault totals. The event page displays recent sequence/time/kind/value records. Health displays parser errors, observed boot sessions, connection/replay state, sampling gaps, missed ticks, dropped events, and transmitter failures. There is no target control or remote arming screen because the instrument has no active target function.

A live indication requires connection, a supported boot or policy announcement, and a valid status received within the last second. It means recent transport, not electrically valid observation: synchronization and gap counters are separate. A periodic policy announcement allows late attachment or recovery from a lost initial boot packet without clearing accumulated evidence. Boot records indicate reset and clear the current displayed session; policy records do not. An unsupported policy freezes incoming status/events until supported policy arrives.

A backwards modular status timestamp flags a suspected unannounced reset and freezes evidence until reconnect or a boot packet. Policy alone does not clear that suspicion. Queued event timestamps are not used for this heuristic because delayed events can legitimately predate the latest status. A reset whose timestamp happens to move forward remains indistinguishable if its boot frame is lost. USB identity, run continuity, and complete reset detection are not guaranteed.

Frames are ASCII `HA1,type,fields*CCCC` followed by LF. CRC-16/CCITT-FALSE covers all bytes before the asterisk, with polynomial 0x1021 and initial value 0xffff. The checksum is four uppercase hexadecimal digits. Numeric fields are unsigned decimal integers bounded to 32 bits. There is no binary byte-order ambiguity, request channel, command ID, acknowledgment protocol, or retransmission guarantee.

B carries reset flags, break threshold 20, mismatch threshold 50, and qualification count 3. P carries only those three policy parameters. S carries fifteen fields in order: time, levels, synced, last_path, last_dead, transitions, command_overlap, feedback_overlap, short_break, mismatch_a, mismatch_b, gaps, missed, dropped, tx_fail. Levels assign bits zero through three to command A, command B, feedback A, feedback B. E carries sequence, time, kind, and value. Event kinds are qualified levels 1, command overlap 2, feedback overlap 3, short break 4, mismatch A 5, mismatch B 6, transfer 7, gap 8, and synchronization 9. Transfer and short-break values are milliseconds; overlap/mismatch values are cumulative counts; gap values are missed ticks.

The parser bounds its receive buffer even without newlines, rejects malformed checksums, unsupported field layouts, negative or oversized numbers, and invalid enum values. It can resynchronize a truncated record followed immediately by a complete HA1 marker. Replay files are capped at sixteen MiB and always labeled not live. Run `python3 app/audit.py --replay capture.txt --headless --export replay.json` for noninteractive analysis. Firmware fixtures are synthetic and must never be presented as physical measurements.

Exports use exclusive creation and mode 0600, refuse existing destinations and final-path symlinks, and include author, source, receipt age, live/replay/connection flags, policy knowledge, reset suspicion, latest status, boot metadata, parse errors, and up to 256 events. They are bounded snapshots, not immutable complete logs. No evidence is uploaded. Retain only authorized records, delete exported files using your workstation's approved procedure, and remember that filesystem deletion is not guaranteed secure erasure. Exiting discards unexported application memory; no automatic recovery file is created.

## Build, CAD verification, and laboratory acceptance

Run `make -C firmware clean all test` with GNU Make, native GCC, and ARM GNU GCC installed. The tested cross-compiler is 14.2.1. Output is `firmware/build/handoff-audit.elf` and `.bin`, linked at 0x08000000. Use a physically controlled ST-LINK programming workflow and a hardware reset. Do not initialize a different clock tree in a debugger script and assume this reset-clock firmware remains compatible. Programming, startup on silicon, and sampled timing have not been exercised here.

Run `python3 -m unittest discover -s tests -p test_app.py` and `python3 tests/integration.py`. The first checks parsing, policy, live/replay handling, restart detection, screens, and private exports. The second feeds actual C-generated synthetic frames through replay and Linux pseudo-terminals, operates real curses screens, exports data, checks overwrite refusal, disconnects, and exits. Firmware tests execute the shipped policy and encoder under address/undefined-behavior sanitizers. The serial-driver test maps a Linux page as modeled registers; that is software control-flow validation, not real USART testing.

Generate CAD with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9 bindings. Run `/usr/bin/python3 tools/check_hardware.py` to perform ERC/DRC and compare a fresh schematic XML netlist against actual board pads, component values, footprint library identities, and the connectivity manifest. Local authored resistor and connector symbols are embedded and supplied in Handoff.kicad_sym. The board has real traces, vias, and an outline. Passive-symbol ERC cannot validate external MCU power, target voltage, harness polarity, or grounding safety.

Before fabrication, inspect the schematic and board visually, confirm supplier footprints and pad drill sizes, and review assembly access and strain relief. Before attachment, measure continuity, shorts, resistor values, ground potential, threshold margin, loading, and back-power behavior. Then measure sampling timing, minimum detectable pulses, output inactivity, watchdog recovery, reset behavior, USB interruption, and hot-plug effects on a sacrificial fixture. ESD, EMC, regulatory compliance, calibration, environmental behavior, and enclosure suitability remain unverified.

## Practical research workflows

A product-security engineer can instrument a common-ground debug multiplexer with two command signals and two independent enabled-state outputs. Begin all-low, exercise normal ownership changes through the target's legitimate interface, and compare observed dead time with the documented requirement. Deliberately shortened intervals should be generated only by approved target test firmware, not by this observer. Preserve the fixture revision and scope trace when investigating a count.

An authorized red team can use the instrument during a supervised laboratory review of maintenance-to-application ownership transitions. The useful question is whether a privilege-boundary design has a timing hole, not whether the observer can secretly obtain access. A penetration tester can attach the exported summary to a report while explicitly separating diagnostic-state overlap from proven data access. A regression engineer can repeat the same paced trace after a firmware fix and compare fault episodes without collecting sensitive payloads.

Authoritative hardware references are ST UM1724 for NUCLEO-64 MB1136 connector and solder-bridge details, ST RM0368 for STM32F401 peripheral behavior, and the STM32F401xD/xE datasheet for electrical limits. Review the editions matching your actual board. The repository artifacts define this prototype's intended contract, not a replacement for those manufacturer limits or for physical qualification.
