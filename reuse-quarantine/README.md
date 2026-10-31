# Reuse Quarantine — Independent Resource Reuse Deadline Witness

Author and creator: jayis1. Hardware revision A. Wire protocol UQ1.
Firmware, application, and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. These license identifiers apply to the original material in the corresponding directories; installed compiler and KiCad libraries retain their upstream licenses.

Status: unbuilt engineering prototype design. The repository provides target-buildable embedded firmware, a functioning Linux desktop application, and routed native KiCad sources. Executed checks and their results are recorded separately in VERIFICATION.md. Fabrication, MCU programming, electrical bring-up, physical timing characterization, and deployment are not claimed. This device observes a diagnostic contract; it does not enforce quarantine or prove that a resource is safe to reuse.

## Purpose and practical utility

Reuse Quarantine checks whether two independently recycled resources remain unavailable for a required interval after return. Each accepted return starts a timer belonging to that exact resource. An allocation before its timer expires is a terminal finding. The other resource can remain occupied, return independently, or be reused after its own deadline. Returning the entire pool does not clear either deadline, and a short-class return cannot shorten a long-class quarantine on the other slot.

The motivating case is a small embedded service with two recyclable transaction descriptors. A delayed peripheral response, asynchronous callback, or external consumer may still refer to an old descriptor after the allocator considers it free. The product's mitigation might require a bounded quarantine before reuse. A cancellation path that skips this delay can reassign the descriptor while stale work still exists. Aggregate free counts cannot show whether the correct descriptor waited; this instrument observes resource identity and elapsed qualified return-to-allocation time externally.

The target owner must supply deliberately slow diagnostic indications representing actual accepted allocations and completed returns. The observer is not connected to a high-speed memory bus and does not infer pointer identity. Its fixed two-slot vocabulary is suitable for a reduced reproduction, a small hardware mailbox pool, or a test adapter that reserves exactly two identifiable resources. A target with hundreds of resources cannot safely truncate identifiers into one bit. Such a target needs a different reviewed interface or an explicitly isolated two-resource experiment.

The LONG modifier chooses one of two documented test policies: 250 nominal milliseconds or 1000 nominal milliseconds after a return. These are laboratory contract values, not universal security recommendations. Their purpose is to test class-specific timing and independent timer ownership at a rate this instrument can observe. A real product's safe quarantine depends on its actual asynchronous consumers and maximum stale-reference lifetime. Waiting a second is not automatically sufficient, and quarantine is not a substitute for correct ownership and generation checks.

## Repository-local originality

The repository was pulled and its device directories inventoried before this concept was selected. Reset Quarantine measures minimum readiness settling after reset, including readiness withdrawal and privilege access. Lease Watch measures heartbeat freshness while an output remains enabled. Scrub Credit requires a distinct cleanup acknowledgment before every allocation and consumes that acknowledgment; elapsed time cannot generate its credit. Slot Affinity retains the first security domain assigned to each resource. Reuse Quarantine instead starts independent identity-bound time obligations on resource return and permits reuse only after expiration, with no cleanup acknowledgment or domain identity.

This is a distinct observable contract within the reviewed repository, not a claim of worldwide invention, patentability, or discovery of quarantine as an engineering technique. The electronics and platform facilities deliberately reuse jayis1-authored passive-carrier infrastructure from Scrub Credit. Startup code, register definitions, scheduling, bounded UART transmission, CRC, desktop serial handling, and CAD routing are reused rather than described as novel. The timer engine changes, wire policy, host temporal checks, boundary tests, and interpretation are specific to this device. The folder is self-contained: builds do not import code from sibling devices.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Obtain permission for diagnostic firmware changes, internal wiring, workload generation, permitted interruptions, evidence retention, and recovery procedures. A network assessment authorization is not necessarily electrical attachment authorization. Begin on an isolated low-energy development bench, with hazardous outputs disconnected. Do not connect this unqualified carrier to deployed access controls, vehicles, medical equipment, industrial safety equipment, mains-referenced electronics, or unknown field wiring.

The assessed attack surface is resource lifecycle integration: cancellation callbacks, timeout recovery, descriptor free lists, class selection, asynchronous completion, and reuse admission. The design provides no exploit delivery, target command interface, packet injection, credential capture, radio transmission, persistence, or automatic discovery. An authorized red team can observe an owner-approved workload that stresses cancellation or delayed completion. Security researchers can compare two allocator revisions. Penetration testers can retain externally observed timing contradictions for an agreed diagnostic interface without extracting sensitive payloads.

The trusted computing base includes the operator, target adapter, harness, processor board, firmware image, and workstation. A return notification must mean the point specified by the product's quarantine contract, not merely that a return was requested. An allocation strobe must mean a new allocation was actually accepted, not an attempt correctly refused by the allocator. LONG must report the agreed resource policy. Instrumentation that silently substitutes a shorter class defeats the intended interpretation even if the observed trace is internally consistent.

The instrument cannot determine whether stale DMA remains active, whether a callback still holds a pointer, whether memory was sanitized, or whether a privilege boundary was crossed. It cannot prove that target diagnostic signals are truthful. A compromised target can emit a plausible sequence while misbehaving internally. Independent waveforms, source review, functional tests, and target logs remain necessary. A terminal finding is evidence of a contradicted diagnostic contract, not by itself a complete vulnerability proof.

The supplied firmware configures every target-facing line as an input, but the carrier is not a hardware data diode. Reprogrammed GPIO can drive through the series resistors. There is no secure boot, signed update protocol, protected key store, persistent boot identity, authenticated telemetry, encrypted export, or trusted time source. Physical SWD access permits replacing firmware. CRC detects accidental corruption rather than forgery. Secure the workstation and debug interface, and treat exported evidence as corroborative rather than tamper-proof.

## Hardware specifications and bill of materials

The external processor module is ST NUCLEO-F401RE with an STM32F401RET6 Cortex-M4 MCU, 512 KiB flash and 96 KiB SRAM. The supplied firmware uses the reset-default nominal 16 MHz HSI clock, a nominal one-millisecond SysTick, and USART2 transmit at 115200 baud, eight data bits, no parity, one stop bit. PA2 connects toward the board's ST-LINK virtual serial bridge. USART receive is disabled. There is no FPGA, RTOS, downloaded SDK, wireless radio, antenna, analog sensor, battery, charger, SD card, or target power output.

The custom PCB is an 80 by 70 mm, two-layer, nominal 1.6 mm passive carrier. It is cable-connected, not a mechanically stacking Nucleo shield. Fourteen through-hole components provide four resistor-conditioned diagnostic inputs, a signal harness, and ground connections. Power regulation, decoupling, debug, USB, reset, boot configuration, and processor circuitry reside on the external development board. The schematic does not pretend those module components are on the passive carrier. Use an insulating support with a covered underside and strain relief; an enclosure and mounting-hole pattern are not supplied.

| Reference | Quantity | Requirement |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE; inspect purchased revision |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 header, 2.54 mm pitch |
| Harness/support | 1 set | Short labeled insulated leads and nonconductive mounting |

Resistor footprints are DIN0207 axial with 7.62 mm lead spacing. Example procurement choices are Yageo MFR-25FBF52-100K and MFR-25FBF52-1K resistors, and Samtec TSW-102-07-G-S / TSW-104-07-G-S headers; verify current manufacturer drawings, lead dimensions, and availability before purchase. These are suggested parts, not verified stock or assembly evidence. KiCad footprints remain the actual dimensional interface to check.

| Signal | Target contact | J2 contact | Nucleo connection | Firmware bit |
| --- | --- | --- | --- | --- |
| SLOT | J3.1 | 1 | CN8.1 / A0 / PA0 | 0 |
| LONG | J4.1 | 2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | 3 | CN8.3 / A2 / PA4 | 2 |
| RETURN | J6.1 | 4 | CN8.4 / A3 / PB0 | 3 |

Pin two of every target connector is ground. Both contacts of J1 are ground only, never power inputs. J2 contains four signals and no ground or supply contact; provide the separate Nucleo ground connection to J1. Nets ending in _IN are target-side nodes before the series resistors. Verify connector orientation and continuity with a meter rather than guessing from a photograph.

Only known common-ground zero-to-3.3 V push-pull diagnostic outputs are supported. Each channel has a 1 kOhm series resistor and a 100 kOhm MCU-side pulldown. There is no galvanic isolation, qualified overvoltage clamp, dedicated ESD protector, fuse, reverse-current barrier, or level translator. Five-volt outputs, 1.8 V logic, negative voltages, differential buses, relay coils, and unidentified signals are unsupported. A disconnected cable tends low but is not separately detected. A series resistor does not make unknown equipment safe to attach.

Power the Nucleo through its intended ST-LINK USB connector using the manufacturer-specified configuration. Reserve a 5 V, 500 mA USB source as a planning allowance, not as a measured demand or compliance result. The carrier itself has no supply rail. Power the observer before enabling target outputs, check ground offset, connect ground before signals, and remove or disable signals before removing observer power. Target and USB grounds are common. Back-power through input protection remains possible when this sequence is violated. Actual current, thresholds, loading, heat, and hot-plug behavior require bench measurements.

Primary references are ST UM1724 for NUCLEO-F401RE connectors and solder bridges, RM0368 for STM32F401 registers, and the STM32F401xD/xE datasheet for electrical limits. UM1724 Table 16 maps CN8 A0–A3 to PA0, PA1, PA4, and PB0. Its virtual serial section describes SB13/SB14 and SB62/SB63 routing; inspect the actual revision before changing bridges. The manufacturer manual is https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned diagnostic adapter          Passive carrier              External Nucleo
SLOT ----------------------------- 1k ---+--------------------- PA0
LONG ----------------------------- 1k ---+--------------------- PA1
ALLOCATE ------------------------- 1k ---+--------------------- PA4
RETURN --------------------------- 1k ---+--------------------- PB0
                                         | separate per signal
                                        100k
                                         |
Target ground -------------------------- GND ------------------ USB ground

GPIO -> continuity guard -> bundled SLOT/LONG and separate strobe qualifiers
     -> prior-data setup guard -> independent occupied/quarantined/free slots
     -> per-slot countdowns -> transactional counters -> UQ1 CRC records
     -> bounded UART TX -> ST-LINK USB VCP -> Ledger / History / Integrity
```

GPIOA and GPIOB are read sequentially, not captured by a cross-port latch. This is deliberately slow diagnostic instrumentation, not an instruction-speed logic analyzer. Begin with twenty milliseconds of stable data, twenty milliseconds strobe high, and twenty milliseconds both strobes low for every operation. Verify these margins independently before trying tighter timing. Sub-millisecond pulse visibility and electrical edge ordering are outside the claimed measurement contract.

## Firmware policy and timing semantics

Before starting, independently establish an empty target pool with any earlier quarantine obligations already expired. Startup assumes both resources available. It does not discover preexisting allocations or infer a target baseline from quiet wires. Resetting only the observer during a live experiment loses volatile history and is unsupported. WAIT becomes TRACK after all inputs qualify and both raw and qualified strobes are low. This establishes an electrical baseline, not proof of target vacancy.

SLOT selects resource zero or one. ALLOCATE rising with LONG low requests observation of an accepted allocation. RETURN rising starts the selected slot's quarantine, using 250 ticks when LONG is low and 1000 ticks when LONG is high. LONG is bundled data, not an event strobe. ALLOCATE with LONG high is reserved and rejected. Both strobes rising together are rejected as two ambiguous attempted operations. A held strobe is one event, not repeated work. Both strobes must return low between operations.

SLOT and LONG qualify together so intermediate data-bit changes do not synthesize an operation. The two strobes qualify independently. A changed candidate needs three subsequent matching acquisition intervals to qualify. Five prior stable data observations are required at the operation endpoint. Raw endpoint data must agree with qualified data, and the opposite strobe must be low in previous qualified, current qualified, and raw states. The current endpoint cannot retroactively satisfy its own setup requirement. Short rejected pulses and unsampled glitches remain outside guaranteed visibility.

Each slot has three states: available, occupied, and cooling. Allocation of an available slot marks it occupied; allocation of an occupied slot is a duplicate; allocation of a cooling slot is early reuse. Return requires occupation, clears occupation, and initializes that slot's countdown. Return of any unoccupied slot is an orphan, including a slot still cooling. Repeated returns cannot restart or extend quarantine. Later LONG changes do not alter an existing timer. A new return only follows a new valid allocation.

Timers advance only on consecutive acquired ticks. The return endpoint sets the full countdown without receiving time from the preceding interval. On each later continuous tick, the previous timer decreases before the new operation is evaluated. Thus an allocation qualifying exactly 250 or 1000 ticks after its return is allowed; one tick earlier is rejected. This explicit endpoint rule concerns qualified observations, not actual raw edge separation. Qualification latency and oscillator error must be included when comparing the result to a physical timing requirement. The design is unsuitable for certifying a minimum electrical delay at the same nominal threshold without independent calibration and margin.

Timers for both resources may expire on one tick, increasing expired by two. Expiration is not an input operation and does not increase attempts. Quarantine proceeds despite unrelated data chatter because the timer is tied to acquired time, not to stable diagnostic data. All updates, including expirations and diagnostics, are speculative until the sample completes without counter exhaustion. If an increment would overflow, the entire sample is discarded and a terminal exhaustion fault retains the prior state. No partially expired resource is published.

A missing acquisition tick halts permanently before any time from that gap can be credited. Repeated identical timestamps are ignored, and ordinary unsigned timestamp wrap works. After a halt, counters, input endpoint, timers, and resource state freeze; only device time and the saturating transmit-failure diagnostic continue. Recovery requires exporting evidence, restoring the target baseline, and starting a new observer campaign. The host cannot clear a finding or command the device to resume.

Conservation equations are launched = completed + occupied population; completed = expired + cooling population; attempts = launched + completed + rejected. Occupied and cooling masks are disjoint, and remaining time is nonzero exactly for cooling slots. Rounds count nonempty occupancy episodes ending with an empty pool, not quarantine completion. Peak occupancy cannot exceed two. The embedded encoder checks these invariants before emitting status, and the companion checks them independently.

## Companion application and interface

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk. No third-party Python package is needed. Enter the exact serial device path, then select New live. The application opens read-only 115200 8N1 transport, refuses double connection, restores terminal attributes where possible, and detects hangup. Ledger shows slot masks, remaining timers, counts, and findings. History retains up to 256 snapshots. Integrity shows origin, policy compatibility, reset evidence, parser errors, source path, freshness, and freeze reason.

Records are ASCII `UQ1,<kind>,<decimal fields>*<four uppercase CRC digits>` followed by LF. CRC-16/CCITT-FALSE covers all bytes before the asterisk, with polynomial 0x1021 and initial 0xffff, no reflection or final xor. B contains raw reset flags. P contains `3,1,5,2,250,1000`: qualification intervals, nominal sample milliseconds, setup observations, slot capacity, short duration, and long duration. P repeats approximately once a second and never clears evidence. Late attachment therefore works without the original boot record.

S fields are `now levels valid halted faults phase active cooling stable observed rounds attempts launched completed rejected gaps missed expired peak remaining0 remaining1 tx_fail`. Every value is unsigned 32-bit decimal. Fault bits are 1 early reuse, 2 orphan return, 4 duplicate allocation, 8 interface ambiguity, 16 acquisition gap, 32 exhaustion, and 64 watchdog startup failure. Exactly one terminal reason is retained. Status is attempted every hundred nominal milliseconds. The UART transport abandons an unfinished frame after a hundred milliseconds instead of blocking acquisition.

The companion validates masks, conservation, monotonic counters, immutable halted evidence, cyclic per-slot reachability, and timer bounds across snapshots. A timer whose deadline lies beyond the next snapshot cannot disappear or migrate to the other slot. A continuing nonhalted timer must decrease by elapsed device time. New timers must be compatible with one of the two fixed durations and the observed interval. These are necessary structural bounds, not an exhaustive reconstruction of hidden operations or all possible schedules. CRC-valid hostile data can satisfy these checks without originating from hardware.

Two advancing device timestamps are required before live progress is displayed. Duplicate timestamps do not renew freshness. Long reception pauses and excessive capped host/device drift freeze interpretation instead of treating queued records as current. Later boot records, counter rollback, and contradictory duplicates also freeze evidence. Lost boot records with otherwise plausible forward values can hide some resets because persistent boot identity is absent. LIVE describes transport progress; it never means that the target is secure or fault-free.

Replay accepts regular files no larger than two MiB, labels them non-live, and counts incomplete tails as errors. Export JSON creates a new mode-0600 file, refuses overwrite and final-component symlinks, and preserves jayis1 attribution, provenance, retained history, reset metadata, errors, and freshness. Use a trusted parent directory. Even activity timing may be sensitive; agree on retention and deletion with the owner. Ordinary filesystem deletion is not secure erasure.

## Reproducible checks and laboratory acceptance

Run `make -C firmware all test` for the Cortex-M4 Thumb soft-float target build and host tests. GNU Arm Embedded GCC and newlib/nosys link the actual ELF and binary. Build products are ignored. Tests cover both slots and duration classes, threshold-minus-one/exact/plus-one acceptance, independent countdown interleaving, a separate absolute-deadline oracle, orphan and duplicate handling, reserved encodings, simultaneous strobes, timer wrap, acquisition loss, counter exhaustion, serializer bounds, UART timeout, lost boot transmission, and watchdog startup failure.

Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` for contract tests, real C-encoder fixtures, checksum-valid impossible snapshots, parser recovery, bounded replay, private export, freshness, backlog, and the actual Tk event loop with Linux pseudoterminals. Run `python3 -m compileall -q app` for syntax compilation. Synthetic fixture records are explicitly not physical measurements. The GUI test receives data, switches every tab, detects disconnect, loads replay, and invokes export.

Run `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9 and standard footprint libraries to regenerate CAD. Run `/usr/bin/python3 tools/check_hardware.py` for native ERC, DRC, fresh XML netlist export, and exact schematic/PCB/manifest reference, value, footprint, pad, and net agreement. Run `python3 tools/check_release.py` for README length, substantive production C count, attribution, and diff whitespace. Passive ERC cannot validate the external powered module. Zero CAD violations are not electrical safety or fabrication qualification.

Before practical use, inspect assembled resistor values, continuity, connector orientation, ground offset, power sequencing, input loading, and logic thresholds. Program the target image and confirm measured sample and UART timing. Establish the empty, expired target baseline. Exercise each resource and class separately, then overlap long quarantine on one slot with short reuse on the other. Correlate early/exact/late cases with independent waveforms. Test watchdog recovery and cable disconnection without hazardous loads. Physical assembly, programming, timing calibration, ESD behavior, actual delayed-consumer lifetime, and sustained USB operation remain explicit laboratory acceptance gates.
