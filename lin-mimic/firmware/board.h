/* LIN Mimic board support contract
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef LIN_MIMIC_BOARD_H
#define LIN_MIMIC_BOARD_H
#include <stdint.h>
#include <stdbool.h>

#define BOARD_NAME "LIN Mimic R1"
#define CPU_HZ 170000000u
#define LIN_UART_HZ 19200u
#define CAPTURE_DEPTH 256u
#define USB_FRAME_MAX 96u
#define SAFE_MAX_TX_PER_SEC 100u
#define SAFE_DOMINANT_TIMEOUT_US 1500u

/* R1 uses STM32G474: USART2 sees the target side and USART3 the controller
 * side. Two TJA1021 transceivers permit a true fail-open inline topology. */
enum board_pin {
    PIN_LIN_A_TX_PA = 2, PIN_LIN_A_RX_PA = 3, PIN_LIN_A_SLP_PA = 4,
    PIN_LIN_B_TX_PB = 10, PIN_LIN_B_RX_PB = 11, PIN_LIN_B_SLP_PB = 12,
    PIN_RELAY_BYPASS_PB = 4, PIN_ARM_BUTTON_PB = 5, PIN_STATUS_LED_PB = 6,
    PIN_TARGET_SENSE_PA = 0, PIN_CURRENT_SENSE_PA = 1
};

typedef struct {
    uint32_t now_us;
    bool arm_pressed;
    bool usb_ready;
    bool bypass_closed;
    uint16_t target_mv;
    int16_t target_ma;
} board_status_t;

void board_init(void);
uint32_t board_micros(void);
void board_delay_us(uint32_t delay);
void board_watchdog_kick(void);
void board_set_bypass(bool transparent);
void board_set_transceiver(unsigned channel, bool enabled);
bool board_read_arm(void);
uint16_t board_target_mv(void);
int16_t board_target_ma(void);
int board_uart_read(unsigned channel, uint8_t *value);
void board_uart_write(unsigned channel, uint8_t value);
int board_usb_read(uint8_t *dst, unsigned capacity);
void board_usb_write(const uint8_t *src, unsigned length);
void board_led_pattern(unsigned pattern);
#endif
