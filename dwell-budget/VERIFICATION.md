# Dwell Budget verification

Author: jayis1.

## Executed checks

- Target build: `make -C firmware all test`, Arm GNU GCC 14.2.1, Cortex-M4/thumb/soft-float, real STM32F401 linker layout and reset startup. Final ELF: 2,780 bytes text, zero initialized data, 10,864 bytes BSS. Binary conversion succeeded. Build uses `-Wall -Wextra -Werror`.
- Four native C suites passed: rolling-window oracle and policy boundaries; CRC known-answer and bounded encoder; production main-loop scheduling with substituted board functions; production serial driver with Linux memory-mapped register stand-ins. Address/undefined sanitizers cover the first three suites, and undefined-behavior sanitizer covers the fixed-address serial test.
- The rolling test compares 50,000 pseudo-random synthetic observations against an independently maintained ring model. Other assertions cover ordinary time wrap, saturation, duplicate ticks, acquisition gaps, missing consent, inhibit, budget boundary, and alarm grace behavior.
- Five Python tests passed through `python3 tests/test_app.py`: real C-encoder fixture interoperability, truncated-frame resynchronization, bounds/impossible status rejection, duplicate integrity, and safe replay/export files.
- One real Tk integration test passed through `xvfb-run -a /usr/bin/python3 tests/test_gui.py`, using Python 3.13.5. It opened a Linux PTY, received 121 synthetic C-encoded status records, exercised all three tabs, checked stale evidence, detected PTY hangup, preserved and exported live provenance after disconnect, and loaded replay data.
- KiCad 9.0.2 ERC: zero violations. DRC: zero violations and zero unconnected items. No rule exclusions were added to obtain that result.
- Fresh schematic XML, PCB pad connectivity, saved netlist, footprint library identities, and values were compared against the authored manifest: 14 components, 30 electrical pads, 9 nets.
- Schematic exported to SVG, rendered to PNG with rsvg-convert, and visually inspected: four distinct input networks, readable component references, and author/title metadata. The carrier intentionally contains no MCU because it uses the external Nucleo module.
- README contains 2,768 whitespace-delimited words. Production firmware has 514 lines in `.c` implementation files, excluding tests. Across production C and headers, 549 nonblank lines remain after stripping block comments. No generated filler is used to meet the firmware threshold.
- Read-only independent code review found no blocking issues in firmware rolling-window semantics, protocol agreement, or app connection lifecycle. That review did not substitute for the builds and GUI tests above.

## Corrections made during validation

The initial rolling-window test incorrectly expected the target alarm to become unnecessary immediately after ACTIVE dropped. The implemented contract correctly keeps the alarm required while the past activity remains over budget in the rolling window; the test expectation was corrected to include the missing-alarm finding. The first attempted SVG rasterizer was unavailable; rendering was completed with the installed rsvg-convert instead. System Python provides pcbnew; an unrelated language-server environment reporting that import missing is not the runtime used for CAD validation.

## Reproduction

From this device directory:

    make -C firmware all test
    python3 tests/test_app.py
    xvfb-run -a /usr/bin/python3 tests/test_gui.py
    /usr/bin/python3 tools/generate_hardware.py
    /usr/bin/python3 tools/check_hardware.py

The hardware generator regenerates native CAD and its XML netlist; local KiCad preferences are ignored. Synthetic telemetry is produced by `firmware/build/fixture` after the tests build it. This output has no physical measurement provenance and should only be used for application verification.

## Outstanding physical work

No board has been fabricated or assembled. No Nucleo has been programmed in this run. Physical GPIO, USB serial, oscillator accuracy, current draw, input leakage, target loading, connector wiring, power sequencing, ESD, EMC, environmental limits, or regulatory behavior have been verified on hardware. No security target has been exercised. Software tests with PTYs and memory-mapped stand-ins do not establish those properties.

Before deployment, validate the purchased module revision and pinout against ST UM1724 and the MCU register map against RM0368, inspect the assembled harness, verify the target-facing GPIOs remain inputs, measure known pulse trains, and compare timing to a calibrated analyzer. Confirm that the target's diagnostic adapter represents the protected state honestly. The design is not fabrication-qualified, certified, or an enforcement mechanism.

Reset inference remains heuristic without a persistent boot identifier. Sampling can miss sub-millisecond pulses and cannot establish continuous-time occupancy. The operator must not convert an incomplete window, stale view, or missing activity into a clean security verdict. These limitations are part of the evidence contract, not pending software features.
