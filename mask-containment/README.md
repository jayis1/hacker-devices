# Mask Containment — Two-Privilege Least-Authority Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol MC1. Firmware and companion application: GPL-2.0-only. Hardware generation and checking utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design. This directory supplies target-linked firmware, a working Linux desktop companion, and routed native KiCad source. Executed verification is recorded in VERIFICATION.md. Fabrication, MCU programming, timing calibration, electrical qualification, and physical deployment have not been performed. This observer is not a permission controller, isolation barrier, safety interlock, or certificate of secure behavior.

## Purpose and overview

Mask Containment asks whether the effective privileges of a small subsystem remain a subset of its externally declared allowed privileges. Two diagnostic outputs describe the current allowed mask; two other outputs describe the corresponding effective mask. The permissions are independent capabilities, not mutually exclusive owners. Both capabilities can legitimately be allowed and active at once. Each pair has four possible states: neither allowed nor active, allowed but withheld, active without permission, and allowed and active. The instrument accumulates separate occupancy histograms, excess-privilege episodes, withheld-service episodes, and longest observed mismatch runs for each capability.

A practical example is an owned development fixture exposing maintenance-read and maintenance-write permissions. A controller might correctly suppress write after permission withdrawal but accidentally leave read enabled through a recovery path. A single combined access indicator cannot attribute that difference. Mask Containment distinguishes which effective capability exceeds the allowed mask and how often that indication recurs. It can also reveal a polarity or wiring error: an allowed-but-withheld state is diagnostic information, but it is not automatically a security violation. A system is allowed to exercise fewer privileges than it possesses.

The inputs must represent actual permission and actual effective state as faithfully as the target permits. Instrumenting both signals from one intended software variable creates a circular test. Prefer independent diagnostic derivation, documented code paths, and corroborating hardware traces. This device does not inspect memory permissions, identify operating-system users, decode a bus, or prove that protected data was read. It checks a declared indication-level least-authority contract on deliberately slow bench signals.

## Repository-local originality

The existing device directories and README titles were inventoried before selecting this design. The complete nearest contracts of Handoff Audit and Quorum Witness were read, along with Baton Order. Handoff Audit treats two paths as mutually exclusive and measures break-before-make transitions and command/feedback mismatch. Quorum Witness combines three votes into a single majority decision. Baton Order measures cyclic transitions between four owners. Mask Containment instead permits both capabilities simultaneously, evaluates each allowed/effective pair independently, treats withheld service as nonviolating, and supplies state occupancy with per-direction episode evidence.

This distinction matters in capability systems: an allowed mask of binary 11 with an effective mask of binary 01 satisfies containment, despite unequal masks. A comparator that treats all disagreement as a security failure is inappropriate. Likewise, effective binary 11 is valid if allowed is binary 11; it is not an ownership collision. The implementation deliberately reuses jayis1-authored STM32 startup, register definitions, passive carrier topology, bounded UART driver, and desktop serial infrastructure. Those foundations are not represented as newly invented circuitry. The new contribution is the containment contract, independent-pair accounting, evidence validation, companion presentation, and associated tests. Novelty is relative to this repository inventory, not a global prior-art search or patentability claim.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written permission to assess. Authorization should cover diagnostic firmware changes, exact connection points, grounding, workloads, retention, responsible operators, and stop conditions. Begin on an isolated development bench with consequential outputs disconnected. Do not attach this unisolated prototype to medical devices, occupied access-control installations, vehicle controls, emergency systems, mains circuits, machinery, or unknown-voltage field wiring. Stop immediately if attachment changes target behavior.

The investigated attack surface is the integration boundary between an allowed capability mask and an effective privilege implementation. Possible defects include stale cached permissions, an incorrect bit shift, a reset path retaining one effective capability, and partial revocation of a two-bit policy. An authorized red team can exercise approved workflows through the target's normal test interface and use the observer to attribute an inconsistent indication. The instrument itself generates no requests, alters no mask, injects no packets, and implements no exploitation, credential collection, covert deployment, persistence, or exfiltration capability.

The target can falsify its diagnostic outputs. The workstation can alter reports. Anyone with physical ST-LINK access can replace firmware. Serial CRC detects accidental corruption, not malicious fabrication. There is no secure element, cryptographic attestation, authenticated pairing, persistent boot identity, encrypted evidence storage, signed update mechanism, or anti-rollback guarantee. Physically control the observer and workstation, record image provenance, and corroborate important findings with independent measurements.

Target GPIOs are inputs in the supplied firmware, but this is not irreversible electrical receive-only enforcement: replacement firmware could configure them as outputs. Common ground, incorrect wiring, input loading, shorts, and back-powering can disturb a target without any intentional command. The app treats incoming bytes and recordings as untrusted, bounds allocations and integer values, and checks internal consistency. These controls cannot authenticate a coherent fabricated stream. The author jayis1 provides no safety certification, production-readiness guarantee, or authorization to exceed the owner's approved scope.

## Hardware specifications

The processor is an external ST NUCLEO-F401RE development board from the MB1136 family, containing the STM32F401RET6 Cortex-M4, 512 KiB flash, and 96 KiB SRAM. Firmware starts from reset using the nominal 16 MHz internal HSI oscillator, a one-millisecond SysTick, and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. PA2 routes telemetry to the module's ST-LINK virtual COM bridge. USART reception is disabled. The Linux workstation provides the display and evidence storage.

The custom PCB is a passive, harness-connected carrier, not a mechanically stackable Arduino shield. It is two-layer, 80 by 70 mm, nominally 1.6 mm thick, with 0.3 mm tracks and a closed board outline. The Nucleo supplies the MCU, regulator, decoupling, reset, programmer, and USB bridge as purchased-module functions. The carrier has no FPGA, radio, antenna, battery, charger, active level translator, analog sensor, memory card, actuator, or target power rail. An insulating support and labelled short leads are required; mounting holes and an enclosure CAD model are not supplied.

### Bill of materials

| Reference | Quantity | Specification and example purchasing candidate |
| --- | --- | --- |
| External processor board | 1 | ST NUCLEO-F401RE, verify MB1136 revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial; Yageo MFR-25FBF52-100K |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial; Yageo MFR-25FBF52-1K |
| J1, J3, J4, J5, J6 | 5 | Straight 1x2 2.54 mm header; Samtec TSW-102-07-G-S |
| J2 | 1 | Straight 1x4 2.54 mm header; Samtec TSW-104-07-G-S |
| Harness and support | 1 set | Insulating fixture, labelled leads, strain relief |

The PCB has fourteen components, thirty electrical pads, and nine nets. Resistor footprints are DIN0207 with 7.62 mm lead spacing. Part candidates are not availability guarantees. Review supplier drawings, plating, lead diameters, and assembly access before ordering. No connector is keyed; continuity checks and labelling are mandatory.

### Connections and electrical limits

| Logical signal | Target connector | MCU-side connector | Nucleo label / MCU pin | Raw bit |
| --- | --- | --- | --- | --- |
| ALLOW0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| ALLOW1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| EFFECT0 | J5.1 | J2.3 | A2 / PA4 | 2 |
| EFFECT1 | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1, both pins | GND | none |

J1 is ground only, not a power connector. J2 carries four signals with no ground pin. Each target signal passes through a 1 kOhm series resistor to an MCU-side node, which has a 100 kOhm pull-down to ground. Net names ending `_IN` are target-side nodes. Plain ALLOW/EFFECT names are MCU-side nodes. The pull-down makes an unplugged signal appear low; it cannot prove cable continuity or distinguish a missing input from a legitimate zero.

Only common-ground, nominal zero-to-3.3 V push-pull diagnostic outputs are supported. Five-volt signals, 1.8 V logic, negative voltages, differential pairs, energized switch contacts, raw relay coils, and unknown industrial interfaces are excluded. There is no galvanic isolation, active buffering, surge protection, dedicated ESD clamp, reverse-current blocker, or hot-plug guarantee. Series resistance is not permission to exceed MCU limits. Never join separately isolated or hazardous grounds through this board.

At a nominal 3.3 V high, the series/pull-down path is 101 kOhm and draws approximately 32.67 microamps per channel before leakage and transient effects. This is a circuit estimate, not a measured loading specification. Reserve a conservative 5 V, 500 mA USB source allowance for the complete development-board assembly while respecting actual board jumpers and USB limits. Consumption and thermal behavior remain unmeasured. Power the observer before enabling target signals, then remove target signals before depowering the observer to avoid back-power through MCU protection structures.

ST UM1724 identifies the Nucleo A0–A3 pin mapping and USART bridge arrangement. The usual ST-LINK serial path uses SB13/SB14 on and SB62/SB63 off; verify the actual board revision and solder bridges. STM32F401 RM0368 defines the peripheral registers. The STM32F401xD/xE datasheet defines electrical limits and input thresholds. These references must be checked against purchased hardware; successful software tests do not validate board modifications or a different MCU.

## Architecture and block diagram

```text
Owned diagnostic target             Passive carrier              External Nucleo
 ALLOW0 -- J3.1 ------------------- 1k --+-- J2.1 -------------- PA0 / A0
 ALLOW1 -- J4.1 ------------------- 1k --+-- J2.2 -------------- PA1 / A1
 EFFECT0 - J5.1 ------------------- 1k --+-- J2.3 -------------- PA4 / A2
 EFFECT1 - J6.1 ------------------- 1k --+-- J2.4 -------------- PB0 / A3
                                       | each MCU-side node
                                      100k
                                       |
 common ground -- J3..J6.2 ----------- GND -- J1 --------------- GND / USB

GPIOA then GPIOB -> independent per-input qualification
  -> pair validity -> allowed/effective four-state classification
  -> independent histograms + excess/withheld episodes and run maxima
  -> transactional update -> checked MC1 snapshot -> bounded USART2 TX
  -> ST-LINK USB serial -> Ledger / History / Integrity screens
  -> explicit private JSON evidence export
```

GPIOA and GPIOB are read consecutively, not latched simultaneously. Relative port skew, interrupt latency, qualification delay, and oscillator error can change apparent ordering. Begin with diagnostic plateaus of at least twenty milliseconds and compare the complete chain against a logic analyzer. This is not a high-speed memory-bus monitor or metastability detector. Short electrical privilege glitches can be missed even when every reported sample is consistent.

## Firmware contract and design decisions

Production firmware is C11 with explicit startup vectors, linker layout, board/register definitions, GPIO and watchdog drivers, bounded serial transmission, a containment engine, and checked framing. It uses no heap, operating system, filesystem, flash journal, or received-command parser. Flash begins at 0x08000000. The linker reserves stack headroom. Firmware requires reset entry and does not normalize an arbitrary bootloader clock configuration. Evidence is volatile and is lost on observer reset.

Each input independently qualifies after its candidate level remains unchanged for three subsequent one-millisecond observations. Thus the first candidate sample plus three matching intervals establish a level. Until a changed candidate qualifies, the previously qualified level remains visible. A brief sampled excursion can be rejected without closing an episode. Qualification is independent so chatter on ALLOW1 cannot indefinitely conceal an established ALLOW0/EFFECT0 failure. Pair validity requires both members of that pair, not all four inputs. Invalid pairs contribute no histogram samples and have no current episode.

For each valid pair the state index is allowed plus twice effective. State zero means neither allowed nor active; one means allowed but withheld; two means effective without allowance; three means allowed and effective. Exactly one bin increments per valid pair per accepted sample. Coverage for a pair is the sum of its four bins. Coverage for two pairs must not be added and called wall time because both can be observed simultaneously, or have different qualification coverage. The counters count observations, not exact continuous electrical milliseconds. The first qualified observation counts immediately; physical interval endpoints are not inferred.

Excess and denied are two-bit current masks. Excess sets a sticky finding for the associated capability. Denied records diagnostic withholding but does not set a security fault. A transition into either mismatch starts that direction's episode and increments its count once. Every matching observation increments the current streak and updates its historical maximum. Leaving that mismatch clears only its current streak. Direction is independent for each capability: a healthy pair changing state cannot reset the other pair's prolonged excess episode. No arbitrary grace window forgives excess privilege in this prototype contract.

Repeated calls at the same timestamp do nothing. A nonzero timestamp delta other than one records an acquisition gap and the skipped nominal ticks. It invalidates qualification, closes current streaks, preserves historical bins/counts/maxima, and starts fresh candidate qualification. A mismatch still present after recovery becomes another observed segment. Therefore episode counts are not necessarily distinct physical incidents. Ordinary 32-bit timestamp wrap works by unsigned subtraction; an entirely unseen full timer cycle is not detectable from the modulo clock alone.

All changes for a sample are speculative until every counter update succeeds. Any overflow discards the entire sample and enters terminal halt with exhaustion flagged. Historical counts stay mutually consistent; qualification and active streaks clear. Watchdog initialization failure also enters halt before first telemetry. In halt, timestamp and saturating transmission-failure diagnostics may advance while acquisition evidence remains frozen. The serializer checks state masks, gap accounting, episode/bin relationships, current streaks, maxima, sticky findings, and halt consistency before sending status.

The independent watchdog uses the MCU's internal low-speed oscillator. Readiness waits are bounded and the main loop feeds the watchdog only after servicing acquisition and telemetry. Physical watchdog period and reset recovery require silicon testing. A reset-flags boot record is not a unique boot identity. No firmware update is accepted over this protocol; programming requires supervised ST-LINK access and hardware reset.

## Wire protocol and application

The main loop samples before servicing UART. Each UART service transfers at most one ready byte, avoiding an unbounded wait. Frame submission copies into fixed storage. A transfer unfinished after one hundred milliseconds is abandoned and counted. Boot is announced once; policy repeats approximately every second; cumulative status is offered approximately every hundred milliseconds. Status is not an event-complete trace, so brief transitions may only be visible in cumulative counts.

Records are ASCII `MC1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE covers bytes preceding the asterisk, using polynomial 0x1021, initial 0xffff, no reflection, and no final XOR. Four uppercase hexadecimal digits encode the checksum. B contains the MCU reset-register snapshot. P contains `3,1,2,4`: qualification intervals, nominal sample period, number of independent pairs, and states per pair. A different policy is unsupported rather than silently interpreted.

S contains, in exact order: now, levels, valid, halted, faults, gaps, missed, excess, denied; eight bins b0 through b7; four episode counts e0 through e3; four current streaks c0 through c3; four maxima x0 through x3; tx_fail. Every field is unsigned 32-bit. Bins zero through three belong to capability zero and four through seven to capability one. Episode/streak/maximum indices zero and one describe excess capabilities zero and one; indices two and three describe withheld capabilities zero and one. Fault bits zero through four mean excess zero, excess one, acquisition gap, counter exhaustion, and watchdog unavailable.

Run `python3 app/console.py` on Linux with Tk installed. Enter an explicit serial path such as `/dev/ttyACM0` and choose New live. Automatic discovery and authenticated pairing are intentionally absent; the app never elevates privileges or sends serial commands. Ledger shows the containment histograms, qualification, counters, and named findings. History retains up to 256 accepted snapshots. Integrity shows source, live/replay provenance, compatibility, connection, evidence age, parser errors, reset flags, and any freeze reason. Disconnect retains evidence. Export before replacing a session; failed connection setup preserves the previous model.

LIVE requires a connection, supported policy, at least two advancing timestamps, and recent progress. It means progressing transport, not valid acquisition or target security. Duplicate timestamps cannot renew freshness. Contradictory duplicates, backwards timestamps, decreasing cumulative counters, disappearing findings, impossible growth, and changes after halt freeze evidence. Pair-bin growth cannot exceed elapsed device time; episode and maximum growth cannot exceed the corresponding mismatch observations. These are necessary consistency bounds, not a proof of complete stream realizability or authenticity.

A reception pause exceeding one second requires explicit reconnect, rather than trusting queued packets. Cumulative host/device divergence is bounded by half a second plus five percent of elapsed host time capped at another half second. The total allowance therefore never grows beyond one second. Honest HSI drift can eventually require reconnect; the app cannot distinguish oscillator error from buffering or detect a fixed initial transport delay. Some unannounced resets remain indistinguishable if the first post-reset values look like forward progress.

Replay is always labelled non-live. Only regular files of at most two MiB are accepted, checked on a nonblocking opened descriptor before allocation. FIFO and device paths cannot stall the replay loader. Framing storage is bounded and resynchronizes on the complete MC1 marker. EOF with a partial tail records an error; a file with no compatible status is rejected. The included fixture comes from the actual C engine and encoder, but is explicitly synthetic software evidence, not a physical capture.

JSON export creates a new file with mode 0600 and refuses overwrite or a final-path symlink. It includes author jayis1, provenance, connection/freshness, parser errors, reset information, freeze reason, latest snapshot, and bounded history. Empty reports still retain metadata. Private permissions do not encrypt evidence. Choose a trusted directory and follow the engagement retention policy; ordinary deletion does not guarantee secure erasure.

## Build and verification procedure

Required tools are GNU Make, ARM GNU GCC with newlib/nosys support, native GCC, Python 3 with Tk, KiCad 9 CLI and Python bindings with standard footprints, and Xvfb for unattended GUI tests. No external Python packages, mobile SDK, cloud service, or runtime network dependency are needed. From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

The target build produces firmware/build/mask-containment.elf and mask-containment.bin. Host tests exercise all 256 transitions between four-input vectors, independent-channel chatter, gaps, rejected glitches, timestamp wrap, transactional exhaustion, CRC, and a deterministic 10,000-plateau independent histogram oracle. Serial tests model registers in a Linux process. Scheduler tests execute production main-loop functions with modeled peripherals, including lost boot transmission and watchdog initialization failure. None of these are physical MCU measurements.

Application tests consume actual C-encoded fixtures through the parser and Linux pseudo-terminals using the real Tk event loop. They exercise screens, live progress, disconnect, retained provenance, replay, malformed framing, impossible valid-CRC records, rollback, delayed transport, private export, overwrite refusal, and bounded file handling. Hardware checks run ERC and DRC with violations fatal and compare every component, value, footprint identity, pin, and net across fresh schematic XML, saved netlist, PCB pads, and the manifest. Passive-symbol ERC cannot validate external supply safety or target voltage.

## Research workflows and remaining acceptance

For a vendor review, document the two capability meanings and why effective must remain a subset of allowed. Establish a known all-low state, exercise each allowed capability without activation, then exercise allowed activation and simultaneous allowed activation. These are legitimate vectors, not faults. Use approved target test firmware to expose a controlled excess indication on a sacrificial fixture, confirm correct attribution, and compare the export with an independent trace. A withheld episode is not evidence of privilege escalation.

For penetration-testing reports, retain authorization, target firmware identity, adapter source, observer revision, wiring, acquisition gaps, and sampling limitations. Compare baseline and remediated builds under identical paced workloads. Excess counts identify an indication-level violation worth investigating, not proof of sensitive-data access. A quiet run only describes the observed workload and capture envelope. Avoid claims about unobserved privilege paths or the authenticity of target diagnostics.

Before fabrication, inspect rendered schematic labels and board routing, verify purchased footprint dimensions, and design insulating support and strain relief. After assembly, check shorts, continuity, resistor values, ground offset, input margins, loading, and power sequencing. Characterize GPIO read skew, actual sample period, minimum detectable pulse, qualification latency, watchdog recovery, USB interruption, and resets with held inputs. Back-power, ESD, EMC, thermal limits, environmental reliability, regulatory suitability, and calibration remain unverified. Passing source builds and CAD checks does not replace physical qualification. Original design, firmware, application, tests, and documentation author: jayis1.
