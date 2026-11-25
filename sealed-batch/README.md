# Sealed Batch — Drain-Sealed Resource Cohort Witness

Author and creator: jayis1. Hardware revision A. Wire protocol SB1, contract revision 1.

Status: unbuilt engineering prototype. This directory contains target-linked firmware, a Linux desktop companion, tests, and a routed passive carrier with native KiCad sources. Software execution and CAD checks are recorded in VERIFICATION.md. Fabrication, MCU programming, electrical bring-up, calibrated timing, and physical target interoperability remain unverified. This is an observation instrument, not an allocator, access control mechanism, protective interlock, or security certification.

Original firmware, application and tests: GPL-2.0-only. Hardware utilities: MIT. Passive carrier: CERN-OHL-S-2.0. Purchased modules, installed tools and standard footprint libraries retain their upstream provenance and licenses.

## Purpose and practical value

Sealed Batch checks a resource lifecycle promise that ordinary occupancy alarms miss: once any member of a batch starts returning, no new member may enter until the entire batch has drained. Four diagnostic resource identities are observed through a two-bit tag bus and separate allocation and release strobes. The observer reconstructs membership, remembers the admitted batch size, and retains the first admission that crosses the drain boundary. It records no payload, credential, memory address, or network packet.

Consider an owner-instrumented embedded gateway that processes a bounded batch of security-sensitive jobs through shared scratch storage. During filling it may admit one, two, three, or four jobs. Once cleanup begins, a cancellation callback must not refill a newly vacant descriptor while a peer from the old batch remains live. Such an admission may be individually well formed: its tag is free, capacity remains, and the new job may even have the same security label. Nevertheless it breaks the owner's promise that cleanup and admission never overlap within one nonempty batch.

The practical research question is whether scheduler, timeout, and cancellation paths obey that lifecycle contract. An owner can expose committed operations from a reduced implementation and compare an independent physical witness with software logs. A penetration tester can retain before-and-after evidence for a remediation. An authorized red team can ask the owner to reproduce a suspected cleanup race without capturing protected contents or running arbitrary commands through the instrument. The device generates no test workload itself.

This design does not prove data erasure, completed DMA, separate human identities, correct cryptographic processing, or absence of cross-job leakage. A diagnostic release must mean the resource has actually left the execution context relevant to the experiment. If the adapter reports only a queued completion callback while hardware still accesses scratch memory, the observed contract is weaker than the intended security property. Review the adapter and corroborate its mapping before interpreting any result.

## Exact contract and examples

An empty pool is ready to accept any identity. While filling, any currently vacant identity may be admitted. A duplicate admission is an error regardless of batch phase. The first valid release seals the current batch against further admissions if other members remain. Any occupied identity may return in any order. When the last member returns, the batch closes and admission reopens. Single-member batches close immediately on their only return. There is no minimum batch size, required full occupancy, fixed ordering, domain alternation, elapsed cooldown, or lifetime quota.

For example, allocate zero, allocate one, then release zero. The remaining member is one, and zero, two and three are physically vacant. All three vacancies are nevertheless ineligible because the batch is draining. Allocating two at this point is the specific Sealed Batch finding. Releasing one before allocating two instead closes the old batch and starts a legal new batch. Repeating allocation and release of zero alone is legal indefinitely.

The firmware stores batch_size, the number of admissions in the current nonempty batch. Before the first release it equals current depth. After a partial return it exceeds depth and never decreases until complete drain clears it to zero. Successful admissions increment both quantities, but the admission predicate permits this only while they are equal. This representation prevents a later quiet interval or selector change from accidentally reopening a draining batch. Tests use a separate explicit filling/draining oracle rather than repeating this comparison.

A duplicate admission takes precedence over the sealed-batch finding. An orphan release has its own fault. Simultaneous strobes or unstable setup are interface ambiguity, not an invented ordering. After any finding, semantic reconstruction stops permanently and the previous coherent membership remains visible. The target may have committed the rejected diagnostic operation; retained observer membership is therefore explanatory evidence, not a claim about the target's current state after the contradiction.

## Repository distinction and provenance

The configured destination is jayis1/hacker-devices on main. Device directories and README purpose summaries were inventoried before selection. The close Domain Cohort and Distance Fit contracts were read. Domain Cohort allows rolling same-domain replacement while peers remain resident; Sealed Batch rejects all replacement after a return, even for the same domain, and does not consume domain labels. Distance Fit chooses among vacancies by current spatial separation; Sealed Batch ignores rank and spatial placement. A first admission of zero followed by one is legal here, unlike that maximin-distance policy.

Queue Overtake constrains departure order; this device permits arbitrary departure order but forbids overlapping fill and drain phases. Prefix Retire observes readiness and ordered visibility within a fixed prepared batch, rather than variable-size resource admission. Thus the useful distinction is a new observable lifecycle contract, not a renamed ordering preference. Repository-local distinctness is claimed; worldwide originality, patentability and invention of batching itself are not.

The register layer, startup, UART transmitter, CRC encoder, qualification framework, passive circuit and desktop transport reuse jayis1-authored Distance Fit infrastructure. The carrier is intentionally electrically identical because the input interface is identical. Device-specific work includes the drain-sealed admission predicate, captured cohort-size invariant, companion transition checks, phase display, regression oracle and operating documentation. Internal UQ-prefixed identifiers are implementation details; SB1 is the public protocol. Builds and runtime do not import sibling device directories.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Scope must cover diagnostic firmware changes, enclosure access, common-ground wiring, approved workloads, interruption, recovery and evidence retention. Network testing permission does not by itself authorize internal electrical attachment. Start with an expendable low-energy fixture disconnected from hazardous outputs and production dependencies. Keep the harness visibly labelled and establish a stop procedure before powering anything.

The investigated attack surface is resource admission around cleanup, cancellation, timeout and batch handover. The trusted boundary includes the operator, diagnostic mapping, target adapter, harness, external MCU board, observer image and workstation. A malicious target can emit truthful-looking diagnostic strobes unrelated to real activity. Missing operations can invalidate reconstructed state without causing an acquisition-time gap. A serial source can forge frames and valid checksums. CRC detects accidental corruption; it is not authentication, signing or replay protection.

There is no secure boot, remote update endpoint, encrypted storage, authenticated pairing, persistent boot identity, secure element, cloud account or network listener. Physical SWD access can replace firmware. Receive-only describes the supplied target-facing GPIO configuration, not an irreversible hardware data diode. Replacement firmware could drive those pins through the resistors. Even correct firmware cannot prevent electrical disturbance from common ground, input loading, shorts, miswiring or back-powering.

Never connect this unisolated prototype to mains-referenced equipment, deployed access controls, vehicles, medical devices, moving machinery or unknown field wiring. Do not collect identifiers or payloads through an expanded adapter without a new scope review. Operation timing and occupancy can still be sensitive metadata. Store exports in engagement-controlled directories, avoid automatic synchronization, and apply the agreed retention and deletion policy. No safety or regulatory approval is implied by passing software tests.

## Hardware specifications and bill of materials

The processor is an external ST NUCLEO-F401RE module containing STM32F401RET6, a Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI, a nominal one-millisecond SysTick and USART2 transmission at 115200 baud, eight data bits, no parity, one stop bit. PA2 sends telemetry toward the ST-LINK virtual COM bridge. The purchased board supplies regulation, decoupling, reset, USB and SWD; these are module facilities, not omitted standalone carrier components.

The cable-connected carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm tracks and a closed board outline. It is not a stacking Arduino shield. Fourteen through-hole components provide thirty pads on nine nets. There is no radio, FPGA, antenna, battery, charger, storage card, analog sensor or target power output. Mounting holes and enclosure CAD are not supplied; use insulated supports, underside protection and strain relief.

| References | Quantity | Part specification |
| --- | --- | --- |
| External module | 1 | NUCLEO-F401RE, purchased revision inspected |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness and support | 1 set | Short insulated leads and nonconductive support |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Headers use standard KiCad footprint geometry. Check actual bodies, lead diameters, drilling, mating clearance and pin-one orientation before purchase or assembly. Source availability, price and manufacturing yield are not verified. USB powers the external module. Reserve a 5 V, 500 mA supply as a conservative planning allowance, not measured consumption. No battery undervoltage path exists; module brownout behavior still needs physical testing.

| Diagnostic | Target connector | Module harness | Nucleo endpoint |
| --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 |

Each target header's second contact is ground. Both J1 contacts are ground, never power. J2 carries only signals; separately connect module ground to J1. Each target-side _IN net crosses 1 kOhm to the corresponding MCU-side node with a 100 kOhm pulldown. Disconnected inputs tend low but are indistinguishable from real low signals. There is no cable continuity detection.

Only known common-ground zero-to-3.3 V push-pull diagnostic outputs are supported. Five-volt, negative, differential, 1.8 V and unknown signals are unsupported. No TVS, fuse, isolation, hot-plug protection or qualified input clamp is supplied. Power the observer before enabling target outputs, and disable those outputs before removing observer power. Series resistance is not permission to exceed MCU ratings. Measure loading and ground offsets on the intended fixture before use.

Manufacturer references are ST UM1724 for module headers and bridge routing, RM0368 for registers, and the STM32F401xD/xE datasheet for electrical limits. Manual: https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf . Check the purchased board revision and SB13/SB14 versus SB62/SB63 arrangement before relying on the virtual COM bridge. Passive-only ERC cannot validate the external module or harness.

## Architecture and firmware decisions

```text
Owner's committed operations       Passive carrier             NUCLEO-F401RE
TAG0 ------------------------------ 1k ---+--------------------- PA0
TAG1 ------------------------------ 1k ---+--------------------- PA1
ALLOCATE -------------------------- 1k ---+--------------------- PA4
RELEASE --------------------------- 1k ---+--------------------- PB0
                                          | each node
                                         100k
                                          |
Target GND ----------------------------- GND ------------------- USB GND

GPIO -> continuity guard -> bundled tag / independent strobes
     -> prior setup check -> occupancy / captured batch size
     -> transactional ledger -> SB1 CRC encoder -> bounded UART TX
     -> ST-LINK virtual COM -> Ledger / History / Integrity screens
```

GPIOA and GPIOB are read sequentially, not as an atomic multi-port latch. This is slow diagnostic instrumentation, not instruction-rate trace. Begin with twenty milliseconds stable tag, twenty milliseconds selected strobe high and twenty milliseconds both strobes low. Verify actual waveforms before tightening margins. Short pulses can fall entirely between samples. Sub-sample ordering and propagation delay are not established by the observer.

The tag qualifies as one two-bit bundle. Allocation and release qualify independently so chatter on one input cannot indefinitely postpone a stable peer. Each group requires three unchanged intervals after the first candidate observation. At an operation edge the qualified tag must already have five stable intervals. Raw tag must still match, the selected strobe must be raw high, and the opposite strobe must be clear in previous, current and raw observations. WAIT exits only after qualification with both strobes low; this electrical baseline does not establish an empty target.

The engine uses fixed-size state, bounded loops and no dynamic allocation. Every observation is speculative until all counter updates succeed. Exhaustion rolls back the entire observation and enters terminal fault, avoiding half an accepted operation. A missed millisecond halts correlation rather than treating unobserved time as debounce stability. Duplicate calls at the same timestamp are ignored; ordinary unsigned timestamp rollover is supported. After halt, only time and permitted transport diagnostics advance. Reset clears volatile evidence and requires independently restoring target emptiness.

Conservation includes admissions equals returns plus live population; attempts equals admissions plus returns plus rejected edges; and admissions equals closed batches plus the active-batch indicator plus overlapping admissions. Per-tag use totals sum to admissions. Last admission ordinals are unique and retained after return. Captured batch size cannot exceed peak occupancy or four, cannot be smaller than depth, and must be zero when empty. Both encoder and companion validate these relationships.

The independent watchdog has bounded startup waits. Failed startup is reflected before the first status frame. UART transmission is bounded and nonblocking; a receiver disappearing cannot block acquisition indefinitely. Startup initializes data and BSS and provides reset and SysTick vectors. registers.h contains the used RM0368 addresses rather than importing an opaque SDK. Physical watchdog reset, oscillator error, brownout and programming recovery are bench acceptance items, not simulated achievements.

## Application and wire interface

Frames start SB1, contain record kind and unsigned decimal fields, and end with an asterisk, four uppercase CRC digits and LF. CRC-16 uses polynomial 0x1021 and initial 0xffff over bytes preceding the asterisk. B announces reset flags once. P periodically announces qualification, tick, setup, slot count, contract and revision as 3,1,5,4,1,1. S carries the ordered FIELDS in app/contract.py, including batch_size immediately before tx_fail. No inbound target command parser exists.

Run python3 app/console.py on Linux with Tk installed. The Ledger screen shows current evidence, batch phase, membership, eligible tags, counters and findings. History retains up to 256 snapshots. Integrity shows source, live/replay provenance, parser errors, reset indicators, freshness and rejection messages. New live opens the selected serial path read-only. Disconnect retains evidence. Replay accepts regular files of at most two MiB. Export JSON creates a private new file and refuses overwrite. Exported reports are not raw wire recordings.

The companion rejects malformed framing, CRC errors, unsupported policies, contradictory duplicates, counter rollback, impossible field relationships and inconsistent sparse transitions. For intervals containing at most eight accepted operations, a finite count-constrained search checks whether some legal batch ordering explains the endpoints, captured size and batch closures. Larger intervals receive necessary conservation and continuous-resident checks. These checks do not prove the actual hidden ordering, authenticate evidence, or reconstruct every timing constraint.

A draining snapshot cannot acquire admissions before a batch closes. A batch cannot close around an old resident that never returned. Without closure, captured size can grow only through new admissions, and an identity cannot be reused. These checks prevent plausible-looking counters from silently erasing the seal. An accepted sparse pair may still conceal a different illegal history; obtain dense evidence or independent waveform capture when order itself matters.

Freshness requires compatible policy, connection, advancing acquisition and recent progress. Duplicate timestamps do not renew it. Reception pauses or excessive host/device timing divergence freeze interpretation until a new session. Internal oscillator drift and buffered transport make this heuristic, not a trusted acquisition timestamp. Without a persistent boot identity, some resets with lost boot records and plausible forward counters remain indistinguishable. Exports preserve uncertainty and provenance after disconnect.

## Build and validation procedure

Use the versions recorded in VERIFICATION.md as the reproducible toolchain reference. Dependencies are GNU Arm Embedded GCC with newlib, host GCC, Python with Tk, KiCad 9 with pcbnew bindings, and Xvfb for headless GUI tests. No Python packages, JavaScript package installation or downloaded vendor SDK are required. From this directory run:

    make -C firmware all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py

The CAD generator is tools/generate_hardware.py and requires system Python with pcbnew. It regenerates native sources, so review changes before committing. The checker runs schematic ERC and PCB DRC, exports a fresh netlist and compares every value, footprint identity, reference, pad and net with the PCB and connectivity manifest. Zero violations does not establish electrical safety or manufacturability.

Tests exercise independent phase-oracle operation histories, filling and draining subsets, arbitrary returns, full-drain reuse, duplicate precedence, gap freeze, counter exhaustion, raw setup faults and timestamp rollover. Companion tests include real C-encoder synthetic fixtures, sparse endpoint checks, invalid checksums, truncated framing, stale delivery, serial attribute restoration, permission errors, private export and actual Tk screens driven through Linux pseudo-terminals. Synthetic fixtures are explicitly not physical measurements.

Before flashing, inspect schematic and PCB, check unpowered continuity and shorts, verify every harness connection against the purchased module, and validate firmware memory placement. Program through physical ST-LINK using an approved local programmer. With the target disconnected, check clock, UART, reset behavior and voltage levels. Then exercise a harmless known legal batch, a partial return, an intentionally prohibited admission, and full-drain reuse on an owned fixture. Compare waveforms and retained identity with independent logs.

Export before reset. Record target revision, adapter mapping, observer image, wiring, authorization and workload alongside the report. An interrupted update has no automatic authenticated recovery mechanism: use physical SWD recovery and repeat validation before reconnecting a target. Host storage failures leave evidence in memory but cannot guarantee a complete export. Recheck any failed output rather than treating an existing filename as a successful report. Physical timing, ESD, power sequencing, USB unplug behavior, input loading and field usefulness remain separate acceptance gates.
