# Barrier Mask — Tagged Multi-Participant Completion Witness

Author and creator: jayis1. Hardware revision A. Protocol BM1. Firmware and application: GPL-2.0-only. Hardware utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design. This directory supplies target-linked firmware, a Linux desktop companion, and routed native KiCad carrier files. Executed verification is recorded in VERIFICATION.md. Fabrication, programming, electrical qualification, timing calibration, and physical deployment have not been performed. This is an observation instrument, not a barrier controller, isolation device, safety interlock, or certification of secure cleanup.

## Purpose and practical value

Barrier Mask asks whether a shared release really follows completion by every member of a small group. An owner-provided diagnostic adapter exposes a two-bit participant identifier, an ARRIVE strobe, and a RELEASE strobe. The observer collects distinct participant arrivals into a four-bit mask. A release is accepted only after all four identities have arrived in the current round. Repeating one identity cannot substitute for a missing participant, even if a simple arrival counter reaches four.

The motivating security problem is incomplete cross-subsystem cleanup. A development appliance may need four engines to finish clearing private state before returning a shared buffer or maintenance interface to another security domain. A barrier implemented with an ordinary counter can be satisfied accidentally by two notifications from one engine while another engine remains unfinished. Logs might report four completions and one successful release, concealing the identity error. This device preserves the distinction between the number of messages and the membership of the completed set.

The diagnostic adapter must export the real identities of four participants and the release decision being evaluated. Identifiers zero through three are unique within the campaign. They are not hashes of a larger population and are not arbitrary transaction classes. ARRIVE means that this participant has finished its agreed obligation for this round. RELEASE means that the target permits the group to leave the barrier. Those meanings require target-owner review; the observer cannot infer them from wire names.

Useful findings include duplicate participant completion, release with missing participants, overlapping arrival and release indications, unstable participant selection, and a round that never completes before its specified deadline. The instrument does not initiate sanitization, read memory, trigger an exploit, or prevent release. It supplies a narrow external consistency check that can support an engineering investigation with little sensitive data collection.

## Repository-relative originality

The repository directories and device titles were inventoried before selecting this concept. The closest README contracts were read in full. Quorum Witness checks simultaneous two-out-of-three majority levels, not accumulated identities in a rendezvous round. Slot Alias tracks exclusive resource occupancy with independent releases in arbitrary order. Barrier Mask has one collective release, resets membership between rounds, and treats a duplicate completion as insufficient evidence rather than a resource allocation collision. Erase Witness checks one ordered REQUEST/BUSY/DONE lifecycle, not four named completion participants. Grant Ledger accounts for one authorization credit, while Epoch Witness preserves an ordered generation high-water mark.

The passive four-input carrier, STM32 support code, scheduler, CRC framing approach, and desktop serial transport intentionally reuse jayis1's existing platform. The distinct contribution is the tagged completion-set policy, its round and participant conservation rules, the deadline semantics, and the permutation and repeated-round tests. Originality is claimed relative to the reviewed repository, not as a worldwide invention, patentability claim, or novel mathematical set algorithm.

## Legal and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Approval should identify the diagnostic firmware, attachment points, permitted workloads, operator, interruption procedure, and evidence-retention policy. Perform initial testing on an isolated development fixture with hazardous actuators disconnected. Never attach this unisolated prototype to live safety systems, medical equipment, deployed door controls, vehicles, mains-related wiring, or unknown industrial interfaces.

An authorized red team can ask a product owner to exercise a suspected cleanup race through normal laboratory controls while the observer records its diagnostic consequences. A penetration tester can compare the same approved workload before and after remediation. Neither workflow authorizes covert attachment, destructive testing, or access outside the agreed scope. A duplicate diagnostic arrival is a candidate integration defect, not automatic proof of residual secrets or exploitable privilege escalation.

Receive-only firmware does not make electrical attachment harmless. Common ground, input loading, back-power, connector mistakes, and exposed copper can disturb a target. Keep the instrument visible and labelled, insulate the underside, and stop if attachment changes behavior. The author, jayis1, does not guarantee electrical safety, security, regulatory compliance, or manufacturing readiness. Absence of a finding is not evidence that all unobserved operations were safe.

## Attack surface and trust model

The measured boundary is the target's participant-completion bookkeeping versus its collective release decision. Faults may arise from duplicate callbacks, missing notification, wrong identity mapping, stale completion state, premature release, or loss of observation. A malicious target can lie on every diagnostic wire. Four different codes do not prove four independent security authorities, and a completion indication does not prove that memory was actually cleared. Corroborate important findings with source review, independent trace, and functional tests of the actual protected resource.

The operator, harness, MCU image, development board, programming interface, and workstation are trusted. Incoming serial bytes and replay files are parser inputs, not trusted evidence merely because they have valid checksums. CRC detects accidental corruption, not forgery. This design provides no authenticated pairing, signed records, secure bootloader, persistent boot identity, encrypted storage, trusted wall clock, cloud service, radio, or automatic update path.

The four target pins remain inputs in the supplied firmware. The only transmitter is USART2 toward the Nucleo ST-LINK serial bridge. This is not a physical data diode: replacement firmware could drive the input pins, and ST-LINK can replace firmware. Protect programming access and keep the workstation under local control. A disconnected signal can look low through its pulldown; cable loss cannot always be distinguished from legitimate inactivity.

## Hardware specifications and bill of materials

The complete assembly is an external ST NUCLEO-F401RE development board, the supplied passive carrier, labelled insulated leads, nonconductive support, and a Linux workstation. The STM32F401RE Cortex-M4 has 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, one-millisecond SysTick, and 115200 baud USART2 with eight data bits, no parity, and one stop bit. PA2 transmits to the ST-LINK virtual serial connection.

The external Nucleo supplies the MCU, regulation, decoupling, reset, USB conversion, and SWD programming. The carrier is not represented as a complete standalone microcontroller board. It contains no FPGA, radio, antenna, battery, charger, active sensor, level shifter, isolated supply, or storage card. It connects by cable, not as a mechanically compatible stacking shield.

The carrier is 80 by 70 mm, with two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks, a ground-layer transition, and a closed board outline. Its schematic and layout contain fourteen components, thirty electrical pads, and nine named nets. No enclosure CAD or mounting holes are supplied; provide a suitable insulating support and strain relief.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header |
| J2 | 1 | 1x4 vertical 2.54 mm header |
| Harness and support | 1 set | Short labelled insulated leads, nonconductive base |

Resistor footprints use 7.62 mm lead spacing. Verify purchased body dimensions, drill fit, connector orientation, and mating clearance before ordering. Availability, price, assembly yield, and enclosure suitability have not been assessed.

## Wiring and electrical limits

| Function | Target header | MCU header | Nucleo / GPIO | Input bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| ARRIVE | J5.1 | J2.3 | A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only; neither pin supplies power. J2 carries four signals without power or ground. Names ending `_IN` denote target-side nodes, while unsuffixed names denote MCU-side nodes after the series resistor. Check connector numbering and harness continuity rather than inferring compatibility from appearance.

Support is restricted to nominal zero-to-3.3 V, common-ground push-pull diagnostic outputs. Each input presents a nominal 101 kOhm resistance path to ground before leakage and source impedance. The series resistor is not qualified overvoltage or ESD protection. There is no dedicated clamp, isolation, reverse-current blocker, fuse, or hot-plug guarantee. Five-volt signals, negative voltages, 1.8 V logic, differential buses, relay coils, and unknown field wiring are excluded.

Power the Nucleo through its documented ST-LINK USB arrangement. Reserve a conservative 5 V, 500 mA source allowance subject to actual jumper settings and USB limits; this is not measured consumption. USB ground reaches the target through Nucleo ground and J1. Establish compatible ground, power the observer before enabling target signals, and remove signals before depowering the observer. Input protection structures can otherwise back-power the MCU. Ground offset, loading, thresholds, leakage, rise time, temperature, and power sequencing remain physical acceptance tasks.

## Architecture and block diagram

```text
Owned diagnostic adapter        Passive carrier           External Nucleo
TAG0 -------------------------- 1k --+-------------------- PA0
TAG1 -------------------------- 1k --+-------------------- PA1
ARRIVE ------------------------ 1k --+-------------------- PA4
RELEASE ----------------------- 1k --+-------------------- PB0
                                     | each MCU node
                                    100k
                                     |
Common ground ---------------------- GND ---------------- GND / USB

GPIO samples -> independent qualification -> tag setup check
             -> four-participant membership mask -> collective release check
             -> round/participant conserved ledgers -> BM1 CRC status
             -> USART2 PA2 TX -> ST-LINK USB -> Linux companion
                                              Rendezvous / History / Integrity
```

GPIOA is read before GPIOB. These are not simultaneous hardware-latched observations across ports. The device cannot resolve processor-speed races or arbitrarily short pulses. Use deliberately slowed diagnostic signals and an independent logic analyzer to qualify the fixture. Initial experiments should allow at least twenty milliseconds tag setup and twenty milliseconds each of high and low strobe time, leaving comfortable margin above the qualification limits.

## Exact firmware measurement contract

Each input qualifies independently. A candidate change requires its initial observation plus three subsequent unchanged one-millisecond intervals. Chatter on an unrelated channel cannot indefinitely postpone another input's qualification. Shorter pulses may be rejected or missed. Qualified ordering is not proof of physical ordering within a sample interval.

At startup, arrange the real target at a between-round baseline and restart the observer under operator control. After all inputs qualify, twenty evaluated ticks with both raw and qualified ARRIVE and RELEASE low establish READY. Tags need not be zero. Activity before READY is outside the measurement segment, not retrospectively counted. Silence alone cannot prove the target's internal barrier is empty; mid-round attachment is unsupported.

While READY, each qualified ARRIVE rising edge increments arrivals. The qualified tag must have five stable evaluated intervals with raw tag agreement. Any raw tag excursion resets setup even if it never qualifies. Stability is updated before that tick's operation, so the fifth stable interval is inclusive. A well-formed first arrival opens a round, sets active, increments rounds, and starts age at zero. Each unique tag sets one mask bit and increments accepted. A repeated tag records duplicate, abandons the current round, and permanently loses correlation for this run. Holding ARRIVE high never creates repeated arrivals.

Each qualified RELEASE rising edge while READY increments releases. If ARRIVE is raw or qualified high on that observation, collision is recorded before membership can be accepted. The symmetric rule applies to arrival while RELEASE is raw or qualified high. Therefore a last arrival and release in the same qualified observation cannot create a valid barrier. A release with fewer than all four mask bits records early. A release after four distinct arrivals increments completed, clears mask and active, and clears READY until a fresh twenty-tick quiet baseline. The tag value during release is irrelevant.

Between successful rounds, activity during baseline acquisition is outside READY and is not counted. The target adapter must remain between rounds until this quiet interval has elapsed; this interface does not provide a return-ready wire. Pacing is a fixture responsibility. An arrival intentionally sent during that interval may be missed and cause a later early-release finding. This design does not claim continuous coverage of arbitrary unpaced target traffic.

The round deadline is one thousand nominal milliseconds from the first qualified accepted arrival. Age advances on each subsequent observed tick. Release qualified at age one thousand is accepted if all other rules hold. At age one thousand and one, timeout takes precedence over edge processing. Arrivals and releases on that fault tick are still counted, but cannot rescue the expired round. Age retains the final round value outside active state and must not be interpreted as a running timer then. This policy limit is a laboratory contract, not a specification for actual sanitization latency.

Any policy fault permanently clears READY and abandons current membership. Restore the real target baseline and deliberately restart the observer for another run, exporting evidence first. Quiet signals cannot reconstruct a lost round. A sampling delta other than one similarly records a gap, accumulates missed ticks, clears qualification, and abandons active membership. Subsequent levels and acquisition diagnostics remain visible, but policy operations are no longer counted. Repeated calls in one tick are ignored; ordinary unsigned timer wrap is supported.

Counter updates operate on a temporary engine copy. Exhaustion rolls back the entire speculative sample and enters a terminal halted state after abandoning previously committed membership. The overflowing sample is not retained. The UART failure counter saturates independently. The public conservation rules are rounds = completed + abandoned + active, and accepted = four times completed + discarded + population(mask). Discarded counts accepted participant notifications whose round became uncertain, not proven failed cleanup. Completed means a matching diagnostic barrier, not a security verdict.

## Firmware and companion interface

The allocation-free C implementation separates startup, board access, input qualification, barrier policy, encoding, and nonblocking UART service. The transmitter copies a bounded frame, sends one ready byte per poll, and abandons a frame stalled for one hundred milliseconds. Watchdog initialization waits are bounded using the tick clock. Startup failure halts measurement before first status. A stopped timebase and real watchdog recovery still require silicon tests.

BM1 records are ASCII `BM1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection, and no final XOR. Four uppercase hexadecimal digits cover bytes before the asterisk. B carries reset flags once after boot. P carries `4,5,3,20,1000`: participant count, setup, qualification, baseline, and deadline. P repeats approximately every second without clearing evidence; S repeats approximately every hundred milliseconds.

S fields are `now levels valid ready halted faults mask active age rounds arrivals releases accepted completed abandoned discarded duplicate early collision unstable timeouts gaps missed tx_fail`, in that order. All are unsigned 32-bit values. Fault bits are duplicate=1, early=2, timeout=4, collision=8, unstable=16, gap=32, exhaustion=64, watchdog=128. Both encoder and parser reject inconsistent membership, ledgers, bounds, and sticky-fault relationships. These necessary checks do not authenticate the sender or prove every possible historical sequence.

Run `python3 app/console.py` from this directory. The standard-library Tk desktop offers Rendezvous, History, and Integrity screens. Select the exact serial path, usually `/dev/ttyACM0`, and choose New live. There is no probing, pairing, target command, or automatic permission change. Disconnect retains evidence. A successful new connection replaces the model; failed opening preserves it. History retains at most 256 snapshots, not every GPIO edge.

Integrity reports provenance, compatibility, parser errors, reset flags, age, connection state, and freeze reasons. LIVE requires two advancing compatible statuses while connected and expires after one second. It describes telemetry progress, not correct acquisition or barrier readiness. Duplicate timestamps do not renew freshness. Counter rollback, contradictory duplicates, disappearing sticky faults, unexpected boot records, or changed halted evidence freeze acceptance. Long receipt pauses and cumulative host/device clock divergence reject buffered stale traffic. Some resets remain indistinguishable without a persistent boot identity.

Replay accepts regular files of at most two MiB, refusing FIFOs and devices without waiting. The parser resynchronizes on the complete BM1 marker. An unfinished tail records an error, and a replay without compatible status is rejected. Export creates a new mode-0600 JSON file, refuses overwrite and final-path symlinks, and includes author jayis1, provenance, integrity state, latest snapshot, and history. Reports are neither signed nor encrypted. Store them privately under the engagement retention policy; filesystem deletion does not guarantee secure erasure.

## Reproducible checks and physical follow-up

Required tools are GNU Make, Arm GNU GCC with newlib/nosys support, native GCC, Python with Tk, Xvfb for headless GUI tests, and KiCad 9 CLI/Python bindings with standard footprints. No downloaded app package or mobile SDK is needed. Run from this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

The target build produces ELF, binary, and map files under firmware/build for Cortex-M4 Thumb soft-float, flash at 0x08000000, and 96 KiB RAM. The linker reserves at least four KiB stack headroom. Program only through a supervised ST-LINK procedure appropriate to the physical board. No programming or target execution is claimed here. `firmware/build/fixture > synthetic.bm1` creates synthetic production-encoder traffic for replay, not hardware measurements.

Tests exercise every participant order, repeated rounds, duplicate identity, incomplete release, simultaneous and held overlap, setup rejection, exact deadline boundaries, gaps, conservation, counter exhaustion, timer wrap, bounded encoding, UART timeout, and watchdog startup. Application tests drive actual Tk event loops with Linux pseudo-terminals, real C-encoder fixtures, malformed frames, replay limits, freshness, disconnect, and private export. The CAD checker runs ERC and DRC and compares a fresh schematic netlist against actual PCB pads, values, full footprint identifiers, and the manifest.

Before fabrication, visually inspect schematic and layout, check purchased footprints, and define mechanical support. Before attaching valuable hardware, verify continuity, resistor values, shorts, grounds, and Nucleo revision. Exercise each channel from a benign source and compare against calibrated waveform capture. Confirm input-only behavior through reset, loading, thresholds, back-power, USB removal, watchdog operation, and oscillator timing. ESD, EMC, thermal behavior, environmental reliability, and regulatory suitability remain unverified.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136, RM0368 for STM32F401 registers, and DS9716 for STM32F401xD/xE electrical characteristics. This implementation reuses the repository's established module mapping and register support; fresh manufacturer document verification was not performed. Check current matching documents, actual solder bridges, and absolute maximum limits before physical assembly. Passing software and passive-network CAD checks does not establish manufacturing readiness or real security enforcement.
