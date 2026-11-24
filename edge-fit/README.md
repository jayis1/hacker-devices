# Edge Fit — Free-Run Splitting Witness

Author and creator: jayis1. Hardware revision A. Protocol EF1, policy revision 1.

Status: unbuilt engineering prototype design. The package contains target-linked firmware, a working Linux desktop companion and native routed KiCad carrier files. Executed checks are recorded in VERIFICATION.md. No fabrication, physical programming, calibrated timing, electrical qualification or real-target interoperability is claimed. This instrument observes a declared allocator policy; it does not allocate resources, enforce security, prevent failures or certify a target.

Original firmware, application and tests: GPL-2.0-only. Original CAD tools: MIT. Original passive carrier design: CERN-OHL-S-2.0. Installed development tools, standard footprints and purchased modules retain their upstream licenses and provenance.

## Purpose and overview

Edge Fit investigates a narrow availability contract: does an allocator consume a contiguous free region only at its endpoints, or does an admission unnecessarily split that region into two smaller free regions? Ordinary occupancy instrumentation detects duplicate ownership but misses this distinction. A pool can remain collision-free and report the right free count while the chosen placement destroys a contiguous opportunity needed by a later request. Edge Fit independently reconstructs four adjacent identities from accepted operation indications and retains the first interior-placement contradiction.

A practical research fixture is an owner-instrumented security gateway with four adjacent staging descriptors. The owner has specified that single-descriptor requests must not split a free interval. A recovery path, stale cached candidate, or incorrectly generalized search routine might choose an interior descriptor despite that promise. The instrument records identity and operation accounting without collecting addresses, application payloads, credentials or customer traffic. A researcher can reduce a failure to a small sequence of admissions and arbitrary cancellations, then compare the same experiment after remediation.

The device does not establish that endpoint placement is universally optimal. It does not implement variable-sized requests, compaction, alignment, address-space discovery, real memory inspection or future-demand prediction. It measures the target's explicitly chosen no-split policy for unit admissions. Returning an occupied identity may legitimately increase fragmentation. Selecting either endpoint can leave different future possibilities; both are legal. A finding is a policy contradiction that may matter to availability, not proof of an exploitable denial of service.

## Exact contract and examples

The identities form the line zero—one—two—three. Zero and three are not adjacent. Each identity represents one resource that can have at most one current owner. The operator must establish an empty target pool before the observer begins interpreting operations. An ALLOCATE pulse reports an already committed admission, not a request that might subsequently be rejected. A RELEASE pulse reports the return of the named occupied identity. Releases have no ordering restriction.

A free run is a maximal contiguous sequence of vacant identities. On admission, the tag must be free and must be either endpoint of its current run. A singleton is its own endpoint. All runs have equal priority regardless of length; both endpoints have equal priority regardless of numeric rank or history. Equivalently, changing the selected free identity to occupied must not increase the number of connected free components. This equivalence supplies the independent test oracle rather than duplicating the implementation's neighboring-bit check.

Starting empty, zero and three are eligible, but one and two would split the four-slot interval. After allocating zero, one and three are eligible; two remains an interior point of [1,2,3]. After allocating zero and three, either one or two is eligible. Full sequential admission zero, one, two, three is legal, as is its reverse. Releasing zero after only zero was occupied restores an empty pool, making one ineligible again. Thus returns can remove permission that previously existed.

Fill the pool, then release zero, two and three. The remaining occupied identity is one. Free runs are [0] and [2,3], and all three free identities are eligible. Edge Fit does not require consuming the singleton first. This example distinguishes no-split placement from shortest-gap selection. Repeating allocate-zero, release-zero is legal indefinitely. The device imposes no fairness, wear-leveling, recency, cooldown or round-robin requirement.

Duplicate admission has precedence over a placement finding. An orphan release names an identity that was not occupied. Interface ambiguity, acquisition loss, counter exhaustion and watchdog startup failure are separate terminal conditions. At the first fault, the observer retains its previous coherent occupancy rather than pretending the target refused its already reported operation. Eligibility displayed after halt describes that retained model, not permission to continue operating the target.

## Repository distinction and implementation provenance

The configured checkout is jayis1/hacker-devices, with main confirmed as remote HEAD. Existing device directory names and README titles were inventoried before implementation; searches for interior admission and free-run splitting found no existing implementation of this contract. The closest Gap Fit, Lowest Fit and Bank Pack contracts were read. No global prior-art, patentability or novel allocation-algorithm claim is made. The contribution is a distinct instrumented research workflow and accepted-trace contract within this repository.

Gap Fit selects any identity within a smallest free run, including an interior identity when the whole pool is empty. Edge Fit rejects that interior admission, but permits the endpoint of a larger run while a singleton exists. Lowest Fit always chooses the lowest free numeric identity; Edge Fit permits the high endpoint. Bank Pack prioritizes a partially occupied fixed bank, whereas Edge Fit considers dynamically changing free intervals across the bank boundary. After zero is allocated, Edge Fit permits three while Bank Pack requires one. These differences are executable behavior, not cosmetic names.

The passive carrier, STM32 startup and register definitions, bounded UART driver, CRC encoder structure, qualification engine and desktop transport reuse jayis1-authored Gap Fit infrastructure. That electrical topology is deliberately not claimed as a new circuit. The no-split policy, endpoint display, persistent-neighbor sparse checks, independent component-count oracle and operating contract are device-specific. Internal UQ-prefixed identifiers remain implementation names; EF1 is the public protocol identity. The folder is self-contained and imports no sibling at runtime.

## Attack surface and threat model

Use only on equipment you own or have explicit written authorization to assess. Permission must cover enclosure access, diagnostic firmware changes, wiring, shared grounds, approved workloads, interruption, recovery and retention of evidence. General network testing permission is not permission for physical attachment. Start with a low-energy development fixture disconnected from consequential outputs and production dependencies. Stop if attachment changes voltage, temperature, ground potential, resets or expected operation.

The investigated surface is resource bookkeeping across allocation, cancellation and reuse. Relevant defects include selecting from a stale free bitmap, confusing a circular array with a line, bypassing endpoint checks in a fast path, or treating a returned boundary as if it remained occupied. An authorized red team can ask the owner to expose the reduced sequence behind a suspected availability weakness. Researchers can compare deliberately correct and defective fixture allocators. Penetration testers can repeat an agreed harmless workload after a fix. The device itself issues no workload, exploit payload, network command or target-side write.

Trusted components include the operator, diagnostic adapter, installed observer image, purchased MCU module, wiring and workstation. The adapter must report every committed operation in order. Missing or invented indications can invalidate reconstruction. A malicious target can emit plausible diagnostics while behaving differently internally, so neither a clean trace nor a violation substitutes for independent evidence. CRC protects against accidental corruption, not authentication, forgery or malicious replay.

Receive-only is the GPIO configuration of this image, not an irreversible electrical data diode. Replacement firmware could drive pins through the resistors. There is no secure boot, signed evidence, persistent boot identity, secret store, remote updater or encrypted transport. Physical SWD can replace the image. The companion does not listen on network sockets or automatically upload exports. Timing and operational metadata can still be sensitive; store exports under the engagement's access and retention rules.

Do not connect this unisolated prototype to mains-referenced circuits, medical equipment, vehicles, occupied access controls, dangerous machinery or unidentified field wiring. Common ground and input loading can disturb a target even when software never transmits. A legal disclaimer does not provide voltage protection or establish regulatory compliance.

## Hardware specification and BOM

The active processor is an external ST NUCLEO-F401RE module containing STM32F401RET6, Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware assumes nominal reset-default 16 MHz HSI. SysTick supplies nominal one-millisecond ticks. USART2 transmits 115200 baud, eight data bits, no parity and one stop bit through PA2 to ST-LINK virtual COM. The purchased module provides USB connectivity, regulation, decoupling, reset and SWD programming; those facilities are not omitted components of a claimed standalone MCU PCB.

The custom carrier is 80 by 70 mm with two copper layers, nominal 1.6 mm board thickness and 0.3 mm tracks. Fourteen through-hole components provide thirty pads and nine nets. It connects to the module with short labelled leads, not by mechanically stacking as an Arduino shield. There is no radio, antenna, FPGA, battery, charger, storage card or analog sensor. There are no mounting holes or production enclosure; use nonconductive supports, underside insulation and strain relief.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | NUCLEO-F401RE; verify purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness/support | 1 set | Short insulated wiring and protected support |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Headers use the standard KiCad Connector_PinHeader_2.54mm library. Confirm physical body dimensions, lead diameter, drill fit and orientation against purchased parts before fabrication. The external module is USB-powered from a suitable 5 V source. Reserving 500 mA is a planning allowance, not measured consumption or a compliance claim. The passive carrier has no supply rail and cannot power the target.

| Signal | Target header | MCU harness | Nucleo endpoint | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Pin two of every target header is ground. Both J1 contacts are ground, not power. J2 contains four signals and no ground; connect module ground separately to J1. Each target-side _IN net reaches its MCU-side net through 1 kOhm, and each MCU-side signal has a 100 kOhm pulldown. A disconnected lead tends low but cannot be distinguished from a legitimate low signal. This is not a cable-continuity monitor.

Only known common-ground zero-to-3.3 V push-pull diagnostics are supported. Five-volt, negative, 1.8 V, differential and unidentified signals are unsupported. There are no level shifters, TVS components, qualified clamps, fuses or isolation barriers. Power the observer before enabling target outputs; connect ground before signals. Disable target outputs before removing observer power to avoid back-powering through input structures. Series resistance is not permission to exceed electrical limits.

Manufacturer references: ST UM1724, Table 16 for CN8 mappings and section 7.10 for USART routing; RM0368 for MCU registers; STM32F401xD/xE datasheet for electrical limits. The manual is https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf . The indexed manufacturer manual confirms the four mappings above and default virtual COM routing with SB13/SB14 ON and SB62/SB63 OFF. Inspect the actual board revision and solder bridges rather than assuming a used module remains at factory settings. Passive-only ERC cannot validate the external supply or harness.

## Architecture and firmware decisions

```text
Owner's committed-operation diagnostics      Passive carrier      External MCU
TAG0 ---------------------------------------- 1k --+-------------- PA0
TAG1 ---------------------------------------- 1k --+-------------- PA1
ALLOCATE ------------------------------------ 1k --+-------------- PA4
RELEASE ------------------------------------- 1k --+-------------- PB0
                                                   | each channel
                                                  100k
                                                   |
Target GND --------------------------------------- GND ------------ USB GND

GPIO -> continuity guard -> bundled tag / independent strobe qualification
     -> prior setup check -> occupancy -> free-run endpoint predicate
     -> transactional ledger -> EF1 CRC encoder -> bounded UART TX
     -> ST-LINK virtual COM -> Linux Ledger / History / Integrity screens
```

GPIOA and GPIOB are read sequentially rather than simultaneously. This is slow diagnostic instrumentation, not instruction-speed tracing. Start with twenty milliseconds stable tag, twenty milliseconds selected strobe high and twenty milliseconds both strobes low per operation. Independently inspect waveforms before reducing those margins. Pulses entirely between samples can be invisible; simultaneous operations cannot establish an order and are rejected rather than arbitrarily serialized.

The tag qualifies as one two-bit bundle; ALLOCATE and RELEASE qualify independently. Each group needs three consecutive unchanged intervals after its candidate first appears. The tag also needs five prior stable qualified intervals at the operation edge. Raw and qualified tag must agree. The selected strobe must still be raw high, and its opposite must be clear in raw, previous qualified and current qualified values. Startup WAIT exits only with all groups qualified and both strobes clear. That electrical baseline does not prove actual target emptiness.

Every admission increments a global ordinal, per-tag cumulative use and latest admission stamp. Occupancy, depth, peak and overlapping-admission totals update together. Return clears the selected occupied bit and may close an occupancy episode. Counters and stamps survive return and full drain, but do not determine endpoint eligibility. Firmware uses fixed storage and no dynamic allocation. The eligibility predicate checks boundary identity or an occupied immediate neighbor, after rejecting occupied candidates.

Each sample is evaluated in a temporary Ledger. If any counter exhausts, the speculative update is discarded and the previous coherent state halts. Sampling skips halt rather than inventing continuous qualification. Repeated timestamps do not create observations. Unsigned timestamp differences support normal wrap; a gap is terminal, not silently repaired. After halt the semantic evidence freezes while device time and the transmission-failure diagnostic may advance. Those timestamps are not valid acquisition progress.

The driver configures target GPIO as inputs without internal pulls, enables USART transmission only and starts SysTick. Watchdog startup waits are bounded; a startup failure appears before initial status. UART service is nonblocking and transmits at most one byte per poll, abandoning a queued frame after one hundred nominal milliseconds. Boot telemetry is attempted once, policy repeats every second, and status is scheduled approximately every hundred milliseconds when transport is available. Physical scheduling, oscillator error and watchdog behavior still require bench testing.

## Protocol and companion application

EF1 frames are ASCII: marker, record kind, comma-separated unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-CCITT-FALSE uses polynomial 0x1021, initial 0xFFFF, no reflection and no final XOR; it covers all bytes before the asterisk. B carries reset flags. P carries [3,1,5,4,0,1]: qualification intervals, nominal sample period, setup intervals, identity count, linear topology and policy revision. Periodic P enables late attachment without pretending a reset occurred.

S fields are now, levels, valid, halted, faults, phase, active, depth, stable, observed, rounds, attempts, launched, completed, rejected, gaps, missed, queued, peak, use0 through use3, stamp0 through stamp3, and tx_fail. The gaps counter means acquisition gaps, not spatial free runs. WAIT and TRACK are phases zero and one. Fault bits in ascending order mean interior split, orphan release, duplicate admission, interface ambiguity, acquisition gap, counter exhaustion and watchdog startup. Only one terminal reason is retained.

Run `python3 app/console.py` from this directory on Linux with Python and Tk installed. No third-party Python package or network service is required. New live opens the entered serial path read-only, preserving terminal attributes and refusing a second connection until disconnected. It restores settings on close or constructor failure where possible. Replay opens a bounded local recording. Export JSON saves a private evidence snapshot including source, provenance, errors, warnings, latest state and retained history.

Ledger displays counters, occupied identities, free intervals, eligible endpoints and findings. History retains at most 256 accepted snapshots. Integrity shows compatibility, reset information, parser errors, live-versus-replay provenance and freshness. Exports use exclusive creation, no symlink following and mode 0600; existing files are not overwritten. Replay is limited to regular files no larger than two MiB. FIFOs, device files and incomplete tails are not silently accepted as complete evidence. Delete retained exports using normal operating-system tools according to authorization terms.

Host validation checks unsigned bounds, conservation, unique latest ordinals, coherent fault context and monotonic history. Sparse transitions derive each tag's returns from admission growth and occupancy change. Identities continuously free or continuously busy constrain possible intermediate occupancy. Two continuously free neighbors prevent a hidden interior admission even when that identity was returned before the next snapshot. Conversely, an endpoint may be filled first, making its neighbor legally admissible; endpoint-only comparison must not reject that history.

For intervals with at most eight accepted operations, a memoized count-constrained search requires some legal ordering. Larger intervals use conservative permanent-occupancy checks without unbounded recursion. These are necessary consistency checks, not proof of actual hidden history: detailed waveform timing and ordinal order are not completely reconstructed. Sparse valid endpoints can conceal a transient policy violation, and forged plausible data remains possible.

A live label requires a connected compatible session, advancing observed acquisition in TRACK, no halt or frozen warning and recent reception. Duplicates do not refresh freshness. Reception pauses or bounded host/device drift freeze interpretation until explicit reconnect. Counter rollback and timestamp regression detect some resets, but without persistent boot identity certain resets remain indistinguishable. Reconnection does not establish target emptiness or renew a research baseline.

## Build, validation and deployment

Run `make -C firmware clean all test`. The target build uses arm-none-eabi-gcc, Cortex-M4 Thumb soft-float, C11 and warnings as errors, linking the supplied startup and linker map. Outputs are firmware/build/edge-fit.elf and edge-fit.bin. Host tests compare repeated operations against an independent free-component-count oracle, cover every occupancy subset, test mirrored admissions, arbitrary returns, immediate reuse, malformed strobes, acquisition loss, timestamp wrap and transactional exhaustion. Separate tests exercise bounded UART and production main-loop scheduling.

Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v`. The real C encoder supplies legal, repeated-renewal and interior-split fixtures. Actual Tk screens and Linux pseudo-terminals exercise receive, disconnect, attribute restoration and exports. Tests also cover malformed framing, incompatible policy, reset suspicion, drift, forbidden sparse histories and legal hidden neighboring admissions. Fixtures are synthetic software execution, not measurements taken from a physical MCU.

Run `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9 Python bindings, followed by `/usr/bin/python3 tools/check_hardware.py`. The checker invokes native ERC/DRC, exports a fresh netlist and compares every reference, pin, net, value and footprint identity across schematic, PCB and manifest. Native project files, authored component symbols and saved netlist are in kicad/. Render and visually inspect schematic labels and board routing in addition to machine checks.

Before fabrication verify purchased parts and fabrication constraints. Before powered testing perform unpowered continuity and short checks, validate voltage domains and harness orientation, review the image and program only the owned observer with an appropriate ST-LINK workflow. No automated flashing is included. Compare controlled fixture pulses against an independent logic analyzer, then establish a known empty target and run an authorized workload. Preserve target build, adapter mapping, image hash, board revision and independent waveform evidence. Fabrication readiness, physical safety and target security conclusions require separate review beyond the executable checks in this package.
