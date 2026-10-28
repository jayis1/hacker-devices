# Clock Ratio — Enabled Clock-Divider Integrity Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol CR1. Firmware and application: GPL-2.0-only. Hardware utilities: MIT. Passive carrier: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design with executable software and native routed KiCad source. This release does not claim physical fabrication, MCU programming, calibrated timing, electrical qualification, or safety certification. See VERIFICATION.md for executed checks. The instrument observes intentionally slow diagnostic indications, not processor clocks or high-speed clock glitches.

## Purpose, overview, and originality

Clock Ratio investigates a security integration assumption that is easily obscured by ordinary heartbeat logs: while a protected timing domain is enabled, does its exported tick maintain the expected relationship to an independent reference? A watchdog-service task might still produce occasional heartbeats even though its clock divider is misconfigured. A security timeout driven by that divider could consequently expire too early or too late. Checking the presence of activity alone does not test the relationship between two timing domains.

The device receives four owner-provided diagnostic outputs: REF, TICK, ACTIVE, and RESET. After a prospective anchor tick, it expects exactly four qualified REF rising edges before the next qualified TICK rising edge, within one hundred nominal milliseconds. It separately identifies too few references, extra references, simultaneous ambiguous edges, expired intervals, ticks outside the enabled window, and observer acquisition gaps. It never writes to the target or changes either clock. A target-owned diagnostic adapter must divide or stretch real signals into the supported slow interface without destroying the ratio being investigated.

The repository directories and README descriptions were inspected before selecting this device. The closest existing designs were read more closely. Lease Watch checks whether an enable has a recent heartbeat; Pulse Envelope checks individual high durations and recovery intervals; Receipt Witness correlates audit acknowledgments; Erase Witness checks a sanitization lifecycle. None of these implements the four-reference-per-tick interval ledger described here. Clock Ratio deliberately reuses jayis1's four-input carrier topology, STM32 board support, startup, and nonblocking serial driver. The novelty is the clock-domain relationship contract and associated evidence model, not a new resistor network. Originality means distinct within the inspected repository, not a worldwide patent or prior-art conclusion.

## Authorized use and ethical disclaimer

Use only on devices you own or have explicit written permission to assess. Authorization must include the particular test points, instrumented firmware, workload, wiring, and evidence retention. A broad penetration-testing agreement does not automatically authorize electrical attachment to every accessible circuit. Keep the instrument visible, identify every lead, and use an isolated development fixture with hazardous actuators disconnected. Do not deploy this unqualified board against production locks, medical equipment, operating vehicles, mains-connected circuitry, live industrial machinery, or any system whose interruption could injure people.

Receive-only operation is not electrically risk-free. Ground loops, miswiring, input loading, and back-power through protection diodes can alter target behavior. Stop if connecting the observer changes the observed system. A finding requires corroboration with the target implementation and an independent logic analyzer. Neither a clean run nor a good interval count certifies the target as secure. There is no protective interlock, emergency shutdown output, glitch injector, exploit payload, credential reader, network scanner, or covert deployment feature.

Timing metadata can reveal operating patterns even without payload collection. Agree on a retention period, export only what the engagement needs, and use a private working directory. Private file permissions are not encryption, tamper resistance, or secure deletion. The author, jayis1, does not authorize unlawful interception, disruption, or access.

## Attack surface and threat model

The assessed boundary is a target's claimed relation between a reference source and the tick used by a protected subsystem. Relevant defects include incorrect divider configuration after reset, missed tick propagation, duplicated tick indications, continuing to tick after a domain is disabled, or mixing reference and downstream signals from different operating generations. An approved test build can expose these states through diagnostic GPIOs while exercising ordinary transitions. Clock Ratio records contradictions; the operator supplies any workload or deliberate configuration change through the target's authorized development tools.

The reference must be independent of the failure being assessed. If both diagnostic lines are generated by the same defective routine, their ratio can remain correct while the real timeout mechanism fails. If a compromised target fabricates all four signals, this observer cannot establish truth. A clock ratio also cannot prove absolute oscillator accuracy: both clocks can slow together without changing the ratio. The one-hundred-millisecond limit gives only a coarse observer-relative bound. It is not a calibrated frequency counter, phase-noise analyzer, pulse-width detector, or metastability detector.

Trusted elements are the reviewed instrument firmware, development board, harness, workstation, operator, and diagnostic mapping. Serial bytes and replay files are untrusted parser inputs. CRC detects accidental corruption but does not authenticate the sender. There is no secure element, signed telemetry, protected persistent boot identity, remote attestation, encrypted transport, or signed update service. Physical SWD access and host account access are trusted. Anyone controlling the workstation or serial source can manufacture plausible evidence.

## Hardware specifications and bill of materials

The complete assembly consists of an external ST NUCLEO-F401RE, the supplied passive carrier, insulated short leads, nonconductive support, and a Linux workstation. The STM32F401RET6 has a Cortex-M4 core, 512 KiB flash, and 96 KiB SRAM. The firmware uses the normal reset-default nominal 16 MHz HSI source and a nominal one-millisecond SysTick without PLL configuration. USART2 transmits at 115200 baud, eight data bits, no parity, and one stop bit. ST-LINK provides programming and the USB virtual COM bridge. Regulation, decoupling, reset circuitry, and SWD reside on that purchased module, not on the passive carrier.

The carrier is an 80 by 70 mm, two-layer, nominal 1.6 mm board with 0.3 mm routed tracks. It connects by a harness rather than stacking as a Nucleo-compatible shield. There are no radios, antennas, FPGA, analog sensors, battery, charger, regulator, isolation components, or removable storage. Its only sensing elements are the four logic inputs. Mounting holes and an enclosure drawing are not included; provide mechanical retention and strain relief before construction.

| Reference | Quantity | Specification |
|---|---:|---|
| R1, R3, R5, R7 | 4 | 100 kΩ pulldown, 1%, quarter watt, axial DIN0207 |
| R2, R4, R6, R8 | 4 | 1 kΩ series resistor, 1%, quarter watt, axial DIN0207 |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical through-hole header, 2.54 mm pitch |
| J2 | 1 | 1x4 vertical through-hole header, 2.54 mm pitch |
| External module | 1 | ST NUCLEO-F401RE |
| External assembly | 1 set | Insulated leads, USB cable, nonconductive support |

Resistor lead spacing is 7.62 mm. Verify actual resistor dimensions and connector mating height before purchasing. No availability or price claim is made. The schematic, PCB, saved XML netlist, and connectivity.json contain consistent references, values, footprints, and nets.

| Bit | Signal | Target header | MCU-side J2 | Nucleo label / MCU pin |
|---:|---|---|---:|---|
| 0 | REF | J3.1 REF_IN | 1 | A0 / PA0 |
| 1 | TICK | J4.1 TICK_IN | 2 | A1 / PA1 |
| 2 | ACTIVE | J5.1 ACTIVE_IN | 3 | A2 / PA4 |
| 3 | RESET | J6.1 RESET_IN | 4 | A3 / PB0 |

J3–J6 pin two is ground. Both J1 pins are ground and must never receive a supply voltage. Each input passes through one kilohm to an MCU-side node with a one-hundred-kilohm pulldown. Unsuffixed names identify the MCU-side nodes. The pulldown makes an unplugged lead tend low; unplugging is not independently detected. A disconnected ACTIVE lead can therefore terminate an interval without demonstrating a real target disable.

Only known zero-to-3.3 V push-pull signals sharing a verified common ground are supported. There is no 5 V tolerance claim, galvanic isolation, dedicated ESD suppression, reverse-polarity protection, or overvoltage disconnection. The series resistor does not make unknown field wiring safe. Never attach RS-232, differential buses, relay coils, mains, or industrial 24 V signals. Verify input limits and loading on the assembled fixture.

Power the Nucleo through its documented ST-LINK USB connector. Reserve a conservative 5 V, 500 mA source allocation for planning; this is not measured consumption or USB certification. Establish common ground, power the observer before enabling target outputs, and remove target signals before depowering the observer to avoid input-diode back-power. Check the actual Nucleo revision, jumpers, solder bridges, and header numbering against ST UM1724; register definitions follow the STM32F401 RM0368 register map. Attempts to fetch current manufacturer PDFs during this run failed, so fresh external revision verification remains a bring-up prerequisite rather than a claimed completed check.

## Architecture and block diagram

```text
Authorized slow diagnostic adapter       Passive carrier        External NUCLEO
REF    --> J3.1 --> R2 1k --> + --> J2.1 ---------------------> PA0 / A0
TICK   --> J4.1 --> R4 1k --> + --> J2.2 ---------------------> PA1 / A1
ACTIVE --> J5.1 --> R6 1k --> + --> J2.3 ---------------------> PA4 / A2
RESET  --> J6.1 --> R8 1k --> + --> J2.4 ---------------------> PB0 / A3
                            | each: 100k pulldown
Common ground --------------+ --> J1 ------------------------> GND

GPIO -> continuity check -> independent input qualification
     -> enable/reset gate -> anchored reference-count interval
     -> transactional conservation ledger -> CR1 decimal/CRC encoder
     -> USART2 PA2 TX -> ST-LINK USB -> Linux desktop
                                      Ratio ledger / History / Integrity
                                      explicit private JSON snapshot
```

GPIOA is sampled before GPIOB. There is no simultaneous hardware latch across ports. Oscillator tolerance, inter-port skew, interrupt latency, wiring capacitance, and detectable pulse width remain unmeasured. Keep diagnostic high and low levels comfortably longer than qualification; ten milliseconds each is a reasonable starting test stimulus, subject to the interval deadline. Never connect a raw processor clock and expect meaningful results from millisecond polling.

## Firmware details and measurement contract

Firmware is C11, with explicit vector startup, linker layout, register definitions, board driver, nonblocking UART driver, measurement engine, and bounded encoder. It has no heap, RTOS, inbound command parser, or downloaded MCU SDK dependency. The build targets Cortex-M4 Thumb with software floating-point ABI. A normal hardware reset is required; a custom bootloader that changes the clock tree violates the board initialization assumptions.

Each input qualifies independently. On a candidate change its age becomes zero. Three additional consecutive one-millisecond observations of the same candidate establish the qualified level. The first observed high after startup or a gap is not a rising edge. Repeated calls at the same timestamp do nothing. Independent qualification prevents chatter on RESET, for example, from postponing qualification of a persistent TICK. Short electrical pulses may nevertheless be rejected or missed entirely. The measured object is the qualified diagnostic signal, not all activity on the wire.

Counting is enabled only after all four inputs are known, qualified ACTIVE is high, qualified RESET is low, raw ACTIVE is high, and raw RESET is low. Raw disable or reset therefore aborts an interval immediately, even before that control signal qualifies. This conservative rule can abort on a short control glitch; it does not claim that the target actually reset. Any qualified TICK rising edge while this gate is false increments outside and sets a sticky finding. REF edges outside the enabled gate are ignored.

A qualified low TICK must be observed while enabled before a rising TICK can anchor an interval. Startup attachment to a high signal does not fabricate an anchor. The first accepted tick increments anchors and begins an empty interval; it does not count as a completed ratio. REF rising edges then accumulate. A later TICK rising edge completes the interval and simultaneously anchors the next one. Exactly four references increments good; any other count increments bad and sets the ratio flag. The ledger always satisfies intervals = good + bad and anchors = intervals + aborted + anchored, where anchored is zero or one.

Reference count saturates at five, which means at least five. The fifth reference sets the extra-reference finding immediately, before waiting for the closing tick. Further references do not wrap or hide excess. This count is not the total number of physical reference edges and is deliberately censored. If REF and TICK qualify high together, their ordering is ambiguous: ambiguous increments, the current interval is aborted if present, and another qualified low TICK is required before anchoring. The ambiguous observation is never silently assigned to either neighboring interval.

An anchored interval ages once per contiguous sampled millisecond. A closing tick at age one hundred is accepted; age one hundred one is expired. Timeout is evaluated before processing edges at that sample, so a late tick does not immediately reanchor. A new low then rising tick is required. Before the first anchor there is no timeout: an enabled but permanently silent TICK remains visibly unanchored, not a successful zero-error run. Applications must not treat absence of completed intervals as a pass.

Any timestamp delta other than one records an acquisition gap, accumulates missed ticks, aborts the open interval, clears qualification, and requires a fresh prospective anchor. Missing time is never assumed to preserve a clock relationship. Qualified levels are retained as historical values until reacquired. Ordinary unsigned timestamp wrap is supported; a complete unobserved timer cycle cannot be distinguished from other histories. Duplicate timestamps have no effect.

Each sample updates a temporary copy of the ledger. If a required counter increment cannot be represented, the entire sample transaction is discarded and a terminal exhaustion fault is set. This preserves conservation rather than publishing half a transition. Halted measurements freeze, while device time and transmitter diagnostics can continue advancing. All counters are RAM-only and are lost on power loss or reset. The independent watchdog is fed from main-loop progress; initialization waits are bounded and failed startup sets a terminal fault before telemetry. Hardware watchdog behavior remains untested on silicon.

## Application interface and record protocol

Run `python3 app/console.py` on Linux with Tk installed. Ratio ledger displays all status fields, the named sticky findings, and the censored reference-count warning. History retains at most 256 accepted statuses. Integrity shows source, live/replay provenance, policy compatibility, freshness, parser errors, reset flags, freeze reason, and operation messages. Select the exact serial device, commonly `/dev/ttyACM0`, rather than relying on automatic discovery.

New live opens the terminal read-only, selects 115200 8N1, and flushes queued input. It refuses another connection while one is open. Failed opening preserves existing evidence. Disconnect retains evidence and its original provenance. Loading a valid replay or successfully starting a new connection replaces the current session; export first. Replay opens nonblocking, requires a regular file, and bounds allocation to two MiB. FIFO and device files are rejected. An incomplete tail increments parser errors, and a recording with no compatible status is rejected.

Frames are ASCII `CR1,kind,decimal-fields*CCCC` followed by LF. CCCC is uppercase CRC-16/CCITT-FALSE over the bytes before the asterisk, polynomial 0x1021, initial value 0xffff, without reflection or final XOR. B contains one reset-flags value and is emitted once at startup. P contains `4,100,3`, describing divisor, deadline, and qualification interval. Policy repeats approximately every second to support late attachment without pretending a repetition is a reboot. S repeats approximately every hundred milliseconds.

The nineteen S fields are now, levels, valid, anchored, halted, faults, refs, age, anchors, intervals, good, bad, aborted, ambiguous, timeouts, outside, gaps, missed, tx_fail. Each is an unsigned decimal value bounded to 32 bits. levels and valid use the bit mapping in the connector table. Fault bits zero through seven mean ratio mismatch, extra reference, ambiguous edges, deadline expiry, tick outside enable, acquisition gap, counter exhaustion, and watchdog startup failure. Faults are sticky until observer reset.

The UART driver owns one bounded frame, services at most one byte per main-loop call, and abandons a stalled transfer after one hundred milliseconds. It does not block acquisition while waiting for TXE. Failed sends increment tx_fail. The parser resynchronizes on the complete CR1 marker after partial transmission, validates field counts, bounds, checksum, conservation, and diagnostic relationships, and counts malformed records. These are necessary consistency checks, not proof of authentic hardware data or every possible inter-record history.

Only advancing accepted statuses renew transport freshness. Contradictory duplicate timestamps, counter rollback, disappearing sticky findings, changed halted evidence, unsupported policy, or a boot record after evidence freeze the model. Live reception pauses beyond one second require reconnect; cumulative host/device progress divergence beyond 500 ms plus five percent of host elapsed time also freezes it. This rejects gradual backlog and large forward clock jumps rather than considering every received packet fresh. A LIVE transport flag does not imply an anchored or nonhalted measurement; inspect those fields separately. Some resets remain indistinguishable without persistent boot identity.

Export JSON creates a new file with mode 0600, refuses overwriting and final-component symlinks, and includes author jayis1, integrity metadata, latest status, and bounded history. It is an explicit snapshot, not an automatically retained full capture or signed evidence chain. No network listener or upload mechanism exists. The host must be trusted, and the operator remains responsible for storage, retention, and independent chain-of-custody requirements.

## Builds, verification, and laboratory acceptance

From this directory run `make -C firmware all test`. Prerequisites are GNU Make, host GCC, Arm GNU Embedded GCC with newlib/nosys support, Python with Tk, KiCad 9 with Python bindings and standard footprints, and Xvfb for headless UI testing. The target outputs are firmware/build/clock-ratio.elf and clock-ratio.bin. Program only through an authorized ST-LINK workflow at the linked flash address 0x08000000, then reset normally. No real board was programmed in this run.

Run `xvfb-run -a /usr/bin/python3 tests/test_app.py` for parser, file-boundary, and real Tk/PTY tests. The fixture comes from the production C engine and encoder; it is synthetic test data, not a physical capture. Run `/usr/bin/python3 tools/generate_hardware.py` to regenerate CAD, then `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC, fresh netlist export, and exact reference/value/footprint/pin/net comparison. Native files contain real symbols, components, routed copper, and a closed board outline. Passive-only ERC cannot validate external supply or ground safety.

Before fabrication, inspect the drawings, confirm parts against footprints, verify the Nucleo documentation, and define mechanical retention. After assembly check continuity, resistance, polarity, input loading, common-ground offsets, back-power, UART rate, oscillator accuracy, and reset recovery. Apply known four-reference intervals, deliberately missing and extra references, simultaneous qualified edges, raw disable glitches, and stopped ticks through an authorized fixture. Compare all outcomes with a calibrated logic analyzer. ESD, EMC, environmental behavior, and regulatory suitability are not established by software or CAD checks.

## Practical security-research workflows

A product-security engineer can expose a slow reference and protected scheduler tick from an owned development build, verify the diagnostic mapping independently, and repeat reset and power-management transitions. The observer can reveal that the scheduler resumed with the wrong divider even when both domains still appear alive. A remediation assessment can run the same controlled sequences before and after a patch and compare good, bad, aborted, and unanchored results without collecting credentials or internal payloads.

An authorized red team can use the bench during a collaborative design review to challenge assumptions about security timeout timing. A penetration tester can document a concrete ratio violation in a vendor-provided test build instead of extrapolating from occasional heartbeat messages. For every campaign retain target revision, adapter source, wiring map, observer image identity, stimulus timing, and capture limitations. Preserve parser errors and acquisition gaps alongside findings. If the diagnostic adapter cannot maintain an independently meaningful ratio at supported speeds, choose a different measurement instrument rather than claiming this bench observes the real clock.
