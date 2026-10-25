# Reset Quarantine — Post-Reset Privilege Settling Evidence Bench

Author and creator: jayis1. Hardware revision A. Protocol RQ1. Firmware and application: GPL-2.0-only. Hardware tooling: MIT. Carrier hardware: CERN-OHL-S-2.0.

Status: engineering prototype design with compiled embedded firmware, an executable companion application, and routed carrier CAD. Physical fabrication, programming, electrical qualification, and timing calibration have not been performed. This device observes a security policy; it does not enforce that policy and is not a safety controller.

## Purpose and repository-local originality

Reset Quarantine investigates a particular trust-boundary failure: a privileged access path becomes usable during restart, before the system has established stable readiness. Products often expose separate diagnostic indications for reset, initialization completion, access enable, and faults. A static truth table can show that access and ready are simultaneously high, but cannot establish whether readiness was continuously present for an adequate settling interval after an observed reset. This device supplies that missing temporal evidence.

The instrument watches four low-voltage signals on an authorized development bench. After observing a qualified clean reset assertion, it waits for reset release and readiness. Readiness must then remain qualified for 250 milliseconds while access and fault are absent. Only afterward does the observer consider access eligible. Access during the waiting interval, readiness withdrawal after eligibility, and faults are recorded distinctly. No output is connected to the target's reset or access controls.

Practical subjects include administrative maintenance ports, storage-controller debug gates, secure peripheral initialization, recovery-mode service interfaces, and fixture-level access interlocks. The device helps answer whether a software update or recovery routine accidentally exposes privilege while initializing. It does not determine whether a boot image is signed, whether memory was erased, or whether a diagnostic ready signal truthfully represents cryptographic checks.

The repository was inventoried before selection. Coincidence Guard examines fresh bilateral consent within a maximum coincidence window. Erase Witness examines a sanitization handshake before access. Lease Watch examines heartbeat freshness. Grant Ledger studies single-use permission, and Quorum Witness studies majority voting. Reset Quarantine instead requires observed reset provenance followed by a minimum quiet readiness interval, then latches revocation until another reset. The important distinction is minimum settling time across restart, rather than a maximum approval delay or a completion handshake. This is repository-local differentiation, not a claim of worldwide patent novelty.

The passive carrier topology, Cortex-M startup, serial transport, and terminal UI infrastructure intentionally reuse jayis1's existing work. Their reuse is disclosed rather than presented as a new electronic principle. The temporal policy engine, input semantics, state transitions, documentation, and policy regression suite are device-specific. Everything resides in this standalone device subfolder; no other repository or shared systems tree is involved.

## Attack surface, threat model, and authorized use

Use only on equipment you own or have explicit written authorization to assess. Agree with the owner on diagnostic points, stimulus generation, shutdown authority, evidence retention, and the intended settling policy before wiring. Never connect this unqualified prototype to production access systems, industrial safety circuits, vehicles, medical equipment, mains wiring, or circuits whose failure could endanger people. Keep hazardous actuators disconnected from development fixtures.

The investigated adversarial opportunity is a reset or recovery interval in which a controller grants access too early or preserves an old ready indication. An authorized researcher may use the product's normal reset button or approved diagnostic facilities to reproduce such intervals. Reset Quarantine does not initiate resets, inject protocol traffic, alter flash, collect credentials, emulate an identity, or install persistence. Its research value is observation and repeatable evidence, not covert exploitation.

Trust boundaries separate the observed controller from its diagnostic outputs, the common-ground wiring from the MCU, the MCU from the USB serial bridge, and the bridge from the host. A compromised target can lie on every diagnostic pin. A substituted USB device or malicious host can forge records. CRC protects against accidental corruption, not intentional forgery. There are no signatures, authenticated commands, trusted wall clocks, immutable storage, secure elements, or identity measurements. Corroborate significant findings with independent instrumentation and preserve custody of the actual firmware and fixture.

The supplied firmware configures all target-facing pins as inputs. That is a software property, not a hardware data diode: reprogrammed firmware could drive those pins through the series resistors. The carrier is also not galvanically isolated. Even passive observation changes electrical loading. The application performs read-only serial acquisition and has no target command interface. These restrictions reduce accidental actuation but do not eliminate grounding, overvoltage, firmware replacement, or misleading-evidence risks.

## Hardware specifications and assembly

The processor assembly is an external NUCLEO-F401RE, MB1136 family, with an STM32F401RE Cortex-M4 MCU. The firmware linker targets 512 KiB flash and 96 KiB RAM. It uses the reset 16 MHz internal HSI clock rather than the maximum processor clock. The development board's ST-LINK interface provides programming and USB virtual serial. The custom carrier contains only resistors, connectors, and copper; MCU power regulation, USB protection, decoupling, clocking, and reset hardware remain on the external development board.

The carrier is an 80 by 70 mm two-layer board, nominally 1.6 mm FR4. There is no FPGA, radio, battery, charger, analog measurement channel, or onboard supply rail. Connectivity consists of four single-ended diagnostic inputs, a ground harness, a four-wire MCU input harness, and the development board's USB connection to the host.

| Reference | Quantity | Part requirement |
| --- | ---: | --- |
| External processor | 1 | NUCLEO-F401RE, inspect actual MB1136 revision |
| R1, R3, R5, R7 | 4 | 100 kilohm, 1%, axial DIN0207, 7.62 mm pitch, at least 0.125 W |
| R2, R4, R6, R8 | 4 | 1 kilohm, 1%, same package and rating |
| J1 and J3 through J6 | 5 | Straight 1x2 headers, 2.54 mm pitch |
| J2 | 1 | Straight 1x4 header, 2.54 mm pitch |
| Harness | 1 set | Insulated, labeled, short leads with external strain relief |

J1 pins 1 and 2 are both ground; neither is a power connection. Connect J1 to a NUCLEO ground. J2 pins 1, 2, 3, and 4 connect to CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0 respectively. J3 observes RESET, J4 READY, J5 ACCESS, and J6 FAULT. Each target connector uses pin 1 for signal and pin 2 for ground. All four signals are active high. RESET is an active-high diagnostic indication of target reset, not the NUCLEO NRST signal and not a direct connection to an active-low reset wire.

Each signal passes through a 1 kilohm series resistor before reaching the GPIO. A separate 100 kilohm resistor pulls each GPIO node toward ground. Only common-ground, push-pull, 0 to 3.3 V diagnostic outputs are supported. This is not a 5 V input adapter. Negative voltages, field wiring, relay coils, unknown open-drain buses, and externally powered high-voltage signals are prohibited. There are no TVS devices, buffers, fuses, or level translators. A resistor is not a guarantee that an MCU absolute maximum rating cannot be exceeded.

A disconnected signal tends low but is not explicitly detected. Losing the FAULT lead can appear fault-free; losing RESET can erase reset provenance; losing READY can look like revocation. These are measurement ambiguities, not proof of target behavior. Verify harness continuity separately. Power the NUCLEO through its intended ST-LINK USB connector. Do not combine external power and USB without consulting the board manual. Disconnect target signals before removing NUCLEO power to avoid input-protection back-powering. USB and target grounds are common, so check ground-potential differences before connection.

The carrier draws no current from a supply connector because it has none. High diagnostic outputs supply the resistor-network loading. Development-board current, startup demand, and host USB behavior have not been measured; no battery runtime or total assembly current is claimed. An engineering release would require measured power and thermal budgets for the purchased board and actual target output characteristics.

## Architecture and block diagram

```text
Owned fixture diagnostic outputs, active high, 3.3 V only
 RESET ----1k----+---- PA0 / A0
 READY ----1k----+---- PA1 / A1
 ACCESS ---1k----+---- PA4 / A2
 FAULT ----1k----+---- PB0 / A3
                | independent 100k pulldown at every GPIO node
               GND -------- NUCLEO GND -------- USB host GND

External NUCLEO-F401RE
  HSI clock -> SysTick -> whole-vector qualifier -> quarantine policy
                                                    | counters / queue
                                                    v
                       bounded CRC encoder -> USART2 PA2 TX
                                                    |
                                              ST-LINK USB VCP
                                                    |
Linux read-only serial -> bounded parser -> Overview / Events / Health
                                                    |
                                         exclusive private JSON export
```

GPIOA and GPIOB are read sequentially. They are not hardware-latched simultaneously. Sampling is nominally once per millisecond, with three consecutive acquired samples required for qualification. This design is appropriate for slow diagnostic state changes, not instruction-level boot races. A high-speed logic analyzer is needed for submillisecond events and accurate edge ordering. Host receipt time includes USB and scheduler delay and must not be substituted for target transition time.

## Exact observable policy

The input vector is encoded as RESET bit 0, READY bit 1, ACCESS bit 2, and FAULT bit 3. Any change in the raw four-bit vector restarts qualification. Three matching consecutive sampled vectors establish a qualified observation. Shorter pulses may intentionally disappear; pulses between samples can be missed completely. The settling contract is over qualified vectors, not every raw electrical excursion.

UNKNOWN is the boot state and the state after an acquisition gap. READY alone never authorizes anything from UNKNOWN, even after a long wait. A qualified RESET assertion with ACCESS and FAULT low enters RESET. READY high under RESET increments a contradiction episode counter but does not hide the reset observation. This lets a researcher distinguish stale readiness from a missing reset indication. If ACCESS or FAULT is asserted with RESET, the state is LOCKED instead and cannot prepare a release.

When reset deasserts without access or fault, readiness determines the next state. READY low enters RELEASED, which means reset was observed but readiness has not yet qualified. READY high enters SETTLING and starts the minimum interval at zero. This applies even if READY remained high across reset: the full interval begins only after release is observed. There is no maximum allowed initialization time. Remaining RELEASED indefinitely is not a timeout violation.

SETTLING requires 250 measured milliseconds of qualified READY high with RESET, ACCESS, and FAULT low. At age 249 it remains SETTLING; at age 250 it enters ELIGIBLE and increments releases. No further edge is needed to finish the timer. READY dropping before eligibility increments interrupted and returns to RELEASED, where a subsequent ready assertion starts a completely new interval. A new clean reset also restarts the preparation. Access during settling increments unsafe and locks the session; fault locks it as well. Interrupted counts interrupted settling attempts, not necessarily distinct attacks.

Access is evaluated against the prior accepted state and the incoming vector before any new eligibility is granted. Therefore access qualifying exactly at the settling deadline is unsafe: observation does not prove that access followed eligibility. Access qualifying on a later tick after ELIGIBLE is allowed while READY remains high and RESET and FAULT remain low. A shorter, unqualified access pulse is outside this observer's guaranteed visibility. The tests deliberately distinguish deadline concurrency from subqualification glitches.

Once ELIGIBLE, ACCESS may rise and fall repeatedly. This is a readiness policy, not a single-use capability. Qualified readiness withdrawal, reset assertion, or fault assertion increments revoked. Readiness withdrawal or fault locks the session; merely restoring readiness cannot repair it. A clean reset can start a new preparation. Reset coincident with asserted access is unsafe and locked until a later clean qualified reset observation. The releases counter counts eligibility milestones, not proven secure boot operations or safe access sessions.

Unsafe counts beginnings of observed unauthorized access episodes. A continuing high access level is not counted once per sample. Contradictions similarly counts beginnings of READY-under-RESET episodes. Qualified safe observations clear the associated episode latch. Acquisition gaps clear continuity assumptions, so one physical episode spanning a gap may be counted again. This is documented uncertainty rather than an attempt to infer unseen behavior.

A sampled time jump other than one millisecond invalidates qualification and reset provenance. Gaps increments, missed accumulates missing ticks, and UNKNOWN requires a fresh qualified reset observation. Last qualified levels remain available but valid becomes false. Three new samples are needed before fresh qualification. Uptime and event sequence use modular unsigned arithmetic; accumulated counters and state age saturate. Ordinary timer wrap is supported, but an outage spanning a complete uptime cycle cannot be reconstructed. quiet_ms retains the last completed settling duration and is not the current readiness age.

## Firmware implementation and decisions

firmware/quarantine.c owns qualification, the state machine, saturating counters, and a bounded event queue. quarantine.h declares the observable contract. main.c schedules acquisition, status, policy announcements, and event transmission. wire.c implements decimal encoding and CRC without printf or dynamic allocation. drivers/board.c configures GPIO, SysTick, USART, reset-cause capture, and the independent watchdog. drivers/serial.c copies one bounded outgoing packet and advances it without waiting for UART readiness. startup.c and linker.ld provide the real embedded reset path, vector table, memory initialization, and stack reservation.

The main loop exclusively owns policy state and the queue; SysTick only increments a volatile clock. The queue holds 32 events. A full queue drops new entries, advances their sequence numbers, and increments dropped. Status is nominally sent every 100 milliseconds, policy every second, and queued events in remaining transmission opportunities. A packet that cannot finish within 100 milliseconds is abandoned and tx_fail increments. Truncated packet recovery belongs to the host parser, so a blocked transmitter does not indefinitely stop acquisition.

UART is transmit-only, 115200 baud, eight data bits, no parity, one stop bit. PA2 goes to the ST-LINK virtual COM path, not a target-facing connector. The watchdog starts before measurements; startup failure produces no plausible measurements. Its physical timeout depends on the independent oscillator and remains unmeasured. A reset loses RAM evidence and returns to UNKNOWN. No field updater, secure bootloader, signed image verification, persistent session identity, or flash logging is implemented. Programming requires a physically controlled ST-LINK workflow followed by hardware reset; jumping into the image from an arbitrary debugger-modified clock setup is unsupported.

## Application screens, protocol, and evidence handling

The companion is a functional Linux terminal application implemented with Python standard-library curses, termios, select, and JSON. Run `python3 app/audit.py --port /dev/ttyACM0 --export assessment.json` from this directory, selecting the actual device explicitly. Keys 1, 2, and 3 select Overview, Events, and Health; e exports and q exits. No package installation, cloud service, or root privilege is required beyond normal serial-device access permissions. Permission failures are reported rather than changing host security settings.

Overview shows qualified input levels, state, state age, last quiet interval, and policy counters. Events displays the latest sixteen entries from a bounded history of 256. Health shows acquisition and transport loss, parser errors, and boot sessions. LIVE requires a connected transport, recognized policy, a status received within one second, and no suspected restart. It does not mean safe access, valid qualification, an honest target, or a complete event log. Disconnect preserves evidence but marks it inactive. Restart the application explicitly to reconnect.

Frames are `RQ1,type,decimal-fields*CCCC` followed by LF. The checksum is uppercase CRC-16/CCITT-FALSE over the bytes before the asterisk, polynomial 0x1021, initial value 0xffff. B contains reset flags, interval 250, and qualification 3. P repeats interval and qualification without clearing session history. Periodic P frames let late subscribers learn policy after missing the boot frame. An incompatible policy freezes status ingestion until compatibility is restored.

S contains time, levels, valid, state, age, quiet_ms, releases, contradictions, unsafe, interrupted, revoked, gaps, missed, dropped, and tx_fail. State numbers are UNKNOWN=0, RESET=1, RELEASED=2, SETTLING=3, ELIGIBLE=4, LOCKED=5. E contains sequence, time, kind, and value. Event kinds 1 through 8 mean levels, state, unsafe, contradiction, interrupted, revoked, release, and missed ticks respectively. All numeric fields are unsigned decimal, with additional boolean and enum range checks.

The parser validates checksum syntax, CRC, field count, version, and numeric bounds. It bounds incomplete input and finds the complete RQ1 marker after a truncated record. Delayed event timestamps may precede a recent status and are not used alone to infer restart. Ordered status time or cumulative-counter regressions freeze evidence until an explicit boot or a new application session. A repeated policy announcement cannot clear suspected restart. Some resets are indistinguishable when time and counters both appear to advance; there is no persistent boot identifier.

Offline replay uses `python3 app/audit.py --replay capture.txt --headless --export replay.json`. Replay is capped at sixteen MiB and can never appear LIVE. Export exclusively creates a new mode-0600 JSON file, refusing overwrite and final-component symlinks. Use a trusted parent directory. Reports include author jayis1, source, replay provenance, connection and freshness state, receipt age, policy compatibility, restart suspicion, parser errors, status, and retained events. Files are neither signed nor encrypted. There is no automatic upload or raw capture retention; apply the engagement's retention rules to exported evidence.

## Build, verification, and research workflow

Run `make -C firmware clean all test` to link the Cortex-M4 image and run host tests. Run `python3 -m unittest discover -s tests -p test_app.py -v` for app regressions, then `python3 tests/integration.py` for real C-encoder frames through replay and separate serial/UI pseudo-terminals. The integration exercise checks actual screens, exports, overwrite refusal, disconnect, and exit. Its input stimuli are synthetic and are not physical measurements.

KiCad 9 and its standard footprint library are required. `/usr/bin/python3 tools/generate_hardware.py` generates the native schematic, PCB, project, local symbol library, connectivity manifest, and XML netlist. `/usr/bin/python3 tools/check_hardware.py` runs ERC/DRC and compares every reference, value, footprint identifier, and pin/net between fresh schematic export, actual PCB pads, the saved netlist, and the manifest. The carrier has real passive symbols, routed copper, vias, and a closed outline. The external NUCLEO is represented honestly by its harness rather than a fictitious onboard processor.

Begin bench validation with an independent logic analyzer and an owned signal fixture. Exercise cold attachment with ready held, a clean reset followed by slow readiness, readiness chatter, early access, access at the interval boundary, and readiness revocation after eligibility. Verify active-high RESET semantics explicitly. Compare actual diagnostic signals with enforcement behavior; a pin named ACCESS may only describe a requested state, not actual access. Retain the firmware revision, wiring map, intended interval, fixture revision, and all uncertainty counters in the assessment report.

A product-security researcher can use the same scripts before and after a remediation to show whether the observed restart policy changed. An authorized penetration tester can document an early privileged-enable window without collecting secrets. A collaborative red team can expose assumptions about stale readiness during recovery while keeping the experiment in the laboratory. Findings should state the measured observable contract and its limitations, not claim that this device authenticated boot or proved absence of exploitable races.

See VERIFICATION.md for executed results. Before manufacturing or target deployment, independently review purchased parts, footprints, strain relief, clearances, connector orientation, voltage thresholds, input loading, ground behavior, and permitted power sequencing. Measure oscillator tolerance, sampling jitter, watchdog recovery, and reset behavior. ESD, EMC, environmental, regulatory, and physical performance validation remain outstanding. A zero-violation CAD report is not a fabrication or safety certificate.

## Manufacturer references

ST UM1724, the STM32 Nucleo-64 MB1136 user manual, documents CN8 A0 through A3 and the ST-LINK USART2 solder-bridge arrangement. Its USART section identifies PA2/PA3, with the default virtual-COM bridges SB13/SB14 enabled and alternate connector bridges disabled. Verify the actual board before use. RM0368 describes STM32F401 peripheral registers; the STM32F401xD/xE datasheet defines electrical limits. ST's cmsis-device-f4 STM32F401xE header provides a register-map cross-check. None of these manufacturer documents certifies this carrier or the target fixture.
