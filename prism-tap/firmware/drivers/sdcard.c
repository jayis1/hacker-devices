/*
 * drivers/sdcard.c — SD Card Interface for Prism-Tap
 *
 * SPI-based SD card access with a simplified FAT32 file system layer.
 * Uses SPI2 (PB13/PB14/PB15) with PC4 as CS.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "sdcard.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- SPI2 helpers ---- */

static void sd_cs_low(void)
{
    GPIO_RESET(SD_SPI_PORT_CS, SD_SPI_PIN_CS);
}

static void sd_cs_high(void)
{
    GPIO_SET(SD_SPI_PORT_CS, SD_SPI_PIN_CS);
}

static uint8_t sd_spi_xfer(uint8_t b)
{
    while (!(SPI_SR(SPI2_BASE) & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)((uint32_t)SPI2_BASE + 0x0C) = b;
    while (!(SPI_SR(SPI2_BASE) & SPI_SR_RXNE))
        ;
    return *(volatile uint8_t *)((uint32_t)SPI2_BASE + 0x0C);
}

static void sd_spi_send(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        sd_spi_xfer(buf[i]);
}

static void sd_spi_recv(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        buf[i] = sd_spi_xfer(0xFF);
}

/* ---- SD card low-level commands ---- */

int sdcard_spi_init(void)
{
    /* Enable GPIOB, GPIOC and SPI2 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOCEN;
    RCC_APB1LENR |= RCC_APB1LENR_SPI2EN;

    /* Configure PB13 (SCK), PB14 (MISO), PB15 (MOSI) as AF5 (SPI2) */
    uint32_t moder = GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (SD_SPI_PIN_SCK * 2));
    moder &= ~(3U << (14 * 2));  /* MISO */
    moder &= ~(3U << (15 * 2));  /* MOSI */
    moder |= (GPIO_MODE_AF << (SD_SPI_PIN_SCK * 2));
    moder |= (GPIO_MODE_AF << (14 * 2));
    moder |= (GPIO_MODE_AF << (15 * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) = moder;

    /* AF5 for SPI2 */
    uint32_t afrl = GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET);
    for (int pin = 13; pin <= 15; pin++) {
        afrl &= ~(0xFU << (pin * 4));
        afrl |= (AF_SPI2_SCK_PB13 << (pin * 4));
    }
    GPIO_REG(GPIOB_BASE, GPIO_AFRL_OFFSET) = afrl;

    /* Configure PC4 (CS) as GPIO output, default high */
    moder = GPIO_REG(GPIOC_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (SD_SPI_PIN_CS * 2));
    moder |= (GPIO_MODE_OUTPUT << (SD_SPI_PIN_CS * 2));
    GPIO_REG(GPIOC_BASE, GPIO_MODER_OFFSET) = moder;

    /* Configure PB12 (SD_DETECT) as input with pull-up */
    moder = GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (SD_DETECT_PIN * 2));
    moder |= (GPIO_MODE_INPUT << (SD_DETECT_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_MODER_OFFSET) = moder;

    uint32_t pupdr = GPIO_REG(GPIOB_BASE, GPIO_PUPDR_OFFSET);
    pupdr &= ~(3U << (SD_DETECT_PIN * 2));
    pupdr |= (GPIO_PUPD_UP << (SD_DETECT_PIN * 2));
    GPIO_REG(GPIOB_BASE, GPIO_PUPDR_OFFSET) = pupdr;

    sd_cs_high();

    /* Configure SPI2: master, mode 0, 8-bit, initial slow clock (div 256) */
    SPI_CR1(SPI2_BASE) = 0;
    SPI_CR1(SPI2_BASE) = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                         (SPI_CR1_BR_DIV256 << SPI_CR1_BR_SHIFT);
    SPI_CR2(SPI2_BASE) = SPI_CR2_DS_8BIT | SPI_CR2_SSOE;
    SPI_CR1(SPI2_BASE) |= SPI_CR1_SPE;

    return 0;
}

int sdcard_send_cmd(uint8_t cmd, uint32_t arg, uint8_t *response, uint8_t resp_len)
{
    uint8_t crc = 0x01;  /* CRC not checked in SPI mode except for CMD0/CMD8 */

    /* CMD0 and CMD8 need valid CRC */
    if (cmd == 0) {
        crc = 0x95;
    } else if (cmd == 8) {
        crc = 0x87;
    }

    /* Send command packet */
    uint8_t pkt[6];
    pkt[0] = 0x40 | (cmd & 0x3F);
    pkt[1] = (uint8_t)(arg >> 24);
    pkt[2] = (uint8_t)(arg >> 16);
    pkt[3] = (uint8_t)(arg >> 8);
    pkt[4] = (uint8_t)(arg & 0xFF);
    pkt[5] = crc;

    sd_cs_low();
    sd_spi_send(pkt, 6);

    /* Wait for response (up to 8 retries) */
    uint8_t r1 = 0xFF;
    for (int i = 0; i < 8; i++) {
        r1 = sd_spi_xfer(0xFF);
        if (r1 != 0xFF)
            break;
    }

    if (response && resp_len > 0)
        response[0] = r1;

    /* Read additional response bytes if requested */
    if (resp_len > 1 && response) {
        sd_spi_recv(&response[1], resp_len - 1);
    }

    /* Send 8 dummy clocks after command */
    sd_spi_xfer(0xFF);

    sd_cs_high();
    return (r1 == 0x00 || r1 == 0x01) ? 0 : -1;
}

/* ---- SD card initialization ---- */

static int sd_initialized = 0;

int sdcard_init(void)
{
    if (sd_initialized)
        return 0;

    if (sdcard_spi_init())
        return -1;

    /* Send 80 dummy clocks to enter SPI mode */
    sd_cs_high();
    for (int i = 0; i < 10; i++)
        sd_spi_xfer(0xFF);

    /* CMD0: reset card */
    uint8_t resp;
    if (sdcard_send_cmd(0, 0, &resp, 1))
        return -2;

    /* CMD8: send interface condition (check SD v2) */
    uint8_t r7[5];
    if (sdcard_send_cmd(8, 0x000001AA, r7, 5) == 0) {
        /* SD v2.x — check voltage range in response */
        if (r7[4] != 0xAA)
            return -3;
    }

    /* ACMD41: send operating condition (repeat until ready) */
    uint32_t timeout = 1000;
    while (timeout--) {
        /* CMD55 first */
        sdcard_send_cmd(55, 0, &resp, 1);
        /* ACMD41 with HCS bit (bit 30) */
        if (sdcard_send_cmd(41, 0x40000000, &resp, 1) == 0 && resp == 0x00)
            break;
        for (volatile int i = 0; i < 10000; i++)
            ;
    }
    if (timeout == 0)
        return -4;

    /* CMD58: read OCR */
    uint8_t ocr[5];
    if (sdcard_send_cmd(58, 0, ocr, 5))
        return -5;

    /* Switch to fast SPI clock (div 2) */
    SPI_CR1(SPI2_BASE) &= ~SPI_CR1_SPE;
    SPI_CR1(SPI2_BASE) = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                         (SPI_CR1_BR_DIV2 << SPI_CR1_BR_SHIFT);
    SPI_CR2(SPI2_BASE) = SPI_CR2_DS_8BIT | SPI_CR2_SSOE;
    SPI_CR1(SPI2_BASE) |= SPI_CR1_SPE;

    sd_initialized = 1;
    return 0;
}

int sdcard_present(void)
{
    /* SD_DETECT is active low */
    return GPIO_READ(SD_DETECT_PORT, SD_DETECT_PIN) ? 0 : 1;
}

/* ---- File system (simplified) ----
 * This is a minimal implementation supporting sequential write/read of files.
 * A real implementation would use FatFs or a custom FAT32 driver.
 */

#define MAX_OPEN_FILES 4
#define SECTOR_SIZE 512

struct file_entry {
    int in_use;
    int is_write;
    uint32_t start_sector;
    uint32_t current_sector;
    uint32_t sector_offset;
    uint32_t total_bytes;
};

static struct file_entry file_table[MAX_OPEN_FILES];
static uint32_t next_free_sector = 1000;  /* simplified allocation */
static char file_names[MAX_OPEN_FILES][32];

int sdcard_open_write(const char *path)
{
    if (!path)
        return -1;

    /* Find free file handle */
    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_table[i].in_use) {
            fd = i;
            break;
        }
    }
    if (fd < 0)
        return -2;

    /* Allocate sectors (simplified: just use next_free_sector) */
    file_table[fd].in_use = 1;
    file_table[fd].is_write = 1;
    file_table[fd].start_sector = next_free_sector;
    file_table[fd].current_sector = next_free_sector;
    file_table[fd].sector_offset = 0;
    file_table[fd].total_bytes = 0;
    strncpy(file_names[fd], path, sizeof(file_names[fd]) - 1);
    file_names[fd][sizeof(file_names[fd]) - 1] = '\0';

    next_free_sector += 1000;  /* reserve 500 KB per file (simplified) */

    return fd;
}

int sdcard_open_read(const char *path)
{
    if (!path)
        return -1;

    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_table[i].in_use) {
            fd = i;
            break;
        }
    }
    if (fd < 0)
        return -2;

    /* In a real implementation, look up the file in the FAT directory.
       For this simplified version, we just open sequentially. */
    file_table[fd].in_use = 1;
    file_table[fd].is_write = 0;
    file_table[fd].start_sector = 1000;  /* would be from directory entry */
    file_table[fd].current_sector = 1000;
    file_table[fd].sector_offset = 0;
    file_table[fd].total_bytes = 0;

    return fd;
}

int sdcard_close(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_table[fd].in_use)
        return -1;
    file_table[fd].in_use = 0;
    return 0;
}

int sdcard_write(int fd, const uint8_t *data, uint32_t len)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_table[fd].in_use ||
        !file_table[fd].is_write)
        return -1;

    /* In a real implementation, this would write to SD card sectors via
       SPI block write commands (CMD24). For this firmware, we simulate
       the write by tracking bytes. */
    file_table[fd].total_bytes += len;

    /* Simulate sector writes */
    while (len > 0) {
        uint32_t chunk = SECTOR_SIZE - file_table[fd].sector_offset;
        if (chunk > len)
            chunk = len;

        /* Would write to SD card here:
         * sdcard_write_block(file_table[fd].current_sector, data, chunk);
         */

        file_table[fd].sector_offset += chunk;
        if (file_table[fd].sector_offset >= SECTOR_SIZE) {
            file_table[fd].current_sector++;
            file_table[fd].sector_offset = 0;
        }

        data += chunk;
        len -= chunk;
    }

    return (int)file_table[fd].total_bytes;
}

int sdcard_read(int fd, uint8_t *data, uint32_t len)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_table[fd].in_use ||
        file_table[fd].is_write)
        return -1;

    /* Simulated read — in production, use CMD17 (read single block) */
    file_table[fd].total_bytes += len;
    return (int)len;
}

int sdcard_mkdir(const char *path)
{
    /* FAT32 directory creation — simplified */
    (void)path;
    return 0;
}

int sdcard_file_exists(const char *path)
{
    /* In a real implementation, scan FAT directory entries.
       For this simplified version, return 0 (not found). */
    (void)path;
    return 0;
}

int sdcard_delete(const char *path)
{
    (void)path;
    return 0;
}

int sdcard_list_dir(const char *path, char *out, uint32_t max_len)
{
    if (!out || max_len == 0)
        return -1;
    (void)path;
    out[0] = '\0';
    return 0;
}

uint64_t sdcard_capacity_bytes(void)
{
    /* Would read CSD register and compute capacity */
    return 32ULL * 1024 * 1024 * 1024;  /* 32 GB placeholder */
}

uint64_t sdcard_free_bytes(void)
{
    return sdcard_capacity_bytes();  /* simplified */
}

/* ---- End of sdcard.c ----
 * Author: jayis1
 */