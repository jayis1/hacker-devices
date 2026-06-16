//==============================================================================
// sdcard.c — Spectre-Sniffer SD Card Driver
// Author: jayis1
// Description: FAT32 filesystem management for microSD card storage.
//              Handles capture file I/O, directory management, and
//              configuration persistence.
//==============================================================================

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "board.h"

static const char *TAG = "SDCARD";

//==============================================================================
// SD Card State
//==============================================================================
typedef struct {
    bool            mounted;
    bool            card_present;
    uint64_t        total_bytes;
    uint64_t        free_bytes;
    sdmmc_card_t   *card;
} sdcard_state_t;

static sdcard_state_t s_sd = {0};

//==============================================================================
// Initialize SD Card
//==============================================================================
int sdcard_init_driver(void) {
    ESP_LOGI(TAG, "Initializing SD card...");

    // Check if card is present
    if (!gpio_get_level(SDCARD_DETECT_GPIO)) {
        ESP_LOGW(TAG, "No SD card detected");
        s_sd.card_present = false;
        return SPECTRE_ERR_SD_CARD;
    }
    s_sd.card_present = true;

    // Mount FAT32 filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SDCARD_SPI_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = FPGA_SPI_MOSI_GPIO,
        .miso_io_num = FPGA_SPI_MISO_GPIO,
        .sclk_io_num = FPGA_SPI_CLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus for SD card: %d", ret);
        return SPECTRE_ERR_IO;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SDCARD_CS_GPIO;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(SDCARD_MOUNT_POINT, &host, &slot_config,
                                   &mount_config, &s_sd.card);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "SD card not found");
            return SPECTRE_ERR_SD_CARD;
        }
        ESP_LOGE(TAG, "Failed to mount SD card: %d", ret);
        return SPECTRE_ERR_SD_CARD;
    }

    s_sd.mounted = true;

    // Get card info
    sdmmc_card_print_info(stdout, s_sd.card);
    s_sd.total_bytes = s_sd.card->csd.capacity;
    s_sd.free_bytes = s_sd.total_bytes;  // Approximate

    // Create required directories
    mkdir(CAPTURE_DIR, 0755);
    mkdir(TEMPEST_DIR, 0755);
    mkdir(TRACES_DIR, 0755);

    ESP_LOGI(TAG, "SD card mounted at %s", SDCARD_MOUNT_POINT);
    return SPECTRE_OK;
}

//==============================================================================
// File Operations
//==============================================================================
int sdcard_write_capture(const char *filename, const int16_t *data,
                          uint32_t num_samples, uint32_t sample_rate_hz,
                          uint64_t center_freq_hz) {
    if (!s_sd.mounted) return SPECTRE_ERR_NOT_INIT;
    if (!filename || !data) return SPECTRE_ERR_INVALID_PARAM;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", CAPTURE_DIR, filename);

    FILE *f = fopen(full_path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", full_path);
        return SPECTRE_ERR_FILE;
    }

    // Write header
    uint32_t header[4] = {
        num_samples,
        sample_rate_hz,
        (uint32_t)(center_freq_hz & 0xFFFFFFFF),
        (uint32_t)((center_freq_hz >> 32) & 0xFFFFFFFF),
    };
    fwrite(header, sizeof(header), 1, f);

    // Write sample data
    fwrite(data, sizeof(int16_t), num_samples, f);

    fclose(f);
    ESP_LOGI(TAG, "Wrote %u samples to %s", num_samples, full_path);
    return SPECTRE_OK;
}

int sdcard_read_capture(const char *filename, int16_t *buffer,
                         uint32_t max_samples) {
    if (!s_sd.mounted) return SPECTRE_ERR_NOT_INIT;
    if (!filename || !buffer) return SPECTRE_ERR_INVALID_PARAM;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", CAPTURE_DIR, filename);

    FILE *f = fopen(full_path, "rb");
    if (!f) return SPECTRE_ERR_FILE;

    // Read header
    uint32_t header[4];
    fread(header, sizeof(header), 1, f);

    uint32_t num_samples = header[0];
    if (num_samples > max_samples) num_samples = max_samples;

    // Read sample data
    uint32_t read = fread(buffer, sizeof(int16_t), num_samples, f);
    fclose(f);

    return (int)read;
}

//==============================================================================
// Directory Operations
//==============================================================================
int sdcard_list_files(const char *dir, char (*filenames)[64], uint32_t max_files) {
    if (!s_sd.mounted) return SPECTRE_ERR_NOT_INIT;
    if (!filenames) return SPECTRE_ERR_INVALID_PARAM;

    DIR *d = opendir(dir);
    if (!d) return SPECTRE_ERR_FILE;

    struct dirent *entry;
    uint32_t count = 0;

    while ((entry = readdir(d)) != NULL && count < max_files) {
        if (entry->d_type == DT_REG) {
            strncpy(filenames[count], entry->d_name, 63);
            filenames[count][63] = '\0';
            count++;
        }
    }

    closedir(d);
    return (int)count;
}

//==============================================================================
// Configuration Persistence
//==============================================================================
int sdcard_save_config(const char *json_config) {
    if (!s_sd.mounted) return SPECTRE_ERR_NOT_INIT;
    if (!json_config) return SPECTRE_ERR_INVALID_PARAM;

    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) return SPECTRE_ERR_FILE;

    fprintf(f, "%s\n", json_config);
    fclose(f);

    ESP_LOGI(TAG, "Configuration saved to %s", CONFIG_FILE);
    return SPECTRE_OK;
}

int sdcard_load_config(char *buffer, uint32_t buffer_size) {
    if (!s_sd.mounted) return SPECTRE_ERR_NOT_INIT;
    if (!buffer || buffer_size == 0) return SPECTRE_ERR_INVALID_PARAM;

    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) return SPECTRE_ERR_FILE;

    size_t read = fread(buffer, 1, buffer_size - 1, f);
    buffer[read] = '\0';
    fclose(f);

    return SPECTRE_OK;
}

//==============================================================================
// Status Queries
//==============================================================================
bool sdcard_is_mounted(void) {
    return s_sd.mounted;
}

bool sdcard_is_present(void) {
    return s_sd.card_present;
}

uint64_t sdcard_get_free_space(void) {
    if (!s_sd.mounted) return 0;
    FATFS *fs;
    DWORD free_clusters;
    if (f_getfree("0:", &free_clusters, &fs) == FR_OK) {
        return (uint64_t)free_clusters * fs->csize * 512;
    }
    return s_sd.free_bytes;
}

uint64_t sdcard_get_total_space(void) {
    return s_sd.total_bytes;
}
