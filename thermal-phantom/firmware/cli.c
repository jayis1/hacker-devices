/*
 * cli.c - Serial command-line interface for manual operation
 *
 * Provides a text-based interface over USB CDC for interactive control.
 * Commands: set_temp, set_mode, arm, fire, read_temp, load_profile,
 *           start_profile, stop, status, pid, version, help
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "cli.h"
#include "board.h"
#include "registers.h"
#include "thermal_ctl.h"
#include "tec_driver.h"
#include "laser_driver.h"
#include "temp_sensor.h"
#include "trigger.h"
#include "safety.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define CLI_BUF_SIZE   128
#define CLI_MAX_ARGS    8

static char cli_buffer[CLI_BUF_SIZE];
static uint16_t cli_buf_index = 0;
static bool cli_initialized = false;

/* ============================================================
 * UART send helpers
 * ============================================================ */

static void cli_send_str(const char *str)
{
    uint32_t uart_base = USART3_BASE;
    while (*str) {
        while (!(USART_ISR(uart_base) & USART_ISR_TXE));
        USART_TDR(uart_base) = (uint8_t)*str++;
    }
}

void cli_print(const char *str)
{
    cli_send_str(str);
}

void cli_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    /* Simple sprintf - in bare-metal we use a minimal implementation */
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        cli_send_str(buf);
    }
}

/* ============================================================
 * Command handlers
 * ============================================================ */

static void cmd_set_temp(int argc, char **argv)
{
    if (argc < 2) {
        cli_print("Usage: set_temp <temp_c>\r\n");
        return;
    }
    float temp = strtof(argv[1], NULL);
    if (temp < SAFETY_MIN_TEMP_C || temp > SAFETY_MAX_TEMP_C) {
        cli_printf("Error: Temperature out of range (%.1f to %.1f)\r\n",
                   SAFETY_MIN_TEMP_C, SAFETY_MAX_TEMP_C);
        return;
    }
    thermal_ctl_set_target(temp);
    thermal_ctl_set_mode(THERMAL_MODE_HEAT);
    cli_printf("Target set to %.1f°C, mode: HEAT\r\n", temp);
}

static void cmd_set_mode(int argc, char **argv)
{
    if (argc < 2) {
        cli_print("Usage: set_mode <heat|cool|idle|laser>\r\n");
        return;
    }
    if (strcmp(argv[1], "heat") == 0) {
        thermal_ctl_set_mode(THERMAL_MODE_HEAT);
        cli_print("Mode: HEAT\r\n");
    } else if (strcmp(argv[1], "cool") == 0) {
        thermal_ctl_set_mode(THERMAL_MODE_COOL);
        cli_print("Mode: COOL\r\n");
    } else if (strcmp(argv[1], "idle") == 0) {
        thermal_ctl_set_mode(THERMAL_MODE_IDLE);
        cli_print("Mode: IDLE\r\n");
    } else if (strcmp(argv[1], "laser") == 0) {
        thermal_ctl_set_mode(THERMAL_MODE_LASER);
        cli_print("Mode: LASER (arm required)\r\n");
    } else {
        cli_print("Unknown mode. Use: heat, cool, idle, laser\r\n");
    }
}

static void cmd_arm_trigger(int argc, char **argv)
{
    trigger_edge_t edge = TRIG_EDGE_RISING;
    uint32_t delay_us = 0;
    
    if (argc >= 3) {
        if (strcmp(argv[2], "rising") == 0) edge = TRIG_EDGE_RISING;
        else if (strcmp(argv[2], "falling") == 0) edge = TRIG_EDGE_FALLING;
        else if (strcmp(argv[2], "both") == 0) edge = TRIG_EDGE_BOTH;
    }
    if (argc >= 4) {
        /* Parse delay=NNus format */
        char *d = strstr(argv[3], "delay=");
        if (d) {
            delay_us = strtoul(d + 6, NULL, 10);
        }
    }
    
    trigger_arm(TRIG_SOURCE_EXTERNAL, edge, delay_us);
    cli_printf("Trigger armed: edge=%d, delay=%luµs\r\n", edge, delay_us);
}

static void cmd_fire_laser(int argc, char **argv)
{
    float power = 50.0f;
    uint32_t duration = 50;  /* ms */
    
    if (argc >= 2) {
        char *p = strstr(argv[1], "power=");
        if (p) power = strtof(p + 6, NULL);
        char *d = strstr(argv[1], "duration=");
        if (d) duration = strtoul(d + 9, NULL, 10);
    }
    if (argc >= 3) {
        char *d = strstr(argv[2], "duration=");
        if (d) duration = strtoul(d + 9, NULL, 10);
    }
    
    if (!laser_is_interlock_ok()) {
        cli_print("ERROR: Laser interlock not engaged\r\n");
        return;
    }
    
    if (laser_fire(power, duration)) {
        cli_printf("Laser fired: %.0f%% for %lums\r\n", power, duration);
    } else {
        cli_print("ERROR: Could not fire laser\r\n");
    }
}

static void cmd_read_temp(int argc, char **argv)
{
    (void)argc; (void)argv;
    temp_readings_t temps;
    temp_read_all(&temps);
    cli_printf("  IR: %.1f°C  Plate: %.1f°C  MCU: %.1f°C\r\n",
               temps.ir_temp, temps.plate_temp, temps.internal);
    cli_printf("  Target: %.1f°C  Mode: %d\r\n",
               thermal_ctl_get_target(), thermal_ctl_get_mode());
    cli_printf("  TEC duty: %.1f%%  Current: %umA\r\n",
               tec_get_duty(), tec_read_current_ma());
}

static void cmd_stop(int argc, char **argv)
{
    (void)argc; (void)argv;
    thermal_ctl_stop();
    trigger_disarm();
    cli_print("All outputs stopped\r\n");
}

static void cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    cli_printf("=== Thermal Phantom Status ===\r\n");
    cli_printf("Firmware: %s\r\n", FIRMWARE_VERSION);
    cli_printf("Author: %s\r\n", AUTHOR_NAME);
    cli_printf("Safety: %s\r\n", safety_get_status_str());
    cli_printf("Battery: %umV (%u%%)\r\n",
               safety_read_battery_mv(), safety_get_battery_pct());
    cli_printf("Mode: %d  Target: %.1f°C\r\n",
               thermal_ctl_get_mode(), thermal_ctl_get_target());
    temp_readings_t temps;
    temp_read_all(&temps);
    cli_printf("IR: %.1f°C  Plate: %.1f°C\r\n", temps.ir_temp, temps.plate_temp);
    cli_printf("TEC: %.1f%%  Fault: %d\r\n", tec_get_duty(), tec_is_fault());
    cli_printf("Laser: Interlock=%d  Shutter=%d\r\n",
               laser_is_interlock_ok(), laser_is_shutter_open());
    cli_printf("Trigger: Armed=%d  Pending=%d\r\n",
               trigger_is_armed(), trigger_is_pending());
    if (thermal_ctl_is_profile_running()) {
        cli_printf("Profile: Step %d/%d  Elapsed: %lums\r\n",
                   thermal_ctl_get_profile_step(),
                   active_profile.step_count,
                   thermal_ctl_get_profile_step_time_ms());
    }
}

static void cmd_pid(int argc, char **argv)
{
    if (argc < 2) {
        float kp, ki, kd;
        thermal_ctl_get_pid(&kp, &ki, &kd);
        cli_printf("Current PID: Kp=%.2f Ki=%.2f Kd=%.2f\r\n", kp, ki, kd);
        cli_print("Usage: pid <kp> <ki> <kd>\r\n");
        return;
    }
    if (argc >= 4) {
        float kp = strtof(argv[1], NULL);
        float ki = strtof(argv[2], NULL);
        float kd = strtof(argv[3], NULL);
        thermal_ctl_set_pid(kp, ki, kd);
        cli_printf("PID set: Kp=%.2f Ki=%.2f Kd=%.2f\r\n", kp, ki, kd);
    }
}

static void cmd_version(int argc, char **argv)
{
    (void)argc; (void)argv;
    cli_printf("Thermal Phantom v%s\r\n", FIRMWARE_VERSION);
    cli_printf("Author: %s\r\n", AUTHOR_NAME);
    cli_printf("Build: %s %s\r\n", __DATE__, __TIME__);
}

static void cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    cli_print("=== Thermal Phantom CLI ===\r\n");
    cli_print("Commands:\r\n");
    cli_print("  set_temp <c>       - Set target temperature\r\n");
    cli_print("  set_mode <mode>     - heat|cool|idle|laser\r\n");
    cli_print("  arm_trigger <src> <edge> [delay=Nus]\r\n");
    cli_print("  fire_laser [power=N%%] [duration=Nms]\r\n");
    cli_print("  read_temp           - Read all sensors\r\n");
    cli_print("  status              - Full system status\r\n");
    cli_print("  pid [kp ki kd]      - Get/set PID parameters\r\n");
    cli_print("  stop                - Emergency stop all\r\n");
    cli_print("  version             - Firmware version\r\n");
    cli_print("  help                - This message\r\n");
}

/* ============================================================
 * Command table
 * ============================================================ */

typedef struct {
    const char *name;
    void (*handler)(int argc, char **argv);
    const char *help;
} cli_command_t;

static const cli_command_t commands[] = {
    {"set_temp",    cmd_set_temp,    "Set target temperature"},
    {"set_mode",    cmd_set_mode,    "Set operating mode"},
    {"arm_trigger", cmd_arm_trigger, "Arm trigger"},
    {"fire_laser",  cmd_fire_laser,  "Fire laser pulse"},
    {"read_temp",   cmd_read_temp,   "Read temperatures"},
    {"stop",        cmd_stop,        "Stop all outputs"},
    {"status",      cmd_status,      "System status"},
    {"pid",         cmd_pid,         "PID tuning"},
    {"version",     cmd_version,     "Firmware version"},
    {"help",        cmd_help,        "Show help"},
};

/* ============================================================
 * Command parsing and dispatch
 * ============================================================ */

static void cli_process_line(char *line)
{
    char *argv[CLI_MAX_ARGS];
    int argc = 0;
    char *token;
    
    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') return;
    
    /* Tokenize */
    token = strtok(line, " \t\r\n");
    while (token != NULL && argc < CLI_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    
    if (argc == 0) return;
    
    /* Find and execute command */
    for (int i = 0; i < (int)(sizeof(commands)/sizeof(commands[0])); i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }
    
    cli_printf("Unknown command: %s (type 'help')\r\n", argv[0]);
}

/* ============================================================
 * Public API
 * ============================================================ */

void cli_init(void)
{
    cli_buf_index = 0;
    cli_initialized = true;
    cli_print("\r\n");
    cli_print("=== Thermal Phantom v");
    cli_print(FIRMWARE_VERSION);
    cli_print(" by ");
    cli_print(AUTHOR_NAME);
    cli_print(" ===\r\n");
    cli_print("Type 'help' for commands.\r\n");
    cli_print("thermal> ");
}

void cli_process(void)
{
    if (!cli_initialized) return;
    
    uint32_t uart_base = USART3_BASE;
    
    /* Check for received characters */
    while (USART_ISR(uart_base) & USART_ISR_RXNE) {
        char c = (char)USART_RDR(uart_base);
        
        /* Echo */
        while (!(USART_ISR(uart_base) & USART_ISR_TXE));
        USART_TDR(uart_base) = c;
        
        /* Handle special characters */
        if (c == '\r') {
            cli_buffer[cli_buf_index] = '\0';
            while (!(USART_ISR(uart_base) & USART_ISR_TXE));
            USART_TDR(uart_base) = '\n';
            cli_process_line(cli_buffer);
            cli_buf_index = 0;
            cli_print("thermal> ");
        } else if (c == '\n') {
            /* Ignore bare LF */
        } else if (c == 0x7F || c == 0x08) {  /* Backspace */
            if (cli_buf_index > 0) {
                cli_buf_index--;
                cli_print("\b \b");
            }
        } else if (cli_buf_index < CLI_BUF_SIZE - 1) {
            cli_buffer[cli_buf_index++] = c;
        }
    }
}

/* Author: jayis1 */