# Sticky Trip — Authorized-Clear Alarm Retention Witness

Author and creator: jayis1. Hardware revision A. Protocol ST1. Firmware, application, and tests: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design. Target firmware has a real Cortex-M4 reset path and build target; the companion is a Linux desktop application. Native KiCad sources describe a routed passive carrier for an external NUCLEO-F401RE. See VERIFICATION.md for executed checks. Fabrication, programming, electrical qualification, and physical timing measurements are not claimed. This device observes diagnostic policy; it does not control an alarm or provide a safety interlock.

## Purpose and overview

Sticky Trip measures whether an alarm indication retains a witnessed trip until a fresh, authorized clear request. Four deliberately slow diagnostic lines expose TRIP, CLEAR, AUTH, and LATCH. The observer reconstructs the expected latch state from the first three lines and compares that obligation against the fourth. Unlike a combinational comparator, it remembers an obligation after the original trip has disappeared. A controller that implements its supposed sticky alarm as a direct copy of the sensor can look correct during the stimulus, then silently lose the security event afterward. This instrument makes that retention failure visible.

A second defect arises when an old clear request remains asserted. If authority is subsequently granted, a level-sensitive controller may clear an alarm even though nobody made a new request. Sticky Trip requires a qualified low-to-high CLEAR transition with AUTH already qualified high in the preceding observation and still high now. Merely raising authorization underneath a held clear does not satisfy this contract. TRIP dominates every clear attempt, including an otherwise authorized simultaneous request. The exact ordering rule is intentionally explicit rather than an undocumented implementation detail.

Potential subjects include tamper-alarm retention in an owned development appliance, sticky fault registers exposed through test firmware, enclosure-monitor event memory, and authorized maintenance workflows. The device records missing-latch and spurious-latch observations, episode counts, longest runs, accepted clear edges, rejected clear edges, and changes into the expected set state. It does not establish who authenticated, whether a sensor represents a real intrusion, or whether an alarm reaches a remote responder. Those remain separate assessment questions requiring independent evidence.

## Originality within this repository

The repository directories and relevant README purposes were inspected before selecting this contract. Latch Ledger studies disconnected mechanical contact chatter and paired disagreement; it does not reconstruct a controller's retained alarm. Reset Quarantine observes reset provenance and a minimum readiness settling interval before privilege. Mask Containment compares independent allowed and effective permissions without memory of a past trip. Sticky Trip instead implements an observed set-dominant memory with fresh-edge, prior-authorization clear semantics. Two identical current input vectors can therefore legitimately have different expected outputs depending on earlier accepted history.

The Nucleo startup, register map, bounded UART, passive carrier topology, and desktop transport foundation intentionally reuse jayis1-authored infrastructure from Mask Containment. Those parts are not claimed as newly invented electronics. The new contribution is the retention/clear policy, its evidence invariants, companion contract, regression vectors, and practical assessment procedure. Originality here means a distinct device-level contribution relative to the inspected repository, not worldwide novelty, an exhaustive prior-art search, or patentability.

## Attack surface, trust model, and legal boundary

Use only on equipment you own or have explicit written permission to assess. The authorization should name the test firmware, physical diagnostic points, allowable stimuli, evidence retention period, and person responsible for stopping the experiment. Begin on a development bench with consequential outputs disconnected. Do not connect this unisolated prototype to installed security alarms, fire detection, medical systems, machinery, vehicle controls, mains circuits, or any system whose unexpected behavior could endanger people. Authorization does not remove electrical hazards or substitute for engineering qualification.

The investigated boundary separates alarm occurrence, permission to clear, clear intent, and retained state. Defects may include a level-sensitive clear handler, a reset path dropping retained state, a stale authorization cache, or a race that lets clear beat trip. An authorized red team can exercise approved maintenance and recovery workflows through the target's ordinary test interface while the observer collects independent indication-level evidence. Sticky Trip sends no target commands, creates no stimulus, captures no credentials, and implements no exploit, covert implant, persistence, network scanner, or exfiltration mechanism.

A malicious target can lie on all four diagnostic lines. Deriving every indication from the same intended variable can create a circular test that hides the actual defect. Prefer a LATCH output derived as near as practical to the retained register or real alarm output, and independently document the adapters exposing TRIP and authority. A hostile workstation or substituted serial device can also fabricate coherent records. CRC is accidental-corruption detection, not authentication. There is no signed evidence, trusted timestamp, secure element, authenticated pairing, tamper resistance, or secure boot guarantee.

The target-facing pins are inputs in the supplied firmware. This is not a hardware data diode: replacement firmware could drive them through the series resistors. Common ground, shorts, input loading, and back-powering can disturb a target despite receive-only software. The local operator must control programming access, inspect wiring, and restrict serial-device permissions. Export files contain diagnostic histories rather than payload data, but those histories can still reveal test schedules and internal behavior. Store them according to the engagement's confidentiality and retention policy.

## Hardware specification and bill of materials

The processor module is an external ST NUCLEO-F401RE, MB1136 family, carrying STM32F401RET6, Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock and a one-millisecond SysTick. USART2 transmits at 115200 baud, eight bits, no parity, one stop bit through PA2 and the module's ST-LINK virtual COM bridge. Reception is disabled. The module supplies its own regulator, MCU decoupling, reset, programming interface, and USB connectivity; those are not missing discrete parts on the passive carrier.

The custom carrier is two-layer, 80 by 70 mm, nominally 1.6 mm thick, with through-hole components, a closed outline, and 0.3 mm routed tracks. It is harness-connected, not a mechanically stackable Arduino shield. There is no FPGA, radio, antenna, battery, charger, analog sensor, onboard flash storage, level translator, target power output, or actuator. An insulating bench support, strain relief, and clearly labelled short leads are required. Revision A supplies no enclosure model or mounting holes.

| Reference | Quantity | Requirement / candidate |
| --- | --- | --- |
| External processor | 1 | ST NUCLEO-F401RE; inspect purchased MB1136 revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial, Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial, Yageo MFR-25FBF52-1K |
| J1, J3, J4, J5, J6 | 5 | 1x2 straight 2.54 mm header, Samtec TSW-102-07-G-S |
| J2 | 1 | 1x4 straight 2.54 mm header, Samtec TSW-104-07-G-S |
| Harness and support | 1 set | Insulated leads, nonconductive tray, external strain relief |

Candidate parts are dimensional examples, not stock guarantees. Resistor footprints are DIN0207 with 7.62 mm lead spacing. Check actual supplier drawings before procurement. The board contains fourteen components, thirty electrical pads, and nine nets. Connector pin one has a rectangular pad, but headers are not keyed: independent continuity checks remain necessary.

### Wiring and electrical restrictions

| Raw bit | Meaning | Target signal pin | MCU-side pin | Nucleo connection |
| --- | --- | --- | --- | --- |
| 0 | TRIP, active high | J3.1 | J2.1 | CN8 A0 / PA0 |
| 1 | CLEAR, active high | J4.1 | J2.2 | CN8 A1 / PA1 |
| 2 | AUTH, active high | J5.1 | J2.3 | CN8 A2 / PA4 |
| 3 | LATCH, active high | J6.1 | J2.4 | CN8 A3 / PB0 |
| none | Ground | J3–J6.2 | J1, both pins | Nucleo GND |

J1 is ground only, never power. J2 has no ground pin. Each target input passes through a 1 kOhm series resistor to an MCU-side node with a 100 kOhm pull-down. Net names ending `_IN` denote the target side; plain signal names denote the GPIO side. A disconnected wire tends low and is not automatically detected. Losing LATCH can resemble missing retention; losing CLEAR can conceal a legitimate request. Validate cable continuity separately from interpreting the evidence.

Only common-ground, nominal 0–3.3 V push-pull diagnostic outputs are supported. Five-volt logic, 1.8 V logic, negative voltages, differential buses, relay coils, dry-contact excitation, unknown field wiring, and hot-plug operation are outside scope. There is no galvanic isolation, active buffer, dedicated ESD clamp, surge protection, or reverse-current blocker. Series resistance is not permission to violate MCU absolute maximum ratings. The total high-level resistor path is 101 kOhm; use actual voltage, tolerances, leakage, and target output impedance when calculating loading rather than treating this nominal resistance as a measured input specification.

Reserve a conservative 5 V, 500 mA USB source allowance for the whole development-board assembly, subject to the board manual and actual USB limits. Consumption and thermal behavior remain unmeasured. Power the observer before enabling target outputs, and remove target outputs before depowering the observer to reduce back-power risk. Do not bridge hazardous or separately isolated grounds. ST UM1724 describes the MB1136 connector and virtual COM arrangements; verify SB13/SB14 and SB62/SB63 configuration against the actual board. RM0368 describes the F401 register map; the STM32F401xD/xE datasheet supplies electrical limits. These documents and the purchased-board revision must be checked before physical bring-up.

## Architecture and block diagram

```text
Owned target                      Passive carrier          External Nucleo
 TRIP ---- J3.1 ------------------ 1k --+-- J2.1 ----------- PA0 / A0
 CLEAR --- J4.1 ------------------ 1k --+-- J2.2 ----------- PA1 / A1
 AUTH ---- J5.1 ------------------ 1k --+-- J2.3 ----------- PA4 / A2
 LATCH --- J6.1 ------------------ 1k --+-- J2.4 ----------- PB0 / A3
                                      | each GPIO node
                                     100k
                                      |
 GND ----- J3..J6.2 ----------------- GND -- J1 ------------ GND / USB

GPIOA then GPIOB -> independent channel qualification
 -> qualified clear-edge provenance -> set-dominant expected memory
 -> compare LATCH -> coverage, mismatch episodes, longest runs
 -> transactional counter update -> ST1 CRC frame -> bounded UART TX
 -> ST-LINK USB -> Linux Ledger / History / Integrity screens
 -> explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not latched simultaneously. Oscillator error, port-read skew, interrupt latency, and qualification can affect apparent ordering. Start with plateaus of at least twenty milliseconds and corroborate waveforms with a logic analyzer. This is not a high-speed glitch detector, memory bus tap, metastability instrument, or proof of continuous electrical retention. An unsampled or rejected short pulse can remain invisible.

## Exact firmware contract

Each bit independently qualifies after the initial candidate sample and three matching one-millisecond intervals. A changed candidate does not replace the previously qualified level until it qualifies. Initial accounting waits until all four inputs have qualified; chatter during this initial acquisition can therefore prevent any coverage. After qualification, chatter on an unrelated input does not invalidate an already established obligation. The valid mask and known flag distinguish accepted input states from an established memory contract.

At startup the expected latch state is unknown, even if every input reads low. No assumption is made about an event before attachment. Once all bits qualify, a high TRIP establishes expected set immediately. Alternatively, an accepted CLEAR edge establishes expected clear. An initially high CLEAR is not an edge. AUTH must be high in both the preceding complete qualified observation and the current one, CLEAR must rise, and current TRIP must be low. Simultaneous newly qualified AUTH and CLEAR is rejected conservatively. TRIP and CLEAR together always select set and reject the edge.

Once known, expected state persists through TRIP release, AUTH withdrawal, and arbitrary time without requests. A held CLEAR is not repeatedly consumed. Rejected clear edges increment rejected and set a sticky finding but do not change the obligation. An accepted clear increments clears even if expected state was already clear. Sets counts transitions from unknown or clear into set, not every sample of an asserted trip. Trip qualifying together with LATCH high is consistent; there is no response grace interval in revision A. Integrations requiring a timed alarm response need a different declared contract rather than silently forgiving mismatches.

Every known observation adds one coverage sample. Expected set with low LATCH adds one missing observation; expected clear with high LATCH adds one spurious observation. Each mismatch direction has total observations, episode entries, current streak, and historical maximum. Leaving a mismatch clears its current streak, not its historical evidence. Both mismatch directions cannot be active in one sample. Counts are qualified observations, not exact electrical milliseconds or guaranteed numbers of distinct physical incidents.

Repeated timestamps are ignored. A nonzero time delta other than one records an acquisition gap and missing nominal ticks, then discards qualification, edge provenance, and expected state. Unknown time never earns retention credit. Historical totals remain. A still-high TRIP can establish a new obligation after requalification; an already-high CLEAR cannot. A continuing physical mismatch spanning a gap can become another observed episode, which is why the exported counts describe segments rather than unique intrusions. Unsigned subtraction handles ordinary timer wrap but cannot identify an entirely unseen full timer cycle.

Sample changes are speculative until all additions succeed. Counter overflow discards that sample, freezes prior cumulative evidence, clears current acquisition state, and enters terminal halt with exhaustion flagged. Watchdog startup failure also halts acquisition. The serializer checks mask ranges, halt consistency, provenance, coverage conservation, mismatch totals, episode maxima, and sticky findings before emitting status. Transmission failures use a separately saturating diagnostic counter. Halted timestamps can advance, but frozen evidence does not resume without reset.

The firmware uses C11, no heap, no RTOS, no filesystem, no command receiver, and no flash journal. Startup installs Cortex-M vectors and initializes data/BSS. The linker maps flash at 0x08000000 and reserves stack headroom. Entry from an arbitrary bootloader-modified clock configuration is unsupported. The watchdog readiness loops are bounded; the foreground loop feeds it after acquisition and telemetry service. Actual watchdog period and reset behavior require silicon testing. Reset loses all RAM evidence; programming is a supervised ST-LINK operation, not an application command.

## Companion application and serial protocol

Run `python3 app/console.py` on Linux with Tk installed. Enter the explicit serial path and select New live. Ledger shows all fields, named findings, and expected latch obligation. History displays up to 256 accepted snapshots. Integrity reports provenance, connection, policy compatibility, reset flags, parser errors, evidence age, and freeze reason. Disconnect preserves evidence but removes live status. Replay loads a bounded regular file and is always labelled non-live. Export JSON creates a new private file and refuses overwrite rather than silently replacing evidence.

Records are ASCII `ST1,<kind>,<decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE covers everything before the asterisk: polynomial 0x1021, initial 0xffff, no reflection, no final XOR, four uppercase hexadecimal digits. B contains reset flags. P contains `3,1,1`: qualification intervals, nominal sample period, and the prior-sample authorization requirement. Policy repeats approximately every second to support late attachment; boot is sent only once and never masquerades as a recurring policy message.

S fields are: now, levels, valid, halted, faults, gaps, missed, known, expected, sets, clears, rejected, coverage, missing, spurious; then t0,t1, e0,e1, c0,c1, x0,x1; finally tx_fail. Each field is unsigned 32-bit. The t/e/c/x pairs mean total observations, episodes, current streak, and maximum streak respectively; index zero is missing and index one spurious. Fault bits zero through five mean missing latch, spurious latch, acquisition gap, exhaustion, unavailable watchdog, and rejected clear. All findings are sticky until observer reset.

Status is offered approximately every hundred milliseconds. UART service sends at most one ready byte per visit and abandons a stalled frame after one hundred milliseconds. Status is cumulative, not an event-complete trace. The app bounds framing storage, checks integer ranges and CRC, resynchronizes on the full ST1 marker, and validates snapshot invariants. Counter rollback, disappearing findings, contradictory duplicate timestamps, impossible growth, changed halted evidence, or an observed boot after measurements freeze the retained session.

LIVE requires a connected source, supported policy, two advancing snapshots, and recent device progress. It does not mean acquisition is known, the target is secure, or the source is authentic. Duplicate timestamps do not refresh freshness. A reception pause longer than one second requires explicit reconnect. Cumulative host/device progress divergence is limited by half a second plus five percent of elapsed host time, capped at one second total allowance. Honest oscillator drift can eventually require reconnect. Fixed initial transport delay and some unannounced resets remain indistinguishable without authenticated boot identity.

Replay input must be a regular file of at most two MiB, checked on an opened nonblocking descriptor before allocation. FIFOs and devices are rejected. A partial final record increments parser errors; a recording without compatible status is rejected. Export uses mode 0600 with exclusive creation and no final-path symlink following. Choose a trusted parent directory. Private permissions do not encrypt files, and ordinary deletion does not guarantee secure erasure. Export metadata includes author jayis1, source, live/replay origin, parser errors, policy, connection, freeze reason, latest snapshot, and bounded history even when no useful trial completed.

## Build, verification, and research workflow

Dependencies are GNU Make, ARM GNU GCC with newlib/nosys, native GCC, Python 3 with Tk, KiCad 9 with standard footprints and pcbnew bindings, and Xvfb for headless GUI testing. No downloaded Python packages, cloud services, or mobile SDK are required. From this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

Firmware outputs are `firmware/build/sticky-trip.elf` and `.bin`. Tests cover retained obligations, initial-high clear rejection, authorization arriving too late, set precedence, all 256 stable vector transitions, unrelated chatter, gaps, timestamp wrap, transactional exhaustion, CRC, bounded output, UART timeout recovery, periodic policy after lost boot, and watchdog failure. Companion tests use actual production-C fixtures, a Linux pseudo-terminal, and the real Tk event loop. The fixture is synthetic software evidence, never a claimed hardware capture. ERC and DRC run with violations fatal; connectivity checking compares every schematic pin, board pad, value, footprint identity, and saved netlist against the manifest.

For a controlled vendor assessment, first document what each diagnostic line represents. Establish a known clear state using a fresh authorized request, introduce a permitted test trip, remove the stimulus, and confirm retention. Try a denied clear and then authority arriving under a held request; neither should remove the obligation. Finally lower CLEAR, maintain authorization, and issue a fresh edge. Repeat with TRIP held high to check set precedence. Record gaps and input validity alongside findings, and export before resetting the observer.

For remediation validation, compare the same paced workload before and after a target firmware fix. Preserve target version, observer source revision, fixture mapping, authorization, and independent waveform evidence. A missing-latch observation supports investigation of the declared indication contract; it does not prove a physical intrusion went unreported. A quiet run cannot establish security for untested paths. Before fabrication inspect rendered CAD, supplier dimensions, grounding, and strain relief. After assembly measure continuity, input margins, loading, sample timing, read skew, watchdog recovery, USB disconnect behavior, back-power, and reset sequencing. ESD, EMC, environmental reliability, calibration, and regulatory suitability remain unverified. Original design, firmware, application, tests, and documentation author: jayis1.
