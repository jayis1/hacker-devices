/**
 * fpga.c — iCE40UP5K FPGA Interface Driver Implementation
 *
 * Full driver for Lattice iCE40UP5K FPGA used as NAND flash timing
 * controller and SPI passthrough. Handles bitstream loading via SPI
 * slave configuration, runtime register access, NAND command/address
 * sequencing, and FIFO-based data capture.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#include "fpga.h"
#include "../board.h"
#include "../registers.h"
#include <stddef.h>

/*===========================================================================
 * Static State
 *===========================================================================*/

static bool g_fpga_loaded = false;
static bool g_fpga_ready = false;
static nand_device_info_t g_nand_info;

/*===========================================================================
 * Internal Helpers — SPI1 Communication
 *===========================================================================*/

/* Wait for SPI TX buffer empty */
static void spi1_wait_tx(void) {
    while (!(SPI1->SR & BIT(1)));  /* TXE */
}

/* Wait for SPI RX buffer not empty */
static void spi1_wait_rx(void) {
    while (!(SPI1->SR & BIT(0)));  /* RXNE */
}

/* Wait for SPI not busy */
static void spi1_wait_not_busy(void) {
    while (SPI1->SR & BIT(7));  /* BSY */
}

/* Send a byte via SPI1 and return received byte */
static uint8_t spi1_transfer(uint8_t data) {
    *(volatile uint8_t *)&SPI1->DR = data;
    spi1_wait_rx();
    return *(volatile uint8_t *)&SPI1->DR;
}

/* Assert FPGA SPI chip select (active low) */
static void fpga_cs_low(void) {
    GPIOA->BSRR = BIT(FPGA_SPI_CS_PIN + 16);  /* Reset = low */
}

/* Deassert FPGA SPI chip select */
static void fpga_cs_high(void) {
    GPIOA->BSRR = BIT(FPGA_SPI_CS_PIN);  /* Set = high */
}

/*===========================================================================
 * GPIO & SPI Initialization
 *===========================================================================*/

int fpga_init(void) {
    /* Enable SPI1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure SPI1 GPIO pins (PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI) */
    /* PA4: CS — output, push-pull */
    GPIOA->MODER &= ~(3UL << 8);
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << 8);
    GPIOA->OSPEEDR |= (GPIO_SPEED_HIGH << 8);
    fpga_cs_high();

    /* PA5: SCK — AF5 */
    GPIOA->MODER &= ~(3UL << 10);
    GPIOA->MODER |= (GPIO_MODE_AF << 10);
    GPIOA->AFR[0] &= ~(0xFUL << 20);
    GPIOA->AFR[0] |= (FPGA_SPI_SCK_AF << 20);
    GPIOA->OSPEEDR |= (GPIO_SPEED_HIGH << 10);

    /* PA6: MISO — AF5 */
    GPIOA->MODER &= ~(3UL << 12);
    GPIOA->MODER |= (GPIO_MODE_AF << 12);
    GPIOA->AFR[0] &= ~(0xFUL << 24);
    GPIOA->AFR[0] |= (FPGA_SPI_MISO_AF << 24);
    GPIOA->OSPEEDR |= (GPIO_SPEED_HIGH << 12);

    /* PA7: MOSI — AF5 */
    GPIOA->MODER &= ~(3UL << 14);
    GPIOA->MODER |= (GPIO_MODE_AF << 14);
    GPIOA->AFR[0] &= ~(0xFUL << 28);
    GPIOA->AFR[0] |= (FPGA_SPI_MOSI_AF << 28);
    GPIOA->OSPEEDR |= (GPIO_SPEED_HIGH << 14);

    /* Configure FPGA control GPIOs */
    /* PB12: CRESET — output, push-pull, initially low (reset asserted) */
    GPIOB->MODER &= ~(3UL << 24);
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << 24);
    GPIOB->BSRR = BIT(12 + 16);  /* Assert reset (active low) */

    /* PB13: CDONE — input */
    GPIOB->MODER &= ~(3UL << 26);

    /* PG6: INTR — input, EXTI6 */
    GPIOG->MODER &= ~(3UL << 12);
    EXTI->RTSR1 |= BIT(FPGA_INTR_EXTI_LINE);  /* Rising edge trigger */
    EXTI->FTSR1 |= BIT(FPGA_INTR_EXTI_LINE);  /* Falling edge trigger */
    NVIC->ISER[NVIC_IRQ_EXTI9_5 / 32] = BIT(NVIC_IRQ_EXTI9_5 % 32);

    /* Configure SPI1: Master, 20 MHz, CPOL=0, CPHA=0, MSB first */
    /* SPI1 clock = APB2 / prescaler. APB2 = 120 MHz, target 20 MHz → /8 */
    SPI1->CR1 = (3UL << 3) |   /* BR[2:0] = 011: fPCLK/16 = 7.5 MHz */
                BIT(2) |        /* MSTR: Master */
                BIT(8) | BIT(9);/* SSI + SSM: Software slave management */
    SPI1->CR2 = (7UL << 8);    /* DS[3:0] = 0111: 8-bit data */
    SPI1->CR1 |= BIT(6);       /* SPE: Enable */

    return FPGA_OK;
}

/*===========================================================================
 * Bitstream Loading
 *===========================================================================*/

int fpga_load_bitstream(const uint8_t *bitstream, uint32_t length) {
    if (!bitstream || length == 0) return FPGA_ERR_LOAD;

    /* Assert CRESET (active low) — hold for 1ms */
    GPIOB->BSRR = BIT(FPGA_CRESET_PIN + 16);
    for (volatile int i = 0; i < 240000; i++);  /* ~1ms @ 240MHz */

    /* Release CRESET — FPGA enters SPI slave config mode */
    GPIOB->BSRR = BIT(FPGA_CRESET_PIN);
    for (volatile int i = 0; i < 48000; i++);   /* ~200µs */

    /* Send bitstream via SPI */
    fpga_cs_low();

    /* Send dummy byte to clear any stale state */
    spi1_transfer(0xFF);

    /* Stream bitstream data */
    for (uint32_t i = 0; i < length; i++) {
        spi1_transfer(bitstream[i]);
    }

    /* Send 100+ dummy clocks for FPGA to finish configuration */
    for (int i = 0; i < 100; i++) {
        spi1_transfer(0xFF);
    }

    fpga_cs_high();

    /* Wait for CDONE to go high */
    uint32_t timeout = 1000000;
    while (!(GPIOB->IDR & BIT(FPGA_CDONE_PIN)) && timeout--);
    if (timeout == 0) return FPGA_ERR_TIMEOUT;

    /* Verify FPGA ID */
    uint32_t id = fpga_read_reg(FPGA_REG_ID);
    if (id != FPGA_EXPECTED_ID) return FPGA_ERR_ID;

    g_fpga_loaded = true;
    g_fpga_ready = true;
    return FPGA_OK;
}

int fpga_verify(void) {
    if (!g_fpga_loaded) return FPGA_ERR_NOT_READY;
    uint32_t id = fpga_read_reg(FPGA_REG_ID);
    return (id == FPGA_EXPECTED_ID) ? FPGA_OK : FPGA_ERR_ID;
}

void fpga_reset(void) {
    GPIOB->BSRR = BIT(FPGA_CRESET_PIN + 16);
    for (volatile int i = 0; i < 240000; i++);
    GPIOB->BSRR = BIT(FPGA_CRESET_PIN);
    g_fpga_loaded = false;
    g_fpga_ready = false;
}

/*===========================================================================
 * Register Access
 *===========================================================================*/

uint32_t fpga_read_reg(uint8_t addr) {
    uint32_t value = 0;

    fpga_cs_low();

    /* Send read command: bit 7 = 0 (read), bits 6:0 = address */
    spi1_transfer(addr & 0x7F);

    /* Read 4 bytes, MSB first */
    value = ((uint32_t)spi1_transfer(0xFF) << 24) |
            ((uint32_t)spi1_transfer(0xFF) << 16) |
            ((uint32_t)spi1_transfer(0xFF) << 8) |
            ((uint32_t)spi1_transfer(0xFF));

    fpga_cs_high();
    return value;
}

void fpga_write_reg(uint8_t addr, uint32_t value) {
    fpga_cs_low();

    /* Send write command: bit 7 = 1 (write), bits 6:0 = address */
    spi1_transfer(addr | 0x80);

    /* Write 4 bytes, MSB first */
    spi1_transfer((value >> 24) & 0xFF);
    spi1_transfer((value >> 16) & 0xFF);
    spi1_transfer((value >> 8) & 0xFF);
    spi1_transfer(value & 0xFF);

    fpga_cs_high();
}

/*===========================================================================
 * NAND Timing Configuration
 *===========================================================================*/

void fpga_config_nand_timing(const nand_timing_t *timing) {
    /* Convert ns to FPGA clock cycles (48 MHz = ~20.83 ns per cycle) */
    /* Each timing register holds 4 × 8-bit values */

    uint32_t tim0 = ((timing->tWC_ns / 21) & 0xFF) |
                    (((timing->tWP_ns / 21) & 0xFF) << 8) |
                    (((timing->tWH_ns / 21) & 0xFF) << 16) |
                    (((timing->tRC_ns / 21) & 0xFF) << 24);

    uint32_t tim1 = ((timing->tRP_ns / 21) & 0xFF) |
                    (((timing->tREH_ns / 21) & 0xFF) << 8) |
                    (((timing->tWB_ns / 21) & 0xFF) << 16) |
                    (((timing->tADL_ns / 21) & 0xFF) << 24);

    uint32_t tim2 = ((timing->tWHR_ns / 21) & 0xFF) |
                    (((timing->tR_max_us * 48) & 0xFF) << 8) |
                    (((timing->tBERS_max_ms * 48000) & 0xFFFF) << 16);

    fpga_write_reg(FPGA_REG_NAND_TIMING_0, tim0);
    fpga_write_reg(FPGA_REG_NAND_TIMING_1, tim1);
    fpga_write_reg(FPGA_REG_NAND_TIMING_2, tim2);
}

/*===========================================================================
 * NAND Command Interface
 *===========================================================================*/

void fpga_nand_send_cmd(uint8_t cmd) {
    fpga_write_reg(FPGA_REG_NAND_CMD, cmd);
    /* Wait for command done */
    while (!(fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_READY));
}

void fpga_nand_send_addr(const uint8_t *addr, uint8_t cycles) {
    uint32_t addr_word = addr[0] | (addr[1] << 8) |
                         (addr[2] << 16) | (addr[3] << 24);
    fpga_write_reg(FPGA_REG_NAND_ADDR, addr_word | ((uint32_t)cycles << 28));
    while (!(fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_READY));
}

void fpga_nand_read_fifo(uint32_t *buffer, uint32_t words) {
    for (uint32_t i = 0; i < words; i++) {
        buffer[i] = fpga_read_reg(FPGA_REG_FIFO_DATA);
    }
}

uint32_t fpga_nand_fifo_count(void) {
    return fpga_read_reg(FPGA_REG_FIFO_COUNT);
}

void fpga_nand_fifo_reset(void) {
    uint32_t ctrl = fpga_read_reg(FPGA_REG_CTRL);
    fpga_write_reg(FPGA_REG_CTRL, ctrl | FPGA_CTRL_FIFO_RESET);
    for (volatile int i = 0; i < 100; i++);
    fpga_write_reg(FPGA_REG_CTRL, ctrl & ~FPGA_CTRL_FIFO_RESET);
}

void fpga_nand_enable(bool enable) {
    uint32_t ctrl = fpga_read_reg(FPGA_REG_CTRL);
    if (enable) {
        ctrl |= FPGA_CTRL_NAND_ENABLE | FPGA_CTRL_ECC_BYPASS;
    } else {
        ctrl &= ~FPGA_CTRL_NAND_ENABLE;
    }
    fpga_write_reg(FPGA_REG_CTRL, ctrl);
}

void fpga_spi_passthrough(bool enable) {
    uint32_t ctrl = fpga_read_reg(FPGA_REG_CTRL);
    if (enable) {
        ctrl |= FPGA_CTRL_SPI_PASSTHRU;
    } else {
        ctrl &= ~FPGA_CTRL_SPI_PASSTHRU;
    }
    fpga_write_reg(FPGA_REG_CTRL, ctrl);
}

void fpga_set_intr_mask(uint32_t mask) {
    fpga_write_reg(FPGA_REG_INTR_MASK, mask);
}

uint32_t fpga_get_intr_status(void) {
    uint32_t status = fpga_read_reg(FPGA_REG_INTR_STATUS);
    /* Write-1-to-clear */
    fpga_write_reg(FPGA_REG_INTR_STATUS, status);
    return status;
}

/*===========================================================================
 * NAND Detection
 *===========================================================================*/

int fpga_nand_detect(nand_device_info_t *info) {
    if (!g_fpga_ready) return FPGA_ERR_NOT_READY;

    memset(info, 0, sizeof(nand_device_info_t));

    /* Enable NAND controller */
    fpga_nand_enable(true);
    fpga_nand_fifo_reset();

    /* Send RESET command */
    fpga_nand_send_cmd(NAND_CMD_RESET);
    for (volatile int i = 0; i < 240000; i++);  /* Wait for reset */

    /* Send READ ID command */
    fpga_nand_send_cmd(NAND_CMD_READ_ID);
    uint8_t addr_byte = 0x00;
    fpga_nand_send_addr(&addr_byte, 1);

    /* Wait for R/B# to go high (data ready) */
    uint32_t timeout = 1000000;
    while ((fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_NAND_BUSY) && timeout--);
    if (timeout == 0) return FPGA_ERR_NO_CHIP;

    /* Read ID bytes from FIFO */
    uint32_t id_data;
    fpga_nand_read_fifo(&id_data, 1);
    info->manufacturer_id = id_data & 0xFF;
    info->device_id = (id_data >> 8) & 0xFF;

    /* Try to read ONFI parameter page */
    fpga_nand_send_cmd(NAND_CMD_READ_PARAM_PAGE);
    uint8_t param_addr = 0x00;
    fpga_nand_send_addr(&param_addr, 1);

    timeout = 1000000;
    while ((fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_NAND_BUSY) && timeout--);
    if (timeout == 0) {
        /* Non-ONFI device — use default timing */
        info->onfi_compliant = false;
        info->detected = true;
        return FPGA_OK;
    }

    /* Read parameter page (256 bytes = 64 words) */
    uint32_t param_data[64];
    fpga_nand_read_fifo(param_data, 64);

    /* Check ONFI signature */
    if ((param_data[0] & 0xFFFF) == 0x494F &&   /* "OI" */
        (param_data[0] >> 16) == 0x464E) {       /* "FN" */
        info->onfi_compliant = true;
        /* Parse ONFI parameters */
        memcpy(&info->onfi, param_data, sizeof(onfi_params_t));

        /* Extract key parameters */
        info->page_size = info->onfi.data_bytes_per_page;
        info->oob_size = info->onfi.spare_bytes_per_page;
        info->pages_per_block = info->onfi.pages_per_block;
        info->blocks_per_lun = info->onfi.blocks_per_lun;
        info->addr_cycles = info->onfi.addr_cycles;
        info->total_pages = info->pages_per_block * info->blocks_per_lun;
        info->total_blocks = info->blocks_per_lun;
        info->total_size_bytes = (uint64_t)info->page_size * info->total_pages;

        /* Configure timing from ONFI timing mode */
        info->timing.tWC_ns = 25;
        info->timing.tWP_ns = 12;
        info->timing.tWH_ns = 5;
        info->timing.tRC_ns = 25;
        info->timing.tRP_ns = 12;
        info->timing.tREH_ns = 5;
        info->timing.tWB_ns = 100;
        info->timing.tADL_ns = 70;
        info->timing.tWHR_ns = 60;
        info->timing.tR_max_us = info->onfi.tR_max;
        info->timing.tBERS_max_ms = info->onfi.tBERS_max;

        fpga_config_nand_timing(&info->timing);
    } else {
        info->onfi_compliant = false;
    }

    info->detected = true;
    return FPGA_OK;
}

/*===========================================================================
 * NAND Page Read
 *===========================================================================*/

int fpga_nand_read_page(uint32_t page_addr, uint8_t *buffer,
                        const nand_device_info_t *info) {
    if (!g_fpga_ready || !info->detected) return FPGA_ERR_NOT_READY;

    fpga_nand_fifo_reset();

    /* Build 5-byte address */
    uint8_t addr[5];
    uint32_t col_addr = 0;  /* Start at column 0 */
    addr[0] = col_addr & 0xFF;
    addr[1] = (col_addr >> 8) & 0xFF;
    addr[2] = page_addr & 0xFF;
    addr[3] = (page_addr >> 8) & 0xFF;
    addr[4] = (page_addr >> 16) & 0xFF;

    /* Send READ PAGE command sequence */
    fpga_nand_send_cmd(NAND_CMD_READ_PAGE);
    fpga_nand_send_addr(addr, info->addr_cycles);
    fpga_nand_send_cmd(NAND_CMD_READ_PAGE_CONFIRM);

    /* Wait for R/B# to go high */
    uint32_t timeout = (info->timing.tR_max_us * 240) + 100000;
    while ((fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_NAND_BUSY) && timeout--);
    if (timeout == 0) return FPGA_ERR_TIMEOUT;

    /* Read data from FIFO */
    uint32_t total_words = (info->page_size + info->oob_size + 3) / 4;
    uint32_t words_read = 0;

    while (words_read < total_words) {
        uint32_t avail = fpga_nand_fifo_count();
        if (avail == 0) {
            /* Check if more data is coming */
            if (!(fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_NAND_BUSY) &&
                fpga_nand_fifo_count() == 0) {
                break;
            }
            continue;
        }
        uint32_t chunk = (avail > (total_words - words_read)) ?
                         (total_words - words_read) : avail;
        fpga_nand_read_fifo((uint32_t *)(buffer + words_read * 4), chunk);
        words_read += chunk;
    }

    return FPGA_OK;
}

int fpga_nand_read_pages_dma(uint32_t start_page, uint32_t count,
                             uint8_t *sdram_buffer,
                             const nand_device_info_t *info) {
    /* For DMA-based NAND reads, we use the BDMA controller */
    /* Each page read is triggered, then BDMA moves FIFO data to SDRAM */

    uint32_t page_size_total = info->page_size + info->oob_size;
    uint32_t words_per_page = (page_size_total + 3) / 4;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t page = start_page + i;
        uint8_t *dest = sdram_buffer + i * page_size_total;

        /* Read page into FPGA FIFO */
        fpga_nand_fifo_reset();

        uint8_t addr[5];
        addr[0] = 0;
        addr[1] = 0;
        addr[2] = page & 0xFF;
        addr[3] = (page >> 8) & 0xFF;
        addr[4] = (page >> 16) & 0xFF;

        fpga_nand_send_cmd(NAND_CMD_READ_PAGE);
        fpga_nand_send_addr(addr, info->addr_cycles);
        fpga_nand_send_cmd(NAND_CMD_READ_PAGE_CONFIRM);

        /* Wait for R/B# */
        uint32_t timeout = (info->timing.tR_max_us * 240) + 100000;
        while ((fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_NAND_BUSY) && timeout--);
        if (timeout == 0) return FPGA_ERR_TIMEOUT;

        /* Drain FIFO to buffer */
        uint32_t words_read = 0;
        while (words_read < words_per_page) {
            uint32_t avail = fpga_nand_fifo_count();
            if (avail == 0) {
                if (!(fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_NAND_BUSY) &&
                    fpga_nand_fifo_count() == 0) break;
                continue;
            }
            uint32_t chunk = (avail > (words_per_page - words_read)) ?
                             (words_per_page - words_read) : avail;
            fpga_nand_read_fifo((uint32_t *)(dest + words_read * 4), chunk);
            words_read += chunk;
        }
    }

    return FPGA_OK;
}

/*===========================================================================
 * Error Strings
 *===========================================================================*/

const char *fpga_error_string(int error_code) {
    switch (error_code) {
    case FPGA_OK:              return "OK";
    case FPGA_ERR_LOAD:        return "FPGA bitstream load failed";
    case FPGA_ERR_ID:          return "FPGA ID mismatch";
    case FPGA_ERR_TIMEOUT:     return "FPGA operation timed out";
    case FPGA_ERR_SPI:         return "SPI communication error";
    case FPGA_ERR_NO_CHIP:     return "No NAND chip detected";
    case FPGA_ERR_ONFI:        return "ONFI parameter page invalid";
    case FPGA_ERR_READ:        return "NAND page read failed";
    case FPGA_ERR_FIFO_OVF:    return "FPGA FIFO overflow";
    case FPGA_ERR_NOT_READY:   return "FPGA not initialized";
    default:                   return "Unknown error";
    }
}

/*===========================================================================
 * FPGA Interrupt Handler (EXTI6 for PG6)
 *===========================================================================*/

void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF6) {
        EXTI->PR1 = EXTI_PR1_PIF6;  /* Clear pending */

        /* Read FPGA interrupt status */
        uint32_t fpga_status = fpga_get_intr_status();

        /* Handle R/B# rise — NAND page ready */
        if (fpga_status & FPGA_INTR_RB_RISE) {
            /* Signal to NAND acquisition state machine */
            /* (handled via global flag in main.c) */
        }

        /* Handle FIFO threshold — drain to SDRAM */
        if (fpga_status & FPGA_INTR_FIFO_THRESHOLD) {
            /* Drain FIFO in ISR context for low latency */
            uint32_t count = fpga_nand_fifo_count();
            /* Quick drain handled by main loop */
        }

        /* Handle FIFO overflow */
        if (fpga_status & FPGA_INTR_FIFO_OVERFLOW) {
            fpga_nand_fifo_reset();
        }
    }
}
