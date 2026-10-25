# Verification — Reset Quarantine revision A

Author: jayis1.

## Executed software and CAD checks

- ARM GNU compiler 14.2.1 linked the Cortex-M4 firmware with warnings as errors. The linked image reports 2,808 bytes text, zero initialized data, and 1,372 bytes BSS. An actual ELF and binary were produced; neither is committed.
- `make -C firmware clean all test` passed policy/encoder tests with AddressSanitizer and UndefinedBehaviorSanitizer, the production serial-driver test with UndefinedBehaviorSanitizer, and the production-main scheduling test with both sanitizers.
- Policy regressions cover cold attachment, clean reset, minimum settling boundary, access concurrent with deadline, allowed later access, readiness chatter, stale ready under reset, revocation, fault lockout, reset recovery, acquisition gaps, ordinary uptime wrap, saturating age/counters, subqualification pulses, queue overflow, and bounded CRC encoding.
- Six Python application unit tests passed under Python 3.13.5. They cover policy/freshness, reset inference, malformed and fragmented records, incompatible policies, bounded event history/private export, and serial-terminal restoration including constructor failure.
- `python3 tests/integration.py` passed. Real production C-encoder output was processed by the real application through replay and separate Linux serial/UI pseudo-terminals. Screen selection, private export, overwrite refusal, late policy recovery, disconnect, and clean exit were exercised. Inputs were synthetic; no physical USB device was used.
- KiCad CLI 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items.
- Fresh schematic netlist, saved XML netlist, manifest, and actual PCB pads agree on 14 components, 30 pins, and 9 nets, including values and full footprint library identifiers.
- Schematic SVG export succeeded and its raster rendering was visually inspected. Component rows, signal labels, harness connectors, and the author/title block are legible. This does not establish electrical safety.
- Independent read-only review found no verified correctness defect in firmware, application, tests, or hardware generator. Firmware test binaries and application unit tests were independently rerun. The owner performed all design-file writes.
- README contains 3,235 whitespace-delimited words. Firmware contains 505 C-file lines and 618 C/header lines; after removing block comments and blank lines, 548 C/header lines remain. Tests are excluded from these firmware counts.
- Every deliverable contains jayis1 attribution in format-valid comments or metadata. Generated build products and local KiCad preference files are ignored.
- A fresh isolated export of the staged device sources passed the complete target build, firmware tests, app unit tests, interactive PTY integration, and CAD checks without relying on the original directory's build products.

## Reference cross-check and retrieval limits

The fetched ST cmsis-device-f4 STM32F401xE header agrees with the GPIO, USART2, watchdog, and RCC peripheral base addresses used here. A previously downloaded local UM1724 manufacturer manual was inspected for the NUCLEO connector and virtual-COM mapping after a fresh ST PDF download timed out. The current run does not claim a successful fresh manual download. The four harness inputs are CN8 A0/PA0, A1/PA1, A2/PA4, and A3/PB0; PA2 serves USART2 TX to ST-LINK with the documented bridge arrangement. Purchased-board revision and bridge continuity must still be checked before wiring.

## What is not verified

No PCB was fabricated or assembled. The target image was not flashed onto a processor. Input thresholds, power sequencing, ground currents, signal loading, oscillator accuracy, interrupt jitter, watchdog timing, ESD, EMC, environmental tolerance, and regulatory compliance are unmeasured. The passive carrier is neither isolated nor overvoltage protected. CAD validation covers the carrier, not the external development board. The design is not certified, manufacturing-qualified, or suitable for safety-critical enforcement.

No cryptographic authentication or persistent boot identity exists. A dishonest diagnostic output can misrepresent enforcement; a malicious serial source can forge records. Subqualification pulses and submillisecond event ordering remain outside the measurement guarantee. No absence-of-vulnerability claim follows from a quiet display.

## Reproduction

From this device directory:

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p test_app.py -v
python3 tests/integration.py
/usr/bin/python3 tools/check_hardware.py
```

Regeneration additionally requires `/usr/bin/python3 tools/generate_hardware.py` and the standard KiCad footprint libraries. Independent review, fabrication review, and physical bench validation remain separate gates.
