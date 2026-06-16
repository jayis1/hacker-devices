/**
 * @file ctrl_task.c
 * @author jayis1
 * @brief FPGA Control Interface Task
 *
 * Manages SPI communication with the Gowin GW1N-LV1 FPGA.
 * Handles register read/write, rule programming, frame injection,
 * and status polling from the ESP32-S3 MCU.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "ctrl";

/* SPI device handle */
static spi_device_handle_t fpga_spi;

/* ===================================================================
 * Command Types (for ctrl_cmd_queue)
 * =================================================================== */
typedef enum {
    CTRL_CMD_WRITE_REG,       /* Write a single FPGA register */
    CTRL_CMD_READ_REG,        /* Read a single FPGA register */
    CTRL_CMD_WRITE_MODE,      /* Set operation mode */
    CTRL_CMD_WRITE_RULE,      /* Program a rule slot */
    CTRL_CMD_READ_RULE,       /* Read a rule slot back */
    CTRL_CMD_INJECT_FRAME,    /* Inject a frame via FIFO */
    CTRL_CMD_CAPTURE_START,   /* Start capture */
    CTRL_CMD_CAPTURE_STOP,    /* Stop capture */
    CTRL_CMD_CAPTURE_READ,    /* Read captured data */
    CTRL_CMD_RESET_FPGA,      /* Reset FPGA */
    CTRL_CMD_POLL_STATUS,     /* Poll status registers */
    CTRL_CMD_LOAD_CONFIG      /* Load all rules from NVS */
} ctrl_cmd_type_t;

/* Control command structure */
typedef struct {
    ctrl_cmd_type_t cmd_type;
    union {
        struct {
            uint16_t addr;
            uint32_t data;
        } reg;
        struct {
            uint32_t mode_reg;
        } mode;
        struct {
            uint8_t slot;
            fpga_rule_t rule;
        } rule;
        struct {
            uint8_t *data;
            uint16_t len;
        } frame;
        struct {
            uint8_t *buffer;
            uint16_t max_len;
            uint16_t *actual_len;
        } capture;
        struct {
            uint32_t timeout_ms;
        } poll;
    };
} ctrl_cmd_t;

/* ===================================================================
 * SPI Protocol Implementation
 * =================================================================== */

/**
 * @brief Build a 16-bit SPI address with read/auto-inc flags.
 */
static inline uint16_t spi_addr_encode(uint16_t addr, bool is_read, bool auto_inc)
{
    uint16_t encoded = addr & REG_ADDR_MASK;
    if (is_read)  encoded |= REG_READ_FLAG;
    if (auto_inc) encoded |= REG_AUTO_INC_FLAG;
    return encoded;
}

/**
 * @brief Write a 32-bit value to an FPGA register over SPI.
 * @param addr  Register address (16-bit)
 * @param data  Value to write
 * @return SPI_OK on success, negative error code on failure.
 */
static int fpga_reg_write(uint16_t addr, uint32_t data)
{
    uint8_t tx_buf[6];
    uint8_t rx_buf[6] = {0};

    /* Build transaction: 16-bit address (MSB first) + 32-bit data (MSB first) */
    uint16_t encoded_addr = spi_addr_encode(addr, false, false);
    tx_buf[0] = (encoded_addr >> 8) & 0xFF;
    tx_buf[1] = encoded_addr & 0xFF;
    tx_buf[2] = (data >> 24) & 0xFF;
    tx_buf[3] = (data >> 16) & 0xFF;
    tx_buf[4] = (data >> 8) & 0xFF;
    tx_buf[5] = data & 0xFF;

    spi_transaction_t trans = {
        .length = 6 * 8,           /* 6 bytes = 48 bits */
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
        .flags = SPI_TRANS_USE_RXDATA,
    };

    esp_err_t ret = spi_device_transmit(fpga_spi, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI write failed (addr=0x%04X): %d", addr, ret);
        return SPI_ERR_TIMEOUT;
    }

    return SPI_OK;
}

/**
 * @brief Read a 32-bit value from an FPGA register over SPI.
 * @param addr  Register address (16-bit)
 * @param data  Pointer to store the read value
 * @return SPI_OK on success, negative error code on failure.
 */
static int fpga_reg_read(uint16_t addr, uint32_t *data)
{
    uint8_t tx_buf[6];
    uint8_t rx_buf[6] = {0};

    /* Build transaction with READ flag set */
    uint16_t encoded_addr = spi_addr_encode(addr, true, false);
    tx_buf[0] = (encoded_addr >> 8) & 0xFF;
    tx_buf[1] = encoded_addr & 0xFF;
    tx_buf[2] = 0; tx_buf[3] = 0; tx_buf[4] = 0; tx_buf[5] = 0;

    spi_transaction_t trans = {
        .length = 6 * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
        .flags = SPI_TRANS_USE_RXDATA,
    };

    esp_err_t ret = spi_device_transmit(fpga_spi, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI read failed (addr=0x%04X): %d", addr, ret);
        return SPI_ERR_TIMEOUT;
    }

    /* Data comes back in the last 4 bytes */
    if (data) {
        *data = ((uint32_t)rx_buf[2] << 24) |
                ((uint32_t)rx_buf[3] << 16) |
                ((uint32_t)rx_buf[4] << 8)  |
                (uint32_t)rx_buf[5];
    }

    return SPI_OK;
}

/**
 * @brief Multi-word read with auto-increment addressing.
 * @param start_addr  Starting register address
 * @param data        Output buffer (32-bit words)
 * @param count       Number of 32-bit words to read
 * @return SPI_OK on success.
 */
static int fpga_reg_read_burst(uint16_t start_addr, uint32_t *data, int count)
{
    if (!data || count <= 0) return SPI_ERR_INVALID_ADDR;

    /* Use the capture data port which has auto-increment */
    for (int i = 0; i < count; i++) {
        uint16_t addr = (i == 0) ? start_addr : (start_addr + i);
        int ret = fpga_reg_read(addr, &data[i]);
        if (ret != SPI_OK) return ret;
    }
    return SPI_OK;
}

/* ===================================================================
 * Rule Programming
 * =================================================================== */

/**
 * @brief Program a single rule slot into the FPGA.
 * @param slot  Rule slot index (0–15)
 * @param rule  Pointer to the rule structure
 * @return SPI_OK on success.
 */
static int fpga_program_rule(uint8_t slot, const fpga_rule_t *rule)
{
    if (slot >= FPGA_MAX_RULES || !rule) {
        return SPI_ERR_INVALID_ADDR;
    }

    /* Decompose the rule into the four slot registers */

    /* REG 0: LBA range start, low 32 bits */
    uint32_t lba_lo = (uint32_t)(rule->lba_start & 0xFFFFFFFF);
    int ret = fpga_reg_write(RULE_ADDR(slot, RULE_OFFS_LBA_LO), lba_lo);
    if (ret != SPI_OK) return ret;

    /* REG 1: LBA range start high + opcode/mask */
    uint32_t lba_hi = (uint32_t)((rule->lba_start >> 32) & 0xFFFF);
    uint32_t lba_hi_op = RULE_LBA_HI_OP(lba_hi, rule->opcode, rule->opcode_mask);
    ret = fpga_reg_write(RULE_ADDR(slot, RULE_OFFS_LBA_HI_OP), lba_hi_op);
    if (ret != SPI_OK) return ret;

    /* REG 2: LBA mask low + data pattern first 4 bytes */
    uint32_t mask_pat = (uint32_t)(rule->lba_mask & 0xFFFFFFFF);
    ret = fpga_reg_write(RULE_ADDR(slot, RULE_OFFS_MASK_PAT), mask_pat);
    if (ret != SPI_OK) return ret;

    /* REG 3: LBA mask high + direction + action + enable + priority */
    uint32_t mask_hi = (uint32_t)((rule->lba_mask >> 32) & 0xFFFF);
    uint32_t ctrl = RULE_CTRL(mask_hi, rule->direction, rule->action,
                              rule->enabled, rule->priority);
    ret = fpga_reg_write(RULE_ADDR(slot, RULE_OFFS_CTRL), ctrl);
    if (ret != SPI_OK) return ret;

    ESP_LOGI(TAG, "Rule slot %d programmed (LBA=0x%016llX, opcode=0x%02X, action=%d)",
             slot, (unsigned long long)rule->lba_start, rule->opcode, rule->action);
    return SPI_OK;
}

/**
 * @brief Read back a rule slot from the FPGA.
 * @param slot  Rule slot index (0–15)
 * @param rule  Pointer to store the rule values
 * @return SPI_OK on success.
 */
static int fpga_read_rule(uint8_t slot, fpga_rule_t *rule)
{
    if (slot >= FPGA_MAX_RULES || !rule) {
        return SPI_ERR_INVALID_ADDR;
    }

    memset(rule, 0, sizeof(fpga_rule_t));

    uint32_t regs[4] = {0};
    int ret = fpga_reg_read_burst(RULE_ADDR(slot, 0), regs, 4);
    if (ret != SPI_OK) return ret;

    rule->lba_start  = ((uint64_t)(regs[1] >> 16) << 32) | regs[0];
    rule->opcode     = (regs[1] >> 8) & 0xFF;
    rule->opcode_mask = regs[1] & 0xFF;
    rule->lba_mask   = ((uint64_t)(regs[3] >> 16) << 32) | regs[2];
    rule->direction  = (regs[3] >> 12) & 0x0F;
    rule->action     = (regs[3] >> 8) & 0x0F;
    rule->enabled    = (regs[3] >> 3) & 0x01;
    rule->priority   = regs[3] & 0x07;

    ESP_LOGD(TAG, "Read rule slot %d", slot);
    return SPI_OK;
}

/**
 * @brief Enable or disable rules globally via bitmask.
 * @param enable_mask  Bit N = 1 enables slot N
 */
static int fpga_set_rule_enable_mask(uint16_t enable_mask)
{
    return fpga_reg_write(REG_RULE_GLOBAL_ENABLE, (uint32_t)enable_mask);
}

/* ===================================================================
 * Frame Injection
 * =================================================================== */

/**
 * @brief Push a frame into the FPGA's injection FIFO for later merge.
 * @param data  Pointer to frame data (including SOF, excluding CRC/EOF)
 * @param len   Length of frame data in bytes
 * @return SPI_OK if fully pushed, error if FIFO full.
 */
static int fpga_inject_frame(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0 || len > FIS_SIZE_DATA_MAX) {
        return SPI_ERR_INVALID_ADDR;
    }

    /* Check FIFO has room */
    uint32_t fifo_level = 0;
    fpga_reg_read(REG_FIFO_LEVEL, &fifo_level);
    if (fifo_level + len > FIS_SIZE_DATA_MAX) {
        ESP_LOGW(TAG, "Injection FIFO full (%d bytes queued, need %d)", fifo_level, len);
        return SPI_ERR_BUSY;
    }

    /* Push data 4 bytes (1 dword) at a time */
    int dwords = (len + 3) / 4;  /* Round up to dword boundary */
    uint32_t aligned_buf[2048];  /* Max 8192 bytes / 4 */

    memcpy(aligned_buf, data, len);
    /* Zero-pad to dword boundary */
    if (len % 4) {
        memset((uint8_t*)aligned_buf + len, 0, 4 - (len % 4));
    }

    for (int i = 0; i < dwords; i++) {
        int ret = fpga_reg_write(REG_FIFO_PUSH, aligned_buf[i]);
        if (ret != SPI_OK) {
            ESP_LOGE(TAG, "FIFO push failed at dword %d/%d", i, dwords);
            return ret;
        }
    }

    ESP_LOGI(TAG, "Injected %d bytes (%d dwords) into FIFO", len, dwords);
    return SPI_OK;
}

/**
 * @brief Enable FIFO injection (merge buffered frame into stream).
 */
static int fpga_enable_injection(void)
{
    return fpga_reg_write(REG_FIFO_CTRL, FIFO_CTRL_ENABLE | FIFO_CTRL_INSERT_IMM);
}

/* ===================================================================
 * Frame Capture
 * =================================================================== */

/**
 * @brief Start capturing a SATA frame to the scratch buffer.
 * @param mode  Capture mode (CAPTURE_CTRL_SINGLE or CAPTURE_CTRL_START)
 */
static int fpga_capture_start(uint32_t mode)
{
    return fpga_reg_write(REG_CAPTURE_CTRL, mode);
}

/**
 * @brief Stop frame capture.
 */
static int fpga_capture_stop(void)
{
    return fpga_reg_write(REG_CAPTURE_CTRL, 0);
}

/**
 * @brief Read captured frame data from scratch buffer.
 * @param buffer  Output buffer (must be at least 8192 bytes)
 * @param max_len  Maximum bytes to read
 * @param actual_len  Output: actual bytes read
 * @return SPI_OK on success.
 */
static int fpga_capture_read(uint8_t *buffer, uint16_t max_len, uint16_t *actual_len)
{
    if (!buffer || !actual_len) return SPI_ERR_INVALID_ADDR;

    /* Read captured frame length */
    uint32_t cap_len = 0;
    int ret = fpga_reg_read(REG_CAPTURE_LEN, &cap_len);
    if (ret != SPI_OK) return ret;

    uint16_t len = (cap_len > max_len) ? max_len : (uint16_t)cap_len;
    *actual_len = len;

    /* Read capture metadata */
    uint32_t fis_type = 0, lba_lo = 0, lba_hi = 0, direction = 0;
    fpga_reg_read(REG_CAPTURE_FIS_TYPE, &fis_type);
    fpga_reg_read(REG_CAPTURE_LBA_LO, &lba_lo);
    fpga_reg_read(REG_CAPTURE_LBA_HI, &lba_hi);
    fpga_reg_read(REG_CAPTURE_DIRECTION, &direction);

    ESP_LOGI(TAG, "Captured frame: FIS=0x%02X, LBA=0x%08X%08X, dir=%s, len=%d",
             (unsigned)fis_type, (unsigned)lba_hi, (unsigned)lba_lo,
             direction ? "WRITE" : "READ", len);

    /* Read frame data byte-by-byte */
    for (uint16_t i = 0; i < len; i++) {
        uint32_t byte_val = 0;
        ret = fpga_reg_read(REG_CAPTURE_DATA_PORT, &byte_val);
        if (ret != SPI_OK) return ret;
        buffer[i] = (uint8_t)(byte_val & 0xFF);
    }

    return SPI_OK;
}

/* ===================================================================
 * General Status Polling
 * =================================================================== */

/**
 * @brief Poll FPGA status registers and set event group bits.
 */
static void fpga_poll_status(void)
{
    uint32_t status = 0;
    if (fpga_reg_read(REG_SYS_STATUS, &status) != SPI_OK) {
        return;
    }

    /* Check for rule match */
    if (status & SYS_STATUS_RX_ACTIVE) {
        uint32_t rule_status = 0;
        fpga_reg_read(REG_STAT_COUNTER_READ, &rule_status);
        /* Rule match handling is triggered by GPIO interrupt line */
    }

    /* Check link status */
    uint32_t link_stat = 0;
    if (fpga_reg_read(REG_STAT_LINK, &link_stat) == SPI_OK) {
        if (link_stat & LINK_STAT_ESTABLISHED) {
            xEventGroupSetBits(system_events, EVENT_BIT_LINK_UP);
        } else {
            xEventGroupClearBits(system_events, EVENT_BIT_LINK_UP);
        }
    }

    /* Check temperature */
    uint32_t temp = 0;
    if (fpga_reg_read(REG_STAT_TEMP, &temp) == SPI_OK) {
        if (temp > 850) {  /* 85°C threshold */
            ESP_LOGW(TAG, "FPGA temperature warning: %d mV (%d°C)",
                     temp, temp / 10);
        }
    }
}

/* ===================================================================
 * GPIO Interrupt Handler for FPGA IRQ
 * =================================================================== */

static void IRAM_ATTR fpga_irq_handler(void *arg)
{
    BaseType_t higher_woken = pdFALSE;
    xEventGroupSetBitsFromISR(system_events, EVENT_BIT_FRAME_MATCH, &higher_woken);
    if (higher_woken) {
        portYIELD_FROM_ISR();
    }
}

/* ===================================================================
 * Control Task Main Loop
 * =================================================================== */

void ctrl_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Control task started");

    /* Attach FPGA to SPI bus */
    spi_device_interface_config_t dev_cfg = {
        .mode           = 0,
        .clock_speed_hz = FPGA_SPI_CLOCK_HZ,
        .spics_io_num   = PIN_FPGA_SPI_CS,
        .queue_size     = 8,
        .flags          = SPI_DEVICE_HALFDUPLEX,
    };
    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &fpga_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %d", ret);
        vTaskDelete(NULL);
        return;
    }

    /* Install GPIO interrupt for FPGA IRQ line */
    gpio_config_t irq_cfg = {
        .pin_bit_mask = (1ULL << PIN_FPGA_IRQ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  /* Active low interrupt */
    };
    gpio_config(&irq_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_FPGA_IRQ, fpga_irq_handler, NULL);

    /* Initial FPGA handshake: read version */
    uint32_t version = 0;
    ret = fpga_reg_read(REG_SYS_VERSION, &version);
    if (ret == SPI_OK) {
        uint8_t major = (version >> 16) & 0xFF;
        uint8_t minor = (version >> 8) & 0xFF;
        uint8_t patch = version & 0xFF;
        ESP_LOGI(TAG, "FPGA firmware v%d.%d.%d (reg=0x%08X)", major, minor, patch, version);
        xEventGroupSetBits(system_events, EVENT_BIT_FPGA_READY);
    } else {
        ESP_LOGE(TAG, "Failed to read FPGA version: %d", ret);
    }

    ctrl_cmd_t cmd;
    while (1) {
        /* Wait for a command or poll timeout */
        if (xQueueReceive(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            switch (cmd.cmd_type) {
                case CTRL_CMD_WRITE_REG:
                    fpga_reg_write(cmd.reg.addr, cmd.reg.data);
                    break;

                case CTRL_CMD_READ_REG: {
                    uint32_t val = 0;
                    fpga_reg_read(cmd.reg.addr, &val);
                    ESP_LOGD(TAG, "Reg 0x%04X = 0x%08X", cmd.reg.addr, val);
                    break;
                }

                case CTRL_CMD_WRITE_MODE:
                    fpga_reg_write(REG_CTRL_MAIN, cmd.mode.mode_reg);
                    ESP_LOGI(TAG, "Mode register set to 0x%08X", cmd.mode.mode_reg);
                    break;

                case CTRL_CMD_WRITE_RULE:
                    fpga_program_rule(cmd.rule.slot, &cmd.rule.rule);
                    break;

                case CTRL_CMD_READ_RULE:
                    fpga_read_rule(cmd.rule.slot, &cmd.rule.rule);
                    break;

                case CTRL_CMD_INJECT_FRAME:
                    if (cmd.frame.data && cmd.frame.len > 0) {
                        fpga_inject_frame(cmd.frame.data, cmd.frame.len);
                        fpga_enable_injection();
                        free(cmd.frame.data);
                    }
                    break;

                case CTRL_CMD_CAPTURE_START:
                    fpga_capture_start(CAPTURE_CTRL_SINGLE);
                    break;

                case CTRL_CMD_CAPTURE_STOP:
                    fpga_capture_stop();
                    break;

                case CTRL_CMD_CAPTURE_READ:
                    if (cmd.capture.buffer && cmd.capture.actual_len) {
                        fpga_capture_read(cmd.capture.buffer, cmd.capture.max_len,
                                          cmd.capture.actual_len);
                    }
                    break;

                case CTRL_CMD_RESET_FPGA:
                    fpga_reg_write(REG_SYS_RESET, 0xDEADBEEF);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    ESP_LOGI(TAG, "FPGA reset via register");
                    break;

                case CTRL_CMD_POLL_STATUS:
                    fpga_poll_status();
                    break;

                case CTRL_CMD_LOAD_CONFIG:
                    /* Load rules from NVS — handled by policy_engine */
                    xEventGroupSetBits(system_events, EVENT_BIT_CONFIG_CHANGE);
                    break;

                default:
                    ESP_LOGW(TAG, "Unknown command type: %d", cmd.cmd_type);
                    break;
            }
        } else {
            /* Periodic status polling */
            static int poll_counter = 0;
            if (++poll_counter >= 10) {  /* Every ~1 second */
                fpga_poll_status();
                poll_counter = 0;
            }
        }
    }
}
