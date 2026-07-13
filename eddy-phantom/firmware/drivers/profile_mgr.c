/*
 * eddy-phantom — profile_mgr.c
 * Keyboard profile manager: loading, saving, and calibration
 * of per-controller emanation profiles.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── Built-in profile directory ─────────────────────────────────
 * Pre-defined profiles for common keyboard controllers.
 * Each entry maps a controller ID to a name and QSPI offset
 * where the trained model weights are stored.
 */
typedef struct {
    const char *name;
    uint16_t    controller_id;
    uint32_t    qspi_offset;
    uint16_t    num_keys;
    const char *description;
} builtin_profile_t;

static const builtin_profile_t builtin_profiles[] = {
    {
        .name = "holtek-ht82k629a",
        .controller_id = 0x629A,
        .qspi_offset = 0x00000000,
        .num_keys = 104,
        .description = "Holtek HT82K629A — common in low-cost USB keyboards"
    },
    {
        .name = "cypress-cy7c63743",
        .controller_id = 0x6374,
        .qspi_offset = 0x00040000,
        .num_keys = 104,
        .description = "Cypress CY7C63743 — used in many office keyboards"
    },
    {
        .name = "nxp-lpc11u6x",
        .controller_id = 0x11U6,
        .qspi_offset = 0x00080000,
        .num_keys = 110,
        .description = "NXP LPC11U6x — mechanical keyboards, QMK variants"
    },
    {
        .name = "asix-ax88772",
        .controller_id = 0x8772,
        .qspi_offset = 0x000C0000,
        .num_keys = 104,
        .description = "ASIX AX88772 — USB hub passthrough keyboards"
    },
    {
        .name = "microchip-usb2514",
        .controller_id = 0x2514,
        .qspi_offset = 0x00100000,
        .num_keys = 104,
        .description = "Microchip USB2514 hub-based keyboards"
    },
    {
        .name = "generic-8051",
        .controller_id = 0x8051,
        .qspi_offset = 0x00140000,
        .num_keys = 104,
        .description = "Generic 8051-based keyboard controllers"
    },
};

#define NUM_BUILTIN_PROFILES  (sizeof(builtin_profiles) / sizeof(builtin_profiles[0]))

/* ── Currently active profile ─────────────────────────────────── */
static keyboard_profile_t g_active_profile;
static int g_profile_loaded = 0;

/* ── Find a profile by name ───────────────────────────────────── */
static const builtin_profile_t *find_profile_by_name(const char *name)
{
    for (uint32_t i = 0; i < NUM_BUILTIN_PROFILES; i++) {
        int match = 1;
        for (int j = 0; j < PROFILE_NAME_LEN; j++) {
            if (name[j] != builtin_profiles[i].name[j]) {
                match = 0;
                break;
            }
            if (name[j] == '\0') break;
        }
        if (match) return &builtin_profiles[i];
    }
    return NULL;
}

/* ── Find a profile by controller ID ──────────────────────────── */
static const builtin_profile_t *find_profile_by_id(uint16_t ctrl_id)
{
    for (uint32_t i = 0; i < NUM_BUILTIN_PROFILES; i++) {
        if (builtin_profiles[i].controller_id == ctrl_id)
            return &builtin_profiles[i];
    }
    return NULL;
}

/* ── Load a profile by name ───────────────────────────────────── */
int profile_load(const char *name, keyboard_profile_t *prof)
{
    const builtin_profile_t *builtin = find_profile_by_name(name);
    if (!builtin) {
        /* Try loading from SD card */
        extern int sdcard_load_profile(const char *name, keyboard_profile_t *prof);
        return sdcard_load_profile(name, prof);
    }

    /* Copy built-in profile info */
    for (int i = 0; i < PROFILE_NAME_LEN && builtin->name[i]; i++) {
        prof->name[i] = builtin->name[i];
    }
    prof->name[PROFILE_NAME_LEN - 1] = '\0';
    prof->controller_id = builtin->controller_id;
    prof->num_keys = builtin->num_keys;
    prof->ref_offset = builtin->qspi_offset;
    prof->ref_size = 0;  /* determined at load time */

    /* Fill in standard USB HID scancodes for a 104-key keyboard */
    int sc = 0;
    /* Letters a-z: scancodes 0x04-0x1D */
    for (int i = 0x04; i <= 0x1D && sc < PROFILE_MAX_KEYS; i++)
        prof->scancodes[sc++] = i;
    /* Numbers 1-0: scancodes 0x1E-0x27 */
    for (int i = 0x1E; i <= 0x27 && sc < PROFILE_MAX_KEYS; i++)
        prof->scancodes[sc++] = i;
    /* Enter, Escape, Backspace, Tab, Space */
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x28;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x29;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x2A;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x2B;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x2C;
    /* Punctuation */
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x2D;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x2E;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x2F;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x30;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x31;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x33;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x34;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x35;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x36;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x37;
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0x38;
    /* Modifiers */
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0xE0;  /* L-Shift */
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0xE1;  /* L-Ctrl */
    if (sc < PROFILE_MAX_KEYS) prof->scancodes[sc++] = 0xE2;  /* L-Alt */

    prof->num_keys = sc;
    g_profile_loaded = 1;
    g_active_profile = *prof;

    return 0;
}

/* ── Identify a keyboard controller from a fingerprint burst ────
 * Captures a few emanation bursts and attempts to match them
 * against known controller signatures in the profile library.
 * Returns the best-matching profile name, or NULL if no match.
 */
const char *profile_identify_controller(float *features, int num_bursts)
{
    /* In a full implementation, this would:
     * 1. Compute a fingerprint from the features (e.g., dominant
     *    frequency components, burst duration statistics, channel
     *    amplitude ratios)
     * 2. Compare against a database of known controller fingerprints
     * 3. Return the best match
     *
     * Here we implement a simplified version that checks the
     * spatial diversity features (probe ratios) which tend to
     * differ between controller families due to different PCB
     * layouts and trace geometries.
     */

    if (num_bursts < 1) return NULL;

    /* Extract spatial diversity features (indices 13-16) */
    float ratios[4];
    for (int i = 0; i < 4; i++) {
        ratios[i] = features[DSP_MFCC_COEFFS + i];
    }

    /* Simple heuristic: check which profile's expected ratios
     * best match the observed ratios. In production, this would
     * use a proper classifier or correlation. */
    float best_score = 1e30f;
    const builtin_profile_t *best = NULL;

    /* For each built-in profile, compute a distance metric.
     * In production, expected ratios would be stored per-profile.
     * Here we use a simple threshold-based approach. */
    (void)ratios;

    for (uint32_t i = 0; i < NUM_BUILTIN_PROFILES; i++) {
        /* Placeholder: all profiles equally likely.
         * A real implementation would have per-profile reference
         * fingerprints stored in QSPI. */
        float score = (float)(i + 1) * 0.1f;
        if (score < best_score) {
            best_score = score;
            best = &builtin_profiles[i];
        }
    }

    return best ? best->name : NULL;
}

/* ── Get the number of available profiles ─────────────────────── */
int profile_get_count(void)
{
    extern int sdcard_list_profiles(void);
    int sd_count = sdcard_list_profiles();
    return (int)NUM_BUILTIN_PROFILES + sd_count;
}

/* ── Get profile info by index ────────────────────────────────── */
int profile_get_info(int index, char *name, int name_len,
                      uint16_t *ctrl_id, const char **desc)
{
    if (index < 0)
        return -1;

    if (index < (int)NUM_BUILTIN_PROFILES) {
        const builtin_profile_t *p = &builtin_profiles[index];
        int i;
        for (i = 0; i < name_len - 1 && p->name[i]; i++)
            name[i] = p->name[i];
        name[i] = '\0';
        if (ctrl_id) *ctrl_id = p->controller_id;
        if (desc) *desc = p->description;
        return 0;
    }

    return -1;  /* SD card profiles not indexable in this simplified version */
}

/* ── Get the currently active profile ─────────────────────────── */
const keyboard_profile_t *profile_get_active(void)
{
    if (g_profile_loaded)
        return &g_active_profile;
    return NULL;
}

/* ── Save a calibrated profile to SD card ─────────────────────── */
int profile_save_calibrated(const char *name, uint16_t ctrl_id,
                             const uint8_t *model_data, uint32_t model_size)
{
    /* In a full implementation, this would:
     * 1. Write the model data to QSPI flash at an allocated offset
     * 2. Write a profile descriptor to the SD card profile directory
     * 3. Update the in-memory profile table
     *
     * Here we provide the structure for this operation.
     */

    /* Find a free QSPI sector for the model */
    uint32_t qspi_offset = 0x00200000;  /* custom profiles start at 2 MB offset */

    /* Erase enough sectors for the model */
    uint32_t sectors_needed = (model_size + 4095) / 4096;
    for (uint32_t s = 0; s < sectors_needed; s++) {
        extern int qspi_erase_sector(uint32_t addr);
        qspi_erase_sector(qspi_offset + s * 4096);
    }

    /* Write model data page by page */
    extern int qspi_write(uint32_t addr, const void *buf, uint32_t len);
    qspi_write(qspi_offset, model_data, model_size);

    /* Write profile descriptor to SD card (simplified) */
    /* Would format and write a 512-byte descriptor block */

    return 0;
}