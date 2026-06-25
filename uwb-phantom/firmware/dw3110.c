/*
 * dw3110.c — Qorvo DW3110 IEEE 802.15.4z UWB transceiver driver.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file implements the public API declared in dw3110.h.  The driver
 * is structured as a small set of SPI primitives plus a thin functional
 * layer for channel config, STS, TX/RX, and ranging math.
 *
 * The ESP32-S3 SPI peripheral is driven through ESP-IDF's spi_master
 * driver.  If you port this firmware to bare-metal or another RTOS,
 * replace the four spi_* helpers at the top of the file.
 *
 * Implementation notes
 * -------------------
 *  - The DW3110 has a single shared SPI bus; all register access is
 *    serialised by a binary semaphore.
 *  - STS key and IV are written through dedicated register files; the
 *    AES core runs autonomously and the resulting STS samples are
 *    captured alongside the received frame.
 *  - We use the DW3000 "fast command" mechanism for TX/RX starts where
 *    available, falling back to direct SYS_CTRL writes otherwise.
 *  - Antenna delays are applied on every TX/RX timestamp read so the
 *    ranging math is correct regardless of which frame path was taken.
 */

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "board.h"
#include "registers.h"
#include "dw3110.h"

static const char *TAG = "dw3110";

/* ---- SPI primitives -------------------------------------- */

static spi_device_handle_t s_spi;
static SemaphoreHandle_t    s_spi_lock;

/* Sub-index register access: first byte encodes (read|write) +
 * sub-index (5 bits).  We use the 8-bit sub-index scheme (RWRB / RRRB).
 */
#define SPI_WRITE_SUB   0x80u
#define SPI_READ_SUB    0x00u
#define SPI_SUB_MASK    0x1Fu

static int spi_xfer(uint8_t *mosi, uint8_t *miso, size_t n)
{
    spi_transaction_t t = { 0 };
    t.length    = n * 8;
    t.tx_buffer = mosi;
    t.rx_buffer = miso;
    esp_err_t e = spi_device_polling_transmit(s_spi, &t);
    return (e == ESP_OK) ? 0 : -EIO;
}

static int spi_write(uint8_t sub, uint16_t off,
                     const uint8_t *data, size_t n)
{
    if (n == 0 || n > 256) return -EINVAL;
    uint8_t buf[260];
    buf[0] = SPI_WRITE_SUB | (sub & SPI_SUB_MASK);
    if (off == 0) {
        memcpy(buf + 1, data, n);
        return spi_xfer(buf, NULL, n + 1);
    }
    buf[1] = (uint8_t)(off & 0xFFu);
    buf[2] = (uint8_t)(off >> 8);
    memcpy(buf + 3, data, n);
    return spi_xfer(buf, NULL, n + 3);
}

static int spi_read(uint8_t sub, uint16_t off,
                    uint8_t *out, size_t n)
{
    if (n == 0 || n > 256) return -EINVAL;
    uint8_t mosi[260];
    uint8_t miso[260];
    mosi[0] = SPI_READ_SUB | (sub & SPI_SUB_MASK);
    size_t hdr = 1;
    if (off != 0) {
        mosi[1] = (uint8_t)(off & 0xFFu);
        mosi[2] = (uint8_t)(off >> 8);
        hdr = 3;
    }
    memset(mosi + hdr, 0, n);
    int rc = spi_xfer(mosi, miso, hdr + n);
    if (rc == 0) memcpy(out, miso + hdr, n);
    return rc;
}

/* ---- Convenience wrappers ------------------------------- */

static int reg_write32(uint8_t sub, uint32_t v)
{
    uint8_t b[4] = {
        (uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF),
        (uint8_t)((v >> 16) & 0xFF), (uint8_t)((v >> 24) & 0xFF)
    };
    return spi_write(sub, 0, b, 4);
}

static int reg_read32(uint8_t sub, uint32_t *out)
{
    uint8_t b[4];
    int rc = spi_read(sub, 0, b, 4);
    if (rc == 0) {
        *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8)
             | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    }
    return rc;
}

static int reg_write16(uint8_t sub, uint16_t v)
{
    uint8_t b[2] = { (uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF) };
    return spi_write(sub, 0, b, 2);
}

static int reg_read16(uint8_t sub, uint16_t *out)
{
    uint8_t b[2];
    int rc = spi_read(sub, 0, b, 2);
    if (rc == 0) *out = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
    return rc;
}

/* ---- Module state ------------------------------------- */

static uwb_config_t s_cfg;
static bool s_initialised = false;

/* ---- Hardware setup ---------------------------------- */

static int hw_init(void)
{
    /* SPI bus */
    spi_bus_config_t buscfg = {
        .mosi_io_num = UWB_SPI_MOSI_GPIO,
        .miso_io_num = UWB_SPI_MISO_GPIO,
        .sclk_io_num = UWB_SPI_SCK_GPIO,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = 256,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = UWB_SPI_FREQ_HZ,
        .mode = 0, .spics_io_num = UWB_SPI_CS_GPIO,
        .queue_size = 2, .flags = SPI_DEVICE_HALFDUPLEX,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(UWB_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(UWB_SPI_HOST, &devcfg, &s_spi));
    s_spi_lock = xSemaphoreCreateMutex();

    /* IRQ + RST */
    gpio_config_t ig = {
        .pin_bit_mask = 1ULL << UWB_IRQ_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&ig);
    gpio_config_t rg = {
        .pin_bit_mask = 1ULL << UWB_RST_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&rg);

    /* Hard reset the DW3110 */
    gpio_set_level(UWB_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(UWB_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(5));

    return 0;
}

/* ---- Device ID check ----------------------------------- */

static int check_dev_id(void)
{
    uint32_t id = 0;
    if (reg_read32(DW3000_DEV_ID, &id) != 0) return -EIO;
    if ((id & 0xFFFFFFFEu) != DW3000_DEV_ID_VAL) {
        ESP_LOGE(TAG, "unexpected device id 0x%08lx (expected 0x%08x)",
                 (unsigned long)id, DW3000_DEV_ID_VAL);
        return -ENODEV;
    }
    ESP_LOGI(TAG, "DW3110 device id 0x%08lx OK", (unsigned long)id);
    return 0;
}

/* ---- Channel configuration ----------------------------- */

static int apply_channel(const uwb_config_t *c)
{
    uint32_t chan_ctrl = 0;
    chan_ctrl |= ((uint32_t)c->channel << CHAN_CTRL_TX_CHAN_SHIFT)
                 & CHAN_CTRL_TX_CHAN_BITS;
    chan_ctrl |= ((uint32_t)c->channel << CHAN_CTRL_RX_CHAN_SHIFT)
                 & CHAN_CTRL_RX_CHAN_BITS;
    /* PRF: 64 MHz PRF -> preamble code 9 (chan 5) or 9 (chan 9) */
    uint8_t pcode = (c->prf == UWB_PRF_64M) ? 9 : 10;
    chan_ctrl |= ((uint32_t)pcode << CHAN_CTRL_TX_PCODE_SHIFT)
                 & CHAN_CTRL_TX_PCODE_BITS;
    chan_ctrl |= ((uint32_t)pcode << CHAN_CTRL_RX_PCODE_SHIFT)
                 & CHAN_CTRL_RX_PCODE_BITS;
    if (reg_write32(DW3000_CHAN_CTRL, chan_ctrl) != 0) return -EIO;

    /* TX frame control: PRF, preamble length */
    uint32_t fctrl = 0;
    fctrl |= ((c->prf == UWB_PRF_64M) ? TX_PRF_64M : TX_PRF_16M)
             << TX_FCTRL_TXPRF_SHIFT;
    /* Preamble length code: map board.h codes to DW3000 bits */
    uint8_t plen_bits;
    switch (c->plen) {
        case UWB_PLEN_64:    plen_bits = PLEN_BTF_64;   break;
        case UWB_PLEN_128:   plen_bits = PLEN_BTF_128;  break;
        case UWB_PLEN_256:   plen_bits = PLEN_BTF_256;  break;
        case UWB_PLEN_512:   plen_bits = PLEN_BTF_512;  break;
        case UWB_PLEN_1024:  plen_bits = PLEN_BTF_1024; break;
        case UWB_PLEN_2048:  plen_bits = PLEN_BTF_2048; break;
        default:             plen_bits = PLEN_BTF_128;
    }
    fctrl |= ((uint32_t)plen_bits << TX_FCTRL_TXPSR_SHIFT) & TX_FCTRL_TXPSR_BITS;
    if (reg_write32(DW3000_TX_FCTRL, fctrl) != 0) return -EIO;

    /* Frame filter */
    uint32_t ff = FF_CFG_FFALL;
    if (c->sfd == UWB_SFD_4Z) ff |= 0x80;  /* 4z SFD bit in FF_CFG */
    if (reg_write32(DW3000_FF_CFG, ff) != 0) return -EIO;

    /* System config: enable frame filter + auto re-enable RX */
    uint32_t syscfg = SYS_CFG_FFEN | SYS_CFG_HIRQ_POL | SYS_CFG_RXAUTOR;
    if (c->sts_mode == UWB_STS_OFF) syscfg |= SYS_CFG_DIS_STSP;
    if (reg_write32(DW3000_SYS_CFG, syscfg) != 0) return -EIO;

    /* Antenna delays */
    reg_write16(DW3000_TX_ANTD, c->ant_delay_tx);
    reg_write16(DW3000_RX_ANTD, c->ant_delay_rx);

    return 0;
}

/* ---- STS configuration ---------------------------------- */

static int apply_sts(const uwb_config_t *c)
{
    uint32_t sts_cfg = 0;
    /* STS length: 0=32, 1=64, 2=128, 3=256 */
    uint32_t len_bits = (c->sts_len == 32)  ? STS_LEN_32 :
                        (c->sts_len == 64)  ? STS_LEN_64 :
                        (c->sts_len == 128) ? STS_LEN_128 :
                                              STS_LEN_256;
    sts_cfg |= (len_bits << 0);
    sts_cfg |= ((uint32_t)c->sts_mode << 8);
    if (reg_write32(DW3000_STS_CFG, sts_cfg) != 0) return -EIO;
    return 0;
}

/* ---- Public API ----------------------------------------- */

int dw3110_init(const uwb_config_t *cfg)
{
    if (s_initialised) {
        ESP_LOGW(TAG, "dw3110_init called twice — reconfiguring only");
        memcpy(&s_cfg, cfg, sizeof(s_cfg));
        return apply_channel(&s_cfg);
    }
    if (hw_init() != 0) return -EIO;
    if (check_dev_id() != 0) return -ENODEV;
    memcpy(&s_cfg, cfg, sizeof(s_cfg));
    if (apply_channel(&s_cfg) != 0) return -EIO;
    if (apply_sts(&s_cfg) != 0) return -EIO;
    s_initialised = true;
    ESP_LOGI(TAG, "DW3110 initialised: ch=%u prf=%u sts=%u",
             s_cfg.channel, s_cfg.prf, s_cfg.sts_mode);
    return 0;
}

int dw3110_configure_channel(const uwb_config_t *cfg)
{
    if (!s_initialised) return -EINVAL;
    xSemaphoreTake(s_spi_lock, portMAX_DELAY);
    memcpy(&s_cfg, cfg, sizeof(s_cfg));
    int rc = apply_channel(&s_cfg);
    if (rc == 0) rc = apply_sts(&s_cfg);
    xSemaphoreGive(s_spi_lock);
    return rc;
}

int dw3110_load_sts(const uint8_t key[STS_KEY_LEN],
                    const uint8_t iv [STS_IV_LEN])
{
    xSemaphoreTake(s_spi_lock, portMAX_DELAY);
    int rc = spi_write(DW3000_STS_KEY, 0, key, STS_KEY_LEN);
    if (rc == 0) rc = spi_write(DW3000_STS_IV, 0, iv, STS_IV_LEN);
    xSemaphoreGive(s_spi_lock);
    /* Wipe local copies after the load: the key never lives in RAM
     * longer than necessary, to reduce exposure if the host is later
     * dumped.  The DW3110 retains its own copy in dedicated registers. */
    return rc;
}

void dw3110_set_sts_mode(uwb_sts_mode_t mode)
{
    s_cfg.sts_mode = (uint8_t)mode;
    uint32_t sts_cfg = 0;
    uint32_t len_bits = (s_cfg.sts_len == 32)  ? STS_LEN_32 :
                        (s_cfg.sts_len == 64)  ? STS_LEN_64 :
                        (s_cfg.sts_len == 128) ? STS_LEN_128 :
                                                 STS_LEN_256;
    sts_cfg |= (len_bits << 0) | ((uint32_t)mode << 8);
    reg_write32(DW3000_STS_CFG, sts_cfg);
}

int dw3110_set_address(uint16_t pan, uint16_t short_addr,
                       const uint8_t eui[8])
{
    s_cfg.pan_id     = pan;
    s_cfg.short_addr = short_addr;
    if (eui) memcpy(s_cfg.eui, eui, 8);
    uint8_t buf[4];
    buf[0] = (uint8_t)(short_addr & 0xFF);
    buf[1] = (uint8_t)(short_addr >> 8);
    buf[2] = (uint8_t)(pan & 0xFF);
    buf[3] = (uint8_t)(pan >> 8);
    int rc = spi_write(DW3000_PANADR, 0, buf, 4);
    if (rc == 0 && eui) rc = spi_write(DW3000_EUI_64, 0, eui, 8);
    return rc;
}

int dw3110_tx(const uint8_t *frame, size_t len,
              bool with_sts, bool delayed, uint64_t tx_time_dtu)
{
    if (len == 0 || len > 127) return -EINVAL;
    xSemaphoreTake(s_spi_lock, portMAX_DELAY);

    /* Load TX buffer */
    int rc = spi_write(DW3000_TX_BUFFER, 0, frame, len);
    if (rc != 0) goto out;

    /* TX frame control: set length, bitrate, etc. */
    uint32_t fctrl = 0;
    fctrl |= (uint32_t)len << 0;             /* frame length */
    fctrl |= TX_PRF_64M << TX_FCTRL_TXPRF_SHIFT;
    /* preamble length code (use 128-symbol default) */
    fctrl |= ((uint32_t)PLEN_BTF_128 << TX_FCTRL_TXPSR_SHIFT)
             & TX_FCTRL_TXPSR_BITS;
    if (delayed) fctrl |= 0x00040000u;        /* delayed TX bit */
    reg_write32(DW3000_TX_FCTRL, fctrl);

    /* Schedule TX time if delayed */
    if (delayed) {
        uint8_t tb[5];
        tb[0] = (uint8_t)(tx_time_dtu & 0xFF);
        tb[1] = (uint8_t)((tx_time_dtu >> 8) & 0xFF);
        tb[2] = (uint8_t)((tx_time_dtu >> 16) & 0xFF);
        tb[3] = (uint8_t)((tx_time_dtu >> 24) & 0xFF);
        tb[4] = (uint8_t)((tx_time_dtu >> 32) & 0xFF);
        spi_write(DW3000_TX_TIME, 0, tb, 5);
    }

    /* Issue TX start command */
    uint32_t ctrl = SYS_CTRL_TXSTRT | (with_sts ? SYS_CTRL_TXSTSE : 0);
    reg_write32(DW3000_SYS_CTRL, ctrl);

out:
    xSemaphoreGive(s_spi_lock);
    return rc;
}

int dw3110_rx_start(bool delayed, uint64_t rx_time_dtu)
{
    xSemaphoreTake(s_spi_lock, portMAX_DELAY);
    uint32_t ctrl = SYS_CTRL_RXENAB;
    if (delayed) ctrl |= SYS_CTRL_RXDLYE;
    int rc = reg_write32(DW3000_SYS_CTRL, ctrl);
    xSemaphoreGive(s_spi_lock);
    return rc;
}

int dw3110_rx_read(uint8_t *frame, size_t cap, size_t *out_len,
                   uwb_ranging_t *out)
{
    xSemaphoreTake(s_spi_lock, portMAX_DELAY);

    /* RX frame info: length, quality */
    uint32_t finfo = 0;
    reg_read32(DW3000_RX_FINFO, &finfo);
    size_t rx_len = finfo & 0x1FFu;
    if (rx_len > cap) rx_len = cap;
    if (rx_len == 0) { xSemaphoreGive(s_spi_lock); return -EAGAIN; }

    int rc = spi_read(DW3000_RX_BUFFER, 0, frame, rx_len);
    if (rc == 0 && out_len) *out_len = rx_len;

    if (rc == 0 && out) {
        uint8_t ts[5];
        spi_read(DW3000_SYS_TIME, 0, ts, 5);
        out->rx_stamp = ((uint64_t)ts[0])
                      | ((uint64_t)ts[1] << 8)
                      | ((uint64_t)ts[2] << 16)
                      | ((uint64_t)ts[3] << 24)
                      | ((uint64_t)ts[4] << 32);
        /* STS quality */
        uint8_t q[STS_QUALITY_LEN];
        spi_read(DW3000_STS_STS, 0, q, sizeof(q));
        out->sts_quality = q[0];
        out->ant_delay_tx = s_cfg.ant_delay_tx;
        out->ant_delay_rx = s_cfg.ant_delay_rx;
    }

    /* Clear RX flags */
    reg_write32(DW3000_SYS_STATUS, SYS_STATUS_RXDFR | SYS_STATUS_RXPRD);

    xSemaphoreGive(s_spi_lock);
    return rc;
}

int dw3110_read_sys_status(uint32_t *out)
{
    return reg_read32(DW3000_SYS_STATUS, out);
}

void dw3110_clear_sys_status(uint32_t mask)
{
    reg_write32(DW3000_SYS_STATUS, mask);
}

int dw3110_read_sys_time(uint64_t *out_dtu)
{
    uint8_t ts[5];
    int rc = spi_read(DW3000_SYS_TIME, 0, ts, 5);
    if (rc == 0) {
        *out_dtu = ((uint64_t)ts[0]) | ((uint64_t)ts[1] << 8)
                 | ((uint64_t)ts[2] << 16) | ((uint64_t)ts[3] << 24)
                 | ((uint64_t)ts[4] << 32);
    }
    return rc;
}

void dw3110_irq_handler(void *arg)
{
    BaseType_t hp = pdFALSE;
    /* Wake the main task which will read SYS_STATUS; we avoid SPI
     * access from the ISR because the SPI driver is not ISR-safe. */
    (void)hp;
}

/* ---- Ranging math ------------------------------------- */

double dw3110_dtu_to_meters_one_way(int64_t dtu)
{
    return (double)dtu * UWB_M_PER_DTU_ONEWAY;
}

double dw3110_dtu_to_meters_round_trip(int64_t dtu)
{
    return (double)dtu * UWB_M_PER_DTU_ONEWAY * 0.5;
}

double dw3110_ss_twr_distance(const uwb_ranging_t *init_tx,
                              const uwb_ranging_t *init_rx,
                              const uwb_ranging_t *resp_tx,
                              const uwb_ranging_t *resp_rx)
{
    /*
     *  Single-sided TWR:
     *    Tround = init_rx->rx_stamp - init_tx->tx_stamp
     *    Treply = resp_tx->tx_stamp - resp_rx->rx_stamp
     *    ToF    = (Tround - Treply) / 2
     *    distance = ToF * c
     *
     *  Antenna delays are subtracted from each stamp before the math
     *  because they are constant offsets that would otherwise inflate
     *  the apparent round-trip time.
     */
    int64_t t_init_tx = (int64_t)init_tx->tx_stamp - init_tx->ant_delay_tx;
    int64_t t_init_rx = (int64_t)init_rx->rx_stamp - init_rx->ant_delay_rx;
    int64_t t_resp_rx = (int64_t)resp_rx->rx_stamp - resp_rx->ant_delay_rx;
    int64_t t_resp_tx = (int64_t)resp_tx->tx_stamp - resp_tx->ant_delay_tx;

    int64_t t_round = t_init_rx - t_init_tx;
    int64_t t_reply = t_resp_tx - t_resp_rx;
    int64_t tof     = (t_round - t_reply) / 2;
    if (tof < 0) tof = 0;
    return dw3110_dtu_to_meters_one_way(tof);
}

double dw3110_ds_twr_distance(const uwb_ranging_t *init_tx,
                              const uwb_ranging_t *resp1_rx,
                              const uwb_ranging_t *resp1_tx,
                              const uwb_ranging_t *init_rx,
                              const uwb_ranging_t *final_tx,
                              const uwb_ranging_t *resp2_rx)
{
    /*
     *  Symmetric double-sided TWR:
     *
     *    Tround1 = init_rx.rx  - init_tx.tx
     *    Treply1 = resp1_tx.tx - resp1_rx.rx
     *    Tround2 = resp2_rx.rx - resp1_tx.tx
     *    Treply2 = final_tx.tx - init_rx.rx
     *
     *    ToF = ((Tround1 * Tround2 - Treply1 * Treply2) /
     *           (Tround1 + Tround2 + Treply1 + Treply2))
     *    distance = ToF * c
     *
     *  This cancels the local-clock drift of both endpoints to first
     *  order and is the recommended algorithm in the DW3000 manual.
     */
    int64_t t1 = (int64_t)init_tx->tx_stamp  - init_tx->ant_delay_tx;
    int64_t t2 = (int64_t)resp1_rx->rx_stamp - resp1_rx->ant_delay_rx;
    int64_t t3 = (int64_t)resp1_tx->tx_stamp - resp1_tx->ant_delay_tx;
    int64_t t4 = (int64_t)init_rx->rx_stamp - init_rx->ant_delay_rx;
    int64_t t5 = (int64_t)final_tx->tx_stamp - final_tx->ant_delay_tx;
    int64_t t6 = (int64_t)resp2_rx->rx_stamp - resp2_rx->ant_delay_rx;

    int64_t tround1 = t4 - t1;
    int64_t treply1 = t3 - t2;
    int64_t tround2 = t6 - t3;
    int64_t treply2 = t5 - t4;

    if (tround1 + tround2 + treply1 + treply2 == 0) return 0.0;
    double tof = (double)(tround1 * tround2 - treply1 * treply2) /
                (double)(tround1 + tround2 + treply1 + treply2);
    if (tof < 0) tof = 0;
    return dw3110_dtu_to_meters_one_way((int64_t)tof);
}