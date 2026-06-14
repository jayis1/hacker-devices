/*
 * main.c — CAN Bus Infiltrator main application
 * STM32F407VGT6, bare-metal, no OS
 */

#include "board.h"
#include "can.h"
#include "ble_spi.h"
#include "sdcard.h"
#include "i2c_eeprom.h"
#include "ws2812b.h"
#include <string.h>

/* ========== Ring Buffer ========== */
#define RING_BUF_SIZE  4096

static can_frame_t ring_buf[2][RING_BUF_SIZE];
static volatile uint32_t ring_head[2] = {0, 0};
static volatile uint32_t ring_tail[2] = {0, 0};
static volatile uint32_t frame_count[2] = {0, 0};
static volatile uint32_t error_count[2] = {0, 0};
static volatile uint32_t inject_count[2] = {0, 0};

/* ========== Fuzzer State ========== */
typedef enum {
    FUZZ_IDLE = 0,
    FUZZ_RANDOM,
    FUZZ_BITFLIP,
    FUZZ_BYTE_MUTATE,
    FUZZ_ARITHMETIC,
    FUZZ_FIELD_AWARE
} fuzz_mode_t;

static volatile fuzz_mode_t fuzz_mode = FUZZ_IDLE;
static volatile uint8_t fuzz_channel = 0;
static volatile uint32_t fuzz_target_id = 0;
static volatile uint32_t fuzz_count = 0;
static volatile uint32_t fuzz_max_count = 1000;

/* ========== SD Recording ========== */
static volatile uint8_t recording_active = 0;
static volatile uint8_t replay_active = 0;

/* ========== PRNG (XORshift32) ========== */
static volatile uint32_t prng_state = 0xDEADBEEF;

static uint8_t prng_next(void) {
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 17;
    prng_state ^= prng_state << 5;
    return (uint8_t)(prng_state & 0xFF);
}

static uint32_t prng_next32(void) {
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 17;
    prng_state ^= prng_state << 5;
    return prng_state;
}

/* ========== LED Patterns ========== */
static void led_set_all(uint8_t r, uint8_t g, uint8_t b) {
    ws2812b_color_t colors[LED_COUNT];
    for (int i = 0; i < LED_COUNT; i++) {
        colors[i].r = r;
        colors[i].g = g;
        colors[i].b = b;
    }
    ws2812b_send(colors, LED_COUNT);
}

static void led_set_index(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    ws2812b_color_t colors[LED_COUNT];
    /* Read current state (simplified — just set all to off then one on) */
    for (int i = 0; i < LED_COUNT; i++) {
        colors[i].r = 0; colors[i].g = 0; colors[i].b = 0;
    }
    if (idx < LED_COUNT) {
        colors[idx].r = r;
        colors[idx].g = g;
        colors[idx].b = b;
    }
    ws2812b_send(colors, LED_COUNT);
}

/* ========== CAN RX Callback ========== */
static void can_rx_handler(const can_frame_t *frame, uint8_t channel) {
    uint32_t next_head = (ring_head[channel] + 1) % RING_BUF_SIZE;

    if (next_head != ring_tail[channel]) {
        ring_buf[channel][ring_head[channel]] = *frame;
        ring_head[channel] = next_head;
    }
    frame_count[channel]++;

    /* Forward to BLE */
    ble_send_can_frame(frame);

    /* Record to SD if active */
    if (recording_active) {
        sd_write_can_frame(channel, frame);
    }
}

/* ========== System Initialization ========== */
static void system_init(void) {
    /* Enable FPU */
    SCB->CPACR |= (0xF << 20);

    /* Enable all GPIO clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB |
                     RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD |
                     RCC_AHB1ENR_GPIOE;

    /* Configure PLL: HSE 8 MHz → SYSCLK 168 MHz
     * PLLM = 8, PLLN = 336, PLLP = 2, PLLQ = 7
     * PLL_VCO = 8/8 * 336 = 336 MHz
     * SYSCLK = 336/2 = 168 MHz
     * USB = 336/7 = 48 MHz
     */
    RCC->CR |= (1 << 16);  /* HSEON */
    while (!(RCC->CR & (1 << 17)));  /* Wait for HSERDY */

    RCC->PLLCFGR = (8 << 0) |    /* PLLM = 8 */
                    (336 << 6) |   /* PLLN = 336 */
                    (0 << 16) |    /* PLLP = 2 */
                    (7 << 24) |    /* PLLQ = 7 */
                    (1 << 22);     /* PLLSRC = HSE */

    RCC->CR |= (1 << 24);  /* PLLON */
    while (!(RCC->CR & (1 << 25)));  /* Wait for PLLRDY */

    /* Flash latency: 5 wait states at 168 MHz, prefetch, I-cache, D-cache */
    FLASH->ACR = (5 << 0) | (1 << 8) | (1 << 9) | (1 << 10);

    /* Switch system clock to PLL */
    RCC->CFGR = (2 << 0);  /* SW = PLL */
    while ((RCC->CFGR & (3 << 2)) != (2 << 2));  /* Wait for PLL as SYSCLK */

    /* APB1 prescaler = 4 (42 MHz), APB2 prescaler = 2 (84 MHz) */
    RCC->CFGR |= (0b100 << 10) | (0b010 << 13);

    /* Enable backup SRAM for persistent config */
    RCC->AHB1ENR |= RCC_AHB1ENR_BKPSRAMEN;

    /* Initialize TIM2 as free-running microsecond counter */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = (84000000 / 1000000) - 1;  /* 1 µs tick (APB1 timer = 84 MHz) */
    TIM2->ARR = 0xFFFFFFFF;  /* Free-running 32-bit */
    TIM2->CR1 = (1 << 0);    /* CEN: enable */

    /* Configure CAN transceiver standby pins (active LOW) */
    gpio_clear(CAN1_STB_PORT, CAN1_STB_PIN);  /* CH1 active */
    gpio_clear(CAN2_STB_PORT, CAN2_STB_PIN);  /* CH2 active */

    /* Configure LED DIN pin as output */
    LED_DIN_PORT->MODER &= ~(3 << (LED_DIN_PIN * 2));
    LED_DIN_PORT->MODER |= (1 << (LED_DIN_PIN * 2));  /* Output */
    LED_DIN_PORT->OSPEEDR |= (3 << (LED_DIN_PIN * 2));  /* Very high speed */

    /* Configure nRF control pins */
    NRF_BOOT_PORT->MODER &= ~(3 << (NRF_BOOT_PIN * 2));
    NRF_BOOT_PORT->MODER |= (1 << (NRF_BOOT_PIN * 2));  /* Output */
    gpio_set(NRF_BOOT_PORT, NRF_BOOT_PIN);  /* HIGH = normal boot */

    NRF_IRQ_PORT->MODER &= ~(3 << (NRF_IRQ_PIN * 2));  /* Input */
    NRF_IRQ_PORT->PUPDR |= (1 << (NRF_IRQ_PIN * 2));   /* Pull-up */
}

/* ========== GPIO Configuration ========== */
static void gpio_init_all(void) {
    /* CAN1 pins (PB0=RX, PB1=TX) — AF9 */
    GPIOB->MODER &= ~(3 << (0 * 2)) & ~(3 << (1 * 2));
    GPIOB->MODER |= (2 << (0 * 2)) | (2 << (1 * 2));  /* Alternate function */
    GPIOB->AFR[0] &= ~(0xF << (0 * 4)) & ~(0xF << (1 * 4));
    GPIOB->AFR[0] |= (9 << (0 * 4)) | (9 << (1 * 4));  /* AF9 = CAN1 */
    GPIOB->OSPEEDR |= (3 << (0 * 2)) | (3 << (1 * 2));  /* Very high speed */

    /* CAN2 pins (PE10=RX, PE11=TX) — AF9 */
    GPIOE->MODER &= ~(3 << (10 * 2)) & ~(3 << (11 * 2));
    GPIOE->MODER |= (2 << (10 * 2)) | (2 << (11 * 2));
    GPIOE->AFR[1] &= ~(0xF << ((10 - 8) * 4)) & ~(0xF << ((11 - 8) * 4));
    GPIOE->AFR[1] |= (9 << ((10 - 8) * 4)) | (9 << ((11 - 8) * 4));
    GPIOE->OSPEEDR |= (3 << (10 * 2)) | (3 << (11 * 2));

    /* CAN standby pins (PE2, PE3) — GPIO output */
    GPIOE->MODER &= ~(3 << (2 * 2)) & ~(3 << (3 * 2));
    GPIOE->MODER |= (1 << (2 * 2)) | (1 << (3 * 2));

    /* SPI3 pins (PB3=SCK, PB4=MISO, PB5=MOSI) — AF6 */
    GPIOB->MODER &= ~(3 << (3 * 2)) & ~(3 << (4 * 2)) & ~(3 << (5 * 2));
    GPIOB->MODER |= (2 << (3 * 2)) | (2 << (4 * 2)) | (2 << (5 * 2));
    GPIOB->AFR[0] &= ~(0xF << (3 * 4)) & ~(0xF << (4 * 4)) & ~(0xF << (5 * 4));
    GPIOB->AFR[0] |= (6 << (3 * 4)) | (6 << (4 * 4)) | (6 << (5 * 4));
    GPIOB->OSPEEDR |= (3 << (3 * 2)) | (3 << (4 * 2)) | (3 << (5 * 2));

    /* SPI3 NSS (PD7) — GPIO output */
    GPIOD->MODER &= ~(3 << (7 * 2));
    GPIOD->MODER |= (1 << (7 * 2));
    GPIOD->OSPEEDR |= (3 << (7 * 2));
    gpio_set(GPIOD, 7);  /* CS high = idle */

    /* USB pins (PA11=DM, PA12=DP) — AF10 */
    GPIOA->MODER &= ~(3 << (11 * 2)) & ~(3 << (12 * 2));
    GPIOA->MODER |= (2 << (11 * 2)) | (2 << (12 * 2));
    GPIOA->AFR[1] &= ~(0xF << ((11 - 8) * 4)) & ~(0xF << ((12 - 8) * 4));
    GPIOA->AFR[1] |= (10 << ((11 - 8) * 4)) | (10 << ((12 - 8) * 4));

    /* I2C1 pins (PA15=SCL, PB6=SDA) — AF4, open drain */
    GPIOA->MODER &= ~(3 << (15 * 2));
    GPIOA->MODER |= (2 << (15 * 2));
    GPIOA->AFR[1] &= ~(0xF << ((15 - 8) * 4));
    GPIOA->AFR[1] |= (4 << ((15 - 8) * 4));
    GPIOA->OTYPER |= (1 << 15);  /* Open drain */
    GPIOA->PUPDR |= (1 << (15 * 2));  /* Pull-up */

    GPIOB->MODER &= ~(3 << (6 * 2));
    GPIOB->MODER |= (2 << (6 * 2));
    GPIOB->AFR[0] &= ~(0xF << (6 * 4));
    GPIOB->AFR[0] |= (4 << (6 * 4));
    GPIOB->OTYPER |= (1 << 6);  /* Open drain */
    GPIOB->PUPDR |= (1 << (6 * 2));  /* Pull-up */

    /* SDIO pins — AF12 */
    GPIOC->MODER &= ~(3 << (8*2)) & ~(3 << (9*2)) & ~(3 << (10*2)) &
                     ~(3 << (11*2)) & ~(3 << (12*2));
    GPIOC->MODER |= (2 << (8*2)) | (2 << (9*2)) | (2 << (10*2)) |
                    (2 << (11*2)) | (2 << (12*2));
    GPIOC->AFR[1] |= (12 << ((8-8)*4)) | (12 << ((9-8)*4)) |
                      (12 << ((10-8)*4)) | (12 << ((11-8)*4)) |
                      (12 << ((12-8)*4));
    GPIOC->OSPEEDR |= (3 << (8*2)) | (3 << (9*2)) | (3 << (10*2)) |
                       (3 << (11*2)) | (3 << (12*2));

    GPIOD->MODER &= ~(3 << (2*2));
    GPIOD->MODER |= (2 << (2*2));
    GPIOD->AFR[0] &= ~(0xF << (2*4));
    GPIOD->AFR[0] |= (12 << (2*4));
    GPIOD->OSPEEDR |= (3 << (2*2));
}

/* ========== Fuzzer ========== */
static void fuzzer_generate_frame(can_frame_t *frame) {
    frame->id = fuzz_target_id;
    frame->id_type = (fuzz_target_id > 0x7FF) ? CAN_EXT_ID : CAN_STD_ID;
    frame->rtr = 0;
    frame->dlc = (prng_next() % 8) + 1;

    switch (fuzz_mode) {
        case FUZZ_RANDOM:
            for (int i = 0; i < frame->dlc; i++)
                frame->data[i] = prng_next();
            break;
        case FUZZ_BITFLIP:
            for (int i = 0; i < frame->dlc; i++) {
                frame->data[i] = prng_next();
                uint8_t bit = prng_next() & 7;
                frame->data[i] ^= (1 << bit);
            }
            break;
        case FUZZ_ARITHMETIC:
            for (int i = 0; i < frame->dlc; i++)
                frame->data[i] = prng_next();
            {
                uint8_t idx = prng_next() % frame->dlc;
                int8_t delta = (int8_t)(prng_next());
                frame->data[idx] += delta;
            }
            break;
        case FUZZ_BYTE_MUTATE:
            for (int i = 0; i < frame->dlc; i++)
                frame->data[i] = 0;  /* Start from known state */
            {
                uint8_t idx = prng_next() % frame->dlc;
                frame->data[idx] = prng_next();  /* Mutate single byte */
            }
            break;
        default:
            for (int i = 0; i < 8; i++)
                frame->data[i] = prng_next();
            break;
    }

    frame->timestamp_us = TIM2->CNT;
}

static void fuzzer_task(void) {
    if (fuzz_mode == FUZZ_IDLE) return;

    can_frame_t frame;
    for (uint32_t i = 0; i < fuzz_max_count; i++) {
        fuzzer_generate_frame(&frame);
        if (can_transmit(fuzz_channel, &frame) == 0) {
            inject_count[fuzz_channel]++;
        }
        fuzz_count++;

        /* Inter-frame delay: 1-5 ms */
        uint32_t delay = (prng_next() % 5) + 1;
        delay_ms(delay);
    }

    fuzz_mode = FUZZ_IDLE;  /* Auto-stop when done */
    led_set_all(0, 255, 0);  /* Green = done */
}

/* ========== Command Processor ========== */
static void process_command(const ble_packet_t *cmd) {
    switch (cmd->opcode) {
        case 0x01: { /* CAN_SNIFF_START */
            uint8_t ch = cmd->data[0];
            can_baudrate_t baud = (can_baudrate_t)cmd->data[1];
            can_init(ch, baud, CAN_MODE_LISTEN);
            can_register_rx_callback(ch, can_rx_handler);
            can_enable_interrupts(ch);
            led_set_all(0, 0, 255);  /* Blue = sniffing */
            break;
        }
        case 0x02: { /* CAN_SNIFF_STOP */
            uint8_t ch = cmd->data[0];
            can_disable_interrupts(ch);
            led_set_all(0, 255, 0);  /* Green = idle */
            break;
        }
        case 0x03: { /* CAN_INJECT */
            can_frame_t frame;
            frame.id = cmd->data[0] | (cmd->data[1] << 8);
            if (cmd->data[2] & 0x01) {
                frame.id |= (cmd->data[3] << 16) | (cmd->data[4] << 24);
            }
            frame.id_type = (cmd->data[2] & 0x01) ? CAN_EXT_ID : CAN_STD_ID;
            frame.dlc = cmd->data[2] >> 4;
            frame.rtr = (cmd->data[2] >> 3) & 1;
            if (frame.dlc > 8) frame.dlc = 8;
            for (int i = 0; i < frame.dlc; i++)
                frame.data[i] = cmd->data[5 + i];
            frame.timestamp_us = TIM2->CNT;
            uint8_t ch = (cmd->data[2] >> 2) & 1;
            can_transmit(ch, &frame);
            inject_count[ch]++;
            led_set_index(1, 255, 0, 0);  /* LED1 red = injection */
            break;
        }
        case 0x04: { /* CAN_FUZZ_START */
            fuzz_channel = cmd->data[0];
            fuzz_mode = (fuzz_mode_t)cmd->data[1];
            fuzz_target_id = cmd->data[2] | (cmd->data[3] << 8) |
                             (cmd->data[4] << 16) | (cmd->data[5] << 24);
            fuzz_max_count = cmd->data[6] | (cmd->data[7] << 8) |
                             (cmd->data[8] << 16) | (cmd->data[9] << 24);
            fuzz_count = 0;
            led_set_all(255, 165, 0);  /* Orange = fuzzing */
            break;
        }
        case 0x05: { /* CAN_FUZZ_STOP */
            fuzz_mode = FUZZ_IDLE;
            led_set_all(0, 255, 0);
            break;
        }
        case 0x08: { /* SD_RECORD_START */
            recording_active = 1;
            sd_start_recording("can_log.bin");
            led_set_index(2, 255, 0, 255);  /* LED2 magenta = recording */
            break;
        }
        case 0x09: { /* SD_RECORD_STOP */
            recording_active = 0;
            sd_stop_recording();
            break;
        }
        case 0x0A: { /* SD_REPLAY_START */
            replay_active = 1;
            sd_start_replay("can_log.bin");
            break;
        }
        case 0x0B: { /* SD_REPLAY_STOP */
            replay_active = 0;
            sd_stop_replay();
            break;
        }
        case 0x0D: { /* DEVICE_INFO */
            /* Send device info response via BLE */
            uint8_t info[] = {
                0x84,  /* DEVICE_INFO_RSP opcode */
                0x01,  /* Version major */
                0x00,  /* Version minor */
                0x02,  /* 2 CAN channels */
                0x01,  /* Features: sniff, inject, fuzz, SD, BLE */
                (BUILD_VERSION >> 8) & 0xFF,
                BUILD_VERSION & 0xFF
            };
            ble_spi_send_command(0x84, info, sizeof(info));
            break;
        }
        case 0x0E: { /* DFU_ENTER */
            /* Set DFU flag in backup SRAM and reset */
            volatile uint32_t *bkp = (volatile uint32_t *)0x40024000;
            *bkp = 0xDFU0DFU0;
            NVIC_SystemReset();
            break;
        }
        case 0x0F: { /* LED_SET */
            uint8_t idx = cmd->data[0];
            uint8_t r = cmd->data[1];
            uint8_t g = cmd->data[2];
            uint8_t b = cmd->data[3];
            led_set_index(idx, r, g, b);
            break;
        }
        default:
            break;
    }
}

/* ========== Main Entry Point ========== */
int main(void) {
    /* Initialize system clocks and GPIO */
    system_init();
    gpio_init_all();

    /* Initialize peripherals */
    ws2812b_init();

    /* Startup LED animation: white sweep */
    for (int i = 0; i < LED_COUNT; i++) {
        led_set_index(i, 255, 255, 255);
        delay_ms(100);
    }
    delay_ms(200);
    led_set_all(0, 0, 0);

    /* Initialize CAN buses */
    can_init(0, CAN_BAUD_500K, CAN_MODE_LISTEN);  /* CH1: listen-only */
    can_init(1, CAN_BAUD_500K, CAN_MODE_NORMAL);   /* CH2: normal */
    can_register_rx_callback(0, can_rx_handler);
    can_register_rx_callback(1, can_rx_handler);
    can_enable_interrupts(0);
    can_enable_interrupts(1);

    /* Initialize BLE SPI interface */
    ble_spi_init();

    /* Initialize SD card */
    sd_init();

    /* Initialize EEPROM */
    i2c_eeprom_init();

    /* Load config from EEPROM */
    uint8_t config_buf[32];
    i2c_eeprom_read(EEPROM_I2C_ADDR, 0x0000, config_buf, sizeof(config_buf));

    /* Enable global interrupts */
    __enable_irq();

    /* Ready LED: green */
    led_set_all(0, 255, 0);

    /* ========== Main Loop ========== */
    while (1) {
        /* Process BLE commands */
        ble_packet_t cmd;
        if (ble_spi_read_response(&cmd, 10) == 0) {
            process_command(&cmd);
        }

        /* Run fuzzer if active */
        if (fuzz_mode != FUZZ_IDLE) {
            fuzzer_task();
        }

        /* Replay from SD if active */
        if (replay_active) {
            can_frame_t frame;
            if (sd_read_next_can_frame(&frame, NULL) == 0) {
                can_transmit(fuzz_channel, &frame);
            } else {
                replay_active = 0;
            }
        }

        /* Feed IWDG watchdog */
        /* IWDG->KR = 0xAAAA; */

        /* Brief delay to prevent bus contention */
        for (volatile int i = 0; i < 1000; i++);
    }
}

/* ========== Interrupt Handlers ========== */
void CAN1_RX0_IRQHandler(void) {
    can_irq_handler(0);
}

void CAN1_RX1_IRQHandler(void) {
    can_irq_handler(0);
}

void CAN2_RX0_IRQHandler(void) {
    can_irq_handler(1);
}

void HardFault_Handler(void) {
    /* Toggle all LEDs red to indicate fault */
    while (1) {
        led_set_all(255, 0, 0);
        for (volatile int i = 0; i < 1000000; i++);
        led_set_all(0, 0, 0);
        for (volatile int i = 0; i < 1000000; i++);
    }
}

void MemManage_Handler(void) {
    HardFault_Handler();
}

void BusFault_Handler(void) {
    HardFault_Handler();
}

void UsageFault_Handler(void) {
    HardFault_Handler();
}

/* NVIC System Reset */
static inline void NVIC_SystemReset(void) {
    SCB->AIRCR = (0x5FA << 16) | (1 << 2);
    while (1);
}

#define BUILD_VERSION 0x0100  /* v1.0 */