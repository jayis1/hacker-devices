# Admission Priority — Nonpreemptive Security-Arbiter Evidence Bench

Author and creator: jayis1. Hardware revision A. Wire protocol AP1. Firmware, application, and tests: GPL-2.0-only. Hardware generation/checking tools: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked firmware, an executable Linux desktop companion, and routed native KiCad source. See VERIFICATION.md for executed checks. Fabrication, MCU programming, calibrated timing, electrical interoperability, and deployment have not been performed. The device is an observer, not an arbiter, access controller, safety interlock, or certification of security.

## Purpose and practical value

Admission Priority investigates a scheduling boundary that a simple mutual-exclusion alarm cannot evaluate: when a shared security resource becomes available, does the target admit the highest-priority pending requester? It distinguishes a wrongly selected low-priority admission from a legitimate low-priority operation that was already running when a higher-priority request arrived. The latter is permitted by this explicitly nonpreemptive contract. Checking only the current request and grant levels would confuse these two histories and could generate misleading security findings.

An owned development appliance might share a maintenance transport between ordinary application work and a higher-priority recovery function. Another might share an audit writer between routine records and an urgent diagnostic client. A defective arbitration path can choose the wrong pending client while preserving perfect mutual exclusion. The observer records admission classes, requester-specific totals, simultaneous ownership, and acquisition coverage. It captures no credentials, application payloads, audit contents, storage addresses, or identities.

The target exports two request indications and two actual grant indications as deliberately slow diagnostic signals. A grant rising edge represents the beginning of service. Holding that grant means the service continues, not that new requests are being admitted. The instrument requires a prior observed request, rejects ambiguous direct ownership handovers, and checks high-priority demand at the low-priority admission boundary. It does not force preemption, impose a service deadline, or demand that every request eventually be served.

## Originality and closest existing devices

The repository directories and README purposes were inventoried before selecting this concept. The closest full contracts examined were Starvation Window and Baton Order; Exclusion Ledger's stated scope was also compared. Starvation Window measures waiting age and repeated bypasses, so it may flag prolonged denial even when every admission obeys fixed priority. Baton Order checks a four-owner strict cycle, which is inappropriate for demand-driven priority admission. Exclusion Ledger summarizes pairwise concurrent enables rather than the selection decision at admission. Admission Priority instead checks a two-requester fixed-priority selection rule with explicit nonpreemption and conservative request-setup semantics.

The same trace can therefore produce different legitimate answers in these instruments. A low grant followed by a new high request is not an Admission Priority inversion. A subsequent fresh low grant while high demand is pending is an inversion immediately, even if the high requester has not waited long enough to violate a fairness budget. A fixed-priority arbiter can also starve its low-priority client without violating this device's contract. These distinctions are useful rather than cosmetic renaming.

The passive carrier, Cortex-M4 startup, register support, CRC implementation, bounded transmitter, and desktop transport deliberately reuse jayis1-authored infrastructure from Threshold Memory. The admission engine, policy, evidence invariants, dedicated fixtures, and laboratory workflow are new within this inspected repository. No global invention, patentability, or exhaustive prior-art claim is made. The carrier is not presented as a novel electrical circuit merely because its labels differ.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written permission to assess. Agree on the diagnostic firmware, exact test points, permissible workloads, connection schedule, stopping procedure, and evidence retention before connecting wires. A general network penetration-testing scope does not automatically authorize physical instrumentation. Use an isolated development fixture with consequential outputs disconnected. Do not attach this unisolated prototype to production locks, medical equipment, vehicles, industrial machinery, emergency infrastructure, mains circuits, or unknown-voltage field wiring.

The assessed attack surface is the integration between pending demand, selection logic, and actual service admission. Relevant faults include an inverted priority comparison, a stale low-priority fast path, a request sampled after grant assertion, duplicated grant state, or a direct owner transition whose order is not observable. An authorized researcher supplies workloads through the target's normal laboratory interface. This observer never creates requests, drives grants, injects faults, sends exploits, scans networks, or modifies target state intentionally.

Diagnostic binding is a critical assumption. A malicious target can lie on every wire. If GRANT merely mirrors the selection variable rather than the real resource gate, the measurement may repeat the bug's intended story rather than expose its actual effect. Review the adapter source and corroborate it against an independent trace or permitted functional measurement. A diagnostic request is not proof of user authentication or authorization. A clean capture cannot prove correct resource contents, useful service, complete audit persistence, confidentiality, or absence of exploitable behavior elsewhere.

The trusted base includes the operator, observer firmware, development board, harness, host, and programming interface. CRC detects accidental corruption, not forgery. There is no cryptographic pairing, signed evidence, protected key, secure boot, persistent boot identifier, authenticated update mechanism, or encrypted storage. A substituted USB source can fabricate plausible records. Physical ST-LINK access can replace firmware. Restrict access and preserve build identities under the engagement's evidence procedure rather than describing JSON as cryptographic attestation.

Receive-only describes the supplied firmware, not irreversible electrical enforcement. Replacement firmware could configure GPIO outputs. Loading, common ground, cable shorts, and back-power can disturb equipment without any deliberate transmission. Stop if attachment changes target behavior or creates heat, unexpected resets, or uncertain ground potentials. The absence of radios and a target command parser reduces functionality and risk; it does not establish electrical safety.

## Hardware specifications and component selection

The complete assembly uses an external ST NUCLEO-F401RE board with STM32F401RET6, a Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI operation and uses a one-millisecond SysTick. USART2 TX on PA2 emits 115200 baud, eight data bits, no parity, one stop bit through the board's ST-LINK virtual COM bridge. USART reception is disabled. The purchased module supplies the MCU, regulator, decoupling, reset, USB interface, and SWD programming facilities; these are not fictitious components omitted from the passive carrier.

The custom carrier is 80 by 70 mm, two-layer, nominally 1.6 mm FR-4. It contains fourteen through-hole components and thirty electrical pads, with nine named nets and a closed board outline. Tracks are 0.3 mm; the project declares 0.2 mm clearance and 0.8 mm through vias with 0.4 mm drills. There are no mounting holes or enclosure files. Use an insulating support with strain relief. This is a short-harness carrier, not a mechanically mating Arduino shield.

| Reference | Quantity | Specification / procurement candidate |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, MB1136 family; inspect purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W; Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W; Yageo MFR-25FBF52-1K |
| J1, J3–J6 | 5 | 1x2 straight 2.54 mm header; Samtec TSW-102-07-G-S |
| J2 | 1 | 1x4 straight 2.54 mm header; Samtec TSW-104-07-G-S |
| Harness/support | 1 set | Short insulated labelled leads and nonconductive bench tray |

Part numbers are purchasing candidates, not current-stock guarantees. Verify supplier drawings, pin finish, lead diameters, and resistor body dimensions against footprints. The resistor footprint is DIN0207 with 7.62 mm lead spacing. There is no battery, charger, FPGA, radio, antenna, active sensor, flash journal, analog front end, target power output, or actuator. Reserve a conservative 5 V, 500 mA USB-source allowance for the development assembly, subject to the Nucleo manual and jumper configuration. Actual current consumption is not measured.

| Bit | Meaning | Target signal pin | MCU-side connector | Nucleo point |
| --- | --- | --- | --- | --- |
| 0 | REQ_H, high-priority demand | J3.1 | J2.1 | A0 / PA0 |
| 1 | REQ_L, low-priority demand | J4.1 | J2.2 | A1 / PA1 |
| 2 | GRANT_H, high-priority service | J5.1 | J2.3 | A2 / PA4 |
| 3 | GRANT_L, low-priority service | J6.1 | J2.4 | A3 / PB0 |
| none | Common ground | J3–J6.2 | J1, both pins | GND |

J1 is ground only; neither pin is a supply. J2 contains four signals and no ground pin. Each external signal passes through 1 kOhm to an MCU-side node with a 100 kOhm pull-down. Plain signal names denote MCU-side nets and `_IN` denotes target-side nets. A disconnected wire tends low and cannot be distinguished reliably from a real low indication. A short to an asserted source can look like legitimate demand or ownership.

Only common-ground nominal 0–3.3 V push-pull diagnostic outputs are supported. Five-volt logic, 1.8 V logic, negative voltages, differential buses, coils, and unknown industrial signals are excluded. There is no isolation, active buffer, dedicated ESD suppression, overvoltage clamp, surge rating, or hot-plug guarantee. The nominal resistive load path is 101 kOhm, but leakage, source impedance, and cable capacitance affect real behavior. Power the observer first, establish compatible ground, then attach signals. Remove signals before depowering the observer.

## Architecture and block diagram

```text
Authorized development target       Passive carrier          External Nucleo
 REQ_H ----- J3.1 ----------------- 1k --+--- J2.1 ----------- PA0 / A0
 REQ_L ----- J4.1 ----------------- 1k --+--- J2.2 ----------- PA1 / A1
 GRANT_H --- J5.1 ----------------- 1k --+--- J2.3 ----------- PA4 / A2
 GRANT_L --- J6.1 ----------------- 1k --+--- J2.4 ----------- PB0 / A3
                                       | each node
                                      100k
                                       |
 GND ------- J3..J6.2 ---------------- GND --- J1 ------------ GND / USB

GPIO samples -> independent qualification -> grant-overlap observer
             -> both-grants-low baseline -> new grant edges
             -> collision / prior request / priority classification
             -> transactional cumulative ledger -> checked AP1 encoder
             -> bounded USART2 TX -> ST-LINK virtual COM -> Linux desktop
             -> Ledger / History / Integrity -> explicit private JSON export
```

GPIOA is read before GPIOB. This is not atomic cross-port acquisition, a high-speed bus analyzer, or a metastability detector. Qualification can change apparent event ordering. Begin with at least twenty-millisecond plateaus and generous setup and neutral intervals, then compare with an independent logic analyzer. Pulses entirely between samples or too short to qualify can disappear. The device measures qualified diagnostic observations, not continuous electrical timing.

## Exact firmware measurement contract

Each of the four inputs qualifies independently. A candidate must appear once and then match for three consecutive one-millisecond observations, requiring four samples after change. Duplicate calls in the same tick do nothing. Once qualified, the old accepted level remains visible while a replacement candidate settles. A short rejected glitch does not clear a demand or end a grant. Independent qualification prevents chatter on one request from indefinitely hiding a stable change on a grant. It does not authenticate that change or reconstruct unobserved edges.

At startup, admission correlation is not ready. It becomes ready only after all four inputs qualify and both grants are low. Request levels need not be low. Attaching to an already high grant never invents an admission. While waiting for baseline, the separate overlap observer still reports both qualified grants high, even if request chatter prevents full-vector qualification. Initial coverage can therefore be limited while an overlap finding remains meaningful.

After baseline, a qualified low-to-high grant transition is one admission. A held grant contributes no further admissions. Both simultaneous rising grants create two admissions, each classified as collided. All admissions increment starts, exactly one of high or low, and exactly one classification counter. The classes have this precedence:

1. Collided: the peer grant is high in either the previous or incoming qualified observation. This includes direct handover when one grant falls exactly as the other rises. An observed neutral sample is required to avoid that ambiguity.
2. Orphan: the admitted side's own request is not high in both adjacent qualified observations. A simultaneous request and grant is insufficient prior setup. Orphan takes precedence over priority inversion.
3. Inversion: a low-priority admission has high-priority demand in either adjacent observation. A simultaneous high-request withdrawal does not prove withdrawal happened before selection; a simultaneous high-request arrival conservatively vetoes the low selection.
4. Accepted: none of the preceding conditions applies.

The contract concerns admission, not continued eligibility. Removing a request while its grant stays high does not add an orphan. A high-priority request arriving during an already held low grant does not add an inversion or demand preemption. The target may finish that service. A later new low admission must reevaluate priority. No liveness deadline, maximum ownership duration, waiting-age estimate, or fairness promise is implemented. A stalled arbiter can have zero bad admissions and almost no useful work; compare starts and observed coverage rather than treating zero findings as success.

Overlap is tracked separately on every sample where both grant channels are qualified. Entering both-high increments overlaps; every both-high observation increments overlap_ticks. Direct both-high persistence remains one episode. A fully observed release and later overlap begins another. Collided admissions can occur without any overlap observation during a direct handover, so those totals are intentionally not equal. Overlaps can also exist before admission baseline, without starts. Counts describe observed segments and sampled occupancy, not necessarily unique physical incidents or exact electrical duration.

A skipped millisecond increments gaps and missed ticks, clears qualification and readiness, and preserves historical counts. Fresh both-low grants are required before counting new admissions; held post-gap grants are not new edges. The overlap observer can resume after its two grant channels requalify. One continuous physical overlap split by a gap can become multiple observed episodes. Ordinary unsigned timestamp wrap is supported, but an entirely unseen full clock cycle is not detectable from a modulo timestamp alone.

All counters are unsigned 32-bit. Each sample is evaluated on a copy of the ledger. Any overflow discards that complete speculative sample and enters terminal halt, retaining previous evidence and conservation identities. Halt clears readiness, qualification, and active overlap; measurement fields otherwise freeze. Time and saturating transmit-failure diagnostics may continue. Watchdog startup failure also halts before the first status. The main loop uses no heap, RTOS, command receiver, filesystem, or network stack. UART servicing sends at most one ready byte per call and abandons a blocked frame after one hundred milliseconds.

## Protocol and companion application

AP1 records are ASCII `AP1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, uses polynomial 0x1021 and initial 0xffff, and is represented by four uppercase hexadecimal digits. B sends reset-register flags once per boot. P sends `3,1,2,0`: qualification intervals, nominal tick period, requester count, and higher-priority requester index. P repeats approximately once per second for late attachment and never clears retained host evidence. S snapshots are offered approximately every hundred milliseconds, not at every admission.

S fields in exact order are:

```text
now levels valid halted faults gaps missed ready observed starts accepted
orphans inversions collided high low overlaps overlap_ticks collision_active
baselines tx_fail
```

Fault values 1, 2, 4, 8, 16, and 32 mean orphan admission, priority inversion, collision/direct handover, acquisition gap, exhausted counter, and unavailable watchdog. Starts equals accepted plus orphans plus inversions plus collided, and also equals high plus low. Baselines cannot exceed gaps plus one. Observed counts fully qualified samples, not necessarily ready samples or exact wall time. The encoder and companion reject contradictory masks, class totals, flags, and occupancy bounds. These checks are necessary consistency conditions, not an exhaustive proof that every sparse stream is physically realizable.

Run `python3 admission-priority/app/console.py` on Linux with Python Tk available. Enter the intended serial path and select New live. No device scanning, target command, or cloud service exists. The application opens the serial port read-only, configures 115200 8N1, flushes buffered input, and restores terminal attributes on close where possible. A failed connection preserves prior evidence; a successful new connection creates a new host session. Disconnect keeps evidence but removes live status.

Ledger shows all fields, named findings, and admission readiness. History retains up to 256 accepted snapshots. Integrity shows source, live/replay provenance, connection state, policy compatibility, parser errors, evidence age, reset flags, and freeze reason. LIVE requires two advancing statuses, compatible policy, a connection, and recent progress. It does not mean acquisition is ready or that the hardware identity is authenticated. Duplicate timestamps do not refresh freshness. Contradictory duplicates, counter rollback, disappearing sticky faults, impossible growth, or changing halted evidence freeze the session.

A received boot after evidence freezes the old session rather than silently clearing it. Missing boots can sometimes be inferred from counter or timestamp rollback, but some resets remain indistinguishable without persistent identity. Reception pauses and cumulative host/device progress divergence freeze live evidence to avoid relabelling queued backlog as current. The divergence allowance is 0.5 seconds plus five percent of elapsed host time, with that additional allowance capped at 0.5 seconds. Long uncalibrated HSI sessions can conservatively require reconnect. Initial fixed transport delay cannot be detected from the first snapshot.

Replay is non-live and accepts only regular files no larger than two MiB, checked after nonblocking open. FIFOs and devices cannot silently block the file reader. Framing is bounded, resynchronizes on the complete AP1 marker, and counts a partial tail at EOF as an error. At least one compatible status is required. Export JSON creates a new file with mode 0600, refuses overwrite and final-path symlinks, and records author jayis1, provenance, diagnostics, latest evidence, and bounded history. These permissions do not encrypt evidence or protect a compromised host. Apply the agreed retention and disposal policy.

## Build, verification, and physical acceptance

Required development tools are GNU Make, Arm GNU GCC with newlib/nosys, native GCC, Python 3 with Tk, Xvfb for unattended GUI tests, and KiCad 9 CLI/Python bindings with standard footprints. There are no downloaded firmware SDKs or third-party Python runtime dependencies. From the repository root:

```sh
make -C admission-priority/firmware clean all test
xvfb-run -a /usr/bin/python3 -m unittest discover -s admission-priority/tests -p 'test_*.py' -v
/usr/bin/python3 admission-priority/tools/check_hardware.py
python3 admission-priority/tools/check_release.py
```

The target build produces ignored ELF, binary, and linker-map outputs. Reset vectors start at 0x08000000 and RAM starts at 0x20000000. The linker reserves stack headroom. Host tests execute the production engine and serializer, enumerate stable demand/grant combinations, test nonpreemption, simultaneous indications, direct handover, gaps, timestamp wrap, exhaustion, and a deterministic ten-thousand-cycle independent admission oracle. UART tests model registers in a Linux process. Scheduler tests exercise actual main-loop functions with modelled board I/O.

Companion tests feed synthetic records generated by the real C encoder through the parser and through Linux pseudo-terminals using a real Tk event loop. They check screens, paced live reception, hangup, framing recovery, incompatible or impossible evidence, private export, overwrite refusal, and bounded replay. Synthetic records are explicitly not measured hardware traffic. The CAD checker runs ERC and DRC with violations fatal and compares every reference, pad, net, value, and footprint against a fresh schematic netlist, PCB, and manifest. Clean passive ERC does not validate the external power supply or target voltage.

Before fabrication, review rendered schematics, footprint dimensions, connector orientation, and board geometry. Before connection, inspect the unpowered assembly for shorts and verify every series and ground path. Program the Nucleo alone through a controlled ST-LINK workflow and reset into the image. Check all input bits, serial output, watchdog behavior, nominal timing, power sequencing, and loading with independent instruments. Manufacturer references are ST UM1724 for MB1136 Nucleo-64 boards, RM0368 for STM32F401 peripherals, and the STM32F401xD/xE datasheet. Check the purchased board revision and USART2 virtual COM solder bridges rather than assuming every module is identically configured.

For an authorized product-security trial, establish both grants low, assert both requests, and confirm high admission. Then release grants and intentionally configure the development fixture to select low while high remains pending. Compare the inversion counter with an independent trace. Next admit low while only low demand exists, assert high demand during the held low grant, and confirm no inversion is added. This paired experiment separates selection defects from legitimate nonpreemption. Finally test simultaneous requests/grants and direct handover to document the conservative ambiguity rules.

A red-team report should state the declared arbitration rule, observed violation, diagnostic binding, workload, target revision, observer build, and limitations. A penetration tester can repeat that workload after remediation without making unsupported claims of privilege escalation or data disclosure. Physical fabrication, flash programming, electrical loading, oscillator calibration, GPIO skew, USB behavior, brownout/watchdog recovery, ESD/EMC, enclosure fit, and regulatory suitability remain separate acceptance work. Passing the software and CAD checks does not make this prototype production-ready.
