# Reconciled historical design requirements

Derived from the completed IST-33 review and IST-127 design record; exact source revisions and hashes are in `source-manifest.json`. These requirements describe the design of record and have not been implemented or tested by this package.

| Finding | Reconciled requirement | Remaining limit |
|---|---|---|
| F1, identity | Do not assign a writer or initiate takeover using an unverified host binding. Require node-side hostname, address and relevant job-ID evidence. | Current identity not verified. |
| F2, attribution | Slot 4 targets ghostblade by design; the old Devices attribution is unsupported by its cited conversion matrix. | Inferred correction, not source-registry confirmation. |
| F3, preservation | Preserve the five ghostblade modifications off-node with reconstruction evidence; keep apex-one inactive and excluded from writers. | Keep decision exists; preservation and enforcement remain unverified. |
| F4, correction gate | Slots 6 and 8 require stored branch-and-PR instructions before migration into any routine, even disabled. No GitHub Actions workflow creation; require evidence for hardware claims. | Stored definitions not repaired or migrated here; remaining prompts need audit. |
| F5, dependency | Slot 7 requires successful artifacts from slots 3 and 5 in the current cycle, newer than its last successful run. Missing/stale input means record a skip and advance the cursor. | No runtime implementation or validation. |
| F5, concurrency | A non-overlap lock is keyed by write target. Slots 4, 6 and 8 serialize against ghostblade. | No lock installed or runtime state checked. |
| F6, recovery | Recover permitted complete prompts and prove source equivalence through authorized recovery owners. | Historical paths and counts do not prove recoverability. |

Corrected write-target accounting: ghostblade: 4/6/8; SoC-Device-Inventions: 3; Devices: 5; unified-TREE: 7; hacker-devices: 2; hardware-design-lab service: 1. Six write targets do not imply six verified repository checkouts or granted write authorities. A due ticker must not become a one-minute model invocation; coalescing and missed-run skipping remain design requirements.

The completed review found no renderable UI in scope. It did not grant electrical, firmware or hardware authority. No hardware acceptance pass is claimed.
