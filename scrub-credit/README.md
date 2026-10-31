# Scrub Credit — Per-Resource Sanitization-Credit Witness

Author and creator: jayis1. Hardware revision A. Wire protocol SC1.
Firmware, application, and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design with target-linked firmware, an executable Linux desktop companion, and native routed KiCad sources. Software and CAD verification are recorded in VERIFICATION.md. Physical fabrication, programming, electrical measurements, timing calibration, and deployment have not been performed. This instrument observes diagnostic indications; it neither performs erasure nor certifies that data was destroyed.

## Purpose and practical value

Scrub Credit checks a resource-recycling boundary: every accepted allocation of a particular slot must consume one fresh sanitization-completion acknowledgment for that same slot. Returning the slot makes it vacant but does not restore the acknowledgment. A second sanitization indication cannot be banked while the slot is already clean, and a sanitization indication while the slot remains occupied is rejected. Two resources are tracked independently, allowing cleanup of one buffer while another remains in use.

The motivating target is an embedded security appliance with two reusable work buffers. Its owner supplies intentionally slow diagnostic instrumentation around accepted allocations, completed returns, and completed cleanup. A cancellation path might return a buffer to the free list without running cleanup. Another defect might route one buffer's cleanup completion to the other buffer's available flag. Both can appear reasonable in aggregate counters: the system has performed some cleanup, and something is free. This device retains resource identity so those aggregates cannot silently substitute for the actual per-slot obligation.

Each slot follows the cycle DIRTY, CLEAN, OCCUPIED, DIRTY. DIRTY means vacant without a witnessed cleanup credit, not measured physical contamination. CLEAN means one accepted diagnostic acknowledgment, not an independently tested zero-filled buffer. OCCUPIED means one accepted allocation not yet returned. The next transition is fixed for each state, but transitions on different slots can interleave freely. Cleanup of slot zero does not authorize allocation of slot one. Releasing every resource does not create any credits. The first allocation after observer startup also requires an observed cleanup.

A useful authorized experiment compares a normal completion path with a timeout or cancellation path. First establish an empty target pool, start the instrument, and exercise cleanup, allocation, and return. Then run the owner's approved workload that previously bypassed a cleanup callback. The device retains the first contradicted lifecycle and stops correlation rather than inventing a recovery. Its evidence can help locate an integration defect; separate tests must establish whether sensitive information was present, whether the cleanup actually erased it, and whether a real privilege boundary was crossed.

## Originality and deliberate platform reuse

The repository was pulled and its device directories and README titles inventoried before design. Closely related purposes and contracts were reviewed. Erase Witness observes a global request/busy/done/access handshake with timing requirements; it does not track two independently reusable slots or consume per-slot credits. Grant Ledger checks one outstanding authorization request and a timed grant/use/revoke sequence, not the interleaved lifecycle of reusable resources. Slot Affinity permanently pins each slot to its first domain, including after release. Scrub Credit permits repeated reuse after fresh per-slot cleanup and does not track domains at all.

Novelty is relative to this reviewed repository, not a worldwide prior-art or patentability claim. Resource sanitization is established engineering practice. The contribution here is a bounded, externally observable cleanup-credit contract, its per-slot state machine, independent oracle tests, sparse-record reachability checks, and reproducible hardware/software package. The circuit topology, startup, board support, bounded UART driver, desktop transport, and CAD utilities deliberately reuse jayis1-authored Slot Affinity infrastructure. Those reused facilities are not claimed as new inventions. Builds are self-contained and do not read sibling device directories. Standard KiCad footprint geometry is upstream library material.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written authorization to assess. Authorization must include internal wiring, owner-provided diagnostic instrumentation, workload generation, permitted interruptions, evidence retention, and recovery. A network penetration-test agreement does not automatically permit electrical attachment. Start on isolated low-energy development equipment with hazardous outputs disconnected. Do not connect this unisolated prototype to deployed locks, vehicles, medical devices, industrial actuators, mains-related circuits, or unidentified field signals.

The assessed attack surface is resource lifecycle integration: cleanup callbacks, asynchronous completion dispatch, free-list insertion, cancellation, timeout handling, and buffer reuse. The device never drives a target-facing line. It has no radio, network service, credential parser, packet injection, exploit loader, storage reader, or automatic discovery. Red teams can observe an owner-approved stress scenario; security researchers can compare firmware revisions; penetration testers can retain external evidence of a promised cleanup-before-reuse rule being contradicted. No unauthorized targeting or covert collection is required or supported.

The trusted computing base includes the operator, target adapter, harness, Nucleo module, programmed image, and workstation. The adapter must expose actual accepted allocation, completed return, and completed sanitization, not requests that were correctly refused. A return before DMA has stopped is not equivalent to completed ownership release. A cleanup-start notification is not a cleanup-completion notification. The owner should review where each indication originates and correlate it with independent waveforms and functional logs.

A dishonest or broken target can emit a perfectly plausible diagnostic trace while leaking data. The instrument does not inspect memory, verify cache invalidation, observe invisible DMA, distinguish tenants, authenticate resource names, or establish cryptographic erasure. A slot identifier must designate one actual resource throughout the run. Truncating an address or hashing many objects into two identities defeats the interpretation. Additional resources require a different reviewed protocol rather than silently multiplexing them.

The device itself has no secure boot, signed update channel, persistent boot identifier, protected keys, or trusted clock. Physical SWD access permits firmware replacement. Receive-only describes the supplied firmware, not a hardware data diode. The host and a malicious USB endpoint can fabricate records. CRC and invariant checks detect some accidental corruption and contradictions but do not authenticate measurements. Exported files are neither encrypted nor signed. Keep programming access and the workstation under physical control and treat evidence as corroborative, not tamper-proof.

## Hardware specifications and bill of materials

The processor is the STM32F401RET6 on an external ST NUCLEO-F401RE development board: Cortex-M4, 512 KiB flash, 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock and a one-millisecond SysTick. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit, on PA2 toward ST-LINK's USB virtual serial bridge. USART reception is disabled. There is no RTOS, downloaded SDK, FPGA, radio, antenna, analog sensor, storage card, battery, or charger.

The custom PCB is an 80 by 70 mm cable-connected passive carrier, not a mechanically stacking Nucleo shield. It has two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks, fourteen through-hole components, thirty pads, and nine electrical nets. The external Nucleo provides MCU power regulation, decoupling, reset, boot configuration, debug access, and USB connection. These facilities are not falsely represented as components on the passive carrier. No enclosure or mounting-hole pattern is supplied; use an insulating support, covered underside, and strain-relieved leads.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, board revision checked before assembly |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header |
| J2 | 1 | 1x4 vertical 2.54 mm header |
| Harness and support | 1 set | Short labelled insulated leads and nonconductive mounting |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Confirm purchased parts against the actual footprints. Cost, availability, thermal performance, manufacturing yield, and connector durability have not been measured.

| Signal | Target contact | J2 contact | Nucleo connection | Firmware bit |
| --- | --- | --- | --- | --- |
| SLOT | J3.1 | 1 | CN8.1 / A0 / PA0 | 0 |
| SCRUB | J4.1 | 2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | 3 | CN8.3 / A2 / PA4 | 2 |
| RETURN | J6.1 | 4 | CN8.4 / A3 / PB0 | 3 |

Pin two of each target connector is ground. Both contacts of J1 are ground only, never supply inputs. J2 carries four signals and no ground or power contact; connect a separate Nucleo ground lead to J1. Names ending _IN are the target side of a series resistor; unsuffixed names are MCU-side nodes. Verify pin-one orientation and continuity with a meter.

Only known common-ground zero-to-3.3 V push-pull diagnostic signals are supported. Each input has 1 kOhm series resistance followed by a 100 kOhm pulldown. There is no galvanic isolation, qualified overvoltage clamp, dedicated ESD protection, fuse, reverse-current barrier, hot-plug qualification, or level translator. Five-volt outputs, negative voltages, differential buses, and 1.8 V logic are unsupported. A disconnected cable tends to read low and is not separately detected. The series resistor does not turn an unknown circuit into a safe connection.

Power the Nucleo through its intended ST-LINK USB connector using the documented board configuration. Reserve a 5 V, 500 mA source for planning; this is not measured consumption or a USB compliance claim. Verify ground offsets before attachment. Power the observer before enabling target outputs and disable those outputs before removing observer power to reduce back-power through input clamps. Both USB and target grounds are connected. Short leads, input loading, and actual thresholds require laboratory validation.

Manufacturer reference: ST UM1724, Table 16, identifies CN8 A0–A3 as PA0, PA1, PA4, PB0 on NUCLEO-F401RE. Section 7.10 documents the default virtual serial routing with SB13/SB14 on and SB62/SB63 off. Check the purchased revision rather than modifying solder bridges blindly. Consult RM0368 for peripheral registers and the STM32F401xD/xE datasheet for electrical limits. The official manual is https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf .

## Architecture and block diagram

```text
Owned diagnostic adapter          Passive carrier                 Nucleo-F401RE
SLOT ----------------------------- 1k ---+------------------------ PA0
SCRUB ---------------------------- 1k ---+------------------------ PA1
ALLOCATE ------------------------- 1k ---+------------------------ PA4
RETURN --------------------------- 1k ---+------------------------ PB0
                                         | each signal separately
                                        100k
                                         |
Target GND ----------------------------- GND --------------------- USB GND

GPIO -> continuity guard -> bundled SLOT/SCRUB and independent strobes
     -> prior setup guard -> two independent dirty/clean/occupied cycles
     -> transactional counters -> SC1 CRC snapshots -> bounded UART TX
     -> ST-LINK virtual serial -> Ledger / History / Integrity desktop
```

The pulldown is repeated on every channel. GPIOA and GPIOB are read separately; there is no cross-port atomic latch. This is a deliberately slow diagnostic observer, not a processor-speed bus analyzer. Begin with twenty milliseconds of stable SLOT/SCRUB data, twenty milliseconds of strobe high, and twenty milliseconds with both strobes low per event. Confirm these waveforms independently before reducing margins. Sub-millisecond ordering and short electrical glitches cannot be inferred from qualified snapshots.

## Firmware contract and design decisions

Before starting the observer, independently establish an empty target pool and begin a new diagnostic campaign. Both slots initially have no cleanup credit, even if the operator believes they were previously sanitized. Repeat genuine cleanup or a reviewed diagnostic acknowledgment within this campaign before first allocation. WAIT becomes TRACK after all inputs qualify and both raw and qualified strobes are low. Quiet strobes establish only the electrical baseline; they do not prove target vacancy. Restarting only the observer during a live target allocation loses history and is unsupported.

SLOT identifies resource zero or one. SCRUB is a data modifier, not an independent event strobe. An ALLOCATE rising edge with SCRUB low allocates the selected slot. A RETURN rising edge with SCRUB low releases it. A RETURN rising edge with SCRUB high acknowledges completed sanitization. ALLOCATE with SCRUB high is reserved and rejected. Both strobes rising together are ambiguous and rejected as two attempted edges. There is no combined return-and-clean operation: the two transitions must occur separately, with an observed low-strobe interval between them.

SLOT and SCRUB qualify as one two-bit bundle so intermediate bit combinations cannot create a synthesized operation type. Each strobe qualifies independently. A changed candidate requires three subsequent matching one-millisecond intervals. Five prior stable data observations are required at an operation endpoint. Endpoint data must also match raw data, and the opposite strobe must be low in previous qualified, current qualified, and raw samples. The endpoint cannot retroactively earn its own setup interval. Short pulses can be missed completely; the device does not claim exhaustive glitch detection.

Allocation first checks occupancy, then credit. An occupied slot produces duplicate allocation. A vacant slot without credit produces a credit fault. A valid allocation clears its credit and marks it occupied. Return requires occupancy and clears it without granting credit. Scrub completion requires a vacant, uncredited slot and grants exactly one credit. Scrubbing a live or already-clean slot produces a credit fault. Scrubbing one slot while the other is occupied is permitted, as are either allocation order and either return order. There is no FIFO, global-domain, alternating-slot, or whole-pool-cleanliness requirement.

Every sample is applied to a speculative ledger copy. If a counter would overflow, the entire sample update is discarded and the previous accepted resource state is retained with an exhaustion fault. A skipped acquisition tick halts correlation permanently; unseen events cannot safely be reconstructed from later quiet signals. Repeated same-timestamp calls do nothing, and ordinary unsigned tick wrap is supported. After a halt, accounting, input endpoint, and resource state freeze; device time and the saturating transmit-failure diagnostic can still advance. Recovery requires export, deliberate target-baseline restoration, and a new observer run. There is no host clear command or automatic rearm.

The central conservation equations are scrubbed = launched + number of clean slots; launched = completed + number of occupied slots; attempts = scrubbed + launched + completed + rejected. Clean and occupied masks must be disjoint. Peak occupancy is at most two. Rounds count nonempty occupancy episodes that ended with an empty pool; they do not count cleanup rounds. With peak occupancy one, every return closes an episode. These checks are implemented in both the embedded encoder and companion.

Production files include main.c for scheduling, engine.c for qualification and policy, wire.c for bounded encoding, drivers/board.c for actual GPIO/clock/UART/watchdog access, drivers/serial.c for nonblocking transmission, startup.c for reset vectors and RAM initialization, registers.h for used MMIO definitions, board.h for platform interfaces, and linker.ld for target memory placement. No heap or inbound command parser is used. Watchdog initialization waits are bounded. A failed watchdog startup emits a terminal diagnostic, not a plausible tracking state. Flash programming, watchdog timing, and physical recovery remain untested.

## Companion application and record interface

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk. No third-party Python package is required. Enter the exact serial path, usually /dev/ttyACM0 for a single lab board, and choose New live. The application opens read-only 115200 8N1 transport, refuses a second connection, restores terminal attributes where possible, and detects hangup. Ledger displays active and clean vacant slots separately. History retains up to 256 snapshots. Integrity displays provenance, compatibility, parser errors, reset evidence, source, and freeze reason. These are functioning screens, not illustrations.

SC1 records are ASCII `SC1,<kind>,<decimal fields>*<four uppercase CRC digits>` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, polynomial 0x1021, initial 0xffff, no reflection, and no final xor. B carries reset flags. P carries `3,1,5,2`: qualification intervals, nominal sample milliseconds, setup observations, and slot capacity. Policy repeats about once a second and does not erase evidence. It enables late desktop attachment to a continuously running observer even when the initial boot record was lost.

S fields are `now levels valid halted faults phase active clean stable observed rounds attempts launched completed rejected gaps missed scrubbed peak tx_fail`. All are unsigned 32-bit decimal integers. Masks active and clean use bits zero and one for the corresponding slots. Fault bits are 1 credit violation, 2 orphan return, 4 duplicate allocation, 8 interface ambiguity, 16 acquisition gap, 32 exhaustion, and 64 watchdog startup failure. Exactly one terminal reason is retained. Status is attempted every hundred nominal milliseconds. An unfinished UART frame expires after a hundred milliseconds instead of blocking acquisition.

The companion checks bounds, conservation, sticky faults, monotonic counters, halted-state immutability, and per-slot reachability. Between snapshots, each slot must move along the dirty-to-clean-to-occupied cycle. The shortest path determines minimum required scrubs, allocations, and returns. Additional hidden complete cycles add one of each. This rejects a credit migrating between slots without the operations needed to explain that movement, even when both individual snapshots have valid CRCs and balanced totals. These are strong structural checks, not complete timing reconstruction or proof of every hidden episode.

Two advancing timestamps are required for live progress. Duplicate timestamps do not renew freshness. Long host reception pauses and excessive capped device/host drift freeze interpretation rather than relabel queued records as current. Counter rollback and later boot records also freeze it. Without persistent boot identity, some resets with lost boot records and plausible forward values remain indistinguishable. LIVE describes transport progress, not a safe target; a terminal fault may be freshly reported.

Replay accepts regular files no larger than two MiB, is explicitly non-live, and rejects a recording without a compatible snapshot. An incomplete tail is counted as a parser error. Export JSON creates a new mode-0600 file, refuses overwrite and final-component symlinks, and includes author, source, provenance, retained history, parser errors, reset evidence, and freshness. Choose a trusted parent directory. Files can disclose activity timing even without payloads; follow the engagement retention policy. Ordinary deletion is not guaranteed secure erasure.

## Build, verification, and physical acceptance

From this directory run `make -C firmware all test`. GNU Arm Embedded GCC plus newlib/nosys link the actual Cortex-M4 Thumb soft-float target image. Outputs under firmware/build are ignored by Git. Host tests use sanitizers and an independent per-slot automaton oracle covering both identities, every operation, repeated cycles, and both return orders. They also exercise setup ambiguity, raw overlap, chatter, gaps, timestamp wrap, held strobes, transactional exhaustion, serial timeout, and periodic policy after lost boot transmission.

Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` for the Python contract, real C-encoder fixtures, malformed input, CRC-valid impossible transitions, freshness, backlog, private export, and real Tk event-loop tests through Linux pseudoterminals. Run `python3 -m compileall -q app` for syntax compilation. Fixtures are synthetic and labelled accordingly; no test output represents a physical target measurement. The GUI test switches all screens, receives serial data, disconnects, replays encoder output, and invokes export.

Generate CAD with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9 pcbnew and standard libraries. Run `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC, fresh schematic netlist export, and exact reference/value/footprint/pin/net agreement against PCB pads and the manifest. Run `python3 tools/check_release.py` for README length, substantive production C count, attribution, and basic scope checks. Passive ERC cannot validate the powered external module. Zero CAD violations do not establish manufacturing qualification or electrical safety.

Before deployment, inspect assembled values, pin orientation, continuity, ground offset, input loading, and power sequencing. Establish the empty target baseline. Exercise each slot independently, then interleave cleanup and reuse while the other slot remains occupied. Test both resource identities for missing cleanup, stacked cleanup, live cleanup, orphan return, and duplicate allocation using owner-approved diagnostics. Correlate every finding with independent waveforms and target logs. Restore the target baseline between terminal cases. Fabrication, MCU programming, electrical characterization, oscillator calibration, ESD behavior, sustained physical USB operation, and actual sanitization efficacy remain separate laboratory acceptance gates.
