/*
 * board.h — Board configuration for Chronos-Phantom
 * Pin assignments, peripheral mappings, and board-level constants.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_BOARD_H
#define CHRONOS_PHANTOM_BOARD_H

#include <stdint.h>

/* --------------------------------------------------------------------- */
/*  Clock tree                                                           */
/* --------------------------------------------------------------------- */
#define HSE_FREQ_HZ        25000000UL   /* External 25 MHz crystal        */
#define LSE_FREQ_HZ        32768UL      /* RTC LSE                         */
#define SYSCLK_HZ          480000000UL  /* 480 MHz from PLL1               */
#define HCLK_HZ            240000000UL  /* AHB / 2                         */
#define APB1_CLK_HZ        120000000UL   /* APB1 (USART, SPI low-speed)    */
#define APB2_CLK_HZ        120000000UL   /* APB2 (SPI, TIMERS)              */
#define PTP_SYS_TICK_HZ   1000000000UL   /* 1 GHz virtual PTP nanosecond tick */

/* --------------------------------------------------------------------- */
/*  GPIO pin assignments (STM32H753 LQFP-100)                            */
/* --------------------------------------------------------------------- */

/* Port A */
#define PA0   GPIO_PIN(0, 0)   /* ETH RMII RefClk (input from PHY)   */
#define PA1   GPIO_PIN(0, 1)   /* ETH RMII RXD0                      */
#define PA2   GPIO_PIN(0, 2)   /* ETH RMII RXD1                      */
#define PA3   GPIO_PIN(0, 3)   /* ETH RMII RXDV (CRS_DV)             */
#define PA4   GPIO_PIN(0, 4)   /* DAC1_OUT1 → TCXO control voltage   */
#define PA5   GPIO_PIN(0, 5)   /* DAC1_OUT2 (unused / spare)         */
#define PA7   GPIO_PIN(0, 7)   /* ETH RMII TXEN                      */
#define PA8   GPIO_PIN(0, 8)   /* USART1_TX → ESP32-C3 RX            */
#define PA9   GPIO_PIN(0, 9)   /* USART1_RX ← ESP32-C3 TX            */
#define PA10  GPIO_PIN(0, 10)  /* USB_DM                              */
#define PA11  GPIO_PIN(0, 11)  /* USB_DP                              */
#define PA15  GPIO_PIN(0, 15)  /* SPI3_NSS → flash (opt)            */

/* Port B */
#define PB0   GPIO_PIN(1, 0)   /* TIM3_CH3 → status LED 1 (LINK_A)  */
#define PB1   GPIO_PIN(1, 1)   /* TIM3_CH4 → status LED 2 (LINK_B)  */
#define PB2   GPIO_PIN(1, 2)   /* BOOT1                              */
#define PB3   GPIO_PIN(1, 3)   /* SPI1_SCK                           */
#define PB4   GPIO_PIN(1, 4)   /* SPI1_MISO                          */
#define PB5   GPIO_PIN(1, 5)   /* SPI1_MOSI                          */
#define PB6   GPIO_PIN(1, 6)   /* TIM4_CH1 → status LED 3 (PTP_LOCK) */
#define PB7   GPIO_PIN(1, 7)   /* TIM4_CH2 → status LED 4 (BLE)      */
#define PB10  GPIO_PIN(1, 10)  /* USART3_TX → GNSS NEO-M9N RX       */
#define PB11  GPIO_PIN(1, 11)  /* USART3_RX ← GNSS NEO-M9N TX       */
#define PB12  GPIO_PIN(1, 12)  /* I2C2_SCL → IMU LSM6DSO             */
#define PB13  GPIO_PIN(1, 13)  /* I2C2_SDA → IMU LSM6DSO             */
#define PB14  GPIO_PIN(1, 14)  /* Mode button (input, active-low)    */

/* Port C */
#define PC1   GPIO_PIN(2, 1)   /* ETH RMII MDIO                      */
#define PC4   GPIO_PIN(2, 4)   /* ETH RMII RXD2 (RGMII only)         */
#define PC5   GPIO_PIN(2, 5)   /* ETH RMII RXD3 (RGMII only)         */
#define PC6   GPIO_PIN(2, 6)   /* TIM8_CH1 → IMU INT1                */
#define PC7   GPIO_PIN(2, 7)   /* IMU INT2                           */
#define PC8   GPIO_PIN(2, 8)   /* 1-PPS input from GNSS (TIM8_CH2)  */
#define PC9   GPIO_PIN(2, 9)   /* GNSS power enable                  */
#define PC14  GPIO_PIN(2, 14)  /* ETH RMII TXD0                      */
#define PC15  GPIO_PIN(2, 15)  /* ETH RMII TXD1                      */

/* Port D */
#define PD0   GPIO_PIN(3, 0)   /* ETH RMII MDC                       */
#define PD2   GPIO_PIN(3, 2)   /* ETH RMII TX_CLK (out to PHY)      */

/* --------------------------------------------------------------------- */
/*  Status LEDs (active-high via TIM PWM channels)                       */
/* --------------------------------------------------------------------- */
#define LED_LINK_A          0
#define LED_LINK_B          1
#define LED_PTP_LOCK        2
#define LED_BLE_ACTIVE      3

/* --------------------------------------------------------------------- */
/*  TCXO control (DAC1 output, 0-3.3V → 0-10 MHz pull)                    */
/* --------------------------------------------------------------------- */
#define TCXO_DAC_CHANNEL    1
#define TCXO_DAC_FULLSCALE   4095
#define TCXO_DAC_CENTER     2048   /* mid-scale ≈ nominal frequency      */

/* --------------------------------------------------------------------- */
/*  BLE UART (USART1 @ 2 Mbps)                                           */
/* --------------------------------------------------------------------- */
#define BLE_UART_BAUD        2000000UL
#define BLE_UART_BUF_SIZE    512

/* --------------------------------------------------------------------- */
/*  GNSS UART (USART3 @ 38400)                                           */
/* --------------------------------------------------------------------- */
#define GNSS_UART_BAUD       38400UL
#define GNSS_UART_BUF_SIZE   256

/* --------------------------------------------------------------------- */
/*  IMU (I2C2)                                                            */
/* --------------------------------------------------------------------- */
#define IMU_I2C_ADDR         0x6A   /* LSM6DSO 7-bit address              */
#define IMU_TAMPER_THRES_MG  1500   /* 1.5 g movement triggers tamper   */

/* --------------------------------------------------------------------- */
/*  Ethernet / PTP                                                        */
/* --------------------------------------------------------------------- */
#define ETH_RX_DESC_COUNT   8
#define ETH_TX_DESC_COUNT   8
#define ETH_BUF_SIZE        1536
#define PTP_ETHERTYPE       0x88F7
#define NTP_UDP_PORT        123

/* --------------------------------------------------------------------- */
/*  Operating modes                                                       */
/* --------------------------------------------------------------------- */
typedef enum {
    MODE_INLINE_MITM = 0,
    MODE_ROGUE_GM,
    MODE_PASSIVE_SNIFF,
    MODE_TRANSPARENT_ONLY,   /* benign passthrough (post-tamper)         */
    MODE_COUNT
} op_mode_t;

/* --------------------------------------------------------------------- */
/*  Battery / power constants                                             */
/* --------------------------------------------------------------------- */
#define BATT_ADC_CHANNEL    12
#define BATT_FULL_MV        4200
#define BATT_EMPTY_MV       3200
#define POE_PRESENT_PIN     GPIO_PIN(2, 13)  /* PC13 */
#define USB_PRESENT_PIN     GPIO_PIN(2, 14) /* PC14 */

/* --------------------------------------------------------------------- */
/*  Convenience macros                                                    */
/* --------------------------------------------------------------------- */
#define GPIO_PIN(port, pin)  (((port) << 8) | (pin))
#define GPIO_PORT(gp)        ((gp) >> 8)
#define GPIO_NUM(gp)         ((gp) & 0xFF)

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))

#endif /* CHRONOS_PHANTOM_BOARD_H */