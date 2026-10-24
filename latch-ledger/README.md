# Latch Ledger — Dry-Contact Tamper-Switch Evidence Bench

Author and creator: **jayis1**. Hardware revision A; protocol LL1.

## Purpose and overview

Latch Ledger is a four-channel bench instrument for investigating a deceptively simple security boundary: the mechanical contacts that tell an enclosure controller whether its lid, door, or removable panel is closed. It measures sampled raw transitions, debounced transitions, short pulses, and disagreement between two pairs of contacts. Its purpose is to help an authorized evaluator discover whether a proposed tamper-detection policy hides short openings, treats two switches inconsistently, or relies on an unstable mechanical assembly. It does not unlock anything, impersonate an access-control credential, inject bus traffic, or defeat an installed alarm.

The device combines a passive, routed through-hole carrier board with an unmodified NUCLEO-F401RE development board and a Linux terminal companion application. The carrier gives each disconnected contact a small measurement current; consequently, “passive” means no programmable target drive or interception, not zero electrical excitation. A contact must be completely disconnected from its original controller and every external supply before connection. This is an instrument for a laboratory fixture, not an inline alarm accessory.

The distinctive research workflow is a paired-contact ledger. Channels zero and one form one pair; channels two and three form the other. An evaluator can mechanically operate a lid with two independent switches, then compare the individual raw histories with the interval during which the debounced states disagree. This separates three commonly conflated phenomena: ordinary contact chatter, an opening suppressed by the chosen debounce policy, and inconsistent placement or mechanics between redundant switches. Findings are observations, not automatic declarations of a vulnerability.

Repository novelty was assessed against the existing device directories and their README purposes before implementation. Existing projects include credential readers, sensor-bus analyzers, electromagnetic instruments, debug probes, and tamper features within other tools. None identified during that inspection provides this disconnected dry-contact, paired-state audit workflow. This is a claim of a distinct contribution within this repository, not a patent search or a claim that contact analyzers have never existed elsewhere.

## Status and evidence boundary

This revision contains implemented firmware, its complete target link configuration and startup code, a working companion application, a populated schematic, a routed PCB, a project file, and automated checks. Software execution and CAD rule checks are reproducible using the commands below. No fabricated board, Nucleo connection, oscilloscope measurement, ESD test, temperature test, or regulatory assessment was performed during this implementation. The hardware is an engineering design for review and controlled bring-up, not a certified or physically validated product.

A successful compile establishes that the code can be built for the selected MCU; it does not establish that the USB bridge, actual oscillator, solder bridges, or wires on a particular board work. Likewise, schematic ERC and board DRC test defined relationships and manufacturing geometry, not whether someone accidentally connects a live circuit. The schematic uses passive connector and resistor symbols because the active development board is external. ERC cannot validate that external board's rail selection or supply behavior.

## Attack surface and threat model

The asset under examination is a product's interpretation of mechanical enclosure state. Potential weaknesses include overly long debounce intervals, single-switch reliance, intermittent harnesses, inconsistent redundant switch placement, and failure to distinguish an open contact from a broken wire. Latch Ledger exposes measurements useful for assessing those hypotheses. It does not connect to the production policy engine, and cannot prove how an application reacts without an independently authorized, separately instrumented experiment.

The instrument also has its own trust boundaries. A hostile or defective serial source can send malformed, oversized, truncated, or numerically invalid records. The application bounds line length, verifies CRC, validates record shapes and field ranges, and rejects unsupported versions. All buffers and retained event lists are bounded. The firmware accepts no serial commands and does not enable its USART receiver. There is no update endpoint, wireless interface, network listener, cloud account, or remote-control mechanism.

An attacker with physical access can replace the development board, reflash it over SWD, or fabricate serial records with valid checksums. CRC is error detection, not authentication. No secure boot, signed evidence, encryption, anti-rollback, tamper resistance, or trusted timestamp is claimed. Restrict physical access to the fixture and control the workstation's serial-device permissions. Avoid running the companion as root. The local operator and host operating system remain trusted.

Noise, cable motion, bad connections, and contact contamination can resemble security-relevant transitions. A missing input cable reads open because of the external pull-up; this is not a diagnosis of why it is open. A short to ground reads closed, so this revision is not an end-of-line supervised alarm-loop tester. Do not use it as a life-safety monitor, an access decision device, or a replacement for a certified controller.

## Hardware specifications and bill of materials

The active module is **ST NUCLEO-F401RE**, based on STM32F401RET6: Cortex-M4, 512 KiB flash, and 96 KiB SRAM. Firmware uses the reset-default 16 MHz HSI clock, not the board's optional external clock arrangement. Sampling is a foreground task with a nominal one-millisecond tick. The board's ST-LINK virtual COM connection carries 115200 baud, eight data bits, no parity, and one stop bit. The instrument has no radio, FPGA, battery, charger, or high-voltage section.

The carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4, with through-hole headers and axial resistors. A nonconductive bench tray should prevent accidental underside contact. There are no mounting holes in revision A; do not improvise metal fasteners through copper. The complete bench arrangement includes the Nucleo, carrier, short harness, USB cable, and disconnected test switches. It is deliberately larger and easier to inspect than a concealed implant.

| Reference | Quantity | Specification / example orderable part |
|---|---:|---|
| External MCU board | 1 | ST NUCLEO-F401RE, MB1136 family |
| R1, R3, R5, R7 | 4 | 10 kΩ, 1%, axial metal film; Yageo MFR-25FBF52-10K |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial metal film; Yageo MFR-25FBF52-1K |
| J1, J3–J6 | 5 | 1×2, 2.54 mm male header; Samtec TSW-102-07-G-S |
| J2 | 1 | 1×4, 2.54 mm male header; Samtec TSW-104-07-G-S |
| Harness | as needed | Insulated female jumper leads, maximum recommended 20 cm |
| Test contacts | up to 4 | Disconnected dry contacts rated for low-level switching |

Resistor footprints use the standard DIN0207 axial, 7.62 mm lead spacing. Verify actual purchased dimensions and bend leads without stressing the body. Header pin one uses a rectangular pad. Procurement substitutions require dimensional and rating review; the listed examples are not a guarantee of stock or pricing.

## Pin map, power, and electrical limitations

Power the Nucleo only through its normal ST-LINK USB connector for this workflow. Connect carrier J1 pin one to Nucleo CN6 pin four, labeled **3V3**; connect J1 pin two to CN6 pin six or seven, labeled **GND**. Do not use the adjacent 5 V pin. Do not simultaneously introduce an external supply. The carrier has no regulator or reverse-polarity protection because it is a restricted accessory powered from the known development board rail.

Connect carrier J2 pins one through four to Nucleo CN8 pins one through four respectively: A0/PA0, A1/PA1, A2/PA4, and A3/PB0. The unusual PA4 mapping is intentional and appears in `drivers/board.c`. Connect each switch between pins one and two of J3, J4, J5, or J6. Pin one is the series-resistor contact connection; pin two is shared ground. All ground returns are electrically common with the USB workstation. There is no galvanic isolation.

For each channel, a 10 kΩ resistor pulls the SENSE net to 3.3 V. A 1 kΩ resistor connects SENSE to the contact header. A closed contact forms an 11 kΩ path to ground, giving nominal 0.3 mA contact current and 0.3 V at SENSE. The four closed contacts together draw nominal 1.2 mA from 3V3, excluding the Nucleo's consumption. Those values assume nominal components and supply; they are design calculations, not measurements. Contact choice matters: some power-switch contacts are unreliable at this small current. Use switches with suitable low-level ratings or document the resulting uncertainty.

No additional bulk or decoupling capacitors are required on this passive resistor-only carrier; the Nucleo supplies its own regulated and decoupled MCU rail. There is intentionally no RC filter that would hide short contact behavior before firmware observes it. Long cables, ESD exposure, external voltages, and hot-plugging are outside the validated design scope. The series resistor is not a mains barrier, surge suppressor, or permission to attach powered signals. Connect the fixture while USB is unplugged. Use an ohmmeter and voltage check to confirm that each switch is isolated from its former system.

## Architecture and block diagram

```text
                       Passive carrier, repeated four times
NUCLEO 3V3 ---- 10k ----+---- 1k ---- disconnected dry contact ---- GND
                       |
                  SENSE0..3
                       |
                PA0 PA1 PA4 PB0
                       |
             STM32F401RE / HSI16
        sample -> raw edges -> 5 ms debounce
             -> pair mismatch -> bounded queue
                       |
              USART2 PA2 TX only
                       |
               ST-LINK USB VCP
                       |
         Linux app: channels / events / help
                       |
          explicit private JSON snapshot
```

`kicad/device.kicad_sch` includes actual component symbols, wires, labels, footprint assignments, and per-component author properties. `device.kicad_pcb` includes corresponding footprints, named nets, copper routes, a board outline, and pin-one markings. `connectivity.json` is a machine-readable design manifest. `tools/check_hardware.py` exports a fresh KiCad XML netlist and compares every schematic and PCB pad assignment against that manifest, in addition to running ERC and DRC. This prevents a visually plausible drawing from silently disagreeing with the board.

## Firmware implementation and decisions

The firmware is plain C11 with no RTOS and no downloaded SDK. `startup.c` installs the Cortex-M vector table, initializes data and BSS, and enters `main`. `linker.ld` describes the exact target flash and RAM regions and reserves stack headroom. The application assumes entry from a hardware reset at the normal flash base. A bootloader that changes clocks or vector relocation is unsupported unless the startup design is revisited.

`drivers/board.c` enables the GPIO and USART clocks, leaves every contact pin as an input without an internal pull resistor, selects PA2 alternate function seven, and starts SysTick. It saves RCC reset flags before clearing them. The only ISR increments a naturally aligned 32-bit tick count; all monitoring, queue, and transmission-buffer operations have one foreground owner. This avoids pretending that `volatile` alone makes a multi-field queue safe across interrupt contexts.

`monitor.c` compares the sampled levels once per observed tick. Raw edges are recorded separately from accepted stable transitions. A candidate must remain unchanged for five milliseconds of consecutive samples before replacing the stable state. A skipped sample increments the missed-sample counter and restarts candidate and dwell timing; an unobserved interval must never count as proof of continuous stability. Initial states are accepted immediately at startup and are not themselves measured transitions. The first raw interval is truncated at boot and excluded from shortest-pulse and short-pulse counts.

The queue holds 64 events. When it is full, new events are discarded, the dropped counter rises, and the event sequence still advances. Counters saturate rather than silently wrapping, except the explicitly modular sequence and millisecond timebase. The companion identifies sequence discontinuities as incidents, not exact lost-record totals. Reports have guaranteed scheduling opportunities even during chatter, preventing a flood of raw transitions from indefinitely hiding health information.

`drivers/serial.c` transmits at most one byte per foreground visit. It uses a bounded buffer and a 100 ms local transmit timeout; it never blocks the sampler waiting for the host. This timeout can detect a stuck peripheral, not whether a PC successfully consumed bytes. There is no transport acknowledgement. The independent watchdog uses LSI with prescaler 64 and reload 1023. Its interval depends on LSI tolerance. Failure to initialize the watchdog stops measurement rather than reporting success without the promised recovery mechanism.

Keep each measurement session below 24 hours and export before resetting. Millisecond subtraction supports ordinary timer-boundary crossings but uninterrupted intervals beyond a full 32-bit cycle are not supported. All measurement state is volatile. Power loss, watchdog reset, or manual reset clears it. A boot record declares a new session; if that record is lost, the app can also infer a restart from a backwards health timestamp that is not an ordinary wrap. This is a heuristic, not cryptographic session identification. If the serial stream is unreliable, restart the measurement deliberately.

## Wire protocol

Each ASCII line is `body*CCCC\n`. The body begins `LL1,` and the four hexadecimal checksum digits are CRC-16/CCITT-FALSE over the body only: polynomial 0x1021, initial value 0xFFFF, no reflection, and no final XOR. All numerical fields are unsigned decimal integers. Lines must fit the 256-byte firmware buffer including termination; the companion rejects overlong input and resumes only at the next newline.

| Type | Fields after type |
|---|---|
| B | reset flags, debounce milliseconds, hardware revision |
| E | sequence, timestamp ms, event kind, channel or pair index, level, previous duration ms |
| S | timestamp, channel, raw, stable, raw edges, stable changes, short pulses, longest open, shortest complete raw interval |
| P | timestamp, pair index, disagreement active, mismatch transitions, longest mismatch ms |
| H | timestamp, latest sequence, queue drops, missed samples, local TX failures |

Event kinds one, two, and three mean raw edge, stable transition, and pair transition respectively. Logic one means open; zero means closed. A shortest raw interval of 4294967295 means no complete interval has yet been observed. Pair counts exclude an initial disagreement present at startup. Status frames rotate every 100 ms through four channels, two pairs, and health, so a complete cycle is nominally 700 ms. Rows are not simultaneous snapshots; use their individual device timestamps when comparing them.

## Companion application and data handling

`app/ledger.py` is an appropriate desktop companion for a USB bench instrument: a dependency-free Linux curses application rather than a mobile app requiring platform-specific serial permissions. It provides three actual screens. The channel screen shows last-observed raw/stable levels, counters, pair information, and health. The event screen shows the newest bounded event history. The help screen states wiring and interpretation limits. Keys one, two, and three select screens; `e` exports; `q` quits cleanly.

The connection state distinguishes an opened TTY from recent valid records. Per-channel and per-pair age checks mark old rows stale even if other traffic continues. Physical disconnection is handled without terminating the interface unexpectedly. Reconnection is deliberately manual: quit, inspect the fixture, and reopen the selected port. This avoids accidentally joining a newly enumerated device to the old research session. The firmware implements no control channel, so UI actions cannot alter a target or falsely acknowledge an unimplemented command.

Exports are explicit, bounded views rather than complete captures. They include author attribution, record ages, connection state, error counters, and recent events. Creation uses mode 0600 and exclusive creation: an existing path is never overwritten. The application retains no persistent logs unless the operator exports them, and it sends no telemetry. Delete exported files using the workstation's normal retention procedure; ordinary deletion is not a promise of secure erasure on modern storage. Enclosure timings can still reveal sensitive operational details, so treat exports as engagement material.

## Build, run, and reproduce validation

Use Debian's ARM GCC 14.2.1 toolchain with newlib, KiCad CLI 9.0.2 and its footprints, and Python 3.13 for the tested configuration. The app uses only Python's standard library. The hardware scripts additionally use KiCad's `pcbnew` Python module; run the distribution Python that owns that package rather than an unrelated virtual environment. `TMPDIR` should point to a writable scratch directory for temporary verification outputs.

```sh
make -C firmware all test
python3 -m unittest discover -s tests -p 'test_app.py'
python3 tests/integration.py
python3 tools/check_hardware.py
python3 app/ledger.py --port /dev/ttyACM0 --export engagement-view.json
```

The firmware build produces an ELF, map, and binary under ignored `firmware/build/`. Host tests use AddressSanitizer and UndefinedBehaviorSanitizer. The integration test compiles a deterministic C fixture through the actual firmware encoder, then exercises replay, a pseudo-terminal serial connection, every screen, export permissions, overwrite refusal, disconnection, and exit. The fixture is explicitly synthetic test data; it is not a fabricated claim about a physical device. `--replay path.ll1` can inspect a saved raw stream and emits a JSON view; nonzero exit reports malformed or truncated input.

For initial flashing, use a separately installed ST-LINK-compatible programmer to program the ELF at its linked flash address, then perform a hardware reset. Do not change option bytes, disable debugging, or program a different Nucleo model by analogy. Confirm that the board is NUCLEO-F401RE and that its USART-to-ST-LINK solder bridges match the documented default arrangement. Flashing was not exercised in this software-only validation environment.

## Authorized research use cases and bring-up

A product-security engineer can compare two enclosure-switch placements while gently opening the same test lid. A penetration tester can demonstrate that a proposed five-millisecond debounce policy suppresses some sampled short openings without making claims about another controller's settings. A red-team evaluator working under written authorization can hand a defender reproducible mechanical observations that support a design review, rather than deploying an implant. A reliability engineer can distinguish chatter from sustained disagreement and use the evidence to improve the same security mechanism.

Before assembly, inspect every footprint against the purchased component. After assembly, with USB disconnected, check for shorts between 3V3 and ground and verify each resistor value. Attach only the Nucleo harness, power it, and measure the carrier rail. Confirm open inputs read high, then close one isolated switch at a time and verify only its expected channel changes. Confirm that closing both contacts in a pair ends disagreement. Compare timestamped transitions against a known laboratory stimulus and oscilloscope before relying on timing results. Record the actual board revision, component substitutions, ambient conditions, lead length, and firmware commit.

Do not deploy an untested board into a live system to compensate for missing bench validation. The device's intended operating envelope is low-energy, short-lead, indoor work with disconnected contacts. Any extension to industrial wiring, powered logic, outdoor cables, or certified alarm loops requires a separate front-end design, component review, and physical verification.

## References, licensing, and legal disclaimer

Pin assignments and the ST-LINK USART arrangement were checked against ST's **UM1724**, STM32 Nucleo-64 boards MB1136 user manual, revision 13, including the NUCLEO-F401RE connector tables and USART section. Register definitions were cross-checked against ST's STM32F401xE CMSIS device header; RM0368 is the corresponding reference manual. The files implement original project logic and a locally authored symbol library; installed KiCad footprints remain their upstream library assets.

Reference sources:

- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- https://pages.hmc.edu/brake/class/e155/fa20/assets/doc/Nucleo-64_User_Manual_UM1724.pdf
- https://github.com/STMicroelectronics/cmsis-device-f4/blob/master/Include/stm32f401xe.h

Author and creator of this design, firmware, application, and documentation: **jayis1**. Firmware is GPL-2.0-only; Python tools and application are MIT; hardware design follows the repository's CERN-OHL-S-2.0 convention. Third-party toolchains and installed library assets retain their own licensing.

**Authorized use only.** Obtain explicit written permission for the product and experiment being evaluated. Disconnect contacts only where authorized and where doing so cannot impair a safety or security function in service. Do not attach this device to equipment you do not own or have permission to test. Do not use it for covert surveillance, unauthorized entry, alarm bypass, or interference with emergency systems. The design is supplied without a guarantee of fitness, certification, or physical validation; the operator is responsible for lawful use, safe assembly, controlled testing, and responsible disclosure of findings.
