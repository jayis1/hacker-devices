/*
 * main.c - Thermal Phantom main firmware entry point
 *
 * System initialization, SysTick timer, main loop scheduler,
 * and command dispatch from communication interface.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "board.h"
#include "registers.h"
#include "thermal_ctl.h"
#include "tec_driver.h"
#include "laser_driver.h"
#include "temp_sensor.h"
#include "trigger.h"
#include "comm.h"
#include "safety.h"
#include "cli.h"
#include <string.h>

/* ============================================================
 * Global state
 * ============================================================ */

volatile uint32_t systick_ms = 0;
static system_state_t system_state = STATE_INIT;
static thermal_profile_t loaded_profile;

/* Sensor cache */
static float last_ir_temp = 25.0f;
static float last_plate_temp = 25.0f;
static uint32_t last_sensor_read_ms = 0;
static uint32_t last_battery_read_ms = 0;
static uint32_t last_safety_tick_ms = 0;
static uint32_t last_heartbeat_ms = 0;

/* ============================================================
 * System clock configuration
 * ============================================================ */

static void clock_init(void)
{
    /* Enable HSE (8 MHz external crystal) */
    uint32_t rcc_base = RCC_BASE;
    
    /* Enable HSE */
    REG32(rcc_base + 0x00) |= (1U << 16);  /* HSEON */
    while (!(REG32(rcc_base + 0x00) & (1U << 17)));  /* Wait for HSERDY */
    
    /* Configure Flash latency for 480 MHz */
    REG32(0x52002000 + 0x00) = 2U;  /* Flash ACR: 2 wait states for VOS1 */
    
    /* Configure PLL1: HSE / 1 * 120 / 2 = 480 MHz */
    REG32(rcc_base + 0x80) = (1U << 0);   /* PLL1 enable HSE source */
    REG32(rcc_base + 0x84) = (0U << 4) |   /* DIVM1 = 1 (no division) */
                             (120U << 8);  /* DIVN1 = 120 (8*120=960) */
    REG32(rcc_base + 0x88) = (1U << 0) |   /* DIVP1 = 2 (960/2=480) */
                             (0U << 8) |   /* DIVQ1 = 1 */
                             (0U << 16);   /* DIVR1 = 1 */
    
    /* Enable PLL1 */
    REG32(rcc_base + 0x80) |= (1U << 24);
    while (!(REG32(rcc_base + 0x80) & (1U << 25)));  /* Wait for PLL1RDY */
    
    /* Switch system clock to PLL1 */
    REG32(rcc_base + 0x18) = (0U << 0);   /* Clear SW */
    REG32(rcc_base + 0x18) = (3U << 0);   /* SW = PLL1 */
    while (((REG32(rcc_base + 0x18) >> 3) & 0x7) != 3);  /* Wait for switch */
}

/* ============================================================
 * SysTick timer - 1ms interrupt
 * ============================================================ */

static void systick_init(void)
{
    /* Configure SysTick for 1ms at 480 MHz */
    REG32(0xE000E014) = 480000 - 1;   /* LOAD */
    REG32(0xE000E018) = 0;            /* VAL = 0 */
    REG32(0xE000E010) = 0x7;          /* ENABLE | TICKINT | CLKSOURCE */
}

void SysTick_Handler(void)
{
    systick_ms++;
    
    /* Fast thermal loop: every 1ms */
    thermal_ctl_fast_tick(last_plate_temp);
}

/* ============================================================
 * GPIO LED init (status indicators)
 * ============================================================ */

static void leds_init(void)
{
    uint32_t pb_base = GPIOB_BASE;
    
    /* Enable GPIOB clock */
    GPIO_MODER(pb_base) &= ~(3U << (LED_STATUS_PIN * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_OUTPUT_PP << (LED_STATUS_PIN * 2));
    GPIO_MODER(pb_base) &= ~(3U << (LED_ARM_PIN * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_OUTPUT_PP << (LED_ARM_PIN * 2));
    GPIO_MODER(pb_base) &= ~(3U << (LED_BLE_PIN * 2));
    GPIO_MODER(pb_base) |= (GPIO_MODE_OUTPUT_PP << (LED_BLE_PIN * 2));
    
    GPIO_CLR(pb_base, LED_STATUS_PIN);
    GPIO_CLR(pb_base, LED_ARM_PIN);
    GPIO_CLR(pb_base, LED_BLE_PIN);
}

/* ============================================================
 * Command handler (from comm.c)
 * ============================================================ */

static void handle_command(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint8_t resp_data[128];
    
    switch (cmd) {
    case CMD_SET_TEMP: {
        if (len >= 4) {
            int16_t temp_raw = data[0] | ((int16_t)data[1] << 8);
            float temp = temp_raw * 0.1f;
            thermal_ctl_set_target(temp);
            thermal_ctl_set_mode(temp > mlx_get_temp() ? 
                                  THERMAL_MODE_HEAT : THERMAL_MODE_COOL);
            resp_data[0] = ERR_NONE;
            comm_send_response(RESP_OK, resp_data, 1);
        }
        break;
    }
    
    case CMD_SET_MODE: {
        if (len >= 1) {
            thermal_mode_t mode = (thermal_mode_t)data[0];
            thermal_ctl_set_mode(mode);
            system_state = (mode == THERMAL_MODE_IDLE) ? STATE_IDLE : 
                           (mode == THERMAL_MODE_HEAT) ? STATE_HEATING :
                           (mode == THERMAL_MODE_COOL) ? STATE_COOLING :
                           STATE_IDLE;
            resp_data[0] = ERR_NONE;
            comm_send_response(RESP_OK, resp_data, 1);
        }
        break;
    }
    
    case CMD_ARM_TRIGGER: {
        if (len >= 4) {
            trigger_source_t src = (trigger_source_t)data[0];
            trigger_edge_t edge = (trigger_edge_t)data[1];
            uint32_t delay = data[2] | ((uint32_t)data[3] << 8);
            trigger_arm(src, edge, delay);
            system_state = STATE_TRIGGER_ARMED;
            resp_data[0] = ERR_NONE;
            comm_send_response(RESP_OK, resp_data, 1);
        }
        break;
    }
    
    case CMD_FIRE_LASER: {
        if (len >= 5) {
            float power;
            uint32_t duration;
            memcpy(&power, data, 4);
            duration = data[4] | ((uint32_t)data[5] << 8) |
                       ((uint32_t)data[6] << 16) | ((uint32_t)data[7] << 24);
            if (laser_fire(power, duration)) {
                system_state = STATE_LASER_FIRING;
                resp_data[0] = ERR_NONE;
            } else {
                resp_data[0] = ERR_LASER_INTERLOCK;
            }
            comm_send_response(RESP_OK, resp_data, 1);
        }
        break;
    }
    
    case CMD_READ_TEMP: {
        temp_readings_t temps;
        temp_read_all(&temps);
        memcpy(resp_data, &temps.ir_temp, 4);
        memcpy(&resp_data[4], &temps.plate_temp, 4);
        memcpy(&resp_data[8], &temps.internal, 4);
        comm_send_response(RESP_DATA, resp_data, 12);
        break;
    }
    
    case CMD_LOAD_PROFILE: {
        if (len >= sizeof(thermal_profile_t)) {
            if (thermal_ctl_load_profile((const thermal_profile_t *)data)) {
                resp_data[0] = ERR_NONE;
            } else {
                resp_data[0] = ERR_PROFILE_INVALID;
            }
            comm_send_response(RESP_OK, resp_data, 1);
        }
        break;
    }
    
    case CMD_START_PROFILE: {
        if (thermal_ctl_start_profile()) {
            system_state = STATE_PROFILE_RUNNING;
            resp_data[0] = ERR_NONE;
        } else {
            resp_data[0] = ERR_NOT_READY;
        }
        comm_send_response(RESP_OK, resp_data, 1);
        break;
    }
    
    case CMD_STOP: {
        thermal_ctl_stop();
        trigger_disarm();
        system_state = STATE_IDLE;
        resp_data[0] = ERR_NONE;
        comm_send_response(RESP_OK, resp_data, 1);
        break;
    }
    
    case CMD_GET_STATUS: {
        comm_send_status();
        break;
    }
    
    case CMD_SET_PID: {
        if (len >= 12) {
            float kp, ki, kd;
            memcpy(&kp, data, 4);
            memcpy(&ki, data + 4, 4);
            memcpy(&kd, data + 8, 4);
            thermal_ctl_set_pid(kp, ki, kd);
            resp_data[0] = ERR_NONE;
            comm_send_response(RESP_OK, resp_data, 1);
        }
        break;
    }
    
    case CMD_DISARM: {
        trigger_disarm();
        system_state = STATE_IDLE;
        resp_data[0] = ERR_NONE;
        comm_send_response(RESP_OK, resp_data, 1);
        break;
    }
    
    case CMD_LASER_SHUTTER_OPEN: {
        if (laser_open_shutter()) {
            resp_data[0] = ERR_NONE;
        } else {
            resp_data[0] = ERR_LASER_INTERLOCK;
        }
        comm_send_response(RESP_OK, resp_data, 1);
        break;
    }
    
    case CMD_LASER_SHUTTER_CLOSE: {
        laser_close_shutter();
        resp_data[0] = ERR_NONE;
        comm_send_response(RESP_OK, resp_data, 1);
        break;
    }
    
    case CMD_READ_BATTERY: {
        uint16_t mv = safety_read_battery_mv();
        uint16_t pct = safety_get_battery_pct();
        resp_data[0] = mv & 0xFF;
        resp_data[1] = (mv >> 8) & 0xFF;
        resp_data[2] = pct & 0xFF;
        resp_data[3] = (pct >> 8) & 0xFF;
        comm_send_response(RESP_DATA, resp_data, 4);
        break;
    }
    
    case CMD_SET_TRIGGER_CONFIG: {
        if (len >= sizeof(trigger_config_t)) {
            trigger_set_config((const trigger_config_t *)data);
            resp_data[0] = ERR_NONE;
            comm_send_response(RESP_OK, resp_data, 1);
        }
        break;
    }
    
    case CMD_GET_VERSION: {
        const char *ver = FIRMWARE_VERSION;
        uint16_t vlen = strlen(ver);
        if (vlen > sizeof(resp_data)) vlen = sizeof(resp_data);
        memcpy(resp_data, ver, vlen);
        comm_send_response(RESP_DATA, resp_data, vlen);
        break;
    }
    
    default:
        resp_data[0] = ERR_UNKNOWN_CMD;
        comm_send_response(RESP_ERROR, resp_data, 1);
        break;
    }
}

/* ============================================================
 * Accessor functions for external modules
 * ============================================================ */

system_state_t get_system_state(void)
{
    return system_state;
}

float get_current_temp(void)
{
    return last_ir_temp;
}

float get_target_temp(void)
{
    return thermal_ctl_get_target();
}

uint16_t get_battery_pct(void)
{
    return safety_get_battery_pct();
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void)
{
    /* Initialize hardware */
    clock_init();
    systick_init();
    leds_init();
    
    /* Initialize subsystems */
    safety_init();
    temp_sensor_init();
    tec_init();
    laser_init();
    trigger_init();
    thermal_ctl_init();
    comm_init();
    cli_init();
    
    /* Register command handler */
    comm_register_handler(handle_command);
    
    /* Set initial state */
    system_state = STATE_IDLE;
    
    /* Enable interrupts */
    __ISB();
    
    /* Main loop */
    uint32_t last_fast_loop = 0;
    
    while (1) {
        uint32_t now = systick_ms;
        
        /* --- Process communication (every iteration) --- */
        comm_process();
        cli_process();
        
        /* --- Check laser timeout --- */
        extern bool laser_check_timeout(void);
        laser_check_timeout();
        
        /* --- Temperature sensor read (every 50ms) --- */
        if (now - last_sensor_read_ms >= TEMP_SENSOR_READ_MS) {
            last_sensor_read_ms = now;
            
            float ir_temp, plate_temp;
            if (mlx_read_temp(&ir_temp)) {
                last_ir_temp = ir_temp;
            }
            if (ds18b20_read_temp(&plate_temp)) {
                last_plate_temp = plate_temp;
            }
        }
        
        /* --- Outer thermal PID loop (every 100ms) --- */
        if (now - last_fast_loop >= THERMAL_LOOP_PERIOD_MS) {
            last_fast_loop = now;
            thermal_ctl_slow_tick(last_ir_temp);
        }
        
        /* --- Safety check (every 10ms) --- */
        if (now - last_safety_tick_ms >= SAFETY_CHECK_MS) {
            last_safety_tick_ms = now;
            safety_tick();
            safety_feed_watchdog();
            
            /* Update system state based on safety */
            if (!safety_is_ok() && system_state != STATE_EMERGENCY_STOP) {
                system_state = STATE_FAULT;
            }
        }
        
        /* --- Battery check (every 5s) --- */
        if (now - last_battery_read_ms >= BATTERY_READ_MS) {
            last_battery_read_ms = now;
            uint16_t bat = safety_get_battery_pct();
            if (bat < BAT_WARN_PCT) {
                comm_send_log("WARNING: Battery low");
            }
        }
        
        /* --- Heartbeat (every 1s) --- */
        if (now - last_heartbeat_ms >= COMM_HEARTBEAT_MS) {
            last_heartbeat_ms = now;
            /* Toggle BLE LED */
            GPIO_WRITE(GPIOB_BASE, LED_BLE_PIN, 
                       comm_is_ble_connected() ? 1 : 0);
        }
        
        /* --- Low power: wait for next interrupt --- */
        __WFI();
    }
}