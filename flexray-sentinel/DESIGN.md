# FlexRay Sentinel Design Traceability

Author: jayis1
Revision: A0

This document maps claimed behavior to implementation artifacts. It is an engineering reference and does not replace safety, EMC, ERC, DRC, or FlexRay conformance review.

| User-visible feature | Firmware implementation | Hardware block and net | Application surface | Verification |
|---|---|---|---|---|
| Dual-channel observation | `fr_decode_record`, channel counters | U3/U4 TJA1083, `FR_A_RX`, `FR_B_RX`, U5 isolator | Status channel cards | Firmware round-trip self-test; hardware unverified |
| Header and frame integrity | `fr_header_crc11`, `fr_frame_crc24` | U2 FPGA capture FIFO to U1 SPI | Evidence events | Corruption self-test |
| Schedule learning | `detector_process`, cycle masks and online timing mean | U1 STM32H743, U6 QSPI | Baseline screen | Detector learning self-test |
| Frozen-baseline anomaly detection | `score_existing`, threshold policy | SW1 physical confirm, U8 secure element | Events and Baseline screens | Deterministic demo and self-test |
| Privacy-first evidence | `fr_payload_fingerprint`; raw storage disabled in defaults | U6 QSPI, optional J5 microSD | Privacy policy toggle | Default-config inspection |
| Receive-only safety boundary | No transmit API or register; replay mode is internal | TJA1083 TXD held disabled; no logic route to bus transmit | No injection UI | Schematic/PCB design review required |
| Bounded hostile-input handling | Fixed arrays, length/channel/slot checks | FIFO watermark and overflow status | Protocol decoder range checks | C self-tests and Node tests |
| Signed export identity | Planned secure-element adapter | U8 ATECC608B on `SEC_I2C` | Export trust indicator planned | Not implemented in host firmware |
| Fail-visible timing loss | FPGA status register contract | TCXO Y1, `CLK_20M`, FPGA PLL lock | Status clock-health field planned | Requires FPGA and board bring-up |

## Coherence anchors

The MCU is STM32H743VIT6 everywhere. The capture FPGA is iCE40UP5K-SG48. Two TJA1083 devices provide channels A and B. Capture timing is derived from a 20 MHz TCXO and an 80 MHz FPGA clock. USB-C is the host link. QSPI is primary evidence storage, ATECC608B is the intended trust anchor, and the A0 bus boundary is receive-only.

## Safety invariants

1. No MCU or FPGA output is connected to a FlexRay transmitter-data input.
2. Software cannot convert the A0 PCB into an injector.
3. Invalid capture lengths are rejected before payload copying.
4. Loss of clock validity suspends timing conclusions.
5. Capture loss is disclosed rather than silently ignored.
6. Raw payload retention is off by default and requires physical confirmation.
7. The companion application contains no transmit or injection control.

## Known verification gaps

KiCad ERC and DRC have not been run in this environment. FPGA RTL, STM32 BSP, signed boot, secure-element export, USB transport, and physical conformance testing remain future engineering tasks. These gaps are explicit and must be closed before fabrication or safety-related use.
