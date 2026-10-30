# Veto Memory verification record

Author: jayis1.

## Release scope

Unbuilt engineering prototype in jayis1/hacker-devices/veto-memory. The inspected default branch was main and `git pull --ff-only origin main` completed before implementation. The starting commit was fe7842aa316bcd1491a07a762c1de29db6bfcf60. Only this device directory was added. Work was performed in standalone-owner mode; supplied peer reports were advisory, not independent verification. The advisory creative-projects destination was rejected in favor of the verified checkout and remote. No daily-ideas content or CI infrastructure was changed.

## Executed software checks

- `make -C veto-memory/firmware clean all test`: passed with Arm GNU GCC 14.2.1, native GCC 14.2.0, warnings treated as errors. Cortex-M4 Thumb ELF and binary linked with startup code and actual flash/RAM linker layout. Size output: text 3412 bytes, data 0 bytes, bss 1672 bytes. These figures exclude stack usage.
- Engine tests ran under AddressSanitizer and UndefinedBehaviorSanitizer. Checks include retained abort and withdrawal, reassertion without renewed permission, clean commit, repeated commit, cancellation, all initial prepare masks against commit-bearing endpoint masks, raw abort and preparation endpoint guards, qualification rejection, initial busy attachment, gap abandonment, halted immutability, timer wrap, duplicate ticks, counter exhaustion, CRC reference vector, bounded encoding, and 20,000 randomized hold segments with invariants checked at each sample.
- UART tests ran against a Linux memory-mapped register model: submission bounds, busy refusal, byte transfer, timeout, recovery, and time wrap passed. This is not a physical USART test.
- Production scheduler tests used controlled board/transport functions: lost boot transmission, repeated late-attachment policy, status scheduling, timeout accounting, and watchdog-startup halt passed.
- `xvfb-run -a python3 -m unittest discover -s veto-memory/tests -p test_app.py -v`: eight tests passed on Python 3.13.5. The C fixture executable was actually run and its output parsed by the companion, not replaced by hand-built plausible telemetry.
- Application checks exercised real Tk event loops, all three screens, Linux serial pseudo-terminals, connection ownership, paced advancing snapshots, hangup, retained evidence, partial-tail accounting, replay replacement, export dialog flow, private mode-0600 JSON, overwrite refusal, failed reconnect preservation, FIFO/oversize replay rejection, CRC failures, bounded framing/resynchronization, invalid snapshots, duplicate and rollback detection, unsupported policy, veto continuity, live freshness, and delayed receipt rejection.
- The disconnect partial-tail regression was first observed failing (zero errors rather than one), then passed after the disconnect path was corrected. Initial engine and app tests also failed before their implementations were present.

## Executed CAD checks

- Native KiCad 9.0.2 generation completed with standard footprint libraries and locally authored passive symbols.
- `kicad-cli sch erc --exit-code-violations`: zero violations.
- `kicad-cli pcb drc --exit-code-violations`: zero violations and zero unconnected items.
- `tools/check_hardware.py` exported a fresh XML schematic netlist and compared it with the PCB and saved connectivity manifest: fourteen components, thirty pads, nine nets. References, values, full library footprint identities, and every pin-to-net assignment agreed. The saved netlist also matched.
- Schematic and fitted PCB SVGs were exported, rasterized, and visually inspected. References and net labels were readable; no obvious text collision or absent route was seen. The carrier is explicitly passive and externally wired to the Nucleo, not an incomplete bare-MCU design.
- Passive-symbol ERC does not validate external supply behavior, pin voltage limits, USB grounding, or safe attachment. PCB DRC does not validate the purchased component dimensions or assembly process.

## Content and review checks

`tools/check_release.py` counted 3566 README words and 554 nonblank, comment-stripped production C lines, excluding headers and tests. All deliverables were checked for jayis1 attribution, native CAD presence, private-key markers, and `git diff --check`. The final count of attributed files is established by the release gate after this record is included.

Owner review traced the input mapping across board.h, board_levels(), generator signals, schematic manifest, README, and protocol bits. The firmware configures PA0, PA1, PA4 and PB0 as inputs with external pulldowns; PA2 is the host-facing TX path and USART RX is disabled. The companion opens the selected serial device read-only. There are no target commands, network endpoints, embedded secrets, target exploit payloads, or automatic updates. Volatile state, nonauthenticated evidence, and observation limits are explicitly documented.

The manufacturer PDF extraction tool was unavailable for content extraction; direct ST PDF downloads failed once with HTTP/2 error and once with timeout. Search-index excerpts of UM1724 revision 17 were retrieved and checked for the F401RE Table 16 pin mapping and section 7.10 virtual COM bridge defaults. Full fresh datasheet and register-manual validation was not completed. Existing repository register support was reviewed and reused, and links to authoritative manufacturer documents are in README.md.

## Not verified physically

No PCB was fabricated, assembled, or powered. No Nucleo was programmed or run on silicon. Footprints were not checked against an actually purchased component lot. No electrical threshold, leakage, loading, power consumption, back-power, oscillator accuracy, watchdog timing, USB interruption on real hardware, ESD, EMC, environmental, or regulatory tests were performed. No actual target transaction was measured. The synthetic fixture and pseudo-terminal tests demonstrate executable software behavior only. Follow README bring-up requirements before attaching any target, and do not describe this release as fabrication-qualified or security-certified.
