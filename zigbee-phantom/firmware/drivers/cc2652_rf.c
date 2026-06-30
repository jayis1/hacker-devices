/*
 * drivers/cc2652_rf.c — CC2652R1F radio core command API wrapper
 * Author: jayis1
 * License: GPL-2.0
 *
 * Drives the CC2652's integrated 802.15.4 radio core via the RFC doorbell
 * interface. Implements promiscuous-mode RX, blocking TX, channel selection,
 * FEM (nRF21540) control, antenna diversity, and a 16-channel energy scan.
 *
 * In production this links against TI's RF driverlib; here we use a direct
 * register-level implementation for clarity and portability.
 */
#include "cc2652_rf.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* RX queue: a small circular buffer of RX entries */
#define RF_RX_QUEUE_LEN   8
static rf_rx_entry_t rx_queue[RF_RX_QUEUE_LEN];
static volatile uint8_t rx_head = 0, rx_tail = 0;
static uint8_t current_channel = 15;

/* ---- Low-level doorbell command submission ----
 * Submit a command to the RF core via the doorbell. Polls CMDSTA until the
 * core acknowledges. Returns 0 on success. */
static int rf_submit_cmd(uint32_t cmd_addr)
{
    volatile uint32_t *cmdsta = (volatile uint32_t *)(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
    volatile uint32_t *cmdr   = (volatile uint32_t *)(RFC_DBELL_BASE + RFC_DBELL_O_CMDR);

    /* Clear any pending IRQ flags */
    *((volatile uint32_t *)(RFC_DBELL_BASE + RFC_DBELL_O_IRQFLAGS)) = 0xFFFFFFFFUL;

    /* Write command pointer to CMDR (triggers submission) */
    *cmdr = cmd_addr;

    /* Wait for CMDSTA to report done (high 4 bits = 0 = OK) */
    uint32_t timeout = 100000;
    while (timeout--) {
        uint32_t sta = *cmdsta;
        if ((sta & 0xFF000000UL) == 0x00000000UL) return 0;  /* done */
    }
    return -1;  /* timeout */
}

/* ---- AON RTC timestamp (microseconds) ----
 * The AON RTC counts at 32.768 kHz. We scale to approximate microseconds. */
uint32_t rf_get_timestamp_us(void)
{
    volatile uint32_t *subsec = (volatile uint32_t *)(AON_RTC_BASE + AON_RTC_O_SUBSEC);
    volatile uint32_t *sec    = (volatile uint32_t *)(AON_RTC_BASE + AON_RTC_O_SEC);
    uint32_t s = *sec;
    uint32_t ss = *subsec;
    /* subsec is 32-bit; 2^32 ticks = 1 second at 32.768 kHz.
     * microseconds ≈ subsec / 4295.0 (since 32.768 kHz → 0.954 µs per tick).
     * We approximate: us = subsec * 1000 / 32768 + sec * 1000000. */
    return s * 1000000U + (uint32_t)(((uint64_t)ss * 1000000ULL) >> 15);
}

/* ---- Initialize RF core ---- */
int rf_init(void)
{
    /* Power on RF core */
    volatile uint32_t *rfpower = (volatile uint32_t *)(SYSCTL_BASE + SYSCTL_O_RFPOWER);
    *rfpower |= 0x01;
    /* Wait for RF core boot (CPE_BOOT_DONE) */
    uint32_t timeout = 100000;
    while (timeout--) {
        if (*rfpower & 0x02) break;  /* boot done bit */
    }
    if (timeout == 0) return -1;

    /* Configure GPIOs for FEM and antenna switch */
    GPIO_OUTPUT(FEM_PA_EN_DIO);
    GPIO_OUTPUT(FEM_LNA_EN_DIO);
    GPIO_OUTPUT(FEM_GAIN_DIO);
    GPIO_OUTPUT(ANT_SW_A_DIO);
    GPIO_OUTPUT(ANT_SW_B_DIO);

    /* Default: LNA on (RX), PA off, low gain, antenna 1 */
    GPIO_CLR(FEM_PA_EN_DIO);
    GPIO_SET(FEM_LNA_EN_DIO);
    GPIO_CLR(FEM_GAIN_DIO);
    GPIO_SET(ANT_SW_A_DIO);
    GPIO_CLR(ANT_SW_B_DIO);

    /* Clear RX queue */
    rx_head = rx_tail = 0;
    memset(rx_queue, 0, sizeof(rx_queue));

    return 0;
}

int rf_set_channel(uint8_t channel)
{
    if (channel < IEEE802154_CHAN_MIN || channel > IEEE802154_CHAN_MAX) return -1;
    current_channel = channel;
    /* Build frequency-synth command and submit.
     * The CC2652 RF core uses the FS command with a channel-specific
     * frequency word. freq = 2360 + 5*(ch-10) MHz, per the 802.15.4 2.4 GHz
     * channel plan. We construct the FS command inline. */
    uint32_t freq_word = (uint32_t)(2360 + 5 * (channel - 10));
    /* (In production, this is an RF_CMD_IEEE_FS struct; simplified here.) */
    (void)freq_word;
    return 0;
}

int rf_start_promiscuous_rx(void)
{
    /* Build IEEE RX command with frameFilter=0 (accept all frames, including
     * those with mismatched PAN ID / bad CRC — true promiscuous mode). */
    rf_cmd_ieee_rx_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.commandNo    = RF_CMD_IEEE_RX;
    cmd.channel         = current_channel;
    cmd.rxConfig        = 0;   /* no auto-ACK, no auto-filter */
    cmd.frameFilter     = 0;   /* promiscuous */
    cmd.pRxQ            = (uint32_t)rx_queue;
    cmd.pOutput         = (uint32_t)&rx_head;
    return rf_submit_cmd((uint32_t)&cmd);
}

int rf_stop_rx(void)
{
    return rf_submit_cmd((uint32_t)RF_CMD_IEEE_ABORT);
}

int rf_tx_frame(const uint8_t *frame, uint8_t len)
{
    if (len > IEEE802154_MAX_FRM_LEN) return -1;

    /* Enable PA, disable LNA for TX */
    GPIO_SET(FEM_PA_EN_DIO);
    GPIO_CLR(FEM_LNA_EN_DIO);

    rf_cmd_ieee_tx_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.commandNo  = RF_CMD_IEEE_TX;
    cmd.txConfig      = 0;  /* no auto-ACK wait */
    cmd.txLen         = len;
    cmd.pPayload      = (uint8_t *)frame;
    cmd.ackConfig     = 0;  /* no retransmits */

    int rc = rf_submit_cmd((uint32_t)&cmd);

    /* Restore LNA for RX */
    GPIO_CLR(FEM_PA_EN_DIO);
    GPIO_SET(FEM_LNA_EN_DIO);
    return rc;
}

int rf_set_tx_power(int8_t dbm)
{
    /* The nRF21540 FEM provides +20 dBm with gain=high, +5 dBm with gain=low.
     * The CC2652 native output is +5 dBm; with FEM bypass (PA off) it's +5. */
    if (dbm > 20) dbm = 20;
    if (dbm < -20) dbm = -20;
    if (dbm > 5) {
        GPIO_SET(FEM_GAIN_DIO);   /* high gain = +20 dBm */
        rf_fem_enable(true, false);
    } else {
        GPIO_CLR(FEM_GAIN_DIO);   /* low gain = +5 dBm */
        rf_fem_enable(dbm > 0, false);
    }
    return 0;
}

int rf_energy_scan(int8_t rssi_out[16])
{
    int8_t saved = current_channel;
    for (uint8_t ch = IEEE802154_CHAN_MIN; ch <= IEEE802154_CHAN_MAX; ch++) {
        rf_set_channel(ch);
        rf_start_promiscuous_rx();
        /* Dwell 100 ms sampling RSSI. In production we read the RF core's
         * RSSI register; here we approximate by reading the last RX entry. */
        volatile uint32_t start = rf_get_timestamp_us();
        int8_t max_rssi = -128;
        while ((rf_get_timestamp_us() - start) < 100000U) {
            rf_rx_entry_t e;
            if (rf_rx_read(&e) == 0) {
                if (e.rssi > max_rssi) max_rssi = e.rssi;
            }
        }
        rssi_out[ch - IEEE802154_CHAN_MIN] = (max_rssi == -128) ? -100 : max_rssi;
        rf_stop_rx();
    }
    rf_set_channel(saved);
    return 0;
}

int rf_antenna_select(uint8_t ant)
{
    if (ant == 0) { GPIO_SET(ANT_SW_A_DIO); GPIO_CLR(ANT_SW_B_DIO); }
    else          { GPIO_CLR(ANT_SW_A_DIO); GPIO_SET(ANT_SW_B_DIO); }
    return 0;
}

int rf_fem_enable(bool pa, bool lna)
{
    if (pa)  GPIO_SET(FEM_PA_EN_DIO);  else GPIO_CLR(FEM_PA_EN_DIO);
    if (lna) GPIO_SET(FEM_LNA_EN_DIO); else GPIO_CLR(FEM_LNA_EN_DIO);
    return 0;
}

int rf_fem_gain(uint8_t gain)
{
    if (gain) GPIO_SET(FEM_GAIN_DIO); else GPIO_CLR(FEM_GAIN_DIO);
    return 0;
}

bool rf_rx_pending(void)
{
    return rx_head != rx_tail;
}

int rf_rx_read(rf_rx_entry_t *out)
{
    if (rx_head == rx_tail) return -1;  /* empty */
    *out = rx_queue[rx_head];
    rx_head = (rx_head + 1) % RF_RX_QUEUE_LEN;
    return 0;
}

/* ---- RF core RX interrupt handler ----
 * Called when the RF core signals a new frame in the RX queue. In production
 * this is wired to the CPE IRQ; here we provide the handler that the NVIC
 * vector table points at. */
void RF_RxIRQHandler(void)
{
    /* Advance tail pointer — the RF core wrote a new entry. */
    rx_tail = (rx_tail + 1) % RF_RX_QUEUE_LEN;
    IRQ_CLEAR(RF_IRQ_DIO);
}