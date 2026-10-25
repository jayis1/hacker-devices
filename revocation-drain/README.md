# Revocation Drain — In-Flight Privilege Retirement Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol RD1. Firmware and application: GPL-2.0-only. Hardware tools: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design, with executable embedded firmware, a Linux desktop application, and routed native KiCad carrier files. Physical fabrication, programming, timing calibration, and electrical qualification are outstanding. This is an observation instrument, not a privilege controller, safety controller, or certification of correct revocation.

## Purpose and repository-local originality

Revocation Drain examines the boundary between withdrawing permission and completing work that was already admitted. A security controller may set a revoked flag immediately, while a peripheral still accepts new work or reports revocation complete before its existing transaction has stopped. That discrepancy matters for debug-session retirement, maintenance-mode exit, secure accelerator reassignment, and device-local administrative sessions. Ordinary application logs can conceal the ordering because the component reporting successful revocation is also responsible for implementing it.

The instrument samples four owner-provided diagnostic indications: REVOKE, ADMIT, BUSY, and ACK. It observes whether admission crosses the revocation boundary, whether BUSY has remained sampled low for a minimum quiet interval before acknowledgment, whether acknowledgment arrives within a bounded deadline, and whether activity reappears after acknowledgment while revocation remains asserted. It never sends work, revokes access, writes a target register, generates credentials, or reads a protected payload. The researcher creates the workload separately under the approved test plan.

Repository review included the existing device directory inventory and README purposes. Grant Ledger addresses single-use authorization. Lease Watch examines heartbeat-to-enable contracts. Receipt Witness correlates ordered audit receipts. Reset Quarantine checks stable readiness before post-reset access. Erase Witness checks sanitization sequencing. Handoff Audit checks break-before-make domain transitions. None of those implements this admission-freeze plus in-flight-drain plus acknowledgment contract. The distinguishing measurement is not merely a renamed timer: acknowledgment must follow an observed quiet period within a revocation episode, and the revoked interval continues to prohibit admissions afterward. Worldwide originality is not claimed.

The low-voltage carrier, startup, basic GPIO driver, and UART transport reuse jayis1's existing Latency Pair platform. The policy engine, protocol, app, and policy tests are specific to this device. Reusing a known carrier reduces unrelated hardware risk; it does not establish that a new security policy or a previous board has been physically validated. The deliverable includes a complete local copy of the necessary platform files, so building this folder does not require compiling another device's firmware.

## Attack surface and threat model

A target under test can misreport its internal activity. The observer sees diagnostic signals, not the underlying queue, memory writes, outstanding DMA transactions, or actual access decisions. A low BUSY indication therefore means only that the diagnostic adapter reports no work. It does not prove that keys were erased, a DMA engine was stopped, or a hardware bus is quiescent. Validate the instrumentation against a separate logic analyzer or target trace before interpreting findings as a product defect.

The trusted boundary includes the workstation, installed observer firmware, Nucleo development board, physical wiring, and approved diagnostic adapter. The serial byte stream and imported recordings are treated as malformed-input surfaces. The app bounds frame storage, verifies CRC and field ranges, checks conservation of trials, rejects incompatible policies, and freezes evidence on clock or cumulative-counter rollback. Those checks detect inconsistency; they do not authenticate a source. A malicious workstation or adapter can forge an internally consistent recording.

There is no radio, network service, cloud dependency, secure element, authenticated pairing, persistent boot counter, signed update facility, or encrypted log storage. CRC-16 is accidental-corruption detection, not a signature. Physical access to ST-LINK permits firmware replacement. The host application opens the serial terminal read-only and the MCU enables only UART transmission toward ST-LINK. No application command can be translated into a target action, because there is no command parser and all target-facing GPIOs remain inputs.

Observation state is volatile. Resetting the Nucleo destroys prior cumulative counters and sticky findings. Repeated policy messages support late attachment without pretending to be reset messages. The app detects a boot message after measurements, backward time, falling cumulative counts, and disappearing fault bits. A reset whose subsequent values all appear forward can escape this heuristic. Preserve session provenance externally and never describe reconnect as proof of an uninterrupted capture. An unplugged wire can look like a legitimate low indication; there is no line-integrity test.

## Legal and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Authorization must include physical attachment, diagnostic firmware changes, workload generation, and retention of timing evidence. Permission for a general penetration test does not automatically authorize connecting an unisolated electronic board to arbitrary test points. Keep the device visible and label every cable. Stop if attachment changes target behavior. The author, jayis1, does not authorize unauthorized interception, covert collection, destructive testing, or interference with third-party equipment.

Operate on low-energy laboratory fixtures with hazardous actuators disconnected. Never attach this board to mains, occupied access-control systems, medical equipment, operating machinery, vehicles in service, or life-safety circuits. Receive-only firmware is not galvanic isolation or electrical invisibility. Incorrect ground potential, unexpected voltages, and power sequencing can disturb a target even when the software never drives a signal. This prototype is not suitable for safety decisions or production interlocking.

Timing metadata can reveal operational patterns. Agree on retention, keep filenames free of personal identifiers, and store exported evidence under the engagement's approved process. The app has no automatic recorder or background upload. Explicit JSON exports are created with private file permissions and refuse overwrites. Permissions are not encryption, secure deletion, or protection against a compromised operating system. The operator remains responsible for backup, access control, and disposal of sensitive reports.

## Hardware specifications and bill of materials

The complete instrument consists of an external ST NUCLEO-F401RE, this passive carrier, insulated leads, an insulating support, and a Linux workstation. The external module supplies the STM32F401RET6 Cortex-M4 MCU, 512 KiB flash, 96 KiB SRAM, reset circuitry, voltage regulation, decoupling, SWD access, and ST-LINK USB virtual COM interface. These functions intentionally live on the purchased module; the carrier is not advertised as a standalone processor board.

Firmware uses the nominal 16 MHz reset HSI oscillator, one-millisecond SysTick, and USART2 at 115200 baud, eight data bits, no parity, one stop bit. No PLL or floating-point unit is required. The carrier is an 80 by 70 mm, two-layer board of nominal 1.6 mm thickness with 0.3 mm tracks. It connects by individual wires rather than stacking directly on the Arduino headers. There are no mounting holes: provide an insulating support and strain relief. There is no FPGA, radio, antenna, battery, charger, mass storage, or analog sensor.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE with ST-LINK |
| R1, R3, R5, R7 | 4 | 100 kΩ, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm through-hole header |
| J2 | 1 | 1x4 vertical 2.54 mm through-hole header |
| Harness and support | 1 set | Insulated leads and nonconductive support |

Resistor footprints have 7.62 mm lead spacing. Confirm actual purchased geometry and pin-one markings before assembly. J1 has ground on both pins; it is not a power header. J2 pins one through four connect to Nucleo CN8 pins one through four: A0/PA0, A1/PA1, A2/PA4, A3/PB0. J3 pin one receives REVOKE, J4 pin one ADMIT, J5 pin one BUSY, and J6 pin one ACK. Each of these connectors has ground on pin two. Unsuffixed net names are the MCU side of the series resistor; `_IN` is the external target side.

Only confirmed zero-to-3.3 V push-pull diagnostic outputs with a shared ground are supported. Each channel presents a 1 kΩ series resistor followed by a 100 kΩ pulldown to ground. This is approximately 101 kΩ DC loading, not an isolation barrier or universal logic-level converter. There are no dedicated clamps, certified ESD protection, reverse-polarity protection, or overvoltage protection. Do not infer 5 V compatibility from generic STM32-family advertising. Input fault tolerance depends on the exact pad and operating conditions.

Power the Nucleo through ST-LINK USB according to the board manual. Allocate a conservative 5 V, 500 mA supply budget for planning; that is not measured consumption or a USB compliance result. No target supply rail connects to the carrier. Connect the intended common ground before diagnostic leads, and disconnect diagnostic leads before removing observer power to avoid back-power through MCU protection structures. Verify ground potential and target response to loading before use. Review the exact Nucleo board revision and solder bridges for USART2-to-ST-LINK routing.

## Architecture and block diagram

```text
Authorized target adapter       Passive carrier              NUCLEO-F401RE
REVOKE -------------------------- 1k --+----------------------- PA0 / A0
ADMIT  -------------------------- 1k --+----------------------- PA1 / A1
BUSY   -------------------------- 1k --+----------------------- PA4 / A2
ACK    -------------------------- 1k --+----------------------- PB0 / A3
                                      | each: 100k to GND
GND ---------------------------------------------------------- GND

GPIO sample -> continuity check -> admission boundary monitor
                                -> drain/quiet/ack state machine
                                -> sticky findings + cumulative conservation
                                -> RD1 bounded ASCII / CRC encoder
                                -> nonblocking USART2 PA2 TX
                                -> ST-LINK virtual COM -> Linux Tk console
```

GPIOA and GPIOB are read consecutively. ACK is on another port, so these are not simultaneous physical observations. There is no debounce filter: short sampled glitches remain observable, but pulses entirely between samples can be missed. Use slow diagnostic signals, preferably held at least ten milliseconds, and validate pulse detection independently. The firmware observes nominal millisecond samples, not calibrated nanosecond ordering. Events qualifying in one sample cannot be ordered internally and are treated conservatively where the policy requires ordering.

## Firmware policy and design decisions

A fresh observer waits for an all-low baseline after its initial sample. It does not retroactively create a trial when attached halfway through revocation. Once ready, normal admission and BUSY activity may occur before REVOKE. The first observed REVOKE assertion starts a drain trial, sets age to zero, and begins watching quiet time. REVOKE must remain asserted throughout the trial. A new trial requires returning through an all-low baseline after withdrawal, timeout, gap, or completed revocation release.

ADMIT is an event indication: each sampled rising edge increments the admissions total. An edge with REVOKE high either in the previous or incoming sample increments blocked-boundary events and sets the admission-leak finding. An ADMIT indication already held high when REVOKE rises also increments that boundary counter. It is not counted as a new admission edge. Consequently blocked-boundary events are not a strict subset of admission edges: one held indication spanning several revocations can produce several boundary findings. The report does not silently equate these different denominators.

BUSY describes in-flight activity. When it first becomes sampled low during a trial, quiet age starts at zero. Every subsequent consecutive low sample increments quiet age to a saturating maximum of twenty milliseconds. BUSY becoming high clears quiet qualification. A rising ACK is accepted only if the previous sample already had twenty milliseconds of quiet and the incoming sample still has BUSY and ADMIT low. ACK in the exact sample that first reaches twenty is early, because that does not demonstrate acknowledgment afterward. ACK simultaneous with REVOKE is also early.

An early acknowledgment sets a sticky finding and increments its counter but does not terminate the trial. If ACK later falls and rises after sufficient quiet, that later edge can complete the handshake. Completed therefore means an accepted acknowledgment after quiet, not a clean or safe trial. Previous admission leaks or early acknowledgments remain visible. The observer deliberately separates completion counts from security verdicts so that a later valid event cannot erase an earlier violation.

The acknowledgment deadline is five hundred milliseconds from REVOKE assertion. An eligible edge at age five hundred is accepted; the next sample exceeds the bound and abandons the trial. Deadline processing precedes acceptance at an overdue sample. A withdrawal of REVOKE while draining abandons the trial with a distinct finding. A completed trial enters the acknowledged state until REVOKE is released. BUSY or ADMIT observed in that state sets activity-after-acknowledgment, including on the release sample. New admission edges remain counted by the boundary monitor independently of the drain state.

Duplicate timestamps are ignored. A nonzero elapsed tick difference other than one means acquisition continuity was lost. The firmware increments gaps, abandons an active trial, and requires a new baseline rather than extrapolating quiet through unknown time. Completed and abandoned totals, admission counters, and sticky findings survive. The conservation identity is `started = completed + abandoned + active`, where active is one only in the draining state. Integer exhaustion enters terminal fault instead of wrapping; closing an active trial preserves conservation before stopping.

The main loop samples before servicing telemetry. The UART driver copies a bounded frame, sends at most one available byte per iteration, and drops a stalled transmission after one hundred milliseconds. There is no blocking transmit wait or dynamic allocation. Startup reports reset cause once, policy repeats approximately every second, and status transmits approximately every hundred milliseconds. Failed watchdog initialization puts the device into terminal fault before its first status. The watchdog and acquisition timing still need physical validation; host testing does not measure oscillator tolerance or real interrupt latency.

## Companion application and protocol

Run `python3 app/console.py` from this directory on Linux with Tk installed. The Drain policy screen shows current state, started, completed, abandoned, active age, quiet age, admission counters, early acknowledgments, and named sticky findings. Snapshots displays up to 256 accepted periodic statuses, not every individual target event. Integrity exposes provenance, parser-error count, policy compatibility, reset flags, freshness, connection state, and frozen-evidence reason. These screens reflect the same validated model.

Select the exact terminal path, confirm authorized bench use, and choose New live. A successful connection starts a new app evidence session without resetting the MCU. Disconnect preserves the previous source label and retained data. Replay imports a regular file of at most one MiB, opened nonblocking before checking its type, so a FIFO cannot block the GUI. Replay completion rejects an incomplete trailing frame and a recording with no accepted status. Export JSON creates a new private file and refuses overwrite. Export before selecting another session if previous evidence must be retained.

Records are ASCII `RD1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE covers all bytes before `*`, uses polynomial 0x1021, initial value 0xffff, no reflection or final XOR, and four uppercase hexadecimal digits. B carries one reset-register value. P carries `20,500`, meaning quiet interval and deadline. S carries fourteen unsigned integers:

```text
time,state,levels,started,completed,abandoned,age,quiet,
admissions,blocked,early,gaps,tx_fail,faults
```

States zero through four mean waiting baseline, ready, draining, acknowledged, and terminal fault. Input bits zero through three mean REVOKE, ADMIT, BUSY, ACK. Fault bits zero through nine mean admission leak, early acknowledgment, timeout, revocation withdrawn, activity after acknowledgment, acquisition gap, transmit loss, watchdog unavailable, counter exhaustion, and orphan acknowledgment. An ACK rising outside a trial or acknowledged state is orphaned. Faults remain sticky until observer reset.

The app requires compatible policy before status acceptance, bounds unsigned fields, verifies trial conservation, and checks state-dependent age and quiet constraints. Repeated identical timestamps do not refresh freshness. Contradictory duplicate timestamps, cumulative rollback, disappearing sticky bits, or a terminal fault disappearing freeze the retained evidence. Live telemetry older than one second is visibly stale, even if the serial descriptor remains open. Linux hangup flags cause disconnect. A resumed stream can continue the same model only if its counters and chronology remain consistent; freshness is evidence age, not proof that every intermediate status arrived.

## Practical workflows and interpretation

For a product-security review, instrument the real admission decision rather than an earlier API call. Instrument BUSY across the complete protected operation, including downstream effects that must finish before retirement. Generate ordinary work with the owner's normal test harness, assert revocation through the supported management path, and observe the resulting sequence. First run an idle retirement, then one with long-running work, then repeated retirement cycles. Preserve adapter source and exact firmware revisions with exported findings.

For an authorized red-team lab exercise, construct an owned training fixture that acknowledges retirement immediately but keeps admitting work for a short interval. The instrument demonstrates why a successful management response does not necessarily mean that authority has stopped. After remediation, repeat identical workloads and compare sticky findings and abandoned totals. Do not convert the observer into an active exploit client or attach it to operational safety equipment simply because the underlying scenario resembles access revocation.

For penetration-test remediation verification, compare captures made with the same signal mapping, workload schedule, power conditions, and nominal timing policy. A clean capture demonstrates only that no sampled contradiction was observed in that experiment. Lost frames, acquisition gaps, instrument restarts, low duty cycles, adapter errors, and events between samples can all limit confidence. A violation should trigger independent trace collection and source review, not an automatic vulnerability severity score.

## Build, verification, and physical bring-up

The firmware needs GNU Arm Embedded GCC, newlib with `nosys.specs`, GNU Make, and a native C compiler for tests. The host application uses Python's standard library and Tk; there are no pip, npm, or cloud dependencies. CAD generation and checking use KiCad 9 Python bindings and `kicad-cli`. Debian packages commonly provide these as `gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`, `python3-tk`, `kicad`, and `xvfb`. Exact versions actually exercised are recorded in VERIFICATION.md.

```sh
make -C firmware all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 -m unittest discover -s tests -p test_gui.py
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 app/console.py
```

The checker exports a fresh schematic netlist and compares reference, value, footprint, and every pin-to-net assignment against the PCB and connectivity manifest. ERC and DRC must return zero violations, with no unrouted items. The schematic contains embedded real symbols and the board contains footprints and routed tracks; filenames alone are not the acceptance criterion. The external Nucleo module is represented by the documented connector interface, not a fictitious carrier MCU symbol.

Before fabrication, inspect the rendered schematic, copper clearances, connector orientation, resistor geometry, and harness labels. Before powering, measure continuity and resistance with all modules disconnected. Verify no supply rail is connected to a signal header. Bring up the Nucleo alone, program the generated image at the normal flash base using an approved ST-LINK workflow, then measure UART output and the one-millisecond timebase. Only afterward attach a current-limited, known-voltage diagnostic fixture and compare every state transition against a logic analyzer.

Host sanitizer tests, actual target linking, GUI pseudo-terminal tests, and CAD connectivity checks are software evidence. They do not demonstrate successful flashing, safe electrical behavior, calibrated timing, EMC performance, ESD survival, or regulatory compliance. Keep these outstanding gates explicit in any downstream manufacturing or deployment decision. Do not fabricate boards for operational deployment solely because the software tests pass.

## Reference documents

Consult the exact module revision against ST's NUCLEO-F401RE product page and UM1724 Nucleo-64 user manual, the STM32F401xD/xE datasheet, and RM0368 reference manual. Useful entry points are https://www.st.com/en/evaluation-tools/nucleo-f401re.html and https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf . Pin names in this design follow those documents; verify the physical header rather than assuming every Nucleo has the same mapping. Author and creator of this design and implementation: jayis1.
