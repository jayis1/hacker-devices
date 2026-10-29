# Contention Turn — Demand-Aware Round-Robin Security-Arbiter Witness

Author and creator: jayis1. Hardware revision A. Wire protocol CT1. Firmware, companion, and tests: GPL-2.0-only. Hardware utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked firmware, executable desktop software, and routed native KiCad source. See VERIFICATION.md for checks actually executed. Fabrication, programming a physical MCU, electrical qualification, oscillator calibration, and deployment remain outstanding. This instrument observes arbitration; it does not implement an arbiter, enforce access control, provide isolation, or certify availability.

## Purpose and practical value

Contention Turn investigates a subtle shared-resource policy failure: a controller advertises round-robin service, but its preference pointer advances incorrectly or fails to advance after an admission. Two clients can remain mutually exclusive while one repeatedly wins a contested selection. Conversely, an idle client should not prevent the only active client from receiving consecutive services. A strict alternation alarm would incorrectly reject that legitimate work. The device records actual service admissions, distinguishes established contention from simultaneous demand changes, and evaluates the next preference against the last unambiguous service.

An authorized development appliance might share an audit transport between application and management domains. Another might arbitrate a debug bridge, storage staging path, or recovery mailbox. The researcher exports four deliberately slow diagnostic indications: request A, request B, actual grant A, and actual grant B. The observer supplies independent cumulative evidence of admissions, wrong contended turns, orphan grants, overlap, correlation resets, and coverage. It captures no credentials, application data, storage addresses, user identities, or target serial-console contents.

The policy is demand-aware two-client round robin. After a clean service admission to A, the next preference is B. If both clients are already requesting at a subsequent admission, B must win. If only A requests, another A admission is permitted and preference remains B. Symmetric rules apply after B. Preference follows the actual clean service even when that selection was itself unfair. This checks a local next-selection rule, not a hypothetical ideal schedule that permanently diverges after one error.

## Repository-relative originality

The repository directories and device titles were inventoried, and the closest measurement contracts were compared before implementation. Admission Priority checks fixed priority and permits nonpreemptive service. Baton Order checks a strict four-owner cycle independent of demand. Starvation Window measures waiting age and repeated bypass thresholds. Handoff Audit measures command/feedback disagreement and insufficient dead time. Pulse Envelope checks independent per-lane pulse widths and recovery. None supplies this specific demand-aware next-preference ledger with unseeded attachment and reseeding after ambiguous admissions.

These differences have observable consequences. A continuously contended A-B-A-B sequence is correct here but repeatedly selecting B can violate a fixed A-priority rule. Repeated A admissions with B idle are correct here but violate unconditional alternation. One wrong selection can be reported immediately without waiting for Starvation Window's denial thresholds. An arbiter that stops granting altogether may produce no wrong-turn findings here while violating an availability deadline elsewhere. Choose the instrument that matches the product's declared scheduling contract.

The passive carrier topology, STM32 startup and register support, CRC routines, bounded UART driver, and desktop transport are intentionally reused from jayis1's Admission Priority platform. The policy engine, history-dependent preference semantics, telemetry contract, oracle tests, and assessment workflow are the device-specific contribution. Reusing infrastructure is not represented as inventing a new resistor circuit. Originality is relative to the inspected repository, not a worldwide patent search or claim that round-robin arbitration itself is new.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written permission to assess. Agree on the test points, diagnostic firmware, workload, responsible operator, stopping procedure, and evidence retention before attaching wires. General penetration-testing permission does not automatically authorize invasive physical instrumentation. Start on an isolated development fixture with consequential outputs disconnected. Do not attach this unisolated prototype to production locks, medical devices, vehicles, emergency systems, industrial machinery, mains wiring, or unknown-voltage field interfaces.

The assessed attack surface is the integration among pending demand, remembered service preference, and actual resource admission. Examples of relevant defects are a pointer updated on request rather than grant, a pointer reset on an idle interval, a retry path that bypasses rotation, or a stale favored-client branch. An authorized red team supplies the approved workloads through the target's own development interface. This observer does not create demand, change grants, inject traffic, generate faults, scan networks, deploy payloads, or retrieve protected information.

Diagnostic binding is a necessary assumption. A malicious target can lie on every wire. If GRANT reflects an intended selection variable rather than actual service entry, the observer may reproduce the target's own misconception. Review the adapter and corroborate it with an independent trace or permitted functional measurement. Requests are not evidence of authentication. Grants are not evidence of useful completed work. Correct alternation does not prove confidentiality, durable audit storage, fairness across more than two clients, or absence of vulnerabilities elsewhere.

The trusted base includes the host, operator, harness, observer image, Nucleo module, and programming interface. CRC detects accidental corruption, not forgery. There is no signed telemetry, secure boot, encrypted evidence store, persistent boot identity, paired secret, or authenticated updater. Physical SWD access permits firmware replacement. Record firmware identity and connection details separately under the engagement's chain-of-custody procedure. The companion has no network listener or automatic upload path.

Receive-only describes the supplied firmware rather than an irreversible electrical barrier. Replacement firmware could reconfigure GPIO outputs. Loading, common ground, shorts, and back-power can disturb equipment without intentional transmission. Stop if attachment changes target behavior, causes heat, resets a board unexpectedly, or reveals uncertain ground compatibility. A disclaimer and a clean software test do not establish electrical safety.

## Hardware specifications and BOM

The complete assembly uses an external ST NUCLEO-F401RE, with STM32F401RET6 Cortex-M4, 512 KiB flash, and 96 KiB SRAM. The module supplies the MCU, regulation, decoupling, reset circuitry, SWD access, and ST-LINK USB virtual COM interface. These circuits are not falsely depicted as absent parts on a standalone custom MCU board. Firmware assumes reset-default nominal 16 MHz HSI operation and a one-millisecond SysTick; it enables USART2 transmission at 115200 baud, eight data bits, no parity, one stop bit.

The custom PCB is a short-harness passive carrier, not a mechanically mating Arduino shield. It measures 80 by 70 mm, has two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks, 0.2 mm declared clearance, and through vias of 0.8 mm diameter with 0.4 mm drills. All fitted components are through-hole. There are no mounting holes or enclosure drawings; use an insulated support and strain relief. Four external digital indications are the sensing interface. No FPGA, radio, antenna, analog sensor, battery, charger, target power supply, storage card, or actuator is included.

| Reference | Quantity | Value and procurement candidate |
| --- | --- | --- |
| External development board | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm 1%, 0.25 W; Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm 1%, 0.25 W; Yageo MFR-25FBF52-1K |
| J1, J3–J6 | 5 | 1x2 2.54 mm vertical header; Samtec TSW-102-07-G-S |
| J2 | 1 | 1x4 2.54 mm vertical header; Samtec TSW-104-07-G-S |
| Harness and support | 1 set | Short insulated labelled leads, nonconductive tray |

Procurement candidates are not stock guarantees. Check supplier drawings, resistor body size, lead diameter, finish, and mating orientation. Resistor footprints use DIN0207 bodies with 7.62 mm lead spacing. The machine-readable manifest contains the same reference, value, footprint, and pin-net information as the schematic and board. The complete assembly is USB powered through the Nucleo's supported ST-LINK arrangement. Reserve a conservative 5 V, 500 mA host-source allowance subject to the module manual; actual power consumption is not measured. The carrier has no power rail and only loads diagnostic outputs.

| Bit | Signal | Target connector | MCU-side connector | Nucleo connection |
| --- | --- | --- | --- | --- |
| 0 | REQ_A | J3 pin 1 | J2 pin 1 | A0 / PA0 |
| 1 | REQ_B | J4 pin 1 | J2 pin 2 | A1 / PA1 |
| 2 | GRANT_A | J5 pin 1 | J2 pin 3 | A2 / PA4 |
| 3 | GRANT_B | J6 pin 1 | J2 pin 4 | A3 / PB0 |
| — | Common ground | J3–J6 pin 2 | J1 both pins | GND |

J1 is ground only; neither pin is a supply. J2 has four signals and no ground pin. Each external input passes through a 1 kOhm series resistor to an MCU-side node with a 100 kOhm pulldown. Plain names denote MCU-side nets; the `_IN` suffix denotes target-side nets. A disconnected signal tends low but cannot be distinguished reliably from real inactivity. At nominal high level the resistive path is 101 kOhm; actual loading also includes leakage and capacitance.

Only nominal zero-to-3.3 V, common-ground, push-pull diagnostic signals are supported. There is no 5 V, 1.8 V, negative-voltage, differential, open-collector, automotive, or industrial interface guarantee. No isolation, active buffer, dedicated ESD clamp, surge protection, or hot-plug guarantee is provided. The series resistor is not an overvoltage barrier. Power the observer first, establish compatible ground, then attach signals. Remove signals before depowering the observer to avoid input-diode back-power. Consult ST UM1724, RM0368, and the STM32F401xD/xE datasheet for the purchased revision and limits; confirm virtual COM solder bridges before assembly.

## Architecture and block diagram

```text
Authorized target             Passive carrier                NUCLEO-F401RE
REQ_A ------ J3.1 ----------- 1k ---+--- J2.1 ---------------> PA0 / A0
REQ_B ------ J4.1 ----------- 1k ---+--- J2.2 ---------------> PA1 / A1
GRANT_A ---- J5.1 ----------- 1k ---+--- J2.3 ---------------> PA4 / A2
GRANT_B ---- J6.1 ----------- 1k ---+--- J2.4 ---------------> PB0 / A3
                                  | each MCU-side node
                                 100k
                                  |
GND -------- J3..J6.2 ------------ GND -- J1 ---------------> GND / USB

GPIO -> independent qualification -> overlap observer
     -> both-grants-low baseline -> new admissions
     -> collision / request setup / demand-aware turn classification
     -> transactional ledger -> CT1 CRC encoder -> USART2 PA2 TX
     -> ST-LINK USB virtual COM -> Linux Ledger / History / Integrity
     -> explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not atomically. Inter-port skew and asynchronous sampling can alter apparent ordering near a boundary. Qualification rejects some short pulses and delays accepted edges; it does not reconstruct continuous physical behavior. Begin with plateaus and neutral intervals of at least twenty milliseconds and compare against a logic analyzer. This is a slow diagnostic-policy observer, not a high-speed bus decoder, metastability detector, or sub-millisecond ordering instrument.

## Exact firmware measurement contract

Every input qualifies independently. A new candidate appears on one sample and then must match for three consecutive one-millisecond observations, so four samples establish the new level. The prior qualified level persists during candidate settling. Unrelated request chatter cannot prevent a grant from qualifying. Duplicate calls at the same timestamp do nothing. Rejected raw glitches are not admissions and do not change the remembered preference. The measured object is the qualified diagnostic signal, not every electrical transition.

At startup, admission readiness and next preference are unknown. Readiness requires all four inputs qualified and both grants low. Requests need not be low. Attaching to an already high grant does not invent a leading edge. The overlap observer operates whenever both grant inputs are qualified, even if request chatter prevents overall readiness. A low baseline makes admission edges countable; it does not establish the arbiter's reset preference.

Each qualified grant rising edge after baseline increments starts, exactly one side total, and exactly one class. Classification is ordered:

1. Collided: peer grant is high in either the prior or incoming qualified observation. This includes direct A-to-B handover without an observed neutral sample. Simultaneous rising grants create two collided admissions. Preference becomes unknown.
2. Orphan: the selected client's request is not high in both adjacent qualified observations. A simultaneous request and grant does not establish prior demand. Preference becomes unknown.
3. Unfair: both requests are high in both adjacent observations, preference was known, and the admitted client differs from that preference. The contended counter also increases.
4. Accepted: a clean admission not classified unfair. An unknown preference is seeded from this admission and seeds increases. Such a seed is accepted as an unambiguous admission, not as proof that the initial selection was fair.

After every clean admission, including an unfair one, preference becomes the other client. Withdrawing a request during a held grant does not retroactively create an orphan. A new competing request during a held grant does not demand preemption or create a new admission. If the competing request arrives or withdraws in the same qualified observation as admission, contention is not proven and the clean admission is accepted. This conservative choice can miss a real unfair selection near a boundary; it avoids inventing intra-sample ordering.

No waiting timeout, maximum service duration, mandatory work-conservation rule, reset-preference check, weighted scheduling, or more-than-two-client policy is implemented. Zero unfair counts can coexist with no progress or almost entirely unseeded observations. Examine starts, seeds, contended admissions, gaps, and coverage together. Initial seeding under contention contributes to contended but not unfair. Repeated resets or ambiguous grants can destroy most of the usable turn history; those diagnostics are not evidence of fairness.

Overlap is a separate diagnostic. Entering both-qualified-grants-high increments overlaps, and every both-high observation increments overlap_ticks. A direct handover can be collided without a both-high sample, so collided and overlaps need not agree. Initial overlap can precede any admission. Missing time splits observed episodes; counts are not guaranteed unique physical incidents or exact electrical durations.

A nonunit nonzero timestamp delta records a gap and missed ticks, clears qualification, readiness, and preference, and retains historical evidence. Fresh both-low grants are required before counting new admissions. The next clean admission reseeds history. Ordinary unsigned tick wrap is supported, but an entirely unobserved full timer cycle cannot be detected. All counters are unsigned 32-bit. Each sample updates a copy; exhaustion discards the whole speculative sample and enters terminal halt, preserving conservation. Halt clears active readiness, preference, and qualification. Historical measurement fields freeze while time and saturating transmission diagnostics can advance.

Firmware contains no heap, RTOS, command receiver, filesystem, or network stack. The independent watchdog starts before normal reporting; startup failure sets terminal fault before the first snapshot. UART output owns a bounded buffer, services at most one available byte per call, and abandons a stalled frame after one hundred milliseconds. Watchdog timing, brownout behavior, GPIO thresholds, and execution on a real module remain hardware tests rather than inferred results from host simulations.

## Protocol and companion application

CT1 frames are ASCII `CT1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, with polynomial 0x1021, initial 0xffff, no reflection, and four uppercase hexadecimal digits. B contains reset-register flags once per boot. P contains `3,1,2,1`: qualification intervals, nominal tick period, client count, and policy revision. P repeats approximately once per second for late attachment without clearing host history. S is offered approximately every hundred milliseconds.

S fields in order are:

```text
now levels valid halted faults gaps missed ready observed starts accepted
orphans unfair collided side_a side_b overlaps overlap_ticks collision_active
baselines turn seeds contended tx_fail
```

Turn is zero unknown, one A preferred, two B preferred. Fault values 1, 2, 4, 8, 16, and 32 mean orphan admission, unfair contended turn, collision, acquisition gap, counter exhaustion, and unavailable watchdog. Starts equals accepted plus orphans plus unfair plus collided, and equals side_a plus side_b. Seeds cannot exceed accepted. Unfair cannot exceed contended, which cannot exceed accepted plus unfair. The encoder and parser check these and other flag, coverage, and transition bounds. Necessary consistency is not an exhaustive waveform-realizability proof or source authentication.

Run `python3 contention-turn/app/console.py` on Linux with Python Tk installed. The desktop is appropriate for a wired laboratory bench and needs no mobile SDK, pip packages, or cloud account. Enter an explicit serial path and select New live. The application opens read-only, configures 115200 8N1, flushes pending bytes, and restores terminal attributes where possible. A failed connection preserves previous evidence. Opening another port while one is owned is rejected. Disconnect preserves the session but removes live status.

Ledger displays all fields, named findings, readiness, and next preferred client. History retains up to 256 accepted snapshots. Integrity displays source, live/replay provenance, connection, policy compatibility, errors, receipt age, reset flags, and freeze reasons. LIVE requires a connection, compatible policy, two advancing snapshots, and recent progress. It does not certify acquisition readiness, known preference, physical device identity, or absence of faults. Duplicate timestamps do not refresh evidence age.

Counter rollback, disappearing sticky faults, contradictory duplicate timestamps, changing halted evidence, and impossible growth freeze the session. A boot after evidence preserves the old session and requires explicit replacement. Missing boot messages can sometimes be inferred from counter or clock rollback; some resets remain indistinguishable without persistent identity. A reception pause exceeding one second or cumulative host/device drift beyond 0.5 seconds plus five percent of host elapsed time, with the additional allowance capped at 0.5 seconds, freezes live evidence rather than relabelling backlog fresh. Initial fixed buffering delay remains unknowable from a first snapshot.

Replay opens nonblocking and accepts only a regular file no larger than two MiB. It cannot hang indefinitely on a FIFO masquerading as a recording. Full-marker resynchronization handles truncated frames, EOF partial tails count as errors, and at least one compatible snapshot is required. Replays are never live. JSON export uses exclusive creation, mode 0600, and final-symlink refusal; existing files are not overwritten. Exports include author jayis1, source, provenance, diagnostics, latest evidence, and bounded history. These permissions are not encryption or secure deletion. Apply the agreed retention policy manually.

## Reproducible validation and laboratory use

Required tools are GNU Make, native GCC, Arm GNU GCC with newlib/nosys, Python 3 with Tk, Xvfb for unattended GUI tests, and KiCad 9 CLI/Python bindings plus standard footprint libraries. The firmware has explicit reset vectors, linker layout, drivers, board.h, and registers.h; no vendor SDK download is needed. Run from the repository root:

```sh
make -C contention-turn/firmware clean all test
xvfb-run -a /usr/bin/python3 -m unittest discover -s contention-turn/tests -p 'test_*.py' -v
/usr/bin/python3 contention-turn/tools/check_hardware.py
python3 contention-turn/tools/check_release.py
```

Target artifacts are ignored ELF, binary, and linker map files. Flash starts at 0x08000000 and RAM at 0x20000000; the linker reserves stack headroom. Program through your authorized ST-LINK workflow only after reviewing the hardware. Reset into the image rather than entering with debugger-altered clocks. The production engine is host-tested against a separate last-served oracle, exhaustive request masks, simultaneous demand changes, gaps, wrap, overlap, startup attachment, and transactional exhaustion. Register-model tests exercise UART timeout and scheduler startup behavior.

Companion tests use actual C-encoder synthetic fixtures, a real Tk event loop, and Linux pseudo-terminals. They exercise connection, disconnect, screens, file restrictions, malformed frames, unsupported policy, freshness, reset suspicion, and private exports. These fixtures are explicitly not hardware captures. The CAD checker runs ERC and DRC with violations fatal and compares every reference, value, footprint, pad, and net among a fresh schematic netlist, PCB, and manifest. Passive-only ERC cannot validate the external module's power supply or target voltage.

For a controlled researcher trial, establish both grants low, hold both requests high, then admit A, return neutral, and admit B. Repeat to establish alternating service. Next configure the owned fixture to admit B twice under persistent dual demand and compare the unfair counter with the independent trace. Then remove A demand and repeat B services: these should be accepted, demonstrating that the measurement is demand-aware rather than unconditional alternation. Finally introduce a direct handover, restore a neutral interval, and verify that the next clean admission reseeds rather than inheriting ambiguous history.

An authorized red-team report should identify the declared scheduling policy, target revision, adapter binding, workload, observed selection discrepancy, observer image, and sampling limitations. A penetration tester can repeat the exact bounded workload after remediation without claiming that a GPIO finding alone proves exploitation or denial of service. Before deployment, separately verify fabrication, soldering, connector orientation, every input bit, serial output, watchdog reset, oscillator accuracy, loading, ground compatibility, brownout recovery, and EMC/ESD suitability. Passing software and CAD checks does not turn an unbuilt engineering prototype into a production-qualified security instrument.
