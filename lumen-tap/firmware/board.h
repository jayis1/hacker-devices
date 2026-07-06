/*
 * lumen-tap/firmware/board.h
 * Pin definitions, board API, and hardware constants for LumenTap.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#ifndef LUMEN_TAP_BOARD_H
#define LUMEN_TAP_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Physical board constants ---------------------------------------- */
#define BOARD_NAME              "LumenTap"
#define BOARD_REV               "1.0"
#define BOARD_AUTHOR            "jayis1"

/* STM32H743VI runs at 480 MHz core, 240 MHz AXI/AHB, 48 MHz APB1, 120 MHz APB2 */
#define SYSCLK_HZ               (480000000UL)
#define AHB_HZ                  (240000000UL)
#define APB1_HZ                  (120000000UL)
#define APB2_HZ                  (120000000UL)

/* Audio sample rates */
#define ADC_SAMPLE_RATE_HZ      (192000UL)   /* ADC1 oversampled rate */
#define OUTPUT_SAMPLE_RATE_HZ   (48000UL)    /* UAC2 streaming rate */
#define DECIMATION_FACTOR       (ADC_SAMPLE_RATE_HZ / OUTPUT_SAMPLE_RATE_HZ) /* 4x */

#define DSP_BLOCK_SAMPLES       (128)        /* samples per DMA half-transfer */
#define DSP_BLOCK_BYTES         (DSP_BLOCK_SAMPLES * sizeof(int16_t))

/* Laser safety limits (IEC 60825-1 Class 1 at 520 nm, 7 mm aperture) */
#define LTM_LASER_PWR_MAX       (39)         /* 0.39 mW cap as PWM ticks 0..255 */
#define LTM_LASER_PWR_DEFAULT   (28)         /* conservative default ~0.28 mW */
#define LTM_LASER_PWM_HZ        (100000UL)   /* 100 kHz PWM carrier */

/* Ambient-light safety threshold (TSL257 full-scale ~ 16383 counts) */
#define LTM_AMBIENT_BLOCK_CNT   (6000)       /* block emission above this */

/* SD log ring buffer size in blocks */
#define LTM_SD_RING_BLOCKS      (32768)      /* ~4 MB worth at 128B header */

/* ---- Port / pin assignments (STM32H743VI LQFP-100) ------------------ */
/* Using STM32 standard PORT/pin naming. All GPIOs on GPIOx. */

/* PORTA */
#define PIN_ADC1_INP0           0    /* PA0  -> ADC1_INP0 (optical return, diff P) */
#define PIN_ADC1_INN0           1    /* PA1  -> ADC1_INN0 (optical return, diff N) */
#define PIN_UART2_TX            2    /* PA2  -> debug UART2 TX */
#define PIN_UART2_RX            3    /* PA3  -> debug UART2 RX */
#define PIN_SPI1_SCK            5    /* PA5  -> SPI1 SCK (SD card) */
#define PIN_SPI1_MISO           6    /* PA6  -> SPI1 MISO */
#define PIN_SPI1_MOSI           7    /* PA7  -> SPI1 MOSI */
#define PIN_USB_OTG_HS_ID       10   /* PA10 -> USB HS ID */
#define PIN_USB_OTG_HS_VBUS     9    /* PA9  -> USB HS VBUS sense */
#define PIN_TIM1_CH1            8    /* PA8  -> TIM1_CH1 (laser PWM) */
#define PIN_I2C1_SCL            15   /* PA15 -> I2C1 SCL (TSL257 + codec) */
#define PIN_I2C1_SDA            14   /* PA14 unused - I2C1 SDA on PB7 */

/* PORTB */
#define PIN_I2C1_SDA_PB         7    /* PB7  -> I2C1 SDA */
#define PIN_SDMMC1_CMD          2    /* PB2  -> SDMMC1 CMD (alt: boot) */
#define PIN_SDMMC1_CK           12   /* PB12 -> SDMMC1 CK  (alt routing) */
#define PIN_ARM_KEY             3    /* PB3  -> keyed arming switch (HW intlk) */
#define PIN_LASER_EN            4    /* PB4  -> laser driver enable (HW gated) */
#define PIN_LED_STATUS          0    /* PB0  -> status LED (green) */
#define PIN_LED_FAULT           1    /* PB1  -> fault LED (red) */
#define PIN_BTN_USER            8    /* PB8  -> user button */
#define PIN_AMBIENT_IRQ         9    /* PB9  -> TSL257 interrupt (optional) */

/* PORTC */
#define PIN_SDMMC1_D0           8    /* PC8  -> SDMMC1 D0 */
#define PIN_SDMMC1_D1           9    /* PC9  -> SDMMC1 D1 */
#define PIN_SDMMC1_D2           10   /* PC10 -> SDMMC1 D2 */
#define PIN_SDMMC1_D3           11   /* PC11 -> SDMMC1 D3 */
#define PIN_CODEC_RST           13   /* PC13 -> CS4271 reset (optional codec) */

/* PORTD */
#define PIN_USB_HS_DM           11   /* PD11 -> USB OTGHS DM (ULPI-less FS) */
#define PIN_USB_HS_DP           12   /* PD12 -> USB OTGHS DP */

/* ---- GPIO abstraction ------------------------------------------------ */
typedef enum {
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_ANALOG,
    GPIO_MODE_AF
} gpio_mode_t;

typedef enum {
    GPIO_PULL_NONE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
} gpio_pull_t;

typedef struct {
    volatile uint32_t MODER;   /* 0x00 */
    volatile uint32_t OTYPER;  /* 0x04 */
    volatile uint32_t OSPEEDR; /* 0x08 */
    volatile uint32_t PUPDR;   /* 0x0C */
    volatile uint32_t IDR;     /* 0x10 */
    volatile uint32_t ODR;     /* 0x14 */
    volatile uint32_t BSRR;    /* 0x18 */
    volatile uint32_t LCKR;    /* 0x1C */
    volatile uint32_t AFRL;    /* 0x20 */
    volatile uint32_t AFRH;    /* 0x24 */
} ltm_gpio_t;

#define GPIOA ((ltm_gpio_t *)0x40020000UL)
#define GPIOB ((ltm_gpio_t *)0x40020400UL)
#define GPIOC ((ltm_gpio_t *)0x40020800UL)
#define GPIOD ((ltm_gpio_t *)0x40020C00UL)

/* ---- RCC abstraction ------------------------------------------------- */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t ICSCR;    /* 0x04 */
    volatile uint32_t HSITRIM;  /* 0x08 (reserved on H7) */
    volatile uint32_t RESERVED0;
    volatile uint32_t CRRCR;    /* 0x0C CRS — not used */
    /* (abridged; only fields used by this firmware) */
} ltm_rcc_base_t;

#define RCC ((volatile uint32_t *)0x58024400UL) /* D1 domain RCC base */

/* Helper: enable AHB1 peripheral clock (GPIO banks on H7 are on AHB4) */
static inline void ltm_gpio_clk_enable(volatile uint32_t *rcc_ahb4enr,
                                       uint32_t bit) {
    rcc_ahb4enr[0] |= bit;  /* RCC_AHB4ENR offset 0xE0 */
}

/* ---- Board API (implemented in main.c / drivers) --------------------- */
void board_init(void);
void board_led_set(int green, int red);
void board_fault(void);  /* never returns; halts & drops laser */

/* Safety interlock state */
typedef struct {
    uint8_t  arm_key_hw;        /* 1 = key present (HW line high) */
    uint8_t  ambient_safe;      /* 1 = ambient below threshold */
    uint8_t  sw_class1_cap;     /* 1 = software power cap active */
    uint8_t  watchdog_ok;       /* 1 = last kick within window */
    uint8_t  operator_override; /* 1 = operator explicitly overrode */
} ltm_safety_t;

extern volatile ltm_safety_t g_safety;

/* Returns 1 if laser emission is permitted by all interlocks. */
int  board_laser_permit(void);

#endif /* LUMEN_TAP_BOARD_H */