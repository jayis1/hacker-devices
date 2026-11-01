# Round Cover verification

Author: jayis1.

## Scope and maturity

Verified as an unbuilt engineering prototype in jayis1/hacker-devices. Only the round-cover directory is part of this change. The repository began clean on its remotely configured default branch, main; `git pull --ff-only` returned already up to date. Directory inventory and nearest-device contracts were reviewed before selecting the unordered coverage-gated renewal policy. The electrical platform and transport infrastructure are disclosed reuse, not newly invented hardware.

This run used standalone-owner implementation and verification. Peer recommendations and a read-only code review were advisory, not full collective approval. The owner made all repository changes. The read-only review found no source-level policy or electrical-claim blocker, but identified the then-missing verification report and a stale fixture executable after new tests were added. This report resolves the first item; a clean rebuild and the complete passing rerun below resolve the second. No review result substitutes for the actual executed checks.

## Executed checks

Commands below run from the repository root unless stated otherwise.

- `make -C round-cover/firmware clean all test`: PASS. Cortex-M4 Thumb firmware compiled and linked with Arm GCC 14.2.1, C11, soft-float, warnings as errors and nosys/newlib support. The ELF has 3,428 bytes text, zero initialized data and 1,676 bytes BSS. These sizes exclude stack consumption and are not measured hardware power or timing results.
- Engine tests: PASS under AddressSanitizer and UndefinedBehaviorSanitizer. The independent covered-set oracle checked 65,640 operation transitions; 576 pairs of four-identity permutations checked arbitrary allocation/release order and renewed rounds. Additional cases cover busy laggards, pool drain without renewal, continuing occupancy across a round boundary, duplicate precedence, orphan release, raw/setup ambiguity, skipped samples, timestamp wrap, counter exhaustion and frozen accepted evidence.
- Serial driver tests: PASS. Bounded submission, timeout, recovery and timer wrap.
- Production main-loop harness: PASS. Lost boot record, repeated late-attachment policy, transport failure and watchdog initialization failure. Board/transport functions are controlled host substitutes, not physical MCU execution.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s round-cover/tests -p 'test_*.py' -v`: PASS, 18 tests, using Python 3.13.5. Real C-encoded partial-round failure, successful shuffled rounds and busy-laggard snapshots were accepted consistently. Tests exercise the Tk screens, history rows, Linux serial pseudo-terminal, hangup, reconnect, failed-open retention, replay bounds, terminal restoration, private export, overwrite refusal, malformed framing, CRC-valid impossible evidence, stale data and coverage history migration.
- `/usr/bin/python3 -m compileall -q round-cover/app`: PASS. Companion uses the Python standard library and system Tk; no third-party dependency lockfile is required.
- `/usr/bin/python3 round-cover/tools/generate_hardware.py`: PASS with KiCad 9.0.2 bindings and installed standard footprints. Native schematic, PCB, project, symbol library, manifest and netlist regenerated for Round Cover.
- `/usr/bin/python3 round-cover/tools/check_hardware.py`: PASS. ERC: zero violations. DRC: zero violations and zero unconnected items. Exact schematic/PCB/saved-netlist/manifest comparison: 14 components, 30 connected pads, 9 nets, including values and full footprint library identities.
- Schematic exported using `kicad-cli sch export svg`, converted with `rsvg-convert`, and visually inspected. Header pin labels, resistor nets, Round Cover title and jayis1 attribution were legible; no visible label overlap or disconnected pin stubs were found. This visual review is not independent electrical qualification. An initially attempted CairoSVG conversion was unavailable; the installed SVG renderer produced the inspected image instead.
- `python3 round-cover/tools/check_release.py`: PASS before this report was added: 3,494 README words, 574 nonblank production C lines after stripping comments, and 34 attributed deliverables. The release gate is rerun after adding this report. Header files and tests are not counted toward the production C total.

The build directory, Python bytecode, KiCad local preferences and scratch renderings are ignored or outside the repository. No binary fixture output is presented as captured hardware data. Every authored deliverable contains jayis1 attribution in a format-valid header or metadata field; upstream tool/library notices remain intact.

## Important interpretation limits

The firmware observes accepted target indications, not the protected operations themselves. The adapter must provide complete, truthful, paced indications from an independently established empty pool and fresh campaign. The companion's sparse-record checks are necessary consistency bounds, not an exact reconstruction of hidden operation order. The protocol's inherited `rounds` field counts closed occupancy episodes; completed coverage rounds are separately derived from the minimum cumulative use count. A busy lagging identity can make the eligible set empty even while other resources are free, by design.

There has been no fabrication, physical board inspection, MCU programming, real ST-LINK/USB test, calibrated timing, electrical loading measurement, ground-offset test, reset/brownout/watchdog bench test, ESD qualification, regulatory evaluation or target interoperability test. The passive carrier is not isolated and is limited to supervised common-ground 3.3 V laboratory diagnostics. External Nucleo configuration and harness mapping require manufacturer-manual and physical checks before assembly. Zero ERC/DRC findings do not make the complete instrument fabrication-qualified or deployment-ready.

Publication is performed only after the final release gate and staged-scope review. The completion report records the commit and fresh remote-default-branch verification; this document does not claim a push before it occurs.
