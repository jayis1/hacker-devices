# AperturePhantom — CrossLink-NX FPGA Gateware Contract

> Author: **jayis1** · License: GPL-2.0

The Lattice CrossLink-NX (LIFMD-25) FPGA runs gateware that exposes the
MIPI CSI-2 link to the STM32H743 control MCU as a register file plus two
FIFOs over SPI. The MCU never touches the high-speed serial itself; it
issues SPI commands described in `firmware/drivers/fpga.c`.

This document is the **contract** the firmware depends on; the Verilog /
Radiant project lives outside this repo's scope but must implement it.

## Top-level blocks

```
                 ┌──────────────────────── CrossLink-NX ────────────────────────┐
   sensor  ─────►│  D-PHY RX  │  CSI-2   │  cap  │  line/  │  SPI   │  MCU SPI    │
   (mipi)        │  (1-4 ln) │  packet  │ FIFO │  frame │  slave │              │
                 │            │  decoder │      │  FSM   │  iface │              │
   host    ◄─────│  D-PHY TX  │  CSI-2   │  inj  │        │        │              │
   (mipi)        │  (1-4 ln) │  encoder │ FIFO │        │        │              │
                 └──────────────────────────────────────────────────────────────┘
```

- **D-PHY RX**: uses CrossLink-NX native MIPI D-PHY hard IP; 1–4 data lanes
  at 80–2500 Mbps/lane. Detects LP→HS transition, receives D-PHY packets.
- **CSI-2 packet decoder**: parses 32-bit packet headers (VC, DT, WC),
  verifies ECC (1-bit correct, 2-bit flag), verifies header CRC, extracts
  payload into the capture FIFO in 8-byte words.
- **Capture FIFO**: 18 Kb dual-clock FIFO (CSI byte clock vs SPI clock);
  read by the MCU via SPI `0x40` command. Overflow sets
  `FPGA_STATUS_CAP_FIFO_OVF`.
- **Inject FIFO**: 18 Kb dual-clock FIFO; written by the MCU via SPI `0x60`
  command; the TX FSM reads from it to assemble CSI-2 long packets with
  FS/FE short packets around them. Underflow sets `FPGA_STATUS_INJ_FIFO_UDF`.
- **CSI-2 packet encoder**: builds short packets (FS/FE/LS/LE) and long
  packets from the inject FIFO, computes header ECC + CRC (or corrupts them
  when fuzz control bits are set), streams to D-PHY TX.
- **D-PHY TX**: native MIPI D-PHY TX hard IP, drives lanes toward the host.
- **Passthrough mux**: when `FPGA_CTRL_PASSTHROUGH` is set, RX→TX is a
  combinational copy with a parallel tap into the capture FIFO (so capture
  can run while the host sees the live sensor). Adds ≤ 100 ps skew.
- **Sensor mute**: when `FPGA_CTRL_MUTE_SENSOR` is set, the RX is gated off
  and the TX drives from the inject FIFO; the sensor's clock and I²CCCI
  keep running (managed by the MCU) so the host doesn't notice.

## SPI command set (matches `fpga.c`)

| Code | Name       | Payload                    | Returns        |
|------|------------|----------------------------|----------------|
| 0x10 | RD_REG     | addr in low 6 bits          | 4 bytes BE     |
| 0x20 | WR_REG     | addr + 4 bytes BE           | 1 ack byte     |
| 0x40 | RD_CAP     | len (2 BE)                  | len data bytes |
| 0x60 | WR_INJ     | len (2 BE) + data           | 1 ack byte     |
| 0x80 | IRQ_ACK    | —                           | 1 ack byte     |

## Register file (matches `registers.h`)

The 64-entry 32-bit register file is the only interface the MCU needs to
drive link state. See `firmware/registers.h` for the full list and bit
definitions (FPGA_REG_VERSION, FPGA_REG_STATUS, FPGA_REG_CONTROL,
FPGA_REG_LANE_MASK, FPGA_REG_DPHY_RATE, FPGA_REG_VC_MASK, FPGA_REG_DT_FILTER,
the capture/inject FIFO level registers, line/word/CRC/ECC counters, and
the TX configuration registers: TX_DTYPE, TX_WIDTH, TX_HEIGHT, TX_VC,
TX_TRIGGER).

## Fuzz corruption paths (CSI-2 receiver fuzzing)

When the corresponding `FPGA_CTRL_TX_*` bit is set in FPGA_REG_CONTROL, the
encoder intentionally emits invalid CSI-2 traffic:

- `TX_CRC_BAD`     → header CRC field is bitwise-inverted.
- `TX_SHORT_LINE`   → the long-packet word-count is half the actual payload length.
- `TX_BAD_DT`       → the data-type field is set to 0xFF (reserved).
- `TX_OVERSIZE`     → word-count is 0xFFFF regardless of payload.

These are designed to probe the host ISP/camera kernel driver for parsing
bugs (OOB reads, buffer overflows, integer overflow in line-stride calc,
DoS). Combined with bad VC (set via FPGA_REG_TX_VC) and missing FS/FE
(handled in `fpga.c` by skipping the trigger strobe), this covers the
common CSI-2 receiver bug classes.

## Power-up defaults

- `CONTROL = FPGA_CTRL_PASSTHROUGH` (so the host sees the sensor normally
  before any MCU command).
- `LANE_MASK = 0x0F`, `DPHY_RATE = 0` (auto-detect from RX), `VC_MASK = 0x01`.
- All counters zero, both FIFOs empty, fuzz bits clear.

## Author / license

Gateware design by **jayis1**. GPL-2.0.