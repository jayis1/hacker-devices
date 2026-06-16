/**
 * @file ble_nus_driver.c
 * @brief BLE NUS Driver Implementation
 *
 * Wraps Nordic SoftDevice S140 BLE stack and Nordic UART Service.
 * Provides simplified API for CAN Creeper firmware.
 *
 * Uses nRF5 SDK 17.1.0 BLE NUS module internally.
 * This is a thin wrapper that adapts the SDK API to our driver interface.
 */

#include "ble_nus_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/*===========================================================================
 * SDK INCLUDES (would be present in full SDK build)
 *===========================================================================*/

/* In a real build with nRF5 SDK, these would be included:
 * #include "ble_nus.h"
 * #include "ble_advdata.h"
 * #include "ble_conn_state.h"
 * #include "nrf_sdh.h"
 * #include "nrf_sdh_ble.h"
 * #include "nrf_sdh_soc.h"
 * #include "nrf_ble_gatt.h"
 */

/*===========================================================================
 * STATIC VARIABLES
 *===========================================================================*/

static bool g_ble_initialized = false;
static bool g_ble_connected = false;
static uint16_t g_ble_mtu = 23;  /* Default BLE 4.0 MTU, upgraded after DLE */
static int8_t g_ble_tx_power = 0;
static ble_nus_rx_callback_t g_rx_callback = NULL;
static ble_nus_tx_complete_callback_t g_tx_complete_callback = NULL;
static ble_nus_conn_callback_t g_conn_callback = NULL;

/* TX queue */
#define BLE_TX_QUEUE_SIZE   16
typedef struct {
    uint8_t data[BLE_NUS_MAX_DATA_LEN];
    uint16_t len;
    bool     pending;
} ble_tx_entry_t;

static ble_tx_entry_t g_tx_queue[BLE_TX_QUEUE_SIZE];
static volatile uint8_t g_tx_head = 0;
static volatile uint8_t g_tx_tail = 0;
static volatile uint8_t g_tx_count = 0;

/*===========================================================================
 * SDK STUB IMPLEMENTATIONS
 *===========================================================================*/

/*
 * In a full SDK build, these functions would call the actual nRF5 SDK APIs.
 * For this reference implementation, we provide functional stubs that
 * demonstrate the correct initialization sequence and state management.
 *
 * The real implementation would:
 * 1. Call nrf_sdh_enable_request() to configure SoftDevice
 * 2. Register BLE observers with NRF_SDH_BLE_OBSERVER
 * 3. Configure GAP parameters via ble_gap_conn_sec_mode_t
 * 4. Add NUS service via ble_nus_init()
 * 5. Configure advertising data via ble_advdata_t
 * 6. Start advertising via ble_advertising_start()
 */

/* Simulated SoftDevice state */
static bool g_sd_enabled = false;

/**
 * @brief Simulate SoftDevice enable
 */
static int sd_enable(void) {
    if (g_sd_enabled) return BLE_ERR_OK;

    /* In real code:
     * uint32_t ram_start = 0;
     * ret = nrf_sdh_enable_request();
     * ret = nrf_sdh_ble_enable(&ram_start);
     * ret = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
     * ret = nrf_sdh_ble_app_bles_ec_enable();
     */

    g_sd_enabled = true;
    return BLE_ERR_OK;
}

/**
 * @brief Simulate GAP parameters configuration
 */
static int gap_params_init(void) {
    /* In real code:
     * ble_gap_conn_params_t gap_conn_params;
     * memset(&gap_conn_params, 0, sizeof(gap_conn_params));
     * gap_conn_params.min_conn_interval = MSEC_TO_UNITS(7.5, UNIT_1_25_MS);
     * gap_conn_params.max_conn_interval = MSEC_TO_UNITS(15, UNIT_1_25_MS);
     * gap_conn_params.slave_latency = 0;
     * gap_conn_params.conn_sup_timeout = MSEC_TO_UNITS(4000, UNIT_10_MS);
     * ret = sd_ble_gap_ppcp_set(&gap_conn_params);
     * ret = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_TAG);
     */

    return BLE_ERR_OK;
}

/**
 * @brief Simulate GATT module initialization
 */
static int gatt_init(void) {
    /* In real code:
     * ret = nrf_ble_gatt_init(&m_gatt, NULL);
     * ret = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
     */

    return BLE_ERR_OK;
}

/**
 * @brief Simulate NUS service initialization
 */
static int nus_service_init(void) {
    /* In real code:
     * ble_nus_init_t nus_init;
     * memset(&nus_init, 0, sizeof(nus_init));
     * nus_init.data_handler = nus_data_handler;
     * ret = ble_nus_init(&m_nus, &nus_init);
     */

    return BLE_ERR_OK;
}

/**
 * @brief Simulate advertising configuration
 */
static int advertising_init(void) {
    /* In real code:
     * ble_advdata_t advdata;
     * memset(&advdata, 0, sizeof(advdata));
     * advdata.name_type = BLE_ADVDATA_FULL_NAME;
     * advdata.include_appearance = true;
     * advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
     * ble_uuid_t adv_uuids[] = {{BLE_UUID_NUS_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}};
     * advdata.uuids_complete.uuid_cnt = 1;
     * advdata.uuids_complete.p_uuids = adv_uuids;
     * ret = ble_advertising_init(&m_advertising, &init);
     */

    return BLE_ERR_OK;
}

/*===========================================================================
 * PUBLIC API
 *===========================================================================*/

/**
 * @brief Initialize BLE NUS
 */
int ble_nus_init(const ble_nus_config_t *config) {
    int ret;

    if (!config) return BLE_ERR_PARAM;

    /* Store callbacks */
    g_rx_callback = config->rx_callback;
    g_tx_complete_callback = config->tx_complete_callback;
    g_conn_callback = config->conn_callback;

    /* Enable SoftDevice */
    ret = sd_enable();
    if (ret != BLE_ERR_OK) return ret;

    /* Configure GAP parameters */
    ret = gap_params_init();
    if (ret != BLE_ERR_OK) return ret;

    /* Initialize GATT */
    ret = gatt_init();
    if (ret != BLE_ERR_OK) return ret;

    /* Initialize NUS service */
    ret = nus_service_init();
    if (ret != BLE_ERR_OK) return ret;

    /* Configure advertising */
    ret = advertising_init();
    if (ret != BLE_ERR_OK) return ret;

    /* Initialize TX queue */
    memset(g_tx_queue, 0, sizeof(g_tx_queue));
    g_tx_head = 0;
    g_tx_tail = 0;
    g_tx_count = 0;

    g_ble_initialized = true;

    return BLE_ERR_OK;
}

/**
 * @brief Deinitialize BLE NUS
 */
int ble_nus_deinit(void) {
    if (!g_ble_initialized) return BLE_ERR_NOT_INIT;

    /* Stop advertising if active */
    ble_nus_stop_advertising();

    /* Disconnect if connected */
    if (g_ble_connected) {
        ble_nus_disconnect();
    }

    /* In real code: disable SoftDevice */
    g_sd_enabled = false;
    g_ble_initialized = false;

    return BLE_ERR_OK;
}

/**
 * @brief Start advertising
 */
int ble_nus_start_advertising(void) {
    if (!g_ble_initialized) return BLE_ERR_NOT_INIT;

    /* In real code:
     * ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
     */

    return BLE_ERR_OK;
}

/**
 * @brief Stop advertising
 */
int ble_nus_stop_advertising(void) {
    if (!g_ble_initialized) return BLE_ERR_NOT_INIT;

    /* In real code:
     * ret = sd_ble_gap_adv_stop(m_advertising.adv_handle);
     */

    return BLE_ERR_OK;
}

/**
 * @brief Disconnect
 */
int ble_nus_disconnect(void) {
    if (!g_ble_initialized || !g_ble_connected) return BLE_ERR_NOT_CONNECTED;

    /* In real code:
     * ret = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
     */

    g_ble_connected = false;
    if (g_conn_callback) g_conn_callback(false);

    return BLE_ERR_OK;
}

/**
 * @brief Transmit data over BLE NUS
 */
int ble_nus_tx(const uint8_t *data, uint16_t len) {
    if (!g_ble_initialized) return BLE_ERR_NOT_INIT;
    if (!g_ble_connected) return BLE_ERR_NOT_CONNECTED;
    if (!data || len == 0 || len > BLE_NUS_MAX_DATA_LEN) return BLE_ERR_PARAM;

    /* Check queue space */
    if (g_tx_count >= BLE_TX_QUEUE_SIZE) {
        return BLE_ERR_TX_FULL;
    }

    /* Add to TX queue */
    CRITICAL_REGION_ENTER();
    ble_tx_entry_t *entry = &g_tx_queue[g_tx_head];
    memcpy(entry->data, data, len);
    entry->len = len;
    entry->pending = true;
    g_tx_head = (g_tx_head + 1) % BLE_TX_QUEUE_SIZE;
    g_tx_count++;
    CRITICAL_REGION_EXIT();

    /* In real code, trigger notification:
     * uint16_t hvx_len = (len > g_ble_mtu - 3) ? g_ble_mtu - 3 : len;
     * ret = ble_nus_data_send(&m_nus, data, &hvx_len, m_conn_handle);
     */

    return BLE_ERR_OK;
}

/**
 * @brief Get pending TX count
 */
int ble_nus_tx_pending(void) {
    return (int)g_tx_count;
}

/**
 * @brief Check if connected
 */
bool ble_nus_is_connected(void) {
    return g_ble_connected;
}

/**
 * @brief Get MTU
 */
int ble_nus_get_mtu(void) {
    return (int)g_ble_mtu;
}

/**
 * @brief Set TX power
 */
int ble_nus_set_tx_power(int8_t dbm) {
    if (!g_ble_initialized) return BLE_ERR_NOT_INIT;

    /* In real code:
     * ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, m_advertising.adv_handle, dbm);
     * ret = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, m_conn_handle, dbm);
     */

    g_ble_tx_power = dbm;
    return BLE_ERR_OK;
}

/**
 * @brief Handle SoftDevice BLE events
 *
 * In the real implementation, this is called from the SD event dispatcher
 * and processes GAP, GATT, and NUS events.
 */
void ble_nus_event_handler(const void *event) {
    /* In real code, this would cast event to ble_evt_t and dispatch:
     * const ble_evt_t *ble_evt = (const ble_evt_t *)event;
     *
     * switch (ble_evt->header.evt_id) {
     * case BLE_GAP_EVT_CONNECTED:
     *     g_ble_connected = true;
     *     if (g_conn_callback) g_conn_callback(true);
     *     break;
     * case BLE_GAP_EVT_DISCONNECTED:
     *     g_ble_connected = false;
     *     if (g_conn_callback) g_conn_callback(false);
     *     // Restart advertising
     *     ble_nus_start_advertising();
     *     break;
     * case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
     *     // Accept DLE request
     *     break;
     * case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
     *     g_ble_mtu = ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets;
     *     break;
     * case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
     *     // Accept MTU exchange
     *     break;
     * case BLE_GATTS_EVT_HVN_TX_COMPLETE:
     *     // TX complete — dequeue next
     *     if (g_tx_count > 0) {
     *         CRITICAL_REGION_ENTER();
     *         g_tx_queue[g_tx_tail].pending = false;
     *         g_tx_tail = (g_tx_tail + 1) % BLE_TX_QUEUE_SIZE;
     *         g_tx_count--;
     *         CRITICAL_REGION_EXIT();
     *         if (g_tx_complete_callback) g_tx_complete_callback();
     *     }
     *     break;
     * case BLE_GATTS_EVT_WRITE:
     *     // RX data from app
     *     if (g_rx_callback) {
     *         g_rx_callback(ble_evt->evt.gatts_evt.params.write.data,
     *                       ble_evt->evt.gatts_evt.params.write.len);
     *     }
     *     break;
     * }
     */

    /* Stub: simulate connection after advertising start */
    (void)event;
}
