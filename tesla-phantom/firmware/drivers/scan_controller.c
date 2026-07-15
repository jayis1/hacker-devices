/*
 * tesla-phantom — drivers/scan_controller.c
 * Automated cartography scan: moves probe across grid, fires EMFI
 * at each point, classifies result, builds fault sensitivity map.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* Maximum scan points (limited by internal memory for the map).
 * For large scans, results are streamed to SD card. */
#define MAX_SCAN_POINTS  4000

static scan_params_t scan_cfg;
static scan_point_t  scan_results[MAX_SCAN_POINTS];
static uint32_t      scan_count = 0;
static uint8_t       scan_active = 0;
static float         cur_x, cur_y;

/* ── Fault classification ─────────────────────────────────────────── */
/* After a pulse, the target's response is classified by checking:
 * 1. Did the target produce expected output? (via trigger line)
 * 2. Did the target crash/reset? (via BLE-reported status or timeout)
 * 3. Was the output different from expected? (data corruption)
 *
 * In a real deployment, the target's response would be monitored via
 * a serial connection, GPIO status pin, or BLE relay. Here we use
 * a simplified approach: the trigger line state after pulse indicates
 * the target's status.
 */

static fault_class_t classify_fault(void) {
    /* Wait briefly for target to respond */
    delay_ms(10);

    /* Check trigger line — if target toggled it, it's alive */
    uint32_t trig_state = gpio_get(GPIO(GPIOC), TRIG_EXT_PIN);

    /* Read FPGA status for any fault indicators */
    uint8_t fpga_stat = fpga_get_status();

    /* If the target's trigger line is in an unexpected state,
     * classify as crash. If it responded normally, check for
     * data corruption (would need target communication). */
    if (fpga_stat & FPGA_ST_TRIG_PEND) {
        /* Target responded → check for data corruption.
         * In a real system, we'd compare output to expected. */
        return FAULT_NONE;
    }

    /* If no response within timeout, target may have crashed */
    delay_ms(50);
    if (!gpio_get(GPIO(GPIOC), TRIG_EXT_PIN)) {
        return FAULT_CRASH;
    }

    /* Default: unknown */
    return FAULT_UNKNOWN;
}

/* ── Public API ───────────────────────────────────────────────────── */

int scan_start(const scan_params_t *params) {
    if (params == NULL) return -1;
    if (params->step_mm <= 0.0f) return -1;
    if (params->x1 < params->x0 || params->y1 < params->y0) return -1;

    /* Calculate total points */
    float width  = params->x1 - params->x0;
    float height = params->y1 - params->y0;
    uint32_t cols = (uint32_t)(width / params->step_mm) + 1;
    uint32_t rows = (uint32_t)(height / params->step_mm) + 1;
    uint32_t total = cols * rows;

    if (total > MAX_SCAN_POINTS) return -1;  /* scan too large */

    scan_cfg = *params;
    scan_count = 0;
    scan_active = 1;
    cur_x = params->x0;
    cur_y = params->y0;

    /* Move to start position */
    stepper_move_xyz(params->x0, params->y0, params->z_height);

    /* Set HV parameters */
    hv_set_voltage(params->pulse.voltage);

    /* Configure FPGA pulse timing */
    fpga_configure_pulse(params->pulse.width_ns,
                         params->pulse.delay_ns,
                         1, 0);

    return 0;
}

int scan_stop(void) {
    scan_active = 0;
    return 0;
}

int scan_step(void) {
    if (!scan_active) return -1;

    /* Current position */
    cur_x = scan_cfg.x0 + (float)(scan_count % 
                (uint32_t)((scan_cfg.x1 - scan_cfg.x0) / scan_cfg.step_mm + 1))
                * scan_cfg.step_mm;
    cur_y = scan_cfg.y0 + (float)(scan_count /
                (uint32_t)((scan_cfg.x1 - scan_cfg.x0) / scan_cfg.step_mm + 1))
                * scan_cfg.step_mm;

    /* Move to position */
    stepper_move_xyz(cur_x, cur_y, scan_cfg.z_height);
    delay_ms(50);  /* settle */

    /* Charge HV */
    hv_charge();
    while (!hv_is_charged()) {
        delay_ms(5);
    }
    uint16_t hv_actual = hv_read_actual();

    /* Fire pulses at this point */
    fault_class_t worst_fault = FAULT_NONE;

    for (uint8_t p = 0; p < scan_cfg.pulses_per_pt; p++) {
        /* Arm and fire */
        fpga_arm();
        fpga_fire();
        delay_ms(10);
        fpga_disarm();

        /* Classify result */
        fault_class_t fc = classify_fault();
        if (fc > worst_fault)
            worst_fault = fc;

        /* Inter-pulse delay */
        delay_ms(100);
    }

    /* Discharge after pulses */
    hv_discharge();

    /* Record result */
    if (scan_count < MAX_SCAN_POINTS) {
        scan_results[scan_count].pos = (position_t){
            cur_x, cur_y, scan_cfg.z_height
        };
        scan_results[scan_count].fault = worst_fault;
        scan_results[scan_count].pulse_idx = scan_cfg.pulses_per_pt;
        scan_results[scan_count].hv_actual = hv_actual;
        scan_results[scan_count].timestamp = millis();
        scan_count++;
    }

    /* Check if scan is complete */
    float width  = scan_cfg.x1 - scan_cfg.x0;
    float height = scan_cfg.y1 - scan_cfg.y0;
    uint32_t cols = (uint32_t)(width / scan_cfg.step_mm) + 1;
    uint32_t rows = (uint32_t)(height / scan_cfg.step_mm) + 1;

    if (scan_count >= cols * rows) {
        scan_active = 0;
    }

    return 0;
}

int scan_is_complete(void) {
    return !scan_active;
}

int scan_get_result(uint32_t idx, scan_point_t *result) {
    if (idx >= scan_count) return -1;
    *result = scan_results[idx];
    return 0;
}

uint32_t scan_get_count(void) {
    return scan_count;
}

int scan_export_map(void *buf, uint32_t maxlen) {
    /* Export scan results as a compact binary map.
     * Format: [width:2][height:2][step:2][count:2]
     *         then for each point: [x:2][y:2][fault:1][hv:2]
     * All values are little-endian, positions in 0.01 mm units. */
    if (buf == NULL || maxlen < 8) return -1;

    uint8_t *p = (uint8_t *)buf;
    uint32_t offset = 0;

    /* Header */
    uint16_t width_mm  = (uint16_t)((scan_cfg.x1 - scan_cfg.x0) * 100);
    uint16_t height_mm = (uint16_t)((scan_cfg.y1 - scan_cfg.y0) * 100);
    uint16_t step_mm   = (uint16_t)(scan_cfg.step_mm * 100);
    uint16_t count     = (uint16_t)scan_count;

    p[offset++] = width_mm & 0xFF;
    p[offset++] = (width_mm >> 8) & 0xFF;
    p[offset++] = height_mm & 0xFF;
    p[offset++] = (height_mm >> 8) & 0xFF;
    p[offset++] = step_mm & 0xFF;
    p[offset++] = (step_mm >> 8) & 0xFF;
    p[offset++] = count & 0xFF;
    p[offset++] = (count >> 8) & 0xFF;

    /* Data points */
    for (uint32_t i = 0; i < scan_count && offset < maxlen - 7; i++) {
        uint16_t x = (uint16_t)(scan_results[i].pos.x_mm * 100);
        uint16_t y = (uint16_t)(scan_results[i].pos.y_mm * 100);
        uint8_t  fault = (uint8_t)scan_results[i].fault;
        uint16_t hv = scan_results[i].hv_actual;

        p[offset++] = x & 0xFF;
        p[offset++] = (x >> 8) & 0xFF;
        p[offset++] = y & 0xFF;
        p[offset++] = (y >> 8) & 0xFF;
        p[offset++] = fault;
        p[offset++] = hv & 0xFF;
        p[offset++] = (hv >> 8) & 0xFF;
    }

    return (int)offset;
}