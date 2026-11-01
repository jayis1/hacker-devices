# Rank Drain verification

Author: jayis1.

## Scope

Executed software and CAD verification for the unbuilt revision-A prototype. Review was standalone-owner verification with read-only advisory input, not full collective approval. The source-level advisory review identified no blocker, but correctly noted that the pre-fix host binary was stale; the final run below began with a clean rebuild and passed the new regression. No physical board, target signals or USB device were used.

## Executed checks

From the repository root:

- `make -C rank-drain/firmware clean all test`: PASS. Arm Cortex-M4 target linked and binary emitted; final ELF has text 3600 bytes, data 0 bytes, BSS 1692 bytes. Host engine exercised 46,984 independent descending-rank oracle transitions with address/undefined-behavior sanitizers, plus occupied-subset release cases, immediate reuse, ambiguity, gap, wrap, rollback and impossible-rank-fault regressions. UART and production scheduler tests passed.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s rank-drain/tests -p 'test_*.py' -v`: PASS, 15 tests. Includes real C-encoder fixtures, CRC-valid impossible snapshots, hidden lower release blocked by a continuous higher incumbent, permitted higher release/reentry, live stale handling, parser resynchronization and actual Tk screens. PTYs exercised serial connection, duplicate-open rejection, hangup and terminal restoration. JSON creation, private mode, overwrite refusal and retention after failed connection were checked.
- `/usr/bin/python3 rank-drain/tools/generate_hardware.py`: PASS. Generated native schematic, PCB, project, authored symbols and connectivity manifest; exported stored XML netlist.
- `/usr/bin/python3 rank-drain/tools/check_hardware.py`: PASS. Zero ERC violations, zero DRC violations, zero unconnected items. Fresh schematic netlist matched the board and manifest across 14 components, 30 pads and 9 nets, including values and full footprint identities.
- `kicad-cli sch export svg`: PASS. Rendered schematic was converted to PNG and visually inspected: labelled resistor chains, separate target and module connectors, ground connections and jayis1 title block are readable with no observed label overlaps.
- `/usr/bin/python3 -m compileall -q rank-drain/app`: PASS.
- `/usr/bin/python3 rank-drain/tools/check_release.py`: PASS before this report was added: 3335 README words, 592 nonblank production C lines after comments removed, 33 attributed deliverables. The report adds one attributed deliverable; the gate is rerun before publication.
- Initial red tests were observed: the inherited engine rejected immediate reuse; the initially absent companion contract failed import; an impossible rank fault was accepted before matching C/Python validation was added. All final regressions passed after implementation.

Tool versions: KiCad CLI 9.0.2; Arm GCC 14.2.1; host GCC 14.2.0; system Python 3.13.5. Build products, Python caches and local KiCad preferences are ignored and are not source deliverables. No third-party dependency downloads are required for the companion.

## Interpretation and open physical gates

The repository was synchronized with `git pull --ff-only origin main` before selection. Directory titles and closest FIFO, LIFO, acquisition-rank and reuse-distance contracts were inspected. RK1 did not collide with another device README protocol marker in that inventory. Originality is repository-relative; the passive electronics and platform code are disclosed reuse of jayis1-authored material.

Passing host tests and target linking does not establish MCU programming, real acquisition timing, watchdog operation on silicon, external module configuration, cable loading, target interoperability, manufacturing suitability, electrical safety, ESD, EMC or regulatory compliance. These remain explicit pre-deployment gates. Passive-symbol ERC cannot verify the external power source. No fabricated hardware measurements or production-readiness claims are included.

The instrument trusts owner-provided diagnostics. Endpoint validation is necessary consistency checking, not authentication or complete hidden-history reconstruction. The application may display fresh telemetry from a halted MCU; HALT and invalid acquisition remain separate from serial freshness. No target transmission, credential collection, network listener or workload generator is implemented.
