/*
 * ACOUSTIC-PHANTOM — Main firmware entry point
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * This file orchestrates the acoustic side-channel capture pipeline:
 *   1. Audio capture from MEMS mic array + contact piezo
 *   2. DSP pipeline (Framing → FFT → MFCC/Spectral envelope)
 *   3. Event detection (energy onset)
 *   4. TFLM inference (classification)
 *   5. BLE event streaming to companion app
 *   6. Optional raw audio storage to microSD
 *
 * The scheduler is cooperative — each task runs to completion and
 * yields. The audio capture is interrupt-driven (I²S DMA half/full
 * transfer) and has strict real-time priority.
 */

#include <string.h>
#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "drivers/audio_capture.h"
#include "drivers/dsp_pipeline.h"
#include "drivers/beamformer.h"
#include "drivers/event_detector.h"
#include "drivers/tflm_inference.h"
#include "drivers/ble_bridge.h"
#include "drivers/storage.h"
#include "drivers/tamper.h"
#include "drivers/ui.h"

/* ---- Global device state ----------------------------------------------- */
static volatile device_state_t g_state = STATE_IDLE;
static attack_profile_t g_profile = PROFILE_KEYBOARD;
static uint8_t g_ble_connected = 0;
static uint32_t g_event_count = 0;
static uint32_t g_capture_start_ms = 0;
static char g_passkey[7] = "000000";  /* BLE pairing passkey */

/* ---- SysTick counter (1 kHz) ------------------------------------------- */
static volatile uint32_t g_ticks = 0;

void SysTick_Handler(void)
{
    g_ticks++;
}

uint32_t board_millis(void)
{
    return g_ticks;
}

void board_delay_ms(uint32_t ms)
{
    uint32_t start = g_ticks;
    while ((g_ticks - start) < ms) {
        /* wait */
    }
}

/* ---- Clock configuration ------------------------------------------------
 * HSE 8 MHz → PLL1 → 480 MHz SYSCLK
 *   PLL1: M=2, N=120, P=1 → VCO=480MHz, SYSCLK=480MHz
 *   AHB=480, APB1=120, APB2=240
 * ------------------------------------------------------------------------ */
static void clock_init(void)
{
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* Configure PLL1: M=2, N=120, P=1, Q=20, R=20 */
    RCC->PLLCKSELR = (2U << 0) | (1U << 4);   /* DIVM=2, REFCLK=HSE */
    RCC->PLLCFGR   = (1U << 1) | (120U << 8) | (0U << 17) | (20U << 24);
    RCC->PLL1DIVR  = (1U << 0) | (0U << 9) | (19U << 16) | (19U << 25);

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY)) { }

    /* Configure bus dividers: D1CPRE=1, HPRE=2, D1PPRE=2, D2PPRE1=2, D2PPRE2=2 */
    RCC->D1CFGR = (0U << 0) | (8U << 4) | (8U << 8);  /* D1CPRE=1, HPRE=2, D1PPRE=4 */
    RCC->D2CFGR = (8U << 0) | (8U << 8) | (8U << 12); /* D2PPRE1=4, D2PPRE2=4 */

    /* Switch SYSCLK to PLL1 */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & 0x07U) != 0x03U) { }

    /* Enable peripheral clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOA | RCC_AHB4ENR_GPIOB |
                    RCC_AHB4ENR_GPIOC | RCC_AHB4ENR_GPIOD |
                    RCC_AHB4ENR_GPIOE;
    RCC->AHB3ENR |= RCC_AHB3ENR_QSPI;
    RCC->APB2ENR |= RCC_APB2ENR_USART1 | RCC_APB2ENR_SPI3 | RCC_APB2ENR_TIM2;
    RCC->APB1LENR |= RCC_APB1LENR_I2C1 | RCC_APB1LENR_SPI2;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1 | RCC_AHB1ENR_DMA2 | RCC_AHB1ENR_SDMMC1;
}

/* ---- SysTick init ------------------------------------------------------ */
static void systick_init(void)
{
    SysTick->LOAD = (SYSTEM_CLOCK_HZ / SYSTICK_HZ) - 1;
    SysTick->VAL = 0;
    SysTick->CSR = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT |
                   SysTick_CTRL_CLKSRC;
}

/* ---- GPIO configuration ------------------------------------------------ */
static void gpio_init(void)
{
    /* PA9/PA10 = USART1 TX/RX (BLE) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (9*2))) | (GPIO_MODE_AF << (9*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (10*2))) | (GPIO_MODE_AF << (10*2));
    GPIOA->AFRL = 0;
    GPIOA->AFRH = (GPIOA->AFHR & ~(0xFFUL << (1*4))) |
                  (BLE_UART_AF << (1*4)) | (BLE_UART_AF << (2*4));

    /* PB12 = I2S2_CK, PB14 = I2S2_SD (MEMS mic array) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (12*2))) | (GPIO_MODE_AF << (12*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (14*2))) | (GPIO_MODE_AF << (14*2));
    GPIOB->AFRH = (GPIOB->AFHR & ~(0xFFUL << (4*4))) |
                  (I2S2_AF << (4*4)) | (I2S2_AF << (6*4));

    /* PB3 = I2S3_CK, PB15 = I2S3_SD, PA4 = I2S3_WS (WM8904 codec) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (3*2))) | (GPIO_MODE_AF << (3*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (15*2))) | (GPIO_MODE_AF << (15*2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFUL << (3*4))) | (I2S3_AF << (3*4));
    GPIOB->AFRH = (GPIOB->AFHR & ~(0xFUL << (7*4))) | (I2S3_AF << (7*4));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (4*2))) | (GPIO_MODE_AF << (4*2));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFUL << (4*4))) | (I2S3_AF << (4*4));

    /* PB6 = I2C1_SCL, PB7 = I2C1_SDA */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (6*2))) | (GPIO_MODE_AF << (6*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (7*2))) | (GPIO_MODE_AF << (7*2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFFUL << (6*4))) |
                  (I2C1_AF << (6*4)) | (I2C1_AF << (7*4));

    /* PA0, PC1, PC13 = buttons (input with pull-up) */
    GPIOA->MODER &= ~(3U << (0*2));  /* input */
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (0*2))) | (GPIO_PUPD_PULLUP << (0*2));
    GPIOC->MODER &= ~(3U << (1*2));
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(3U << (1*2))) | (GPIO_PUPD_PULLUP << (1*2));
    GPIOC->MODER &= ~(3U << (13*2));
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(3U << (13*2))) | (GPIO_PUPD_PULLUP << (13*2));

    /* PC15 = tamper interrupt (input with pull-up, falling edge) */
    GPIOC->MODER &= ~(3U << (15*2));
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(3U << (15*2))) | (GPIO_PUPD_PULLUP << (15*2));
    EXTI->IMR1 |= (1U << 15);
    EXTI->FTSR1 |= (1U << 15);
    NVIC_ISER0 |= (1U << EXTI15_10_IRQn);

    /* PA5 = WS2812B output (TIM2_CH1 PWM) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (5*2))) | (GPIO_MODE_AF << (5*2));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFUL << (5*4))) | (1U << (5*4));  /* AF1=TIM2 */
}

void board_init(void)
{
    clock_init();
    systick_init();
    gpio_init();

    /* Subsystem init */
    ui_init();
    ble_bridge_init(g_passkey);
    storage_init(g_passkey);
    audio_capture_init();
    beamformer_init();
    dsp_pipeline_init(g_profile);
    event_detector_init(g_profile);
    tflm_inference_init(g_profile);
    tamper_init();
}

/* ---- Tamper handler ---------------------------------------------------- */
static void on_tamper(void)
{
    /* Zeroize all sensitive state */
    audio_capture_wipe();
    storage_wipe_buffers();
    dsp_pipeline_reset();
    event_detector_reset();
    tflm_inference_reset();

    g_state = STATE_TAMPER;
    led_set(0, 255, 0, 0);  /* red = tamper */
    led_set(1, 255, 0, 0);
    led_set(2, 255, 0, 0);
    led_update();
    ui_show_tamper();

    /* Wait for power cycle — do not resume */
    while (1) { }
}

/* EXTI15_10 handler (tamper on PC15) */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR1 & (1U << 15)) {
        EXTI->PR1 = (1U << 15);  /* clear */
        on_tamper();
    }
}

/* ---- Button handling (debounced) --------------------------------------- */
static uint8_t btn_debounce[3] = {0};

uint8_t btn_pressed(uint8_t idx)
{
    uint32_t pin;
    GPIO_TypeDef *port;
    switch (idx) {
        case 0: port = GPIOA; pin = BTN_POWER_PIN;  break;
        case 1: port = GPIOC; pin = BTN_PROFILE_PIN; break;
        case 2: port = GPIOC; pin = BTN_ARM_PIN;    break;
        default: return 0;
    }
    /* Active low (pull-up, button to GND) */
    uint8_t raw = !(port->IDR & (1U << pin));
    /* Simple debounce: 3 consecutive reads */
    if (raw) {
        btn_debounce[idx]++;
        if (btn_debounce[idx] > 3) {
            btn_debounce[idx] = 0;
            return 1;
        }
    } else {
        btn_debounce[idx] = 0;
    }
    return 0;
}

/* ---- State transitions ------------------------------------------------- */
static void enter_state(device_state_t new_state)
{
    g_state = new_state;
    switch (new_state) {
        case STATE_IDLE:
            led_set(0, 0, 0, 64);   /* blue = idle */
            led_set(1, 0, 0, 0);
            led_set(2, 0, 0, 0);
            audio_capture_stop();
            ui_show_idle(g_profile, g_event_count);
            break;
        case STATE_ARMED:
            led_set(0, 64, 32, 0);  /* yellow = armed */
            led_set(1, 64, 32, 0);
            led_set(2, 0, 0, 0);
            audio_capture_start();
            g_capture_start_ms = board_millis();
            ui_show_armed(g_profile);
            break;
        case STATE_CAPTURING:
            led_set(0, 0, 64, 0);   /* green = capturing */
            led_set(1, 0, 64, 0);
            led_set(2, 0, 64, 0);
            ui_show_capturing(g_profile, g_event_count);
            break;
        case STATE_CALIBRATING:
            led_set(0, 64, 0, 64);  /* magenta = calibrating */
            led_set(1, 64, 0, 64);
            led_set(2, 64, 0, 64);
            ui_show_calibrating();
            break;
        case STATE_STORAGE:
            led_set(0, 0, 64, 64);  /* cyan = storage mode */
            ui_show_storage();
            break;
        default:
            break;
    }
    led_update();
}

static void cycle_profile(void)
{
    g_profile = (attack_profile_t)((g_profile + 1) % PROFILE_COUNT);
    dsp_pipeline_set_profile(g_profile);
    event_detector_set_profile(g_profile);
    tflm_inference_set_profile(g_profile);
    ui_show_profile(g_profile);
}

/* ---- Main scheduler ----------------------------------------------------
 *
 * The scheduler runs at 1 kHz. It checks for new audio frames from
 * the DMA interrupt, processes them through the DSP + inference pipeline,
 * and handles BLE communication and UI updates. All tasks are cooperative
 * — they must complete within the 1 ms tick.
 *
 * Audio frames arrive every 10 ms (hop interval). The full DSP + inference
 * pipeline takes ~3-5 ms on the Cortex-M7 at 480 MHz, leaving ample
 * headroom.
 */

typedef struct {
    audio_frame_t frame;
    event_t       events[MAX_EVENTS_PER_FRAME];
    int           num_events;
    classification_t results[MAX_EVENTS_PER_FRAME];
} processing_slot_t;

static processing_slot_t g_slot;

static void process_audio_frame(void)
{
    /* Get the latest frame from the capture driver */
    if (!audio_capture_get_frame(&g_slot.frame)) {
        return;  /* no new frame */
    }

    /* Run beamforming on the 4-mic array data */
    beamformer_process(&g_slot.frame);

    /* Run DSP pipeline (FFT → MFCC / spectral envelope) */
    dsp_pipeline_process(&g_slot.frame);

    /* Detect acoustic events in this frame */
    g_slot.num_events = event_detector_process(&g_slot.frame,
                                               g_slot.events,
                                               MAX_EVENTS_PER_FRAME);

    /* Classify each detected event */
    for (int i = 0; i < g_slot.num_events; i++) {
        tflm_inference_classify(&g_slot.events[i], &g_slot.results[i]);
        g_slot.results[i].timestamp_ms = board_millis() - g_capture_start_ms;
        g_event_count++;

        /* Stream result over BLE */
        if (g_ble_connected) {
            ble_bridge_send_event(&g_slot.results[i]);
        }

        /* Optionally store raw audio */
        if (storage_is_enabled()) {
            storage_append_event(&g_slot.events[i], &g_slot.results[i]);
        }
    }

    /* Update display every 200 ms */
    static uint32_t last_ui = 0;
    if ((board_millis() - last_ui) > 200) {
        last_ui = board_millis();
        if (g_state == STATE_ARMED || g_state == STATE_CAPTURING) {
            ui_update_live(g_event_count, g_slot.num_events,
                          beamformer_get_bearing());
        }
    }
}

/* ---- BLE callbacks ----------------------------------------------------- */
static void on_ble_connect(void)
{
    g_ble_connected = 1;
    ui_show_ble_connected();
}

static void on_ble_disconnect(void)
{
    g_ble_connected = 0;
    ui_show_ble_disconnected();
}

static void on_ble_command(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    switch (cmd) {
        case BLE_CMD_SET_PROFILE:
            if (len >= 1 && data[0] < PROFILE_COUNT) {
                g_profile = (attack_profile_t)data[0];
                dsp_pipeline_set_profile(g_profile);
                event_detector_set_profile(g_profile);
                tflm_inference_set_profile(g_profile);
            }
            break;
        case BLE_CMD_ARM:
            enter_state(STATE_ARMED);
            break;
        case BLE_CMD_DISARM:
            enter_state(STATE_IDLE);
            break;
        case BLE_CMD_START_CALIBRATION:
            enter_state(STATE_CALIBRATING);
            tflm_inference_start_calibration();
            break;
        case BLE_CMD_CALIBRATION_SAMPLE:
            if (g_state == STATE_CALIBRATING) {
                tflm_inference_add_calibration_sample(data, len);
            }
            break;
        case BLE_CMD_FINISH_CALIBRATION:
            tflm_inference_finish_calibration();
            storage_save_calibration(g_profile);
            enter_state(STATE_IDLE);
            break;
        case BLE_CMD_ENABLE_STORAGE:
            storage_enable();
            break;
        case BLE_CMD_DISABLE_STORAGE:
            storage_disable();
            break;
        case BLE_CMD_LIST_FILES:
            ble_bridge_send_file_list();
            break;
        case BLE_CMD_DOWNLOAD_FILE:
            if (len >= 4) {
                uint32_t file_idx = data[0] | (data[1] << 8) |
                                    (data[2] << 16) | (data[3] << 24);
                storage_stream_file(file_idx);
            }
            break;
        case BLE_CMD_SET_BEAM:
            if (len >= 2) {
                int16_t angle = data[0] | (data[1] << 8);
                beamformer_set_steering(angle);
            }
            break;
        case BLE_CMD_GET_STATUS:
            ble_bridge_send_status(g_state, g_profile, g_event_count,
                                  storage_get_battery_pct());
            break;
        default:
            break;
    }
}

/* ---- Main loop --------------------------------------------------------- */
int main(void)
{
    /* Initialize all hardware and subsystems */
    board_init();

    /* Register BLE callbacks */
    ble_bridge_set_callbacks(on_ble_connect, on_ble_disconnect,
                             on_ble_command);

    /* Show boot screen */
    ui_show_boot();
    board_delay_ms(1000);

    /* Enter idle state */
    enter_state(STATE_IDLE);

    /* Main super-loop */
    uint32_t last_btn_check = 0;
    while (1) {
        /* 1. Process audio frames (highest priority in the loop) */
        if (g_state == STATE_ARMED || g_state == STATE_CAPTURING) {
            process_audio_frame();
        }

        /* 2. Check BLE events */
        ble_bridge_poll();

        /* 3. Handle file streaming if in progress */
        if (storage_is_streaming()) {
            storage_stream_chunk();
        }

        /* 4. Button handling (every 50 ms) */
        if ((board_millis() - last_btn_check) > 50) {
            last_btn_check = board_millis();

            if (btn_pressed(0)) {
                /* POWER button: cycle idle → armed → idle */
                if (g_state == STATE_IDLE) {
                    enter_state(STATE_ARMED);
                } else if (g_state == STATE_ARMED || g_state == STATE_CAPTURING) {
                    enter_state(STATE_IDLE);
                }
            }
            if (btn_pressed(1)) {
                /* PROFILE button: cycle attack profiles */
                cycle_profile();
            }
            if (btn_pressed(2)) {
                /* ARM button: toggle capture */
                if (g_state == STATE_ARMED) {
                    enter_state(STATE_CAPTURING);
                } else if (g_state == STATE_CAPTURING) {
                    enter_state(STATE_ARMED);
                }
            }
        }

        /* 5. State-specific periodic tasks */
        if (g_state == STATE_CAPTURING) {
            /* Auto-promote armed → capturing when events start flowing */
            if (g_event_count > 0 && g_state == STATE_ARMED) {
                enter_state(STATE_CAPTURING);
            }
        }

        /* 6. Low-power WFI between iterations */
        __asm volatile ("wfi");
    }

    return 0;
}

/* ---- Hard fault handler ------------------------------------------------ */
void HardFault_Handler(void)
{
    /* On hard fault, zeroize and halt */
    audio_capture_wipe();
    storage_wipe_buffers();
    led_set(0, 255, 0, 0);
    led_set(1, 255, 0, 0);
    led_set(2, 255, 0, 0);
    led_update();
    while (1) { }
}

/* ---- FPU access -------------------------------------------------------- */
/* Enable CP10/CP11 full access in CPACR before main() */
void SystemInit(void) __attribute__((constructor(101)));
void SystemInit(void)
{
    volatile uint32_t *cpacr = (volatile uint32_t *)0xE000ED88UL;
    *cpacr |= (0xFU << 20);  /* CP10/CP11 full access */
    __asm volatile ("dsb");
    __asm volatile ("isb");
}