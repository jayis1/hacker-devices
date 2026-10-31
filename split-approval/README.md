# Split Approval — Per-Proposal Separation-of-Duties Witness

Author and creator: jayis1. Hardware revision A. Wire protocol SP1.
Firmware, application, and tests: GPL-2.0-only. CAD generation and verification utilities: MIT. Passive carrier design: CERN-OHL-S-2.0. These identifiers apply to the original material in their corresponding directories; installed toolchains and standard KiCad libraries retain their upstream licenses.

Status: unbuilt engineering prototype with target-buildable firmware, a Linux desktop companion, and native routed KiCad carrier sources. Executed checks are recorded in VERIFICATION.md. Physical fabrication, programming, electrical qualification, calibrated timing, and deployment have not been performed. This is an observer, not an approval controller, safety interlock, cryptographic identity verifier, or compliance certification.

## Purpose and overview

Split Approval checks a specific separation-of-duties boundary: the actor whose proposal occupies a slot must not be the actor whose accepted approval completes it. Two proposal slots can be pending independently, and either of two named actors may propose either slot. One actor can legitimately propose both slots. Completion order is unrestricted. The instrument retains the proposer associated with each pending slot and checks approval identity against that historical value, rather than against the actor currently appearing on another channel or slot.

The motivating system is an owner-controlled embedded maintenance console that separates preparation of a privileged change from authorization to apply it. A callback-context error can approve a proposal under its original author's identity even though the user interface presents a different reviewer. Another defect can associate a reviewer with the wrong pending descriptor. Ordinary operation counts can show balanced proposals and approvals while hiding self-approval. This device preserves the small identity relationship needed to expose that contradiction without receiving the contents of the proposed change.

For example, actor zero proposes slot zero and then slot one. Actor one approves slot one first and slot zero second. Both completions are valid, and there is no required FIFO or alternating-role schedule. By contrast, if actor zero approves either pending proposal, the instrument freezes a self-approval finding and retains the outstanding proposer and slot. An approval of a vacant slot is classified as orphan approval. A proposal targeting an already pending slot is duplicate proposal, even if its actor differs from the original author.

The words actor and slot have precise meanings. ACTOR must distinguish two actual authorization principals for this experiment, not two screens used by the same account or a hash of a larger user population. SLOT must identify one of two independent pending proposal records. This instrument cannot establish that the actors are different humans, that their credentials are independent, or that an accepted diagnostic approval caused the intended physical action. Those questions require additional source, identity, and functional evidence.

## Novelty relative to this repository

The configured main branch was pulled before design. Device directories and all available README titles were inventoried, and the closest policy descriptions were inspected. A repository-wide README search for self-approval, separation of duties, and proposer terminology found no matching existing contract. Owner Bind is the closest implementation platform, but it requires release by the same assigned client and forbids one client owning both live slots. Split Approval deliberately requires a different actor at completion and permits the same proposer in both slots. These are different observable policies, not interchangeable labels.

Coincidence Guard checks fresh paired consent timing and held-input reuse; it does not bind an approval identity to a particular proposal's author. Barrier Mask accumulates a fixed membership set before collective release rather than independently completing proposals. The distinctive contribution here is the per-proposal separation-of-duties policy, independent-slot behavior, retained proposer evidence, companion interpretation, and independent array-oracle tests. Originality is repository-relative, not a claim of worldwide invention or patentability.

The passive carrier geometry and low-level STM32 platform deliberately reuse jayis1-authored work. The UART scheduler, startup code, CRC writer, and Linux transport are reusable infrastructure. This folder is self-contained: builds and app execution do not import sibling device folders. Reusing a known carrier avoids introducing unnecessary electrical variation while evaluating a new diagnostic contract. It does not turn prior software checks into physical validation of this assembly.

## Authorized use, attack surface, and threat model

Use only on equipment you own or have explicit written authorization to assess. The authorization should cover opening the enclosure, diagnostic firmware, attachment points, ground connections, permitted workloads, interruption, and evidence retention. Begin on a low-energy development fixture with hazardous outputs disconnected. Do not connect this unisolated prototype to production access controls, vehicles, medical equipment, machinery, mains-related circuits, or unknown field voltages. Stop if attachment changes target behavior.

The assessed attack surface is proposal and approval bookkeeping at an embedded privilege boundary. Relevant defects include self-approval through reused session context, duplicate proposal insertion, orphan completion, wrong-slot approval routing, and diagnostic identity changing at an operation endpoint. An authorized red team can request an owner-approved test workload that exercises those paths. A penetration tester can capture evidence before and after a confirmed remediation. A security researcher can compare callback scheduling and pending-record reuse while avoiding collection of privileged payload contents.

The adapter must export accepted target operations, not arbitrary requests that the target correctly refuses. A rejected self-approval attempt must not be presented as an accepted approval. Conversely, suppressing an erroneous accepted operation makes the diagnostic evidence falsely reassuring. Review the adapter and correlate it with independent functional traces. The device does not initiate tests, inject credentials, execute exploits, capture passwords, persist on a target, or enforce the policy. A malicious target can lie on all four wires.

Trusted elements include the operator, wiring, MCU image, development board, programming interface, and workstation. CRC protects against accidental corruption, not deliberate forgery. There is no signed evidence, secure boot, trusted wall clock, persistent boot identity, authentication service, encrypted export, network listener, radio, or cloud dependency. The supplied firmware leaves target GPIO as inputs, but a replacement image could drive those pins. Receive-only is a property of this image, not an irreversible hardware data diode.

## Hardware specification and bill of materials

The complete assembly comprises an external ST NUCLEO-F401RE board, the supplied passive carrier, short labelled leads, an insulating support, and a Linux workstation. The external STM32F401RET6 Cortex-M4 provides 512 KiB flash and 96 KiB SRAM. Firmware uses nominal reset-default 16 MHz HSI, one-millisecond SysTick, and USART2 transmit-only telemetry at 115200 baud, eight data bits, no parity, one stop bit. PA2 routes toward the Nucleo ST-LINK virtual COM bridge.

The Nucleo supplies MCU, regulator, decoupling, reset circuit, boot configuration, SWD programming, and USB connectivity. The custom carrier does not claim those components. Its layout is an 80 by 70 mm cable-connected board, not a stacking shield, with two copper layers, nominal 1.6 mm thickness, 0.3 mm tracks, fourteen through-hole components, thirty connected electrical pads, and nine named nets. There are no mounting holes or enclosure files. Provide insulated supports, strain relief, and a protective cover.

| Item | Quantity | Specification |
| --- | --- | --- |
| External MCU module | 1 | ST NUCLEO-F401RE, revision checked before wiring |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm header |
| J2 | 1 | 1x4 vertical 2.54 mm header |
| Harness and support | 1 set | Short insulated wires, labels, nonconductive base |

Resistors use DIN0207 footprints with 7.62 mm lead spacing. Verify purchased dimensions, connector access, mating orientation, drill fit, and environmental suitability. Stock availability, pricing, and assembly yield have not been verified. There is no FPGA, antenna, battery, charger, analog sensor, active level shifter, target supply output, galvanic isolator, or storage card. Omitting radios also avoids introducing a transmitter authorization or RF interference requirement into this measurement.

| Function | Target pin | J2 pin | Nucleo connection | Sample bit |
| --- | --- | --- | --- | --- |
| SLOT | J3.1 | 1 | CN8.1 A0 / PA0 | 0 |
| ACTOR | J4.1 | 2 | CN8.2 A1 / PA1 | 1 |
| PROPOSE | J5.1 | 3 | CN8.3 A2 / PA4 | 2 |
| APPROVE | J6.1 | 4 | CN8.4 A3 / PB0 | 3 |

Pin two of each target header is ground. Both J1 pins are ground only, not supply inputs. J2 carries four signals with no ground or supply contact. Connect a separate Nucleo ground lead to J1. Nets ending `_IN` identify the target side of the series resistor; unsuffixed nets are MCU-side nodes. Verify orientation and continuity rather than inferring pin order from wire colors.

Only known common-ground zero-to-3.3 V push-pull diagnostic outputs are supported. Each signal passes through 1 kOhm to a node pulled down by 100 kOhm. The nominal resistive loading path is 101 kOhm before leakage and source impedance. This is not a qualified overvoltage barrier. There is no dedicated ESD clamp, fuse, reverse-current blocker, hot-plug guarantee, or isolation. Exclude 5 V, negative voltages, 1.8 V logic, differential buses, relay coils, and unknown industrial wiring.

Power the external module through its documented ST-LINK USB arrangement. A conservative source-planning allowance is 5 V at 500 mA, not measured consumption or a USB compliance claim. Check jumper and solder-bridge settings against the actual module revision. USB and target grounds become common. Enable target outputs only after observer power is established; remove target outputs before depowering the observer to avoid input-structure back-power. Consult ST UM1724 for MB1136 wiring, RM0368 for registers, and the STM32F401RE datasheet for electrical limits. The inherited connector map still requires confirmation against the purchased board before physical use.

## Architecture and block diagram

```text
Owned diagnostic adapter           Passive carrier          External Nucleo
SLOT ------------------------------ 1k --+------------------ PA0
ACTOR ----------------------------- 1k --+------------------ PA1
PROPOSE --------------------------- 1k --+------------------ PA4
APPROVE --------------------------- 1k --+------------------ PB0
                                         | each MCU node
                                        100k
                                         |
Target ground -------------------------- GND --------------- USB ground

GPIO -> continuity guard -> bundled identity / independent strobe qualifiers
     -> prior setup check -> per-slot proposer ledger -> self-approval check
     -> bounded SP1 / CRC encoder -> USART2 TX -> ST-LINK USB
     -> Linux Ledger / History / Integrity screens -> private JSON evidence
```

GPIOA and GPIOB are read sequentially, not through an atomic cross-port latch. This is a deliberately slow diagnostic interface, not a high-speed bus analyzer. Begin with twenty milliseconds of identity setup, twenty milliseconds of high strobe, and twenty milliseconds with both strobes low per operation. These margins are fixture starting points, not calibrated electrical specifications. Verify waveforms independently. Input qualification does not establish sub-sample edge ordering, cable delays, clock tolerance, or MCU port-read skew.

## Firmware contract and decisions

Before starting the observer, independently establish that both target proposal slots are empty. Quiet wires cannot establish that fact. WAIT becomes TRACK when every input has qualified and both raw and qualified strobes are low. Activity before that baseline is outside coverage. Attaching the desktop late to an already-running observer is supported through periodic policy frames and retained status; restarting the observer midway through unknown target activity is not.

SLOT and ACTOR qualify as a single two-bit bundle. PROPOSE and APPROVE qualify independently. A candidate requires three subsequent matching sample intervals after its initial observation. Five prior stable identity observations are required before accepting an operation endpoint; setup is not earned on the endpoint itself. Raw identity must agree with the qualified identity. The opposite strobe must be low in previous qualified, current qualified, and raw observations. Two simultaneous rising strobes reject both edges as ambiguous. Short pulses may be missed or intentionally filtered.

A proposal first checks whether its slot is pending. If so, the operation is rejected and historical proposer evidence remains unchanged. Otherwise the current actor is stored for that slot and occupancy is set. The other slot's actor imposes no restriction. An approval first checks that its slot is pending. If vacant, it is orphan approval. If occupied and ACTOR equals the stored proposer, it is self-approval. Only the other actor can complete the proposal and clear occupancy. Completion retains the last proposer for historical inspection. Either actor may propose that slot again later.

All sample updates occur on a speculative copy. If any cumulative counter would overflow, the entire sample update is discarded and the prior accepted ledger freezes under counter-exhaustion fault. A missing acquisition tick similarly produces permanent halt rather than pretending that the unobserved interval was quiet. Calls at the same timestamp are ignored. Ordinary unsigned timestamp wrap is supported; outages longer than the modular time interpretation cannot be reconstructed. On halt only device time and the saturating transport-failure diagnostic may advance. Export evidence, reestablish the target baseline, and deliberately restart to recover.

The ledger conserves accepted proposals = completed approvals + pending slots. Attempts = accepted proposals + completed approvals + rejected operation edges. Accepted proposals also equal closed nonempty episodes + one if any proposal is pending + proposals accepted while another proposal was pending. Peak occupancy cannot exceed two. These relationships provide checkable evidence consistency, not proof that the target reported truthfully. There is no approval deadline, cancellation operation, stale-approval generation tag, or cryptographic payload binding. Targets requiring those semantics need a different interface.

`engine.c` implements acquisition and policy. `wire.c` validates and serializes bounded snapshots. `drivers/board.c` owns input GPIO, SysTick, USART setup, reset-cause capture, and bounded watchdog initialization. `drivers/serial.c` handles copied, nonblocking transmission. `startup.c` initializes data and BSS and supplies the Cortex-M4 reset vector. `board.h`, `registers.h`, and `linker.ld` specify target pins, addresses, and memory placement. No heap, RTOS, SDK download, or host-to-target command parser is required.

## Companion application and protocol

Records are ASCII `SP1,<kind>,<decimal fields>*<four uppercase CRC digits>` followed by LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, polynomial 0x1021, initial 0xffff, no reflection or final xor. B contains reset flags. P contains `3,1,5,2`: qualification intervals, nominal sample milliseconds, setup observations, and slot capacity. Policy repeats about once per second and does not reset evidence. B is a one-shot boot announcement, not a periodic keepalive.

S fields in order are `now levels valid halted faults phase active order stable observed rounds attempts launched completed rejected gaps missed queued peak tx_fail`. Launched means accepted proposals; completed means accepted different-actor approvals. Rounds counts closed nonempty episodes and queued counts proposals made while a peer proposal was pending, not FIFO entries. Active is the pending-slot mask. Order is a legacy packed field: bits zero and one retain slot proposers, and bits two and three retain ever-proposed membership. Unseen slots have zero stored proposer.

Fault bits are 1 self-approval, 2 orphan approval, 4 duplicate pending proposal, 8 interface ambiguity, 16 acquisition gap, 32 counter exhaustion, and 64 watchdog startup failure. The first terminal reason remains latched. Status transmission is attempted every hundred nominal milliseconds when UART is available. A stalled packet expires after a hundred milliseconds rather than blocking acquisition. Failed submissions and timeouts increment tx_fail. Status is sparse evidence, not a complete operation trace.

Run `python3 app/console.py` on Linux with Python 3.10 or newer and Tk installed. No third-party Python package is needed. Enter the exact virtual serial path and select New live. The serial transport opens read-only, configures 115200 8N1, refuses a second simultaneous connection, restores terminal settings when possible, and detects hangup. Ledger displays pending slot/proposer pairs, counters, phase, and findings. History holds up to 256 snapshots. Integrity displays origin, compatibility, parser errors, reset metadata, freshness, and freeze reasons.

Two advancing device timestamps are required before live progress is shown. Duplicate timestamps do not renew freshness. A prolonged reception pause or excessive cumulative host/device drift freezes interpretation rather than presenting buffered records as current. The allowance is bounded, so small persistent backlog cannot accumulate indefinitely. Uncalibrated oscillator drift can conservatively force reconnect. Counter rollback, disappearing sticky faults, changed halted evidence, and later boot records also freeze the session. Some lost-boot resets remain indistinguishable without persistent boot identity.

The companion checks single-frame conservation and necessary transition bounds between sparse snapshots. Changing a retained proposer requires enough new proposals and completions to replace any old live proposal. Ever-proposed history cannot disappear. These checks do not reconstruct every hidden operation and cannot independently recover an approval actor absent from retained snapshots; firmware performs the per-edge actor comparison. A valid checksum alone does not establish validity or authenticity.

Replay accepts regular files up to two MiB, rejects FIFOs and device files, bounds partial frames, resynchronizes on a complete SP1 marker, and counts incomplete EOF tails. Replay is never live. Export creates a new mode-0600 JSON file and refuses overwriting or final-component symlinks. Reports retain jayis1 authorship, origin, connectivity, freshness, errors, reset information, latest state, and bounded history. Storage is unencrypted and unsigned; use a trusted directory and agreed retention policy.

## Build, tests, and physical acceptance

From this folder run `make -C firmware all test`. Requirements are GNU make, host GCC with address and undefined-behavior sanitizers, and arm-none-eabi GCC with newlib/nosys support. Outputs are `firmware/build/split-approval.elf`, `.bin`, and `.map`. No test flashes hardware. Run `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v` for contract, protocol, transport, replay, export, and actual Tk event-loop tests. Run `python3 -m compileall -q app` for byte compilation.

The independent C oracle uses a two-element array of vacant entries or proposer identities. It explores both actors, both slots, and both operation types through repeated reuse. Specific regressions cover same-proposer parallel pending records, both approval orders, self-approval, opposite actor completion, simultaneous strobes, setup ambiguity, raw opposite-strobe intrusion, identity chatter, sampling loss, wrap, and transactional counter exhaustion. A C fixture exercises the real encoder through the real companion. All fixture evidence is synthetic software output, not a physical target measurement.

Native CAD generation requires KiCad 9 and matching pcbnew Python bindings. Run `/usr/bin/python3 tools/generate_hardware.py` and `/usr/bin/python3 tools/check_hardware.py`. The checker runs ERC and DRC, exports a fresh XML netlist, and compares reference, value, footprint identity, pin number, and net assignment against actual PCB pads and the manifest. Passive-only ERC cannot validate the external module's power, programming, or protection. Run `python3 tools/check_release.py` for document length, substantive C line count, attribution, and whitespace checks.

Before fabrication, inspect resistor dimensions, labels, connector polarity, copper, clearance, continuity, and enclosure support. Before powered use, confirm the exact module map, grounds, leakage, thresholds, loading, supply sequencing, and target adapter semantics. Start with both proposals empty. Exercise both actors in both slots, same-actor parallel proposals, reverse completion order, repeated slot reuse, self-approval, duplicate proposal, and orphan approval on a safe fixture. Reestablish the real baseline between terminal cases. Compare independent logic-analyzer traces with the retained evidence.

Programming, ST-LINK routing, watchdog recovery, oscillator calibration, sustained serial load, ESD behavior, enclosure safety, and actual target interoperability remain laboratory gates. Describe this release as a software-build-tested and CAD-checked prototype only when the recorded checks pass, never as production-qualified hardware. Absence of a finding does not establish that unobserved operations were authorized or that two distinct human decision makers participated.
