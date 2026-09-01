/*
 * BootROM Banshee overlay store
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <string.h>
#include "overlay_store.h"

static bb_overlay_t g_overlays[BB_MAX_OVERLAYS];
static uint32_t g_overlay_count;

static void set_bytes(bb_overlay_t *overlay, const uint8_t *bytes, uint32_t length) {
    overlay->length = length;
    for (uint32_t i = 0; i < length && i < sizeof(overlay->bytes); ++i) {
        overlay->bytes[i] = bytes[i];
    }
}

void overlay_store_init(void) {
    static const uint8_t rollback_shadow[] = {
        0x42, 0x42, 0x52, 0x4F, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x05, 0xDE, 0xAD, 0xBE, 0xEF
    };
    static const uint8_t header_ghost[] = {
        0x47, 0x48, 0x4F, 0x53, 0x54, 0x48, 0x44, 0x52,
        0x00, 0x02, 0x00, 0x00, 0x5A, 0xA5, 0x5A, 0xA5
    };
    static const uint8_t manifest_delta[] = {
        0x4D, 0x41, 0x4E, 0x49, 0x46, 0x45, 0x53, 0x54,
        0x90, 0x00, 0x00, 0x00, 0x52, 0x42, 0x4B, 0x21
    };
    static const uint8_t safe_transparent[] = {
        0x54, 0x52, 0x41, 0x4E, 0x53, 0x50, 0x41, 0x52
    };

    memset(g_overlays, 0, sizeof(g_overlays));
    g_overlay_count = 4u;

    strcpy(g_overlays[0].name, "transparent");
    g_overlays[0].base_addr = 0u;
    g_overlays[0].enabled = true;
    set_bytes(&g_overlays[0], safe_transparent, sizeof(safe_transparent));

    strcpy(g_overlays[1].name, "rollback-shadow");
    g_overlays[1].base_addr = 0x00001000u;
    g_overlays[1].enabled = true;
    set_bytes(&g_overlays[1], rollback_shadow, sizeof(rollback_shadow));

    strcpy(g_overlays[2].name, "header-ghost");
    g_overlays[2].base_addr = 0x00000000u;
    g_overlays[2].enabled = true;
    set_bytes(&g_overlays[2], header_ghost, sizeof(header_ghost));

    strcpy(g_overlays[3].name, "manifest-delta");
    g_overlays[3].base_addr = 0x00002000u;
    g_overlays[3].enabled = true;
    set_bytes(&g_overlays[3], manifest_delta, sizeof(manifest_delta));
}

const bb_overlay_t *overlay_store_get_by_name(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    for (uint32_t i = 0; i < g_overlay_count; ++i) {
        if (strcmp(g_overlays[i].name, name) == 0) {
            return &g_overlays[i];
        }
    }
    return NULL;
}

const bb_overlay_t *overlay_store_get_by_index(uint32_t index) {
    if (index >= g_overlay_count) {
        return NULL;
    }
    return &g_overlays[index];
}

uint32_t overlay_store_count(void) {
    return g_overlay_count;
}

bool overlay_store_apply(const char *name, bb_spi_frame_t *frame) {
    const bb_overlay_t *overlay = overlay_store_get_by_name(name);
    if (overlay == NULL || frame == NULL || !overlay->enabled) {
        return false;
    }
    if (strcmp(overlay->name, "transparent") == 0) {
        return false;
    }
    if (frame->addr != overlay->base_addr) {
        return false;
    }

    uint32_t consumed = 0u;
    for (uint32_t i = 0; i < BB_MAX_CAPTURE_WORDS && consumed < overlay->length; ++i) {
        uint32_t word = 0u;
        for (uint32_t b = 0; b < 4u && consumed < overlay->length; ++b) {
            word |= ((uint32_t)overlay->bytes[consumed]) << (8u * b);
            consumed++;
        }
        frame->data[i] = word;
    }
    if (frame->data_len < overlay->length) {
        frame->data_len = (uint8_t)overlay->length;
    }
    return true;
}
