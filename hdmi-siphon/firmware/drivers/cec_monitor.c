/**
 * @file    cec_monitor.c
 * @brief   HDMI-CEC bus monitor and command injector
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Implements HDMI-CEC (Consumer Electronics Control) bus sniffing and 
 * injection per the HDMI-CEC specification (HDMI 1.4a). The CEC bus is 
 * a single-wire, open-drain bidirectional bus running at ~3.6 kHz.
 *
 * CEC frames consist of:
 *   - Start bit (4.5 ms low)
 *   - Data bits: logical 0 = 1.5 ms high + 1.5 ms low, logical 1 = 0.6 ms high + 2.4 ms low
 *   - EOM (End of Message) bit
 *   - ACK bit (1.5 ms)
 * 
 * CEC addresses are 4 bits: logical device addresses (0-15).
 * Common addresses: 0=TV, 1=Recorder, 4=Player, 5=Audio, 15=Broadcast
 */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "board.h"
#include "registers.h"
#include "cec_monitor.h"
#include "sd_card.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "CEC_MONITOR";

/** CEC nominal bit period (us) — 2.4 ms per the spec */
#define CEC_BIT_PERIOD_US             2400U

/** CEC half bit period (us) */
#define CEC_HALF_BIT_US               1200U

/** CEC start signal length (us) — 4.5 ms */
#define CEC_START_LOW_US              4500U

/** Maximum CEC message length (bytes) — per spec: header + up to 15 data bytes */
#define CEC_MAX_MSG_LEN                  16U

/** Debounce time for CEC edges (us) */
#define CEC_DEBOUNCE_US                  200U

/** Max CEC log entries in memory before flushing to SD */
#define CEC_LOG_MAX_ENTRIES               64U

/** Timeout for waiting on ACK (us) */
#define CEC_ACK_TIMEOUT_US              5000U

/** Logical device address for HDMI-Siphon */
#define CEC_OUR_ADDRESS                    5U   /* Audio system */

/** CEC retry count for failed transmissions */
#define CEC_MAX_RETRIES                     3U

/* =========================================================================
 * CEC Opcodes (subset)
 * ========================================================================= */

#define CEC_OPCODE_FEATURE_ABORT      0x00
#define CEC_OPCODE_GIVE_PHYSICAL_ADDR 0x83
#define CEC_OPCODE_REPORT_PHYSICAL_ADDR 0x84
#define CEC_OPCODE_GIVE_OSD_NAME      0x46
#define CEC_OPCODE_SET_OSD_NAME       0x47
#define CEC_OPCODE_USER_CONTROL_PRESSED 0x44
#define CEC_OPCODE_USER_CONTROL_RELEASED 0x45
#define CEC_OPCODE_STANDBY            0x36
#define CEC_OPCODE_ACTIVE_SOURCE      0x82
#define CEC_OPCODE_REQUEST_ACTIVE_SOURCE 0x85
#define CEC_OPCODE_VENDOR_ID          0x87
#define CEC_OPCODE_GIVE_DEVICE_POWER_STATUS 0x8F
#define CEC_OPCODE_REPORT_POWER_STATUS 0x90
#define CEC_OPCODE_MENU_REQUEST       0x8D
#define CEC_OPCODE_MENU_STATUS        0x8E
#define CEC_OPCODE_VOLUME_UP          0x41
#define CEC_OPCODE_VOLUME_DOWN        0x42
#define CEC_OPCODE_MUTE               0x43

/* User Control Codes (for USER_CONTROL_PRESSED) */
#define CEC_UC_SELECT                 0x00
#define CEC_UC_UP                     0x01
#define CEC_UC_DOWN                   0x02
#define CEC_UC_LEFT                   0x03
#define CEC_UC_RIGHT                  0x04
#define CEC_UC_EXIT                   0x0D
#define CEC_UC_PLAY                   0x44
#define CEC_UC_PAUSE                  0x46
#define CEC_UC_STOP                   0x47
#define CEC_UC_FAST_FORWARD           0x49
#define CEC_UC_REWIND                 0x4A
#define CEC_UC_NUM_0                  0x20
#define CEC_UC_NUM_1                  0x21
#define CEC_UC_NUM_9                  0x29
#define CEC_UC_POWER                  0x6B
#define CEC_UC_VOLUME_UP              0x41
#define CEC_UC_VOLUME_DOWN            0x42
#define CEC_UC_MUTE                   0x43

/* =========================================================================
 * Local State
 * ========================================================================= */

/** CEC message ring buffer entry */
typedef struct {
    uint64_t    timestamp_us;   /**< Timestamp of reception */
    uint16_t    source_addr;    /**< Source logical address (4-bit) */
    uint16_t    dest_addr;      /**< Destination logical address (4-bit) */
    uint8_t     opcode;         /**< CEC opcode */
    uint8_t     data[CEC_MAX_MSG_LEN]; /**< Payload */
    uint8_t     data_len;       /**< Payload length */
    bool        is_broadcast;   /**< True if broadcast message */
} cec_log_entry_t;

/** CEC state machine states */
typedef enum {
    CEC_STATE_IDLE,         /**< Waiting for start bit */
    CEC_STATE_START_BIT,    /**< Detected start bit */
    CEC_STATE_RECEIVING,    /**< Receiving message bits */
    CEC_STATE_ACK,          /**< Waiting for ACK */
    CEC_STATE_COMPLETE      /**< Message complete, processing */
} cec_state_t;

/** CEC monitor state */
typedef struct {
    cec_state_t     state;              /**< Current state */
    uint8_t         rx_buffer[CEC_MAX_MSG_LEN]; /**< Receive buffer */
    uint8_t         rx_byte;            /**< Current byte being assembled */
    uint8_t         rx_bit_count;       /**< Bits received in current byte */
    uint8_t         rx_byte_count;      /**< Bytes received in message */
    bool            rx_eom;             /**< End of message flag received */
    uint64_t        last_edge_time;     /**< Timestamp of last bus edge */
    uint8_t         last_level;         /**< Last sampled bus level */
    bool            monitoring;         /**< Monitor enabled */
    uint32_t        msg_count;          /**< Total messages captured */
    cec_log_entry_t log_buffer[CEC_LOG_MAX_ENTRIES];
    uint8_t         log_head;           /**< Circular buffer head */
    uint8_t         log_count;          /**< Number of entries in log */
} cec_monitor_t;

static cec_monitor_t s_cec = {0};

/* =========================================================================
 * CEC Address Name Table
 * ========================================================================= */

static const char *cec_addr_name(uint8_t addr)
{
    static const char *names[] = {
        "TV", "Rec1", "Rec2", "Tuner1", "Player", "Audio", "Tuner2",
        "Tuner3", "Rec3", "Tuner4", "Reserved", "Reserved",
        "Reserved", "Reserved", "Specific", "Broadcast"
    };
    if (addr <= 15) return names[addr];
    return "Unknown";
}

static const char *cec_opcode_name(uint8_t opcode)
{
    switch (opcode) {
        case CEC_OPCODE_FEATURE_ABORT:        return "Feature Abort";
        case CEC_OPCODE_GIVE_PHYSICAL_ADDR:   return "Give Physical Address";
        case CEC_OPCODE_REPORT_PHYSICAL_ADDR: return "Report Physical Address";
        case CEC_OPCODE_GIVE_OSD_NAME:        return "Give OSD Name";
        case CEC_OPCODE_SET_OSD_NAME:         return "Set OSD Name";
        case CEC_OPCODE_USER_CONTROL_PRESSED: return "User Control Pressed";
        case CEC_OPCODE_USER_CONTROL_RELEASED: return "User Control Released";
        case CEC_OPCODE_STANDBY:              return "Standby";
        case CEC_OPCODE_ACTIVE_SOURCE:        return "Active Source";
        case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:return "Request Active Source";
        case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS: return "Give Power Status";
        case CEC_OPCODE_REPORT_POWER_STATUS:  return "Report Power Status";
        default:                              return "Unknown";
    }
}

/* =========================================================================
 * Initialization
 * ========================================================================= */

void cec_monitor_init(void)
{
    /* Configure CEC pins as open-drain */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_CEC_RX) | (1ULL << PIN_CEC_TX),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    /* Set initial high (inactive) state on CEC bus */
    gpio_set_level(PIN_CEC_TX, 1);

    /* Initialize state */
    s_cec.state = CEC_STATE_IDLE;
    s_cec.monitoring = true;
    s_cec.last_level = 1; /* Bus idle high */
    s_cec.msg_count = 0;
    s_cec.log_head = 0;
    s_cec.log_count = 0;

    ESP_LOGI(TAG, "CEC monitor initialized (monitoring bus)");
}

/* =========================================================================
 * Bus Timing
 * ========================================================================= */

/**
 * @brief Wait for specified duration with microsecond precision
 * @param us Microseconds to delay
 */
static inline void cec_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

/**
 * @brief Read current time in microseconds
 */
static inline uint64_t cec_time_us(void)
{
    return esp_timer_get_time();
}

/**
 * @brief Sample CEC bus level
 */
static inline uint8_t cec_read_bus(void)
{
    return (uint8_t)gpio_get_level(PIN_CEC_RX);
}

/**
 * @brief Drive CEC bus low (open-drain pulls low)
 */
static inline void cec_bus_low(void)
{
    gpio_set_level(PIN_CEC_TX, 0);
}

/**
 * @brief Release CEC bus (pull-up resistor brings high)
 */
static inline void cec_bus_release(void)
{
    gpio_set_level(PIN_CEC_TX, 1);
}

/* =========================================================================
 * CEC Frame Decoding
 * ========================================================================= */

/**
 * @brief Decode CEC message and log it
 */
static void cec_decode_and_log(void)
{
    uint8_t header = s_cec.rx_buffer[0];
    uint8_t source = (header >> 4) & 0x0F;
    uint8_t dest   = header & 0x0F;
    bool broadcast = (dest == 0x0F);

    uint8_t opcode = (s_cec.rx_byte_count > 1) ? s_cec.rx_buffer[1] : 0xFF;
    uint8_t data_len = (s_cec.rx_byte_count > 2) ? s_cec.rx_byte_count - 2 : 0;

    /* Log the message */
    s_cec.msg_count++;

    cec_log_entry_t *entry = &s_cec.log_buffer[s_cec.log_head];
    entry->timestamp_us = cec_time_us();
    entry->source_addr = source;
    entry->dest_addr = dest;
    entry->opcode = opcode;
    entry->data_len = data_len;
    entry->is_broadcast = broadcast;
    memcpy(entry->data, s_cec.rx_buffer + 2, data_len);

    s_cec.log_head = (s_cec.log_head + 1) % CEC_LOG_MAX_ENTRIES;
    if (s_cec.log_count < CEC_LOG_MAX_ENTRIES) {
        s_cec.log_count++;
    }

    /* Print to console */
    ESP_LOGI(TAG, "CEC: %s[%s → %s] op=0x%02X (%s) len=%u",
             broadcast ? "BC " : "",
             cec_addr_name(source), cec_addr_name(dest),
             opcode, cec_opcode_name(opcode),
             data_len);

    /* Log keylogging events */
    if (opcode == CEC_OPCODE_USER_CONTROL_PRESSED && data_len >= 1) {
        uint8_t uc = s_cec.rx_buffer[2];
        ESP_LOGW(TAG, "CEC KEY: source=%s, key=0x%02X",
                 cec_addr_name(source), uc);
    }

    /* Write to SD card log periodically */
    if (s_cec.msg_count % 10 == 0) {
        char log_line[128];
        snprintf(log_line, sizeof(log_line),
                 "[%llu] %s[%s->%s] op=0x%02X len=%u\n",
                 entry->timestamp_us,
                 broadcast ? "BC" : "",
                 cec_addr_name(source),
                 cec_addr_name(dest),
                 opcode, data_len);

        sd_card_write_log("cec_activity.log", log_line, strlen(log_line));
    }
}

/* =========================================================================
 * CEC Bus Monitoring (Passive)
 * ========================================================================= */

/**
 * @brief Monitor CEC bus for one complete message (non-blocking poll)
 *
 * Called periodically from the CEC task. Implements a simple state
 * machine that tracks bus edges and reconstructs CEC frames.
 *
 * CEC bit timing (from spec):
 *   Start bit: 4.5 ms low, then release
 *   Logical 0: 1.5 ms low + 1.5 ms high = 3.0 ms
 *   Logical 1: 0.6 ms low + 2.4 ms high = 3.0 ms
 *   Nominal bit period: 2.4 ms
 */
void cec_monitor_poll(void)
{
    if (!s_cec.monitoring) return;

    uint8_t level = cec_read_bus();
    uint64_t now = cec_time_us();

    /* Detect edges */
    if (level == s_cec.last_level) {
        return;  /* No edge */
    }

    uint64_t dt = now - s_cec.last_edge_time;
    s_cec.last_edge_time = now;
    s_cec.last_level = level;

    /* Debounce short glitches */
    if (dt < CEC_DEBOUNCE_US) return;

    switch (s_cec.state) {
        case CEC_STATE_IDLE:
            if (level == 0 && dt > CEC_START_LOW_US - 500 && dt < CEC_START_LOW_US + 500) {
                /* Detected start bit — beginning of message */
                s_cec.state = CEC_STATE_RECEIVING;
                s_cec.rx_byte = 0;
                s_cec.rx_bit_count = 0;
                s_cec.rx_byte_count = 0;
                s_cec.rx_eom = false;
                memset(s_cec.rx_buffer, 0, CEC_MAX_MSG_LEN);
            }
            break;

        case CEC_STATE_RECEIVING:
            /* We're tracking edges for data bits.
             * For simplicity in this poll-driven approach, we detect
             * the end of each bit by the falling edge.
             * Bit value determined by low-pulse duration:
             *   Short low (~0.6ms) = logical 1
             *   Long low (~1.5ms) = logical 0 */
            if (level == 1 && dt > 500 && dt < 2000) {
                /* Rising edge — end of low pulse, bit value determined */
                uint8_t bit_val;

                /* Use FPGA bit-bang registers for hardware-timed reception */
                uint32_t cec_status = reg_read(REG_CEC_STATUS);
                if (cec_status & CEC_STATUS_RX_READY) {
                    s_cec.rx_buffer[s_cec.rx_byte_count] = (uint8_t)reg_read(REG_CEC_RX_DATA);
                    s_cec.rx_byte_count++;
                    reg_clear_bits(REG_CEC_CTRL, CEC_CTRL_ENABLE);
                }
            }
            break;

        case CEC_STATE_COMPLETE:
            cec_decode_and_log();
            s_cec.state = CEC_STATE_IDLE;
            break;

        default:
            s_cec.state = CEC_STATE_IDLE;
            break;
    }
}

/* =========================================================================
 * CEC Message Injection
 * ========================================================================= */

int cec_monitor_send(uint8_t destination, uint8_t opcode,
                     const uint8_t *data, uint8_t data_len)
{
    if (data_len > CEC_MAX_MSG_LEN - 2) {
        ESP_LOGE(TAG, "CEC message too long: %u bytes", data_len);
        return -1;
    }

    /* Check if bus is free (idle high for >5ms) */
    uint8_t bus_level = cec_read_bus();
    if (bus_level == 0) {
        ESP_LOGW(TAG, "CEC bus busy, cannot send");
        return -2;
    }

    /* Build frame in FPGA CEC TX buffer */
    uint8_t header = ((CEC_OUR_ADDRESS & 0x0F) << 4) | (destination & 0x0F);
    reg_write(REG_CEC_TX_DATA, (uint32_t)header);
    reg_write(REG_CEC_TX_LEN, 1 + (uint32_t)data_len);

    /* Write data bytes */
    for (uint8_t i = 0; i < data_len; i++) {
        reg_write(REG_CEC_TX_DATA + 1 + i, data[i]);
    }

    /* Trigger transmission */
    reg_set_bits(REG_CEC_CTRL, CEC_CTRL_ENABLE);

    /* Wait for transmission complete or timeout */
    bool done = reg_wait_for_bit(REG_CEC_STATUS, CEC_STATUS_TX_BUSY, 100);
    if (!done) {
        ESP_LOGW(TAG, "CEC send timeout");
        reg_clear_bits(REG_CEC_CTRL, CEC_CTRL_ENABLE);
        return -3;
    }

    /* Check for ACK error */
    uint32_t cec_status = reg_read(REG_CEC_STATUS);
    if (cec_status & CEC_STATUS_ERROR) {
        ESP_LOGW(TAG, "CEC NAK received");
        return -4;
    }

    ESP_LOGI(TAG, "CEC sent: %s → %s, op=0x%02X",
             cec_addr_name(CEC_OUR_ADDRESS),
             cec_addr_name(destination), opcode);

    return 0;
}

int cec_monitor_send_standby(void)
{
    return cec_monitor_send(0, CEC_OPCODE_STANDBY, NULL, 0);
}

int cec_monitor_send_active_source(uint16_t physical_addr)
{
    uint8_t data[2] = {
        (uint8_t)((physical_addr >> 8) & 0xFF),
        (uint8_t)(physical_addr & 0xFF)
    };
    return cec_monitor_send(0x0F, CEC_OPCODE_ACTIVE_SOURCE, data, 2);
}

int cec_monitor_send_user_control(uint8_t destination, uint8_t uc_code)
{
    uint8_t data[1] = { uc_code };
    int ret = cec_monitor_send(destination, CEC_OPCODE_USER_CONTROL_PRESSED, data, 1);
    if (ret == 0) {
        cec_delay_us(1000);
        ret = cec_monitor_send(destination, CEC_OPCODE_USER_CONTROL_RELEASED, NULL, 0);
    }
    return ret;
}

/* =========================================================================
 * Query
 * ========================================================================= */

bool cec_monitor_is_bus_active(void)
{
    return (cec_read_bus() == 0);
}

uint32_t cec_monitor_get_message_count(void)
{
    return s_cec.msg_count;
}

int cec_monitor_get_log(cec_log_entry_t *entries, int max_entries)
{
    if (entries == NULL || max_entries <= 0) return 0;

    int to_copy = (s_cec.log_count < max_entries) ? s_cec.log_count : max_entries;
    int start = (s_cec.log_head - s_cec.log_count + CEC_LOG_MAX_ENTRIES) % CEC_LOG_MAX_ENTRIES;

    for (int i = 0; i < to_copy; i++) {
        entries[i] = s_cec.log_buffer[(start + i) % CEC_LOG_MAX_ENTRIES];
    }

    return to_copy;
}

void cec_monitor_enable(bool enable)
{
    s_cec.monitoring = enable;
    if (enable) {
        reg_set_bits(REG_CEC_CTRL, CEC_CTRL_ENABLE);
    } else {
        reg_clear_bits(REG_CEC_CTRL, CEC_CTRL_ENABLE);
    }
    ESP_LOGI(TAG, "CEC monitoring %s", enable ? "enabled" : "disabled");
}

/* =========================================================================
 * FreeRTOS Task
 * ========================================================================= */

void cec_task(void *pvParameters)
{
    ESP_LOGI(TAG, "CEC monitor task started");
    cec_monitor_init();

    while (1) {
        cec_monitor_poll();
        vTaskDelay(pdMS_TO_TICKS(5));  /* Poll at ~200 Hz */
    }
}
