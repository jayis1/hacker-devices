# Weight Ceiling verification

Author: jayis1.

## Execution scope

This run used degraded standalone-owner execution with advisory review, not full collective approval. All repository writes and publication are performed by the owner. The hardware remains an unbuilt engineering prototype. Software fixtures are synthetic; no physical target measurements are included.

## Checks executed

- Repository identity: origin is `https://github.com/jayis1/hacker-devices.git`; remote HEAD selects `main`. Initial working tree was clean. `git pull --ff-only` reported already up to date before implementation. No temporary task branch was created.
- Inventory: top-level directories and available README titles inspected; nearest Slot Alias, Burst Budget, Grant Ledger and Split Approval contracts read. README search for weighted and aggregate-budget terminology found no matching weighted-live-reservation device. Unequal live weights, not slot count or timed replenishment, distinguish this contract.
- Test-first engine check: new acceptance test failed to compile before engine.h existed. New application tests failed import before contract.py existed. These were expected red-stage failures, followed by implementation and successful execution.
- `make -C firmware all test`: ARM target compilation and linking succeeded with warnings treated as errors. ARM GCC 14.2.1 produced text 3280 bytes, data 0 bytes, BSS 1660 bytes. Linker stack-headroom assertion passed. ELF, BIN and MAP outputs are reproducible ignored build artifacts.
- Engine test: 9,928 independent weighted-array-oracle branches passed with address and undefined-behavior sanitizers. Cases include exact ceiling, unequal-weight pairs, arbitrary releases, rolling reuse, duplicate-before-budget precedence, orphan release, interface ambiguity, setup, chatter, short pulses, gap, timestamp wrap, immutable halt and transactional exhaustion.
- Serial test: bounded submission, copied transmission, timeout, recovery and wrap passed using a Linux memory-mapped peripheral model, not a physical USART.
- Scheduler test: lost boot announcement, periodic late-attachment policy, timeout accounting and watchdog-startup fault passed with controlled board functions.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: all 17 tests passed on Python 3.13.5. This includes a real Tk event loop with serial PTYs, three screens, exact-ceiling interpretation, actual C encoder fixture replay, malformed/resynchronized data, EOF truncation, incompatible policy, later reset, stalled freshness, gradual backlog, immutable halt, serial-open failures, terminal restoration, disconnect, private JSON export and overwrite refusal.
- `python3 -m compileall -q app`: passed.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native KiCad sources, authored symbols, routed board, project settings, manifest and XML netlist.
- `/usr/bin/python3 tools/check_hardware.py`: KiCad 9.0.2 reported zero ERC violations, zero DRC violations and zero unconnected items. Fresh schematic XML, saved XML, PCB pad nets and manifest agree on 14 components, 30 pads and 9 nets; component values and full footprint identifiers also match.
- Schematic SVG exported with kicad-cli, rasterized and visually inspected. Signal labels, series resistors, pulldowns and headers are legible without obvious symbol/label collisions. Rendering is not electrical validation.
- `python3 tools/check_release.py`: passed minimum 2,000-word README and 500 substantive production C-line gates. Measured README: 3,280 whitespace-delimited words. Measured production C: 564 nonempty lines after comment removal, excluding headers and tests. All deliverable text files include jayis1 attribution in format-valid content.
- Advisory read-only source review found no concrete weighted-engine, companion or pin-mapping defect. Its only documentation finding was the then-missing verification file, resolved by this document. Review is advisory, not a substitute for executed owner checks or laboratory validation.

## Important limitations and release-to-lab gates

No fabricated board, soldered assembly, flashed MCU, real ST-LINK serial path, electrical measurements, hardware watchdog trial, calibrated sampling clock, ESD evaluation, enclosure validation or operational target assessment was performed. Passive ERC cannot validate external power, voltage compatibility or the truth of diagnostic signals. The design is not fabrication-qualified or certified safe.

Costs are fixed integers assigned to four unique resources; they are not measured physical consumption. The observer cannot authenticate diagnostics, infer hidden operations or enforce a budget. Sparse companion constraints are necessary bounds rather than an exhaustive hidden-history proof. Lost-boot resets may be indistinguishable. Volatile evidence disappears when the MCU resets, and exported evidence is unsigned and unencrypted.

Before physical use, independently verify the purchased module revision, connector orientation, UART solder bridges, common ground, supply sequence, input loading and thresholds. Reestablish a genuinely empty target pool before each run. Exercise all tags, both exact-ceiling pairs, arbitrary release order, rolling replacement and terminal fault paths against independent waveform capture. Use only explicitly authorized low-energy laboratory fixtures.

## Reproduction

From the device directory:

    make -C firmware all test
    xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
    python3 -m compileall -q app
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py

The final publication commit and remote-branch verification are reported by the owner after push; no prospective remote success is asserted by this document.
