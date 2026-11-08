# Lowest Fit verification record

Author: jayis1.

## Scope and provenance

Completed in standalone-owner mode. Peer reports were advisory and do not establish independent implementation review. The checkout is jayis1/hacker-devices and its configured remote default branch is main. The conflicting creative-projects suggestion was not adopted. No daily-ideas files or other repositories were changed.

Initial git pull --ff-only returned Already up to date. The only initial untracked path was lowest-fit/tests/test_engine.c. Session history confirmed that it was interrupted output of this same authorized task. It was recovered, not discarded or silently treated as unrelated material. No temporary task branch was created; the initial local and remote branch inventories contained only main.

Device directories and README titles were inventoried. The full relevant contracts of Cursor Fit, Oldest Free, Bank Pack and Rank Drain were inspected, and repository README content was searched for first-fit and lowest-free selection. The new minimum-free admission rule is distinct from those contracts. No worldwide prior-art review was performed.

## Executed tests

Commands below are from the repository root.

- make -C lowest-fit/firmware clean all test: passed. The Cortex-M4 image compiles and links using ARM GNU GCC 14.2.1 with -Wall -Wextra -Werror. Linked text is 3632 bytes, data zero and BSS 1692 bytes. This is target compilation/linking, not physical STM32 execution.
- Engine test: 19,024 transitions against an independent sorted-vacancy-list oracle passed under AddressSanitizer and UndefinedBehaviorSanitizer. Enumeration explores repeated operation words through depth ten and prunes halted paths. Separate cases cover all occupancy masks, immediate reuse, arbitrary return order, duplicate precedence, orphan release, raw endpoint ambiguity, two-strobe rejection, acquisition gaps, terminal freeze, counter exhaustion, duplicate timestamps and ordinary timestamp wrap.
- The recovered engine test was first run against the inherited cyclic engine and failed its eligibility assertion. After implementing numeric selection it passed. Desktop policy and protocol tests also failed against inherited behavior before the LF1 contract was implemented.
- Serial tests passed bounded-buffer, timeout, recovery and wrap checks with UndefinedBehaviorSanitizer. The register model uses a Linux memory mapping, not a real USART.
- Production main-loop tests passed late-attachment policy announcements, lost boot transmission, serial failure accounting and watchdog startup failure, using controlled board functions and UndefinedBehaviorSanitizer.
- xvfb-run -a python3 -m unittest discover -s lowest-fit/tests -p test_app.py: all 22 tests passed. Coverage includes minimum eligibility across every occupancy and varied retained history, CRC-valid impossible sparse higher-slot reuse, legitimate lower-slot filling before higher reuse, permissible repeated zero reuse, stamp migration, terminal immutability, foreign protocol rejection, parser recovery, truncated tails, unsupported policy, later reset, stale duplicates, cumulative drift, counter rollback, bounded replay, private export, overwrite refusal and serial attribute restoration.
- GUI tests exercised the actual Tk event loop and all three tabs. Linux pseudo-terminals exercised serial input, duplicate-open refusal and hangup. Permission failure and invalid-device paths retain prior evidence. Real C-engine/encoder fixtures exercise legal operation, terminal findings and repeated lowest-tag renewal through the companion. Fixtures are explicitly synthetic, not physical captures.
- /usr/bin/python3 lowest-fit/tools/generate_hardware.py generated native KiCad files. /usr/bin/python3 lowest-fit/tools/check_hardware.py passed schematic ERC with zero violations and PCB DRC with zero violations and zero unconnected items. Fresh schematic XML connectivity matches the PCB pads and authored manifest: 14 components, 30 pads and 9 nets. Values and full footprint identifiers match; saved netlist connectivity matches the fresh export.
- kicad-cli sch export svg rendered the native schematic; rsvg-convert produced a raster for visual inspection. References, resistor values, net labels, connector pins and title/author appeared separated and legible at the reviewed scale. This is a visual self-review, not independent electrical qualification.
- python3 lowest-fit/tools/check_release.py confirmed 3167 README words and 596 nonblank production C lines after removing comments, excluding headers and tests. It also checks author presence, required CAD files, private-key patterns and whitespace. The final file count includes this verification record.
- Tool versions: ARM GNU GCC 14.2.1; host GCC 14.2.0; Python 3.13.5; KiCad 9.0.2.

## Review and remaining limitations

Eligibility depends on numeric rank and occupancy only. History remains evidence, not preference. Sparse host checks reject a continuously free lower identity being bypassed, but do not reconstruct every hidden event or prove that a plausible initial snapshot is reachable. Legitimate release-before-admission and repeated minimum-slot reuse remain permitted. The C encoder validates retained findings against the same stated contract, while oracle tests use an independent sorted list.

The carrier intentionally reuses existing jayis1-authored electronics. The MCU, supply, USB and programming circuitry reside on an external NUCLEO-F401RE, not on this passive PCB. Input-only GPIO is not galvanic isolation or an immutable transmission barrier. Dedicated ESD protection, level translation, signed firmware, authenticated evidence and persistent boot identity are absent.

The requested manufacturer PDF could not be retrieved: the configured extraction backend is search-only, curl encountered an HTTP/2 failure, and an HTTP/1.1 retry timed out without data. Local inherited pin/register definitions were reviewed against the code and carrier, but fresh independent manufacturer-document verification was not completed. Confirm UM1724, RM0368 and the STM32F401xD/xE datasheet against the purchased module before construction.

Not performed: fabrication, assembly, SWD programming, physical MCU execution, calibrated timing, current measurement, ESD tests, electrical bring-up, real USB unplug testing, physical diagnostic-adapter integration, manufacturing review or regulatory/safety certification. README.md expressly labels these limits. The deliverable is an executable engineering design, not a validated manufactured appliance.

Publication must be checked separately after committing: read origin's main ref, compare it with local HEAD, verify the worktree is clean and confirm that no task branch exists. A successful local commit alone is not publication evidence.
