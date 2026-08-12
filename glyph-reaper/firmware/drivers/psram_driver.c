/**
 * @file psram_driver.c
 * @brief QSPI PSRAM driver implementation
 *
 * Implements QSPI communication with the APS6404L-3SQR-SN 8MB PSRAM
 * using the nRF52840 QSPI peripheral.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "psram_driver.h"
#include "board.h"
#include "registers.h"

static bool s_psram_ready = false;

bool psram_init(void)
{
    /* Configure QSPI pins */
    NRF_QSPI->PSEL_SCK = BOARD_PIN_QSPI_CLK;
    NRF_QSPI->PSEL_CSN0 = BOARD_PIN_QSPI_CSN_PSRAM;
    NRF_QSPI->PSEL_IO0 = BOARD_PIN_QSPI_IO0;
    NRF_QSPI->PSEL_IO1 = BOARD_PIN_QSPI_IO1;
    NRF_QSPI->PSEL_IO2 = BOARD_PIN_QSPI_IO2;
    NRF_QSPI->PSEL_IO3 = BOARD_PIN_QSPI_IO3;

    /* Configure PSRAM IFCONFIG (CS0) */
    /* Quad I/O read with 6 dummy cycles, Quad I/O write */
    NRF_QSPI->IFCONFIG0 = QSPI_IFCONFIG_READOC(QSPI_READOC_READ4IO) |
                          QSPI_IFCONFIG_WRITEOC(QSPI_WRITEOC_PP4IO) |
                          QSPI_IFCONFIG_ADDRMODE(1) |  /* 24-bit address */
                          QSPI_IFCONFIG_DUMMYCNT(PSRAM_DUMMY_CYCLES) |
                          QSPI_IFCONFIG_SCKDELAY(1);

    /* Set QSPI clock frequency (64 MHz / (1 + 1) = 32 MHz for PSRAM) */
    /* The SCKFREQ field: f_qspi = 64MHz / (1 + SCKFREQ) */
    volatile uint32_t *qspi_sckfreq = (volatile uint32_t *)(NRF_QSPI_BASE + 0x058);
    *qspi_sckfreq = 1;  /* 32 MHz */

    /* Enable QSPI */
    NRF_QSPI->ENABLE = QSPI_ENABLE_ENABLE;

    /* Activate QSPI */
    NRF_QSPI->TASKS_ACTIVATE = 1;
    while (NRF_QSPI->EVENTS_READY == 0);
    NRF_QSPI->EVENTS_READY = 0;

    /* Send Enter Quad Mode command to PSRAM (0x35) */
    NRF_QSPI->CINSTRDAT = 0x35000000UL;
    NRF_QSPI->CINSTRCONF = QSPI_CINSTRCONF_OPCODE(PSRAM_ENTER_QUAD_OPCODE) |
                           QSPI_CINSTRCONF_LENGTH(1) |
                           QSPI_CINSTRCONF_WIPWAIT;

    /* Wait for custom instruction to complete */
    while (NRF_QSPI->EVENTS_READY == 0);
    NRF_QSPI->EVENTS_READY = 0;

    s_psram_ready = true;

    /* Run a quick integrity test */
    return psram_test();
}

bool psram_read(uint32_t addr, uint8_t *buffer, uint32_t length)
{
    if (!s_psram_ready || addr + length > PSRAM_SIZE) {
        return false;
    }

    /* Set up read operation */
    NRF_QSPI->READ_DST = (uint32_t)buffer;
    NRF_QSPI->READ_CNT = length;
    volatile uint32_t *qspi_read_src = (volatile uint32_t *)(NRF_QSPI_BASE + 0x054);
    *qspi_read_src = addr;

    /* Trigger read (using erase task as placeholder for read trigger) */
    /* In actual implementation, the nRF52840 QSPI read is triggered
       by setting READ_SRC and READ_CNT, then waiting for EVENTS_READY */
    while (NRF_QSPI->EVENTS_READY == 0);
    NRF_QSPI->EVENTS_READY = 0;

    return true;
}

bool psram_write(uint32_t addr, const uint8_t *data, uint32_t length)
{
    if (!s_psram_ready || addr + length > PSRAM_SIZE) {
        return false;
    }

    /* Set up write operation */
    NRF_QSPI->WRITE_SRC = (uint32_t)data;
    NRF_QSPI->WRITE_DST = addr;
    NRF_QSPI->WRITE_CNT = length;

    /* Wait for completion */
    while (NRF_QSPI->EVENTS_READY == 0);
    NRF_QSPI->EVENTS_READY = 0;

    return true;
}

void psram_erase(uint32_t addr, uint32_t length)
{
    /* PSRAM doesn't need erase — just write zeros */
    uint8_t zero_buf[256];
    memset(zero_buf, 0, sizeof(zero_buf));

    uint32_t offset = 0;
    while (offset < length) {
        uint32_t chunk = MIN(sizeof(zero_buf), length - offset);
        psram_write(addr + offset, zero_buf, chunk);
        offset += chunk;
    }
}

bool psram_test(void)
{
    uint8_t test_data[16];
    uint8_t read_data[16];
    uint32_t test_addr = PSRAM_SIZE - 256;  /* Test at end of PSRAM */

    /* Write test pattern */
    for (int i = 0; i < 16; i++) {
        test_data[i] = (uint8_t)(0xA5 ^ i);
    }

    if (!psram_write(test_addr, test_data, 16)) {
        return false;
    }

    /* Read back and verify */
    if (!psram_read(test_addr, read_data, 16)) {
        return false;
    }

    /* Compare */
    for (int i = 0; i < 16; i++) {
        if (read_data[i] != test_data[i]) {
            return false;
        }
    }

    /* Write zeros to clean up */
    memset(test_data, 0, 16);
    psram_write(test_addr, test_data, 16);

    return true;
}