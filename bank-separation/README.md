# Bank Separation — Nontransitive Resource Co-Residency Witness

Author and creator: jayis1. Hardware revision A. Wire protocol BS1.
Firmware, application and tests: GPL-2.0-only. CAD generation and validation utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. External tools and component libraries retain their own licenses; their authorship is not claimed here.

Status: unbuilt engineering prototype. This release contains target-linked firmware, a desktop companion, and routed native KiCad files. Executed checks are recorded in VERIFICATION.md. No physical device has been fabricated, programmed, attached, or electrically qualified. Bank Separation is an observer of a declared diagnostic contract, not a hardware partitioner, safety interlock, or proof that a target prevents information leakage.

## Purpose and practical value

Bank Separation investigates resource isolation policies that cannot be expressed as either complete mutual exclusion or simple security-domain equality. Four resource identities have a fixed conflict relationship: zero conflicts with one, one conflicts with two, and two conflicts with three. Every other pair of different identities may overlap. The instrument reconstructs occupancy from tagged admission and release events and stops when an admission would place a forbidden pair in the active set. Unlike a universal overlap detector, it accepts legitimate concurrency. Unlike a capacity monitor, it distinguishes different pairs having exactly the same population.

A motivating laboratory model is an embedded accelerator with three sensitive scratch banks, called A, B and C. Resource zero uses A; resource one uses A and B; resource two uses B and C; resource three uses C. A product security policy requires jobs sharing a scratch bank to execute without concurrent residency, while disjoint jobs may overlap. This mapping yields the path-shaped conflict graph exactly. It represents a reduced owner-controlled reproduction, not a claim about any commercial accelerator. A scheduler that checks only one bank of a two-bank job can admit an unsafe pair while its aggregate occupancy remains within a two-job limit.

The diagnostic interface announces actual accepted operations, not attempted requests. The observer is deliberately outside the allocator's execution environment, making it useful for comparing an external event stream against logs produced by the allocator itself. It does not read scratch memory, discover physical bank mappings, observe DMA transactions, or recover secrets. Its useful output is a reproducible contradiction between a declared occupancy trace and the stated exclusion policy. Independent source inspection and functional isolation tests are still needed to connect that contradiction to a security impact.

## Distinction from existing designs

The repository was synchronized and device directories inventoried before choosing this contract. The closest purposes were inspected, including Exclusion Ledger, Read Share, Wear Balance and Reuse Quarantine. Exclusion Ledger records overlap of four grant levels under universal exclusion; it has neither accepted-operation reconstruction nor selective allowed overlap. Read Share uses three mutually compatible readers and one exclusive writer, giving a star-shaped conflict relation. This design instead uses the path 0--1--2--3: compatibility is not transitive and there is no single writer identity incompatible with everyone. Wear Balance compares cumulative use among free resources, whereas repeated use here is unrestricted. Reuse Quarantine requires elapsed return-to-reuse time, which this policy does not measure.

Originality is scoped to the inspected repository. Conflict graphs and bank partitioning are established techniques, not newly invented mathematics or a worldwide novelty claim. The contribution is the small instrument's nontransitive co-residency contract, explicit policy announcement, graph-aware telemetry validation, independent edge-list test oracle, and operator workflow. The passive carrier, register definitions, startup, UART queue, CRC implementation, timing qualification and desktop transport deliberately reuse jayis1-authored Wear Balance infrastructure. The new folder is self-contained and has no runtime imports from sibling projects. Reuse is disclosed rather than presented as a novel circuit.

## Authorization and threat model

Use only on equipment you own or have explicit written authorization to assess. Authorization must cover diagnostic firmware changes, enclosure access, common-ground attachment, workload execution, service interruption, and evidence retention. Begin with an emulator or development fixture with no hazardous outputs. Do not connect this unisolated prototype to mains-referenced circuits, deployed access controls, vehicles, medical equipment, industrial safety systems, or equipment whose unexpected behavior could injure someone. Stop if attachment changes logic levels, operation, temperature, or grounding behavior.

The relevant attack surface is the target scheduler's resource eligibility decision, including multi-bank reservation, cancellation, error recovery and release bookkeeping. An authorized red team may use owner-approved stress workloads to reveal a missing conflict check. Researchers may compare scheduler revisions. Penetration testers may use the device to document a reduced reproduction after the owner exposes diagnostic signals. None of these workflows requires payload extraction, target command injection, radio transmission, stealth deployment, persistence, credential collection, or Internet access; none is implemented by this device.

The adapter and workstation are trusted components. A dishonest adapter can omit, reorder or fabricate admissions. Aliasing several resources into one two-bit identity invalidates the contract. Each identity denotes one unique reservation, not a class permitting multiple simultaneous members. The four identities and their bank sets must remain fixed throughout a session. The initial active set must be independently established as empty. Quiet wires alone cannot prove that baseline. Physical access can reprogram the MCU, and CRC provides accidental-corruption detection rather than authentication. There is no signed firmware, attestation, secure boot, encrypted export, persistent session identity, or secure element.

## Policy and diagnostic contract

The forbidden unordered pairs are {0,1}, {1,2}, and {2,3}. Allowed two-resource sets are {0,2}, {0,3}, and {1,3}. Any singleton and the empty set are legal. No three-resource set is legal. Firmware stores the symmetric per-tag forbidden masks [2,5,10,4] and announces them to the companion. Admission of an already active tag is a duplicate, checked before bank conflict. Otherwise admission requires that its conflict mask have no intersection with the current active set. Release requires the named tag to be active and removes only that tag. Release order is unrestricted.

A successful admission increments that tag's cumulative use counter and the global admission count. Release increments the completion count but does not refund cumulative use. Uses are accounting evidence, not a fairness obligation or physical wear estimate. Repeatedly admitting and releasing zero is legal even when other resources have never run. Releasing zero from {0,2} does not make one eligible because two remains a blocker. Releasing zero from {0,3} does make one eligible because one and three do not share a bank. These cases distinguish identity-specific conflicts from population ceilings and blanket prohibition of overlap.

Supply a stable tag with both strobes low for twelve nominal milliseconds, hold exactly one strobe high for six milliseconds, then hold both low for six milliseconds. ALLOCATE reports entry into residency; RELEASE reports its end. Both refer to accepted target operations. The adapter must preserve event order and must not silently discard events when busy. These deliberately slow indications support controlled reproductions rather than transparent observation of production-speed jobs. If slowing or instrumenting the target destroys the behavior of interest, this device is not an appropriate measurement method.

## Hardware specifications and bill of materials

The external processor board is ST NUCLEO-F401RE, based on STM32F401RET6, with 512 KiB flash and 96 KiB SRAM. Firmware runs on the nominal reset-default 16 MHz HSI clock and uses SysTick for nominal millisecond sampling. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit, through the board's ST-LINK virtual COM bridge. ST-LINK supplies programming and debugging access. The purchased module supplies regulation, MCU decoupling, reset circuitry and USB connectivity; these functions are not misleadingly represented as parts on the passive carrier.

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness. It connects by short labelled leads rather than stacking directly onto the development board. It has fourteen through-hole components, thirty connected pads and nine nets. There are no radios, antennas, batteries, chargers, analog sensors, target power outputs, relays or removable storage. No enclosure model or mounting holes are included. Use nonconductive supports, strain relief and an underside cover before any bench attachment.

| Reference | Quantity | Part specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, board revision checked before assembly |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness/support | 1 set | Insulated short wires, labels, nonconductive support |

Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Confirm actual part bodies, drill fit and connector mating geometry before ordering. No supplier availability, price or procurement claim is made. Plan an appropriate USB source for the purchased board; 5 V at 500 mA is a conservative source allowance, not measured consumption. The carrier has no independent supply connection and does not power the target. Its only electrical connection to the target is the diagnostic signals and common ground.

| Signal | Target connector | MCU connector | Nucleo pin | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Pin two of each target connector is ground. Both J1 contacts are ground only. J2 has neither a supply contact nor a ground contact, so a separate Nucleo ground lead to J1 is mandatory. Each target `_IN` node passes through a 1 kOhm resistor to its MCU node; a 100 kOhm resistor connects that MCU node to ground. Only common-ground, push-pull, zero-to-3.3 V diagnostic signals are supported. Exclude 5 V, 1.8 V, negative, differential, unknown or mains-related signals. The resistors are not an isolation or overvoltage protection barrier.

Power the observer before enabling diagnostic outputs; disable outputs before depowering it to prevent back-powering. There is no dedicated ESD clamp, fuse or guaranteed hot-plug tolerance. USB and target grounds become common. Check loading, signal thresholds, cable capacitance and ground potential before use. The nominal resistive load is 101 kOhm excluding leakage, but a static resistor calculation cannot establish dynamic signal integrity.

## Architecture and block diagram

```text
Owned target diagnostic adapter       Passive carrier         NUCLEO-F401RE
TAG0 ---------------------------------- 1k --+---------------- PA0
TAG1 ---------------------------------- 1k --+---------------- PA1
ALLOCATE ------------------------------ 1k --+---------------- PA4
RELEASE ------------------------------- 1k --+---------------- PB0
                                             | each separately
                                            100k
                                             |
Target ground -------------------------------+------ GND ----- USB ground

GPIO acquisition -> qualification -> prior tag/setup and strobe checks
 -> occupancy set -> fixed conflict graph -> transactional ledger
 -> BS1 bounded encoder -> USART2 TX -> ST-LINK USB serial
 -> desktop Ledger / History / Integrity -> private local JSON export
```

Target-facing MCU pins remain inputs under the supplied firmware. UART reception is disabled, and the application opens the serial port read-only. There are no commands to start target jobs, alter policy, clear a fault or drive the diagnostic pins. Receive-only is a property of this firmware and wiring arrangement, not an immutable hardware diode. GPIOA and GPIOB are sampled separately, so transitions across ports are not atomically latched. Slow setup rules are essential and do not turn polling into a high-speed logic analyzer.

## Firmware architecture and failure behavior

Firmware is C11 without an RTOS, dynamically allocated memory or downloaded SDK. Startup installs the vector table, initializes data and BSS, and calls main. The linker defines the flash and RAM regions. The board driver configures input pins, SysTick, reset-cause capture, USART and watchdog. The serial driver services a bounded 768-byte buffer without blocking on the entire transmission and reports timeout or submission failure. The main loop samples once per observed tick, services serial traffic, and emits policy and status independently. No radio stack or networking library is linked.

Tag bits qualify together as one bundle; ALLOCATE and RELEASE qualify independently. A changed candidate requires three matching intervals before qualification. Before an operation is accepted, the previously qualified tag must have five stable observations. The raw tag must still agree at the qualifying endpoint, the chosen raw strobe must be high, and the opposite strobe must be low in previous qualified, current qualified and raw states. Simultaneous rising strobes are ambiguous rather than ordered arbitrarily. Holding a strobe produces one event, not repeated admissions.

A repeated timestamp is ignored. A skipped acquisition tick causes terminal correlation loss; missing time is not treated as stable evidence. Ordinary unsigned timestamp rollover is supported. Every sample is processed on a speculative ledger copy. If any cumulative counter exhausts, the entire sample rolls back and the prior coherent ledger is marked halted. A rejected operation preserves accepted occupancy and use counts. All faults are terminal because silent wires cannot reconstruct a lost resource history. The watchdog is fed after a policy halt so evidence remains available rather than being erased by automatic reboot.

Fault bits are bank conflict 1, orphan release 2, duplicate admission 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog startup failure 64. Interface checks precede resource semantics; duplicate identity precedes graph conflict. A halt clears qualification validity and freezes ledger interpretation, while time and saturating transport diagnostics may advance. The retained active set is the last accepted observer state, not necessarily the target state after a violating admission. It must never be used to authorize subsequent target operations.

## Companion application and protocol

Run `python3 app/console.py` from this directory on Linux with Tk support. The standard-library application has Ledger, History and Integrity screens. Enter an explicitly verified serial path, preferably a stable `/dev/serial/by-id/` name, then select New live. There is no discovery sweep. Disconnect restores original terminal attributes; a second connection is refused while the first is owned. Opening a non-TTY, permission denial and hangup are handled without silently replacing retained evidence. Reconnect is explicit and begins new host-side provenance.

The Ledger screen shows occupancy, per-tag cumulative admissions, accounting counters, the exact forbidden-pair list and currently eligible free tags according to retained state. History contains up to 256 accepted snapshots. Integrity shows source, replay/live origin, policy compatibility, parser errors, reset information, host freshness and freeze reasons. Replay accepts a regular file no larger than two MiB, rejects FIFOs and devices, and records a truncated final frame. Export creates a new local JSON file with mode 0600 and refuses overwrite or a final-path symlink. Exports credit jayis1 and include qualifiers rather than only the latest numeric snapshot.

Frames are ASCII `BS1,type,decimal-fields*CCCC` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk with polynomial 0x1021 and initial value 0xffff. B carries reset flags. P carries `[3,1,5,4,2,5,10,4]`: qualification intervals, nominal sample period, prior tag setup, resource count, and the four conflict masks. Policy repeats every second to support late attachment without clearing history. S carries the fields in `app/contract.py`: time, levels, validity, halt, faults, phase, occupancy, depth, setup, observation and operation counters, gaps, missed time, overlapping admissions, peak occupancy, four use counters and transport failures. Status is scheduled every 100 milliseconds when transport is available.

The parser requires exact policy agreement and validates unsigned bounds and conservation relationships. Admissions equal completions plus depth and also the sum of all tag uses. Attempts equal accepted admissions plus completions plus rejected operations. Closed occupancy episodes plus the current nonempty episode indicator plus overlapping admissions equal total admissions. Active sets must be independent sets of the graph, and peak occupancy cannot exceed two. Cumulative history cannot migrate between tags or decrease.

Sparse telemetry deserves special care. Per-tag release counts can be derived from admission growth and occupancy changes. If an incumbent has no releases between snapshots, it occupied the entire interval, so any new admission conflicting with that incumbent is impossible even if the newcomer left before the later snapshot. The companion rejects that hidden-cycle contradiction. If the incumbent did release, ordering is unknown and the companion does not invent a conflict. These are necessary consistency checks, not a complete reconstruction of hidden operations or authentication of an honest target.

Live freshness requires advancing device time, connection state and compatible policy. Duplicate timestamps do not renew it. A prolonged host reception pause, counter rollback, contradictory duplicate, changed halted evidence or diverging host/device progress freezes interpretation. Drift allowance is bounded, not an ever-growing excuse for backlog. Some observer resets without a visible boot record remain indistinguishable from plausible forward traffic. Retain the adapter mapping, firmware revision and operator notes with exported evidence; the wire format alone cannot establish session identity.

## Reproducible build and validation

From this directory run `make -C firmware all test`. Dependencies are GNU Make, host GCC and an Arm bare-metal GCC distribution with newlib/nosys support. Tested versions and actual sizes are recorded in VERIFICATION.md. The target uses Cortex-M4 Thumb soft-float flags and treats compiler warnings as errors. Host tests use address and undefined-behavior sanitizers for the engine and appropriate host checks for UART and scheduler behavior. Build products are ignored and are not represented as source deliverables.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v` for the desktop tests on a headless Linux host. The tests feed the real C encoder's synthetic fixture into the parser, exercise the Tk event loop through a serial pseudo-terminal, select all screens, disconnect, replay and export. Synthetic traffic is not hardware measurement. Policy tests compare repeated operation words against an independent edge-list oracle and test every ordered pair, legal nontransitive overlap, wrong-blocker release, duplicate precedence and transactional failure paths.

With KiCad 9 and its Python bindings and standard footprints installed, run `/usr/bin/python3 tools/generate_hardware.py`, then `/usr/bin/python3 tools/check_hardware.py`. The generator emits schematic symbols, wires, labels, board footprints, routed tracks, outline, project settings and a netlist. The checker runs ERC and DRC with violations fatal and compares every component value, footprint identity and pin-to-net relationship against the manifest. Zero design-rule violations do not certify the external module, harness, insulation or intended application. Review a rendered schematic as well as machine reports.

## Bring-up, evidence handling and remaining qualification

Before fabrication, review ST UM1724 Table 16, STM32F401xD/xE datasheet and RM0368 for the purchased module revision. The manufacturer manual's indexed Table 16 corroborates CN8.1 through CN8.4 as PA0, PA1, PA4 and PB0 and PA2 as USART2 TX. Direct PDF download timed out during this run; search-index excerpts are not a substitute for full module-revision and solder-bridge review. Confirm ST-LINK virtual-COM connections, power jumpers, voltage limits and reset-clock assumptions before assembly.

First inspect an unpowered carrier for shorts, resistance values, ground continuity and header orientation. With target disconnected, power the Nucleo and verify firmware identity and idle BS1 telemetry. Only then attach a known low-energy diagnostic fixture with outputs disabled. Exercise a singleton, each permitted pair, both release orders and a deliberately encoded forbidden admission in an emulator. Compare independent waveforms with observer results. Do not create a dangerous real co-residency condition merely to test the alarm. Measure current, voltage loading and timing before extending the experiment.

A red-team report should state which target resources corresponded to each tag, how the adapter was instrumented, which operations were accepted, and where the first contradiction occurred. A clean trace does not prove confidentiality, fairness, sanitization, bounded latency or isolation after release. Researchers should preserve both passing and failing fixtures with explicit synthetic labels. Penetration testers should minimize retained traces and use an approved storage location; local JSON files are not encrypted or signed. Delete exports according to engagement policy, recognizing that ordinary unlink is not guaranteed secure erasure on flash storage.

Physical assembly, programming, signal integrity, oscillator calibration, ESD resilience, enclosure safety and real-target interoperability remain unverified. The design is intentionally restricted to supervised laboratory evaluation. No certification, production readiness, global novelty, exploitation success or measured security improvement is claimed.
