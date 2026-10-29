# Mask Containment verification

Author: jayis1.

## Executed software and CAD checks

- ARM GNU GCC 14.2.1 linked the Cortex-M4 image using the supplied reset vectors, linker script, explicit peripheral drivers, engine, and encoder. Target size: 3244 bytes text, zero initialized data, 1700 bytes BSS. Outputs are ignored build artifacts, reproducible with `make -C firmware clean all test`.
- Native GCC 14.2.0 compiled with warnings as errors. Engine tests ran with AddressSanitizer and UndefinedBehaviorSanitizer. They exercised all 256 input-vector transitions, independent qualification under unrelated chatter, gap recovery, rejected excursions, tick wrap, duplicate ticks, transactional exhaustion, serializer rejection, a CRC known-answer vector, and a deterministic independent 10,000-plateau histogram/episode oracle.
- The UART register-model test passed submission bounds, busy handling, timeout, wrap, and recovery. This maps process memory, not actual peripheral registers.
- The production main scheduler passed modeled lost-boot delivery, repeating policy, status generation, and watchdog-startup failure. These tests are not evidence of physical watchdog timing.
- Python 3.13.5 ran seven application tests under Xvfb. Tests consumed actual C-encoder output, validated histogram and episode fields, rejected checksum-valid impossible snapshots, detected duplicate/rollback/growth contradictions, bounded framing/replay, checked progress and accumulated backlog, and exercised private export/overwrite refusal.
- The actual Tk event loop was exercised through a Linux serial pseudo-terminal: connection, duplicate-connect refusal, paced synthetic frames, three screens, LIVE progress, hangup, retained provenance, replay, export, and failed replacement connection. This is not a physical USB test.
- KiCad 9.0.2 generated native schematic, symbol library, PCB, project, and netlist. ERC reported zero violations. DRC reported zero violations and zero unconnected items.
- Fresh schematic XML, saved netlist, PCB pads, and manifest agreed on fourteen components, thirty electrical pads, nine nets, values, and full footprint identities.
- The schematic was exported to SVG, rasterized with rsvg-convert, and visually inspected. Four separately named allowed/effective paths, series resistors, pull-downs, ground pins, MCU connector labels, and jayis1 title attribution were readable without observed label collisions.

## Test development and review notes

The new engine tests initially failed because the new engine interface did not yet exist. Application tests initially failed because the new contract module did not exist. After implementation, one fixture expectation incorrectly allocated the second capability's state-three plateau to state zero. An independent sample-by-sample Python calculation confirmed the firmware histogram; the expected values were corrected, and the full application suite passed. No synthetic fixture is presented as a physical measurement.

Review was performed by the standalone owner using the supplied advisory reports, source inspection, executable independent oracles, and CAD consistency checks. This was not full collective review or independent physical sign-off. Reused infrastructure remains credited to jayis1. No unrelated device files, repository-wide infrastructure, or workflow files are part of this addition.

## Reproduction

From this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p test_app.py -v
/usr/bin/python3 tools/generate_hardware.py
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_release.py
```

The release checker counts README words and nonblank noncomment production C/header lines separately, verifies required deliverables, and checks jayis1 attribution in every nonignored source/design file. Build products, Python caches, and local KiCad preferences are excluded from publication.

## Not verified

No board was fabricated or assembled. No MCU was programmed. Actual input thresholds, loading, ground compatibility, sampling jitter, GPIO port skew, oscillator accuracy, USB behavior, watchdog recovery on silicon, back-power, ESD, EMC, thermal behavior, environmental reliability, enclosure suitability, or regulatory compliance were measured. No real target was connected. The external Nucleo power circuitry is not represented as carrier circuitry, and passive-symbol ERC cannot certify it. Perform the README's physical acceptance procedure before any deployment.
