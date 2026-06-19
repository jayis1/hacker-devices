/**
 * fpga_bitstream.c — embedded iCE40UP5K bitstream placeholder
 * Author: jayis1
 * License: Apache-2.0
 *
 * In the full build this file is generated from the Verilog sources in
 * firmware/fpga/ by `make fpga` (yosys + nextpnr-ice40 + icepack) and
 * converted to a C array via `xxd -i`. The bitstream is ~70 kB for the
 * 5K LUT device.
 *
 * For compile-check purposes a minimal placeholder is provided. The
 * real bitstream implements:
 *   - a 17-byte SPI command decoder (opcode + 3×4-byte args + crc)
 *   - a carry-chain delay line (250 ps resolution) on the trigger path
 *   - a pulse-width counter driving the laser gate
 *   - a UART/SPI pattern matcher (64-bit pattern + mask, 50 Msym/s)
 *   - a MEMS mirror raster sequencer (I²C-driven, but timed here)
 *   - a target-clock frequency counter (for cycle-accurate delay calc)
 *
 * Replace ps_bitstream[] with the generated array before flashing.
 */

#include <stdint.h>

const uint8_t  ps_bitstream[] = {
    /* Placeholder: a real iCE40 bitstream starts with a serial sync
     * word and ends with a CRC-checked payload. The first ~50 bytes
     * are a dummy header so fpga_configure() has something to send. */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0xAA,
    0x99, 0x7E, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint32_t ps_bitstream_len = sizeof(ps_bitstream);