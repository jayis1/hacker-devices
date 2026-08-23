/*
 * Dockruptor Virtual Register Map
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DOCKRUPTOR_REGISTERS_H
#define DOCKRUPTOR_REGISTERS_H

#define DR_REG_STATUS             0x0000u
#define DR_REG_HOST_ROLE          0x0004u
#define DR_REG_DOCK_ROLE          0x0008u
#define DR_REG_ACTIVE_MODE        0x000Cu
#define DR_REG_NEGOTIATED_MV      0x0010u
#define DR_REG_NEGOTIATED_MA      0x0014u
#define DR_REG_BATTERY_PERCENT    0x0018u
#define DR_REG_TEMPERATURE_C      0x001Cu
#define DR_REG_RULE_FLAGS         0x0020u
#define DR_REG_EVENT_COUNT        0x0024u
#define DR_REG_RELAY_STATE        0x0028u
#define DR_REG_CAPTURE_DEPTH      0x002Cu

#define DR_STATUS_ATTACHED_HOST   (1u << 0)
#define DR_STATUS_ATTACHED_DOCK   (1u << 1)
#define DR_STATUS_SAFE_MODE       (1u << 2)
#define DR_STATUS_OBSERVE_ONLY    (1u << 3)
#define DR_STATUS_WIRELESS        (1u << 4)

#endif
