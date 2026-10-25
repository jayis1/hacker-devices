# Verification record

Author: jayis1

Scope: Erase Witness revision A engineering prototype. These are software and CAD results, not physical hardware measurements. Synthetic vectors are clearly distinguished from captures. Repository destination is jayis1/hacker-devices, configured remote default branch main. No daily-ideas files or automation workflows were touched.

## Executed checks

- `make -C firmware clean all test`: passed. ARM GCC 14.2.1 (20241119) linked the actual Cortex-M4 image with warnings treated as errors. ELF text 2788 bytes, data 0 bytes, BSS 1372 bytes. No external SDK required; startup, linker, MMIO map and drivers are supplied.
- Host GCC 14.2.0: policy/encoder tests under AddressSanitizer and UndefinedBehaviorSanitizer passed. Checks include ordered completion, premature done, insufficient dwell, delayed busy, busy timeout, simultaneous request/busy, early access, completion/access coincidence, abort, gaps, tick wrap, same-tick calls, saturating counters, queue overflow, CRC reference vector, and small output buffers.
- Production UART driver tests under UndefinedBehaviorSanitizer passed: invalid sizes, busy refusal, byte order, register-not-ready behavior, timeout, recovery and counter wrap. Register memory is simulated using Linux mmap, not physical USART hardware.
- Actual production main-loop test passed: the first boot transfer is deliberately lost; two subsequent policy announcements and at least twenty status frames are emitted, acquisition remains qualified, and the transmit-failure counter increases.
- `python3 -m unittest discover -s tests -p test_app.py -v`: eight tests passed on Python 3.13.5. Covers real C fixture, partial and oversized frames, CRC and numeric bounds, late policy, stale transport, replay, incompatible policy, reset/counter rollback, timestamp wrap, queued events, screens, private export and overwrite refusal.
- `python3 tests/integration.py`: passed. Real C-encoder output traversed the actual replay CLI and Linux serial pseudo-terminal. A separate terminal exercised the actual curses Overview, Events and Health screens, mode-0600 JSON export, refusal to replace previous evidence, unplug handling and clean exit. The lost-boot replay recovers supported policy without inventing a boot event.
- KiCad 9.0.2 generation completed with fourteen components and nine nets. `tools/check_hardware.py` reported zero ERC violations, zero DRC violations and zero unconnected items. Fresh schematic XML, saved netlist, PCB pads and manifest agree on all thirty component pins, component values and footprint library identifiers.
- The schematic was exported to SVG, rasterized with rsvg-convert and visually reviewed for connector/resistor placement, labels and author/title. All four signal rows are present. This is not a physical board inspection.
- README contains 3141 whitespace-delimited words. Production firmware contains 630 C/header lines, 565 after removal of block comments and blank lines; test code is excluded from those counts. Required driver, board, register and Makefile deliverables are included.
- Every deliverable text file contains jayis1 attribution in format-valid comments, metadata, symbol properties or title blocks. Existing standard KiCad footprint identifiers are retained. The hardware generator explicitly acknowledges circuit reuse rather than claiming a new analog topology.
- A read-only independent source review reported no blocking firmware/application defects. The owner rebuilt and reran the checks; review conclusions do not substitute for the listed executions.

The initial policy test was run before implementation and failed for the missing erase.h API. Implemented sources subsequently passed the clean build and test suite. No generated capture is claimed as observed hardware behavior. Build outputs, caches and local KiCad preferences are ignored rather than committed.

## Important residual limitations

No board was fabricated, assembled, flashed, powered or connected to a target. GPIO pin assignment, ST-LINK routing, supply configuration and target voltage require checks against the actual board revision and ST manufacturer documents before use. Oscillator tolerance, real sample intervals, inter-port skew, qualification latency, watchdog timeout, input loading, ground offset, back-powering, ESD, EMC and thermal behavior have not been measured. No storage data was examined; the instrument cannot establish sanitization effectiveness or authenticity of target diagnostics.

The all-low resynchronization boundary can be imitated by disconnected inputs because of pull-downs. The filter intentionally ignores short pulses, and a chattering signal delays qualification of the whole vector. Historical completion counts do not imply safe access: access may have been asserted early in the same cycle, so inspect unsafe and loss counters. Gaps invalidate lifecycle continuity; reset loses volatile evidence. Some lost-boot resets remain indistinguishable from forward traffic. UART and CRC are unauthenticated. The host and physical fixture must be trusted.

## Reproduction

From this device folder run:

```sh
make -C firmware clean all test
python3 -m unittest discover -s tests -p test_app.py -v
python3 tests/integration.py
/usr/bin/python3 tools/check_hardware.py
```

To regenerate CAD use `/usr/bin/python3 tools/generate_hardware.py` before the last command. KiCad's system Python bindings and standard footprint libraries must be installed. Temporary reports honor TMPDIR; use a scratch directory outside the repository. Do not interpret a missing dependency as a passed check.
