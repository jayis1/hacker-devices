# Domain Cohort verification

Author: jayis1.

This release was executed in standalone-owner mode with advisory reports, not a full collective review. All findings below are software or design-file checks; none is a fabricated physical measurement.

## Executed checks

- Repository destination verified as jayis1/hacker-devices, configured default branch main. Initial worktree was clean, and `git pull --ff-only` completed before invention. No other repository was modified.
- Existing directories and README concepts inventoried. Owner Bind, Exclusion Ledger and Read Share were inspected as close alternatives; the README explains their policy differences.
- Test-first policy check: the independent domain-array oracle failed against the inherited client-exclusivity implementation before the domain-cohort policy was applied.
- Test-first sparse-frame check: a domain switch without a closed cohort initially passed inherited transition validation and failed the new regression. The production companion now rejects it while accepting same-domain rolling reuse and a genuine full-drain switch.
- `make -C firmware clean all test`: target compile/link and host tests passed. Arm GCC 14.2.1, host GCC 14.2.0. Executable identified as 32-bit little-endian ARM EABI5. Target text 3328 bytes, data 0 bytes, BSS 1660 bytes.
- Engine test passed 4136 oracle branches with address/undefined-behavior sanitizers. Additional explicit tests cover both domains, sharing, rolling reentry, complete drain, wrong-domain release, ambiguity, rejected short pulses, gaps, timer wrap, exhaustion rollback, encoder bounds and invalid live-domain mixtures.
- Serial and production main-loop harnesses passed bounds, timeout, recovery, timer wrap, periodic late-attachment policy, lost boot submission, and watchdog-startup failure checks. These harnesses do not execute MMIO on physical hardware.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v`: 17 tests passed. Includes real Tk event-loop / Linux PTY input, all three screens, duplicate connection refusal, hangup, replay, export, permissions and overwrite refusal. The real C fixture exercises parallel cohorts, rolling reuse, a full-drain domain change, and terminal wrong-domain release. All fixture data is synthetic.
- `python3 -m compileall -q app`: passed on Python 3.13.5. Runtime imports resolve through the application path; a static editor that does not model the tests' sys.path setup can report unresolved local imports.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native KiCad sources and XML netlist using KiCad 9.0.2 / matching pcbnew.
- `/usr/bin/python3 tools/check_hardware.py`: zero schematic ERC violations, zero PCB DRC violations, zero unconnected items. Exact schematic/PCB/manifest agreement for 14 components, 30 electrical pads and 9 nets, including values and full footprint identities.
- A fresh schematic SVG was rendered and visually inspected: labelled channels, component values, connector numbering and jayis1 attribution are legible without overlapping net labels.
- Release checker: README 3110 whitespace-separated words; 562 nonblank, comment-stripped production C lines, excluding headers and tests. Original source, documentation, metadata and native design files contain jayis1 attribution. Build products and local KiCad preferences are ignored.

## Review and remaining gates

Policy review confirms symmetric same-domain sharing, no FIFO constraint, no alternating-domain requirement, release-domain checking, and permanent halt after continuity loss. Sparse checks are necessary consistency bounds, not complete hidden-history proofs. The device cannot prove erasure between cohorts, enforce partition separation, authenticate the target adapter or identify invisible operations.

The design reuses a passive common-ground carrier. Its 1 kOhm series resistors are not qualified overvoltage protection, and it has no galvanic isolation or dedicated ESD clamp. Supplied firmware leaves target-facing GPIO as inputs, but reprogramming can change that. External Nucleo connector, jumper and bridge settings must be checked against the purchased board's manufacturer manual before wiring.

Fabrication, physical programming, electrical loading, oscillator timing, sustained real USB transport, watchdog hardware recovery, connector fit, enclosure safety, ESD and regulatory qualification remain unperformed. This release is build-tested and CAD-checked, not fabrication-qualified or production-certified. No physical hardware was available in this execution environment.
