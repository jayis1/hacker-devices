# Distance Fit verification

Author: jayis1.

## Scope and provenance

Recovered interrupted output of the same device-design task, identified through its originating task transcript. Repository: jayis1/hacker-devices. Remote HEAD was main; required `git pull --ff-only` returned already up to date at starting commit 62d2c1d53f7ca719d9cb6152b95644b5dd2f19fd. No temporary task branch was created. The standalone owner reviewed and executed the implementation; advisory peer reports were not an independent review of these files. This is degraded standalone-owner validation, not full collective approval.

## Executed checks

Commands below run from the repository root unless noted otherwise.

- `make -C distance-fit/firmware all test`: passed. GNU Arm GCC 14.2.1 linked the Cortex-M4 target ELF and produced the binary. Reported sections: text 3724 bytes, data 0 bytes, BSS 1692 bytes. `arm-none-eabi-readelf -h` confirmed an ARM ELF32 executable, soft-float EABI, entry address 0x08000175. Linking is not physical execution.
- Host GCC 14.2.0 built the engine suite with AddressSanitizer and UndefinedBehaviorSanitizer. It passed 10,408 independent wavefront-oracle transitions, all occupancy masks, repeated operations, tie behavior, arbitrary returns, duplicate/orphan context, raw ambiguity, acquisition loss, timestamp wrap and exhaustion tests.
- UART host tests passed buffer bounds, timeout, recovery and wrap checks. Main-loop host tests passed periodic late-attach policy, lost boot, timeout and watchdog-startup cases. Hardware register interactions in these suites are simulated; no MCU was attached.
- `xvfb-run -a python3 -m unittest discover -s distance-fit/tests -p 'test_app.py' -v`: all 15 tests passed under Python 3.13.5. Coverage includes legal sparse prefix pairs, impossible CRC-valid endpoints, large-gap persistent occupancy bounds, real C-generated synthetic fixtures, malformed framing, replay bounds, private export, overwrite refusal, permission failure and termios restoration.
- The same suite exercised the actual Tk event loop, all three screens, serial pseudoterminal input, disconnection, replay replacement and JSON export. These are synthetic inputs, not physical USB or target measurements.
- `/usr/bin/python3 distance-fit/tools/generate_hardware.py`: generated native schematic, PCB, project, symbol library, connectivity manifest and schematic netlist using KiCad 9.0.2.
- `/usr/bin/python3 distance-fit/tools/check_hardware.py`: ERC zero violations; DRC zero violations and zero unconnected items. Fresh schematic export, saved netlist, PCB and manifest agreed for 14 components, 30 pads and 9 nets, including exact values and footprint identities.
- `kicad-cli sch export svg` produced a schematic render. CairoSVG was unavailable; `rsvg-convert` successfully converted the SVG for visual inspection. Components and labels are visible, separated and within the sheet. The external MCU module remains a purchased component wired by harness, not part of the carrier PCB.
- `python3 distance-fit/tools/check_release.py`: passed with 2,837 README words and 613 substantive nonblank production C lines after stripping comments. The initial run checked 33 attributed deliverables; this report adds one authored deliverable. The checker requires jayis1 attribution in every deliverable and detects private-key markers.
- `git diff --check`: passed. Final staging and remote read-back are performed as publication gates after this report is written.

## Explicit limits

No fabrication, physical programming, electrical continuity measurement, oscilloscope timing, EMC/ESD assessment, power characterization, target integration or regulatory validation was performed. Zero ERC/DRC findings do not validate the external supply, module revision, harness or signal voltage. Board files describe an unbuilt carrier, not a production-qualified product.

The witness depends on an owner-supplied ordered diagnostic stream and an independently established empty pool. It does not discover resources or authenticate target statements. Sparse host checks are necessary constraints, not full history reconstruction; bounded count searches deliberately ignore timing. CRC is not a signature. Receive-only firmware is not an immutable data diode. Reset provenance is heuristic without persistent boot identity. These limitations are part of the evidence interpretation, not hidden verification omissions.
