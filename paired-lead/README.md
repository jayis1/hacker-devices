# Paired Lead — Independent Paired-Resource Admission Skew Witness

Author and creator: jayis1. Hardware revision A. Protocol PL1, fixed partner-map revision 1.
Firmware, companion application and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. Standard installed tool and footprint libraries retain their upstream licenses and attribution.

Status: unbuilt engineering prototype. Firmware is target-linked, the Linux desktop application is exercised with synthetic serial transports, and the native KiCad carrier is routed and checked. See VERIFICATION.md for executed checks. Fabrication, MCU programming, timing calibration, electrical qualification and physical interoperability are not claimed. This instrument is an observer, not a scheduler, protective interlock, authenticator or certification authority.

## Purpose and practical utility

Paired Lead audits a specific scheduling promise: two associated resources may proceed asynchronously, but neither may receive more than two additional accepted admissions ahead of its partner. The device tracks four unique resource identities, arranged into fixed pairs zero/one and two/three. Each pair has its own independent historical balance. Resources also have exclusive occupancy: an allocation starts a lifetime, and a release ends it. Release never refunds the admission or erases historical imbalance.

A motivating experiment is an embedded gateway with two independent replicated audit streams. Each stream has a primary and secondary processing lane, and a laboratory scheduler promises bounded speculative progress between those lanes. If the scheduler accidentally compares against a global counter, progress in the other stream can authorize excessive work on a lagging stream. If it clears a lead counter at release, a fast primary can run indefinitely while its own secondary receives nothing. Paired Lead observes harmless diagnostic reservations to expose those bookkeeping errors without reading audit payloads or injecting requests.

This is not a replication correctness checker. Admissions do not prove that matching records were processed, that either lane persisted data, or that the secondary completed a task. The two lanes can perform unrelated work and still produce balanced counts. A trusted adapter must map actual accepted operations into the stated identities; payload matching and successful completion require separate evidence. The benefit is a narrow, reproducible admission-policy witness independent of the target's own logging transport.

An authorized security researcher can compare a correct emulator with a faulty one that pools credits across streams. A red team can use a preapproved, harmless workload on a disconnected development target to demonstrate that a promised bound is not enforced. A penetration tester can repeat the same trace after remediation. None of these workflows requires credentials, network scanning, exploitation, persistence or disruption of operational equipment.

## Exact contract and examples

All resources begin free and all four admission counts begin at zero. The partner of tag `t` is `t XOR 1`. Allocation is accepted only if the selected resource is free and its existing count is strictly less than its partner's count plus two. The selected count then increases once. A release is accepted only if the selected resource is occupied; it changes occupancy but no use count. Consequently both signed differences, `use0-use1` and `use2-use3`, remain within the inclusive interval minus two through plus two.

Allocate zero, release zero, allocate zero is legal. Zero is now two admissions ahead of one. Releasing zero permits another lifetime electrically, but does not create historical allowance: a third allocation of zero is a finding. One admission of one reduces the difference to one and permits one further admission of zero, even if one remains busy. Releasing one without a new admission cannot create any additional allowance. There is no time-based refill, expiry or demand input.

The other pair has no influence. Alternating zero and one for ten complete lifetimes each while never using two or three is legal. Such a trace violates global coverage and least-used-free policies, but not this contract. Conversely, giving resource two a hundred admissions cannot pay for zero's third unmatched admission. Progress cannot be transferred across pairs. Both directions are symmetric: one may lead zero by two, and three may lead two by two.

All four resources may be occupied at once. Releases can occur in any order and the pool may remain continuously occupied through many paired admissions. The protocol's historical field `rounds` means completed nonempty occupancy episodes: it increments when the final occupied resource releases. It does not count paired synchronization points, epochs, balanced moments, completed records or replication batches. A balanced pair may still have outstanding operations.

The instrument stops on the first contradiction. The violating operation is counted as a rejected observation, while retained occupancy and use counts describe only the previously accepted history. This does not imply that the target rejected the operation: typically the target already admitted it, which is exactly why the observer reports a contradiction. Preserve this distinction in findings and screenshots.

## Originality and disclosed platform reuse

The synchronized jayis1/hacker-devices repository was inventoried by device directory and README title before selection. Searches for paired admission bounds and the closest measurement contracts were inspected. Round Cover enforces unordered coverage across all four identities, including busy participants. Wear Balance selects a least-used currently free candidate. Use Quota applies fixed nonrefundable lifetime limits. Burst Budget replenishes an aggregate allowance with elapsed time. None of those contracts provides independent, symmetric, bounded historical lead for fixed partner pairs.

The contribution is the paired lead measurement contract, the independent signed-walk oracle, its partner-progress and unrelated-pair discrimination cases, and matching desktop interpretation. It is a distinct repository design, not a claim of worldwide algorithmic novelty or a patent search. Fixed paired counters are not themselves presented as newly invented mathematics.

The passive carrier, MCU startup, register definitions, GPIO acquisition, bounded UART transport, CRC framing and desktop transport intentionally reuse jayis1-authored Round Cover infrastructure. Those circuits and generic drivers are not new electrical inventions. Internal `UQ_` symbols remain inherited implementation names; the externally visible protocol is PL1 and cannot be used interchangeably with sibling devices. The directory is self-contained, with no runtime imports from sibling folders, unified system tree or external Python packages.

## Attack surface, trust boundaries and authorization

Use only on equipment you own or have explicit written authorization to assess. Agree on target and adapter revisions, wiring points, diagnostic changes, identity mapping, permitted workloads, stop conditions and evidence retention. Permission to assess software does not automatically permit opening an enclosure or attaching common-ground electronics. Begin with an emulator or an expendable development board with hazardous actuators and production dependencies disconnected.

The security-relevant surface is admission-accounting integration. Candidate defects include replacing per-pair state with an aggregate counter, clearing use history at release, treating an empty pool as a fresh campaign, comparing only currently free partners, charging the wrong identity, and permitting a third unmatched admission through an off-by-one check. The device does not generate these operations; the operator supplies a supervised fixture and an authorized workload.

The adapter must report accepted operations, not rejected requests, using stable unique identities. It must preserve every operation and its order. A target can lie on all four diagnostic wires, and a compromised workstation can rewrite every exported report. CRC detects accidental corruption rather than forgery. No secure boot, signed firmware, authentication, encryption, persistent boot identifier, trusted clock or cryptographic attestation is provided. Physical ST-LINK access can replace the image, including its input-only pin configuration.

Start only after independently establishing an empty pool and zero experimental admission history. Quiet strobes do not prove either premise. Resetting the observer destroys history but does not authorize a reset of target obligations. A continuity gap or observer reset requires an explicitly fresh experiment. Reconnecting the companion starts new host provenance without changing continuously running MCU counts; a missing boot record can still make some resets indistinguishable from valid forward traffic.

Do not connect this unisolated prototype to deployed access controls, medical devices, vehicles, moving machinery, mains-related circuits or unknown industrial wiring. Receive-only firmware does not prevent loading, back-power, grounding mistakes or damage. Stop if attachment changes behavior, temperature or signal levels. A passing trace is not security certification or permission to operate a target outside its owner's scope.

## Hardware specification and bill of materials

The processor is an external ST NUCLEO-F401RE module containing STM32F401RET6, a Cortex-M4 MCU with 512 KiB flash and 96 KiB SRAM. Firmware assumes the nominal reset-default 16 MHz HSI oscillator, a nominal one-millisecond SysTick and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. PA2 connects to the module's ST-LINK virtual COM interface. The purchased module provides regulation, decoupling, reset, SWD programming and USB connectivity; these are not missing components of a purported standalone MCU PCB.

The supplied carrier is 80 by 70 mm, two copper layers and nominal 1.6 mm thickness, with 0.3 mm tracks and a closed outline. Fourteen through-hole components expose thirty connected electrical pads across nine nets. The carrier is harness-connected, not a mechanically mating shield. There are no radios, antennas, FPGA, active sensors, batteries, chargers, storage cards or target power outputs. No mounting holes or enclosure CAD are supplied; insulated supports, underside protection and strain relief are required.

| References | Quantity | Requirement |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, purchased revision verified |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness/support | 1 set | Short insulated labelled leads and nonconductive supports |

The resistor footprints use DIN0207 bodies and 7.62 mm lead spacing; headers use KiCad standard single-row vertical footprints. Match actual lead diameter, body dimensions, drill fit, mating geometry and orientation before procurement. Availability and pricing have not been verified. Reserve a suitable USB source for the purchased module; 5 V at 500 mA is a planning allowance, not measured consumption or a USB compliance assertion. The passive carrier supplies no target power.

| Signal | Target pin | MCU-side pin | Nucleo point | Bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Pin two on each target header is ground. Both J1 contacts are ground only, never supply inputs. J2 has four signals and no ground or power contact: connect a separate Nucleo ground lead to J1. Each target `_IN` node passes through a 1 kOhm resistor to its MCU node, with a 100 kOhm pulldown from that MCU node to ground. A disconnected wire reads low and is not distinguishable from a genuine low diagnostic indication.

Only known common-ground, push-pull zero-to-3.3 V signals are supported. Five-volt, negative, 1.8 V, differential and unknown field interfaces are excluded. Series resistors are not isolation, dedicated ESD protection, surge suppression or a hot-plug guarantee. Power the observer before enabling target signals and disable target signals before depowering it. USB and target grounds become common. Inspect ground offset, source impedance, leakage, cable capacitance and logic thresholds before attachment.

Consult ST UM1724, the STM32F401xD/xE datasheet and RM0368 before assembly. Verify the actual module revision, CN8 pin mapping, VCP solder bridges and power jumpers. The inherited map was checked for internal agreement across files; a fresh manufacturer PDF retrieval failed during this run, so no fresh manufacturer-document review is claimed. Reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . Passive-board ERC does not validate that external module's configuration or electrical safety.

## Architecture and block diagram

```text
Owned target adapter             Passive carrier             Nucleo-F401RE
TAG0 ---------------------------- 1k --+--------------------- PA0
TAG1 ---------------------------- 1k --+--------------------- PA1
ALLOCATE ------------------------ 1k --+--------------------- PA4
RELEASE ------------------------- 1k --+--------------------- PB0
                                       | each MCU node
                                      100k
                                       |
Target ground -------------------------+------ J1 ----------- module GND

GPIO -> continuity guard -> tag bundle + independent strobe qualification
 -> prior setup / opposite-strobe guard -> exclusive occupancy
 -> per-pair historical lead test -> transactional evidence
 -> PL1 bounded encoder -> USART2 TX -> ST-LINK USB
 -> Linux Ledger / History / Integrity -> explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not atomically latched across ports. This is a deliberately slow diagnostic observer, not a high-speed bus analyzer. The adapter must stretch accepted operations without changing their order. If instrumentation changes the behavior under investigation, conclusions cannot automatically be generalized to the uninstrumented system.

## Firmware and timing design

The firmware is C11 with no RTOS, heap, filesystem or downloaded SDK. Startup initializes data and BSS and installs the core vectors. The linker places flash at 0x08000000 and SRAM at 0x20000000 and checks stack headroom. Drivers configure GPIO inputs, SysTick, USART2 TX, reset-cause capture and the independent watchdog. There is no target-facing command parser; USART reception is disabled.

Tags qualify as one two-bit bundle, preventing separately delayed bits from synthesizing an unobserved identity. Allocation and release strobes qualify independently, preventing chatter on one from indefinitely hiding the other. Each candidate requires its initial observation plus three matching intervals. An operation additionally requires five prior stable tag observations, raw/qualified tag agreement, a still-high raw selected strobe and a low opposite strobe in previous qualified, current qualified and raw states.

The reproducible test pacing is twelve nominal milliseconds with the tag selected and strobes low, six milliseconds with exactly one strobe high, and six milliseconds low afterward. Held strobes count once. Short pulses can be filtered or missed and are outside the supported interface. A quiet qualified baseline enables tracking but cannot establish the target's initial empty and zero-history premise.

Interface validation precedes semantics. Allocation rejects a duplicate occupied identity before considering paired lead. Release rejects an unoccupied identity. Fault bits are paired lead 1, orphan release 2, duplicate allocation 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog startup failure 64. Two simultaneous rising strobes are ambiguous and count as two rejected edges without inventing an order.

Each sample updates a speculative state copy. If any diagnostic increment would overflow, that entire sample is rolled back and the preceding coherent state halts. Eligibility arithmetic widens before adding two to the partner count. Repeated timestamps do nothing, ordinary unsigned clock wrap is supported, and skipped samples halt rather than pretending missing intervals were stable. After halt, time and transport diagnostics can advance but accepted evidence stays frozen. The watchdog remains fed so a policy finding is not deliberately rebooted away.

Serial service uses a bounded 768-byte buffer, refuses a second outstanding submission and writes at most one byte per poll. A hundred-millisecond timeout discards an incomplete transmission and increments a saturating transport counter. The next complete protocol marker allows host resynchronization. The firmware periodically repeats compatibility information for late attachment; it does not repeat reset records that would erase provenance.

## Application and protocol

Run `python3 app/console.py` on Linux with Tk support. The companion uses only the Python standard library. It needs no package install, cloud account, network service or dependency lockfile. Enter a verified serial path, preferably under `/dev/serial/by-id/`, then choose New live. The application does not probe arbitrary devices. A second connection is refused until disconnect, original terminal settings are restored on close, and a failed open preserves previous evidence.

Ledger displays counts, occupancy, signed pair leads, eligible free identities and decoded findings. Eligibility describes retained observer state, not target authorization, and the HALTED indication must not be ignored. History displays up to 256 retained snapshots. Integrity displays source, live/replay provenance, parser errors, reset hints, freshness and reasons interpretation froze. Replay accepts regular files of at most two MiB, rejects devices and FIFOs, and counts truncated tails. Export creates a new mode-0600 JSON file, refuses overwrite and final-path symlinks, and includes metadata even when no operation completed.

PL1 frames are ASCII `PL1,type,decimal-fields*CCCC` plus LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, with initial value 0xffff and polynomial 0x1021. B reports reset flags once. P repeats `[3,1,5,4,2,1]`: qualification intervals, nominal sample period, prior tag setup, resource count, inclusive maximum lead and partner-map revision. S is scheduled every hundred milliseconds when transport is available. Status fields are:

```text
now levels valid halted faults phase active depth stable
observed rounds attempts launched completed rejected gaps missed
queued peak use0 use1 use2 use3 tx_fail
```

Encoder and parser enforce unsigned bounds, masks, depth, peak, pair differences and conservation. Admissions equal releases plus active population and equal the sum of per-tag use counts. Attempts equal admissions plus releases plus rejected edges. Closed occupancy episodes plus nonempty indication plus overlapping admissions equal admissions. Per-tag histories cannot decrease or migrate across sparse records. These are necessary endpoint checks, not reconstruction of all hidden ordering: an illegal excursion followed by catch-up could fit legal endpoints in forged telemetry. The real MCU checks every qualified operation and halts at that excursion.

Live freshness requires compatible policy, advancing device time and a live connection. Duplicate bytes do not refresh it. Contradictory timestamps, rollback, changed halted evidence, long receive pauses or divergent host/device progress freeze interpretation. Drift allowance is capped rather than increasing indefinitely. Synthetic fixtures and replay are never physical measurements. JSON is neither encrypted nor signed; store it under engagement retention policy and do not assume ordinary deletion securely erases flash storage.

## Build, verification and physical acceptance

From this directory run `make -C firmware all test` with GNU Make, host GCC and Arm bare-metal GCC/newlib. This produces `firmware/build/paired-lead.elf` and `.bin`, and runs engine, UART and scheduler tests. Target compilation uses Cortex-M4 Thumb soft-float with warnings as errors. Engine tests use AddressSanitizer and UndefinedBehaviorSanitizer. The independent oracle represents each pair as a signed walk, unlike the engine's cumulative counters, and enumerates operation prefixes including repeated identities and release permutations.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v` for protocol and real Tk event-loop tests. They use actual C-encoder fixtures and Linux pseudo-terminals to exercise screens, independent pair progress, partner recovery, serial disconnect, replay, export, malformed input and failure paths. These are synthetic transport checks, not proof of USB or physical GPIO behavior. Syntax checking is `python3 -m compileall -q app`.

With KiCad 9 bindings and standard footprints installed, regenerate deliberately using `/usr/bin/python3 tools/generate_hardware.py`. Run `/usr/bin/python3 tools/check_hardware.py` to execute ERC, DRC and schematic/PCB/netlist/manifest comparisons of references, values, footprint identities and every connected pin. Render the schematic for visual inspection as well. `python3 tools/check_release.py` checks the requested content minimums, attribution and whitespace; passing quotas is not electrical evidence.

Before physical testing, inspect unpowered shorts, resistor values, connector orientation and ground continuity. Check the purchased Nucleo power and bridge configuration, program via its normal ST-LINK workflow, and verify idle policy/status before enabling target outputs. Exercise both lead directions, the inclusive second admission, forbidden third admission, partner recovery, unused independent pair, duplicate allocation, orphan release and acquisition loss on an expendable fixture. Compare independent waveforms with retained evidence, measure loading and calibrate timing. Fabrication, programming, real watchdog/reset behavior, ESD, mechanics and physical target interoperability remain explicit bench acceptance gates.
