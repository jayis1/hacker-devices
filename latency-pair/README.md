# Latency Pair — Paired-Class Timing Evidence Bench

Author and creator: jayis1. Hardware revision A. Wire protocol LP1. Firmware and companion application: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, a working Linux desktop application, and routed native KiCad carrier files. Fabrication, programming, physical electrical tests, oscillator calibration, and measurements of an actual target have not been performed. This is a diagnostic timing observer, not a constant-time certificate, an exploitation tool, or a security enforcement device.

## Purpose and overview

Latency Pair investigates whether two explicitly labelled classes of operation have consistently different low-speed execution durations. Embedded authentication and privacy features often contain delays that are much larger than an individual processor instruction: a missing user may return immediately, an existing user may trigger storage access, or a rejected request may travel through a different scheduler path. A software log can hide these effects if timestamps are coarse, buffered, or taken at different places. The instrument observes an approved target's BUSY indication independently, associates it with an adapter-provided class, and compares adjacent operations as pairs.

The target exports four diagnostic signals: BUSY, CLASS, VALID, and INHIBIT. Class zero is called A and class one is called B. The labels contain no credential, username, secret, address, or payload. They are experimental categories selected by the authorized operator. The instrument never determines whether a credential is correct; it does not submit requests or search a space of secrets. It records the timing of a workload generated separately by the owner. A completed cohort contains thirty-two accepted pairs, each with one A operation and one B operation.

The useful output is deliberately descriptive: completed pair count, total duration for each class, the number of pairs where each class was slower, ties, pairs whose absolute difference reached five sampled milliseconds, and the maximum absolute difference. The desktop computes the signed mean B-minus-A difference from the two totals. These measures help prioritize investigation of gross timing asymmetry. They are not p-values, confidence intervals, evidence of exploitability, or proof of resistance to timing attacks. No raw per-operation payload or timing waveform is retained.

## Originality within this repository

Existing device directories and README scopes were inspected before choosing this design. Dwell Budget measures occupied time within a rolling privilege window. Retry Ledger checks failure budgets and lockout indications. Handoff Audit examines break-before-make domain switching; Grant Ledger consumes single-use authorization credit. The power and electromagnetic instruments have different acquisition mechanisms and objectives. None of those reviewed scopes describes adjacent class-pair timing, whole-pair rejection, bounded cohorts, and directional pair counts.

Latency Pair reuses jayis1's passive four-input carrier layout, minimal STM32 board support, bounded transmit driver, CRC encoder structure, and Linux serial/Tk infrastructure. That reuse is intentional rather than a claim that the resistor network is a new invention. The timing acquisition contract, paired cohort algorithm, protocol fields, presentation, and regression tests are specific to this device. Originality is claimed only relative to this repository inventory, not as a worldwide prior-art search or a patentability claim.

## Attack surface and threat model

The assessed surface is class-dependent execution time in an owned development target. Potential defects include asymmetric validation paths, class-dependent retry delays, unexpected cache or storage accesses, and different scheduling behavior for otherwise comparable requests. The test adapter must explain exactly what the BUSY interval includes. A pulse around a log write is not necessarily the duration externally observable by a network client. Correlate the observer with the externally visible response before claiming a real information leak.

The operator, workstation, installed observer firmware, wiring, and external development board are trusted. Target diagnostics and captured byte streams may be malformed or misleading. A malicious target can fabricate a perfectly consistent BUSY signal. VALID is an instrumentation assertion, not an independent attestation that the operation was comparable. The observer cannot detect a missing wire that happens to resemble a legitimate low level, discover hidden activity between samples, or establish that CLASS was assigned without looking at an outcome.

There is no authentication, pairing service, network stack, secure element, persistent boot identifier, signed capture, or signed update mechanism. CRC detects accidental corruption only. Physical possession of ST-LINK permits firmware replacement. A compromised workstation can forge exported JSON. These limitations are acceptable only inside the stated physically controlled bench model. Do not advertise the serial connection as authenticated evidence transport or expose it through an untrusted remote gateway.

Target-facing pins are inputs throughout operation. USART2 has transmission enabled toward ST-LINK, but no receiver or command parser. The app opens the serial terminal read-only and sends nothing. Boot, reset, application disconnect, malformed serial input, and replay cannot initiate a target action because the device has no active target interface. Programming is a separate physical maintenance operation, not an app feature. An operator resets the Nucleo to start a new cohort; reconnecting the app does not reset firmware or silently begin a new experiment.

## Authorized use and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Permission must cover physical attachment, diagnostic firmware changes, experimental workloads, and retention of activity timing. A penetration-testing engagement does not automatically authorize connecting an unisolated board to every test point. Keep the instrument visible, label its cables, and document the target and adapter versions in the assessment record. Do not use it for covert collection, credential discovery, or unauthorized access.

Restrict initial work to low-energy development fixtures with hazardous actuators disconnected. Never connect this carrier to mains wiring, vehicles in service, medical devices, operating industrial machinery, occupied access-control installations, or life-safety circuits. Receive-only software is not electrical isolation. Incorrect ground potential, reversed wiring, input loading, or back-power can disturb or damage a target. Stop immediately if attachment changes behavior. The author, jayis1, does not authorize misuse or claim suitability for safety-critical measurement.

Timing metadata can still reveal activity patterns even without payloads. Agree on retention before collecting, use neutral filenames, and avoid personal identifiers in exports. The application has no cloud upload, network listener, background archive, or automatic capture-file creation. Explicit exports use private permissions and refuse overwrites. Private permissions are not encryption, secure deletion, or protection from a compromised workstation. Delete retained evidence using the laboratory's approved process when it is no longer required.

## Hardware specifications and bill of materials

The complete instrument consists of an external ST NUCLEO-F401RE board, this passive carrier, insulated leads, an insulating support, and a Linux workstation. The development board supplies the STM32F401RET6 Cortex-M4 processor, 512 KiB flash, 96 KiB SRAM, voltage regulation, reset, decoupling, SWD programming, and ST-LINK USB virtual COM. Those parts are not missing from an alleged standalone MCU design: they are explicitly part of the purchased external module.

Firmware uses the reset-default nominal 16 MHz HSI clock, a nominal one-millisecond SysTick, and USART2 at 115200 baud with eight data bits, no parity, and one stop bit. No PLL, radio, antenna, FPGA, analog sensor, battery, charger, mass-storage card, or target power supply is used. The carrier is an 80 by 70 mm, two-copper-layer board with nominal 1.6 mm thickness and 0.3 mm tracks. It is connected by wires rather than mechanically aligned as a stacking shield. It has no mounting holes; provide an insulating support and strain relief.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, with ST-LINK |
| R1, R3, R5, R7 | 4 | 100 kΩ, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm through-hole header |
| J2 | 1 | 1x4 vertical 2.54 mm through-hole header |
| Harness and support | 1 set | Insulated leads and nonconductive support |

Resistor footprints use 7.62 mm lead spacing. Verify purchased part geometry and connector pin-one markings before assembly. J1 has ground on both pins: neither pin is a supply. Connect J1 to Nucleo ground. J2 pins one through four connect to Nucleo CN8 pins one through four, respectively A0/PA0, A1/PA1, A2/PA4, and A3/PB0. J3 through J6 expose BUSY_IN, CLASS_IN, VALID_IN, and INHIBIT_IN on pin one; each pin two is ground. Unsuffixed net names identify the MCU side of the series resistors.

Only verified zero-to-3.3 V push-pull diagnostics with common ground are supported. Each input has a series 1 kΩ resistor and a 100 kΩ pulldown on its MCU-side node. The resulting loading is approximately 101 kΩ to ground. This resistor arrangement does not establish 5 V tolerance, overvoltage protection, negative-voltage immunity, ESD qualification, or galvanic isolation. There are no dedicated clamps or reverse-polarity protection components. Unplugged VALID tends low and prevents acquisition; unplugged BUSY can conceal activity. Those are different failure modes and must not be conflated.

Power the Nucleo through the ST-LINK USB connector according to its board manual. Reserve a conservative 5 V, 500 mA supply allocation for planning; this is not a measured current draw or USB compliance result. Never connect target supply rails to the carrier. Disconnect target leads before removing observer power to avoid input-diode back-power. Check continuity, resistor values, ground potential, logic voltage, and the target's response to loading before the first trial. Review the specific board revision and power jumpers rather than relying on visual similarity to another Nucleo model.

## Architecture and block diagram

```text
Owned diagnostic adapter      Passive carrier          NUCLEO-F401RE
BUSY    ---------------------- 1k --+------------------- PA0 / CN8 A0
CLASS   ---------------------- 1k --+------------------- PA1 / CN8 A1
VALID   ---------------------- 1k --+------------------- PA4 / CN8 A2
INHIBIT ---------------------- 1k --+------------------- PB0 / CN8 A3
                                    | each node: 100k to GND
GND --------------------------------------------------- GND

GPIO snapshot -> tick-continuity check -> idle baseline / BUSY measurement
              -> class and validity guards -> opposite-class pair join
              -> bounded cohort aggregates -> LP1 / CRC encoder
              -> nonblocking USART2 PA2 TX -> ST-LINK USB virtual COM
              -> Linux Pairs / History / Integrity -> private JSON export
```

GPIOA and GPIOB are read consecutively, not latched simultaneously. BUSY, CLASS, and VALID share GPIOA; INHIBIT is on GPIOB. Cross-port changes can appear in adjacent samples. This is a slow diagnostic interface, not a high-speed timing analyzer. Narrow pulses or invalid intervals between samples can be missed. Use a calibrated logic analyzer to qualify the adapter and determine the practical minimum pulse and setup durations. The firmware has no debounce filter, so sampled glitches can reject a pair rather than being silently removed.

## Firmware contract and design decisions

A new observer begins without an active measurement. It needs a sampled idle BUSY level with VALID high and INHIBIT low before it will accept a rising BUSY. CLASS must already match the previous sampled level when BUSY rises. Hold the selected class throughout the operation and through the falling BUSY observation. Changing class on the same observation as either boundary rejects the candidate. An operation already high when the observer attaches is ignored until an idle baseline appears; there is no known starting edge from which to measure it.

At an accepted rising sample, age starts at zero. Each subsequent consecutive sample increments age. A falling BUSY completes the operation at that age, so a pulse high on one sampled tick and low on the next measures one sampled millisecond. Legal durations are one through one thousand ticks inclusive. Still-high age one thousand remains potentially valid; the next sample would exceed the duration bound and rejects it. The exact endpoint matters: a falling edge at age one thousand is accepted, while a falling edge one tick later is not.

The first valid operation becomes a pending half-pair. The next accepted operation must have the opposite class. Either A-then-B or B-then-A is supported; the comparison always maps values back to A and B. Repeated class at the next start abandons the entire candidate, including the stored first half. The firmware does not slide a window until a favorable pairing appears. A complete pair updates bounded aggregate totals and direction counts, then leaves no pending half. The operator should counterbalance pair order in the external workload to reduce order-related confounding.

The pending half has a ten-thousand-tick lifetime starting at its completion. This includes the second operation's duration. At age ten thousand the candidate is rejected before processing a completion on that same sample; therefore the second completion must occur earlier. VALID must remain asserted and INHIBIT clear throughout both operations and the interval between them. Either condition becoming unacceptable abandons the candidate. A rejected candidate increments one counter even when both a first half and an active second half are discarded. Idle invalidity without a candidate blocks readiness but does not count as a rejection.

Duplicate tick timestamps do not create new observations. A nonzero elapsed tick delta other than one increments the gap counter, abandons any candidate, and invalidates readiness. The gap-ending sample is not used as a new baseline. A subsequent valid idle observation is required. Previously completed pair totals and sticky findings survive gaps, but no duration is extrapolated through unknown time. Normal unsigned 32-bit tick wrap works; a halt spanning an entire timer cycle is not distinguishable from ordinary elapsed time.

Acquisition stops permanently for that boot after thirty-two accepted pairs. The cohort statistics then remain immutable. Current time and input-level telemetry continue, and serial loss remains reportable, but the firmware does not examine further trials or record further acquisition gaps. Resetting the observer clears all RAM state; there is no flash journal. Diagnostic counters saturate instead of wrapping. Cohort totals are naturally bounded by fixed pair count and duration, avoiding the need for floating-point arithmetic or unbounded storage on the MCU.

The main loop services sampling before serial output. The transmit driver copies a bounded frame, emits at most one ready byte per iteration, and abandons stalled output after one hundred milliseconds. A busy serial link cannot block measurement. Startup emits reset-cause information once, policy approximately once per second, and status approximately every hundred milliseconds when output is available. Policy repetition supports late attachment without impersonating a reboot. A failed watchdog initialization leaves the acquisition state waiting with a sticky fault rather than generating apparently valid trials.

## Companion application and wire interface

Run `python3 app/console.py` from this directory on Linux with Tk installed. The Pairs screen shows state, cohort progress, signed mean difference, directional pair counts, threshold count, maximum difference, rejected candidates, freshness, and named findings. History shows up to 256 accepted telemetry rows, not raw individual operations. Integrity shows source provenance, protocol compatibility, parser errors, reset flags, age, connection state, and any frozen-evidence reason. These screens share one model and receive real serial bytes rather than demo-generated success values.

Choose the exact serial terminal and select New live. A successful new connection clears previous displayed measurements. Disconnect preserves evidence and whether it came from live acquisition or replay. Replay accepts only regular files no larger than one MiB, opened nonblocking to avoid hanging on a FIFO or device. Replay explicitly checks the end of the stream: a nonempty partial tail freezes the recording as truncated. Export creates a new JSON file with mode 0600 and refuses to overwrite any existing path. Export first before starting another session if the old evidence must be retained.

Frames are ASCII `LP1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xffff, no reflection or final XOR. It covers bytes before the asterisk and is printed as four uppercase hexadecimal digits. B contains one reset-flags integer. P contains `1000,10000,32,5`, representing duration bound, pending-half lifetime, cohort size, and absolute-difference threshold. Status S contains fifteen unsigned integers in this order:

```text
time,state,levels,pairs,sum_a,sum_b,a_slower,b_slower,equal,large,
max_delta,rejected,gaps,tx_fail,faults
```

State values zero through three mean waiting baseline, ready, active measurement, and completed cohort. Level bits zero through three are BUSY, CLASS, VALID, and INHIBIT. Fault bits zero through seven mean class instability, invalid candidate, inhibited candidate, timeout, repeated class, acquisition gap, serial loss, and failed watchdog initialization. Faults remain sticky for the boot. Errors describe instrumentation and evidence integrity; a nonzero timing difference is not itself a fault bit or an automatic vulnerability verdict.

The app requires a compatible policy before accepting statuses. It checks integer bounds, pair conservation, duration totals, threshold consistency, state, and monotonic cumulative fields. A timestamp rollback, counter rollback, disappearing sticky flags, contradictory duplicate, or boot record after measurements freezes the session. Identical duplicate timestamps do not refresh freshness. The complete LP1 marker permits recovery after a truncated live frame. These checks improve consistency but do not authenticate the sender. A reset whose received values all look forward can remain undetected without a persistent boot identifier; repeated policy records do not solve that problem.

## Research workflows and interpretation

For a product-security assessment, select two public, nonsecret request classes whose response timing should be comparable under the product's design. Instrument the same start and end boundaries for both. Keep request length, background workload, temperature, storage state, and response path controlled. Counterbalance order between pairs. Perform an observer reset before the cohort, connect the app, verify policy and baseline, then run the external authorized workload. Export the completed result and note all rejected candidates and sticky findings, rather than selecting only a clean-looking mean.

For a red-team lab exercise, use an owned training fixture that deliberately contains a long and short response path. The purpose is to demonstrate why equal-looking response messages may still have different timings, not to discover real account identities. Run the instrument before and after a defensive change such as consistent work scheduling. A lower observed difference can motivate further examination, but does not demonstrate that a remote attacker cannot distinguish the classes using a faster instrument or more observations.

For penetration-test remediation verification, compare reproducible cohorts from the same diagnostic adapter and firmware revision. Store adapter source, target revision, scope traces, calibration notes, and workload schedule separately under the engagement's evidence policy. This observer does not retain per-pair rows suitable for a full statistical test, and telemetry History is not a substitute for them. If rigorous distributional analysis is required, use a higher-resolution measurement system with an independently designed experimental and statistical plan.

Selection bias deserves particular attention. Timeout, invalidity, inhibit, and class errors remove entire candidate pairs. That avoids using a surviving favorable half, but does not make the remaining cohort unbiased. One class could fail the adapter more often than the other. The rejection count has no class-specific denominator, so it cannot quantify that bias. Treat repeated rejections as a reason to fix the instrumentation and rerun, not as harmless noise. Adjacent pairing can reduce slow drift but cannot remove order effects, carryover effects, or confounding workloads.

## Build, tests, CAD checks, and bring-up gates

Build from this directory with `make -C firmware all test`. The embedded build uses arm-none-eabi GCC, newlib's nosys specifications, the included startup vectors, and the STM32F401RE linker script. Host tests use GCC with AddressSanitizer and UndefinedBehaviorSanitizer for algorithm, encoder, and scheduling tests; the mapped-MMIO serial-driver test uses UndefinedBehaviorSanitizer. It is explicitly a software test of a simulated register page, not a physical USART experiment. No downloaded SDK, generated vendor source tree, or network package install is needed by the firmware build.

Run application tests using `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'`. They exercise malformed records, impossible values, policy mismatch, duplicate and rollback behavior, bounded replay, truncation, private export, real C-encoder fixtures, and an actual Tk event loop with a Linux pseudo-terminal. The fixture represents synthetic stimuli passed through the production acquisition algorithm. It is not measured hardware evidence. Xvfb tests screen content and connection lifecycle, not operating-system USB enumeration or permissions on an actual board.

Regenerate hardware with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9's pcbnew module and standard installed footprints. Run `/usr/bin/python3 tools/check_hardware.py` for native ERC, DRC, unconnected-item checks, and exact reference/pin/net/value/footprint comparison between schematic, board, saved XML netlist, and connectivity manifest. The board has fourteen real components and nine nets. All target signal paths and grounds are routed. Passing passive-only ERC does not validate the external development board, target power sequencing, or application-specific electrical safety.

Before physical use, review the schematic and PCB in KiCad, verify every assembled connection, inspect orientation, check for shorts, and perform powered tests without a target attached. Confirm the actual clock and UART timing, watchdog reset behavior, brownout behavior, and input thresholds. Then qualify a sacrificial diagnostic fixture with a calibrated scope, checking setup, pulse width, and signal integrity. Only after those gates should the operator collect exploratory timing evidence. None of these physical steps is claimed complete by the software/CAD checks in VERIFICATION.md.

## Manufacturer references and provenance

ST's [UM1724 Nucleo-64 user manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf), table 16, specifies CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0 for NUCLEO-F401RE. Its USART section describes the default ST-LINK virtual COM bridge selection: SB13 and SB14 on, SB62 and SB63 off. Verify these against the actual module revision; do not alter solder bridges merely to match an assumption.

ST's [RM0368 reference manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) is the register and clock reference for the minimal board support. The [NUCLEO-F401RE product page](https://www.st.com/en/evaluation-tools/nucleo-f401re.html) identifies the supported external module. Standard KiCad footprint geometry is referenced by library identifier; upstream library licensing remains applicable. All original designs, firmware, tests, tooling, application code, and documentation in this device directory are authored by jayis1.
