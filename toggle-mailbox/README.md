# Toggle Mailbox — Two-Phase Bundled-Data Stability Witness

Author and creator: jayis1. Hardware revision A. Protocol TM1. Firmware and companion application: GPL-2.0-only. CAD generation and checking utilities: MIT. Passive carrier design: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype design with target-linked firmware, executable desktop software, and routed native KiCad files. Executed checks are recorded in VERIFICATION.md. No fabrication, MCU programming, timing calibration, physical electrical qualification, or deployment is claimed. This instrument is an observer, not a mailbox controller, isolation barrier, safety interlock, or proof of correct receiver behavior.

## Purpose and practical value

Toggle Mailbox observes a small but consequential integration boundary: a sender publishes a two-bit security-state value, toggles a request phase, and promises to leave that value unchanged until a receiver toggles acknowledgment to the same phase. The next message uses the opposite phase. Unlike a pulse interface, either a rising or falling transition represents a new operation. A held high request is not a new message, and an acknowledgment returning low is just as meaningful as one going high.

The motivating defect is early reuse of a shared control mailbox. A sender can overwrite a maintenance-mode selector or authorization-state code while another component is still consuming the previous value. Logs may show a request and an acknowledgment without recording what happened to the payload between them. A second failure occurs when a sender toggles request again before acknowledgment: the two transitions can cancel as a level, concealing the outstanding operation from a slow receiver. This instrument keeps an external ledger and records the first sampled contradiction rather than silently treating equal levels as recovered synchronization.

Four owner-provided diagnostic outputs carry DATA0, DATA1, REQ, and ACK. The instrument checks observed setup before request, raw and qualified payload stability while outstanding, request overtaking, unsolicited acknowledgment, ambiguous simultaneous handshake transitions, and a bounded acknowledgment deadline. It retains completed and abandoned counts separately. The payload is an operator-defined four-state diagnostic category, not a mechanism for exporting credentials or application contents. No command is transmitted to the target.

## Originality relative to this repository

Existing device directories and README descriptions were inventoried before implementation, with a repository-wide search for toggle, two-phase, bundled-data, and mailbox contracts. The closest detailed contracts, Parity Latch and Receipt Witness, were read in full. Parity Latch measures a parity-coded word around a rising strobe and a fixed hold interval. It has no acknowledgment phase and cannot establish a variable, receiver-terminated hold obligation. Receipt Witness correlates rising action, append, and durability pulses through a FIFO; it has no parallel payload and does not interpret both handshake polarities.

Barrier Mask aggregates distinct participant completions before collective release. That is also different from a single outstanding, phase-encoded transfer whose data must remain unchanged. Toggle Mailbox deliberately reuses jayis1's passive carrier geometry, register definitions, startup, UART driver, scheduler pattern, and desktop serial transport. The new contribution is the two-phase mailbox policy, payload lifetime, conservative correlation-loss behavior, evidence contract, and corresponding tests. This is repository-relative originality, not a claim to have invented asynchronous handshaking or established worldwide patent novelty.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Approval should identify the development image, diagnostic adapter, attachment points, permissible workloads, electrical reference, operator, stop procedure, and evidence-retention policy. Begin with an isolated laboratory fixture whose hazardous outputs are disconnected. Never attach this unisolated prototype to occupied door controls, vehicles, medical devices, mains wiring, unknown industrial signals, or machinery whose interruption can cause injury.

The measured attack surface is the trust boundary between sender publication and receiver acknowledgment. Relevant defects include prematurely recycling a shared payload, a stale acknowledgment from an earlier operation, a repeated request before retirement, inconsistent reset phases, and an indefinitely pending transfer. An authorized red team can ask the owner to exercise approved concurrency and recovery workloads while recording these diagnostic effects. The instrument itself neither creates those workloads nor exploits the resulting fault.

The target adapter is not automatically trustworthy. It must expose the actual mailbox phases and actual published value, rather than a sanitized log of what software intended to do. A malicious target can lie on all four wires. An acknowledgment does not prove that the receiver used the value, enforced its permissions, or safely copied it. Corroborate important findings with source review, functional testing, and an independent analyzer. Even a flawless diagnostic capture cannot prove the absence of an unobserved race.

The operator, observer firmware, harness, Nucleo module, programming access, and workstation are trusted. Serial input and replay files are untrusted parser inputs. The checksum detects accidental corruption; it does not provide authentication, replay prevention, confidentiality, or signed evidence. There is no secure updater, device certificate, persistent boot identity, encrypted storage, radio, network service, cloud upload, credential capture, or stealth deployment feature.

Receive-only software is not a physical data diode. Different firmware could drive the GPIO pins, and ST-LINK can replace the image. Even with the supplied image, a common-ground connection, input loading, miswiring, or back-power can disturb the target. Stop if attachment changes behavior. Author jayis1 makes no safety, regulatory, production-readiness, or certification claim.

## Hardware specifications and bill of materials

The complete instrument comprises an external ST NUCLEO-F401RE development board, the passive carrier described here, short labelled insulated leads, nonconductive support, and a Linux workstation. The STM32F401RE Cortex-M4 provides 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, one-millisecond SysTick, and USART2 TX at 115200 baud, eight data bits, no parity, one stop bit. PA2 communicates toward the module's ST-LINK virtual serial bridge.

The external Nucleo provides the MCU, regulation, decoupling, reset, SWD programming, and USB serial conversion. These facilities are not represented as fictional carrier components. The carrier has no FPGA, radio, antenna, battery, charging circuit, analog sensor, storage card, active level translator, or target power output. It connects by harness rather than stacking as a mechanically compatible shield.

| References | Quantity | Specification |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | 1x2 vertical 2.54 mm through-hole header |
| J2 | 1 | 1x4 vertical 2.54 mm through-hole header |
| Harness/support | 1 set | Labelled insulated wires, strain relief, insulating base |

The carrier is 80 by 70 mm, nominal 1.6 mm thick, with two copper layers, 0.3 mm tracks, a ground-layer transition, and a closed outline. Fourteen components, thirty electrical pads, and nine named nets appear in the schematic and PCB. The resistor footprint is DIN0207 with 7.62 mm lead spacing. There are no mounting holes or enclosure CAD. Verify actual purchased component dimensions, drill fit, mating access, and mechanical support before fabrication. Supplier availability and pricing are not asserted.

## Pinout, electrical limits, and power

| Function | Target connector | MCU connector | Nucleo label / GPIO | Bit |
| --- | --- | --- | --- | --- |
| DATA0 | J3.1 | J2.1 | A0 / PA0 | 0 |
| DATA1 | J4.1 | J2.2 | A1 / PA1 | 1 |
| REQ | J5.1 | J2.3 | A2 / PA4 | 2 |
| ACK | J6.1 | J2.4 | A3 / PB0 | 3 |
| Ground | J3–J6.2 | J1 both pins | GND | none |

J1 is ground only; neither pin supplies power. J2 contains four signals and no ground pin. Net names ending `_IN` refer to target-side connections; plain names refer to MCU-side nodes after the series resistors. Each MCU-side node has a 100 kOhm pulldown. Check physical pin numbering and harness continuity instead of relying on the apparent orientation of a connector drawing.

Only nominal zero-to-3.3 V common-ground push-pull diagnostic outputs are supported. The nominal resistive path from each target signal to ground is 101 kOhm, excluding leakage and source impedance. The series resistor is not qualified ESD or overvoltage protection. No isolation, reverse-current blocker, clamp network, fuse, or hot-plug guarantee is supplied. Five-volt signals, negative voltages, 1.8 V domains, differential buses, relay coils, and unknown field wiring are out of scope.

Power the Nucleo using its documented ST-LINK USB arrangement. Reserve a conservative 5 V, 500 mA source allowance, subject to actual jumper settings and USB limits; this is a planning budget rather than measured consumption. USB ground reaches the target through the Nucleo and J1. Establish compatible grounds and power the observer before enabling target signals. Remove target signals before depowering the observer, since input protection structures can otherwise back-power the MCU.

The mapping follows the repository's established NUCLEO-F401RE implementation and the manufacturer UM1724/RM0368 references. Verify the purchased module revision, CN8 mapping, reset clock configuration, and ST-LINK USART solder bridges before wiring. Register definitions are explicit and supplied in registers.h. Electrical thresholds, leakage, loading, power sequencing, ground offset, ESD behavior, and timing accuracy remain physical acceptance tasks.

## Architecture

```text
Owned diagnostic adapter       Passive carrier                 External Nucleo
 DATA0 ------------------------ 1k --+------------------------- PA0 / A0
 DATA1 ------------------------ 1k --+------------------------- PA1 / A1
 REQ   ------------------------ 1k --+------------------------- PA4 / A2
 ACK   ------------------------ 1k --+------------------------- PB0 / A3
                                    | each MCU-side node
                                   100k
                                    |
 Common ground -------------------- GND ---------------------- GND / USB

GPIO reads -> independent input qualification -> phase-change recognition
           -> payload setup / outstanding hold / retirement deadline
           -> completed + abandoned + active conservation ledger
           -> bounded TM1 CRC snapshots -> USART2 PA2 -> ST-LINK USB
           -> Mailbox / History / Integrity desktop screens -> private JSON
```

GPIOA is read before GPIOB. The request and acknowledgment are therefore not latched simultaneously by hardware. This is a deliberately slow diagnostic instrument, not a native high-speed asynchronous-bus analyzer or a metastability detector. Start with at least twenty milliseconds of payload setup and clearly separated handshake transitions, then validate the adapter with calibrated equipment. Software qualification does not establish nanosecond setup or hold margins.

## Exact firmware contract

Firmware is C11 with startup vectors, a linker map, register definitions, GPIO/watchdog support, bounded UART service, a measurement engine, and checked serialization. There is no heap, RTOS, filesystem, network stack, flash log, or received-command parser. The image starts at 0x08000000 and reserves stack headroom in its linker assertion. All evidence is volatile and disappears on observer reset.

Each input qualifies independently after its candidate level remains unchanged for three subsequent one-millisecond observations. Thus a change requires the first observation plus three matching intervals. Calls within the same tick are ignored. Unrelated channel chatter does not defer qualification of another input. Short pulses can be rejected or entirely missed; an absent diagnostic event is not proof that no physical event occurred.

Initial readiness requires all four inputs qualified, raw and qualified vectors equal, and REQ equal to ACK for twenty evaluated ticks. Both low/low and high/high are legal initial idle phases. A two-bit value may be established before readiness. Unequal phases at attachment are not invented into a request. This baseline is prospective: it says nothing about transfers before the observer became ready. Arrange target idle deliberately before beginning the campaign.

Within a ready segment every qualified REQ transition increments starts, regardless of direction. Payload setup requires five observed intervals of raw data matching the stable qualified data. The interval ending at the qualified request is included. A simultaneous qualified payload change therefore fails setup. Because request itself is qualified, this is not a measurement referenced to its exact physical edge. At an admitted request, captured stores the two-bit payload and active becomes one.

While active, both raw and qualified payload must equal captured. A single sampled raw excursion records hold failure even if qualification would reject it. Hold includes the qualified acknowledgment endpoint: changing payload in the same observation that completes acknowledgment is conservatively a failure. The captured field remains historical after completion; it does not assert that the current data pins still carry that value.

A qualified ACK transition completes only a previously active transfer. Completion increments completed, clears active and age, and updates max_latency if appropriate. The next request must occur on a later qualified observation, but no return-to-zero pulse sequence is needed. Both handshake polarities are supported indefinitely. An ACK while idle is an orphan. A REQ while active is an overtake; the old and new requests are both abandoned. REQ and ACK changing in one qualified observation are overlap, not an accepted zero-time transfer.

Age increments on each subsequent observed millisecond. Acknowledgment at age 1000 is accepted; age 1001 is timeout. Deadline expiry takes precedence over overlap, overtake, payload hold, and acknowledgment processing. Overlap precedes overtake, which precedes hold, which precedes orphan/setup checks. Consequently one observation can have several underlying defects but records one primary correlation failure. Request and acknowledgment edges on that fault tick are still counted. A newly observed request is pre-accounted as abandoned until it is actually admitted, preventing early-return paths from silently dropping it.

The first correlation failure permanently clears ready for that observer run. Subsequent equal phases do not restore identity. Reset the observer only after deliberately returning the target to a known idle baseline and exporting prior evidence. A sampling jump other than one millisecond records a gap and missing ticks, abandons an active transfer, invalidates qualification, and also prevents automatic rearming. Later gaps may continue to accumulate even after correlation loss. Qualified levels can continue updating, but they do not renew the failed segment.

The invariant is starts = completed + abandoned + active. Completed cannot exceed observed acknowledgments. Maximum latency describes accepted completions only, not the delay of abandoned work. Counters are updated transactionally: on arithmetic exhaustion the speculative update is discarded, the previous active transfer is abandoned, and acquisition enters terminal HALTED with a sticky exhaustion flag. A watchdog startup failure also halts before first status. Time remains modulo 32 bits; transmission-failure counts saturate. HALTED acquisition fields freeze while time and transmission diagnostics may advance.

## Protocol and desktop application

The scheduler samples before UART work. Submission copies one bounded frame, and each UART poll sends at most one ready byte. A blocked transmission is abandoned after one hundred milliseconds and counted. The main loop feeds the independent watchdog when startup succeeded. Boot is announced once, policy initially and approximately every second, and cumulative status approximately every hundred milliseconds. Periodic policy supports late attachment without falsely implying a reboot. This is not a complete per-edge trace.

Frames are ASCII `TM1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection, and no final XOR over bytes before the asterisk. CRC is four uppercase hexadecimal digits. B contains one reset-flags integer. P contains `5,3,20,1000` for setup, qualification, baseline, and deadline. All numeric fields fit unsigned 32-bit values.

S fields in order are `now, levels, valid, ready, halted, faults, captured, active, age, starts, acks, completed, abandoned, setup_fail, hold, overtake, orphan, overlap, timeouts, gaps, missed, max_latency, tx_fail`. Fault bits zero through eight mean insufficient setup, payload changed, request overtake, orphan acknowledgment, overlap, timeout, gap, exhaustion, and watchdog unavailable. Ready is correlation eligibility, not proof of secure application behavior.

Launch `python3 app/console.py` from this directory on Linux with Tk installed. Enter the exact serial path, normally `/dev/ttyACM0`, and select New live. Mailbox displays measurements and named findings. History retains at most 256 snapshots. Integrity displays source, live/replay provenance, connection, compatibility, parse errors, age, reset flags, and any freeze reason. Disconnect retains evidence. A successful new connection or replay replaces the session, so export first. A failed connection preserves the previous model.

LIVE describes progressing compatible telemetry, not necessarily valid acquisition: always inspect ready, halted, and faults separately. It requires a connected session and at least two advancing accepted snapshots within one second. Identical timestamps do not refresh freshness. Contradictory duplicates, backwards time, counter rollback, disappearing sticky findings, and mutations of halted evidence freeze the retained session. Long reception pauses or cumulative host/device progress divergence also freeze evidence rather than pretending queued data is current. These heuristics cannot identify every reset without a persistent boot identifier.

Replay is explicitly non-live. It accepts only regular files up to two MiB, opened nonblocking and checked before allocation; FIFOs and device files are refused. Framing storage is bounded and resynchronizes on the entire TM1 marker. A partial tail at EOF increments parser errors and an empty recording is rejected. The included fixture is generated by the real C encoder but remains synthetic software data, not physical measurement.

Export JSON creates a new private file with mode 0600, rejects overwriting, and refuses a final-path symlink. Reports include author jayis1, provenance, connection, policy, parser errors, reset flags, freshness, freeze reason, latest snapshot, and retained history. Empty exports still preserve session metadata. File permissions are not encryption. Use a trusted destination and the engagement's retention policy; deletion does not guarantee secure erasure.

## Build and verification

Requirements are GNU Make, ARM GNU GCC with newlib/nosys support, native GCC, Python 3 with Tk, KiCad 9 CLI and Python bindings, installed standard KiCad footprints, and Xvfb for desktop automation. No downloaded MCU SDK or third-party Python package is required. Run from this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
```

The target outputs are firmware/build/toggle-mailbox.elf and .bin. Programming requires a supervised ST-LINK workflow and reset into the documented clock state; programming was not performed in this release. Native tests execute the production engine and serializer with sanitizers. UART tests model registers in a Linux process rather than testing physical USART hardware. Scheduler tests cover lost boot transmission, recurring compatibility policy, and watchdog startup failure.

The app tests feed production-C frames through Linux pseudo-terminals and a real Tk event loop, exercise all three screens, inspect private exports, and verify disconnect behavior. Parser tests include checksum-valid impossible ledgers, rollback, duplicates, malformed framing, replay size/type bounds, and backlog. The CAD checker runs ERC and DRC with violations fatal, exports a fresh XML netlist, and compares every reference/pin/net, value, and full footprint identity with PCB pads and the connectivity manifest. The saved netlist is checked too. Passive-symbol ERC does not validate external voltage or power sources.

## Research workflows and remaining physical acceptance

A product-security engineer can map four maintenance states onto the payload, observe normal toggling, and compare the same approved concurrent workload before and after fixing premature reuse. A red team working with the product owner can collect narrow evidence of an integration weakness without reading credentials or taking control of the target. A penetration tester can document whether a vendor's mailbox lifetime guarantee matches its exported diagnostic behavior. Each report should identify the adapter source, firmware images, wiring, initial phases, timing assumptions, and any missing samples.

Before fabrication, review schematic rendering and actual component dimensions. Check continuity and shorts unpowered, mount the carrier on an insulating support, and bring up the Nucleo without a target. Measure ground offset, loading, thresholds, and power sequencing. Verify that target-facing pins remain inputs and test back-power risk. Correlate paced known-good and deliberately malformed fixture sequences with an independent analyzer, including both phase polarities, a raw payload glitch, exact deadline endpoints, and an observer restart while the target is busy.

Physical watchdog recovery, oscillator tolerance, USB behavior with a real module, ESD, EMC, enclosure suitability, environmental limits, and regulatory compliance remain unverified. Manufacturer references are ST UM1724 for Nucleo-64 MB1136 and RM0368 for STM32F401 peripherals; verify editions appropriate to the actual board and silicon. This design supplies reviewable source and executable software evidence, not an assertion that an unbuilt board is safe to deploy.

Original design, firmware, software, and documentation author: jayis1.
