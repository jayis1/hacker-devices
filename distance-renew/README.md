# Distance Renew — Admission-Distance Resource Reuse Witness

Author and creator: jayis1. Hardware revision A. Protocol DN1, policy revision 1.
Firmware, application and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier: CERN-OHL-S-2.0. Installed tools and standard footprint libraries retain their upstream licenses and attribution.

Status: unbuilt engineering prototype. This package contains target-linked firmware, a Linux desktop companion, and routed native KiCad sources. Executed checks appear in VERIFICATION.md. No physical fabrication, MCU programming, electrical qualification, timing calibration or actual target interoperability is claimed. The instrument observes a diagnostic contract; it does not enforce allocation policy or establish memory safety.

## Purpose and practical value

Distance Renew checks whether an allocator separates consecutive uses of the same resource by at least two other accepted admissions. Four unique resource identities are available. Each identity has exclusive occupancy, but the history of its last admission survives release and complete pool drain. The instrument reports an immediate repeat or a repeat separated by only one admission as a contradiction. It accepts reuse after exactly two intervening admissions, including while those intervening resources remain occupied.

The motivating experiment is a small embedded service with a descriptor recycler that promises a minimum allocation-distance separation. Such a policy can be part of a reduced diagnostic model for delayed references or speculative consumers. A faulty free-list implementation may return the most recently released descriptor directly to the front, even though its documented rotation policy requires turnover first. Time-based measurements would miss the distinction between a long idle interval with no turnover and a short interval with several intervening allocations. This device measures accepted-operation distance rather than wall-clock quarantine.

The policy is not a universal memory-safety remedy. Two other allocations do not prove that stale callbacks completed, DMA stopped, secrets were erased or readers reached a quiescent state. A real product must justify its own reuse contract using its actual architecture. The number two is an explicit laboratory policy chosen for a four-identity reproduction, not a recommendation for production reclamation. Independent source review, functional tests and waveforms are necessary before assigning security impact to a finding.

A security researcher can compare a correct rotating free-list model with a deliberately faulty immediate-recycle model on owned hardware. An authorized red team can document a reduced reproduction using harmless work items and owner-provided diagnostic outputs. A penetration tester can repeat the same approved workload after remediation and compare retained evidence. The instrument never sends workload commands, scans networks, collects credentials, reads payload memory or installs code in a target.

## Exact contract and distinguishing examples

At experiment start, all resources must independently be known free, and no previous admission belongs to the experiment. The MCU begins its accepted-admission ordinal at zero. Each accepted allocation advances this ordinal once and stores the resulting one-based ordinal in the selected resource's last-admission stamp. A zero stamp means that identity has never been admitted during this experiment. For a previously used free identity, allocation is permitted only when the current global ordinal minus its stored stamp is at least two. The newly allocated identity must therefore be at least three ordinal positions after its previous admission.

Release requires the selected resource to be occupied and clears only its occupancy. Release changes neither the global admission ordinal nor any last-admission stamp. Waiting, disconnecting the companion and draining the entire pool also create no distance. Occupied resources are never eligible, regardless of their ordinal age. Duplicate allocation is checked before historical distance, so allocating an occupied resource is reported consistently as a duplicate rather than sometimes as a reuse error.

In the following examples each identity is released before its next appearance unless stated otherwise. The word 0,1,0 is invalid because only one admission intervenes before the second zero. The word 0,1,2,0 is valid at the inclusive boundary. Repeating 0,1,2 forever is valid even if identity three never runs. This is intentionally not an all-resource fairness or coverage promise. The word 0,1,2,3,1,0 is also valid and changes relative service order; there is no fixed cyclic successor relation. First use of any identity is unrestricted except for occupancy and interface correctness.

Admissions while a resource is occupied still contribute distance. Allocate zero, allocate one, allocate two, release zero, allocate zero is legal: one and two need not complete first. Conversely, release zero, release one and release two do not create three admissions. The target adapter must distinguish actual acceptance from completion and from rejected requests. A strobe reporting a request that the allocator correctly denied would manufacture a false contradiction.

The first contradictory operation halts semantic tracking. The rejected observation is counted, while occupancy, per-tag uses and stamps retain the previously accepted history. This does not mean that the target rejected the operation; the observer normally sees an operation the target already accepted. Halt makes later acquisition insufficient to repair history. Export the evidence and restart only after the owner establishes a genuinely new experimental baseline.

## Originality and disclosed platform reuse

The synchronized jayis1/hacker-devices directory inventory and README titles were reviewed before selection. The nearest detailed contracts were read. Reuse Quarantine counts time after return, not admissions since allocation. Round Cover requires every resource to participate before a new coverage round. Baton Order enforces a specific four-domain cycle. Paired Lead compares cumulative counts between fixed partners. Bank Separation constrains co-residency through a conflict graph. None of these contracts implements a rolling two-admission exclusion history with independently tracked resource occupancy and last-admission ordinal evidence.

A three-intervening-admission rule with only four identities would collapse into fixed cyclic behavior. This design deliberately uses two, allowing three-identity operation, unused peers and changing successor order. Its contribution is the admission-distance contract, absolute ordinal stamps, independent recent-identity queue oracle, sparse telemetry checks and corresponding operator interpretation. Originality is relative to the inspected repository, not worldwide prior art or patentability. Reuse-distance and quarantine algorithms are established engineering ideas.

The passive carrier, register definitions, startup, bounded UART driver, CRC code, acquisition qualification and desktop transport reuse jayis1-authored Paired Lead infrastructure. The reused electronics are not represented as a novel circuit. Internal UQ-prefixed identifiers remain inherited names; they do not imply protocol compatibility. DN1 is distinct from sibling protocols. The folder is self-contained and does not import sibling code at runtime or add any shared hardware tree or unified CLI.

## Attack surface, threat model and authorized use

Use only on equipment you own or have explicit written authorization to assess. Authorization should cover diagnostic firmware changes, enclosure access, common-ground wiring, identity mapping, workload, interruption limits, recovery and evidence retention. Permission for a network test does not automatically authorize hardware attachment. Begin with an emulator or an expendable development fixture, with hazardous actuators and production dependencies disconnected.

The investigated attack surface is allocator bookkeeping: free-list insertion, cancellation handling, reset of history at release, wrong-tag accounting and off-by-one distance checks. The observer trusts the operator, target diagnostic adapter, harness, Nucleo module and workstation. A target can omit or falsify indications. A malicious serial device can generate checksum-valid records. A compromised workstation can rewrite exported data. CRC detects accidental corruption, not authenticity, replay or provenance.

There is no secure boot, firmware signature verifier, secure element, encrypted telemetry, trusted wall clock, permanent boot identifier or authenticated command channel. Programming is through physical ST-LINK access, which can replace the image. Although the supplied firmware configures target pins as inputs, the circuit is not an irreversible data diode: different firmware could drive GPIO through the resistors. The instrument does not provide an isolation guarantee or prevent misuse merely by being receive-only in normal software.

Do not attach this unqualified prototype to medical devices, vehicles, deployed access controls, machinery, mains-referenced equipment or unknown field wiring. Stop if attachment changes behavior, voltage, temperature or ground conditions. A clean trace is neither certification nor proof that the uninstrumented target is secure. All conclusions must remain bounded by the diagnostic adapter's accuracy and the acquisition limitations below.

## Hardware specifications and bill of materials

The external active module is ST NUCLEO-F401RE with STM32F401RET6: Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, nominal one-millisecond SysTick and USART2 transmit at 115200 baud, eight data bits, no parity and one stop bit. PA2 connects to the board's ST-LINK virtual serial bridge. The purchased module provides regulation, decoupling, reset circuitry, USB connectivity and SWD programming. These functions are external module components, not missing parts of a claimed standalone MCU PCB.

The custom passive carrier is 80 by 70 mm, two copper layers and nominal 1.6 mm thickness. Tracks are 0.3 mm. Fourteen through-hole parts provide thirty electrical pads across nine named nets. The carrier connects with a labelled harness rather than mechanically stacking onto Nucleo headers. There is no radio, antenna, FPGA, battery, charger, removable storage, analog sensor or target power output. No enclosure CAD or mounting holes are supplied; use insulated supports, underside protection and strain relief.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, purchased revision checked |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness/support | 1 set | Short labelled insulated leads and nonconductive supports |

Resistor footprints use DIN0207 bodies with 7.62 mm lead spacing. Confirm purchased part drawings, lead diameter, body size, drill fit, mating orientation and plating before procurement. Standard KiCad footprint library identities are retained. No stock, price or procurement guarantee is made. Plan a suitable 5 V USB supply for the purchased module; 500 mA is a conservative planning allowance, not measured consumption or USB compliance evidence. The carrier neither powers nor charges the target.

| Signal | Target header | MCU header | Nucleo point | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target header's second contact is ground. Both J1 contacts are ground, never power inputs. J2 contains four signals and no ground, so a separate Nucleo ground lead to J1 is mandatory. Each target-side net ending in _IN passes through a 1 kOhm resistor to its MCU node; a 100 kOhm resistor pulls that MCU node down to ground. An unplugged input is indistinguishable from a valid low indication.

Only known common-ground, push-pull zero-to-3.3 V diagnostic signals are supported. Five-volt, negative, 1.8 V, differential and unknown interfaces are excluded. Series resistance is not galvanic isolation, ESD protection or a surge barrier. USB and target grounds become common. Check ground offset, source impedance, MCU thresholds, input leakage and cable capacitance before use. Power the observer before enabling target signals and remove target signals before depowering it to avoid back-power through input protection structures.

Manufacturer references are ST UM1724 for the Nucleo-64 connector and bridge configuration, RM0368 for STM32F401 peripheral registers, and the STM32F401xD/xE datasheet for electrical limits. Verify the actual purchased module revision and USART2 VCP solder bridges before assembly. The inherited map was checked for agreement among firmware, schematic and documentation; this run does not claim a fresh manufacturer-document electrical review. Reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned target adapter             Passive carrier             Nucleo-F401RE
TAG0 ----------------------------- 1k --+-------------------- PA0
TAG1 ----------------------------- 1k --+-------------------- PA1
ALLOCATE ------------------------- 1k --+-------------------- PA4
RELEASE -------------------------- 1k --+-------------------- PB0
                                        | each MCU node
                                       100k
                                        |
Target ground --------------------------+---- J1 ------------ module GND

GPIO -> continuity guard -> bundled tag / independent strobe qualification
 -> prior setup and opposite-strobe guard -> exclusive occupancy
 -> last-admission ordinal distance -> transactional evidence
 -> DN1 bounded encoder -> USART2 TX -> ST-LINK USB
 -> Linux Ledger / History / Integrity -> explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not atomically latched. This device is a slow diagnostic observer, not a CPU trace analyzer, high-speed bus decoder or glitch detector. The adapter must stretch operations without changing their order. Start with twelve nominal milliseconds of stable tag and low strobes, six milliseconds of exactly one high strobe, then six milliseconds low. Validate actual waveforms independently. If instrumentation removes the original race or changes scheduling materially, do not generalize the instrumented result to production behavior.

## Firmware design decisions

The firmware is C11 without an RTOS, dynamic heap, filesystem, downloaded SDK or command receiver. Startup initializes data and BSS and provides Cortex-M vectors. The linker places flash at 0x08000000 and SRAM at 0x20000000 and reserves stack headroom. Drivers implement GPIO acquisition, SysTick, USART2 TX, reset-cause capture and the independent watchdog. Target-facing GPIO remain inputs and internal pulls are disabled; external pulldowns define the disconnected level.

The two tag bits qualify as a bundle, avoiding artificial identities caused by independently delayed tag bits. Allocation and release qualify independently, so chatter on one strobe does not indefinitely suppress the other. Each candidate needs its first sample plus three matching intervals. An operation also requires five prior stable tag observations, raw/qualified tag agreement, the selected raw strobe still high, and the opposite strobe low in previous qualified, current qualified and raw states. Held strobes count once; short pulses can be filtered or missed.

A quiet qualified baseline starts tracking but cannot establish that the target is empty or that prior history may be forgotten. The owner must establish both premises externally. Interface validation precedes resource semantics. Fault bits are admission distance 1, orphan release 2, duplicate allocation 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog startup failure 64. Two simultaneous rising strobes count as two rejected edges, never an invented allocation/release order.

Each sample updates a speculative state copy. Counter exhaustion rolls back the entire sample and halts the preceding coherent state, preventing a stamp from advancing without its admission count. Accepted ordinal counters do not wrap. Ordinary unsigned millisecond wrap is supported, repeated timestamps do nothing, and skipped samples halt rather than assuming missing intervals were stable. After halt, uptime and transport diagnostics may advance; semantic evidence remains frozen. The watchdog continues to be fed so a policy violation is not deliberately rebooted away.

The UART driver owns a bounded 768-byte buffer, refuses a second pending submission and services at most one ready byte per iteration. A hundred-millisecond timeout abandons an incomplete frame and increments a saturating transport failure count. No serial wait blocks sampling. Status is scheduled every hundred milliseconds when transport is available. Policy repeats approximately every second for late attachment; the boot record is sent only once and is not reused as periodic compatibility information.

## Companion application and interface

Run python3 app/console.py on Linux with Tk installed. The application uses the standard library only and has no dependency download, cloud account, network listener or package lockfile. Select the verified serial device path, preferably under /dev/serial/by-id/, then choose New live. The app does not scan or probe arbitrary devices. A second connection is refused until disconnect; original terminal settings are restored on close and setup failure. Failed connection attempts preserve previously retained evidence.

Ledger displays per-resource cumulative uses, last-admission ordinals, occupancy, eligible free identities and decoded findings. Intervening-admission counts are derived from the global ordinal; an unused identity is explicitly marked None rather than being assigned a fabricated age. Eligibility describes the retained observer state, not permission from a target, and must not be acted upon when HALTED. History shows up to 256 snapshots. Integrity shows provenance, compatibility, parser errors, reset hints, freshness and freeze reasons.

Replay accepts only an opened regular file of at most two MiB and rejects devices and FIFOs without blocking. A partial final frame counts as corruption; empty or incompatible captures are rejected. Export creates a new mode-0600 JSON file, refuses overwrite and final-component symlinks, and includes session metadata even with no completed operations. Exports are neither signed nor encrypted. Apply engagement retention policy and do not assume ordinary deletion securely erases flash-backed storage.

DN1 frames are ASCII DN1,type,decimal-fields followed by an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE covers all preceding bytes, with initial value 0xffff and polynomial 0x1021. B contains reset flags. P contains [3,1,5,4,2,1]: qualification intervals, nominal sample period, prior tag setup, identity count, required intervening admissions and policy revision. Status field order is:

```text
now levels valid halted faults phase active depth stable
observed rounds attempts launched completed rejected gaps missed
queued peak use0 use1 use2 use3 stamp0 stamp1 stamp2 stamp3 tx_fail
```

Here launched is the global accepted-admission ordinal. The inherited rounds field counts closed nonempty occupancy episodes, not reuse cycles, sweeps or generation epochs. Admissions equal releases plus active population and equal the sum of per-tag uses. Attempts equal admissions plus releases plus rejected observations. Stamps are unique when nonzero; every ordinal among the latest three admissions must still appear because legal renewal cannot yet replace it. Each tag's cumulative count requires sufficient ordinal span for its minimum spacing.

Across sparse snapshots, counts cannot decrease or migrate between identities. A stamp cannot change without a new admission for that tag. New stamps must follow the previous snapshot's ordinal and preserve the minimum separation from the old stamp and between repeated new admissions. These are necessary endpoint constraints, not reconstruction of all hidden operations or proof against forged telemetry. The MCU checks each qualified operation; a malicious sender can still conceal an excursion behind plausible sparse endpoints.

Live freshness requires compatible policy, a connected port and advancing device timestamps. Duplicate timestamps do not refresh it. Clock/counter rollback, contradictory duplicates, disappearing faults, changed halted evidence, long reception pauses and excessive host/device progress divergence freeze interpretation. Drift tolerance is capped. Reconnect flushes input and starts new host provenance but does not reset MCU state. Without a persistent boot identifier, some missed resets remain indistinguishable from forward traffic.

## Build, verification and bench acceptance

From this directory run make -C firmware all test with GNU Make, host GCC and Arm bare-metal GCC/newlib. The Makefile creates firmware/build/distance-renew.elf and .bin with Cortex-M4 Thumb soft-float flags and warnings as errors. Host engine tests use address and undefined-behavior sanitizers. The independent oracle stores the last two admitted identities as a queue rather than computing ordinal differences, and enumerates operation prefixes including repeated identities and arbitrary release order. UART and scheduler tests cover bounds, timeouts, recovery, lost boot transmission, periodic policy and watchdog-start failure.

Run xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v for contract, parser and real Tk event-loop tests. C-encoder fixtures feed the actual application and Linux pseudo-terminals exercise serial handling, hangup, settings restoration, screens and export. They are explicitly synthetic, not physical GPIO or USB captures. Use python3 -m compileall -q app for syntax checking. No simulation is presented as an assembled-device measurement.

With KiCad 9 and its Python bindings installed, tools/generate_hardware.py regenerates the native schematic, board, project, authored symbols and manifest. Run it using /usr/bin/python3. Then run tools/check_hardware.py with the same interpreter: it executes ERC and DRC, exports a fresh schematic netlist, and compares references, values, full footprint identifiers and every pad assignment with the PCB and connectivity manifest. Passive-symbol ERC cannot validate external module configuration or voltage compatibility. Render and visually inspect the schematic as a separate check.

Before physical acceptance, inspect unpowered shorts, component values, connector orientation, harness continuity and ground arrangement. Verify module power jumpers and VCP bridges, program through ST-LINK, and inspect idle policy/status before enabling any target output. Exercise first use, immediate repeat, one-intervening rejection, exact-two acceptance, arbitrary release order, all four occupied, independent three-tag cycling, acquisition loss and reset. Compare waveforms against reports, measure loading, calibrate timing and inspect watchdog behavior. Those physical, mechanical, ESD and regulatory gates remain open until measured on actual assembled hardware.
