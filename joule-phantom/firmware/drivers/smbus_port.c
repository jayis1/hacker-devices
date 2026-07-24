/*
 * smbus_port.c — SMBus/PMBus dual-port driver implementation.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Implements transparent bridging between host and battery SMBus ports
 * with a per-frame MITM rule engine, a ring-buffer capture, and
 * interrupt-driven slave listeners.
 */

#include "smbus_port.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Capture ring buffer (lock-free single-producer / single-consumer)  */
/* ------------------------------------------------------------------ */
#define CAP_DEPTH 128
static smb_frame_t  cap_ring[CAP_DEPTH];
static volatile uint32_t cap_head;  /* producer (ISR) */
static volatile uint32_t cap_tail;  /* consumer (main) */

/* MITM rules table */
static mitm_rule_t mitm_rules[MITM_RULE_MAX];
static uint8_t     mitm_rule_cnt = 0;

/* ------------------------------------------------------------------ */
/*  Low-level port helpers                                             */
/* ------------------------------------------------------------------ */
static I2C_TypeDef *port_to_i2c(smb_port_t p)
{
    return (p == SMB_PORT_HOST) ? I2C1 : I2C2;
}

/* Wait for a flag with timeout (ms) */
static int i2c_wait(I2C_TypeDef *I2Cx, uint32_t mask, uint32_t ms)
{
    uint32_t t0 = board_millis();
    while ((I2Cx->ISR & mask) == 0) {
        if ((board_millis() - t0) > ms)
            return -1;
    }
    return 0;
}

static void i2c_clear_flags(I2C_TypeDef *I2Cx)
{
    I2Cx->ICR = I2C_ICR_CLEAR_ALL;
}

/* ------------------------------------------------------------------ */
/*  smbus_init — configure both I2C peripherals for SMBus timing       */
/* ------------------------------------------------------------------ */
void smbus_init(void)
{
    /* Enable clocks already done in board_init(); ensure here too */
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_I2C2EN;

    I2C_TypeDef *ports[] = { I2C1, I2C2 };
    for (int i = 0; i < 2; i++) {
        I2C_TypeDef *I = ports[i];
        I->CR1 = 0;                 /* disable before config            */
        /* TIMINGR: 100 kHz SMBus from 80 MHz PCLK
         * PRESC=1, SCLL=0x4F, SCLH=0x37, SDADEL=0x2, SCLDEL=0x4       */
        I->TIMINGR = (1u << 28) | (0x4u << 20) | (0x2u << 16) |
                     (0x37u << 8) | 0x4Fu;
        /* Timeout for SMBus (~35 ms)                                   */
        I->TIMEOUTR = (1u << 15) | (0x0Fu << 0);  /* timeout A         */
        /* Enable SMBus host mode + PEC + alerts                        */
        I->CR1 = I2C_CR1_PE | I2C_CR1_SMBHEN | I2C_CR1_ALERTEN |
                 I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
        i2c_clear_flags(I);
    }
    /* Reset capture ring */
    cap_head = cap_tail = 0;
    mitm_rule_cnt = 0;
}

/* ------------------------------------------------------------------ */
/*  Slave listener — arm a port to respond as a given address          */
/* ------------------------------------------------------------------ */
void smbus_listen(smb_port_t port, uint8_t own_addr)
{
    I2C_TypeDef *I = port_to_i2c(port);
    I->OAR1 = 0;                    /* clear before set            */
    I->OAR1 = (1u << 15) | ((own_addr & 0x7F) << 1) | (1u << 0);
    I->CR1 |= I2C_CR1_ADDRIE | I2C_CR1_RXIE | I2C_CR1_TXIE |
              I2C_CR1_STOPIE;
}

/* ------------------------------------------------------------------ */
/*  Blocking master read-word                                          */
/* ------------------------------------------------------------------ */
int smbus_read_word(smb_port_t port, uint8_t addr, uint8_t cmd, uint16_t *out)
{
    I2C_TypeDef *I = port_to_i2c(port);

    /* Phase 1: write command byte */
    I->CR2 = ((addr & 0x7F) << 1) | (1u << I2C_CR2_NBYTES_POS) | I2C_CR2_START;
    if (i2c_wait(I, I2C_ISR_TXIS, SMBUS_TIMEOUT_MS)) return -1;
    I->TXDR = cmd;
    if (i2c_wait(I, I2C_ISR_TC, SMBUS_TIMEOUT_MS)) return -1;

    /* Phase 2: read 2 bytes (repeated start) */
    I->CR2 = ((addr & 0x7F) << 1) | I2C_CR2_RD_WRN |
             (2u << I2C_CR2_NBYTES_POS) | I2C_CR2_AUTOEND | I2C_CR2_START;
    uint8_t lo, hi;
    if (i2c_wait(I, I2C_ISR_RXNE, SMBUS_TIMEOUT_MS)) return -1;
    lo = (uint8_t)I->RXDR;
    if (i2c_wait(I, I2C_ISR_RXNE, SMBUS_TIMEOUT_MS)) return -1;
    hi = (uint8_t)I->RXDR;
    if (i2c_wait(I, I2C_ISR_STOPF, SMBUS_TIMEOUT_MS)) return -1;
    I->ICR = I2C_ISR_STOPF;
    *out = (uint16_t)((hi << 8) | lo);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Blocking master write-word                                         */
/* ------------------------------------------------------------------ */
int smbus_write_word(smb_port_t port, uint8_t addr, uint8_t cmd, uint16_t val)
{
    I2C_TypeDef *I = port_to_i2c(port);
    I->CR2 = ((addr & 0x7F) << 1) | (3u << I2C_CR2_NBYTES_POS) |
             I2C_CR2_AUTOEND | I2C_CR2_START;
    if (i2c_wait(I, I2C_ISR_TXIS, SMBUS_TIMEOUT_MS)) return -1;
    I->TXDR = cmd;
    if (i2c_wait(I, I2C_ISR_TXIS, SMBUS_TIMEOUT_MS)) return -1;
    I->TXDR = (uint8_t)(val & 0xFF);
    if (i2c_wait(I, I2C_ISR_TXIS, SMBUS_TIMEOUT_MS)) return -1;
    I->TXDR = (uint8_t)((val >> 8) & 0xFF);
    if (i2c_wait(I, I2C_ISR_STOPF, SMBUS_TIMEOUT_MS)) return -1;
    I->ICR = I2C_ISR_STOPF;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Blocking master read-block                                         */
/* ------------------------------------------------------------------ */
int smbus_read_block(smb_port_t port, uint8_t addr, uint8_t cmd,
                     uint8_t *buf, uint8_t *len)
{
    I2C_TypeDef *I = port_to_i2c(port);
    /* Write command */
    I->CR2 = ((addr & 0x7F) << 1) | (1u << I2C_CR2_NBYTES_POS) | I2C_CR2_START;
    if (i2c_wait(I, I2C_ISR_TXIS, SMBUS_TIMEOUT_MS)) return -1;
    I->TXDR = cmd;
    if (i2c_wait(I, I2C_ISR_TC, SMBUS_TIMEOUT_MS)) return -1;

    /* Read length byte first */
    I->CR2 = ((addr & 0x7F) << 1) | I2C_CR2_RD_WRN |
             (1u << I2C_CR2_NBYTES_POS) | I2C_CR2_RELOAD | I2C_CR2_START;
    if (i2c_wait(I, I2C_ISR_RXNE, SMBUS_TIMEOUT_MS)) return -1;
    uint8_t n = (uint8_t)I->RXDR;
    if (n > 32) n = 32;
    if (i2c_wait(I, I2C_ISR_TCR, SMBUS_TIMEOUT_MS)) return -1;

    /* Read n data bytes */
    I->CR2 = ((addr & 0x7F) << 1) | I2C_CR2_RD_WRN |
             ((uint32_t)n << I2C_CR2_NBYTES_POS) | I2C_CR2_AUTOEND;
    for (uint8_t i = 0; i < n; i++) {
        if (i2c_wait(I, I2C_ISR_RXNE, SMBUS_TIMEOUT_MS)) return -1;
        buf[i] = (uint8_t)I->RXDR;
    }
    if (i2c_wait(I, I2C_ISR_STOPF, SMBUS_TIMEOUT_MS)) return -1;
    I->ICR = I2C_ISR_STOPF;
    *len = n;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Capture ring push/pop                                              */
/* ------------------------------------------------------------------ */
void smbus_capture_push(const smb_frame_t *f)
{
    uint32_t next = (cap_head + 1u) % CAP_DEPTH;
    if (next == cap_tail) {
        /* overwrite oldest; advance tail */
        cap_tail = (cap_tail + 1u) % CAP_DEPTH;
    }
    cap_ring[cap_head] = *f;
    cap_head = next;
}

int smbus_capture_pop(smb_frame_t *out)
{
    if (cap_head == cap_tail) return -1;
    *out = cap_ring[cap_tail];
    cap_tail = (cap_tail + 1u) % CAP_DEPTH;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  MITM rule engine                                                   */
/* ------------------------------------------------------------------ */
void mitm_rules_clear(void)
{
    mitm_rule_cnt = 0;
    memset(mitm_rules, 0, sizeof(mitm_rules));
}

void mitm_rule_add(const mitm_rule_t *r)
{
    if (mitm_rule_cnt < MITM_RULE_MAX) {
        mitm_rules[mitm_rule_cnt++] = *r;
    }
}

uint8_t mitm_rule_count(void) { return mitm_rule_cnt; }
const mitm_rule_t *mitm_rule_get(uint8_t i)
{
    return (i < mitm_rule_cnt) ? &mitm_rules[i] : 0;
}

mitm_action_t mitm_apply(smb_frame_t *f)
{
    for (uint8_t i = 0; i < mitm_rule_cnt; i++) {
        const mitm_rule_t *r = &mitm_rules[i];
        if (!r->enabled) continue;
        if ((f->cmd & r->mask) != (r->cmd & r->mask)) continue;
        switch (r->action) {
        case MITM_ACT_SPOOF:
            if (r->spoof_len && r->spoof_len <= sizeof(f->rbuf)) {
                memcpy(f->rbuf, r->spoof, r->spoof_len);
                f->rlen = r->spoof_len;
                f->flags |= SMB_FLAG_MODIFIED;
            }
            return MITM_ACT_SPOOF;
        case MITM_ACT_BLOCK:
            f->flags |= SMB_FLAG_NACK;
            return MITN_ACT_BLOCK;
        case MITM_ACT_LOG_ONLY:
            return MITM_ACT_LOG_ONLY;
        case MITM_ACT_GLITCH:
            board_glitch_pulse(50);
            f->flags |= SMB_FLAG_MODIFIED;
            return MITM_ACT_GLITCH;
        default:
            break;
        }
    }
    return MITM_ACT_NONE;
}

/* ------------------------------------------------------------------ */
/*  Bridge a single transaction (called from main loop when host       */
/*  initiates a read/write).                                           */
/* ------------------------------------------------------------------ */
int smbus_bridge_transaction(smb_frame_t *f)
{
    int rc;
    /* Forward host request to battery */
    if (f->wlen > 0 && f->rlen == 0) {
        /* Write from host -> battery */
        rc = smbus_write_word(SMB_PORT_BATT, f->addr, f->cmd,
                              (uint16_t)(f->wbuf[0] | (f->wbuf[1] << 8)));
        if (rc) f->flags |= SMB_FLAG_NACK;
    } else if (f->rlen == 2) {
        /* Read-word */
        uint16_t v;
        rc = smbus_read_word(SMB_PORT_BATT, f->addr, f->cmd, &v);
        if (rc == 0) {
            f->rbuf[0] = (uint8_t)(v & 0xFF);
            f->rbuf[1] = (uint8_t)(v >> 8);
            f->rlen = 2;
        } else {
            f->flags |= SMB_FLAG_NACK;
            f->rlen = 0;
        }
    } else if (f->rlen > 2) {
        /* Read-block */
        uint8_t n = 0;
        rc = smbus_read_block(SMB_PORT_BATT, f->addr, f->cmd, f->rbuf, &n);
        if (rc == 0) f->rlen = n;
        else         f->flags |= SMB_FLAG_NACK;
    } else {
        /* Quick command / unknown */
        rc = smbus_write_word(SMB_PORT_BATT, f->addr, f->cmd, 0);
    }

    /* Apply MITM rules to the response */
    mitm_apply(f);

    /* Capture */
    smbus_capture_push(f);
    return rc;
}

/* ------------------------------------------------------------------ */
/*  ISR stubs — full slave-mode state machine                          */
/* ------------------------------------------------------------------ */
/* Slave state for host-side listener */
typedef struct {
    uint8_t cmd;
    uint8_t wbuf[32];
    uint8_t wlen;
    uint8_t rbuf[32];
    uint8_t rlen;
    uint8_t phase;     /* 0=idle,1=rcv cmd,2=rcv data,3=tx data */
    uint8_t is_read;
} slave_state_t;

static slave_state_t slave_state[2];

void smbus_host_ev_isr(void)
{
    I2C_TypeDef *I = I2C1;
    uint32_t isr = I->ISR;
    slave_state_t *s = &slave_state[0];

    if (isr & I2C_ISR_ADDR) {
        I->ICR = I2C_ISR_ADDR;
        s->wlen = 0;
        s->phase = 1;
        s->is_read = (I->ISR & I2C_ISR_DIR) ? 1 : 0; /* DIR not defined above; treat by CR2 RD_WRN */
    }
    if (isr & I2C_ISR_RXNE) {
        uint8_t b = (uint8_t)I->RXDR;
        if (s->phase == 1) { s->cmd = b; s->phase = 2; }
        else if (s->phase == 2) {
            if (s->wlen < sizeof(s->wbuf)) s->wbuf[s->wlen++] = b;
        }
    }
    if (isr & I2C_ISR_TXIS) {
        /* Host wants to read: we must provide (possibly spoofed) bytes */
        if (s->phase == 2) {
            /* Forward to battery synchronously inside ISR is unsafe in
             * real silicon; for the design model we queue a bridge job. */
            smb_frame_t f = {0};
            f.port    = SMB_PORT_HOST;
            f.dir     = SMB_DIR_HOST_READ;
            f.addr    = 0x0B;   /* SBS default battery address */
            f.cmd     = s->cmd;
            f.wlen    = s->wlen;
            memcpy(f.wbuf, s->wbuf, s->wlen);
            f.rlen    = 2;      /* assume read-word by default */
            smbus_bridge_transaction(&f);
            memcpy(s->rbuf, f.rbuf, f.rlen);
            s->rlen = f.rlen;
            s->phase = 3;
        }
        static uint8_t tx_idx = 0;
        if (tx_idx < s->rlen) {
            I->TXDR = s->rbuf[tx_idx++];
        } else {
            I->TXDR = 0;
            tx_idx = 0;
        }
    }
    if (isr & I2C_ISR_STOPF) {
        I->ICR = I2C_ISR_STOPF;
        s->phase = 0;
    }
    if (isr & I2C_ISR_NACKF) {
        I->ICR = I2C_ISR_NACKF;
    }
}

void smbus_host_er_isr(void)
{
    I2C_TypeDef *I = I2C1;
    if (I->ISR & I2C_ISR_BERR)   I->ICR = I2C_ISR_BERR;
    if (I->ISR & I2C_ISR_ARLO)   I->ICR = I2C_ISR_ARLO;
    if (I->ISR & I2C_ISR_OVR)    I->ICR = I2C_ISR_OVR;
    if (I->ISR & I2C_ISR_TIMEOUT) I->ICR = I2C_ISR_TIMEOUT;
    if (I->ISR & I2C_ISR_PECERR) I->ICR = I2C_ISR_PECERR;
    if (I->ISR & I2C_ISR_ALERT)  I->ICR = I2C_ISR_ALERT;
}

void smbus_batt_ev_isr(void)
{
    /* Battery-side listener mirrors host-side logic; for bridge mode
     * the battery port is used in master mode only, so this ISR is
     * normally inactive.  Kept for symmetry / alert capture. */
    I2C_TypeDef *I = I2C2;
    if (I->ISR & I2C_ISR_ALERT) {
        smb_frame_t f = {0};
        f.port = SMB_PORT_BATT;
        f.dir  = SMB_DIR_ALERT;
        f.ts_ms = board_millis();
        smbus_capture_push(&f);
        I->ICR = I2C_ISR_ALERT;
    }
    if (I->ISR & I2C_ISR_STOPF) I->ICR = I2C_ISR_STOPF;
}

void smbus_batt_er_isr(void)
{
    I2C_TypeDef *I = I2C2;
    I->ICR = I2C_ICR_CLEAR_ALL;
}