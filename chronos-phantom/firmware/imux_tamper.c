/*
 * imux_tamper.c — IMU-based tamper detection and zeroization
 *
 * Reads the LSM6DSO accelerometer over I2C2, computes a sliding-window
 * magnitude, and triggers zeroization if the device is moved beyond the
 * configured threshold. This is an anti-forensic measure for red-team use.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <string.h>
#include "imux_tamper.h"
#include "registers.h"
#include "board.h"

void tamper_init(tamper_t *t)
{
    memset(t, 0, sizeof(*t));
    t->threshold_mg = TAMPER_DEFAULT_MG;
    t->state = TAMPER_STATE_NORMAL;
}

void tamper_set_threshold(tamper_t *t, uint16_t mg)
{
    t->threshold_mg = mg;
}

static int16_t abs16(int16_t v) { return v < 0 ? -v : v; }

void tamper_sample(tamper_t *t, int16_t x_mg, int16_t y_mg, int16_t z_mg)
{
    t->accel_window[t->window_idx][0] = x_mg;
    t->accel_window[t->window_idx][1] = y_mg;
    t->accel_window[t->window_idx][2] = z_mg;
    t->window_idx = (t->window_idx + 1) % TAMPER_WINDOW_SAMPLES;
}

int tamper_check(tamper_t *t)
{
    if (t->state == TAMPER_STATE_ZEROIZED)
        return 0;

    /* Compute max deviation in the window vs. the first sample
     * (which approximates the "rest" position).
     */
    int16_t baseline[3];
    memcpy(baseline, t->accel_window[0], sizeof(baseline));

    int16_t max_dev = 0;
    for (int i = 1; i < TAMPER_WINDOW_SAMPLES; i++) {
        for (int axis = 0; axis < 3; axis++) {
            int16_t dev = abs16(t->accel_window[i][axis] - baseline[axis]);
            if (dev > max_dev) max_dev = dev;
        }
    }

    if (max_dev > (int16_t)t->threshold_mg) {
        t->alert_count++;
        t->state = TAMPER_STATE_ALERT;
        return 1;
    }
    return 0;
}

void tamper_zeroize(tamper_t *t)
{
    /* In real firmware this would:
     *  1. Zero the covert-channel buffers (already in RAM, just memset).
     *  2. Zero the BLE TX buffer.
     *  3. Reset PTP/NTP engines to transparent passthrough mode.
     *  4. Clear the skew configuration.
     *  5. Optionally trigger a hardware watchdog reset to clear RAM.
     *
     * Here we just mark the state. The main loop handles the actual
     * buffer clearing when this state is observed.
     */
    t->state = TAMPER_STATE_ZEROIZED;
    memset(t->accel_window, 0, sizeof(t->accel_window));
}