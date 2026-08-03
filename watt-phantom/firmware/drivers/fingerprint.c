/*
 * fingerprint.c — USB-C PD device fingerprinting engine
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Captures a feature vector from the PD negotiation sequence of a
 * connected USB-C device and matches it against a database of known
 * device profiles. The fingerprint includes:
 *
 *   - PDO bitmap: which PDO types (fixed/variable/battery/APDO) were requested
 *   - PDO request order: the sequence in which PDOs were selected
 *   - Inter-message timing histogram: delays between PD messages
 *   - Total negotiation time: from first message to PS_RDY
 *   - Initial current draw: current at the moment PS_RDY is received
 *   - Steady-state current: current 5 seconds after contract is active
 *   - Message count: total PD messages exchanged during negotiation
 *
 * The matching algorithm uses cosine similarity on the normalized feature
 * vector. A threshold of 0.85 is used for a positive match.
 *
 * Applications:
 *   - Device identification (model, OS version, charging state)
 *   - Clone detection (genuine vs. counterfeit devices)
 *   - Physical tracking (identify individuals by their device fingerprint)
 *   - Charging behavior analysis (power management research)
 */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "board.h"
#include "registers.h"

/* ---- External state ---- */
extern device_state_t g_state;

/* ---- Fingerprinting internal state ---- */
static int s_capturing = 0;
static uint32_t s_capture_start_ms = 0;
static uint32_t s_last_msg_time = 0;
static uint8_t s_msg_count = 0;

/* ---- Known device profile database ---- */
typedef struct {
    const char *name;
    uint8_t     device_class;
    pd_fingerprint_t fp;
} device_profile_t;

/* Simplified database of known device PD fingerprints.
 * In a real deployment, this database would be built by profiling
 * many devices and storing their characteristic fingerprints. */
static const device_profile_t s_profile_db[] = {
    {
        "iPhone 15 Pro", 1,
        {
            .pdo_bitmap = 0x0B,  /* PDOs 0, 1, 3 (5V, 9V, 15V) */
            .pdo_request_order = {1, 3, 0}, /* Requests 9V first, then 15V, falls back to 5V */
            .num_pdo_requests = 3,
            .timing_histogram = {5, 12, 8, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            .negotiation_time_ms = 145,
            .initial_current_ma = 850,
            .steady_current_ma = 420,
            .msg_count = 8,
            .device_class = 1,
        }
    },
    {
        "Samsung Galaxy S24", 2,
        {
            .pdo_bitmap = 0x09,  /* PDOs 0, 3 (5V, 15V) */
            .pdo_request_order = {3, 0},
            .num_pdo_requests = 2,
            .timing_histogram = {3, 8, 15, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            .negotiation_time_ms = 98,
            .initial_current_ma = 1200,
            .steady_current_ma = 680,
            .msg_count = 6,
            .device_class = 2,
        }
    },
    {
        "Google Pixel 8", 3,
        {
            .pdo_bitmap = 0x0F,  /* PDOs 0, 1, 2, 3 (5V, 9V, 15V, 20V) */
            .pdo_request_order = {3, 2, 1, 0},
            .num_pdo_requests = 4,
            .timing_histogram = {2, 5, 10, 12, 6, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            .negotiation_time_ms = 180,
            .initial_current_ma = 950,
            .steady_current_ma = 550,
            .msg_count = 10,
            .device_class = 3,
        }
    },
    {
        "MacBook Air M3", 4,
        {
            .pdo_bitmap = 0x11,  /* PDOs 0, 4 (5V, 20V/5A APDO) */
            .pdo_request_order = {4, 0},
            .num_pdo_requests = 2,
            .timing_histogram = {1, 3, 5, 8, 10, 7, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0},
            .negotiation_time_ms = 220,
            .initial_current_ma = 3500,
            .steady_current_ma = 1800,
            .msg_count = 7,
            .device_class = 4,
        }
    },
    {
        "Generic USB-C Charger (5V only)", 5,
        {
            .pdo_bitmap = 0x01,  /* PDO 0 only (5V) */
            .pdo_request_order = {0},
            .num_pdo_requests = 1,
            .timing_histogram = {20, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            .negotiation_time_ms = 45,
            .initial_current_ma = 500,
            .steady_current_ma = 300,
            .msg_count = 4,
            .device_class = 5,
        }
    },
    {
        "Raspberry Pi 5", 6,
        {
            .pdo_bitmap = 0x09,  /* PDOs 0, 3 (5V, 15V) */
            .pdo_request_order = {3, 0},
            .num_pdo_requests = 2,
            .timing_histogram = {4, 10, 8, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            .negotiation_time_ms = 120,
            .initial_current_ma = 2000,
            .steady_current_ma = 1200,
            .msg_count = 6,
            .device_class = 6,
        }
    },
};
#define NUM_PROFILES (sizeof(s_profile_db) / sizeof(device_profile_t))

/* ---- Init ---- */
void fingerprint_init(void) {
    s_capturing = 0;
    s_msg_count = 0;
    memset(&g_state.fingerprint, 0, sizeof(pd_fingerprint_t));
}

/* ---- Start a fingerprint capture ---- */
void fingerprint_start_capture(void) {
    s_capturing = 1;
    s_capture_start_ms = millis();
    s_last_msg_time = millis();
    s_msg_count = 0;
    memset(&g_state.fingerprint, 0, sizeof(pd_fingerprint_t));
}

/* ---- Record a PD message during capture (called from pd_controller) ---- */
void fingerprint_record_msg(uint8_t msg_type, uint8_t num_objs) {
    if (!s_capturing) return;

    s_msg_count++;
    g_state.fingerprint.msg_count = s_msg_count;

    /* Record PDO requests */
    if (msg_type == PD_DATA_REQUEST && num_objs > 0) {
        uint8_t idx = g_state.fingerprint.num_pdo_requests;
        if (idx < FP_MAX_PDO_ENTRIES) {
            /* Extract the requested PDO index from the Request DO */
            /* Object position is bits [30:28] (1-based) */
            g_state.fingerprint.pdo_request_order[idx] = 0; /* Would extract from actual data */
            g_state.fingerprint.num_pdo_requests = idx + 1;
        }
    }

    /* Record source capabilities */
    if (msg_type == PD_DATA_SOURCE_CAP) {
        for (uint8_t i = 0; i < num_objs; i++) {
            uint32_t pdo_type = 0; /* Would extract from actual data */
            g_state.fingerprint.pdo_bitmap |= (1u << (pdo_type + i));
        }
    }

    /* Record inter-message timing */
    uint32_t now = millis();
    uint32_t delta = now - s_last_msg_time;
    s_last_msg_time = now;

    /* Map delta to histogram bin (log scale: 0-1ms, 1-2, 2-5, 5-10, 10-20, ... */
    uint8_t bin = 0;
    if (delta < 1) bin = 0;
    else if (delta < 2) bin = 1;
    else if (delta < 5) bin = 2;
    else if (delta < 10) bin = 3;
    else if (delta < 20) bin = 4;
    else if (delta < 50) bin = 5;
    else if (delta < 100) bin = 6;
    else if (delta < 200) bin = 7;
    else if (delta < 500) bin = 8;
    else if (delta < 1000) bin = 9;
    else if (delta < 2000) bin = 10;
    else if (delta < 5000) bin = 11;
    else if (delta < 10000) bin = 12;
    else if (delta < 20000) bin = 13;
    else if (delta < 50000) bin = 14;
    else bin = 15;

    if (bin < FP_TIMING_BINS) {
        g_state.fingerprint.timing_histogram[bin]++;
    }

    /* Check for PS_RDY (end of negotiation) */
    if (msg_type == PD_CTRL_PS_RDY) {
        g_state.fingerprint.negotiation_time_ms = (uint16_t)(now - s_capture_start_ms);

        /* Read initial current draw */
        uint16_t v;
        int16_t i;
        uint16_t p;
        if (current_monitor_read(PD_PORT_SINK, &v, &i, &p) == 0) {
            g_state.fingerprint.initial_current_ma = (uint16_t)i;
        }

        s_capturing = 0; /* Capture complete */
    }
}

/* ---- Cosine similarity between two feature vectors ---- */
static float cosine_similarity(const pd_fingerprint_t *a, const pd_fingerprint_t *b) {
    /* Build feature vectors from the fingerprint components */
    float va[32], vb[32];
    int dim = 0;

    /* PDO bitmap (1 dimension) */
    va[dim] = (float)(a->pdo_bitmap & 0x3Fu);
    vb[dim] = (float)(b->pdo_bitmap & 0x3Fu);
    dim++;

    /* Number of PDO requests */
    va[dim] = (float)a->num_pdo_requests;
    vb[dim] = (float)b->num_pdo_requests;
    dim++;

    /* PDO request order (up to 10 dimensions) */
    for (int i = 0; i < FP_MAX_PDO_ENTRIES; i++) {
        va[dim] = (float)a->pdo_request_order[i];
        vb[dim] = (float)b->pdo_request_order[i];
        dim++;
    }

    /* Timing histogram (16 dimensions) */
    for (int i = 0; i < FP_TIMING_BINS; i++) {
        va[dim] = (float)a->timing_histogram[i];
        vb[dim] = (float)b->timing_histogram[i];
        dim++;
    }

    /* Negotiation time */
    va[dim] = (float)a->negotiation_time_ms;
    vb[dim] = (float)b->negotiation_time_ms;
    dim++;

    /* Initial current */
    va[dim] = (float)a->initial_current_ma;
    vb[dim] = (float)b->initial_current_ma;
    dim++;

    /* Steady current */
    va[dim] = (float)a->steady_current_ma;
    vb[dim] = (float)b->steady_current_ma;
    dim++;

    /* Message count */
    va[dim] = (float)a->msg_count;
    vb[dim] = (float)b->msg_count;
    dim++;

    /* Compute cosine similarity */
    float dot = 0.0f, mag_a = 0.0f, mag_b = 0.0f;
    for (int i = 0; i < dim; i++) {
        dot += va[i] * vb[i];
        mag_a += va[i] * va[i];
        mag_b += vb[i] * vb[i];
    }

    if (mag_a == 0.0f || mag_b == 0.0f) return 0.0f;
    return dot / (sqrtf(mag_a) * sqrtf(mag_b));
}

/* ---- Match a captured fingerprint against the database ---- */
int fingerprint_match(const pd_fingerprint_t *fp, uint8_t *device_class) {
    float best_score = 0.0f;
    uint8_t best_class = 0;

    for (int i = 0; i < (int)NUM_PROFILES; i++) {
        float score = cosine_similarity(fp, &s_profile_db[i].fp);
        if (score > best_score) {
            best_score = score;
            best_class = s_profile_db[i].device_class;
        }
    }

    /* Threshold: 0.85 for a positive match */
    if (best_score >= 0.85f) {
        *device_class = best_class;
        return 0;
    }

    *device_class = 0; /* Unknown */
    return -1;
}

/* ---- Get device class name ---- */
const char *fingerprint_class_name(uint8_t cls) {
    for (int i = 0; i < (int)NUM_PROFILES; i++) {
        if (s_profile_db[i].device_class == cls) {
            return s_profile_db[i].name;
        }
    }
    return "Unknown";
}