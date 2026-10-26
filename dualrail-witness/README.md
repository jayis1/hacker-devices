# Dualrail Witness — Complementary Permission-Encoding Evidence Bench

Author and creator: jayis1. Hardware revision A. Wire protocol DW1. Firmware and application: GPL-2.0-only. Hardware tools: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, a working Linux desktop companion, and routed native KiCad files. Physical fabrication, programming, electrical qualification, timing calibration, and deployment are not performed. This device is an observer, not a permission controller, protective interlock, safety relay, or certificate of compliance.

## Purpose, overview, and originality

Dualrail Witness examines a failure mode in redundant permission encoding: a controller represents authorization on two complementary wires, but its downstream enable logic or diagnostic fault output does not consistently reject an illegal code. One rail alone is insufficient evidence of permission. The intended representation is ALLOW high and DENY low for authorization, ALLOW low and DENY high for denial. Both low and both high are invalid encodings. An independent observer records the time spent in invalid codes, output assertion without prior valid permission, and missing fault reporting during sustained invalid encoding.

The practical question is not whether two wires exist. It is whether the target integrates those wires into a fail-closed decision. A disconnected denial rail, a stale allow indication, or a decoder that tests only ALLOW may create behavior that ordinary application logs do not expose. A separate MCU can observe diagnostic indications while the target logger is stalled or taking a different execution path. The resulting evidence remains conditional on the diagnostic adapter: a malicious target can lie on all four wires, and a software-derived enable signal need not represent a physical access path.

The repository device directories and README titles were inspected before selection, and the full measurement contracts of Handoff Audit and Quorum Witness were compared. Handoff Audit studies two ownership paths and break-before-make intervals. Quorum Witness studies three independent votes and a majority decision. Dualrail Witness instead treats two rails as one encoded Boolean, distinguishes invalid encoding from a valid denial, and observes the target's fault-response indication separately from the final enable. The new engine, failure definitions, protocol, screens, and tests are specific to this contract. The passive carrier and STM32 support patterns intentionally reuse work by jayis1. Novelty is claimed within the inspected repository, not as a worldwide prior-art or patentability conclusion.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Obtain approval for the actual test points, diagnostic firmware, shared ground, test workload, possible interruptions, and retained reports. Restrict initial assessment to a sacrificial common-ground development fixture with hazardous actuators and production dependencies disconnected. Do not attach this prototype to operating access controls, occupied buildings, medical equipment, vehicles, industrial protection loops, mains circuits, or unknown field wiring.

The assessment surface is the integration between redundant permission representation, downstream enable, and diagnostic fault handling. Relevant defects include stuck or disconnected rails, inconsistent decoder conditions, enable remaining active under a reported fault, and invalid codes persisting without a corresponding diagnostic assertion. The device does not create those conditions, issue target requests, collect credentials, inject packets, scan networks, transmit radio signals, or implement an exploitation payload. An authorized red team supplies approved scenarios through the target's own laboratory interface. This instrument observes only.

The target-facing firmware configures all four diagnostic pins as inputs with internal pulls disabled. Only PA2 transmits, toward the Nucleo ST-LINK virtual COM interface. The USART receiver is disabled; there is no inbound command parser or remote arming operation. The companion opens its serial device read-only. These are properties of the supplied firmware, not a physical one-way barrier: replacing firmware could reconfigure MCU pins. Shared grounds, input loading, assembly mistakes, and power sequencing can still disturb a target. Stop immediately if attachment changes its behavior unexpectedly.

The trusted setup consists of reviewed firmware, a physically controlled development board, correct harness wiring, the operator, and the workstation. Serial bytes and replay files are treated as untrusted input. CRC detects accidental corruption but supplies neither authentication nor forgery resistance. There is no secure boot, signed updater, encrypted evidence, persistent boot identifier, pairing secret, or tamper-evident journal. SWD programming is a physically supervised maintenance activity. Record firmware provenance independently and corroborate consequential findings with a logic analyzer and target source inspection.

## Hardware specifications and bill of materials

The complete instrument uses an external ST NUCLEO-F401RE development board, the supplied passive carrier PCB, insulated leads, and a Linux workstation. The STM32F401RET6 is a Cortex-M4 with 512 KiB flash and 96 KiB SRAM. The firmware uses its reset-default nominal 16 MHz HSI clock, a one-millisecond SysTick, and USART2 TX at 115200 baud, eight data bits, no parity, one stop bit. There is no FPGA, radio, battery, analog sensor, charger, target power output, SD card, or operating system in the instrument.

The carrier is an 80 by 70 mm two-layer board with nominal 1.6 mm thickness, 0.3 mm copper tracks, and fourteen through-hole components. It is connected by a harness rather than mechanically stacked as an Arduino shield. MCU decoupling, voltage regulation, USB power handling, reset, SWD, and serial conversion are provided by the external Nucleo, not omitted from an alleged standalone MCU design. The KiCad schematic describes the passive carrier honestly through its connectors. No enclosure or mounting holes are supplied; use an insulating support and strain relief.

| Item | Quantity | Specification |
| --- | --- | --- |
| External processor board | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 straight through-hole header, 2.54 mm pitch |
| J2 | 1 | 1x4 straight through-hole header, 2.54 mm pitch |
| Harness | one set | Short labeled insulated signal and ground leads |

The resistors use KiCad DIN0207 axial footprints with 7.62 mm lead spacing. Headers use the standard PinHeader vertical footprints. Check purchased parts against footprint dimensions, drill sizes, mating access, and connector orientation before ordering. The square pad identifies pin one. Component availability, supplier substitutions, enclosure durability, and assembly process capability have not been qualified.

## Pin mapping, power, and electrical limits

| Signal | Target connector | MCU harness | Nucleo point | Protocol bit |
| --- | --- | --- | --- | --- |
| ALLOW | J3.1 | J2.1 | CN8 A0 / PA0 | 0 |
| DENY | J4.1 | J2.2 | CN8 A1 / PA1 | 1 |
| ENABLE | J5.1 | J2.3 | CN8 A2 / PA4 | 2 |
| FAULT | J6.1 | J2.4 | CN8 A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | Nucleo GND | none |

All four inputs are active high. A numeric low-two-bit code of one means allow, two means deny, and zero or three means invalid. Names ending `_IN` in CAD identify the target side of the series resistor. Plain signal names identify the MCU side, where each 100 kOhm pulldown attaches. J1 is ground only: neither pin is a supply input. J2 has signals only. No jumper or command reverses polarity.

Use nominal zero-to-3.3 V, common-ground push-pull diagnostic outputs. The resistor network presents approximately 101 kOhm DC load per high input, ignoring leakage. At 3.3 V this is approximately 32.67 microamps with approximately 3.267 V on the unloaded MCU node. These are calculated values, not measured specifications; source impedance, resistor tolerance, cable capacitance, MCU leakage, and electrical thresholds require review. Pulldowns make disconnected wires low, but do not prove disconnection: the same state can be driven deliberately. Both disconnected rails produce invalid encoding, while some single-wire failures remain indistinguishable from valid denial or permission.

There is no isolation, qualified ESD protection, TVS network, fuse, level translation, or reverse-current blocking. The series resistors are not permission to attach overvoltage sources. Do not connect 5 V, 1.8 V, negative serial voltage, differential buses, relay coils, or industrial field signals. Power the Nucleo before enabling target outputs, establish compatible ground first, disconnect signals before removing observer power, and avoid back-powering through MCU protection structures. Reserve a conservative 5 V, 500 mA USB source budget while respecting the board manual's actual configuration; consumption has not been measured.

The connector mapping follows ST UM1724 Table 16 for NUCLEO-F401RE. The normal virtual-COM connection uses SB13/SB14 enabled and SB62/SB63 disabled. Verify the physical board revision and solder bridges. The supplied register definitions follow STM32F401 RM0368; a similarly named MCU is not automatically compatible. Debugger scripts must not silently change the reset clock before running this image.

## Architecture and block diagram

```text
Authorized 3.3 V fixture                  Passive carrier A
ALLOW  --> J3.1 ---- R2 1k ----+------ J2.1 --> PA0 / A0
DENY   --> J4.1 ---- R4 1k ----+------ J2.2 --> PA1 / A1
ENABLE --> J5.1 ---- R6 1k ----+------ J2.3 --> PA4 / A2
FAULT  --> J6.1 ---- R8 1k ----+------ J2.4 --> PB0 / A3
                             | each input node
                         100k pulldown
                             |
Common ground ---------------+------ J1 ---> Nucleo / USB ground

GPIO samples -> independent qualifiers -> encoded-permission engine
             -> episode counts, current durations, historical maxima
             -> bounded CRC status / recurring policy / one boot record
             -> USART2 TX -> ST-LINK USB -> Linux desktop
                                           Permission / History / Integrity
                                           private operator-selected JSON
```

GPIOA and GPIOB are read sequentially. FAULT is not physically latched at exactly the same instant as the other three bits. Port-read skew, interrupt latency, oscillator error, and minimum detectable pulses have not been measured. Use deliberately paced diagnostic vectors with high and low times initially at least ten milliseconds. This device is not a high-speed logic analyzer, dual-rail cryptographic side-channel instrument, or proof that submillisecond glitches are absent.

## Exact firmware measurement contract

The production implementation is C11 with explicit startup vectors, linker script, register definitions, board driver, bounded UART driver, qualification/measurement engine, and checked text encoder. It contains no dynamic allocation, RTOS, filesystem, or persistent state. The linker maps flash at 0x08000000 and reserves at least 4 KiB stack headroom. A new observer reset discards RAM evidence. There is no flash-writing path and thus no persistent-write atomicity claim.

Each input qualifies independently. A changed candidate needs three further matching observations on distinct consecutive millisecond ticks. Calls repeated within a tick do not advance qualification. The first observation establishes candidates but no trusted vector; after three matching intervals all four validity bits can become set. A chattering channel does not postpone an already-qualified unrelated channel, but initial measurement waits until every channel has qualified at least once. Thereafter the last qualified value remains in use until its replacement qualifies. Durations describe qualified states, not continuous raw electrical truth; rejected glitches can remain invisible.

An invalid episode begins when both qualified rails are equal. It continues across a direct transition between zero and three because neither represents valid encoding. Returning to either valid code ends that episode. The invalid counter increments at entry. Current invalid duration starts at zero; every subsequent observed interval adds one millisecond before processing the next qualified endpoint. The historical maximum retains the longest observed interval. This makes endpoint recovery at a threshold account for the interval that just elapsed rather than erasing it.

Unsafe enable means qualified ENABLE is high while the current code is not allow, while FAULT is high, or while the preceding evaluated vector did not already establish allow without fault. This conservative prior-vector rule counts a simultaneous permission-and-enable qualification as early, even if the next tick is acceptable. It does not claim to determine physical precedence within a sampling interval. Holding enable through an invalid code or through fault assertion is one unsafe episode until the condition clears. Unsafe episodes, current duration, and maximum duration are distinct from invalid-code episodes.

A silent-response interval is an invalid code with qualified FAULT low. At twenty observed milliseconds it increments the silent-response counter once and sets a sticky finding. A fault assertion qualifying exactly at the twenty-millisecond endpoint does not erase the just-completed silent interval. FAULT high clears the current silent interval; dropping it again while encoding remains invalid starts a new one. Therefore several missing-response episodes can occur inside one continuous invalid-code episode. This instrument does not certify the reason FAULT was asserted; it may represent an unrelated fault in the target.

A sample-time jump other than one millisecond increments acquisition-gap and missed-tick diagnostics, clears qualification and active intervals, and requires fresh qualification. Unobserved time does not contribute to durations. Counts, maxima, and sticky findings already recorded survive. The first post-gap observation establishes candidates but does not qualify them. A continuously bad physical condition can be counted again after reacquisition; episode totals are observed segments, not guaranteed distinct real-world faults.

Counters and durations are unsigned 32-bit values. Overflow is terminal rather than wrapping or silently resetting: HALTED retains the latest measurement evidence, while device time and transport diagnostics may continue. Millisecond time intentionally wraps modulo 32 bits. Ordinary one-tick wrap is supported. A stall spanning a complete timer cycle cannot be reconstructed; the independent watchdog is intended to reset a stalled loop much sooner. Its physical operation and oscillator tolerance remain bench tests.

## Runtime, protocol, and application

The main loop samples before servicing serial output. The UART driver copies a complete bounded frame, transmits at most one byte per poll, and abandons a blocked transfer after one hundred milliseconds. It never blocks waiting for a workstation. Transmission failures are counted. A watchdog startup failure sets HALTED before status serialization. Readiness waits use bounded SysTick timeouts. No sampled-data queue exists; the desktop receives periodic cumulative snapshots, not every transition.

Status is scheduled approximately every hundred milliseconds. A boot record is sent once. Policy is sent initially and approximately every second, permitting late attachment without inventing a new boot. All records use ASCII `DW1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial value 0xffff, no reflection or final XOR, and covers all bytes before the asterisk. The checksum is exactly four uppercase hexadecimal digits. A frame is bounded below 384 bytes.

B carries one MCU reset-flags integer. P carries `3,20`: qualification intervals and silent-response grace milliseconds. S carries sixteen fields in order: `now, levels, valid, halted, faults, invalid, unsafe, silent, invalid_ms, unsafe_ms, silent_ms, max_invalid, max_unsafe, gaps, missed, tx_fail`. Levels and validity masks use the pin-order bits in the table. Finding bits zero through six mean invalid encoding, unsafe enable, silent response, sample gap, transmitter failure, counter exhaustion, and watchdog startup failure. All numbers are unsigned decimal bounded to 32 bits.

Run `python3 app/console.py` from this directory. The application uses standard-library Tk, termios, poll, and JSON; no package download is needed. Select the serial path explicitly and choose New live session. Permission displays qualified inputs, measurement state, counters, durations, and named findings. History retains up to 256 status snapshots. Integrity displays provenance, policy compatibility, reset flags, parser errors, receipt age, connection state, and freeze reason. New live session intentionally replaces old evidence; export first. Disconnect preserves evidence and provenance. The physical stop procedure is disconnecting the target signal harness; disconnecting the app does not stop MCU observation.

Replay accepts regular files only, opened nonblocking, with a two MiB limit enforced before and during reading. FIFOs and device nodes are rejected. Replay is never live. A nonempty partial tail at EOF is an error, and empty captures do not count as accepted evidence. The CLI supports `python3 app/console.py --replay capture.txt --export report.json`. A nonzero exit reports malformed, frozen, or empty replay. The supplied C fixture produces explicitly synthetic data, not measurements from a fabricated instrument.

Freshness requires a connected serial source, matching policy, no freeze, and advancing accepted device time within one second. Identical duplicates do not refresh receipt age. Contradictory duplicates, counter rollback, disappearing findings, impossible selected increments, or mutation of halted measurements freeze the session. A host pause exceeding one second also freezes resumed observations instead of relabeling queued data live. Cumulative host/device clock progress is compared with a 500 ms plus five-percent allowance in either direction. These are conservative backlog heuristics, not precise clock synchronization or authenticated continuity.

A boot arriving after an accepted status freezes retained evidence until the operator starts a new session. Policy messages never clear that freeze. Some resets remain indistinguishable when their boot frame is lost and later timestamps and counters happen to progress. Snapshot consistency checks are necessary bounds, not a complete realizability proof or forgery defense. LIVE describes transport freshness, while UNKNOWN and HALTED describe measurement validity; these must not be conflated.

Exports contain author jayis1, protocol, source/provenance, connection and freshness, parser errors, freeze reason, boot metadata, latest status, and retained history. Files are created exclusively with mode 0600 and refuse overwriting existing files or final-path symlinks. Select a trusted destination directory. There is no telemetry upload or hidden database. Reports are not encrypted or signed. Exiting discards unexported memory; filesystem deletion is not guaranteed secure erasure. Follow the engagement's retention procedure.

## Builds, verification, and laboratory acceptance

Required tools are GNU Make, ARM GNU GCC with newlib/nosys support, host GCC, Python 3 with Tk, KiCad 9 CLI and Python bindings, standard KiCad footprints, and Xvfb for headless GUI testing. The tested versions and actual results are recorded in VERIFICATION.md. From this device directory run:

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 -m unittest discover -s tests -p test_gui.py
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
```

The target build links the real startup, driver, engine, and encoder into a Cortex-M4 executable and raw image. Programming requires a physically controlled ST-LINK workflow followed by hardware reset into the documented clock state; no board was programmed here. Native tests exercise qualification, normal authorization, invalid codes, silent-response thresholds, simultaneous enable, chatter, acquisition gaps, timestamp wrap, malformed state serialization, overflow, runtime scheduling, and UART timeout recovery. Host register modeling is software control-flow testing, not peripheral validation on silicon.

The GUI test operates a real Tk event loop under Xvfb, uses Linux pseudo-terminals as a modeled serial source, and feeds packets generated by the production C encoder. It exercises all screens, paced reception, stale backlog, permission denial, descriptor cleanup, replay, exclusive private export, disconnect, and reconnect. This establishes application functionality against synthetic transport, not USB compatibility with a physical Nucleo. No fabricated lab trace is supplied.

The CAD checker runs ERC and DRC with violations treated as failures, then compares a freshly exported schematic netlist against board pads, values, footprint library identities, and the connectivity manifest. The schematic embeds real authored connector and resistor symbols; the PCB contains traces, vias, and a closed outline. Passive-symbol ERC does not validate external module power, source voltages, USB safety, or harness polarity. Inspect rendering and purchased component dimensions before fabrication.

Before use, assemble and inspect a carrier, verify continuity and resistor values unpowered, bring up the Nucleo without the target, measure loading and ground offsets, then verify thresholds, qualification latency, watchdog recovery, cable interruption, USB stalls, and power-sequencing effects with a sacrificial fixture. Compare deliberately slow vectors and rejected short glitches with a calibrated analyzer. ESD, EMC, environmental limits, thermal behavior, enclosure suitability, and regulatory qualification remain outstanding. Software and CAD checks alone do not make the design production-ready.

## Practical research workflows

A product-security engineer can expose the redundant permission rails, independent access-enable indication, and diagnostic fault line from an owned development controller. Begin with valid denial, establish allow without enable, then enable through the target's legitimate interface. Next exercise invalid encoding only through approved fixture controls. Compare invalid episodes, unsafe-enable duration, and fault reporting against the target's specified behavior. Preserve the pin mapping and independent scope trace with every significant finding.

An authorized red team can demonstrate why checking only one permission rail is inadequate without attempting unauthorized access or collecting secrets. A penetration tester can compare original and remediated firmware using the same paced diagnostic vectors and explain exactly what each output represents. A clean counter means only that this sampled contract did not report a fault under those observations. It does not prove authentic authorization, complete fault coverage, physical isolation, or absence of faster races.

## Manufacturer references

- ST UM1724, Nucleo-64 MB1136 user manual, Table 16 and USART bridge guidance: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 peripheral reference: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

Review manufacturer editions appropriate to the physical board and silicon. Original design, firmware, application, and documentation author: jayis1.
