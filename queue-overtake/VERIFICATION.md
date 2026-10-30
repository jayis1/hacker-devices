# Queue Overtake verification

Author: jayis1.

## Executed checks

This release was implemented and self-reviewed in standalone-owner mode. Advisory reports were planning input, not independent implementation approval. All results below came from actual local execution; no physical target was connected.

- Repository identity: `https://github.com/jayis1/hacker-devices.git`; remote HEAD identifies `main`. Initial worktree clean; `git pull --ff-only` reported already up to date. Existing device directories and relevant README contracts were inspected before selecting the rolling tagged FIFO concept.
- Target build: `make -C firmware all test`, with arm-none-eabi GCC 14.2.1, host GCC 14.2.0, C11, warnings as errors, Cortex-M4 Thumb, soft-float ABI, newlib/nosys, and the supplied flash/RAM linker script. Final target size: 3316 bytes text, zero data, 1660 bytes BSS. ELF inspection identifies ARM executable code with vectors at 0x08000000 and BSS at 0x20000000. No unresolved symbols remain. The linker reserves at least 4096 bytes of stack headroom.
- C engine suite: 46,984 independently checked operation prefixes through depth eight. A separate list oracle includes arbitrary tag ordering, immediate reuse, queue filling, orphan departures, duplicate admissions, and overtakes; terminal branches are pruned only after evidence-freeze assertions. AddressSanitizer and UndefinedBehaviorSanitizer enabled.
- C boundaries: simultaneous operation strobes, opposite raw strobe at qualification, changing tag at strobe, rejected short pulse, held high strobe, setup threshold-minus-one and exact threshold, duplicate tick, timestamp wrap, acquisition gap, counter exhaustion with transactional dequeue rollback, malformed packed queue, peak/queued inconsistency, checksum known vector, and bounded serialization.
- UART suite: actual serial driver against Linux-mapped test registers; invalid sizes, busy refusal, transmitted byte order, timeout, timer wrap, and recovery. UndefinedBehaviorSanitizer enabled. This is not a physical USART test.
- Main-loop suite: production scheduler with controlled board functions; lost boot transmission, repeating policy for late attachment, status scheduling, transport timeout accounting, and failed watchdog startup producing valid terminal evidence.
- Companion: `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v` passed all 19 tests under Python 3.13.5. `python3 -m compileall -q app` passed.
- App coverage: checksum-valid impossible states; illegal silent queue reorder; valid head reuse; forbidden new-head overtaking; sparse valid list histories compared against all earlier snapshots; empty boundaries; policy mismatch; non-refreshing duplicate timestamps; receive pause and gradual backlog; rollback, halt, and boot handling; stream resynchronization; incomplete EOF; private export and overwrite refusal; FIFO and oversize replay rejection; serial setup-failure attribute restoration.
- GUI coverage: real Tk event loop, Linux pseudoterminals, and production C encoder fixture. Checked live reception, all three tabs, displayed head-to-tail queue, retained terminal fault, JSON export, double-connect refusal, real PTY hangup, replay replacement, restored serial attributes, and late-attach fragment recovery. Test fixture observations are synthetic, not measurements from a fabricated board.
- CAD: KiCad CLI 9.0.2. Generator produced actual component symbols and routed board footprints. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML netlist, saved XML netlist, PCB, and manifest agree on fourteen components, thirty pads, nine nets, component values, and full library footprint identities.
- Schematic SVG rendered and visually inspected: four corresponding tag/strobe resistor channels, connector labels, ground-only header, and jayis1 title-block attribution. This verifies presentation, not electrical behavior.
- Release gate: README has 3552 whitespace-delimited words. Production `.c` files contain 563 nonblank lines after stripping comments; headers and tests are not counted. Authorship and private-key-pattern checks passed. `git diff --check` passed. Build products, Python caches, and KiCad local preference files are excluded.

## Review findings resolved

Tests were first run without the engine and companion contract implementations and failed as expected. During owner review, a logically impossible summary was identified: a historical admission behind pending work cannot coexist with a claimed peak depth of one. Added negative tests failed in both C and Python, then the same necessary constraint was added to both validators and all tests passed again. This is a consistency check, not a claim that all sparse summaries are exactly reconstructible.

The repeated platform circuitry and drivers are explicitly identified in README as reuse of existing jayis1 work. The new policy is learned arrival order with rolling tag reuse, not renamed reader/writer exclusion or fixed ordinal batch retirement. Only this device directory is intended for publication; unrelated devices and repository automation are untouched.

## Not verified

No fabrication, physical flashing, oscillator calibration, loaded power measurements, ESD testing, EMC testing, cable qualification, real adapter integration, or environmental qualification was performed. A resistor-only carrier has no isolation, voltage translation, certified overvoltage protection, or hot-plug guarantee. Watchdog operation and GPIO timing are build-tested but not physically exercised. The manufacturer connector map was checked against ST UM1724 search evidence; the actual purchased module revision and jumper settings still require bench inspection.

A malicious adapter or workstation can falsify observations. CRC is not authentication. There is no persistent boot identity, so some resets remain indistinguishable if their boot frame is lost and subsequent values appear plausible. Queue snapshots are periodic retained summaries, not complete event transcripts. Clean software and CAD checks do not establish production readiness or the absence of exploitable defects in an attached target.
