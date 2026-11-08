# Bank Pack — Partial-Bank-First Allocation Witness

Author and creator: jayis1. Hardware revision A. Wire protocol BK1, policy revision 1.

Status: unbuilt engineering prototype. This release contains target-linked C firmware, a functional Linux desktop companion, and routed native KiCad carrier files. Executed checks are recorded in VERIFICATION.md. Fabrication, physical MCU programming, electrical qualification, calibrated timing and real-target interoperability are not claimed. This device observes diagnostic indications; it is not an allocator, isolation boundary, safety interlock, or certificate of security.

Original firmware, application and tests: GPL-2.0-only. Original CAD tools: MIT. Original carrier: CERN-OHL-S-2.0. Purchased modules, standard component libraries and installed tools retain their upstream provenance and licenses.

## Purpose and practical utility

Bank Pack detects unnecessary opening of a resource bank when a partially occupied bank already has room. Consider a security service with four request slots distributed across two independently managed scratch banks. A consolidation policy promises to fill a partial bank before bringing another empty bank into use. A cancellation or fast-path allocation bug can violate that promise while every individual slot remains uniquely owned. Occupancy-only monitors therefore see no collision, and aggregate resource counts remain correct. The instrument reconstructs occupancy from committed-operation indications and evaluates the destination at each admission.

A useful laboratory experiment maps two descriptors into bank A and two into bank B. The owner provides a slow diagnostic adapter reporting accepted allocation and release operations. Harmless workloads exercise cancellation, arbitrary completion order, complete drain, and repeated immediate reuse. Bank Pack stops at the first operation that unnecessarily opens an empty bank, preserving the last coherent occupancy and accounting history. The output can narrow a firmware reproduction to a particular allocation decision without recording credentials, memory addresses, payloads or customer identities.

Consolidation can matter where an empty bank can be quarantined, inspected, power-gated, or reserved for a later isolated workload. However, this instrument does not observe any of those physical actions. A policy violation is evidence of a bookkeeping contradiction, not proof of information leakage, excess power consumption, failed erasure, or an exploitable vulnerability. The researcher must separately establish the target-specific security consequence and determine whether diagnostic instrumentation changes the behavior being investigated.

## Exact contract and distinguishing examples

There are four unique resource identities, numbered zero through three. Bank A contains zero and one; bank B contains two and three. Each identity can have at most one live allocation. The experiment begins with all four resources independently established as free. On ALLOCATE, the selected identity must be free. If either bank contains exactly one live resource, the allocation must fill a free slot in a partial bank. If no bank is partial, any free identity is legal. All ties are unrestricted. A full pool has no eligible allocation.

RELEASE must name an occupied identity. It removes only that identity, without imposing release order or requiring migration of survivors. Releases can legitimately leave both banks partial. This is an admission policy, not an invariant that the current occupied set always uses the smallest possible number of banks. Treating every two-partial-bank snapshot as a fault would incorrectly reject legal completion histories. Firmware and companion deliberately preserve this distinction.

Starting empty, allocate three is legal, and the next allocation must be two. After both are occupied, either zero or one may open bank A. Starting with only zero occupied, allocating two is a packing fault because one remains available in bank A. Starting with all four occupied, release zero and two; either zero or two can now be allocated because both choices fill a partial bank. Starting empty, allocate three, release three, allocate three is legal: there is no recency or round-robin requirement. These examples appear in executable tests.

Each accepted admission increments a global ordinal, a per-tag cumulative use counter, and that tag's retained last-admission stamp. These values support accounting and evidence review; they do not decide eligibility. Current bank populations alone determine packing. Returns preserve historical counts and stamps, and a completely drained pool has no preferred first bank. Observer reset discards all volatile history and requires a new target baseline. Host reconnection alone neither resets the target nor establishes that it is empty.

## Repository-local originality and reuse

The actual destination is jayis1/hacker-devices, with main as the remote default branch. The working tree was checked and pulled before implementation. Existing directory names and README descriptions were inventoried, and consolidation, fragmentation, packing and partial-occupancy terminology were searched. The closest policy and electrical designs were read before selecting this concept. The inspected repository did not contain this partial-bank-first admission contract. No worldwide prior-art or patentability claim is made; bank packing is established engineering, not newly invented mathematics.

Bank Separation forbids selected pairs from coexisting, using a nontransitive conflict graph. Bank Pack permits every occupancy subset and instead checks whether an admission unnecessarily opened an empty bank. Cursor Fit remembers a cyclic search position, while this device permits either initial bank and any tie between partial banks. Oldest Free compares admission recency, which is irrelevant here. The distinction is observable: repeated same-tag use after full drain is legal here, and a numeric rotation that opens the other bank prematurely is not.

The passive carrier, STM32 register definitions, startup, UART queue, CRC, acquisition qualification and desktop transport reuse jayis1-authored Cursor Fit infrastructure. The bank eligibility rule, bank-population display, sparse partial-bank barrier check, independent population oracle and workflow are device-specific. Internal UQ-prefixed identifiers are inherited implementation names. BK1 and the policy tuple define the external contract. The folder is self-contained: it does not import another device at runtime, add a shared systems tree, or change unrelated repositories.

## Attack surface, trust boundaries and authorized use

Use only on equipment you own or have explicit written permission to assess. Authorization must cover enclosure access, diagnostic firmware changes, common-ground attachment, approved workloads, possible service interruption, recovery and evidence retention. General network penetration-testing permission is not permission to alter physical equipment. Begin with a low-energy development fixture, disconnected from consequential outputs and production dependencies. Stop if attachment changes temperature, grounding, voltage levels or expected operation.

The investigated attack surface includes stale bank-population caches, cancellation bookkeeping, wrong-bank fast paths, pool expansion, and incorrect completion identity. Authorized red teams can ask the owner to expose a suspected availability or isolation-policy regression through a harmless reduced workload. Security researchers can compare allocator revisions and release interleavings. Penetration testers can repeat the approved trace after remediation and attach qualified evidence to an engagement report. The device implements no exploit payload, covert deployment, credential extraction, network discovery, radio transmission or target command channel.

The adapter, observer firmware, purchased MCU module, workstation and operator are trusted. An adapter can omit, reorder or fabricate operations. A compromised host can alter exports. Physical programming access can replace firmware. CRC detects accidental corruption, not malicious forgery or replay. There is no secure boot, signed evidence, encrypted store, persistent boot identity, trusted wall clock or authenticated remote updater. Do not interpret a passing trace as a security guarantee against a malicious target. Timing records can still reveal operational details and require access-controlled retention.

The supplied firmware configures diagnostic GPIO as inputs, disables UART reception and provides no command parser. The companion opens serial read-only. Receive-only is a software configuration, not an irreversible electrical diode: replacement firmware could drive GPIO. Even an input can disturb equipment through loading, common ground, wiring errors or back-powering. Exclude mains-referenced circuits, occupied access controls, vehicles, medical devices, dangerous machinery and unknown field wiring. A legal disclaimer does not establish electrical safety.

## Hardware specifications and component selection

The processor is an external ST NUCLEO-F401RE development board containing an STM32F401RET6 Cortex-M4, with 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI operation. SysTick provides nominal one-millisecond acquisition ticks. USART2 TX uses PA2 at 115200 baud, eight data bits, no parity and one stop bit, connected through the module's ST-LINK virtual COM bridge. The purchased module supplies regulation, decoupling, reset, USB and SWD; these are not components deceptively omitted from a standalone MCU design.

The custom passive carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm tracks. It contains fourteen through-hole components, thirty pads and nine nets. It connects by short labelled leads rather than stacking as a shield. There are no radios, antennas, FPGA, analog sensors, batteries, chargers, removable storage or target power output. Use nonconductive supports, strain relief and an underside cover. Mounting holes and a production enclosure are not included.

| References | Quantity | Candidate part / specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, verify actual revision |
| R1, R3, R5, R7 | 4 | Yageo MFR-25FBF52-100K, 100 kOhm, 1%, axial |
| R2, R4, R6, R8 | 4 | Yageo MFR-25FBF52-1K, 1 kOhm, 1%, axial |
| J1, J3, J4, J5, J6 | 5 | Samtec TSW-102-07-G-S, 1x2, 2.54 mm header |
| J2 | 1 | Samtec TSW-104-07-G-S, 1x4, 2.54 mm header |
| Harness/support | 1 set | Insulated short leads and nonconductive base |

These are procurement candidates, not verified stock or price claims. Check supplier drawings, lead diameter, resistor body, 7.62 mm lead forming, header mating height, plating and drill fit before ordering. Standard DIN0207 and vertical 2.54 mm header footprints are used. Reserve a 5 V, 500 mA USB source allowance for the purchased module as planning headroom, not measured consumption. The carrier has no supply rail and cannot power a target.

| Signal | Target contact | MCU contact | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Each target connector's second pin is ground. Both J1 pins are ground, never supply inputs. J2 carries only signals, so a separate module ground lead to J1 is mandatory. Each target-side signal passes through 1 kOhm to its MCU node; that node has a 100 kOhm pulldown. A disconnected signal looks low, so the circuit cannot establish cable continuity. The nominal static load is approximately 101 kOhm, excluding leakage and dynamic capacitance.

Support only verified common-ground push-pull zero-to-3.3 V signals. Exclude 5 V, 1.8 V, negative, differential and unknown-voltage signals. Series resistance is not galvanic isolation, surge suppression or dedicated ESD protection. No level translation, ESD clamp or hot-plug guarantee is supplied. Verify logic thresholds, source impedance, ground offset and harness capacitance on the assembled fixture. Power the observer before enabling target outputs; disable those outputs before depowering it to avoid clamp back-power. USB and target grounds become common.

## Architecture and block diagram

```text
Owned target adapter           Passive carrier          External Nucleo
TAG0 -------------------------- 1k --+------------------- PA0
TAG1 -------------------------- 1k --+------------------- PA1
ALLOCATE ---------------------- 1k --+------------------- PA4
RELEASE ----------------------- 1k --+------------------- PB0
                                     | each MCU node
                                    100k
                                     |
Target ground ---------------------- GND --------------- module GND

GPIO -> continuity guard -> tag bundle / independent strobe qualification
     -> prior setup checks -> partial-bank-first admission comparison
     -> transactional occupancy, counts and stamps -> bounded BK1 encoder
     -> USART2 TX -> ST-LINK USB -> Ledger / History / Integrity -> JSON
```

GPIOA and GPIOB are read separately, so cross-port edges are not atomically latched. This is a deliberately slow diagnostic observer, not a native-speed bus analyzer. The adapter must export actual committed operations rather than attempted requests, preserve their order, and never silently drop its own queued records. If slowing or instrumenting a target removes the suspected failure, this measurement method is unsuitable.

## Firmware design and failure behavior

C11 firmware uses no RTOS, heap, network stack or vendor SDK. Startup installs vectors and initializes data and BSS. The linker defines flash and SRAM, with stack headroom. Board support configures GPIO, reset-cause capture, SysTick, USART and independent watchdog. The main loop acquires whenever time advances, services a bounded UART queue, and schedules status every nominal 100 ms. Policy repeats every nominal second, allowing late attachment without reusing boot records as compatibility announcements.

The two tag bits qualify as a bundle; allocation and release strobes qualify independently. A candidate followed by three matching intervals establishes qualification. Before accepting a strobe edge, the previously qualified tag must have five stable observations. Raw tag and strobe must agree at acceptance, and the opposite strobe must be low in previous qualified, current qualified and raw states. Simultaneous rises are rejected as ambiguous, not assigned invented order. A held strobe contributes one event.

Use twelve nominal milliseconds of stable tag with both strobes low, six with exactly one strobe high, and six low afterward. Tests use this conservative sequence. These are software intervals, not calibrated electrical timing limits. Short pulses may be filtered or missed entirely. Quiet wires cannot establish an empty pool; the operator must independently reset the experimental target state before beginning.

Repeated timestamps are ignored. A skipped tick terminates interpretation rather than inventing continuous acquisition. Ordinary unsigned timestamp wrap is supported. Interface ambiguity precedes semantic policy checks, duplicate admission precedes packing, and orphan return has a separate reason. Every sample operates on a speculative state copy; counter exhaustion discards that entire transition before marking prior coherent evidence terminal. Counts, occupancy and stamps cannot half-commit. Watchdog service continues after a policy halt to retain evidence instead of intentionally erasing it through reboot.

Fault bits are packing 1, orphan release 2, duplicate allocation 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog startup failure 64. Halt clears qualification validity and freezes accepted history, while timestamps and transport diagnostics may continue. Retained occupancy is the observer's last coherent state, not a statement about target occupancy after the violating operation. Reset requires a fresh experiment; reconnect does not clear an embedded fault.

## Application and software interface

Run `python3 app/console.py` on Linux with Tk support. No third-party Python package is required. Enter an explicitly verified serial path and choose New live. There is no discovery sweep or automatic connection. Ledger displays occupancy, bank populations, currently eligible tags, per-tag counts, retained stamps and findings. History shows up to 256 accepted snapshots. Integrity retains source, provenance, compatibility, parser errors, reset information, freshness and freeze reasons. The app does not command the target or enforce allocation policy.

BK1 records contain comma-separated ASCII decimal integers, then an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT begins at 0xffff and covers bytes before the asterisk. B records carry reset flags. P records carry `[3,1,5,4,2,1]`: qualification intervals, nominal sampling period, prior setup, pool size, adjacent slots per bank and policy revision. S order is defined in `app/contract.py` and emitted identically by `firmware/wire.c`. Failed serialization returns no record rather than publishing a partial status.

The companion checks unsigned bounds, occupancy, retained reason plausibility, unique nonzero stamps and conservation. Admissions equal releases plus live depth and equal the sum of tag use counts. Attempts equal admissions plus releases plus rejected edges. Per-tag release counts are derived from use growth and membership changes. If a bank retains one continuous incumbent and a continuously free sibling, it remains partial: an initially empty other bank cannot open, even if its new allocation is released before the next snapshot. This sparse check is necessary, not a complete reconstruction of all hidden histories. Releasing the incumbent or filling the sibling can legitimately remove that obligation.

Two advancing compatible records are needed for live progress. Duplicate timestamps do not refresh freshness. Contradictory duplicates, counter rollback, later boot, impossible transitions, reception pauses and excessive cumulative host/device divergence freeze interpretation. Drift allowance is capped so it cannot subsidize unlimited backlog. Uncalibrated oscillators can cause conservative rejection, and some resets without persistent boot identity remain indistinguishable from forward traffic. Advancing terminal timestamps mean transport activity, not continued acquisition.

Disconnect preserves evidence and restores terminal settings. A second connection is refused until the first closes. Replay accepts regular files of at most two MiB, rejects devices and FIFOs, and counts truncated final frames. Resynchronization searches the complete BK1 marker. Export exclusively creates an owner-only JSON file and refuses overwrite, preserving provenance and integrity qualifications even for empty sessions. Nothing is uploaded automatically. Exports are not encrypted or signed; retain and delete them according to engagement policy.

## Build, verification and bench workflow

From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

ARM GNU GCC with newlib/nosys support builds the actual Cortex-M4 image. Host GCC runs sanitizer-backed engine and UART tests. KiCad 9, its Python bindings and standard footprints validate CAD. Use distro Python when an environment lacks pcbnew. Tk and Xvfb enable headless GUI execution. The hardware generator reproduces schematic symbols, nets, footprints, routed tracks, outline, project settings and connectivity manifest. Build products and KiCad personal preferences are ignored.

| Capability | Implementation | Verification |
| --- | --- | --- |
| Diagnostic acquisition | J2, board driver, qualifier | Setup, raw conflict and gap tests |
| Packing decision | `ledger_eligible`, bank display | Independent population oracle |
| Accounting integrity | Transactional engine and encoder | Conservation, exhaustion, malformed snapshots |
| Local evidence | UART, parser, desktop screens | Real C fixtures, PTY, replay and export tests |
| Electrical connectivity | Native schematic and routed PCB | ERC, DRC, exact pad/net/value comparison |

Before fabrication, review the rendered schematic, native PCB, footprints and actual development-board revision. ST UM1724 documents Nucleo connectors and VCP solder bridges; RM0368 and the STM32F401xD/xE datasheet describe registers and electrical limits. Manufacturer-indexed UM1724 text confirms USART2 PA2/PA3 routing and default SB13/SB14 enabled with SB62/SB63 disabled. Check the complete current manual for the purchased board rather than relying only on an indexed excerpt: https://www.st.com/resource/en/user_manual/um1724-.pdf .

Inspect an unpowered carrier for shorts, resistor values and connector orientation. Power the observer with target disconnected and check idle telemetry, then attach a verified low-energy fixture with outputs disabled. Program using the module's supported SWD procedure after reviewing power configuration. Confirm reset-default clock assumptions rather than inheriting an unknown bootloader state. No physical flashing was performed in this release.

Exercise both initial bank choices, sibling completion, full occupancy, two partial banks produced by releases, complete drain and deliberate synthetic policy violations. Compare independent waveforms and target logs with the observer. Do not create a hazardous real target condition merely to test a finding. Record tag mapping, firmware revision, adapter behavior and workload scope with exports. A clean trace says only that accepted diagnostic observations matched this contract; it proves neither confidentiality nor sanitization, fairness, bounded latency, or production readiness.
