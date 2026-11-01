# Use Quota — Nonrefundable Resource-Lifetime Witness

Author and creator: jayis1. Hardware revision A. Wire protocol LQ1.
Firmware, application and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. External tools and standard footprint libraries retain their existing licenses and authorship.

Status: unbuilt engineering prototype with target-linked firmware, an executable desktop companion and routed native KiCad sources. See VERIFICATION.md for executed software and CAD checks. No fabrication, MCU programming, measured power, calibrated timing or physical target interoperability is claimed. This is an observation instrument, not a quota enforcer, protective interlock, cryptographic authenticator or security certification.

## Purpose and overview

Use Quota detects a resource-accounting error that can remain invisible to ordinary concurrency checks: a target releases a reservation and accidentally restores a lifetime use allowance that should have remained spent. Four named resources have fixed experiment-wide admission quotas of one, two, three and four respectively. The observer reconstructs each resource's occupancy and cumulative accepted admissions. Release clears occupancy but never refunds use history. Emptying the entire pool, waiting, reconnecting the companion or using another resource never replenishes those quotas.

A practical laboratory example is a gateway with four diagnostic capability handles. Each handle permits a bounded number of accepted operations before its experimental authorization must be replaced. A faulty implementation stores the allowance in the reservation object and reconstructs it from its initial value whenever that object is freed. It can then accept operations indefinitely while never having two operations outstanding on the same handle. Use Quota distinguishes that bug from legitimate reuse of a handle that still has remaining tickets. The adapter reports actual admissions, not requests that the target rejected.

The experiment is deliberately small enough to exhaust every quota safely with a harmless software fixture. These numbers are not recommended limits for any production capability system, nor estimates of physical flash endurance or cryptographic strength. They make boundary tests short and auditable. An owner can implement dummy operations that touch no sensitive data, compare a corrected allocator with a deliberately defective emulator, and correlate externally observed admissions with source-level expectations. The instrument itself does not issue those operations or modify target memory.

For example, allocate zero, release zero, allocate zero is a violation on the last operation. Allocate one, release one, allocate one is permitted; a later release and third allocation of one is not. Allocating all four distinct resources at once is permitted because quotas constrain cumulative use, not aggregate concurrency. A resource may remain active after spending its final ticket and may still be released normally. The quota has no expiry timer. Releasing another tag cannot restore permission to the exhausted one.

## Originality and disclosed reuse

The jayis1/hacker-devices checkout was synchronized and its device directories and README titles inventoried before selection. Nearby purpose and contract sections were inspected, and a README search for quotas and lifetime limits did not reveal this measurement contract. Bank Separation restricts which resource pairs coexist. Weight Ceiling charges live weighted capacity and refunds it on release. Wear Balance chooses the least-used free candidate without an absolute lifetime cap. Replay Sieve forbids repeated command identities inside a signalled session; it neither provides unequal multi-use allowances nor reconstructs resource release. Retry Ledger counts authentication failures and lockout behavior rather than accepted per-resource lifetimes.

The new contribution is the nonrefundable unequal per-resource quota contract, independent remaining-ticket oracle, exact-boundary tests, matching telemetry validation and remaining-ticket user interface. The passive carrier, MCU startup, register definitions, UART service, CRC code, acquisition qualification and desktop transport deliberately reuse jayis1-authored Bank Separation infrastructure. Those generic circuits and drivers are not represented as new inventions. Originality here is relative to the reviewed repository, not a worldwide patent or prior-art claim. This folder is self-contained and has no runtime dependency on another device folder or shared hardware tree.

## Attack surface, authorization and threat model

Use only on equipment you own or have explicit written authorization to assess. Agree on diagnostic instrumentation, physical access, attachment points, permitted workload, interruptions and evidence retention before connecting anything. Begin with an emulator or low-energy development fixture with hazardous outputs disconnected. Do not connect this unisolated design to deployed access controls, medical equipment, moving vehicles, industrial safety equipment, mains-related circuits or unknown field wiring. Stop if attachment changes target behavior, temperature or signal levels.

The relevant attack surface is the integration between capability use accounting and resource lifecycle. Defects include off-by-one admission checks, refunding a spent lifetime allowance at release, clearing all histories when the pool empties, charging the wrong tag, using a shared pool instead of independent budgets, and renewing allowance after an unrelated operation. An authorized red team can demonstrate a reduced reproduction under written scope. Security researchers can compare implementations. Penetration testers can repeat the same diagnostic trace after remediation without collecting passwords, keys or command payloads.

The target diagnostic adapter, observer image, wiring, programming interface and workstation are trusted components. Each two-bit tag identifies exactly one resource, not a hash of multiple identities or a class containing simultaneous members. The mapping and quota table must remain fixed throughout the run. The adapter must preserve every accepted allocation and release in order; omitted or invented events invalidate conclusions. An adversarial target can lie on all four wires. The instrument cannot independently prove execution, authorization, identity, cleanup or the absence of hidden operations.

The observer must begin with an independently established empty pool and fresh experimental use history. Quiet strobes are not proof of either condition. In particular, resetting the observer while retaining target history destroys its accounting relationship; it does not authorize a target renewal. Reconnect merely starts new host provenance while a continuously running observer retains its counts. There is no external session reset signal or command to clear history. Document how a fresh experiment was established and keep old exports separate.

Receive-only describes the supplied target-facing firmware, not an immutable hardware diode. Replacement firmware could reconfigure MCU pins. There is no radio, network service, exploit payload, credential collection, covert storage, remote command parser or target write path. Serial CRC detects accidental corruption, not malicious forgery. There is no secure boot, authenticated pairing, signed update channel, encrypted evidence, trusted clock or persistent boot identity. Updates require physical ST-LINK access and a fresh experiment. Protect that access; do not treat a clean trace as certification of security.

## Hardware specifications and bill of materials

The processor is an external ST NUCLEO-F401RE module using STM32F401RET6, Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, a nominal one-millisecond SysTick and USART2 at 115200 baud, eight data bits, no parity, one stop bit. USART2 TX on PA2 reaches the ST-LINK virtual COM bridge. The purchased module supplies USB connectivity, programming, regulator, decoupling and reset circuitry. These functions are not falsely depicted as components on the passive carrier.

The custom PCB is 80 by 70 mm, two copper layers and nominal 1.6 mm thickness. Fourteen through-hole components provide thirty connected pads and nine nets. Copper tracks are 0.3 mm wide. The carrier attaches with short labelled leads rather than plugging directly into the development board. There are no mounting holes or enclosure model; use nonconductive supports, underside insulation and strain relief. There are no RF devices, antennas, batteries, chargers, analog sensors, storage sockets or target power outputs.

| Reference | Quantity | Specification |
| --- | --- | --- |
| External processor | 1 | ST NUCLEO-F401RE, purchased module revision verified before assembly |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness | 1 set | Short insulated leads, labels and mechanical supports |

Resistor footprints use DIN0207 bodies with 7.62 mm lead spacing. Confirm actual component body, lead diameter, drill fit, mating headers and orientation before procurement. No supplier availability, pricing or measured power claim is made. Provision a suitable USB source for the purchased module; 5 V at 500 mA is a conservative source allowance, not measured consumption. The passive carrier has no independent supply and does not power the target.

| Signal | Target connector | MCU connector | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3.1 | J2.1 | CN8.1 / A0 / PA0 | 0 |
| TAG1 | J4.1 | J2.2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | J2.3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | J2.4 | CN8.4 / A3 / PB0 | 3 |

Pin two on every target connector is ground. Both J1 contacts are ground only. J2 has four signals and no ground or supply contact: a separate Nucleo ground lead to J1 is mandatory. Each target `_IN` node passes through a 1 kOhm resistor to its MCU node; a 100 kOhm resistor pulls that MCU node to ground. Only common-ground push-pull zero-to-3.3 V diagnostic signals are supported. Do not connect 5 V, negative, differential, unknown or mains-referenced signals. The resistors are not an isolation barrier, hot-plug guarantee or overvoltage protector.

Power the observer before enabling target diagnostics, and disable diagnostics before depowering it to avoid back-powering. There is no dedicated ESD clamp or fuse on the carrier. Target and USB grounds become common. Check ground potential, input loading, logic thresholds and cable capacitance before attachment. The nominal static resistor path is 101 kOhm, excluding MCU leakage; it says nothing about dynamic signal integrity. Electrical qualification remains a separate required bench activity.

## Architecture and block diagram

```text
Owned target adapter               Passive carrier       NUCLEO-F401RE
TAG0 ------------------------------- 1k --+--------------- PA0
TAG1 ------------------------------- 1k --+--------------- PA1
ALLOCATE --------------------------- 1k --+--------------- PA4
RELEASE ---------------------------- 1k --+--------------- PB0
                                          | each independently
                                         100k
                                          |
Target GND -------------------------------+---- J1 ------- module GND

GPIO sample -> bundled tag / independent strobe qualification
 -> prior setup / raw endpoint validation -> occupancy and lifetime uses
 -> transactional quota check -> LQ1 encoder -> USART2 TX -> ST-LINK USB
 -> desktop Ledger / History / Integrity -> private local JSON export
```

The four target pins remain inputs, internal pulls are disabled and USART reception is disabled. GPIOA and GPIOB are read separately, not atomically latched. This is a slow diagnostic observer, not a bus analyzer or high-speed glitch detector. Its utility depends on deliberately stretching accepted-operation indications while preserving their order. If instrumentation hides the behavior under study, another measurement approach is required.

For each event, hold its tag with both strobes low for twelve nominal milliseconds, hold exactly one strobe high for six milliseconds, then hold both low for six milliseconds. ALLOCATE indicates an already accepted target admission; RELEASE indicates retirement of that same resource. Held strobes generate one event. Pulses shorter than qualification can be missed and must not be used. Simultaneous strobes are ambiguous rather than assigned an arbitrary order.

## Firmware design and failure behavior

Production firmware is C11, without an RTOS, heap allocation or downloaded SDK. Startup installs the Cortex-M vector table, initializes data and BSS and calls main. The linker places flash at 0x08000000 and SRAM at 0x20000000. The board driver captures reset flags, configures input pins, SysTick, USART and an independent watchdog. The serial driver maintains a bounded 768-byte transmit buffer, rejects concurrent submissions and services one byte per poll without waiting for an entire transmission.

The tag bits qualify together to avoid inventing intermediate identities; ALLOCATE and RELEASE qualify independently so chatter on one cannot conceal the other. A candidate needs three matching intervals. An event requires five prior stable tag observations, matching raw and qualified tag at its endpoint, a high raw selected strobe, and the opposite strobe low in prior qualified, current qualified and raw states. A clean both-low baseline starts tracking but does not prove an empty target history; that is an operator prerequisite.

Admission checks duplicate occupancy first. Otherwise it requires uses[tag] below the fixed quota, sets the occupancy bit and increments that tag's use and total admissions. Release requires that tag occupied, clears only its occupancy bit and increments completions; use history is retained. Concurrent distinct tags and arbitrary release order are permitted. Successful admissions occurring while another tag is active also increment the overlapping-admission counter. Emptying the pool increments the closed-episode counter. Those counters support consistency checking but grant no new tickets.

Every acquired sample is evaluated on a speculative ledger copy. Counter exhaustion discards that copy and halts the previous coherent state, avoiding partial commits. Repeated timestamps are ignored. A skipped sample halts correlation rather than granting stability for unseen time. Ordinary unsigned timer rollover is supported. Fault bits are quota exhaustion 1, orphan release 2, duplicate allocation 4, interface ambiguity 8, acquisition gap 16, counter exhaustion 32 and watchdog initialization failure 64. Interface validation precedes resource semantics; duplicate occupancy precedes quota exhaustion.

All interpretation faults are terminal. A halt clears qualification and freezes accepted ledger state while device time and saturating transport failure diagnostics may continue. The watchdog is fed after a policy halt so evidence remains available rather than being erased by a deliberate reset. Retained occupancy describes the last accepted observer state, not necessarily the target state after its violating admission. The app's remaining tickets and eligible list are explanatory evidence, never authorization to perform another operation.

## Application, protocol and evidence handling

Run `python3 app/console.py` on Linux with Tk support. The standard-library companion offers Ledger, History and Integrity screens. Enter a verified serial path, preferably `/dev/serial/by-id/`, then choose New live. No discovery scan is performed. The serial descriptor is opened read-only; disconnect restores terminal attributes, and a second open is refused while the first connection remains owned. Failed opens preserve retained evidence. Reconnect is explicit and begins a new host-side session.

Ledger shows active tags, per-tag cumulative admissions, fixed quotas, remaining tickets, currently eligible free tags and fault reasons. History retains up to 256 accepted snapshots. Integrity shows source, live/replay origin, connection state, reset information, parser errors, freshness and freeze reasons. Replay accepts only regular files of at most two MiB and records a truncated final frame. FIFO and device paths are rejected. Export creates a new mode-0600 JSON file, refuses overwrite and final-path symlinks, and includes metadata even without results. Every exported report credits jayis1.

Frames are ASCII `LQ1,type,decimal-fields*CCCC` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xffff over bytes preceding the asterisk. B contains reset flags. P contains `[3,1,5,4,1,2,3,4]`: qualification intervals, nominal sample period, tag setup, resource count and four quotas. Policy repeats every second so late attachment does not require a reset. S contains the fields enumerated by `app/contract.py`, including time, input state, occupancy, counters, four uses and transport failures. Status is scheduled every 100 milliseconds when the transport is available.

Both encoder and app validate quota bounds and conservation. Admissions equal completions plus active depth and the sum of all four use counters. Attempts equal admissions plus completions plus rejections. Closed episodes plus the nonempty-episode indicator plus overlapping admissions equal total admissions. Depth matches occupancy population; peak cannot exceed four. A balanced aggregate cannot legalize one tag exceeding its quota. Cumulative histories cannot decrease, migrate between identities or be replenished by a hidden cycle between snapshots.

Sparse records cannot reconstruct exact operation order. Per-tag release counts are inferred from admission growth and occupancy changes, and must fit global completions. Peak growth needs supporting admissions. These are necessary consistency checks, not a complete realizability proof. The parser bounds frames, resynchronizes on the full marker and rejects unsupported policy. Live freshness requires advancing device time, not mere packet receipt. Contradictory duplicate timestamps, counter rollback, changed halted evidence, extended reception pauses or diverging host/device progress freeze interpretation. A bounded drift allowance prevents unlimited stale-data accumulation. Some resets remain indistinguishable without a persistent boot identity.

## Build, validation and laboratory acceptance

From this directory run `make -C firmware all test`. Required tools are GNU Make, host GCC and Arm bare-metal GCC with newlib/nosys support. Target flags select Cortex-M4 Thumb soft-float, C11 and warnings as errors. Host engine tests use AddressSanitizer and UndefinedBehaviorSanitizer. The independent oracle consumes remaining tickets rather than reproducing the firmware's use-count representation; it explores repeated allocation/release words and tests every exact quota boundary, all resource pairs, full occupancy, release order, duplicate precedence, gaps and transactional exhaustion.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v` for headless desktop tests. These exercise real C-encoder output, the Tk event loop through serial pseudo-terminals, all screens, disconnect/reconnect, hangup, replay, failed connection retention, export and overwrite refusal. Fixtures are explicitly synthetic diagnostic observations, not fabricated hardware measurements. App policy tests enumerate use-count combinations and occupancy masks and reject refund or migration contradictions.

With KiCad 9 Python bindings and standard footprints installed, run `/usr/bin/python3 tools/generate_hardware.py` followed by `/usr/bin/python3 tools/check_hardware.py`. The generator emits actual symbols, pins, wires, labels, footprints, routed tracks, outline, project settings and netlist. The checker runs ERC/DRC and compares every component value, footprint identity and pin/net mapping between schematic, PCB, saved netlist and manifest. Also inspect a rendered schematic; zero CAD violations do not qualify external wiring, the purchased module, insulation or deployment safety. Run `python3 tools/check_release.py` for attribution and content gates.

Before assembly review ST UM1724, particularly Table 16 and section 7.10, the STM32F401xD/xE datasheet and RM0368 against the purchased board revision. Manufacturer-indexed UM1724 content corroborates CN8.1 through CN8.4 as PA0, PA1, PA4 and PB0 and the USART2/ST-LINK default solder-bridge path. The source is https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . Indexed excerpts do not establish the configuration of your physical module; check power jumpers and solder bridges directly.

Inspect resistance values, shorts, ground continuity and connector orientation unpowered. Power only the observer, verify its image identity and LQ1 idle telemetry, then attach a known low-energy diagnostic fixture with outputs disabled. Establish empty occupancy and unused quotas explicitly. Exercise each inclusive limit, a normal final release, a one-over-limit emulator admission and independent budgets while another resource remains active. Correlate waveforms with results and measure voltage loading and timing before expanding scope. Do not exceed a real protected system's allowance merely to trigger the instrument.

A useful report identifies tag mapping, target firmware, observer image, adapter source, baseline procedure and first contradiction. Keep passing and failing synthetic fixtures labelled. Minimize exports and store them under engagement policy; JSON is neither encrypted nor signed. Ordinary file deletion does not guarantee secure erasure on flash media. Outstanding physical work includes fabrication, programming, reset/watchdog validation, electrical loading, timing calibration, ESD resilience, enclosure review and actual target interoperability. This design remains for supervised authorized laboratory evaluation only.
