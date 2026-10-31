# Slot Generation — Reused-Resource Generation Witness

Author and creator: jayis1. Revision A. Wire protocol SG1.
Firmware, companion application, and tests: GPL-2.0-only. Hardware tools: MIT. Passive carrier: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked C firmware, an executable Linux desktop companion, and native routed KiCad files. Executed checks appear in VERIFICATION.md. No fabrication, physical programming, electrical qualification, calibrated timing, or production deployment is claimed. This instrument observes a diagnostic contract; it does not enforce memory safety, authenticate a resource owner, or provide a safety interlock.

## Purpose and practical value

Slot Generation investigates stale resource releases after reuse. A target-owned diagnostic adapter exports a one-bit slot identity, one-bit generation, ALLOCATE strobe, and RELEASE strobe. There are two independently reusable slots. The first allocation of each slot may use either generation. Every subsequent allocation of that slot must toggle its generation. A release must identify both an occupied slot and the generation belonging to its current owner. The observer retains generation history even when neither slot is occupied.

The motivating case is a delayed completion callback that refers to a resource already released and reassigned. A simple occupancy monitor may accept that callback because the slot is occupied again. Slot Generation distinguishes the immediately preceding owner from the current owner. For example, allocate slot zero with generation zero, release zero/zero, allocate zero/one, then attempt release zero/zero. The final release is stale: it must not retire the currently active zero/one ownership. The device freezes its evidence with slot zero still occupied at generation one.

A second defect is reallocation without updating the generation. Even if no delayed callback happens during a particular test, accepting the same generation on successive allocations contradicts the diagnostic contract. This device reports that failure at allocation rather than waiting for a later stale release. Either slot can be released first, either initial generation is legal, and one slot can cycle while the other remains occupied. There is no global alternating-slot requirement, fixed numerical ordering, FIFO rule, or LIFO rule.

This is deliberately a small, auditable instrument rather than a universal temporal-memory-safety analyzer. One generation bit cannot distinguish owners separated by two reuses. After generation zero, one, zero, a very old zero-generation callback aliases the current generation. That blind spot is fundamental to the interface, not something CRC or additional software checking can repair. Limit experiments to the immediately preceding generation, or replace the interface with a wider generation protocol before studying arbitrarily delayed completions.

## Originality relative to the repository

The configured default branch was pulled and existing device directories and README purpose statements were inspected before this concept was selected. Slot Alias tracks four resource identities but has no per-slot generation, so it cannot distinguish an old owner from a new owner of the same occupied resource. Epoch Witness checks globally ordered firmware generations; it does not permit independent alternating generation bits for reusable resource slots. Queue Overtake learns admission order and requires head departure; this design explicitly permits either active slot to release next.

The new contribution is the per-slot retained-generation contract, stale-release finding, non-toggling-reuse finding, occupancy/generation evidence representation, and independent array-oracle tests. It is not a claim of worldwide invention, patentability, or a new theory of ABA avoidance. The passive carrier, STM32 register support, bounded serial scheduler, CRC writer, and Linux transport intentionally reuse jayis1-authored Queue Overtake platform code. This directory is self-contained: builds and application execution do not import sibling device directories. External component geometry and manufacturer manuals retain their provenance.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Permission must cover internal test points, diagnostic firmware changes, the approved workload, potential interruption, grounding, and evidence retention. A network penetration-test agreement does not automatically permit opening or instrumenting hardware. Begin with a low-energy laboratory fixture, disconnected hazardous outputs, visible labelled leads, and an owner-reviewed recovery procedure. Do not connect this unisolated prototype to deployed access controls, vehicles, medical equipment, industrial actuators, mains-related circuitry, or unknown wiring.

The assessed attack surface is the resource-lifetime integration boundary: allocation, retirement, asynchronous callback dispatch, and generation bookkeeping. A red team working with the owner can request controlled delayed-completion tests on an isolated development build. A penetration tester can compare diagnostic evidence before and after a fix. A security researcher can study whether cleanup paths and normal completion paths obey the same generation contract. The instrument does not create exploit traffic, trigger callbacks, corrupt memory, extract credentials, or autonomously control a target.

The target adapter must emit events at the agreed semantic boundaries. ALLOCATE means a specific resource has actually been assigned to the indicated owner generation, not simply requested. RELEASE means the target is accepting the release at the lifetime boundary under investigation, not merely seeing an incoming message that it will correctly reject. If the adapter reports rejected requests as accepted releases, the instrument will report a contradiction in the adapter's declared contract rather than prove an allocator vulnerability. Review adapter source and correlate results with independent functional measurements.

The operator, harness, instrument firmware, Nucleo board, programming access, and workstation are trusted. A malicious target can lie on every diagnostic line. CRC detects accidental corruption but is not authentication. Serial records can be forged, replayed, or edited. There is no secure boot, authenticated updater, persistent boot identity, trusted wall clock, signed evidence, encrypted export, secure element, radio, or network service. Programming access can replace the supplied receive-only firmware with an output-driving image; this is not a certified hardware data diode.

Physical attachment can disturb a target through loading, shorts, common-ground currents, or back-power. A disconnected input may look low through its pulldown. There is no cable-presence detector or tamper evidence. Stop if connection changes target behavior. Export only agreed evidence, restrict file access, and follow the owner's retention policy. Ordinary deletion is not secure erasure. Absence of a finding is not proof of uninterrupted observation, complete coverage, or secure resource management.

## Hardware specifications and assembly boundary

The compute module is an external ST NUCLEO-F401RE, MB1136 family, containing an STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock without a PLL. SysTick supplies nominal one-millisecond acquisition intervals. USART2 transmits on PA2 at 115200 baud, eight data bits, no parity, and one stop bit through the board's ST-LINK virtual COM bridge. The module supplies regulation, decoupling, reset, boot configuration, SWD, and USB connectivity; these are not claimed to exist on the passive carrier.

The carrier is a cable-connected interface, not a stacking shield. Its outline is 80 by 70 mm with two copper layers, nominal 1.6 mm thickness, and 0.3 mm tracks. The design contains fourteen through-hole components, thirty connected electrical pads, and nine named nets. There are no mounting holes or enclosure files. Use insulating supports, a protective cover, and strain relief. No FPGA, battery, charger, antenna, analog sensor, level shifter, isolated supply, or target power output is present.

| Item | Quantity | Specification |
| --- | --- | --- |
| External MCU module | 1 | ST NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm through-hole header |
| J2 | 1 | 1x4 vertical 2.54 mm through-hole header |
| Mechanical support | 1 set | Insulated base, labelled short harness, protective cover |

Resistor footprints use DIN0207 geometry with 7.62 mm lead spacing. Header footprints are standard single-row vertical KiCad footprints. Check the dimensions and ratings of actual purchased components before assembly. No stock availability, BOM price, production yield, connector fit, or environmental qualification is guaranteed. The input range is known common-ground zero-to-3.3 V push-pull logic only. Five-volt, 1.8 V, negative, differential, relay, industrial, and unidentified signals are unsupported.

Each target signal passes through 1 kOhm to the MCU-side node, with a 100 kOhm pulldown from that node to ground. The nominal source-to-ground resistance is 101 kOhm before leakage and source impedance. The series resistor is not a qualified overvoltage protector. No dedicated ESD clamp, galvanic isolation, fuse, reverse-current blocker, or hot-plug guarantee is provided. Validate actual loading and logic thresholds with the assembled fixture.

| Function | Target header pin 1 | J2 pin | Nucleo connection |
| --- | --- | --- | --- |
| SLOT | J3 | 1 | CN8 pin 1, A0, PA0 |
| GENERATION | J4 | 2 | CN8 pin 2, A1, PA1 |
| ALLOCATE | J5 | 3 | CN8 pin 3, A2, PA4 |
| RELEASE | J6 | 4 | CN8 pin 4, A3, PB0 |

Pin two of each target header is ground. Both J1 pins are ground only; neither accepts supply voltage. J2 has four signals and no ground or power pin. Connect a separate Nucleo ground lead to J1. Nets ending `_IN` are target-side signals; unsuffixed nets are MCU-side signals. Confirm pin-one orientation by continuity rather than by wire color.

Power the Nucleo through its documented ST-LINK USB connection after checking board revision, jumpers, and solder bridges. A conservative source-planning allowance is 5 V at 500 mA; actual consumption is unmeasured and no USB certification is implied. USB ground joins target ground. Power the observer before enabling target diagnostic outputs, and remove those outputs before removing observer power to avoid back-power through MCU protection structures.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136 connector and ST-LINK routing, STM32F401 reference manual RM0368 for peripheral registers, and the STM32F401RE datasheet for electrical ratings. The inherited platform map is shown above; fresh manufacturer PDF retrieval failed during this run, so verify it against the actual purchased board manual before wiring. Leave BOOT0 in normal flash-boot configuration and confirm PA2 reaches the virtual COM bridge. This documented bring-up gate is not replaced by successful software or CAD checks.

## Architecture and timing

```text
Owner-controlled diagnostic adapter        Passive carrier       NUCLEO-F401RE
SLOT ------------------------------------ 1k --+---------------- PA0
GENERATION ------------------------------ 1k --+---------------- PA1
ALLOCATE -------------------------------- 1k --+---------------- PA4
RELEASE --------------------------------- 1k --+---------------- PB0
                                               | each input
                                              100k
                                               |
Target ground -------------------------------- GND ------------- USB ground

GPIO -> continuity guard -> bundled identity qualification / independent strobes
     -> prior setup guard -> retained per-slot generation and occupancy engine
     -> transactional counters -> SG1 CRC frames -> USART2 TX -> ST-LINK USB
     -> Linux Ledger, History, Integrity screens -> private JSON evidence export
```

GPIOA and GPIOB are read separately, so the four-bit observation is not atomic across ports. This is not a bus-speed logic analyzer. Use deliberately slowed diagnostic events: start with twenty milliseconds of identity setup, twenty milliseconds of strobe high, and twenty milliseconds with both strobes low after each operation. Independently inspect waveforms before shortening these intervals. Qualification cannot establish cable delay, oscillator tolerance, electrical edge quality, interrupt latency, or exact physical simultaneity.

## Firmware design and exact policy

`engine.c` implements the hardware-independent contract. `drivers/board.c` configures receive-only target GPIO, SysTick, transmit-only USART2, reset-cause capture, and the independent watchdog. `drivers/serial.c` bounds nonblocking transmission. `wire.c` serializes validated evidence without heap allocation. `startup.c`, `registers.h`, `board.h`, and `linker.ld` define the target reset, peripheral, pin, and memory interfaces. There is no RTOS or downloaded SDK requirement.

SLOT and GENERATION qualify as one two-bit bundle; ALLOCATE and RELEASE qualify independently. A changed candidate needs three subsequent matching sample intervals to qualify. Five prior eligible identity-setup observations are required before an operation endpoint. Setup cannot be earned on that endpoint itself. Raw/qualified identity disagreement, simultaneous operation edges, or an opposite strobe high in raw or qualified evidence is terminal interface ambiguity. A pulse shorter than qualification may remain invisible; this is not an exhaustive glitch detector.

Before starting the observer, independently establish that both target slots are free. The observer waits for fully qualified quiet strobes before tracking. Quiet levels alone do not prove that resources are free. Attaching the observer halfway through unknown ownership is not a valid baseline. Attaching the desktop to an already-running observer is supported because the observer retained prior ownership history and repeats its compatibility policy.

Allocation checks occupancy first. An already live slot produces duplicate-live-slot fault even if a different generation is presented. Otherwise a previously seen slot must toggle its retained generation; a never-seen slot accepts either value. Release checks occupancy first, then requires the current generation. An absent slot is an orphan release; an occupied slot with the wrong generation is a generation finding. A valid release clears occupancy but retains last generation and seen history. Either release order is legal.

Every sample is evaluated on a speculative copy. Counter exhaustion discards the whole sample and preserves the earlier resource state rather than committing half an allocation. A skipped acquisition tick halts correlation permanently. Duplicate calls at one timestamp are ignored; adjacent unsigned timestamp wrap is supported. After halt, only device time and the saturating transport-failure counter may advance. Restart requires a new independently established target baseline and a new observer run. There is no host clear command or automatic recovery from quiet inputs.

The evidence conserves allocations = releases + occupied-slot count; operation attempts = allocations + releases + rejected edges; and allocations = completed nonempty episodes + current nonempty indicator + allocations made while another slot was live. Peak occupancy is at most two. `order` is a legacy field name for a generation-history nibble, not a queue: bits zero and one store last generation for slots zero and one, and bits two and three store whether those slots were ever allocated. A never-seen slot has zero stored generation. `active` contains the two current occupancy bits.

## Protocol and companion application

Frames are ASCII `SG1,<kind>,<decimal fields>*<four uppercase CRC hex digits>\n`. CRC-16/CCITT-FALSE covers all bytes before the asterisk: polynomial 0x1021, initial 0xffff, no reflection, no final xor. Boot kind B has one reset-flags value. Policy kind P carries `3,1,5,2`, meaning qualification intervals, nominal sample milliseconds, setup observations, and slot capacity. Policy repeats about once per second independently of the one-shot boot record.

Status kind S fields are `now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail`. `launched` means accepted allocations, `completed` accepted releases, `rounds` closed nonempty episodes, and `queued` allocations while another slot was live; it does not imply FIFO ordering. Fault bits are 1 generation mismatch or non-toggling reuse, 2 orphan release, 4 duplicate live slot, 8 interface ambiguity, 16 acquisition gap, 32 counter exhaustion, and 64 watchdog startup failure. Only the first terminal fault is retained.

Status transmission is attempted every hundred nominal milliseconds when the transmitter is available. The bounded buffer expires after a hundred milliseconds rather than blocking acquisition indefinitely. A failed submission or timeout increments `tx_fail`. These snapshots are not a complete event log, and terminal level fields retain the fault observation rather than track later voltages. Neither serial input nor desktop buttons can command the target.

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk installed. No third-party Python packages are required. Select the exact virtual serial path, usually `/dev/ttyACM0`, and choose New live. The port opens read-only with 115200 8N1 configuration. A second connection is refused until disconnect. The application restores terminal settings when possible, detects hangup, and keeps evidence after disconnect.

The Ledger screen displays active slot/generation pairs, counters, phase, and findings. History retains up to 256 accepted snapshots. Integrity displays source, replay/live provenance, parser errors, policy compatibility, reset metadata, freshness, and freeze reason. Two advancing timestamps are needed for live progress. Duplicate timestamps do not refresh evidence. A long reception pause or excessive cumulative host/device drift freezes interpretation rather than presenting queued data as current. Uncalibrated oscillator drift can cause conservative rejection and must be investigated rather than hidden.

The companion rejects out-of-range and inconsistent snapshots. Sparse transition checks preserve seen history and require sufficient allocations and releases to explain changed generations or occupancy. These are necessary lower bounds, not exhaustive reconstruction of hidden operations or proof of generation parity over every unseen cycle. Counter rollback, disappearing faults, changed halted evidence, or a later boot freezes the session. Without a persistent boot identity, some lost-boot resets with plausible forward counters remain indistinguishable.

Replay accepts regular files up to two MiB, rejects FIFOs, bounds framing, resynchronizes on the complete marker, and records partial EOF tails as errors. Replay is always labelled non-live. Export creates a new mode-0600 JSON file, refuses overwrite and final-component symlinks, and includes author jayis1, provenance, integrity metadata, latest state, and history. Files are unsigned and unencrypted. Use a trusted destination directory and retain integrity qualifications in downstream reports.

## Build, verification, and laboratory workflow

From this directory run `make -C firmware all test`. Requirements are GNU make, host GCC with address/undefined-behavior sanitizers, and arm-none-eabi GCC with newlib/nosys support. Outputs are `firmware/build/slot-generation.elf`, `.bin`, and `.map`. No test flashes hardware. Inspect the map and verify the exact board before using a separately approved programming procedure.

Run `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v` for parser, model, export, replay, and actual Tk event-loop tests through Linux pseudoterminals. Run `python3 -m compileall -q app` for byte compilation. The C fixture exercises first allocation, release, toggled reuse, and stale release through the real encoder. It is synthetic software-generated evidence, not a measurement. The independent C oracle explores both slots, both initial generations, both operations, arbitrary release ordering, and repeated ownership cycles.

Native CAD generation uses KiCad 9 and its matching pcbnew Python module. Run `/usr/bin/python3 tools/generate_hardware.py`, then `/usr/bin/python3 tools/check_hardware.py`. The checker executes ERC and DRC, exports a fresh schematic netlist, and compares reference, value, footprint, pin, and net identity against actual board pads and the connectivity manifest. The schematic contains real embedded symbols; the PCB has routed copper and a closed outline. Rule checks cannot qualify the external module, wiring, protection, or physical safety.

For bring-up, review adapter semantics and document the two resources and generation lifetime. Establish both resources free, verify continuity and grounding, power the observer, and only then enable diagnostic outputs. Exercise both initial generations, independent slot cycling, overlapping ownership, and both release orders. Deliberately generate one stale release on a safe fixture and confirm the current generation stays active in the retained evidence. Repeat with same-generation reallocation, orphan release, and duplicate allocation, resetting the target baseline between terminal findings.

Physical acceptance remains outstanding: assembly inspection, resistor measurements, connector orientation, ground offsets, leakage, power sequencing, ST-LINK routing, waveform correlation, oscillator calibration, watchdog recovery, long-duration serial load, ESD behavior, and enclosure safety. Compare the instrument against an independent logic analyzer and target functional traces. Until those gates pass, describe this as a build-tested software and CAD prototype, not fabrication-qualified instrumentation or a production security boundary.
