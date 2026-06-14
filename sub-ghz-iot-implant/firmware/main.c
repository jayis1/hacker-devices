/*
 * main.c — Substation Sub-GHz IoT Gateway Implant
 * Main application entry point and task management
 *
 * CC1352P-based multi-protocol Sub-GHz attack platform
 * Target: ARM Cortex-M4F @ 48 MHz
 */

#include "board.h"
#include "registers.h"
#include <string.h>
#include <stdint.h>

/* Driver headers */
#include "drivers/radio_subghz.h"
#include "drivers/sdram.h"
#include "drivers/ble_comms.h"

/* ============================================
 * Application State
 * ============================================ */
typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_IDLE,
    APP_STATE_SNIFFING,
    APP_STATE_TX,
    APP_STATE_MITM_ZIGBEE,
    APP_STATE_MITM_ZWAVE,
    APP_STATE_ROLLING_CAPTURE,
    APP_STATE_USB_CONNECTED,
    APP_STATE_ERROR
} app_state_t;

static volatile app_state_t g_state = APP_STATE_INIT;
static volatile uint32_t g_pkt_count = 0;
static volatile uint32_t g_uptime_ms = 0;
static radio_config_t g_radio_cfg;
static sdram_ring_t g_capture_ring;

/* Simple FreeRTOS-like task scheduler (bare-metal) */
typedef void (*task_func_t)(void);

typedef struct {
    task_func_t func;
    uint32_t    period_ms;
    uint32_t    last_run_ms;
} task_t;

/* Forward declarations */
static void task_radio(void);
static void task_ble(void);
static void task_usb(void);
static void task_led(void);
static void task_buttons(void);
static void task_watchdog(void);
static void radio_rx_callback(const radio_packet_t *pkt);

/* Task table (ordered by priority) */
static task_t tasks[] = {
    { task_radio,      1,    0 },   /* Radio: 1 ms (event-driven) */
    { task_ble,        10,   0 },   /* BLE: 10 ms */
    { task_usb,        10,   0 },   /* USB: 10 ms */
    { task_buttons,    50,   0 },   /* Buttons: 50 ms (debounce) */
    { task_led,        100,  0 },   /* LED: 100 ms */
    { task_watchdog,   1000, 0 },   /* Watchdog: 1 s */
};

#define NUM_TASKS (sizeof(tasks) / sizeof(tasks[0]))

/* ============================================
 * System Tick (1 ms)
 * ============================================ */
static volatile uint32_t systick_count = 0;

void SysTick_Handler(void) {
    systick_count++;
    g_uptime_ms++;
}

/* ============================================
 * Main Entry Point
 * ============================================ */
int main(void) {
    /* Stage 1: Initialize hardware */
    board_init_clocks();
    board_init_gpio();

    /* Stage 2: Initialize peripherals */
    board_init_peripherals();

    /* Stage 3: Initialize SDRAM */
    if (sdram_init() != 0) {
        /* SDRAM init failed — blink LED fast RED */
        while (1) {
            board_led_set(0x400000);
            for (volatile int i = 0; i < 500000; i++);
            board_led_off();
            for (volatile int i = 0; i < 500000; i++);
        }
    }

    /* Run SDRAM self-test */
    sdram_test();

    /* Stage 4: Initialize capture ring buffer */
    sdram_ring_init(&g_capture_ring, SDRAM_RING_SIZE);

    /* Stage 5: Initialize radio */
    radio_subghz_init();

    /* Stage 6: Configure default radio settings (868 MHz, FSK) */
    g_radio_cfg.frequency = RADIO_DEFAULT_FREQ;
    g_radio_cfg.tx_power = RADIO_DEFAULT_POWER;
    g_radio_cfg.baud_rate = 50000;
    g_radio_cfg.modulation = 2;  /* GFSK */
    g_radio_cfg.deviation = 25000;
    g_radio_cfg.channel_bw = 100000;
    g_radio_cfg.sync_word[0] = 0xAA;
    g_radio_cfg.sync_word[1] = 0xBB;
    g_radio_cfg.sync_word[2] = 0xCC;
    g_radio_cfg.sync_word[3] = 0xDD;
    g_radio_cfg.sync_len = 32;

    radio_subghz_configure(&g_radio_cfg);

    /* Stage 7: Initialize BLE */
    ble_comms_init();
    ble_comms_start_advertising(BLE_DEVICE_NAME);

    /* Stage 8: Initialize SysTick (1 ms period) */
    /* SysTick = System clock / 1000 = 48000 ticks */
    REG32(0xE000E014) = 48000 - 1;    /* Reload value */
    REG32(0xE000E018) = 0;             /* Current value */
    REG32(0xE000E010) = 0x07;          /* Enable, tick interrupt, processor clock */

    /* Stage 9: Enable global interrupts */
    __asm volatile ("cpsie i" ::: "memory");

    /* Set initial state */
    g_state = APP_STATE_IDLE;
    board_led_set(LED_COLOR_IDLE);

    /* ============================================
     * Main Super-Loop
     * ============================================ */
    while (1) {
        uint32_t now = systick_count;

        for (int i = 0; i < NUM_TASKS; i++) {
            if ((now - tasks[i].last_run_ms) >= tasks[i].period_ms) {
                tasks[i].func();
                tasks[i].last_run_ms = now;
            }
        }

        /* Low-power wait for next tick */
        __asm volatile ("wfi" ::: "memory");
    }

    return 0;  /* Never reached */
}

/* ============================================
 * Radio Task — Process received packets
 * ============================================ */
static void task_radio(void) {
    /* Check for packets in capture ring */
    uint8_t buf[256];
    int len = sdram_ring_read(&g_capture_ring, buf, sizeof(buf));

    if (len > 0) {
        /* Forward to BLE if connected */
        if (ble_comms_get_state() == BLE_STATE_CONNECTED ||
            ble_comms_get_state() == BLE_STATE_STREAMING) {
            ble_comms_send_capture(buf, (uint16_t)len);
        }
    }
}

/* ============================================
 * BLE Task — Handle BLE communication
 * ============================================ */
static void task_ble(void) {
    ble_comms_task();

    /* Send status update every second */
    static uint32_t last_status_ms = 0;
    if ((g_uptime_ms - last_status_ms) >= 1000) {
        ble_status_t status = {
            .mode = (uint8_t)g_state,
            .freq = g_radio_cfg.frequency,
            .rssi = 0,  /* Updated by radio callback */
            .pkt_count = g_pkt_count,
            .uptime_s = g_uptime_ms / 1000,
            .battery_pct = 0,  /* TODO: ADC read */
        };
        ble_comms_send_status(&status);
        last_status_ms = g_uptime_ms;
    }
}

/* ============================================
 * USB Task — Handle USB CDC-ECM communication
 * ============================================ */
static void task_usb(void) {
    /* TODO: Implement USB CDC-ECM packet streaming */
}

/* ============================================
 * LED Task — Update status LED
 * ============================================ */
static void task_led(void) {
    static uint32_t blink_count = 0;

    switch (g_state) {
        case APP_STATE_IDLE:
            /* Slow pulse blue */
            board_led_set(LED_COLOR_IDLE);
            break;
        case APP_STATE_SNIFFING:
            /* Steady green */
            board_led_set(LED_COLOR_SNIFFING);
            break;
        case APP_STATE_TX:
            /* Blink red */
            if (blink_count % 2 == 0) {
                board_led_set(LED_COLOR_TX);
            } else {
                board_led_off();
            }
            blink_count++;
            break;
        case APP_STATE_MITM_ZIGBEE:
        case APP_STATE_MITM_ZWAVE:
            /* Fast blink cyan */
            if (blink_count % 2 == 0) {
                board_led_set(LED_COLOR_BLE_CONN);
            } else {
                board_led_off();
            }
            blink_count++;
            break;
        case APP_STATE_USB_CONNECTED:
            /* Steady purple */
            board_led_set(LED_COLOR_USB_CONN);
            break;
        case APP_STATE_ERROR:
            /* Fast blink red */
            if (blink_count % 2 == 0) {
                board_led_set(0xFF0000);
            } else {
                board_led_off();
            }
            blink_count++;
            break;
        default:
            board_led_off();
            break;
    }
}

/* ============================================
 * Button Task — Handle user input
 * ============================================ */
static uint32_t btn_mode_press_ms = 0;
static uint32_t btn_action_press_ms = 0;
static bool btn_mode_pressed = false;
static bool btn_action_pressed = false;

static void task_buttons(void) {
    /* Read button states (active low) */
    uint32_t gpio_in = GPIO_DIN0;
    bool mode_now = !(gpio_in & (1 << PIN_BTN_MODE));
    bool action_now = !(gpio_in & (1 << PIN_BTN_ACTION));

    /* Mode button: Cycle through operating modes */
    if (mode_now && !btn_mode_pressed) {
        /* Button press detected */
        btn_mode_press_ms = g_uptime_ms;
        btn_mode_pressed = true;
    } else if (!mode_now && btn_mode_pressed) {
        /* Button released */
        btn_mode_pressed = false;

        /* Cycle mode */
        switch (g_state) {
            case APP_STATE_IDLE:
                /* Start Sub-GHz sniffing */
                g_state = APP_STATE_SNIFFING;
                radio_subghz_configure(&g_radio_cfg);
                radio_subghz_start_rx(radio_rx_callback);
                break;
            case APP_STATE_SNIFFING:
                /* Stop sniffing, go to idle */
                radio_subghz_stop_rx();
                g_state = APP_STATE_IDLE;
                break;
            default:
                g_state = APP_STATE_IDLE;
                radio_subghz_stop_rx();
                break;
        }
    }

    /* Action button: Perform context-sensitive action */
    if (action_now && !btn_action_pressed) {
        btn_action_press_ms = g_uptime_ms;
        btn_action_pressed = true;
    } else if (!action_now && btn_action_pressed) {
        btn_action_pressed = false;

        switch (g_state) {
            case APP_STATE_SNIFFING:
                /* Transmit last captured packet (replay) */
                /* TODO: Get last packet from ring buffer and transmit */
                break;
            case APP_STATE_IDLE:
                /* Start BLE advertising */
                ble_comms_start_advertising(BLE_DEVICE_NAME);
                break;
            default:
                break;
        }
    }
}

/* ============================================
 * Watchdog Task — Feed the watchdog
 * ============================================ */
static void task_watchdog(void) {
    WDT_LOAD = 0x00FFFFFF;  /* Reload watchdog timer */
}

/* ============================================
 * Radio RX Callback — Called from ISR context
 * ============================================ */
static void radio_rx_callback(const radio_packet_t *pkt) {
    g_pkt_count++;

    /* Store packet in SDRAM ring buffer */
    sdram_ring_write(&g_capture_ring,
                     (const uint8_t *)pkt,
                     sizeof(radio_packet_header_t) + pkt->length);

    /* Update RSSI in status */
    /* (could also push to a queue for BLE streaming) */
}

/* ============================================
 * Board LED Control (WS2812B)
 * ============================================ */
void board_led_set(uint32_t color) {
    /* Send 24-bit color to WS2812B via bit-bang on DIO_16 */
    /* WS2812B protocol: ~800 kHz, each bit is ~1.25 µs
     * '1': high for 0.8 µs, low for 0.45 µs
     * '0': high for 0.4 µs, low for 0.85 µs
     */
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    /* GRB order for WS2812B */
    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

    __asm volatile ("cpsid i" ::: "memory");  /* Disable interrupts */

    for (int bit = 23; bit >= 0; bit--) {
        if (grb & (1 << bit)) {
            /* Send '1': high ~0.8µs, low ~0.45µs */
            GPIO_DOUT0 |= (1 << PIN_WS2812B);   /* High */
            for (volatile int d = 0; d < 10; d++);  /* ~0.8 µs @ 48 MHz */
            GPIO_DOUT0 &= ~(1 << PIN_WS2812B);  /* Low */
            for (volatile int d = 0; d < 5; d++);   /* ~0.45 µs */
        } else {
            /* Send '0': high ~0.4µs, low ~0.85µs */
            GPIO_DOUT0 |= (1 << PIN_WS2812B);   /* High */
            for (volatile int d = 0; d < 5; d++);   /* ~0.4 µs */
            GPIO_DOUT0 &= ~(1 << PIN_WS2812B);  /* Low */
            for (volatile int d = 0; d < 10; d++);  /* ~0.85 µs */
        }
    }

    __asm volatile ("cpsie i" ::: "memory");  /* Enable interrupts */

    /* Reset latch: low for > 50 µs */
    for (volatile int d = 0; d < 2400; d++);  /* ~50 µs */
}

void board_led_off(void) {
    board_led_set(0x000000);
}

/* ============================================
 * Hard Fault Handler (debug)
 * ============================================ */
void HardFault_Handler(void) {
    /* Blink LED rapidly on hard fault */
    while (1) {
        board_led_set(0xFF0000);  /* Solid red */
        for (volatile int i = 0; i < 200000; i++);
        board_led_off();
        for (volatile int i = 0; i < 200000; i++);
    }
}

void NMI_Handler(void)    __attribute__((weak, alias("HardFault_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("HardFault_Handler")));
void BusFault_Handler(void)  __attribute__((weak, alias("HardFault_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("HardFault_Handler")));