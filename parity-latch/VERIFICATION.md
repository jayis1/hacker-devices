# Verification record — Parity Latch revision A

Author: jayis1.

Scope: software and native CAD validation of an unbuilt engineering prototype. No physical measurements are represented by the synthetic fixtures. Work was performed in standalone-owner mode with advisory review, not a full collective engineering sign-off.

## Executed checks

- `make -C firmware clean all test`: PASS. ARM GNU GCC 14.2.1 linked the real Cortex-M4 startup, board/USART drivers, engine, and encoder with warnings treated as errors. ELF inspection identifies an ARM EABI5 executable with entry 0x08000175. Final section sizes: text 3516 bytes, data 0 bytes, BSS 1660 bytes. A raw `.bin` was produced by objcopy. Build products are intentionally ignored rather than committed.
- Native engine/encoder tests: PASS under AddressSanitizer and UndefinedBehaviorSanitizer. Cases include valid and invalid parity, raw setup/hold excursions, exact setup and hold boundaries, collision at completion precedence, startup-high suppression, rearming, gaps, exact missed-tick accounting, timer wrap, duplicate ticks, counter exhaustion while idle and active, transactional rollback after partial speculative updates, and malformed-state rejection. A deterministic 100,000-sample sequence checks invariants under held states, transitions, glitches, and gaps.
- Native serial-driver tests: PASS under UndefinedBehaviorSanitizer. Tested bounded submission, busy rejection, TX-ready service, timeout, recovery, and timestamp wrap using a Linux register-memory model. This is not real USART hardware testing.
- Production main-loop tests: PASS. One boot, recurring policy, periodic status, watchdog feeding, and watchdog-start failure are exercised with simulated board functions.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: PASS, 12 tests. Python 3.13.5. Includes production-C fixture ingestion, CRC/syntax/bounds, truncated-frame recovery, impossible conservation and failure attribution, duplicate/rollback handling, policy/reset behavior, first-status freshness suppression, backlog protection, bounded regular-file replay, private exclusive export, retained empty-session metadata, a real Tk event loop with Linux PTY reception and hangup, all three tabs, GUI replay/export, overwrite refusal, and failed-connection evidence preservation. PTYs model serial transport; no physical USB device was connected.
- Python application byte compilation: PASS.
- `/usr/bin/python3 tools/generate_hardware.py`: PASS using KiCad 9.0.2. Native schematic, PCB, project, authored symbol library, XML netlist, and connectivity manifest generated.
- `/usr/bin/python3 tools/check_hardware.py`: PASS. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic netlist, PCB pads, component values, footprint library identities, manifest, and saved netlist agree: 14 components, 30 pads, 9 nets.
- Schematic SVG export and raster inspection: PASS. Real resistor/connector symbols, separated reference/value labels, net labels, and jayis1 title attribution are visible; no label collisions were observed at the inspected rendering scale.
- `python3 tools/check_release.py`: PASS before this record was added: 3,461 whitespace-delimited README words and 555 nonblank, noncomment production `.c` lines. Headers, tests, and generated CAD are excluded from the C count. Every release file is checked for jayis1 attribution; the gate is rerun before publication.

## Advisory review corrections

A read-only code review identified missing rejection of snapshots attributing transfer failures to an all-good population, insufficient first-status freshness gating, and missing transactional/timing boundary coverage. The owner reproduced the two behavior defects with failing C/Python tests, fixed the encoder/model constraints and live-progress rule, added the boundary regressions, and reran the complete checks above. No other agent edited or published repository files.

## Not tested and limitations

Fabrication, assembly, electrical continuity, MCU programming, on-silicon startup, actual sampling jitter, clock calibration, GPIO input inactivity during all reset conditions, voltage/loading margins, ESD, EMC, USB interoperability, watchdog recovery, environmental behavior, and regulatory compliance are NOT TESTED. The carrier has no isolation or qualified overvoltage protection. CAD checks cannot validate the separately purchased Nucleo, harness wiring, or target electronics.

Manufacturer reference retrieval was attempted from ST's UM1724 URL. The extraction backend could not extract pages; direct HTTPS retrieval failed with an HTTP/2 stream error and the HTTP/1.1 retry timed out. This run therefore reuses the repository's previously documented NUCLEO-F401RE mapping and RM0368 register platform, not a claimed fresh manufacturer-document audit. Inspect the actual board revision and applicable manufacturer documents before fabrication or connection.

Parity, timing qualification, counter conservation, and application validation provide necessary sampled-evidence checks, not cryptographic authenticity or a complete realizability proof. Parity misses even-bit corruption. The target can lie about its internal state. Sampling cannot resolve faster unobserved behavior, and observer resets discard all RAM evidence. Publication of this design does not constitute physical qualification.
