/*
 * can.c — CAN bus driver implementation for STM32F407 bxCAN
 * Dual-channel support (CAN1, CAN2)
 */

#include "can.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ========== Private Constants ========== */
#define CAN1_BASE_ADDR  0x40006400UL
#define CAN2_BASE_ADDR  0x40006800UL
#define CAN_FILTER_BASE 0x40006600UL

/* CAN register offsets */
#define CAN_MCR_OFF     0x00
#define CAN_MSR_OFF     0x04
#define CAN_TSR_OFF     0x08
#define CAN_RF0R_OFF    0x0C
#define CAN_RF1R_OFF    0x10
#define CAN_IER_OFF     0x14
#define CAN_ESR_OFF     0x18
#define CAN_BTR_OFF     0x1C

/* TX mailbox register offsets (each mailbox is 16 bytes) */
#define CAN_TIR_OFF(n)  (0x180 + (n) * 0x10)
#define CAN_TDTR_OFF(n) (0x184 + (n) * 0x10)
#define CAN_TDLR_OFF(n) (0x188 + (n) * 0x10)
#define CAN_TDHR_OFF(n) (0x18C + (n) * 0x10)

/* RX FIFO register offsets (each FIFO is 16 bytes) */
#define CAN_RIR_OFF(f)   (0x1B0 + (f) * 0x10)   /* Not standard — use sFIFOMailBox */
#define CAN_RDLR_OFF     0x1B4
#define CAN_RDHR_OFF     0x1BC

/* Filter register offsets */
#define CAN_FMR_OFF     0x200
#define CAN_FM1R_OFF    0x204
#define CAN_FS1R_OFF    0x20C
#define CAN_FFA1R_OFF   0x214
#define CAN_FA1R_OFF    0x21C

/* ========== Private Macros ========== */
#define CAN_REG(base, off)   (*(volatile uint32_t *)((base) + (off)))
#define CAN_MCRx(base)       CAN_REG(base, CAN_MCR_OFF)
#define CAN_MSRx(base)       CAN_REG(base, CAN_MSR_OFF)
#define CAN_TSRx(base)       CAN_REG(base, CAN_TSR_OFF)
#define CAN_RF0Rx(base)      CAN_REG(base, CAN_RF0R_OFF)
#define CAN_IERx(base)       CAN_REG(base, CAN_IER_OFF)
#define CAN_ESRx(base)       CAN_REG(base, CAN_ESR_OFF)
#define CAN_BTRx(base)       CAN_REG(base, CAN_BTR_OFF)

/* ========== Private Variables ========== */
static can_rx_callback_t rx_callbacks[2] = {NULL, NULL};

static inline uint32_t get_base(uint8_t channel) {
    return (channel == 0) ? CAN1_BASE_ADDR : CAN2_BASE_ADDR;
}

/* ========== Baud Rate Configuration ========== */
static void can_set_baud(uint32_t base, can_baudrate_t baud) {
    /* APB1 = 42 MHz, APB1 timer = 84 MHz
     * CAN is on APB1, but bxCAN uses APB1 clock directly
     * For 42 MHz APB1:
     *   1 Mbps:  BRP=2, TS1=13, TS2=2, SJW=1 → 42MHz/(2+1)/(1+13+2) = 1.0 Mbps
     *   500 Kbps: BRP=5, TS1=13, TS2=2, SJW=1 → 42MHz/(5+1)/(1+13+2) = 500 Kbps
     *   250 Kbps: BRP=11, TS1=13, TS2=2, SJW=1 → 42MHz/(11+1)/(1+13+2) = 250 Kbps
     *   125 Kbps: BRP=23, TS1=13, TS2=2, SJW=1 → 42MHz/(23+1)/(1+13+2) = 125 Kbps
     */
    uint32_t btr = 0;
    switch (baud) {
        case CAN_BAUD_125K:
            btr = CAN_BTR_BRP(23) | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1);
            break;
        case CAN_BAUD_250K:
            btr = CAN_BTR_BRP(11) | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1);
            break;
        case CAN_BAUD_500K:
            btr = CAN_BTR_BRP(5) | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1);
            break;
        case CAN_BAUD_1M:
            btr = CAN_BTR_BRP(2) | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1);
            break;
        default:
            btr = CAN_BTR_BRP(5) | CAN_BTR_TS1(13) | CAN_BTR_TS2(2) | CAN_BTR_SJW(1);
            break;
    }
    CAN_BTRx(base) = btr;
}

/* ========== Initialize CAN ========== */
int can_init(uint8_t channel, can_baudrate_t baud, can_mode_t mode) {
    uint32_t base = get_base(channel);

    /* Enable CAN peripheral clocks */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    if (channel == 1) {
        RCC->APB1ENR |= RCC_APB1ENR_CAN2EN;
    }

    /* Exit sleep mode */
    CAN_MCRx(base) &= ~CAN_MCR_SLEEP;

    /* Request initialization mode */
    CAN_MCRx(base) |= CAN_MCR_INRQ;
    uint32_t timeout = 100000;
    while (!(CAN_MSRx(base) & (1 << 0))) {
        if (--timeout == 0) return -1;
    }

    /* Configure MCR:
     * - NART: No automatic retransmission (for injection control)
     * - ABOM: Automatic bus-off management
     * - TTCM: Time-triggered communication (for timestamps)
     */
    CAN_MCRx(base) = CAN_MCR_NART | CAN_MCR_ABOM | CAN_MCR_TTCM;

    /* Set baud rate */
    can_set_baud(base, baud);

    /* Set mode */
    uint32_t btr = CAN_BTRx(base);
    if (mode == CAN_MODE_LISTEN) {
        btr |= CAN_BTR_SILM;
        btr &= ~CAN_BTR_LBKM;
    } else if (mode == CAN_MODE_LOOPBACK) {
        btr &= ~CAN_BTR_SILM;
        btr |= CAN_BTR_LBKM;
    } else if (mode == CAN_MODE_SILENT_LOOPBACK) {
        btr |= CAN_BTR_SILM | CAN_BTR_LBKM;
    } else {
        btr &= ~(CAN_BTR_SILM | CAN_BTR_LBKM);
    }
    CAN_BTRx(base) = btr;

    /* Exit initialization mode */
    CAN_MCRx(base) &= ~CAN_MCR_INRQ;
    timeout = 100000;
    while ((CAN_MSRx(base) & (1 << 0))) {
        if (--timeout == 0) return -2;
    }

    /* Set sniffer filter (accept all frames) */
    can_set_sniffer(channel);

    /* Configure NVIC interrupts */
    if (channel == 0) {
        NVIC_EnableIRQ(CAN1_RX0_IRQn);
        NVIC_EnableIRQ(CAN1_RX1_IRQn);
        NVIC_SetPriority(CAN1_RX0_IRQn, 1);
        NVIC_SetPriority(CAN1_RX1_IRQn, 1);
    } else {
        NVIC_EnableIRQ(CAN2_RX0_IRQn);
        NVIC_SetPriority(CAN2_RX0_IRQn, 1);
    }

    return 0;
}

/* ========== Set Sniffer Filter ========== */
int can_set_sniffer(uint8_t channel) {
    /* CAN1 uses filter banks 0-13, CAN2 uses filter banks 14-27 */
    uint8_t bank = (channel == 0) ? 0 : 14;

    /* Enter filter init mode */
    volatile uint32_t *fmr = (uint32_t *)(CAN1_BASE_ADDR + CAN_FMR_OFF);
    *fmr |= (1 << 0);  /* FINIT */

    /* Set filter bank for all-accept */
    volatile uint32_t *fs1r = (uint32_t *)(CAN1_BASE_ADDR + CAN_FS1R_OFF);
    *fs1r |= (1 << bank);  /* 32-bit scale for this bank */

    volatile uint32_t *fm1r = (uint32_t *)(CAN1_BASE_ADDR + CAN_FM1R_OFF);
    *fm1r &= ~(1 << bank);  /* Mask mode */

    /* Set filter values: accept all (mask = 0, ID = 0) */
    volatile uint32_t *filter = (uint32_t *)(CAN1_BASE_ADDR + 0x240 + bank * 8);
    filter[0] = 0x00000000;  /* FR1: ID */
    filter[1] = 0x00000000;  /* FR2: Mask (0 = don't care = accept all) */

    /* Activate filter */
    volatile uint32_t *fa1r = (uint32_t *)(CAN1_BASE_ADDR + CAN_FA1R_OFF);
    *fa1r |= (1 << bank);

    /* Assign to FIFO 0 */
    volatile uint32_t *ffa1r = (uint32_t *)(CAN1_BASE_ADDR + CAN_FFA1R_OFF);
    *ffa1r &= ~(1 << bank);  /* FIFO 0 */

    /* Exit filter init mode */
    *fmr &= ~(1 << 0);

    return 0;
}

/* ========== Transmit ========== */
int can_transmit(uint8_t channel, const can_frame_t *frame) {
    uint32_t base = get_base(channel);

    /* Find an empty mailbox */
    uint32_t tsr = CAN_TSRx(base);
    uint8_t mailbox;
    if (tsr & CAN_TSR_TME0)      mailbox = 0;
    else if (tsr & CAN_TSR_TME1)  mailbox = 1;
    else if (tsr & CAN_TSR_TME2)  mailbox = 2;
    else return -1;  /* All mailboxes full */

    /* Build TIxR (Transmit Identifier Register) */
    uint32_t tir = 0;
    if (frame->id_type == CAN_EXT_ID) {
        tir |= (1 << 2);                         /* IDE = 1 (extended) */
        tir |= ((frame->id & 0x1FFFFFFF) << 3); /* Extended ID */
    } else {
        tir &= ~(1 << 2);                        /* IDE = 0 (standard) */
        tir |= ((frame->id & 0x7FF) << 21);     /* Standard ID */
    }
    if (frame->rtr) tir |= (1 << 1);  /* RTR bit */
    tir |= ((frame->dlc & 0xF) << 16); /* DLC */

    /* Write to mailbox */
    CAN_REG(base, CAN_TIR_OFF(mailbox)) = tir;
    CAN_REG(base, CAN_TDLR_OFF(mailbox)) =
        ((uint32_t)frame->data[3] << 24) | ((uint32_t)frame->data[2] << 16) |
        ((uint32_t)frame->data[1] << 8)  | (uint32_t)frame->data[0];
    CAN_REG(base, CAN_TDHR_OFF(mailbox)) =
        ((uint32_t)frame->data[7] << 24) | ((uint32_t)frame->data[6] << 16) |
        ((uint32_t)frame->data[5] << 8)  | (uint32_t)frame->data[4];

    /* Request transmission */
    CAN_REG(base, CAN_TIR_OFF(mailbox)) |= (1 << 0);  /* TXRQ */

    return 0;
}

/* ========== Receive (polling) ========== */
int can_receive(uint8_t channel, can_frame_t *frame, uint32_t timeout_ms) {
    uint32_t base = get_base(channel);
    uint32_t fifo = 0;  /* Using FIFO 0 */
    volatile uint32_t *rfxr = (fifo == 0) ?
        &CAN_REG(base, CAN_RF0R_OFF) : &CAN_REG(base, CAN_RF1R_OFF);

    /* Wait for message */
    uint32_t timeout = timeout_ms * 42000;  /* Approx loop count */
    while (!(*rfxr & (1 << 0))) {  /* FMP bits */
        if (--timeout == 0) return -1;
    }

    /* Read frame from FIFO */
    can_frame_from_fifo(base, fifo, frame);
    return 0;
}

/* ========== Read frame from FIFO ========== */
static void can_frame_from_fifo(uint32_t base, uint8_t fifo, can_frame_t *frame) {
    /* Access FIFO mailbox registers */
    volatile uint32_t *rir = &CAN_REG(base, 0x1B0 + fifo * 0x10);
    volatile uint32_t *rdlr = &CAN_REG(base, 0x1B4 + fifo * 0x10);
    volatile uint32_t *rdhr = &CAN_REG(base, 0x1BC + fifo * 0x10);

    uint32_t rir_val = *rir;

    /* Decode identifier */
    frame->id_type = (rir_val & (1 << 2)) ? CAN_EXT_ID : CAN_STD_ID;
    if (frame->id_type == CAN_EXT_ID) {
        frame->id = (rir_val >> 3) & 0x1FFFFFFF;
    } else {
        frame->id = (rir_val >> 21) & 0x7FF;
    }

    frame->rtr = (rir_val >> 1) & 1;
    frame->dlc = (rir_val >> 16) & 0xF;

    /* Read data */
    uint32_t rdl = *rdlr;
    uint32_t rdh = *rdhr;
    for (int i = 0; i < 4; i++) frame->data[i]   = (rdl >> (i * 8)) & 0xFF;
    for (int i = 0; i < 4; i++) frame->data[i+4] = (rdh >> (i * 8)) & 0xFF;

    /* Timestamp from TIM2 */
    frame->timestamp_us = TIM2->CNT;

    /* Release FIFO */
    volatile uint32_t *rf0r = &CAN_REG(base, CAN_RF0R_OFF);
    *rf0r |= (1 << 5);  /* RFOM0: Release FIFO 0 output mailbox */
}

/* ========== Register RX Callback ========== */
void can_register_rx_callback(uint8_t channel, can_rx_callback_t cb) {
    if (channel < 2) rx_callbacks[channel] = cb;
}

/* ========== Enable/Disable Interrupts ========== */
void can_enable_interrupts(uint8_t channel) {
    uint32_t base = get_base(channel);
    CAN_IERx(base) |= CAN_IER_FMPIE0 | CAN_IER_EWGIE | CAN_IER_EPVIE | CAN_IER_BOFIE;
}

void can_disable_interrupts(uint8_t channel) {
    uint32_t base = get_base(channel);
    CAN_IERx(base) &= ~(CAN_IER_FMPIE0 | CAN_IER_EWGIE | CAN_IER_EPVIE | CAN_IER_BOFIE);
}

/* ========== Error State ========== */
uint8_t can_get_error_state(uint8_t channel) {
    uint32_t base = get_base(channel);
    uint32_t esr = CAN_ESRx(base);
    uint8_t state = 0;
    if (esr & CAN_ESR_EWGF) state |= 0x01;  /* Error warning */
    if (esr & CAN_ESR_EPVF) state |= 0x02;  /* Error passive */
    if (esr & CAN_ESR_BOFF) state |= 0x04;  /* Bus-off */
    return state;
}

/* ========== Reset Bus ========== */
void can_reset_bus(uint8_t channel) {
    uint32_t base = get_base(channel);
    /* With ABOM enabled, bus-off recovery is automatic.
     * Force recovery by requesting init mode then exiting. */
    CAN_MCRx(base) |= CAN_MCR_INRQ;
    for (volatile int i = 0; i < 10000; i++);
    CAN_MCRx(base) &= ~CAN_MCR_INRQ;
    for (volatile int i = 0; i < 10000; i++);
}

/* ========== IRQ Handler ========== */
void can_irq_handler(uint8_t channel) {
    uint32_t base = get_base(channel);

    /* Check FIFO 0 message pending */
    if (CAN_RF0Rx(base) & (1 << 0)) {
        can_frame_t frame;
        can_frame_from_fifo(base, 0, &frame);

        if (rx_callbacks[channel]) {
            rx_callbacks[channel](&frame, channel);
        }
    }

    /* Check for errors */
    if (CAN_ESRx(base) & (CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF)) {
        error_count[channel]++;
    }
}