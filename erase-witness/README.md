# Erase Witness — Sanitization-to-Access Sequence Evidence Bench

Author and creator: jayis1. Hardware revision A; protocol EW1.
Firmware and application: GPL-2.0-only. Hardware tooling: MIT. Carrier design: CERN-OHL-S-2.0.

Status: engineering prototype design with target-linked firmware, executable companion application, and checked routed CAD. No physical prototype has been assembled or measured for this release. This instrument observes a diagnostic handshake. It does not read storage contents, execute an erase command, certify data destruction, or prove that a device's diagnostic outputs are truthful.

## Purpose and novelty

Erase Witness addresses a specific product-security question: does a storage appliance keep access disabled until its sanitization controller has visibly entered a busy phase and subsequently reported completion? A software user interface may say that sanitization finished while a different subsystem has already restored access. Observing the externally exposed relationship is useful when reviewing controller integration, reset recovery, service-mode transitions, and changes to an appliance's ownership-transfer workflow.

The device watches four deliberately exported, active-high diagnostic lines: REQUEST, BUSY, DONE, and ACCESS. It checks their qualified order, a minimum observed BUSY dwell, two progress deadlines, and whether ACCESS becomes high before an accepted completion. The application preserves both completion and unsafe-access counts; a completed handshake is not automatically a safe cycle. Firmware never drives the appliance, and the application has no control command interface.

The repository was inventoried by directory and README purpose before selecting this concept. The closest projects are Grant Ledger, which checks single-use authorization; Handoff Audit, which checks mutually exclusive security-domain handoff; and Quorum Witness, which checks majority agreement. This design instead recognizes a multi-stage sanitization lifecycle with a minimum dwell and completion-to-access separation. It intentionally reuses the electrically identical four-input passive carrier, STM32 board support, bounded serial architecture, and terminal application infrastructure authored by jayis1. Reusing those components is not presented as a novel circuit. The distinct contribution is the lifecycle measurement contract and its tests. No worldwide novelty or patentability claim is made.

## Authorized use and threat model

Use only on equipment you own or have explicit written authorization to assess. Obtain approval for the diagnostic firmware, cabling, operator workflow, and evidence retention. Sanitization may destroy valuable information; this instrument does not initiate it, but the separate target test procedure can. Use disposable test media and an isolated development fixture. Never connect to production access systems, life-safety circuits, vehicle controllers, mains wiring, unknown industrial signals, or equipment whose loss of service could harm people.

The trust boundary is between a target's sanitization-state indication and its reported access-enable decision. A faulty implementation might assert DONE without BUSY, keep BUSY active indefinitely, drop REQUEST prematurely, restore ACCESS during work, or retain a stale DONE across operations. The observer flags deviations from the explicit contract below. The target can lie on all four lines, so a correct-looking trace cannot demonstrate real storage erasure. Independently validate the binding between diagnostic pins and the actual access gate. An output called ACCESS that merely mirrors a UI variable is not a trustworthy physical enforcement signal.

The instrument trusts its own firmware, the local host, cabling, and chosen serial port. CRC detects accidental corruption, not deliberate forgery. Neither firmware nor application provides authenticated pairing, cryptographic attestation, signed evidence, persistent boot identity, or replay resistance. There is no network listener, radio, credential collection, storage-bus probing, automatic target discovery, or active exploitation. An adversarial USB device can send fabricated EW1 records; treat the application as a local measurement display, not an authentication authority.

Data minimization is deliberate. Records contain input levels, state, durations, counters, and transport health, not sectors, filenames, keys, passwords, identities, or media contents. Exported source paths can still identify a lab fixture. Keep reports in a private engagement directory, restrict access, and apply the agreed retention period. Removing an evidence file does not guarantee secure deletion on modern storage.

## Hardware specifications and bill of materials

The compute module is an external ST NUCLEO-F401RE, MB1136 family, with STM32F401RET6 in LQFP64. The target image uses Cortex-M4 Thumb instructions with software floating-point ABI, 512 KiB flash, 96 KiB RAM, and the reset internal 16 MHz HSI clock. It needs no RTOS or downloaded SDK. SysTick supplies a nominal one-millisecond time base. USART2 transmits to ST-LINK's USB virtual COM bridge on PA2 at 115200 baud, eight data bits, no parity, and one stop bit. USART receive is disabled.

The supplied PCB is an 80 by 70 mm, two-layer, 1.6 mm FR-4 passive carrier. It is connected by insulated harnesses, not stacked as a pin-compatible shield. There are no radios, FPGA, analog sensors, battery, charger, onboard regulator, storage device, or display. The Nucleo's intended ST-LINK USB connector powers the development board; the host provides the user interface and optional evidence storage. Reserve a conservative 500 mA USB source budget for the development-board assembly, while checking the actual board manual and power configuration. Consumption is not measured here.

| Reference | Quantity | Part specification | Purpose |
|---|---:|---|---|
| J1, J3–J6 | 5 | 1x02 straight 2.54 mm through-hole header | Common ground and target input connections |
| J2 | 1 | 1x04 straight 2.54 mm through-hole header | MCU input harness |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, 0.25 W, axial DIN0207, 7.62 mm pitch | MCU-side pull-down |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, 0.25 W, same footprint | Input series resistance |
| External | 1 | NUCLEO-F401RE development board | MCU, programming, USB serial bridge |
| External | As needed | Insulated harness and nonconductive enclosure | Strain relief and touch protection |

J1 has GND on both pins; neither is a supply input. Pin 2 of each target header J3–J6 is ground. The carrier carries no power rail and cannot power a target. The following table is the cross-layer mapping used in board.h, the hardware generator, firmware input bits, and application labels.

| Signal | Target header pin | Series resistor | Pull-down | J2 pin | Nucleo label / MCU | Bit |
|---|---|---|---|---:|---|---:|
| REQUEST | J3.1 / REQUEST_IN | R2 | R1 | 1 | A0 / PA0 | 0 |
| BUSY | J4.1 / BUSY_IN | R4 | R3 | 2 | A1 / PA1 | 1 |
| DONE | J5.1 / DONE_IN | R6 | R5 | 3 | A2 / PA4 | 2 |
| ACCESS | J6.1 / ACCESS_IN | R8 | R7 | 4 | A3 / PB0 | 3 |

Only known 0–3.3 V logic with a common reference is supported. There is no galvanic isolation, ESD suppression, level conversion, polarity protection, or overvoltage tolerance claim. A series resistor is not a safe interface to higher voltages. Check ground offset and signal levels before connecting. Power the observer before enabling target outputs, and disconnect signals before removing observer power, to avoid input-clamp back-power paths. The pull-down makes a disconnected wire look low; cable disconnection is not independently detected and can mimic an all-low baseline. This limitation matters especially when evaluating reset behavior.

Consult ST UM1724 for the specific Nucleo board revision and ST-LINK serial solder bridges, ST RM0368 for the STM32F401 registers, and the STM32F401xD/xE datasheet for input and supply limits. Confirm Arduino pin labels on the actual board. The external module contains its own supply decoupling and programming circuitry; those are not falsely represented as components on the passive carrier. Mechanical mounting holes and an enclosure drawing are not supplied, so define retention and strain relief before construction.

## Architecture and block diagram

```text
Authorized fixture              Passive carrier                   External Nucleo
REQUEST -> J3.1 -- R2 1k --+---- J2.1 ---------------------------> PA0 / A0
BUSY    -> J4.1 -- R4 1k --+---- J2.2 ---------------------------> PA1 / A1
DONE    -> J5.1 -- R6 1k --+---- J2.3 ---------------------------> PA4 / A2
ACCESS  -> J6.1 -- R8 1k --+---- J2.4 ---------------------------> PB0 / A3
                          | each MCU-side signal
                          R1/R3/R5/R7 100k
                          |
Fixture GND --------------+---- J1 -----------------------------> Nucleo GND

GPIO -> vector qualification -> sanitization lifecycle engine
     -> bounded events + status + policy -> UART TX -> ST-LINK USB
     -> Linux Overview / Events / Health -> private JSON export
```

GPIOA is read before GPIOB, not atomically with it. The design is for paced diagnostic indications rather than high-speed bus edges. Start with every level held for at least ten milliseconds and use comfortably larger timing margins for phase transitions. Three-sample qualification suppresses short noise but can also conceal a short security-relevant pulse. Oscillator tolerance, interrupt latency, inter-port skew, wiring delay, and actual detectable pulse width remain unmeasured. Never use a zero fault count as proof that no short access glitch occurred.

## Firmware contract and design decisions

On reset the engine starts UNKNOWN. Only a qualified all-low vector establishes IDLE. REQUEST must then be observed high while BUSY and DONE remain low. This enters WAIT_BUSY. BUSY must subsequently rise while REQUEST stays high and DONE stays low. The engine enters BUSY and measures its qualified dwell. After at least 100 nominal milliseconds, the permitted completion transition is BUSY low and DONE high with REQUEST still high. That enters COMPLETE. ACCESS may rise only in a later qualified observation retaining REQUEST and DONE high and BUSY low.

REQUEST remains asserted throughout the cycle, including completion and permitted access. To begin another cycle, return every signal to low. A target that uses pulse-based requests or different acknowledgment polarity needs a deliberate diagnostic adapter implemented by the target owner; it cannot simply be wired in and interpreted under this contract. The monitor does not automatically learn a policy from traffic, because a faulty baseline could otherwise become the definition of correct behavior.

Simultaneous qualified REQUEST and BUSY from IDLE is rejected because the request-only stage was not observed. DONE without the required BUSY stage is rejected. BUSY-to-DONE before minimum dwell is rejected. Dropping REQUEST or asserting conflicting phase bits in an active state is rejected unless the entire vector is low, which explicitly aborts WAIT_BUSY or BUSY and returns to IDLE. Unexpected phases move to FAULT. Both FAULT and UNKNOWN require an all-low baseline; a plausible-looking suffix cannot retroactively establish the missing operation prefix.

WAIT_BUSY expires at 500 nominal milliseconds. BUSY expires at 5000 nominal milliseconds. These are demonstration diagnostic-policy limits, not claims about how long a particular storage technology takes to erase. A production device with a longer sanitization operation needs a reviewed policy revision and matching application constants. Deadline evaluation precedes acceptance of the new qualified vector at the same tick, so reaching the deadline exactly is considered expired. Durations advance for the preceding qualified interval, not for an assumed raw edge instant.

ACCESS high without an already accepted completion and a currently matching completion handshake increments unsafe once per continuous observed unsafe episode. ACCESS high in the same qualified vector as completion is counted unsafe even though completed also increments. Consequently completed counts accepted handshake sequences, not wholly fault-free operations. ACCESS remaining high while the handshake drops is also unsafe. Historical counts remain until reset. The application displays these counters separately rather than collapsing them into a misleading green pass indicator.

The qualifier requires three identical whole-input vectors on consecutive distinct millisecond observations. Repeated calls in the same millisecond do not advance qualification. Any bit change restarts qualification for all four signals. Last-qualified levels remain authoritative until another vector qualifies; raw chatter is not separately counted. If the main loop misses a tick, continuity is invalidated, UNKNOWN is entered, and the gap and missed counters increase. The first post-gap sample is discarded for qualification. Three subsequent consecutive samples can qualify levels, but a complete all-low baseline is still required before another cycle is accepted.

Counters and elapsed durations saturate at unsigned 32-bit maximum. Time and event sequence fields wrap modulo 32 bits. A gap may split one physical unsafe episode into two observations; counts are not guaranteed distinct physical incidents. Reset loses all volatile evidence. The independent watchdog is configured for a nominal roughly two-second period subject to internal low-speed oscillator tolerance. Main-loop progress feeds it; initialization readiness waits are bounded. Failure to initialize the watchdog produces no plausible evidence stream. There is no firmware update protocol, persistent configuration, journal, or power-loss recovery ledger.

Every target-facing GPIO remains input-only. Only PA2 transmits toward the local ST-LINK bridge. There is no heap allocation or inbound command parser. The event queue holds 32 entries and drops new events on overflow while recording dropped. Sequence allocation continues on a drop. Status has a nominal 100 ms schedule, repeated policy a one-second schedule, and spare transmit opportunities drain events. The nonblocking UART copies one bounded packet and abandons an uncompleted transfer after 100 ms. Partial transfers increment tx_fail and may leave a fragment before the next protocol marker.

## Application and record format

The companion is a Linux terminal application using Python's standard-library curses, termios, select, and JSON. It has real Overview, Events, and Health screens; no package installation is needed. Run from this directory:

```sh
python3 app/audit.py --port /dev/ttyACM0 --export assessment.json
```

Choose the actual serial device explicitly. Press 1, 2, or 3 for screens, e to export, and q to quit. The port is opened read-only; no target commands are sent. Permission failures are reported without changing system permissions. On unplug the retained evidence stays visible and is no longer LIVE. Exit and restart to reconnect. Automatic discovery and pairing are intentionally absent; physically identify and control the board and host.

LIVE means supported policy, connected serial transport, no inferred restart, and a status received within one second. It does not mean qualified inputs, a safe target, or complete evidence. Replay is never live. A status timestamp that regresses under modular comparison, or any cumulative counter that decreases, freezes the session as a suspected reset. A policy announcement cannot clear that suspicion; a boot frame or explicit new application session can. Lost boot records can still leave some resets undetectable when timestamps and counters happen to move forward. CRC provides no protection against intentional frame injection.

Each ASCII record is `EW1,type,decimal-fields*CCCC` plus LF. CCCC is uppercase CRC-16/CCITT-FALSE over bytes before the asterisk, polynomial 0x1021, initial 0xffff. B contains reset flags followed by policy values 100, 500, 5000, 3. P contains those four policy values without clearing session history. Repeated policy supports late attachment after loss of the one-shot boot frame. Unsupported policy blocks updates until a supported one is received.

S contains fifteen unsigned 32-bit fields in order: time, levels, valid, state, elapsed, busy_ms, completed, rejected, unsafe, timeouts, aborted, gaps, missed, dropped, tx_fail. State codes are UNKNOWN=0, IDLE=1, WAIT_BUSY=2, BUSY=3, COMPLETE=4, FAULT=5. A valid flag means vector qualification, not accepted lifecycle history. E contains sequence, time, kind, value. Kinds are qualified levels=1, state transition=2, unsafe count=3, rejection count=4, timeout count=5, abort count=6, completion count=7, missed-tick gap=8. Events can be older than the latest status because they are queued.

The parser bounds incomplete lines and validates field counts, decimal syntax, numeric limits, checksum, state, and event types. It resynchronizes on a complete EW1 marker after a truncated transfer. The UI retains at most 256 events and shows the latest sixteen on its Events page; exports retain the entire bounded history. A replay file is limited to sixteen MiB. Use `python3 app/audit.py --replay capture.txt --headless --export replay.json` for offline processing.

Export creates a new mode-0600 JSON file exclusively and refuses existing paths and final-component symlinks. Choose a trusted directory. Reports include author jayis1, source, replay provenance, connection and freshness, receipt age, restart and policy warnings, boot metadata, parser errors, latest counters, and event history. Disconnect does not erase the source provenance. Unexported evidence is discarded on application exit. Exports are snapshots rather than raw continuous captures and are neither signed nor tamper-evident.

## Build, CAD checks, and reproducible verification

Run `make -C firmware clean all test` to build the actual Cortex-M4 ELF and binary and run host tests. The image links at 0x08000000 with explicit startup and linker layout. Program only through your physically controlled ST-LINK workflow and start with hardware reset; no board was programmed during the automated checks. The host UART test uses mapped ordinary memory at register addresses, which verifies driver branches but cannot demonstrate real peripheral timing.

Run `python3 -m unittest discover -s tests -p test_app.py -v` and `python3 tests/integration.py`. The latter executes the real C encoder, passes its synthetic frames through replay and serial pseudo-terminals, and drives the real curses UI through a separate terminal. It checks screens, live export, private permissions, overwrite refusal, disconnection, and exit. These fixtures are synthetic test vectors, not measurements of a physical target.

Generate CAD with `/usr/bin/python3 tools/generate_hardware.py` using KiCad 9 and standard footprint libraries. Run `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC, fresh schematic-netlist export, and equality of references, values, footprint identifiers, and every pin/net against both PCB pads and connectivity.json. The local Quorum symbol library name records reuse of jayis1's passive symbols; the sheet title, signals, and UUID namespace identify Erase Witness. There are actual resistor and connector symbols, routed copper, vias, and a closed board outline, not empty KiCad placeholders.

See VERIFICATION.md for the exact tool versions and execution evidence. Passing passive-symbol ERC does not validate external ground or supply safety. Passing DRC does not establish manufacturing tolerances or correct component purchases. Before fabrication inspect the schematic and board, verify footprints against parts, and define strain relief. After assembly measure continuity, resistors, input voltage, loading, back-power paths, sample latency, clock tolerance, USB interruptions, watchdog recovery, and long-running fault behavior. ESD, EMC, regulatory suitability, mechanical robustness, and environmental qualification remain outstanding.

## Research workflows and interpretation

A product-security researcher can instrument an owned development appliance so the four pins reflect its real sanitization controller and access gate. Begin with an independent logic-analyzer trace and disposable data. Exercise a normal cycle, a deliberately delayed busy assertion, a premature done indication, and early access using the target's approved diagnostic harness. Compare expected lifecycle transitions with exported counts and retain firmware revision, target revision, wiring, stimulus script, and timing margins in the engagement record.

A penetration tester can use the instrument to verify a remediation for access being restored before ownership-transfer cleanup completes. Repeat the same paced stimuli before and after the patch. A useful report says that a specific observable sequencing violation disappeared under specified tests, not that the medium is unrecoverable or all races are absent. Independently test storage contents and the actual enforcement path using authorized procedures appropriate to the device.

An authorized red team can use the bench during a collaborative architecture review to make cross-subsystem assumptions visible without collecting credentials or attacking production hardware. A fault may indicate diagnostic mapping error, unsupported signal polarity, cable loss, or inadequate observation resolution rather than a malicious component. Preserve acquisition gaps, parse errors, and transport failures alongside fault counts. When evidence quality is uncertain, mark the assessment inconclusive and repeat with improved instrumentation rather than converting missing observations into a pass.
