/*
 * board.h — Hardware description for BLE Phantom
 *
 * Pin assignments, clock config, and hardware constants.
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* ============================================================
 * Clock configuration
 * ============================================================ */
#define CLOCK_HSE_FREQ      8000000U    /* 8 MHz crystal */
#define CLOCK_SYS_FREQ      84000000U   /* 84 MHz SYSCLK */
#define CLOCK_AHB_FREQ      84000000U   /* 84 MHz AHB */
#define CLOCK_APB1_FREQ     21000000U   /* 21 MHz APB1 */
#define CLOCK_APB2_FREQ     42000000U   /* 42 MHz APB2 */

/* ============================================================
 * GPIO pin assignments
 * ============================================================ */

/* Radio A IRQ and control */
#define PIN_RADIO_A_IRQ      0    /* PA0 - EXTI0 */
#define PORT_RADIO_A_IRQ     'A'
#define PIN_RADIO_A_RST      4    /* PA4 */
#define PORT_RADIO_A_RST     'A'
#define PIN_RADIO_A_CS       8    /* PA8 */
#define PORT_RADIO_A_CS      'A'

/* Radio B IRQ and control */
#define PIN_RADIO_B_IRQ      1    /* PA1 - EXTI1 */
#define PORT_RADIO_B_IRQ     'A'
#define PIN_RADIO_B_RST      2    /* PB2 */
#define PORT_RADIO_B_RST     'B'
#define PIN_RADIO_B_CS       12   /* PB12 */
#define PORT_RADIO_B_CS      'B'

/* SPI1 (Radio A) pins */
#define PIN_SPI1_SCK         5    /* PA5 */
#define PIN_SPI1_MISO        6    /* PA6 */
#define PIN_SPI1_MOSI        7    /* PA7 */

/* SPI2 (Radio B) pins */
#define PIN_SPI2_SCK         10   /* PB10 */
#define PIN_SPI2_MISO        11   /* PB11 */
#define PIN_SPI2_MOSI        15   /* PB15 */

/* USB pins */
#define PIN_USB_DM           11   /* PA11 */
#define PIN_USB_DP           12   /* PA12 */
#define PIN_VBUS_DETECT      9    /* PA9 */

/* I2C1 pins (battery gauge) */
#define PIN_I2C1_SDA         5    /* PB5 */
#define PIN_I2C1_SCL         6    /* PB6 */

/* LEDs (active low) */
#define PIN_LED_RADIO_A      0    /* PB0 */
#define PORT_LED_RADIO_A     'B'
#define PIN_LED_RADIO_B      1    /* PB1 */
#define PORT_LED_RADIO_B     'B'
#define PIN_LED_USB          14   /* PC14 */
#define PORT_LED_USB         'C'
#define PIN_LED_POWER        15   /* PC15 */
#define PORT_LED_POWER       'C'

/* User button (active low) */
#define PIN_USER_BUTTON      13   /* PC13 */
#define PORT_USER_BUTTON     'C'

/* Debug UART */
#define PIN_UART_TX          2    /* PA2 */
#define PIN_UART_RX          3    /* PA3 */

/* ============================================================
 * SPI configuration
 * ============================================================ */
#define SPI1_SPEED_HZ       8000000U    /* 8 MHz SPI clock */
#define SPI2_SPEED_HZ       8000000U    /* 8 MHz SPI clock */

/* SPI CR1 baud rate divisors for 42 MHz APB2 clock:
 * BR=0: /2 = 21 MHz  BR=1: /4 = 10.5 MHz  BR=2: /8 = 5.25 MHz
 * BR=3: /16 = 2.6 MHz  BR=4: /32 = 1.3 MHz  BR=5: /64 = 656 kHz
 * For 8 MHz target: /4 = 10.5 MHz (closest above 8 MHz)
 */
#define SPI_CR1_BR_FOR_8MHZ  1  /* /4 = 10.5 MHz */

/* Radio instances */
#define RADIO_A_SPI          SPI1
#define RADIO_B_SPI          SPI2

/* ============================================================
 * DMA configuration
 * ============================================================ */
#define DMA_RADIO_A_STREAM   3   /* DMA1 Stream 3 for SPI1_RX */
#define DMA_RADIO_A_CHANNEL  3   /* Channel 3 */
#define DMA_RADIO_B_STREAM   5   /* DMA1 Stream 5 for SPI2_RX */
#define DMA_RADIO_B_CHANNEL  5   /* Channel 5 */

/* ============================================================
 * Ring buffer sizes
 * ============================================================ */
#define RADIO_RING_BUF_SIZE  (32 * 1024)   /* 32 KB per radio */
#define USB_TX_BUF_SIZE      512
#define USB_RX_BUF_SIZE      512

/* ============================================================
 * Battery / power
 * ============================================================ */
#define BATTERY_CAPACITY_MAH  600
#define VBUS_DETECT_PIN        9    /* PA9 */
#define VBUS_DETECT_PORT       'A'

/* ============================================================
 * LED macros (active low — writing 0 turns LED ON)
 * ============================================================ */
#define LED_RADIO_A_ON()     GPIOB->ODR &= ~(1U << PIN_LED_RADIO_A)
#define LED_RADIO_A_OFF()    GPIOB->ODR |= (1U << PIN_LED_RADIO_A)
#define LED_RADIO_A_TOGGLE() GPIOB->ODR ^= (1U << PIN_LED_RADIO_A)

#define LED_RADIO_B_ON()     GPIOB->ODR &= ~(1U << PIN_LED_RADIO_B)
#define LED_RADIO_B_OFF()    GPIOB->ODR |= (1U << PIN_LED_RADIO_B)
#define LED_RADIO_B_TOGGLE() GPIOB->ODR ^= (1U << PIN_LED_RADIO_B)

#define LED_USB_ON()         GPIOC->ODR &= ~(1U << PIN_LED_USB)
#define LED_USB_OFF()        GPIOC->ODR |= (1U << PIN_LED_USB)
#define LED_USB_TOGGLE()     GPIOC->ODR ^= (1U << PIN_LED_USB)

#define LED_POWER_ON()       GPIOC->ODR &= ~(1U << PIN_LED_POWER)
#define LED_POWER_OFF()      GPIOC->ODR |= (1U << PIN_LED_POWER)
#define LED_POWER_TOGGLE()   GPIOC->ODR ^= (1U << PIN_LED_POWER)

/* ============================================================
 * Button macro (active low — pressed = 0)
 * ============================================================ */
#define BUTTON_PRESSED()     !(GPIOC->IDR & (1U << PIN_USER_BUTTON))

/* ============================================================
 * Chip select macros (active low)
 * ============================================================ */
#define CS_A_ASSERT()        GPIOA->ODR &= ~(1U << PIN_RADIO_A_CS)
#define CS_A_DEASSERT()      GPIOA->ODR |= (1U << PIN_RADIO_A_CS)

#define CS_B_ASSERT()        GPIOB->ODR &= ~(1U << PIN_RADIO_B_CS)
#define CS_B_DEASSERT()      GPIOB->ODR |= (1U << PIN_RADIO_B_CS)

/* ============================================================
 * Reset macros (active low)
 * ============================================================ */
#define RESET_A_ASSERT()     GPIOA->ODR &= ~(1U << PIN_RADIO_A_RST)
#define RESET_A_DEASSERT()   GPIOA->ODR |= (1U << PIN_RADIO_A_RST)

#define RESET_B_ASSERT()     GPIOB->ODR &= ~(1U << PIN_RADIO_B_RST)
#define RESET_B_DEASSERT()   GPIOB->ODR |= (1U << PIN_RADIO_B_RST)

/* ============================================================
 * USB descriptors are in usb_descriptors.h
 * ============================================================ */

#endif /* BOARD_H */