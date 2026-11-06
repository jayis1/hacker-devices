# Oldest Free — Least-Recent Free-Resource Selection Witness

Author and creator: jayis1. Hardware revision A. Protocol OF1, policy revision 1.

Status: unbuilt engineering prototype. The firmware is target-compiled and linked, the desktop application is exercised with synthetic serial fixtures, and the passive carrier has native KiCad schematic and routed PCB sources. These checks are not fabrication, electrical qualification, programming, timing calibration, or physical target interoperability. See VERIFICATION.md for executed checks and remaining work.

Firmware, application and tests: GPL-2.0-only. Hardware utilities: MIT. Original carrier design: CERN-OHL-S-2.0. Installed tools and standard component footprints retain their upstream licenses and provenance.

## Purpose and practical utility

Oldest Free observes a four-resource allocator whose declared selection policy is to choose the least recently admitted resource among those currently free. It catches a subtle bookkeeping defect: releasing a resource can accidentally move it to the front of a free list, causing a recently used resource to be selected while an older free alternative remains available. The instrument independently retains the accepted admission history, observes return indications, and freezes evidence at the first contradictory selection. It does not allocate resources, impose the policy, or inspect the contents of the resources being allocated.

A useful laboratory target is an embedded service with four reusable request descriptors. The owner instruments actual descriptor admissions and returns using four slow diagnostic GPIO signals. A harmless workload exercises cancellation, completion, and immediate resubmission. A faulty recycler may treat completion order as allocation recency; this observer distinguishes those histories without collecting pointers, payloads, credentials, or application traffic. The owner can correlate a finding with source-level logs and an independent waveform capture to establish whether the exported events accurately represent the defect.

The contribution is a small standalone research instrument with an explicit free-set/last-admission contract, retained ordinal evidence, a transactional implementation, and independently tested application checks. Least-recent selection itself is an established algorithm. Originality here is relative to the inspected repository, not a worldwide prior-art search, patentability claim, or assertion that the resistor circuit is novel.

## Exact contract and distinguishing examples

At the start of an experiment all four resources must be known free and their experimental history must be reset. Each resource has an occupancy bit and a last-admission stamp. A stamp of zero means it has never been admitted during this experiment. A successful ALLOCATE increments a global admission ordinal, writes that ordinal to the selected resource, increments its cumulative use counter, and marks it occupied. Allocation is legal only when the selected resource is free and its stamp equals the minimum stamp among all free resources. Unused resources tie at zero, so any unused free resource may be chosen first.

RELEASE requires occupancy and clears only that identity. It does not refresh the stamp, increment the admission ordinal, or impose any release order. Complete drain does not forget history. Waiting creates no new eligibility. A busy resource is excluded from the selection comparison even if it is older than every free resource. If the pool is full there is no eligible allocation; a further allocation is a duplicate fault, not an opportunity to evict an occupant.

For example, allocate zero, release zero, then allocate zero is invalid while one, two and three remain unused and free. Allocate all four in order, release three, allocate three is valid because three is the only free identity. Releasing zero and one afterwards makes zero the oldest free candidate even though three has just been admitted again. Returning zero later than one does not make zero younger: recency is defined at admission, never at return.

This is not least-used selection. Hold three peers busy and repeatedly reuse the only free resource. That high-use resource can later become the oldest free identity after each peer is admitted more recently. It may then be selected even though its cumulative use count exceeds every peer's count. Likewise, no minimum time quarantine or minimum number of intervening admissions is imposed: immediate reuse is legal when no older alternative is free. A session does not claim balanced physical wear, fair treatment of clients, or memory safety.

## Repository comparison and recovered implementation

The default-branch repository was synchronized and its device directories inspected before completing this design. An interrupted oldest-free directory was traced to an earlier execution of the same authorized task, rather than silently adopting unknown local changes. The inherited partial implementation still used a fixed release-rank rule; the new least-recent allocation tests exposed that mismatch before correction.

The closest existing contracts were read. Wear Balance compares cumulative use counts among free resources; this design compares last-admission ordinals. Distance Renew requires two intervening admissions irrespective of which alternatives are free; this design chooses the oldest available alternative and permits immediate renewal when appropriate. Rank Drain constrains release by fixed numeric rank; this design permits every occupied resource to return in any order. The README search for least-recent and oldest-free selection found no existing equivalent device contract.

The passive carrier, startup, register map, CRC, bounded UART transport, acquisition qualification, and desktop transport reuse jayis1-authored infrastructure from Rank Drain. Reuse is intentional and is not presented as new electronics. Internal UQ-prefixed symbols are inherited implementation names; the OF1 framing and policy distinguish this device. Everything needed at runtime lives within this folder. There is no sibling import, shared systems tree, unified command interface, network service, or added automation workflow.

## Threat model, attack surface and legal boundaries

Use only on equipment you own or have explicit written authorization to assess. Permission must cover enclosure access, diagnostic firmware changes, common-ground attachment, workloads, service interruption, recovery, and evidence retention. A network penetration-testing authorization does not automatically authorize physical instrumentation. Begin with an expendable, low-energy development fixture whose hazardous outputs and production dependencies are disconnected.

The relevant attack surface is allocator bookkeeping: reversed free-list insertion, recency updates at the wrong boundary, clearing history when occupancy becomes empty, stale cached candidates, and wrong-identity updates during cancellation. An authorized red team can ask an owner to reproduce a suspected availability or resource-reuse weakness with harmless descriptors. Researchers can compare implementations and boundary conditions. Penetration testers can repeat the approved trace after remediation. The instrument contains no target-command transmitter, exploitation routine, covert persistence, credential capture, wireless interface, or automated workload generator.

Trust is placed in the operator, diagnostic adapter, physical mapping, MCU image and workstation. The adapter must report accepted operations, not rejected requests, and each tag must identify one unique resource rather than a class containing multiple simultaneous objects. All four identities must be interchangeable for the stated selection contract. Dynamic unavailability that is not represented as occupancy is unsupported. Dishonest or missing target indications can invalidate every conclusion.

CRC protects against accidental transport damage, not forgery. There is no secure boot, signed log, encrypted export, persistent boot identifier, authenticated update channel, or trusted wall clock. Physical ST-LINK access can replace the image. Receive-only describes the supplied firmware's target-facing GPIO configuration, not a certified data diode: other firmware could drive those pins. A clean observation is not proof that an uninstrumented target is secure. Do not connect this prototype to mains-referenced equipment, medical systems, vehicles, production access controls, dangerous machinery or unknown field wiring.

## Hardware specifications, power and BOM

The active module is an external ST NUCLEO-F401RE with STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. The target image assumes the reset-default nominal 16 MHz HSI oscillator. SysTick provides nominal one-millisecond sampling timestamps. USART2 sends 115200 baud, eight data bits, no parity, one stop bit through PA2 to the module's ST-LINK virtual serial bridge. The purchased module supplies regulation, decoupling, reset, USB connectivity and SWD programming. These functions are external module facilities, not missing circuitry on a claimed standalone MCU board.

The custom passive carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm tracks. Fourteen through-hole components connect thirty pads across nine nets. It connects to the Nucleo with short labelled leads rather than stacking mechanically on its headers. No FPGA, radio, battery, charger, analog sensor, removable storage or target power output is included. Use insulating supports, underside protection and strain relief; enclosure CAD and mounting holes are not provided.

| Reference | Quantity | Component specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, purchased revision checked |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2, 2.54 mm through-hole header |
| J2 | 1 | Vertical 1x4, 2.54 mm through-hole header |
| Harness and support | 1 set | Insulated short leads and nonconductive base |

Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Check actual component body sizes, drill fits and connector orientations before procurement. No current supplier stock or price is promised. Reserve a 5 V, 500 mA USB source allowance for the external module; this is planning headroom, not measured current consumption or USB compliance evidence. The carrier has no independent supply rail and cannot power the target.

| Signal | Target pin | Carrier MCU pin | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

The second pin of each target connector is ground. Both J1 contacts are ground, never supply inputs. J2 carries signals only; a separate module ground lead to J1 is required. Each target-side net ending in _IN connects through 1 kOhm to its MCU node, and that node has a 100 kOhm pulldown to ground. An unplugged wire is indistinguishable from a legitimate low indication. The pulldown prevents an intentionally floating input from being treated as a robust disconnection detector.

Only known common-ground, push-pull zero-to-3.3 V diagnostic signals are supported. Exclude 5 V, negative, 1.8 V, differential, and unknown field interfaces. Series resistors are neither galvanic isolation nor surge protection; dedicated ESD clamps and active level translation are absent. Check source impedance, ground offset, input thresholds, leakage, harness capacitance and signal loading on the assembled fixture. Power the observer before enabling target outputs and remove target signals before depowering it to prevent input-clamp back-power. USB and target grounds become common.

Manufacturer references are ST UM1724 for Nucleo-64 connectors, jumpers and VCP bridges; RM0368 for STM32F401 registers; and the STM32F401xD/xE datasheet for electrical limits. Verify the actual purchased revision before assembly. The inherited pin map is cross-checked among these design files, but a fresh manufacturer PDF retrieval failed during this run. Do not interpret that as an independent electrical review. Reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned diagnostic adapter       Passive carrier           Nucleo-F401RE
TAG0 --------------------------- 1k --+-------------------- PA0
TAG1 --------------------------- 1k --+-------------------- PA1
ALLOCATE ----------------------- 1k --+-------------------- PA4
RELEASE ------------------------ 1k --+-------------------- PB0
                                      | each MCU node
                                     100k
                                      |
Target ground ----------------------- GND ---------------- module GND

GPIO -> continuity guard -> tag bundle + independent strobe qualification
     -> prior setup check -> free-set minimum last-admission policy
     -> transactional occupancy, ordinals, counters -> bounded OF1 encoder
     -> USART2 TX -> ST-LINK USB serial -> Ledger / History / Integrity
```

The four target-facing GPIOs are inputs with internal pulls disabled. USART reception is disabled and the desktop opens serial read-only. There is no application command that changes a resource. PA0, PA1 and PA4 are read separately from PB0, so acquisition is not atomic across ports. This is a deliberately slow diagnostic observer, not a logic analyzer for native allocator clock rates. Owners must expose and verify a bounded low-rate event stream rather than connecting arbitrary internal buses directly.

## Firmware implementation and timing decisions

C11 firmware uses no RTOS, dynamic allocation, vendor SDK download or network stack. Startup initializes data and BSS; the linker defines flash and SRAM and reserves stack headroom. Board support configures GPIO, SysTick, USART, reset-cause capture and independent watchdog. The main loop samples when the millisecond timestamp advances, services at most one UART byte per iteration, and schedules status records at nominal 100 ms intervals when transport is available. Policy announcements repeat at nominal one-second intervals for late attachment. Boot records are separate and do not periodically erase host evidence.

The two tag bits qualify together as a bundle. ALLOCATE and RELEASE qualify independently so unrelated chatter cannot delay a stable peer indefinitely. Qualification requires a candidate observation followed by three matching intervals. Before accepting a strobe edge, the tag must have five prior stable qualified observations; the raw tag and relevant raw strobe must agree at the accepting sample. The opposite strobe must be low in the previous qualified, current qualified and raw states. Simultaneous rising strobes are ambiguous, not ordered by software preference.

A conservative adapter sequence is twelve nominal milliseconds with the tag selected and both strobes low, six milliseconds with exactly one strobe high, and six milliseconds with both low again. Tests use this sequence. These are observation intervals, not calibrated pulse specifications. Subqualification disturbances can be filtered; the firmware does not prove that no narrow pulse occurred between samples. Quiet startup wires do not prove an empty allocator: the owner must establish that baseline independently.

A repeated timestamp is ignored. A skipped acquisition tick halts interpretation instead of inventing continuity. Unsigned ordinary timestamp wrap is supported. Interface faults take precedence over semantic checks; duplicate allocation precedes recency comparison, and orphan release is independently checked. Any policy, interface or continuity fault retains the last accepted model and stops further semantic changes. Only timestamp and transport-failure diagnostics can advance after halt. A rejected observation does not mean the target rejected the underlying operation; it means the observer cannot extend its accepted history consistently.

Every sample updates a speculative state copy. Counter exhaustion discards the entire speculative transition and marks the old coherent state terminal, avoiding partial release or stamp updates. Admission, completion, rejection, occupancy-episode, overlapping-admission and per-resource counters support conservation checks before serialization. The watchdog is fed after a policy halt so the retained finding is not intentionally rebooted away. A real reset loses volatile history; export before restarting, then establish a new target baseline.

## Companion application and protocol

Run `python3 app/console.py` on Linux with Python Tk support. The application uses only the standard library. Enter a verified serial path and choose New live; no automatic device probing occurs. The three screens are Ledger, History and Integrity. Ledger displays occupancy, counters, stamps, eligible allocation identities and terminal findings. History retains up to 256 accepted snapshots. Integrity shows source, provenance, connection state, parse errors, reset evidence, policy compatibility and freshness qualifications.

OF1 records use ASCII decimal unsigned values, comma separators, a star, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT uses initial 0xffff. B carries reset flags. P carries `[3,1,5,4,0,1]`: qualification intervals, nominal sample period, setup requirement, resource count, unused stamp and policy revision. S fields are declared in `app/contract.py` in the same order emitted by `firmware/wire.c`. The encoder rejects internally inconsistent states and oversized buffers rather than submitting partial records.

The app rejects malformed bounds, invalid occupancy, duplicate nonzero stamps, impossible terminal findings and counter contradictions. Across sparse snapshots it requires stamp changes to have matching admissions, derives per-tag releases, and rejects renewal that bypasses a continuously free older peer. These are necessary consistency checks, not complete reconstruction of hidden operations. A first snapshot cannot establish a previously unobserved history. Valid CRC and plausible endpoints do not authenticate the target or prove every intermediate selection legal.

Two advancing compatible status records are required before live progress is asserted. Duplicate timestamps do not renew freshness. Clock rollback, counter rollback, contradictory duplicates, later boot records and impossible transitions freeze interpretation. Reception pauses and excessive cumulative host/device timing divergence require reconnect rather than relabeling backlog as fresh. The drift allowance is bounded; uncalibrated oscillator behavior can cause conservative disconnection. A reset whose counters and timestamp remain indistinguishable from forward progress cannot always be detected without a persistent boot identity.

Disconnect preserves evidence and restores serial settings. A second live open is refused. Replay is limited to regular files of at most two MiB and counts a truncated trailing record as an error. Corrupted-prefix recovery resynchronizes at the complete OF1 marker. Exports are exclusive-create JSON files with owner-only mode and retain provenance, errors, stale state and reset qualifications even without completed operations. Export never silently overwrites an existing report. No cloud upload occurs; use the host filesystem to delete retained reports according to the approved retention policy.

## Build, checks and operator procedure

From this directory run:

```sh
make -C firmware all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py'
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

Target compilation requires ARM GNU GCC and newlib; host tests require GCC with sanitizers. CAD generation requires KiCad 9 Python bindings, libraries and kicad-cli; the distro Python is used because bindings may not be installed in a virtual environment. App tests require Tk and Xvfb. Build artifacts and local KiCad preferences are ignored. Do not replace target compilation with a host-only syntax check.

The engine tests compare repeated operation words against an independent move-to-back identity-list oracle, rather than reusing the production minimum-stamp algorithm. Additional tests exercise arbitrary return order, busy older identities, immediate only-free reuse, distinction from frequency, qualification faults, overflow rollback, timestamp wrap and frozen evidence. Transport tests cover bounds, timeout and recovery. Main-loop tests cover lost initial boot transmission, periodic compatibility records and watchdog startup failure. Application tests use both real C-encoder fixtures and checksum-valid impossible records, with actual Tk event loops and Linux pseudo-terminals. These fixtures are synthetic, not measured hardware traces.

Before fabrication, open the native schematic and PCB, inspect connector orientation and footprints, run the supplied connectivity checker, and review the manufacturer's actual board revision. Before attachment, continuity-check every net, inspect shorts, confirm ground conditions and verify zero-to-3.3 V signals with independent equipment. Program the linked image using the module's supported SWD tooling only after confirming the exact module and power setup. Flashing was not performed in this run.

Begin an experiment with target resources reset and free, both strobes low, and the observer freshly reset. Verify compatible policy reception and advancing status before exercising the approved workload. Capture target logs and waveforms alongside observer evidence. At the first finding, stop the workload, export the report, document the tag mapping and investigate. Disconnecting the application alone does not reset observer history; resetting the observer alone does not establish a new target baseline. A repeatable reduced reproduction with explicit qualifications is the deliverable, not a blanket claim of target security.
