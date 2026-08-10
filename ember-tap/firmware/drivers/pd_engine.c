/*
 * pd_engine.c — USB-PD BMC engine on FUSB302B, state machine, and fuzzer
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This driver talks to the FUSB302B over I2C1. It handles:
 *   - CC line attach detection and orientation
 *   - PD frame TX (SOP, SOP', SOP'') with hardware GoodCRC
 *   - PD frame RX with FIFO drain + header decode
 *   - A ring buffer of captured frames
 *   - A stateful PD-message fuzzer
 *
 * The FUSB302B auto-generates GoodCRC for received SOP messages when
 * AUTO_CRC is enabled, so we only handle application-layer messages.
 */

#include "pd_engine.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- I2C helpers (simplified — real driver in power_monitor.c provides
 * the low-level i2c1_write / i2c1_read used here). We declare them extern
 * to avoid circular include. ---- */
extern int i2c1_write(uint8_t addr, const uint8_t *data, uint8_t len);
extern int i2c1_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

/* FUSB302 register access wrappers */
static int fusb_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return i2c1_write(FUSB302_I2C_ADDR, buf, 2);
}

static int fusb_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    return i2c1_read(FUSB302_I2C_ADDR, reg, buf, len);
}

static int fusb_read_reg(uint8_t reg) {
    uint8_t v;
    if (fusb_read(reg, &v, 1) != 0) return -1;
    return v;
}

/* ---- PRNG (xorshift32) ---- */
static uint32_t prng_state = 0xDEADBEEF;

void pd_srand(uint32_t s) { prng_state = s ? s : 0xDEADBEEF; }

uint32_t pd_rand(void) {
    uint32_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_state = x;
    return x;
}

/* ---- Frame ring buffer ---- */
static pd_frame_t pd_ring[PD_RING_SIZE / sizeof(pd_frame_t)];
static volatile uint16_t pd_ring_head = 0;
static volatile uint16_t pd_ring_tail = 0;

static int pd_ring_push(const pd_frame_t *f) {
    uint16_t next = (pd_ring_head + 1) % (PD_RING_SIZE / sizeof(pd_frame_t));
    if (next == pd_ring_tail) return -1; /* overflow */
    pd_ring[pd_ring_head] = *f;
    pd_ring_head = next;
    return 0;
}

int pd_get_captured(pd_frame_t *out) {
    if (pd_ring_head == pd_ring_tail) return 0;
    *out = pd_ring[pd_ring_tail];
    pd_ring_tail = (pd_ring_tail + 1) % (PD_RING_SIZE / sizeof(pd_frame_t));
    return 1;
}

/* ---- Message ID counter (per spec, increments per message) ---- */
static uint8_t msg_id_counter = 0;

/* ---- Header encode/decode ---- */
uint16_t pd_build_header(uint8_t msg_type, uint8_t spec_rev,
                         uint8_t port_power_role, uint8_t port_data_role,
                         uint8_t msg_id, uint8_t numobj) {
    uint16_t h = 0;
    h |= (msg_type & 0x1F);
    h |= ((port_data_role & 0x1) << 5);
    h |= ((spec_rev & 0x3) << 6);
    h |= ((port_power_role & 0x1) << 8);
    h |= ((msg_id & 0x1F) << 9);
    h |= ((numobj & 0x3) << 14);
    return h;
}

void pd_decode_header(uint16_t raw, pd_header_t *out) {
    out->raw             = raw;
    out->msg_type        = raw & 0x1F;
    out->port_data_role  = (raw >> 5) & 0x1;
    out->spec_rev        = (raw >> 6) & 0x3;
    out->port_power_role = (raw >> 8) & 0x1;
    out->msg_id          = (raw >> 9) & 0x1F;
    out->numobj          = (raw >> 14) & 0x3;
}

/* ---- Init ---- */
void pd_engine_init(void) {
    /* Power-up + reset the FUSB302B */
    fusb_write(FUSB302_REG_RESET, 0x01);       /* SW reset */
    for (volatile int i = 0; i < 10000; i++);  /* small delay */
    fusb_write(FUSB302_REG_POWER, 0x0F);       /* enable all power domains */

    /* Configure as a sink for attach detection initially:
     *   - Enable pull-down on CC1 and CC2 (PD_EN on both)
     *   - Enable automatic GoodCRC on SOP */
    fusb_write(FUSB302_REG_SWITCHES0,
               FUSB302_SW0_PD_EN | FUSB302_SW0_AUTO_CRC);
    fusb_write(FUSB302_REG_SWITCHES1,
               FUSB302_SW1_AUTO_CRC);

    /* Mask only the SOP' / SOP'' interrupts initially — we want SOP RX */
    fusb_write(FUSB302_REG_MASK,  0xFE);  /* unmask SOP RX */
    fusb_write(FUSB302_REG_MASKA, 0xFF);
    fusb_write(FUSB302_REG_MASKB, 0x01);

    /* Flush any stale FIFO data */
    fusb_write(FUSB302_REG_CONTROL0,
               FUSB302_CTRL0_TX_FLUSH | FUSB302_CTRL0_RX_FLUSH);

    pd_srand(0x12345678);
    msg_id_counter = 0;
    pd_ring_head = pd_ring_tail = 0;
}

/* ---- Sniff mode ---- */
static int sniff_active = 0;

void pd_sniff_start(void) {
    /* Listen on both CC lines — measure loop to auto-detect orientation.
     * In sniff mode we enable both RX muxes. */
    fusb_write(FUSB302_REG_SWITCHES0, FUSB302_SW0_PD_EN | FUSB302_SW0_AUTO_CRC);
    fusb_write(FUSB302_REG_SWITCHES1,
               FUSB302_SW1_AUTO_CRC | FUSB302_SW1_RXCC1 | FUSB302_SW1_RXCC2);
    sniff_active = 1;
}

void pd_sniff_stop(void) {
    fusb_write(FUSB302_REG_SWITCHES1, FUSB302_SW1_AUTO_CRC);
    sniff_active = 0;
}

/* ---- Read a received frame from FIFO ----
 * The FUSB302 RX FIFO layout: [SOP token][header_lo][header_hi][obj...][CRC...]
 * We read until FIFO empty (STATUS0 RX_EMPTY set).
 */
static void pd_rx_drain(void) {
    uint8_t status0;
    if (fusb_read(FUSB302_REG_STATUS0, &status0, 1) != 0) return;
    if (status0 & 0x20) return; /* RX_EMPTY */

    pd_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    /* Read the first byte: it's the SOP token (0x1F=SOP, 0x1D=SOP', etc.)
     * The FUSB302 marks the SOF with a high bit. We read it and store. */
    uint8_t rxbuf[32];
    int n = fusb_read(FUSB302_REG_FIFOS, rxbuf, 32);
    if (n < 3) return;

    /* First byte = SOP token, next 2 = header, rest = objects + CRC */
    frame.sof = rxbuf[0] & 0x1F;
    uint16_t raw = rxbuf[1] | (rxbuf[2] << 8);
    pd_decode_header(raw, &frame.hdr);

    int objbytes = frame.hdr.numobj * 4;
    if (objbytes > 28) objbytes = 28;
    if (n >= 3 + objbytes) {
        memcpy(frame.objs, &rxbuf[3], objbytes);
        frame.obj_len = objbytes;
    }
    /* CRC follows — we assume hardware checked it */
    frame.crc_ok = 1;

    /* Timestamp from TIM2 (configured as free-running ms counter in main) */
    extern volatile uint32_t g_ms_tick;
    frame.ts_ms = g_ms_tick;

    pd_ring_push(&frame);
}

/* Called from the FUSB_INT EXTI handler (wired in main.c). */
void pd_irq_handler(void) {
    uint8_t irq;
    if (fusb_read(FUSB302_REG_INTERRUPT, &irq, 1) != 0) return;
    if (irq & FUSB302_INT_RXSOP) {
        pd_rx_drain();
    }
    if (irq & FUSB302_INT_TXSOP) {
        /* TX complete — no action needed; fuzzer advances on TX done */
    }
}

/* ---- Send a PD message ---- */
int pd_send(uint8_t sof, const pd_header_t *hdr, const uint8_t *objs,
            uint8_t obj_len) {
    if (!hdr) return -1;
    if (obj_len > 28) return -1;

    uint8_t txbuf[32];
    int idx = 0;

    /* SOP token */
    switch (sof) {
        case 0: txbuf[idx++] = FUSB302_FIFO_TX_TOKEN_SOP; break;
        case 1: txbuf[idx++] = FUSB302_FIFO_TX_TOKEN_SOP_PRIME; break;
        case 2: txbuf[idx++] = FUSB302_FIFO_TX_TOKEN_SOP_DOUBLE; break;
        default: return -1;
    }

    /* Header (little-endian) */
    txbuf[idx++] = hdr->raw & 0xFF;
    txbuf[idx++] = (hdr->raw >> 8) & 0xFF;

    /* Data objects */
    if (obj_len > 0 && objs) {
        memcpy(&txbuf[idx], objs, obj_len);
        idx += obj_len;
    }

    /* Select TX on whichever CC is active — for simplicity we alternate.
     * A production build reads MEASURE to detect orientation. We assume CC1
     * here; the real code checks STATUS0 BC_LVL + PD_ACTIVE. */
    fusb_write(FUSB302_REG_SWITCHES1,
               FUSB302_SW1_TXCC1 | FUSB302_SW1_AUTO_CRC);

    /* Flush TX FIFO then write */
    fusb_write(FUSB302_REG_CONTROL0, FUSB302_CTRL0_TX_FLUSH);
    for (int i = 0; i < idx; i++) {
        fusb_write(FUSB302_REG_FIFOS, txbuf[i]);
    }

    /* Kick TX */
    fusb_write(FUSB302_REG_CONTROL0, FUSB302_CTRL0_TX_START);

    return 0;
}

/* ---- Advertise as source (spoof) ---- */
int pd_advertise_src(uint16_t *pdos, uint8_t cnt) {
    if (cnt > 7) return -1;

    /* Switch to source role: enable Rp (pull-up) instead of Rd */
    fusb_write(FUSB302_REG_SWITCHES0,
               FUSB302_SW0_PU_EN | FUSB302_SW0_AUTO_CRC);

    pd_header_t hdr;
    hdr.raw = pd_build_header(PD_DATA_SRCCAP, PD_SPECREV_3_0,
                              1, 1, msg_id_counter++, cnt);
    return pd_send(0, &hdr, (uint8_t *)pdos, cnt * 4);
}

/* ---- Request a voltage/current from a real source ---- */
int pd_request(uint16_t mv, uint16_t ma) {
    /* Build a Fixed PDO Request object:
     *   bits 0..3:   object position (1-based)
     *   bits 4..9:   0
     *   bits 10..19: operating current (10 mA units)
     *   bits 20..31: operating voltage (50 mV units)
     * For simplicity we request position 1 (first PDO) with our V/I. */
    uint32_t req = 0;
    req |= (1u);                         /* object position 1 */
    req |= ((ma / 10u) & 0x3FF) << 10;   /* operating current */
    req |= ((mv / 50u) & 0xFFF) << 20;   /* operating voltage */

    pd_header_t hdr;
    hdr.raw = pd_build_header(PD_DATA_REQUEST, PD_SPECREV_3_0,
                              0, 0, msg_id_counter++, 1);
    return pd_send(0, &hdr, (uint8_t *)&req, 4);
}

/* ---- Hard Reset ---- */
int pd_hard_reset(void) {
    /* Hard Reset is signaled by setting CONTROL0 with a special token.
     * The FUSB302 has a dedicated mechanism: write 0x46 to FIFOS then
     * assert TX_START. */
    fusb_write(FUSB302_REG_SWITCHES1, FUSB302_SW1_TXCC1);
    fusb_write(FUSB302_REG_CONTROL0, FUSB302_CTRL0_TX_FLUSH);
    fusb_write(FUSB302_REG_FIFOS, 0x46);   /* Hard Reset signaling */
    fusb_write(FUSB302_REG_CONTROL0, FUSB302_CTRL0_TX_START);
    return 0;
}

/* ---- Role swap ---- */
int pd_role_swap(void) {
    pd_header_t hdr;
    hdr.raw = pd_build_header(PD_CTRL_DRSWAP, PD_SPECREV_3_0,
                              1, 1, msg_id_counter++, 0);
    return pd_send(0, &hdr, NULL, 0);
}

/* ---- Dead battery emulation ---- */
void pd_dead_battery(int on) {
    if (on) {
        /* Enable only Rp (pull-up) — this looks like a dead battery that
         * needs 5 V current-limited boot. */
        fusb_write(FUSB302_REG_SWITCHES0,
                   FUSB302_SW0_PU_EN | FUSB302_SW0_AUTO_CRC);
    } else {
        /* Restore normal Rd (pull-down) sink configuration */
        fusb_write(FUSB302_REG_SWITCHES0,
                   FUSB302_SW0_PD_EN | FUSB302_SW0_AUTO_CRC);
    }
}

/* ---- Stateful fuzzer ----
 *
 * The fuzzer tracks the PD contract FSM and sends the next expected
 * message with mutations applied. This achieves far higher crash coverage
 * than random fuzzing because every frame is at least structurally valid
 * enough to reach the DUT's parser.
 */
static fuzz_campaign_t *fuzz = NULL;
static uint8_t fuzz_msg_id = 0;

/* Mutation helpers */
static uint16_t mutate_header(uint16_t raw, fuzz_profile_t prof) {
    switch (prof) {
        case FUZZ_PROF_HEADER: {
            /* Flip a random bit in a random header field */
            uint32_t r = pd_rand();
            uint8_t bit = r & 0x0F;
            raw ^= (1U << bit);
            /* Occasionally overflow numobj */
            if (r & 0x100) raw |= (0x3 << 14);
            break;
        }
        case FUZZ_PROF_PDO:
            /* Leave header sane — mutation is in the PDO payload */
            break;
        case FUZZ_PROF_TIMING:
            /* Header is fine; timing violation handled in fuzz_tick */
            break;
        case FUZZ_PROF_CHUNK:
            /* Set numobj to 0 but send payload — tests chunking path */
            raw &= ~(0x3 << 14);
            raw |= (1U << 14); /* force 1 obj to test chunk parser */
            break;
        default:
            break;
    }
    return raw;
}

static void mutate_pdo(uint8_t *objs, uint8_t len, fuzz_profile_t prof) {
    if (prof != FUZZ_PROF_PDO || len < 4) return;
    uint32_t r = pd_rand();
    /* Corrupt voltage field (bits 20..31 of a Fixed PDO) */
    uint32_t *pdo = (uint32_t *)objs;
    if (r & 1) {
        /* Overvoltage: set to 25 V (500 in 50 mV units) */
        *pdo &= ~(0xFFF << 20);
        *pdo |= (500u << 20);
    } else if (r & 2) {
        /* Overcurrent: set to 10 A (1000 in 10 mA units) */
        *pdo &= ~(0x3FF << 10);
        *pdo |= (1000u << 10);
    } else {
        /* Random garbage in upper bits */
        *pdo ^= (r & 0xFFFF0000);
    }
}

/* Send the next fuzz message. We cycle through the contract FSM:
 *   1. Source Cap (we are spoofing source)
 *   2. Request (from DUT — we can't force this, so we send a malformed Accept)
 *   3. PS_RDY
 *   4. Soft Reset
 *   ... then loop.
 */
static void fuzz_send_next(void) {
    if (!fuzz || fuzz->sent >= fuzz->count) {
        fuzz->running = 0;
        return;
    }

    static int fsm_state = 0;
    pd_header_t hdr;
    uint8_t objs[28];
    uint8_t objlen = 0;

    switch (fsm_state) {
        case 0: {
            /* Send a mutated Source Capabilities with 1 Fixed PDO */
            uint32_t pdo = 0;
            pdo |= (1u << 31) | (1u << 30); /* Fixed supply, dual-role */
            pdo |= (100u << 10);            /* 1 A max */
            pdo |= (100u << 20);            /* 5 V (100 × 50 mV) */
            memcpy(objs, &pdo, 4);
            objlen = 4;
            hdr.raw = pd_build_header(PD_DATA_SRCCAP, PD_SPECREV_3_0,
                                      1, 1, fuzz_msg_id, 1);
            fsm_state = 1;
            break;
        }
        case 1: {
            /* Send PS_RDY prematurely (no Request received) */
            hdr.raw = pd_build_header(PD_CTRL_PSRDY, PD_SPECREV_3_0,
                                      1, 1, fuzz_msg_id, 0);
            fsm_state = 2;
            break;
        }
        case 2: {
            /* Send Soft Reset */
            hdr.raw = pd_build_header(PD_CTRL_SOFTRESET, PD_SPECREV_3_0,
                                      1, 1, fuzz_msg_id, 0);
            fsm_state = 3;
            break;
        }
        case 3: {
            /* Send a Get Source Cap (role-inconsistent — we are source) */
            hdr.raw = pd_build_header(PD_CTRL_GETSRCCAP, PD_SPECREV_3_0,
                                      0, 0, fuzz_msg_id, 0); /* wrong role */
            fsm_state = 0;
            break;
        }
        default:
            fsm_state = 0;
            break;
    }

    /* Apply mutations */
    hdr.raw = mutate_header(hdr.raw, fuzz->profile);
    mutate_pdo(objs, objlen, fuzz->profile);

    pd_send(0, &hdr, objs, objlen);
    fuzz_msg_id = (fuzz_msg_id + 1) & 0x1F;
    fuzz->sent++;

    /* Timing profile: occasionally send immediately (violates tSenderResponse) */
    if (fuzz->profile == FUZZ_PROF_TIMING) {
        uint32_t r = pd_rand();
        if (r & 1) {
            /* Send a second frame immediately to stress the DUT */
            hdr.raw = pd_build_header(PD_CTRL_PING, PD_SPECREV_3_0,
                                      1, 1, fuzz_msg_id, 0);
            pd_send(0, &hdr, NULL, 0);
            fuzz_msg_id = (fuzz_msg_id + 1) & 0x1F;
            fuzz->sent++;
        }
    }
}

void fuzz_start(fuzz_campaign_t *cfg) {
    if (!cfg) return;
    fuzz = cfg;
    fuzz->sent = 0;
    fuzz->crash_cnt = 0;
    fuzz->running = 1;
    fuzz_msg_id = 0;
    pd_srand(cfg->seed);
    /* Enable source role for spoofing */
    fusb_write(FUSB302_REG_SWITCHES0,
               FUSB302_SW0_PU_EN | FUSB302_SW0_AUTO_CRC);
}

void fuzz_stop(void) {
    if (fuzz) fuzz->running = 0;
}

void fuzz_tick(void) {
    if (!fuzz || !fuzz->running) return;
    /* Rate-limit: send one fuzz frame every 5 ms (200/s) */
    static uint32_t last = 0;
    extern volatile uint32_t g_ms_tick;
    if (g_ms_tick - last < 5) return;
    last = g_ms_tick;
    fuzz_send_next();
}

/* end of file — author: jayis1 */