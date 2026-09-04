/*
 * radio_link.h
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef MCTP_WRAITH_RADIO_LINK_H
#define MCTP_WRAITH_RADIO_LINK_H

#include <stdint.h>

void radio_link_init(void);
void radio_link_send_status(const char *topic, const char *payload);
void radio_link_print_log(void);

#endif
