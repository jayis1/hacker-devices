# Oldest Free verification record

Author: jayis1.

## Publication recovery recheck

The publication-owner recovery reran the ARM target build and all host firmware tests, including 162600 independent oracle transitions; all passed. All 17 application tests passed again under Xvfb. Fresh KiCad ERC and DRC reported zero violations and zero unconnected items, and the connectivity comparison passed for 14 components, 30 pads and 9 nets. Release checks confirmed 3147 README words, 595 substantive production C lines and attribution in all 34 deliverables. A read-only advisory code review found no material engine or companion consistency blockers; this remains standalone-owner verification, not collective approval. The remote default branch was confirmed as main and the initial pull was already up to date. These repeat checks do not add physical hardware validation to the earlier evidence below.

## Readiness

Build-validated and CAD-validated engineering prototype; not hardware-validated. This record describes executed software and CAD checks, not fabrication readiness, electrical certification, physical measurements, or target security certification. The work was recovered from an interrupted execution of the same authorized device-design task; its provenance was checked against the originating session before adoption. Review was standalone-owner verification with advisory read-only critique, not full collective approval.

## Executed checks

Commands were run from the repository root unless otherwise stated.

- `git pull --ff-only origin main`: already up to date at the initial baseline e7f13b8348396d53da201c090bdb055a35e1a08e. Origin is jayis1/hacker-devices; remote HEAD identifies main. No temporary branch was created.
- Directory inventory and closest README contracts were inspected: Wear Balance uses cumulative frequency, Distance Renew uses admission distance, and Rank Drain constrains release priority. Oldest Free instead selects minimum last-admission ordinal among currently free resources. A repository README search for least-recent and oldest-free contracts found only this design. This is repository-relative differentiation, not worldwide novelty research.
- `make -C oldest-free/firmware -B all test`: passed with ARM GNU GCC 14.2.1, Cortex-M4 soft-float target, C11 and warnings treated as errors. The linked image reports 3640 bytes text, zero initialized data, and 1692 bytes BSS. These are link-time sections, not measured stack consumption or cycle timing.
- Engine test: 162600 independent move-to-back recency-list oracle transitions passed. AddressSanitizer and UndefinedBehaviorSanitizer were enabled. Additional cases cover busy oldest alternatives, unrestricted return order, only-free immediate renewal, frequency/recency distinction, duplicate/orphan precedence, raw setup ambiguity, transactional exhaustion, timestamp wrap, frozen evidence and serialization bounds.
- Serial test: bounds, timeout, recovery and timestamp wrap passed with UndefinedBehaviorSanitizer.
- Main-loop test: periodic late-attachment policy, lost boot transmission, timeout and watchdog startup handling passed with UndefinedBehaviorSanitizer. These tests use substituted board primitives and do not execute MCU peripheral accesses on physical hardware.
- `xvfb-run -a python3 -m unittest discover -s oldest-free/tests -p test_app.py`: all 17 tests passed under Python 3.13.5. Actual Tk event loops and Linux pseudo-terminals exercise the three screens, disconnect/hangup, termios restoration, replay and exclusive private JSON export. Permission denial preserves prior evidence. Parser/contract checks cover real C-generated synthetic fixtures, incompatible policy, malformed frames, partial tails, impossible recency findings, stamp migration, hidden illegal renewal, halted immutability, stale duplicate timestamps, cumulative delivery drift and counter rollback. No synthetic fixture is represented as physical telemetry.
- `/usr/bin/python3 oldest-free/tools/check_hardware.py`: passed using KiCad 9.0.2. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML, saved netlist, PCB pad nets, footprint identities, values and connectivity manifest agree for 14 components, 30 pads and 9 nets.
- `kicad-cli sch export svg` and `kicad-cli sch export pdf`: successful. The PDF was rendered with pdftoppm and visually inspected for component placement, net-label legibility and title-block attribution. No visible label/pin overlaps were found. This does not validate physical connector orientation or external module circuitry.
- `python3 oldest-free/tools/check_release.py`: README exceeds 2000 words (3147); production C exceeds 500 substantive nonblank, noncomment lines (595). Authored deliverables contain jayis1 attribution. Build outputs, Python caches and KiCad local preferences are ignored. The final staged whitespace and scope checks are run before publication.

## Scope and limitations

The schematic is a passive resistor/header carrier for an external purchased NUCLEO-F401RE; it is not a complete standalone MCU board. Module power regulation, USB, reset and SWD are supplied by that module. Passive ERC cannot validate the external power source, voltage tolerance, harness mapping or purchased board revision.

Fresh retrieval of ST UM1724 failed: the direct HTTPS request returned an HTTP/2 stream error, and the configured web extractor cannot extract documents. The inherited pin map and register definitions were cross-checked within the design, but this run does not claim a fresh manufacturer-document electrical review. Verify UM1724, RM0368 and the MCU datasheet against the purchased module before assembly or programming.

No board was fabricated, powered, flashed or connected to a target. No oscilloscope capture, timing calibration, stack high-water measurement, EMC/ESD qualification, brownout experiment, regulatory testing or physical interoperability test was performed. There is no secure boot, authenticated telemetry or persistent reset identity. CRC is accidental-error detection only. Sparse endpoint consistency checks do not prove all hidden operations followed the policy. Authorized-use restrictions and safe common-ground 3.3 V diagnostic attachment limits in README.md remain mandatory.

The hardware generator is included for reproducibility; CAD verification in this recovery ran against the delivered native files rather than regenerating random PCB object identifiers. The GitHub CLI was not authenticated, so publication uses the repository's existing Git HTTPS transport; no CLI authentication or credential files were changed.
