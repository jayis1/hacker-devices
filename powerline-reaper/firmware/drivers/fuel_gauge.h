/*
 * fuel_gauge.h — MAX17048 fuel gauge interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef FUEL_GAUGE_H
#define FUEL_GAUGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int      fuel_gauge_init(void);
uint16_t fuel_gauge_voltage_mv(void);
uint8_t  fuel_gauge_percent(void);
uint16_t fuel_gauge_soc_raw(void);

#ifdef __cplusplus
}
#endif

#endif /* FUEL_GAUGE_H */