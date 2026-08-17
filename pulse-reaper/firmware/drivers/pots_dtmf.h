/*
 * pots_dtmf.h — POTS DTMF / caller-ID parser
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_POTS_DTMF_H
#define PULSEREAPER_POTS_DTMF_H

#include "protocol_detect.h"

extern const pr_protocol_t pr_proto_pots_dtmf;

void pots_dtmf_init(void);

#endif