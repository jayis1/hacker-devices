/*
 * vcsel_driver.h — VCSEL modulation control for optical injection (MITM)
 * Author: jayis1
 * Date:   2026-06-17
 */

#ifndef FIBER_PHANTOM_VCSEL_DRIVER_H
#define FIBER_PHANTOM_VCSEL_DRIVER_H

#include <stdint.h>

/* Initialize VCSEL control pins and modulation driver */
void vcsel_init(void);

/* Enable VCSEL laser (applies bias current, enables modulation driver) */
int vcsel_enable(void);

/* Disable VCSEL laser (cuts bias and modulation current) */
void vcsel_disable(void);

/* Set VCSEL bias current in milliamps (range: 0–20 mA) */
int vcsel_set_bias_ma(uint8_t ma);

/* Get VCSEL bias current setting */
uint8_t vcsel_get_bias_ma(void);

/* Check VCSEL status (returns 1 if enabled and bias current OK) */
uint8_t vcsel_is_active(void);

/* Inject raw bytes via VCSEL modulation.
 * data: bytes to modulate onto the optical signal
 * len: number of bytes
 * bitrate_mbps: target modulation rate (1, 1000, 10000)
 * Returns 0 on success. */
int vcsel_inject(const uint8_t *data, uint32_t len, uint32_t bitrate_mbps);

/* Inject a pre-built Ethernet frame via VCSEL.
 * The FPGA handles 8b/10b encoding, preamble, and CRC.
 * This function writes the frame to the FPGA injection FIFO. */
int vcsel_inject_frame(const uint8_t *frame, uint32_t len);

/* Run VCSEL self-test: enable, modulate test pattern, check monitor.
 * Returns 0 on success. */
int vcsel_self_test(void);

#endif /* FIBER_PHANTOM_VCSEL_DRIVER_H */