# Bank Spread — Least-Occupied-Bank Admission Witness

Author and creator: jayis1. Hardware revision A. Wire protocol BD1, policy revision 1.

Status: unbuilt engineering prototype. Target firmware is compiled and linked for STM32F401RE; a Linux desktop companion and routed native KiCad carrier are supplied. VERIFICATION.md records executed checks. No physical programming, fabrication, electrical qualification, calibrated timing, or target interoperability is claimed. This is a diagnostic observer, not an allocator, isolation device, safety interlock, or security certification.

Original firmware, application and tests: GPL-2.0-only. Original CAD tools: MIT. Original carrier: CERN-OHL-S-2.0. Purchased modules, installed tools and component libraries retain their upstream licenses and provenance.

## Purpose and practical utility

Bank Spread investigates a resource-selection defect that can be invisible to ordinary capacity monitors: an allocator places a second concurrent operation into one bank while another bank is empty. A system can maintain correct unique ownership, total capacity and completion accounting while still violating its declared contention-spreading policy. The instrument observes committed allocation and release indications from a deliberately slow owner-supplied diagnostic adapter. It reconstructs current bank populations, checks the destination at admission, and retains the first contradiction along with the last coherent accounting state.

A useful laboratory model is an embedded cryptographic service with four equal reservation slots arranged as two pairs, one pair per execution bank. The service promises to prefer the less occupied bank to reduce avoidable concentration under concurrent workloads. The security researcher exercises harmless requests, cancellations and out-of-order completions while comparing the diagnostic trace against the external observer. A cached population, stale cancellation flag or incorrect bank-index calculation may select an unnecessarily crowded bank. Such a finding can motivate an availability investigation without collecting cryptographic keys, request payloads, addresses or customer identities.

The instrument does not measure utilization, CPU cycles, memory contention, energy, response latency or confidentiality. Two occupied slots can have very different workloads. A least-population choice is meaningful only when it is the actual policy promised by the owner, not when another scheduler legitimately uses weights, locality, affinity, unavailable-bank masks or priority. A contradiction is a reproducible policy finding, not automatic proof of denial of service or side-channel leakage. Establish the target-specific consequence separately before making security claims.

## Exact policy and examples

There are four unique identities: zero, one, two and three. Bank A contains zero and one; bank B contains two and three. Every identity permits at most one live reservation. Begin with all resources independently established as free. ALLOCATE selects a free identity whose bank has the minimum current population of the two banks. Either bank may win a tie; either free identity within that bank may be selected. A full bank has no free identity and cannot win an allocation. RELEASE must name an occupied identity and clears only that identity. Returns can occur in any order.

Starting empty, allocate zero is legal. The next admission must be two or three, because bank A has one resident and bank B has none. Once zero and two are live, either one or three is legal: the populations are tied. If zero, one and two are live, release two leaves bank A full and bank B empty. That state is legal because returns do not trigger migration, eviction or retroactive invalidation. The next admission must use bank B. The observer constrains selection, not the population difference of every retained snapshot.

A complete drain removes bank preference. Repeated allocate-three/release-three cycles are legal even when the other identities have never been used. Cumulative per-tag admissions and last-admission ordinals are retained as evidence, but do not influence eligibility. Conversely, a free sibling in a nonempty bank is ineligible while the other bank stays empty, regardless of historical fairness. A duplicate admission has its own fault reason and takes precedence over population comparison; an orphan release also has a separate reason. Interface ambiguity is checked before either semantic condition.

## Originality relative to this repository

The destination is jayis1/hacker-devices, not daily-ideas. Before implementation the configured remote default branch was confirmed as main, the repository was pulled, existing directories were listed, and README concepts were searched. The closest contracts inspected were Bank Pack, Bank Separation and Wear Balance. The interrupted bank-spread test was traced to an earlier execution of this same task and recovered rather than discarded or mistaken for unrelated work. No worldwide novelty or patentability claim is made: least-population scheduling is established engineering.

Bank Pack deliberately does the opposite when one bank is partial and the other empty: it fills the partial bank. Bank Separation forbids particular identity pairs from coexisting regardless of admission population. Bank Spread permits every occupancy subset, including two residents in the same bank after peers return. Wear Balance compares cumulative lifetime use among vacant identities; Bank Spread ignores lifetime use for selection and considers current bank populations instead. These differences are executable: the new regression fails against Bank Pack's engine at the first sibling-eligibility assertion.

The passive carrier, register definitions, startup, bounded UART driver, CRC, acquisition qualification and desktop transport reuse jayis1-authored Bank Pack infrastructure. The least-population rule, corresponding fault interpretation, sparse-history barrier, independent population oracle and application guidance are changed for this device. Internal UQ identifiers remain implementation names, while BD1 uniquely distinguishes the external protocol. Each folder is self-contained; no sibling imports, unified systems tree, new automation workflow or unrelated repository integration is required.

## Attack surface, authorization and trust model

Use only on equipment you own or have explicit written authorization to assess. Permission must cover enclosure access, diagnostic firmware changes, shared-ground attachment, harmless workload execution, potential service interruption, recovery procedures and evidence retention. General network testing authorization does not automatically authorize physical modifications. Start on a low-energy development fixture with consequential outputs disabled. Stop if attachment changes operation, voltage levels, grounding or temperature. Never use this prototype on mains-referenced wiring, deployed access controls, vehicles, medical equipment or machinery whose unexpected behavior could injure someone.

The investigated attack surface is an allocator's admission decision, including cached occupancy, cancellation, completion identity and bank mapping. Authorized red teams can ask the owner to expose an availability regression through a reduced diagnostic reproduction. Security researchers can compare scheduler revisions and adverse completion interleavings. Penetration testers can repeat an approved trace after remediation and attach the qualified evidence to their report. The device provides no exploit payload, target command channel, credential extraction, covert storage, radio transmission, persistence or network discovery.

The adapter, operator, MCU firmware and workstation are trusted. An adapter can omit, duplicate, reorder or fabricate operations. A malicious host can edit exports. Physical programming access can replace firmware. CRC detects accidental corruption, not authentication or replay. There is no secure boot, signed evidence, encrypted export, persistent boot identifier or trusted wall clock. A clean trace cannot establish that an adversarial target reported honestly. Keep adapter source, mapping and independent logs with the experiment, and minimize retained timing data because even payload-free traces can reveal operational patterns.

The supplied firmware configures target-facing pins as inputs and leaves USART reception disabled. The companion opens serial read-only. These are software safeguards, not an irreversible electrical diode: replacement firmware could drive pins. Loading, common ground, back-power and wiring errors can disturb equipment even without an intentional output. The passive carrier has no isolation or dedicated ESD protection. The authorized-use disclaimer does not substitute for electrical review.

## Hardware specifications and bill of materials

The external processor module is ST NUCLEO-F401RE with an STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware assumes reset-default nominal 16 MHz HSI operation. SysTick supplies nominal one-millisecond acquisition intervals. USART2 TX on PA2 uses 115200 baud, eight data bits, no parity and one stop bit through the purchased board's ST-LINK virtual COM bridge. The module supplies regulation, MCU decoupling, USB, reset and SWD programming. Those functions are intentionally external to the passive carrier, not missing parts of a purported standalone MCU board.

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm thickness, with 0.3 mm tracks. Fourteen through-hole components provide thirty pads and nine electrical nets. It connects using short labelled leads rather than stacking as a shield. No radio, antenna, FPGA, battery, charger, sensor, removable storage or target power output is present. Reserve a 5 V, 500 mA USB-source allowance for the module as planning headroom, not a measured power figure. Use nonconductive supports, strain relief and an underside cover. Mounting holes and production enclosure CAD are not supplied.

| Reference | Quantity | Candidate specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE; verify actual revision |
| R1, R3, R5, R7 | 4 | Yageo MFR-25FBF52-100K, 100 kOhm, 1%, axial |
| R2, R4, R6, R8 | 4 | Yageo MFR-25FBF52-1K, 1 kOhm, 1%, axial |
| J1, J3, J4, J5, J6 | 5 | Samtec TSW-102-07-G-S, 1x2, 2.54 mm |
| J2 | 1 | Samtec TSW-104-07-G-S, 1x4, 2.54 mm |
| Harness/support | 1 set | Short insulated leads and nonconductive base |

These are procurement candidates, not guaranteed stock, prices or mechanical fit. Check resistor body dimensions, lead forming to 7.62 mm, drill clearance, mating height and supplier drawings before ordering. Standard DIN0207 axial and vertical 2.54 mm header footprints are used.

| Signal | Target contact | MCU contact | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Every target connector's second pin is ground. Both J1 pins are ground, never supply inputs. J2 carries signals only, so connect a separate module ground lead to J1. Each incoming signal passes through 1 kOhm to its MCU node, which has a 100 kOhm pulldown. A disconnected input looks low; cable continuity is not proven. Support only verified common-ground push-pull zero-to-3.3 V logic. Exclude 5 V, 1.8 V, negative, differential and unknown-voltage sources. Series resistance is not surge protection or isolation. Disable target outputs before depowering the observer, and power the observer before enabling target outputs, to avoid back-powering through input protection structures.

## Architecture and block diagram

```text
Owned diagnostic adapter          Passive carrier        External Nucleo
TAG0 ------------------------------- 1k --+---------------- PA0
TAG1 ------------------------------- 1k --+---------------- PA1
ALLOCATE --------------------------- 1k --+---------------- PA4
RELEASE ---------------------------- 1k --+---------------- PB0
                                          | each MCU node
                                         100k
                                          |
Target ground -------------------------- GND ------------- module GND

GPIO -> continuity guard -> tag bundle / independent strobe qualification
     -> prior setup check -> least-populated-bank comparison
     -> transactional occupancy, counters and stamps -> bounded BD1 encoder
     -> USART2 TX -> ST-LINK USB -> Ledger / History / Integrity -> JSON
```

GPIOA and GPIOB are read separately, not atomically across ports. This is a deliberately slow diagnostic observer rather than a transparent production-speed bus analyzer. The adapter must report actual committed operations, preserve their order and expose any upstream overflow instead of silently dropping events. Each tag denotes one unique reservation, not a class containing multiple concurrent members. The two-bank mapping must remain fixed throughout the session. If slowing or instrumenting the target removes the failure, this instrument is unsuitable for that reproduction.

## Firmware and failure behavior

The C11 implementation uses no RTOS, heap, network stack or vendor SDK. Startup installs the Cortex-M4 vectors and initializes data and BSS. The linker describes target flash and SRAM. Board support configures inputs, reset-cause capture, SysTick, USART and independent watchdog. The main loop acquires on advancing ticks, services a bounded UART queue and schedules status every nominal 100 ms. Policy repeats every nominal second to support late attachment; repeated policy announcements do not erase session history.

The two tag bits qualify as a bundle. Allocation and release strobes qualify independently so unrelated chatter cannot indefinitely delay a real peer indication. A candidate followed by three matching intervals qualifies. A previously qualified tag must have five stable observations before a strobe is accepted. Raw and qualified tag must agree at acceptance; the opposite strobe must be low in previous, current and raw observations. Simultaneous rises are ambiguous, not assigned invented ordering. A held strobe contributes only one event.

Use twelve nominal milliseconds of stable tag with both strobes low, six with exactly one strobe high and six low afterward. Tests use that conservative pattern. These are software intervals rather than calibrated electrical limits. Short pulses can be filtered or missed. Quiet strobes cannot prove an empty pool: independently establish target emptiness before resetting observer history. A skipped acquisition tick terminates interpretation instead of fabricating continuous debounce stability. Repeated timestamps are ignored; ordinary unsigned timestamp wrap is supported.

Every observation works on a speculative state copy. Counter exhaustion discards the whole tentative transition before marking the previous coherent evidence terminal. Faults are spread-policy violation 1, orphan release 2, duplicate allocation 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog startup failure 64. Halt clears validity and freezes accepted accounting; timestamps and transport diagnostics may continue. Watchdog service continues after semantic halt so evidence is not intentionally erased. The retained active mask describes the last coherent observer state, not the target after a violating operation.

## Application and protocol

Run `python3 app/console.py` on Linux with Tk support. No third-party Python package is required. Enter an explicitly verified serial path and choose New live. There is no discovery sweep or automatic connection. Ledger displays occupancy, bank populations, eligible tags, cumulative uses, retained ordinals and findings. History holds up to 256 accepted snapshots. Integrity shows source, provenance, compatibility, parser errors, reset information, freeze reasons and freshness. These are working screens backed by actual parsing and transport, not mock dashboards.

BD1 records contain comma-separated decimal integers followed by an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT starts at 0xffff and covers bytes before the asterisk. B carries reset flags. P carries `[3,1,5,4,2,1]`: qualification intervals, nominal sample period, prior setup, pool size, adjacent slots per bank and policy revision. S field order is defined in `app/contract.py` and emitted by `firmware/wire.c`. Bounds failures produce no partial status record. BD1 is intentionally different from Bank Pack's BK1 and Bank Separation's BS1.

The companion checks unsigned bounds, masks, retained reason plausibility, unique nonzero ordinals and conservation. Admissions equal releases plus active depth and the sum of per-tag uses. Attempts equal admissions plus releases plus rejected edges. Per-tag return counts follow from use growth and membership changes. A continuously occupied identity blocks admission of its sibling if the other bank was empty and received no admissions throughout the interval, including a hidden sibling allocate/return cycle. Releasing the incumbent or admitting something into the other bank can legitimately remove that obligation. These are necessary sparse endpoint checks, not complete hidden-history reconstruction.

Live progress needs two advancing compatible records. Duplicate timestamps do not refresh freshness. Contradictory duplicates, rollback, later boot, impossible transitions, excessive reception pauses and capped host/device divergence freeze interpretation. Advancing terminal timestamps indicate transport, not continuing acquisition. Without persistent boot identity some resets remain indistinguishable from forward traffic. Uncalibrated oscillators can cause conservative rejection; do not weaken integrity guards merely to make a trace look clean.

Disconnect preserves evidence and restores terminal settings. A second connection is refused while a port is owned. Replay accepts regular files up to two MiB, rejects devices and FIFOs, and counts truncated tails. Framing resynchronizes on the complete BD1 marker. Export exclusively creates an owner-only JSON file and refuses overwrite. Provenance and integrity metadata remain present even when no operations completed. Nothing uploads automatically; exports are neither encrypted nor signed and require engagement-specific retention and deletion.

## Build, validation and bench workflow

From this directory run:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

ARM GNU GCC with newlib/nosys support builds the actual Cortex-M4 image. Host GCC runs sanitizer-backed policy, UART and main-loop tests. Tk and Xvfb support GUI execution without a physical display. KiCad 9, distro Python bindings and standard footprints validate CAD; regenerate with `/usr/bin/python3 tools/generate_hardware.py`. The checker compares fresh schematic netlist, saved netlist, board pads, values, footprint identifiers and manifest, in addition to ERC and DRC. Generated firmware binaries, caches and KiCad personal preferences are ignored.

Before fabrication, review rendered schematic, PCB, footprints and actual purchased module. Consult ST UM1724 for Nucleo connector and virtual-COM routing, RM0368 for registers, and the STM32F401xD/xE datasheet for electrical limits. Manufacturer manual entry: https://www.st.com/resource/en/user_manual/um1724-.pdf . Verify actual solder bridges and reset clock assumptions instead of inheriting unknown bootloader state. Inspect an unpowered carrier for shorts and connector orientation before programming through the module's supported SWD process.

Exercise either initial bank, tied choices, balanced filling, arbitrary return order, an unbalanced state created by returns, complete drain and repeated same-tag renewal. Compare a harmless deliberate spread violation against independent waveforms and target logs. Record mapping, adapter version, firmware revision, approved workload and source provenance with every export. Do not create a dangerous real-target condition to exercise an alarm. A clean trace establishes only that accepted diagnostic observations matched this declared admission contract, not physical isolation, sanitization, fairness, bounded latency or production readiness.
