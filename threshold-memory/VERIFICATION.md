# Threshold Memory verification

Author: jayis1.

Execution mode: standalone owner using advisory reports, not a completed collective or independent peer review. Only the threshold-memory directory was authored in this run. The destination was verified as jayis1/hacker-devices, configured default branch main; initial git pull --ff-only origin main reported up to date.

## Executed checks

- GNU Arm Embedded GCC 14.2.1, C11, Cortex-M4/Thumb, warnings as errors: clean target compile and link passed. ELF section report: 3268 bytes text, zero initialized data, 1660 bytes BSS. BIN and map generated under ignored firmware/build.
- Production engine and wire encoder: AddressSanitizer and UndefinedBehaviorSanitizer tests passed. Covered unknown middle-band attachment, both hysteresis histories, exact settling deadline, missing/spurious alarm episodes, inhibit, acquisition gaps, ordinary timestamp wrap, transactional exhaustion, encoder bounds, output chatter, all 256 four-plateau severity sequences, and 100000 deterministic arbitrary-signal stress samples.
- Production UART register model: bounds, busy rejection, timeout, recovery, and wrap tests passed under UndefinedBehaviorSanitizer. This is a mapped Linux register model, not measured silicon behavior.
- Production main loop with simulated board I/O: one-shot boot, repeated policy for late attachment, periodic status, normal acquisition, and halted watchdog-start failure passed.
- Python 3.13.5 / Tk application: seven unittest cases passed under Xvfb. Tests use the real C encoder fixture, strict parsing, CRC-valid impossible evidence, truncated-frame resynchronization, replay EOF, reset/rollback detection, duplicate freshness, reception pause, private export and overwrite refusal, FIFO/oversized replay rejection, and real Tk event-loop serial ingestion through a Linux PTY. All three screens were exercised; double connect and hangup preserved evidence correctly.
- KiCad CLI 9.0.2 schematic ERC: zero violations.
- KiCad PCB DRC: zero violations, zero unconnected items.
- Fresh schematic netlist, saved netlist, PCB and manifest: exact agreement across fourteen components, thirty pads, nine nets, component values and footprint identities.
- Native schematic exported to SVG, rendered to PNG, and visually inspected: readable labels, clear row grouping, no observed label/pin collisions, and explicit ground-only harness.
- Release gate: README 3246 whitespace-delimited words; 541 nonblank, comment-stripped production .c lines, excluding headers and tests. Each deliverable contains jayis1 attribution. Git whitespace check passed. Build products and local CAD preferences excluded.

The initial firmware test failed before engine.h existed; the initial app test failed before contract.py existed. Both were deliberate test-first checks. A first PTY integration test delivered 800 ms of simulated device time as a burst and correctly tripped the application's forward-progress guard; the test was corrected to pace fixture records at their real 20 ms intervals. No production guard was weakened to accommodate that fixture.

## Reproduction

Run from the repository root:

    make -C threshold-memory/firmware clean all test
    xvfb-run -a /usr/bin/python3 -m unittest discover -s threshold-memory/tests -p test_app.py -v
    /usr/bin/python3 threshold-memory/tools/check_hardware.py
    python3 threshold-memory/tools/check_release.py

## Explicitly outstanding

No hardware was fabricated, programmed, powered, or connected to an operational target. Electrical loading, actual oscillator and watchdog behavior, response timing, board-revision compatibility, USB interoperability, ESD, brownout behavior, hot plug, enclosure fit, and regulatory suitability require laboratory qualification. CAD rule checks establish source consistency, not fabrication readiness or safe field use. There was no worldwide prior-art search. The app and protocol provide plausibility checks, not authenticated measurement evidence.
