# Slot Alias verification record

Author: jayis1.

## Execution mode and scope

Standalone-owner implementation with advisory review, not full collective approval. Only slot-alias/ was added. Existing device files were not changed. Repository origin was verified as jayis1/hacker-devices, configured default branch main; the initial clean checkout was updated with `git pull --ff-only`. No task branch was created. Publication commit identity and remote equality are verified separately at delivery.

## Executed software checks

- Initial engine test compile failed because the new engine header did not yet exist; implementation followed the failing specification.
- `make -C slot-alias/firmware clean all test` passed from a clean build directory.
- Arm GNU GCC 14.2.1 linked the Cortex-M4 target with `-Wall -Wextra -Werror`. ELF size: text 3444 bytes, data 0 bytes, BSS 1668 bytes. Binary and linker map were produced; build artifacts are ignored.
- Host GCC 14.2.0 ran the engine with AddressSanitizer and UndefinedBehaviorSanitizer. Tests include duplicate live identity, orphan release, lawful reuse, all 24 release permutations, 10,000 operations compared against an independent occupancy bitset, 100,000 randomized samples, setup acceptance/rejection boundary, unqualified raw excursions, cross-channel chatter, overlapping strobes, sampling loss, wrap and arithmetic exhaustion.
- UART tests passed with a Linux register model, including bounded submissions, stalled transmission, recovery and timer wrap. These are not physical USART measurements.
- Production main-loop scheduler tests passed for periodic policy after a lost boot frame and watchdog initialization failure before first status.
- `xvfb-run -a python3 -m unittest discover -s slot-alias/tests -p test_app.py -v` passed all 12 tests using Python 3.13.5. Tests exercised real Tk event loops and Linux pseudo-terminals, all three tabs, synthetic production-encoder traffic, hangup, replay, private export, overwrite refusal, failed connection preservation, malformed/CRC-valid impossible snapshots, freshness, backlog, reset suspicion, FIFO rejection and file-size limits.
- `python3 slot-alias/tools/check_release.py` passed: 3103 README words, 559 substantive nonblank C implementation lines excluding comments, and 108 header lines. Tests are not counted as firmware implementation.

## Executed hardware checks

KiCad 9.0.2 generated native schematic, routed PCB, project, authored symbol library, XML netlist and connectivity manifest. `tools/check_hardware.py` ran fresh ERC and DRC with violations treated as errors: zero ERC violations, zero DRC violations, zero unconnected items. It independently compared schematic netlist nodes and actual PCB pads against the manifest: 14 components, 30 electrical pads, 9 nets, with matching values and full footprint identities. The saved netlist also matched.

The schematic was exported to SVG, rendered to PNG, and visually inspected. Connector labels and resistor channels were readable without obvious overlaps. This does not validate the external module, power domain, connector harness or real component dimensions. Generated preview images were inspection scratch artifacts rather than fabrication deliverables.

## Review and attribution

A read-only independent review found no blocking policy or conservation defect. Its suggested setup-boundary, raw-excursion and independent-qualification regressions were added and passed. Owner review corrected two accidental substring replacements in shared-platform adaptation: a desktop docstring and the release checker's Git repository-root option. All final checks were rerun after correction.

The release checker verifies jayis1 attribution in every deliverable, rejects symlinks and scans for obvious embedded private keys and credential literals. `git diff --check` passed. This is a bounded source inspection, not a comprehensive secret-scanning or supply-chain certification. Third-party toolchains and KiCad standard footprint identities are not claimed as original inventions.

## Limits and outstanding physical work

No board was fabricated, assembled, programmed or electrically tested. Timing, GPIO behavior on silicon, HSI accuracy, watchdog timing, electrical loading, thresholds, USB behavior, power sequencing, back-power, ESD/EMC, environment and regulatory compliance remain unverified. The carrier has no isolation or qualified overvoltage protection.

Manufacturer document retrieval was attempted through web extraction and direct HTTPS. Extraction was unavailable for the configured search backend; ST PDF download failed with an HTTP/2 error and an HTTP/1.1 retry timed out. The existing repository NUCLEO-F401RE mapping was reused. Current ST UM1724, RM0368 and DS9716 must be checked against the actual board before physical construction or wiring. This release is an executable engineering design, not a manufacturing-ready or calibrated instrument.
