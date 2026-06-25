/*
 * fuel_gauge.h — MAX17048 LiPo fuel gauge interface.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UWB_PHANTOM_FG_H
#define UWB_PHANTOM_FG_H

#ifdef __cplusplus
extern "C" {
#endif

int  fuel_gauge_init(void);
int  fuel_gauge_percent(int *out);
int  fuel_gauge_voltage_mv(int *out);
int  fuel_gauge_soc_pct(int *out);

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_FG_H */