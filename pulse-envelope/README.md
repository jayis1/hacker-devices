# Pulse Envelope — Four-Lane Privileged-Pulse Timing Evidence Bench

Author and creator: jayis1. Revision A. Wire protocol PE1. Firmware and application: GPL-2.0-only. Hardware utilities: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, an executable Linux desktop application, and routed native KiCad carrier files. No assembled hardware, electrical measurements, programming trial, calibration, or regulatory qualification is claimed. The instrument is a diagnostic observer, not a safety controller or an authorization enforcement mechanism.

## Purpose and repository-local originality

Pulse Envelope examines the shape of individual security-sensitive enable pulses. A maintenance-unlock output may be required to remain asserted long enough for a downstream component to recognize it, deassert before a maximum duration, and remain inactive for a mandatory recovery interval before the next operation. Those three obligations differ from average duty cycle or total operation count. A stream can satisfy a long-term exposure budget while containing one excessively long unlock, many inadequately short strobes, or two operations separated by insufficient recovery.

The device observes four independent, slow diagnostic outputs from an owned development target. Each lane records tracked starts, completed pulses, short pulses, long pulses, premature restarts, aborted observations, last completed width, and peak observed width. The fixed laboratory envelope is a minimum high duration of 10 ms, maximum high duration of 200 ms, and minimum low recovery duration of 50 ms. These are illustrative integration requirements, not universal access-control recommendations. Changing them requires firmware, parser, tests, and protocol compatibility review together.

The repository directories and README scopes were inventoried before implementation. Dwell Budget measures moving-window occupancy; Burst Budget models replenishing admission credits; Grant Ledger correlates request, approval, and single use; Dualrail Witness checks complementary encoding. Pulse Envelope instead evaluates per-pulse lower and upper width bounds and a nonbankable low recovery interval on each of four independent lanes. It deliberately reuses jayis1's four-input passive carrier topology, MCU support, bounded serial framing, and local desktop transport patterns. No claim is made that this is globally unprecedented or patentable. The new contribution is the bounded pulse-envelope evidence model and its integration into a reproducible bench design.

## Authorized use and ethical disclaimer

Use only on equipment you own or have explicit written permission to assess. The permission must cover physical attachment, diagnostic firmware instrumentation, planned workloads, electrical inspection, and retention of timing evidence. Red-team authorization does not automatically permit access to every board or every exposed test point. Keep the observer visible and label its harness. This release has no stealth function, target command channel, credential collection, exploit payload, or traffic injection capability.

Do not attach the carrier to deployed locks, vehicles in operation, medical equipment, life-safety systems, mains circuits, unknown field wiring, or high-energy machinery. Use a development fixture with hazardous actuators removed. Receive-only software does not eliminate disruption from ground loops, input loading, reverse connections, or input-diode back-power. Stop if attachment changes target behavior. A timing finding is supporting evidence that requires corroboration, not proof that the underlying product is exploitable or safe.

Timing records can reveal operational patterns even when no payloads or identities are collected. Agree on retention, store exports privately, and avoid customer names in filenames. The application has no network listener or upload function. Private file permissions do not provide encryption, authenticated evidence custody, secure deletion, or protection against a compromised workstation.

## Attack surface and threat model

The assessed surface is an instrumented output whose asserted lifetime represents a privileged operation. Examples include an isolated development build's service-mode indication, a secure peripheral's diagnostic write-enable mirror, or a laboratory privacy-shutter permission indication. The operator must establish what each GPIO actually means. If a compromised target emits misleading diagnostics, this instrument can produce internally consistent but misleading evidence. It does not inspect actual memory accesses, authenticate users, or attest firmware execution.

The trusted base consists of the physical harness, the reviewed observer image, the operator, the diagnostic adapter, and the host workstation. Inputs and replay files are untrusted for parser purposes. CRC detects accidental byte corruption, not malicious forgery. There is no signed telemetry, secure element, authenticated pairing, or persistent hardware identity. Anyone who controls the USB source or workstation can manufacture a plausible capture. Record target revision and connection details separately in the authorized assessment notes.

The observer's GPIOs remain inputs. USART2 transmits toward ST-LINK only; its receiver is disabled. There are no target-facing outputs, radios, command handlers, network stacks, removable media, automatic firmware updates, or remote configuration. Physical SWD access remains trusted. No signed bootloader, anti-rollback mechanism, flash evidence journal, or power-failure-safe archive is implemented. Reset clears every volatile measurement. Those omissions are explicit deployment limitations rather than implied security features.

## Hardware specifications and connections

The complete instrument comprises one external ST NUCLEO-F401RE development board, the supplied passive carrier, short insulated leads, a nonconductive support, and a Linux workstation. The STM32F401RE provides a Cortex-M4 processor, 512 KiB flash, and 96 KiB SRAM. Firmware assumes reset-default 16 MHz HSI operation, a nominal 1 ms SysTick, and USART2 at 115200 baud, eight data bits, no parity, one stop bit. The purchased Nucleo supplies regulation, decoupling, MCU support, SWD, reset circuitry, and ST-LINK USB virtual COM. The carrier is not a standalone MCU board.

The PCB measures 80 by 70 mm, with two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks, through-hole headers, and axial resistors. It connects by cables rather than stacking on the Nucleo. There are no mounting holes or enclosure drawings; provide insulated support and strain relief. There is no FPGA, analog transducer, antenna, battery, charger, regulator, isolator, or active buffer on the carrier. Its sensors are the four external logic indications.

| Lane | Target connector | MCU-side connector | Nucleo signal | MCU pin |
| --- | --- | --- | --- | --- |
| CH0 | J3 pin 1 | J2 pin 1 | A0 | PA0 |
| CH1 | J4 pin 1 | J2 pin 2 | A1 | PA1 |
| CH2 | J5 pin 1 | J2 pin 3 | A2 | PA4 |
| CH3 | J6 pin 1 | J2 pin 4 | A3 | PB0 |
| Ground | J3–J6 pin 2 | J1 both pins | GND | GND |

Each channel uses a 1 kΩ series resistor followed by a 100 kΩ MCU-side pulldown. R2, R4, R6, and R8 are series parts; R1, R3, R5, and R7 are pulldowns. Fit 1% quarter-watt axial DIN0207 resistors with 7.62 mm lead spacing. J1 and J3–J6 are two-pin 2.54 mm vertical headers; J2 is a four-pin header of the same pitch. Verify purchased part dimensions and mating orientation. The machine-readable BOM, values, footprints, and pin nets are in kicad/connectivity.json.

Only common-ground, zero-to-3.3 V push-pull logic is supported. There is no 5 V input guarantee, negative-voltage tolerance, differential receiver, ESD suppressor, fuse, or galvanic isolation. The series resistors do not establish an electrical safety boundary. Pulldowns make disconnected signals tend low but cannot distinguish unplugging from legitimate inactivity. Input impedance, leakage, cable capacitance, source drive strength, and logic thresholds must be checked on the assembled setup.

Power the Nucleo through its intended ST-LINK USB connector with its normal documented jumper configuration. Reserve a conservative 5 V, 500 mA source allocation for planning, not as a measured current draw. Never attach target supply power to J1 or J2. Establish ground first, power the observer before enabling target signals, and disconnect signals before removing observer power. Inspect virtual-COM solder bridges and header orientation against ST UM1724 for the purchased revision; inspect registers against RM0368. The live manufacturer PDF fetch failed during this run, so these external revision checks remain a physical bring-up prerequisite rather than a claimed fresh datasheet review.

## Architecture and block diagram

```text
Authorized target outputs       Passive carrier       External NUCLEO-F401RE
CH0 --> J3.1 --> 1k --> + --> J2.1 -------------------> PA0 / A0
CH1 --> J4.1 --> 1k --> + --> J2.2 -------------------> PA1 / A1
CH2 --> J5.1 --> 1k --> + --> J2.3 -------------------> PA4 / A2
CH3 --> J6.1 --> 1k --> + --> J2.4 -------------------> PB0 / A3
                       | each has 100k to GND
GND -------------------+ --> J1 --------------------> GND

GPIO reads -> continuity check -> independent qualification
           -> four pulse/recovery state machines
           -> transactional cumulative ledger -> PE1 CRC encoder
           -> USART2 PA2 TX -> ST-LINK USB virtual COM
           -> Linux Envelope / History / Integrity screens
           -> explicit private JSON export
```

GPIOA and GPIOB are read separately. Inter-port skew, interrupt latency, actual oscillator tolerance, and electrical propagation are not measured. Cross-channel simultaneity is not a feature of this design; lanes are independent. It is not a logic analyzer capable of resolving fast strobes or narrow glitches. Start with deliberately paced diagnostic signals and verify them using an external calibrated analyzer.

## Firmware semantics and design decisions

Firmware is C11 with explicit startup vectors, a linker script, MCU register definitions, board support, a nonblocking serial driver, the pulse engine, and a checked decimal/CRC encoder. No external MCU SDK download is required. Hardware initialization assumes a normal reset, not a bootloader or debugger that has changed the clock tree. The only enabled interrupt source is SysTick. The main loop samples each new millisecond, services at most one serial byte per iteration, schedules telemetry, and feeds the watchdog.

Each lane qualifies independently. On a raw change, its candidate age becomes zero. Three further successive millisecond observations of that same candidate establish the new qualified level. Thus a transition requires the initial changed sample plus three confirming samples. Unrelated channel chatter cannot delay that qualification. Pulses that fail qualification are not counted; the measured object is the qualified signal, not all electrical activity. Rejected low glitches inside a high interval do not split the pulse. The minimum width check therefore cannot detect every physically short pulse, especially pulses shorter than the qualification requirement.

At startup each lane is UNKNOWN. A high encountered at attachment has no known leading edge and never becomes a fabricated tracked pulse. A qualified low starts LOW with age zero. LOW age advances toward 50 and then saturates. A qualified rise starts HIGH and increments starts. If preceding qualified low age is less than 50, rapid also increments. Exactly 50 is accepted. Waiting longer does not accumulate credits for subsequent rapid pulses: every falling edge starts a new recovery interval.

HIGH age starts at zero and advances once per contiguous tick before processing a possible qualified fall. A fall completes the pulse and stores its width. Widths below 10 increment shorts; exactly 10 is accepted. Passing 200 increments longs once, even if the output remains high indefinitely; exactly 200 followed by a fall does not increment longs. Width and peak saturate at 201. A displayed value of 201 means at least 201 sampled milliseconds, not an exact measurement. Longs counts threshold crossings, including active or subsequently aborted pulses; it is not restricted to completed pulses. Rapid and short/long can describe the same pulse, but a pulse cannot be both short and long.

Each lane maintains the conservation equation starts = completed + aborted + one if currently HIGH. A nonunit timestamp delta declares an acquisition gap. The engine records the gap and missed ticks, aborts tracked HIGH pulses, clears prospective phases and qualification, and requires a newly qualified low baseline. It does not assume that missing time was low, high, or a continuation of the previous pulse. Historical totals, last completed widths, peaks, and sticky findings remain. A gap itself does not invent a short or long finding for the unknown interval.

Duplicate timestamps do nothing. Unsigned subtraction supports ordinary 32-bit tick wrap; observation spanning an entire tick cycle is ambiguous and unsupported. Diagnostic counters never silently wrap. Each acquisition update is performed on a temporary copy and committed only if every required counter update succeeds. Exhaustion discards that whole sample's accounting and enters a terminal halted state, preserving pulse conservation. The last retained phase may still say HIGH after halt; it is frozen evidence, not a claim about current input. Device time and transport diagnostics can continue advancing while measurements remain halted.

The independent watchdog uses the internal low-speed oscillator and a nominal roughly two-second timeout. Initialization waits are bounded by the millisecond clock. A failed watchdog start sets a terminal fault before the first status record. This is tested control flow, not a physical watchdog timing guarantee. A stuck CPU, stopped clock, power loss, or reset can erase volatile state; the observer does not provide persistent continuity proof.

## Application, framing, and evidence handling

Run `python3 app/console.py` on Linux with Tk installed. Enter the explicit serial device path and choose New live session. Envelope shows all four lanes, current phases, censored ages, pulse counts, findings, and acquisition diagnostics. History retains the most recent 256 accepted status records. Integrity exposes source, provenance, policy compatibility, freshness, reset flags, parser errors, and any freeze reason. There is no automatic device discovery or authenticated device identity. New live and Replay replace the old session, including when opening fails: export important evidence before either action. Disconnect alone preserves it.

The application opens the serial terminal read-only, configures 115200 8N1, flushes preexisting input, and polls without blocking the GUI. Actual serial hardware is not exercised by the software tests. Permission errors and disappearing devices are shown without pretending the stream remains live. Terminal settings are restored when possible. The observer accepts no commands, so the app cannot arm target actions or clear embedded counters.

PE1 frames are ASCII `PE1,kind,decimal-fields*CCCC` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, with polynomial 0x1021 and initial value 0xffff. CCCC is four uppercase hexadecimal digits. B carries one reset-flags integer and is sent once at startup. P carries `3,10,200,50` and repeats approximately every second, allowing late attachment without interpreting policy repetition as reset. S is scheduled approximately every 100 ms and carries eight common fields followed by ten fields for each lane.

The common field order is now, levels, valid, halted, faults, gaps, missed, tx_fail. Lane order is CH0 through CH3; within each lane it is phase, age, starts, completed, shorts, longs, rapid, aborted, last, peak. All fields are unsigned decimal integers bounded to 32 bits. Phase is zero UNKNOWN, one LOW, or two HIGH. Levels and valid use lane-number bits. Fault bits zero through six indicate short, long, rapid, acquisition gap, transmitter loss, exhaustion, and failed watchdog startup. All findings are sticky until reset.

Frames and UART buffers are bounded to 768 bytes. A stuck transmission is abandoned after 100 ms without blocking acquisition. Full-marker resynchronization recovers when a new frame follows a truncated one. The parser rejects bad checksums, unsupported fields, impossible pulse conservation, inconsistent flags, invalid phases, and widths beyond their censoring bound. Structural validity is not cryptographic authenticity or a proof that every inter-record history is realizable.

Only advancing accepted statuses refresh live transport freshness. Identical timestamps do not renew it; contradictory duplicates freeze evidence. Counter rollback, disappearing sticky flags, backwards device time, and a boot record after measurements also freeze the session. Live host pauses longer than one second and cumulative host/device divergence beyond the documented implementation allowance trigger reconnect requirements rather than promoting queued old snapshots to fresh evidence. The allowance is 500 ms plus five percent of host elapsed time. Without a persistent boot identifier, some resets whose counters and timestamps appear to advance remain indistinguishable from continued operation.

Replay accepts only an opened regular file of at most two MiB, rejects FIFOs without blocking, and marks provenance as not live. EOF with a partial frame is an error. Exports use exclusive creation, mode 0600, and refusal to follow a final symlink; existing destinations are not overwritten. JSON includes author jayis1, source, status, bounded history, errors, freeze reason, connection state, policy, and age. It is a session snapshot, not an immutable full-resolution audit log. Delete retained files manually under the assessment retention policy.

## Reproducible builds, tests, and practical use cases

Install GNU Make, host GCC, Arm GNU Embedded GCC with newlib/nosys support, Python 3 with Tk, KiCad 9 with standard footprint libraries and Python bindings, and Xvfb for GUI tests. Run `make -C firmware all test`. The target build links Cortex-M4 firmware and creates build/pulse-envelope.elf and build/pulse-envelope.bin. Program at 0x08000000 through an authorized ST-LINK workflow and reset normally. Programming and execution on a real board remain outstanding acceptance steps.

Run `xvfb-run -a /usr/bin/python3 tests/test_app.py`. The tests consume synthetic traces from the actual C engine and encoder, exercise malformed and impossible snapshots, file protections, framing recovery, reset suspicion, live pauses, and the real Tk event loop over a Linux pseudo-terminal. They navigate screen content, observe hangup, load a replay, and export retained evidence. Synthetic fixture outcomes are not measurements from physical equipment. C tests cover exact thresholds, long held signals, independent chatter, gaps, startup high, timestamp wrap, terminal exhaustion, random sample sequences, main-loop scheduling, watchdog failure, and a simulated UART register page.

Generate CAD with `/usr/bin/python3 tools/generate_hardware.py`; verify with `/usr/bin/python3 tools/check_hardware.py`. The checker runs ERC and DRC and compares every schematic pin, board pad, value, and footprint identity with the manifest and saved netlist. The schematic uses actual authored connector and resistor symbols; the board has placed library footprints and routed nets. Passive-only ERC cannot validate external power, voltage limits, harness orientation, purchased board revisions, or signal quality. Visually review the design and consult VERIFICATION.md before ordering.

An authorized researcher can compare the timing envelopes of four instrumented firmware builds under identical paced workloads. A red team can document a development unit's unlock indication exceeding its stated maximum without collecting credentials or issuing exploit payloads. A penetration tester can show that repeated requests violate mandatory recovery even when rolling activity stays low. A product team can catch too-short diagnostic strobes that downstream slow logic may miss. In every case the workload generator is separate, the meaning of each lane must be documented, and findings require corroboration against source review and a calibrated analyzer.

Physical acceptance must include continuity, shorts, input loading, threshold margin, connector orientation, power sequencing, actual sample rate, oscillator error, baud rate, watchdog behavior, unplugging, and reset behavior. Compare known pulse widths on both sides of every boundary. Confirm target pins remain inputs throughout reset and initialization. Fabrication, assembly, calibration, ESD, EMC, and regulatory compliance remain unverified. Software and CAD success do not waive those requirements.
