/*
 * ACOUSTIC-PHANTOM — TFLM inference engine implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements on-device neural network inference for acoustic event
 * classification. Uses a simplified int8 quantized MLP forward pass
 * (compatible with TFLM tensor layout) loaded from QSPI NOR flash.
 * Also implements the per-keyboard online calibration step: a linear
 * classifier (regularized linear discriminant) trained on-device from
 * labeled feature vectors collected during the guided calibration
 * wizard.
 */

#include <string.h>
#include <math.h>
#include "tflm_inference.h"
#include "dsp_pipeline.h"

/* ---- QSPI NOR model base address (XIP-mapped) -------------------------- */
#define MODEL_BASE_ADDR    0x90000000UL
#define MODEL_OFFSET_KEYBOARD  0x000000
#define MODEL_OFFSET_HDD       0x040000
#define MODEL_OFFSET_PRINTER   0x080000
#define MODEL_OFFSET_SMPS      0x0C0000
#define MODEL_OFFSET_RELAY     0x100000
#define CALIB_OFFSET_BASE      0x140000

/* ---- Model header (stored in NOR flash) -------------------------------- */
typedef struct {
    uint32_t magic;           /* 0xDEADAC01 */
    uint32_t profile;         /* attack_profile_t */
    uint32_t input_dim;       /* feature vector dimension */
    uint32_t hidden_dim;      /* hidden layer size */
    uint32_t num_classes;     /* output classes */
    uint32_t weight_offset;   /* offset to int8 weights from model base */
    uint32_t bias_offset;     /* offset to int32 biases */
    uint32_t label_offset;    /* offset to label strings */
} model_header_t;

/* ---- Label tables ------------------------------------------------------ */
static const char *s_key_labels[] = {
    "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
    "Q","R","S","T","U","V","W","X","Y","Z","0","1","2","3","4","5",
    "6","7","8","9","SPACE","ENTER","SHIFT","TAB","ESC","CTRL","ALT",
    "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
    "UP","DOWN","LEFT","RIGHT","HOME","END","PGUP","PGDN","INS","DEL",
    "BACKSPACE","CAPS","-","=","[","]","\\",";","'","`",",",".","/",
    "NUM0","NUM1","NUM2","NUM3","NUM4","NUM5","NUM6","NUM7","NUM8","NUM9",
    "SCROLL","PAUSE","MENU","WIN_L","WIN_R","PRTSC","SCROLL_LK","NUM_LK",
    "UNKNOWN_89","UNKNOWN_90","UNKNOWN_91","UNKNOWN_92",
    "UNKNOWN_93","UNKNOWN_94","UNKNOWN_95","UNKNOWN_96",
    "UNKNOWN_97","UNKNOWN_98","UNKNOWN_99","UNKNOWN_100",
    "UNKNOWN_101","UNKNOWN_102","UNKNOWN_103"
};

static const char *s_hdd_labels[] = {
    "SEEK_SHORT","SEEK_MEDIUM","SEEK_LONG","SEEK_FULL",
    "SPIN_UP","SPIN_DOWN","IDLE","PARK",
    "READ_BURST","WRITE_BURST","UNKNOWN"
};

static const char *s_printer_labels[] = {
    "DOT_SINGLE","DOT_DOUBLE","DOT_TRIPLE","CHAR_SPACE",
    "CHAR_PRINT","LINE_FEED","CR","FF",
    "BOLD","COMPRESSED","UNKNOWN"
};

static const char *s_smps_labels[] = {
    "IDLE","CPU_LOAD_LOW","CPU_LOAD_HIGH","CRYPTO_AES",
    "CRYPTO_RSA","CRYPTO_ECC","MEMORY_ACCESS","DMA_TRANSFER",
    "UNKNOWN"
};

static const char *s_relay_labels[] = {
    "RELAY_OPEN","RELAY_CLOSE","RELAY_DOUBLE","UNKNOWN"
};

/* ---- State ------------------------------------------------------------- */
static attack_profile_t s_profile = PROFILE_KEYBOARD;
static const model_header_t *s_model = NULL;

/* Calibration state */
static uint8_t s_calibrating = 0;
static uint8_t s_calib_current_key = 0;
static uint8_t s_calib_samples_collected = 0;

/* Calibration data: class means and shared covariance (LDA) */
#define MAX_FEAT_DIM  64
static float s_calib_means[CALIB_CLASSES_MAX][MAX_FEAT_DIM];
static float s_calib_shared_cov[MAX_FEAT_DIM * MAX_FEAT_DIM];
static float s_calib_inv_cov[MAX_FEAT_DIM * MAX_FEAT_DIM];
static uint16_t s_calib_counts[CALIB_CLASSES_MAX];
static uint16_t s_calib_num_classes = 0;
static uint16_t s_calib_feat_dim = 0;

/* Previous classification (for inter-event timing) */
static uint32_t s_last_classified_ms = 0;

/* ---- Softmax ----------------------------------------------------------- */
static void softmax(const float *logits, float *probs, int n)
{
    float max_val = logits[0];
    for (int i = 1; i < n; i++) {
        if (logits[i] > max_val) max_val = logits[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        probs[i] = expf(logits[i] - max_val);
        sum += probs[i];
    }
    for (int i = 0; i < n; i++) {
        probs[i] /= sum;
    }
}

/* ---- 2x2 matrix inverse (for 2-D LDA) ---------------------------------- */
/* For full-dimension LDA we'd need a general matrix inverse; in practice
 * we use diagonal loading + Cholesky. For this implementation we use
 * a simplified nearest-centroid classifier with diagonal covariance
 * (which works well for keyboard MFCC features and is fast on MCU). */

/* ---- Quantized MLP forward pass ---------------------------------------- */
static int mlp_forward(const float *input, uint16_t input_dim,
                       const int8_t *w1, const int8_t *b1,
                       uint16_t hidden, const int8_t *w2, const int8_t *b2,
                       uint16_t output, float *logits)
{
    /* Hidden layer: ReLU(int8 weights × float input + int32 bias) */
    float hidden_out[256];  /* max hidden size */

    if (hidden > 256) return -1;

    for (int h = 0; h < hidden; h++) {
        float sum = 0.0f;
        for (int i = 0; i < input_dim; i++) {
            /* int8 weight (scale=1/127) × float input */
            sum += (float)w1[h * input_dim + i] / 127.0f * input[i];
        }
        /* int32 bias (scale=1/4096) */
        int32_t bias_val = ((const int32_t *)b1)[h];
        sum += (float)bias_val / 4096.0f;
        hidden_out[h] = sum > 0 ? sum : 0.0f;  /* ReLU */
    }

    /* Output layer: linear */
    for (int o = 0; o < output; o++) {
        float sum = 0.0f;
        for (int h = 0; h < hidden; h++) {
            sum += (float)w2[o * hidden + h] / 127.0f * hidden_out[h];
        }
        int32_t bias_val = ((const int32_t *)b2)[o];
        sum += (float)bias_val / 4096.0f;
        logits[o] = sum;
    }

    return 0;
}

/* ---- Nearest-centroid classifier (calibration mode) -------------------- */
static int nearest_centroid(const float *features, uint16_t dim,
                            float *confidences)
{
    if (s_calib_num_classes == 0 || s_calib_feat_dim == 0) return -1;

    float min_dist = 1e30f;
    float dists[CALIB_CLASSES_MAX];

    for (int c = 0; c < s_calib_num_classes; c++) {
        float dist = 0.0f;
        for (int d = 0; d < dim && d < s_calib_feat_dim; d++) {
            float diff = features[d] - s_calib_means[c][d];
            /* Diagonal Mahalanobis distance (inverse variance weighting) */
            float inv_var = s_calib_shared_cov[d * s_calib_feat_dim + d];
            if (inv_var < 1e-6f) inv_var = 1e-6f;
            dist += diff * diff / inv_var;
        }
        dists[c] = dist;
        if (dist < min_dist) min_dist = dist;
    }

    /* Convert distances to softmax-like confidences */
    float sum = 0.0f;
    for (int c = 0; c < s_calib_num_classes; c++) {
        confidences[c] = expf(-(dists[c] - min_dist) * 2.0f);
        sum += confidences[c];
    }
    for (int c = 0; c < s_calib_num_classes; c++) {
        confidences[c] /= sum;
    }

    /* Find argmax */
    int best = 0;
    for (int c = 1; c < s_calib_num_classes; c++) {
        if (confidences[c] > confidences[best]) best = c;
    }
    return best;
}

/* ---- Top-5 selection --------------------------------------------------- */
static void select_top5(const float *probs, int n,
                        uint8_t *top5_id, float *top5_conf)
{
    for (int r = 0; r < 5; r++) {
        int best = 0;
        for (int i = 0; i < n; i++) {
            if (probs[i] > probs[best]) best = i;
        }
        top5_id[r] = (uint8_t)best;
        top5_conf[r] = probs[best];
        probs[best] = -1.0f;  /* remove for next rank */
    }
}

/* ---- Get model pointer for current profile ----------------------------- */
static const model_header_t *get_model_header(attack_profile_t profile)
{
    uint32_t offset = 0;
    switch (profile) {
        case PROFILE_KEYBOARD: offset = MODEL_OFFSET_KEYBOARD; break;
        case PROFILE_HDD:      offset = MODEL_OFFSET_HDD;      break;
        case PROFILE_PRINTER:  offset = MODEL_OFFSET_PRINTER;  break;
        case PROFILE_SMPS:     offset = MODEL_OFFSET_SMPS;     break;
        case PROFILE_RELAY:    offset = MODEL_OFFSET_RELAY;    break;
        default: return NULL;
    }
    return (const model_header_t *)(MODEL_BASE_ADDR + offset);
}

/* ---- Public API -------------------------------------------------------- */
void tflm_inference_init(attack_profile_t profile)
{
    s_profile = profile;
    s_model = get_model_header(profile);
    s_calibrating = 0;
    s_last_classified_ms = 0;

    /* Load calibration data from NOR flash if available */
    const model_header_t *calib_hdr =
        (const model_header_t *)(MODEL_BASE_ADDR + CALIB_OFFSET_BASE);
    if (calib_hdr->magic == 0xCA11B001 && calib_hdr->profile == profile) {
        s_calib_num_classes = (uint16_t)calib_hdr->num_classes;
        s_calib_feat_dim = (uint16_t)calib_hdr->input_dim;
        /* Means and covariance are stored after the header */
        const float *data = (const float *)((const uint8_t *)calib_hdr +
                           sizeof(model_header_t));
        memcpy(s_calib_means, data,
               s_calib_num_classes * s_calib_feat_dim * sizeof(float));
        /* Shared covariance (diagonal stored as full matrix) follows */
        const float *cov_data = data +
                                s_calib_num_classes * s_calib_feat_dim;
        memcpy(s_calib_shared_cov, cov_data,
               s_calib_feat_dim * s_calib_feat_dim * sizeof(float));
    }
}

void tflm_inference_set_profile(attack_profile_t profile)
{
    s_profile = profile;
    s_model = get_model_header(profile);
}

void tflm_inference_reset(void)
{
    s_calibrating = 0;
    s_calib_current_key = 0;
    s_calib_samples_collected = 0;
    memset(s_calib_means, 0, sizeof(s_calib_means));
    memset(s_calib_shared_cov, 0, sizeof(s_calib_shared_cov));
    memset(s_calib_counts, 0, sizeof(s_calib_counts));
    s_calib_num_classes = 0;
    s_calib_feat_dim = 0;
}

void tflm_inference_classify(const event_t *event, classification_t *result)
{
    memset(result, 0, sizeof(classification_t));

    /* Get the feature vector from the DSP pipeline */
    const feature_vector_t *feat = dsp_pipeline_get_features();

    if (feat->dim == 0) {
        /* No features (e.g., relay profile uses onset timing only) */
        result->class_id = 0;
        result->confidence = 0.5f;
        result->inter_event_ms = board_millis() - s_last_classified_ms;
        s_last_classified_ms = board_millis();
        return;
    }

    float confidences[128] = {0};  /* max classes */
    int predicted = -1;

    if (s_calibrating || s_calib_num_classes > 0) {
        /* Use calibration-based nearest-centroid classifier */
        predicted = nearest_centroid(feat->data, feat->dim, confidences);
    } else if (s_model && s_model->magic == 0xDEADAC01) {
        /* Use pre-trained MLP model from flash */
        float logits[128];
        const int8_t *weights = (const int8_t *)((const uint8_t *)s_model +
                                  s_model->weight_offset);
        const int8_t *biases = (const int8_t *)((const uint8_t *)s_model +
                                s_model->bias_offset);

        mlp_forward(feat->data, s_model->input_dim,
                    weights, biases, s_model->hidden_dim,
                    weights + s_model->hidden_dim * s_model->input_dim,
                    biases + s_model->hidden_dim * 4,
                    s_model->num_classes, logits);

        softmax(logits, confidences, s_model->num_classes);

        predicted = 0;
        for (int i = 1; i < (int)s_model->num_classes; i++) {
            if (confidences[i] > confidences[predicted]) predicted = i;
        }
    }

    if (predicted < 0) {
        result->class_id = 0;
        result->confidence = 0.0f;
    } else {
        result->class_id = (uint8_t)predicted;
        result->confidence = confidences[predicted];
        select_top5(confidences,
                    s_calib_num_classes > 0 ? s_calib_num_classes :
                    (s_model ? s_model->num_classes : 0),
                    result->top5_id, result->top5_conf);
    }

    result->inter_event_ms = board_millis() - s_last_classified_ms;
    s_last_classified_ms = board_millis();
}

/* ---- Calibration ------------------------------------------------------- */
void tflm_inference_start_calibration(void)
{
    s_calibrating = 1;
    s_calib_current_key = 0;
    s_calib_samples_collected = 0;
    memset(s_calib_means, 0, sizeof(s_calib_means));
    memset(s_calib_shared_cov, 0, sizeof(s_calib_shared_cov));
    memset(s_calib_counts, 0, sizeof(s_calib_counts));
    s_calib_num_classes = 0;
}

void tflm_inference_add_calibration_sample(const uint8_t *data, uint16_t len)
{
    /* data format: [class_id (1 byte)] [feature_dim (2 bytes)] [features (floats)] */
    if (len < 3) return;

    uint8_t  class_id = data[0];
    uint16_t feat_dim = data[1] | (data[2] << 8);

    if (class_id >= CALIB_CLASSES_MAX) return;
    if (feat_dim > MAX_FEAT_DIM) feat_dim = MAX_FEAT_DIM;
    if (len < (uint16_t)(3 + feat_dim * 4)) return;

    const float *features = (const float *)(data + 3);

    /* Accumulate mean */
    for (int d = 0; d < feat_dim; d++) {
        s_calib_means[class_id][d] += features[d];
    }

    /* Accumulate diagonal variance (running) */
    for (int d = 0; d < feat_dim; d++) {
        float diff = features[d];  /* will subtract mean later */
        s_calib_shared_cov[d * MAX_FEAT_DIM + d] += diff * diff;
    }

    s_calib_counts[class_id]++;
    s_calib_samples_collected++;

    /* Track the number of distinct classes */
    if (class_id >= s_calib_num_classes) {
        s_calib_num_classes = class_id + 1;
    }
    if (feat_dim > s_calib_feat_dim) {
        s_calib_feat_dim = feat_dim;
    }
}

void tflm_inference_finish_calibration(void)
{
    /* Finalize means and covariance */
    for (int c = 0; c < s_calib_num_classes; c++) {
        if (s_calib_counts[c] == 0) continue;
        for (int d = 0; d < s_calib_feat_dim; d++) {
            s_calib_means[c][d] /= s_calib_counts[c];
        }
    }

    /* Compute diagonal variance: var = E[x²] - (E[x])² */
    for (int d = 0; d < s_calib_feat_dim; d++) {
        float sum_sq = s_calib_shared_cov[d * MAX_FEAT_DIM + d];
        float total_count = 0;
        for (int c = 0; c < s_calib_num_classes; c++) {
            total_count += s_calib_counts[c];
        }
        if (total_count > 0) {
            float mean_of_sq = sum_sq / total_count;
            /* Compute global mean across all classes */
            float global_mean = 0.0f;
            for (int c = 0; c < s_calib_num_classes; c++) {
                global_mean += s_calib_means[c][d] * s_calib_counts[c];
            }
            global_mean /= total_count;
            float variance = mean_of_sq - global_mean * global_mean;
            if (variance < 1e-6f) variance = 1e-6f;
            s_calib_shared_cov[d * MAX_FEAT_DIM + d] = variance;
        }
    }

    s_calibrating = 0;
}

const char *tflm_inference_get_label(uint8_t class_id)
{
    switch (s_profile) {
        case PROFILE_KEYBOARD:
            if (class_id < 104) return s_key_labels[class_id];
            return "UNKNOWN";
        case PROFILE_HDD:
            if (class_id < 11) return s_hdd_labels[class_id];
            return "UNKNOWN";
        case PROFILE_PRINTER:
            if (class_id < 11) return s_printer_labels[class_id];
            return "UNKNOWN";
        case PROFILE_SMPS:
            if (class_id < 9) return s_smps_labels[class_id];
            return "UNKNOWN";
        case PROFILE_RELAY:
            if (class_id < 4) return s_relay_labels[class_id];
            return "UNKNOWN";
        default:
            return "UNKNOWN";
    }
}