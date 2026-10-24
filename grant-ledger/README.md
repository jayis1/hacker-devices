# Grant Ledger — Single-Use Authorization Evidence Bench

Author and creator: jayis1. Revision A engineering prototype.
Firmware and application: GPL-2.0-only. Hardware scripts: MIT. Hardware design: CERN-OHL-S-2.0.

This project contains executable target firmware, a Linux companion application, and a routed passive carrier for an external NUCLEO-F401RE development board. It is not an assembled, calibrated, certified, or fabrication-qualified instrument. Software builds and CAD rule checks are separate from physical validation; consult VERIFICATION.md for the exact checks performed.

## Purpose and originality

Grant Ledger investigates a small but important hardware security contract: does every privileged operation consume a fresh authorization grant exactly once? Development targets frequently expose diagnostic GPIOs around request handling, authorization decisions, protected writes, and cancellation. A software log can say that access was approved while a peripheral latch accidentally permits a second operation. This instrument observes the four corresponding low-speed logic signals independently, maintains a conservative single-credit transaction model, and exports a bounded evidence snapshot.

The research question is not whether a credential is valid, whether cryptography is strong, or whether an attacker can bypass authentication. It is whether observed request, grant, use, and revoke edges conform to an explicitly documented temporal contract. The observer does not know identities, payloads, passwords, keys, or memory addresses. It neither drives the target nor participates in the authorization decision. An observed deviation is a candidate integration defect requiring corroboration, not automatic proof of exploitable privilege escalation.

Repository inventory was performed before design. The closest existing projects are Lease Watch, which relates recurring heartbeat freshness to enabled outputs; Latch Ledger, which records mechanical tamper contacts; and Gray Audit, which checks quadrature transitions. Grant Ledger instead accounts for single-use authorization credit, consumes that credit on use, rejects overlapping transactions, and gives revocation precedence. Its analog carrier topology and STM32 board support deliberately reuse proven repository patterns authored by jayis1. The transaction policy, wire format, app, and tests are distinct. Originality is claimed relative to the reviewed repository, not as a patent or worldwide prior-art assertion.

## Authorized-use and ethical boundaries

Use only on equipment you own or have explicit written permission to assess. Agree on the exact test points, operating mode, permissible interruptions, and evidence-retention policy before connecting anything. Bench instrumentation can disturb a target through loading, grounding, power sequencing, or operator mistakes even when the firmware is receive-only. Do not attach this design to deployed door locks, vehicle controls, medical equipment, industrial machinery, life-safety equipment, or operational access-control circuits.

Obtain the target owner's approval for diagnostic firmware instrumentation. Use sacrificial development units with actuators and hazardous outputs disconnected. A red-team engagement is not permission to touch every circuit in scope; physical access, electrical modification, and collection of behavioral telemetry need explicit authorization. Do not use the device for covert collection. Keep the board visible, label the USB cable, and stop if the target behaves unexpectedly.

Snapshots reveal activity counts and timing even without credentials. Store them privately, redact device or customer identifiers from report filenames, and retain only what the agreed investigation requires. The application never uploads evidence. Export is an explicit operator action, and files are created exclusively with private permissions. Those choices reduce accidental disclosure but do not protect a compromised workstation, an existing hostile parent directory, or an administrator reading the files.

## Attack surface and threat model

The trusted setup consists of the operator, the reviewed firmware image, the physical wiring, and an authorized workstation. Potentially untrusted elements include target signals, malformed serial traffic, replay files, and a target that intentionally misrepresents its own internal authorization events. The trust boundary from the target to the instrument is electrical as well as logical. Incorrect voltage can damage the board; misleading GPIO instrumentation can produce perfectly consistent but false evidence.

The four input pins are never configured as outputs. USART2 transmits only toward the development board's ST-LINK virtual COM interface; there is no UART receiver, command parser, wireless radio, pairing service, remote control, network stack, storage-card interface, or automatic update feature. The companion opens the serial port read-only. Reconnection does not arm any active function because none exists. Startup, reset, watchdog recovery, and host disappearance cannot intentionally drive a target signal.

A compromised app can forge exported results, but cannot issue firmware commands over this protocol. CRC protects accidental corruption, not authenticity. A malicious USB device can impersonate the stream. No cryptographic identity or signed measurement claim is made. Physical possession and independently verified wiring remain necessary. The external ST-LINK programming interface is trusted and physically controlled; this release has no signed bootloader, anti-rollback enforcement, or secure storage. Do not place it on an untrusted network expecting authenticated updates.

## Hardware specifications and interface contract

The processor is the STM32F401RE on a separately purchased NUCLEO-F401RE board: Cortex-M4, 512 KiB flash, 96 KiB SRAM. Firmware runs at the reset-default 16 MHz HSI clock, uses SysTick for a nominal one-millisecond software tick, and emits 115200 baud, eight-bit, no-parity, one-stop-bit ASCII over USART2 TX on PA2. ST-LINK converts that serial output to USB. The development board supplies its own regulator, clock-support circuitry, decoupling, debug interface, and USB connector; none are silently implied to exist on the carrier PCB.

The carrier is an 80 by 70 mm, two-layer, 1.6 mm PCB with through-hole 2.54 mm headers and axial resistors. It has no MCU, regulator, battery, charger, radio, FPGA, analog sensor, active buffer, optocoupler, or isolation barrier. Its purpose is to make the low-speed input harness explicit and inspectable. Use an insulating bench support or enclosure; no mounting holes or enclosure CAD are supplied. It connects by short labeled jumper wires rather than mechanically stacking on the Nucleo headers.

| Connection | Carrier reference | Nucleo point | MCU | Meaning |
|---|---|---|---|---|
| REQUEST | J3 pin 1, via R2 to J2 pin 1 | A0 | PA0 | New authorization request rising edge |
| GRANT | J4 pin 1, via R4 to J2 pin 2 | A1 | PA1 | Approval rising edge |
| USE | J5 pin 1, via R6 to J2 pin 3 | A2 | PA4 | Protected operation rising edge |
| REVOKE | J6 pin 1, via R8 to J2 pin 4 | A3 | PB0 | Active-high cancellation level |
| Ground | J1 both pins; J3–J6 pin 2 | GND | GND | Shared signal reference |

Each input has 1 kΩ series resistance and a 100 kΩ pull-down on the MCU side. R1, R3, R5, and R7 are the pull-downs; R2, R4, R6, and R8 are the series resistors. Use 1% quarter-watt axial parts with 7.62 mm lead spacing, five two-pin headers, and one four-pin header. The exact footprint identifiers, values, pad numbers, and nets are in kicad/connectivity.json and are compared against a fresh schematic netlist by the checker.

Restrict the target to common-ground 3.3 V CMOS outputs with verified logic thresholds and drive capability. Do not use 5 V TTL, negative-voltage serial, automotive wiring, mains wiring, differential buses, high-energy sources, or raw relay coils. There is no level translation, ESD suppressor, overvoltage clamp designed for abuse, fuse, or galvanic isolation. Series resistance is not a safety barrier. The pull-down provides an idle tendency but cannot distinguish a disconnected cable from a real low signal.

Power the Nucleo through its intended ST-LINK USB connector using the manufacturer's normal configuration. Do not connect target power to J1 or J2. Check the board revision, STM32 pin mapping, virtual-COM solder bridges, and Arduino-header orientation against ST UM1724 before assembly. Ground both systems before attaching signals, power the observer before enabling target outputs, and remove signal connections before powering the observer down. Otherwise input protection structures can back-power the MCU. Do not assume the series resistor makes that sequence harmless.

At a nominal 3.3 V high, each resistor path draws about 33 microamps and the MCU-side voltage is slightly below the target output. These are design estimates; actual input leakage, target impedance, cable capacitance, and threshold margin must be measured. The passive carrier takes no separate supply. Reserve a conservative 500 mA USB source budget for the development-board assembly, but measure actual consumption and follow its USB power limits rather than claiming this is a measured current draw.

## Architecture and block diagram

```text
Authorized development target               Passive carrier A
REQUEST  --> J3 -- 1k --+--> J2.1 --> A0 / PA0
GRANT    --> J4 -- 1k --+--> J2.2 --> A1 / PA1
USE      --> J5 -- 1k --+--> J2.3 --> A2 / PA4
REVOKE   --> J6 -- 1k --+--> J2.4 --> A3 / PB0
                       | each
                     100k
                       |
Ground ----------------+--> J1 ----> Nucleo GND

GPIO reads -> whole-vector qualification -> single-credit state machine
           -> bounded event ring + priority status snapshots
           -> USART2 TX -> ST-LINK USB COM -> Linux console
                                               | overview / events / health
                                               + explicit private JSON export
```

The GPIOA inputs are read together; PB0 is read subsequently. That inter-port skew is unmeasured. This is a low-speed logical contract checker, not a high-speed logic analyzer. Require stable vectors for at least 10 ms in initial experiments and use a scope to establish actual timing. Fast events can be missed or appear tied. A lack of recorded faults does not prove that unobserved transactions were correct.

## Firmware policy and design decisions

The production firmware is C11 with explicit startup vectors, linker memory layout, board definitions, peripheral registers, polling UART driver, transaction engine, and bounded wire encoder. It requires no downloaded SDK. Register definitions are limited to the STM32F401 peripherals used here; consult ST RM0368 and the STM32F401xD/xE datasheet before substitutions. A different MCU, clock initialization routine, bootloader vector offset, or Nucleo revision requires review rather than an assumed compatible build.

All four raw inputs form one vector. Three consecutive observations at successive millisecond ticks qualify that vector. Repeated calls in the same tick do not advance qualification, and a changed candidate restarts the count. Whole-vector qualification deliberately trades throughput for conservative evidence: one chattering signal can suppress recognition of the others. The policy is unsuitable for high-rate requests or independently overlapping protocols. It is intended for explicitly paced laboratory diagnostics.

At startup and after every sampling gap, the engine is unsynchronized and has no credit. Three observed all-low samples establish synchronization. Initial high signals never become invented rising edges. If the loop skips a tick, the engine increments gap and missed-sample counters, cancels its transaction, restarts qualification, and requires an all-low recovery baseline. An uncertain interval is not silently stitched into a valid authorization chain. The last qualified levels remain visible until a new vector qualifies.

The three states are IDLE, PENDING, and CREDIT. A qualified request edge in IDLE enters PENDING. A later qualified grant edge in PENDING enters CREDIT. A later use edge in CREDIT increments the accepted count and consumes the credit immediately, returning to IDLE. Holding USE high does not repeatedly consume credit; a new use requires a qualified low followed by another high. One outstanding request and one credit are supported. The request and grant signals need not remain asserted after their edges.

Each PENDING or CREDIT phase has its own 1000 ms window. Transitioning to CREDIT starts a fresh window, rather than preserving the request timer. Expiration occurs before processing edges at the boundary, so a grant or use qualified exactly at expiry cannot rescue the old phase. Counters report expiration separately from any orphan-grant or no-credit fault produced by that same observation. Window age uses saturating accumulation rather than an indefinitely old absolute timestamp.

A second request while PENDING or CREDIT records overlap and cancels the transaction; it does not silently replace the old request. A grant outside PENDING records an orphan and cancels any outstanding credit. A use without CREDIT records no-credit. Two or more simultaneous qualified rising edges among REQUEST, GRANT, and USE are classified as ambiguous order and cancel the transaction. They are never interpreted as a convenient valid sequence. These rules intentionally favor visible false positives over fabricated causal proof.

REVOKE is active high and has highest priority. While qualified high, it cancels credit and blocks requests and grants. A use edge in that condition records revoked-use. A simultaneous revoke and use therefore cannot count as accepted. Releasing revoke does not restore old credit: the target must issue a new request and grant. Revocations count qualified rising edges, not milliseconds. Fault counters are cumulative; the instrument has no command to erase inconvenient evidence during a run.

All ordinary counters saturate at unsigned 32-bit maximum. Event sequence numbers and device tick values intentionally wrap. The event ring contains 32 entries, drops new records when full, and increments a dropped counter; sequence allocation still advances for a dropped event. Snapshot transmission has priority every 100 ms. UART transmission owns a bounded buffer, never waits for hardware readiness, and abandons a stuck transfer after 100 ms. That failure increments tx_fail. The next packet carries its own full marker and checksum so the app can recover from an unfinished prior line.

The independent watchdog uses the internal low-speed oscillator and a nominal approximately two-second timeout. Startup waits for oscillator and register-update readiness are bounded by the SysTick timebase. If watchdog startup fails the firmware remains silent. These are software design choices, not measured watchdog or oscillator performance. A stalled tick or physical clock fault needs hardware testing. Every reset clears volatile counters and queues; there is no flash journal or persistent session identifier.

## Companion application and protocol

The companion is a dependency-free Linux terminal application using curses, termios, and select. Its overview shows transaction state, synchronization, qualified inputs, request/grant/accepted totals, window age, revocations, and six fault totals. Its event page shows the most recent bounded history. Its health page shows parser errors, boot sessions, connection state, sampling loss, event loss, and transmitter failure counts. Press 1, 2, or 3 to switch screens, e to export, and q to quit.

From this directory, run `python3 app/ledger.py --port /dev/ttyACM0 --export assessment.json`. Choose the serial device explicitly; there is no automatic device discovery or authenticated identity handshake. Permission denial and opening failures terminate with a clear error. A disconnected stream leaves evidence visible but not live. Restart the application to reconnect; there is no automatic reconnection that might silently select a different device. USB serial control-line behavior is host-dependent, so confirm opening the port does not reset the target setup unexpectedly.

Frames are `GL1,type,decimal-fields*CCCC` plus LF. CRC-16/CCITT-FALSE covers every byte before the asterisk, using polynomial 0x1021 and initial value 0xffff; CCCC is four uppercase hexadecimal digits. B has reset flags, window milliseconds, and qualification samples. E has sequence, device time, kind, and value. Event kinds are levels 1, fault 2, state 3, sampling gap 4, accepted count 5, and synchronization 6. Fault values are no-credit 0, orphan 1, overlap 2, ambiguity 3, revoked-use 4, and expiry 5.

S has nineteen unsigned decimal fields: time, levels, state, synchronized flag, age, requests, grants, accepted, revoked, gaps, missed samples, dropped events, transmit failures, followed by the six fault counters in the order above. Levels use bit zero REQUEST, bit one GRANT, bit two USE, and bit three REVOKE. The parser enforces version, field counts, unsigned bounds, enum bounds, and supported boot policy. Its receive buffer is bounded even if a hostile sender never emits a newline.

Only valid status records refresh the live indicator, and freshness expires after one second without one. Replay is always labeled not live. A boot packet clears the current displayed session. A backwards modular status timestamp suggests an unannounced reset, freezes status acceptance, and requires reconnect or a boot record. Queued event timestamps do not trigger this heuristic. A reset whose timestamps happen to move forward is indistinguishable without a boot record; no claim of reliable device identity or complete restart detection is made.

For saved synthetic or authorized capture data, run `python3 app/ledger.py --replay capture.txt --headless --export replay.json`. Replay input is limited to 16 MiB. Exports include author, source, freshness, connection state, receipt age, reset suspicion, status, boot metadata, parse-error count, and up to 256 events. They are snapshots, not complete immutable audit logs. Existing destinations are refused, and new files are mode 0600. The example fixture is generated by the real C engine and encoder and is explicitly synthetic, not physical measurement evidence.

## Build and verification workflow

Counter interpretation matters: requests and grants count policy-processed qualified edges, not every physical pulse. Edges ignored while REVOKE is high, ambiguous tied edges, and activity during unsynchronized acquisition do not increment those totals. Inspect level events and external traces when comparing traffic volume. A CRC-valid boot packet declaring an unsupported window or qualification policy latches an incompatible-policy warning, prevents LIVE status, and freezes new status/event acceptance until a supported boot packet arrives. If no boot packet is observed, policy compatibility is assumed from GL1 rather than authenticated.

Run `make -C firmware all test` with ARM GNU GCC, native GCC, and Make installed. The target build links for Cortex-M4 with warnings treated as errors and produces build/grant-ledger.elf and build/grant-ledger.bin. Program at 0x08000000 through your authorized ST-LINK workflow, then perform a hardware reset. Do not run a debugger clock script that changes the reset HSI contract before entry. Programming and real board startup remain physical acceptance steps.

Run `python3 -m unittest discover -s tests -p test_app.py` and `python3 tests/integration.py`. Tests exercise the real transaction engine and encoder, malformed app input, truncated-line recovery, private export, replay labeling, serial pseudo-terminal transport, actual curses navigation, disconnect, and exit. The UART test maps a Linux memory page to model registers; this tests driver control flow, not silicon timing. Pseudo-terminals similarly test host plumbing, not USB electrical performance.

Generate CAD with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9 bindings, then run `/usr/bin/python3 tools/check_hardware.py`. The checker runs ERC and DRC with violations treated as failures, exports a fresh XML schematic netlist, and checks every component value, footprint identity, pad, pin, and net against the manifest. Passive-symbol ERC cannot verify external MCU power, voltage thresholds, harness orientation, or insulation. Inspect the schematic and layout visually before ordering, and measure an assembled prototype before treating this as a reliable instrument.

## Practical use cases and physical acceptance

A product security researcher can instrument an owned controller's request, decision, protected-write, and cancel paths, then compare normal and deliberately rejected workflows. The target itself creates the test activity through its normal laboratory interface. Grant Ledger merely observes. A duplicated protected operation after one grant should appear as a second use without credit, provided all edges meet the sampling contract. Verify the result against a logic analyzer and source-level trace before filing a security finding.

A red team working in an approved development lab can test whether cancellation invalidates a pending privilege decision. Record the target revision, harness map, test inputs, and expected timing before running the scenario. A penetration tester can include the exported counters as supporting evidence while clearly distinguishing observations from causality and exploitability. An engineering regression team can repeat the same paced scenario after a fix without retaining sensitive application payloads.

Before physical use, verify resistor values, continuity, shorts, connector orientation, ground potential, input loading, threshold margins, cable capacitance, startup pin modes, actual sample timing, watchdog reset, serial behavior, and power sequencing on a sacrificial target. Test deliberate cable removal, host disconnect, slow and simultaneous edges, stopped target activity, and observer reset. Fabrication, assembly, programming, electrical bring-up, ESD, EMC, regulatory compliance, and calibration remain unverified. Passing code and CAD checks does not waive any of those measurements.
