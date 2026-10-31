# Slot Affinity verification

Author: jayis1.

Execution mode: standalone owner, with advisory peer material and an additional read-only source review. This is not a full collective engineering review. Only the owner changed repository files. Repository identity was verified as jayis1/hacker-devices, configured default branch main. Initial worktree was clean and `git pull --ff-only` returned already up to date. No task branch was created.

## Executed acceptance evidence

- Target: `make -C slot-affinity/firmware all test` built and linked the STM32F401RE image with arm-none-eabi-gcc 14.2.1, warnings treated as errors. ELF size: 3316 bytes text, zero initialized data, 1660 bytes BSS. Binary and map were produced in ignored firmware/build/.
- Test-first policy check: the new independent affinity oracle initially failed against the reused homogeneous-cohort implementation. After implementing first-use slot pinning, 3848 oracle branches passed under address/undefined-behavior sanitizers. Cases include both domain labels, both slots, allocation/release order, unchanged reuse, changed affinity, same-domain and different-domain concurrency, terminal state preservation, acquisition loss, clock wrap, setup ambiguity, chatter and counter exhaustion.
- Bounded serial tests passed: bounds, timeout, recovery and wrap. Production-main harness passed late attachment, lost boot record, transmission timeout and watchdog-startup handling.
- `xvfb-run -a python3 -m unittest discover -s slot-affinity/tests -p 'test_*.py' -v`: 18 tests passed. These exercise real Tk screens and event loop with Linux pseudoterminals, disconnect retention, the real C encoder's synthetic fixture, private JSON export, overwrite refusal, invalid frames, freshness and immutable affinity across sparse snapshots.
- `python3 -m compileall -q slot-affinity/app` passed. No external Python packages are required; Tk and Linux terminal APIs are prerequisites.
- KiCad 9.0.2 generated native schematic, routed PCB, project, symbol library and netlist. `tools/check_hardware.py` ran fresh ERC and DRC: zero violations and zero unconnected items. Fresh schematic XML, saved netlist, PCB pads and manifest agree on 14 components, 30 pads and nine nets, including values and complete footprint identities.
- The schematic was exported to SVG, rasterized and visually inspected: all four input networks, pin labels, component values, title and jayis1 attribution are readable. Render artifacts remain outside the repository.
- Release checker reports 3061 README words and 559 substantive production C lines, excluding comments and tests. Every deliverable includes jayis1 attribution. Build products, Python caches and KiCad local preference files are ignored.
- Read-only advisory source review reported no correctness blockers. Its missing-verification-document observation is resolved by this file.

Commands above were executed from the repository root unless otherwise specified. Hardware tools are run with `/usr/bin/python3` to use the system KiCad bindings. Tests and generated fixtures are synthetic software evidence, not measurements from a fabricated board. Runtime exports also credit jayis1.

## Review boundaries and unresolved physical gates

The implementation intentionally reuses the documented four-input passive carrier and board-support platform. Slot affinity is the new policy, not new electrical isolation. The image never enables a target-facing output, receives no commands, and does not perform exploitation or collect payload contents. Source review checked the label lifetime, mismatch precedence, counter conservation, halt behavior, protocol marker and GUI interpretation together.

The hardware remains unbuilt. Fabrication, flashing, actual Nucleo revision and ST-LINK bridge confirmation, electrical voltage/loading tests, ground offsets, power sequencing, oscillator timing, watchdog behavior on silicon, sustained physical serial traffic and ESD qualification have not been performed. ERC of passive symbols cannot establish the safety of the externally powered system. No claim of certification, deployment readiness, authenticated evidence or globally novel invention is made.

The observer's first-use history is volatile. Restarting the observer without independently resetting the target's assignment campaign loses the original bindings; this is outside the measurement contract. A lost boot record can conceal some resets. Sparse transition checks are necessary constraints, not an exact reconstruction of every hidden operation. Refer to README.md for the full authorized-use and electrical limitations.
