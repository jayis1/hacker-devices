# Toggle Mailbox verification

Author: jayis1.

## Scope

This release was executed by a standalone owner using advisory review, not a full collective hardware review. The verified destination was jayis1/hacker-devices, configured default branch main. The checkout was clean before work and `git pull --ff-only` reported already up to date. No daily-ideas content or other device directory was changed. The only new project is toggle-mailbox.

This is an unbuilt prototype design. Software execution and CAD consistency are verified below; fabricated hardware, MCU flashing, electrical performance, and target integration are not.

## Executed checks

- `make -C firmware clean all test`: passed. ARM GNU GCC 14.2.1 linked the actual Cortex-M4 target image with warnings treated as errors. Final ELF reports 3408 bytes text, zero initialized data, and 1676 bytes BSS. Startup, GPIO input configuration, USART2 transmit, watchdog, engine, and serializer are linked, not replaced by a host-only stub.
- Host GCC 14.2.0 engine/encoder tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. Tests exercise both initial phase polarities, four payloads, repeated handshakes, raw hold violations, acknowledgment-endpoint hold, overtaking, overlap, orphan acknowledgment, insufficient setup, startup with unequal phases, independent qualification under data chatter, rejected request glitches, inclusive deadline and deadline-plus-one, acquisition gap, timer wrap, transactional counter exhaustion, terminal serialization, and invalid active payload. Randomized input stress checks invariants after each sample.
- UART register-model tests passed under UndefinedBehaviorSanitizer: invalid submissions, busy refusal, byte order, timeout, recovery, and timer wrap. This models registers in Linux memory and is not physical USART validation.
- Production scheduler tests passed: lost boot submission does not prevent recurring policy, status continues, transmit failures are counted, and watchdog initialization failure is reflected before telemetry.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: all 13 tests passed with Python 3.13.5. Tests use real C-encoder fixture bytes, a real Tk event loop, and Linux pseudo-terminals. They cover all screens, serial hangup, double connection refusal, replay, export, mode 0600, overwrite refusal, failed-connection evidence retention, CRC/framing, partial tail, file limits/FIFO refusal, impossible records, rollback, duplicate timestamps, policy mismatch, reset, host pause/backlog, and post-correlation-loss immutability.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native KiCad schematic, PCB, project, symbol library, manifest, and XML netlist. KiCad 9.0.2 was used.
- `/usr/bin/python3 tools/check_hardware.py`: passed. ERC found zero violations. PCB DRC found zero violations and zero unconnected items. Fresh schematic export, saved netlist, PCB pads, values, full footprint identifiers, and manifest agree for fourteen components, thirty electrical pads, and nine nets.
- Schematic and combined copper/silkscreen/outline SVG exports were rendered using rsvg-convert and visually inspected. Signal labels, resistor lanes, ground connections, connector references, board outline, electrical warning, and author/title were visible. The rendering fallback used rsvg-convert because the optional cairosvg Python package was unavailable; no dependency installation was needed.
- README was measured at 3286 whitespace-delimited words. Production C/header files contain 640 nonblank lines after removing block comments, excluding tests and build products. These counts establish the requested minimums, not correctness by themselves.
- Authored deliverables include jayis1 in valid source comments, documentation credits, or structured metadata. Generated build products, Python caches, and KiCad local preferences are excluded by the project .gitignore.

## Review corrections

Read-only advisory review identified two reproducible parser-contract gaps. Both were reproduced with failing tests before correction. An active snapshot now requires qualified data to equal captured data in both C and Python validators. Once a prior snapshot establishes correlation loss, the companion refuses changes to handshake accounting and captured payload while allowing documented qualification, gap, time, and transport diagnostics. Regression tests confirm rejection of fabricated post-loss completions and acceptance of later gap diagnostics. The owner applied and verified all changes.

No test fixture is a physical measurement. A progressing live transport can report an invalid acquisition segment; users must inspect ready, halted, and fault fields separately. Snapshot validity checks are necessary constraints, not cryptographic authenticity or a complete formal reachability proof.

## Outstanding physical acceptance

Before fabrication or use: verify current manufacturer documentation against the purchased Nucleo revision; confirm CN8 pin mapping, USART solder bridges, power jumpers, reset clock, component dimensions, drill fit, and harness continuity. Check shorts, voltage compatibility, common-ground offset, loading, input leakage, power sequencing, and back-power. Verify target-facing pins remain inputs. Program through supervised ST-LINK and compare paced known-good/bad vectors with an independent analyzer. Calibrate timing and test watchdog recovery, actual USB disconnect behavior, ESD/EMC, thermal limits, and enclosure support. No isolation, protection certification, safety role, or field-deployment approval is claimed.
