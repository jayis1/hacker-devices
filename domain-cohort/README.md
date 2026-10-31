# Domain Cohort — Homogeneous Security-Domain Residency Witness

Author and creator: jayis1. Hardware revision A. Protocol DC1.
Firmware, companion and tests: GPL-2.0-only. Hardware generation and checking utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. This release provides a self-contained target firmware build, Linux desktop companion, and routed native KiCad carrier design. VERIFICATION.md distinguishes software and CAD evidence from physical validation. No fabricated board, programmed MCU, electrical qualification, calibrated timing, or production approval is claimed. This is an observation instrument, not an isolation mechanism or safety interlock.

## Purpose and useful distinction

Domain Cohort checks whether a two-slot execution pool keeps simultaneously resident work inside a single security domain. An owner-controlled diagnostic adapter reports a slot bit, a domain bit, an allocation strobe and a release strobe. Both slots may operate concurrently when their domain labels agree. A different domain may enter only after the entire incumbent pool drains. Releases must name the domain recorded at allocation. This provides concrete evidence for an isolation policy that is neither simple mutual exclusion nor independent per-slot ownership.

Consider a security appliance that batches cryptographic work from two trust partitions through two descriptors sharing scratch storage. The architecture permits two jobs from one partition to run together, but promises that jobs from different partitions are never resident together. A cancellation path could free one descriptor while another remains active, then incorrectly admit work from the other partition into the free descriptor. A per-slot duplicate detector sees two different legal indices and misses the isolation violation. A blanket overlap alarm incorrectly condemns legitimate same-domain parallelism. Domain Cohort separates the two cases without collecting protected job contents.

For example, allocate slot zero to domain one, allocate slot one to domain one, release slot zero as domain one, then allocate slot zero to domain zero. The first two allocations are legitimate sharing. The final allocation is a mixed-domain admission, despite using a vacant slot. The observer retains slot one's incumbent domain and does not accept the conflicting allocation into its ledger. Releasing the last domain-one slot before allocating domain zero instead creates a legal cohort boundary. No alternating-domain requirement exists: another domain-one cohort is equally legal.

A domain is a security label, not a unique client. Many callers may share a label if the owner confirms that they belong to the same isolation partition. The instrument cannot distinguish those callers, detect two jobs incorrectly mapped to one slot by a dishonest adapter, or prove that a released slot's residual data was erased. It checks co-residency, not sanitization, confidentiality, fairness, or absence of microarchitectural channels after a drain.

## Originality and intentional reuse

The configured repository main branch was pulled and device directories inventoried before selection. README titles and matching domain/cohort descriptions were searched, and the close policies were read. Owner Bind requires different clients in simultaneously occupied slots under a one-resource-per-client contract. This device instead permits same-domain parallelism and rejects different-domain co-residency. Exclusion Ledger rejects any pair of domain grants and does not reconstruct labelled reusable slots. Read Share has asymmetric reader and writer classes; Domain Cohort has two symmetric domains, each allowed to share internally. Slot Generation tests generation toggling rather than security partition compatibility.

The contribution is the homogeneous-domain admission contract, explicit full-drain handover rule, labelled retained residency, sparse-snapshot drain constraint, and corresponding regression workflow. These are repository-relative distinctions, not a worldwide novelty or patentability claim. Domain batching and partition isolation are established engineering concepts. The electrical carrier, STM32 register layer, startup, bounded serial transmitter, CRC encoder, and desktop transport reuse jayis1-authored Owner Bind platform sources. Internal OB_ constants and some client variable names remain platform names; here client always means domain. This directory does not import sibling code at build or runtime. Standard KiCad footprint geometry remains upstream material rather than a claim of original authorship.

## Authorized use, attack surface and trust

Use only on equipment you own or have explicit written authorization to assess. The agreement should cover internal attachment, diagnostic firmware, target workloads, interruption, common-ground wiring, collection limits, and recovery. Permission to test a network application does not automatically authorize opening an appliance or wiring internal signals. Start on an isolated low-energy development fixture with hazardous outputs disconnected, a visible labelled harness, and a documented stop procedure. Never attach this unisolated prototype to production access controls, medical equipment, vehicles, machinery, mains-related wiring, or unidentified field voltages.

The assessed attack surface is admission and cleanup at a shared execution-pool boundary. Relevant defects include cross-partition admissions before outstanding work drains, wrong-domain completion callbacks, duplicate allocation, orphan release, and diagnostic identity changes at operation endpoints. A researcher can compare normal, timeout and cancellation paths. An authorized red team can ask the owner to exercise an approved failure workload while retaining an independent observation. A penetration tester can compare pre-fix and post-fix builds using the same adapter and baseline. The instrument does not generate exploit traffic, induce faults, inject commands, capture credentials, persist on targets, or conceal its presence.

The diagnostic adapter must report effective allocation and accepted release rather than requests that the target correctly rejected. Release must mean the work is no longer resident in the relevant shared execution context. If it merely means that a callback was queued while DMA continues, the measured policy is not the intended security property. Review adapter source with the owner and corroborate its events against functional logs and a logic analyzer. A dishonest target can lie on every wire. Absence of findings cannot establish security, completeness, or correct internal buffer use.

Trusted components include the operator, target adapter, harness, Nucleo module, supplied image, programming interface, and workstation. Serial input and replay files receive bounded parsing but are not authenticated. CRC detects accidental corruption; it does not sign evidence or prevent intentional forgery. There is no secure boot, authenticated update service, persistent boot identifier, trusted wall clock, encrypted storage, network listener, cloud account, or remote management channel. An attacker with SWD access can replace this image and drive nominal input pins. Receive-only therefore describes supplied firmware behavior, not an irreversible physical diode. Even with the correct image, loading, miswiring, shorts and back-power can disturb a target.

## Hardware, power and connectivity

The assembly uses an external ST NUCLEO-F401RE development board, the passive carrier in kicad/, short insulated leads, an insulating support and a Linux workstation. Its STM32F401RET6 Cortex-M4 supplies 512 KiB flash and 96 KiB SRAM. Firmware uses nominal reset-default 16 MHz HSI, one-millisecond SysTick and USART2 transmission at 115200 baud, eight data bits, no parity and one stop bit. PA2 sends telemetry to ST-LINK's virtual serial bridge; the USART receiver is disabled. No radio, FPGA, analog sensor, battery, charger, storage card, antenna or target power output is included.

The Nucleo provides regulation, MCU decoupling, reset, boot configuration, USB connectivity and SWD. Those are external module facilities, not fictional components on the passive carrier. The custom board is cable-connected, not a mechanically stacking shield. It measures 80 by 70 mm with two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks and a closed outline. Fourteen through-hole components provide thirty electrical pads on nine named nets. No mounting holes or enclosure CAD is supplied; use insulated supports, strain relief and a cover preventing underside contact.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, purchased revision checked before wiring |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header |
| J2 | 1 | 1x4 vertical 2.54 mm header |
| Harness and support | 1 set | Short labelled insulated leads and nonconductive support |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Check purchased dimensions, lead diameter, drill fit, connector mating clearance and environmental ratings. Price, availability and yield are not verified. The schematic represents the external module interface as connectors, not as an inaccurately reduced MCU symbol.

| Signal | Target pin | J2 pin | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| SLOT | J3.1 | 1 | CN8.1 / A0 / PA0 | 0 |
| DOMAIN | J4.1 | 2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | 3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | 4 | CN8.4 / A3 / PB0 | 3 |

Target header pin two is ground. Both J1 contacts are ground only, never power inputs. J2 has four signal contacts and no ground or supply contact. A separate Nucleo ground wire goes to J1. Names ending _IN identify the target side of each series resistor; unsuffixed names identify MCU-side nodes. Verify continuity and pin-one orientation instead of relying on wire colors.

Only known common-ground zero-to-3.3 V push-pull diagnostic logic is supported. Every signal traverses 1 kOhm into a node with a 100 kOhm pulldown. The nominal path to ground is 101 kOhm before source impedance and leakage. This is not a qualified voltage-protection network. There is no dedicated ESD clamp, galvanic isolation, reverse-current barrier, fuse, level shifter or hot-plug guarantee. Five-volt signals, negative voltages, differential buses, unknown wiring and 1.8 V logic are unsupported.

Power through the documented Nucleo ST-LINK USB connection after checking the purchased revision's jumpers and solder bridges. Use 5 V at 500 mA as a conservative source-planning allowance, not a measured consumption or USB-compliance claim. Target and USB grounds are connected. Power the observer before enabling target outputs; disable target outputs before removing observer power. Otherwise input structures may back-power the module. References for physical acceptance are ST UM1724 for the MB1136 board, RM0368 for registers, and the STM32F401RE datasheet for electrical limits. The reused connection map still requires verification against the actual purchased module before assembly.

## Architecture

```text
Owned adapter                      Passive carrier                Nucleo-F401RE
SLOT ------------------------------ 1k ---+------------------------ PA0
DOMAIN ---------------------------- 1k ---+------------------------ PA1
ALLOCATE -------------------------- 1k ---+------------------------ PA4
RELEASE --------------------------- 1k ---+------------------------ PB0
                                          | each node
                                         100k
                                          |
Target GND ------------------------------ GND -------------------- USB GND

GPIO -> continuity guard -> identity bundle / independent strobes
     -> prior setup validation -> homogeneous domain pool ledger
     -> transactional counter commit -> DC1 CRC status -> bounded TX
     -> ST-LINK virtual serial -> desktop Ledger / History / Integrity
```

GPIOA and GPIOB are read separately, without a cross-port atomic latch. This is a slow diagnostic instrument, not a processor-speed analyzer. Begin with twenty milliseconds identity setup, twenty milliseconds strobe high, and twenty milliseconds both strobes low per operation. Verify waveforms independently before reducing these fixture intervals. Sampling qualification does not establish physical edge ordering inside one sample interval, propagation delay, interrupt jitter or oscillator calibration.

## Firmware measurement contract

Independently establish an empty target pool before starting the observer. WAIT transitions to TRACK after every input qualifies and both raw and qualified strobes are low. Quiet strobes cannot prove an empty target, so attaching at an unknown active state is outside the contract. Late desktop attachment to a continuously running observer is different: retained ledger state and periodic policy announcements support that workflow.

SLOT and DOMAIN qualify as a two-bit bundle; ALLOCATE and RELEASE qualify independently so identity chatter cannot postpone an operation indefinitely. A new candidate needs three subsequent matching sample intervals. Five prior stable identity observations are required before an accepted operation. Setup cannot be earned on the operation endpoint. Raw identity must equal qualified identity, and the opposite strobe must be low in previous qualified, current qualified and raw observations. Simultaneous rising strobes are rejected together. Short pulses may be missed; this design makes no exhaustive glitch-detection claim.

An allocation first checks whether its slot is already occupied. If vacant, it checks the other slot: an active other slot must have the same domain. Successful allocation stores the domain and sets occupancy. A release first checks that its slot is occupied, then requires the stored domain to match. Only a matching release clears occupancy. Duplicate and orphan errors take precedence over domain mismatch. Historical labels on vacant slots impose no ban. Either slot may release first; a released slot may rejoin the existing domain while its peer stays active. Only the final release closes the nonempty cohort.

Every sample operates on a speculative ledger copy. Counter exhaustion discards the entire sample update, retaining prior accepted residency. Any skipped acquisition tick permanently halts correlation rather than silently resetting qualification. Duplicate calls at the same timestamp do nothing; ordinary unsigned timestamp wrap works. A halt freezes residency and accounting while device time and the saturating transport-failure count may advance. There is no remote clear or automatic recovery. Export evidence, establish a new empty target baseline, then deliberately restart the observer.

The conservation laws are allocations = releases + occupied slots, attempts = allocations + releases + rejected edges, and allocations = closed cohorts + active-cohort indicator + overlapping allocations. Peak occupancy cannot exceed two. The legacy order field packs last-domain bits for slots zero and one in bits zero and one, and ever-allocated membership in bits two and three. Unseen slots have zero stored labels. Both active slots must have identical domains; historical vacant labels may differ. These invariants are checked before encoding and again in the companion.

engine.c implements the policy. wire.c validates and encodes snapshots. drivers/board.c configures input GPIO, SysTick, reset-cause capture, USART and bounded watchdog startup. drivers/serial.c provides bounded nonblocking transport. startup.c initializes data and BSS and supplies the reset vector. board.h, registers.h and linker.ld describe the actual STM32 memory and register interfaces. No heap, operating system, target command parser or downloaded SDK is required.

## Application and wire format

DC1 frames are ASCII: `DC1,<kind>,<decimal fields>*<four uppercase CRC digits>\n`. CRC-16/CCITT-FALSE covers bytes before the asterisk, polynomial 0x1021, initial 0xffff, without reflection or final xor. B contains reset flags. P contains `3,1,5,2`: qualification intervals, nominal sample milliseconds, setup observations and slot capacity. P repeats about once per second independently of the one-shot B record, permitting late attachment without treating compatibility announcements as resets.

S fields are `now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail`. Launched means allocations; completed means releases; rounds means closed nonempty cohorts; queued means allocations accepted while the other slot remained live. Queued is sharing coverage, not a FIFO or time measurement. Fault bits are 1 domain mismatch or mixed admission, 2 orphan release, 4 duplicate slot, 8 interface ambiguity, 16 acquisition gap, 32 counter exhaustion and 64 watchdog startup failure. Only the first terminal reason is retained.

Status is attempted every hundred nominal milliseconds when transmission storage is available. A stalled frame expires after a hundred milliseconds instead of blocking acquisition. Failed submissions and timeouts count toward tx_fail. Status is a sparse snapshot, not a complete operation log. Terminal input levels describe the finding endpoint, not present electrical voltages. Firmware receives no host commands.

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk. There are no third-party Python dependencies. Enter the exact virtual serial path and choose New live. The application configures read-only 115200 8N1, refuses duplicate connections, restores terminal attributes when possible, and detects hangup. Ledger shows labelled residency, counters and findings. History retains up to 256 snapshots. Integrity shows provenance, compatibility, parser errors, reset evidence and freeze reason. Replay loads a regular file of at most two MiB and remains explicitly non-live.

Two advancing device timestamps are required before displaying live progress. Duplicate timestamps do not renew freshness. Long reception pauses or excessive capped host/device drift freeze interpretation rather than presenting a growing backlog as current. Counter rollback, disappearing findings, changed halted evidence or later boot records also freeze it. Without persistent boot identity, some resets whose boot records are lost and whose counters move plausibly forward remain indistinguishable. CRC and plausibility checks are not authentication.

Sparse transition checks require enough allocations and releases to explain changed occupancy or labels. If both endpoints have residents but their domains differ, at least one cohort must have closed between them. This rejects individually valid frames claiming a rolling domain switch without a drain. These checks are necessary bounds, not exhaustive reconstruction of hidden operations. Export creates a new mode-0600 JSON file, refuses overwriting and final-component symlinks, and includes author, provenance, parser diagnostics, freshness, latest state and retained history. Exports are unsigned and unencrypted; use trusted directories and agreed retention, and do not describe ordinary deletion as secure erasure.

## Build, regression and physical acceptance

Run `make -C firmware all test` from this directory. The target is STM32F401RE using Cortex-M4 Thumb soft-float code, GNU Arm Embedded GCC and newlib/nosys. This release is checked with Arm GCC 14.2.1, host GCC 14.2.0, Python 3.13.5 and KiCad 9.0.2. There is no SDK version to fetch. Artifacts are firmware/build/domain-cohort.elf, .bin and .map. Tests do not flash hardware. Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` for application and real Tk event-loop tests through Linux pseudoterminals. Run `python3 -m compileall -q app` for byte compilation.

The C tests enumerate operation words against an independent vacant-or-domain array oracle, including both domains, slots, allocation and release. Regressions cover same-domain sharing, reentry without drain, drain-before-switch, wrong-domain release, duplicate slots, orphan release, simultaneous strobes, raw opposite-strobe ambiguity, chatter, missed ticks, wrap and transactional exhaustion. Fixtures come from the real C encoder but are synthetic software evidence, never physical measurements. Companion tests include checksum-valid impossible states and an impossible domain switch whose individual snapshots otherwise satisfy conservation.

Generate native CAD using `/usr/bin/python3 tools/generate_hardware.py`, then validate with `/usr/bin/python3 tools/check_hardware.py`. The matching KiCad pcbnew module and standard footprint libraries are required. The checker executes ERC and DRC, exports a fresh schematic netlist, and compares references, values, footprint identities and every pin/net with PCB pads and the manifest. Passive-only ERC cannot approve the external module, electrical limits or physical safety. Run `python3 tools/check_release.py` for word count, substantive production C count, format-valid authorship and scope checks.

Before physical use, inspect resistors, continuity, ground offsets, connectors, leakage and power sequencing. Establish an empty target, power the observer, then enable diagnostic outputs. Exercise same-domain parallelism in both domains, both release orders, released-slot reentry, and a full-drain domain change. On an approved fixture separately introduce cross-domain admission and wrong-domain release, checking retained incumbent evidence. Reset target baseline between terminal fault tests. Compare with independent waveforms and functional logs. Fabrication, programming, ST-LINK routing, watchdog recovery, sustained serial load, timing calibration, ESD behavior and target loading remain laboratory acceptance gates, not outcomes inferred from compilation or CAD checks.
