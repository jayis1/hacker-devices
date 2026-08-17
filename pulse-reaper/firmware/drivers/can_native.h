/*
 * can_native.h — CAN / CAN-FD bit-slicer and parser
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_CAN_NATIVE_H
#define PULSEREAPER_CAN_NATIVE_H

#include "protocol_detect.h"

extern const pr_protocol_t pr_proto_can;

void can_native_init(void);

#endif