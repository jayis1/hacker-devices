# Entropy Sentry verification record

Author: jayis1.

## Executed software and CAD gates

Repository destination was verified as jayis1/hacker-devices, configured remote default branch main. The starting worktree was clean and `git pull --ff-only origin main` completed before implementation. Device directory names and README purposes were inventoried; the README describes the distinct health-test-to-release contract and acknowledges shared carrier infrastructure. No daily-ideas files or repository automation were changed.

Tool versions used: arm-none-eabi-gcc 14.2.1 (20241119), host GCC 14.2.0, Python 3.13.5, KiCad CLI 9.0.2. No Python package dependencies are required for the companion application. Hardware generation requires the system pcbnew bindings and installed standard footprint library.

Executed from the repository root:

- `make -C entropy-sentry/firmware clean all test`: linked the real Cortex-M4 image with warnings treated as errors. ELF sections reported text 2864 bytes, data 0, BSS 408; main-loop state and frame buffers additionally occupy stack, for which the linker reserves at least 4096 bytes.
- `make -C entropy-sentry/firmware all test`: final expanded regression run passed. The health and production-main tests use AddressSanitizer plus UndefinedBehaviorSanitizer. The memory-mapped UART test uses UndefinedBehaviorSanitizer.
- `python3 -m unittest discover -s entropy-sentry/tests -p test_app.py -v`: all six application tests passed.
- `python3 entropy-sentry/tests/integration.py`: real C encoder to replay export and late-attachment replay passed; separate serial and UI pseudo-terminals exercised the real curses application, screens, private export, overwrite refusal, unplug handling, and exit.
- `python3 -m py_compile entropy-sentry/app/audit.py entropy-sentry/tests/*.py entropy-sentry/tools/*.py`: passed.
- `/usr/bin/python3 entropy-sentry/tools/generate_hardware.py`: generated native KiCad artifacts and a real schematic XML netlist.
- `/usr/bin/python3 entropy-sentry/tools/check_hardware.py`: zero ERC violations, zero DRC violations, zero unconnected items; exact consistency for 14 components, 30 pins, and 9 nets, including values and footprint library identifiers.
- KiCad schematic SVG export, PNG conversion, and visual inspection: signal labels BIT/CLOCK/ALARM/RELEASE, separate pulldown/series elements, connector mapping, and author/title block were readable. This is not an assembly inspection.

The README contains 3273 whitespace-delimited words. Production C sources and headers contain 599 nonblank, noncomment lines, excluding tests, build artifacts, and linker script. Every published file contains jayis1 in a format-valid comment, documentation field, or supported metadata. Build artifacts, bytecode, and KiCad local preferences are excluded.

## Regression scope

Health tests cover run lengths 31/32, proportion windows with 48/49 reference matches, full-window qualification, persistent faults after later passing data, release-before-qualification episode accounting, setup and rate faults, acquisition gaps, fresh-window recovery, clock stalls, the inclusive 100-millisecond boundary, a rising edge on the first overdue tick, ordinary uptime wrap, saturating counters, queue overflow and ordered draining, bounded encoding, and the standard CRC check vector.

Production-main tests exercise loss of the one-shot boot transmission, independently repeating policy announcements, periodic status, watchdog feeding, and transmitter-failure accounting. UART tests exercise rejected empty/oversized submissions, busy rejection, unavailable transmitter, finite timeout, uptime wrap, and subsequent recovery. These are software peripheral models, not measured USART or watchdog behavior.

Application tests cover real encoder frames, private exclusive export, framing recovery after a truncated record, oversized input, unsupported policy, signed/overflow numeric rejection, status rollback, duplicate statuses not refreshing freshness, disappearing sticky fault masks freezing evidence, and actual Linux PTY hangup. Integration tests explicitly distinguish synthetic replay from live transport simulation; no physical measurements are presented.

Independent read-only review identified three correctness issues: duplicate frames renewing freshness, sticky masks disappearing without restart detection, and clock-only timeout erasing independently observed BIT stability. Each issue was reproduced by a failing regression, corrected, and rerun successfully. Automatic event-sequence reconciliation was not added; the README documents manual sequence inspection and the relevant loss counters.

## References and limitations

The manufacturer PDF download was unavailable in this environment (HTTP/2 error followed by an HTTP/1.1 timeout). Search retrieval of the manufacturer's UM1724 Table 16 corroborated the specific NUCLEO-F401RE CN8 mapping and PA2 USART2 assignment. The reference URL is retained in the README; this is not a claim that the complete manual was downloaded or every board revision inspected.

Physical fabrication, component procurement, connector orientation on assembled hardware, programming, oscillator tolerance, sampling jitter, analog setup/hold margins, power sequencing, input leakage, USB current, thermal behavior, ESD/EMC, environmental qualification, and watchdog recovery remain unverified. The input resistors do not constitute overvoltage protection, and common-ground operation provides no isolation. No manufacturing-readiness or regulatory certification is asserted.

No entropy estimation, NIST certification, statistical false-positive budget, cryptographic unpredictability claim, authenticated telemetry, signed evidence, or hardware-enforced receive-only guarantee is provided. The alternating synthetic passing stream is intentionally predictable. Source-specific validation and verification that diagnostic indications correspond to real release enforcement remain the operator's responsibility.
