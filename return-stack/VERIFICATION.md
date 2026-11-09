# Return Stack verification record

Author: jayis1.

## Release scope

Recovered the unfinished return-stack package after tracing its provenance to an interrupted execution of this same authorized repository task. Completed the device documentation and generated the actual native CAD files. The repository destination is jayis1/hacker-devices and the remote-configured default branch is main. No temporary task branch, shared systems tree, workflow or daily-ideas change is part of this release.

The acceptance claim is a buildable, tested engineering design package, not a fabricated or physically characterized device. Review was performed in standalone-owner mode. Advisory peer reports did not constitute an independent implementation review or physical validation.

## Executed checks

Commands are run from the repository root. Build outputs and temporary reports are excluded from the commit.

- `make -C return-stack/firmware clean all test`: passed. ARM image linked with 3,808 bytes text, zero bytes initialized data and 1,708 bytes BSS. ELF, raw binary and linker map were generated.
- Engine tests under AddressSanitizer and UndefinedBehaviorSanitizer: 19,024 transitions compared with an independent literal free-list oracle, plus all 24 return permutations and explicit fault/timing boundaries. Passed.
- Serial driver tests under UndefinedBehaviorSanitizer: submission bounds, overlap refusal, modeled register transmission, exact timeout, unsigned time wrap and subsequent recovery. Passed. This is a Linux register model, not real UART silicon.
- Production main-loop harness: lost boot frame, repeating late-attachment policy, transport timeout accounting and watchdog-start failure. Passed. Board operations are controlled test doubles, not physical hardware.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s return-stack/tests -v`: 24 tests passed. Includes actual tkinter screens, Linux PTY receive/hangup and terminal restoration, real C-encoder synthetic fixtures, immediate head reuse, sparse priority barriers, return/admission stamp migration, malformed framing, foreign protocol rejection, private export, overwrite refusal, bounded replay and retained evidence on connection failure.
- `/usr/bin/python3 return-stack/tools/generate_hardware.py`: generated native schematic, PCB, project, symbol library, library table, XML netlist and connectivity manifest. Fourteen components on nine nets.
- `/usr/bin/python3 return-stack/tools/check_hardware.py`: ERC zero violations; DRC zero violations and zero unconnected items. Fresh schematic netlist, saved netlist, PCB and manifest matched all fourteen component values/footprints and all thirty pad/net assignments across nine nets.
- Schematic PDF/SVG and PCB SVG were exported with KiCad. Rasterized schematic and a cropped board view were visually inspected for labels, component organization, ground routing and carrier outline. This is a layout readability check, not a fabricated-board inspection.
- `/usr/bin/python3 return-stack/tools/check_release.py`: README 3,381 whitespace-delimited words; production C 619 nonblank, comment-stripped lines, excluding headers and tests. Attribution and required-artifact checks passed before adding this verification record and are rerun for publication.
- `git diff --check`: passed. Staged paths and staged content are inspected before commit; every deliverable contains jayis1 attribution in a format-compatible field or comment.

## Tool versions

- GNU Arm Embedded GCC 14.2.1, release 14.2.rel1.
- Host GCC 14.2.0, Debian package 14.2.0-19.
- System Python 3.13.5 with tkinter, running the application suite under Xvfb.
- KiCad CLI and pcbnew 9.0.2, with the installed standard footprint libraries.

## Documentation and source verification limits

The direct ST PDF download failed with an HTTP/2 error and then an HTTP/1.1 timeout. An alternative indexed retrieval of ST's UM1724 Rev 17 provided the F401RE CN8 pin mapping and section 7.10/default USART bridge configuration; those agree with the documented harness and board driver. Before assembly, consult the manufacturer's complete current document and the actual module revision. This release does not claim a full fresh datasheet audit or current supplier availability. The first attempted Python SVG rasterizer was absent; installed PDF/rsvg tools supplied the successfully inspected renderings instead.

The source review confirmed input-only target GPIO configuration, disabled USART receiver, bounded UART framing, speculative accounting on counter exhaustion, terminal halt retention, no network transport and exclusive private exports. These are implemented safeguards, not proof against a compromised workstation or replaced firmware. Sparse telemetry validation is intentionally necessary-but-incomplete: CRC and plausible accounting do not authenticate history.

## Outstanding physical work

No PCB was fabricated, populated or electrically measured. No MCU was flashed during this run. Hardware behavior, HSI drift, baud tolerance, actual sampling jitter, connector mechanics, power consumption, watchdog reset, ESD performance, EMI, signal loading and target adapter correctness remain unverified. The carrier is unisolated and has no external overvoltage clamps. It must not be used as a safety interlock or attached to consequential or unknown-voltage systems.

Before deployment, verify purchased part dimensions and board bridge configuration, perform unpowered continuity checks, program under supervision, measure input and ground limits, validate slow diagnostic waveforms against a scope, and compare target traces with the observer's results. Preserve baseline assumptions and limitations in every research report. A successful build and zero CAD rule violations are not fabrication approval or security certification.
