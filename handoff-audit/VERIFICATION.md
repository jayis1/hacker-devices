# Verification record

Author: jayis1

## Executed checks

- Repository identity: `https://github.com/jayis1/hacker-devices.git`; remote HEAD is `main`. Initial worktree was clean. `git pull --ff-only origin main` completed before implementation. The daily-ideas repository was not used.
- Inventory: inspected device directory names and README titles, then the nearby grant-ledger implementation and documentation. New measurement policy is paired command/feedback break-before-make auditing; shared carrier/support infrastructure is intentionally reused, not claimed novel.
- `make -C firmware clean all test`: passed with ARM GNU GCC 14.2.1 and native GCC. Target ELF: 2800 bytes text, 0 initialized data, 1388 BSS. The linker checks 4 KiB stack headroom; this is not a measured maximum stack depth.
- Production policy and wire encoder executed under AddressSanitizer and UndefinedBehaviorSanitizer. Tests cover normal and direct transfers, exact 19/20 ms boundary, same-path return, independent overlap episodes, both mismatch channels, mismatch threshold/recovery, sampling loss and resynchronization, tick wrap, saturating duration/counters, queue exhaustion, CRC known vector, null input and encoder capacity bounds.
- Production UART driver executed with modeled Linux MMIO under UndefinedBehaviorSanitizer: invalid lengths, buffer ownership, stalled transmit, timeout, wrap and recovery passed. No claim of silicon MMIO timing validation.
- Actual production main loop executed with deterministic board/transport doubles. Simulated loss of the first boot transmission increments tx_fail and does not prevent subsequent periodic policy and status transmissions.
- `python3 -m unittest discover -s tests -p test_app.py`: five tests passed. Tests exercise malformed/split/truncated frames, bounded buffering, live/replay freshness, missing initial policy, incompatible policy recovery, timestamp rollback, boot reset, screens, private export and overwrite refusal.
- `python3 tests/integration.py`: passed using frames from the real C engine/encoder. Verified replay export, loss of the initial boot frame followed by policy recovery without clearing status, separate serial/UI pseudo-terminals, real curses navigation, live export, mode 0600, overwrite refusal, disconnect and clean exit.
- `python3 -m compileall -q app tests`: passed. Application runtime is Python 3.13.5 standard library; no external package installation or lockfile is needed.
- KiCad 9.0.2 generation and checker passed: zero ERC violations, zero DRC violations, zero unconnected items. Fourteen components, thirty component pins and nine nets match across fresh exported schematic netlist, saved device.net, actual PCB pads, values, footprint library identities and connectivity.json.
- Schematic PDF rendering and PCB SVG rendering were visually inspected. Real connector/resistor symbols, net labels, routed front/back copper, outline, safety labels and jayis1 attribution are present. These inspections do not constitute fabrication release.
- UM1724 Rev 13 cached manufacturer text: Table 16 confirms A0/PA0, A1/PA1, A2/PA4, A3/PB0 and USART2 TX/PA2; section 6.8 confirms default ST-LINK virtual-COM solder bridges. Fresh manufacturer PDF downloads timed out; no fresh-document retrieval is claimed.
- README: 3336 whitespace-separated words. Production C and headers: 587 nonblank, noncomment lines, excluding tests and build products. Counts are secondary evidence, not a substitute for functional tests.
- Every deliverable includes jayis1 attribution using comments or native metadata. Build products, Python caches and KiCad local preferences are ignored.

## Review resolution

Read-only code review identified that a one-shot boot packet would prevent LIVE forever after late attachment or initial transmission loss. Separate periodic HA1,P policy announcements now recover compatibility without clearing measurements or incrementing boot sessions. The production-main and app/encoder regressions above cover this behavior. Policy announcements deliberately do not clear timestamp-regression suspicion. All edits were performed by the task owner; advisory reviewers made no repository changes.

## Explicitly unverified

No hardware was fabricated, assembled, flashed or measured. GPIO timing, HSI/LSI accuracy, watchdog reset on silicon, electrical thresholds, power sequencing, current draw, back-powering, USB control-line effects, loading, EMI/ESD, EMC, regulatory compliance, environmental performance and enclosure suitability remain unverified. The carrier has no isolation, active protection or independent sensing of real switch continuity. A receive-only design can still disturb an electrically incompatible target.

Host tests, pseudo-terminals and CAD rule checks do not establish instrument accuracy or security certification. The README describes a laboratory engineering prototype and a physical acceptance checklist rather than claiming a finished validated hardware product.
