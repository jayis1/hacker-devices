# Slot Alias — Outstanding Resource-Identity Collision Witness

Author and creator: jayis1. Revision A. Protocol SA1. Firmware and application: GPL-2.0-only. Hardware utilities: MIT. Carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design. Software builds, executable tests, and native KiCad checks are recorded in VERIFICATION.md. Fabrication, MCU programming, calibrated timing, electrical qualification, and field deployment have not been performed. This instrument observes diagnostic indications; it is not a memory protection unit, access controller, safety interlock, or proof of freedom from use-after-free defects.

## Purpose and practical value

Slot Alias observes exclusive ownership of a small resource pool. An owner-provided diagnostic adapter exports a two-bit slot identifier, an allocation strobe, and a release strobe. The instrument records which of four slots are outstanding. Allocating an already occupied slot produces an alias finding. Releasing a slot that is not occupied produces an orphan-release finding. Any outstanding slot may be released next: no first-in-first-out or last-in-first-out rule is imposed.

The motivating security defect is overlapping ownership of a descriptor, DMA staging buffer, cryptographic work slot, or privileged mailbox entry. Two operations may accidentally receive the same slot while the first operation still owns it. Software logs can contain plausible allocation and release counts while hiding that collision. A sampled external identity ledger can expose the diagnostic inconsistency without reading the resource contents, recovering credentials, or controlling the target. The intended workflow is an explicitly instrumented, deliberately slow development fixture.

For example, a target allocates slots one and two, releases slot one, then allocates slot two again. The total outstanding count alone may look reasonable, but slot two never retired. Slot Alias records the duplicate and preserves how many allocation attempts were abandoned when correlation became uncertain. Conversely, allocation of one and two followed by release of one and two is legitimate. Reallocating one after its release is also legitimate and increments a separate reuse counter. These distinctions make the device useful for validating allocator and ownership integration contracts.

The four tags must name four actual exclusive resources, not broad classes of resource. Hashing many concurrent objects down to two bits creates artificial collisions. The adapter must not issue a release until the agreed ownership boundary really ends. Instrumentation that emits a release at enqueue time rather than completion changes the meaning of every result. The observer cannot establish that mapping for the operator and cannot see resources omitted from the diagnostic interface.

## Repository-relative originality

Existing directories and device purpose statements were inventoried before choosing this design. The closest projects were inspected in detail. Context Stack validates nested returns and permits equal-tag reentrancy; Slot Alias explicitly rejects two concurrent allocations with the same tag and permits arbitrary release order. Receipt Witness correlates anonymous audit receipts in FIFO order; this device correlates named occupancy rather than durability. Grant Ledger tracks a single authorization credit, not a four-resource identity set. The new policy is therefore not a renamed stack, queue, or single-credit state machine.

The passive carrier, register definitions, startup, UART scheduler, CRC framing, and desktop transport deliberately reuse jayis1's existing reviewed platform. The identity-set engine, membership tests, legitimate-reuse accounting, protocol contract, and exhaustive release-order regressions implement the distinct behavior. Novelty here means a new useful repository design, not a claim of worldwide invention, patentability, or an unprecedented resource-tracking algorithm.

## Legal and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Permission should identify the target, diagnostic firmware, exact attachment points, approved workloads, expected interruptions, and evidence-retention policy. Initial experiments belong on an isolated development fixture with hazardous actuators disconnected. Do not connect this unisolated prototype to deployed door controls, vehicles, medical equipment, industrial protection systems, mains-related test points, or unknown field wiring.

An authorized red team can ask an owner to expose a suspected resource-ownership race through normal laboratory test paths and compare evidence before and after a fix. The instrument does not create a race workload, exploit a vulnerability, inject target traffic, change permissions, or prevent a collision. A penetration-testing finding must be corroborated with source review, target logs, and independent waveform capture. A diagnostic collision is a candidate integration defect, not automatic proof of exploitable memory corruption.

Physical attachment can disturb the target through loading, grounding, back-power, or mistakes even with receive-only firmware. Keep the instrument visible and labelled, use short insulated leads, and stop if connection changes behavior. The author, jayis1, does not authorize misuse or guarantee electrical safety, regulatory compliance, or security. Absence of a recorded fault is not certification that a target is safe or secure.

## Attack surface and trust boundaries

Relevant defects include duplicate live-slot assignment, duplicate release, missing allocation notification, missing release notification, overlapping strobes on the shared tag bus, and unstable tags. The target adapter must accurately report internal transitions. A malicious target can lie on every wire. A disconnected input can look low through its pulldown. Neither condition can be distinguished reliably from valid idle behavior by this interface alone.

The operator, harness, MCU image, development board, programming interface, and workstation are trusted. Serial traffic and replay files are treated as untrusted parser input. CRC detects accidental corruption but provides no authentication, signature, freshness proof, or resistance to intentional forgery. There is no secure bootloader, authenticated updater, persistent boot identity, cryptographic attestation, encrypted storage, trusted wall clock, radio, network listener, cloud upload, or remote control channel.

Receive-only means the supplied firmware configures the four target pins as inputs and never sends target commands. It is not a certified physical data diode: replacement firmware could drive those pins. ST-LINK remains a privileged local programming interface. Keep it under physical control. The companion opens the selected serial terminal read-only, but host serial-control behavior still requires testing on the actual board.

## Hardware specifications and bill of materials

The complete assembly comprises an external ST NUCLEO-F401RE board, the passive carrier in kicad/, labelled harness leads, an insulating support, and a Linux workstation. The STM32F401RE Cortex-M4 provides 512 KiB flash and 96 KiB SRAM. Firmware assumes the reset-default nominal 16 MHz HSI clock and uses one-millisecond SysTick. USART2 sends 115200 baud, eight data bits, no parity, one stop bit, through the Nucleo ST-LINK virtual serial connection.

The external development board supplies regulation, decoupling, MCU, reset, SWD programming, and USB conversion. The carrier is not represented as a self-contained MCU board. It contains no FPGA, radio, antenna, battery, charger, active sensor, level translator, isolated supply, or storage card. It is cable connected, not a mechanically compatible stacking shield.

The carrier PCB measures 80 by 70 mm, with two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks, a ground-layer transition, and a closed outline. Fourteen components, thirty electrical pads, and nine named nets are represented in the schematic, PCB, and connectivity manifest. There are no mounting holes or enclosure files; provide nonconductive support and strain relief.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, axial DIN0207 pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, axial DIN0207 series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header |
| J2 | 1 | 1x4 vertical 2.54 mm header |
| Harness/support | 1 set | Insulated labelled leads and nonconductive support |

Resistor footprints use 7.62 mm lead spacing. Match purchased body dimensions, connector mating access, drill fit, and support geometry before fabrication. Vendor stock, price, availability, and manufacturing yield have not been verified.

## Wiring, power, and electrical limits

| Function | Target header | MCU header | Nucleo / GPIO | Bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| ALLOC | J5.1 | J2.3 | A2 / PA4 | 2 |
| FREE | J6.1 | J2.4 | A3 / PB0 | 3 |
| Common ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only; neither pin supplies power. J2 carries four signals and no power or ground. Names ending `_IN` are target-side nodes; unsuffixed names are MCU-side nodes after their series resistors. Verify connector pin numbering and harness continuity before attachment. Do not infer compatibility from header appearance.

Only nominal zero-to-3.3 V, common-ground push-pull diagnostic outputs are supported. Each input presents a nominal 101 kOhm resistor path to ground before leakage and source impedance. The series resistor is not qualified overvoltage or ESD protection. There is no dedicated clamp, galvanic isolation, reverse-current blocker, fuse, or hot-plug guarantee. Five-volt signals, negative voltages, 1.8 V logic, differential buses, relay coils, and unknown field signals are outside the contract.

Power the Nucleo through its documented ST-LINK USB arrangement. Reserve a conservative 5 V, 500 mA source allowance while respecting actual jumper settings and USB limits; this is not a measured current figure. USB ground reaches the target through Nucleo ground and J1. Establish compatible ground, power the observer before enabling target signals, and remove target signals before depowering the observer. Otherwise MCU protection structures can back-power it. Ground offsets, loading, thresholds, rise times, leakage, power sequencing, and temperature require physical measurements.

## Architecture and block diagram

```text
Owned diagnostic adapter           Passive carrier              External Nucleo
TAG0 ----------------------------- 1k --+----------------------- PA0
TAG1 ----------------------------- 1k --+----------------------- PA1
ALLOC ---------------------------- 1k --+----------------------- PA4
FREE ----------------------------- 1k --+----------------------- PB0
                                        | each node
                                       100k
                                        |
Common ground ------------------------- GND -------------------- GND / USB

GPIO samples -> independent qualification -> shared-tag setup check
             -> four-bit occupancy set -> duplicate/orphan classification
             -> conserved allocation ledger -> bounded SA1 CRC snapshots
             -> USART2 PA2 TX -> ST-LINK USB -> Linux desktop
                                                Ownership / History / Integrity
```

GPIOA is read before GPIOB. These are not simultaneous hardware-latched samples across both ports. The observer cannot resolve processor-speed races or capture arbitrarily short pulses. Use a deliberately slowed adapter and verify its waveforms independently. Initial experiments should provide at least twenty milliseconds tag setup and twenty milliseconds each of strobe high and low time, rather than operating close to qualification boundaries.

## Exact firmware measurement contract

Each input qualifies independently. A candidate change needs its initial observation plus three subsequent unchanged one-millisecond intervals before the qualified value changes. Chatter on another input cannot defer that qualification indefinitely. Shorter pulses may be rejected or missed entirely. Qualified ordering is not a claim about physical ordering within a sampling interval.

At startup, arrange an actual all-free target pool and restart the observer under operator control. All four inputs must qualify, followed by twenty evaluated ticks with raw and qualified ALLOC and FREE both low. Tags need not be zero. This quiet baseline enables READY, but silence does not itself prove that the pool is empty. Mid-run attachment is unsupported even if the target happens to be between strobes.

Once ready, every qualified ALLOC rising edge increments allocations. If the indicated slot is free, its occupancy bit is set and pending increases. If that slot has previously been accepted and retired during this run, reuse also increases. A second allocation while its bit is already set increments duplicate, records the offending attempt as abandoned, and invalidates the complete correlation segment. Four unique slots can be outstanding; any fifth allocation necessarily names an occupied slot and is a duplicate rather than a separate capacity-overflow category.

A qualified FREE rising edge increments releases. If its slot is occupied, the bit clears, pending decreases, and retired increases. Any outstanding slot can retire next. If the bit is clear, orphan increases and correlation is invalidated. Holding either strobe high never creates repeated operations. A subsequent operation needs another qualified low-to-high transition.

Simultaneous rising strobes, or a rising strobe while the opposite qualified strobe remains high, record collision. Both attempts are counted when both rise. Collision precedes setup and membership evaluation because the shared tag bus cannot establish order. A malformed allocation attempt is still conserved as abandoned. This device deliberately does not interpret simultaneous free-and-reallocate as an atomic replacement transaction.

The qualified tag must have five stable evaluated intervals before an operation is accepted, with sampled raw tag agreeing. A raw excursion resets setup even when it never qualifies. A qualified tag change also resets setup. The implementation updates stability before evaluating that tick's operation, so acceptance at the fifth stable evaluated interval is inclusive. This is a qualification-domain setup rule, not a measured physical margin. There is no post-edge hold obligation beyond the next operation's setup requirement.

Every operation fault permanently clears READY for that observer run and abandons outstanding slots. Quiet wires never reconstruct ownership. Reestablish the real target baseline and deliberately restart the observer to begin a new run, exporting old evidence first. Input levels and acquisition diagnostics remain visible after correlation loss, but subsequent operations are not counted as if their ownership were known.

A tick delta other than one records a gap, accumulates missed ticks, invalidates qualification, and abandons outstanding ownership. Calls in the same tick are ignored. Normal unsigned timer wrap is supported. The code never treats skipped observations as continuous stable input. Counter updates occur on a temporary engine copy. If any checked addition exhausts its range, the speculative sample is discarded, prior pending slots are abandoned, and a terminal halted state is entered. The overflowing sample is not included in retained counts.

The invariant is allocations = retired + abandoned + pending. Occupied is a four-bit slot bitmap, not the current input tag; pending equals its population count. Peak is the largest accepted simultaneous occupancy, at most four. Reuse counts accepted allocations of previously seen slots and cannot exceed retired. Abandoned includes accepted slots whose later fate became unknown and malformed allocation attempts; it is not a count of proven target leaks. A slot may remain legitimately occupied indefinitely: this revision has no lifetime deadline or leak detector.

## Firmware transport and companion interface

The allocation-free C implementation separates startup, board access, qualification, ownership policy, encoding, and nonblocking UART service. The UART copies a bounded frame, transmits one ready byte per poll, and abandons a frame stalled for one hundred milliseconds. tx_fail saturates independently. Watchdog startup waits are bounded using the tick clock; startup failure halts measurement before the first status. A stopped tick itself still requires physical watchdog testing.

SA1 frames are ASCII `SA1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection, and no final XOR. Four uppercase hexadecimal digits cover bytes before the asterisk. B carries reset flags once after boot. P carries `4,5,3,20`: slot capacity, setup, qualification, and baseline. P repeats approximately every second without clearing prior evidence. S repeats approximately every hundred milliseconds.

The S fields are `now levels valid ready halted faults pending occupied allocations releases retired abandoned duplicate orphan reuse collision unstable gaps missed peak tx_fail`, in that order. All are unsigned 32-bit integers. Levels and valid masks use the wiring-table bit order. Fault bits are duplicate=1, orphan=2, collision=8, unstable=16, gap=32, exhaustion=64, watchdog=128. Bit value 4 is reserved and rejected. The C encoder and Python parser reject inconsistent ledgers. Internal previously-seen slot bits are not transmitted; the app checks the public necessary bounds rather than claiming full reconstruction of hidden state.

Launch `python3 app/console.py` from this directory. The standard-library Tk application provides Ownership, History, and Integrity screens. Enter the exact port, usually `/dev/ttyACM0`, and choose New live. There is no automatic probing. Disconnect retains evidence. A successful new connection replaces the model; a failed opening preserves it. History retains at most 256 snapshots, not every GPIO edge.

Integrity exposes provenance, compatibility, parser errors, reset flags, data age, connection state, and freeze reasons. LIVE requires two advancing compatible statuses while connected. It describes telemetry progress, not valid resource correlation: READY must be checked separately. Duplicate timestamps do not renew freshness. Counter rollback, contradictory duplicates, disappearing sticky faults, unexpected boot records, or changed halted evidence freeze the model. Host/device progress divergence and long reception pauses conservatively reject stale buffered traffic. Without a persistent boot identity, some resets remain indistinguishable from forward traffic.

Replay accepts only regular files of at most two MiB, rejecting FIFOs and devices without blocking. Framing resynchronizes on the complete SA1 marker. An unfinished tail records an error; an empty or incompatible recording is rejected. Export creates a new mode-0600 JSON file, refuses overwrite and final-path symlinks, and includes author jayis1, source, provenance, integrity qualifications, latest snapshot, and history. Files are not signed or encrypted. Use a trusted destination and the approved retention policy.

## Building, testing, and physical follow-up

Required tools are GNU Make, Arm GNU GCC with newlib/nosys support, native GCC, Python with Tk, Xvfb for headless GUI tests, and KiCad 9 CLI/Python bindings with standard footprints. There are no downloaded application packages or mobile SDK dependencies. Run from this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

The target build produces ELF, binary, and map under firmware/build for Cortex-M4 Thumb soft-float, flash at 0x08000000, and 96 KiB RAM. The linker reserves at least four KiB stack headroom. Program only through a supervised ST-LINK procedure suitable for the actual board. No programming or silicon execution is claimed here.

Tests cover duplicate and orphan identity, legitimate reuse, every release permutation of four allocated slots, a long independent occupancy oracle, qualification rejection, collisions, gaps, timer wrap, exhaustion, UART bounds, scheduler startup, malformed frames, replay limits, and real Tk event loops using Linux pseudo-terminals. `firmware/build/fixture > synthetic.sa1` produces production-encoder synthetic traffic, not a hardware capture. CAD checking runs ERC and DRC and compares fresh schematic connectivity, component values, and full footprint identifiers against actual PCB pads and the manifest.

Before valuable hardware is attached, inspect the assembled carrier unpowered, verify continuity and shorts, measure resistor values, and confirm the Nucleo revision and solder bridges. Exercise each input from a benign source and compare with a calibrated analyzer. Check GPIO direction through boot and reset, thresholds, loading, back-power, USB loss, ground offsets, and thermal behavior. ESD, EMC, enclosure suitability, environmental limits, and regulatory compliance remain unverified.

Reference documents are ST UM1724 for Nucleo-64 MB1136, RM0368 for STM32F401 peripheral registers, and DS9716 for STM32F401xD/xE electrical characteristics. Current manufacturer PDFs were not successfully retrieved in this run; the established repository board mapping was reused rather than inventing a new pinout. Verify current documents and the actual module before wiring. A passing software test or passive-network rule check does not establish physical compatibility or manufacturing readiness.
