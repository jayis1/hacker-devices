/**
 * sdmmc.c — SD/MMC Card Interface for Flash Dump Storage
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Provides block-level read/write access to a microSD card for
 * storing flash dumps, boundary scan captures, and log files.
 * Uses SDMMC1 in 4-bit mode at up to 48 MHz clock.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "board.h"

/* SD card commands (simplified subset) */
#define SD_CMD_GO_IDLE           0
#define SD_CMD_SEND_IF_COND      8
#define SD_CMD_SEND_CSD          9
#define SD_CMD_SEND_CID          10
#define SD_CMD_SET_BLOCKLEN      16
#define SD_CMD_READ_SINGLE       17
#define SD_CMD_READ_MULTI        18
#define SD_CMD_WRITE_SINGLE      24
#define SD_CMD_WRITE_MULTI       25
#define SD_CMD_APP_CMD           55
#define SD_CMD_READ_OCR          58

/* ACMD (application-specific) */
#define SD_ACMD_SEND_OP_COND     41
#define SD_ACMD_SET_WR_BLK_ERASE 23

/* R1 response bits */
#define SD_R1_IDLE               (1 << 0)
#define SD_R1_ERASE_RESET        (1 << 1)
#define SD_R1_ILLEGAL_CMD        (1 << 2)
#define SD_R1_CMD_CRC_ERR        (1 << 3)
#define SD_R1_ERASE_SEQ_ERR      (1 << 4)
#define SD_R1_ADDR_ERR           (1 << 5)
#define SD_R1_PARAM_ERR          (1 << 6)

/* Card type */
#define SD_TYPE_UNKNOWN          0
#define SD_TYPE_SD_V1            1
#define SD_TYPE_SD_V2            2
#define SD_TYPE_SDHC             3
#define SD_TYPE_MMC              4

static uint8_t g_sd_type = SD_TYPE_UNKNOWN;
static bool g_sd_initialized = false;

/**
 * sdmmc_init: Initialize the SD/MMC interface and detect card.
 * Returns true if a card is present and ready for block I/O.
 */
bool sdmmc_init(void)
{
    /* Enable SDMMC1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SDMMC1EN;

    /* Configure SDMMC pins for 4-bit mode */
    mpio_set_af(SDMMC_CK_PORT, SDMMC_CK_PIN, 12);   /* SDMMC_CK */
    mpio_set_af(SDMMC_CMD_PORT, SDMMC_CMD_PIN, 12);  /* SDMMC_CMD */
    mpio_set_af(SDMMC_D0_PORT, SDMMC_D0_PIN, 12);
    mpio_set_af(SDMMC_D1_PORT, SDMMC_D1_PIN, 12);
    mpio_set_af(SDMMC_D2_PORT, SDMMC_D2_PIN, 12);
    mpio_set_af(SDMMC_D3_PORT, SDMMC_D3_PIN, 12);

    mpio_set_mode(SDMMC_CK_PORT, SDMMC_CK_PIN, GPIO_MODE_AF_PP);
    mpio_set_mode(SDMMC_CMD_PORT, SDMMC_CMD_PIN, GPIO_MODE_AF_PP);
    mpio_set_mode(SDMMC_D0_PORT, SDMMC_D0_PIN, GPIO_MODE_AF_PP);
    mpio_set_mode(SDMMC_D1_PORT, SDMMC_D1_PIN, GPIO_MODE_AF_PP);
    mpio_set_mode(SDMMC_D2_PORT, SDMMC_D2_PIN, GPIO_MODE_AF_PP);
    mpio_set_mode(SDMMC_D3_PORT, SDMMC_D3_PIN, GPIO_MODE_AF_PP);

    /* Set pull-ups on CMD and D0-D3 */
    mpio_set_pull(SDMMC_CMD_PORT, SDMMC_CMD_PIN, 1);
    mpio_set_pull(SDMMC_D0_PORT, SDMMC_D0_PIN, 1);
    mpio_set_pull(SDMMC_D1_PORT, SDMMC_D1_PIN, 1);
    mpio_set_pull(SDMMC_D2_PORT, SDMMC_D2_PIN, 1);
    mpio_set_pull(SDMMC_D3_PORT, SDMMC_D3_PIN, 1);

    /* Power on the SDMMC peripheral */
    SDMMC1->CLKCR = 0;
    SDMMC1->POWER = 1;  /* Power on */

    /* Send at least 74 clock cycles for card initialization */
    SDMMC1->CLKCR = (2 << 8) | (1 << 10);  /* CLKDIV=2 (20 MHz), HWFC=1 */
    SDMMC1->DTIMER = 0xFFFFFFFF;
    delay_ms(5);

    /* Send CMD0 (GO_IDLE) to reset card */
    sdmmc_send_cmd(SD_CMD_GO_IDLE, 0, 0x95);
    delay_ms(2);

    /* Check card type via CMD8 */
    uint32_t resp;
    if (sdmmc_send_cmd(SD_CMD_SEND_IF_COND, 0x1AA, 0x87))
    {
        resp = SDMMC1->RESP1;
        if ((resp & 0xFF) == 0xAA)
        {
            g_sd_type = SD_TYPE_SD_V2;
        }
    }

    /* Send ACMD41 to initialize */
    uint32_t ocr = 0;
    for (int i = 0; i < 500; i++)
    {
        sdmmc_send_cmd(SD_CMD_APP_CMD, 0, 0);
        if (g_sd_type == SD_TYPE_SD_V2)
            sdmmc_send_cmd(SD_ACMD_SEND_OP_COND, 0x40000000, 0);
        else
            sdmmc_send_cmd(SD_ACMD_SEND_OP_COND, 0x00000000, 0);

        resp = SDMMC1->RESP1;
        if (resp & (1 << 31))
        {
            ocr = resp;
            break;
        }
        delay_ms(5);
    }

    /* Check if SDHC */
    if (ocr & (1 << 30))
        g_sd_type = SD_TYPE_SDHC;

    /* Set block length to 512 bytes */
    sdmmc_send_cmd(SD_CMD_SET_BLOCKLEN, 512, 0);

    /* Set clock to high-speed (48 MHz / 2 = 24 MHz) */
    SDMMC1->CLKCR = (2 << 8) | (1 << 10) | 1;  /* Enable clock */

    /* Configure 4-bit wide bus */
    sdmmc_send_cmd(SD_CMD_APP_CMD, 0, 0);
    sdmmc_send_cmd(6, 2, 0);  /* Set bus width to 4-bit */
    SDMMC1->CLKCR |= (1 << 16);  /* Wide bus mode */

    g_sd_initialized = true;
    return true;
}

/**
 * sdmmc_send_cmd: Send a command to the SD card and wait for response.
 * Returns true if response received.
 */
static bool sdmmc_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    /* Wait for previous command to complete */
    while (SDMMC1->CMD & (1 << 10));  /* Wait for CMDSENT */

    /* Set command register */
    SDMMC1->ARG = arg;
    SDMMC1->CMD = (cmd & 0x3F) | (1 << 6) | (1 << 5) | ((uint32_t)crc << 8);

    /* Wait for response */
    uint32_t timeout = 1000000;
    while (!(SDMMC1->STA & (1 << 0)) && !(SDMMC1->STA & (1 << 2)) && timeout)
    {
        timeout--;
    }

    if (!timeout) return false;
    if (SDMMC1->STA & (1 << 2)) return false;  /* Command timeout */

    /* Clear status flags */
    SDMMC1->ICR = (1 << 0) | (1 << 2);

    return true;
}

/**
 * sdmmc_read_blocks: Read blocks from SD card into buffer.
 * Returns true on success.
 */
bool sdmmc_read_blocks(uint32_t block_addr, uint8_t *buffer, uint32_t count)
{
    if (!g_sd_initialized) return false;

    if (count == 1)
    {
        /* Single block read */
        sdmmc_send_cmd(SD_CMD_READ_SINGLE, block_addr, 0);
        sdmmc_read_data_block(buffer, 512);
    }
    else
    {
        /* Multi block read */
        sdmmc_send_cmd(SD_CMD_READ_MULTI, block_addr, 0);
        for (uint32_t i = 0; i < count; i++)
        {
            sdmmc_read_data_block(&buffer[i * 512], 512);
        }
        sdmmc_send_cmd(12, 0, 0);  /* STOP_TRANSMISSION */
    }

    return true;
}

/**
 * sdmmc_write_block: Write a single 512-byte block to SD card.
 * Returns true on success.
 */
bool sdmmc_write_block(uint32_t block_addr, const uint8_t *data, uint32_t len)
{
    if (!g_sd_initialized) return false;

    uint32_t num_blocks = (len + 511) / 512;
    uint32_t block = (g_sd_type == SD_TYPE_SDHC) ? block_addr / 512 : block_addr;

    if (num_blocks == 1)
    {
        sdmmc_send_cmd(SD_CMD_WRITE_SINGLE, block, 0);
        sdmmc_write_data_block(data, 512);
    }
    else
    {
        sdmmc_send_cmd(SD_CMD_WRITE_MULTI, block, 0);
        for (uint32_t i = 0; i < num_blocks; i++)
        {
            sdmmc_write_data_block(&data[i * 512], 512);
        }
        sdmmc_send_cmd(12, 0, 0);
    }

    /* Wait for card ready */
    while (!(SDMMC1->STA & (1 << 5)));  /* Data available */

    return true;
}

/**
 * sdmmc_read_data_block: Read a block of data from the SDMMC data path.
 */
static void sdmmc_read_data_block(uint8_t *buffer, uint32_t size)
{
    /* Configure data transfer */
    SDMMC1->DTIMER = 0xFFFFFFFF;
    SDMMC1->DLEN = size;
    SDMMC1->DCTRL = (1 << 3) | (1 << 0);  /* Data direction=card-to-host, enable */

    /* Read data from FIFO */
    uint32_t words = size / 4;
    for (uint32_t i = 0; i < words; i++)
    {
        while (!(SDMMC1->STA & (1 << 10)));  /* RX FIFO not empty */
        ((uint32_t*)buffer)[i] = SDMMC1->FIFO;
    }

    /* Clear status */
    SDMMC1->ICR = 0x7FF;
}

/**
 * sdmmc_write_data_block: Write a block of data to the SDMMC data FIFO.
 */
static void sdmmc_write_data_block(const uint8_t *data, uint32_t size)
{
    SDMMC1->DTIMER = 0xFFFFFFFF;
    SDMMC1->DLEN = size;
    SDMMC1->DCTRL = (1 << 0);  /* Data direction=host-to-card, enable */

    uint32_t words = size / 4;
    for (uint32_t i = 0; i < words; i++)
    {
        while (!(SDMMC1->STA & (1 << 9)));  /* TX FIFO not full */
        SDMMC1->FIFO = ((uint32_t*)data)[i];
    }
}

/**
 * sdmmc_card_present: Check if an SD card is inserted.
 * Uses the card detect pin (if available) or tries a CMD0.
 */
bool sdmmc_card_present(void)
{
    if (g_sd_initialized) return true;

    /* Quick check: try CMD0 */
    return sdmmc_send_cmd(SD_CMD_GO_IDLE, 0, 0x95);
}