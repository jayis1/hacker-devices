# Paired Lead verification record

Author: jayis1.

Scope: standalone-owner implementation and self-review, with supplied advisory reports. No full collective approval or independent electrical review is claimed. All checks below were executed against this directory; fixture data is synthetic, not measured target traffic.

## Executed software checks

- The new second-same-tag-lifetime test was first compiled against the existing Round Cover engine. It failed at the expected assertion because coverage renewal rejects that operation. Paired Lead subsequently passed the same distinguishing test.
- `make -C firmware clean all test` passed using Arm GCC 14.2.1 and host GCC 14.2.0. The linked Cortex-M4 image has 3484 bytes text, zero initialized data and 1676 bytes BSS. ELF, BIN and map products are reproducible build outputs excluded from Git.
- AddressSanitizer/UndefinedBehaviorSanitizer engine tests passed 170568 independent signed-walk oracle transitions through operation-prefix depth eight. They cover both directions and both pairs, inclusive two-admission lead, third-admission rejection, release/idle persistence, partner recovery, busy partners, independent pair progress, duplicate-before-lead precedence, setup/raw ambiguity, simultaneous strobes, clock wrap, gaps, transactional counter exhaustion and immutable halted history.
- UART register-model tests passed buffer limits, concurrent submission refusal, nonblocking service, timeout, recovery and timestamp wrap. Production scheduler tests passed periodic compatibility after a lost boot record, transport timeout accounting and watchdog-startup failure. These are host models, not MCU peripheral measurements.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v` passed all 17 tests on Python 3.13.5. The count-product test checks all four counters independently over zero through five. CRC-valid cross-pair credit-transfer records and an unsupported partner-map revision are rejected.
- Actual C-encoder fixtures passed through the actual Tk event loop. The tests exercise all three screens, independent-pair and partner-progress summaries, bounded history, pseudo-terminal connection, hangup, terminal-setting restoration, failed open, replay, private JSON export and overwrite refusal. Synthetic PTYs do not validate physical USB.
- `/usr/bin/python3 -m compileall -q app` passed. Python imports intentionally add the local app directory at runtime; no third-party modules or lockfiles are needed.

## Executed CAD checks

KiCad 9.0.2 generated native schematic, PCB and project files using authored symbols and standard installed footprints. `/usr/bin/python3 tools/check_hardware.py` passed:

- ERC: zero violations.
- DRC: zero violations and zero unconnected items.
- Exact agreement across freshly exported schematic, saved netlist, PCB and connectivity manifest: 14 components, 30 electrical pads and 9 nets, including values and full footprint identities.
- Schematic exported to SVG, converted with rsvg-convert and visually inspected. Ground-only J1, signal-only J2, target signal/ground connectors, series resistors, pulldowns and labels are legible and consistent. The external powered Nucleo is deliberately represented by its harness, not a fictitious standalone MCU supply circuit.

These checks do not establish fabrication readiness or electrical safety. The manufacturer UM1724 PDF could not be freshly retrieved: the extraction backend did not support extraction, direct HTTP/2 failed, and HTTP/1.1 retries timed out. The inherited module pin map remains explicitly subject to manufacturer/purchased-board verification before assembly. No component stock, procurement fit or module revision claim is made.

## Content and review gates

`python3 tools/check_release.py` passed the 2000-word README and 500-line production-C minimums: 3187 README words and 573 nonblank, comment-stripped production C lines. Headers and tests are excluded from that C total. Generic platform reuse is expressly disclosed; those inherited lines are not claimed as newly invented firmware. Every deliverable contains valid jayis1 attribution, including native CAD metadata. Build products, Python caches and KiCad local preferences are ignored.

Self-review compared the new engine and companion against the reused platform, checked fixed partner-map and lead-bound agreement, reviewed safe input-only configuration and bounded transport, and checked scope/whitespace. The repository was pulled before selection. Only the new paired-lead directory is intended for publication; no sibling design, shared system tree, CI workflow or daily-ideas content is changed.

## Outstanding physical acceptance

No board was fabricated or flashed. Supply sequencing, common-ground loading, reset/watchdog behavior, oscillator accuracy, GPIO timing, resistor and connector fit, ESD, mechanical support, real USB serial and physical target interoperability remain unverified. The README contains the authorized-use restriction and an explicit pre-assembly/bench checklist. This release is a build-tested, simulation-tested and CAD-checked prototype design, not a deployed or certified device.
