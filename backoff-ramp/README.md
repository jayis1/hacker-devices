# Backoff Ramp — Failure-Dependent Retry-Delay Evidence Bench

Author and creator: jayis1. Hardware revision A. Wire protocol BR1. Firmware, application, and tests: GPL-2.0-only. Hardware tools: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked firmware, an executable Linux desktop companion, and native routed KiCad carrier files. Software and CAD verification are recorded in VERIFICATION.md. Fabrication, MCU programming, electrical qualification, physical interoperability, and timing calibration have not been performed. This instrument observes a declared policy; it does not enforce authentication, implement rate limiting, or constitute a safety interlock.

## Purpose, overview, and repository-local originality

Backoff Ramp asks whether successive failed operations actually cause progressively longer delays before another attempt. An authentication service may describe an exponential-backoff policy while a peripheral accepts requests immediately, a recovery branch accidentally clears the exponent, or an implementation applies the first delay forever. Aggregate failure counts and a lockout flag do not reveal those differences. A separate, receive-only microcontroller observes deliberately exported diagnostic indications and retains both outcome accounting and early-retry findings.

The operator's target adapter exposes ATTEMPT, FAILURE, SUCCESS, and RESET. An accepted failure establishes a nominal ten-millisecond delay after the first failure, twenty after the second, then forty, eighty, one hundred sixty, three hundred twenty, and six hundred forty. Further failures remain capped at six hundred forty. A matching success clears escalation. The instrument compares the next qualified attempt against the outstanding delay, including exact-boundary behavior, and continues tracking an early attempt so that its eventual failure still increases the required delay.

This is a small laboratory contract, not a recommended production authentication policy. Real services may need seconds or minutes, per-account state, randomized jitter, distributed quotas, and protections against denial of service. This implementation has one serial transaction stream and one escalation grade. It is useful for accelerated integration tests in which the owner intentionally maps a policy into slow GPIO indications. It does not infer user identity or merge separate principals.

Existing repository directories and device-purpose statements were inspected before selecting the design. Retry Ledger counts authentication failures toward a lockout obligation; Burst Budget reconstructs replenishing admission credit; Pulse Envelope checks individual pulse widths and recovery intervals. Backoff Ramp instead derives the required interval from the sequence of completed failure outcomes and explicitly resets that sequence only on success or an observed reset baseline. A fixed pulse-recovery minimum cannot express that dependency. The reused passive carrier, STM32 support, CRC framing pattern, and desktop transport were authored by jayis1; the distinct contribution is this escalating-delay engine and its evidence contract, not a claim to have invented a novel resistor network. Originality is repository-relative, not a worldwide prior-art or patentability assertion.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written permission to assess. Authorization should cover diagnostic instrumentation, attachment points, power sequencing, test workloads, expected interruptions, and retention of evidence. Work on isolated development equipment with hazardous outputs disconnected. Never connect this unisolated prototype to deployed locks, vehicle controls, medical devices, mains-related signals, operational machinery, or unknown industrial wiring. Do not use it for covert collection or unsolicited interference.

The assessed boundary is between an operation's outcome and the next admission decision. Relevant defects include ignoring a backoff timer, decrementing it too quickly, failing to increase the exponent, retaining a stale success signal, accepting overlapping requests, and allowing a reset path to appear as an ordinary retry. An authorized red team can ask the owner to reproduce a suspected weakness through normal lab requests and compare the external timing record before and after a fix. The instrument itself sends no attempts, guesses no credentials, and cannot alter the target's decision.

The operator, host, development board, programmer, wiring, and observer image are trusted. Target indications and serial input are not authenticated. A malicious target can lie on all four pins; a malicious USB device can forge valid BR1 frames. CRC only detects accidental corruption. There is no signed log, cryptographic attestation, trusted wall clock, persistent boot identifier, authenticated update service, or secure storage. Protect physical access and workstation permissions. Independently correlate diagnostic outputs with the actual admission and outcome paths before drawing a security conclusion.

No passwords, usernames, request payloads, identifiers, keys, or network traffic are captured. Counters and timing can still reveal operational activity. Export only what the engagement requires, use private directories, redact customer-identifying filenames, and agree on retention. Removing a JSON file is not secure erasure. A correct-looking trace proves only consistency of the observed indications under the documented sampling model; it does not prove that production authentication is secure. The author, jayis1, does not authorize misuse or warrant electrical safety or regulatory compliance.

## Hardware specifications and components

The compute module is an external ST NUCLEO-F401RE development board with STM32F401RET6, Cortex-M4, 512 KiB flash, and 96 KiB RAM. Firmware uses the reset-default nominal sixteen-megahertz HSI clock, no PLL, and a one-millisecond SysTick. USART2 TX on PA2 feeds the board's ST-LINK virtual COM bridge at 115200 baud, eight data bits, no parity, and one stop bit. Receive is disabled. The target-facing pins remain inputs without internal pulls.

The carrier is an eighty-by-seventy-millimeter, two-layer, 1.6 mm FR-4 board. It has fourteen through-hole parts, nine nets, 0.3 mm tracks, a routed ground transition, and a closed edge outline. It is a harness-connected carrier, not a pin-compatible stacking shield. The external Nucleo supplies MCU decoupling, regulation, reset, SWD, programming, and USB connectivity. Those circuits are not represented as imaginary components on the carrier. There is no radio, FPGA, battery, charger, isolated supply, storage card, sensor, or onboard display. The workstation supplies the application screens.

| Reference | Quantity | Specification |
|---|---:|---|
| External module | 1 | ST NUCLEO-F401RE, verify actual MB1136 revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W axial DIN0207 series resistor |
| J1, J3–J6 | 5 | Straight 1x02, 2.54 mm through-hole header |
| J2 | 1 | Straight 1x04, 2.54 mm through-hole header |
| External harness | 1 set | Short insulated leads and strain relief |

The resistor footprints use 7.62 mm lead spacing. Confirm purchased component dimensions against the footprints. Power the Nucleo through its intended ST-LINK USB connector with its documented jumper configuration. Reserve a conservative five-volt, 500 mA USB-source budget; this is an allocation, not measured consumption or USB certification. The passive carrier has no supply rail and cannot intentionally power the target. No enclosure or mounting holes are provided; insulate and restrain the assembly before use.

| Input bit | Signal | Target connector | Series / pulldown | J2 pin | Nucleo pin |
|---:|---|---|---|---:|---|
| 0 | ATTEMPT | J3.1 | R2 / R1 | 1 | A0 / PA0 |
| 1 | FAILURE | J4.1 | R4 / R3 | 2 | A1 / PA1 |
| 2 | SUCCESS | J5.1 | R6 / R5 | 3 | A2 / PA4 |
| 3 | RESET | J6.1 | R8 / R7 | 4 | A3 / PB0 |

Pin two of every target header is ground. Both J1 pins are ground, never supply. Connect J1 to Nucleo ground and the target's approved diagnostic reference. Only known common-ground zero-to-3.3 V push-pull signals are supported. Each input has a nominal 101 kOhm path to ground through the resistor pair. The series resistor is not a voltage translator or isolation barrier. There are no dedicated ESD clamps, overvoltage disconnects, reverse-polarity protection, or industrial transient ratings.

Measure voltage and ground offset before attachment. Power the observer before enabling target outputs; disconnect target signals before removing observer power to avoid back-power through input structures. Keep leads short and away from noisy wiring. A disconnected lead tends to read low, so broken-wire detection is not available. A missing RESET wire can prevent correlation, while a missing event wire can conceal behavior. Verify wiring with an independently observed controlled sequence, not just a quiet application screen.

## Architecture and block diagram

```text
Authorized diagnostic fixture       Passive carrier                 External module
ATTEMPT -> J3.1 ----- R2 1k ----+---- J2.1 -------------------------> PA0 / A0
FAILURE -> J4.1 ----- R4 1k ----+---- J2.2 -------------------------> PA1 / A1
SUCCESS -> J5.1 ----- R6 1k ----+---- J2.3 -------------------------> PA4 / A2
RESET   -> J6.1 ----- R8 1k ----+---- J2.4 -------------------------> PB0 / A3
                               | each MCU-side node
                               R1/R3/R5/R7 100k
                               |
Fixture reference ------------+---- J1 ---------------------------> Nucleo GND

GPIO reads -> independent qualification -> backoff engine -> validated BR1 snapshots
SysTick -------- nominal milliseconds --------^                   |
                                                               USART2 TX
                                                                  |
                                                    ST-LINK USB virtual COM
                                                                  |
                                                Linux desktop, replay, private JSON
```

No software path drives the diagnostic fixture. The MCU-to-host serial transmitter is the only application data output. The desktop opens the serial device read-only and does not implement control commands. Firmware updates require deliberate use of the external ST-LINK programming interface. There is no network listener, cloud upload, radio service, remote discovery, or default password.

## Measurement contract and firmware decisions

Each channel qualifies independently after a changed raw value is followed by three further consecutive equal samples. Thus four samples establish a new level. A raw glitch that fails qualification does not change the accepted level. Firmware does not claim continuous electrical stability between samples. PA0, PA1, and PA4 are read together, then PB0 is read separately; the acquisition is not atomic across ports. Events are rising transitions of qualified levels, not every sample spent high.

The engine begins UNKNOWN. It requires an observation with RESET high and all event inputs low, followed by RESET low with all event inputs still low, before entering READY. RESET dominates simultaneous events. Activity while reset is high and activity while UNKNOWN are outside operation coverage and are not included in attempts. A held reset counts as one observed reset segment; a new segment after an acquisition gap may be counted separately even if the physical line never changed. Quiet alone after a gap or malformed transaction does not recover correlation.

READY means that correlation is established and no result is pending. It does not mean retry permission: inspect `remaining`. A qualified ATTEMPT edge increments attempts and enters PENDING. If delay remains after accounting for the current tick, the attempt also increments early and sets the sticky early-retry bit. The attempt remains pending regardless of this finding. This choice preserves the outcome sequence instead of allowing a timing violation to clear the escalation history.

PENDING accepts exactly one later FAILURE or SUCCESS edge. FAILURE increments the capped grade, assigns the corresponding delay, and returns READY. SUCCESS clears grade and delay and returns READY. Results must arrive within one thousand nominal milliseconds of the accepted attempt. A result at exactly the deadline is accepted; a result on the following tick is too late. Timeout takes precedence over any same-tick operation and ends correlation. The timeout count is distinct from malformed-event count.

A new attempt while one is pending, an outcome without a pending attempt, multiple rising event inputs in one qualified observation, or a newly rising event overlapping a held other event is malformed. The engine records one malformed observation and returns UNKNOWN. If a new attempt collided with an existing pending attempt, both are abandoned, preserving conservation. Falling ATTEMPT and rising result in the same observation is allowed when no event levels overlap. A held result must fall before another event rises; otherwise it cannot be safely distinguished from contradictory signaling under this interface.

At each covered sample, remaining delay decreases before the new event is evaluated. Therefore an attempt qualifying exactly at expiration is not early. Delay is assigned on the failure observation and does not decrease until subsequent covered samples. The required sequence is 10, 20, 40, 80, 160, 320, 640 nominal milliseconds, with saturation at grade seven. The delay continues decreasing while an early attempt awaits its outcome. A later failure replaces it with the newly escalated full delay. Success resets it only when correlated with a pending attempt.

Every accepted attempt belongs to exactly one of failures, successes, abandoned, or the currently pending operation. This identity is checked before encoding and independently in the application. Coverage counts qualified correlated observations, not physical elapsed time or the number of successful trials. Events outside coverage cannot be inferred from counters. Early does not imply a failed outcome, and a successful outcome does not erase a prior early finding.

A skipped millisecond invalidates qualification and correlation, records a gap and missed ticks, abandons any pending attempt, and requires a fresh reset baseline. Repeated calls in the same millisecond are ignored. Ordinary unsigned timestamp wrap is supported. Cumulative counters never wrap silently: updates are performed on a temporary ledger and committed as a unit. Exhaustion rolls back the speculative sample, abandons any previously pending attempt, and enters a terminal halted state. Historical counters and fault bits remain. Only timestamp and transmission-failure diagnostics may advance afterward.

The independent watchdog is enabled with bounded initialization waits. If startup fails, firmware emits a halted ledger rather than claiming valid measurements. UART work is nonblocking and sends at most one byte per service call. A queued frame that stalls for one hundred milliseconds is abandoned and counted. The next complete protocol marker lets the companion resynchronize after a partial frame. Firmware has no dynamic allocation or target command parser. Startup initializes data and BSS, supplies the Cortex-M vector table, and links against the actual flash and RAM map rather than a desktop-only simulation.

## Application interface and evidence protocol

The companion is a dependency-free Python standard-library Tk desktop application for Linux, an appropriate interface for a local serial bench. Install the distribution's Python Tk package. Start it with `python3 backoff-ramp/app/console.py`. Select the intended serial path and press New live. Disconnect before opening a second port. A failed replacement connection leaves retained evidence available. Opening configures 115200 8N1 and flushes queued input; closing restores original terminal attributes where the device still exists.

The Ledger screen shows numeric fields, decoded findings, state, and required delay. History retains the latest 256 accepted snapshots. Integrity shows source, live/replay provenance, connection status, policy compatibility, parser errors, boot flags, freeze reason, and age. Replay reads only regular files up to two MiB; FIFOs and device files are rejected without blocking. Replay is never labeled live. Export JSON creates a new file with private permissions and refuses to overwrite an existing file. Reports include author jayis1, provenance, diagnostics, current snapshot, and bounded history even when no successful operation occurred.

Frames are ASCII, beginning with `BR1,`, followed by kind and unsigned decimal fields, then an asterisk, four uppercase hexadecimal CRC digits, and LF. CRC16/CCITT-FALSE covers everything before the asterisk, with initial FFFF and polynomial 1021. B carries raw MCU reset flags once per boot. P repeats the policy values `3,10,7,1000` on startup and approximately once per second; it does not reset host evidence. S carries the following fields in order:

```text
now levels valid halted faults gaps missed state grade remaining age
attempts failures successes abandoned early malformed timeouts resets coverage tx_fail
```

All wire integers are unsigned 32-bit. States are zero UNKNOWN, one READY, two PENDING. Fault bits are one early retry, two malformed observation, four acquisition gap, eight counter exhaustion, sixteen unavailable watchdog, and thirty-two response timeout. `levels` and `valid` are four-bit masks in connector order. `grade` is zero through seven; `remaining` and `age` are nominal sampled milliseconds. Status transmission is attempted every hundred milliseconds; it is a cumulative summary, not a lossless event log.

The parser rejects invalid framing, checksums, ranges, conservation, and impossible relationships. Across snapshots it rejects counter rollback, disappearing sticky flags, contradictory duplicate timestamps, excessive growth, and inconsistent backoff transitions. These are necessary plausibility checks, not a complete proof that a trace was realizable. Live status requires compatible policy, two advancing snapshots, an open connection, and recent progress. A reception pause longer than one second freezes evidence; cumulative host/device drift is bounded with a capped tolerance so slow backlog cannot remain indefinitely live. Reconnect explicitly to discard possible backlog.

A later boot record freezes rather than silently replacing evidence. A lost boot record may still be detected through timestamp or counter regression, but a reset whose new counters and time exceed the retained values is indistinguishable without persistent identity. Disconnect preserves origin and evidence but removes live status. A malformed replay tail increments parser errors; inspect Integrity rather than treating a loaded file as automatically clean.

## Reproducible build, verification, and bench workflow

Use GNU Arm Embedded tools with newlib, host GCC, Python with Tk, Xvfb for headless UI tests, and KiCad 9 with Python pcbnew. From the repository root, run:

```sh
make -C backoff-ramp/firmware clean all test
xvfb-run -a /usr/bin/python3 -m unittest discover -s backoff-ramp/tests -p 'test_app.py' -v
/usr/bin/python3 backoff-ramp/tools/check_hardware.py
python3 backoff-ramp/tools/check_release.py
```

The target build emits ELF, BIN, and a linker map under ignored `firmware/build/`. Host tests exercise the production engine, encoder, scheduler, and UART logic. The C fixture is explicitly synthetic: it goes through the real engine and wire encoder, then feeds the desktop tests. UI tests use a real Tk event loop and Linux pseudo-terminal, not physical USB hardware. The hardware checker exports a fresh netlist, runs ERC and DRC, and compares every schematic and board pad against the connectivity manifest, including component values and footprint identities.

To regenerate the authored carrier sources after a deliberate hardware change, run `/usr/bin/python3 backoff-ramp/tools/generate_hardware.py`, then rerun checks and review the resulting diff. The generated native schematic has actual symbols, labels, wires, and electrical nets; the PCB has matching footprints and routed copper. A clean rule check does not establish safe voltage limits, mechanical fit, manufacturability, or immunity to interference. Review a rendered schematic and physical connector orientation before assembly.

For initial bring-up, inspect soldering unpowered, check ground continuity, and verify there are no unexpected shorts. Program the target image at the linked flash origin using the Nucleo's documented ST-LINK workflow. Verify the serial policy record before connecting the target. With an approved signal source, establish the reset baseline, generate one attempt and failure, then compare an intentionally early retry with a clearly delayed retry. Verify escalation across all grades and success reset. Capture waveforms with an independent calibrated instrument; use generous margins until qualification latency and oscillator error are characterized.

A useful regression campaign compares an implementation that always waits ten milliseconds against one that correctly escalates. Another checks whether a stale success indication improperly overlaps the next attempt. A third interrupts the observer's acquisition and confirms that the app reports uncertainty instead of continuing an assumed countdown. These are controlled defensive tests, not automated attacks. Record fixture version, adapter binding, physical measurements, software revision, and any deviations from the declared policy alongside exported evidence.

## Authoritative component references and remaining limits

Consult ST UM1724 for the Nucleo-64 board, Arduino connector mapping, ST-LINK virtual COM solder bridges, and power-selection details; ST RM0368 for STM32F401 RCC, GPIO, USART, and watchdog registers; and the STM32F401xD/xE datasheet for electrical characteristics. Manufacturer entry points are https://www.st.com/en/evaluation-tools/nucleo-f401re.html and https://www.st.com/en/microcontrollers-microprocessors/stm32f401re.html. KiCad documentation is available at https://docs.kicad.org/ . Check the actual board revision and latest errata before programming hardware.

This release does not establish oscillator calibration, silicon behavior under brownout, EMI immunity, ESD tolerance, reliable hot plug, or trustworthy target instrumentation. It deliberately observes only slow exported signals and can miss pulses shorter than its qualification window. It cannot distinguish every reset or disconnected wire, identify principals, prevent early requests, or certify rate-limiting security. Those limits are part of the instrument's contract, not issues hidden by passing software tests.
