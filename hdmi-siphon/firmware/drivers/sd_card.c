/**
 * @file    sd_card.c
 * @brief   microSD card driver for frame storage and configuration
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Manages the microSD card via SPI. Provides FAT32 filesystem access
 * for storing captured frames (JPEG), EDID dumps, CEC logs, and
 * configuration files. Handles mount/unmount, directory creation,
 * and file write in a manner tolerant of sudden power loss.
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "board.h"
#include "sd_card.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "SD_CARD";

/** Maximum path length */
#define PATH_MAX_LEN                    256U

/** Maximum filename length */
#define FILENAME_MAX_LEN                 64U

/** Card detect retries */
#define CARD_DETECT_RETRIES                3U

/** Mount retry delay (ms) */
#define MOUNT_RETRY_DELAY_MS            1000U

/* =========================================================================
 * Local State
 * ========================================================================= */

/** Current mount state */
static bool s_mounted = false;

/** SDMMC card info */
static sdmmc_card_t *s_card = NULL;

/** Total space in bytes */
static uint64_t s_total_bytes = 0;

/** Free space in bytes */
static uint64_t s_free_bytes = 0;

/* =========================================================================
 * Initialization
 * ========================================================================= */

esp_err_t sd_card_init(void)
{
    ESP_LOGI(TAG, "Initializing microSD card");

    /* Configure SPI bus for SD card */
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs   = PIN_SD_CS;
    slot_config.host_id   = SPI2_HOST;
    slot_config.gpio_miso = PIN_SD_MISO;
    slot_config.gpio_mosi = PIN_SD_MOSI;
    slot_config.gpio_sck  = PIN_SD_SCLK;

    /* Configure SPI bus */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_SD_MOSI,
        .miso_io_num     = PIN_SD_MISO,
        .sclk_io_num     = PIN_SD_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus for SD: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Mount FAT32 filesystem */
    esp_vfs_fat_sdspi_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 8,
        .allocation_unit_size   = 16 * 1024
    };

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Running without SD card — captures will be stored in RAM only");
        s_mounted = false;
        return ret;
    }

    s_mounted = true;

    /* Get card info */
    sdmmc_card_print_info(stdout, s_card);
    s_total_bytes = (uint64_t)s_card->csd.capacity * s_card->csd.sector_size;
    s_free_bytes  = s_total_bytes; /* Approximate until statfs */

    /* Create required directories */
    mkdir(CAPTURE_DIR, 0755);
    mkdir(COVERT_DIR, 0755);
    mkdir(SD_MOUNT_POINT "/cec_logs", 0755);
    mkdir(SD_MOUNT_POINT "/edid_dumps", 0755);
    mkdir(SD_MOUNT_POINT "/config", 0755);

    ESP_LOGI(TAG, "SD card mounted at %s (total: %llu MB, free: %llu MB)",
             SD_MOUNT_POINT,
             s_total_bytes / (1024 * 1024),
             s_free_bytes / (1024 * 1024));

    return ESP_OK;
}

/* =========================================================================
 * File Operations
 * ========================================================================= */

esp_err_t sd_card_write_frame(const char *filename, const uint8_t *data, size_t len)
{
    if (!s_mounted) {
        ESP_LOGW(TAG, "SD not mounted, cannot write frame");
        return ESP_ERR_INVALID_STATE;
    }

    char full_path[PATH_MAX_LEN];
    int written = snprintf(full_path, sizeof(full_path), "%s/%s", CAPTURE_DIR, filename);
    if (written >= (int)sizeof(full_path)) {
        ESP_LOGE(TAG, "Path too long: %s/%s", CAPTURE_DIR, filename);
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(full_path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s for writing", full_path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t written_bytes = fwrite(data, 1, len, f);
    if (written_bytes != len) {
        ESP_LOGE(TAG, "Short write: %zu of %zu bytes", written_bytes, len);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    fclose(f);
    ESP_LOGD(TAG, "Wrote %zu bytes to %s", len, full_path);
    return ESP_OK;
}

esp_err_t sd_card_write_log(const char *filename, const char *data, size_t len)
{
    if (!s_mounted) return ESP_ERR_INVALID_STATE;

    char full_path[PATH_MAX_LEN];
    snprintf(full_path, sizeof(full_path), "%s/cec_logs/%s", SD_MOUNT_POINT, filename);

    FILE *f = fopen(full_path, "a");  /* append mode */
    if (f == NULL) return ESP_ERR_NOT_FOUND;

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    return (written == len) ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t sd_card_write_edid(const uint8_t *edid_data, size_t len)
{
    if (!s_mounted || edid_data == NULL) return ESP_ERR_INVALID_STATE;

    char filename[FILENAME_MAX_LEN];

    /* Generate timestamped filename */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(filename, sizeof(filename), "edid_%Y%m%d_%H%M%S.bin", tm_info);

    char full_path[PATH_MAX_LEN];
    snprintf(full_path, sizeof(full_path), "%s/edid_dumps/%s", SD_MOUNT_POINT, filename);

    FILE *f = fopen(full_path, "wb");
    if (f == NULL) return ESP_ERR_NOT_FOUND;

    size_t written = fwrite(edid_data, 1, len, f);
    fclose(f);

    ESP_LOGI(TAG, "EDID dumped to %s (%zu bytes)", full_path, written);
    return (written == len) ? ESP_OK : ESP_ERR_NO_MEM;
}

/* =========================================================================
 * Space Management
 * ========================================================================= */

uint64_t sd_card_get_free_space(void)
{
    if (!s_mounted || s_card == NULL) return 0;

    FATFS *fs;
    DWORD free_clusters;

    f_getfree("0:", &free_clusters, &fs);
    s_free_bytes = (uint64_t)free_clusters * s_card->csd.sector_size * 2; /* approximation */

    return s_free_bytes;
}

uint64_t sd_card_get_total_space(void)
{
    return s_total_bytes;
}

bool sd_card_has_space_for(size_t required_bytes)
{
    return sd_card_get_free_space() >= required_bytes;
}

/* =========================================================================
 * Directory Listing
 * ========================================================================= */

int sd_card_list_captures(sd_card_file_info_t *files, int max_files)
{
    if (!s_mounted || files == NULL || max_files <= 0) return -1;

    DIR *dir = opendir(CAPTURE_DIR);
    if (dir == NULL) {
        ESP_LOGW(TAG, "Cannot open capture directory");
        return -1;
    }

    int count = 0;
    struct dirent *entry;
    struct stat st;

    while ((entry = readdir(dir)) != NULL && count < max_files) {
        char full_path[PATH_MAX_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", CAPTURE_DIR, entry->d_name);

        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            strncpy(files[count].name, entry->d_name, sizeof(files[count].name) - 1);
            files[count].size = st.st_size;
            files[count].mod_time = st.st_mtime;
            count++;
        }
    }

    closedir(dir);
    return count;
}

/* =========================================================================
 * State
 * ========================================================================= */

bool sd_card_is_mounted(void)
{
    return s_mounted;
}

esp_err_t sd_card_unmount(void)
{
    if (!s_mounted) return ESP_OK;

    /* Flush any pending writes */
    sync();

    esp_err_t ret = esp_vfs_fat_sdmmc_unmount();
    if (ret == ESP_OK) {
        s_mounted = false;
        s_card = NULL;
        ESP_LOGI(TAG, "SD card unmounted");
    } else {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
    }

    return ret;
}
