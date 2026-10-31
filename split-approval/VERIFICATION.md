# Split Approval verification record

Author: jayis1.

This record covers software execution and native CAD validation only. It does not claim a fabricated carrier, programmed physical MCU, measured electrical behavior, calibrated timing, independent identity verification, or production qualification. Work was performed in standalone-owner mode with advisory material, not full collective approval.

## Executed checks

- Required initial `git pull --ff-only`: already up to date on configured default branch main in jayis1/hacker-devices. Initial worktree was clean. Device directory and README-title inventory preceded selection; closest contracts inspected were Owner Bind, Coincidence Guard, and Barrier Mask. README keyword search found no existing self-approval / proposer contract.
- Test-first check: compiled the new policy test against the inherited same-client-return policy. It failed at the assertion requiring a different actor's approval to succeed. The revised policy then passed.
- `make -C firmware all test`: PASS, including actual Cortex-M4 target linking with arm-none-eabi GCC 14.2.1, `-Wall -Wextra -Werror`, startup and linker script. ELF text 3296 bytes, initialized data 0 bytes, BSS 1660 bytes. Binary and map generated locally and excluded from Git.
- Production engine tests: PASS, 9352 independent array-oracle branches, plus same-proposer parallel pending records, both approval orders, reuse, self-approval, orphan and duplicate operations, ambiguous strobes, short pulses, held strobes, raw intrusion, identity chatter, timestamp wrap, gaps, and transactional exhaustion. AddressSanitizer and UndefinedBehaviorSanitizer enabled for the engine test.
- Production serial driver tests: PASS, bounded submissions, timeout boundary, recovery, and timestamp wrap under a Linux register model, not physical USART hardware.
- Production main-loop scheduler tests: PASS, lost boot submission, periodic late-attachment policy, transmission timeout, and watchdog initialization failure. Controlled board/transport functions substitute for the physical board.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: PASS, 15 tests. Includes actual Tk event loop and Linux PTY reception/hangup, all three screens, duplicate connection refusal, C-encoder replay, private JSON export, overwrite refusal, bounds, malformed framing, semantic rejection, freshness, and frozen evidence.
- `python3 -m compileall -q app`: PASS. Dynamic test imports resolve during actual execution; no third-party Python package installed.
- `/usr/bin/python3 tools/generate_hardware.py`: generated real native KiCad sources and XML netlist using KiCad 9.0.2.
- `/usr/bin/python3 tools/check_hardware.py`: PASS, zero ERC violations, zero DRC violations, zero unconnected items. Compared 14 components, 30 pads, and 9 nets across fresh schematic XML, stored XML, actual PCB pads, values, full footprint identities, and manifest.
- Schematic SVG exported using kicad-cli, rasterized, and visually inspected. Four distinct input networks, ground-only header, resistor values, pin labels, and jayis1 title metadata are legible and connected by matching net labels.
- Release gate before this record: 3149 README words, 554 nonblank production C lines after comment removal, and all 33 then-present deliverables attributed to jayis1. Adding this record increases the deliverable count by one; rerun the checker for the final count.
- Read-only fresh-context review found no blocking code or hardware-generator defect. It identified the then-missing verification record; this document resolves that finding. Review is advisory, not collective sign-off.

## Interpretation and remaining gates

The instrument checks two diagnostic actor identities, not independent human consent or cryptographic principals. It does not authenticate telemetry, bind proposal payloads, prevent stale approvals after slot reuse, or prove completeness between samples. The app checks necessary sparse-snapshot constraints rather than reconstructing all hidden operations. All synthetic fixtures are labelled as such; no fabricated target measurements are supplied.

Confirm the external board revision and pin map against ST UM1724 before wiring. Fabrication inspection, SWD programming, power sequencing, connector continuity, signal levels, loading, ground offsets, watchdog recovery, calibrated timing, sustained serial throughput, EMC/ESD, enclosure protection, and target interoperability remain physical acceptance work. The passive schematic's ERC cannot validate these properties.

Build products, GUI caches, and KiCad local preferences are ignored. The native project, schematic, routed PCB, embedded symbol library, netlist, and connectivity manifest are release sources. No task branch was created. Publication verification is performed against the remote main ref after committing; the final run report supplies the resulting commit identifier.
