/*
 * power_monitor.h — TPS25982 eFuse driver + VBUS glitch control
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EMBER_POWER_MONITOR_H
#define EMBER_POWER_MONITOR_H

#include <stdint.h>

/* Initialize I2C1 + TPS25982. Also configures OVP/ILIM defaults. */
void power_monitor_init(void);

/* Low-level I2C1 transfer (used by pd_engine too). */
int  i2c1_write(uint8_t addr, const uint8_t *data, uint8_t len);
int  i2c1_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

/* Read live VBUS voltage in mV. */
uint32_t power_get_vbus_mv(void);

/* Read live VBUS current in mA. */
uint32_t power_get_vbus_ma(void);

/* Set the OVP threshold (in 0.1 V units, e.g. 220 = 22.0 V). */
void power_set_ovp(uint16_t tenths_v);

/* Set the current limit in mA (TPS25982 supports up to 5 A). */
void power_set_ilim(uint16_t ma);

/* Enable / disable the eFuse (cuts VBUS to the DUT). */
void power_enable(int on);

/* VBUS glitch: droop or crowbar VBUS for `us` microseconds.
 *   type=0: droop (pull down via glitch FET for N us)
 *   type=1: crowbar (hard short via FET — dangerous, interlocked)
 * Returns 0 on success, -1 if blocked by hardware interlock. */
int  power_vbus_glitch(uint32_t us, int type);

/* Read the thermistor (°C, approximate). */
int8_t power_get_temp_c(void);

/* Check if the hardware interlock (KILL) is asserted. */
int  power_kill_asserted(void);

#endif /* EMBER_POWER_MONITOR_H */
/* end of file — author: jayis1 */