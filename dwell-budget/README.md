# Dwell Budget — Rolling Privilege-Exposure Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol DB1. Firmware and application: GPL-2.0-only. Hardware generation/checking scripts: MIT. Carrier hardware design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, a working Linux desktop companion, and routed KiCad carrier files. Physical fabrication, programming, electrical qualification, and timing calibration have not been performed. This is a diagnostic observation instrument, not an access controller, a safety device, or a certificate of security compliance.

## Purpose and repository-local originality

Dwell Budget examines a security property that is easy to overlook when reviewing individual authorization events: how much privileged activity accumulates across a moving interval. A maintenance interface may correctly authenticate every request yet remain enabled almost continuously through repeated short requests. A privacy-sensitive sensor may satisfy a maximum single-capture duration while allowing repeated captures to consume nearly every moment. A diagnostic output can express those activities as a slow logic level. This instrument measures that level's sampled occupancy, tracks activity without consent or during an inhibit, and checks whether the target's own alarm indication responds to a rolling-budget excess.

The policy implemented here permits up to 2,000 active samples in the most recent 10,000 sampled milliseconds. The first sample beyond that limit raises a sticky excess finding. A separate alarm level is expected no later than twenty further sampled milliseconds after the observed excess starts, and must remain asserted while the rolling total remains excessive. The numbers are a fixed bench contract, not a universal security recommendation. They deliberately describe a slow, inspectable laboratory adapter rather than a high-speed processor trace interface.

This concept was selected after inventorying existing repository directories and README descriptions. Retry Ledger counts authentication failures and observes lockout; Lease Watch compares heartbeats with enable state; Receipt Witness correlates ordered audit indications; Reset Quarantine examines post-reset settling. None of those README scopes describes an exact moving-window occupancy ring with rolling eviction, cumulative denied exposure, and a target alarm-level contract. Dwell Budget reuses the repository's practical four-input passive carrier topology and basic STM32 support, but its measurement algorithm, policy, protocol, and evidence presentation are distinct. This is a claim of differentiation within this repository, not a patent search or a claim that rolling-window instruments have never existed elsewhere.

## Attack surface, threat model, and limits

The assessed surface is the lifetime of a privileged diagnostic state. Relevant implementation mistakes include resetting a per-operation timer on every request, using fixed calendar buckets where the policy actually requires a rolling window, accepting new activity while a revocation input is asserted, and clearing an alarm as soon as activity ends even though past activity still occupies the observation window. An authorized evaluator generates those conditions using a separate owned fixture. Dwell Budget does not generate requests, drive the assessed output, defeat authentication, or alter the target's configuration.

The target exposes four active-high, zero-to-3.3 V push-pull diagnostic outputs: ACTIVE, CONSENT, INHIBIT, and ALARM. ACTIVE must accurately represent the protected state under review, not merely the arrival of a request. CONSENT represents the adapter's current authorization judgment. INHIBIT denotes a condition that forbids activity regardless of consent. ALARM reports the target's rolling-budget alarm. The meaning and latency of each output must be reviewed with the target owner. If the target lies on these wires, the instrument can produce a perfectly consistent but meaningless record. It does not attest internal state or observe actual data access.

The workstation, instrument firmware, physical harness, and diagnostic adapter are trusted. Serial input and replay files are treated as untrusted bytes for parser purposes, but CRC is only accidental-corruption detection. There is no message authentication, signed capture, secure element, persistent session identifier, remote management service, or cryptographic boot chain. Physical access to ST-LINK permits firmware replacement. A compromised workstation can falsify exports. An observer reset loses the RAM ring and every lifetime counter; the application detects some resets, not all possible resets.

## Authorized use and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Authorization must cover physical attachment, diagnostic instrumentation, planned workloads, storage of timing evidence, and retention periods. Keep the instrument visible and labelled. A general red-team agreement does not automatically authorize attaching an unisolated board to production equipment. Do not connect this design to life-safety systems, vehicles in service, occupied access-control equipment, mains test points, unknown field wiring, or high-energy machinery.

Receive-only firmware does not make electrical attachment harmless. Shared grounds, input loading, reversed connectors, and input-diode back-power can disrupt a target. Begin with an isolated development fixture in the organizational sense: remove hazardous actuators and production dependencies. The carrier itself is not galvanically isolated. Stop immediately if attachment changes target behavior. Findings require corroboration with target source review and a calibrated measurement instrument. The author, jayis1, provides this design for authorized research and does not authorize covert collection, unauthorized access, or interference.

## Hardware specifications and bill of materials

The full instrument comprises an external ST NUCLEO-F401RE development board, the passive carrier, insulated labelled leads, and a Linux workstation. The STM32F401RET6 has a Cortex-M4 core, 512 KiB flash, and 96 KiB SRAM. Firmware uses the nominal reset-default 16 MHz HSI clock with no PLL, a one-millisecond SysTick, and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. The Nucleo's ST-LINK provides SWD programming and USB virtual serial. Regulation, MCU decoupling, reset circuitry, USB power handling, and programming hardware are supplied by that purchased module, not omitted from an allegedly standalone MCU board.

The carrier has an 80 by 70 mm outline, two copper layers, nominal 1.6 mm thickness, 0.3 mm traces, and fourteen through-hole components. It is cable-connected rather than a mechanically aligned stacking shield. There are no mounting holes; use an insulating support and strain relief. There are no radios, batteries, antennas, charge controllers, analog sensors, target power outputs, or target-facing transmit paths. The device's only electrical observations are the four diagnostic logic inputs.

| Reference | Quantity | Part requirement |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kΩ, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial DIN0207 series resistor |
| J1, J3–J6 | 5 | 1x2 vertical 2.54 mm through-hole header |
| J2 | 1 | 1x4 vertical 2.54 mm through-hole header |
| Harness/support | 1 set | Insulated leads and nonconductive carrier support |

Resistor footprints have 7.62 mm lead spacing. Check actual purchased component geometry and connector mating orientation against the footprints before ordering. No vendor stock or price guarantee is implied. J1 pins one and two both connect to ground: neither is a supply. J2 pins one through four connect to Nucleo CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0. J3 pin one is ACTIVE_IN, J4 CONSENT_IN, J5 INHIBIT_IN, and J6 ALARM_IN. Pin two of each target connector is ground. Unsuffixed signal nets are the MCU-side nodes after the series resistors.

Only zero-to-3.3 V push-pull logic with verified common ground is supported. Each target channel sees approximately 101 kΩ to ground through the series/pulldown pair. The 1 kΩ resistor does not establish overvoltage protection, 5 V tolerance, negative-voltage immunity, or ESD qualification. There are no dedicated clamps, isolation barriers, or reverse-polarity protection components. Internal GPIO pulls are disabled; external pulldowns make an unplugged input tend low, but an unplugged ACTIVE wire cannot be distinguished from legitimate inactivity.

Power the Nucleo through its documented ST-LINK USB connection and check its power jumpers against the board manual. Reserve a conservative 5 V, 500 mA supply allocation for planning; this is not a measured draw or USB compliance claim. Disconnect target leads before removing observer power to avoid back-power. Confirm ground potential, resistor loading, connector continuity, and signal voltage with suitable instruments before attaching all channels. Reference documents are ST UM1724 for the Nucleo-64 board and ST RM0368 for STM32F401 registers; verify the specific purchased board revision against those documents.

## Architecture and block diagram

```text
Owned target outputs           Passive carrier             NUCLEO-F401RE
ACTIVE  ----------------------- 1k --+---------------------- PA0 / A0
CONSENT ----------------------- 1k --+---------------------- PA1 / A1
INHIBIT ----------------------- 1k --+---------------------- PA4 / A2
ALARM   ----------------------- 1k --+---------------------- PB0 / A3
                                    | each node has 100k to GND
GND ------------------------------------------------------- GND

GPIO observations -> continuity check -> 10,000-slot rolling ring
                                      -> consent/inhibit classifier
                                      -> occupancy and alarm policy
                                      -> saturating lifetime ledger
                                      -> bounded DB1/CRC encoder
                                      -> USART2 PA2 TX -> ST-LINK USB
                                      -> Linux Budget / History / Integrity
```

GPIOA and GPIOB are read consecutively, not latched simultaneously. Therefore a cross-port transition near a sample can appear in either adjacent sample. The firmware does not infer nanosecond edge ordering or claim capture of every short pulse. An external logic analyzer is required to establish adapter timing, clock error, and signal quality. The carrier's simplicity makes its electrical boundary inspectable, but it cannot turn software-generated diagnostics into independent physical truth.

## Firmware semantics and design decisions

Every distinct one-millisecond tick contributes one instantaneous sample. A high ACTIVE sample adds one sampled millisecond to occupancy and lifetime active exposure. If CONSENT is low or INHIBIT is high on that observation, it also adds one denied sample. Both causes set separate sticky flags, but a sample violating both conditions counts only once in denied exposure. There is deliberately no debounce: a qualified debounce filter would silently erase short activity that matters to occupancy. Conversely this means noise and narrow sampled glitches can produce findings. The measurement is a sampled estimate, not integration of continuous analog pulse width.

The ring stores two bits of classification in each byte: active and denied. Before inserting a new observation, the oldest slot's contributions are subtracted. The cursor then advances modulo ten thousand. This gives constant-time insertion and exact eviction for the discrete sample model without scanning the ring on each tick. The allocation is ten thousand bytes, comfortably within the selected MCU's SRAM budget. A future packed-bit implementation could reduce memory, but this version prefers transparent bounds and simple tests over unnecessary compression.

Window fill starts at zero. A complete ten-second view is available only after ten thousand consecutive samples. During warmup, displayed occupancy is a lower bound over the already observed portion; it must not be interpreted as proof that the unobserved past was idle. An excess during warmup is still meaningful because more than two thousand observed active samples already exceed the allowance regardless of missing earlier history. The user interface displays fill explicitly rather than synthesizing an all-idle past.

An occupancy of exactly two thousand is permitted. Two thousand and one starts an excess episode, sets the sticky excess flag, and initializes its age to zero. Twenty additional consecutive sampled ticks later, absent ALARM sets the missing-alarm flag. The target must maintain ALARM while occupancy exceeds the limit, including idle periods while old active slots remain inside the window. Ending ACTIVE does not immediately clear excess. A fresh rise above the limit after recovery increments the episode count. An acquisition gap splits observation episodes, so this number is not necessarily a count of distinct physical incidents.

Duplicate timestamps do not create samples. Any nonzero elapsed tick delta other than one marks a gap, increments gap and missed-tick counters, clears the prospective ring, and starts another incomplete window. The gap-ending observation itself is not counted. Historical totals, peak occupancy, and sticky findings remain intact. Unknown time is never filled with repeated values or assumed idle. Unsigned modular time differences permit normal 32-bit tick wrap. Lifetime counters saturate at their maximum rather than wrapping to zero; a stalled system spanning a full counter cycle cannot be diagnosed unambiguously.

The UART driver copies only a bounded frame, services at most one ready byte per loop, and abandons stalled output after one hundred milliseconds. Sampling never waits for serial transmission. The application resynchronizes on the complete DB1 marker if a new frame follows a truncated one. The main loop sends a boot record once, policy announcements about once per second, and status about once per hundred milliseconds. Policy repetition allows late attachment without pretending a repeated announcement is a reset. A failed watchdog initialization produces a sticky watchdog fault and no acquired window. Watchdog and oscillator timing are not calibrated in this release.

## Companion application and protocol

Run `python3 app/console.py` from this directory on Linux with Tk installed. The Budget screen shows current occupancy, incomplete-window state, denied exposure, lifetime totals, and named findings. History retains up to 256 accepted rows. Integrity shows provenance, compatibility, parser errors, reset flags, age, connection state, and any reason the evidence has been frozen. Enter the exact serial terminal and choose New live. The port is opened read-only and the application sends no device commands. Starting a new session replaces old measurements; export first if they must be retained.

Disconnect preserves evidence and its provenance. Replay accepts only an opened regular file at most one MiB, rejecting FIFOs and device files without a blocking read. Export creates a new JSON file with mode 0600 and refuses overwrites. Exports include author jayis1, row history, parser errors, policy compatibility, age, frozen reason, and connection state. Private permissions are not encryption, secure deletion, or cryptographic evidence custody. The application provides no cloud upload, automatic session archive, or network listener.

Frames use ASCII `DB1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC is CRC-16/CCITT-FALSE: polynomial 0x1021, initial value 0xffff, no reflection or final XOR, calculated over bytes preceding the asterisk and rendered as four uppercase hexadecimal digits. B contains one MCU reset-flags value. P contains `10000,2000,20`, meaning window, limit, and response interval. S contains fourteen unsigned 32-bit integers in this order: time, fill, levels, occupied, unauthorized, total_active, total_denied, excess_episodes, gaps, missed, faults, tx_fail, over_age, peak. Level bits zero through three correspond to ACTIVE, CONSENT, INHIBIT, and ALARM.

Fault bits zero through six mean excess, no consent, inhibited activity, missing alarm, acquisition gap, serial loss, and failed watchdog initialization. All remain sticky until observer reboot. A status arriving before a compatible policy is ignored. CRC-valid but internally impossible records are rejected. Repeated identical timestamps do not refresh freshness; contradictory duplicates freeze the session. Time rollback, lifetime-counter rollback, disappearing sticky flags, or a boot record after measurements also freeze evidence. Without a persistent boot identifier, a reset whose first received values happen to advance every monitored field can remain indistinguishable from continuing operation. The interface explicitly makes no stronger session-continuity promise.

## Reproduction, validation, and use cases

Install GNU Make, host GCC, Arm GNU Embedded GCC with newlib/nosys specifications, Python 3 with Tk, KiCad 9 libraries and Python bindings, and Xvfb for headless GUI tests. There are no pip, npm, or mobile-framework dependencies. Build with `make -C firmware all test`. Run model tests with `python3 tests/test_app.py` and GUI transport tests with `xvfb-run -a /usr/bin/python3 tests/test_gui.py`. Generate CAD using `/usr/bin/python3 tools/generate_hardware.py`; verify it using `/usr/bin/python3 tools/check_hardware.py`. Use the system Python carrying pcbnew rather than an unrelated virtual environment. Generated build products are ignored and should not be committed.

The C tests compare rolling totals against an independent synthetic ring model, check inclusive budget and alarm boundaries, exercise authorization combinations, timestamp wrap, gaps, saturation, CRC vectors, and short output buffers. The production main loop is exercised with substituted board functions to verify scheduling, acquisition under a busy UART, and watchdog failure. App tests consume the actual C encoder's synthetic stream. The Tk test opens a Linux PTY, receives that stream through the real event loop, visits all screens, checks stale state, handles hangup, exports retained evidence, and loads a replay. None of these tests constitutes physical USB, GPIO, or target validation.

An authorized red team can use the instrument on a development build to demonstrate whether repeated short maintenance windows evade a stated exposure policy. A security researcher can compare fixed-bucket and moving-window implementations without retaining request contents or credentials. A penetration tester can provide a reproducible timing finding whose export separates current occupancy from historical faults. A product team can examine privacy-indicator behavior after activity stops but its rolling allowance remains exhausted. In every case, the workload generator is separate and must stay within the owner's approved test plan.

Before fabrication, review the schematic, connector orientation, footprint sizes, and assembly clearance with the selected components. Before trusting measurements, program a sacrificial Nucleo, verify all target pins remain inputs, measure sample timing and serial baud, and compare known pulse trains against a calibrated analyzer. Check power sequencing and unplug behavior. A zero-violation ERC/DRC result demonstrates consistency with configured CAD rules, not electrical immunity, regulatory compliance, or correct external harness wiring. See VERIFICATION.md for executed checks and outstanding laboratory work.
