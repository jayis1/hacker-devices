# Replay Sieve — Session-Bounded Duplicate-Acceptance Witness

Author and creator: jayis1. Hardware revision A. Wire protocol RS1. Firmware, companion application, and tests: GPL-2.0-only. CAD utilities: MIT. Passive carrier: CERN-OHL-S-2.0.

Status: unbuilt engineering prototype with target-linked C firmware, an executable Linux desktop companion, and routed native KiCad carrier files. Software and CAD results are recorded in VERIFICATION.md. Fabrication, programming an actual MCU, electrical bring-up, calibrated timing, and deployment have not been performed. This device observes a diagnostic contract; it is not an anti-replay firewall, cryptographic authenticator, safety controller, or certificate of secure behavior.

## Purpose and practical value

Replay Sieve examines whether a target accepts a command identity more than once inside a declared session. A development adapter exposes two identity bits, an ACCEPT strobe, and a SESSION level. The observer maintains a four-bit set of identities already accepted. It allows distinct identities in any order, records repeated acceptance as a sticky finding, and clears the set only when a new correctly delimited session opens. Completion of the earlier command does not authorize its reuse.

The motivating failure is an embedded command handler that treats an idempotency key as an in-flight resource instead of remembering its use for the entire authenticated session. When the first operation completes, a buggy handler discards the key and accepts the same signed request again. Ordinary operation counters can look plausible because there is never concurrent execution. Replay Sieve preserves identity membership after the operation finishes, exposing the difference between exclusive ownership and at-most-once acceptance.

A useful demonstration sends identities three, one, and three within one session. Three and one are both fresh even though the numeric order decreases. The second three is a duplicate. Ending that session while ACCEPT is low, then opening a new one, permits three again. This is a finite exact-identity experiment, not a rolling sequence-number window. Two bits name exactly four distinct identities in an agreed test campaign. Never connect the low bits of a larger sequence counter or hash unrelated commands into those four values; collisions would create false findings and destroy the interpretation.

The instrument is useful when target logs share the same command queue or fault-prone session manager being tested. Its independent MCU retains a small cumulative ledger without receiving request bodies, credentials, keys, or personal data. It cannot prove that the target actually executed a protected action, that the session was authenticated, or that the adapter faithfully represents internal state. Independent waveform capture, adapter source review, and a functional check remain necessary.

## Originality within this repository

The repository directories and design titles were inventoried before implementation, and the closest measurement contracts were read in full. Slot Alias tracks simultaneous ownership of named resources and permits reuse after retirement. Replay Sieve deliberately forbids identity reuse after completion until the session ends. Epoch Witness checks ordered firmware generations and allows repeated equal generations; Replay Sieve allows arbitrary first-use order but flags repeated identities. Quiescent Cut tracks a retirement-time reader cohort, not a session-wide accepted-identity set. Existing radio replay tools transmit or decode protocol traffic; this instrument neither captures nor retransmits such traffic.

Novelty is relative to these reviewed repository designs, not a claim of worldwide novelty or patentability. The passive carrier topology, startup code, register definitions, serial transport, CRC encoder pattern, and desktop transport reuse existing work authored by jayis1. The distinct contribution is the session-scoped identity policy, conservative boundary handling, retained membership semantics, evidence contract, and regression experiments. All necessary source files are local to this folder; no sibling directory is needed to build it.

## Legal and ethical disclaimer

Use only on equipment you own or have explicit written authorization to assess. Authorization should cover diagnostic instrumentation, physical attachment, test workloads, permitted interruptions, and evidence retention. Permission to test an application does not automatically permit connecting an unisolated circuit to its electronics. Use visible labelled leads and a controlled, low-energy development fixture with hazardous actuators disconnected.

Do not connect to deployed access controls, vehicles in service, medical equipment, industrial protection systems, mains-related circuitry, unknown field wiring, or production command paths. The supplied firmware is receive-only toward the target, but loading, grounding errors, and back-power can still damage equipment. Stop if the attachment changes the target's behavior. The author, jayis1, does not authorize unauthorized access, covert collection, destructive experiments, or interference with third parties.

For red teams, the intended activity is an owner-approved laboratory demonstration using normal test commands on an isolated training target. Penetration testers can use the resulting evidence for remediation retests, not as a substitute for validating security impact. This device has no exploit payload, credential collection, persistence mechanism, packet injector, or autonomous attack mode.

## Attack surface and threat model

The assessed boundary is between session lifecycle, command identity tracking, and the actual acceptance indication. Defects include early eviction of a remembered identity, clearing replay state when ACCEPT falls, treating numeric decrease as the only replay condition, reusing a tag without a new session, and accepting a command at the same observed boundary as session creation or withdrawal. These are sampled diagnostic findings, not verified CPU instruction ordering.

Trusted components are the operator, target adapter, wiring, installed observer image, external development board, programming interface, and workstation. A malicious target can lie on every wire, including SESSION. The observer does not independently authenticate a session or distinguish a malicious reset from a legitimate new session. Tie the adapter's SESSION indication to the security boundary under review and document how it is generated. A wire broken low can resemble an inactive session; this design has no continuity or tamper detector.

Serial traffic and replay files are treated as untrusted parser inputs. Range, framing, checksum, conservation, monotonicity, and membership checks reject selected inconsistent records. They do not authenticate a device. CRC-16 is accidental-corruption detection, not cryptography. There is no secure element, encrypted channel, signed evidence, pairing credential, secure bootloader, signed updater, persistent boot identifier, anti-rollback protection, or network service. ST-LINK can replace the image. A hostile USB device can impersonate RS1. Protect the bench physically and preserve image hashes separately where evidence provenance matters.

## Hardware specifications and bill of materials

The complete assembly uses an external ST NUCLEO-F401RE development board, the passive carrier in kicad/, short labelled insulated leads, insulating support, and a Linux workstation. The MCU is STM32F401RET6, Cortex-M4, with 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default nominal 16 MHz HSI clock, nominal one-millisecond SysTick, and USART2 at 115200 baud, eight data bits, no parity, one stop bit. There is no PLL setup, RTOS, dynamic allocation, floating-point dependency, or vendor SDK requirement.

The purchased module provides the MCU, regulator, decoupling, reset, SWD interface, USB power arrangement, and ST-LINK virtual COM bridge. These are external module facilities, not missing carrier components. The carrier itself has no FPGA, radio, antenna, active sensor, battery, charger, isolation, storage card, level translator, or target power output. It is cable connected, not a mechanically compatible stacking shield.

The native PCB is 80 by 70 mm, two copper layers, nominally 1.6 mm thick, with 0.3 mm tracks and a closed outline. It has fourteen through-hole components, thirty electrical pads, and nine named nets. There are no mounting holes or enclosure files. Provide strain relief, underside insulation, and a nonconductive support before assembly is connected to another system.

| Reference | Quantity | Required part characteristics |
| --- | --- | --- |
| External module | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 100 kOhm, 1%, quarter-watt axial pulldown |
| R2, R4, R6, R8 | 4 | 1 kOhm, 1%, quarter-watt axial series resistor |
| J1, J3, J4, J5, J6 | 5 | Vertical 1x2 through-hole header, 2.54 mm pitch |
| J2 | 1 | Vertical 1x4 through-hole header, 2.54 mm pitch |
| Harness and support | 1 set | Short insulated leads and nonconductive mounting |

Resistors use KiCad DIN0207 axial footprints with 7.62 mm lead spacing; headers use standard PinHeader footprints. Match purchased body dimensions, drill fit, pin-one orientation, mating clearance, and support geometry before ordering. No availability, price, production yield, or physical fit is asserted.

## Wiring, power, and electrical limits

| Function | Target connector | MCU harness | Nucleo connection | Bit |
| --- | --- | --- | --- | --- |
| TAG0 | J3 pin 1 | J2 pin 1 | CN8 A0 / PA0 | 0 |
| TAG1 | J4 pin 1 | J2 pin 2 | CN8 A1 / PA1 | 1 |
| ACCEPT | J5 pin 1 | J2 pin 3 | CN8 A2 / PA4 | 2 |
| SESSION | J6 pin 1 | J2 pin 4 | CN8 A3 / PB0 | 3 |
| Common ground | J3–J6 pin 2 | J1 both pins | Nucleo GND | none |

J1 is ground only: never connect a supply to either pin. J2 carries four signals and no ground or power. Net names ending `_IN` are target-side nodes before the series resistor; unsuffixed nodes reach the MCU and individual pulldowns. Confirm numbering by continuity rather than cable appearance.

Only nominal zero-to-3.3 V, common-ground, push-pull diagnostic outputs are supported. The nominal resistive path to ground is 101 kOhm per channel before leakage and source impedance. There is no qualified ESD clamp, overvoltage protection, galvanic isolation, fuse, reverse-current blocker, or hot-plug guarantee. A series resistor is not an electrical safety barrier. Five-volt logic, 1.8 V logic, negative signals, differential buses, relay coils, and industrial voltages are outside scope.

Power the module through its documented ST-LINK USB arrangement. Reserve a conservative 5 V, 500 mA source budget subject to actual jumper configuration and USB limits; this is not measured consumption. Establish compatible ground, power the observer first, and remove target signals before removing observer power. Otherwise MCU protection structures can back-power the board. Verify module revision and solder bridges before relying on USART2-to-ST-LINK routing.

## Architecture and block diagram

```text
Owner-instrumented target          Passive carrier         External Nucleo
TAG0 ----------------------------- 1k --+------------------ PA0 / A0
TAG1 ----------------------------- 1k --+------------------ PA1 / A1
ACCEPT --------------------------- 1k --+------------------ PA4 / A2
SESSION -------------------------- 1k --+------------------ PB0 / A3
                                        | each node
                                       100k
                                        |
Common ground ------------------------- GND --------------- GND / USB

GPIO reads -> continuity guard -> bundled tag + independent qualifiers
           -> prior tag setup check -> session boundary state machine
           -> persistent-in-session identity set -> admission categories
           -> bounded RS1 encoder -> USART2 PA2 TX -> ST-LINK USB bridge
           -> Linux Ledger / History / Integrity -> private JSON export
```

GPIOA is read before GPIOB; these are not simultaneously latched physical samples. Inter-port skew has not been measured. Short pulses may be missed or rejected, and events within the same sample cannot be reliably ordered. Use deliberately slowed instrumentation: twenty milliseconds of tag setup and twenty milliseconds of each high/low strobe period are sensible initial laboratory margins. This is not a processor-speed command bus analyzer.

## Exact firmware measurement contract

The engine starts in WAIT. Arrange the actual target outside a session before starting the observer. All inputs must qualify and both raw and qualified ACCEPT and SESSION must be low before IDLE is established. Starting during a session does not invent a fresh identity set or retroactively count existing activity. Tags need not be zero. An operator must establish the real initial condition; electrical silence alone cannot prove application state.

TAG1:TAG0 qualifies as a single bundle to avoid constructing a mixed identity from individually delayed bits. ACCEPT and SESSION qualify independently so tag chatter cannot hide a sustained strobe. A change requires its first observation plus three subsequent unchanged millisecond observations. Qualification is based on actual consecutive calls; duplicate calls in the same tick do not advance it. A raw tag disagreement or a qualified tag change resets setup. Acceptance requires five stable evaluated tag intervals already completed before the current admission observation, plus matching raw and qualified tag at that endpoint.

A qualified SESSION rise from IDLE opens a session only if ACCEPT was and remains low in previous qualified, current qualified, and current raw samples. Opening increments sessions and clears seen. A qualified ACCEPT rise is classified before session transitions. Consequently simultaneous opening and acceptance is invalid, not fresh. A qualified SESSION fall closes an active session only while ACCEPT is low across the same boundary checks, incrementing ended. The completed session's seen bitmap remains visible until the next valid opening. SESSION withdrawal while ACCEPT is held high halts correlation rather than silently discarding the set.

Every qualified ACCEPT rise after the initial baseline increments admissions. If the session and setup requirements fail, invalid increments and the run halts. Otherwise the tag selects one of four seen bits. A clear bit becomes set and fresh increments. An already-set bit increments duplicate and records a sticky finding while correlation continues. Holding ACCEPT high counts once even if the tag later changes. There is no post-admission tag-hold contract; the adapter must present the identity at the defined sampled admission endpoint. The next edge must independently meet setup.

There is no numeric ordering requirement and no release event. An identity remains seen regardless of operation completion or elapsed time. Even after all four identities have been used, the set remains valid: every further admission is duplicate until a new session. Short unqualified SESSION-low glitches do not clear the set. A raw withdrawal at an ACCEPT endpoint invalidates that admission even before SESSION qualifies low, avoiding an optimistic interpretation of debounce latency.

Any nonzero tick delta other than one records a gap and missed ticks, then permanently halts correlation. A lost sample could hide a session boundary or acceptance, so recovery from quiet signals would invent history. Normal unsigned timestamp wrap is supported. All policy counter updates happen on a temporary state copy. Counter exhaustion rolls back the entire sample and halts the prior committed state without changing its set or session accounting. Halt preserves phase and seen as retained evidence, clears validity, and allows only time and transmitter diagnostics to advance. No evidence persists across an observer reset.

The public conservation identities are admissions = fresh + duplicate + invalid and sessions = ended + one if the retained phase is SESSION. Fresh cannot exceed four times sessions. Seen population cannot exceed cumulative fresh. Duplicate requires a prior fresh admission. Boundary faults without a new ACCEPT edge can set the invalid-session fault without incrementing invalid; that counter classifies admissions, not every boundary error. These distinctions are implemented in both firmware and companion.

## Runtime, protocol, and application

The main loop samples before servicing transport. UART submission copies a bounded frame, each poll transmits at most one ready byte, and a stalled frame is abandoned after one hundred nominal milliseconds. Watchdog startup waits are bounded using SysTick; failure halts measurement before first status. Physical watchdog timing, a stopped tick, and actual power recovery remain silicon tests. Target GPIO remains input-only; USART receiver is disabled. Programmable inputs are not a physical data diode because replacement firmware could drive them.

RS1 records are ASCII `RS1,<kind>,<unsigned decimal fields>*<CRC>` followed by LF. Four uppercase CRC digits use CRC-16/CCITT-FALSE, polynomial 0x1021, initial 0xffff, no reflection or final XOR, over bytes before the asterisk. B carries reset flags once at startup. P carries `3,1,5,4`: qualification intervals, nominal sample milliseconds, prior setup intervals, and identity capacity. P repeats approximately every second to support late attachment without clearing history. S is scheduled approximately every hundred milliseconds.

S fields in order are `now levels valid halted faults phase seen stable observed sessions ended admissions fresh duplicate invalid gaps missed tx_fail`. Phases zero, one, and two are WAIT, IDLE, SESSION. Fault bits are duplicate identity=1, invalid session or setup=2, acquisition gap=4, exhaustion=8, watchdog startup=16. All except duplicate halt correlation. Stable is capped at five; seen is a bitmap of identities, not a copy of the current two-bit tag.

Launch `python3 app/console.py` on Linux with Tk installed. The local desktop has Ledger, History, and Integrity screens. Enter the exact serial device and select New live. There is no auto-probing, command channel, cloud upload, automatic updater, or remote control. The port is opened read-only. A second connection is refused until disconnect. Failed opening preserves evidence; successful replacement starts a new host session. Export old evidence first if it matters.

History retains at most 256 accepted snapshots, not every physical edge. Integrity shows source provenance, compatibility, receipt age, parser errors, reset flags, connection state, and freeze reason. Live requires two advancing compatible statuses while connected and a recent advancing receipt. It means telemetry progress, not healthy acquisition; halted evidence can still be transmitted. Duplicate timestamps do not renew freshness. Counter rollback, contradictory duplicates, changed halted evidence, disappearing sticky faults, or membership changes inconsistent with new sessions and fresh admissions freeze acceptance.

A reception pause over one second and cumulative host/device progress divergence also freeze live collection rather than presenting queued old records as current. Drift allowance is half a second of buffering plus an oscillator allowance capped at another half second. Conservative freezing can occur on a long uncalibrated session. Without a persistent boot identity, some resets are indistinguishable when the boot record is lost and later times and counters appear to advance. Reconnect and restart the documented target baseline after disruption.

Replay accepts regular files only, up to two MiB, using nonblocking opening and file-type checks so FIFOs cannot hang the interface. It is always labelled replay. The parser resynchronizes on the complete RS1 marker, bounds partial lines, counts truncated EOF tails, and refuses a replay with no compatible status. Export creates a new JSON file with mode 0600 and refuses overwrite and final-path symlinks. It includes author jayis1, provenance, integrity qualifications, latest snapshot, and retained history. Reports are unsigned and unencrypted; follow the engagement's storage and disposal policy.

## Build, test, and physical acceptance

Required tools are GNU Make, Arm GNU GCC with newlib/nosys support, host GCC, Python 3 with Tk, Xvfb for headless GUI tests, and KiCad 9 CLI/Python bindings with standard footprint libraries. There are no pip or npm dependencies. Run from this directory:

```sh
make -C firmware all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
python3 app/console.py
```

The actual Cortex-M4 build emits ELF, binary, and map under firmware/build, with flash at 0x08000000, 96 KiB RAM, and linker-reserved stack headroom. ST-LINK programming is a separate supervised physical step. The fixture executable emits synthetic production-encoder traffic, not measured hardware output. Tests exercise identity permutations, out-of-order first use, repeated identities, legitimate new-session reuse, conservative simultaneous boundaries, raw withdrawal, ignored glitches, gaps, overflow, wrap, transport bounds, scheduler startup, malformed records, freshness, replay, exports, and real Tk/PTY interaction.

Hardware checking runs ERC and DRC and compares fresh schematic connectivity, component values, and full footprint identities with actual PCB pads and the manifest. Passive ERC cannot validate external power, harness orientation, threshold margins, or module internals. Before fabrication inspect the rendered schematic and copper, purchased dimensions, connector access, and insulating support. Before valuable equipment is attached, measure continuity and resistance, ground offsets, logic thresholds, loading, power sequencing, and GPIO direction through resets. Compare timing against an independent calibrated analyzer and exercise unplug, brownout, watchdog recovery, and short pulses.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136, RM0368 for STM32F401 registers, and DS9716 for electrical characteristics. This release reuses the established repository mapping; fresh manufacturer-document verification was not performed. Check current documents and the physical module revision before wiring. Fabrication, flashing, electrical interoperability, ESD, EMC, enclosure suitability, environmental reliability, and regulatory qualification remain outstanding. Passing software and CAD checks does not establish physical readiness or permission to test another person's system.
