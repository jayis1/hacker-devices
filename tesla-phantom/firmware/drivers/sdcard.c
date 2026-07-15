/*
 * tesla-phantom — drivers/sdcard.c
 * microSD card logging via SPI4.
 * Stores EMFI burst records, SCA traces, and scan results.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* microSD card via SPI4:
 * CS:   PE4
 * SCK:  PE2
 * MISO: PE5
 * MOSI: PE6
 *
 * We implement a minimal SPI SD card protocol for writing log files.
 * A full FAT filesystem implementation is beyond scope; we use a
 * simple append-only logging scheme where records are written sequentially.
 */

#define SD_CMD_TIMEOUT  100  /* ms */
#define SD_BLOCK_SIZE   512

static uint8_t sd_initialized = 0;
static uint32_t sd_block_count = 0;
static uint32_t sd_write_block = 0;  /* current write position (block aligned) */

/* ── SPI4 helpers ─────────────────────────────────────────────────── */
static void sd_spi_init(void) {
    volatile spi_regs_t *s = SPI4;

    s->CR1 = 0;
    s->CR2 = SPI_CR2_DS_8BIT;
    s->CR1 = SPI_CR1_MSTR
           | SPI_CR1_SSM
           | SPI_CR1_SSI
           | SPI_CR1_BR_DIV64   /* 240 MHz / 64 = 3.75 MHz (slow for init) */
           | SPI_CR1_CPHA;
    s->CR1 |= SPI_CR1_SPE;
}

static void sd_spi_set_speed_fast(void) {
    volatile spi_regs_t *s = SPI4;
    s->CR1 &= ~SPI_CR1_SPE;
    s->CR1 = (s->CR1 & ~(0x7 << 3)) | SPI_CR1_BR_DIV4;  /* 60 MHz */
    s->CR1 |= SPI_CR1_SPE;
}

static uint8_t sd_spi_xfer(uint8_t tx) {
    volatile spi_regs_t *s = SPI4;
    spi_wait_tx(s);
    s->DR = tx;
    spi_wait_rx(s);
    return (uint8_t)s->DR;
}

static void sd_cs_low(void) {
    gpio_clr(GPIO(GPIOE), SD_CS_PIN);
}

static void sd_cs_high(void) {
    gpio_set(GPIO(GPIOE), SD_CS_PIN);
}

/* ── SD card commands (SPI mode) ──────────────────────────────────── */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    sd_spi_xfer(0x40 | cmd);
    sd_spi_xfer((arg >> 24) & 0xFF);
    sd_spi_xfer((arg >> 16) & 0xFF);
    sd_spi_xfer((arg >>  8) & 0xFF);
    sd_spi_xfer( arg        & 0xFF);
    sd_spi_xfer(crc | 0x01);

    /* Wait for response (non-0xFF) */
    uint8_t resp;
    uint32_t timeout = millis();
    do {
        resp = sd_spi_xfer(0xFF);
        if ((millis() - timeout) > SD_CMD_TIMEOUT) return 0xFF;
    } while (resp == 0xFF);

    return resp;
}

static void sd_send_dummy_bytes(int count) {
    for (int i = 0; i < count; i++)
        sd_spi_xfer(0xFF);
}

/* ── Public API ───────────────────────────────────────────────────── */

int sdcard_init(void) {
    sd_spi_init();

    /* Send 80 dummy clocks with CS high (enter SPI mode) */
    sd_cs_high();
    sd_send_dummy_bytes(10);

    /* CMD0: GO_IDLE_STATE */
    sd_cs_low();
    uint8_t resp = sd_send_cmd(0, 0, 0x94);
    if (resp != 0x01) {
        sd_cs_high();
        return -1;  /* not in idle state */
    }

    /* CMD8: SEND_IF_COND (check SD v2) */
    resp = sd_send_cmd(8, 0x000001AA, 0x86);
    if (resp == 0x01) {
        /* SD v2 — read 4-byte response */
        sd_spi_xfer(0xFF);
        sd_spi_xfer(0xFF);
        sd_spi_xfer(0xFF);
        sd_spi_xfer(0xFF);

        /* CMD55 + ACMD41: SD_SEND_OP_COND (with HCS) */
        uint32_t timeout = millis();
        do {
            sd_send_cmd(55, 0, 0);
            resp = sd_send_cmd(41, 0x40000000, 0);
            if ((millis() - timeout) > 1000) {
                sd_cs_high();
                return -1;
            }
        } while (resp != 0x00);

        /* CMD58: read OCR */
        resp = sd_send_cmd(58, 0, 0);
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) ocr[i] = sd_spi_xfer(0xFF);

        /* Check CCS bit (bit 30) — if set, SDHC/SDXC (block-addressed) */
        if (ocr[0] & 0x40) {
            sd_block_count = 0;  /* block-addressed */
        }
    } else {
        /* SD v1 — use ACMD41 without HCS */
        uint32_t timeout = millis();
        do {
            sd_send_cmd(55, 0, 0);
            resp = sd_send_cmd(41, 0, 0);
            if ((millis() - timeout) > 1000) {
                sd_cs_high();
                return -1;
            }
        } while (resp != 0x00);
    }

    sd_cs_high();

    /* Switch to fast SPI clock */
    sd_spi_set_speed_fast();

    sd_initialized = 1;
    sd_write_block = 0;

    return 0;
}

static int sd_write_block_data(uint32_t block, const uint8_t *data) {
    sd_cs_low();

    /* CMD24: WRITE_BLOCK */
    uint8_t resp = sd_send_cmd(24, block, 0);
    if (resp != 0x00) {
        sd_cs_high();
        return -1;
    }

    /* Send start block token */
    sd_spi_xfer(0xFE);

    /* Send 512 bytes of data */
    for (int i = 0; i < SD_BLOCK_SIZE; i++)
        sd_spi_xfer(data[i]);

    /* Send dummy CRC */
    sd_spi_xfer(0xFF);
    sd_spi_xfer(0xFF);

    /* Read data response token */
    resp = sd_spi_xfer(0xFF);
    if ((resp & 0x1F) != 0x05) {
        sd_cs_high();
        return -1;  /* write error */
    }

    /* Wait for write to complete (busy: DO=0) */
    uint32_t timeout = millis();
    while (sd_spi_xfer(0xFF) == 0x00) {
        if ((millis() - timeout) > 500) {
            sd_cs_high();
            return -1;
        }
    }

    sd_cs_high();
    return 0;
}

static int sd_read_block_data(uint32_t block, uint8_t *data) {
    sd_cs_low();

    /* CMD17: READ_SINGLE_BLOCK */
    uint8_t resp = sd_send_cmd(17, block, 0);
    if (resp != 0x00) {
        sd_cs_high();
        return -1;
    }

    /* Wait for start block token (0xFE) */
    uint32_t timeout = millis();
    while (sd_spi_xfer(0xFF) != 0xFE) {
        if ((millis() - timeout) > 100) {
            sd_cs_high();
            return -1;
        }
    }

    /* Read 512 bytes */
    for (int i = 0; i < SD_BLOCK_SIZE; i++)
        data[i] = sd_spi_xfer(0xFF);

    /* Read and discard CRC */
    sd_spi_xfer(0xFF);
    sd_spi_xfer(0xFF);

    sd_cs_high();
    return 0;
}

/* ── Logging functions ────────────────────────────────────────────── */

int sdcard_log_burst(burst_record_t *rec) {
    if (!sd_initialized) return -1;

    /* Write burst record as a 512-byte block (padded) */
    uint8_t buf[SD_BLOCK_SIZE];
    for (int i = 0; i < SD_BLOCK_SIZE; i++) buf[i] = 0;

    /* Serialize record (little-endian) */
    uint8_t *p = buf;
    *p++ = (rec->timestamp >>  0) & 0xFF;
    *p++ = (rec->timestamp >>  8) & 0xFF;
    *p++ = (rec->timestamp >> 16) & 0xFF;
    *p++ = (rec->timestamp >> 24) & 0xFF;
    *p++ = (rec->samples >>  0) & 0xFF;
    *p++ = (rec->samples >>  8) & 0xFF;
    *p++ = (rec->samples >> 16) & 0xFF;
    *p++ = (rec->samples >> 24) & 0xFF;
    *p++ = (rec->rate_khz >> 0) & 0xFF;
    *p++ = (rec->rate_khz >> 8) & 0xFF;
    *p++ = rec->gain_db;
    *p++ = (rec->trigger_offset >> 0) & 0xFF;
    *p++ = (rec->trigger_offset >> 8) & 0xFF;
    *p++ = (rec->peak_magnitude >> 0) & 0xFF;
    *p++ = (rec->peak_magnitude >> 8) & 0xFF;
    *p++ = (uint8_t)rec->fault;

    /* Magic byte to identify burst records */
    buf[510] = 0xB1;
    buf[511] = 0x57;

    int rc = sd_write_block_data(sd_write_block, buf);
    if (rc == 0) sd_write_block++;
    return rc;
}

int sdcard_log_scan_point(scan_point_t *pt) {
    if (!sd_initialized) return -1;

    uint8_t buf[SD_BLOCK_SIZE];
    for (int i = 0; i < SD_BLOCK_SIZE; i++) buf[i] = 0;

    uint8_t *p = buf;
    /* Position (fixed-point, 0.01 mm) */
    int32_t x = (int32_t)(pt->pos.x_mm * 100);
    int32_t y = (int32_t)(pt->pos.y_mm * 100);
    int32_t z = (int32_t)(pt->pos.z_mm * 100);

    *p++ = (x >>  0) & 0xFF; *p++ = (x >> 8) & 0xFF;
    *p++ = (x >> 16) & 0xFF; *p++ = (x >> 24) & 0xFF;
    *p++ = (y >>  0) & 0xFF; *p++ = (y >> 8) & 0xFF;
    *p++ = (y >> 16) & 0xFF; *p++ = (y >> 24) & 0xFF;
    *p++ = (z >>  0) & 0xFF; *p++ = (z >> 8) & 0xFF;
    *p++ = (z >> 16) & 0xFF; *p++ = (z >> 24) & 0xFF;

    *p++ = (uint8_t)pt->fault;
    *p++ = pt->pulse_idx;
    *p++ = (pt->hv_actual >> 0) & 0xFF;
    *p++ = (pt->hv_actual >> 8) & 0xFF;
    *p++ = (pt->timestamp >>  0) & 0xFF;
    *p++ = (pt->timestamp >>  8) & 0xFF;
    *p++ = (pt->timestamp >> 16) & 0xFF;
    *p++ = (pt->timestamp >> 24) & 0xFF;

    buf[510] = 0x5C;
    buf[511] = 0x4E;

    int rc = sd_write_block_data(sd_write_block, buf);
    if (rc == 0) sd_write_block++;
    return rc;
}

int sdcard_log_session(const char *text, uint32_t len) {
    if (!sd_initialized) return -1;

    /* Write text data in 512-byte blocks */
    uint32_t offset = 0;
    while (offset < len) {
        uint8_t buf[SD_BLOCK_SIZE];
        for (int i = 0; i < SD_BLOCK_SIZE; i++) buf[i] = 0;

        uint32_t chunk = (len - offset > SD_BLOCK_SIZE) ?
                          SD_BLOCK_SIZE : (len - offset);
        for (uint32_t i = 0; i < chunk; i++)
            buf[i] = (uint8_t)text[offset + i];

        int rc = sd_write_block_data(sd_write_block, buf);
        if (rc != 0) return rc;
        sd_write_block++;
        offset += chunk;
    }
    return 0;
}

int sdcard_save_trace(const char *name, int16_t *data, uint32_t samples) {
    if (!sd_initialized) return -1;

    /* Write trace as raw 16-bit samples in 512-byte blocks.
     * Each block holds 256 samples. */
    (void)name;  /* filename not used in raw block scheme */

    uint32_t total_samples = samples * ADC_CHANNELS;
    uint32_t offset = 0;

    while (offset < total_samples) {
        uint8_t buf[SD_BLOCK_SIZE];
        uint32_t chunk = (total_samples - offset > 256) ?
                          256 : (total_samples - offset);
        for (uint32_t i = 0; i < chunk; i++) {
            int16_t v = data[offset + i];
            buf[i * 2]     = (v >> 0) & 0xFF;
            buf[i * 2 + 1] = (v >> 8) & 0xFF;
        }
        /* Zero-pad remaining */
        for (uint32_t i = chunk * 2; i < SD_BLOCK_SIZE; i++)
            buf[i] = 0;

        int rc = sd_write_block_data(sd_write_block, buf);
        if (rc != 0) return rc;
        sd_write_block++;
        offset += chunk;
    }

    return 0;
}