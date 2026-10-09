# Hardware Design Lab / GhostWisp

Documentation review candidate v5, 2026-10-09. This package describes historical hardware work and reconciles completed design corrections. It contains no firmware, application, device-tree source, or recovered working-tree diff. Current GitHub source and selected build definitions have now been inspected at a pinned commit; source is referenced, not bundled. This is not a verified current source release or evidence of successful migration, boot, or deployment.

## Layout

- `README.md`: purpose, local use, verification and limitations.
- `design-summary.md`: reconciled design requirements; no implemented behavior is claimed.
- `source-manifest.json`: exact evidence references, retrieved-body hashes and historical source classification. Referenced evidence is not bundled.
- `SHA256SUMS`: hashes for all three content files, excluding itself.

## Setup and checks

Extract `hardware-ghostwisp-review-v5.zip` into a new directory using the commands below. No package installation, credentials or network access is needed to read it. Use a Markdown viewer and Python 3 for JSON inspection. Run from the directory containing the downloaded archive:

```sh
unzip hardware-ghostwisp-review-v5.zip -d hardware-ghostwisp-review-v5
cd hardware-ghostwisp-review-v5
sha256sum -c SHA256SUMS
python3 -m json.tool source-manifest.json
```

GNU coreutils provides `sha256sum`; on macOS use `shasum -a 256 -c SHA256SUMS`. A matching hash proves byte identity only. This archive has no executable product source. The current-source commands below were checked against selected pinned build definitions but were not executed in this documentation task.

## Current GhostWisp source and command reference

The user identified [jayis1/ghostblade](https://github.com/jayis1/ghostblade) as GhostWisp's home. Read-only inspection on 2026-10-09 pinned `main` at `8924ecbc2ba0d8dc07bffe933fa1c932c94e4f30`. Its [GhostWisp README](https://github.com/jayis1/ghostblade/blob/8924ecbc2ba0d8dc07bffe933fa1c932c94e4f30/devices/ghostwisp/README.md) and directory listing locate the project under `devices/ghostwisp/`, alongside the larger GhostBlade project. Keep that distinction: the root firmware target builds GhostBlade, not GhostWisp.

Observed GhostWisp layout: `docs/` design contracts; `firmware/rp2350b/` target firmware and CMake configuration; `protocol/` companion protocol; `tests/` host tests; `Makefile` and `check.sh` local checks. These entries were inspected as listings and selected files, not recursively audited. No root or GhostWisp-directory AGENTS.md appeared in those listings. The root CONTRIBUTING.md requires consistent documentation/build commands and prohibits repository-hosted automation files.

To inspect the same source in a separate local clone:

```sh
git clone https://github.com/jayis1/ghostblade.git ghostblade-review
cd ghostblade-review
git checkout --detach 8924ecbc2ba0d8dc07bffe933fa1c932c94e4f30
```

These are source-repository commands, not commands for this extracted archive. For the local host boot test, the inspected Makefile requires GNU Make and GCC. The extended check also uses Bash and binutils (`nm`, `strings`):

```sh
make -C devices/ghostwisp test
make -C devices/ghostwisp check
```

For the target firmware, the inspected CMake definition requires CMake 3.21+, an ARM embedded toolchain and Pico SDK 2.0+ at an existing local path:

```sh
cmake -S devices/ghostwisp/firmware/rp2350b -B build/ghostwisp \
  -DPICO_SDK_PATH=/path/to/pico-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build/ghostwisp
```

Expected outputs per build definition are `ghostwisp.elf`, `.uf2`, `.hex`, `.bin` and `.map`; none were produced or verified in this task. `make check` can report success while skipping cross-compilation when SDK/toolchain prerequisites are absent. The inspected CMake target lists `main.c`, `ghostwisp_platform.c` and `ghostwisp_boot.c`; peripheral-driver presence elsewhere does not prove those drivers are linked into that target. The source README's test counts and fail-closed signature-verifier description are upstream claims, not newly verified results. No flashing, radio activation or hardware acceptance was performed.

This source discovery does not recover the five historical unstaged modifications or prove source-host equivalence. The unchanged design summary concerns historical delivery/migration controls, not a claim that current GhostWisp implements them. Review and publication must preserve existing GhostBlade/GhostWisp source and root READMEs.

## Provenance and limitations

The workspace was imported from Ignis on 2026-10-03. Its exported READMEs reference six distinct historical repositories, including `jayis1/ghostblade` and `jayis1/unified-TREE`. The user's correction on 2026-10-09 selects [jayis1/hacker-devices](https://github.com/jayis1/hacker-devices) as this candidate's proposed documentation destination; the later GhostWisp clarification identifies its existing source in `jayis1/ghostblade`, not a request to relocate that source. Read-only GitHub inspection found branch `main` at `e44d6f7a2065915ea04565f05d162f55c0515c65`. This is the candidate's observed base, not approval to publish. Donald O selected placement at `docs/hardware-ghostwisp/`, preserving the existing root README and device directories. Neutral must review this exact artifact against the selected base before any push. No repository content is changed by extracting this package.

The pinned root listing contains device directories and a root README, but no GhostWisp directory or root AGENTS.md. This was a root-level inspection, not a recursive source audit. The current root README blob `c088599074beb0cac55db01a1bccf7757048ed37` claims complete device implementations; those claims have not been validated here. No current device source, build configuration or hardware behavior was verified or bundled. The package's historical design requirements must not be treated as implementations in hacker-devices. The earlier Devices entry is superseded by the hacker-devices correction. hardware-design-lab and SoC-Device-Inventions remain supplied context; this package does not consolidate or relocate repositories. The selected publication layout remains subject to the exact-artifact review gate.

The original review (IST-33) and accepted design corrections (IST-127) are historical design evidence, not current implementations. The manifest retains private evidence identifiers and document revisions as non-public provenance references; those records are not bundled or public documentation links.

Source recovery remains blocked in IST-130 (IST-130) and IST-164 (IST-164). Five historical unstaged ghostblade modifications are not included; their off-node preservation and reconstruction remain unverified. The recorded apex-one disposition is to keep an inactive historical checkout, but enforcement has not been verified. Source-host identity, current registry equivalence, full prompt recovery and physical-device acceptance remain unresolved. Hardware acceptance requires a qualified hardware owner; browser QA cannot establish boot readiness.

No push, including a staging branch, is authorized until Oracle/Neutral approves the exact artifact or commit and the destination is established. Donald O owns publication. All existing migration, pause, activation and access gates remain applicable. This candidate excludes run transcripts, credentials, original operational records and source-host configuration. No product features or routines are changed.

The six historical import entries use repository/file source identifiers. Their reported upstream commits come from export metadata, not fresh upstream verification. Their SHA-256 values and byte counts describe historical exported documents including wrappers; they must not be compared as if they were raw upstream README hashes. Complete private provenance mapping is retained in the internal review record.
