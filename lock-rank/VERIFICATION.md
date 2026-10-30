# Lock Rank verification record

Author: jayis1.

Scope: software and CAD validation of an unbuilt revision-A prototype. This run used standalone-owner execution, not full collective review. Advisory findings were evaluated by the owner; no peer performed repository writes.

## Executed checks

- GNU Arm Embedded GCC 14.2.1 linked the Cortex-M4 image with `-Wall -Wextra -Werror`, the supplied startup vector, linker map, and newlib nosys specification. The ELF reports 3364 bytes text, zero initialized data, and 1692 bytes BSS. Target linking is not physical programming or execution.
- `make -C firmware all test` passed the engine, serial-register-model, and production-main scheduler tests. The engine suite includes all 256 steady before/after ownership-mask combinations against a separate classification oracle, independent-client chatter, gap recovery, wraparound, terminal exhaustion, malformed snapshots, and encoder capacity rejection. AddressSanitizer and UndefinedBehaviorSanitizer were enabled for the engine; serial and main tests used UndefinedBehaviorSanitizer.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_app.py' -v` passed 11 tests. Coverage includes production-C fixture decoding, real Tk screens through a Linux PTY, disconnect and replay, double-connect protection, private export and overwrite refusal, FIFO/oversize replay refusal, terminal restoration, CRC-valid impossible evidence, truncated framing, parser resynchronization, duplicate timestamps, reception pause, and reset freezing.
- Two advisory app findings were reproduced as failing tests before correction: impossible missed-tick/coverage growth, and qualification disappearing without gap or halt. Both regression tests now pass. These checks establish necessary consistency, not cryptographic authentication or exact waveform realizability.
- KiCad CLI 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. Fresh schematic XML connectivity exactly matches PCB pads and the authored manifest: 14 components, 30 pads, 9 nets. Values and full library footprint identifiers also match. Saved netlist connectivity matches the fresh export.
- The schematic was exported to SVG, rendered to PNG, and visually inspected for symbol separation, net-label readability, pin wiring, and author/title metadata. No visible overlap was identified. This does not replace assembly review.
- Release checker passed: 2970 README words and 545 nonblank, noncomment production C lines, excluding tests and headers. All deliverables contain jayis1 attribution in format-compatible comments or metadata. Python source compilation and Git whitespace checks passed.

## Reproduction

Run from this device directory:

    make -C firmware all test
    xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_app.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py

Hardware regeneration, when desired:

    /usr/bin/python3 tools/generate_hardware.py

Requires GNU Arm Embedded GCC/newlib/binutils, host GCC/make, Python 3 with Tk, Xvfb, and KiCad 9 with system-Python pcbnew and standard footprint libraries. No remote package download or application dependency install is required by these commands. Generated build outputs and KiCad preferences remain ignored. The reproducible synthetic fixture is `firmware/build/fixture`; its output is not a physical capture.

## Unverified physical acceptance

No fabrication, assembly, flashing, GPIO electrical qualification, voltage-margin measurement, cable-loading measurement, physical timing calibration, actual watchdog-reset trial, physical USB bridge test, environmental test, ESD/surge assessment, or regulatory certification occurred. The external Nucleo is part of the complete assembly and is not present on the passive carrier PCB. Confirm its actual revision and jumpers before wiring. A passing passive schematic ERC cannot validate an external power source.

The ownership adapter is trusted and intentionally low-speed. Lock Rank does not observe blocked requests, recursive depth, internal mutex identities, additional clients, or resources beyond the declared pair. No rank finding does not prove deadlock freedom. Physical research requires written authorization and the README's bench acceptance workflow.
