/**
 * drivers/crc32c.h — CRC-32C (Castagnoli) Calculator Interface
 *
 * PCIe Screamer — NVMe Interposer
 * Author: Nous Research
 * Date: 2026-06-16
 */

#ifndef CRC32C_H
#define CRC32C_H

#include <stdint.h>
#include <stddef.h>

/**
 * crc32c_calculate — Calculate CRC-32C (Castagnoli) over data
 *
 * Uses polynomial 0x1EDC6F41 (Castagnoli), the same CRC used by
 * iSCSI, SCTP, Btrfs, and the PCIe Screamer wire protocol.
 *
 * This is a software implementation for the embedded firmware.
 * The FPGA gateware implements a parallel CRC-32C in hardware
 * for line-rate frame verification.
 *
 * @param data  Input bytes
 * @param len   Number of bytes
 * @return      32-bit CRC value (not inverted)
 */
uint32_t crc32c_calculate(const uint8_t *data, uint16_t len);

/**
 * crc32c_calculate_continue — Continue a CRC-32C calculation
 *
 * Allows incremental CRC calculation across multiple calls.
 *
 * @param data   Input bytes
 * @param len    Number of bytes
 * @param crc    Previous CRC value (from crc32c_calculate or prior continue)
 * @return       Updated 32-bit CRC value
 */
uint32_t crc32c_calculate_continue(const uint8_t *data, uint16_t len,
                                  uint32_t crc);

/**
 * crc32c_verify — Verify CRC-32C over data with expected value
 *
 * @param data      Input bytes
 * @param len       Number of bytes
 * @param expected  Expected CRC value
 * @return          1 if CRC matches, 0 if mismatch
 */
int crc32c_verify(const uint8_t *data, uint16_t len, uint32_t expected);

/**
 * crc32c_append — Append CRC-32C to data buffer
 *
 * Calculates CRC over data[0..len-1] and appends the 4-byte
 * CRC value at data[len..len+3] in big-endian order.
 *
 * @param data  Data buffer (must have 4 extra bytes space)
 * @param len   Data length before CRC
 */
void crc32c_append(uint8_t *data, uint16_t len);

#endif /* CRC32C_H */
