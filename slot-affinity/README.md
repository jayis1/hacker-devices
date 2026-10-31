# Slot Affinity — First-Use Security-Domain Pinning Witness

Author and creator: jayis1. Hardware revision A. Wire protocol AF1.
Firmware, companion and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype. This directory supplies target-buildable firmware, a Linux desktop application and routed native KiCad carrier files. Executed checks are recorded in VERIFICATION.md. No physical fabrication, MCU programming, electrical characterization, calibrated timing, production certification or deployed target experiment is claimed. This instrument observes a diagnostic contract; it does not implement isolation or enforce access control.

## Purpose and practical value

Slot Affinity checks whether a reusable resource slot remains pinned to the security domain that first acquired it during an observed campaign. The important distinction is that releasing the resource does not erase its domain affinity. Even draining every slot does not authorize a new domain to acquire a previously assigned slot. Two slots are tracked independently. Both may belong to the same domain, or each may belong to a different domain, and both may be occupied concurrently. The first allocation of each slot establishes its binding; subsequent allocations must preserve that binding.

A motivating example is a security appliance that uses dedicated work buffers for two trust partitions. Buffers are allocated lazily: the first request using a slot assigns its security partition for the remainder of the process lifetime. A cleanup bug might correctly clear the busy flag but accidentally clear the partition assignment too. The next request from another partition could then obtain a buffer that still contains state belonging to the first partition. An ordinary live-ownership detector sees a legitimate release followed by an allocation to a free resource. Slot Affinity remembers the historical partition and flags the reassignment instead.

For example, allocate slot zero to domain one, release slot zero as domain one, and then allocate slot zero to domain zero. The final operation violates affinity even when no other resource is occupied. Conversely, allocate slot zero to domain one and slot one to domain zero: this is legitimate independent partitioning. Release in either order, reuse either slot with its original domain, or leave a slot unused indefinitely. Neither a global alternating rule nor a homogeneous-pool rule applies. The instrument distinguishes lifetime partitioning from merely exclusive live ownership.

The domain bit is an agreed security category, not a unique requester. Multiple clients may belong to the same domain. Slot identifiers must name two actual, distinct resources within the campaign. Do not truncate an address, hash many buffers into two slots, or multiplex unrelated generations without an independently defined baseline. Such adapters create collisions and destroy the meaning of the retained evidence. The observer cannot identify secret contents, establish erasure, detect a dishonest label, or show that two physically separate buffers do not share a cache line.

## Originality and platform reuse

The repository's configured default branch was pulled before work. Device directories and README titles were inventoried, and nearby ownership contracts were examined. Owner Bind checks one-to-one live client bindings and permits reassignment after release. Domain Cohort requires homogeneous concurrent residency and permits domain changes after full drain. Slot Generation requires a generation bit to toggle on reuse. Slot Alias checks resource occupancy without domain identity. This design instead preserves an immutable first-use label independently for each slot, including when both slots are vacant. These policies accept different traces and identify different defects.

Novelty is repository-relative, not a patentability or global prior-art claim. First-use assignment and partitioned allocation are established techniques. The contribution is a bounded external evidence instrument, its precise lifetime-binding contract, independent oracle tests, sparse-telemetry immutability validation and a reproducible lab workflow. The passive carrier, STM32 board support, startup, bounded serial sender and desktop transport intentionally reuse jayis1-authored Domain Cohort platform code. Internal OB_ identifiers and client variable names remain platform conventions; client means domain here. This directory is self-contained and does not import sibling sources when building or running. Standard KiCad footprint geometry remains upstream library material.

## Authorized use and threat model

Use only on equipment that you own or have explicit written authorization to evaluate. The agreement must cover opening the enclosure, internal diagnostic wiring, owner-provided instrumentation, test workload, expected interruptions, data retention and recovery. A network penetration-testing agreement does not necessarily authorize electrical attachment. Start with an isolated low-energy development fixture and disconnected hazardous outputs. Do not connect this unisolated prototype to production access controllers, medical equipment, vehicles, industrial machinery, mains-related wiring, or unidentified field voltages.

The assessed attack surface is resource lifecycle integration: cleanup callbacks, lazy allocation, cancellation, timeout recovery and resource reuse across trust domains. The device receives only four slow diagnostic signals. It does not inject packets, exercise exploits, induce voltage faults, read passwords, extract firmware, collect payloads or conceal its presence. An authorized red team may ask an owner to run an approved stress workload while observing whether resources migrate. A product researcher can compare patched and vulnerable allocator builds. A penetration tester can retain external evidence that a promised fixed-domain allocation rule was contradicted under the approved scenario.

The adapter must expose accepted allocation and completed release, not a request that the allocator correctly rejected. Release must mean the resource is genuinely no longer occupied under the stated contract. An early callback preceding DMA completion is not an equivalent boundary. Review the adapter with the owner and compare its signals against independent waveforms and functional logs. A malicious or defective adapter can hide every relevant event. The absence of a finding is not proof of secure isolation, correct sanitization, exhaustive coverage or a trustworthy target.

Trusted components include the operator, adapter, harness, Nucleo module, programmed firmware and workstation. CRC protects against accidental line corruption; it does not authenticate evidence. There is no secure boot, signed update service, authenticated remote control, persistent boot identifier or trusted wall clock. Someone with SWD access can replace the firmware and drive pins that this image configures as inputs. Receive-only describes the supplied implementation, not a physical data diode. Common grounds, loading, back-power and wiring errors can affect a target even without deliberate transmission.

## Hardware specification and bill of materials

Use an external ST NUCLEO-F401RE board with its STM32F401RET6 Cortex-M4: 512 KiB flash and 96 KiB SRAM. Firmware uses reset-default nominal 16 MHz HSI, one-millisecond SysTick, and USART2 transmit at 115200 baud, eight data bits, no parity and one stop bit. PA2 connects to the module's ST-LINK virtual serial bridge; USART reception is disabled. There is no radio, FPGA, antenna, storage card, battery, charger or analog sensor. The Nucleo supplies regulation, MCU decoupling, reset, boot configuration, programming access and USB connectivity; these are external module facilities, not missing components hidden inside the carrier schematic.

The custom carrier is a cable-connected passive adapter, not a mechanically stacking shield. Its outline is 80 by 70 mm, with two copper layers and nominal 1.6 mm board thickness. Routed tracks are 0.3 mm. Fourteen through-hole components provide thirty electrical pads and nine named nets. No enclosure or mounting-hole pattern is supplied. Use an insulating base, strain relief and a cover that prevents conductive contact with the underside. Check purchased resistor and connector dimensions against the footprints before ordering boards.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, revision checked before assembly |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header |
| J2 | 1 | 1x4 vertical 2.54 mm header |
| Harness and support | 1 set | Short labelled insulated leads and nonconductive support |

Resistors use DIN0207 geometry with 7.62 mm lead spacing. Component availability, pricing, thermal behavior and manufacturing yield are not verified. The schematic represents the external module interface as connectors rather than an incomplete MCU circuit.

| Signal | Target pin | J2 pin | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| SLOT | J3.1 | 1 | CN8.1 / A0 / PA0 | 0 |
| DOMAIN | J4.1 | 2 | CN8.2 / A1 / PA1 | 1 |
| ALLOCATE | J5.1 | 3 | CN8.3 / A2 / PA4 | 2 |
| RELEASE | J6.1 | 4 | CN8.4 / A3 / PB0 | 3 |

Pin two on every target header is ground. Both J1 contacts are ground only, never power inputs. J2 contains four signal contacts with no ground or supply contact. Connect a separate Nucleo ground wire to J1. Names ending _IN denote target-side nodes; unsuffixed names denote MCU-side nodes. Verify continuity and pin-one orientation with a meter instead of relying on wire colors.

Only known common-ground zero-to-3.3 V push-pull diagnostic signals are supported. Each input passes through 1 kOhm to a node with a 100 kOhm pulldown. This is not a qualified protection network. There is no galvanic isolation, dedicated ESD clamp, fuse, reverse-current barrier, hot-plug qualification or level translator. Five-volt signals, negative voltages, differential buses and 1.8 V logic are unsupported. Target and USB grounds are connected. Power the observer before enabling target outputs; disable outputs before removing observer power to reduce the risk of input back-power.

Power through the documented Nucleo ST-LINK USB connector after checking the purchased board's jumpers and solder bridges. Allow a 5 V, 500 mA source for planning, not as a measured draw or compliance claim. Check ST UM1724 for board connections, RM0368 for register definitions, and the STM32F401RE datasheet for electrical limits. Physical revision, ground offset, source impedance, cable length and loading still require bench verification.

## Architecture and firmware decisions

```text
Owned adapter                  Passive carrier                 Nucleo-F401RE
SLOT -------------------------- 1k ---+------------------------- PA0
DOMAIN ------------------------ 1k ---+------------------------- PA1
ALLOCATE ---------------------- 1k ---+------------------------- PA4
RELEASE ----------------------- 1k ---+------------------------- PB0
                                      | each input node
                                     100k
                                      |
Target GND ------------------------- GND ---------------------- USB GND

GPIO -> continuity guard -> bundled identity / independent strobes
     -> prior setup check -> immutable per-slot affinity + live occupancy
     -> transactional counters -> AF1 CRC snapshots -> bounded UART TX
     -> ST-LINK virtual serial -> Ledger / History / Integrity desktop
```

The drawing's pulldown is repeated independently on all four channels. GPIOA and GPIOB are read separately; no cross-port atomic latch is present. This is not a processor-speed bus analyzer. Begin with twenty milliseconds of identity setup, twenty milliseconds of strobe high and twenty milliseconds with both strobes low per operation. Confirm waveforms independently before reducing these fixture intervals. Sampling does not establish exact electrical edge ordering within a millisecond or calibrated physical timing.

Independently establish that both target resources are vacant and their campaign affinity is not yet assigned before starting the observer. WAIT becomes TRACK after all channels qualify and raw and qualified strobes are low. Quiet strobes do not prove an empty target or a cleared target affinity. Restarting only the observer while a target retains prior assignments loses the original reference and is outside the contract. Late desktop attachment to the same continuously running observer is supported because affinity remains in MCU RAM and policy announcements repeat.

SLOT and DOMAIN qualify as a two-bit bundle. ALLOCATE and RELEASE qualify independently so unrelated identity chatter cannot delay strobe qualification forever. A changed candidate needs three subsequent matching sample intervals. Five prior stable identity observations are required before an operation. The endpoint cannot retroactively earn setup. Raw identity must match qualified identity; the opposite strobe must be low in previous qualified, current qualified and raw observations. Simultaneous rising strobes are rejected together. Short pulses may be entirely missed: no exhaustive glitch detection is claimed.

Allocation first checks live occupancy. An occupied slot produces a duplicate fault. A vacant, never-seen slot captures either domain as its immutable label. A vacant, previously seen slot accepts only its existing label. The other slot has no influence on this decision. Release first checks occupancy, then compares its domain against the retained label. Only a matching release clears occupancy. Neither release nor full pool drain clears ever-assigned membership or labels. Duplicate and orphan classifications precede domain mismatch, preserving a deterministic fault explanation.

Each sample operates on a speculative ledger copy. Counter exhaustion discards the whole update and preserves the previous accepted state. Skipped acquisition ticks permanently halt correlation instead of pretending that unobserved operations did not occur. Same-timestamp calls do nothing; ordinary unsigned timer wrap is supported. On halt, residency, labels and accounting freeze while device time and the saturating transmission-failure diagnostic may advance. There is no host clear command, automatic rearm or flash journal. Export, deliberately reset the target campaign, and restart the observer to recover. RAM retention does not survive reset or power failure.

Conservation requires allocations = releases + occupied slots, and attempts = allocations + releases + rejected edges. Allocations also equal closed nonempty episodes + active-episode indicator + allocations admitted while another slot remained occupied. Peak occupancy cannot exceed two. The legacy order field packs immutable labels in bits zero and one and ever-assigned membership in bits two and three. An unseen slot has a zero label bit. This convention does not mean FIFO order, generation parity or domain equality between slots.

engine.c implements qualification, policy and invariants. wire.c validates and encodes snapshots. drivers/board.c configures GPIO, reset-cause capture, SysTick, UART and bounded watchdog initialization. drivers/serial.c sends nonblocking frames and handles timeouts. startup.c initializes RAM and supplies the reset vector. board.h, registers.h and linker.ld describe the actual target interfaces. No heap, RTOS, downloaded SDK or host-to-target command parser is required.

## Companion application and protocol

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk. No third-party Python package is needed. Enter the exact serial path and select New live. The app opens a read-only 115200 8N1 connection, refuses a second connection, restores terminal attributes where possible and detects hangup. Ledger displays live occupancy separately from pinned historical domains. History retains up to 256 snapshots. Integrity displays source, provenance, compatibility, parser errors, reset evidence and freeze reason. Replay accepts regular files of at most two MiB and remains explicitly non-live.

AF1 records are ASCII: `AF1,<kind>,<decimal fields>*<four uppercase CRC digits>\n`. CRC-16/CCITT-FALSE covers bytes before the asterisk, polynomial 0x1021, initial 0xffff, no reflection and no final xor. B carries reset flags. P carries `3,1,5,2`: qualification intervals, nominal sample milliseconds, setup observations and slot capacity. P repeats about once per second without resetting evidence. The protocol marker distinguishes this policy from sibling devices even though the platform field layout is shared.

S fields are `now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail`. Launched counts allocations; completed counts releases; rounds counts closed nonempty occupancy episodes; queued counts allocations while the other slot was occupied. Queued is concurrency coverage, not a queue or elapsed-time measurement. Fault bits are 1 affinity mismatch or wrong-domain release, 2 orphan release, 4 duplicate live slot, 8 interface ambiguity, 16 acquisition gap, 32 counter exhaustion and 64 watchdog startup failure. Only one terminal reason is retained.

Status is attempted every hundred nominal milliseconds when transmission storage is available. A stalled frame expires after a hundred milliseconds rather than blocking acquisition. Failed submissions and timeouts increment tx_fail. The records are sparse snapshots, not a complete operation log. Terminal input levels describe the finding endpoint, not current voltages. The companion checks unsigned bounds, conservation, sticky faults, halted-state immutability and temporal label retention. Previously assigned labels cannot change even if intervening counters describe a full drain and reallocation. New historical membership requires enough intervening allocations and releases to explain it.

Two advancing timestamps are required before live progress is displayed. Duplicate timestamps do not renew freshness. Long reception pauses and excessive capped host/device drift freeze interpretation instead of presenting queued evidence as current. Counter rollback or a later boot record also freezes it. Some resets with lost boot records and plausible forward counters cannot be distinguished without persistent boot identity. Necessary transition bounds are not an exhaustive reconstruction of hidden events. CRC and these checks do not provide authenticity.

Export creates a new mode-0600 JSON file, refuses overwrite and final-component symlinks, and includes author, retained evidence, history, provenance, errors and freshness. Use a trusted output directory. Reports are unencrypted and unsigned; retain them according to the engagement agreement. Ordinary deletion is not guaranteed secure erasure. A truncated replay tail is counted as a parser error; a recording without a compatible snapshot is rejected. Do not strip integrity metadata when sharing results.

## Build, tests and physical acceptance

Run `make -C firmware all test`. GNU Arm Embedded GCC and newlib/nosys build Cortex-M4 Thumb soft-float code for STM32F401RE. Outputs are firmware/build/slot-affinity.elf, .bin and .map. Host tests use GCC sanitizers. Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` for parser, model and real Tk event-loop tests through Linux pseudoterminals. Run `python3 -m compileall -q app` for byte compilation. Tests do not flash a board or create physical target measurements.

The independent C oracle stores fixed assignments separately from occupancy and explores both slots, domains and operations. It checks legal same-domain and different-domain concurrency, repeated reuse, both release orders, reassignment after drain, wrong releases, orphan and duplicate operations, setup ambiguity, chatter, missed ticks, timer wrap and transactional exhaustion. The real C encoder generates synthetic fixtures consumed by the companion. App tests cover CRC-valid impossible affinity changes, first-use history, disconnect, replay, freshness, private export and GUI screens.

Generate CAD with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9 pcbnew and standard footprint libraries. Run `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC, fresh schematic netlist export and exact reference/value/footprint/pin/net agreement with PCB pads and the manifest. Run `python3 tools/check_release.py` for documentation length, production C count and attribution. Passive-only ERC cannot approve the external powered module, electrical limits or physical safety.

Before physical deployment, inspect component values, connector orientation, continuity, leakage, ground offset and power sequencing. Establish an unassigned empty target campaign, start observation, and exercise all first-assignment combinations. Reuse slots with unchanged domains, then separately attempt an approved wrong-domain reuse after full drain. Compare the retained original binding with independent waveforms and owner logs. Reset the target campaign between terminal cases. Fabrication, flashing, ST-LINK routing, electrical loading, watchdog recovery, oscillator calibration, ESD behavior and sustained physical serial operation remain laboratory acceptance gates, not conclusions inferred from software or CAD tests.
