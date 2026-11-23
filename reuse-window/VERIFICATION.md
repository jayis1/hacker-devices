# Reuse Window verification

Author: jayis1.

## Executed checks

- `make -C reuse-window/firmware clean all test`: passed using arm-none-eabi-gcc 14.2.1. Target ELF linked for Cortex-M4; GNU size reported text 3760 bytes, data 0, BSS 1724. BIN generation succeeded.
- Host C tests passed: 43,048 independent timestamp-list oracle transitions, rolling third-admission regression, exact boundaries, per-tag independence, rollback, gap, wrap and framing. Serial tests covered bounds, timeout, recovery and wrap. Main-loop tests covered late attachment, lost boot announcement, transmit timeout and watchdog startup failure. Sanitizers are enabled by the Makefile for applicable host binaries.
- `xvfb-run -a python -m unittest discover -s reuse-window/tests -p test_app.py -v`: 16 tests passed on Python 3.14.9. Tests consume real C-encoder synthetic fixtures, including 2,458 dense snapshots and sparse subsequences. The actual Tk event loop and serial PTYs exercise live/replay screens, export, disconnection and failure paths. These are simulated transports, not physical USB measurements.
- `/usr/bin/python3 reuse-window/tools/check_hardware.py`: passed with KiCad 9.0.2. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic netlist, saved netlist, PCB pad connectivity, component values, footprint identifiers and manifest agree: 14 components, 30 pads, 9 nets.
- Exported the schematic with `kicad-cli sch export svg` and rendered it with rsvg-convert. Visually inspected all four input channels, ground and MCU headers, net-label placement and jayis1 title-block attribution. CairoSVG was unavailable; the installed rsvg-convert supplied the successful render instead.
- Release check confirms 2,893 README words and 612 nonblank, noncomment production C lines, excluding headers and tests. It verifies attributed deliverables, required native KiCad files, a private-key marker check and Git whitespace checks. Build products, bytecode and KiCad local preferences are ignored.

## Review and correction

A read-only code review identified an impossible sparse transition the companion accepted: enough total elapsed time had passed to permit renewal, but the retained newest deadline implied that renewal happened before permission existed. Added a failing regression and reproduced that acceptance, then constrained the implied latest admission time against its earliest legal offset. Applied the same check to the penultimate retained admission and added a separate regression. The complete 16-test app suite, including legal dense and sparse C histories, passes after the correction. Endpoint checks remain necessary bounds, not an authenticity guarantee or complete history reconstruction.

## Publication scope and limitations

The configured repository is jayis1/hacker-devices and its remote default branch is main. The required fast-forward pull reported already up to date before changes. The pre-existing untracked reuse-window directory was established from the originating task transcript as interrupted output of this same task; it was recovered rather than discarded. No other device or shared repository infrastructure is part of this change. No daily-ideas content or task branch is involved.

This is degraded standalone-owner execution with advisory review, not a completed collective review. No hardware was fabricated or programmed. No electrical thresholds, input loading, oscillator timing, current consumption, ESD behavior, thermal behavior, regulatory compliance or real target interaction was measured. CAD checks validate the passive carrier's internal connectivity, not the external module's assembly or purchased board revision. Physical bring-up and authorized target validation remain explicitly required before use.
