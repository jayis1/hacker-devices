/*
 * eddy-phantom — sdcard.c
 * MicroSD card driver via SPI4 with FatFS-style file operations.
 * Used for: profile storage, raw burst logging, session logs.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── SD card SPI commands (SPI mode) ──────────────────────────── */
#define SD_CMD0_GO_IDLE     0       /* R1 */
#define SD_CMD1_INIT        1       /* R1 */
#define SD_CMD8_SEND_IF     8       /* R7 */
#define SD_CMD9_SEND_CSD    9       /* R1 + data */
#define SD_CMD16_SET_BLK    16      /* R1 */
#define SD_CMD17_READ_BLK   17      /* R1 + data */
#define SD_CMD24_WRITE_BLK  24      /* R1 + data */
#define SD_CMD55_APP        55      /* R1 */
#define SD_ACMD41_SD_INIT   41      /* R1 */
#define SD_ACMD42_SET_WIDTH 42      /* R1 */

#define SD_R1_IDLE          0x01
#define SD_R1_READY         0x00
#define SD_BLOCK_SIZE       512

/* Token for single block write */
#define SD_TOKEN_START      0xFE
#define SD_TOKEN_MBR        0xFC
#define SD_TOKEN_STOP       0xFD

/* ── SPI4 low-level ──────────────────────────────────────────── */
static uint8_t spi4_xfer(uint8_t tx)
{
    spi_regs_t *spi = SPI(4);

    volatile uint32_t timeout = 0xFFFF;
    while (!(spi->SR & SPI_SR_TXE) && --timeout);

    /* Write byte (lower 8 bits of DR for 8-bit frame) */
    *(volatile uint8_t *)&spi->DR = tx;

    timeout = 0xFFFF;
    while (!(spi->SR & SPI_SR_RXNE) && --timeout);

    return (uint8_t)spi->DR;
}

static void sd_cs_select(void)
{
    gpio_clr(GPIO(SD_CS_PORT), SD_CS_PIN);
}

static void sd_cs_deselect(void)
{
    gpio_set(GPIO(SD_CS_PORT), SD_CS_PIN);
}

/* ── Send SD command and get R1 response ──────────────────────── */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t r1;
    uint8_t retry = 0;

    /* Send command: 0x40 | cmd, arg[31:24], arg[23:16], arg[15:8], arg[7:0], CRC */
    spi4_xfer(0x40 | cmd);
    spi4_xfer((arg >> 24) & 0xFF);
    spi4_xfer((arg >> 16) & 0xFF);
    spi4_xfer((arg >> 8) & 0xFF);
    spi4_xfer(arg & 0xFF);

    /* CRC (only valid for CMD0 and CMD8; others use dummy CRC) */
    if (cmd == 0) {
        spi4_xfer(0x95);  /* valid CRC for CMD0 */
    } else if (cmd == 8) {
        spi4_xfer(0x87);  /* valid CRC for CMD8 */
    } else {
        spi4_xfer(0xFF);  /* dummy CRC */
    }

    /* Wait for response (up to 8 retries) */
    do {
        r1 = spi4_xfer(0xFF);
    } while (r1 == 0xFF && ++retry < 10);

    return r1;
}

/* ── Wait for SD card ready (after write) ─────────────────────── */
static int sd_wait_ready(uint32_t timeout)
{
    uint8_t r;
    do {
        r = spi4_xfer(0xFF);
        if (r == 0xFF) return 0;
    } while (--timeout);
    return -1;
}

/* ── Initialize SD card ───────────────────────────────────────── */
int sdcard_init(void)
{
    spi_regs_t *spi = SPI(4);
    uint8_t r1;
    uint32_t retry;

    /* Configure SPI4: master, mode 0, 8-bit, baud rate /64 initially
     * (400 kHz max during initialization: 120 MHz / 256 = 469 kHz) */
    spi->CR1 = 0;
    spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV256;
    spi->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    spi->CR1 |= SPI_CR1_SPE;

    /* Send 80+ dummy clocks with CS high to enter SPI mode */
    sd_cs_deselect();
    for (int i = 0; i < 10; i++) {
        spi4_xfer(0xFF);
    }

    /* CMD0: GO_IDLE_STATE — reset card to SPI mode */
    sd_cs_select();
    r1 = sd_send_cmd(SD_CMD0_GO_IDLE, 0);
    sd_cs_deselect();

    if (r1 != SD_R1_IDLE) {
        return -1;  /* card not responding */
    }

    /* CMD8: SEND_IF_COND — check for SD v2.x */
    sd_cs_select();
    r1 = sd_send_cmd(SD_CMD8_SEND_IF, 0x000001AA);
    if (r1 & SD_R1_IDLE) {
        /* SD v2.x — read remaining 4 bytes of R7 response */
        spi4_xfer(0xFF);  /* OCR byte */
        spi4_xfer(0xFF);  /* OCR byte */
        spi4_xfer(0xFF);  /* check pattern echo */
        spi4_xfer(0xFF);  /* check pattern echo */
    }
    sd_cs_deselect();

    /* ACMD41: SD_SEND_OP_COND — initialize card (send repeatedly) */
    sd_cs_select();
    retry = 0;
    do {
        sd_send_cmd(SD_CMD55_APP, 0);
        r1 = sd_send_cmd(SD_ACMD41_SD_INIT, 0x40000000);  /* HCS = 1 for SDHC */
        if (++retry > 1000) {
            sd_cs_deselect();
            return -1;
        }
    } while (r1 != SD_R1_READY);
    sd_cs_deselect();

    /* CMD16: SET_BLOCKLEN to 512 (for non-SDHC cards) */
    sd_cs_select();
    sd_send_cmd(SD_CMD16_SET_BLK, SD_BLOCK_SIZE);
    sd_cs_deselect();

    /* Switch to higher SPI speed: /8 = 15 MHz */
    spi->CR1 &= ~SPI_CR1_SPE;
    spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV8;
    spi->CR1 |= SPI_CR1_SPE;

    return 0;
}

/* ── Read a single 512-byte block ─────────────────────────────── */
static int sd_read_block(uint32_t block_addr, uint8_t *buf)
{
    uint8_t r1;
    uint16_t i;

    sd_cs_select();
    r1 = sd_send_cmd(SD_CMD17_READ_BLK, block_addr);
    if (r1 != 0) {
        sd_cs_deselect();
        return -1;
    }

    /* Wait for start token (0xFE) */
    uint16_t retry = 0;
    uint8_t token;
    do {
        token = spi4_xfer(0xFF);
        if (token == SD_TOKEN_START) break;
        if (++retry > 1000) {
            sd_cs_deselect();
            return -1;
        }
    } while (1);

    /* Read 512 bytes */
    for (i = 0; i < SD_BLOCK_SIZE; i++) {
        buf[i] = spi4_xfer(0xFF);
    }

    /* Read 2-byte CRC (discard) */
    spi4_xfer(0xFF);
    spi4_xfer(0xFF);

    sd_cs_deselect();
    return 0;
}

/* ── Write a single 512-byte block ────────────────────────────── */
static int sd_write_block(uint32_t block_addr, const uint8_t *buf)
{
    uint8_t r1;
    uint16_t i;

    sd_cs_select();
    r1 = sd_send_cmd(SD_CMD24_WRITE_BLK, block_addr);
    if (r1 != 0) {
        sd_cs_deselect();
        return -1;
    }

    /* Send start token for single block write */
    spi4_xfer(SD_TOKEN_START);

    /* Send 512 bytes */
    for (i = 0; i < SD_BLOCK_SIZE; i++) {
        spi4_xfer(buf[i]);
    }

    /* Send 2-byte dummy CRC */
    spi4_xfer(0xFF);
    spi4_xfer(0xFF);

    /* Read data response token */
    uint8_t resp = spi4_xfer(0xFF);
    if ((resp & 0x1F) != 0x05) {  /* 0x05 = data accepted */
        sd_cs_deselect();
        return -1;
    }

    /* Wait for card to finish writing */
    if (sd_wait_ready(0xFFFF) != 0) {
        sd_cs_deselect();
        return -1;
    }

    sd_cs_deselect();
    return 0;
}

/* ── Log a burst record to SD card ──────────────────────────────
 * Appends burst metadata (not raw samples — too large for SD)
 * to the burst log file. Raw samples go to SDRAM ring buffer.
 */
int sdcard_log_burst(burst_record_t *rec)
{
    /* In a full implementation, this would use FatFs to append
     * to a log file. Here we implement a simplified block-based log.
     *
     * Log format (per entry, 16 bytes):
     *   [0..3]   timestamp_ms
     *   [4]      scancode
     *   [5]      confidence
     *   [6]      classified
     *   [7]      trigger_channel
     *   [8..11]  vga_gain[0..1] (packed)
     *   [12..15] vga_gain[2..3] (packed)
     */

    static uint8_t block_buf[SD_BLOCK_SIZE];
    static uint16_t block_offset = 0;
    static uint32_t current_block = 1000;  /* starting block for burst log */

    /* Pack record into log entry */
    uint8_t entry[16];
    entry[0]  = (rec->timestamp_ms >> 24) & 0xFF;
    entry[1]  = (rec->timestamp_ms >> 16) & 0xFF;
    entry[2]  = (rec->timestamp_ms >> 8) & 0xFF;
    entry[3]  = rec->timestamp_ms & 0xFF;
    entry[4]  = rec->scancode;
    entry[5]  = rec->confidence;
    entry[6]  = rec->classified;
    entry[7]  = rec->trigger_channel;
    entry[8]  = (rec->vga_gain[0] >> 8) & 0xFF;
    entry[9]  = rec->vga_gain[0] & 0xFF;
    entry[10] = (rec->vga_gain[1] >> 8) & 0xFF;
    entry[11] = rec->vga_gain[1] & 0xFF;
    entry[12] = (rec->vga_gain[2] >> 8) & 0xFF;
    entry[13] = rec->vga_gain[2] & 0xFF;
    entry[14] = (rec->vga_gain[3] >> 8) & 0xFF;
    entry[15] = rec->vga_gain[3] & 0xFF;

    /* Copy entry to block buffer */
    for (int i = 0; i < 16; i++) {
        block_buf[block_offset++] = entry[i];
    }

    /* If block is full, write to SD */
    if (block_offset >= SD_BLOCK_SIZE) {
        if (sd_write_block(current_block, block_buf) != 0)
            return -1;
        current_block++;
        block_offset = 0;

        /* Clear buffer for next block */
        for (int i = 0; i < SD_BLOCK_SIZE; i++)
            block_buf[i] = 0;
    }

    return 0;
}

/* ── Log session text to SD card ──────────────────────────────── */
int sdcard_log_session(const char *text, uint32_t len)
{
    /* Write text to a session log file (simplified block-based) */
    static uint32_t session_block = 2000;  /* starting block for session log */
    static uint8_t  session_buf[SD_BLOCK_SIZE];
    static uint16_t session_offset = 0;

    for (uint32_t i = 0; i < len; i++) {
        session_buf[session_offset++] = (uint8_t)text[i];
        if (session_offset >= SD_BLOCK_SIZE) {
            sd_write_block(session_block, session_buf);
            session_block++;
            session_offset = 0;
            for (int j = 0; j < SD_BLOCK_SIZE; j++)
                session_buf[j] = 0;
        }
    }

    /* Flush remaining data */
    if (session_offset > 0) {
        sd_write_block(session_block, session_buf);
        session_block++;
        session_offset = 0;
    }

    return 0;
}

/* ── Load a keyboard profile from SD card ───────────────────────
 * Profiles are stored as binary blocks on the SD card.
 * Each profile starts at a known block address and contains:
 *   [0..31]    profile name (null-terminated string)
 *   [32..33]   controller_id
 *   [34..35]   num_keys
 *   [36..36+N*2] scancode table
 *   [...]      reference offset in QSPI for feature data
 *   [...]      reference size
 */
int sdcard_load_profile(const char *name, keyboard_profile_t *prof)
{
    /* Profile directory starts at block 100 */
    /* Each profile descriptor is 1 block (512 bytes) */
    /* First 16 profiles are at blocks 100-115 */

    uint8_t buf[SD_BLOCK_SIZE];

    for (int slot = 0; slot < 16; slot++) {
        if (sd_read_block(100 + slot, buf) != 0)
            continue;

        /* Check if slot is empty (first byte = 0xFF) */
        if (buf[0] == 0xFF)
            continue;

        /* Compare name */
        int match = 1;
        for (int i = 0; i < PROFILE_NAME_LEN; i++) {
            if (name[i] != (char)buf[i]) {
                match = 0;
                break;
            }
            if (name[i] == '\0') break;
        }

        if (!match)
            continue;

        /* Found matching profile — parse it */
        for (int i = 0; i < PROFILE_NAME_LEN; i++) {
            prof->name[i] = (char)buf[i];
        }
        prof->controller_id = buf[32] | (buf[33] << 8);
        prof->num_keys = buf[34] | (buf[35] << 8);

        if (prof->num_keys > PROFILE_MAX_KEYS)
            prof->num_keys = PROFILE_MAX_KEYS;

        for (int i = 0; i < prof->num_keys; i++) {
            prof->scancodes[i] = buf[36 + i*2] | (buf[36 + i*2 + 1] << 8);
        }

        /* QSPI reference offset and size */
        uint32_t ref_off = buf[36 + prof->num_keys * 2] |
                           (buf[36 + prof->num_keys * 2 + 1] << 8) |
                           (buf[36 + prof->num_keys * 2 + 2] << 16) |
                           (buf[36 + prof->num_keys * 2 + 3] << 24);
        prof->ref_offset = ref_off;
        prof->ref_size = buf[36 + prof->num_keys * 2 + 4] |
                         (buf[36 + prof->num_keys * 2 + 5] << 8) |
                         (buf[36 + prof->num_keys * 2 + 6] << 16) |
                         (buf[36 + prof->num_keys * 2 + 7] << 24);

        return 0;
    }

    return -1;  /* profile not found */
}

/* ── List available profiles (returns count) ──────────────────── */
int sdcard_list_profiles(void)
{
    uint8_t buf[SD_BLOCK_SIZE];
    int count = 0;

    for (int slot = 0; slot < 16; slot++) {
        if (sd_read_block(100 + slot, buf) != 0)
            continue;
        if (buf[0] != 0xFF && buf[0] != 0x00)
            count++;
    }

    return count;
}