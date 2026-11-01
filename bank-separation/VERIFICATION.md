# Bank Separation verification record

Author: jayis1.

This run used standalone-owner execution and review with advisory peer reports, not full collective review. No physical device was assembled or exercised. The following are actual software and CAD execution results, not predicted measurements.

## Executed checks

- `make -C bank-separation/firmware all test` completed with Arm GCC 14.2.1, C11, Cortex-M4 Thumb soft-float, `-Wall -Wextra -Werror`. The linked target ELF has text 3444 bytes, data 0 bytes and BSS 1676 bytes. A raw target binary was produced. Build outputs are ignored rather than committed.
- `arm-none-eabi-readelf -h -S ...` confirmed ARM ELF32, soft-float EABI, vector section at 0x08000000, code in flash and BSS at 0x20000000. The linker reserves stack headroom. This proves target linking, not booting on silicon.
- The engine test compared 6648 transitions through operation-word depth seven with an independent edge-list/occupancy oracle. All ordered resource pairs, duplicate precedence, nontransitive legal overlap, repeated use, release of the correct versus wrong blocker, malformed raw endpoints, simultaneous strobes, acquisition gaps, timestamp wrap and transactional exhaustion passed. AddressSanitizer and UndefinedBehaviorSanitizer were enabled for this host test.
- The serial test passed bounded submission, busy rejection, byte service, exact timeout, timestamp wrap and recovery using mapped host registers. The scheduler test passed lost boot, late-attach periodic policy, timeout accounting and failed watchdog startup. Both ran with UndefinedBehaviorSanitizer; neither exercises real MCU peripherals.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s bank-separation/tests -p 'test_*.py' -v` passed 16 tests with Python 3.13.5. Tests covered graph-independent sets, eligible candidates, sparse hidden-conflict detection, release-before-admission permission, unrestricted reuse, real C-encoder replay, malformed framing, counter consistency, freshness, reset/policy mismatch, replay bounds, terminal restoration and private export.
- The GUI test exercised all three Tk screens, live pseudo-terminal input, second-open refusal, disconnect attribute restoration, hangup, replay of the C fixture, retained evidence after failed connection, JSON export and overwrite refusal. No real USB device or target was attached. The C fixture is explicitly synthetic diagnostic traffic.
- `/usr/bin/python3 bank-separation/tools/generate_hardware.py` generated native KiCad schematic, PCB, project, authored local symbols, netlist and manifest with KiCad 9.0.2 and installed standard footprints.
- `/usr/bin/python3 bank-separation/tools/check_hardware.py` reported zero ERC violations, zero DRC violations and zero unconnected items. Schematic, PCB and saved netlist agree with the connectivity manifest for all 14 components, 30 pads and 9 nets, including values and footprint identifiers.
- Schematic SVG export succeeded; its raster rendering was visually inspected. It shows four separate series/pulldown paths, both J1 contacts grounded, J2's four signal contacts correctly labelled, nonoverlapping net labels and jayis1 author credit.
- `python3 bank-separation/tools/check_release.py` confirmed 3308 README words and 572 nonempty production C lines after comment removal, excluding headers and tests. Every deliverable contains jayis1 attribution in a valid comment or metadata field. Generated build/cache/preference files are ignored.
- Owner review compared firmware pin mapping with the schematic and manifest, exact conflict masks with application policy validation, all-pair tests with the independent edge list, and terminal halt behavior with host freeze rules. No networking, target command channel, secret, new automation workflow or unrelated repository content was added.

## Test-first history and reuse

The new engine test first failed with missing engine.h; the new graph application tests first failed with missing contract. Both passed after implementation. Reused transport tests were adapted to BS1 and the new C fixture. Generic platform code and the passive carrier are explicitly reused jayis1 work, not claimed as new inventions. All work is confined to bank-separation in jayis1/hacker-devices.

## References and limitations

A direct HTTPS download of ST UM1724 timed out after 25 seconds with no data. An alternative web search returned indexed manufacturer Table 16 content confirming NUCLEO-F401RE CN8.1/2/3/4 as PA0/PA1/PA4/PB0 and PA2 as USART2 TX. The manufacturer source is https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . This does not establish the purchased module revision, physical solder bridges or power configuration; full manufacturer documentation review remains an assembly prerequisite.

Not performed: fabrication, assembly, programming, real reset/watchdog/peripheral behavior, measured power consumption, electrical loading, timing calibration, ESD testing, enclosure qualification or real-target interoperability. No fabrication-readiness or isolation-certification claim is made. The receive-only diagnostic architecture cannot authenticate adapter statements or detect omitted target operations.

GitHub CLI had no authenticated host session. Existing Git HTTPS fetch/pull access worked, so publication uses Git without credential changes. This pre-commit verification record deliberately does not claim a future push succeeded or invent a final commit identifier; publication must be checked against the remote after pushing.
