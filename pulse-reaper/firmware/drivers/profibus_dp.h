/*
 * profibus_dp.h — Profibus DP parser
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_PROFIBUS_DP_H
#define PULSEREAPER_PROFIBUS_DP_H

#include "protocol_detect.h"

extern const pr_protocol_t pr_proto_profibus_dp;

void profibus_dp_init(void);

#endif