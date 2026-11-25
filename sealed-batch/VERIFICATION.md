# Sealed Batch verification record

Author: jayis1.

## Scope and readiness

Engineering prototype sources, not fabricated or field-qualified hardware. The sole implementation owner incorporated advisory review and independently ran the checks below. This was degraded standalone-owner execution, not full collective review. No physical measurements, programmed-target results, production readiness or security certification are claimed.

Repository: jayis1/hacker-devices. Default branch: main. Initial base: 0183b8cac745dedabd614e84c668acaa39279c87. The initial worktree was clean and `git pull --ff-only` returned already up to date. All 156 top-level non-hidden directories were inventoried, including the non-device docs directory; names and README purpose summaries were examined before selection. Domain Cohort and Distance Fit were examined in detail. The existing collective runner held the repository lock; the old empty `.git/cron-owner.lock` directory was left untouched. No temporary task branch was created.

## Executed checks

- GNU Arm Embedded GCC 14.2.1 (20241119, package 15:14.2.rel1-1), host GCC 14.2.0, Python 3.13.5 and KiCad 9.0.2 were used. These are the tested toolchain versions, not claims about latest releases.
- `make -C firmware all test`: target STM32F401RE ELF and BIN linked with warnings as errors. Size output: 3700 bytes text, zero data, 1696 bytes BSS. Source includes the real reset vector, linker memory map, register drivers, watchdog and nonblocking transmitter; no simulated target image is substituted.
- Host engine tests passed 19,304 operation transitions against an independent explicit-phase oracle under address and undefined-behavior sanitizers. Coverage includes every filling mask, every draining subset of a full batch, duplicate-before-seal precedence, full-drain reuse, raw setup rejection, simultaneous strobes, gap freeze, exhaustion rollback and timestamp rollover.
- Serial register-model tests passed bounds, busy rejection, timeout, recovery and timestamp wrap. These are Linux mapped-register tests, not physical USART measurements.
- Production main-loop tests passed lost initial boot, periodic compatibility announcements, serial timeout accounting and watchdog startup failure. Board calls were controlled fixtures, not a running MCU.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v`: all 16 tests passed. They exercise independent legal sparse prefixes, CRC-valid contradictory snapshots, real C-encoder synthetic recordings, framing recovery, replay bounds, stale transport, permission denial, termios restoration, private export and overwrite refusal.
- Actual Tk Ledger, History and Integrity screens ran under Xvfb. A Linux pseudo-terminal delivered a draining batch, opening a second connection was refused, attributes were restored, hangup was detected, the real C-encoder replay produced the expected fault and retained rows, and the export was read back. This is application integration, not physical USB verification.
- The first firmware test failed against the inherited platform because the batch state did not exist. Independent advisory review later identified a sparse completed-batch peak underreport. A dedicated CRC-valid regression reproduced that failure; including peak in the bounded ordering search fixed it and the complete suite passed afterward.
- The first app run also caught two malformed negative-test fixtures that violated snapshot invariants before reaching the intended transition check. Their historical counters were corrected; validation was not weakened.
- `/usr/bin/python3 tools/generate_hardware.py` generated actual native schematic, routed PCB, project, symbol library and XML netlist.
- `/usr/bin/python3 tools/check_hardware.py`: zero ERC violations, zero DRC violations and zero unconnected items. Fresh schematic connectivity matched all 14 components, 30 pads and nine nets against PCB and manifest, including values and full footprint identities.
- The schematic was exported to SVG with KiCad and rasterized using rsvg-convert. Visual inspection confirmed the ground/module headers, four input resistor channels, readable separated labels and jayis1 title attribution. CairoSVG was unavailable; rsvg-convert successfully supplied the render instead.
- Release checks reported 2996 README words and 606 nonblank production C lines after comment removal; tests, headers and generated CAD are excluded from that C count. Reused platform implementation is explicitly credited and distinguished from device-specific logic.
- The release checker verifies author attribution in every deliverable, native CAD file presence, absence of private-key markers and whitespace errors. Build products, bytecode and local KiCad preferences are ignored.

## Interpretation limits and remaining gates

Neither host tests nor CAD checks establish real acquisition timing, electrical compatibility or manufacturability. Fabrication, programming, oscillator tolerance, source loading, ground offsets, power sequencing, brownout, physical watchdog reset, USB behavior, ESD and fixture interoperability require separate bench work. Standard module header references must be reconciled against the actual purchased board and solder bridges before assembly.

The observer trusts owner-provided diagnostics and cannot prove a target reports all committed operations. CRC is not authentication. Sparse search establishes existence of some count-consistent ordering for at most eight accepted operations, including peak, not that the hidden actual history was legal. Longer gaps receive necessary conservative checks. No persistent boot identity or secure update mechanism is provided.

No battery, radio, network service, target command channel or device storage exists. Low-battery testing is therefore inapplicable; USB brownout remains unverified. Host export open failures are tested with injected errors, not a physically exhausted filesystem. Interrupted programming must be recovered through physical SWD and followed by revalidation; no automated updater is claimed.

Publication is performed on main with jayis1 as author and committer. The completion response records the actual commit and remote read-back; this file does not claim a future push already occurred.
