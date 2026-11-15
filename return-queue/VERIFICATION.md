# Return Queue verification record

Author: jayis1.

## Executed checks

- Target: GNU Arm Embedded GCC 14.2.1, Cortex-M4 Thumb soft-float, C11, warnings treated as errors. `make -C return-queue/firmware clean all test` passed. Linked image: 3,808 bytes text, zero initialized data, 1,708 bytes BSS. ELF, binary and map are ignored build products.
- Engine: AddressSanitizer and UndefinedBehaviorSanitizer test passed. Independent literal FIFO checked 19,024 operation transitions through depth ten; all 24 full-drain return permutations were accepted in their corresponding subsequent allocation order. Regression coverage includes virgin queue ordering, immediate only-vacancy reuse, duplicate and orphan precedence, simultaneous strobes, raw endpoint disagreement, missed acquisition, frozen terminal state, overflow rollback and unsigned timestamp wrap.
- Red/green evidence: the first queue-specific regression failed on the interrupted directory's inherited last-return-first comparator (`ledger_eligible(&s,1)` after allocating and returning zero). The corrected earliest-return comparator passed the same test and complete oracle suite.
- Production serial driver under a Linux MMIO test mapping: bounds, nonblocking transmission, timeout, recovery and wrap passed with UndefinedBehaviorSanitizer.
- Production main-loop scheduler under controlled board hooks: periodic policy announcement, lost initial boot record, transport diagnostics and watchdog initialization failure passed.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s return-queue/tests -v`: 16 tests passed. Includes actual tkinter screens, Linux PTY connection and hangup, terminal restoration, replay of real C-encoder synthetic fixtures, JSON export, overwrite refusal, malformed frames, unknown policy, stale reception, cumulative drift, rollback and impossible sparse histories. An independent FIFO enumerator checks every prefix-to-end snapshot pair in legal operation words through depth seven.
- KiCad 9.0.2: `/usr/bin/python3 return-queue/tools/generate_hardware.py` and `/usr/bin/python3 return-queue/tools/check_hardware.py` passed. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic netlist, saved netlist, manifest and PCB agreed on 14 components, 30 pads and 9 nets, including values and full footprint identities. No ERC/DRC waivers were required.
- Schematic exported with native KiCad PDF and SVG exporters. PDF rendered to PNG and visually inspected: labels and connector numbering were legible, four input networks were distinct, and the title block credited jayis1. Render products remain scratch artifacts, not source deliverables.
- Release checker passed: README exceeds 2,000 words; production `.c` files contain 619 nonblank, noncomment lines, excluding headers and tests. Every authored deliverable includes jayis1 attribution. Git whitespace checks passed. Generated target binaries and local KiCad preferences are ignored.

## Scope and limitations

This was degraded standalone-owner execution using advisory material, not independent collective design approval. No physical board was fabricated, programmed or measured. The firmware build establishes target linking, not correct silicon operation. PTYs and controlled register mappings are synthetic test environments. GUI tests exercise transport/error paths but do not establish ST-LINK interoperability. Passive-only ERC cannot prove the external module supply, GPIO voltage safety, harness loading or manufacturing quality.

The carrier and embedded platform reuse existing jayis1-authored infrastructure; the queue policy, oracle tests, sparse-history checks and device documentation are the device-specific work. Its distinction from Return Stack and Oldest Free is functional, not an assertion of novel electronics or worldwide invention.

Remaining physical validation: purchased-part and module-revision checks, assembly inspection, continuity, safe programming, clock/baud measurements, GPIO thresholds/loading, cable behavior, watchdog recovery, actual target event fidelity, ESD handling and reproducible authorized experiments. Until those checks are performed this remains an unbuilt engineering prototype, not production hardware or a security certification.
