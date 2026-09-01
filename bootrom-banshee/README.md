# BootROM Banshee

**Author:** jayis1  
**Copyright:** Copyright (c) 2026 jayis1  
**Category:** Secure boot, SPI/QSPI flash interception, and pre-OS trust-boundary research platform

## Legal and Ethical Disclaimer

BootROM Banshee is designed strictly for **authorized** security research, defensive validation, hardware assurance testing, red-team simulation, product security review, and lab development. It must only be used on equipment you own or are explicitly authorized to assess in writing. The platform can influence early-boot behavior, storage responses, timing windows, and fallback states. Misuse against production systems, safety-critical systems, consumer devices, industrial controllers, medical devices, vehicles, or third-party hardware without authorization may cause outages, corruption, unsafe states, warranty violations, and legal consequences. Use only in controlled environments, with rollback plans, logged approvals, and power safety precautions.

## Device Purpose and Overview

BootROM Banshee is a portable inline hardware platform for studying one of the least instrumented but most security-critical trust boundaries in embedded systems: the moment a processor reads its first boot bytes from external nonvolatile memory and decides what to trust next. Many devices with secure boot, measured boot, partial authentication, or vendor fallback logic still begin life on power-up by talking to a flash device over SPI, Dual SPI, Quad SPI, Octal-compatible subsets, or eMMC-like strap-controlled buses. At that stage the target is often running immutable mask ROM code with very limited visibility and minimal defensive telemetry. If a researcher wants to understand whether the target validates headers, retries reads safely, handles faulted opcodes, rejects malformed timing, or exposes undocumented recovery states, there are few compact tools built specifically for that layer.

BootROM Banshee fills that gap. It is an **inline boot-bus interception and mutation instrument** that sits between a target SoC and its external boot flash or removable storage. In transparent mode it behaves like a precise monitor with timestamped capture, command decoding, and read-path telemetry. In active mode it can enforce carefully bounded, scriptable manipulations such as selective block overlays, opcode rewriting, transaction delay insertion, strap-state emulation, chip-select glitch shaping, deterministic read truncation, and staged fallback induction. The objective is not destructive corruption. The objective is repeatable security research into how early-boot code behaves when assumptions about storage, timing, integrity, and fallback are challenged under controlled conditions.

The practical value is broad. A red team assessing an appliance may need to know whether a signed-loader chain can be diverted into UART download mode by a small number of malformed read sequences. A product security team may need to validate that the ROM does not silently accept stale rollback headers after a timeout. A researcher may want to test whether a target misparses SFDP, JEDEC IDs, or alternate status-register layouts. A defender may want evidence that boot recovery remains safe even when the flash is slow, partially degraded, or adversarial. BootROM Banshee makes those experiments repeatable without manually soldering a new emulator for every target.

## Why the Device Is Original

BootROM Banshee is not just a logic analyzer, a simple flash emulator, or a generic voltage fault tool repackaged with a dramatic name. Its novelty comes from treating the **boot-storage relationship itself** as a hostile interface that can be proxied and policy-tested. Typical tooling falls into one of several categories:

- logic analyzers that record traffic but do not safely intervene
- flash programmers that rewrite storage offline but do not test live boot assumptions
- glitching tools that perturb voltage or clock broadly without protocol awareness
- development flash emulators optimized for engineering bring-up rather than adversarial research

BootROM Banshee is different because it keeps the protocol semantics in the loop. It understands boot-bus operations well enough to intercept specific opcodes, target selected address ranges, inject latency only on chosen reads, overlay alternate headers without modifying the physical flash image, and deliberately induce fallback behavior while staying within operator-defined safety limits. It is therefore a **storage-trust adversary simulator** rather than just a bus tap.

The device is also operationally useful because it supports side-by-side comparison between passive capture, semi-transparent proxying, and active mutation. A researcher can observe a clean boot baseline, replay the same power-on sequence with a single modification, and export a structured evidence bundle showing exactly which reads changed and what the target did afterward. That repeatability is often the difference between an interesting lab anecdote and a credible assessment artifact.

## Threat Model and Attack Surface

BootROM Banshee is built to test what happens when an adversary can influence or emulate the target’s external boot medium, electrical timing, or strap-derived boot context during the first milliseconds of execution.

### Security Questions Under Test

1. Does the immutable boot ROM authenticate every critical structure or only a subset?
2. Are retries, CRC failures, or timeout paths safe, or do they lead to fallback modes with weaker controls?
3. Can opcode-level anomalies such as altered fast-read modes, malformed dummy cycles, or inconsistent JEDEC identity values trigger undocumented parser behavior?
4. Does the target pin or strap sample only once, or can timing distortions create a race between bus discovery and fallback selection?
5. Can partial overlays of image headers, rollback counters, partition descriptors, or manifest blocks alter trust decisions without rewriting the original flash?
6. Do secondary boot stages trust metadata that the ROM failed to validate strongly?
7. Are captured failure states exploitable for persistent access, forensic extraction, or denial-of-service?

### Targeted Attack Surface Areas

BootROM Banshee focuses on the following early-boot surfaces:

- **SPI/QSPI flash transactions:** read, fast-read, quad-read, write-enable, status-register, reset, SFDP, and JEDEC-ID handling.
- **Address decoding:** boot vector, manifest, partition table, A/B slot metadata, recovery markers, anti-rollback regions.
- **Protocol timing:** inter-byte delay, chip-select pulse width, response latency, stall windows, dummy-cycle count.
- **Boot straps:** external pull state, multiplexed boot-mode pins, forced alternate-boot simulations.
- **Fallback logic:** UART download mode, USB recovery, ROM shell, maintenance partitions, safe mode, watchdog-triggered retry loops.
- **Evidence and attestation:** whether the platform leaves enough telemetry to prove what happened during anomalous boots.

### Adversaries Simulated

The device models several relevant adversaries:

- a malicious replacement flash device inserted in a supply-chain or repair scenario
- a compromised inline board or interposer attached during depot maintenance
- a red-team operator with physical access during an authorized assessment
- a sophisticated hardware implant that does not rewrite storage permanently but alters what the processor sees at boot time
- a degradation scenario where a device behaves as if storage is intermittently faulty, slow, or inconsistent

## Practical Goals

BootROM Banshee is meant to answer realistic assessment questions, not just produce pretty traces. Typical goals include:

- determining whether a target’s secure-boot chain resists live header overlays
- mapping which external reads are security-critical versus cosmetic
- identifying when a product enters undocumented recovery interfaces
- proving whether anti-rollback checks happen in ROM, stage-1, or only later
- validating that watchdog resets and fallback counters cannot be manipulated into a weaker path
- capturing early-boot evidence in a way that can be attached to client reports or internal bug tickets

## Hardware Specifications

### Core Processing

- **Primary MCU:** STM32H753ZI, Cortex-M7 at up to 480 MHz, selected for high-speed control logic, DMA orchestration, and USB/Ethernet-class management support.
- **Timing Fabric / Bus Assist:** Lattice ECP5-25F FPGA, responsible for deterministic bus turnaround, chip-select shaping, dummy-cycle insertion, lane steering, and high-resolution timestamping.
- **Secure Evidence Element:** Microchip ATECC608B, used to sign capture bundles and scenario manifests so exported traces can be tied to an operator profile.
- **Storage:** 128 MB QSPI NOR for local scenarios, overlays, captures, and recovery bundles; microSD for large session exports.
- **External RAM:** 64 MB HyperRAM or equivalent for burst capture buffering and replay queues.

### Target-Facing Interposer Interfaces

- 2x high-density board-to-board mezzanine connectors for target-side and flash-side interposer harnesses
- level-shifted SPI / Dual SPI / Quad SPI lanes with 1.2 V, 1.8 V, and 3.3 V support
- selectable series damping and termination network
- strap-control outputs through digitally switched resistor banks
- isolated sense inputs for target reset, watchdog, boot-done, and UART activity
- optional eMMC strap/command observation header for future expansion and mixed-interface research

### Connectivity and I/O

- Wi-Fi 6 / BLE 5.3 management module for field use
- USB-C management port with CDC, DFU, and capture export
- 10/100 Ethernet PHY for lab automation and remote orchestration
- microSD slot for evidence export and overlay libraries
- SWD/JTAG headers for MCU and FPGA programming
- 1.69-inch IPS display for local state, scenario arm status, and target voltage view
- three front buttons: Safe, Arm, Trigger
- buzzer and tri-color LED bar for clear active-mode indication

### Sensing and Safety

- dual current-sense amplifiers for target and flash rails
- voltage monitors on VCCIO domains and target reset rail
- thermal sensor near level shifters and FPGA core regulator
- hardware watchdog and transparent-bypass analog switch path
- default power-up mode: passive proxy with mutation disabled until acknowledged

### Power

- USB-C PD sink on management port for bench power
- 2-cell Li-ion field pack option with balanced charger and fuel gauge
- isolated low-noise rails for FPGA, MCU, and target I/O domains
- brownout supervisor that forces passive bypass on internal instability

### Form Factor

- 102 mm x 64 mm x 18 mm aluminum-shielded portable chassis
- replaceable interposer harnesses for SOIC-8 clip boards, WSON breakout, and mezzanine target adapters
- intended for bench, depot, and on-site product-security engagements

## Architecture and Block Diagram

BootROM Banshee is split into a deterministic dataplane and a policy-aware control plane.

```text
                +------------------------------------------------------+
                |                  BootROM Banshee                     |
                |                                                      |
[Target SoC] <--+--> [Level shifters] --> [ECP5 timing fabric] --> [Level shifters] <--+--> [Boot flash]
                |               |                 |                    |                  |
                |               |                 |                    |                  |
                |               |            [HyperRAM capture]        |             [strap bank]
                |               |                 |                    |                  |
                |               +--------> [STM32H753 control MCU] <---+------------------+
                |                                 |      |       |
                |                              [ATECC] [QSPI] [ESP32-C6]
                |                                 |      |       |
                |                           [USB-C/Ethernet] [Display/UI]
                +------------------------------------------------------+
```

### Functional Blocks

1. **Bus Interception Layer**  
   The FPGA and level-shifter stage transparently proxy SPI/QSPI transactions in passive mode. When a scenario is armed, the same layer can delay, rewrite, or overlay responses in a precisely bounded way.

2. **Overlay Engine**  
   Local storage holds synthetic data slices such as alternate JEDEC IDs, image headers, manifest blocks, rollback metadata, and recovery markers. These overlays are mapped to address ranges and applied without rewriting the physical flash.

3. **Timing and Fault Layer**  
   The timing engine can add dummy cycles, stall responses, shorten reads, glitch chip-select duration, or simulate intermittent flash readiness. These are protocol-aware perturbations rather than blind power glitches.

4. **Strap Emulation Layer**  
   Digitally controlled resistor ladders emulate alternate boot-mode pulls so the operator can test whether strap sampling and bus discovery interact in unsafe ways.

5. **Safety and Evidence Layer**  
   Current, temperature, domain voltage, and runtime state are tracked continuously. Any unsafe reading or watchdog anomaly forces passive mode, logs the trip reason, and preserves the last stable session data.

6. **Operator Control Layer**  
   The companion app manages scenario authoring, arm/trigger gating, evidence export, and an authorization acknowledgement step that prevents accidental active use.

## Firmware Design and Rationale

The firmware in this repository is a compile-ready host simulation of the BootROM Banshee control plane. It is intentionally structured so the logic can be compiled and exercised on a workstation while keeping the hardware abstractions clear for later MCU/FPGA integration.

### Firmware Design Goals

- default to transparent observation
- separate protocol mutation from safety monitoring
- make overlays deterministic and auditable
- support address-targeted interventions instead of broad corruption
- provide repeatable scenario timelines for assessments
- produce evidence bundles that can survive review and reproduction

### Firmware Modules

- `main.c` – boot sequence simulation, runtime loop, event capture, and demonstration harness
- `board.h` – shared types, limits, enums, scenario state, and telemetry structures
- `registers.h` – simulated hardware register map and status bits
- `drivers/spi_link.*` – target/flash transaction model, opcode decode, capture queue, and pass-through state
- `drivers/overlay_store.*` – address-range overlays, manifest selection, and synthetic response slices
- `drivers/glitch_engine.*` – delay insertion, truncation, chip-select anomaly simulation, and strap timing effects
- `drivers/telemetry.*` – current, temperature, voltage, and watchdog status model
- `drivers/radio.*` – operator command queue abstraction and status publication model

### Key Firmware Behaviors

- **Passive proxy on startup:** the device powers up observing only.
- **Arm-before-trigger workflow:** active mutation requires a named scenario and an explicit trigger.
- **Selective overlaying:** only specific addresses or opcodes are changed.
- **Time-bounded manipulation:** scenarios expire automatically and restore transparent behavior.
- **Evidence tagging:** capture summaries include scenario name, overlay ID, and signed-export metadata fields.
- **Safety interlocks:** over-current, over-temperature, and unstable I/O voltage abort the run and restore safe pass-through.

### Representative Scenarios

1. **rollback-shadow** – overlay a stale manifest or anti-rollback field on selected reads to verify that ROM and stage-1 both reject downgrade metadata.
2. **jedec-masquerade** – answer JEDEC ID and SFDP queries with a synthetic device personality to test parser assumptions and flash-driver trust.
3. **late-ready-stall** – keep the flash “busy” across a chosen timing window, then return valid data to observe timeout and retry behavior.
4. **header-ghost** – replace only the first 512 bytes of the boot image with a crafted header while leaving later reads untouched.
5. **strap-siren** – bias the boot straps during reset release to test fallback mode precedence and race handling.
6. **cs-whisper** – shorten selected chip-select windows or truncate bursts to discover whether partial data causes unsafe recovery.

## Companion Application Interface

The companion application is a lightweight static web console stored in `app/`. A browser-based interface is deliberate: it works on a laptop, tablet, or an embedded management view without requiring a mobile build chain during concept validation.

### Primary Screens

- **Overview** – live state, target power, active scenario, and passive/active mode banner
- **Scenario Studio** – scenario cards, timing controls, overlay selectors, and trigger actions
- **Overlay Library** – address-range overlays, manifest snippets, JEDEC/SFDP personalities, rollback templates
- **Capture Review** – decoded transactions, address heat map, and session export
- **Safety & Ethics** – authorization acknowledgement, rail limits, strap guardrails, and abort controls

### Operator Workflow

1. Connect to the device over USB-C, Ethernet, or local Wi-Fi.
2. Confirm the device is in passive mode and the target rails are within safe range.
3. Review a baseline boot capture.
4. Acknowledge authorized-use and lab-safety requirements.
5. Choose or edit a scenario.
6. Arm the scenario, then trigger during a controlled power cycle or reset.
7. Compare the modified run against the baseline.
8. Export a signed evidence bundle for reporting.

## Use Cases

### Red Teams

- evaluate embedded appliances for insecure recovery or maintenance boot paths
- test smart access systems, industrial gateways, cameras, and network gear that boot from external SPI NOR
- simulate a malicious depot repair event where storage appears genuine but serves altered boot metadata
- identify persistence opportunities that begin before the operating system or secure monitor loads

### Security Researchers

- study which boot reads are actually authenticated on different SoCs
- compare vendor ROM behavior under malformed flash timing and identity changes
- reproduce undocumented recovery shells and bootloader pivots deterministically
- research anti-rollback assumptions without permanently reflashing devices

### Product Security and Defenders

- validate secure-boot implementations against storage impersonation
- test recovery behavior before product release
- prove that watchdog loops and timeout handling fail closed
- generate reproducible traces for firmware, silicon, and supplier escalation

### Incident Response and Forensics Labs

- capture early-boot reads from suspicious hardware with minimal intrusion
- compare suspect and baseline flash identities
- identify whether a device’s startup behavior changes under degraded storage timing

## Hardware Design Decisions

Several implementation choices are intentional:

- **FPGA + MCU split:** deterministic bus handling belongs in the FPGA; policy, telemetry, and operator logic belong in the MCU.
- **Overlay rather than full emulation first:** partial overlays are enough to test many security assumptions while keeping proxy behavior realistic.
- **Strap control included:** many embedded devices have critical boot decisions spread across flash reads and boot-mode pins.
- **Signed evidence bundles:** hardware assessments often need defensible trace artifacts, especially in client work.
- **Passive default:** the safest and most credible baseline for a security-research device is observe-first, mutate-second.

## Example Assessment Patterns

### Secure-Boot Validation

A target gateway advertises signed firmware and anti-rollback protections. The operator first captures a passive boot and marks the addresses for the ROM header, manifest table, and rollback counter. The `rollback-shadow` scenario then overlays only the rollback field with an older but correctly formatted value while preserving all later reads. If the target still boots the older slot or enters a weaker loader, the evidence clearly shows a trust gap. If it fails closed, the capture proves the control is present and where it occurs.

### Recovery-Mode Discovery

A field controller has no documented debug port but exposes accessible flash pads. Using `late-ready-stall`, the operator delays selected read responses near the end of ROM retries. The target begins emitting UART recovery messages not visible in normal boot. The session export includes stall timing, target reset transitions, and the exact addresses that preceded recovery, making the result actionable for product-security teams.

### Supply-Chain Impersonation Simulation

A red team wants to model a malicious replacement flash part in an authorized assessment. `jedec-masquerade` serves a synthetic JEDEC ID and altered SFDP tables while keeping the main firmware image unchanged. If the SoC enables a different read mode or trust path based on the reported device identity, the trace makes that decision visible.

## Limitations and Future Extensions

The current reference design focuses on SPI/QSPI-class boot media because that covers a wide range of routers, cameras, gateways, appliances, and embedded control boards. Future hardware revisions could add:

- eMMC inline mutation support with command/response interpretation
- Octal SPI and HyperBus timing personalities
- automated diffing of boot captures across firmware versions
- formal scenario signatures for team-controlled test packs
- lab automation APIs for power cyclers and external logic analyzers

## Repository Contents

```text
bootrom-banshee/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── spi_link.c
│       ├── spi_link.h
│       ├── overlay_store.c
│       ├── overlay_store.h
│       ├── glitch_engine.c
│       ├── glitch_engine.h
│       ├── telemetry.c
│       ├── telemetry.h
│       ├── radio.c
│       └── radio.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── index.html
    ├── styles.css
    └── app.js
```

## Conclusion

BootROM Banshee is a practical, original security-research instrument aimed at a real gap in hardware assessment workflows: protocol-aware adversarial testing of early boot trust over external storage buses. It gives red teams, embedded security researchers, and product defenders a way to observe, perturb, and document the exact moment a device decides what firmware to trust. That combination of inline transparency, selective mutation, strap influence, and evidence export is what makes the platform useful. It is neither a toy emulator nor a broad destructive glitch box. It is a purpose-built boot trust manipulator for authorized research.
