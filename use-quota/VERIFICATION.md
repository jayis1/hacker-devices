# Use Quota verification record

Author: jayis1.

Execution mode: standalone owner with advisory peer material and a read-only code review, not full collective review. All repository writes and publication are performed by the owner. No physical hardware was assembled or exercised. Results below are actual software and CAD execution, not predicted electrical measurements.

## Executed acceptance checks

- `make -C use-quota/firmware all test` passed using Arm GCC 14.2.1, C11, Cortex-M4 Thumb soft-float and `-Wall -Wextra -Werror`. ELF text is 3440 bytes, data zero and BSS 1676 bytes. A raw target binary was produced. Build outputs are ignored.
- `arm-none-eabi-readelf -h -S use-quota/firmware/build/use-quota.elf` confirmed ARM ELF32, EABI soft-float, vectors at 0x08000000, code in flash and BSS at 0x20000000. This demonstrates target linking, not boot on silicon.
- Host engine tests passed 134464 independently predicted operation transitions through word depth eight using a remaining-ticket array and independent occupied flags. Dedicated regressions cover each inclusive quota, final release, one-over-limit rejection, pool-drain history retention, all ordered pairs, four-way occupancy, reverse retirement, duplicate-before-quota precedence, wait without refund, terminal freeze, orphan release, simultaneous strobes, raw endpoint ambiguity, gaps, timestamp wrap and transactional exhaustion. AddressSanitizer and UndefinedBehaviorSanitizer were enabled.
- UART tests passed bounded submission, busy rejection, service, exact timeout, wrap and recovery against mapped Linux registers. Main-loop tests passed lost boot transmission, periodic late-attach policy, transport timeout and watchdog startup failure with UndefinedBehaviorSanitizer. These are host models, not peripheral measurements.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s use-quota/tests -p 'test_*.py' -v` passed 14 tests under Python 3.13.5. They exercise all remaining-ticket combinations and occupancy masks, independent resource budgets, impossible refund/migration snapshots, real C-encoder output, parser corruption and recovery, policy mismatch, reset records, partial EOF, stale/duplicate timestamps, bounded replay files, serial restoration and private exports.
- The GUI test uses the real Tk event loop with a Linux pseudo-terminal, visits all three screens, checks remaining tickets and eligible free tags, refuses a second connection, restores attributes on disconnect, reconnects, detects hangup, replays the synthetic C fixture, retains evidence after a failed connection, exports JSON and refuses overwrite. This is not a real USB or target test.
- `/usr/bin/python3 use-quota/tools/generate_hardware.py` generated a populated native schematic, routed PCB, project, local symbols, connectivity manifest and XML netlist using KiCad 9.0.2 and installed standard footprints.
- `/usr/bin/python3 use-quota/tools/check_hardware.py` passed: zero ERC violations, zero DRC violations, zero unconnected items; exact agreement for 14 components, 30 pads and 9 nets across schematic, PCB, manifest and saved netlist. Component values and footprint library identities also matched.
- Schematic SVG export and PNG rasterization succeeded. Visual inspection confirmed four separately labelled series/pulldown paths, grounded J1 contacts, four signal-only J2 contacts, clear labels and jayis1 title-block attribution.
- `python3 use-quota/tools/check_release.py` reported 3120 README words and 572 nonempty production C lines after comment removal, excluding tests and headers. The attribution gate checks every deliverable, including CAD metadata, and excludes build/cache/local-preference files.

## Test-first implementation and review

The new C oracle was run before implementation and failed on missing engine.h. The new Python quota tests failed on missing contract. After implementation both passed, including the full reused UART/scheduler/GUI regression suite. The production policy changes replace pair-conflict rejection with cumulative per-tag limits; the carrier and platform code are explicitly disclosed reuse of jayis1-authored infrastructure.

Owner review checked input-only GPIO configuration, pin-to-net correspondence, serial-only output, quota announcement agreement, transactional updates, retained evidence and parser bounds. A read-only reviewer reported PASS with no substantive policy or app-contract defects; its only finding was this then-missing verification document, now supplied. Review remains advisory and does not constitute collective or physical validation. Protocol LQ1 avoids collision with the existing Reuse Quarantine UQ1 marker.

## Reference checks and limitations

A manufacturer-domain search returned indexed ST UM1724 Table 16 and section 7.10 content confirming NUCLEO-F401RE CN8.1/2/3/4 as PA0/PA1/PA4/PB0, PA2 USART2 TX, and default ST-LINK serial solder-bridge configuration. Source: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . The search service used a fallback after its configured backend was unreachable. No complete fresh component-datasheet audit or physical module revision check is claimed; full UM1724, datasheet and RM0368 review remains a pre-assembly requirement.

Not performed: fabrication, assembly, target programming, physical reset/watchdog tests, brownout tests, measured consumption, electrical loading, signal integrity, calibrated timing, ESD testing, enclosure qualification or actual target interoperability. The passive carrier has no isolation or dedicated overvoltage protection. Diagnostic statements are unauthenticated; omitted operations and some observer resets cannot be detected. The device is an unbuilt supervised laboratory prototype, not a fabrication certification or enforcement mechanism.

All release files are confined to use-quota in jayis1/hacker-devices. This pre-commit record does not predict a successful push or invent its commit identifier. Publication is verified separately against the remote default branch after pushing.
