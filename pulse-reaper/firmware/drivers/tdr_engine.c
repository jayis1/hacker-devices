/*
 * tdr_engine.c — TDR acquisition and cable classification
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Drives the FPGA to acquire a 2 us reflectogram (200 m at 0.67c), then
 * classifies the cable against a small signature library based on
 * characteristic impedance, reflection pattern, and live-conductor
 * detection.
 */

#include "tdr_engine.h"
#include "fpga_dsp.h"
#include "ble_c2.h"
#include "oled.h"
#include "board.h"
#include "clamp_afc.h"
#include <string.h>

#define TDR_VELOCITY_FACTOR 0.67f   /* typical PVC-jacketed twisted pair */
#define TDR_SAMPLE_RATE_HZ  100000000u  /* 100 MSa/s */
#define TDR_SAMPLES        FPGA_REFLECTOGRAM_SAMPLES

static tdr_result_t g_result;
static int16_t      g_reflectogram[FPGA_REFLECTOGRAM_SAMPLES];

/* ----------------------------------------------------------------------- */
/*  Signature library                                                       */
/* ----------------------------------------------------------------------- */

typedef struct {
    cable_type_t type;
    uint32_t     impedance_mohm;
    const char  *name;
} cable_sig_t;

static const cable_sig_t s_sigs[] = {
    { CABLE_CAT5E,          100000, "Cat 5e UTP 100ohm"   },
    { CABLE_CAT6,           100000, "Cat 6 UTP 100ohm"   },
    { CABLE_CAT6A,          100000, "Cat 6A UTP 100ohm"  },
    { CABLE_CAT7,           100000, "Cat 7 S/FTP 100ohm" },
    { CABLE_PROFIBUS_DP,    150000, "Profibus DP 150ohm" },
    { CABLE_RS485_BELDEN9841,120000,"RS485 Belden 120ohm"},
    { CABLE_MIL1553_TWINAX,  78000, "MIL-1553 twinax 78ohm"},
    { CABLE_POTS_RISER,     100000, "POTS riser 100ohm" },
};

#define NUM_SIGS  (int)(sizeof(s_sigs)/sizeof(s_sigs[0]))

/* ----------------------------------------------------------------------- */
/*  Init                                                                    */
/* ----------------------------------------------------------------------- */

void tdr_engine_init(void) {
    memset(&g_result, 0, sizeof(g_result));
    g_result.type = CABLE_UNKNOWN;
}

/* ----------------------------------------------------------------------- */
/*  DSP helpers                                                             */
/* ----------------------------------------------------------------------- */

/* Find the index of the largest-magnitude sample in a window. */
static int find_peak(const int16_t *buf, int start, int end) {
    int idx = start;
    int32_t mag = 0;
    for (int i = start; i < end; i++) {
        int32_t a = buf[i] < 0 ? -buf[i] : buf[i];
        if (a > mag) { mag = a; idx = i; }
    }
    return idx;
}

/* Estimate the characteristic impedance from the reflection amplitude.
 * This is a simplification — a real implementation uses the reflection
 * coefficient and the known pulse source impedance (50 ohm). */
static uint32_t estimate_impedance(int16_t launch_amp, int16_t open_refl_amp) {
    if (launch_amp == 0) return 0;
    /* rho = Vr/Vi ; Z = Z0 * (1+rho)/(1-rho), Z0 = 50 */
    int32_t rho_q8 = ((int32_t)open_refl_amp << 8) / (int32_t)launch_amp;
    if (rho_q8 == 256) return 999999;  /* open */
    if (rho_q8 == -256) return 0;       /* short */
    int32_t z_q8 = (50 * (256 + rho_q8)) / (256 - rho_q8);
    if (z_q8 < 0) z_q8 = 0;
    return (uint32_t)z_q8 * 1000u / 256u;  /* milliohms */
}

/* Detect a live (powered) conductor by looking for 50/60 Hz envelope on
 * the first few samples (the TDR pulse itself is much faster). */
static int detect_live_conductor(const int16_t *buf, int n) {
    /* In practice we'd look at a separate low-rate ADC channel; here we
     * check for a large DC offset on the first samples relative to the
     * later baseline. */
    int32_t early = 0, late = 0;
    int early_n = n / 10;
    int late_n  = n / 10;
    for (int i = 0; i < early_n; i++) early += buf[i];
    for (int i = n - late_n; i < n; i++) late += buf[i];
    if (early_n == 0 || late_n == 0) return 0;
    early /= early_n;
    late  /= late_n;
    int32_t diff = early - late;
    if (diff < 0) diff = -diff;
    return diff > 200;  /* threshold: > 200 LSBs offset -> likely live */
}

/* ----------------------------------------------------------------------- */
/*  Acquire + classify                                                      */
/* ----------------------------------------------------------------------- */

int tdr_engine_acquire_and_classify(void) {
    /* Safety: jaw must be closed */
    if (!clamp_afc_jaw_closed()) {
        g_result.type = CABLE_UNKNOWN;
        return -1;
    }

    /* Discharge the TDR path before arming */
    clamp_afc_tdr_discharge();

    /* Arm the FPGA TDR engine */
    if (fpga_dsp_arm_tdr() != 0) {
        g_result.type = CABLE_UNKNOWN;
        return -1;
    }

    /* Read the reflectogram */
    int n = fpga_dsp_read_reflectogram(g_reflectogram, TDR_SAMPLES);
    if (n <= 0) {
        g_result.type = CABLE_UNKNOWN;
        return -1;
    }

    /* Launch amplitude = first sample (the incident pulse). */
    int16_t launch = g_reflectogram[0];
    int16_t open_refl = 0;
    int open_idx = find_peak(g_reflectogram, n / 4, n);
    open_refl = g_reflectogram[open_idx];

    /* Estimate impedance */
    uint32_t z = estimate_impedance(launch, open_refl);
    g_result.impedance_mohm = z;

    /* Estimate length: time to the open reflection */
    /* samples * (1/100MHz) * velocity * 0.5 (round trip) -> metres */
    /* len_mm = open_idx * 10 * 0.67 / 2 * 1000 = open_idx * 3.35 (mm) */
    g_result.length_mm = (uint32_t)(open_idx * 3.35f);

    /* Near/far distances (tap point is at sample 0) */
    g_result.dist_to_near_mm = 0u;  /* tap is at the clamp */
    g_result.dist_to_far_mm  = g_result.length_mm;

    /* Termination quality: if the far reflection is small relative to
     * launch, the far end is well terminated. */
    int32_t far_ratio = (launch != 0)
        ? ((open_refl < 0 ? -open_refl : open_refl) * 100) / (launch < 0 ? -launch : launch)
        : 100;
    g_result.terminated_far = (far_ratio < 10);
    g_result.terminated_near = 1;  /* near end is the clamp */

    /* Shielded? Cat 7 and MIL-1553 are shielded; the reflectogram for
     * shielded cable shows a much smaller coupling amplitude. */
    g_result.shielded = ((open_refl < 0 ? -open_refl : open_refl) < 50);

    /* Live conductor? */
    g_result.live_conductor = detect_live_conductor(g_reflectogram, n);

    /* Classify by impedance + shield + length heuristics */
    cable_type_t best = CABLE_UNKNOWN;
    int best_score = 0;
    for (int i = 0; i < NUM_SIGS; i++) {
        int32_t zdiff = (int32_t)z - (int32_t)s_sigs[i].impedance_mohm;
        if (zdiff < 0) zdiff = -zdiff;
        int score = 1000 - (zdiff / 100);  /* closer impedance = higher score */
        if (g_result.shielded && s_sigs[i].type == CABLE_CAT7) score += 100;
        if (score > best_score) {
            best_score = score;
            best = s_sigs[i].type;
        }
    }
    if (best_score < 100) best = CABLE_UNKNOWN;
    g_result.type = best;

    /* Copy label */
    const char *nm = "Unknown";
    for (int i = 0; i < NUM_SIGS; i++) {
        if (s_sigs[i].type == best) { nm = s_sigs[i].name; break; }
    }
    int j = 0;
    while (nm[j] && j < 31) { g_result.label[j] = nm[j]; j++; }
    g_result.label[j] = '\0';

    /* Safety flag: refuse inject if live */
    clamp_afc_set_cable_safe(!g_result.live_conductor && g_result.type != CABLE_POWER);

    return 0;
}

const tdr_result_t *tdr_engine_get_result(void) {
    return &g_result;
}

/* ----------------------------------------------------------------------- */
/*  Display + BLE                                                           */
/* ----------------------------------------------------------------------- */

void tdr_engine_show_result(void) {
    char line1[22];
    char line2[22];

    /* line1 = label (truncated) */
    int i = 0;
    while (g_result.label[i] && i < 21) { line1[i] = g_result.label[i]; i++; }
    line1[i] = '\0';

    /* line2 = "Z=xxx L=yyy" (cheap int formatter) */
    int p = 0;
    const char *zstr = "Z=";
    while (*zstr) line2[p++] = *zstr++;
    uint32_t zohm = g_result.impedance_mohm / 1000u;
    char tmp[12]; int ti = 0;
    if (zohm == 0u) tmp[ti++] = '0';
    while (zohm) { tmp[ti++] = (char)('0' + (zohm % 10u)); zohm /= 10u; }
    while (ti) line2[p++] = tmp[--ti];
    const char *lstr = " L=";
    while (*lstr) line2[p++] = *lstr++;
    uint32_t lm = g_result.length_mm / 1000u;
    ti = 0;
    if (lm == 0u) tmp[ti++] = '0';
    while (lm) { tmp[ti++] = (char)('0' + (lm % 10u)); lm /= 10u; }
    while (ti) line2[p++] = tmp[--ti];
    const char *mstr = "m";
    while (*mstr) line2[p++] = *mstr++;
    line2[p] = '\0';

    oled_show_status(line1, line2);
}

void tdr_engine_send_reflectogram_over_ble(void) {
    /* Send a BLE frame with: [TDR_RESULT opcode][type][z_msb..lsb][len_msb..lsb]
     * followed by the reflectogram in 64-byte chunks. */
    uint8_t hdr[12];
    hdr[0] = 0x10;  /* TDR_RESULT opcode for the app */
    hdr[1] = (uint8_t)g_result.type;
    hdr[2] = (uint8_t)(g_result.impedance_mohm >> 24);
    hdr[3] = (uint8_t)(g_result.impedance_mohm >> 16);
    hdr[4] = (uint8_t)(g_result.impedance_mohm >> 8);
    hdr[5] = (uint8_t)(g_result.impedance_mohm);
    hdr[6] = (uint8_t)(g_result.length_mm >> 24);
    hdr[7] = (uint8_t)(g_result.length_mm >> 16);
    hdr[8] = (uint8_t)(g_result.length_mm >> 8);
    hdr[9] = (uint8_t)(g_result.length_mm);
    hdr[10] = (uint8_t)(g_result.shielded ? 1 : 0);
    hdr[11] = (uint8_t)(g_result.live_conductor ? 1 : 0);
    ble_c2_send_raw(hdr, sizeof(hdr));

    /* Send reflectogram in 64-sample (128-byte) chunks */
    int total = FPGA_REFLECTOGRAM_SAMPLES;
    for (int off = 0; off < total; off += 64) {
        uint8_t chunk[130];
        chunk[0] = 0x11;  /* TDR_CHUNK opcode */
        chunk[1] = (uint8_t)(off & 0xFF);
        chunk[2] = (uint8_t)((off >> 8) & 0xFF);
        int n = (total - off < 64) ? (total - off) : 64;
        for (int i = 0; i < n; i++) {
            int16_t s = g_reflectogram[off + i];
            chunk[3 + i * 2]     = (uint8_t)(s & 0xFF);
            chunk[3 + i * 2 + 1] = (uint8_t)((s >> 8) & 0xFF);
        }
        ble_c2_send_raw(chunk, 3 + n * 2);
    }
}

int tdr_engine_cable_is_safe_for_inject(void) {
    return !g_result.live_conductor && g_result.type != CABLE_POWER
        && g_result.type != CABLE_UNKNOWN;
}