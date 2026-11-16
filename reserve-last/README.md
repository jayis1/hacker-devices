# Reserve Last — Scarcity-Gated Reserve Admission Witness

Author and creator: jayis1. Hardware revision A. Protocol RZ1, policy revision 1.

Status: unbuilt engineering prototype. This package contains target-linked C firmware, a functional Linux desktop companion, and routed native KiCad carrier sources. See VERIFICATION.md for executed software and CAD checks. Fabrication, physical programming, electrical qualification, calibrated timing, and real-target interoperability are not claimed. The instrument observes a diagnostic contract; it is not an allocator, enforcement boundary, safety interlock, or security certificate.

Original firmware, application, and tests: GPL-2.0-only. Original hardware utilities: MIT. Original carrier: CERN-OHL-S-2.0. Purchased modules, standard footprints, and installed tools retain their upstream provenance and licenses.

## Purpose and practical utility

Reserve Last investigates whether an embedded resource allocator consumes its designated fallback resource before ordinary capacity is exhausted. A target can preserve unique ownership, correct aggregate counts, and valid completion identities while unnecessarily consuming its reserve. A stale availability cache, inverted fallback condition, or cancellation fast path can cause this contradiction. Ordinary collision monitoring does not expose it. This device reconstructs committed occupancy and evaluates whether the reserve admission was justified at that particular observation.

A useful laboratory target is a security service with three ordinary request descriptors and one overflow descriptor. The overflow descriptor might reside in a different storage area, require expensive cleanup, or be intended to remain idle until a normal pool is full. The owner exports actual allocations and returns through a deliberately slow diagnostic adapter. Harmless authorized workloads exercise full capacity, cancellations, repeated reuse, and arbitrary completion order. Reserve Last retains the first contradictory admission without collecting credentials, memory addresses, request payloads, or application identities.

This instrument does not prove that reserve capacity remains available to emergency clients. Its narrow contract permits reserve use once ordinary resources are busy, regardless of requester identity. A policy that reserves a descriptor exclusively for privileged recovery requires a separate authorization indication and is not implemented here. Nor does the instrument establish that reserve contents are erased, that a fallback bank is powered down, or that capacity exhaustion cannot cause denial of service. Security significance depends on the actual target contract and must be established independently.

## Exact contract and examples

Four unique identities are numbered zero through three. Identities zero, one, and two are ordinary resources. Identity three is the designated reserve. Every identity supports at most one live allocation. The operator independently establishes an empty target pool before resetting and starting the observer. Quiet diagnostic strobes alone cannot prove this baseline.

An ALLOCATE indication represents an already committed target operation. The named identity must be free. Any free ordinary identity is eligible, with no numeric rank, fairness, recency, or rotation constraint among ordinary resources. The reserve is eligible only when all three ordinary identities are occupied. A full pool has no eligible allocation. Duplicate admission is classified before reserve misuse so an attempt to allocate an already occupied reserve remains a duplicate, not a scarcity finding.

A RELEASE indication must name an occupied identity and removes only that identity. Release order is entirely unrestricted. Importantly, a reserve that was legitimately admitted may remain occupied after ordinary resources return. The device does not demand preemption, migration, or reserve eviction. The policy constrains admission rather than every occupancy snapshot. Every occupancy subset is reachable by filling the whole pool legally and then returning selected identities.

Starting empty, allocate two is legal. Allocate one, release one, allocate one is also legal indefinitely, even if zero remains unused. Allocate two, zero, one, three is legal. Allocate two and then three is a reserve violation because ordinary identities zero and one remain free. After a legal full fill, releasing two while three remains occupied is legal. Releasing three and then immediately reallocating it while two remains free is a violation. Refilling two first makes that later reserve admission legal again.

The engine retains per-tag use counts and latest admission ordinals independently of occupancy. These counters survive returns and full drain but do not influence eligibility. At the first semantic or interface fault, interpretation halts permanently, retaining the previous coherent occupancy. The observed target operation may already have happened; the retained model is not a claim that the target refused it. Displayed eligibility after halt refers only to retained observer state and must never be used as authorization to continue the target workload.

## Repository-relative originality and reuse

The destination is jayis1/hacker-devices, whose configured remote default branch is main. The repository was clean and pulled before implementation. Existing device directories and README titles were inventoried, and reserve, fallback, and emergency allocation terminology was searched. Closest contracts were examined before choosing this design. No inspected device implemented this specific scarcity-gated reserve admission contract. This is a repository-relative originality statement, not a global prior-art, patentability, or novel-electronics claim.

Lowest Fit selects one minimum numeric free identity. Reserve Last permits all free ordinary identities; for example, an initial admission of two is legal here and rejected there. Bank Pack prefers partially occupied banks and permits either initial bank, whereas this design always withholds identity three until ordinary exhaustion. Admission Priority compares pending requesters at service admission, not pool capacity. Return Queue and Return Stack derive preferences from release order, which is irrelevant here. These are different accepted operation languages, not just renamed products.

The passive carrier, STM32 startup and register map, bounded UART transmitter, CRC implementation, acquisition qualification, and desktop transport deliberately reuse jayis1-authored Lowest Fit infrastructure. The reserve eligibility rule, sparse ordinary-vacancy barrier check, interface text, independent occupancy oracle, and operating contract are specific to this device. Internal UQ-prefixed symbols are inherited implementation names. The external marker is RZ1. This folder is self-contained and imports no sibling at runtime. No unified systems tree or unrelated repository is modified.

## Attack surface, threat model, and authorized use

Use only on equipment you own or have explicit written permission to assess. Authorization must cover enclosure access, diagnostic firmware modifications, common-ground wiring, approved workloads, possible interruption, recovery, and evidence retention. General network penetration-test authorization is not automatically permission for physical instrumentation. Start with a low-energy development fixture disconnected from production dependencies and consequential outputs.

The investigated attack surface includes fallback selection, cached resource availability, cancellation cleanup, wrong-bit updates, and mismatches between ordinary and reserve allocation paths. Authorized red teams can ask an owner to expose a suspected resource-policy regression through harmless workloads. Security researchers can compare firmware revisions under equivalent operation sequences. Penetration testers can reproduce the approved finding after remediation and attach qualified evidence to a report. The observer does not generate target workloads, inject faults, scan networks, extract secrets, or transmit exploits.

The trusted base includes the operator, target adapter, pin mapping, firmware image, purchased board, and workstation. A dishonest adapter can fabricate, omit, or reorder every operation. A compromised host can change exports. CRC protects against accidental corruption, not malicious forgery, authentication failure, or replay. There is no secure boot, signed evidence, encrypted storage, protected key, persistent boot identifier, or trusted wall clock. Timing records may still disclose operational information; keep exports local under engagement-specific retention and deletion procedures.

Receive-only is a property of the supplied firmware and app, not an irreversible electrical diode. Replacement firmware could drive target-facing GPIO. Even input-only attachment can disturb equipment through loading, shared grounds, shorts, or clamp back-power. Exclude mains-referenced circuits, vehicles, medical systems, occupied access controls, hazardous machinery, and unknown field wiring. Stop if attachment causes resets, heat, unexpected levels, or uncertain ground potential. Software tests and legal disclaimers cannot establish electrical safety.

## Hardware specification and bill of materials

The external active module is an ST NUCLEO-F401RE containing STM32F401RET6, a Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI. SysTick supplies nominal one-millisecond sampling. USART2 TX on PA2 emits 115200 baud, eight data bits, no parity, one stop bit through the module's ST-LINK virtual COM bridge. The purchased board supplies regulation, MCU decoupling, reset, USB, and SWD programming. These are external module facilities, not missing circuits on a claimed standalone MCU board.

The custom passive carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, and 0.3 mm tracks. Fourteen through-hole components connect thirty pads across nine nets. It uses short labeled leads, not a mechanically mating shield. No FPGA, radio, antenna, sensor, battery, charger, isolated supply, removable storage, or target power output is included. Use an insulating support, underside protection, and strain relief. Mounting holes and enclosure fabrication files are not supplied.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE; verify purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2, 2.54 mm through-hole header |
| J2 | 1 | Vertical 1x4, 2.54 mm through-hole header |
| Harness and support | 1 set | Insulated short leads, nonconductive base |

Resistor footprints use DIN0207 bodies with 7.62 mm lead spacing. Verify purchased dimensions, drill fit, and connector orientation. Reserve a 5 V, 500 mA USB source allowance for the external module as planning headroom, not measured draw or USB certification. The passive carrier contains no supply rail and cannot power the target. Supplier availability and price are not guaranteed.

| Signal | Target contact | MCU contact | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target connector's second pin is ground. Both J1 pins are ground, never supply inputs. J2 carries signals only; connect module ground separately to J1. Each target input passes through 1 kOhm to its MCU node, with 100 kOhm from that node to ground. A disconnected input appears low and cannot be distinguished from legitimate low state. There is no cable-integrity monitoring.

Support only known common-ground push-pull signals between zero and 3.3 V. Do not attach 5 V, 1.8 V, negative, differential, or unknown-voltage sources. Series resistance is not galvanic isolation, surge protection, or dedicated ESD suppression. Validate source impedance, ground offset, leakage, thresholds, and cable capacitance on the actual assembly. Power the observer before enabling target outputs; remove target signals before depowering the observer. USB and target grounds become common.

Manufacturer references are ST UM1724 for Nucleo-64 connectors and bridge configuration, RM0368 for STM32F401 peripherals, and the STM32F401xD/xE datasheet for electrical limits. The design inherits this platform mapping; confirm the purchased board revision and VCP solder bridges before construction. Reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . No fresh manufacturer-document revision review or electrical bring-up is claimed.

## Architecture and firmware

```text
Owned diagnostic adapter       Passive carrier             External Nucleo
TAG0 ---------------------------- 1k --+--------------------- PA0
TAG1 ---------------------------- 1k --+--------------------- PA1
ALLOCATE ------------------------ 1k --+--------------------- PA4
RELEASE ------------------------- 1k --+--------------------- PB0
                                      | each MCU node
                                     100k
                                      |
Target ground ----------------------- GND ------------------ module GND

GPIO -> continuity guard -> bundled tag and independent strobe qualification
     -> prior tag setup check -> reserve scarcity comparison
     -> transactional occupancy and counters -> RZ1 encoder
     -> USART2 TX -> ST-LINK USB serial -> Ledger / History / Integrity
```

GPIO internal pulls and UART reception are disabled. GPIOA and GPIOB are sampled separately, not atomically across ports. This is a slow diagnostic observer rather than a native-speed analyzer. The adapter must export every committed operation in order without hidden queue overflow. Instrumentation can perturb the target timing; if slowing the target removes the suspected issue, this technique may be unsuitable.

The C11 image uses no RTOS, dynamic allocation, vendor SDK, or network stack. Startup initializes data and BSS. The linker maps flash and SRAM with stack headroom. Board support configures GPIO, reset-cause capture, USART, SysTick, and the independent watchdog. The main loop samples when time advances, services at most one UART byte each iteration, and offers status at nominal 100 ms intervals. Policy announcements repeat nominally once per second so late attachment does not require the one-shot boot record.

The tag bits qualify as one bundle. ALLOCATE and RELEASE qualify independently so chatter on one strobe cannot indefinitely postpone the other's qualification. A candidate sample followed by three matching intervals establishes a qualified value. Acceptance requires five prior stable qualified tag observations, agreement with the current raw tag, and an asserted raw strobe. The opposite strobe must be low in the previous qualified, current qualified, and current raw states. Simultaneous rising edges count as two rejected observations, never invented ordering.

A conservative adapter sequence holds the selected tag with both strobes low for twelve nominal milliseconds, asserts one strobe for six, and returns both low for six. These are software-sampling allowances, not calibrated electrical specifications. Short pulses may be missed. Endpoint checks cannot reconstruct all subqualification glitches or order events occurring within one sample. Scope correlation is required before relying on a real adapter.

Repeated timestamps do nothing. Skipped acquisition ticks halt interpretation rather than invent continuity. Ordinary unsigned timestamp wrap is supported. Every sample operates on a speculative state copy; counter exhaustion discards all speculative changes before halting the prior coherent state. Interface errors precede semantic classification. After halt, accepted evidence freezes while time and transport diagnostics can advance. The watchdog continues being fed to retain evidence, but genuine reset or power loss still destroys volatile history.

## Companion and protocol

Run `python3 app/console.py` on Linux with Tk installed. No third-party Python packages are required. Enter an explicit serial path and select New live. Ledger shows occupancy, uses, stamps, reserve eligibility, and findings. History retains up to 256 accepted snapshots. Integrity shows source, live/replay provenance, parser errors, reset evidence, and freshness qualifications. These are functional screens with connection, replay, disconnect, and export actions, not static mockups.

RZ1 records contain ASCII unsigned decimal fields separated by commas, followed by a star, four uppercase hexadecimal CRC digits, and LF. CRC-16/CCITT starts at 0xffff. B records carry reset flags. P records carry `[3,1,5,4,3,1]`: qualification intervals, nominal sample period, setup requirement, pool size, designated reserve identity, and policy revision. S field order is declared in app/contract.py and emitted by firmware/wire.c. A failed serialization produces no partial frame.

Snapshot checks enforce occupancy and accounting conservation, bounds, unique latest admission ordinals, and fault plausibility. Admissions equal releases plus active population. Per-tag use counts sum to admissions. Attempts equal admissions plus releases plus rejected observations. Sparse transition checks derive each identity's releases from new admissions and changed membership. An ordinary identity that began free and received no admission stayed free throughout the interval; any new reserve admission across that interval is impossible, including hidden allocate-return cycles.

Conversely, an initially free ordinary slot may fill before reserve use, and an occupied reserve may remain live while ordinary slots return. The companion accepts those possibilities. Necessary endpoint checks cannot prove every hidden operation was legal or authenticate an initial snapshot. Two advancing compatible statuses establish transport progress, not truthfulness of target telemetry. Duplicate timestamps cannot refresh evidence. Counter rollback, contradictory duplicates, changed terminal evidence, later boot records, reception pauses, and excessive cumulative host/device drift freeze interpretation.

Disconnect retains evidence and restores serial settings. A second concurrent connection is refused. Replay opens only regular files up to two MiB, rejecting devices and FIFOs; incomplete trailing records count as damage. Export exclusively creates owner-only JSON files and refuses overwrites, preserving provenance and errors even without completed operations. There is no upload endpoint. Private file permissions are not encryption or secure erasure. Without persistent boot identity, some resets remain indistinguishable from advancing valid traffic.

## Reproduction, verification, and operation

From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py'
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

ARM GNU GCC and newlib build the Cortex-M4 ELF and binary. Host GCC runs sanitizer-backed engine, transmitter, and main-loop tests. Two independent reserve oracles enumerate repeated operation words through depth seven. Additional cases cover all occupancies, arbitrary ordinary choices, retained reserve after ordinary returns, repeated reuse, duplicates, orphan returns, simultaneous strobes, gaps, freeze, wrap, and transactional exhaustion. Desktop tests feed real C-encoder synthetic fixtures through the model and actual Tk screens; Linux pseudo-terminals exercise serial connection and hangup. Synthetic fixtures are not hardware measurements.

KiCad 9 and its standard footprints are required for regeneration and validation. Distro Python supplies pcbnew. The generator writes native schematic, routed PCB, project, symbol library, and connectivity manifest. The checker runs ERC and DRC and compares every schematic pin and PCB pad, component value, and footprint identity against the manifest and saved netlist. Passing passive-only ERC does not validate external supply behavior or the purchased MCU module. Build products and local KiCad preference files are ignored.

Before assembly, inspect routing, connector orientation, and real parts. Before attachment, continuity-check every net, verify absence of shorts, measure source voltage independently, and establish compatible ground. Flash through the purchased module's supported SWD process; programming is not performed by this release. Confirm reset-default clock assumptions. Start with all target resources independently known free, strobes low, and a newly reset observer. Wait for advancing compatible telemetry before beginning approved workloads. On a finding, stop, export evidence, and record target revision, adapter mapping, observer image, and independent corroboration. A clean run establishes only that the observed indication sequence met this narrow contract during the measured interval.
