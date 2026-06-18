/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * SWD Engine — ARM Serial Wire Debug bit-bang implementation
 *
 * Implements the ARM Debug Interface v5/v6 SWD protocol via GPIO
 * bit-banging with deterministic timing. Supports DP/AP access,
 * memory reads/writes, core control (halt/resume/step), and
 * firmware extraction from on-chip flash.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "swd_driver.h"

/* =========================================================================
 * Internal state
 * ========================================================================= */

static GPIO_TypeDef *swd_swdio_port;
static uint8_t        swd_swdio_pin;
static GPIO_TypeDef *swd_swclk_port;
static uint8_t        swd_swclk_pin;

static uint32_t       swd_clock_hz = 1000000UL; /* default 1 MHz */
static uint32_t       swd_clk_half_period_us = 0; /* computed from clock */
static uint8_t        swd_target_ap = 0; /* active AP select */
static uint32_t       swd_last_error = 0;

/* Delay loops tuned for 250 MHz core: ~1 cycle/4ns */
static inline void swd_delay_us(uint32_t us)
{
    /* Approximate: 250 MHz → 250 cycles per us. Each loop iteration ~3 cycles. */
    volatile uint32_t count = us * (SYSTEM_CLOCK_HZ / 3000000UL);
    while (count--) { __asm volatile ("nop"); }
}

static inline void swd_clk_pulse(void)
{
    gpio_set(swd_swclk_port, swd_swclk_pin);
    swd_delay_us(swd_clk_half_period_us);
    gpio_clear(swd_swclk_port, swd_swclk_pin);
    swd_delay_us(swd_clk_half_period_us);
}

static inline void swd_write_bit(uint8_t bit)
{
    if (bit) gpio_set(swd_swdio_port, swd_swdio_pin);
    else     gpio_clear(swd_swdio_port, swd_swdio_pin);
    swd_clk_pulse();
}

static inline uint8_t swd_read_bit(void)
{
    /* Switch SWDIO to input with pull-up before clock edge */
    gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_INPUT);
    gpio_set_pupd(swd_swdio_port, swd_swdio_pin, GPIO_PUPD_PULLUP);
    uint8_t bit = gpio_read(swd_swdio_port, swd_swdio_pin);
    swd_clk_pulse();
    /* Restore to output for next write phase */
    gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_OUTPUT);
    return bit;
}

/* =========================================================================
 * SWD transaction: 8-bit packet request + 1 turnaround + 3 response +
 * 1 turnaround (read) or 32 data + 1 parity + 1 turnaround (write)
 * ========================================================================= */

typedef struct {
    uint8_t  start;
    uint8_t  ap;     /* 0=DP, 1=AP */
    uint8_t  rw;     /* 0=read, 1=write */
    uint8_t  addr;  /* 4-bit, register address (uses A2:A3 for select) */
} swd_packet_req_t;

static uint8_t swd_parity_u32(uint32_t v)
{
    uint8_t p = 0;
    for (int i = 0; i < 32; i++) { p ^= (uint8_t)((v >> i) & 1); }
    return p;
}

static uint8_t swd_parity_u3(uint8_t v)
{
    /* 3-bit parity for request header (APnDP, RnW, A2) */
    uint8_t p = 0;
    p ^= (v >> 0) & 1;
    p ^= (v >> 1) & 1;
    p ^= (v >> 2) & 1;
    return p;
}

static gb_status_t swd_transfer(const swd_packet_req_t *req,
                                  uint32_t *data_in, uint32_t data_out)
{
    uint8_t response;
    uint32_t data = 0;
    uint8_t parity;

    /* Build 8-bit request header:
     * [start=1][start=0][AP][RW][addr2][addr3][parity][stop][park]
     */
    uint8_t header = 0x81; /* b1000_0001: start=1, second bit=0, park=1 */
    header |= (req->ap  ? (1U << 2) : 0);
    header |= (req->rw  ? (1U << 3) : 0);
    header |= ((req->addr & 0x4) ? (1U << 4) : 0);
    header |= ((req->addr & 0x8) ? (1U << 5) : 0);
    uint8_t parity_bits = (req->ap ? 1 : 0) ^ (req->rw ? 1 : 0)
                          ^ ((req->addr >> 2) & 1) ^ ((req->addr >> 3) & 1);
    header |= (parity_bits ? (1U << 6) : 0);

    /* Send 8 header bits (LSB first) */
    gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 8; i++) {
        swd_write_bit((header >> i) & 1);
    }

    /* Turnaround: 1 idle clock with SWDIO as input */
    gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_INPUT);
    gpio_set_pupd(swd_swdio_port, swd_swdio_pin, GPIO_PUPD_PULLUP);
    swd_clk_pulse();

    /* Read 3-bit response */
    response = 0;
    for (int i = 0; i < 3; i++) {
        response |= (swd_read_bit() << i);
    }

    /* Response codes:
     * 001 = OK, 010 = WAIT, 100 = FAULT, others = protocol error
     */
    if (response != 0x01 && response != 0x02) {
        swd_last_error = response;
        /* Turnaround and exit */
        gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_OUTPUT);
        swd_clk_pulse();
        return (response == 0x04) ? GB_ERR_LOCKED : GB_ERR_PROTOCOL;
    }
    if (response == 0x02) {
        /* WAIT — caller should retry */
        return GB_ERR_TIMEOUT;
    }

    if (req->rw == SWD_READ) {
        /* Read 32 data bits + 1 parity */
        data = 0;
        for (int i = 0; i < 32; i++) {
            data |= ((uint32_t)swd_read_bit() << i);
        }
        parity = swd_read_bit();
        if (parity != swd_parity_u32(data)) {
            return GB_ERR_PARITY;
        }
        /* Turnaround to output */
        gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_OUTPUT);
        swd_clk_pulse();
        if (data_in) *data_in = data;
    } else {
        /* Turnaround to output */
        gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_OUTPUT);
        swd_clk_pulse();
        /* Write 32 data bits + 1 parity */
        for (int i = 0; i < 32; i++) {
            swd_write_bit((data_out >> i) & 1);
        }
        swd_write_bit(swd_parity_u32(data_out));
    }

    return GB_OK;
}

/* =========================================================================
 * Public API implementation
 * ========================================================================= */

void swd_init(probe_channel_t swdio_ch, probe_channel_t swclk_ch)
{
    swd_swdio_port = probe_pinmap[swdio_ch].port;
    swd_swdio_pin  = probe_pinmap[swdio_ch].pin;
    swd_swclk_port = probe_pinmap[swclk_ch].port;
    swd_swclk_pin  = probe_pinmap[swclk_ch].pin;

    /* Configure SWDIO as output, push-pull, high speed */
    gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_OUTPUT);
    gpio_set_otyper(swd_swdio_port, swd_swdio_pin, GPIO_OTYPE_PP);
    gpio_set_ospeed(swd_swdio_port, swd_swdio_pin, GPIO_OSPEED_HIGH);
    gpio_set_pupd(swd_swdio_port, swd_swdio_pin, GPIO_PUPD_NONE);

    /* Configure SWCLK as output, push-pull, high speed */
    gpio_set_mode(swd_swclk_port, swd_swclk_pin, GPIO_MODE_OUTPUT);
    gpio_set_otyper(swd_swclk_port, swd_swclk_pin, GPIO_OTYPE_PP);
    gpio_set_ospeed(swd_swclk_port, swd_swclk_pin, GPIO_OSPEED_HIGH);
    gpio_set_pupd(swd_swclk_port, swd_swclk_pin, GPIO_PUPD_NONE);
    gpio_clear(swd_swclk_port, swd_swclk_pin);

    swd_set_clock(1000000UL);
    swd_last_error = 0;
}

void swd_set_clock(uint32_t hz)
{
    if (hz == 0) return;
    swd_clock_hz = hz;
    swd_clk_half_period_us = 500000UL / hz;
    if (swd_clk_half_period_us == 0) swd_clk_half_period_us = 1;
}

uint32_t swd_get_clock(void)
{
    return swd_clock_hz;
}

gb_status_t swd_line_reset(void)
{
    gpio_set_mode(swd_swdio_port, swd_swdio_pin, GPIO_MODE_OUTPUT);
    /* Drive SWDIO high for >= 50 clock cycles */
    gpio_set(swd_swdio_port, swd_swdio_pin);
    for (int i = 0; i < 64; i++) {
        swd_clk_pulse();
    }
    /* Idle: drive SWDIO low for a few clocks */
    gpio_clear(swd_swdio_port, swd_swdio_pin);
    for (int i = 0; i < 4; i++) {
        swd_clk_pulse();
    }
    return GB_OK;
}

gb_status_t swd_read_dp(uint8_t addr, uint32_t *val)
{
    swd_packet_req_t req = { .start = 1, .ap = 0, .rw = SWD_READ, .addr = addr };
    return swd_transfer(&req, val, 0);
}

gb_status_t swd_write_dp(uint8_t addr, uint32_t val)
{
    swd_packet_req_t req = { .start = 1, .ap = 0, .rw = SWD_WRITE, .addr = addr };
    return swd_transfer(&req, NULL, val);
}

gb_status_t swd_read_ap(uint8_t apsel, uint8_t addr, uint32_t *val)
{
    gb_status_t st;
    uint32_t select = ((uint32_t)apsel << 24) | (addr & 0xF0);
    st = swd_write_dp(SWD_DP_SELECT, select);
    if (st != GB_OK) return st;
    /* Read AP requires a dummy read then RDBUFF */
    swd_packet_req_t req = { .start = 1, .ap = 1, .rw = SWD_READ, .addr = addr };
    st = swd_transfer(&req, NULL, 0); /* discarded */
    if (st != GB_OK) return st;
    return swd_read_dp(SWD_DP_RDBUFF, val);
}

gb_status_t swd_write_ap(uint8_t apsel, uint8_t addr, uint32_t val)
{
    gb_status_t st;
    uint32_t select = ((uint32_t)apsel << 24) | (addr & 0xF0);
    st = swd_write_dp(SWD_DP_SELECT, select);
    if (st != GB_OK) return st;
    swd_packet_req_t req = { .start = 1, .ap = 1, .rw = SWD_WRITE, .addr = addr };
    return swd_transfer(&req, NULL, val);
}

gb_status_t swd_connect(swd_target_info_t *info)
{
    gb_status_t st;
    uint32_t val;

    if (!info) return GB_ERR_PARAM;
    memset(info, 0, sizeof(*info));

    st = swd_line_reset();
    if (st != GB_OK) return st;

    /* Read DP IDCODE */
    st = swd_read_dp(SWD_DP_IDCODE, &val);
    if (st != GB_OK) return st;
    info->idcode = val;

    /* Clear sticky errors */
    swd_write_dp(SWD_DP_ABORT, SWD_ORUNERRCLR | SWD_STICKYERRCLR | 0x00000016);

    /* Power up debug and system domains */
    st = swd_write_dp(SWD_DP_CTRLSTAT,
                       SWD_CDBGPWRUPREQ | SWD_CSYSPWRUPREQ);
    if (st != GB_OK) return st;

    /* Wait for power-up ack (poll a few times) */
    for (int i = 0; i < 100; i++) {
        st = swd_read_dp(SWD_DP_CTRLSTAT, &val);
        if (st == GB_OK &&
            (val & (SWD_CDBGPWRUPACK | SWD_CSYSPWRUPACK)) ==
                (SWD_CDBGPWRUPACK | SWD_CSYSPWRUPACK)) {
            break;
        }
    }

    /* Enumerate Access Ports 0..7 */
    info->ap_count = 0;
    for (uint8_t apsel = 0; apsel < 8; apsel++) {
        uint32_t idr = 0;
        st = swd_read_ap(apsel, SWD_AP_IDR, &idr);
        if (st == GB_OK && idr != 0) {
            info->ap_idr[info->ap_count] = idr;
            /* Heuristic type classification from IDR */
            uint8_t type = (uint8_t)((idr >> 13) & 0xF);
            info->ap_type[info->ap_count] = type;
            info->ap_count++;
        }
    }

    /* Use first AHB-AP for memory access; find one with type==1 */
    swd_target_ap = 0;
    for (uint8_t i = 0; i < info->ap_count; i++) {
        if (info->ap_type[i] == 1) { /* AHB-AP */
            swd_target_ap = i;
            break;
        }
    }

    /* Set CSW for 32-bit auto-increment memory access */
    swd_write_ap(swd_target_ap, SWD_AP_CSW,
                  SWD_CSW_SIZE_32 | SWD_CSW_ADDR_INC | SWD_CSW_DBGSWEN);

    /* Read RDP level (heuristic: check option bytes via flash controller.
     * For generic ARM, this is vendor-specific; default to 0 (unlocked). */
    info->rdp_level = 0;
    info->exploit_available = 0;
    info->sram_base = 0x20000000UL;
    info->flash_base = 0x08000000UL;

    return GB_OK;
}

gb_status_t swd_mem_read32(uint32_t addr, uint32_t *buf, uint32_t count)
{
    gb_status_t st;
    swd_write_ap(swd_target_ap, SWD_AP_TAR, addr);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t val = 0;
        st = swd_read_ap(swd_target_ap, SWD_AP_DRW, &val);
        if (st != GB_OK) return st;
        if (buf) buf[i] = val;
    }
    return GB_OK;
}

gb_status_t swd_mem_write32(uint32_t addr, const uint32_t *buf, uint32_t count)
{
    gb_status_t st;
    swd_write_ap(swd_target_ap, SWD_AP_TAR, addr);
    for (uint32_t i = 0; i < count; i++) {
        st = swd_write_ap(swd_target_ap, SWD_AP_DRW, buf[i]);
        if (st != GB_OK) return st;
    }
    return GB_OK;
}

gb_status_t swd_mem_read8(uint32_t addr, uint8_t *buf, uint32_t count)
{
    /* Read in 32-bit aligned blocks, handle unaligned head/tail */
    uint32_t aligned = (addr + 3) & ~3UL;
    uint32_t tail_off = addr & 3;
    uint8_t  tmp[4];

    if (tail_off) {
        gb_status_t st = swd_mem_read32(addr & ~3UL, (uint32_t *)tmp, 1);
        if (st != GB_OK) return st;
        uint32_t copy = MIN(count, 4 - tail_off);
        for (uint32_t i = 0; i < copy; i++) buf[i] = tmp[tail_off + i];
        buf += copy;
        count -= copy;
        addr += copy;
    }
    while (count >= 4) {
        uint32_t w;
        gb_status_t st = swd_mem_read32(addr, &w, 1);
        if (st != GB_OK) return st;
        buf[0] = (uint8_t)(w & 0xFF);
        buf[1] = (uint8_t)((w >> 8) & 0xFF);
        buf[2] = (uint8_t)((w >> 16) & 0xFF);
        buf[3] = (uint8_t)((w >> 24) & 0xFF);
        buf += 4;
        addr += 4;
        count -= 4;
    }
    if (count) {
        gb_status_t st = swd_mem_read32(addr, (uint32_t *)tmp, 1);
        if (st != GB_OK) return st;
        for (uint32_t i = 0; i < count; i++) buf[i] = tmp[i];
    }
    return GB_OK;
}

gb_status_t swd_flash_extract(uint32_t addr, uint8_t *buf, uint32_t len,
                                void (*progress)(uint32_t done, uint32_t total))
{
    if (len > FLASH_MAX_DUMP_BYTES) return GB_ERR_PARAM;
    uint32_t blocks = (len + FLASH_BLOCK_SIZE - 1) / FLASH_BLOCK_SIZE;
    uint32_t done = 0;
    for (uint32_t b = 0; b < blocks; b++) {
        uint32_t off = b * FLASH_BLOCK_SIZE;
        uint32_t chunk = MIN(FLASH_BLOCK_SIZE, len - off);
        gb_status_t st = swd_mem_read8(addr + off, buf + off, chunk);
        if (st != GB_OK) return st;
        done += chunk;
        if (progress) progress(done, len);
    }
    return GB_OK;
}

gb_status_t swd_halt(void)
{
    /* Write DHCSR with DBGKEY | C_HALT */
    uint32_t cmd = SWD_DHCSR_DBGKEY | SWD_DHCSR_C_HALT;
    return swd_mem_write32(SWD_DHCSR, &cmd, 1);
}

gb_status_t swd_resume(void)
{
    uint32_t cmd = SWD_DHCSR_DBGKEY | SWD_DHCSR_C_RESUME;
    return swd_mem_write32(SWD_DHCSR, &cmd, 1);
}

gb_status_t swd_step(void)
{
    uint32_t cmd = SWD_DHCSR_DBGKEY | SWD_DHCSR_C_STEP;
    return swd_mem_write32(SWD_DHCSR, &cmd, 1);
}

gb_status_t swd_read_core_reg(uint8_t reg_num, uint32_t *val)
{
    gb_status_t st;
    /* Write register select to DCRSR (reg_num, write=0) */
    uint32_t sel = (uint32_t)reg_num & 0x1F;
    st = swd_mem_write32(SWD_DCRSR, &sel, 1);
    if (st != GB_OK) return st;
    /* Poll DHCSR S_REGRDY */
    for (int i = 0; i < 100; i++) {
        uint32_t dhcsr = 0;
        st = swd_mem_read32(SWD_DHCSR, &dhcsr, 1);
        if (st == GB_OK && (dhcsr & SWD_DHCSR_S_REGRDY)) break;
    }
    return swd_mem_read32(SWD_DCRDR, val, 1);
}

gb_status_t swd_write_core_reg(uint8_t reg_num, uint32_t val)
{
    gb_status_t st;
    st = swd_mem_write32(SWD_DCRDR, &val, 1);
    if (st != GB_OK) return st;
    uint32_t sel = ((uint32_t)reg_num & 0x1F) | (1U << 16); /* write bit */
    st = swd_mem_write32(SWD_DCRSR, &sel, 1);
    if (st != GB_OK) return st;
    for (int i = 0; i < 100; i++) {
        uint32_t dhcsr = 0;
        st = swd_mem_read32(SWD_DHCSR, &dhcsr, 1);
        if (st == GB_OK && (dhcsr & SWD_DHCSR_S_REGRDY)) break;
    }
    return GB_OK;
}

gb_status_t swd_get_rdp_level(uint8_t *level)
{
    /* Generic implementation: many STM32 expose FLASH_OPTR at 0x40022020.
     * For unknown parts, default to 0 (unlocked). Vendor-specific
     * exploit scripts override this. */
    if (!level) return GB_ERR_PARAM;
    *level = 0;
    return GB_OK;
}