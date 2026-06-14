/*
 * sdcard.c — SDIO SD card driver for STM32F407
 * Supports SDSC and SDHC cards via SDIO with DMA
 */

#include "sdcard.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* SDIO register base */
#define SDIO_R_POWER     (*(volatile uint32_t *)(SDIO_BASE + 0x00))
#define SDIO_R_CLKCR    (*(volatile uint32_t *)(SDIO_BASE + 0x04))
#define SDIO_R_ARG      (*(volatile uint32_t *)(SDIO_BASE + 0x08))
#define SDIO_R_CMD      (*(volatile uint32_t *)(SDIO_BASE + 0x0C))
#define SDIO_R_RESPCMD  (*(volatile uint32_t *)(SDIO_BASE + 0x10))
#define SDIO_R_RESP1    (*(volatile uint32_t *)(SDIO_BASE + 0x14))
#define SDIO_R_RESP2    (*(volatile uint32_t *)(SDIO_BASE + 0x18))
#define SDIO_R_RESP3    (*(volatile uint32_t *)(SDIO_BASE + 0x1C))
#define SDIO_R_RESP4    (*(volatile uint32_t *)(SDIO_BASE + 0x20))
#define SDIO_R_DTIMER   (*(volatile uint32_t *)(SDIO_BASE + 0x24))
#define SDIO_R_DLEN     (*(volatile uint32_t *)(SDIO_BASE + 0x28))
#define SDIO_R_DCTRL    (*(volatile uint32_t *)(SDIO_BASE + 0x2C))
#define SDIO_R_DCOUNT   (*(volatile uint32_t *)(SDIO_BASE + 0x30))
#define SDIO_R_STA      (*(volatile uint32_t *)(SDIO_BASE + 0x34))
#define SDIO_R_ICR      (*(volatile uint32_t *)(SDIO_BASE + 0x38))
#define SDIO_R_MASK     (*(volatile uint32_t *)(SDIO_BASE + 0x3C))
#define SDIO_R_FIFOCNT  (*(volatile uint32_t *)(SDIO_BASE + 0x48))
#define SDIO_R_FIFO     (*(volatile uint32_t *)(SDIO_BASE + 0x80))

/* SDIO commands */
#define SD_CMD_GO_IDLE_STATE     0
#define SD_CMD_SEND_OP_COND      1
#define SD_CMD_ALL_SEND_CID      2
#define SD_CMD_SET_REL_ADDR      3
#define SD_CMD_SEL_DESEL_CARD    7
#define SD_CMD_SEND_IF_COND      8
#define SD_CMD_SEND_CSD          9
#define SD_CMD_SEND_CID          10
#define SD_CMD_STOP_TRANSMISSION 12
#define SD_CMD_SET_BLOCKLEN      16
#define SD_CMD_READ_SINGLE_BLOCK 17
#define SD_CMD_READ_MULTIPLE_BLOCK 18
#define SD_CMD_WRITE_SINGLE_BLOCK 24
#define SD_CMD_WRITE_MULTIPLE_BLOCK 25
#define SD_CMD_APP_CMD           55
#define SD_ACMD_SEND_OP_COND     41

/* SD card types */
#define SD_TYPE_UNKNOWN  0
#define SD_TYPE_SDV1      1
#define SD_TYPE_SDV2      2
#define SD_TYPE_SDHC      3

static sd_card_info_t sd_info;
static volatile uint8_t sd_initialized = 0;

/* Recording state */
#define SD_LOG_HEADER_SIZE  16
typedef struct {
    uint32_t magic;          /* 0xCAFEBABE */
    uint16_t version;        /* 0x0100 = v1.0 */
    uint16_t channel;        /* CAN channel */
    uint32_t frame_count;    /* Number of frames written */
    uint32_t start_time;    /* Start timestamp */
    uint8_t  reserved[4];   /* Reserved */
} sd_log_header_t;

static sd_log_header_t log_header;
static volatile uint8_t recording_active = 0;
static volatile uint8_t replaying_active = 0;

static sd_result_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t resp_type, uint32_t timeout) {
    SDIO_R_ICR = 0xFFFFFFFF;  /* Clear all flags */

    SDIO_R_ARG = arg;
    SDIO_R_CMD = (resp_type << 6) | (cmd & 0x3F) | ((resp_type > 0) ? (1 << 6) : 0);

    uint32_t t = timeout;
    while (!(SDIO_R_STA & (1 << 6))) {  /* CCRCFAIL or CMDSENT */
        if (--t == 0) return SD_ERROR_TIMEOUT;
    }

    return SD_OK;
}

sd_result_t sd_init(void) {
    /* Enable SDIO clock */
    RCC->APB2ENR |= (1 << 11);  /* SDIOEN */
    RCC->AHB1ENR |= (1 << 8);   /* DMA2EN */

    /* Power off */
    SDIO_R_POWER = 0;
    for (volatile int i = 0; i < 100000; i++);

    /* Power on */
    SDIO_R_POWER = 0x03;  /* PWRON */
    for (volatile int i = 0; i < 100000; i++);

    /* Clock: 400 kHz for identification */
    SDIO_R_CLKCR = (118 << 0) | (1 << 8);  /* CLKDIV=118, CLKEN */

    /* Send CMD0 (GO_IDLE_STATE) */
    sd_send_cmd(SD_CMD_GO_IDLE_STATE, 0, 0, 100000);

    /* Send CMD8 (SEND_IF_COND) - check SD v2 */
    sd_result_t r = sd_send_cmd(SD_CMD_SEND_IF_COND, 0x000001AA, 3, 100000);
    if (r == SD_OK) {
        sd_info.card_type = SD_TYPE_SDV2;
    } else {
        sd_info.card_type = SD_TYPE_SDV1;
    }

    /* Send ACMD41 (SD_SEND_OP_COND) */
    uint32_t hcs = (sd_info.card_type == SD_TYPE_SDV2) ? (1 << 30) : 0;
    uint32_t retry = 1000;
    do {
        sd_send_cmd(SD_CMD_APP_CMD, 0, 1, 100000);
        sd_send_cmd(SD_ACMD_SEND_OP_COND, hcs | 0x00FF8000, 3, 100000);
    } while (!(SDIO_R_RESP1 & (1 << 31)) && --retry);

    if (retry == 0) return SD_ERROR_TIMEOUT;

    /* Check if SDHC */
    if (SDIO_R_RESP1 & (1 << 30)) {
        sd_info.card_type = SD_TYPE_SDHC;
    }

    /* Send CMD2 (ALL_SEND_CID) */
    sd_send_cmd(SD_CMD_ALL_SEND_CID, 0, 2, 100000);
    for (int i = 0; i < 4; i++) {
        ((volatile uint32_t *)sd_info.cid)[i] = (&SDIO_R_RESP1)[i];
    }

    /* Send CMD3 (SET_REL_ADDR) */
    sd_send_cmd(SD_CMD_SET_REL_ADDR, 0, 1, 100000);

    /* Switch to high-speed clock (25 MHz) */
    SDIO_R_CLKCR = (0 << 0) | (1 << 8) | (1 << 11);  /* CLKDIV=0, 4-bit bus */

    sd_info.block_size = SDCARD_BLOCK_SIZE;
    sd_info.block_count = 0;  /* Would read from CSD */

    sd_initialized = 1;
    return SD_OK;
}

sd_result_t sd_read_blocks(uint32_t block_addr, uint8_t *buf, uint32_t count) {
    if (!sd_initialized) return SD_ERROR_NO_CARD;

    uint32_t addr = (sd_info.card_type == SD_TYPE_SDHC) ? block_addr : block_addr * 512;

    /* Set block size */
    sd_send_cmd(SD_CMD_SET_BLOCKLEN, 512, 1, 100000);

    /* Configure DMA2 Stream 3 Channel 4 for SDIO */
    DMA2_Stream3->CR = (4 << 25) |   /* Channel 4 */
                       (2 << 16) |    /* 32-bit memory */
                       (2 << 13) |    /* 32-bit peripheral */
                       (1 << 10) |    /* Memory increment */
                       (0 << 9)  |    /* Peripheral no increment */
                       (0 << 6);      /* Peripheral-to-memory */
    DMA2_Stream3->PAR = (uint32_t)&SDIO_R_FIFO;
    DMA2_Stream3->M0AR = (uint32_t)buf;
    DMA2_Stream3->NDTR = count * (512 / 4);
    DMA2_Stream3->FCR = (1 << 2) | (3 << 0);  /* FIFO threshold full */

    /* Enable DMA */
    DMA2_Stream3->CR |= (1 << 0);

    /* Set data length and control */
    SDIO_R_DTIMER = 0xFFFFFFFF;
    SDIO_R_DLEN = count * 512;
    SDIO_R_DCTRL = (9 << 4) | (1 << 1) | (1 << 0);  /* 512 bytes, DMA, DTEN */

    /* Send read command */
    if (count == 1) {
        sd_send_cmd(SD_CMD_READ_SINGLE_BLOCK, addr, 1, 100000);
    } else {
        sd_send_cmd(SD_CMD_READ_MULTIPLE_BLOCK, addr, 1, 100000);
    }

    /* Wait for DMA completion (simplified — production uses DMA interrupt) */
    for (volatile int i = 0; i < 10000000; i++) {
        if (!(DMA2_Stream3->CR & (1 << 0))) break;  /* DMA disabled = done */
    }

    /* Stop transmission for multi-block read */
    if (count > 1) {
        sd_send_cmd(SD_CMD_STOP_TRANSMISSION, 0, 1, 100000);
    }

    return SD_OK;
}

sd_result_t sd_write_blocks(uint32_t block_addr, const uint8_t *buf, uint32_t count) {
    if (!sd_initialized) return SD_ERROR_NO_CARD;

    uint32_t addr = (sd_info.card_type == SD_TYPE_SDHC) ? block_addr : block_addr * 512;

    /* Set block size */
    sd_send_cmd(SD_CMD_SET_BLOCKLEN, 512, 1, 100000);

    /* Configure DMA for write */
    DMA2_Stream3->CR = (4 << 25) | (2 << 16) | (2 << 13) |
                       (1 << 10) | (1 << 6);  /* Memory-to-peripheral */
    DMA2_Stream3->PAR = (uint32_t)&SDIO_R_FIFO;
    DMA2_Stream3->M0AR = (uint32_t)buf;
    DMA2_Stream3->NDTR = count * (512 / 4);
    DMA2_Stream3->CR |= (1 << 0);

    /* Set data length and control */
    SDIO_R_DTIMER = 0xFFFFFFFF;
    SDIO_R_DLEN = count * 512;
    SDIO_R_DCTRL = (9 << 4) | (1 << 3) | (1 << 0);  /* 512 bytes, DMA, DTEN, dir=card */

    /* Send write command */
    if (count == 1) {
        sd_send_cmd(SD_CMD_WRITE_SINGLE_BLOCK, addr, 1, 100000);
    } else {
        sd_send_cmd(SD_CMD_WRITE_MULTIPLE_BLOCK, addr, 1, 100000);
    }

    /* Wait for completion */
    for (volatile int i = 0; i < 10000000; i++) {
        if (!(DMA2_Stream3->CR & (1 << 0))) break;
    }

    if (count > 1) {
        sd_send_cmd(SD_CMD_STOP_TRANSMISSION, 0, 1, 100000);
    }

    return SD_OK;
}

sd_result_t sd_get_info(sd_card_info_t *info) {
    if (!sd_initialized) return SD_ERROR_NO_CARD;
    *info = sd_info;
    return SD_OK;
}

sd_result_t sd_start_recording(const char *filename) {
    (void)filename;  /* Simplified — use fixed block address */
    recording_active = 1;
    log_header.magic = 0xCAFEBABE;
    log_header.version = 0x0100;
    log_header.channel = 0;
    log_header.frame_count = 0;
    log_header.start_time = TIM2->CNT;
    return SD_OK;
}

sd_result_t sd_stop_recording(void) {
    recording_active = 0;
    return SD_OK;
}

sd_result_t sd_write_can_frame(uint8_t channel, const can_frame_t *frame) {
    (void)channel;
    if (!recording_active) return SD_ERROR_WRITE;

    /* Pack CAN frame into 20-byte record:
     * [0-3]   timestamp (µs)
     * [4-7]   CAN ID
     * [8]     flags (IDE, RTR, DLC)
     * [9-16]  data bytes
     * [17-19] reserved
     */
    uint8_t record[20];
    uint32_t ts = frame->timestamp_us;
    record[0] = (ts >> 0) & 0xFF;
    record[1] = (ts >> 8) & 0xFF;
    record[2] = (ts >> 16) & 0xFF;
    record[3] = (ts >> 24) & 0xFF;
    record[4] = (frame->id >> 0) & 0xFF;
    record[5] = (frame->id >> 8) & 0xFF;
    record[6] = (frame->id >> 16) & 0xFF;
    record[7] = (frame->id >> 24) & 0xFF;
    record[8] = ((frame->id_type & 1) << 7) | ((frame->rtr & 1) << 6) | (frame->dlc & 0x0F);
    for (int i = 0; i < 8; i++) record[9 + i] = frame->data[i];
    record[17] = channel;
    record[18] = 0;
    record[19] = 0;

    /* Write to next block on SD card */
    uint32_t block = 1 + log_header.frame_count;  /* Block 0 = header */
    sd_write_blocks(block, record, 1);

    log_header.frame_count++;
    return SD_OK;
}

int sd_read_next_can_frame(can_frame_t *frame, uint32_t *timestamp) {
    (void)frame;
    (void)timestamp;
    /* Not fully implemented — would read sequentially from SD */
    return -1;
}

sd_result_t sd_start_replay(const char *filename) {
    (void)filename;
    replaying_active = 1;
    return SD_OK;
}

sd_result_t sd_stop_replay(void) {
    replaying_active = 0;
    return SD_OK;
}