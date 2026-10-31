# Owner Bind verification record

Author: jayis1.

## Scope and readiness

Verification was performed in standalone-owner mode. Advisory reports were treated as recommendations, not executed evidence; no full collective review is claimed. The active checkout was verified as `https://github.com/jayis1/hacker-devices.git`, with remote HEAD pointing to main. `git pull --ff-only origin main` completed before creation. No daily-ideas content or repository was modified.

The design is an unbuilt engineering prototype. Software execution and CAD checks below succeeded. They do not establish physical electrical safety, calibrated timing, actual MCU programming, fabricated board quality, or target correctness.

## Executed software checks

- `make -C owner-bind/firmware clean all test`: passed.
- ARM GCC 14.2.1 linked the Cortex-M4 firmware and produced ELF, binary, and map outputs. Size: 3328 bytes text, zero initialized data, 1660 bytes BSS. The linker reserves stack headroom in 96 KiB SRAM.
- Host GCC 14.2.0 engine suite ran with AddressSanitizer and UndefinedBehaviorSanitizer. An independent two-entry ownership-array oracle checked 4136 operation branches through depth seven, covering both slots, both clients, and both operations along legal prefixes.
- Explicit regressions passed for same-client reuse, migration, either release order, wrong-client release, a client attempting a second slot, held strobes, short rejected pulses, simultaneous strobes, raw opposite-strobe interference, client chatter, duplicate timestamps, ordinary timestamp wrap, missed samples, and transactional counter exhaustion while a binding remains active.
- C serialization bounds, invalid-state rejection, CRC reference vector, and terminal evidence retention passed.
- UART driver tests passed for bounds, busy submission, byte transfer, timeout, recovery, and timer wrap using a Linux memory-mapped register model. This is not a physical UART test.
- Production main-loop tests passed for lost boot transmission, repeated late-attachment policy, failed submission, transmission timeout diagnostics, and watchdog startup failure before telemetry. Board/transport inputs were controlled test doubles, not real peripherals.
- `xvfb-run -a python3 -m unittest discover -s owner-bind/tests -p test_app.py -v`: all 15 tests passed on Python 3.13.5.
- Application tests include the real C encoder fixture, CRC-valid contradictory client bindings, counter/state bounds, client-change transition checks, same-client reuse, retained history, framing resynchronization, incompatible policy, reset detection, partial EOF, FIFO rejection, private JSON export, overwrite and symlink refusal, duplicate-timestamp freshness, serial-open failure, and halted evidence immutability.
- A real Tk event loop exercised all three screens through a Linux pseudoterminal: connect, receipt, refusal of a second connection, hangup, retained evidence, real-encoder replay, and JSON export/readback. The data is synthetic. Physical ST-LINK USB operation has not been tested.
- `python3 -m compileall -q owner-bind/app`: passed.

Tests were written and run before the new engine and companion contract modules existed; those initial runs failed on the absent modules. Final clean runs passed after implementation. The inherited support code was reviewed and exercised again rather than assuming earlier project results applied.

## Executed CAD checks

KiCad CLI and pcbnew version 9.0.2 generated and checked the native project.

- `/usr/bin/python3 owner-bind/tools/generate_hardware.py`: generated the project, embedded library, connectivity manifest, and schematic netlist.
- `/usr/bin/python3 owner-bind/tools/check_hardware.py`: passed.
- Schematic ERC: zero violations.
- PCB DRC: zero violations and zero unconnected items.
- Fresh schematic netlist, saved netlist, actual PCB pads, and manifest agreed on 14 components, 30 electrical pads, and 9 named nets. Values and full footprint identifiers also matched.
- The schematic was freshly exported to SVG, rendered to PNG, and visually inspected. SLOT, CLIENT, ALLOCATE, RELEASE, ground-only headers, resistor values, title, and jayis1 attribution were readable without net-label/pin-number overlap.

The carrier is passive and cable connected to an external NUCLEO-F401RE. ERC does not validate that external board, USB source, ground offsets, protection, jumper configuration, or cable harness. Native routed files are not a claim of fabrication qualification.

## Content and review gates

`python3 owner-bind/tools/check_release.py` passed: README contains 3201 whitespace-delimited words and firmware contains 562 nonblank production C lines after comments are removed, excluding tests and headers. Applicable authored and generated deliverables contain jayis1 attribution. Build outputs and local KiCad preferences are ignored. Source self-review checked input bounds, no target-facing output or command path, whole-sample overflow behavior, client exclusivity, arbitrary release order, replay limits, export permissions, and preservation of uncertainty. No automated workflow or dependency service was added.

## Outstanding physical work and retrieval limitation

Fresh retrieval of ST UM1724 failed: the web extractor had no extraction backend, direct HTTPS returned an HTTP/2 error, and an HTTP/1.1 retry timed out. The inherited MB1136 connector map must therefore be confirmed against the purchased module's manufacturer manual before connection. RM0368 peripheral mappings and module assumptions are reused platform definitions, not newly measured hardware properties.

Before bench use, complete assembly inspection, continuity, resistor checks, connector orientation, grounding, input leakage and thresholds, power sequencing, ST-LINK routing, MCU programming, watchdog behavior, waveform correlation, oscillator calibration, sustained serial load, ESD assessment, and enclosure/strain-relief review. Compare against independent target traces and a logic analyzer. No physical measurements, fabricated boards, or field results are represented by the synthetic fixtures.
