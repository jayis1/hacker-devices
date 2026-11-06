# Cursor Fit verification record

Author: jayis1.

## Scope and provenance

The release was completed in standalone-owner mode. Advisory reports were planning inputs, not evidence of an independent implementation review. The pre-existing untracked cursor-fit directory was traced to interrupted output of this same authorized task. It was inspected and rebuilt rather than silently adopting unknown work. The missing README was completed with the implemented contract, hardware mapping, operational limits, and repository-relative comparison.

Repository: jayis1/hacker-devices. Configured remote default: main. Initial `git pull --ff-only` returned `Already up to date.` No task branch was created. The only initial untracked directory was cursor-fit; no unrelated edits were included. Publication is verified separately against the remote branch after commit.

## Executed checks

Commands below are run from the repository root unless otherwise stated.

- `make -C cursor-fit/firmware clean all test`: passed a fresh ARM build and all C test binaries. ARM GNU GCC 14.2.1 produced an ELF and binary for Cortex-M4 with warnings treated as errors. Linked size: text 3656 bytes, data 0 bytes, BSS 1692 bytes. This is target compilation and linking, not execution on a physical STM32.
- Engine test: 19,024 independent rotating-list oracle transitions passed. Enumeration explores operation words through depth ten, pruning terminal paths. Separate assertions cover full-pool handling, retained cursor after drain, only-free reuse, distinction from least-recent selection, duplicate admission, orphan return, strobe/tag ambiguity, acquisition gap, terminal freeze, transactional exhaustion and timestamp wrap. Engine uses AddressSanitizer and UndefinedBehaviorSanitizer.
- Serial test: buffer bounds, timeout, recovery and wrap passed with UndefinedBehaviorSanitizer.
- Main-loop test: late-attachment policy announcement, lost boot transmission, timeout and watchdog startup cases passed with UndefinedBehaviorSanitizer.
- `xvfb-run -a python3 -m unittest discover -s cursor-fit/tests -p test_app.py`: all 20 tests passed. Tests exercise checksum-valid impossible transitions, sparse cyclic barriers, legitimate busy-predecessor return, ordinal migration, stale telemetry, counter rollback, cumulative timing drift, malformed/truncated records, policy mismatch, reset handling, bounded replay, private exports, overwrite refusal, serial attribute restoration, permission failure and actual Tk screens. Linux pseudo-terminals exercise connection, duplicate-open refusal and hangup. C-generated fixtures are synthetic, not hardware measurements.
- `/usr/bin/python3 cursor-fit/tools/check_hardware.py`: passed KiCad 9.0.2 schematic ERC with zero violations, PCB DRC with zero violations and zero unconnected items. A freshly exported schematic XML netlist matches the PCB pads and declared manifest, including references, values and full footprint identifiers: 14 components, 30 pads, 9 nets. The saved device.net connectivity also matches.
- `kicad-cli sch export svg -o /root/.hermes/cache/scratch/cursor-fit-render/ cursor-fit/kicad/device.kicad_sch`: exported the native schematic. Rendered with rsvg-convert and visually inspected: connector labels, resistor values, wire labels and title/author are separated and legible. No unintended overlap was observed at the reviewed scale. This is not an independent electrical design review.
- `python3 cursor-fit/tools/check_release.py`: checks at least 2000 README words, at least 500 nonblank production C lines after removing comments, author attribution in every source deliverable, required CAD files, private-key patterns and whitespace. README has 3127 words; production C has 601 substantive lines, excluding headers and tests. Final deliverable count is produced by the check after this record is added.
- Host tool versions: GCC 14.2.0, Python 3.13.5, KiCad 9.0.2, ARM GNU GCC 14.2.1.

## Review findings and limitations

The cursor policy derives from the unique latest admission ordinal, advances only on successful admission, skips occupied identities, and persists through releases and drain. The independent engine oracle rotates a list instead of deriving the cursor from stamps. Sparse desktop checks reject an untouched continuously free cyclic barrier, but do not claim complete reconstruction of all hidden operation sequences. Individually consistent first snapshots cannot prove their prior history. Tests intentionally distinguish snapshot consistency from transition consistency.

The inherited passive carrier and firmware pin map agree. The MCU and power circuitry reside on the external NUCLEO-F401RE, not on the custom carrier. GPIO direction is software-enforced receive-only, not a physical isolation boundary. Dedicated ESD clamps, level translators, secure boot, log authentication and persistent boot identity are absent. These constraints are explicit in README.md.

A fresh ST UM1724 PDF download timed out. The inherited manufacturer-reference mapping was inspected locally, but fresh independent datasheet verification was not completed. Before construction, check UM1724, RM0368 and the STM32F401xD/xE datasheet against the actual purchased module revision, especially VCP solder bridges, connector numbering, clock assumptions and electrical limits.

The gh CLI has no authenticated session; repository operations use the existing Git HTTPS configuration without reading or printing credential files. No authentication configuration was changed. CairoSVG was unavailable for rendering; installed rsvg-convert successfully provided the raster used for visual inspection.

Not performed: physical fabrication, assembly, SWD flashing, oscillator calibration, power measurement, ESD testing, electrical bring-up, real USB disconnection, target adapter validation, actual target workload capture, manufacturing review or regulatory/safety certification. No test result is presented as evidence that those activities occurred. The deliverable is an executable engineering design, not a validated manufactured appliance.
