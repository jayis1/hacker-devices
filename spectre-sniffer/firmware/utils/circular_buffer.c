//==============================================================================
// circular_buffer.c — Lock-Free Circular Buffer for ADC Data
// Author: jayis1
// Description: Thread-safe, lock-free circular buffer for high-speed ADC
//              data transfer between FPGA read and SD card write tasks.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "board.h"

static const char *TAG = "CIRCBUF";

//==============================================================================
// Circular Buffer Structure
//==============================================================================
typedef struct {
    int16_t         *buffer;
    uint32_t         size;           // Must be power of 2
    uint32_t         mask;           // size - 1
    volatile uint32_t write_index;   // Next write position
    volatile uint32_t read_index;    // Next read position
    volatile uint32_t overrun_count; // Number of times writer overtook reader
    bool             initialized;
} circular_buffer_t;

static circular_buffer_t s_cbuf = {0};

//==============================================================================
// Initialize Circular Buffer
//==============================================================================
int circular_buffer_init(uint32_t size) {
    // Size must be power of 2
    if (size < 64 || (size & (size - 1)) != 0) {
        ESP_LOGE(TAG, "Buffer size must be power of 2 and >= 64");
        return SPECTRE_ERR_INVALID_PARAM;
    }

    s_cbuf.buffer = (int16_t *)heap_caps_malloc(
        size * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!s_cbuf.buffer) {
        ESP_LOGE(TAG, "Failed to allocate circular buffer (%u samples)", size);
        return SPECTRE_ERR_NOMEM;
    }

    s_cbuf.size = size;
    s_cbuf.mask = size - 1;
    s_cbuf.write_index = 0;
    s_cbuf.read_index = 0;
    s_cbuf.overrun_count = 0;
    s_cbuf.initialized = true;

    ESP_LOGI(TAG, "Circular buffer initialized: %u samples (%u KB)",
             size, (size * sizeof(int16_t)) / 1024);
    return SPECTRE_OK;
}

//==============================================================================
// Write a Single Sample (Producer)
//==============================================================================
bool circular_buffer_write(int16_t sample) {
    if (!s_cbuf.initialized) return false;

    uint32_t next_write = (s_cbuf.write_index + 1) & s_cbuf.mask;

    // Check if buffer is full (write would overtake read)
    if (next_write == s_cbuf.read_index) {
        // Buffer full - increment overrun counter and overwrite oldest
        s_cbuf.overrun_count++;
        s_cbuf.read_index = (s_cbuf.read_index + 1) & s_cbuf.mask;
    }

    s_cbuf.buffer[s_cbuf.write_index] = sample;
    // Memory barrier to ensure sample is written before updating index
    __sync_synchronize();
    s_cbuf.write_index = next_write;

    return true;
}

//==============================================================================
// Write Multiple Samples (Bulk Producer)
//==============================================================================
uint32_t circular_buffer_write_bulk(const int16_t *samples, uint32_t count) {
    if (!s_cbuf.initialized || !samples || count == 0) return 0;

    uint32_t written = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (circular_buffer_write(samples[i])) {
            written++;
        }
    }

    return written;
}

//==============================================================================
// Read a Single Sample (Consumer)
//==============================================================================
bool circular_buffer_read(int16_t *sample) {
    if (!s_cbuf.initialized || !sample) return false;

    if (s_cbuf.read_index == s_cbuf.write_index) {
        // Buffer empty
        return false;
    }

    *sample = s_cbuf.buffer[s_cbuf.read_index];
    // Memory barrier
    __sync_synchronize();
    s_cbuf.read_index = (s_cbuf.read_index + 1) & s_cbuf.mask;

    return true;
}

//==============================================================================
// Read Multiple Samples (Bulk Consumer)
//==============================================================================
uint32_t circular_buffer_read_bulk(int16_t *buffer, uint32_t max_count) {
    if (!s_cbuf.initialized || !buffer || max_count == 0) return 0;

    uint32_t read = 0;

    while (read < max_count && circular_buffer_read(&buffer[read])) {
        read++;
    }

    return read;
}

//==============================================================================
// Status Queries
//==============================================================================
uint32_t circular_buffer_available(void) {
    if (!s_cbuf.initialized) return 0;
    return (s_cbuf.write_index - s_cbuf.read_index) & s_cbuf.mask;
}

uint32_t circular_buffer_free(void) {
    if (!s_cbuf.initialized) return 0;
    return s_cbuf.size - circular_buffer_available() - 1;
}

bool circular_buffer_is_empty(void) {
    return s_cbuf.read_index == s_cbuf.write_index;
}

bool circular_buffer_is_full(void) {
    return ((s_cbuf.write_index + 1) & s_cbuf.mask) == s_cbuf.read_index;
}

uint32_t circular_buffer_get_overrun_count(void) {
    return s_cbuf.overrun_count;
}

uint32_t circular_buffer_get_size(void) {
    return s_cbuf.size;
}

//==============================================================================
// Reset Buffer
//==============================================================================
void circular_buffer_reset(void) {
    s_cbuf.write_index = 0;
    s_cbuf.read_index = 0;
    s_cbuf.overrun_count = 0;
    ESP_LOGI(TAG, "Circular buffer reset");
}

//==============================================================================
// Cleanup
//==============================================================================
void circular_buffer_deinit(void) {
    if (s_cbuf.buffer) {
        heap_caps_free(s_cbuf.buffer);
        s_cbuf.buffer = NULL;
    }
    s_cbuf.initialized = false;
    ESP_LOGI(TAG, "Circular buffer deinitialized");
}
