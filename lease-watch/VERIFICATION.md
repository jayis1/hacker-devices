# Lease Watch revision A verification

Author: jayis1.

## Executed checks

- Repository identity: jayis1/hacker-devices; configured remote default branch main. Initial tracked worktree clean. `git pull --ff-only` reported already up to date before device creation.
- Inventoried existing top-level device directories and README purposes. New function is a four-signal heartbeat-to-enable temporal contract monitor, not quadrature decoding or detached mechanical contact analysis.
- `make -C firmware clean all test`: Cortex-M4 target compiled and linked with arm-none-eabi-gcc 14.2.1, warnings as errors. ELF sections: 2688 bytes text, zero initialized data, 1408 bytes BSS. No flash-programming or hardware execution claim.
- Host engine and encoder tests passed under GCC 14.2.0 AddressSanitizer and UndefinedBehaviorSanitizer. Coverage includes no initial heartbeat, lease expiration, reset precedence, not-ready enable, skipped samples, renewed continuity, one-sample glitches, repeated timestamps, ordinary timestamp wrap, queue overflow, and encoder capacity failure.
- Regression checks passed for retaining qualified ENABLE across a sampling gap and saturating heartbeat age near UINT32_MAX. These are accelerated state tests, not a multi-week wall-clock run.
- Production serial driver passed a Linux MMIO model test under UndefinedBehaviorSanitizer, covering invalid submissions, busy rejection, byte output, wrap-safe timeout, timeout acknowledgement, and recovery.
- `python3 -m unittest discover -s tests -p test_app.py`: six tests passed on Python 3.13.5. Includes CRC known vector, strict protocol validation, oversized frame resynchronization, freshness and reboot inference, private exclusive exports, and 10,000 seeded random input chunks with bounded frame memory.
- `python3 tests/integration.py`: production C encoder fixture accepted with zero parser errors; expected eight heartbeat edges and expired alarm observed. Actual curses application exercised through separate UI and serial Linux PTYs. Overview, Events, Health, JSON creation, mode 0600, overwrite refusal, disconnect, and normal exit passed. Data was synthetic, not acquired from physical pins.
- KiCad 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations, zero unconnected items. Fresh XML schematic export and actual PCB pads agree with manifest on 14 components, 30 pins, nine nets, component values and full library footprint identifiers.
- Schematic exported to SVG and PDF. PDF rendered and visually inspected: four series resistors, four pulldowns, six connectors, separated labels, and author/title are legible. No fabrication or assembly inspection claimed.
- Firmware C/header count excluding blank lines and comments: 576. This includes production headers, not test files. README whitespace-delimited words: 2923.
- ST UM1724 revision 13, Table 16 and section 6.8 checked against the locally cached manufacturer document: A0/PA0, A1/PA1, A2/PA4, A3/PB0 and PA2 USART2/ST-LINK routing match. Direct manufacturer PDF retrieval was attempted but failed; the already available manufacturer PDF/text was used instead.

## Review resolutions

The staged source was exported with `git checkout-index` into a fresh scratch directory. Target build, both C test executables, six Python unit tests, the PTY integration test, and KiCad checks passed there without copying ignored build artifacts. All 29 staged files are inside lease-watch, contain jayis1 attribution, and are regular files. A scoped pattern scan found no embedded credential assignments, private keys, eval, pickle loading, or shell=True subprocess use. This is a bounded scan, not a guarantee of absence of every vulnerability.

Post-gap raw inputs no longer replace qualified state. UNCERTAIN remains visible even if enable is low, and recovery requires qualified low then rising heartbeat. Saturating accumulated age prevents a stalled heartbeat from becoming fresh after a complete timestamp cycle. Stale duration uses the same saturation principle. These changes were tested before final publication.

The terminal UI integration initially expected an entire rewritten error string. Curses preserves an unchanged prefix; the assertion now checks the changed error text and independently verifies that the existing export content is unchanged. This was a test-observation correction, not suppression of an application failure.

## Reproduction

From the device directory:

    make -C firmware clean all test
    python3 -m unittest discover -s tests -p test_app.py
    python3 tests/integration.py
    /usr/bin/python3 tools/check_hardware.py

Hardware regeneration, if needed:

    /usr/bin/python3 tools/generate_hardware.py
    /usr/bin/python3 tools/check_hardware.py

No Python packages or JavaScript packages are required by the companion. KiCad Python bindings and system footprint libraries are needed by the hardware tools. Compiler/newlib and KiCad are system dependencies, not downloaded build-time dependencies. Tests write temporary artifacts under the environment's TMPDIR; build products and KiCad local preferences are ignored by Git.

## Outstanding physical work

Fabrication, assembly, debugger programming, GPIO acquisition on actual hardware, threshold/loading measurements, sample jitter, HSI drift, watchdog reset timing, USB enumeration, cable noise, brownout, thermal, ESD, EMC, and regulatory checks are not performed. The carrier deliberately has no isolation or external overvoltage/back-power protection. Use only the constrained powered 3.3 V bench setup in the README. No production-ready or safety-certified claim is made.

The CRC is corruption detection, not authentication. Export is a bounded snapshot rather than a complete capture. The host and development board are trusted components. There is no remotely accessible service, cloud upload, target transmitter, or target-control command.
