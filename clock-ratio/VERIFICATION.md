# Clock Ratio verification record

Author and creator: jayis1.

## Scope

This is standalone-owner verification with advisory review, not full collective approval. The deliverable is an unbuilt engineering prototype design. Checks below establish executable software and internal CAD consistency, not a physically working or fabrication-qualified instrument.

The configured repository was verified as jayis1/hacker-devices, with remote default branch main. The checkout was initially clean and `git pull --ff-only` reported already up to date at daf7e7c3c752693eed73b309340451d8676019d0. No task branch was created. Existing device directories and README scopes were inventoried; the nearest heartbeat, pulse-envelope, receipt, and lifecycle designs were examined before choosing the clock-ratio contract. No other repository was modified.

## Executed checks

- `make -C firmware all test`: passed with Arm GNU Embedded GCC 14.2.1. Actual Cortex-M4 target linking produced an ELF and binary, using the supplied startup and linker script. Final size output: 3108 bytes text, zero initialized data, 1644 bytes BSS. Build products are ignored rather than committed.
- Engine test executable: passed with address and undefined-behavior sanitizers. Exercises accepted four-reference intervals, too few and extra references, ambiguous edges, exact inclusive deadline and first-late sample, simultaneous edges at expiry, independent qualification under reset chatter, reset abort, sample gaps and recovery, out-of-enable ticks, transactional exhaustion, timestamp wrap, encoder capacity, and impossible conservation. A deterministic 200000-sample randomized run checks invariants and serialization after every sample.
- Main-loop test executable: passed with undefined-behavior sanitizer. Exercises one-shot boot, recurring policy, periodic statuses, watchdog feeding, and failed watchdog startup before telemetry.
- Serial-driver test executable: passed with undefined-behavior sanitizer. Exercises buffer bounds, busy rejection, register-model byte service, timeout, recovery, and timestamp wrap. Linux mapped memory substitutes for the USART registers; this is not silicon testing.
- `xvfb-run -a /usr/bin/python3 tests/test_app.py`: all nine tests passed. Covers the real C-generated fixture, CRC/syntax and size limits, checksum-valid impossible status, duplicate and rollback handling, private export and overwrite refusal, regular-file replay limits, FIFO rejection, long reception pause, gradual backlog, partial-frame recovery, boot/policy freeze behavior, empty-session metadata, and real Tk screens receiving the fixture over a PTY followed by hangup. These are synthetic data and simulated serial transport, not USB hardware measurements.
- `/usr/bin/python3 tools/generate_hardware.py`: generated fourteen components and nine nets with KiCad 9.0.2.
- `/usr/bin/python3 tools/check_hardware.py`: schematic ERC zero violations; PCB DRC zero violations and zero unconnected items. Fresh netlist, saved netlist, PCB, and connectivity manifest agree on all fourteen references, thirty pads, nine nets, values, and full footprint identifiers.
- The native schematic was exported to SVG, rendered, and visually inspected. Connector numbering, four signal labels, resistor values, ground mapping, title, and author were legible without apparent label overlap.
- README word count: 3215. Firmware C/header nonblank lines after removing block comments: 570, excluding tests, binaries, and generated CAD. Supporting board code is deliberately reused and credited to jayis1, not represented as a new circuit invention.
- Every deliverable source file contains jayis1 attribution, including supported KiCad metadata. Local preferences and caches are ignored. Source whitespace checking passed.

## Review correction

Read-only advisory review found a precedence mismatch: simultaneous qualified REF/TICK edges at the first expired sample could return through ambiguity handling before reporting timeout. A regression test first reproduced the failure. Timeout handling was moved ahead of ambiguity handling, matching the documented contract, and all software and CAD checks were rerun successfully. The resulting interval remains aborted and reports timeout rather than ambiguity at that boundary.

## Not verified and known limitations

No board has been manufactured, assembled, programmed, or electrically measured. External module pin mapping, solder bridges, supply configuration, physical part dimensions, ground compatibility, signal thresholds, loading, back-power, ESD, EMC, oscillator calibration, timing skew, watchdog operation, and mechanical retention require bench validation. Manufacturer PDF retrieval was attempted through the configured extractor and direct HTTPS; extraction was unsupported and direct requests failed with an HTTP/2 transport error or timed out. Fresh manufacturer-document review remains an explicit prerequisite before construction.

The passive carrier is not isolated or protected for field wiring. Only known common-ground zero-to-3.3 V diagnostic outputs are supported. Millisecond qualification cannot observe processor-speed clocks or every electrical glitch. Diagnostic signals can lie. Enabled operation before the first anchor has no timeout and must not be reported as a successful test. A LIVE display describes transport freshness, not measurement validity or security. CRC is not authentication, evidence is volatile, and some resets cannot be inferred without persistent boot identity.

Final publication details are reported in the run result after reading back the remote main reference; this file does not claim an unperformed push or remote verification.
