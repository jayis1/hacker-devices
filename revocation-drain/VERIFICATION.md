# Verification record

Author: jayis1

## Verified software and CAD results

- GNU Arm Embedded GCC 14.2.1 (20241119): Cortex-M4 target linked with warnings treated as errors; binary generated. `arm-none-eabi-size` reports 2780 bytes text, zero bytes initialized data, and 856 bytes BSS. Linker reserves at least 4096 bytes below the stack boundary.
- Native GCC 14.2.0: production drain engine, wire encoder, and main-loop tests pass with AddressSanitizer and UndefinedBehaviorSanitizer. UART MMIO behavior test passes with UndefinedBehaviorSanitizer. The register mapping in that test is synthetic, not a hardware peripheral.
- Drain tests cover successful retirement, simultaneous admission/revocation, early acknowledgment at the quiet boundary, later acknowledgment recovery, post-acknowledgment activity, exact age-500 acceptance, age-501 timeout, missing startup baseline, gaps, withdrawal, cross-input chatter, duplicate ticks, timer wrap, counter exhaustion and watchdog fault. One million deterministic pseudorandom samples check production invariants after every sample.
- Main-loop test verifies repeated policy announcements, one-shot boot record, serial timeout reporting, and watchdog-failure state before status output. Board functions are explicit host test boundaries, not MCU execution.
- Python 3.13.5: six model/replay/export tests pass. Actual production C encoder output is parsed by the app, with one completed synthetic trial, zero parser errors, and no frozen-evidence reason.
- Two Tk GUI/serial tests pass under Xvfb. They exercise actual event-loop widgets through a Linux pseudo-terminal, history rows, stale telemetry, hangup, retained live provenance after disconnect, explicit private JSON export, replay replacement, stale-summary clearing, FIFO rejection and terminal restoration after injected constructor failure.
- KiCad 9.0.2: zero schematic ERC violations, zero PCB DRC violations, zero unconnected items. Fresh schematic netlist matches board and manifest across 14 components, 30 pads, and 9 nets, including values and footprint identities. Saved netlist is checked separately.
- Schematic SVG exported with KiCad and rendered to PNG; visual inspection shows all four labeled channels, the ground and Nucleo headers, resistor pairs and target connectors, with no gross overlap or missing channel.
- README contains 3193 whitespace-delimited words. Firmware contains 538 physical C-source lines and 575 nonblank C/header lines after removing block comments. Tests, CAD generators and vendored dependencies are not counted as firmware.
- Read-only review found no blocker in policy temporal semantics, conservation, parser handling or disconnect behavior. The owner reran all tests after adding exact-deadline and pseudorandom regression coverage.

## Reproduce from this directory

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p test_app.py
xvfb-run -a python3 -m unittest discover -s tests -p test_gui.py
/usr/bin/python3 tools/check_hardware.py
```

To regenerate the CAD first, run `/usr/bin/python3 tools/generate_hardware.py`. The generator requires installed KiCad footprint libraries. Do not commit temporary build outputs or local KiCad preferences. No Python package installation or Node dependency installation is needed.

## Not verified

No board has been fabricated, assembled, flashed or connected to a physical target during this verification. Electrical loading, short-circuit behavior, pin compatibility on the exact purchased board revision, power sequencing, oscillator accuracy, UART timing, watchdog timing, ESD, EMC, mechanical fit and regulatory compliance remain laboratory acceptance gates. CAD checks do not establish those properties.

Synthetic fixtures and pseudo-terminals are software tests, not real measurements. No claim of authenticated evidence, complete event capture, safety certification, globally unique invention, or enforceable revocation is made. See README.md for the required low-voltage authorized bench scope and limitations.
