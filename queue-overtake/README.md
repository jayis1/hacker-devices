# Queue Overtake — Tagged FIFO Departure-Order Witness

Author and creator: jayis1. Hardware revision A. Protocol QO1.
Firmware, application, and tests: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. Target-linked C firmware, a working Linux desktop companion, and routed native KiCad files are provided. Software and CAD verification are recorded in VERIFICATION.md. No physical board has been fabricated, programmed, electrically qualified, or timing calibrated. This instrument observes a diagnostic contract; it is not a scheduler, safety interlock, tamper-proof recorder, or certification of target security.

## Purpose and practical value

Queue Overtake checks whether tagged work leaves a security-sensitive queue in the order it actually entered. A target-owned diagnostic adapter exports a two-bit identity and separate ENQUEUE and DEQUEUE strobes. Identities may arrive in any order. Each outstanding identity must be unique. Departure must name the oldest outstanding identity, not the numerically smallest tag and not whichever worker happens to finish first. An identity becomes reusable immediately after its accepted departure, even while other identities remain queued.

A motivating example is a development gateway that accepts policy mutations through several asynchronous processing paths. Its contract promises that effects become visible in admission order. A later, faster operation must not bypass an earlier policy change. Ordinary totals such as five accepted and five completed cannot establish that ordering. A second microcontroller can retain the expected queue and show an attempted out-of-order departure without collecting the policy contents or relying solely on the gateway's own logger.

Consider enqueue 2, enqueue 0, dequeue 2, enqueue 2. The expected queue is now [0, 2]. Dequeue 0 is correct; dequeue 2 is an overtake, even though that identity was also the first identity in an earlier operation. This distinguishes identities from fixed ordinal positions. The instrument retains the queue at the first fault, allowing the operator to see the expected head and the observed tag. It counts successful admissions, successful departures, rejected operation edges, nonempty episodes, admissions behind existing work, and peak depth.

A second useful experiment deliberately admits tags in descending numeric order. Correct behavior follows that admission order, not ascending tag order. A test that uses only 0, 1, 2, 3 admissions can accidentally validate an implementation that merely sorts identities. Reuse and arbitrary order are therefore first-class test cases, not cosmetic variations. A queue that never holds more than one entry has not meaningfully exercised the ordering policy; peak depth and the queued-admission counter make that coverage limitation visible.

## Originality relative to the repository

The default branch was pulled and device directories and README purpose statements were inventoried before selecting this design. The closest contracts are Receipt Witness, Prefix Retire, Slot Alias, Context Stack, and Read Share. Receipt Witness correlates anonymous action, append, and durability notifications; its interface has no identities and therefore cannot detect substitution of a later operation's receipt for the oldest operation's receipt. Queue Overtake explicitly receives the departure identity and compares it to the retained head.

Prefix Retire expects fixed ordinals zero through three in a batch and retains completed membership until that batch closes. This device instead learns the order from admission events, permits queues of any depth through four, and permits immediate tag reuse after departure. Slot Alias accepts any release order from a set of occupied tags; Queue Overtake rejects an occupied tag if it is not the head. Context Stack requires last-in-first-out nested returns, the opposite end of the order. Read Share permits several readers but excludes a writer; no reader/writer mode exists here.

The contribution is the tagged rolling FIFO contract, packed head-to-tail evidence, sparse-record survivor checks, executable list-oracle tests, and associated research workflow. FIFO queues are established technology. No worldwide novelty, prior-art completeness, or patentability claim is made. The resistor carrier, STM32 support, bounded UART transport, checksum framing pattern, and desktop transport intentionally reuse jayis1-authored Read Share platform work. This folder is self-contained and imports no sibling device at runtime. Standard footprint geometry and manufacturer documents retain their original provenance; project authorship does not claim authorship of those external assets.

## Authorized use, attack surface, and trust model

Use only on equipment you own or have explicit written authorization to assess. Permission must cover the exact test points, instrumentation changes, workloads, physical attachment, possible interruption, and evidence retention. A generic application penetration-test agreement does not authorize attachment to internal hardware. Begin on an isolated, low-energy development fixture with hazardous outputs disconnected. Do not attach this unisolated design to deployed access controls, medical equipment, vehicles, industrial actuators, mains-related circuits, or unknown field wiring.

The assessed attack surface is the integration between ordered admission and actual retirement or visible effects. Relevant bugs include a fast completion overtaking older work, duplicate use of a live descriptor identity, a departure without admission, incorrect tag attribution, and a reset that silently discards queue history. An authorized red team can work with the owner to expose such a race using ordinary synthetic workloads. A penetration tester can compare pre-fix and post-fix ordering evidence. A researcher can study cancellation and scheduler changes, but must not label a cancellation as a successful dequeue unless it is genuinely the contracted head retirement.

The adapter must emit events at the agreed boundaries. ENQUEUE should mean the operation was actually accepted into the relevant order, not merely that a caller requested admission. DEQUEUE should mean the contracted retirement boundary was crossed, not merely that a worker finished an internal computation. If visibility occurs later through another queue, observe that queue instead. The observer cannot infer the correct semantic boundary from GPIO voltage. Adapter source review and independent functional measurements remain necessary.

The operator, wiring, instrument image, Nucleo module, programming interface, and workstation are trusted. A malicious target can lie on every diagnostic wire. CRC protects against accidental corruption, not forgery, replay, or a compromised host. There is no secure element, signed log, encrypted export, persistent boot identity, trusted wall clock, network listener, radio, cloud upload, or authenticated update path. ST-LINK programming access can replace the firmware and change input pins into outputs. Receive-only describes the supplied firmware, not a certified physical data diode.

Physical attachment can alter behavior through loading, shorts, common-ground paths, or back-power. Keep the harness visible, labelled, and strain relieved. Stop if connecting the instrument changes target behavior. A disconnected channel can look low because of its pulldown; there is no cable-presence detection. Absence of findings does not establish complete observation or absence of exploitable bugs. Volatile evidence is lost on reset. Export only the data agreed in the engagement, restrict local access, and use the owner's retention policy; ordinary deletion is not secure erasure.

## Hardware specifications and BOM

The compute module is an external ST NUCLEO-F401RE, MB1136 family, with STM32F401RET6 Cortex-M4, 512 KiB flash, and 96 KiB SRAM. Firmware uses reset-default nominal 16 MHz HSI without a PLL. SysTick provides a nominal one-millisecond acquisition interval. USART2 TX on PA2 sends 115200-baud, eight-bit, no-parity, one-stop-bit records through the onboard ST-LINK virtual COM bridge. The Nucleo supplies regulation, decoupling, reset, SWD, and USB connectivity. Those are supplied module facilities, not omitted imaginary components on the carrier.

The carrier is a cable-connected passive interface rather than a pin-compatible stacking shield. It measures 80 by 70 mm, has two copper layers, nominal 1.6 mm thickness, 0.3 mm traces, a closed outline, fourteen through-hole components, thirty electrical pads, and nine nets. There are no mounting holes or enclosure CAD. Use insulating supports, a cover, and suitable strain relief. No battery, charger, FPGA, antenna, radio, analog sensor, active level shifter, isolated supply, or target power output is present.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical through-hole 2.54 mm header |
| J2 | 1 | 1x4 vertical through-hole 2.54 mm header |
| Harness and support | 1 set | Labelled leads, insulating base, protective cover |

Resistors use DIN0207 geometry with 7.62 mm lead spacing. Headers use standard single-row vertical KiCad footprints. Select actual purchased components against package dimensions, mating geometry, lead spacing, and environmental requirements. No price, stock availability, assembly yield, or connector-fit guarantee is made. The design is limited to known common-ground zero-to-3.3 V push-pull logic. Five-volt, 1.8 V, negative, differential, relay, industrial, and unidentified signals are unsupported.

Each target input passes through 1 kOhm to its MCU-side node; a 100 kOhm pulldown connects that node to ground. The resistor path to ground is nominally 101 kOhm before leakage and source impedance. This series resistor is not a qualified overvoltage barrier. There is no dedicated ESD clamp, isolation, fuse, reverse-current protection, or hot-plug guarantee. Keep cables short and validate loading with the real target. Do not infer immunity or certified electrical safety from successful ERC or DRC.

| Function | Target connector pin 1 | J2 pin | Nucleo connection |
| --- | --- | --- | --- |
| TAG0 | J3 | 1 | CN8 pin 1, A0, PA0 |
| TAG1 | J4 | 2 | CN8 pin 2, A1, PA1 |
| ENQUEUE | J5 | 3 | CN8 pin 3, A2, PA4 |
| DEQUEUE | J6 | 4 | CN8 pin 4, A3, PB0 |

Pin two of each target header is ground. Both J1 pins are ground only. Neither accepts power. J2 contains only four signals and has no ground or power pin. Connect a separate Nucleo ground lead to J1. Nets ending `_IN` are target-side nodes; corresponding unsuffixed names are MCU-side nodes after the series resistors. Verify pin-one orientation and continuity instead of relying on wire colors.

Power the Nucleo from its documented ST-LINK USB connector after checking the purchased board revision and jumpers. A conservative source-planning allowance is 5 V at 500 mA; actual consumption is unmeasured and this is not USB certification. USB ground joins target ground. Power the observer before enabling target indications, and remove indications before removing observer power. Otherwise MCU protection structures may back-power an unpowered module. Confirm ground offset, leakage, thresholds, and power sequencing on the physical fixture.

Manufacturer reference: [ST UM1724, STM32 Nucleo-64 MB1136 user manual](https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf). Its NUCLEO-F401RE connector table maps CN8 A0/A1/A2/A3 to PA0/PA1/PA4/PB0. Consult STM32F401 reference manual RM0368 for RCC, GPIO, USART, watchdog, and clock details. Check the board's actual ST-LINK routing and solder bridges before assuming PA2 reaches the virtual COM bridge. Leave BOOT0 configured for normal flash boot. SWD programming and reset remain provided by the development board.

## Architecture and block diagram

```text
Authorized target diagnostic adapter       Passive carrier         Nucleo
TAG0 ------------------------------------- 1k --+------------------ PA0
TAG1 ------------------------------------- 1k --+------------------ PA1
ENQUEUE ---------------------------------- 1k --+------------------ PA4
DEQUEUE ---------------------------------- 1k --+------------------ PB0
                                                | each input
                                               100k
                                                |
Target reference ------------------------------ GND --------------- USB GND

GPIO samples -> continuity check -> bundled-tag / independent-strobe qualification
             -> prior setup and opposite-strobe guard -> tagged FIFO policy
             -> transactional accounting -> QO1 CRC frames -> USART2 TX
             -> ST-LINK virtual COM -> Ledger / History / Integrity screens
```

GPIOA and GPIOB are read separately, so this is not an atomic cross-port capture instrument. It is not a bus-speed logic analyzer. Use deliberately slow diagnostic events: begin with twenty milliseconds of tag setup, twenty milliseconds of strobe high, and twenty milliseconds with both strobes low after each operation. Independently inspect waveforms before shortening anything. Qualification cannot calibrate oscillator tolerance, port-read skew, interrupt latency, cable delay, or electrical thresholds. Processor-speed events require a trustworthy rate-limited adapter, not direct connection with an assumption that polling sees everything.

## Firmware contract and design decisions

`firmware/engine.c` contains a hardware-independent state machine. `drivers/board.c` configures input GPIO, SysTick, transmit-only USART2, reset-cause capture, and the independent watchdog. `drivers/serial.c` provides bounded, nonblocking transmission. `wire.c` serializes validated snapshots without heap allocation or formatted-print dependencies. `startup.c` supplies vector initialization and data/BSS initialization. `registers.h`, `board.h`, and `linker.ld` define the exact peripheral and memory interface. There is no RTOS or downloaded SDK requirement.

The two tag bits qualify as a single bundle. ENQUEUE and DEQUEUE qualify independently, preventing tag chatter from indefinitely hiding a held operation strobe. A changed candidate requires three subsequent consecutive matching sample intervals to qualify. The tag must also have five prior eligible setup observations before an operation is accepted. Setup cannot be earned on the operation endpoint itself. A raw-high opposite strobe, qualified opposite strobe, simultaneous two-edge observation, or raw/qualified tag disagreement is interface ambiguity. A short rejected pulse can remain invisible; qualification is not an exhaustive glitch detector.

At startup the operator must independently establish an empty target queue, then start or reset the observer with both strobes quiet. The firmware waits for fully qualified quiet strobes before tracking. Quiet levels alone cannot prove emptiness, and connecting halfway through a running queue is not a valid new baseline. Late attachment of the desktop to an already-running observer is supported because the observer retained the earlier events; late attachment of the observer to an unknown target queue is not.

An enqueue of a currently absent tag appends it to the retained queue. An enqueue of an outstanding tag is a duplicate finding. With four unique tags, an attempted fifth outstanding admission necessarily repeats a tag; it is not silently truncated. A dequeue of an absent tag is an orphan finding. A dequeue of a present but non-head tag is an overtake finding. Only a head dequeue removes an entry. There is no cancellation operation, timeout-based removal, arbitrary release, priority exemption, or automatic recovery after a fault. A held strobe generates one rising-edge operation, not repeated operations.

The queue is packed into eight bits, two bits per entry, head first in the least significant bits. Its length is the population count of `active`; unused high bits must be zero. This is necessary because tag zero is valid: an empty queue and a one-entry queue containing zero both have packed value zero, but different active masks. The companion decodes the same representation and checks that each live identity occurs exactly once. Queue entries survive faults so the expected order remains inspectable.

All sample accounting uses a speculative state copy. If any cumulative counter would overflow, the sample is rolled back and terminal exhaustion is recorded without a half-enqueue or half-dequeue. A skipped acquisition tick halts correlation rather than pretending the missing interval was stable. Duplicate calls at the same millisecond are ignored. Ordinary unsigned timestamp wrap is supported for adjacent ticks. After halt, only device time and the saturating transport-failure diagnostic may advance; queue evidence remains frozen. Restart requires a newly established target baseline and an explicit new observer run.

The core conservation laws are accepted enqueues = accepted dequeues + current depth; observed operation edges = enqueues + dequeues + rejected edges; and enqueues = closed nonempty episodes + current nonempty indicator + admissions behind existing work. Peak depth is bounded by four, current depth, and historical admissions. These constraints are enforced by both encoder and companion using sufficiently wide arithmetic. They catch many corrupt or impossible summaries but are not cryptographic proof or a reconstruction of every event between periodic records.

Fault bits are 1 overtake, 2 orphan dequeue, 4 duplicate live tag, 8 interface ambiguity, 16 acquisition gap, 32 counter exhaustion, and 64 watchdog startup failure. Only the first terminal reason is retained. Two rejected edges are possible only for simultaneous operation ambiguity. The periodic status is a retained snapshot rather than a full event log. Following a fault, the input-level fields describe the terminal observation, not continuously refreshed target voltages.

## Wire protocol and companion application

Each ASCII frame is `QO1,<kind>,<decimal fields>*<four uppercase CRC hex digits>\n`. CRC-16/CCITT-FALSE covers every byte before the asterisk, with polynomial 0x1021, initial 0xffff, no reflection, and no final xor. Boot kind B has one reset-flags field. Policy kind P carries `3,1,5,4`: qualification intervals, nominal sampling milliseconds, setup observations, and tag capacity. Policy repeats about once per second, separately from the one-shot boot record, so late attachment does not require a reboot or erase history.

Status kind S fields, in order, are `now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail`. The names `launched` and `completed` mean accepted enqueue and dequeue respectively. `rounds` counts nonempty episodes ending at an accepted final dequeue, not batches of four. `queued` counts admissions made while another tag was outstanding. Status is attempted every hundred nominal milliseconds when the transmitter is free. The single bounded transmit buffer has a hundred-millisecond timeout; failure increments `tx_fail`. No bytes received from the workstation command the target or clear the instrument.

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk installed. Python 3.13 was used for verification. No third-party Python package is required. Select the exact virtual serial device, normally `/dev/ttyACM0`, and choose New live. The app opens the port read-only and configures 115200 8N1. It refuses a second connection until the current one is disconnected, restores terminal attributes when possible, detects hangup, and retains evidence after disconnect. Choosing the exact device avoids accidentally treating an unrelated serial peripheral as this instrument.

The Ledger screen shows decoded head-to-tail order, counters, phase, and findings. History retains up to 256 accepted snapshots. Integrity shows source, live or replay provenance, policy compatibility, parser errors, reset metadata, freshness, and a freeze reason. Two advancing device timestamps are required before indicating live progress. Duplicate timestamps do not renew freshness. A long host pause or accumulated host/device drift freezes the session rather than presenting queued old data as fresh. This conservative protection can also reject a slow or poorly calibrated instrument; reconnect and investigate rather than disabling it to obtain a green indicator.

Sparse-record validation preserves FIFO survivors: accepted departures may remove only a prefix of the old queue, and surviving entries must lead all newly admitted entries. A silent reorder with unchanged counts is rejected even if each snapshot separately has a valid mask. Counter rollback, clock rollback, disappearing sticky faults, changed terminal evidence, and a later boot also freeze interpretation. Without a persistent boot identity, a lost boot frame plus plausible forward counters can still conceal a reset. CRC and transition checks do not authenticate a source.

Replay accepts only regular files up to two MiB, opens nonblocking, and checks type before reading so a FIFO cannot freeze the interface. Framing is bounded and resynchronizes on the complete QO1 marker. Partial EOF tails and malformed frames increment error counts. A file with no compatible status is rejected. Replay is explicitly non-live. Export creates a new mode-0600 JSON file, refuses overwrite and final-component symlinks, and includes author jayis1, source, provenance, integrity metadata, latest state, and retained history. Exports are unsigned and unencrypted; choose a trusted directory and preserve the integrity qualifications in any downstream report.

## Building, verification, and laboratory workflow

From this directory, run `make -C firmware all test` for target linking and host tests. Requirements are GNU make, host GCC with address/undefined-behavior sanitizer support, and arm-none-eabi GCC with newlib and nosys specifications. The result is `firmware/build/queue-overtake.elf` and a raw flash image `firmware/build/queue-overtake.bin`. Inspect the linker map before programming any module. No automatic flashing command runs during tests, and no test requires attachment to a live target.

Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v` for companion and actual Tk event-loop tests through Linux pseudoterminals. Run `python3 -m compileall -q app` for byte compilation. The fixture executable uses the production engine and encoder to produce explicitly synthetic observations, including valid reuse followed by an overtake. These are software-generated test data, not physical measurements. The independent C list oracle explores operation sequences including duplicate tags, absent tags, queue filling, head departure, and reuse; it does not simply compare the implementation to a second copy of its packed representation.

KiCad generation requires KiCad 9, its standard through-hole footprint libraries, and the matching `pcbnew` Python module. Run `/usr/bin/python3 tools/generate_hardware.py`, then `/usr/bin/python3 tools/check_hardware.py`. The checker executes ERC and DRC, exports a fresh schematic netlist, and compares every reference, value, footprint identity, pad, and net against the PCB and connectivity manifest. Native schematic symbols, routed copper, a project file, and the saved XML netlist are included. A clean rule check is not physical electrical qualification.

For an initial authorized experiment, first review the adapter and document the meaning of both strobes. Establish an empty queue with outputs inhibited. Check ground and continuity, power the observer, and only then enable low-energy diagnostic outputs. Run one tag through a complete cycle, then several arbitrary-order admissions and FIFO departures. Next test reuse while older work remains queued. Capture a deliberately out-of-order departure on the lab fixture and verify that the retained queue shows the expected older head. Reset the target baseline before restarting the observer after any halt.

Physical acceptance remains outstanding: verify resistor values and assembly, shorts, common-ground offsets, input loading, unpowered leakage, connector polarity, USB power sequencing, ST-LINK configuration, waveform setup margins, clock accuracy, watchdog recovery, and long-duration acquisition under real serial load. Use an independent logic analyzer to correlate instrument evidence with adapter transitions and functional effects. Until those checks pass, this is a build-tested software and CAD prototype, not fabrication-qualified instrumentation or a production safety device.
