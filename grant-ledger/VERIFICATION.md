# Grant Ledger verification record

Author: jayis1

## Executed software and CAD checks

Commands were run from the repository root unless noted. These results describe software execution and design-file checks, not physical measurements.

- `make -C grant-ledger/firmware clean all test`: passed. ARM GNU GCC 14.2.1 linked the actual STM32F401RE target sources and produced ELF and binary. Linked size: 2620 bytes text, zero initialized data, 1388 bytes BSS. Warnings are errors.
- Native production transaction engine and wire encoder tests passed under AddressSanitizer and UndefinedBehaviorSanitizer. Covered startup-high rejection, qualification interruptions, same-tick rejection, ordered request/grant/use, double consumption, orphan grant, overlap, tied edges, revoke precedence/release, phase-window renewal, exact expiry boundary, skipped samples, all-low recovery, tick wrap, queue saturation, counter saturation, CRC known answer, and short encoder buffer.
- Unmodified production UART driver passed Linux MMIO-model tests under UndefinedBehaviorSanitizer: null/empty/oversized submissions, busy rejection, actual byte progression, stalled transmission, timeout across wrap, and recovery. This does not exercise a physical USART.
- `python3 -m unittest discover -s grant-ledger/tests -p test_app.py`: nine tests passed. Covered CRC, malformed and oversized input, partial-frame resynchronization, bounded history, freshness, private exclusive export, restart freeze, replay labels, named input ordering, incompatible boot policy, status wrap, and delayed event timestamps.
- `python3 grant-ledger/tests/integration.py`: passed. The actual C fixture encoder fed the actual application. Verified replay JSON and expected accepted/no-credit counts. Separate Linux serial and UI pseudo-terminals exercised live ingestion, overview/events/health screens, private JSON export, overwrite refusal, disconnect indication, and clean exit. Fixture signals are synthetic.
- `/usr/bin/python3 grant-ledger/tools/generate_hardware.py`: generated native KiCad 9 files using installed library footprints and authored symbols.
- `/usr/bin/python3 grant-ledger/tools/check_hardware.py`: passed with KiCad 9.0.2. ERC zero violations, DRC zero violations, zero unconnected items. Fresh schematic XML netlist and actual PCB pads agree with the manifest for all 14 components, 30 pins, and 9 nets, including values and full footprint identifiers.
- Schematic exported to SVG and visually inspected in a browser: four separate labeled channels, pull-down and series resistors, ground/header mapping, and author title block present.
- README whitespace word count: 3229. Production `.c` files: 509 physical lines. Production `.c` and `.h` files: 552 nonblank, non-comment-start lines, excluding tests. These counts are supplementary checks, not proof of correctness.
- Every deliverable checked for jayis1 attribution. No credential-pattern matches or symlinks were found. Generated builds, local KiCad preferences, and Python caches are ignored.

## Review findings resolved

A read-only independent review found the overview's binary input label order was reversed. The app now prints each named input explicitly, and a failing-then-passing regression verifies REQUEST-only display. The app now also latches an incompatible boot-policy warning and prevents fresh status until a supported boot record. Additional timing-boundary and reset/wrap tests passed. Request/grant counter semantics are documented as policy-processed edges, not a physical pulse count.

## Not verified

No board was fabricated, assembled, programmed, or connected to a target. Actual GPIO threshold margins, loading, cable parasitics, inter-port skew, oscillator accuracy, acquisition timing, USB behavior, watchdog recovery, power sequencing, back-power, ESD, EMC, regulatory compliance, and calibration remain unverified. No claim of safety certification, authenticated evidence, complete event capture, or fabrication readiness is made.

The Nucleo board support and passive carrier topology are adapted from the repository's jayis1-authored receive-only designs. Register mapping is implemented for STM32F401RE/RM0368; no new vendor-document revision audit or hardware conformance test was performed in this run. Recheck current ST board documentation and component dimensions before procurement. Builds/tests ran on the local toolchain, not a certified hermetic or credential-isolated build environment.

The instrument deliberately cannot prove a target's debug GPIOs faithfully represent internal authorization. Use an external logic analyzer and target code review to corroborate findings. Fast or ambiguous signals outside the paced bench contract remain unknown rather than validated.
