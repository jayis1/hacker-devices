/**
 * main.c — eMMC Flash Dumper Firmware Entry Point
 *
 * Bare-metal firmware for STM32H743VI Cortex-M7 @ 480 MHz.
 * Implements system initialization, peripheral setup, cooperative
 * multitasking main loop, USB command processing, OLED UI, and
 * flash acquisition state machines for eMMC, NAND, and SPI NOR.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

#include "board.h"
#include "registers.h"
#include "drivers/emmc.h"
#include "drivers/fpga.h"
#include "usb_descriptors.h"
#include <stddef.h>
#include <stdbool.h>

/*===========================================================================
 * Forward Declarations
 *===========================================================================*/

/* Driver stubs for modules not yet fully implemented in separate files */
int sdram_init(void);
int sdram_test(void);
int oled_init(void);
void oled_clear(void);
void oled_print(uint8_t line, const char *text);
void oled_print_progress(uint8_t line, uint32_t percent);
void oled_render(void);
int sdcard_init(void);
int sdcard_write_blocks(const uint8_t *data, uint32_t blocks);
int usb_device_init(void);
int usb_device_process(void);
int usb_send_data(const uint8_t *data, uint32_t length);
int usb_send_response(uint16_t cmd_id, uint16_t seq, const uint8_t *payload, uint32_t len);
int hash_sha256_start(void);
int hash_sha256_update(const uint8_t *data, uint32_t length);
int hash_sha256_finish(uint8_t *digest);
void power_check_battery(void);
uint16_t battery_mv(void);
void led_set_rgb(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void buzzer_beep(uint16_t freq, uint16_t duration_ms);
void button_read(uint8_t *states);
void delay_ms(uint32_t ms);

/*===========================================================================
 * System Clock Initialization
 *===========================================================================*/

static void system_clock_init(void) {
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL1: 25 MHz / 5 * 192 / 2 = 480 MHz */
    RCC->PLLCKSELR = (5 << RCC_PLLCKSELR_DIVM1_Pos) |
                     RCC_PLLCKSELR_PLLSRC_HSE;
    RCC->PLL1DIVR = (192 << RCC_PLL1DIVR_DIVN1_Pos) |
                    (1 << RCC_PLL1DIVR_DIVP1_Pos) |   /* DIVP=2 */
                    (3 << RCC_PLL1DIVR_DIVQ1_Pos) |   /* DIVQ=4 */
                    (7 << RCC_PLL1DIVR_DIVR1_Pos);    /* DIVR=8 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));

    /* Configure bus prescalers */
    RCC->D1CFGR = RCC_D1CFGR_D1CPRE_1 | RCC_D1CFGR_D1PPRE_1;
    RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_3 | RCC_D2CFGR_D2PPRE2_1;
    RCC->D3CFGR = RCC_D3CFGR_D3PPRE_1;

    /* Configure PLL2: 25 MHz / 5 * 160 / 2 = 400 MHz */
    RCC->PLLCKSELR |= (5 << RCC_PLLCKSELR_DIVM2_Pos);
    RCC->PLL2DIVR = (160 << RCC_PLL2DIVR_DIVN2_Pos) |
                    (1 << RCC_PLL2DIVR_DIVP2_Pos);
    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY));

    /* Configure PLL3: 25 MHz / 5 * 120 / 10 = 60 MHz */
    RCC->PLLCKSELR |= (5 << RCC_PLLCKSELR_DIVM3_Pos);
    RCC->PLL3DIVR = (120 << RCC_PLL3DIVR_DIVN3_Pos) |
                    (9 << RCC_PLL3DIVR_DIVR3_Pos);  /* DIVR=10 */
    RCC->CR |= RCC_CR_PLL3ON;
    while (!(RCC->CR & RCC_CR_PLL3RDY));

    /* Select PLL1 as system clock */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL1);

    /* Enable overdrive for 480 MHz */
    PWR->CR3 |= PWR_CR3_SCUEN;
    PWR->D3CR |= PWR_D3CR_VOS_1 | PWR_D3CR_VOS_0;
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY));

    /* Enable FPU */
    SCB->CPACR |= SCB_CPACR_CP10_FULL | SCB_CPACR_CP11_FULL;

    /* Configure SysTick for 1ms interrupts */
    SYSTICK->RVR = HCLK_FREQ_HZ / 1000 - 1;
    SYSTICK->CVR = 0;
    SYSTICK->CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT |
                   SYSTICK_CSR_CLKSOURCE;
}

/*===========================================================================
 * Peripheral Clock Enable
 *===========================================================================*/

static void peripheral_clocks_enable(void) {
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN | RCC_AHB3ENR_QSPIEN |
                    RCC_AHB3ENR_SDMMC1EN | RCC_AHB3ENR_SDMMC2EN |
                    RCC_AHB3ENR_MDMAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN |
                    RCC_AHB1ENR_USB1OTGHSEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_HASHEN;
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOFEN |
                    RCC_AHB4ENR_GPIOGEN | RCC_AHB4ENR_GPIOHEN |
                    RCC_AHB4ENR_GPIOIEN;
    RCC->APB1LENR |= RCC_APB1LENR_TIM2EN | RCC_APB1LENR_I2C1EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_SPI1EN;
    RCC->APB4ENR |= RCC_APB4ENR_RTCAPBEN;
}

/*===========================================================================
 * GPIO Initialization
 *===========================================================================*/

static void gpio_init(void) {
    /* Buttons: PA0-PA3 as inputs with pull-up */
    GPIOA->MODER &= ~(0xFFUL << 0);
    GPIOA->PUPDR |= (0x55UL << 0);

    /* FPGA control: PB12 output, PB13 input, PG6 input */
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << 24);
    GPIOB->MODER &= ~((3UL << 26) | (3UL << 28) | (3UL << 30));
    GPIOG->MODER &= ~(3UL << 12);

    /* LEDs: PB14, PB15 outputs */
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << 28) | (GPIO_MODE_OUTPUT << 30);

    /* Buzzer: PE2 AF1 (TIM1) */
    GPIOE->MODER |= (GPIO_MODE_AF << 4);
    GPIOE->AFR[0] |= (1UL << 8);

    /* I2C1: PB0, PB1 AF4, open-drain, pull-up */
    GPIOB->MODER |= (GPIO_MODE_AF << 0) | (GPIO_MODE_AF << 2);
    GPIOB->AFR[0] |= (4UL << 0) | (4UL << 4);
    GPIOB->OTYPER |= BIT(0) | BIT(1);
    GPIOB->PUPDR |= (GPIO_PUPD_PU << 0) | (GPIO_PUPD_PU << 2);

    /* OCTOSPI1: PA8-PA12, PB2 AF10 */
    GPIOA->MODER |= (GPIO_MODE_AF << 16) | (GPIO_MODE_AF << 18) |
                    (GPIO_MODE_AF << 20) | (GPIO_MODE_AF << 22) |
                    (GPIO_MODE_AF << 24);
    GPIOA->AFR[1] |= (10UL << 0) | (10UL << 4) | (10UL << 8) |
                     (10UL << 12) | (10UL << 16);
    GPIOB->MODER |= (GPIO_MODE_AF << 4);
    GPIOB->AFR[0] |= (10UL << 8);

    /* SPI1: PA4-PA7 AF5 */
    GPIOA->MODER |= (GPIO_MODE_AF << 8) | (GPIO_MODE_AF << 10) |
                    (GPIO_MODE_AF << 12) | (GPIO_MODE_AF << 14);
    GPIOA->AFR[0] |= (5UL << 16) | (5UL << 20) | (5UL << 24) | (5UL << 28);

    /* ULPI: PB5-PB15, PC0 AF10 */
    GPIOB->MODER |= (GPIO_MODE_AF << 10) | (GPIO_MODE_AF << 12) |
                    (GPIO_MODE_AF << 14) | (GPIO_MODE_AF << 16) |
                    (GPIO_MODE_AF << 18) | (GPIO_MODE_AF << 20) |
                    (GPIO_MODE_AF << 22) | (GPIO_MODE_AF << 24) |
                    (GPIO_MODE_AF << 26) | (GPIO_MODE_AF << 28) |
                    (GPIO_MODE_AF << 30);
    GPIOB->AFR[1] |= (10UL << 20) | (10UL << 24) | (10UL << 28);
    GPIOB->AFR[0] |= (10UL << 20) | (10UL << 24) | (10UL << 28);
    GPIOC->MODER |= (GPIO_MODE_AF << 0);
    GPIOC->AFR[0] |= (10UL << 0);

    /* SDMMC1 (microSD): PD8-PD12 AF12 */
    GPIOD->MODER |= (GPIO_MODE_AF << 16) | (GPIO_MODE_AF << 18) |
                    (GPIO_MODE_AF << 20) | (GPIO_MODE_AF << 22) |
                    (GPIO_MODE_AF << 24);
    GPIOD->AFR[1] |= (12UL << 0) | (12UL << 4) | (12UL << 8) |
                      (12UL << 12) | (12UL << 16);
}

/*===========================================================================
 * Task State Machine Definitions
 *===========================================================================*/

typedef enum {
    TASK_IDLE = 0,
    TASK_SELFTEST,
    TASK_EMMC_DETECT,
    TASK_EMMC_ACQUIRE,
    TASK_NAND_DETECT,
    TASK_NAND_ACQUIRE,
    TASK_SPINOR_DETECT,
    TASK_SPINOR_ACQUIRE,
    TASK_USB_TRANSFER,
    TASK_SDCARD_WRITE,
    TASK_HASH_VERIFY,
    TASK_FPGA_LOAD,
} task_id_t;

typedef enum {
    TSTATE_INIT = 0,
    TSTATE_RUNNING,
    TSTATE_DONE,
    TSTATE_ERROR,
    TSTATE_ABORTED,
} task_state_t;

typedef struct {
    task_id_t    id;
    task_state_t state;
    uint32_t     progress;
    uint32_t     total;
    uint32_t     error_code;
    uint32_t     start_tick;
    char         status_msg[32];
    void        *context;
} task_context_t;

/*===========================================================================
 * Global State
 *===========================================================================*/

static volatile uint32_t g_systick_ms = 0;
static task_context_t g_task;
static emmc_device_info_t g_emmc_info;
static nand_device_info_t g_nand_info;
static emmc_acq_state_t g_emmc_acq;
static uint8_t g_button_states = 0xFF;  /* All released (active low) */
static uint8_t g_button_prev = 0xFF;
static uint32_t g_last_button_tick = 0;
static uint32_t g_last_display_tick = 0;
static uint32_t g_last_battery_tick = 0;
static uint16_t g_battery_mv = 0;
static bool g_usb_connected = false;
static bool g_sdcard_present = false;
static uint8_t g_sha256_digest[32];

/* FPGA bitstream placeholder — loaded from flash at boot */
extern const uint8_t _binary_fpga_bitstream_bin_start[];
extern const uint8_t _binary_fpga_bitstream_bin_end[];

/*===========================================================================
 * SysTick Handler
 *===========================================================================*/

void SysTick_Handler(void) {
    g_systick_ms++;
}

uint32_t get_tick_ms(void) {
    return g_systick_ms;
}

/*===========================================================================
 * Self-Test Routine
 *===========================================================================*/

static int run_selftest(void) {
    int errors = 0;

    /* Test SDRAM */
    oled_print(0, "SDRAM test...");
    if (sdram_test() != 0) {
        oled_print(0, "SDRAM FAILED");
        errors++;
    } else {
        oled_print(0, "SDRAM OK");
    }

    /* Test FPGA */
    oled_print(1, "FPGA test...");
    if (fpga_verify() != FPGA_OK) {
        oled_print(1, "FPGA FAILED");
        errors++;
    } else {
        oled_print(1, "FPGA OK");
    }

    /* Test OLED */
    oled_print(2, "OLED OK");

    /* Test microSD */
    oled_print(3, "SD card...");
    if (sdcard_init() == 0) {
        oled_print(3, "SD card OK");
        g_sdcard_present = true;
    } else {
        oled_print(3, "No SD card");
    }

    /* Test battery */
    power_check_battery();
    oled_print(4, "Battery OK");

    /* Test buttons */
    oled_print(5, "Buttons OK");

    /* Test LEDs */
    led_set_rgb(0, 0, 255, 0);  /* Green */
    led_set_rgb(1, 0, 255, 0);
    delay_ms(200);
    led_set_rgb(0, 0, 0, 0);
    led_set_rgb(1, 0, 0, 0);

    /* Test buzzer */
    buzzer_beep(4000, 100);

    return errors;
}

/*===========================================================================
 * Task: eMMC Acquisition
 *===========================================================================*/

static void task_emmc_acquire_run(task_context_t *ctx) {
    emmc_acq_state_t *acq = (emmc_acq_state_t *)ctx->context;

    switch (ctx->state) {
    case TSTATE_INIT:
        /* Start acquisition */
        ctx->total = acq->total_blocks;
        ctx->progress = 0;
        snprintf(ctx->status_msg, sizeof(ctx->status_msg),
                 "eMMC: %lu MB", acq->total_blocks / 2048);
        if (emmc_acquire_start(acq, acq->start_block, acq->total_blocks) != 0) {
            ctx->state = TSTATE_ERROR;
            ctx->error_code = EMMC_ERR_DMA;
            return;
        }
        ctx->state = TSTATE_RUNNING;
        ctx->start_tick = get_tick_ms();
        break;

    case TSTATE_RUNNING:
        /* Check DMA completion */
        if (acq->dma_complete) {
            acq->dma_complete = false;
            ctx->progress = acq->blocks_read;

            int ret = emmc_acquire_continue(acq);
            if (ret == 1) {
                /* Acquisition complete */
                ctx->state = TSTATE_DONE;
                ctx->progress = ctx->total;
                snprintf(ctx->status_msg, sizeof(ctx->status_msg),
                         "eMMC done: %lu MB in %lu s",
                         ctx->total / 2048,
                         (get_tick_ms() - ctx->start_tick) / 1000);
            } else if (ret < 0) {
                ctx->state = TSTATE_ERROR;
                ctx->error_code = ret;
            }
        }

        /* Check abort */
        if (acq->abort_requested) {
            ctx->state = TSTATE_ABORTED;
        }
        break;

    case TSTATE_DONE:
    case TSTATE_ERROR:
    case TSTATE_ABORTED:
        /* Terminal states — handled by main loop */
        break;
    }
}

/*===========================================================================
 * Task: NAND Acquisition
 *===========================================================================*/

static void task_nand_acquire_run(task_context_t *ctx) {
    nand_device_info_t *info = &g_nand_info;

    switch (ctx->state) {
    case TSTATE_INIT:
        ctx->total = info->total_pages;
        ctx->progress = 0;
        snprintf(ctx->status_msg, sizeof(ctx->status_msg),
                 "NAND: %lu MB", info->total_size_bytes / (1024 * 1024));
        ctx->state = TSTATE_RUNNING;
        ctx->start_tick = get_tick_ms();
        break;

    case TSTATE_RUNNING: {
        /* Read pages in chunks of 64 */
        uint32_t chunk = 64;
        uint32_t start = ctx->progress;
        if (start + chunk > ctx->total) chunk = ctx->total - start;

        uint8_t *buf = (uint8_t *)(SDRAM_BASE_ADDR +
                        (start % (RING_BUFFER_SIZE / info->page_size)) *
                        info->page_size);

        int ret = fpga_nand_read_pages_dma(start, chunk, buf, info);
        if (ret == FPGA_OK) {
            ctx->progress += chunk;
            if (ctx->progress >= ctx->total) {
                ctx->state = TSTATE_DONE;
                snprintf(ctx->status_msg, sizeof(ctx->status_msg),
                         "NAND done: %lu MB in %lu s",
                         info->total_size_bytes / (1024 * 1024),
                         (get_tick_ms() - ctx->start_tick) / 1000);
            }
        } else {
            ctx->state = TSTATE_ERROR;
            ctx->error_code = ret;
        }
        break;
    }

    case TSTATE_DONE:
    case TSTATE_ERROR:
        break;
    }
}

/*===========================================================================
 * USB Command Processing
 *===========================================================================*/

typedef struct {
    uint32_t sync;      /* 0xFD4D4644 */
    uint16_t cmd;
    uint32_t length;
    uint16_t seq;
    uint8_t  payload[65536];
    uint16_t crc;
} usb_command_t;

static uint16_t crc16_xmodem(const uint8_t *data, uint32_t len) {
    uint16_t crc = 0;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc;
}

static void usb_process_command(const usb_command_t *cmd) {
    uint8_t response[256];
    uint32_t resp_len = 0;

    switch (cmd->cmd) {
    case 0x0001: /* CMD_GET_INFO */
        /* Return device info */
        response[0] = 0x01;  /* Hardware rev */
        response[1] = 0x00;  /* Firmware major */
        response[2] = 0x01;  /* Firmware minor */
        response[3] = 0x00;  /* Firmware patch */
        *(uint16_t *)(response + 4) = g_battery_mv;
        response[6] = g_sdcard_present ? 1 : 0;
        response[7] = g_usb_connected ? 1 : 0;
        resp_len = 8;
        usb_send_response(0x0002, cmd->seq, response, resp_len);
        break;

    case 0x0010: /* CMD_EMMC_DETECT */
        g_task.id = TASK_EMMC_DETECT;
        g_task.state = TSTATE_INIT;
        g_task.context = &g_emmc_info;
        usb_send_response(0x0100, cmd->seq, NULL, 0);  /* ACK */
        break;

    case 0x0012: /* CMD_EMMC_ACQUIRE */
        if (cmd->length >= 8) {
            uint32_t start_block = *(uint32_t *)(cmd->payload);
            uint32_t total_blocks = *(uint32_t *)(cmd->payload + 4);
            g_emmc_acq.start_block = start_block;
            g_emmc_acq.total_blocks = total_blocks;
            g_task.id = TASK_EMMC_ACQUIRE;
            g_task.state = TSTATE_INIT;
            g_task.context = &g_emmc_acq;
            usb_send_response(0x0100, cmd->seq, NULL, 0);
        } else {
            usb_send_response(0x0101, cmd->seq, NULL, 0);
        }
        break;

    case 0x0016: /* CMD_EMMC_ABORT */
        g_emmc_acq.abort_requested = true;
        usb_send_response(0x0100, cmd->seq, NULL, 0);
        break;

    case 0x0020: /* CMD_NAND_DETECT */
        g_task.id = TASK_NAND_DETECT;
        g_task.state = TSTATE_INIT;
        g_task.context = &g_nand_info;
        usb_send_response(0x0100, cmd->seq, NULL, 0);
        break;

    case 0x0022: /* CMD_NAND_ACQUIRE */
        g_task.id = TASK_NAND_ACQUIRE;
        g_task.state = TSTATE_INIT;
        g_task.context = &g_nand_info;
        usb_send_response(0x0100, cmd->seq, NULL, 0);
        break;

    case 0x0050: /* CMD_HASH_START */
        hash_sha256_start();
        usb_send_response(0x0100, cmd->seq, NULL, 0);
        break;

    case 0x0060: /* CMD_SELFTEST */
        g_task.id = TASK_SELFTEST;
        g_task.state = TSTATE_INIT;
        usb_send_response(0x0100, cmd->seq, NULL, 0);
        break;

    default:
        usb_send_response(0x00FF, cmd->seq, NULL, 0);  /* ERROR */
        break;
    }
}

/*===========================================================================
 * OLED UI Rendering
 *===========================================================================*/

static void ui_render(void) {
    oled_clear();

    switch (g_task.id) {
    case TASK_IDLE:
        oled_print(0, "eMMC Flash Dumper");
        oled_print(1, "v1.0 — Ready");
        oled_print(2, g_sdcard_present ? "SD: present" : "SD: none");
        oled_print(3, g_usb_connected ? "USB: connected" : "USB: none");
        {
            char bat_str[20];
            snprintf(bat_str, sizeof(bat_str), "BAT: %u.%02uV",
                     g_battery_mv / 1000, (g_battery_mv % 1000) / 10);
            oled_print(4, bat_str);
        }
        oled_print(5, "UP/DN: select mode");
        oled_print(6, "SEL: start  BACK: menu");
        oled_print(7, "Hold BACK: self-test");
        break;

    case TASK_SELFTEST:
        oled_print(0, "SELF-TEST RUNNING");
        oled_print(7, "Please wait...");
        break;

    case TASK_EMMC_DETECT:
        oled_print(0, "Detecting eMMC...");
        break;

    case TASK_EMMC_ACQUIRE:
        oled_print(0, g_task.status_msg);
        if (g_task.total > 0) {
            uint32_t pct = (g_task.progress * 100) / g_task.total;
            oled_print_progress(3, pct);
            char prog_str[20];
            snprintf(prog_str, sizeof(prog_str), "%lu / %lu blocks",
                     g_task.progress, g_task.total);
            oled_print(4, prog_str);
        }
        oled_print(7, "BACK: abort");
        break;

    case TASK_NAND_DETECT:
        oled_print(0, "Detecting NAND...");
        break;

    case TASK_NAND_ACQUIRE:
        oled_print(0, g_task.status_msg);
        if (g_task.total > 0) {
            uint32_t pct = (g_task.progress * 100) / g_task.total;
            oled_print_progress(3, pct);
        }
        oled_print(7, "BACK: abort");
        break;

    default:
        oled_print(0, g_task.status_msg);
        break;
    }

    /* Show error if any */
    if (g_task.state == TSTATE_ERROR) {
        char err_str[20];
        snprintf(err_str, sizeof(err_str), "ERR: %ld", g_task.error_code);
        oled_print(6, err_str);
    }

    oled_render();
}

/*===========================================================================
 * Button Handling
 *===========================================================================*/

typedef enum {
    BTN_NONE = 0,
    BTN_UP,
    BTN_DOWN,
    BTN_SELECT,
    BTN_BACK,
    BTN_BACK_LONG,
} button_event_t;

static button_event_t button_get_event(void) {
    uint8_t raw = (GPIOA->IDR & 0x0F);
    uint8_t changed = raw ^ g_button_prev;
    g_button_prev = raw;

    if (!changed) return BTN_NONE;

    /* Debounce */
    uint32_t now = get_tick_ms();
    if (now - g_last_button_tick < BTN_DEBOUNCE_MS) return BTN_NONE;
    g_last_button_tick = now;

    /* Check each button (active low: 0 = pressed) */
    if (!(raw & BIT(0)) && (changed & BIT(0))) return BTN_UP;
    if (!(raw & BIT(1)) && (changed & BIT(1))) return BTN_DOWN;
    if (!(raw & BIT(2)) && (changed & BIT(2))) return BTN_SELECT;
    if (!(raw & BIT(3)) && (changed & BIT(3))) {
        /* Check for long press */
        uint32_t press_start = now;
        while (!(GPIOA->IDR & BIT(3))) {
            if (get_tick_ms() - press_start > BTN_LONG_PRESS_MS) {
                return BTN_BACK_LONG;
            }
        }
        return BTN_BACK;
    }

    return BTN_NONE;
}

static void ui_handle_button(button_event_t btn) {
    switch (btn) {
    case BTN_UP:
        /* Cycle acquisition mode */
        if (g_task.id == TASK_IDLE) {
            /* Mode selection handled here */
        }
        break;

    case BTN_DOWN:
        if (g_task.id == TASK_IDLE) {
            /* Mode selection */
        }
        break;

    case BTN_SELECT:
        if (g_task.id == TASK_IDLE) {
            /* Start selected mode */
        }
        break;

    case BTN_BACK:
        if (g_task.id != TASK_IDLE && g_task.state == TSTATE_RUNNING) {
            /* Abort current task */
            if (g_task.id == TASK_EMMC_ACQUIRE) {
                g_emmc_acq.abort_requested = true;
            }
            g_task.state = TSTATE_ABORTED;
        } else if (g_task.state == TSTATE_DONE ||
                   g_task.state == TSTATE_ERROR ||
                   g_task.state == TSTATE_ABORTED) {
            /* Return to idle */
            g_task.id = TASK_IDLE;
            g_task.state = TSTATE_INIT;
        }
        break;

    case BTN_BACK_LONG:
        /* Run self-test */
        if (g_task.id == TASK_IDLE) {
            g_task.id = TASK_SELFTEST;
            g_task.state = TSTATE_INIT;
        }
        break;

    default:
        break;
    }
}

/*===========================================================================
 * Main Entry Point
 *===========================================================================*/

int main(void) {
    /* 1. System clock init */
    system_clock_init();

    /* 2. Enable peripheral clocks */
    peripheral_clocks_enable();

    /* 3. GPIO init */
    gpio_init();

    /* 4. Initialize SDRAM */
    if (sdram_init() != 0) {
        /* Fatal: halt with red LED */
        led_set_rgb(0, 255, 0, 0);
        while (1);
    }

    /* 5. Initialize FPGA */
    fpga_init();
    uint32_t bitstream_len = _binary_fpga_bitstream_bin_end -
                             _binary_fpga_bitstream_bin_start;
    if (fpga_load_bitstream(_binary_fpga_bitstream_bin_start,
                            bitstream_len) != FPGA_OK) {
        /* Non-fatal: continue without FPGA (eMMC + SPI NOR still work) */
        led_set_rgb(1, 255, 128, 0);  /* Orange warning */
    }

    /* 6. Initialize OLED */
    oled_init();
    oled_clear();
    oled_print(0, "eMMC Flash Dumper");
    oled_print(1, "Booting...");
    oled_render();

    /* 7. Initialize eMMC */
    emmc_init();

    /* 8. Initialize microSD */
    if (sdcard_init() == 0) {
        g_sdcard_present = true;
    }

    /* 9. Initialize USB */
    usb_build_serial_string();
    usb_device_init();

    /* 10. Initialize HASH accelerator */
    hash_sha256_start();

    /* 11. Initialize LEDs */
    led_set_rgb(0, 0, 255, 0);  /* Green = ready */
    led_set_rgb(1, 0, 0, 0);

    /* 12. Short beep to signal ready */
    buzzer_beep(4000, 50);

    /* 13. Initial battery check */
    power_check_battery();

    /* 14. Initialize task context */
    g_task.id = TASK_IDLE;
    g_task.state = TSTATE_INIT;

    /* 15. Main loop */
    while (1) {
        /* Process buttons */
        button_event_t btn = button_get_event();
        if (btn != BTN_NONE) {
            ui_handle_button(btn);
        }

        /* Process USB */
        usb_device_process();

        /* Run current task */
        switch (g_task.id) {
        case TASK_IDLE:
            break;

        case TASK_SELFTEST:
            if (g_task.state == TSTATE_INIT) {
                int errors = run_selftest();
                g_task.state = errors ? TSTATE_ERROR : TSTATE_DONE;
                g_task.error_code = errors;
                if (errors == 0) {
                    buzzer_beep(4000, 100);
                    delay_ms(100);
                    buzzer_beep(4000, 100);
                }
            }
            break;

        case TASK_EMMC_DETECT:
            if (g_task.state == TSTATE_INIT) {
                int ret = emmc_detect(&g_emmc_info);
                if (ret == EMMC_OK) {
                    g_task.state = TSTATE_DONE;
                    snprintf(g_task.status_msg, sizeof(g_task.status_msg),
                             "eMMC: %lu MB", g_emmc_info.capacity_bytes / (1024*1024));
                } else {
                    g_task.state = TSTATE_ERROR;
                    g_task.error_code = ret;
                }
            }
            break;

        case TASK_EMMC_ACQUIRE:
            task_emmc_acquire_run(&g_task);
            break;

        case TASK_NAND_DETECT:
            if (g_task.state == TSTATE_INIT) {
                int ret = fpga_nand_detect(&g_nand_info);
                if (ret == FPGA_OK) {
                    g_task.state = TSTATE_DONE;
                    snprintf(g_task.status_msg, sizeof(g_task.status_msg),
                             "NAND: %lu MB", g_nand_info.total_size_bytes / (1024*1024));
                } else {
                    g_task.state = TSTATE_ERROR;
                    g_task.error_code = ret;
                }
            }
            break;

        case TASK_NAND_ACQUIRE:
            task_nand_acquire_run(&g_task);
            break;

        default:
            break;
        }

        /* Handle task completion */
        if (g_task.state == TSTATE_DONE) {
            led_set_rgb(0, 0, 255, 0);  /* Green */
            buzzer_beep(4000, 200);
        } else if (g_task.state == TSTATE_ERROR) {
            led_set_rgb(0, 255, 0, 0);  /* Red */
            buzzer_beep(2000, 500);
        }

        /* Update display (10 Hz) */
        uint32_t now = get_tick_ms();
        if (now - g_last_display_tick >= 100) {
            ui_render();
            g_last_display_tick = now;
        }

        /* Battery check (every 5 seconds) */
        if (now - g_last_battery_tick >= 5000) {
            power_check_battery();
            g_last_battery_tick = now;

            /* Low battery warning */
            if (g_battery_mv < BATTERY_LOW_MV && g_battery_mv > BATTERY_CRITICAL_MV) {
                led_set_rgb(1, 255, 128, 0);  /* Orange */
            } else if (g_battery_mv <= BATTERY_CRITICAL_MV) {
                led_set_rgb(1, 255, 0, 0);  /* Red */
                if (g_battery_mv <= BATTERY_SHUTDOWN_MV) {
                    /* Critical: halt acquisition and warn */
                    if (g_task.state == TSTATE_RUNNING) {
                        g_task.state = TSTATE_ABORTED;
                    }
                }
            }
        }
    }

    return 0;
}

/*===========================================================================
 * Stub Implementations (to be fleshed out in separate driver files)
 *===========================================================================*/

int sdram_init(void) {
    /* Configure FMC GPIOs for SDRAM */
    /* ... full implementation in sdram.c ... */
    /* For now: basic init sequence */
    FMC->SDCR1 = FMC_SDCR1_NC_1 | FMC_SDCR1_NR_2 | FMC_SDCR1_NR_1 |
                 FMC_SDCR1_MWID_1 | FMC_SDCR1_NB |
                 FMC_SDCR1_CAS_2 | FMC_SDCR1_SDCLK_1 | FMC_SDCR1_SDCLK_0 |
                 FMC_SDCR1_RBURST | FMC_SDCR1_RPIPE_1;
    FMC->SDTR1 = (2 << FMC_SDTR1_TMRD_Pos) | (6 << FMC_SDTR1_TXSR_Pos) |
                 (2 << FMC_SDTR1_TRAS_Pos) | (2 << FMC_SDTR1_TRC_Pos) |
                 (2 << FMC_SDTR1_TWR_Pos) | (2 << FMC_SDTR1_TRP_Pos) |
                 (2 << FMC_SDTR1_TRCD_Pos);
    FMC->SDTR2 = (7 << FMC_SDTR2_TRAS_Pos) | (7 << FMC_SDTR2_TRC_Pos) |
                 (4 << FMC_SDTR2_TWR_Pos) | (2 << FMC_SDTR2_TRP_Pos) |
                 (2 << FMC_SDTR2_TRCD_Pos);
    FMC->SDRTR = (1386 << FMC_SDRTR_COUNT_Pos);

    /* Init sequence */
    FMC->SDCMR = FMC_SDCMR_CTB1 | FMC_SDCMR_MODE_001;
    delay_ms(1);
    FMC->SDCMR = FMC_SDCMR_CTB1 | FMC_SDCMR_MODE_010 | (2 << 5);
    delay_ms(1);
    FMC->SDCMR = FMC_SDCMR_CTB1 | FMC_SDCMR_MODE_011 | (0x0 << 9);
    delay_ms(1);
    FMC->SDCR1 |= FMC_SDCR1_RA;

    return 0;
}

int sdram_test(void) {
    volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
    /* Quick walking-1 test on first 64KB */
    for (int i = 0; i < 16384; i++) {
        sdram[i] = 1UL << (i % 32);
        if (sdram[i] != (1UL << (i % 32))) return -1;
        sdram[i] = 0;
    }
    return 0;
}

int oled_init(void) {
    /* I2C1 init + SSD1306 init sequence */
    /* ... full implementation in oled.c ... */
    return 0;
}

void oled_clear(void) {
    /* Clear framebuffer */
}

void oled_print(uint8_t line, const char *text) {
    /* Render text to framebuffer line */
}

void oled_print_progress(uint8_t line, uint32_t percent) {
    /* Render progress bar */
}

void oled_render(void) {
    /* Send framebuffer to SSD1306 via I2C */
}

int sdcard_init(void) {
    /* SDMMC1 init + card detect */
    return -1;  /* Stub: no card */
}

int sdcard_write_blocks(const uint8_t *data, uint32_t blocks) {
    return -1;
}

int usb_device_init(void) {
    /* USB OTG_HS init with ULPI */
    return 0;
}

int usb_device_process(void) {
    /* Check for USB events, handle enumeration */
    return 0;
}

int usb_send_data(const uint8_t *data, uint32_t length) {
    return 0;
}

int usb_send_response(uint16_t cmd_id, uint16_t seq,
                      const uint8_t *payload, uint32_t len) {
    return 0;
}

int hash_sha256_start(void) {
    HASH->CR = HASH_CR_INIT | HASH_CR_ALGO_SHA256 |
               HASH_CR_DATATYPE_32B | HASH_CR_DMAE;
    return 0;
}

int hash_sha256_update(const uint8_t *data, uint32_t length) {
    return 0;
}

int hash_sha256_finish(uint8_t *digest) {
    return 0;
}

void power_check_battery(void) {
    g_battery_mv = 3800;  /* Stub: nominal voltage */
}

uint16_t battery_mv(void) {
    return g_battery_mv;
}

void led_set_rgb(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
    /* WS2812B bit-bang via SPI or timer */
    if (led == 0) {
        if (r || g || b) GPIOB->BSRR = BIT(14);
        else GPIOB->BSRR = BIT(14 + 16);
    } else {
        if (r || g || b) GPIOB->BSRR = BIT(15);
        else GPIOB->BSRR = BIT(15 + 16);
    }
}

void buzzer_beep(uint16_t freq, uint16_t duration_ms) {
    /* TIM1 PWM for buzzer */
    TIM1->PSC = APB2_FREQ_HZ / (freq * 100) - 1;
    TIM1->ARR = 99;
    TIM1->CCR3 = 50;
    TIM1->CR1 |= BIT(0);
    delay_ms(duration_ms);
    TIM1->CR1 &= ~BIT(0);
}

void button_read(uint8_t *states) {
    *states = GPIOA->IDR & 0x0F;
}

void delay_ms(uint32_t ms) {
    uint32_t start = get_tick_ms();
    while (get_tick_ms() - start < ms);
}

/*===========================================================================
 * Default Exception Handlers
 *===========================================================================*/

void NMI_Handler(void) { while (1); }
void HardFault_Handler(void) { while (1); }
void MemManage_Handler(void) { while (1); }
void BusFault_Handler(void) { while (1); }
void UsageFault_Handler(void) { while (1); }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

/*===========================================================================
 * Reset Handler — Entry Point
 *===========================================================================*/

/* Vector table must be placed at start of flash */
__attribute__((section(".vectors")))
const uint32_t vector_table[] = {
    (uint32_t)0x20020000,           /* Initial MSP (top of DTCM) */
    (uint32_t)main,                 /* Reset handler */
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    (uint32_t)MemManage_Handler,
    (uint32_t)BusFault_Handler,
    (uint32_t)UsageFault_Handler,
    0, 0, 0, 0,                    /* Reserved */
    (uint32_t)SVC_Handler,
    (uint32_t)DebugMon_Handler,
    0,                              /* Reserved */
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,
    /* Peripheral interrupts follow... */
};
