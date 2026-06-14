/*
 * can.h — CAN bus driver for STM32F407 bxCAN
 */

#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>
#include <stddef.h>

#define CAN_STD_ID   0
#define CAN_EXT_ID   1
#define CAN_MAX_DATA_LEN  8

typedef struct {
    uint32_t id;
    uint8_t  id_type;       /* CAN_STD_ID or CAN_EXT_ID */
    uint8_t  dlc;           /* Data Length Code (0-8) */
    uint8_t  data[8];
    uint8_t  rtr;           /* Remote Transmission Request */
    uint32_t timestamp_us;  /* Hardware timestamp from TIM2 */
} can_frame_t;

typedef enum {
    CAN_BAUD_125K  = 0,
    CAN_BAUD_250K  = 1,
    CAN_BAUD_500K  = 2,
    CAN_BAUD_1M    = 3,
    CAN_BAUD_CUSTOM = 4
} can_baudrate_t;

typedef enum {
    CAN_MODE_NORMAL    = 0,
    CAN_MODE_LISTEN    = 1,
    CAN_MODE_LOOPBACK  = 2,
    CAN_MODE_SILENT_LOOPBACK = 3
} can_mode_t;

typedef void (*can_rx_callback_t)(const can_frame_t *frame, uint8_t channel);

/* Initialize CAN peripheral */
int can_init(uint8_t channel, can_baudrate_t baud, can_mode_t mode);

/* Set hardware acceptance filter */
int can_set_filter(uint8_t channel, uint32_t id, uint32_t mask, uint8_t bank);

/* Set sniffer mode (accept all frames) */
int can_set_sniffer(uint8_t channel);

/* Transmit a CAN frame */
int can_transmit(uint8_t channel, const can_frame_t *frame);

/* Receive a CAN frame (polling) */
int can_receive(uint8_t channel, can_frame_t *frame, uint32_t timeout_ms);

/* Register RX callback for interrupt-driven reception */
void can_register_rx_callback(uint8_t channel, can_rx_callback_t cb);

/* Enable/disable CAN interrupts */
void can_enable_interrupts(uint8_t channel);
void can_disable_interrupts(uint8_t channel);

/* Get error state */
uint8_t can_get_error_state(uint8_t channel);

/* Reset bus (bus-off recovery) */
void can_reset_bus(uint8_t channel);

/* IRQ handler (called from interrupt) */
void can_irq_handler(uint8_t channel);

#endif /* CAN_DRIVER_H */