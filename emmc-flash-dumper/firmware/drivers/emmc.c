/**
 * emmc.c — eMMC 5.1 Flash Driver Implementation
 *
 * Full JESD84-B51 eMMC 5.1 driver for STM32H743VI SDMMC2 peripheral.
 * Implements HS400 mode initialization, DMA-based multi-block read,
 * partition switching, and EXT_CSD parsing for forensic acquisition.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#include "emmc.h"
#include "../board.h"
#include "../registers.h"
#include <stddef.h>

/*===========================================================================
 * Static State
 *===========================================================================*/

static emmc_device_info_t g_emmc_info;
static bool g_initialized = false;
static bool g_card_present = false;
static uint32_t g_current_clock = 0;
static uint8_t  g_current_bus_width = 1;
static bool     g_ddr_mode = false;
static bool     g_hw_flow = false;

/*===========================================================================
 * Internal Helpers
 *===========================================================================*/

/* Wait for SDMMC2 status flag with timeout */
static int sdmmc2_wait_status(uint32_t mask, uint32_t timeout_ms) {
    uint32_t start = SYSTICK->CVR;
    uint32_t elapsed = 0;
    while (!(SDMMC2->STA & mask)) {
        if (SDMMC2->STA & (SDMMC_STA_CTIMEOUT | SDMMC_STA_DTIMEOUT |
                            SDMMC_STA_CCRCFAIL | SDMMC_STA_DCRCFAIL)) {
            return -1;
        }
        elapsed = (start - SYSTICK->CVR) / (SYSTICK->RVR / 1000);
        if (elapsed > timeout_ms) return -2;
    }
    return 0;
}

/* Send a command and wait for response */
static int sdmmc2_send_cmd(uint8_t cmd_idx, uint32_t arg, uint8_t resp_type) {
    /* Wait for command path to be free */
    while (SDMMC2->STA & SDMMC_STA_CMDSENT);

    /* Clear status flags */
    SDMMC2->ICR = 0xFFFFFFFF;

    /* Set argument */
    SDMMC2->ARG = arg;

    /* Build command register */
    uint32_t cmd_reg = cmd_idx | SDMMC_CMD_CPSMEN;

    switch (resp_type) {
    case EMMC_RESP_NONE:
        break;
    case EMMC_RESP_R1:
    case EMMC_RESP_R1b:
    case EMMC_RESP_R5:
        cmd_reg |= SDMMC_CMD_WAITRESP_SHORT;
        break;
    case EMMC_RESP_R2:
        cmd_reg |= SDMMC_CMD_WAITRESP_LONG;
        break;
    case EMMC_RESP_R3:
    case EMMC_RESP_R4:
        cmd_reg |= SDMMC_CMD_WAITRESP_SHORT;
        break;
    }

    /* Send command */
    SDMMC2->CMD = cmd_reg;

    /* Wait for response */
    int ret = sdmmc2_wait_status(SDMMC_STA_CMDREND | SDMMC_STA_CCRCFAIL |
                                  SDMMC_STA_CTIMEOUT, 100);
    if (ret < 0) return ret;

    /* Check for CRC error */
    if (SDMMC2->STA & SDMMC_STA_CCRCFAIL) {
        SDMMC2->ICR = SDMMC_STA_CCRCFAIL;
        return -3;
    }

    return 0;
}

/* Read data from SDMMC2 FIFO */
static void sdmmc2_read_fifo(uint32_t *buf, uint32_t words) {
    for (uint32_t i = 0; i < words; i++) {
        buf[i] = SDMMC2->FIFO;
    }
}

/* Configure data transfer */
static void sdmmc2_config_data(uint32_t length, bool read_dir, uint32_t block_size) {
    SDMMC2->DLEN = length;
    SDMMC2->DCTRL = SDMMC_DCTRL_DTEN |
                    (read_dir ? SDMMC_DCTRL_DTDIR : 0) |
                    SDMMC_DCTRL_DTMODE;  /* Block mode */
    /* Set block size: 0=1B, 1=2B, ..., 9=512B */
    uint32_t bs = 0;
    uint32_t sz = block_size;
    while (sz > 1) { sz >>= 1; bs++; }
    SDMMC2->DCTRL |= (bs << SDMMC_DCTRL_DBLOCKSIZE_Pos);
}

/*===========================================================================
 * GPIO & Clock Initialization
 *===========================================================================*/

int emmc_init(void) {
    if (g_initialized) return EMMC_OK;

    /* Enable SDMMC2 clock */
    RCC->AHB3ENR |= RCC_AHB3ENR_SDMMC2EN;

    /* Configure eMMC GPIO pins for AF12 */
    /* CLK: PC12 */
    GPIOC->MODER &= ~(3UL << 24);
    GPIOC->MODER |= (GPIO_MODE_AF << 24);
    GPIOC->AFR[1] &= ~(0xFUL << 16);
    GPIOC->AFR[1] |= (EMMC_CLK_AF << 16);
    GPIOC->OSPEEDR |= (GPIO_SPEED_VERYHIGH << 24);

    /* CMD: PD2 */
    GPIOD->MODER &= ~(3UL << 4);
    GPIOD->MODER |= (GPIO_MODE_AF << 4);
    GPIOD->AFR[0] &= ~(0xFUL << 8);
    GPIOD->AFR[0] |= (EMMC_CMD_AF << 8);
    GPIOD->OSPEEDR |= (GPIO_SPEED_VERYHIGH << 4);

    /* DAT0-DAT3: PC8-PC11 */
    for (int i = 0; i < 4; i++) {
        uint32_t pin = 8 + i;
        GPIOC->MODER &= ~(3UL << (pin * 2));
        GPIOC->MODER |= (GPIO_MODE_AF << (pin * 2));
        GPIOC->AFR[1] &= ~(0xFUL << ((pin - 8) * 4));
        GPIOC->AFR[1] |= (EMMC_DAT0_AF << ((pin - 8) * 4));
        GPIOC->OSPEEDR |= (GPIO_SPEED_VERYHIGH << (pin * 2));
    }

    /* DAT4: PD3 */
    GPIOD->MODER &= ~(3UL << 6);
    GPIOD->MODER |= (GPIO_MODE_AF << 6);
    GPIOD->AFR[0] &= ~(0xFUL << 12);
    GPIOD->AFR[0] |= (EMMC_DAT4_AF << 12);
    GPIOD->OSPEEDR |= (GPIO_SPEED_VERYHIGH << 6);

    /* DAT5-DAT7: PC6, PC7, PD4 */
    GPIOC->MODER &= ~((3UL << 12) | (3UL << 14));
    GPIOC->MODER |= (GPIO_MODE_AF << 12) | (GPIO_MODE_AF << 14);
    GPIOC->AFR[0] &= ~((0xFUL << 24) | (0xFUL << 28));
    GPIOC->AFR[0] |= (EMMC_DAT5_AF << 24) | (EMMC_DAT6_AF << 28);
    GPIOC->OSPEEDR |= (GPIO_SPEED_VERYHIGH << 12) | (GPIO_SPEED_VERYHIGH << 14);

    GPIOD->MODER &= ~(3UL << 8);
    GPIOD->MODER |= (GPIO_MODE_AF << 8);
    GPIOD->AFR[0] &= ~(0xFUL << 16);
    GPIOD->AFR[0] |= (EMMC_DAT7_AF << 16);
    GPIOD->OSPEEDR |= (GPIO_SPEED_VERYHIGH << 8);

    /* Power on SDMMC2 */
    SDMMC2->POWER = 0x03;  /* 1.8V-3.3V capable */

    /* Set initial clock to 400 kHz for identification */
    emmc_set_clock(400000);

    /* Enable interrupts */
    SDMMC2->MASK = SDMMC_STA_CMDREND | SDMMC_STA_CMDSENT |
                   SDMMC_STA_DATAEND | SDMMC_STA_DCRCFAIL |
                   SDMMC_STA_CTIMEOUT | SDMMC_STA_DTIMEOUT |
                   SDMMC_STA_RXOVERR | SDMMC_STA_TXUNDERR;

    NVIC->ISER[SDMMC2_IRQ_NUM / 32] = BIT(SDMMC2_IRQ_NUM % 32);

    g_initialized = true;
    return EMMC_OK;
}

/*===========================================================================
 * Clock & Bus Configuration
 *===========================================================================*/

uint32_t emmc_set_clock(uint32_t freq_hz) {
    /* SDMMC2 clock = HCLK / (2 * (CLKDIV + 1)) */
    /* HCLK = 240 MHz, so CLKDIV = (240MHz / (2 * freq)) - 1 */
    uint32_t clkdiv = (HCLK_FREQ_HZ / (2 * freq_hz)) - 1;
    if (clkdiv > 0x3FF) clkdiv = 0x3FF;

    /* Disable clock while changing */
    SDMMC2->CLKCR &= ~SDMMC_CLKCR_CLKEN;

    SDMMC2->CLKCR = (SDMMC2->CLKCR & ~0x3FF) | clkdiv;

    /* Enable clock */
    SDMMC2->CLKCR |= SDMMC_CLKCR_CLKEN;

    g_current_clock = HCLK_FREQ_HZ / (2 * (clkdiv + 1));
    return g_current_clock;
}

void emmc_set_bus_width(uint8_t width) {
    uint32_t widbus;
    switch (width) {
    case 1: widbus = SDMMC_CLKCR_WIDBUS_1; break;
    case 4: widbus = SDMMC_CLKCR_WIDBUS_4; break;
    case 8: widbus = SDMMC_CLKCR_WIDBUS_8; break;
    default: return;
    }
    SDMMC2->CLKCR = (SDMMC2->CLKCR & ~(3UL << 14)) | widbus;
    g_current_bus_width = width;
}

void emmc_set_ddr_mode(bool enable) {
    if (enable) {
        SDMMC2->CLKCR |= SDMMC_CLKCR_DDR;
    } else {
        SDMMC2->CLKCR &= ~SDMMC_CLKCR_DDR;
    }
    g_ddr_mode = enable;
}

void emmc_set_hw_flow_control(bool enable) {
    if (enable) {
        SDMMC2->CLKCR |= SDMMC_CLKCR_HWFC_EN;
    } else {
        SDMMC2->CLKCR &= ~SDMMC_CLKCR_HWFC_EN;
    }
    g_hw_flow = enable;
}

/*===========================================================================
 * Card Detection & Initialization
 *===========================================================================*/

int emmc_detect(emmc_device_info_t *device_info) {
    int ret;

    if (!g_initialized) {
        ret = emmc_init();
        if (ret != EMMC_OK) return ret;
    }

    /* Reset device info */
    memset(device_info, 0, sizeof(emmc_device_info_t));

    /* CMD0: GO_IDLE_STATE — reset to idle */
    ret = sdmmc2_send_cmd(EMMC_CMD0_GO_IDLE_STATE, 0, EMMC_RESP_NONE);
    if (ret < 0) return EMMC_ERR_NO_CARD;

    /* Small delay after CMD0 */
    for (volatile int i = 0; i < 10000; i++);

    /* CMD1: SEND_OP_COND — poll until not busy */
    uint32_t ocr = OCR_VDD_270_360 | OCR_ACCESS_MODE_SECTOR;
    uint32_t timeout = 1000;  /* 1 second */
    do {
        ret = sdmmc2_send_cmd(EMMC_CMD1_SEND_OP_COND, ocr, EMMC_RESP_R3);
        if (ret < 0 && ret != -2) return EMMC_ERR_NO_CARD;
        if (ret == 0) {
            ocr = SDMMC2->RESP1;
            if (ocr & OCR_CARD_POWER_UP) break;
        }
        for (volatile int i = 0; i < 10000; i++);
    } while (timeout--);

    if (timeout == 0) return EMMC_ERR_TIMEOUT;

    /* Check if high-capacity card */
    bool is_high_capacity = (ocr & OCR_CARD_CAPACITY) != 0;

    /* CMD2: ALL_SEND_CID */
    ret = sdmmc2_send_cmd(EMMC_CMD2_ALL_SEND_CID, 0, EMMC_RESP_R2);
    if (ret < 0) return EMMC_ERR_INIT;
    device_info->cid[0] = SDMMC2->RESP1;
    device_info->cid[1] = SDMMC2->RESP2;
    device_info->cid[2] = SDMMC2->RESP3;
    device_info->cid[3] = SDMMC2->RESP4;

    /* CMD3: SET_RELATIVE_ADDR — assign RCA 0x0001 */
    ret = sdmmc2_send_cmd(EMMC_CMD3_SET_RELATIVE_ADDR, 0x00010000, EMMC_RESP_R1);
    if (ret < 0) return EMMC_ERR_INIT;
    device_info->rca = 0x0001;

    /* CMD9: SEND_CSD */
    ret = sdmmc2_send_cmd(EMMC_CMD9_SEND_CSD, 0x00010000, EMMC_RESP_R2);
    if (ret < 0) return EMMC_ERR_INIT;
    device_info->csd[0] = SDMMC2->RESP1;
    device_info->csd[1] = SDMMC2->RESP2;
    device_info->csd[2] = SDMMC2->RESP3;
    device_info->csd[3] = SDMMC2->RESP4;

    /* CMD7: SELECT_CARD */
    ret = sdmmc2_send_cmd(EMMC_CMD7_SELECT_DESELECT, 0x00010000, EMMC_RESP_R1b);
    if (ret < 0) return EMMC_ERR_INIT;

    /* Wait for card to enter transfer state */
    ret = emmc_wait_ready(500);
    if (ret < 0) return EMMC_ERR_BUSY;

    /* CMD8: SEND_EXT_CSD */
    ret = emmc_read_ext_csd(device_info->ext_csd);
    if (ret < 0) return EMMC_ERR_INIT;

    /* Parse EXT_CSD */
    emmc_parse_ext_csd(device_info->ext_csd, device_info);

    /* Check if HS400 is supported */
    if (device_info->hs400_supported) {
        /* Switch to HS400 */
        ret = emmc_switch_to_hs400();
        if (ret == EMMC_OK) {
            device_info->hs_timing = HS_TIMING_HS400;
            device_info->bus_width = BUS_WIDTH_8BIT_DDR;
        }
    } else if (device_info->cmd_set_rev >= 5) {
        /* At least switch to high-speed */
        ret = emmc_switch(EXT_CSD_HS_TIMING, HS_TIMING_HIGH_SPEED);
        if (ret == EMMC_OK) {
            emmc_set_clock(52000000);
            device_info->hs_timing = HS_TIMING_HIGH_SPEED;
        }
    }

    /* Compute capacity */
    device_info->capacity_blocks = emmc_compute_capacity(device_info);
    device_info->capacity_bytes = device_info->capacity_blocks * 512ULL;

    g_card_present = true;
    return EMMC_OK;
}

/*===========================================================================
 * HS400 Mode Switch Sequence
 *===========================================================================*/

static int emmc_switch_to_hs400(void) {
    int ret;

    /* Step 1: Switch to High Speed timing */
    ret = emmc_switch(EXT_CSD_HS_TIMING, HS_TIMING_HIGH_SPEED);
    if (ret < 0) return EMMC_ERR_HS400;

    /* Step 2: Increase clock to 52 MHz */
    emmc_set_clock(52000000);

    /* Step 3: Switch to 8-bit SDR bus */
    ret = emmc_switch(EXT_CSD_BUS_WIDTH, BUS_WIDTH_8BIT);
    if (ret < 0) return EMMC_ERR_HS400;
    emmc_set_bus_width(8);

    /* Step 4: Switch to HS400 timing */
    ret = emmc_switch(EXT_CSD_HS_TIMING, HS_TIMING_HS400);
    if (ret < 0) return EMMC_ERR_HS400;

    /* Step 5: Switch to 8-bit DDR bus */
    ret = emmc_switch(EXT_CSD_BUS_WIDTH, BUS_WIDTH_8BIT_DDR);
    if (ret < 0) return EMMC_ERR_HS400;

    /* Step 6: Enable DDR mode and HW flow control */
    emmc_set_ddr_mode(true);
    emmc_set_hw_flow_control(true);

    /* Step 7: Increase clock to 200 MHz */
    emmc_set_clock(200000000);

    return EMMC_OK;
}

/*===========================================================================
 * EXT_CSD Operations
 *===========================================================================*/

int emmc_read_ext_csd(uint8_t *buffer) {
    int ret;

    /* Configure for 512-byte read */
    sdmmc2_config_data(512, true, 512);

    /* Send CMD8 */
    ret = sdmmc2_send_cmd(EMMC_CMD8_SEND_EXT_CSD, 0, EMMC_RESP_R1);
    if (ret < 0) return ret;

    /* Wait for data */
    ret = sdmmc2_wait_status(SDMMC_STA_DATAEND | SDMMC_STA_DCRCFAIL |
                              SDMMC_STA_DTIMEOUT, 100);
    if (ret < 0) return ret;

    /* Read FIFO */
    sdmmc2_read_fifo((uint32_t *)buffer, 128);  /* 512 bytes = 128 words */

    return EMMC_OK;
}

int emmc_switch(uint8_t index, uint8_t value) {
    uint32_t arg = (3UL << 24) |     /* Access: write byte */
                   (index << 16) |
                   (value << 8) |
                   0x01;              /* Mode 1 */

    int ret = sdmmc2_send_cmd(EMMC_CMD6_SWITCH, arg, EMMC_RESP_R1b);
    if (ret < 0) return EMMC_ERR_SWITCH;

    /* Wait for busy to clear */
    ret = emmc_wait_ready(100);
    if (ret < 0) return EMMC_ERR_SWITCH;

    /* Verify switch status */
    uint32_t status = SDMMC2->RESP1;
    if ((status & 0xFF) != 0x01) return EMMC_ERR_SWITCH;

    return EMMC_OK;
}

int emmc_switch_partition(uint8_t partition) {
    /* Read current PARTITION_CONFIG */
    uint8_t part_config = g_emmc_info.ext_csd[EXT_CSD_PARTITION_CONFIG];
    part_config = (part_config & 0xF8) | (partition & 0x07);

    return emmc_switch(EXT_CSD_PARTITION_CONFIG, part_config);
}

/*===========================================================================
 * Block Read Operations
 *===========================================================================*/

int emmc_read_single_block(uint32_t block_addr, uint8_t *buffer) {
    int ret;

    /* Set block size */
    sdmmc2_config_data(512, true, 512);

    /* Send CMD17 */
    ret = sdmmc2_send_cmd(EMMC_CMD17_READ_SINGLE_BLOCK, block_addr, EMMC_RESP_R1);
    if (ret < 0) return ret;

    /* Wait for data */
    ret = sdmmc2_wait_status(SDMMC_STA_DATAEND | SDMMC_STA_DCRCFAIL |
                              SDMMC_STA_DTIMEOUT, 100);
    if (ret < 0) return ret;

    /* Read FIFO */
    sdmmc2_read_fifo((uint32_t *)buffer, 128);

    return EMMC_OK;
}

int emmc_acquire_start(emmc_acq_state_t *acq, uint32_t start_block,
                       uint32_t total_blocks) {
    if (!g_card_present) return EMMC_ERR_NO_CARD;
    if (total_blocks == 0) return EMMC_ERR_PARAM;

    /* Initialize acquisition state */
    acq->start_block = start_block;
    acq->total_blocks = total_blocks;
    acq->blocks_read = 0;
    acq->blocks_remaining = total_blocks;
    acq->block_size = 512;
    acq->dma_blocks = 128;  /* 64 KB per DMA transfer */
    acq->sdram_offset = 0;
    acq->errors = 0;
    acq->retries = 0;
    acq->dma_complete = false;
    acq->acquisition_active = true;
    acq->abort_requested = false;

    /* Set block count via CMD23 */
    uint32_t first_chunk = (total_blocks > acq->dma_blocks) ?
                           acq->dma_blocks : total_blocks;
    int ret = sdmmc2_send_cmd(EMMC_CMD23_SET_BLOCK_COUNT, first_chunk,
                              EMMC_RESP_R1);
    if (ret < 0) return ret;

    /* Configure data transfer */
    sdmmc2_config_data(first_chunk * 512, true, 512);

    /* Configure MDMA Channel 0: SDMMC2 FIFO → SDRAM */
    MDMA_Channel0->CSAR = (uint32_t)&SDMMC2->FIFO;
    MDMA_Channel0->CDAR = SDRAM_BASE_ADDR + acq->sdram_offset;
    MDMA_Channel0->CBNDTR = first_chunk * 512;
    MDMA_Channel0->CTCR = MDMA_CTCR_SINC_0 | MDMA_CTCR_DINC_0 |
                          MDMA_CTCR_SWAP_0 |
                          (0x0C << MDMA_CTCR_TRG_Pos);  /* Trigger: SDMMC2 TX */
    MDMA_Channel0->CCR = MDMA_CCR_EN | MDMA_CCR_TCIE |
                         (3 << MDMA_CCR_PRIO_Pos) |
                         (3 << MDMA_CCR_BURST_Pos) |
                         MDMA_CCR_DIR_PER2MEM;

    /* Enable SDMMC2 DMA */
    SDMMC2->DCTRL |= (3UL << 2);  /* DMA mode */

    /* Send CMD18 (READ_MULTIPLE_BLOCK) */
    ret = sdmmc2_send_cmd(EMMC_CMD18_READ_MULTIPLE_BLOCK, start_block,
                          EMMC_RESP_R1);
    if (ret < 0) {
        MDMA_Channel0->CCR &= ~MDMA_CCR_EN;
        acq->acquisition_active = false;
        return ret;
    }

    return EMMC_OK;
}

int emmc_acquire_continue(emmc_acq_state_t *acq) {
    if (!acq->acquisition_active) return EMMC_ERR_ABORTED;
    if (acq->abort_requested) {
        emmc_acquire_abort(acq);
        return EMMC_ERR_ABORTED;
    }

    /* Update counters */
    uint32_t blocks_this_chunk = (acq->blocks_remaining > acq->dma_blocks) ?
                                  acq->dma_blocks : acq->blocks_remaining;
    acq->blocks_read += blocks_this_chunk;
    acq->blocks_remaining -= blocks_this_chunk;
    acq->sdram_offset += blocks_this_chunk * 512;

    /* Wrap ring buffer if needed */
    if (acq->sdram_offset >= RING_BUFFER_SIZE) {
        acq->sdram_offset = 0;
    }

    if (acq->blocks_remaining == 0) {
        /* Acquisition complete — send CMD12 */
        sdmmc2_send_cmd(EMMC_CMD12_STOP_TRANSMISSION, 0, EMMC_RESP_R1b);
        MDMA_Channel0->CCR &= ~MDMA_CCR_EN;
        acq->acquisition_active = false;
        return 1;  /* Done */
    }

    /* Set next block count */
    uint32_t next_chunk = (acq->blocks_remaining > acq->dma_blocks) ?
                           acq->dma_blocks : acq->blocks_remaining;
    int ret = sdmmc2_send_cmd(EMMC_CMD23_SET_BLOCK_COUNT, next_chunk,
                              EMMC_RESP_R1);
    if (ret < 0) {
        acq->errors++;
        if (acq->retries++ < 3) {
            return emmc_acquire_continue(acq);  /* Retry */
        }
        emmc_acquire_abort(acq);
        return EMMC_ERR_DMA;
    }
    acq->retries = 0;

    /* Reconfigure DMA */
    MDMA_Channel0->CDAR = SDRAM_BASE_ADDR + acq->sdram_offset;
    MDMA_Channel0->CBNDTR = next_chunk * 512;
    MDMA_Channel0->CCR |= MDMA_CCR_EN;

    /* Reconfigure data transfer */
    sdmmc2_config_data(next_chunk * 512, true, 512);
    SDMMC2->DCTRL |= (3UL << 2);

    /* Continue CMD18 */
    ret = sdmmc2_send_cmd(EMMC_CMD18_READ_MULTIPLE_BLOCK,
                          acq->start_block + acq->blocks_read,
                          EMMC_RESP_R1);
    if (ret < 0) {
        acq->errors++;
        MDMA_Channel0->CCR &= ~MDMA_CCR_EN;
        return ret;
    }

    return 0;  /* More blocks remain */
}

int emmc_acquire_abort(emmc_acq_state_t *acq) {
    acq->abort_requested = true;
    acq->acquisition_active = false;

    /* Stop DMA */
    MDMA_Channel0->CCR &= ~MDMA_CCR_EN;

    /* Send CMD12 */
    sdmmc2_send_cmd(EMMC_CMD12_STOP_TRANSMISSION, 0, EMMC_RESP_R1b);

    return EMMC_OK;
}

/*===========================================================================
 * Status & Utility
 *===========================================================================*/

uint32_t emmc_get_status(void) {
    sdmmc2_send_cmd(EMMC_CMD13_SEND_STATUS, 0x00010000, EMMC_RESP_R1);
    return SDMMC2->RESP1;
}

int emmc_wait_ready(uint32_t timeout_ms) {
    uint32_t start = SYSTICK->CVR;
    while (1) {
        uint32_t status = emmc_get_status();
        uint8_t state = (status >> 9) & 0x0F;
        if (state == EMMC_STATE_TRAN) return EMMC_OK;
        if (state == EMMC_STATE_DATA) return EMMC_OK;

        uint32_t elapsed = (start - SYSTICK->CVR) / (SYSTICK->RVR / 1000);
        if (elapsed > timeout_ms) return EMMC_ERR_TIMEOUT;
    }
}

/*===========================================================================
 * EXT_CSD Parsing
 *===========================================================================*/

void emmc_parse_ext_csd(const uint8_t *ext_csd, emmc_device_info_t *info) {
    info->cmd_set_rev = ext_csd[EXT_CSD_CMD_SET_REV];
    info->rev = ext_csd[EXT_CSD_REV];
    info->card_type = ext_csd[EXT_CSD_CARD_TYPE];
    info->pre_eol = ext_csd[EXT_CSD_PRE_EOL_INFO];
    info->life_est_a = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_A];
    info->life_est_b = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_B];

    /* Check HS400 support */
    info->hs400_supported = (info->card_type & 0x04) != 0;
    info->enhanced_strobe = (info->card_type & 0x08) != 0;

    /* Boot partition sizes */
    uint32_t boot_mult = ext_csd[EXT_CSD_BOOT_SIZE_MULT];
    info->boot0_size = boot_mult * 128;  /* 128 KB units */
    info->boot1_size = boot_mult * 128;

    /* RPMB size */
    uint32_t rpmb_mult = ext_csd[EXT_CSD_RPMB_SIZE_MULT];
    info->rpmb_size = rpmb_mult * 128;

    /* Erase group size */
    uint32_t erase_mult = (info->csd[2] >> 16) & 0x1F;
    uint32_t erase_size = (info->csd[2] >> 8) & 0x1F;
    info->erase_group_size = (erase_mult + 1) * (erase_size + 1);
}

uint32_t emmc_compute_capacity(const emmc_device_info_t *info) {
    /* For high-capacity cards (>= 2GB), use EXT_CSD SEC_COUNT */
    if (info->ext_csd[EXT_CSD_SEC_COUNT] != 0 ||
        info->ext_csd[EXT_CSD_SEC_COUNT + 1] != 0 ||
        info->ext_csd[EXT_CSD_SEC_COUNT + 2] != 0 ||
        info->ext_csd[EXT_CSD_SEC_COUNT + 3] != 0) {
        return info->ext_csd[EXT_CSD_SEC_COUNT] |
               (info->ext_csd[EXT_CSD_SEC_COUNT + 1] << 8) |
               (info->ext_csd[EXT_CSD_SEC_COUNT + 2] << 16) |
               (info->ext_csd[EXT_CSD_SEC_COUNT + 3] << 24);
    }

    /* Fallback: compute from CSD for standard-capacity cards */
    uint32_t c_size = ((info->csd[1] >> 16) & 0x03) |
                      ((info->csd[2] << 2) & 0xFFC);
    uint32_t mult = (info->csd[2] >> 8) & 0x07;
    uint32_t block_len = (info->csd[2] >> 16) & 0x0F;

    return (c_size + 1) * (1UL << (mult + 2)) * (1UL << block_len) / 512;
}

/*===========================================================================
 * Error Strings
 *===========================================================================*/

const char *emmc_error_string(int error_code) {
    switch (error_code) {
    case EMMC_OK:              return "OK";
    case EMMC_ERR_NO_CARD:     return "No eMMC card detected";
    case EMMC_ERR_TIMEOUT:     return "Operation timed out";
    case EMMC_ERR_CRC:         return "CRC error";
    case EMMC_ERR_INIT:        return "Initialization failed";
    case EMMC_ERR_HS400:       return "HS400 mode switch failed";
    case EMMC_ERR_SWITCH:      return "SWITCH command failed";
    case EMMC_ERR_BUSY:        return "Card busy timeout";
    case EMMC_ERR_PARAM:       return "Invalid parameter";
    case EMMC_ERR_DMA:         return "DMA transfer error";
    case EMMC_ERR_ABORTED:     return "Acquisition aborted";
    case EMMC_ERR_VOLTAGE:     return "Voltage range not supported";
    case EMMC_ERR_UNSUPPORTED: return "Feature not supported by card";
    default:                   return "Unknown error";
    }
}

/*===========================================================================
 * SDMMC2 Interrupt Handler
 *===========================================================================*/

#define SDMMC2_IRQ_NUM 65

void SDMMC2_IRQHandler(void) {
    uint32_t status = SDMMC2->STA;

    if (status & SDMMC_STA_DATAEND) {
        SDMMC2->ICR = SDMMC_STA_DATAEND;
        /* Data transfer complete — handled by MDMA ISR */
    }

    if (status & SDMMC_STA_DCRCFAIL) {
        SDMMC2->ICR = SDMMC_STA_DCRCFAIL;
        /* CRC error — will be handled by retry logic */
    }

    if (status & SDMMC_STA_DTIMEOUT) {
        SDMMC2->ICR = SDMMC_STA_DTIMEOUT;
    }

    if (status & SDMMC_STA_CTIMEOUT) {
        SDMMC2->ICR = SDMMC_STA_CTIMEOUT;
    }

    if (status & SDMMC_STA_RXOVERR) {
        SDMMC2->ICR = SDMMC_STA_RXOVERR;
    }
}
