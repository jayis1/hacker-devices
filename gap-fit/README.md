# Gap Fit — Shortest-Free-Run Admission Witness

Author and creator: jayis1. Hardware revision A. Wire protocol GF1, policy revision 1.

Status: unbuilt engineering prototype design with target-linked firmware, a functional Linux desktop companion and native routed KiCad carrier sources. Executed checks appear in VERIFICATION.md. Fabrication, flashing, electrical bring-up, oscillator calibration and physical target interoperability have not been performed. This is a diagnostic observer, not an allocator, protective interlock, memory debugger or security certification.

Original firmware, application and tests: GPL-2.0-only. Original CAD utilities: MIT. Passive carrier: CERN-OHL-S-2.0. Installed tools, standard footprint libraries and purchased modules retain their upstream provenance and licenses.

## Purpose and practical value

Gap Fit examines an allocator decision that ordinary occupancy checks cannot explain: when several contiguous free regions exist, does a single-slot admission consume a slot from a smallest free region, rather than unnecessarily consuming a larger region? An embedded service can maintain correct exclusive ownership and correct total capacity while violating this declared fragmentation policy. The instrument independently reconstructs four adjacent resource identities from accepted admission and return indications. It identifies the first allocation inconsistent with the shortest-gap contract and preserves the preceding coherent state.

A motivating laboratory system is an owner-instrumented gateway with four adjacent staging descriptors. Small requests need one descriptor. A future operation may need a contiguous region, although this observer does not accept multi-slot operations. A cancellation path can create an isolated vacancy next to a larger available interval. A buggy fast path that selects by numeric rank, cached free-list head or bank identity can consume the larger interval while the singleton remains vacant. The experiment isolates that selection mistake without reading customer payloads, memory addresses, credentials or cryptographic material.

This device does not promise to minimize fragmentation globally. Even a correct shortest-gap choice can split its selected interval because the contract permits any identity within a tied smallest interval. It does not model variable request sizes, compaction, alignment constraints, real byte addresses or future demand. Its research question is narrow and testable: does the target implement its explicitly declared single-slot shortest-free-run policy? A policy contradiction may motivate an availability investigation, but is not itself proof of denial of service or exploitability.

## Contract, examples and repository distinction

The resource topology is the fixed line zero—one—two—three. Endpoints do not wrap. Every tag names one unique resource, not a class containing several simultaneous objects. At the start, all resources must independently be known free. Every accepted ALLOCATE occupies one currently free identity. Every accepted RELEASE vacates exactly one occupied identity. Releases can occur in any order. Duplicate admissions and orphan returns are independent findings.

A gap is a maximal contiguous interval of free identities. On allocation, compare the sizes of all current gaps. The selected tag must belong to a gap of minimum size. Equal-size gaps tie, and every tag inside any minimum gap is eligible. This rule is evaluated immediately before admission. It does not require relocation or eviction after a release merges free intervals. Current occupancy determines eligibility; elapsed time, lifetime use counts, last-admission stamps and return order do not assign preference.

Starting empty, all four tags are legal because they share one gap. Allocate one first: free gaps are now [0] and [2,3], so only zero is eligible. Allocating two or three at that point is a finding. Conversely, allocate two first: gaps [0,1] and [3] mean only three is eligible, despite its higher numeric rank. After one and zero are occupied, two and three share a tied minimum gap and either may be chosen. A full pool has no eligible admission. A completely drained pool again permits every tag.

The topology matters. Zero and three are not neighbors. With one occupied, free zero must not be joined to the interval containing two and three as if the pool were circular. Returning the occupied separator can merge gaps and legitimately remove the earlier restriction. Filling the small gap can also remove it. A host that simply compares initial and final eligible sets would wrongly reject those legal hidden histories; the companion checks per-tag lifetime changes before drawing conclusions.

The configured destination is jayis1/hacker-devices on main. Device directories and README purposes were inventoried, and shortest-gap, smallest-hole, best-fit and fragmentation terminology was searched before choosing this design. The closest contracts were read. Lowest Fit always selects the lowest free numeric identity; Gap Fit can require the highest one. Bank Pack prioritizes fixed half-bank occupancy; Gap Fit follows dynamic contiguous intervals that can cross the boundary between one and two. Cursor Fit retains a cyclic cursor; Gap Fit has no remembered selection position. Reserve Last privileges ordinary tags over one reserve tag; Gap Fit gives no identity a fixed class.

For example, after allocating one, Bank Pack and Gap Fit both permit zero, but after allocating zero, Bank Pack requires one while Gap Fit permits one, two or three in the single remaining run. These are distinct accepted-trace sets, not cosmetic product renames. The repository-local contribution is the topology-sensitive admission contract, explicit free-gap display, bounded sparse-history checks and independently tested implementation. No worldwide novelty, patentability or new allocation-algorithm claim is made; best-fit allocation is established engineering.

The passive circuit, register-level platform, startup, CRC, bounded UART, qualification and Linux transport intentionally reuse jayis1-authored Reserve Last infrastructure. The resistor carrier is not claimed as a newly invented circuit. Internal UQ-prefixed C names are retained implementation identifiers; GF1 identifies the distinct public protocol. This folder is self-contained at runtime and imports no sibling device, unified hardware tree or shared command framework.

## Attack surface, trust model and authorized use

Use only on equipment you own or have explicit written authorization to assess. Permission must cover enclosure access, diagnostic firmware changes, attachment points, common-ground wiring, permitted workloads, interruptions, recovery and evidence retention. Network penetration-test permission does not automatically authorize physical instrumentation. Start with a low-energy development fixture disconnected from production dependencies and hazardous outputs. Stop if connection changes voltage, temperature, timing, resets or expected operation.

The assessed surface is resource bookkeeping across allocation, cancellation, return and reuse. Relevant defects include stale free-run lengths, a search that ignores a newly isolated vacancy, circular arithmetic applied to a linear array, and a fast path that bypasses the declared selection rule. Authorized red teams can ask the owner to export a reduced allocation sequence using harmless dummy operations. Security researchers can compare correct and deliberately defective allocators. Penetration testers can replay an agreed workload after remediation. The instrument does not generate the workload, issue target commands, install persistence, capture credentials or transmit radio signals.

The operator, target diagnostic adapter, observer image, module, wiring and workstation are trusted. Target signals, serial bytes and replay files are treated as potentially malformed. The diagnostic adapter must report committed accepted operations in their actual order, not attempted requests that the target correctly refused. Silent omissions can invalidate the reconstruction. A malicious target can emit a perfectly plausible trace while violating its internal contract. Therefore neither a clean trace nor a finding establishes memory safety without independent corroboration.

Receive-only describes the target-facing GPIO configuration in the supplied image. It is not an immutable data diode: reprogrammed pins could drive through the series resistors. There is no secure boot, signed update service, encrypted transport, pairing, persistent boot identifier, remote administration or protected key store. CRC detects accidental corruption, not forgery. Physical SWD can replace the image. Preserve tool versions, image hashes, fixture provenance and waveform captures separately when evidence integrity matters.

Do not connect the unqualified carrier to deployed access controls, medical devices, moving vehicles, industrial safety equipment, mains-referenced circuits or unknown field wiring. Common ground can disturb an otherwise passive experiment. A disclaimer is not an electrical protection component. This design provides no isolation, surge qualification or guarantee that attachment is nonintrusive.

## Hardware specifications and bill of materials

The active processor is an external ST NUCLEO-F401RE development module with STM32F401RET6, Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI and uses nominal one-millisecond SysTick. USART2 transmits at 115200 baud, eight data bits, no parity and one stop bit. PA2 connects toward the ST-LINK virtual COM interface; receive is disabled. The purchased module supplies USB connectivity, voltage regulation, decoupling, reset, programming and debug. Those circuits are not falsely represented as components on the custom carrier.

The carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm routed tracks. It contains fourteen through-hole components, thirty pads and nine electrical nets. It connects with short labelled leads rather than mechanically stacking as an Arduino shield. There is no radio, antenna, FPGA, battery, charger, analog sensor, enclosure or mounting-hole pattern. Provide nonconductive support, strain relief and insulation beneath the board.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE; check purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2, 2.54 mm through-hole header |
| J2 | 1 | Vertical 1x4, 2.54 mm through-hole header |
| Harness/support | 1 set | Short insulated leads and protected support |

Resistors use DIN0207 body footprints with 7.62 mm lead spacing. Headers use the standard KiCad Connector_PinHeader_2.54mm library. Confirm purchased body dimensions, lead diameters, hole fit and connector orientation before fabrication. A suitable USB 5 V source powers the purchased module. Reserving 500 mA is a planning allowance, not measured consumption or a USB compliance claim. The passive carrier has no supply rail and cannot power the target.

| Signal | Target header | MCU harness | Nucleo endpoint | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Pin two of every target header is ground. Both contacts of J1 are ground only, not a supply input. J2 has four signals and no ground, so a separate connection from module ground to J1 is mandatory. Each target-side net ends in _IN and reaches its MCU-side net through 1 kOhm. The MCU-side net has a 100 kOhm pulldown to ground. Disconnection tends low but cannot be distinguished from a legitimate low input.

Only known common-ground zero-to-3.3 V push-pull diagnostics are supported. Five-volt, negative, 1.8 V, differential and unidentified signals are unsupported. There is no level shifter, TVS protector, qualified clamp, fuse or isolation barrier. Power the observer before enabling target outputs. Connect ground before signal leads and disable outputs before removing observer power; input structures can otherwise back-power the MCU. The series resistor is not permission to exceed pin limits.

Manufacturer references are ST UM1724 for connectors, power and solder bridges; RM0368 for the STM32F401 register map; and the STM32F401xD/xE datasheet for electrical limits. The module manual is https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf and the MCU product page is https://www.st.com/en/microcontrollers-microprocessors/stm32f401re.html . Verify the actual purchased board revision and ST-LINK virtual COM routing before programming or attachment. Passing passive-board ERC does not validate the external module supply or harness.

## Architecture and firmware design

```text
Authorized diagnostic outputs     Passive carrier       External Nucleo
TAG0 ----------------------------- 1k --+--------------- PA0
TAG1 ----------------------------- 1k --+--------------- PA1
ALLOCATE ------------------------- 1k --+--------------- PA4
RELEASE -------------------------- 1k --+--------------- PB0
                                        | each channel
                                       100k
                                        |
Target GND ---------------------------- GND ------------- USB GND

GPIO -> continuity guard -> bundled tag / independent strobe qualification
     -> prior setup check -> occupancy -> shortest maximal free-run selection
     -> transactional ledger -> GF1 CRC records -> bounded USART TX
     -> ST-LINK virtual COM -> Linux Ledger / History / Integrity screens
```

GPIOA and GPIOB are read sequentially, not atomically across ports. This is slow diagnostic instrumentation, not an instruction-speed trace analyzer. Begin with twenty milliseconds of stable tag, twenty milliseconds strobe high and twenty milliseconds both strobes low for each operation. Confirm actual waveforms independently before reducing those margins. A pulse between samples can be invisible. Simultaneous edges cannot establish an ordering and are rejected rather than arbitrarily serialized.

The two tag bits qualify as one bundle. ALLOCATE and RELEASE qualify independently, so chatter on one cannot postpone all observation of the other. Each group needs three consecutive unchanged intervals after its candidate is first observed. The tag additionally needs five prior stable qualified intervals at an operation edge. Raw tag and qualified tag must agree; the selected strobe must remain raw high; the opposite strobe must be low in raw, prior qualified and current qualified states. Startup WAIT exits only after full qualification with both strobes clear. This electrical baseline does not prove that the real target pool is empty.

The engine uses fixed storage and no dynamic allocation. A successful admission increments the global ordinal, per-tag use count and latest stamp; occupancy, depth, peak and concurrent-admission accounting update together. A return removes one occupied bit and may close an occupancy episode. Counts and stamps survive full drain but do not influence eligibility. Gap selection scans maximal free runs and compares their lengths. A duplicate allocation takes precedence over selection mismatch. Orphan release, interface ambiguity, acquisition loss, counter exhaustion and watchdog startup failure have distinct terminal reasons.

Every sample is computed in a temporary Ledger. If an increment exhausts a counter, the speculative observation is discarded and the previous coherent state halts. The rejected policy observation retains prior occupancy; it does not imply that the target rejected its already-announced operation. Acquisition skips halt instead of inventing continuous qualification. Repeated calls at the same timestamp do not create samples. Unsigned timestamp differences support ordinary timer wrap. After halt, semantic evidence freezes while the wall timestamp and transmission-failure diagnostic may continue. The companion explicitly does not label advancing halted timestamps as live acquisition.

The board driver configures target-facing pins as inputs with external pulldowns, enables only USART transmission and starts SysTick. The independent watchdog startup uses bounded waits; failure is reflected before initial status. UART service is nonblocking, transmits at most one byte per poll and abandons a frame after one hundred nominal milliseconds. Boot telemetry is sent once, policy telemetry repeats every second and status is scheduled approximately every hundred milliseconds when the queue is available. Actual scheduling and oscillator timing require bench measurement.

## Protocol and software interface

GF1 records are ASCII: marker, comma-separated record kind and unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits and LF. CRC is CCITT-FALSE with polynomial 0x1021, initial 0xFFFF, no reflection and no final XOR, covering bytes before the asterisk. B contains reset flags. P contains [3,1,5,4,0,1]: qualification intervals, nominal sample period, setup intervals, identity count, linear-topology code and policy revision. Repeating P supports late attachment without pretending a reset occurred.

S contains, in order: now, levels, valid, halted, faults, phase, active, depth, stable, observed, rounds, attempts, launched, completed, rejected, gaps, missed, queued, peak, use0 through use3, stamp0 through stamp3 and tx_fail. The gaps field counts acquisition discontinuities, not free spatial intervals; spatial gaps are derived from active. Phase zero is WAIT and phase one is TRACK. Fault bits represent shortest-gap bypass, orphan, duplicate, interface ambiguity, acquisition gap, exhaustion and watchdog startup respectively. Zero means no terminal finding.

The app requires Python with Tk on Linux; it has no third-party Python dependency, package download or network service. Run `python3 app/console.py` from this directory. Enter the owner's serial device path and choose New live. The app opens the port read-only, saves terminal attributes and refuses a second connection until disconnected. It restores settings on normal close and constructor failure where possible. Serial hangup removes live status without deleting retained evidence. Choose Replay for synthetic or previously captured GF1 data, and Export JSON to save a private snapshot. Replacing a session is deliberate; export important evidence first.

Ledger shows counters, occupancy, free intervals, eligible tags and findings. History retains up to 256 accepted snapshots. Integrity shows source, live-versus-replay provenance, parser errors, reset information, freshness and frozen warnings. Export uses mode 0600, exclusive creation and no symlink following, so it refuses overwriting an existing path. Replay is restricted to regular files of at most two MiB and a partial final frame is counted as an error. FIFOs and device files are not treated as recordings. Evidence is local; there is no automatic upload or cloud retention. Delete exports with the operating system according to the engagement's retention agreement.

Host validation checks unsigned bounds, conservation, distinct retained admission ordinals, legal fault context and counter monotonicity. For sparse transitions it derives returns per tag from admission growth and occupancy changes. Continuously free and continuously occupied tags constrain every possible hidden intermediate occupancy. If a newly admitted identity is ineligible in all such occupancies, the transition is impossible. For intervals containing at most eight accepted operations, a memoized count-constrained search also requires some legal topology ordering. Larger intervals skip that search and retain conservative bounds; hostile counts cannot cause unbounded recursion.

These checks are necessary evidence checks, not complete history reconstruction. The search ignores detailed waveform timing, last-stamp ordering and some episode details; finding one possible operation order does not establish the order the target actually used. Sparse valid endpoints can conceal a temporary policy violation. The physical observer sees each qualified operation, while transport snapshots may omit intermediate observations. Likewise CRC-valid data can still be forged by an untrusted source.

A live label requires a connected compatible session, advancing observed acquisition in TRACK, no halt or frozen warning and recent receipt. Duplicate timestamps do not refresh freshness. Reception pauses and bounded host/device drift force reconnect rather than silently promoting backlog to current evidence. Lost boot records are handled with timestamp and cumulative-counter rollback detection, but some resets remain indistinguishable without a persistent boot identifier. Reconnect does not establish an empty target pool or authorize renewed experimental history.

## Build, test, CAD and deployment workflow

From this directory run `make -C firmware clean all test`. The Makefile uses arm-none-eabi-gcc, Cortex-M4 Thumb soft-float, C11 and warnings as errors. It links startup, engine, encoder and drivers with the supplied memory map and emits build/gap-fit.elf and build/gap-fit.bin under firmware. Host tests include an independent flood-fill graph oracle, all occupancy subsets, repeated use, policy rejection, duplicate precedence, malformed setup, sampling loss, timer wrap, transactional exhaustion, bounded serialization, UART timeout recovery and production-main-loop telemetry. Tests are synthetic, not physical MCU execution.

Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py'` for the companion suite. Real C-encoder fixtures pass through the parser and actual Tk screens. Linux pseudo-terminals exercise live opening, receipt, disconnect and terminal restoration. Tests cover exports, refusal to overwrite, replay bounds, policy mismatch, malformed framing, reset suspicion, frozen evidence, drift and CRC-valid impossible sparse transitions. Synthetic fixtures must never be relabeled as hardware measurements.

Native KiCad sources are device.kicad_sch, device.kicad_pcb and device.kicad_pro in kicad/. Authored component symbols, the symbol-library table, saved XML netlist and connectivity manifest accompany them. With KiCad Python bindings installed for the system interpreter, run `/usr/bin/python3 tools/generate_hardware.py`, then `/usr/bin/python3 tools/check_hardware.py`. Generation is deterministic in symbol identity and explicitly credits jayis1. Checking exports a fresh schematic netlist, compares all reference/pin/net mappings, values and full footprint identities with PCB pads and the manifest, and invokes native ERC and DRC. Review rendered schematics and the physical footprint plan as well as machine checks.

Before fabrication inspect component dimensions, connector polarity, board clearance, drill fit, copper routing and protection requirements. Before connection perform unpowered continuity and isolation checks, establish compatible voltages, confirm module connector mapping, inspect the built binary and program only the owned observer via ST-LINK using an appropriate verified local tool. This package does not automatically flash hardware. Start with a controlled waveform fixture, verify each diagnostic pin independently and compare against an independent logic analyzer. Only then attach an authorized instrumented target with a known empty pool.

Record the target build, adapter mapping, observer image, instrument revision, serial provenance, workload and independent waveform evidence. Demonstrate both a legal singleton-first trace and an intentional larger-gap bypass on an isolated emulator. Repeat with mirrored slot identities to expose accidental numeric bias. Test separator release and small-gap consumption to ensure remediation has not simply forbidden useful allocations. Keep physical bring-up, manufactured-board quality, electrical disturbance, environmental behavior and security conclusions separate from the software and CAD checks reported here.
