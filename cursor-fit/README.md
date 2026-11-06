# Cursor Fit — Retained-Cursor Free-Resource Selection Witness

Author and creator: jayis1. Hardware revision A. Protocol CF1, policy revision 1.

Status: unbuilt engineering prototype. The package provides target-linked firmware, a working Linux desktop companion, and native routed KiCad carrier sources. Software and CAD verification are recorded in VERIFICATION.md. Fabrication, MCU programming, electrical qualification, calibrated timing, and physical target interoperability have not been performed. This device observes a diagnostic contract; it is not an allocator, security enforcement boundary, safety interlock, or certification instrument.

Original firmware, application and tests: GPL-2.0-only. Original hardware utilities: MIT. Original carrier design: CERN-OHL-S-2.0. Installed tools, purchased modules and standard footprint libraries retain their upstream provenance and licenses.

## Purpose and practical utility

Cursor Fit investigates an allocator fault that can remain invisible to occupancy-only checks: a four-resource pool promises cyclic next-free selection, but its remembered search position is reset, moved by release, or bypassed by a cancellation path. Every individual resource can remain exclusively owned while the selection policy is wrong. The device independently records accepted admission order and occupancy, derives the retained cursor, and stops at the first incompatible indication. It does not inspect memory contents or make allocation decisions for the target.

A practical target is an embedded security service with four reusable request descriptors, audit buffers, or isolated processing slots. The owner exports actual accepted admissions and releases through a deliberately slow GPIO diagnostic adapter. A harmless workload exercises completion, cancellation, full drain, and immediate resubmission. The observer can distinguish an accidentally restarted first-fit scan from the intended retained-cursor policy. It also distinguishes duplicate admission and orphan return from a selection error, preserving a useful reduced reproduction without recording credentials, payloads, addresses, or user identities.

The useful contribution is the combination of a precise measurement contract, independent occupancy and cursor evidence, fault-retaining firmware, a bounded serial interface, an executable desktop validator, and reproducible tests. Cyclic allocation itself is established engineering. Originality is relative to the inspected repository; no worldwide prior-art search, patentability claim, or claim of novel resistor electronics is made.

## Exact measurement contract

The pool contains exactly four interchangeable identities numbered zero through three. At the experimental baseline all are free and the target's next-free search cursor is zero. On each accepted ALLOCATE, scan identities in cyclic ascending order starting at the retained cursor and skipping occupied identities. The first free identity is the only legal selection. After accepting it, move the cursor to the selected identity plus one, wrapping after three. If all identities are occupied, there is no eligible selection.

RELEASE requires that its identity currently be occupied. It clears that identity without moving the cursor. Any occupied identity may return in any order. Complete pool drain, idle time, and the fact that a returned identity is numerically smaller than the cursor do not reset the search. A new observer run requires a new known target baseline; reconnecting the workstation is not a target reset.

The implementation stores a global admission ordinal and each identity's latest accepted admission stamp. The unique stamp equal to the global ordinal identifies the most recent admission, from which the next cursor is derived. Zero admissions imply cursor zero. This avoids a second cursor variable that could disagree with retained history. Per-identity lifetime use counts remain separate from last-admission stamps. Both survive release and complete drain, but neither survives observer power loss.

For example, allocate zero, release zero, then allocate zero is a fault: the search now starts at one, which is free. Allocate all four in order, release three, allocate three is legal because three is the only free candidate; skipping busy resources is required. After filling the pool, renew one, renew three, then release one and two. The cursor is zero, zero remains busy, and one must be selected even though two was admitted less recently. These examples are executable regressions, not merely descriptive illustrations.

The device does not enforce minimum reuse time, minimum intervening admissions, least-frequency selection, least-recent selection, arbitrary fairness across clients, or secure memory erasure. It assumes unavailability is represented by occupancy. A temporarily disabled free slot, variable-size allocation, hidden reserved identity, or priority exception requires a different contract. Indications must describe committed operations rather than requests that may later be rejected.

## Repository comparison and provenance

The configured repository is jayis1/hacker-devices, and its default branch is main. Directories and README device titles were inventoried, selection-policy terminology was searched, and the closest contracts were read. The untracked cursor-fit directory was traced to an interrupted execution of this same authorized task and recovered rather than mistaken for unrelated work or replaced with a second device.

Oldest Free chooses the least-recently admitted free identity, whereas Cursor Fit follows numeric cyclic order from its retained cursor. Baton Order requires a strict four-domain ownership cycle and does not represent a concurrently occupied resource pool with busy-slot skipping. Contention Turn observes demand-aware arbitration between two requesters, learns service preference from grants, and can reseed after ambiguity; this design begins at an explicitly reset cursor and permanently stops correlation after loss. Those differences produce distinct accepted traces and distinct findings.

The passive carrier, register definitions, startup, bounded UART, CRC, acquisition qualification, and desktop transport reuse jayis1-authored infrastructure from Oldest Free. Reuse is deliberate. The new policy engine, cursor eligibility, sparse transition bounds, oracle tests, and workflow are the device-specific work. Internal UQ-prefixed C names are inherited identifiers; the CF1 marker and policy tuple define the external protocol. Runtime files are self-contained in this folder and do not import sibling devices or a shared systems tree.

## Threat model and authorized use

Use only on equipment you own or have explicit written authorization to assess. Scope permission to enclosure access, diagnostic firmware changes, common-ground connection, allowed workloads, service interruption, recovery, and evidence retention. Network penetration-testing permission does not automatically authorize physical instrumentation. Start with a low-energy development fixture with consequential outputs and production dependencies disconnected.

The investigated attack surface is allocator bookkeeping: resetting the search position on empty-pool transitions, updating it during return rather than admission, stale cached selections, wrong-identity completion, and retry paths that bypass normal selection. Authorized red teams can ask the owner to reproduce suspected resource-reuse or availability weaknesses using harmless descriptors. Security researchers can compare cancellation implementations and boundary behavior. Penetration testers can repeat the same approved trace after remediation and attach the qualified evidence to a report.

The trusted base includes the diagnostic adapter, tag mapping, observer image, external MCU module, operator, and workstation. A dishonest target can fabricate all indications. A compromised host can alter reports. Physical programming access can replace firmware. CRC detects accidental damage, not authentication, replay attacks, or malicious forgery. There is no secure boot, signed evidence, persistent boot identifier, trusted wall clock, encrypted local store, or authenticated updater. Sensitive operational timing can remain in otherwise content-free reports; minimize retention and keep exports under the engagement's access controls.

The supplied firmware leaves target-facing pins as inputs and has no target command parser. There is no radio, network service, credential collector, exploit routine, fault injector, or autonomous workload generator. Receive-only describes software configuration, not an irreversible electrical data diode: alternative firmware could drive GPIO. Even input-only attachment can disturb a target through loading, common ground, shorts, or back-power. Do not attach this unisolated prototype to mains-referenced equipment, vehicles, medical systems, occupied access controls, dangerous machinery, or unknown field wiring. A legal disclaimer does not establish electrical safety.

## Hardware specifications and bill of materials

The active module is an external ST NUCLEO-F401RE containing an STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI operation. SysTick supplies nominal one-millisecond acquisition timestamps. USART2 transmits at 115200 baud, eight data bits, no parity, and one stop bit through PA2 to the module's ST-LINK virtual serial bridge. The purchased module provides regulation, decoupling, reset, USB connectivity, and SWD. These are external module functions, not omitted circuits on a purported standalone MCU PCB.

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm routed tracks. It has fourteen through-hole components, thirty pads and nine nets. It connects to the module by short labelled leads rather than stacking as a shield. No FPGA, radio, analog sensor, removable storage, battery, charger, or target power output is included. Use insulating supports, underside protection and strain relief. Enclosure drawings and mounting holes are not supplied.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External board | 1 | ST NUCLEO-F401RE; verify purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2, 2.54 mm through-hole header |
| J2 | 1 | Vertical 1x4, 2.54 mm through-hole header |
| Harness and support | 1 set | Short insulated leads and nonconductive base |

Resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. Check physical body size, drill fit, header orientation and supplier availability before buying parts. Reserve a 5 V, 500 mA USB source allowance for the module as planning headroom, not a measured consumption or compliance claim. The carrier has no supply rail, no regulator to thermally qualify, and no capacity to power the target.

| Signal | Target contact | MCU-side contact | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Each target connector's second pin is ground. Both J1 pins are ground, never supply inputs. J2 carries signals only; connect module ground separately to J1. Each target-side _IN net connects through 1 kOhm to its MCU node, with 100 kOhm from that node to ground. A disconnected wire looks like a low indication, so this network cannot prove cable continuity.

Support only known common-ground, push-pull zero-to-3.3 V diagnostic signals. Exclude 5 V, 1.8 V, negative, differential and unknown-voltage interfaces. Series resistance is not isolation, surge suppression, or dedicated ESD protection. No active level translation or ESD clamps are fitted. Verify source impedance, input thresholds, ground offset, leakage and harness capacitance on the assembled fixture. Power the observer before enabling target outputs; remove target signals before depowering the observer to avoid clamp back-power. USB and target grounds become common.

Manufacturer references are ST UM1724 for Nucleo-64 connectors, solder bridges and programming, RM0368 for STM32F401 peripheral registers, and the STM32F401xD/xE datasheet for electrical limits. The inherited mapping is checked against code and CAD, but a fresh manufacturer PDF download timed out during this run. Independently confirm the actual board revision, VCP bridge configuration and electrical limits before construction. Reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf .

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
     -> prior setup guard -> retained-cursor first-free comparison
     -> transactional occupancy, ordinals, counts -> bounded CF1 encoder
     -> USART2 TX -> ST-LINK USB serial -> Ledger / History / Integrity
```

Target-facing GPIO internal pulls are disabled. USART reception is disabled, and the application opens serial read-only. PA0, PA1 and PA4 are acquired in a different port read from PB0, so the device does not promise atomic cross-port sampling. It is a slow diagnostic observer, not a native-speed bus analyzer. The owner must validate an adapter that exposes meaningful operations without overflowing or silently dropping its own diagnostic queue.

## Firmware decisions and failure behavior

The C11 firmware has no RTOS, dynamic allocation, network stack or vendor SDK dependency. Startup initializes data and BSS; the linker maps flash and SRAM and reserves stack headroom. Board support initializes GPIO, reset-cause capture, SysTick, USART and independent watchdog. The main loop samples when time advances, services one UART byte per iteration, and offers status records at nominal 100 ms intervals. Policy announcements repeat at nominal one-second intervals so a late-attaching companion can establish compatibility without a reboot. Boot records remain separate from those periodic announcements.

The two tag bits qualify as one bundle. The allocation and release strobes qualify independently, so chatter on one cannot indefinitely postpone the other's qualification. A candidate observation followed by three matching intervals establishes a qualified value. A strobe needs five prior stable qualified tag observations; current raw tag and strobe must agree at acceptance. The opposite strobe must be low in prior qualified, current qualified, and raw states. Simultaneous rises are rejected as ambiguous rather than assigned a fictitious order.

A conservative diagnostic sequence holds the tag with both strobes low for twelve nominal milliseconds, raises exactly one strobe for six, then returns both low for six. The tests use this sequence. These are software observation intervals, not calibrated electrical limits. Short pulses can be filtered or missed entirely. A quiet initial bus does not prove an empty allocator: the operator must establish the target baseline independently.

Repeated timestamps are ignored. Skipped acquisition ticks stop interpretation rather than inventing continuous sampling. Ordinary unsigned timestamp wrap is supported. Interface errors precede semantic checks, duplicate allocation precedes cyclic-selection checks, and orphan release has its own classification. Any finding latches a terminal state. Occupancy and accepted history remain at the last coherent transition; timestamp and transport-failure diagnostics may continue. A rejected observation does not assert that the target rejected its operation.

Every sample is applied to a speculative state copy. Counter exhaustion discards the whole speculative transition before marking the old state terminal. This prevents partially credited admission, return or stamp updates. The encoder verifies conservation relationships before sending: admissions equal completed releases plus live depth, attempts equal admissions plus releases plus rejections, and per-identity use totals equal admissions. The watchdog remains fed after a policy halt so retained evidence is not intentionally erased by reboot. Real reset loses volatile history and requires a new experiment.

## Companion application and wire interface

Run `python3 app/console.py` on Linux with Python Tk support. There are no third-party Python packages. Enter an explicitly selected serial path and choose New live; no discovery or automatic connection occurs. Ledger shows occupancy, counters, per-tag stamps, derived search cursor, eligible identity and findings. History shows up to 256 accepted snapshots. Integrity retains source, live/replay provenance, connection state, parse errors, reset flags and freshness qualifications.

CF1 records contain ASCII decimal unsigned integers separated by commas, then a star, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT starts at 0xffff. B records contain reset flags. P records contain `[3,1,5,4,0,1]`: qualification intervals, nominal sample period, setup requirement, pool size, initial cursor and policy revision. S field order is declared in `app/contract.py` and emitted identically by `firmware/wire.c`. Failed serialization returns no record rather than transmitting partial output.

The app validates integer bounds, occupancy, terminal reason, unique nonzero ordinals, retained fault plausibility and counter conservation. Sparse transitions must preserve stamps without new admissions and explain membership changes with per-tag admission/release counts. A continuously free peer with no new admission forms a cyclic barrier: later candidates cannot be selected past it and earlier candidates cannot wrap past it for repeated reuse. These are necessary endpoint checks, not complete hidden-history reconstruction. A plausible first snapshot or CRC-valid interval cannot establish that every earlier operation was legal.

Two advancing compatible records are required for live progress. Duplicate timestamps cannot refresh it. Contradictory duplicates, rollback, later boot records, impossible transitions, reception pauses and excessive cumulative host/device divergence freeze interpretation. Drift allowance is capped, so it cannot gradually legitimize an unlimited backlog. An uncalibrated oscillator can cause conservative freshness failure. Without persistent boot identity, some resets remain indistinguishable from forward progress. Advancing terminal timestamps prove transport activity, not continued policy acquisition.

Disconnect preserves evidence and restores terminal settings; a second simultaneous connection is refused. Replay accepts only regular files no larger than two MiB and counts a truncated tail as damage. Resynchronization uses the complete CF1 marker. Exports are exclusive-create JSON with owner-only permissions, preserving provenance, errors and reset qualifications even without completed operations. Existing reports are never silently overwritten. No automatic cloud upload occurs. Delete reports using the host filesystem under the approved retention policy; flash history is not stored by the device.

## Building, exercising and operating

From this device directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py'
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

ARM GNU GCC and newlib are required for the actual Cortex-M4 image. Host GCC supplies sanitizer-backed tests. KiCad 9, its Python bindings and standard footprints are required for CAD checks; use distro Python when virtual environments lack pcbnew. Tk and Xvfb are needed for headless GUI tests. `tools/generate_hardware.py` reproduces the authored native carrier sources when intentional hardware changes are required; review its output and rerun validation after regeneration. Build outputs and KiCad personal preferences are ignored.

Engine tests enumerate repeated operation words through depth ten against an independent rotating-list oracle rather than calling the production cursor calculation to derive expectations. Other regressions cover drain without cursor reset, busy skipping, only-free immediate reuse, recency-policy distinction, setup ambiguity, duplicate/orphan operations, gap freeze, timestamp wrap and transactional exhaustion. UART and main-loop tests cover bounded buffers, timeout recovery, lost initial boot records, periodic policy and watchdog startup failure. Desktop tests exercise real C-encoder synthetic fixtures, forged CRC-valid records, Tk event loops, pseudo-terminal connection/hangup, replay, private export and overwrite refusal. Synthetic fixtures are not physical measurements.

Before fabrication, review native schematic, footprints, connector orientation, routing and the actual module revision. Before attachment, continuity-check every net, inspect shorts, verify ground compatibility and independently measure signal levels. Program the linked image using the verified module's supported SWD tooling; no physical flashing was performed for this release. Confirm the intended image starts from reset-default clock configuration rather than inheriting an unknown bootloader state.

Start with the target pool reset and free, cursor zero, both strobes low, and a freshly reset observer. Wait for compatible advancing telemetry before the approved workload. Capture independent target logs or waveforms alongside the report. On a finding, stop the workload, export evidence, record the tag map and isolate a reduced reproduction. Resetting only the observer cannot repair an unknown target baseline. A clean result applies only to the declared indications and observed interval; it is never a blanket security approval.
