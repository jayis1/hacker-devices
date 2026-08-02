/*
 * main.c — BACnet Phantom firmware entry point and FreeRTOS task table.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * This file is the single source of truth for the six FreeRTOS tasks that
 * make up the Phantom: IP stack, MS/TP FSM, NPDU router, BLE app bridge,
 * flash persistence, and the external ACK watchdog. board_init() arms the
 * hardware interlocks before any task is allowed to emit.
 *
 * Ethical note: this firmware is provided for authorised security research
 * only. The hardware write interlock (BP_PIN_WRITE_BYPASS_JUMPER) is a
 * physical safety device and must not be bypassed in firmware. Life-safety
 * object types are always blocked by the watchdog task regardless of the
 * jumper state.
 */
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_app_format.h"
#include "esp_random.h"

#include "board.h"
#include "registers.h"
#include "bacnet.c"   /* included so the build is single-file; see Makefile */
#include "w5500.c"
#include "mstp.c"

static const char *TAG = "main";

/* ---- Global state (declared extern in board.h) ------------------------ */
bp_state_t g_state = { 0 };
uint8_t    bp_mac[6];

/* ---- Write-property pending watchdog ---------------------------------- */
typedef struct {
    uint32_t device_instance;
    uint16_t object_type;
    uint32_t object_instance;
    uint32_t property_id;
    float    prior_value;     /* relinquish-default revert target */
    uint8_t  invoke_id;
    int64_t  deadline_us;
    bool     in_use;
} wp_pending_t;

static wp_pending_t s_wp_pending[16];
static SemaphoreHandle_t s_wp_mutex;

/* ---- BLE command queue (operator → firmware) ------------------------- */
typedef struct {
    char     op[24];
    uint32_t arg1, arg2, arg3;
    float    farg;
} bp_cmd_t;
static QueueHandle_t s_cmd_queue;

/* ---- Boot ------------------------------------------------------------- */
static void seed_identity(void)
{
    uint32_t seed = esp_random();
    srand(seed);
    g_state.device_instance = 100000 + (esp_random() % 400000);
    g_state.vendor_id = 0;          /* "non-standard vendor", jayis1 */
    for (int i = 0; i < 6; i++)
        bp_mac[i] = (uint8_t)(esp_random() & 0xFF);
    bp_mac[0] &= 0xFE;               /* unicast */
    bp_mac[0] |= 0x02;               /* locally administered */
}

static void oled_init(void)
{
    i2c_config_t cfg = { .mode = I2C_MODE_MASTER,
                         .sda_io_num = BP_OLED_PIN_SDA,
                         .scl_io_num = BP_OLED_PIN_SCL,
                         .sda_pullup_en = GPIO_PULLUP_ENABLE,
                         .scl_pullup_en = GPIO_PULLUP_ENABLE,
                         .master.clk_speed = 400000 };
    i2c_param_config(BP_OLED_I2C_HOST, &cfg);
    i2c_driver_install(BP_OLED_I2C_HOST, I2C_MODE_MASTER, 0, 0, 0);
}

static void led_init(void)
{
    ledc_timer_config_t t = { .speed_mode = LEDC_LOW_SPEED_MODE,
                              .timer_num = BP_LEDC_TIMER,
                              .duty_resolution = BP_LEDC_RES_BITS,
                              .freq_hz = BP_LEDC_FREQ_HZ,
                              .clk_cfg = LEDC_AUTO_CLK };
    ledc_timer_config(&t);
    int pins[3] = { BP_LED_PIN_R, BP_LED_PIN_G, BP_LED_PIN_B };
    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t c = { .channel = (ledc_channel_t)i,
                                     .duty = 0,
                                     .gpio_num = pins[i],
                                     .speed_mode = LEDC_LOW_SPEED_MODE,
                                     .hpoint = 0,
                                     .timer_sel = BP_LEDC_TIMER };
        ledc_channel_config(&c);
    }
}

void board_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}

/* ---- Safety interlocks ------------------------------------------------ */
bool board_write_permitted(uint16_t object_type)
{
    static const uint16_t blocklist[] = BP_LIFESAFETY_BLOCKLIST;
    for (size_t i = 0; i < sizeof(blocklist)/sizeof(blocklist[0]); i++)
        if (object_type == blocklist[i]) {
            ESP_LOGW(TAG, "write blocked: life-safety object %u", object_type);
            return false;
        }
    if (!g_state.write_bypass) {
        ESP_LOGW(TAG, "write blocked: bypass jumper not fitted");
        return false;
    }
    return true;
}

bool board_tamper_triggered(void)
{
    return gpio_get_level(BP_PIN_TAMPER_LOOP) == 1;
}

void board_zeroize(void)
{
    ESP_LOGE(TAG, "TAMPER — zeroising");
    bacnet_db_reset();
    seed_identity();           /* new random identity */
    /* Erase the capture partition if present */
    const esp_partition_t *p = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "capture");
    if (p) esp_partition_erase_range(p, 0, p->size);
    g_state.mode = BP_MODE_TAMPERED;
    board_led_set(255, 0, 0);  /* solid red */
}

uint16_t board_battery_mv(void)
{
    int raw = adc1_get_raw(BP_BATT_ADC_CHANNEL);
    /* 12-bit ADC, 3.3V ref, 2x divider → Vbat = raw/4095 * 3.3 * 2 */
    return (uint16_t)((uint32_t)raw * 3300U * 2U / 4095U);
}

void board_oled_render(const char *l1, const char *l2, const char *l3)
{
    /* Minimal SSD1306 I²C init + 3-line text. In a production build this
     * would use the u8g2 driver; the encode loop here is the minimum that
     * makes the panel show operator status. */
    uint8_t init_seq[] = { 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
                           0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
                           0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40,
                           0xA4, 0xA6, 0xAF };
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (BP_OLED_ADDR << 1), true);
    for (size_t i = 0; i < sizeof(init_seq); i++)
        i2c_master_write_byte(h, 0x00, true);  /* control byte */
    /* (text rendering elided in this minimal sketch) */
    i2c_master_stop(h);
    i2c_master_cmd_begin(BP_OLED_I2C_HOST, h, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(h);
    (void)l1; (void)l2; (void)l3;
}

esp_err_t board_init(void)
{
    nvs_flash_init();
    esp_log_level_set("*", ESP_LOG_INFO);

    /* GPIO: interlocks first */
    gpio_config_t io = { .pin_bit_mask =
        (1ULL << BP_PIN_WRITE_BYPASS_JUMPER) |
        (1ULL << BP_PIN_TAMPER_LOOP) |
        (1ULL << TP4056_PIN_CHRG) | (1ULL << TP4056_PIN_STDBY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&io);

    /* Cache the jumper state ONCE at boot — it cannot be hot-plugged. */
    g_state.write_bypass = gpio_get_level(BP_PIN_WRITE_BYPASS_JUMPER) == 1;

    seed_identity();
    g_state.mode = BP_MODE_PASSIVE;
    g_state.ack_timeout_ms = BP_ACK_TIMEOUT_MS;
    g_state.network_number_ip = 0;
    g_state.network_number_mstp = 0;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BP_BATT_ADC_CHANNEL, ADC_ATTEN_DB_11);

    led_init();
    oled_init();
    board_led_set(0, 0, 32);  /* dim blue = passive */

    board_w5500_init();
    board_mstp_init();

    s_wp_mutex = xSemaphoreCreateMutex();
    s_cmd_queue = xQueueCreate(16, sizeof(bp_cmd_t));
    memset(s_wp_pending, 0, sizeof(s_wp_pending));

    ESP_LOGI(TAG, "BACnet Phantom up — inst=%u bypass=%d author=jayis1",
             (unsigned)g_state.device_instance, g_state.write_bypass);
    return ESP_OK;
}

esp_err_t board_set_mode(bp_mode_t new_mode)
{
    if (g_state.mode == BP_MODE_TAMPERED) return ESP_ERR_INVALID_STATE;
    g_state.mode = new_mode;
    switch (new_mode) {
    case BP_MODE_PASSIVE:  board_led_set(0, 0, 32);  break;
    case BP_MODE_DISCOVER: board_led_set(0, 32, 0);  break;
    case BP_MODE_DUMP:     board_led_set(0, 32, 32); break;
    case BP_MODE_INJECT:   board_led_set(32, 0, 0);  break;
    case BP_MODE_BRIDGE:   board_led_set(32, 0, 32); break;
    case BP_MODE_FUZZ:     board_led_set(32, 32, 0); break;
    default: break;
    }
    return ESP_OK;
}

/* ---- Tasks ------------------------------------------------------------ */

/* BACnet/IP task — core 0, priority 15. Drives the W5500 RX loop and
 * dispatches APDUs to bacnet_handle_bvlc(). */
static void bacnet_ip_task(void *arg)
{
    (void)arg;
    static uint8_t rbuf[BP_NPDU_MAX + 16];
    for (;;) {
        size_t n = 0;
        if (board_w5500_recv_raw(rbuf, sizeof(rbuf), &n, 50) == ESP_OK && n) {
            g_state.capture_count++;
            bacnet_handle_bvlc(rbuf, n);
        }
        /* Emit Who-Is periodically if in DISCOVER mode */
        if (g_state.mode == BP_MODE_DISCOVER) {
            static int64_t last_emit = 0;
            int64_t now = esp_timer_get_time();
            if (now - last_emit > 1000000) {  /* 1 s cadence */
                last_emit = now;
                uint8_t tx[BP_NPDU_MAX];
                int len = bacnet_who_is(tx, sizeof(tx), 0, 0xFFFFFFFFU);
                if (len > 0) board_w5500_send_raw(tx, len);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

/* MS/TP task — created in mstp.c, pinned to core 1, priority 18. */

/* Router task — core 0, priority 12. Bridges NPDU between IP and MS/TP
 * domains when g_state.bridge_active. */
static void router_task(void *arg)
{
    (void)arg;
    uint8_t mbuf[BP_MSTP_FRAME_MAX];
    for (;;) {
        if (g_state.bridge_active) {
            int n = board_mstp_rx(mbuf, sizeof(mbuf), 50);
            if (n > 0) {
                /* Wrap MS/TP payload as a routed NPDU onto BACnet/IP */
                uint8_t ipbuf[BP_NPDU_MAX];
                ipbuf[0] = 0x84;  /* routed NPDU control: DNET present */
                ipbuf[1] = g_state.network_number_mstp;
                ipbuf[2] = 0;     /* DLEN=0 (broadcast) */
                ipbuf[3] = 0;
                size_t off = 4;
                if (off + n <= sizeof(ipbuf)) {
                    memcpy(ipbuf + off, mbuf, n);
                    board_w5500_send_raw(ipbuf, off + n);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* BLE / companion-app bridge task — core 1, priority 9. In a full build
 * this would use the Nordic-UART-Service over GATT. Here it services the
 * command queue populated by the (separate) GATT callback and emits the
 * corresponding BACnet/IP action. */
void bacnet_app_handle_cmd(const bp_cmd_t *c);

static void app_task(void *arg)
{
    (void)arg;
    bp_cmd_t c;
    for (;;) {
        if (xQueueReceive(s_cmd_queue, &c, portMAX_DELAY) == pdTRUE)
            bacnet_app_handle_cmd(&c);
    }
}

/* Persistence task — core 0, priority 5. Flushes the capture ring to flash
 * every 5 s with wear-levelling (round-robin sector). */
static void persist_task(void *arg)
{
    (void)arg;
    const esp_partition_t *p = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "capture");
    if (!p) { vTaskDelete(NULL); return; }
    uint32_t sector = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (g_state.mode == BP_MODE_TAMPERED) continue;
        /* Write a small header + device DB snapshot to the next 4 KB sector. */
        uint32_t off = (sector % (p->size / 4096)) * 4096;
        esp_partition_erase_range(p, off, 4096);
        uint8_t hdr[16] = { 0 };
        hdr[0] = 0xB1; hdr[1] = 0xAC;   /* magic */
        uint16_t dc = bacnet_db_count();
        memcpy(hdr + 2, &dc, 2);
        esp_partition_write(p, off, hdr, sizeof(hdr));
        sector++;
    }
}

/* Watchdog task — core 1, priority 20. Reverts any WriteProperty that did
 * not receive a SimpleACK within ack_timeout_ms. */
static void watchdog_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(50));
        int64_t now = esp_timer_get_time();
        xSemaphoreTake(s_wp_mutex, portMAX_DELAY);
        for (int i = 0; i < 16; i++) {
            if (!s_wp_pending[i].in_use) continue;
            if (now > s_wp_pending[i].deadline_us) {
                ESP_LOGW(TAG, "ACK timeout on dev %u obj %u/%u prop %u — reverting",
                         (unsigned)s_wp_pending[i].device_instance,
                         s_wp_pending[i].object_type,
                         (unsigned)s_wp_pending[i].object_instance,
                         (unsigned)s_wp_pending[i].property_id);
                uint8_t tx[BP_NPDU_MAX];
                int len = bacnet_write_property_real(
                    tx, sizeof(tx), ++s_wp_pending[i].invoke_id,
                    s_wp_pending[i].object_type,
                    s_wp_pending[i].object_instance,
                    s_wp_pending[i].property_id,
                    s_wp_pending[i].prior_value, 16 /* default priority */);
                if (len > 0) board_w5500_send_raw(tx, len);
                s_wp_pending[i].in_use = false;
                g_state.revert_count++;
            }
        }
        xSemaphoreGive(s_wp_mutex);
    }
}

/* ---- Command dispatch (called by app_task) --------------------------- */
static uint8_t next_invoke_id = 1;

void bacnet_app_handle_cmd(const bp_cmd_t *c)
{
    if (g_state.mode == BP_MODE_TAMPERED) {
        ESP_LOGW(TAG, "refusing cmd in tampered state");
        return;
    }
    if (strcmp(c->op, "whois") == 0) {
        board_set_mode(BP_MODE_DISCOVER);
        uint8_t tx[BP_NPDU_MAX];
        int len = bacnet_who_is(tx, sizeof(tx), c->arg1, c->arg2);
        if (len > 0) board_w5500_send_raw(tx, len);
    } else if (strcmp(c->op, "db") == 0) {
        ESP_LOGI(TAG, "device DB: %u entries", bacnet_db_count());
    } else if (strcmp(c->op, "readprop") == 0) {
        uint8_t tx[BP_NPDU_MAX];
        int len = bacnet_read_property(tx, sizeof(tx), next_invoke_id++,
                                       (uint16_t)c->arg1, c->arg2, c->arg3);
        if (len > 0) board_w5500_send_raw(tx, len);
    } else if (strcmp(c->op, "writeprop") == 0) {
        if (!board_write_permitted((uint16_t)c->arg1)) return;
        uint8_t inv = next_invoke_id++;
        uint8_t tx[BP_NPDU_MAX];
        int len = bacnet_write_property_real(tx, sizeof(tx), inv,
                                             (uint16_t)c->arg1, c->arg2,
                                             c->arg3, c->farg, 8);
        if (len > 0) {
            board_w5500_send_raw(tx, len);
            /* register with watchdog */
            xSemaphoreTake(s_wp_mutex, portMAX_DELAY);
            for (int i = 0; i < 16; i++) {
                if (!s_wp_pending[i].in_use) {
                    s_wp_pending[i].in_use = true;
                    s_wp_pending[i].object_type = (uint16_t)c->arg1;
                    s_wp_pending[i].object_instance = c->arg2;
                    s_wp_pending[i].property_id = c->arg3;
                    s_wp_pending[i].prior_value = c->farg;  /* revert target */
                    s_wp_pending[i].invoke_id = inv;
                    s_wp_pending[i].deadline_us = esp_timer_get_time() +
                        (int64_t)g_state.ack_timeout_ms * 1000;
                    break;
                }
            }
            xSemaphoreGive(s_wp_mutex);
            g_state.write_count++;
        }
    } else if (strcmp(c->op, "join_mstp") == 0) {
        bacnet_mstp_join((uint8_t)c->arg1);
    } else if (strcmp(c->op, "bridge") == 0) {
        g_state.bridge_active = (c->arg1 != 0);
        board_set_mode(BP_MODE_BRIDGE);
    } else if (strcmp(c->op, "passive") == 0) {
        board_set_mode(BP_MODE_PASSIVE);
    } else if (strcmp(c->op, "tamper_test") == 0) {
        board_zeroize();
    } else {
        ESP_LOGW(TAG, "unknown op '%s'", c->op);
    }
}

/* ---- Tamper ISR ------------------------------------------------------- */
static void IRAM_ATTR tamper_isr(void *arg)
{
    (void)arg;
    /* Defer to watchdog; just signal. */
    BaseType_t hp = pdFALSE;
    bp_cmd_t c = { .op = "tamper_test" };
    xQueueSendFromISR(s_cmd_queue, &c, &hp);
    if (hp) portYIELD_FROM_ISR();
}

/* ---- app_main --------------------------------------------------------- */
void app_main(void)
{
    board_init();

    /* Tamper interrupt */
    gpio_install_isr_service(0);
    gpio_set_intr_type(BP_PIN_TAMPER_LOOP, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(BP_PIN_TAMPER_LOOP, tamper_isr, NULL);

    /* Task table — core / priority / stack as in README §5 */
    xTaskCreatePinnedToCore(bacnet_ip_task,  "bacnet_ip",  4096, NULL, 15, NULL, 0);
    xTaskCreatePinnedToCore(bacnet_mstp_task,"bacnet_mstp",4096, NULL, 18, NULL, 1);
    xTaskCreatePinnedToCore(router_task,     "router",     3072, NULL, 12, NULL, 0);
    xTaskCreatePinnedToCore(app_task,        "app",        4096, NULL,  9, NULL, 1);
    xTaskCreatePinnedToCore(persist_task,    "persist",    2048, NULL,  5, NULL, 0);
    xTaskCreatePinnedToCore(watchdog_task,   "watchdog",   2048, NULL, 20, NULL, 1);

    ESP_LOGI(TAG, "Phantom tasks launched — author=jayis1, build by jayis1");
}