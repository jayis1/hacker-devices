# Lowest Fit — Fixed-Rank Free-Resource Selection Witness

Author and creator: jayis1. Hardware revision A. Protocol LF1, policy revision 1.

Status: unbuilt engineering prototype. This package includes target-linked C firmware, a functioning Linux desktop companion, and native routed KiCad carrier sources. VERIFICATION.md records executed checks. Fabrication, physical programming, electrical qualification, calibrated timing and real-target interoperability are not claimed. This instrument observes a diagnostic contract; it is not an allocator, an enforcement boundary, a safety interlock or a security certificate.

Original firmware, application and tests: GPL-2.0-only. Original hardware utilities: MIT. Original carrier: CERN-OHL-S-2.0. Standard footprints, installed tools and purchased modules retain their upstream provenance and licenses.

## Purpose and practical utility

Lowest Fit investigates whether a small resource pool actually chooses its lowest-numbered free identity at every accepted admission. A pool may remain collision-free while its allocator violates this promise. An incorrect free-list insertion, stale bitmap, accidentally retained search cursor, or fast-path cancellation handler can select a higher slot while a lower slot is available. Aggregate occupancy and request completion totals do not reveal that contradiction. An independent observer can retain the last coherent occupied set and identify the selected tag that violated the rule.

A practical laboratory target is an embedded security service with four reusable request descriptors, privilege-context slots or audit buffers. The owner exports committed admissions and returns over a deliberately slow diagnostic interface. Harmless workloads exercise cancellation, full drain, immediate reuse and arbitrary completion order. Lowest Fit records only resource identities, counts and diagnostic timing: it does not read pointers, credentials, memory payloads or client traffic. The observer helps reduce a failing experiment to a reproducible allocation choice without providing an exploitation channel.

Numeric selection is not inherently more secure than another allocation policy. Its value here is conformance to a target's explicit contract. For example, a recovery procedure may assume that a reserved low-index descriptor is reused before capacity expands to another region. A finding can expose an undocumented exception or broken bookkeeping, but security impact must be established separately. The device neither proves memory erasure nor measures whether a chosen resource is physically isolated, cheaper, faster or safer.

## Exact measurement contract

The target has four unique, interchangeable identities numbered zero through three. The operator establishes that all are free at the beginning of the experiment. An ALLOCATE indication names an operation already committed by the target, not merely a requested operation that might be denied. If the selected identity is occupied, the observer reports a duplicate. Otherwise it must be the minimum numeric member of the current free set. Selecting any other free identity produces a lowest-free violation. If the pool is full, every attempted allocation is a duplicate.

A RELEASE indication must name a currently occupied identity and removes only that identity. Any occupied identity can return at any time and in any order. The next allocation uses the minimum of the new free set. There is no remembered search cursor, no preference for the most recently returned slot, no fairness rotation and no requirement to visit all four slots. A completely drained pool always permits zero and no other identity as the next admission.

Allocate zero, release zero, allocate zero is therefore legal. Repeating that sequence indefinitely is legal even while one, two and three remain unused. Allocate zero and then allocate two is a violation because one remains free. Fill zero, one, two and three; return three and one; then allocate one and three is legal. Return order does not assign priority. These examples distinguish the design from cyclic, least-used, least-recent and partial-bank-first selection.

The engine maintains occupancy separately from historical counters. Each accepted allocation increments a global ordinal, that identity's cumulative use count and its latest admission stamp. These values survive release and complete pool drain. They support consistency checking and reporting but never decide eligibility. The unique greatest nonzero stamp identifies the latest admission; it does not imply a moving priority. Reset loses volatile history and requires a new target baseline. Reconnecting the desktop does not reset the experiment.

At the first semantic or interface fault, interpretation halts. The rejected observation is counted but the previous coherent occupancy is retained. This is not a claim that the target rejected the operation: the target usually already performed it. The displayed eligibility after a halt describes the observer's retained model, not authorization to continue. Export evidence and investigate the mismatch rather than driving further target operations from that display.

## Repository-relative originality and provenance

The actual destination is jayis1/hacker-devices, with main as its configured remote default branch. Device directories and README titles were inventoried and lowest-free, lowest-number and first-fit terminology were searched. The closest contracts were read. An existing untracked lowest-fit/tests/test_engine.c was traced to an interrupted execution of this same authorized task. Its independent sorted-vacancy oracle was recovered and first run against the inherited engine; it failed before the selection rule was implemented.

Cursor Fit retains a cyclic search cursor across returns and complete drain. Lowest Fit restarts its numeric comparison at zero on every admission. Oldest Free compares admission recency; Lowest Fit ignores recency. Bank Pack allows either initial bank and prioritizes partial-bank occupancy; Lowest Fit allows only the minimum free identity. Rank Drain constrains release by maximum occupied rank and leaves admission unrestricted; Lowest Fit constrains admission and leaves release unrestricted. These differences produce different accepted traces, not merely different product names.

The passive carrier, startup, register map, acquisition qualification, bounded UART queue, CRC and desktop transport deliberately reuse jayis1-authored Cursor Fit infrastructure. The numeric admission rule, sparse lower-free barrier check, display, independent oracle and operating contract are device-specific. UQ-prefixed internal identifiers remain inherited implementation names. LF1 is the external protocol marker. The folder is self-contained at runtime and imports no sibling device or unified hardware tree. No worldwide novelty, patentability or new resistor-topology claim is made; fixed-priority allocation is established engineering.

## Attack surface, threat model and legal boundaries

Use only on equipment you own or have explicit written authorization to assess. Scope permission to opening enclosures, modifying diagnostic firmware, connecting grounds, running approved workloads, possible service interruption, recovery and evidence retention. A network penetration-test authorization is not automatically permission for physical instrumentation. Start with a low-energy development fixture whose production dependencies and consequential outputs are disconnected.

The investigated attack surface is resource bookkeeping: cached availability, cancellation paths, completion identity, wrong-bit updates, free-list ordering and unintended policy changes between slow and fast paths. An authorized red team can ask an owner to expose a suspected availability or isolation regression through harmless operations. Researchers can compare allocator revisions under the same reduced workload. Penetration testers can repeat the approved experiment after remediation and attach qualified evidence to a report. This device does not generate workloads, send target commands, discover networks or exploit vulnerabilities.

The trusted base includes the operator, diagnostic adapter, pin mapping, MCU image, purchased board and workstation. A malicious adapter can fabricate or omit every indication. A compromised workstation can alter exports. Physical programming access can replace the firmware. CRC detects accidental corruption, not authentication, malicious replay or forgery. There is no secure boot, persistent boot identity, signed evidence, encrypted export, trusted wall clock or authenticated updater. Timing metadata can still reveal operational information, so exports need engagement-appropriate access control and deletion rules.

The supplied firmware keeps target-facing pins as inputs and disables serial reception. The app opens the serial path read-only. Receive-only is a software property, not a certified data diode: replacement firmware could drive GPIO through the same resistors. Even an input can disturb a target through loading, back-power, shorts or common ground. Do not attach this unisolated prototype to mains-referenced equipment, medical devices, vehicles, occupied access controls, hazardous machinery or unknown field wiring. A disclaimer cannot replace electrical qualification.

## Hardware specifications and component selection

The active module is an external ST NUCLEO-F401RE with STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. The image assumes reset-default nominal 16 MHz HSI. SysTick supplies nominal one-millisecond sampling. USART2 sends 115200 baud, eight data bits, no parity and one stop bit through PA2 to the module's ST-LINK virtual COM bridge. The purchased module provides regulation, decoupling, reset, USB connectivity and SWD programming. These are external module facilities, not missing circuits on a purported standalone MCU board.

The custom passive carrier measures 80 by 70 mm with two copper layers, nominal 1.6 mm thickness and 0.3 mm tracks. Fourteen through-hole components connect thirty pads across nine nets. It connects by short labelled leads instead of mechanically stacking as a shield. No FPGA, radio, antenna, analog sensor, battery, charger, removable storage, isolated supply or target power output is included. Insulating supports, underside protection and strain relief are required; enclosure CAD and mounting holes are not supplied.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE; verify purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2, 2.54 mm through-hole header |
| J2 | 1 | Vertical 1x4, 2.54 mm through-hole header |
| Harness and support | 1 set | Short insulated leads and nonconductive base |

Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Verify actual body dimensions, drill fit and header orientation against purchased parts. Reserve a 5 V, 500 mA USB source allowance for the external module as planning headroom, not measured current consumption or USB compliance evidence. No present supplier availability or price is guaranteed. The carrier has no power rail and cannot power the target.

| Signal | Target contact | MCU contact | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target connector's second contact is ground. Both J1 contacts are ground, never supply inputs. J2 contains signals only; connect module ground separately to J1. Each target _IN net passes through 1 kOhm to its MCU node, with 100 kOhm from that node to ground. A disconnected wire appears low and cannot be distinguished from a legitimate low indication. Pulldowns do not provide cable-integrity monitoring.

Support only known common-ground push-pull signals between zero and 3.3 V. Exclude 5 V, 1.8 V, negative, differential and unknown-voltage sources. Series resistance is not isolation, surge suppression or dedicated ESD protection. There are no active translators or external clamps. Validate ground offset, source impedance, input thresholds, leakage and harness capacitance on the assembled fixture. Power the observer before enabling target outputs and remove target signals before depowering it to avoid clamp back-power. USB and target grounds become common.

Manufacturer references are ST UM1724 for Nucleo-64 connectors, bridge configuration and programming, RM0368 for STM32F401 peripherals, and the STM32F401xD/xE datasheet for electrical limits. The mapping and register definitions are inherited and checked locally against the carrier and code. Before construction, independently confirm the actual purchased board revision, VCP solder bridges and electrical limits. Reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned diagnostic adapter       Passive carrier           External Nucleo
TAG0 --------------------------- 1k --+-------------------- PA0
TAG1 --------------------------- 1k --+-------------------- PA1
ALLOCATE ----------------------- 1k --+-------------------- PA4
RELEASE ------------------------ 1k --+-------------------- PB0
                                      | each MCU node
                                     100k
                                      |
Target ground ----------------------- GND ---------------- module GND

GPIO -> continuity guard -> bundled tag + independent strobe qualification
     -> prior tag setup check -> minimum-free numeric comparison
     -> transactional occupancy, counts and stamps -> LF1 encoder
     -> USART2 TX -> ST-LINK USB serial -> Ledger / History / Integrity
```

Target-facing GPIO internal pulls are disabled. The two MCU ports are read separately; acquisition is not atomic across ports. This is a slow diagnostic observer, not a native-speed bus analyzer. The owner must validate an adapter that exports accepted operations in order without silently dropping or overflowing its own queue. Instrumentation can perturb the timing of the behavior being measured and must be considered in the interpretation.

## Firmware design and failure behavior

The C11 image has no RTOS, dynamic allocation, network stack or vendor SDK dependency. Startup initializes data and BSS. The linker maps flash and SRAM and reserves stack headroom. Board support initializes GPIO, reset-cause capture, USART, SysTick and the independent watchdog. The main loop samples when time advances, services at most one UART byte per iteration and offers status at nominal 100 ms intervals. Separate policy announcements repeat at nominal one-second intervals for late attachment; they do not masquerade as resets or clear evidence.

The two tag bits qualify as a bundle. ALLOCATE and RELEASE qualify independently, preventing unrelated strobe chatter from indefinitely postponing a peer's qualification. A candidate sample followed by three matching intervals establishes a qualified value. Acceptance requires five prior stable qualified tag observations, current raw tag agreement and the asserted raw strobe. The opposite strobe must be low in previous qualified, current qualified and current raw states. Simultaneous qualified rising edges are ambiguous and count as two rejected observations, not invented ordered operations.

Use a conservative adapter sequence: hold tag and both strobes low for twelve nominal milliseconds, raise exactly one strobe for six, then return both low for six. These durations describe software samples, not calibrated electrical limits. Short pulses may be missed or filtered. Endpoint checks cannot reconstruct all subqualification glitches or physical ordering within a sampling interval. Quiet startup does not prove an empty target pool; baseline establishment remains the operator's responsibility.

Repeated timestamps are ignored. A skipped acquisition tick halts semantic interpretation instead of fabricating continuity. Ordinary unsigned timestamp wrap is supported. Interface rejection precedes semantic checks; duplicate allocation precedes the lowest-free comparison. Every sample is evaluated on a speculative state copy. Counter exhaustion discards the speculative changes before halting the prior coherent state, avoiding half-credited allocations or releases. Timestamp and transport-failure diagnostics can continue after a halt, while accepted policy evidence freezes.

The encoder checks conservation before sending: admissions equal releases plus live population; attempts equal admissions plus releases plus rejected observations; cumulative per-tag uses sum to admissions. The watchdog remains fed after a policy halt so retained evidence is not deliberately erased by reset. Real power loss, brownout or reset still destroys volatile history. There is no low-battery state because the design has no battery. Host disconnection does not drive a target output or restart the observer.

## Companion application and protocol

Run `python3 app/console.py` on Linux with Tk installed. The app has no third-party Python dependencies. Enter an explicit serial path and choose New live; there is no automatic discovery or connection. Ledger displays occupancy, use counts, stamps, eligibility and findings. History shows up to 256 accepted snapshots. Integrity shows source, live/replay provenance, parser errors, reset information and connection/freshness qualifications. Selecting tabs, connecting, disconnecting, replaying and exporting are implemented actions, not mock screens.

LF1 records use ASCII unsigned decimal fields separated by commas, then a star, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT starts at 0xffff. B records contain reset flags. P records contain `[3,1,5,4,0,1]`: qualification intervals, nominal sample period, setup requirement, pool size, fixed lowest rank and policy revision. S field order is defined by app/contract.py and emitted identically by firmware/wire.c. Failed serialization returns no frame rather than submitting partial output.

Snapshot validation checks bounds, occupancy, conservation, unique latest admission ordinals and fault plausibility. Sparse transition validation derives per-tag releases from new admissions plus old occupancy minus new occupancy. A lower identity that began free and had no new admission remained free throughout the interval; no higher admission is possible across that barrier, including a hidden admission immediately followed by release. A lower busy identity may legitimately return before the next allocation. These are necessary endpoint checks, not proof of every hidden operation or authenticity of an initial snapshot.

Two advancing compatible statuses are required for live progress. Duplicate timestamps cannot refresh it. Contradictory duplicates, counter rollback, later boot records, impossible transitions, reception pauses and excessive cumulative host/device drift freeze interpretation. Drift allowance is capped so a slowly falling-behind reader cannot accumulate unlimited stale-data tolerance. An uncalibrated oscillator can trigger conservative failure. Without persistent boot identity, some resets remain indistinguishable from advancing traffic. Advancing halted timestamps indicate transport progress, not continued policy acquisition.

Disconnect preserves evidence and restores serial settings. A second simultaneous connection is refused. Replay accepts only regular files of at most two MiB, rejects incompatible empty input and counts truncated tails as damage. Parser recovery recognizes the complete LF1 marker. Export uses exclusive-create JSON with owner-only permissions, retaining provenance and errors even when no operations completed. Existing reports are never overwritten. No cloud upload or network listener is present. Use the host filesystem for approved retention and deletion.

## Build, verification and operating workflow

From this device directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py'
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

ARM GNU GCC and newlib produce the Cortex-M4 ELF and binary. Host GCC runs sanitizer-backed tests. KiCad 9 with standard footprints and pcbnew is required for CAD generation and validation; distro Python supplies those bindings. Tk and Xvfb support headless GUI tests. tools/generate_hardware.py regenerates native schematic, board, project, library and connectivity records. Review regenerated output and rerun electrical and design-rule checks. Build products and KiCad local preference files are ignored.

The engine tests compare repeated operation words through depth ten against an independent sorted list of vacant identities, rather than reusing the production eligibility calculation as the oracle. Additional cases cover every occupancy, repeated lowest-tag reuse, return order, duplicates, orphan returns, simultaneous strobes, raw endpoint disagreement, acquisition gaps, frozen terminal evidence, wrap and transactional exhaustion. Serial and main-loop tests exercise bounds, timeout recovery, lost boot records, periodic policy and watchdog startup failure. Desktop tests use actual C-encoder synthetic fixtures and Linux pseudo-terminals; they are not physical measurements.

Before fabrication, inspect connector orientation, actual footprints, routing, ground topology and the purchased module revision. Before attachment, continuity-check all nets, check shorts, independently measure source voltage and confirm compatible ground. Flash using the verified module's supported SWD process; physical programming was not performed for this release. Confirm reset-default clock operation rather than assuming an arbitrary bootloader left the correct clock tree.

Begin with all target resources independently known free, both strobes low and a newly reset observer. Wait for compatible advancing telemetry, then run only the approved workload. Capture independent target logs or waveforms for correlation. On a finding, stop the workload, export evidence and record pin mapping, target revision, observer image and known limitations. A clean report concerns only the exported indications and observed interval. It cannot establish global system security or replace the target owner's functional and electrical testing.
