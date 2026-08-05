/*
 * sdcard.c — SD card campaign logging driver
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Stores glitch shot results and sweep metadata on a MicroSD card
 * via the STM32H723 SDIO peripheral. Each campaign creates a CSV
 * file with one row per shot, containing:
 *  timestamp, vector_mask, offset_ns, width_ns, depth_mv, series_r,
 *  em_voltage, em_width, clock_shape, clock_cycle, outcome,
 *  elapsed_us, target_response
 *
 * The driver uses a simplified FATFS-like interface. In a real build,
 * the FatFS library (or LittleFS) would provide the file system layer.
 */

#include "sdcard.h"
#include "../registers.h"
#include "../board.h"

/* ---- Private state ------------------------------------------------- */

static bool g_sd_initialized;
static uint32_t g_log_count;
static uint32_t g_campaign_start_ms;
static char g_campaign_filename[32];

/* ---- Number formatting helpers (no libc) --------------------------- */

static void u32_to_str(uint32_t val, char *buf)
{
    char tmp[11];
    int i = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    for (int j = 0; j < i; j++)
        buf[j] = tmp[i - 1 - j];
    buf[i] = 0;
}

static void append_str(char *dst, const char *src, uint32_t *offset, uint32_t max)
{
    while (*src && *offset < max - 1) {
        dst[*offset] = *src;
        (*offset)++;
        src++;
    }
    dst[*offset] = 0;
}

static void append_u32(char *dst, uint32_t val, uint32_t *offset, uint32_t max)
{
    char tmp[11];
    u32_to_str(val, tmp);
    append_str(dst, tmp, offset, max);
}

/* ---- SDIO / FatFS skeleton ----------------------------------------- */

/*
 * The SDIO peripheral is initialized here. A full implementation
 * would use the FatFs library to mount the card, open files, and
 * write data. This skeleton shows the interface and logging format.
 */

void sdcard_init(void)
{
    /*
     * Configure SDIO pins:
     *  PC8  = SDIO_D0
     *  PC9  = SDIO_D1
     *  PC10 = SDIO_D2
     *  PC11 = SDIO_D3
     *  PC12 = SDIO_CK
     *  PD2  = SDIO_CMD
     */
    /* Pin configuration omitted for brevity — uses AF12 for SDIO */

    /* Enable SDIO clock */
    /* RCC_APB2ENR |= BIT(10); — SDIO clock enable */

    /*
     * SD card initialization sequence:
     *  1. Send CMD0 (GO_IDLE)
     *  2. Send CMD8 (SEND_IF_COND)
     *  3. Send ACMD41 (SD_SEND_OP_COND)
     *  4. Send CMD2 (ALL_SEND_CID)
     *  5. Send CMD3 (SEND_RELATIVE_ADDR)
     *  6. Send CMD7 (SELECT_CARD)
     *  7. Set block size to 512 (CMD16)
     *
     * Then mount FAT filesystem via FatFs.
     */

    g_sd_initialized = false; /* set true after successful init */
    g_log_count = 0;

    /* Generate campaign filename based on boot timestamp */
    uint32_t ts = board_millis();
    uint32_t i = 0;
    append_str(g_campaign_filename, "campaign_", &i, sizeof(g_campaign_filename));
    append_u32(g_campaign_filename, ts, &i, sizeof(g_campaign_filename));
    append_str(g_campaign_filename, ".csv", &i, sizeof(g_campaign_filename));
    g_campaign_start_ms = ts;
}

/* ---- Logging ------------------------------------------------------- */

void sdcard_log_shot(const glitch_params_t *params,
                     const glitch_result_t *result)
{
    if (!g_sd_initialized)
        return;

    /*
     * Write a CSV row to the campaign file.
     * Format: timestamp_ms,vector,offset_ns,width_ns,depth_mv,series_r,
     *         em_v,em_w,clock_shape,clock_cycle,outcome,elapsed_us,response
     */

    char line[256];
    uint32_t i = 0;

    /* Timestamp */
    append_u32(line, board_millis() - g_campaign_start_ms, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));

    /* Vector mask */
    append_u32(line, params->vector_mask, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));

    /* Offset, width */
    append_u32(line, params->trigger_offset_ns, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));
    append_u32(line, params->glitch_width_ns, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));

    /* Power glitch params */
    append_u32(line, params->power_depth_mv, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));
    append_u32(line, params->power_series_r, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));

    /* EM params */
    append_u32(line, params->em_pulse_mv, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));
    append_u32(line, params->em_pulse_width_ns, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));

    /* Clock params */
    append_u32(line, params->clock_shape, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));
    append_u32(line, params->clock_cycle_offset, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));

    /* Result */
    append_u32(line, (uint32_t)result->outcome, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));
    append_u32(line, result->elapsed_us, &i, sizeof(line));
    append_str(line, ",", &i, sizeof(line));

    /* Target response (truncate at comma boundary) */
    append_str(line, result->target_response, &i, sizeof(line));

    append_str(line, "\n", &i, sizeof(line));

    /* Write to file (FatFs f_write would go here) */
    /* f_write(&g_file, line, i, &bytes_written); */

    g_log_count++;
}

void sdcard_log_sweep_start(const sweep_config_t *config)
{
    if (!g_sd_initialized)
        return;

    /* Write sweep configuration as a header comment */
    char header[128];
    uint32_t i = 0;
    append_str(header, "# Sweep start: x_param=", &i, sizeof(header));
    append_u32(header, config->x_param, &i, sizeof(header));
    append_str(header, " y_param=", &i, sizeof(header));
    append_u32(header, config->y_param, &i, sizeof(header));
    append_str(header, "\n", &i, sizeof(header));

    /* f_write(&g_file, header, i, &bytes_written); */
}

void sdcard_log_sweep_end(const sweep_status_t *status)
{
    if (!g_sd_initialized)
        return;

    char footer[128];
    uint32_t i = 0;
    append_str(footer, "# Sweep end: cells=", &i, sizeof(footer));
    append_u32(footer, status->completed_cells, &i, sizeof(footer));
    append_str(footer, " success=", &i, sizeof(footer));
    append_u32(footer, status->success_count, &i, sizeof(footer));
    append_str(footer, "\n", &i, sizeof(footer));

    /* f_write(&g_file, footer, i, &bytes_written); */
}

uint32_t sdcard_get_log_count(void)
{
    return g_log_count;
}

bool sdcard_read_log_entry(uint32_t index, char *buf, uint32_t buf_len)
{
    (void)index;
    (void)buf;
    (void)buf_len;
    /* In a full implementation, this would seek to the index-th line
     * in the campaign file and read it into buf. */
    return false;
}

/* ---- End of file --------------------------------------------------- */