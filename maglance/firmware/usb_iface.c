/*
 * usb_iface.c — USB CDC serial interface implementation for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements a simple ASCII command protocol over USB CDC (virtual serial).
 * The same protocol is forwarded to the BLE module via USART1.
 *
 * This is a simplified implementation that uses a ring buffer for
 * received data and processes commands line-by-line. A full
 * implementation would include proper USB descriptor tables and
 * endpoint handling — this is a skeletal driver that provides the
 * command parsing and dispatch logic.
 */

#include "usb_iface.h"
#include "registers.h"
#include "board.h"
#include "coil_driver.h"
#include "magnetometer.h"
#include "profile_manager.h"
#include <string.h>
#include <stdarg.h>

/* ---- Ring buffer for received data ---- */
#define RX_BUF_SIZE 256
#define TX_BUF_SIZE 512
#define MAX_ARGS    8
#define MAX_CMDS    32

static char    s_rx_buf[RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;
static char     s_rx_line[RX_BUF_SIZE];
static uint16_t s_rx_line_len = 0;

static char     s_tx_buf[TX_BUF_SIZE];

/* ---- Command registry ---- */
typedef struct {
    const char         *cmd;
    command_handler_t   handler;
} cmd_entry_t;

static cmd_entry_t s_cmd_table[MAX_CMDS];
static uint8_t     s_cmd_count = 0;

/* ---- Simple string utilities ---- */

static int str_cmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int str_len(const char *s)
{
    int n = 0;
    while (s[n]) n++;
    return n;
}

static int str_starts_with(const char *str, const char *prefix)
{
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

static char *str_tok(char *str, const char *delim, char **saveptr)
{
    char *token;
    if (str == NULL) str = *saveptr;

    /* Skip leading delimiters */
    while (*str) {
        const char *d = delim;
        int is_delim = 0;
        while (*d) {
            if (*str == *d) { is_delim = 1; break; }
            d++;
        }
        if (!is_delim) break;
        str++;
    }

    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }

    token = str;

    /* Find end of token */
    while (*str) {
        const char *d = delim;
        int is_delim = 0;
        while (*d) {
            if (*str == *d) { is_delim = 1; break; }
            d++;
        }
        if (is_delim) {
            *str++ = '\0';
            break;
        }
        str++;
    }

    *saveptr = str;
    return token;
}

static int parse_int(const char *s, uint32_t *out)
{
    uint32_t val = 0;
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    *out = val * sign;
    return 0;
}

/* ---- TX output (simplified — direct to buffer) ---- */

static void tx_raw(const char *data, uint16_t len)
{
    /*
     * In a full implementation, this would write to the USB CDC
     * TX endpoint. For this skeletal driver, we just copy to the
     * TX buffer. The BLE UART would also receive this data.
     */
    for (uint16_t i = 0; i < len && i < TX_BUF_SIZE; i++) {
        s_tx_buf[i] = data[i];
    }
    /* TODO: actually transmit via USB CDC endpoint */
    /* TODO: forward to USART1 (BLE) */
}

void usb_iface_send(const char *str)
{
    tx_raw(str, (uint16_t)str_len(str));
}

void usb_iface_sendf(const char *fmt, ...)
{
    /* Simplified printf — only supports %s, %d, %u, %x */
    char buf[256];
    int idx = 0;
    va_list args;
    va_start(args, fmt);

    while (*fmt && idx < 250) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
            case 's': {
                const char *s = va_arg(args, const char *);
                while (*s && idx < 250) buf[idx++] = *s++;
                break;
            }
            case 'd': {
                int d = va_arg(args, int);
                if (d < 0) { buf[idx++] = '-'; d = -d; }
                char tmp[12];
                int ti = 0;
                if (d == 0) tmp[ti++] = '0';
                while (d > 0) { tmp[ti++] = '0' + (d % 10); d /= 10; }
                while (ti > 0) buf[idx++] = tmp[--ti];
                break;
            }
            case 'u': {
                uint32_t u = va_arg(args, uint32_t);
                char tmp[12];
                int ti = 0;
                if (u == 0) tmp[ti++] = '0';
                while (u > 0) { tmp[ti++] = '0' + (u % 10); u /= 10; }
                while (ti > 0) buf[idx++] = tmp[--ti];
                break;
            }
            case 'x': {
                uint32_t x = va_arg(args, uint32_t);
                buf[idx++] = '0';
                buf[idx++] = 'x';
                int started = 0;
                for (int i = 28; i >= 0; i -= 4) {
                    uint8_t nib = (x >> i) & 0xF;
                    if (nib || started || i == 0) {
                        buf[idx++] = nib < 10 ? '0' + nib : 'A' + nib - 10;
                        started = 1;
                    }
                }
                break;
            }
            case '%':
                buf[idx++] = '%';
                break;
            default:
                buf[idx++] = '%';
                buf[idx++] = *fmt;
                break;
            }
        } else {
            buf[idx++] = *fmt;
        }
        fmt++;
    }
    buf[idx] = '\0';
    va_end(args);

    tx_raw(buf, (uint16_t)idx);
}

void usb_iface_send_binary(const uint8_t *data, uint16_t len)
{
    tx_raw((const char *)data, len);
}

/* ---- Command handlers ---- */

static int cmd_pulse(int argc, char **argv)
{
    if (argc < 5) {
        usb_iface_send("ERR 1 Usage: PULSE <width_ns> <current_ma> "
                       "<polarity> <count> <delay_us>\n");
        return -1;
    }

    uint32_t width_ns, current_ma, polarity, count, delay_us;
    parse_int(argv[1], &width_ns);
    parse_int(argv[2], &current_ma);
    parse_int(argv[3], &polarity);
    parse_int(argv[4], &count);
    parse_int(argv[5], &delay_us);

    if (count <= 1) {
        int rc = coil_driver_pulse(width_ns, current_ma,
                                   (polarity_t)polarity);
        if (rc == 0) usb_iface_send("OK\n");
        else usb_iface_sendf("ERR %d Pulse failed\n", rc);
    } else {
        int rc = coil_driver_pulse_burst(width_ns, current_ma,
                                         (polarity_t)polarity,
                                         count, delay_us);
        if (rc == 0) usb_iface_send("OK\n");
        else usb_iface_sendf("ERR %d Burst failed\n", rc);
    }
    return 0;
}

static int cmd_dc(int argc, char **argv)
{
    if (argc < 3) {
        usb_iface_send("ERR 1 Usage: DC <current_ma> <polarity>\n");
        return -1;
    }

    uint32_t current_ma, polarity;
    parse_int(argv[1], &current_ma);
    parse_int(argv[2], &polarity);

    int rc = coil_driver_dc_start(current_ma, (polarity_t)polarity);
    if (rc == 0) usb_iface_send("OK\n");
    else usb_iface_sendf("ERR %d DC start failed\n", rc);
    return 0;
}

static int cmd_sweep(int argc, char **argv)
{
    if (argc < 6) {
        usb_iface_send("ERR 1 Usage: SWEEP <start_hz> <stop_hz> "
                       "<steps> <dwell_ms> <current_ma>\n");
        return -1;
    }

    uint32_t start_hz, stop_hz, steps, dwell_ms, current_ma;
    parse_int(argv[1], &start_hz);
    parse_int(argv[2], &stop_hz);
    parse_int(argv[3], &steps);
    parse_int(argv[4], &dwell_ms);
    parse_int(argv[5], &current_ma);

    int rc = coil_driver_sweep_start(start_hz, stop_hz,
                                     (uint16_t)steps, (uint16_t)dwell_ms,
                                     current_ma);
    if (rc == 0) usb_iface_send("OK\n");
    else usb_iface_sendf("ERR %d Sweep start failed\n", rc);
    return 0;
}

static int cmd_sense(int argc, char **argv)
{
    if (argc < 2) {
        usb_iface_send("ERR 1 Usage: SENSE <rate_hz> <duration_s>\n");
        return -1;
    }

    uint32_t rate_hz, duration_s;
    parse_int(argv[1], &rate_hz);
    parse_int(argv[2], &duration_s);

    /* Set magnetometer rate */
    uint8_t tmrc;
    if (rate_hz >= 600) tmrc = RM3100_TMRC_600HZ;
    else if (rate_hz >= 300) tmrc = RM3100_TMRC_300HZ;
    else if (rate_hz >= 150) tmrc = RM3100_TMRC_150HZ;
    else tmrc = RM3100_TMRC_75HZ;

    magnetometer_start_continuous(tmrc);

    /* Stream data for the specified duration */
    usb_iface_send("OK STREAMING\n");

    mag_array_t reading;
    uint32_t samples = rate_hz * duration_s;
    for (uint32_t i = 0; i < samples; i++) {
        if (magnetometer_read_all(&reading) == 0) {
            /* Send binary frame (20 bytes) */
            uint8_t frame[20];
            frame[0] = 0xAA;
            frame[1] = 0x55;
            for (int s = 0; s < 3; s++) {
                frame[2 + s*6 + 0] = (reading.sensor[s].x >> 8) & 0xFF;
                frame[2 + s*6 + 1] = reading.sensor[s].x & 0xFF;
                frame[2 + s*6 + 2] = (reading.sensor[s].y >> 8) & 0xFF;
                frame[2 + s*6 + 3] = reading.sensor[s].y & 0xFF;
                frame[2 + s*6 + 4] = (reading.sensor[s].z >> 8) & 0xFF;
                frame[2 + s*6 + 5] = reading.sensor[s].z & 0xFF;
            }
            usb_iface_send_binary(frame, 20);
        }
        /* Rate limiting delay */
        for (volatile int d = 0; d < 1000; d++) { }
    }

    usb_iface_send("OK DONE\n");
    return 0;
}

static int cmd_stop(int argc, char **argv)
{
    (void)argc; (void)argv;
    coil_driver_dc_stop();
    coil_driver_sweep_stop();
    usb_iface_send("OK\n");
    return 0;
}

static int cmd_profile_load(int argc, char **argv)
{
    if (argc < 2) {
        usb_iface_send("ERR 1 Usage: PROFILE LOAD <slot>\n");
        return -1;
    }

    uint32_t slot;
    parse_int(argv[2], &slot);

    profile_t p;
    if (profile_manager_load((uint8_t)slot, &p) != 0) {
        usb_iface_sendf("ERR %d Profile load failed\n", ERR_PROFILE_CRC);
        return -1;
    }

    /* Apply the profile */
    g_status.active_profile = (uint8_t)slot;
    g_status.mode = (operating_mode_t)p.mode;
    g_status.polarity = (polarity_t)p.polarity;
    g_status.pulse_width_ns = p.pulse_width_ns;
    g_status.pulse_count = p.pulse_count;
    g_status.delay_us = p.delay_us;
    g_status.target_current_ma = p.current_ma;

    usb_iface_sendf("OK %s\n", p.name);
    return 0;
}

static int cmd_profile_save(int argc, char **argv)
{
    if (argc < 3) {
        usb_iface_send("ERR 1 Usage: PROFILE SAVE <slot> <name>\n");
        return -1;
    }

    uint32_t slot;
    parse_int(argv[2], &slot);

    profile_t p;
    memset(&p, 0, sizeof(p));
    p.mode = (uint8_t)g_status.mode;
    p.polarity = (uint8_t)g_status.polarity;
    p.tip_id = g_status.tip_id;
    p.pulse_width_ns = g_status.pulse_width_ns;
    p.pulse_count = g_status.pulse_count;
    p.delay_us = g_status.delay_us;
    p.current_ma = g_status.target_current_ma;

    /* Copy name (argv[3]) */
    const char *name = argv[3];
    int n = 0;
    while (name[n] && n < 23) {
        p.name[n] = name[n];
        n++;
    }
    p.name[n] = '\0';

    if (profile_manager_save((uint8_t)slot, &p) != 0) {
        usb_iface_send("ERR 2 Save failed\n");
        return -1;
    }

    usb_iface_send("OK\n");
    return 0;
}

static int cmd_profile_get(int argc, char **argv)
{
    if (argc < 2) {
        usb_iface_send("ERR 1 Usage: PROFILE GET <slot>\n");
        return -1;
    }

    uint32_t slot;
    parse_int(argv[2], &slot);

    profile_t p;
    if (profile_manager_load((uint8_t)slot, &p) != 0) {
        usb_iface_send("ERR 3 Empty or corrupt\n");
        return -1;
    }

    usb_iface_sendf("OK %d %d %d %u %u %u %u %s\n",
                    p.mode, p.polarity, p.tip_id,
                    p.pulse_width_ns, p.pulse_count,
                    p.delay_us, p.current_ma, p.name);
    return 0;
}

static int cmd_profile(int argc, char **argv)
{
    if (argc < 2) {
        usb_iface_send("ERR 1 Usage: PROFILE <LOAD|SAVE|GET> <slot> [...]\n");
        return -1;
    }

    if (str_cmp(argv[1], "LOAD") == 0) return cmd_profile_load(argc, argv);
    if (str_cmp(argv[1], "SAVE") == 0) return cmd_profile_save(argc, argv);
    if (str_cmp(argv[1], "GET") == 0)  return cmd_profile_get(argc, argv);

    usb_iface_send("ERR 1 Unknown PROFILE subcommand\n");
    return -1;
}

static int cmd_get_field(int argc, char **argv)
{
    (void)argc; (void)argv;
    mag_array_t reading;
    if (magnetometer_read_all(&reading) != 0) {
        usb_iface_send("ERR 4 Sensor read failed\n");
        return -1;
    }

    usb_iface_sendf("OK %d %d %d %d %d %d %d %d %d\n",
                    reading.sensor[0].x, reading.sensor[0].y, reading.sensor[0].z,
                    reading.sensor[1].x, reading.sensor[1].y, reading.sensor[1].z,
                    reading.sensor[2].x, reading.sensor[2].y, reading.sensor[2].z);
    return 0;
}

static int cmd_get_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    usb_iface_sendf("OK %d %u %u %d %d %d\n",
                    g_status.mode,
                    g_status.current_ma,
                    g_status.vbat_mv,
                    g_status.temp_bridge_c,
                    g_status.temp_coil_c,
                    g_status.active_profile);
    return 0;
}

static int cmd_get_tip(int argc, char **argv)
{
    (void)argc; (void)argv;
    usb_iface_sendf("OK %d MagLance-Tip-%d\n", g_status.tip_id, g_status.tip_id);
    return 0;
}

static int cmd_set_tip(int argc, char **argv)
{
    if (argc < 2) {
        usb_iface_send("ERR 1 Usage: SET TIP <tip_id>\n");
        return -1;
    }
    uint32_t tip_id;
    parse_int(argv[2], &tip_id);
    g_status.tip_id = (uint8_t)tip_id;
    usb_iface_send("OK\n");
    return 0;
}

static int cmd_get(int argc, char **argv)
{
    if (argc < 2) {
        usb_iface_send("ERR 1 Usage: GET <FIELD|STATUS|TIP>\n");
        return -1;
    }
    if (str_cmp(argv[1], "FIELD") == 0)  return cmd_get_field(argc, argv);
    if (str_cmp(argv[1], "STATUS") == 0) return cmd_get_status(argc, argv);
    if (str_cmp(argv[1], "TIP") == 0)    return cmd_get_tip(argc, argv);
    usb_iface_send("ERR 1 Unknown GET target\n");
    return -1;
}

static int cmd_set(int argc, char **argv)
{
    if (argc < 2) {
        usb_iface_send("ERR 1 Usage: SET <TIP> <value>\n");
        return -1;
    }
    if (str_cmp(argv[1], "TIP") == 0) return cmd_set_tip(argc, argv);
    usb_iface_send("ERR 1 Unknown SET target\n");
    return -1;
}

static int cmd_cal(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* Calibration routine — drive coil at known current, measure field */
    usb_iface_send("OK CALIBRATION_START\n");

    if (coil_driver_arm() != 0) {
        usb_iface_send("ERR 5 Safety not armed\n");
        return -1;
    }

    /* Sweep current from 100 mA to 5 A in 10 steps, measure field at each */
    for (int step = 0; step <= 10; step++) {
        uint32_t current = 100 + step * 490;  /* 100 mA to 5 A */
        coil_driver_dc_start(current, POLARITY_NORTH);

        /* Wait for field to stabilize */
        for (volatile int d = 0; d < 100000; d++) { }

        mag_array_t reading;
        magnetometer_read_all(&reading);

        usb_iface_sendf("OK %u %d %d %d\n",
                        current,
                        reading.sensor[0].x,
                        reading.sensor[0].y,
                        reading.sensor[0].z);

        coil_driver_dc_stop();
    }

    coil_driver_disarm();
    usb_iface_send("OK CALIBRATION_DONE\n");
    return 0;
}

static int cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    usb_iface_send(
        "MagLance Command Reference (author: jayis1)\n"
        "=============================================\n"
        "PULSE <width_ns> <current_ma> <polarity> <count> <delay_us>\n"
        "  Fire one or more magnetic field pulses.\n"
        "  polarity: 0=NORTH, 1=SOUTH\n\n"
        "DC <current_ma> <polarity>\n"
        "  Start continuous DC field.\n\n"
        "SWEEP <start_hz> <stop_hz> <steps> <dwell_ms> <current_ma>\n"
        "  Start frequency sweep.\n\n"
        "SENSE <rate_hz> <duration_s>\n"
        "  Stream magnetometer data (passive mode).\n\n"
        "STOP\n"
        "  Stop all coil output.\n\n"
        "PROFILE LOAD <slot>\n"
        "PROFILE SAVE <slot> <name>\n"
        "PROFILE GET <slot>\n"
        "  Manage stored profiles (0-15).\n\n"
        "CAL <tip_id>\n"
        "  Run coil calibration.\n\n"
        "GET FIELD\n"
        "  Read all 3 magnetometers.\n\n"
        "GET STATUS\n"
        "  Get system status.\n\n"
        "GET TIP / SET TIP <tip_id>\n"
        "  Query or set connected coil tip.\n\n"
        "HELP\n"
        "  Show this help.\n"
    );
    return 0;
}

/* ---- Command processing ---- */

static void process_command(char *line)
{
    /* Parse command and arguments */
    char *argv[MAX_ARGS + 1];
    int argc = 0;
    char *saveptr = NULL;

    char *tok = str_tok(line, " \t\r\n", &saveptr);
    while (tok && argc < MAX_ARGS) {
        argv[argc++] = tok;
        tok = str_tok(NULL, " \t\r\n", &saveptr);
    }
    argv[argc] = NULL;

    if (argc == 0) return;

    /* Look up command in registry */
    for (uint8_t i = 0; i < s_cmd_count; i++) {
        if (str_cmp(argv[0], s_cmd_table[i].cmd) == 0) {
            s_cmd_table[i].handler(argc, argv);
            return;
        }
    }

    usb_iface_sendf("ERR 1 Unknown command: %s\n", argv[0]);
}

void usb_iface_register_command(const char *cmd, command_handler_t handler)
{
    if (s_cmd_count < MAX_CMDS) {
        s_cmd_table[s_cmd_count].cmd = cmd;
        s_cmd_table[s_cmd_count].handler = handler;
        s_cmd_count++;
    }
}

static void register_builtin_commands(void)
{
    usb_iface_register_command("PULSE", cmd_pulse);
    usb_iface_register_command("DC", cmd_dc);
    usb_iface_register_command("SWEEP", cmd_sweep);
    usb_iface_register_command("SENSE", cmd_sense);
    usb_iface_register_command("STOP", cmd_stop);
    usb_iface_register_command("PROFILE", cmd_profile);
    usb_iface_register_command("GET", cmd_get);
    usb_iface_register_command("SET", cmd_set);
    usb_iface_register_command("CAL", cmd_cal);
    usb_iface_register_command("HELP", cmd_help);
}

int usb_iface_init(void)
{
    s_rx_head = 0;
    s_rx_tail = 0;
    s_rx_line_len = 0;
    s_cmd_count = 0;

    register_builtin_commands();

    /*
     * Full USB CDC initialization would include:
     * 1. Enable USB clock
     * 2. Configure PA11 (DM) and PA12 (DP) as AF
     * 3. Set up USB device descriptors
     * 4. Configure endpoints (CDC control + bulk)
     * 5. Enable USB peripheral and connect
     * 6. Handle enumeration interrupts
     *
     * This is skeletal — the command processing logic works
     * with data fed from any transport (USB, BLE UART, etc.)
     */

    return 0;
}

void usb_iface_poll(void)
{
    /*
     * Process received characters from the ring buffer.
     * Accumulate into a line and process when newline is received.
     *
     * In a full implementation, the USB CDC RX interrupt would
     * fill the ring buffer. Here we check if there's data available.
     */

    while (s_rx_tail != s_rx_head) {
        char c = s_rx_buf[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) % RX_BUF_SIZE;

        if (c == '\n' || c == '\r') {
            if (s_rx_line_len > 0) {
                s_rx_line[s_rx_line_len] = '\0';
                process_command(s_rx_line);
                s_rx_line_len = 0;
            }
        } else if (s_rx_line_len < RX_BUF_SIZE - 1) {
            s_rx_line[s_rx_line_len++] = c;
        } else {
            /* Line too long — reset */
            s_rx_line_len = 0;
        }
    }
}

int usb_iface_is_connected(void)
{
    /* TODO: check USB enumerated state */
    return 1;  /* Assume connected for testing */
}

/* ---- RX interrupt handler (called by USB CDC RX callback) ---- */
void usb_iface_rx_callback(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (s_rx_head + 1) % RX_BUF_SIZE;
        if (next != s_rx_tail) {  /* Buffer not full */
            s_rx_buf[s_rx_head] = data[i];
            s_rx_head = next;
        }
    }
}