/*
 * ble_spi.c — SPI interface to nRF52840 BLE co-processor
 * STM32F407 SPI3 master → nRF52840 SPI slave
 */

#include "ble_spi.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* SPI3 register offsets */
#define SPI_CR1_SPE    (1 << 6)
#define SPI_CR1_MSTR   (1 << 2)
#define SPI_CR1_BR(x)  ((x) << 3)  /* Baud rate: 0=/2, 1=/4, 2=/8, ... */
#define SPI_CR1_SSM    (1 << 9)
#define SPI_CR1_SSI    (1 << 8)
#define SPI_CR1_CPOL   (1 << 1)
#define SPI_CR1_CPHA   (1 << 0)
#define SPI_CR2_SSOE   (1 << 2)
#define SPI_SR_TXE     (1 << 1)
#define SPI_SR_RXNE    (1 << 0)
#define SPI_SR_BSY     (1 << 7)

static volatile uint8_t ble_seq = 0;
static void (*ble_callback)(const ble_packet_t *) = NULL;

/* ========== SPI Low-Level ========== */
static void spi_select(void) {
    GPIOD->ODR &= ~(1 << SPI3_NSS_PIN);  /* CS low */
    for (volatile int i = 0; i < 50; i++);  /* ~0.5 µs delay */
}

static void spi_deselect(void) {
    GPIOD->ODR |= (1 << SPI3_NSS_PIN);  /* CS high */
    for (volatile int i = 0; i < 50; i++);
}

static uint8_t spi_xfer(uint8_t tx_byte) {
    while (!(SPI3->SR & SPI_SR_TXE));
    SPI3->DR = tx_byte;
    while (!(SPI3->SR & SPI_SR_RXNE));
    return (uint8_t)SPI3->DR;
}

/* ========== Initialize SPI3 ========== */
int ble_spi_init(void) {
    /* Enable SPI3 clock */
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

    /* Configure SPI3: Master, CPOL=0, CPHA=0, 8-bit, MSB first, /8 clock */
    SPI3->CR1 = SPI_CR1_MSTR | SPI_CR1_BR(2) |  /* fPCLK/8 = 5.25 MHz */
                SPI_CR1_SSM | SPI_CR1_SSI;  /* Software NSS management */
    SPI3->CR2 = SPI_CR2_SSOE;

    /* Enable SPI */
    SPI3->CR1 |= SPI_CR1_SPE;

    /* Configure NSS pin as GPIO output (already done in gpio_init_all) */
    return 0;
}

/* ========== Send Command ========== */
int ble_spi_send_command(uint8_t opcode, const uint8_t *data, uint16_t len) {
    spi_select();

    /* Sync byte */
    spi_xfer(0xAA);
    /* Header */
    spi_xfer(opcode);
    spi_xfer(ble_seq++);
    spi_xfer((len >> 8) & 0xFF);
    spi_xfer(len & 0xFF);

    /* Payload */
    uint8_t checksum = opcode ^ (ble_seq - 1) ^ (len >> 8) ^ (len & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        spi_xfer(data[i]);
        checksum ^= data[i];
    }
    /* Checksum */
    spi_xfer(checksum);

    spi_deselect();
    return 0;
}

/* ========== Read Response ========== */
int ble_spi_read_response(ble_packet_t *pkt, uint32_t timeout_ms) {
    /* Check if nRF has data ready (IRQ pin low) */
    if (gpio_read(NRF_IRQ_PORT, NRF_IRQ_PIN) != 0) {
        return -1;  /* No data ready */
    }

    spi_select();

    /* Wait for sync byte */
    uint32_t timeout = timeout_ms * 16800;
    uint8_t sync;
    do {
        sync = spi_xfer(0x00);
        if (sync == 0xAA) break;
    } while (--timeout);

    if (timeout == 0) { spi_deselect(); return -1; }

    /* Read header */
    pkt->opcode = spi_xfer(0x00);
    pkt->seq    = spi_xfer(0x00);
    pkt->length = (spi_xfer(0x00) << 8) | spi_xfer(0x00);

    if (pkt->length > BLE_PKT_MAX) { spi_deselect(); return -2; }

    /* Read payload */
    for (uint16_t i = 0; i < pkt->length; i++) {
        pkt->data[i] = spi_xfer(0x00);
    }

    /* Read checksum */
    uint8_t rx_checksum = spi_xfer(0x00);

    spi_deselect();

    /* Verify checksum */
    uint8_t calc_checksum = pkt->opcode ^ pkt->seq ^
                            (pkt->length >> 8) ^ (pkt->length & 0xFF);
    for (uint16_t i = 0; i < pkt->length; i++) {
        calc_checksum ^= pkt->data[i];
    }

    if (calc_checksum != rx_checksum) return -3;

    /* Invoke callback if registered */
    if (ble_callback) ble_callback(pkt);

    return 0;
}

/* ========== Register Callback ========== */
void ble_spi_register_callback(void (*cb)(const ble_packet_t *)) {
    ble_callback = cb;
}

/* ========== Start Advertising ========== */
int ble_spi_start_advertising(const char *name) {
    uint8_t data[32];
    uint8_t len = (uint8_t)strlen(name);
    if (len > 28) len = 28;  /* BLE advertising name limit */

    data[0] = len + 1;       /* Length field */
    data[1] = 0x09;          /* Complete Local Name type */
    memcpy(&data[2], name, len);

    return ble_spi_send_command(BLE_CMD_SET_ADV_DATA, data, len + 2);
}

/* ========== Connect ========== */
int ble_spi_connect(const uint8_t *addr) {
    return ble_spi_send_command(BLE_CMD_CONNECT_SCAN, addr, 6);
}

/* ========== Send CAN Frame over BLE ========== */
int ble_send_can_frame(const can_frame_t *frame) {
    uint8_t data[13];

    /* Encode CAN frame into BLE packet:
     * Byte 0-3: CAN ID (little-endian)
     * Byte 4: [R7 R6 R5 R4 R3 R2 R1 R0]
     *          R7=IDE (0=std, 1=ext), R6=RTR, R3-R0=DLC
     * Byte 5-12: Data bytes
     */
    data[0] = (frame->id >> 0) & 0xFF;
    data[1] = (frame->id >> 8) & 0xFF;
    data[2] = (frame->id >> 16) & 0xFF;
    data[3] = (frame->id >> 24) & 0xFF;
    data[4] = ((frame->id_type & 1) << 7) |
              ((frame->rtr & 1) << 6) |
              (frame->dlc & 0x0F);

    for (int i = 0; i < 8; i++) {
        data[5 + i] = frame->data[i];
    }

    return ble_spi_send_command(BLE_CMD_SEND_DATA, data, 13);
}