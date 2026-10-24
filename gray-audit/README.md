# Gray Audit — Quadrature Input Integrity Bench

Author and creator: jayis1

Hardware revision A. Firmware protocol GA1. Firmware and application: GPL-2.0-only. Hardware generation and checking scripts: MIT. This is an engineering prototype design, not an assembled or certified instrument. See VERIFICATION.md for the exact software and CAD checks performed. All physical performance claims below are design constraints or acceptance targets until measured on a prototype.

## Purpose and overview

Gray Audit examines the trust boundary between a human-operated rotary encoder and the software that interprets its two-bit Gray-code sequence. Security-relevant control panels often use incremental encoders for numeric entry, menu selection, maintenance settings, or confirmation gestures. A software decoder can accidentally treat illegal transitions as valid movement, accumulate switch chatter as extra clicks, or hide observation gaps behind a believable position counter. Gray Audit provides a compact bench instrument for identifying those conditions before the encoder is trusted by a product.

The instrument observes two independent A/B pairs. It measures legal forward and reverse transitions, diagonal jumps, direction reversals, uninterrupted four-transition cycles, per-phase activity, state dwell, and edge-interval distributions. It exports those observations to a local terminal application. It does not synthesize encoder signals, recover combinations, bypass locks, read credentials, or modify a target controller. Its intended targets are detached, voltage-free mechanical encoders on an authorized laboratory bench. Test an encoder separately from its original controller, then compare the resulting observations with the controller's documented decoder policy and separately collected test results.

The distinctive workflow is an integrity audit rather than a conventional position display. Illegal diagonal transitions are visible and never converted into displacement. A missed sample explicitly breaks sequence continuity instead of guessing what happened. Raw direction reversals are retained rather than filtered away as inconvenient bounce. The instrument deliberately refuses to label its accumulated position as ground-truth shaft position. This makes it useful when reviewing an embedded application's assumptions about human input.

## Repository distinction and originality boundary

The repository's existing device directories and README descriptions were checked before selecting this concept. Existing rotary-encoder references describe user-interface controls on other tools, not an instrument analyzing quadrature transition integrity. Latch Ledger is the nearest physical implementation: it qualifies independent dry contacts and measures paired contact behavior. Gray Audit instead implements a two-bit transition graph, directional run tracking, phase activity, interval histograms, and explicit sequence discontinuity handling. These are different measurements and a different review workflow, not renamed debounce records.

The passive four-input carrier topology and minimal Nucleo peripheral approach are intentionally reused engineering patterns authored by jayis1. Reusing a safe input circuit is preferable to inventing a complicated electrical interface merely to appear different. Quadrature decoding itself is established technology. The claim here is a new device concept within this repository, not a patent novelty claim or proof that no similar laboratory instrument exists anywhere.

## Authorized use, attack surface, and threat model

Use only equipment you own or have explicit written authorization to assess. Do not attach the carrier to deployed access-control equipment, industrial machinery, medical equipment, occupied-building controls, or any system whose interruption could harm people. Disconnect encoders completely from their original electronics before attaching them. Do not use this project to capture private entry sequences. Obtain consent for any human-interface study and retain only the measurements needed for the agreed evaluation.

The attack surface under study is an implementation's interpretation of ambiguous input. Potential faults include both phases changing between observations, excessive reversal counts from contact chatter, one missing phase, phase asymmetry, and discontinuities introduced by a slow polling loop. These can cause unexpected menu selections or incorrect numeric entry in a poorly designed product. Gray Audit measures evidence relevant to those hypotheses; it does not declare a product vulnerable from one anomalous counter.

The instrument has its own trust boundaries. The encoder leads are untrusted electrically and logically. The serial stream is untrusted to the application. The USB-connected host and ST-LINK programming interface remain trusted by the operator. CRC detects accidental serial corruption, not malicious fabrication. There is no authenticated device identity, secure boot provisioning, signed evidence, trusted wall clock, or anti-replay protocol. A malicious local user can counterfeit a capture. Treat exports as laboratory notes, not authenticated forensic proof.

No command parser exists on the MCU. USART receive is disabled; sensing pins are always inputs. The companion app opens the TTY read-only and never sends a command. A reconnect requires an explicit app restart and cannot replay an intrusive operation. Removing active capabilities makes physical arming and command authentication unnecessary for this revision, but it does not eliminate the need to control access to the programming port or host computer.

## Hardware specification and bill of materials

The complete instrument consists of a NUCLEO-F401RE development board, a wired passive carrier, two optional detached mechanical encoders, and a USB host. The custom PCB does not include the MCU: J1 and J2 are an explicit harness boundary to the development board. This avoids pretending that a connector-only carrier is a complete standalone microcontroller board.

| Item | Specification / purchasing candidate | Quantity |
| --- | --- | --- |
| External MCU board | ST NUCLEO-F401RE, MB1136, STM32F401RE Cortex-M4 | 1 |
| R1, R3, R5, R7 | 10 kΩ 1% axial; Yageo MFR-25FBF52-10K | 4 |
| R2, R4, R6, R8 | 1 kΩ 1% axial; Yageo MFR-25FBF52-1K | 4 |
| J1, J3–J6 | 1×2 2.54 mm through-hole header; Samtec TSW-102-07-G-S | 5 |
| J2 | 1×4 2.54 mm through-hole header; Samtec TSW-104-07-G-S | 1 |
| PCB | 80 × 70 mm, two copper layers, 1.6 mm FR-4 | 1 |
| Leads | Short insulated female jumper leads, labelled at both ends | as needed |

The resistor footprint is an axial DIN0207 body with 7.62 mm lead spacing. Header footprints are standard vertical 2.54 mm through-hole parts. Verify the purchasing candidate's current mechanical drawing and availability before ordering; this run did not obtain distributor stock or pricing. Do not substitute parts based only on similar names. No radio, FPGA, battery, charger, regulator, microphone, or nonvolatile capture memory is fitted to the carrier. The development board supplies regulated 3.3 V and provides USB power, MCU decoupling, reset, debug, and the ST-LINK virtual COM port.

The firmware uses the reset-state 16 MHz HSI clock, not the maximum rated core clock. It targets 512 KiB flash and 96 KiB SRAM. UART output is 115200 baud, eight data bits, no parity, one stop bit. A SysTick millisecond counter schedules observations. The independent watchdog uses LSI and a nominal roughly two-second timeout; oscillator tolerance means it is not a precision timing reference. Firmware does not enable any ADC, DMA, radio, or timer capture input.

### Pin contract

| Carrier connection | Nucleo connection | MCU pin | Meaning |
| --- | --- | --- | --- |
| J1 pin 1 | 3V3 | board supply | carrier pull-up supply |
| J1 pin 2 | GND | board ground | common return |
| J2 pin 1 | A0 | PA0 | axis 0 A, state bit 0 |
| J2 pin 2 | A1 | PA1 | axis 0 B, state bit 1 |
| J2 pin 3 | A2 | PA4 | axis 1 A, state bit 0 |
| J2 pin 4 | A3 | PB0 | axis 1 B, state bit 1 |
| J3 / J4 pin 1 | encoder 0 A / B | through 1 kΩ | isolated encoder contacts |
| J5 / J6 pin 1 | encoder 1 A / B | through 1 kΩ | isolated encoder contacts |
| J3–J6 pin 2 | encoder common | GND | contact return |

PA2 is USART2 TX to ST-LINK VCP, not an encoder input. The Nucleo manual specifies the default ST-LINK USART bridges SB13/SB14 on and the shield USART bridges SB62/SB63 off. Check the actual board revision and bridge state rather than assuming a previously modified board is stock. A0 through A3 are Arduino connector names; this carrier is connected by labelled wires, not stacked mechanically as an Arduino shield.

### Electrical limits and safety

Every sensing node has a 10 kΩ pull-up to 3.3 V and a 1 kΩ series resistor to its external contact terminal. Closing the contact therefore produces a resistor divider, not exactly zero volts at the MCU input. Nominal closed-contact current is 0.300 mA and the nominal low level is 0.300 V. Four closed inputs consume 1.20 mA from the carrier supply. These are resistor-network calculations, not measured prototype results. Verify input thresholds and actual levels during bring-up, including resistor tolerance, supply tolerance, lead resistance, leakage, and contact resistance.

Only voltage-free, detached mechanical contacts are supported. There is no galvanic isolation, surge arrestor, ESD certification, reverse-polarity protection, or field-voltage input protection. The 1 kΩ resistors do not make the interface safe for powered encoder outputs or miswired supplies. Never connect 5 V, 12 V, 24 V, mains, motor outputs, or external batteries. Do not connect a second supply to J1. Ground is shared with the USB host. Stop if any unexpected potential exists between an encoder contact and the instrument ground.

Some mechanical contacts require a minimum wetting current greater than this design supplies. Consult the encoder's specification; results from an unsuitable contact system are not meaningful reliability evidence. The instrument is neither a life-safety monitor nor a production encoder receiver. It uses short bench wiring and does not claim industrial cable tolerance. The carrier has no extra bypass capacitor because it has no active load; the Nucleo provides supply decoupling. This is not a justification for long, noisy harnesses.

## Architecture and block diagram

```text
Detached encoder 0 A/B ---- J3/J4 -- 1k series --+-- PA0/PA1 inputs
                                               +-- 10k to 3V3
Detached encoder 1 A/B ---- J5/J6 -- 1k series --+-- PA4/PB0 inputs
                                               +-- 10k to 3V3
Encoder commons ---------- carrier GND ----------- Nucleo GND

NUCLEO-F401RE
  HSI clock -> SysTick -> bounded foreground sampler
  input states -> Gray transition graph -> counters / interval histograms
               -> discontinuity detection -> bounded anomaly queue
  periodic records + queued events -> CRC CSV -> USART2 TX -> ST-LINK USB

Linux host
  read-only serial -> bounded framing / CRC / schema validation
                   -> overview / distributions / events
                   -> explicit private JSON export, no automatic upload
```

The schematic contains real symbols, labelled wires, footprints, and connected nets. The PCB is routed, not merely an outline with unconnected parts. `connectivity.json` is a design manifest, not a substitute for the schematic: the checker independently exports the schematic netlist and compares every pin against actual PCB pads and the manifest. Passive-pin ERC cannot validate the external MCU board, voltage compatibility, or the harness. Those remain separate engineering checks.

## Firmware behavior and design decisions

`firmware/audit.c` contains the production transition engine. States are encoded with A in bit zero and B in bit one. The positive sequence is 00 → 01 → 11 → 10 → 00. One-bit changes are legal; two-bit changes increment the illegal counter without changing position. After an illegal transition the engine resynchronizes to the observed state, clears the direction context, and breaks the partial cycle. It does not invent a missing intermediate edge.

A forward or reverse cycle means four contiguous legal transitions in the same direction, beginning from any observed state. It is not a calibrated mechanical detent or shaft revolution. A reversal breaks the partial run and starts a new run in the new direction. Position is the signed sum of legal observed transitions. It saturates at signed 32-bit limits and latches a saturation indicator; continuing operation cannot restore the lost absolute count. Unsigned aggregate counters saturate instead of wrapping. The event sequence number intentionally wraps modulo 32 bits.

Repeated calls during the same millisecond do nothing. An elapsed interval other than one millisecond records a sample gap, increments missed-time accounting, and resynchronizes both axes without awarding motion across the gap. State dwell accumulates only continuously observed one-millisecond intervals. Interval histograms record time between observed changes; illegal changes are included as edges. After a sampling gap there is no interval spanning unknown time. Histogram bins are 0–1, 2–3, 4–7, 8–15, 16–31, 32–63, 64–127, and at least 128 milliseconds.

The input algorithm deliberately does not debounce. Debouncing could conceal the behavior under investigation. Consequently reversal counts often represent normal mechanical chatter rather than an attack. Firmware makes no automatic maliciousness decision. An operator should compare multiple specimens, both rotation directions, different speeds, and known-good fixture behavior before interpreting an anomaly.

This is a millisecond polling instrument for slow manual encoders. Require at least five milliseconds between meaningful transitions during baseline tests, and validate with an oscilloscope or logic analyzer. That is an operating restriction, not a measured maximum throughput guarantee. Changes entirely between samples can be missed, and two physical changes can appear as a diagonal. Axis 1 spans GPIOA and GPIOB, so its two input bits are not sampled atomically. The resulting inter-port skew is unmeasured. High-speed motors and precision motion metrology are outside scope.

The foreground loop never waits for UART readiness. Each serial poll moves at most one byte. Output records time out after 100 ms if the peripheral does not progress; failed transmissions increment health telemetry. The anomaly queue holds 32 entries, drops new events when full, and counts drops rather than overwriting old evidence. Periodic reports take priority every 100 ms: two status records, two distribution records, and one health record in a repeating cycle. Thus each periodic record is nominally refreshed every 500 ms, independent of alarm volume. Actual timing still requires bench measurement.

`startup.c` initializes data and BSS and supplies reset, fault, and SysTick vectors. No peripheral interrupts are enabled. `registers.h` uses explicit STM32F401 register addresses. `board.h` freezes the supported clock, baud, and buffer sizes. There is no runtime configuration, heap allocation in the embedded implementation, downloaded payload, or update protocol. Program through the Nucleo ST-LINK using your normal trusted development workflow. A target build confirms linkage, not that flashing or hardware operation has been tested.

## Application and wire interface

The companion is an appropriate desktop terminal application rather than a mobile radio app: the instrument has a wired USB serial interface and no wireless hardware. `app/gray.py` uses Python's standard library and Linux termios/curses. It has three implemented screens, a real serial reader, strict parsing, stale-data indicators on overview rows, disconnect handling, and explicit export. No package installation, web service, cloud account, or telemetry is required.

Run from this device directory:

```sh
python3 app/gray.py --port /dev/ttyACM0 --export /path/to/new-gray-session.json
```

Use keys 1, 2, and 3 for overview, distributions, and recent events; e exports; q exits. A serial EOF or error marks the session disconnected and leaves existing observations available. Silent devices may leave the port connected while rows grow stale; connection state alone is not a freshness guarantee. The app never automatically reconnects. Restart it after checking the cable and device. Export uses exclusive creation, refuses existing files and symlinks, and requests mode 0600. Choose another destination for a subsequent snapshot. Delete exports using normal local file-management tools according to the engagement retention policy; the application has no hidden capture store.

Every frame is ASCII `GA1,type,numeric-fields*CCCC` followed by LF. CCCC is uppercase CRC-16/CCITT-FALSE, polynomial 0x1021, initial 0xFFFF, covering the bytes before the asterisk. No quoting or arbitrary text fields are allowed. The parser bounds line storage and discards an oversized frame until the next newline, preventing unbounded memory growth. Values are range checked, with signed position as the only signed field. Protocol type and exact field count must match.

Fields after the record type are:

- B: zero timestamp, reset flags.
- S: milliseconds, axis, state, position, forward, reverse, illegal, reversals, edges, forward cycles, reverse cycles, phase A changes, phase B changes, edge age, minimum interval, maximum interval, saturated flag.
- D: milliseconds, axis, four dwell counters in state-number order, eight interval bins.
- H: milliseconds, sample calls accepted, gaps, missed milliseconds, dropped events, transmission failures, next event sequence.
- E: milliseconds, sequence, axis, kind, previous state, next state. Kinds are 1 illegal, 2 reversal, 3 gap, 4 saturation.

An edge age or minimum interval of 4294967295 means no applicable observation has been established. Device time wraps modulo 32 bits. The app clears accumulated rows on a boot record or inferred backward periodic time jump, allowing ordinary wraparound. Queued event timestamps do not drive restart inference because they can legitimately predate a recent status record. A restart with a lost boot record and a plausible timestamp cannot always be detected; there is no persistent boot identifier. Exports preserve per-row receipt ages, connection state, parser errors, reset inference count, and the limited recent-event window. They do not turn local monotonic time into a trusted wall clock.

## Practical assessment workflows

A product security researcher can characterize a detached input assembly before reviewing firmware. Rotate slowly in each direction, pause at several positions, and compare legal counts, illegal transitions, and phase activity. A phase that never changes may indicate a broken lead, wrong contact assignment, or defective encoder. A diagonal does not identify its cause; confirm wiring and sampling limitations first.

An authorized red-team laboratory can use the instrument to document which physical-input assumptions deserve review without connecting an injection tool to a deployed product. For example, a decoder design that assumes every sample differs by one bit can be challenged with ordinary bench observations from worn switches. The next step is controlled software testing of that decoder, not covert manipulation of an installed access panel. Gray Audit does not provide an exploit or a signal replay path.

A penetration tester evaluating a client's kiosk can remove a spare input assembly with permission, capture its observed transition statistics, and include those facts in a defensive report. Compare them against the application's specified behavior under ambiguous input. Keep target identifiers and personal information out of the export filename. Correlating timing with real user entry is outside this instrument's intended workflow.

For regression testing, retain a known-good encoder and a documented manual motion procedure. Record firmware revision, wiring photograph, contact type, and an independent reference trace alongside each export. Do not compare position totals across sessions as though the device had an absolute reference. Reset starts from zero at an arbitrary observed phase. A useful conclusion is often that ambiguity exists and must be handled, not that a particular count proves a security failure.

## Build, validation, and physical bring-up

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p 'test_app.py'
python3 tests/integration.py
/usr/bin/python3 tools/check_hardware.py
```

The target build requires GNU Arm Embedded tools and newlib with nosys specs. Host tests use GCC with address and undefined-behavior sanitizers. Hardware checking requires KiCad 9 CLI and its system-Python pcbnew module. Regenerate authored hardware with `/usr/bin/python3 tools/generate_hardware.py`; inspect any resulting diff before accepting it. Run the checker again after regeneration. Tests inject synthetic observations explicitly; they are not measurements from a physical encoder.

Before powering a prototype, inspect solder joints and continuity against the exported netlist. Verify that no supply-to-ground short exists and that every header pin reaches only its intended resistor network. With all encoder leads disconnected, verify the development board's supply and reset configuration. Connect the carrier power harness, then measure all four sense nodes high. Close one contact at a time through a detached test switch and measure its low level. Verify that the application reports the correct axis and phase. Only then connect a suitable mechanical encoder.

Further physical acceptance work must measure HSI/UART timing, polling latency under sustained chatter, watchdog recovery, inter-port sampling skew, contact threshold margins, cable sensitivity, and behavior during unplugging and brownout. Inspect the enclosure and strain relief; the bare PCB is not suitable for field use. No fabrication, electrical bring-up, environmental testing, EMC testing, regulatory approval, or independent physical validation was performed during this design run. Do not describe the instrument as production-ready based on a clean compiler or CAD report.

## References and provenance

ST's UM1724, STM32 Nucleo-64 boards (MB1136), is the authority for connector mapping, power configuration, and ST-LINK USART bridges: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . A previously downloaded copy was available for this run; a fresh direct download failed. Consult the manufacturer's current revision before assembly.

ST's RM0368 reference manual covers STM32F401 peripheral registers and reset behavior. The STM32F401xD/xE datasheet defines electrical limits; read it together with the actual encoder datasheet before hardware use. KiCad libraries supply standard footprint geometry. Authored circuitry, algorithms, application, and documentation are credited to jayis1; upstream component names and library identities remain intact. No claim is made that jayis1 designed the commercial Nucleo board or its MCU.
