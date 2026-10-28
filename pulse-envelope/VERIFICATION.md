# Verification record

Author: jayis1. Engineering prototype; software/CAD evidence only.

## Executed checks

- `make -C firmware all test`: target-linked with Arm GNU GCC 14.2.1, Cortex-M4 soft-float, warnings as errors. ELF section sizes: text 3204 bytes, data 0 bytes, BSS 1764 bytes. Startup/linker included; no claim of actual board execution.
- C engine tests with AddressSanitizer and UndefinedBehaviorSanitizer: minimum 9/10 ms, maximum 200/201 ms, held-long censoring, 49/50 ms recovery, acquisition-gap conservation, high-at-attachment, four independent channels, unrelated chatter, rejected narrow pulse, duplicate ticks, timestamp wrap, terminal counter exhaustion, encoder short-buffer rejection, and 100000 deterministic random ticks passed.
- Production main-loop test with substituted hardware: recurring policy, boot/status scheduling, acquisition with busy TX, transmitter timeout accounting, and failed watchdog initialization passed.
- UART driver test with a Linux mapped register page: bounds, ownership, timeout, recovery, timestamp wrap passed. Not an actual USART peripheral test.
- `xvfb-run -a /usr/bin/python3 tests/test_app.py`: seven unittest cases passed. Includes actual Tk event loop and real Linux PTY, all three view contents, disconnect, replay, and exclusive private export. Fixtures come from the production C engine/encoder, not fabricated physical measurements.
- KiCad 9.0.2 generation and `tools/check_hardware.py`: zero ERC violations, zero DRC violations, zero unconnected items. Exact fresh schematic/PCB/manifest comparison passed for 14 components, 30 pads, and 9 nets, including values and footprint library identities.
- Schematic exported to SVG, rendered through rsvg-convert, and visually inspected: readable separated lane nets, real symbols, connected wires, and jayis1 title-block credit.
- README: 3042 whitespace-separated words. Production firmware: 515 nonblank C lines after removing block comments, plus 101 corresponding header lines. Tests and generated artifacts excluded from those code counts.

## Scope and known limits

This was standalone-owner execution with advisory peer reports, not a completed collective independent review. Board support and passive topology reuse reviewed repository material authored by jayis1; the pulse policy, checks and documentation are specific to this device.

Physical fabrication, programming, real GPIO behavior, actual USB transport, power sequencing, signal thresholds, input loading, oscillator and watchdog timing, calibration, ESD, EMC, and regulatory compliance are untested. The passive carrier is not isolated or protected for unknown voltage sources. The manufacturer manual retrieval failed (HTTP/2 error, then HTTP/1.1 timeout). Verify Nucleo revision, header mapping, ST-LINK virtual-COM bridges and clock assumptions against UM1724/RM0368 before assembly. No fresh external datasheet review is claimed.

The width model uses qualified levels, excludes unqualified glitches, and censors widths at 201 ms. It is not complete electrical edge capture. Acquisition gaps abandon active observations. CRC is not authentication. Reset inference cannot establish device identity or detect every reset. The app stores bounded snapshots, not a full immutable capture. New session/replay actions replace old evidence: export first. These limits are part of the measurement contract, not failures hidden by successful build or CAD commands.

## Reproduction

Run all commands from this device directory:

    make -C firmware clean all test
    xvfb-run -a /usr/bin/python3 tests/test_app.py
    /usr/bin/python3 tools/generate_hardware.py
    /usr/bin/python3 tools/check_hardware.py

Build outputs and KiCad local preferences are ignored. Every committed original file credits jayis1 using valid comments or metadata. No task branch, automation workflow, third-party vendored source, or changes to other devices are required.
