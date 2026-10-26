# Starvation Window — Contended Security-Resource Fairness Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol SW1. Firmware and companion application: GPL-2.0-only. Hardware generation and checking utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, a functional Linux desktop application, and routed native KiCad files. Physical fabrication, programming, electrical qualification, timing calibration, and operational deployment have not been performed. This instrument reports sampled diagnostic evidence; it is not an isolation barrier, protective interlock, security certification, or substitute for independent measurement.

## Purpose and practical distinction

Starvation Window investigates a distinct availability failure in shared-resource security: an arbiter can preserve perfect mutual exclusion while indefinitely denying one security domain access. An application domain may repeatedly reacquire a shared diagnostic, storage, or maintenance resource while a management domain remains continuously pending. The instrument measures each pending domain's observed waiting age and counts peer service entries that bypass it. A fixed, declared policy identifies a hundred nominal milliseconds without service or three peer bypasses as a starvation episode. It does not generate the workload or change arbitration.

A useful deployment is an owned evaluation fixture that shares a maintenance resource between an application domain and a recovery domain. The fixture exports four low-speed, 3.3 V diagnostic indications. Starvation Window distinguishes continuous denial with no progress anywhere from repeated service of the competing domain. This is useful when a low-priority tenant can prevent audit, recovery, or administrative progress without ever violating an access-control decision. The operator compares a suspect scheduler against a repaired scheduler with the same workload. A request is only a declared policy input, not proof of authorization. Secondary overlap and dead-time observations qualify whether apparent service respected mutual exclusion.

The repository inventory and closest README purposes were checked before publication. Handoff Audit already measures break-before-make command/feedback transitions, so that concept was rejected as a primary invention. Coincidence Guard evaluates bilateral consent; Quorum Witness compares majority permission; Revocation Drain observes retirement after activity stops; Dwell Budget aggregates privilege exposure. None of those measures per-requester bounded waiting and repeated competing service while demand stays pending. Starvation Window's primary contribution is this fairness evidence workflow, not its secondary switching checks. The passive carrier and low-level MCU transport intentionally reuse existing jayis1-authored designs. Novelty is repository-relative, not a worldwide prior-art or patentability claim.

## Authorized use and threat model

Use this design only on equipment you own or have explicit written permission to assess. Record the equipment owner, permitted diagnostic interfaces, allowed connection windows, restoration steps, and evidence-retention policy before connecting anything. Do not attach this prototype to operational locks, vehicles, medical equipment, emergency systems, mains wiring, public networks, or safety-critical machinery. A nominally passive connection can still load a circuit, create a common-ground path, or inject current into an unpowered semiconductor. Written authorization does not remove electrical hazards.

The primary threat is defective or adversarial arbitration that repeatedly favors one requester and prevents a waiting security function from progressing. The secondary threats are simultaneous enable, insufficient neutral time, and enable without a request. A malicious target may falsify diagnostic outputs, omit transitions, or keep the evidence interface quiet. Therefore no clean recording proves fairness or confidentiality. Map enable indications as close as possible to real service admission and inspect the adapter implementation. Record whether an indication describes a software variable, peripheral status bit, output latch, or physical gate. A long service legitimately permitted by the target's policy may trigger this instrument's deliberately strict default threshold; review that policy rather than labeling every finding an exploitable denial of service.

The instrument's workstation is another trust boundary. A compromised companion can mislabel or delete evidence, and an attacker with physical access can reflash the MCU. CRC detects ordinary transmission corruption, not deliberate fabrication. There is no authenticated attestation, signed capture chain, trusted clock, secure element, remote update channel, password store, network listener, cloud service, or credential collection. Captured values are local counters and sampled logic states. Treat exported JSON as untrusted input if another program later consumes it; this application never evaluates recorded text as code.

## Hardware specification

The compute module is an ST NUCLEO-F401RE with STM32F401RET6, Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware uses the reset 16 MHz internal HSI oscillator with no PLL and nominal one-millisecond SysTick interrupts. This is a development-board carrier, not a custom bare-MCU board. The module supplies its own regulator, decoupling, reset circuit, SWD access, and ST-LINK virtual COM interface. Those circuits are not falsely represented as additional carrier components.

The carrier contains fourteen fitted components: six 2.54 mm through-hole headers and eight axial resistors. Each diagnostic channel has a 1 kΩ series resistor between its external input and MCU-side node, plus a 100 kΩ pull-down from the MCU-side node to ground. Use 1% quarter-watt resistors in DIN0207, 7.62 mm pitch footprints. The PCB is two-layer, 1.6 mm thick, with an 80 by 70 mm outline, 0.3 mm tracks, 0.2 mm nominal clearance, and 0.8 mm through vias with 0.4 mm drills. No mounting holes are provided; use insulated enclosure supports and prevent the underside from contacting conductive surfaces.

There are no radios, antennas, batteries, battery chargers, storage cards, analog sensors, output relays, or target power supplies. Power only the Nucleo through its supported USB/ST-LINK arrangement after checking the exact board revision. Never power the carrier from the target. J1 carries ground on both pins; it is not a supply connector. The hardware has no galvanic isolation, voltage conversion, active input buffer, dedicated ESD clamp, or overvoltage tolerance guarantee. The carrier does not provide electrical isolation between observed domains.

Only short, common-ground, push-pull 0–3.3 V diagnostic connections are permitted. Do not connect 5 V, bipolar, differential, open-drain without a separately reviewed pull-up, high-speed memory buses, or unknown-voltage signals. The series resistor limits incidental current but is not a protection circuit. A high signal draws approximately 33 microamps through its pull-down; input capacitance and harness capacitance also load the source. Check logic thresholds, loading, ground potential, and power sequencing before connection. Energized targets must not drive an unpowered observer. Disconnect the harness before removing observer power.

### Authoritative interface mapping

| Carrier | Meaning | MCU pin | Nucleo Arduino header |
|---|---|---|---|
| J3 pin 1 | REQUEST_A | PA0 | A0 |
| J4 pin 1 | REQUEST_B | PA1 | A1 |
| J5 pin 1 | ENABLE_A | PA4 | A2 |
| J6 pin 1 | ENABLE_B | PB0 | A3 |
| J3–J6 pin 2 | Signal return | GND | GND |
| J2 pins 1–4 | MCU-side signals in the order above | PA0, PA1, PA4, PB0 | A0–A3 |
| J1 pins 1–2 | Ground only | GND | GND |

J2 connects through a short keyed or carefully labeled harness to the module's A0–A3 header positions. Check continuity rather than relying on wire color. These are logical adapter connections, not a mechanically stackable Arduino shield footprint. USART2 TX uses PA2 through the Nucleo ST-LINK virtual COM route; the target never sees UART output. PA3 is not enabled as UART RX. Follow ST's UM1724 for the exact board revision and solder-bridge configuration. Consult the STM32F401xD/xE datasheet and RM0368 when inspecting register definitions and voltage limits.

## Architecture and data flow

```text
Owned fixture: request A / request B / actual enable A / actual enable B
        | four 0–3.3 V diagnostic signals, common ground
        v
J3–J6 -> 1k series paths -> MCU-side nodes -> J2 -> NUCLEO-F401RE
                              |                       |
                         100k pull-downs          1 ms sampling
                              |                       |
                             GND             fairness + ownership observers
                                                      |
                                            bounded CRC ASCII frames
                                                      |
                                            PA2 -> ST-LINK VCP -> USB
                                                      |
                                          Linux Tk companion application
                                          fairness / snapshots / integrity
                                                      |
                                          explicit private JSON export
```

GPIOA and GPIOB are read consecutively, not simultaneously. ENABLE_B resides on another port, so a transition near a sample boundary may be ordered differently from its physical occurrence. No debounce filter hides a sampled fault. Inputs changing within the same millisecond are treated as one sampled vector; changes entirely between samples can be missed. Use diagnostic signals held for at least several milliseconds and confirm the relationship to actual resource gating with an independent logic analyzer. Do not interpret the histogram as nanosecond switching performance.

## Firmware measurement rules

Fairness is evaluated independently for A and B on every consecutive sample, even while the secondary switching observer awaits baseline. A domain is waiting when its request is high and its own enable is low. The first waiting sample has age zero. Every further consecutive waiting sample adds one nominal millisecond. Request withdrawal or own enable assertion clears that episode's age, bypass counter and reported latch. A hundred complete observed waiting intervals sets the side's sticky starvation bit and increments its lifetime starvation episode counter once. Reaching the threshold and receiving service in the same sample does not prove continued denial at the deadline and does not trigger the duration finding.

A service entry is a sampled enable rising edge with that side's request also high. Held enable is not repeated service. While the other side was waiting in both the previous and current samples, that entry increments the waiting side's bypass count. The third bypass triggers the same episode finding, potentially before the duration threshold. An episode that first crosses the bypass budget and later crosses the time budget is counted once. A request arriving simultaneously with peer service is not counted as already bypassed. A requester that receives even one sampled enable ends its current denial episode; this instrument cannot determine whether that service was useful, complete, or maliciously short.

Waiting starts from observable evidence, not an inferred enqueue time before attachment. At a missing sample, both current waiting ages, bypass counts and reported latches reset, while lifetime service and starvation counts remain. If demand remains asserted, a fresh segment can produce another finding for what physically was one uninterrupted denial. Both domains can be starved together during complete scheduler inactivity; a held peer grant advances the denied side's waiting age but not its bypass count. This separates elapsed denial from repeated favoring of a competing requester. Service and starvation counters saturate into terminal fault instead of wrapping. The field names and secondary observer API retain an internal `isolation` name, but the fairness algorithm is part of the production sampling path, not an app-only derived estimate.

At startup the observer enters BASELINE. It requires twenty consecutive nominal millisecond intervals with both enables sampled low before entering READY. Request indications do not delay baseline qualification. Attaching while an enable is already high does not invent a previous owner or a switch. However, observed overlaps and unrequested enables are still counted during baseline, so waiting for a clean starting point cannot silently hide obvious contradictions.

In READY, the last sole owner is remembered across a neutral interval. The first both-low sample starts quiet time at zero. Each subsequent consecutive both-low sample increments quiet time. When the opposite sole enable appears, the firmware records the quiet age from the preceding sample. A direct A-to-B or B-to-A transition records zero. An incoming opposite owner after nineteen complete quiet intervals is early; after twenty it satisfies the nominal policy. This conservative convention avoids claiming that an unobserved fraction of the arrival interval was also neutral. Re-enabling the same owner is not a cross-domain switch and does not enter the histogram.

The four histogram buckets represent zero, one through nineteen, twenty through ninety-nine, and at least one hundred nominal milliseconds. The switch count equals their sum. The early-switch count equals the first two buckets. Minimum dead time starts at the unsigned sentinel 4294967295, meaning no cross-domain switch has yet been measured. Quiet age saturates below that sentinel, so a genuine long interval cannot be confused with absence of observations. Minimum and histogram counts describe observed transitions, not every physical operation performed by the fixture.

Both enables high sets a sticky overlap finding. Entry into a sampled overlap episode increments overlaps once; subsequent consecutive overlapping samples increment overlap duration. Only intervals bracketed by two overlap samples contribute to duration. Leaving overlap does not add a speculative trailing interval. Overlap destroys the unique predecessor and returns ownership analysis to BASELINE. A fresh twenty-interval both-low baseline is required before new switch measurements. The lifetime overlap counters remain intact.

Each enabled domain whose own request is low contributes an independent contradiction episode. If both requests disappear while both enables remain high, the unrequested count increments twice. Holding the contradiction does not repeatedly increment it. Restoring a request and later removing it can start another episode without changing the enable. This captures stale ownership after a request disappears, but a high request is not sufficient proof that access was legitimately granted by the target's full security policy.

Duplicate timestamps are ignored because main-loop iterations can occur multiple times within one tick. A nonzero tick difference other than one is an acquisition discontinuity. The observer records a gap, clears current predecessor and quiet eligibility, and returns to baseline. It does not interpolate overlap duration through missing time. Unsafe levels at the new segment boundary can start new observed episodes, so one persistent physical fault may be counted again after a gap. Sticky findings and completed aggregate measurements survive. Ordinary 32-bit timestamp wrap with a one-tick difference is supported.

Counter exhaustion is terminal rather than wrapping. Firmware reserves all affected switch histogram counters before updating them so their conservation identity survives. Watchdog initialization failure also enters HALTED before the first telemetry status. HALTED continues reporting time and input levels but stops measurement accumulation. Reboot clears volatile evidence; the application must preserve exported captures externally. Brownout behavior, watchdog timing, fault reset, and oscillator accuracy require physical tests before deployment.

## Firmware implementation and protocol

The implementation uses no heap allocation and no blocking serial transmit loop. `drivers/board.c` configures input pins, reset-cause capture, SysTick, USART2 transmission, and the independent watchdog using the STM32 register map. `drivers/serial.c` owns a bounded transmit buffer, sends at most one available character per service call, and abandons a stalled frame after one hundred milliseconds. `main.c` samples before transport servicing. `isolation.c` implements the observer and `wire.c` encodes bounded telemetry.

Wire records are ASCII `SW1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial 0xffff, no reflection, no final XOR, four uppercase hexadecimal digits. It covers bytes before the asterisk. B contains the MCU reset-register snapshot once after boot. P contains `20,1,100,3`: secondary neutral interval, nominal sample period, waiting deadline, and bypass threshold. P repeats approximately every second so an application attaching late can establish compatibility without confusing a repeated policy announcement with a reboot.

S status fields are, in order: time, state, levels, quiet, last_owner, switches, early, overlaps, overlap_ms, unrequested, gaps, tx_fail, faults, minimum, h0, h1, h2, h3, wait_a, wait_b, bypass_a, bypass_b, starved_a, starved_b, services_a, services_b. States zero, one, and two describe the secondary switching observer's BASELINE, READY, and global HALTED state. Fairness runs in either nonterminal state. Levels bits zero through three are REQUEST_A, REQUEST_B, ENABLE_A, ENABLE_B. Last owner is zero for unknown, one for A, and two for B. Fault bits are overlap 1, early switch 2, unrequested enable 4, acquisition gap 8, transmit loss 16, unavailable watchdog 32, exhaustion 64, starved A 128, and starved B 256. Status is offered approximately every hundred milliseconds; these are snapshots, not a lossless raw edge trace.

## Companion application and evidence handling

Run `python3 app/console.py` on Linux with Tk available. No third-party Python packages, mobile SDK, network service, or package-install scripts are required. The desktop application is appropriate for a stationary wired bench and exposes three real screens. Fairness prioritizes waiting ages, bypass counters, starvation episodes and service entries, followed by secondary levels, state, dead time, histogram and sticky findings. Snapshots displays up to 256 accepted statuses. Integrity shows source provenance, parser errors, compatibility, connection status, reset register, and frozen-evidence reason.

Discover enumerates conventional Linux USB serial paths. Choose the exact device, confirm written authorization, and select New live. Opening the port establishes 115200 8N1 and flushes old queued input; it does not reset the MCU or send commands. Disconnect closes the transport but preserves source and accepted evidence. A new live session replaces retained history, so export important evidence before reconnecting. The application has no command execution, active probing, or target configuration feature.

Replay reads a regular file capped at one MiB; FIFOs and devices are rejected after a nonblocking open and file-type check. Empty recordings and truncated trailing records are rejected. CRC errors increment the error counter without being treated as valid snapshots. A partial record followed by a new protocol marker is resynchronized rather than consuming the valid record. The app requires the compatible policy before accepting statuses, verifies histogram conservation, checks bounds, and rejects contradictory record relationships.

Duplicate identical timestamps do not refresh freshness. Counter rollback, time regression, disappearing sticky findings, increasing minimum, impossible overlap growth, and a terminal state disappearing freeze the retained evidence. Explicit boot records after measurements also freeze it. These checks are heuristics, not authenticated reboot detection: a restart with matching or forward counters and no captured boot record can be indistinguishable from uninterrupted operation. No persistent boot identity is claimed.

Live evidence becomes stale without fresh status. A prolonged reception pause or host event-loop pause disconnects and freezes the session instead of presenting an operating-system backlog as fresh measurements. Reconnect explicitly to flush that backlog and begin a new host session. The MCU's aggregates remain since its last reset, so late attachment is not the beginning of a new hardware measurement period. JSON export uses exclusive creation with private mode 0600 and refuses to overwrite an existing file. It includes provenance, parser errors, freshness, frozen reason, compatibility and retained rows, including metadata when no row was accepted. No automatic capture files are created.

## Research workflows and limits

For the primary fairness experiment, hold recovery-domain demand high in an owned arbiter fixture while scheduling repeated application-domain requests. First verify that the pending recovery requester eventually receives service below the declared limits. Then configure the supported lab scheduler to favor the application requester for three new service entries while recovery remains pending. Compare the bypass counter and starvation finding with a logic-analyzer reference. Repeat with a single held application grant to exercise elapsed denial without repeated bypasses, and with neither domain served to demonstrate complete stall. Reverse the domains and repeat. These cases expose a class of availability failures that a mutually exclusive, correctly ordered handoff test alone cannot detect.

For product-security testing, attach the observer to an owned dual-domain arbiter fixture. First establish an all-low baseline. Exercise A ownership, a comfortably long neutral interval, and B ownership. Compare the reported histogram to an independent logic analyzer. Then intentionally configure the fixture's supported test harness to omit the neutral interval, overlap the enables, and withdraw a request while its enable remains asserted. The observer should separate these findings instead of conflating a request anomaly with an overlap. Restore the normal fixture configuration before ending the session.

For authorized red-team training, use a nonproduction demonstration resource such as a visible LED bank representing exclusive ownership. Demonstrate that a management API response can disagree with the actual enable contract. Do not use the observer as a vehicle for gaining privileges or bypassing access controls. For penetration-test remediation validation, preserve identical signal mapping, firmware revision, workload and supply conditions across before-and-after captures. A lower fault count without equivalent stimulus is not evidence of a successful repair.

Record instrument firmware, module revision, carrier revision, mapping source, workload, operator authorization and reference measurements with the exported capture. A zero-finding session can still contain missed narrow pulses, untrustworthy diagnostic adapters, acquisition gaps, transport corruption, or observer resets. This instrument provides a focused hypothesis and reproducible measurements for further investigation. It does not establish confidentiality, causality, actual memory isolation, or exploitability by itself.

## Build, checks, and bring-up

Build the target with `make -C firmware all test`. Run application model tests with `python3 -m unittest discover -s tests -p test_app.py` and real Tk/serial pseudo-terminal tests with `xvfb-run -a python3 -m unittest discover -s tests -p test_gui.py`. Generate CAD with `/usr/bin/python3 tools/generate_hardware.py` and validate it with `/usr/bin/python3 tools/check_hardware.py`. The system Python is used for KiCad bindings. Exact exercised tool versions and outcomes belong in VERIFICATION.md, not in unsupported claims of production readiness.

The CAD checker runs electrical and design rule checks, exports a fresh schematic netlist, and compares component reference, value, footprint and every pad-to-net association against both the routed PCB and connectivity manifest. Embedded symbols, real footprint assignments, and actual copper routes are included. The passive schematic cannot validate the external Nucleo power circuitry or an incorrectly wired harness. Before fabrication, inspect the schematic rendering and board orientation independently.

Before connecting a target, inspect assembly, measure every ground and series path, and verify resistor values with the module disconnected. Power and program the Nucleo alone using an approved ST-LINK process, then confirm UART output, all four input bits, watchdog behavior and the nominal timebase. Use a current-limited, known-voltage fixture and independent reference analyzer for the first functional trials. Validate startup with high inputs, ordinary transitions, exact boundary intervals, long idle periods, USB disconnection, reset, power removal and gap recovery. Software tests and CAD checks do not substitute for these physical acceptance gates.

Manufacturer references: https://www.st.com/en/evaluation-tools/nucleo-f401re.html ; UM1724 Nucleo-64 board manual; STM32F401xD/xE datasheet; RM0368 reference manual. Verify the actual purchased board revision against these documents before assembly. All first-party design, firmware, companion software, tests and documentation in this folder are authored by jayis1.
