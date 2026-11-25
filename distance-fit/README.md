# Distance Fit — Nearest-Resident Separation Witness

Author and creator: jayis1. Hardware revision A. Protocol DF1, policy revision 1.

Status: unbuilt engineering prototype. Firmware is target-linked, the Linux companion is exercised with synthetic serial traffic, and the passive carrier has native routed KiCad sources. See VERIFICATION.md for executed checks. Fabrication, physical programming, electrical bring-up, calibrated timing and real-target interoperability remain unverified. This is an observation instrument, not an allocator, protective interlock or security certification.

Original firmware, application and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier: CERN-OHL-S-2.0. Installed development tools, standard footprints and purchased modules retain their upstream provenance and licenses.

## Purpose and practical value

Distance Fit checks a spatial placement promise: each newly occupied resource must maximize its distance to the nearest currently occupied resource. The instrument follows four ordered resource identities using a two-bit identity bus and separate allocation and return indications. It retains the first contradictory admission with cumulative accounting and the preceding coherent occupancy. It does not read memory addresses or application payloads. Its question is placement at admission, not whether two resources can ever be adjacent.

A practical experiment is an owner-instrumented embedded gateway with four ordered staging locations. The owner has specified a spread-first placement policy to defer adjacent occupancy while more separated options remain. A stale candidate cache, incorrect distance calculation, circular wrap assumption or cancellation bug can violate that policy without producing duplicate ownership. Ordinary capacity counters cannot distinguish those cases: the number of available locations remains correct. Distance Fit supplies a small independently implemented witness that can accompany a reduced reproduction on a bench.

The four locations are diagnostic identities, not automatically physical memory banks, cache sets or isolation domains. The owner must justify the identity ordering and the relevance of the declared placement contract. A clean trace does not prove reduced coupling, confidentiality, memory safety or resistance to fault injection. Maximin selection is established engineering, not a newly invented mathematical algorithm. This device's contribution is a specific observable contract, bounded acquisition, retained evidence and a practical comparison workflow.

## Exact policy and examples

The topology is the line zero—one—two—three. Endpoints do not wrap. Before each admission, calculate the selected free identity's minimum absolute index difference from every occupied identity. Compare that score with the corresponding score of every other free identity. Admission is legal precisely when no other free identity has a larger score. Equal maximum scores tie. In an empty pool every identity is eligible; a full pool has no eligible identity. Returns can remove any occupied identity in any order.

Starting empty, admitting one is legal. With one occupied, free zero and two are each distance one away, while three is distance two away. Only three is eligible. After admitting three, both zero and two have nearest-resident distance one, so either is eligible. Starting with zero instead makes three the next required identity. A legal full admission order is zero, three, one, two. This demonstrates that adjacency eventually becomes unavoidable and is not itself a violation.

Returns can revoke eligibility. With zero and three occupied, one and two tie. Return three while leaving zero resident: three becomes the unique farthest vacancy, so one is no longer eligible. Conversely, returning the only resident restores unrestricted choice. Repeated admission and return of one in an otherwise empty pool is legal indefinitely. There is no elapsed cooldown, lifetime quota, fairness requirement, rotating cursor or mandatory coverage of every identity.

A duplicate admission is checked before spatial preference. An orphan return is a separate finding. After a contradiction the observer freezes semantic reconstruction, keeping the last coherent occupancy and counting the rejected observation. This does not imply that the target rejected its operation: the diagnostic adapter reports committed operations. The target may now have a different state. Displayed eligibility after halt is explanatory evidence, never authorization to continue operating the target.

## Repository distinction and provenance

The destination is jayis1/hacker-devices on its configured default branch main. Existing directories were listed before work; the interrupted distance-fit implementation was identified in the originating task transcript and recovered rather than duplicated. Repository README searches for maximin, farthest and nearest-resident terminology found no existing documented equivalent. The closest Edge Fit, Gap Fit and Distance Renew contracts were read before completion. Novelty is repository-local, not a claim of worldwide originality or patentability.

Edge Fit permits only endpoints of a maximal free interval and rejects an interior first admission. Distance Fit permits every first admission. Gap Fit chooses a smallest free interval: after admitting one it requires zero, whereas Distance Fit requires three. Distance Renew measures intervening admission ordinals before reuse; Distance Fit measures current index separation and permits immediate reuse after complete drain. These are different accepted histories, not merely alternative labels for the same policy.

The resistor carrier, register-level platform, UART driver, qualification machinery and desktop transport derive from jayis1-authored Edge Fit infrastructure. Reusing these known electrical interfaces is intentional; the carrier circuit is not claimed as a novel invention. Device-specific elements are the nearest-resident predicate, distance display, bounded sparse-history checks, independent wavefront oracle and operating contract. Internal UQ-prefixed names remain implementation details. DF1 is the public protocol marker. The folder is self-contained at runtime and adds no unified hardware tree or shared CLI framework.

## Attack surface, trust model and authorized use

Use only on equipment you own or have explicit written authorization to assess. Written scope should cover diagnostic firmware changes, enclosure access, common-ground wiring, permitted workloads, interruption, recovery and evidence retention. Network assessment permission alone does not authorize physical instrumentation. Begin with an expendable low-energy development fixture disconnected from production dependencies and hazardous outputs. Stop if attachment changes voltage, resets, temperature or expected behavior.

The investigated attack surface is allocator bookkeeping and placement selection across admission, cancellation and reuse. An authorized red team can ask the owner to expose a harmless reduced trace behind a suspected availability or separation-policy weakness. Researchers can compare correct and deliberately defective fixture allocators. Penetration testers can repeat an approved workload after remediation and retain comparable evidence. The observer generates no workload, exploit traffic, target command, credential request or radio transmission.

Trusted components include the operator, identity mapping, diagnostic adapter, observer image, MCU module, harness and workstation. The adapter must emit every committed operation in order. Missing indications can invalidate reconstruction without producing an electrical acquisition gap. A malicious target can emit plausible diagnostics unrelated to its actual internals. A malicious serial source can forge valid checksums. CRC detects accidental corruption, not authentication. Interpret both clean traces and findings alongside independent source review and waveform evidence.

Receive-only describes this firmware's target-facing GPIO configuration, not an immutable data diode. Replacement firmware could drive through the series resistors. There is no secure boot, signed telemetry, persistent boot identifier, secure element, encrypted transport, remote updater or authenticated pairing. Physical SWD permits image replacement. The companion opens no network listener and does not upload exports. Timing and operation metadata can still be sensitive; apply engagement-specific access controls and deletion rules.

Never attach this unisolated prototype to mains-referenced circuits, medical equipment, vehicles, deployed access controls, moving machinery or unknown field wiring. A common ground can disturb a target even without software writes. A disclaimer is not an electrical protection component and does not establish regulatory compliance.

## Hardware specification and bill of materials

The active processor is an external ST NUCLEO-F401RE development module with STM32F401RET6, Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI, nominal one-millisecond SysTick and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. PA2 routes toward the ST-LINK virtual COM interface. The purchased module provides USB, regulation, decoupling, reset and SWD. These are external module facilities, not missing components on a purported standalone MCU board.

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness and 0.3 mm tracks. Fourteen through-hole components provide thirty pads on nine nets. Short labelled leads connect the module; this is not a mechanically stacking Arduino shield. There is no radio, antenna, FPGA, battery, charger, storage card or analog sensor. No enclosure or mounting-hole design is supplied. Use nonconductive supports, underside insulation and strain relief.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | NUCLEO-F401RE, inspect purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness and support | 1 set | Insulated short leads and protected fixture |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Headers use standard KiCad pin-header footprints. Verify actual body size, drill fit and connector orientation against purchased components. USB supplies the external module; reserve a 5 V, 500 mA source as a conservative planning allowance, not measured consumption. The carrier has no power supply rail and cannot power a target. Input pulldown loading and module consumption require measurement during bring-up, not inference from ERC.

| Diagnostic | Input connector | Module harness | Nucleo endpoint |
| --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 |

Every target header's second pin is ground. Both J1 pins are ground, never power. J2 contains signals only; separately connect module ground to J1. Each target-side _IN net reaches its MCU-side net through 1 kOhm and has a 100 kOhm MCU-side pulldown. A disconnected signal tends low but is indistinguishable from a legitimate low. Cable continuity is not monitored.

Only known common-ground zero-to-3.3 V push-pull diagnostic outputs are supported. Negative, 5 V, 1.8 V, differential and unidentified signals are unsupported. There are no level shifters, TVS parts, fuses, qualified clamps or isolation barriers. Power the observer before enabling target outputs; connect ground before signals. Disable outputs before removing observer power to avoid back-powering. Series resistance does not authorize exceeding MCU ratings.

Consult ST UM1724 for CN8 and USART bridge routing, RM0368 for registers, and the STM32F401xD/xE datasheet for electrical limits. Manufacturer manual: https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf . Check the actual revision and SB13/SB14 versus SB62/SB63 configuration before relying on virtual COM routing. Passive-only ERC cannot validate the purchased module, external source or harness.

## Architecture and firmware decisions

```text
Owner's committed operations       Passive carrier         NUCLEO-F401RE
TAG0 ------------------------------ 1k --+----------------- PA0
TAG1 ------------------------------ 1k --+----------------- PA1
ALLOCATE -------------------------- 1k --+----------------- PA4
RELEASE --------------------------- 1k --+----------------- PB0
                                         | each channel
                                        100k
                                         |
Target GND ---------------------------- GND ---------------- USB GND

GPIO -> continuity guard -> bundled tag / independent strobe qualification
     -> prior setup check -> occupancy -> nearest-resident distance maximum
     -> transactional ledger -> DF1 CRC encoder -> bounded UART transmit
     -> ST-LINK virtual COM -> Ledger / History / Integrity desktop screens
```

GPIOA and GPIOB are read sequentially, not atomically. This is slow diagnostic instrumentation rather than instruction-rate trace. Begin with twenty milliseconds of stable tag, twenty milliseconds selected strobe high and twenty milliseconds both strobes low. Check actual waveforms before reducing margins. Pulses entirely between samples can be invisible. Simultaneous allocation and return indications are rejected as ambiguous rather than assigned an invented ordering.

The tag qualifies as a two-bit bundle; the strobes qualify independently so unrelated chatter cannot postpone a stable peer indefinitely. Each group needs three unchanged intervals after its initial candidate. At the operation edge the tag must already have five stable qualified intervals. Raw tag must match qualified tag, the selected strobe must remain raw high, and the opposite strobe must be clear in previous, current and raw states. WAIT exits only after qualification with both strobes low. That electrical baseline is not proof of an empty target.

The engine uses fixed-size state, bounded loops and no heap allocation. Each observation is applied to a temporary ledger and committed only if counters remain representable. Exhaustion halts without publishing half an operation. A missed millisecond halts rather than treating unobserved time as stability. Following halt only device time and permitted transport diagnostics advance. Reset clears volatile evidence, so the operator must independently restore target baseline before beginning another experiment.

The independent watchdog has bounded startup waits; failure produces terminal evidence before initial status. The UART driver has a bounded buffer and timeout rather than blocking acquisition while a receiver is absent. Startup copies initialized data, clears BSS and provides the reset and SysTick vector paths. registers.h documents the limited RM0368 register subset instead of requiring a vendored vendor library. Build products and local KiCad preferences are excluded from version control.

## Protocol and companion application

Frames begin DF1, carry a record kind and unsigned decimal fields, and end with an asterisk, four uppercase CRC hex digits and LF. CRC-16 uses polynomial 0x1021 and initial 0xffff over bytes preceding the asterisk. B reports reset flags once. P periodically announces qualification, tick, setup, slot count, topology and policy revision as 3,1,5,4,0,1. S reports the ordered fields in app/contract.py. The encoder and parser share the documented contract, not runtime code.

Status includes uptime, qualified levels, validity, halt reason, occupancy, depth, observed samples, attempts, admissions, returns, rejections, gaps, peak occupancy, per-tag uses and last admission ordinals. Attempts equal admissions plus returns plus rejected observations. Admissions equal returns plus live population. Cohort closure and overlapping admissions provide additional conservation checks. Fault bits distinguish distance preference, orphan return, duplicate admission, interface ambiguity, acquisition gap, exhaustion and watchdog startup.

Run python3 app/console.py on Linux with Tk installed. The Ledger screen shows retained counters, vacancy distances, eligible tags and findings. History retains up to 256 snapshots. Integrity shows provenance, transport state, parser errors, reset indicators, age and rejection messages. New live opens the selected serial path read-only; Disconnect retains evidence. Replay accepts bounded regular-file recordings. Export JSON writes a new private file and refuses overwrite. Export is a report, not the raw wire recording format.

The companion rejects invalid syntax, checksums, bounds, contradictory duplicates, counter rollback and impossible state relationships. Sparse transition validation derives each tag's returns from admission and occupancy changes. Continuously occupied and continuously vacant identities constrain possible intermediate occupancy. For at most eight operations a finite count-constrained search checks possible legal orderings; larger gaps use necessary conservative bounds. An accepted endpoint pair is not proof of the actual hidden order or of every timing constraint.

Freshness requires connection, compatible policy, advancing acquisition and recent progress. Duplicate timestamps do not refresh it. Excessive reception pauses or host/device progress divergence freeze interpretation until a new session. Timing is heuristic: nominal internal oscillator drift, buffered transport and reset aliasing cannot establish absolute acquisition provenance. There is no persistent boot identity. A lost reset record with otherwise plausible forward counters may remain undetectable. Preserve independent experiment logs when this distinction matters.

## Build, validation and first-use procedure

Install GNU Arm Embedded GCC with newlib, host GCC, Python 3 with Tk, KiCad 9 with pcbnew bindings, and Xvfb for headless GUI tests. From this directory run:

    make -C firmware all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py

The CAD generator is tools/generate_hardware.py and uses the system Python with pcbnew. It overwrites generated carrier sources, so review changes before regeneration. The hardware checker runs ERC and DRC, exports a fresh schematic netlist, and compares every component value, footprint identity, pad and net against both the PCB and connectivity manifest. The firmware test oracle expands occupied wavefronts rather than duplicating the nearest-distance implementation. Tests include all occupancy masks, repeated operations, arbitrary returns, policy violations, malformed evidence and host transport failures.

Before physical programming, inspect the PCB and schematic, verify continuity and shorts with power removed, and confirm each connector against the actual module. Flash the linked ELF through the module's physical ST-LINK using a locally approved programmer. No hardware programming was performed for this release. With target disconnected, confirm clock, serial output, reset behavior and input voltage limits. Check the passive network under controlled low-energy signals before connecting any owned target fixture.

For an experiment, establish target emptiness independently, reset the observer, hold both strobes clear, and verify TRACK without faults. Apply a known legal sequence followed by an intentionally incorrect harmless fixture operation. Compare waveform timing, displayed identity and retained first fault. Export before any reset. Repeat with the legal implementation and compare equivalent workload traces rather than merely counting fewer alarms. Keep firmware version, target revision, wiring map and operator authorization alongside the report.

Physical bring-up must additionally evaluate input loading, oscillator tolerance, ground offsets, USB disconnects, watchdog reset, wiring faults and power sequencing. Host tests cannot establish electrical safety or actual resource behavior. No fabricated measurements, compliance claims or claimed production readiness accompany this design.
