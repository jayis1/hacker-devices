# Wear Balance verification record

Author: jayis1.

This is standalone-owner validation with advisory reports, not a completed independent collective review. All measurements below are software/build/CAD results. No physical instrument was assembled or attached to a target.

## Executed gates

Commands below ran from the repository root and completed successfully:

- `make -C wear-balance/firmware clean all test`: ARM target ELF and binary produced with GCC 14.2.1, Cortex-M4 Thumb soft float, C11, `-Wall -Wextra -Werror`. ELF size: text 3408 bytes, data 0, BSS 1676. Engine host test used AddressSanitizer and UndefinedBehaviorSanitizer; UART and scheduler tests used UndefinedBehaviorSanitizer.
- Engine oracle compared 7080 operation transitions from repeated words through depth six against an independent array-based occupancy/use oracle. Additional checks covered biased reuse after full drain, arbitrary ties, busy low-use exclusion, orphan and duplicate operations, simultaneous strobes, raw endpoint disagreement, acquisition loss, timestamp wrap, idempotent timestamps, invalid snapshot rejection, bounded serialization, CRC known vector, and transactional exhaustion on an allocation endpoint.
- UART test exercised size/null/busy rejection, byte service, timeout at its exact deadline, wrap, and recovery using a Linux register mapping. Scheduler test covered lost boot frame, repeated compatibility policy for late attach, timeout accounting, and watchdog startup failure. These are not actual peripheral tests.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s wear-balance/tests -p 'test_app.py' -v`: 11 tests passed under Python 3.13.5. Tests included real production C-encoder fixture replay, impossible snapshots, per-tag history migration, sparse single-admission bias, malformed frame recovery, freshness, policy mismatch, reset, replay bounds, private export, terminal restoration, and the real Tk event loop with a serial PTY.
- The GUI test selected all three screens, exercised live serial ingest, refused a second connection, detected hangup, loaded synthetic C telemetry, preserved replay on failed connection, exported JSON, and refused overwrite. No physical USB transport or target instrument was used.
- `/usr/bin/python3 wear-balance/tools/generate_hardware.py`: generated native KiCad schematic, PCB, project, local symbols, netlist, and manifest using KiCad 9.0.2 and installed footprints.
- `/usr/bin/python3 wear-balance/tools/check_hardware.py`: zero ERC violations, zero DRC violations, zero unconnected items. Exact reference/value/footprint and pin-to-net agreement among schematic, PCB, saved netlist, and connectivity manifest: 14 components, 30 pads, 9 nets.
- `kicad-cli sch export svg ...`: schematic export succeeded. Rendered with rsvg-convert and visually inspected: four separate series/pulldown networks, ground-only J1, correctly labelled J2, nonoverlapping labels, and jayis1 title credit.
- `python3 wear-balance/tools/check_release.py`: README 3295 whitespace-delimited words; 573 nonempty production C lines after stripping comments, excluding headers and tests. Every deliverable has jayis1 attribution in comments or format-valid metadata. Build/cache/local-preference files are ignored.
- `git diff --check`: no whitespace errors. Scope inspection found only the new wear-balance directory; no root automation, existing devices, or other repositories changed.

## Test-first and review notes

The new engine test initially failed because engine.h did not yet exist; the new application tests initially failed because contract.py did not yet exist. Both passed after implementation. The state machine, protocol order, firmware input mapping, board nets, and GUI meanings were compared during owner review. Generic jayis1-authored carrier/transport infrastructure was reused explicitly; the lifetime/free-candidate policy and new contract tests are not renamed weighted-budget behavior.

## Qualification not performed

Physical fabrication, assembly, programming, reset behavior on a real module, measured current, oscillator error, electrical loading, cable timing, ESD, enclosure safety, and target adapter truthfulness remain unverified. The passive board contains no isolation or overvoltage protection. Passing its ERC/DRC does not verify an external Nucleo or harness. This release is an unbuilt engineering prototype, not fabrication-ready certified hardware.

A fresh manufacturer UM1724 PDF retrieval failed: the configured extraction backend did not support extraction, HTTPS curl encountered an HTTP/2 error, and an HTTP/1.1 retry timed out. No document content was fabricated. The inherited NUCLEO-F401RE mapping must be checked against the purchased module revision and manufacturer documents before assembly. Supplier stock and prices were not verified.

GitHub CLI was unauthenticated; repository fetch/pull access worked through the existing Git HTTPS configuration. Publication is performed with Git, not by creating a PR or changing credential configuration. Remote publication must be verified after push; this pre-commit record intentionally contains no invented commit hash or premature push-success assertion.
