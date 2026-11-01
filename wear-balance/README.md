# Wear Balance — Least-Used Free-Resource Selection Witness

Author and creator: jayis1. Hardware revision A. Protocol WB1.
Firmware, application, and tests: GPL-2.0-only. Hardware generation and checking utilities: MIT. Original passive carrier design: CERN-OHL-S-2.0. Installed tools and third-party component libraries retain their upstream licenses and are not claimed as original work.

Status: unbuilt engineering prototype. Target firmware is compile-and-link tested, software fixtures exercise the desktop interface, and native KiCad files have routed nets. See VERIFICATION.md for actual results and limitations. Fabrication, programming, electrical qualification, target interoperability, and real wear measurements have not been performed. This is an observer of declared operations, not a wear sensor, allocator, security interlock, or certification instrument.

## Purpose and overview

Wear Balance detects a subtle availability weakness: an allocator can obey every live-occupancy limit while repeatedly selecting an already heavily used resource even though a less-used resource is free. In an authorized durability or denial-of-service study, such behavior can concentrate writes, erase reservations, relay actuations, or other expensive operations into a small part of an otherwise interchangeable pool. The instrument observes four unique resource identities, retains cumulative accepted use counts independently of occupancy, and checks each new selection against the least-used currently free alternatives.

A motivating experiment is an embedded storage controller with four equal-cost erase-work slots representing four independently reusable test regions. An owner-supplied diagnostic adapter announces the region identity and accepted reservation boundary. The observer catches repeated selection of region zero when regions two and three have never been used and remain available. It does not issue erase commands, inspect storage contents, estimate flash lifetime, or infer actual erase completion from electrical current. The same contract can be evaluated on harmless software-emulated resources before any real media endurance experiment.

The policy is intentionally exact and small. All four resources begin free with use count zero. ALLOCATE(tag) is legal only if that tag is free and its count equals the minimum count among all free tags. A successful allocation marks the tag occupied and increments its count exactly once. RELEASE(tag) is legal only for an occupied tag; release clears occupancy without changing use history. Equal counts permit any tie-breaking order. Busy resources are excluded from the comparison because the target cannot legitimately choose them. Counts persist after the entire pool drains. Time alone never makes a choice fairer.

For example, allocate zero, release zero, allocate one, release one leaves counts [1,1,0,0]. Selecting zero now is a bias finding, while either two or three is legal. Conversely, allocate all four tags, release zero, allocate zero is legal: the other three resources are still busy, so zero is the only available choice even though its cumulative count then becomes larger. This distinction avoids confusing utilization balance with a global bound on count spread. The implementation does not require every count to stay within one of every other count.

## Originality and reuse boundaries

Before selecting the concept, the repository was pulled and its directory inventory and device titles were checked. Purpose text for the closest weighted-capacity and starvation witnesses was read, and a repository-wide README search for least-used allocation and wear balancing found no matching observer contract. Weight Ceiling checks an unequal-cost sum of live reservations and refunds cost on release. Starvation Window checks waiting age and repeated competing service while demand remains pending. Contention Turn uses demand-aware round-robin preference. Wear Balance instead retains per-resource cumulative use and compares only the free candidate set at each admission; no demand signal or fixed rotation is involved.

The contribution is the cumulative-use/free-set contract, transactional counter updates, corresponding telemetry consistency checks, an independent array oracle, and the operator workflow for investigating allocation concentration. The passive carrier, MCU startup, board support, CRC implementation, bounded serial transport, and desktop transport infrastructure deliberately reuse jayis1-authored code from Weight Ceiling. Those circuits and generic drivers are not represented as new inventions. Originality is relative to the inspected repository, not a worldwide prior-art search, patent claim, or claim that least-used allocation is a new algorithm. This folder is self-contained and requires no imports from another device folder or unified hardware tree.

## Attack surface, authorization, and threat model

Use only on equipment you own or have explicit written authorization to assess. Permission must cover opening the enclosure, adding diagnostic firmware, attaching a common-ground instrument, running the workload, and retaining the resulting evidence. Begin with an emulator or expendable development fixture. Do not attach the unisolated prototype to production access controls, medical equipment, vehicles, mains-related circuits, public infrastructure, or machinery whose operation could injure someone. Stop immediately if attachment changes target behavior, produces heating, or alters measured logic levels.

The security-relevant attack surface is the resource-selection decision inside an embedded allocator. Relevant defects include always preferring the lowest address, retaining a stale least-use cache, clearing history whenever the pool empties, comparing busy resources instead of available alternatives, or incrementing the wrong resource counter. A malicious tenant might intentionally repeat an otherwise valid request pattern that exposes such a weakness. This instrument merely observes a supervised, owner-approved experiment. It contains no exploitation engine, stealth behavior, persistence mechanism, credential collector, radio transmitter, or target command interface.

The diagnostic adapter is a trusted part of the experiment. It must expose accepted allocations rather than attempted requests that the target rejected. Each tag must identify one unique, interchangeable resource with equal per-allocation experimental cost, not a category containing several concurrent members. The adapter must report every allocation and release in order. A resource unavailable for another reason must be represented as busy under the experiment's defined contract or the experiment must be stopped. Dynamic eligibility masks, unequal physical endurance, changing operation cost, and preexisting lifetime wear are not supported by this version.

The observer cannot prove a target is honest. Missing operations, fabricated diagnostic signals, dishonest physical mappings, or a compromised workstation can invalidate the evidence. CRC detects accidental corruption, not deliberate tampering. There is no authenticated attestation, secure boot, signed update channel, encrypted export, persistent boot identity, or trusted time source. A clean session proves neither resistance to denial of service nor balanced physical wear. Correlate findings with the adapter source, independent waveform capture, target logs, and actual resource mapping before drawing security conclusions.

## Hardware specifications and bill of materials

The external processor module is ST NUCLEO-F401RE containing STM32F401RET6, a Cortex-M4 with 512 KiB flash and 96 KiB SRAM. The firmware uses the nominal reset-default 16 MHz HSI oscillator, a nominal one-millisecond SysTick, and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. PA2 sends status to the module's ST-LINK virtual COM bridge. The Nucleo provides power regulation, MCU decoupling, reset, SWD programming, and USB connectivity. Those functions are not falsely shown as components on the passive carrier.

The custom board is an 80 by 70 mm, two-layer, nominal 1.6 mm carrier. It has fourteen through-hole components, thirty connected electrical pads, and nine nets. It connects to the Nucleo using short labelled leads; it is not a stacking shield. There are no radios, antennas, batteries, chargers, memory cards, analog sensors, relays, or target power outputs. No enclosure CAD or mounting holes are supplied. Use insulated supports, strain relief, and an underside cover before bench attachment.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | NUCLEO-F401RE; purchased board revision verified before use |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2, 2.54 mm pitch header |
| J2 | 1 | Vertical 1x4, 2.54 mm pitch header |
| Harness/support | 1 set | Insulated short leads, labels, nonconductive enclosure support |

Resistor footprints are DIN0207 with 7.62 mm lead spacing. Confirm actual body size, lead diameter, drill fit, connector orientation, and mating geometry before fabrication. Pricing, stock, shipping, and component availability are not independently established; obtain supplier quotations rather than treating this BOM as a procurement commitment. Budget an appropriate USB source for the purchased module; 5 V at 500 mA is a conservative planning allowance, not measured consumption or a compliance assertion. The carrier has no independent supply rail and does not power the target.

| Signal | Target header | MCU-side J2 | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | 1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | 2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | 3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | 4 | CN8.4 / A3 / PB0 | 3 |

Pin two of J3 through J6 is ground. Both J1 pins are ground only, never supply inputs. J2 carries four signals and has no ground or supply contact; use a separate Nucleo ground lead to J1. Each `_IN` target node connects through 1 kOhm to its MCU-side node, which has a 100 kOhm pulldown. The nominal resistive path to ground is 101 kOhm, excluding source impedance and MCU leakage. Evaluate capacitive loading, thresholds, noise margin, and cable length on the actual fixture.

Only known common-ground, push-pull, zero-to-3.3 V diagnostic signals are supported. Exclude 5 V TTL, 1.8 V logic, negative voltage, differential signals, open-drain interfaces without separate review, and unknown field wiring. The series resistors are not a safety barrier. There is no galvanic isolation, dedicated ESD clamp, fuse, active buffer, or guaranteed hot-plug tolerance. Power the observer before enabling target outputs and disable those outputs before depowering the observer to avoid back-powering. USB and target grounds become common.

Check the actual module against ST UM1724, the STM32F401xD/xE datasheet, and RM0368. In particular, verify CN8 pin mapping, ST-LINK virtual-COM solder bridges, supported power jumpers, GPIO voltage limits, and reset clock assumptions. The manufacturer PDF could not be freshly downloaded during this run; the inherited mapping is therefore subject to this explicit pre-assembly check. Passing passive-board ERC cannot validate the external module or its configuration.

## Architecture and block diagram

```text
Owned target adapter        Passive carrier                  Nucleo-F401RE
TAG0 ----------------------- 1k ---+------------------------- PA0
TAG1 ----------------------- 1k ---+------------------------- PA1
ALLOCATE ------------------- 1k ---+------------------------- PA4
RELEASE -------------------- 1k ---+------------------------- PB0
                                   | each node separately
                                  100k
                                   |
Target ground -------------------- GND ---------------------- USB ground

GPIO -> continuity guard -> tag/strobe qualification -> prior setup check
     -> active set + per-tag lifetime counters -> least-used FREE policy
     -> transactional state -> bounded WB1 encoder -> USART2 TX
     -> ST-LINK USB serial -> Ledger / History / Integrity -> private JSON
```

The target-facing pins are inputs in the supplied firmware. USART reception is disabled; there is no configuration command that can allocate or release a target resource. Physical programming access can replace the firmware, so receive-only behavior is not an irreversible hardware diode. PA0/PA1/PA4 and PB0 are read from two GPIO ports, not atomically latched across ports. Diagnostic transitions must obey the slow interface contract; the board is not a high-speed bus analyzer.

## Firmware design and timing contract

The firmware is C11 without an RTOS, dynamic allocation, SDK download, or interrupt-driven capture queue. Explicit startup initializes data and BSS, and a linker script defines flash/RAM regions with stack headroom. Board drivers configure GPIO, SysTick, USART, reset-cause capture, and the independent watchdog. The main loop samples once per observed tick and services UART without waiting for transmission completion. A repeated timestamp is ignored. A jump of more than one tick terminates correlation instead of inventing missed samples; ordinary unsigned timestamp wrap remains supported.

The two-bit tag qualifies as an atomic bundle. ALLOCATE and RELEASE qualify independently so chatter on one cannot indefinitely postpone the other. Qualification requires a candidate observation followed by three matching intervals. Before accepting an operation edge, the tag must already have accumulated five stable qualified observations. The current raw tag must agree, the relevant raw strobe must still be high, and the opposite strobe must be low in prior qualified, current qualified, and raw states. Simultaneous rising strobes are interface ambiguity, not an arbitrary order chosen by software.

For a deliberately conservative fixture, drive the selected tag with both strobes low for twelve nominal milliseconds, assert exactly one strobe for six milliseconds, then hold both low for six milliseconds. The tests use that sequence. These are software observation intervals, not calibrated electrical timing guarantees. Narrow pulses may be filtered or missed; signal transitions faster than the sampling contract are outside scope. An initial quiet qualified state establishes only a software baseline. The operator must separately ensure that the target pool is empty and its experimental use history is reset before starting the observer.

Fault precedence is interface ambiguity before operation semantics, then duplicate allocation before selection bias, with orphan release checked independently. Any policy or continuity fault halts interpretation and retains the last accepted occupancy and use counts. A bias finding does not mean the target refused the operation; it means the observer rejected an operation that the adapter said the target accepted. After halt, only time and transport-failure diagnostics may advance. There is no automatic recovery from quiet wires, because quietness cannot reconstruct lost lifetime use.

Updates happen on a speculative copy of the state. If any cumulative counter would overflow, the entire observation is discarded and the previous coherent state is marked exhausted. A release cannot partially decrement occupancy while failing to increment completion. Use counters, admission/release counts, closed occupancy episodes, overlapping admissions, and peak depth support independent conservation checks before serialization. Watchdog startup failure produces a terminal reason immediately. The watchdog is fed even after a policy halt to preserve the evidence rather than rebooting away the finding; an actual watchdog reset starts a new volatile session and must not be mistaken for continuity.

## Application and wire protocol

Run `python3 app/console.py` from this folder on Linux with Python Tk support. The application uses only the standard library; no package install scripts or network service are required. Enter a verified serial device path, normally a stable `/dev/serial/by-id/` path or `/dev/ttyACM0`, and select New live. The app does not automatically probe unknown devices. A second live connection is refused until Disconnect restores the original terminal settings. Permission denial, a non-TTY path, hangup, and constructor failure are shown without replacing retained evidence.

The Ledger screen displays current active membership, all four cumulative use counters, occupancy depth, peak depth, operation totals, and decoded findings. History retains up to 256 accepted snapshots. Integrity displays connection state, source provenance, parser errors, reset hints, freshness, and frozen-session reasons. Replay reads a regular file of at most two MiB, never a FIFO or unbounded device. Replay evidence is always labelled replay and never live. Export JSON creates a new private file with mode 0600, refuses overwrite and symlinks, and includes author, source, history, latest state, and integrity qualifiers.

WB1 frames are ASCII, newline terminated, with a four-digit uppercase CRC16-CCITT checksum after `*`. The CRC covers everything before `*`, initialized to 0xffff with polynomial 0x1021 and no final XOR. `WB1,B` carries raw reset flags once at boot. `WB1,P` repeats policy values `3,1,5,4`: qualification interval count, nominal sample period, setup count, and number of slots. `WB1,S` carries the fields below in exact order. Numeric values are unsigned decimal 32-bit integers; field counts, bounds, masks, policy values, CRC, and conservation relationships are checked before accepting status.

```text
now levels valid halted faults phase active depth stable
observed rounds attempts launched completed rejected gaps missed
queued peak use0 use1 use2 use3 tx_fail
```

Status is scheduled every hundred milliseconds, with policy every second to support late attachment. Boot records are not repeated as policy announcements. The bounded 768-byte writer refuses partial serialization; UART work is bounded and a hundred-millisecond transmit timeout drops the pending frame and increments a transport diagnostic. The parser resynchronizes on the complete `WB1,` marker after malformed or truncated frames and counts framing errors. An incomplete replay tail is counted even when earlier snapshots were valid.

The companion rejects disappearing faults, backwards counters, contradictory duplicate timestamps, and mutations of halted evidence. Per-tag use changes must explain each membership change; a single new admission cannot bypass a previously free, less-used alternative. Sparse multiple-admission transitions receive necessary consistency checks, not exhaustive reconstruction of every hidden history. Freshness requires connection, compatible policy, advancing device time, and recent reception. Long pauses or excessive host/device progress divergence freeze the session. These guards reduce stale-data confusion but cannot authenticate devices or detect every reset when boot telemetry is lost.

## Build, assembly, and validation workflow

From this folder run `make -C firmware all test`. The default cross prefix is `arm-none-eabi-`; override CROSS only with a compatible Cortex-M4 toolchain. Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_app.py' -v` for headless GUI and parser tests. Run `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC, and exact schematic/PCB/manifest connectivity, then `python3 tools/check_release.py` for attribution and size gates. KiCad generation uses the installed KiCad 9 Python bindings and footprint libraries; regenerate with `/usr/bin/python3 tools/generate_hardware.py` only when intentionally rebuilding design artifacts.

Inspect the schematic, board outline, reference positions, header orientation, and every resistor before assembly. Measure resistance and absence of unintended shorts without the target attached. Confirm every harness lead end-to-end, then test only against a low-energy 3.3 V fixture. Use the Nucleo's normal ST-LINK workflow to program `firmware/build/wear-balance.elf`; programming was not performed here. Reset target experimental history and the observer together, establish the empty baseline, and first run tie, bias, busy-exclusion, orphan-release, and duplicate-allocation examples. Cross-check strobes with a logic analyzer before attaching a target under assessment.

The software test matrix covers short repeated operation words against an independent count-array oracle, biased reuse after drain, exclusion of busy low-use tags, simultaneous strobes, duplicate allocation, orphan release, gaps, clock wrap, serialization bounds, counter exhaustion, UART timeout, policy repetition after lost boot, and watchdog startup. Application tests include real C-encoder replay, impossible CRC-valid snapshots, history migration, stale live reception, private export, bounded replay, PTY serial operation, all three screens, disconnect, and error preservation. These are synthetic host tests, not measurements from a fabricated board.

## Research use cases, evidence handling, and limits

An authorized red team can use an approved workload to demonstrate that an allocator concentrates accepted work on a repeatedly reused region while equivalent free alternatives exist. A penetration tester can compare a vulnerable build and a repaired build using the same tag mapping and workload, exporting the first rejected boundary and preserved counters. A researcher can compare arbitrary tie-breaking strategies or show why excluding occupied resources changes the interpretation of apparent wear imbalance. None of these workflows requires the observer to transmit into the target or collect user payloads.

Record target version, adapter source revision, pin mapping, initial history assumption, workload, observer build, and authorization reference alongside exports. The device retains no evidence in nonvolatile storage, but the workstation retains explicit exports until removed by the operator. Select a retention period appropriate to the engagement, restrict file permissions, and delete exports and backups according to the agreed policy. Do not treat JSON CRCs or local timestamps as a chain of custody. Independent custody controls are required for formal evidence.

Important limits remain: no physical wear measurement, no persistent lifetime counters across reset, no support for unequal resource costs, no authentication, no secure update, no electrical isolation, no high-speed event capture, no proof that adapter indications correspond to physical operations, and no fabrication readiness claim. Late attachment can show device cumulative history but cannot recover missing intermediate waveforms. Halt requires exporting the result and establishing a new controlled baseline, not pressing reconnect to erase an inconvenient finding. The deliverable is a testable engineering design for authorized laboratory research, with physical qualification explicitly left to the assembly and bring-up stage.
