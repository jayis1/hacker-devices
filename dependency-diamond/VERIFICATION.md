# Dependency Diamond verification

Author: jayis1.

## Scope and review mode

This release was implemented and checked in standalone-owner mode. Advisory planning reports informed the checks; they are not an independent review of the resulting code or a collective hardware sign-off. No physical target or observer board was used. The README deliberately calls this an unbuilt engineering prototype, not a fabrication-qualified product.

The repository was confirmed as jayis1/hacker-devices, with remote default branch main. A clean checkout was pulled with `git pull --ff-only origin main` before inventory and implementation. No task branch was created. Only dependency-diamond is in the publication scope; daily-ideas was not touched.

## Executed checks

- `make -C firmware clean all test`: passed. Arm GNU GCC 14.2.1 linked a Cortex-M4 image with 3,220 bytes text, zero initialized data bytes, and 1,652 bytes BSS. The linker reserves stack headroom in the STM32F401RE memory map. This establishes compilation and linking, not successful flashing or execution on silicon.
- Host GCC 14.2.0 engine tests: passed under AddressSanitizer and UndefinedBehaviorSanitizer. The test enumerates 40,320 permutations of eight unique START/FINISH events and compares each prefix through its first illegal event against a separately expressed dependency oracle. Additional regressions cover overlapping siblings, repeated rounds, duplicate starts before and after completion, missing join predecessors, simultaneous strobes, opposite raw strobe at qualification, insufficient setup, held strobes, unrelated tag chatter, acquisition loss, timer wrap, bounded encoding, impossible graph state, and transactional overflow at final completion.
- Serial register-model tests: passed under UndefinedBehaviorSanitizer. Covered submission bounds, busy refusal, byte output, timeout, recovery, and timestamp wrap. Register mapping is a Linux test double, not a physical USART.
- Main-loop tests: passed under UndefinedBehaviorSanitizer. Covered a lost initial boot frame, repeated policy for late host attachment, busy output, transmitter timeout, and watchdog initialization failure before first status.
- `xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v`: all 14 tests passed using Python 3.13.5. Covered graph and conservation rejection, impossible sibling identity swaps, valid new-round transitions, checksum-valid invalid evidence, wrong protocol/policy, duplicate freshness, reception pause, rollback, halted immutability, framing resynchronization, partial EOF, replay restrictions, and private export.
- GUI integration tests used the actual Tk event loop and Linux pseudo-terminals with production C-encoder synthetic records paced by device timestamps. Ledger, History, and Integrity screens were exercised; export contents and mode 0600 were checked; duplicate connection was refused; hangup removed live status while retaining provenance; replay replaced the session. No physical USB path was tested.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native KiCad 9 files, authored symbols, routed board, connectivity manifest, and XML netlist.
- `/usr/bin/python3 tools/check_hardware.py`: KiCad CLI 9.0.2 reported zero ERC violations, zero DRC violations, and zero unconnected items. Fresh schematic netlist, saved netlist, PCB pads, values, full footprint identities, and manifest agreed on 14 components, 30 pads, and 9 nets. Passive-symbol ERC does not establish external-module power correctness.
- Schematic and combined copper/silkscreen SVG exports were rendered and visually inspected. Net labels, component references, ground-only header warning, outline, and input-only voltage warning were legible, with no visible overlap requiring correction. This is not a manufacturing or connector-fit review.
- `python3 tools/check_release.py`: README exceeds 2,000 words; production C files contain 548 nonblank, noncomment lines, excluding tests and headers. Every deliverable contains jayis1 attribution. Required native CAD files exist. Authorship checks, private-key marker scan, and whitespace checks pass.

## Corrections during validation

The first overflow regression initialized observed too close to exhaustion, so observation counting halted before the intended operation-count boundary. The fixture was corrected to retain observation headroom while preserving necessary conservation; the final-completion rollback test then passed. No production behavior was weakened to satisfy it. Initial PCB SVG export used a directory with KiCad's single-file default; rerunning with explicit `--mode-single` and a filename produced the inspected render.

## Unverified and excluded claims

No fabrication, MCU flashing, electrical bring-up, current measurement, waveform acquisition, calibrated timing, watchdog timing measurement, brownout experiment, ESD test, EMC test, enclosure validation, or field deployment occurred. The external Nucleo pin map and register definitions are reused from the existing platform; current manufacturer manuals and the purchased board revision must be checked before connection. Standard footprint geometry is reused, not claimed as newly invented library work.

The instrument relies on an owner-provided diagnostic adapter and an operator-established empty graph. It cannot authenticate target claims, detect every missing notification, prove cryptographic validation, certify hardware isolation, or guarantee continuous session identity across an unobserved reset. A stalled graph has no deadline and remains incomplete. Tests establish the documented sampled policy and software transport behavior, not the security of a real target.
