# Baton Order verification

Author: jayis1.

## Scope

Executed software and CAD verification for an unbuilt revision A research instrument. This was a standalone-owner implementation with advisory review, not full collective approval. All changes were applied and reverified by the owner. No physical fabrication, MCU programming, electrical characterization, timing calibration, or target deployment occurred.

## Executed checks

- Repository identity: origin is jayis1/hacker-devices; configured remote default branch is main. Initial worktree was clean. Required `git pull --ff-only` returned already up to date. Existing device directories and related README contracts were inspected before selecting the cyclic ownership concept.
- `make -C firmware clean all test`: passed. ARM GNU GCC 14.2.1 linked a Cortex-M4 image using the explicit STM32 startup/linker layout, with warnings treated as errors. ELF size: 3460 bytes text, zero initialized data, 1720 bytes BSS. The linker reserves additional stack headroom. Binary conversion succeeded.
- Native engine test: passed with AddressSanitizer and UndefinedBehaviorSanitizer. Exercises all sixteen directed transitions, all sixteen input masks, held-input immunity, idle readmission, repeat/skip/reverse classification, collision episode continuity, baseline recovery, independent qualification, gaps, rollover, transactional counter exhaustion, invalid serializer state, and bounded output.
- Engine tests also pass a deterministic 10,000-turn matrix oracle and 50,000 raw-chatter samples with acquisition gaps, checking state validity and serialization throughout.
- Modeled USART test: passed. Covers invalid sizes, busy submission, byte service, timeout, recovery, and timestamp wrap. This is a Linux register model, not physical USART execution.
- Production scheduler test: passed with modeled board functions. Repeated policy survives lost boot transmission. Watchdog initialization failure produces valid terminal state before telemetry and suppresses watchdog feeding.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: eight tests passed using Python 3.13.5. Includes actual C fixture parsing, checksum-valid impossible states, truncated frames/EOF, incompatible policy, duplicate/rollback detection, instantaneous and gradually accumulating backlog, private export, overwrite refusal, regular-file limits, FIFO rejection, and replay.
- GUI test drives the actual Tk event loop and a Linux pseudo-terminal with paced real-encoder output. Verified three screens, correct counters, live progress, double-connect rejection, hangup handling, retained live provenance after disconnect, replay replacement, JSON export, and evidence preservation on failed reconnect. Fixture data is synthetic, not a physical capture.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native schematic, symbol library, project, routed PCB, XML netlist, and connectivity manifest.
- `/usr/bin/python3 tools/check_hardware.py` with KiCad 9.0.2: zero ERC violations, zero DRC violations, zero unconnected items. Fresh schematic XML, stored netlist, actual PCB pad nets, values, footprint identities, and manifest agree for fourteen components, thirty electrical pads, and nine nets.
- Schematic exported to SVG, rendered to PNG, and visually inspected. Signal labels, component references, and title/author fields are readable and separated. Connectivity uses named nets; the passive carrier is not a representation of the external MCU module.
- README measured at 3439 whitespace-separated words. Production firmware has 576 C source lines, including 548 nonblank lines after removing comments. Including headers gives 692 lines, 656 nonblank noncomment lines. Tests, CAD generation, and vendor libraries are excluded from these firmware counts.

## Review corrections

Advisory review reproduced a collision episode that remained active when another channel had never qualified. A new startup-chatter regression failed against the initial engine. Episode closure now uses the same independently qualified known-high predicate as episode entry, before the whole-vector ordering gate. Regression and random raw-sample checks pass.

Advisory review also demonstrated a slowly growing serial backlog hidden by an indefinitely expanding drift tolerance. A new 96-device-ms-per-100-host-ms regression failed against the original freshness check. The added drift allowance is now capped, giving at most one second cumulative divergence. Regression passes. README documents the deliberate risk of freezing a long honest session because oscillator error and buffering are not distinguishable, and the inability to identify initial fixed delay.

## Remaining limitations

Tests establish software execution and CAD consistency, not hardware fitness. External Nucleo power, solder bridges, GPIO thresholds, grounding, harness loading, power sequencing, back-power, actual watchdog behavior, USB operation, oscillator error, ESD/EMC, temperature, insulation, mechanical support, and regulatory acceptance require supervised bench validation. CRC is not authentication. Diagnostic outputs can lie, short events can be missed, and a receive-only observer cannot prove the resource operations represented by those outputs occurred.

The custom PCB contains only passive parts and connectors; ERC does not validate the external module supply or target voltages. No fabrication-ready or production-safe claim is made. All project authorship: jayis1.
