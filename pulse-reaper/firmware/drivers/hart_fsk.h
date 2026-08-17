/*
 * hart_fsk.h — HART FSK (Bell 202) parser
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_HART_FSK_H
#define PULSEREAPER_HART_FSK_H

#include "protocol_detect.h"

extern const pr_protocol_t pr_proto_hart_fsk;

void hart_fsk_init(void);

#endif