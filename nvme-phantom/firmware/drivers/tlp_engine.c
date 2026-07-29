/*
 * drivers/tlp_engine.c — FPGA TLP inspector / injector rule interface
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * The Lattice ECP5 FPGA runs the data-plane TLP engine.  The STM32H5 talks
 * to it over SPI1 using a simple framed protocol:
 *
 *   [0xA5][uint8 cmd][uint16 len][payload...][uint8 crc8]
 *
 * Commands:
 *   0x01  RESET         — reset rule engine, clear all rules
 *   0x02  LOAD_RULE     — push a 32-byte rule into BRAM (payload = rule bytes)
 *   0x03  COMMIT        — atomically activate the loaded rule set
 *   0x04  READ_DECODED  — read one decoded NVMe command from the FPGA FIFO
 *                         (payload = max bytes; returns cmd bytes + CRC)
 *   0x05  INJECT_SQ     — inject a 64-byte NVMe SQ entry toward the SSD
 *   0x06  INJECT_CPL    — inject a 16-byte completion toward the host
 *   0x07  SET_MODE      — tell FPGA which mode to operate in (passive/MITM/...)
 *   0x08  GET_STATS     — read capture/inject counters
 *   0x09  DMA_BUILD     — build a synthetic NVMe Read/Write with PRP=target
 *   0x0A  SPOOF_IDENT   — load a spoofed Identify Controller response (4096 B)
 *   0x0B  ARM_MODIFY    — tell FPGA to assert PEX8606 modify-override line
 *   0x0C  DISARM_MODIFY — release the modify-override line
 *
 * The FPGA responds with [0x5A][uint8 status][uint16 len][payload...][crc8].
 * status=0x00 means OK; anything else is an error code.
 */

#include "../board.h"
#include "../registers.h"
#include "pcie_switch.c"   /* for pcie_switch_enable_modify() */

/* ---- SPI1 low-level ---------------------------------------------------- */

static void spi1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_SPI1;
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB | RCC_AHB1ENR_GPIOC;
    /* PB3/PB4/PB5 AF5, PA15 = CS output, PC6 = INT input, PC8 = CDONE in, PC9 = CRESET out */
    volatile uint32_t *gpiob_moder = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER_OFF);
    volatile uint32_t *gpiob_afrl  = (volatile uint32_t *)(GPIOB_BASE + GPIO_AFRL_OFF);
    *gpiob_moder &= ~((3U << (3*2)) | (3U << (4*2)) | (3U << (5*2)));
    *gpiob_moder |=  ((2U << (3*2)) | (2U << (4*2)) | (2U << (5*2)));  /* AF */
    *gpiob_afrl  &= ~((0xFU << (3*4)) | (0xFU << (4*4)) | (0xFU << (5*4)));
    *gpiob_afrl  |=  ((5U  << (3*4)) | (5U  << (4*4)) | (5U  << (5*4)));
    volatile uint32_t *gpioc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER_OFF);
    *gpioc_moder &= ~((3U << (6*2)) | (3U << (8*2)));    /* PC6, PC8 = input */
    *gpioc_moder |=  ((1U << (9*2)));                     /* PC9 = output (CRESET) */
    volatile uint32_t *gpioa_moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER_OFF);
    *gpioa_moder &= ~(3U << (15*2));
    *gpioa_moder |=  (1U << (15*2));                      /* PA15 = output (CS) */

    /* Configure SPI1: master, CPOL=0, CPHA=0, 40 MHz from 125 MHz APB2 -> div4 */
    volatile uint32_t *spi_cr1 = (volatile uint32_t *)(SPI1_BASE + SPI_CR1);
    volatile uint32_t *spi_cr2 = (volatile uint32_t *)(SPI1_BASE + SPI_CR2);
    *spi_cr1 = 0;
    *spi_cr2 = (7U << 8);                 /* 8-bit frame, FIFO threshold */
    *spi_cr1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV4;  /* master, div4 = ~31 MHz */
    *spi_cr1 |= SPI_CR1_SPE;              /* enable */
}

static void spi1_cs_low(void)
{
    volatile uint32_t *odr = (volatile uint32_t *)(GPIOA_BASE + GPIO_ODR_OFF);
    *odr &= ~(1U << FPGA_CS_PIN);
}

static void spi1_cs_high(void)
{
    volatile uint32_t *odr = (volatile uint32_t *)(GPIOA_BASE + GPIO_ODR_OFF);
    *odr |=  (1U << FPGA_CS_PIN);
}

static uint8_t spi1_xfer(uint8_t tx)
{
    volatile uint32_t *dr  = (volatile uint32_t *)(SPI1_BASE + SPI_DR);
    volatile uint32_t *sr  = (volatile uint32_t *)(SPI1_BASE + SPI_SR);
    *dr = tx;
    while (!(*sr & SPI_SR_RXNE)) { }
    return (uint8_t)*dr;
}

/* ---- CRC-8 (poly 0x07, init 0x00) -------------------------------------- */

static uint8_t crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
    }
    return crc;
}

/* ---- Framed transaction ------------------------------------------------ */

static int fpga_xfer(uint8_t cmd, const uint8_t *payload, uint16_t len,
                     uint8_t *resp, uint16_t *resp_len)
{
    uint8_t hdr[4] = { 0xA5, cmd, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8) };
    uint8_t crc = crc8(hdr + 1, 3) ^ crc8(payload, len);
    spi1_cs_low();
    /* send header + payload + crc */
    for (int i = 0; i < 4; i++) spi1_xfer(hdr[i]);
    for (uint16_t i = 0; i < len; i++) spi1_xfer(payload[i]);
    spi1_xfer(crc);
    /* read response: [0x5A][status][len_lo][len_hi][payload...][crc] */
    uint8_t r[4];
    for (int i = 0; i < 4; i++) r[i] = spi1_xfer(0xFF);
    if (r[0] != 0x5A) {
        spi1_cs_high();
        return -1;
    }
    uint16_t rlen = (uint16_t)r[2] | ((uint16_t)r[3] << 8);
    if (rlen > *resp_len) rlen = *resp_len;     /* truncate if buffer too small */
    for (uint16_t i = 0; i < rlen; i++) resp[i] = spi1_xfer(0xFF);
    uint8_t rcrc = spi1_xfer(0xFF);
    spi1_cs_high();
    *resp_len = rlen;
    if (rcrc != crc8(r + 1, 3) ^ crc8(resp, rlen)) return -2;
    return r[1];                                 /* status code */
}

/* ---- Public API -------------------------------------------------------- */

int tlp_engine_init(void)
{
    spi1_init();
    /* Reset the ECP5 and load the bitstream from W25Q128 (handled by
     * storage.c + fpga_spi load logic in main.c).  Here we just reset
     * the rule engine. */
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    int rc = fpga_xfer(0x01, NULL, 0, resp, &rlen);
    if (rc != 0) return -1;
    g_state.fpga_ready = 1;
    return 0;
}

int tlp_engine_set_mode(board_mode_t mode)
{
    uint8_t m = (uint8_t)mode;
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    return fpga_xfer(0x07, &m, 1, resp, &rlen);
}

int tlp_engine_load_rule(const uint8_t rule[32])
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    return fpga_xfer(0x02, rule, 32, resp, &rlen);
}

int tlp_engine_commit_rules(void)
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    return fpga_xfer(0x03, NULL, 0, resp, &rlen);
}

int tlp_engine_clear_rules(void)
{
    return tlp_engine_init();                     /* reset = clear */
}

/* Read one decoded NVMe command from the FPGA FIFO.  Returns 0 on success,
 * -1 if no command available. */
int tlp_engine_read_decoded(nvme_cmd_t *cmd)
{
    uint8_t resp[80];
    uint16_t rlen = sizeof(resp);
    int rc = fpga_xfer(0x04, NULL, 0, resp, &rlen);
    if (rc != 0 || rlen < 64) return -1;
    nvme_decode_sq(resp, cmd);
    g_state.capture_count++;
    return 0;
}

/* Inject a raw 64-byte NVMe submission entry toward the SSD. */
int tlp_engine_inject_sq(const uint8_t sq[64])
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    int rc = fpga_xfer(0x05, sq, 64, resp, &rlen);
    if (rc == 0) g_state.inject_count++;
    return rc;
}

/* Inject a 16-byte completion toward the host. */
int tlp_engine_inject_cpl(const uint8_t cq[16])
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    int rc = fpga_xfer(0x06, cq, 16, resp, &rlen);
    if (rc == 0) g_state.inject_count++;
    return rc;
}

/* Build a synthetic NVMe DMA command: Read or Write with PRP1 = host_phys.
 *   dir = 0: Read FROM host memory (NVMe Write with PRP1=host_phys, SLBA=capbuf)
 *   dir = 1: Write TO host memory (NVMe Read with PRP1=host_phys, SLBA=capbuf)
 * Returns the injected CID. */
int tlp_engine_dma_build(uint64_t host_phys, uint32_t len, uint8_t dir,
                         uint32_t capture_lba, uint16_t *out_cid)
{
    /* 64-byte SQ entry: opcode, flags=0, CID=random, NSID=1,
     * PRP1=host_phys, PRP2=0, CDW10=SLBA_lo, CDW11=SLBA_hi, CDW12=NLB-1 */
    uint8_t sq[64];
    for (int i = 0; i < 64; i++) sq[i] = 0;
    sq[0] = (dir == 0) ? 0x01 : 0x02;            /* Write or Read           */
    uint16_t cid = (uint16_t)(g_state.uptime_s ^ 0x5A5A);
    sq[2] = (uint8_t)(cid & 0xFF);
    sq[3] = (uint8_t)(cid >> 8);
    sq[4] = 1;                                    /* NSID = 1               */
    /* PRP1 = host_phys (bytes 16..23) */
    for (int i = 0; i < 8; i++) sq[16 + i] = (uint8_t)(host_phys >> (i*8));
    /* SLBA = capture_lba (bytes 40..47) */
    for (int i = 0; i < 8; i++) sq[40 + i] = (uint8_t)((uint64_t)capture_lba >> (i*8));
    /* NLB = len/512 - 1 (bytes 44..45, low 16 bits of CDW12) */
    uint16_t nlb = (uint16_t)((len / 512) - 1);
    sq[44] = (uint8_t)(nlb & 0xFF);
    sq[45] = (uint8_t)(nlb >> 8);
    int rc = tlp_engine_inject_sq(sq);
    if (rc == 0 && out_cid) *out_cid = cid;
    return rc;
}

/* Load a spoofed 4096-byte Identify Controller response into the FPGA. */
int tlp_engine_spoof_ident(const uint8_t ident[4096])
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    return fpga_xfer(0x0A, ident, 4096, resp, &rlen);
}

/* Arm / disarm the PEX8606 inline-modify override (enables TLP rewrite). */
int tlp_engine_arm_modify(void)
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    int rc = fpga_xfer(0x0B, NULL, 0, resp, &rlen);
    if (rc == 0) pcie_switch_enable_modify(1);
    return rc;
}

int tlp_engine_disarm_modify(void)
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    int rc = fpga_xfer(0x0C, NULL, 0, resp, &rlen);
    pcie_switch_enable_modify(0);                 /* always disarm switch    */
    return rc;
}

/* Get capture / inject counters from the FPGA. */
int tlp_engine_get_stats(uint32_t *cap, uint32_t *inj)
{
    uint8_t resp[8];
    uint16_t rlen = sizeof(resp);
    int rc = fpga_xfer(0x08, NULL, 0, resp, &rlen);
    if (rc == 0 && rlen >= 8) {
        *cap = (uint32_t)resp[0] | ((uint32_t)resp[1] << 8) |
               ((uint32_t)resp[2] << 16) | ((uint32_t)resp[3] << 24);
        *inj = (uint32_t)resp[4] | ((uint32_t)resp[5] << 8) |
               ((uint32_t)resp[6] << 16) | ((uint32_t)resp[7] << 24);
    }
    return rc;
}