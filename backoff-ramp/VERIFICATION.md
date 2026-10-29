# Backoff Ramp verification record

Author: jayis1.

This is standalone-owner implementation and verification using the supplied advisory reviews, not full collective or independent hardware review. The repository was confirmed as jayis1/hacker-devices, remote default main. It was clean and `git pull --ff-only` reported already current before implementation. No task branch was created. Existing device directories and purpose statements were inventoried, with particular comparison against Retry Ledger, Burst Budget, and Pulse Envelope. No daily-ideas files were accessed or changed.

## Executed software checks

- `make -C backoff-ramp/firmware clean all test` passed. GNU Arm Embedded GCC 14.2.1 linked the Cortex-M4 ELF and generated the binary. Size: text 3352 bytes, data 0 bytes, BSS 1668 bytes. Target compilation uses C11, Wall, Wextra, and Werror.
- Host GCC 14.2.0 ran engine tests with AddressSanitizer and UndefinedBehaviorSanitizer. Tests cover escalation through the cap, success reset, early attempts remaining correlated, exact delay and response boundaries, simultaneous outcomes, orphan outcomes, overlapping attempts, reset, gaps, startup and post-gap reset segments, counter exhaustion while idle and pending, ordinary timer wrap, held strobes, unrelated chatter, and deterministic varied-dwell raw sequences. Every sampled state in the test helper is validated and encoded.
- Production UART-driver tests passed against a Linux memory-mapped register model: invalid submissions, bounded queue ownership, transmit service, timeout, recovery, and timer wrap. This is not a silicon test.
- Production main-loop scheduler tests passed with modeled peripherals: lost initial boot transmission, repeated policy announcements, periodic statuses, watchdog feeding, and watchdog initialization failure before valid acquisition.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s backoff-ramp/tests -p 'test_app.py' -v` passed nine tests on Python 3.13.5. The suite feeds the real C encoder's synthetic fixture into the application; validates impossible snapshots with valid CRC; checks partial-frame resynchronization, EOF errors, late attachment, boot provenance, contradictory duplicates, sparse transition checks, stale duplicates, reception pauses and cumulative lag; verifies private exclusive JSON export and FIFO/oversized replay rejection; and exercises the actual Tk event loop against a Linux PTY, three screens, replay, reconnect refusal, terminal restoration, and hangup.

## Executed hardware checks

KiCad CLI and pcbnew version 9.0.2 loaded generated native design files. `tools/check_hardware.py` passed:

- Schematic ERC: zero violations.
- PCB DRC: zero violations and zero unconnected items.
- Fresh schematic XML netlist, saved netlist, PCB pads, and explicit connectivity manifest agree: fourteen components, thirty pads, nine nets.
- Component values and full footprint library identities match.
- Schematic was exported as SVG, rasterized, and visually inspected. ATTEMPT, FAILURE, SUCCESS, RESET, ground wiring, connector designators, resistor values, and jayis1 title-block attribution are legible; no obvious symbol/label collisions were observed.

The board is a passive harness carrier for an external NUCLEO-F401RE, not an incomplete custom MCU board. Passive pin ERC cannot check the external module's power arrangement or the target's voltage. Passing CAD checks is not fabrication qualification.

## Scope, attribution, and release gates

`tools/check_release.py` requires at least 2000 README words and 500 nonblank, noncomment production C lines, excluding headers and tests from the latter count. Executed counts were 3258 words and 560 production C lines. It checks jayis1 attribution in all deliverable text files and exact signal-label agreement between the hardware generator and board pin-map header. Build outputs, Python caches, and KiCad local preferences are excluded from the release. All newly authored and adapted files credit jayis1.

The original initial test runs failed because the new engine and application contract were not yet implemented; these were expected red-phase failures. An overflow test was corrected to construct an internally consistent saturated ledger, including coverage. A reset-segment edge case after a gap was fixed and regressed. The final checks above passed on the completed sources.

No credentials, network clients, target command channel, exploit payload, or automated workflow infrastructure was added. Only the backoff-ramp directory is part of this change. Final publication is performed on main without force-push, followed by a remote branch/readback check; the resulting commit identifier is reported outside this file to avoid a self-referential hash.

## Not verified

No PCB was fabricated or assembled. No image was programmed into physical hardware. Electrical loading, actual ST-LINK/USB interoperability, board solder bridges, physical reset and brownout behavior, oscillator accuracy, pin sampling skew, ESD, thermal behavior, EMC, mechanical retention, and real target-instrumentation fidelity remain unverified. Synthetic fixtures and PTYs are not physical measurement evidence. Native KiCad files and successful target linking must not be represented as a calibrated or safety-certified instrument.
