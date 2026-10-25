# Latency Pair verification record

Author: jayis1.

## Scope and result

Engineering prototype design; software execution and native CAD validation completed. No fabricated board, programmed MCU, calibrated timing measurement, USB hardware session, target capture, electrical qualification, ESD test, brownout experiment, regulatory approval, or fabrication-readiness claim is made.

Repository: jayis1/hacker-devices. Configured remote HEAD: main. Initial `git pull --ff-only origin main` returned already up to date. Existing device directories and README scopes were reviewed before selection; no `systems/` tree or daily-ideas repository was used. All deliverable changes are confined to latency-pair/. No task branch was created.

## Executed checks

- `make -C latency-pair/firmware all test`: passed. ARM GCC 14.2.1 (20241119) linked the actual Cortex-M4 image using startup vectors, linker script, and production drivers. Image section sizes: text 2960 bytes, data 0 bytes, BSS 884 bytes. Stack margin is checked by the linker script.
- Four host C executables passed: acquisition algorithm, wire encoder, production main scheduling, and production serial driver. The first three use AddressSanitizer plus UndefinedBehaviorSanitizer; the mapped-register serial test uses UndefinedBehaviorSanitizer.
- Acquisition tests cover both pair orders, ties, threshold boundary, repeated class, label setup and hold, invalid and inhibited candidates, maximum duration, pending timeout, gaps with and without an active second half, duplicate ticks, timestamp wrap, saturation, high-at-attach behavior, and cohort immutability. An independent aggregate oracle checks 100 randomized legal cohorts. An additional deterministic 100000-sample malformed-input stream checks invariants at every observation.
- Encoder tests include the standard CRC known vector, every insufficient buffer capacity for the policy frame, null output, invalid statistics, and impossible acquisition state. Production main tests exercise telemetry scheduling, stalled host output, continued acquisition, serial loss, gaps, and watchdog initialization failure.
- `xvfb-run -a python3 -m unittest discover -s latency-pair/tests -p 'test_*.py'`: nine tests passed under Python 3.13.5. The Tk test uses a real event loop and synthetic Linux PTY transport. It verifies all three screens, batched C-encoder telemetry, history row count, stale state, hangup, retained provenance, private export, overwrite refusal, replay truncation, and cleared display on reconnect.
- Protocol model tests reject malformed and impossible frames, incompatible policy, contradictory duplicate timestamps, rollback and reset indications. Replay rejects FIFOs. Fixtures come from the real C acquisition algorithm and encoder, not hand-authored successful measurement output.
- `python3 -m compileall -q latency-pair/app latency-pair/tests latency-pair/tools`: passed.
- GCC 14.2.0 `-fanalyzer -Wall -Wextra -Werror -fsyntax-only` on pair.c, wire.c, and drivers/serial.c: passed without diagnostics.
- `/usr/bin/python3 latency-pair/tools/generate_hardware.py`: generated native schematic, PCB, project, symbols, XML netlist, and connectivity manifest using KiCad 9.0.2.
- `/usr/bin/python3 latency-pair/tools/check_hardware.py`: ERC zero violations, DRC zero violations, zero unconnected items. Exact schematic/PCB/manifest/saved-netlist agreement: 14 components, 30 pads, 9 nets, including component values and full footprint identities.
- Native schematic SVG export succeeded. Its raster rendering was visually inspected for connector labels, resistor values, channel names, and author/title block. This is a drawing check, not electrical validation.
- README word count: 3539 using whitespace splitting. Production C/header lines: 719 total, 647 nonblank after block-comment removal. C source files alone contain 540 nonblank lines after block-comment removal; test code is excluded from these counts.

## Review findings and corrections

A separate read-only review reproduced CRC-valid impossible records that the first parser version accepted. A tie could coexist with unequal sums and a large difference, and ACTIVE could be reported with BUSY and VALID low. Regression tests were added and observed failing before changes. Firmware encoding validation and app parsing now check direction/sum bounds, tie and threshold relationships, exact single-pair maximum, and READY/ACTIVE input consistency. The full suite above passed after correction.

## Build fingerprint

Locally built firmware binary SHA-256:

`01cadc1628ec2308b2fba0048c66b9f042f5ef62d6408cb2827d61b4874a909a`

This identifies the verified local binary, not a signature or hardware attestation. Build products, Python bytecode, and KiCad local preference files are intentionally ignored rather than committed.

## Remaining physical gates

Verify purchased components and module revision, inspect the assembled carrier, measure continuity and loading, confirm ground/voltage compatibility, program through physically controlled ST-LINK, calibrate sampling and serial clocks, test watchdog and brownout behavior, and qualify diagnostic setup/hold with a scope. Input-protection and isolation limits in README.md remain binding. Statistical significance and real-target exploitability are outside this instrument's implemented claims.
