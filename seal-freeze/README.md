# Seal Freeze — Reset-Bounded Configuration Immutability Witness

Author and creator: jayis1. Hardware revision A. Protocol SF1. Firmware, application, and tests: GPL-2.0-only. Hardware utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. This directory contains target-linked C firmware, a Linux desktop companion, a populated native KiCad schematic, and a routed passive carrier PCB. Software and CAD verification are recorded in VERIFICATION.md. Fabrication, MCU programming, physical interoperability, electrical qualification, and timing calibration have not been performed. This instrument observes a declared diagnostic contract; it is not a configuration lock, security enforcement controller, or safety interlock.

## Purpose and overview

Seal Freeze asks a narrowly defined security question: after an embedded controller declares its security configuration sealed, does that configuration remain the same until an independently observed reset? Some devices allow a boot-time choice of maintenance mode, debug policy, storage ownership, or protection profile, then claim to lock that choice. A status bit labeled locked is not enough evidence. A faulty service path may alter the underlying configuration without changing that bit, or temporarily withdraw the lock while an interface continues to report a normal state. A separate observer can retain the original configuration and expose both failures.

The device receives four deliberately slow diagnostic outputs from an owner-controlled test build: two configuration bits, a seal indication, and a reset indication. After observing a reset baseline and an open configuration interval, it captures the two-bit value at a properly ordered seal transition. It then compares every subsequent qualified configuration against the captured value. Seal withdrawal is recorded separately and does not release the original obligation. Returning the configuration to its original value stops the current mismatch segment but does not erase historical findings. Only a reset or loss of acquisition continuity ends the current contract.

The two-bit value must be an injective representation of four agreed test configurations, not a hash, truncated register, or low bits of a larger setting. For example, an isolated appliance development fixture might expose four distinct protection profiles. The owner documents the mapping and its connection to the actual effective register. A clean trace says only that these exported indications followed the stated contract. It does not prove that unobserved registers stayed unchanged, that the implementation resists hostile firmware, or that the selected profile is secure in the first place.

## Repository-local originality

Existing device directories and README titles were inventoried after pulling the configured main branch. The closest contracts were read before selecting this design. Sticky Trip reconstructs a one-bit alarm obligation with authorized clear semantics; it does not capture configuration at a lock boundary. Epoch Witness checks monotonic generation announcements and execution ordering, allowing later higher generations. Toggle Mailbox checks payload stability during an outstanding two-phase transfer, ending its obligation at acknowledgment. Reset Quarantine checks readiness settling before access. Seal Freeze instead requires reset provenance, captures an arbitrary two-bit value once per reset interval, and retains that exact value even when the seal itself is withdrawn.

The passive carrier topology, STM32 startup, register definitions, bounded serial transmitter, CRC framing pattern, and desktop transport infrastructure reuse jayis1-authored work from Sticky Trip. Their reuse is deliberate and disclosed. The distinct contribution is the reset-bounded immutability policy, its validation invariants, evidence presentation, and regression procedure. This is a claim of a new contribution within the inspected repository, not a worldwide prior-art search or a claim of patent novelty. No other repository, shared systems tree, or unified-system command is involved.

## Attack surface, threat model, and authorized use

Use only on equipment you own or have explicit written authorization to assess. Agree on the test firmware, diagnostic points, allowable reset operations, evidence retention, and stop conditions before wiring. Work on a low-energy development bench with hazardous outputs disconnected. Never connect this unisolated prototype to installed access controls, life-safety equipment, medical systems, moving vehicles, mains circuits, or operational industrial machinery. The author, jayis1, does not authorize unauthorized access, covert monitoring, or interference with third-party equipment.

The investigated attack surface is a product's transition from mutable boot configuration to supposedly immutable runtime policy. An authorized penetration tester can exercise approved service, recovery, and maintenance workflows using the target's ordinary interface while Seal Freeze watches the diagnostic outcomes. Relevant defects include a missed write-protect operation, a later configuration write, an erroneous unlock branch, or a reset indication that does not reflect real lifecycle transitions. The instrument does not generate these stimuli, inject bus transactions, recover credentials, install persistence, or change target memory.

A malicious target can lie on all four lines. Prefer effective-register readback or hardware-derived indications over copies of intended software variables, and corroborate findings with source inspection and independently captured waveforms. Even an apparently hardware-derived diagnostic needs review: a mux can select stale feedback, and a GPIO adapter can introduce delay. The observer cannot determine whether RESET represents the actual reset domain governing the protected register. That binding is a prerequisite of the experiment, not a result inferred by this firmware.

The operator, workstation, observer firmware, development board, and wiring are trusted. Serial CRC detects accidental corruption, not forgery. There is no authenticated device identity, secure boot provisioning, signed evidence, persistent boot identifier, trusted wall clock, flash evidence journal, or secure update service. A substituted serial source can manufacture plausible records. Physically control SWD access and restrict serial-device permissions. Do not run the application with elevated privileges merely for convenience. Diagnostic exports may reveal internal test schedules even though they contain no payload bytes or credentials.

## Hardware specifications and bill of materials

The active assembly is an external ST NUCLEO-F401RE, MB1136 family, using STM32F401RET6 in LQFP64. Firmware targets Cortex-M4 Thumb with software floating-point ABI, 512 KiB flash, and 96 KiB SRAM. It uses the reset-default nominal 16 MHz HSI oscillator rather than a PLL. SysTick provides the nominal one-millisecond sampling clock. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit, through the development board's ST-LINK USB virtual COM bridge. The receiver remains disabled.

The custom PCB is a passive harness carrier, not a standalone MCU board or pin-compatible Nucleo shield. It is 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4, with 0.3 mm tracks and a closed rectangular outline. It contains fourteen through-hole components and nine named nets. MCU regulation, decoupling, reset, clock infrastructure, USB support, and programming circuitry reside on the separately purchased development board. There is no radio, FPGA, battery, charger, active sensor, storage device, or display on the carrier.

| References | Quantity | Specification and purchasing example |
| --- | ---: | --- |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W axial DIN0207, 7.62 mm pitch; Yageo MFR-25 family or dimensionally equivalent |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W, same package and pitch |
| J1, J3–J6 | 5 | 1x02 straight 2.54 mm through-hole header; Samtec TSW-102-07-G-S or equivalent |
| J2 | 1 | 1x04 straight 2.54 mm through-hole header; Samtec TSW-104-07-G-S or equivalent |
| External | 1 | ST NUCLEO-F401RE development board |
| External | As needed | Insulated harnesses, USB cable, labeled nonconductive bench tray |

These are purchasing candidates, not verified live stock or price claims. Verify resistor body dimensions, lead pitch, header plating, mating connectors, and purchased module revision before ordering. There are no mounting holes in revision A. Do not improvise conductive fasteners through the board; use a nonconductive tray with strain relief and protection against underside contact. A production mechanical enclosure requires a separate fit and safety review.

### Wiring, power, and electrical limits

| Bit | Signal | Target connector | MCU harness | Nucleo pin |
| --- | --- | --- | --- | --- |
| 0 | CFG0, least significant bit | J3.1 | J2.1 | CN8 A0 / PA0 |
| 1 | CFG1 | J4.1 | J2.2 | CN8 A1 / PA1 |
| 2 | SEAL, active high | J5.1 | J2.3 | CN8 A2 / PA4 |
| 3 | RESET, active high | J6.1 | J2.4 | CN8 A3 / PB0 |
| none | Common ground | J3–J6.2 | J1, both pins | Nucleo GND |

J1 is ground only: neither pin is a supply connection. J2 has no ground pin. Each target input passes through 1 kOhm to a GPIO node with a 100 kOhm pull-down. Names ending `_IN` identify the target side; plain names identify the MCU side. Only nominal 0–3.3 V common-ground push-pull diagnostic outputs are supported. Do not connect five-volt signals, 1.8 V logic, negative voltages, differential pairs, field wiring, coils, or contact excitation sources. No level translator, galvanic isolation, dedicated ESD clamp, surge protector, or reverse-current blocker is included.

Reserve a conservative 5 V, 500 mA USB source allowance for the development-board assembly, subject to actual USB limits and the board's power configuration. Consumption has not been measured. A high input sees a nominal 101 kOhm resistor path to ground; assess tolerances, leakage, cable capacitance, and target drive impedance rather than treating that nominal value as a calibrated specification. Power the observer before enabling target outputs; disable those outputs before removing observer power. Do not join hazardous or independently isolated grounds. Series resistance does not make overvoltage or hot-plugging safe.

The manufacturer references are ST UM1724 for MB1136 connectors, power arrangements, solder bridges and ST-LINK virtual COM; RM0368 for STM32F401 registers; and the STM32F401xD/xE datasheet for electrical limits. Check SB13/SB14 and SB62/SB63 against the purchased board before bring-up. The firmware expects reset-entry clock conditions, not entry from an arbitrary bootloader that has changed clock divisors. A disconnected wire tends low and is not independently detected. Losing SEAL resembles withdrawal; losing RESET can conceal the end of an obligation; losing a configuration lead can change the interpreted profile.

## Architecture and block diagram

```text
Owner-controlled diagnostic adapter       Passive carrier       Nucleo inputs
CFG0 ---- J3.1 --------------------------- 1k --+-- J2.1 -------- PA0
CFG1 ---- J4.1 --------------------------- 1k --+-- J2.2 -------- PA1
SEAL ---- J5.1 --------------------------- 1k --+-- J2.3 -------- PA4
RESET --- J6.1 --------------------------- 1k --+-- J2.4 -------- PB0
                                              | each node
                                             100k
                                              |
GND ----- J3..J6.2 -------------------------- GND -- J1 -------- GND

GPIO reads -> independent qualification -> observed reset baseline
 -> open configuration interval -> seal-edge setup check -> captured value
 -> immutable comparison + seal-withdrawal ledger -> transactional counters
 -> SF1 CRC framing -> bounded UART TX -> ST-LINK -> Linux desktop
 -> Ledger / History / Integrity -> explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not simultaneously latched. Port-read skew, oscillator tolerance, interrupt latency, and qualification affect apparent ordering. Use deliberately paced diagnostic plateaus of at least twenty milliseconds initially, and confirm actual waveforms independently. This is not a high-speed register bus analyzer, metastability detector, nanosecond glitch recorder, or proof of uninterrupted electrical stability. Short pulses can be missed between samples or rejected during qualification.

## Firmware details and exact policy

Each input independently qualifies after an initial candidate sample followed by three matching nominal one-millisecond intervals. An already qualified level is retained until a replacement candidate qualifies. Initial policy accounting waits for all four valid bits. Initial chatter can therefore postpone coverage, but later chatter on one input does not prevent another channel from updating. Qualification is a declared interpretation of samples, not a reconstruction of everything between them.

State zero is UNKNOWN. Startup does not infer a baseline from quiet wires or an already asserted seal. A qualified RESET assertion always ends an existing obligation and has precedence over configuration changes and sealing. Each newly observed reset segment increments resets, including an initially high reset once all channels become valid. While RESET is high, the observer must see SEAL low to establish a usable baseline. RESET release with SEAL still low enters OPEN. Release with SEAL high after that baseline increments rejected and leaves the contract unknown. A reset that never shows low SEAL cannot establish OPEN.

State one is OPEN. Configuration may change freely. A fresh qualified rising SEAL captures the value only when the previous complete qualified observation had exactly the same configuration. Simultaneous qualification of a changed configuration and SEAL is rejected; the observer returns to UNKNOWN and requires another reset baseline. This conservative setup rule does not prove raw analog setup time, but it avoids guessing which value a simultaneous event locked. Initially high seals, and seals occurring without an eligible reset interval, do not establish a contract.

State two is SEALED. The captured value stays immutable until reset, acquisition gap, or terminal halt. Every SEALED observation increments coverage. A differing configuration increments mismatch totals and a low SEAL increments withdrawal totals. Both can occur together, so their totals must not be added as elapsed time. Each direction has an observed-segment count, current streak, and historical maximum. Returning to the original value or raising SEAL ends the relevant current streak, not its retained history. Raising SEAL again never recaptures a changed configuration during the same obligation.

Duplicate timestamps are ignored. A nonzero timestamp delta other than one records a gap and missed nominal ticks, discards qualification and current contract provenance, and retains cumulative findings. A new reset baseline is required; silence or a still-high seal is not recovery. An initially high reset after requalification starts a newly observed reset segment, which may correspond to the same physical reset as before a gap. Episode and reset counts describe observations, not necessarily distinct physical incidents. Ordinary unsigned timer wrap is supported; an entirely unseen full timer cycle is not identifiable.

Sample updates are transactional. If any addition would overflow, none of that sample's tentative counter updates is committed. The previous cumulative evidence is frozen, current state becomes unknown, and exhaustion produces terminal halt. Watchdog startup failure also halts acquisition. Transmission failures use a separate saturating counter. Timestamps and that diagnostic may advance after halt, but policy evidence cannot resume without observer reset. RAM evidence is lost on observer reset, so export before resetting whenever possible.

The firmware is C11 with no heap, RTOS, filesystem, incoming command parser, or persistent configuration. Startup supplies Cortex-M vectors, data initialization and BSS clearing; the linker supplies the actual flash/RAM map and stack allowance. UART service submits complete bounded frames, sends at most one ready byte per iteration, and abandons a stalled transmission after one hundred milliseconds. The watchdog startup waits are bounded. Physical watchdog timing and actual serial throughput remain bring-up measurements, not results of the host tests.

## Companion software and protocol

Run `python3 app/console.py` on Linux with Tk installed. Enter the explicit device path and select New live. Ledger displays all fields, the UNKNOWN/OPEN/SEALED state, captured value, and named findings. History retains up to 256 accepted snapshots. Integrity displays source, live or replay origin, policy compatibility, reset information, parser errors, connection state, evidence age, and any freeze reason. Disconnect retains evidence but removes live status. Reconnection starts a distinct session; a failed replacement connection does not erase the old report.

Frames are ASCII `SF1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE covers all bytes before the asterisk, polynomial 0x1021, initial 0xffff, no reflection or final XOR, encoded as four uppercase hexadecimal digits. B carries observer reset flags and is emitted once. P carries `3,1,1`: qualification intervals, nominal sample interval, and revision-one reset/setup policy. Policy repeats approximately once per second for late attachment; it never masquerades as a new boot.

S fields are now, levels, valid, halted, faults, gaps, missed, state, captured, seals, resets, rejected, coverage, mismatch, unsealed, t0, t1, e0, e1, c0, c1, x0, x1, tx_fail. All are unsigned 32-bit integers. The t/e/c/x fields mean total observations, segments, current streak, and maximum streak; index zero is configuration mismatch and one is seal withdrawal. Fault bits zero through five mean mismatch, withdrawal, acquisition gap, exhaustion, watchdog unavailable, and rejected sealing. Findings are sticky until observer reset. Captured is zero outside SEALED and must not be interpreted as a real configuration in that case.

Status is offered approximately every hundred milliseconds. It is a cumulative snapshot, not an event-complete recording. The app validates CRC, bounds line storage, checks numeric and cross-field constraints, and resynchronizes on the complete SF1 marker. Counter rollback, disappearing sticky faults, contradictory duplicate timestamps, impossible growth, changing halted evidence, or a boot after accepted data freeze the retained session. A captured value cannot change within a continuously observed seal count. These are necessary consistency conditions, not cryptographic authentication or a complete proof that every accepted snapshot sequence has a physical realization.

LIVE requires a connected source, compatible policy, two advancing timestamps, and recent progress. Duplicate timestamps do not renew freshness. A reception pause exceeding one second requires reconnect. Cumulative host/device progress divergence is bounded by half a second plus five percent of elapsed host time, capped at one second total allowance. This conservative rule can reject long sessions with honest oscillator drift. Fixed initial buffering and certain resets without a received boot frame remain indistinguishable. LIVE does not mean SEALED, continuous acquisition, authentic source, or secure target.

Replay accepts only a regular file of at most two MiB, checked on an opened nonblocking descriptor before reading. FIFOs and devices are rejected. A nonempty truncated tail is counted as a parser error; an empty or incompatible recording is rejected. Replay is always labeled non-live. Export JSON uses exclusive creation, mode 0600, and no final-path symlink following. It refuses overwrites. Choose a trusted parent directory; private permissions do not encrypt evidence. Export includes author jayis1 and integrity metadata even when the run has no useful policy coverage.

## Build, validation, and practical research workflow

Dependencies are GNU Make, ARM GNU GCC with newlib/nosys support, native GCC, Python 3 with Tk, KiCad 9 with standard footprints and pcbnew, and Xvfb for headless desktop testing. No package registry, cloud account, mobile SDK, or downloaded firmware SDK is required. From this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

The target outputs are `firmware/build/seal-freeze.elf` and `.bin`. Tests exercise all four captured values against all four subsequent values, every stable input-vector pair, startup without provenance, simultaneous seal/data changes, gaps, unrelated chatter, timer wrap, counter exhaustion, serialization, UART recovery, periodic policy after lost boot, and watchdog startup failure. The desktop tests feed the real C encoder's synthetic fixture through the parser and actual Tk event loop using a Linux pseudo-terminal. They do not demonstrate physical USB or target interoperability. CAD checks compare every schematic pin, PCB pad, footprint identity, component value and saved netlist with the manifest, in addition to ERC and DRC.

For an authorized product review, first verify signal mapping, electrical compatibility, and the owner's intended lock policy. Start the observer before the target's controlled reset. Hold RESET high with SEAL low, release reset, choose a configuration, allow it to settle, then seal. Exercise only approved normal service operations. A configuration deviation or seal withdrawal should remain visible even after the target returns to its prior state. Export the report with target version, observer source revision, fixture mapping, and independent waveform notes in the engagement record.

For remediation validation, repeat the same paced workload on the changed target firmware. Test a legitimate reset and reseal to a different value separately from an illegal post-seal change. Do not interpret a quiet UNKNOWN session as success: it has no established immutability coverage. Before fabrication inspect generated CAD and mechanical clearances. After assembly measure continuity, input margins, loading, port skew, sample timing, watchdog recovery, cable failure behavior, power sequencing and USB disconnect recovery. Environmental reliability, ESD, EMC, regulatory compliance and deployment suitability remain unverified. All original project work is credited to jayis1.
