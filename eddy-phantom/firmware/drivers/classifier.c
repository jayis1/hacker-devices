/*
 * eddy-phantom — classifier.c
 * Neural network-based keystroke classifier using CMSIS-NN-style
 * fully-connected layers. Maps 19-dimensional feature vectors to
 * predicted USB HID scancodes with confidence scores.
 *
 * Architecture:
 *   Input:  19 features (13 MFCC + 4 spatial + 2 timing)
 *   Layer1: 19 → 64 (ReLU)
 *   Layer2: 64 → 32 (ReLU)
 *   Layer3: 32 → N scancodes (Softmax)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── Network weights (quantized int8 for CMSIS-NN) ──────────────
 * In production, these are loaded from QSPI flash.
 * Here we provide placeholder structures and a simple float-based
 * inference engine for portability.
 */

typedef struct {
    float weights[CLS_LAYER1_OUT * CLS_LAYER1_IN];
    float biases[CLS_LAYER1_OUT];
} fc_layer1_t;

typedef struct {
    float weights[CLS_LAYER2_OUT * CLS_LAYER1_OUT];
    float biases[CLS_LAYER2_OUT];
} fc_layer2_t;

typedef struct {
    float weights[CLS_MAX_SCANCODES * CLS_LAYER2_OUT];
    float biases[CLS_MAX_SCANCODES];
} fc_layer3_t;

/* ── Calibration reference templates ────────────────────────────
 * During calibration, we store per-key mean feature vectors.
 * The classifier can fall back to nearest-neighbor matching
 * if the neural network is not yet trained.
 */
typedef struct {
    uint8_t  scancode;
    float    mean_features[DSP_FEATURE_DIM];
    float    var_features[DSP_FEATURE_DIM];
    int      sample_count;
    uint8_t  active;
} key_template_t;

static key_template_t g_templates[CLS_MAX_SCANCODES];
static int g_num_active_templates = 0;

/* ── Network state ────────────────────────────────────────────── */
static fc_layer1_t g_layer1;
static fc_layer2_t g_layer2;
static fc_layer3_t g_layer3;
static uint8_t g_conf_threshold = CLS_CONF_THRESHOLD;
static int g_nn_trained = 0;  /* 0 = use template matching, 1 = use NN */

/* ── Current profile scancode table ───────────────────────────── */
static uint16_t g_scancode_table[CLS_MAX_SCANCODES];
static int      g_num_scancodes = 0;

/* ── ReLU activation ──────────────────────────────────────────── */
static void relu(float *x, int n)
{
    for (int i = 0; i < n; i++) {
        if (x[i] < 0.0f) x[i] = 0.0f;
    }
}

/* ── Softmax ──────────────────────────────────────────────────── */
static void softmax(float *x, int n)
{
    float max_val = x[0];
    for (int i = 1; i < n; i++) {
        if (x[i] > max_val) max_val = x[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }

    if (sum > 1e-10f) {
        for (int i = 0; i < n; i++) {
            x[i] /= sum;
        }
    }
}

/* ── Fully connected layer forward pass ───────────────────────── */
static void fc_forward(const float *input, int in_dim,
                       const float *weights, const float *biases,
                       float *output, int out_dim)
{
    for (int o = 0; o < out_dim; o++) {
        float acc = biases[o];
        for (int i = 0; i < in_dim; i++) {
            acc += input[i] * weights[o * in_dim + i];
        }
        output[o] = acc;
    }
}

/* ── Nearest-centroid classification (template matching) ────────
 * Used when the neural network has not been trained yet.
 * Computes Mahalanobis-like distance to each template.
 */
static int template_classify(const float *features,
                              uint8_t *scancode, uint8_t *confidence)
{
    float best_dist = 1e30f;
    float second_dist = 1e30f;
    int best_idx = -1;

    for (int t = 0; t < CLS_MAX_SCANCODES; t++) {
        if (!g_templates[t].active) continue;

        float dist = 0.0f;
        for (int f = 0; f < DSP_FEATURE_DIM; f++) {
            float diff = features[f] - g_templates[t].mean_features[f];
            float var = g_templates[t].var_features[f];
            if (var < 1e-8f) var = 1e-8f;
            dist += diff * diff / var;
        }

        if (dist < best_dist) {
            second_dist = best_dist;
            best_dist = dist;
            best_idx = t;
        } else if (dist < second_dist) {
            second_dist = dist;
        }
    }

    if (best_idx < 0) {
        *scancode = 0;
        *confidence = 0;
        return -1;
    }

    *scancode = g_templates[best_idx].scancode;

    /* Confidence based on ratio of best to second-best distance */
    if (second_dist > 1e-10f) {
        float ratio = best_dist / second_dist;
        /* Lower ratio = more confident. Convert to 0-100 scale. */
        float conf = 100.0f * expf(-ratio * 5.0f);
        if (conf > 100.0f) conf = 100.0f;
        *confidence = (uint8_t)conf;
    } else {
        *confidence = 90;  /* only one template, moderately confident */
    }

    return 0;
}

/* ── Neural network classification ────────────────────────────── */
static int nn_classify(const float *features,
                        uint8_t *scancode, uint8_t *confidence)
{
    float hidden1[CLS_LAYER1_OUT];
    float hidden2[CLS_LAYER2_OUT];
    float output[CLS_MAX_SCANCODES];

    /* Layer 1: 19 → 64 (ReLU) */
    fc_forward(features, CLS_LAYER1_IN,
               g_layer1.weights, g_layer1.biases,
               hidden1, CLS_LAYER1_OUT);
    relu(hidden1, CLS_LAYER1_OUT);

    /* Layer 2: 64 → 32 (ReLU) */
    fc_forward(hidden1, CLS_LAYER1_OUT,
               g_layer2.weights, g_layer2.biases,
               hidden2, CLS_LAYER2_OUT);
    relu(hidden2, CLS_LAYER2_OUT);

    /* Layer 3: 32 → N scancodes (Softmax) */
    fc_forward(hidden2, CLS_LAYER2_OUT,
               g_layer3.weights, g_layer3.biases,
               output, g_num_scancodes);
    softmax(output, g_num_scancodes);

    /* Find argmax */
    float max_prob = 0.0f;
    int max_idx = 0;
    for (int i = 0; i < g_num_scancodes; i++) {
        if (output[i] > max_prob) {
            max_prob = output[i];
            max_idx = i;
        }
    }

    *scancode = (uint8_t)g_scancode_table[max_idx];
    *confidence = (uint8_t)(max_prob * 100.0f);

    return 0;
}

/* ── Public API ───────────────────────────────────────────────── */

int classifier_init(void)
{
    /* Clear templates */
    for (int i = 0; i < CLS_MAX_SCANCODES; i++) {
        g_templates[i].active = 0;
        g_templates[i].sample_count = 0;
    }
    g_num_active_templates = 0;
    g_num_scancodes = 0;
    g_nn_trained = 0;
    g_conf_threshold = CLS_CONF_THRESHOLD;

    /* Initialize weights to zero (will be loaded from QSPI) */
    for (int i = 0; i < CLS_LAYER1_OUT * CLS_LAYER1_IN; i++)
        g_layer1.weights[i] = 0.0f;
    for (int i = 0; i < CLS_LAYER1_OUT; i++)
        g_layer1.biases[i] = 0.0f;

    for (int i = 0; i < CLS_LAYER2_OUT * CLS_LAYER1_OUT; i++)
        g_layer2.weights[i] = 0.0f;
    for (int i = 0; i < CLS_LAYER2_OUT; i++)
        g_layer2.biases[i] = 0.0f;

    for (int i = 0; i < CLS_MAX_SCANCODES * CLS_LAYER2_OUT; i++)
        g_layer3.weights[i] = 0.0f;
    for (int i = 0; i < CLS_MAX_SCANCODES; i++)
        g_layer3.biases[i] = 0.0f;

    return 0;
}

int classifier_infer(float *features, uint8_t *scancode, uint8_t *confidence)
{
    if (g_nn_trained && g_num_scancodes > 0) {
        return nn_classify(features, scancode, confidence);
    } else if (g_num_active_templates > 0) {
        return template_classify(features, scancode, confidence);
    } else {
        *scancode = 0;
        *confidence = 0;
        return -1;
    }
}

int classifier_load_profile(uint32_t qspi_offset)
{
    /* Load weights and scancode table from QSPI flash.
     * Format at qspi_offset:
     *   [0..3]     magic (0xEDD70001)
     *   [4..7]     num_scancodes
     *   [8..8+N*2] scancode table
     *   [...]      layer1 weights + biases
     *   [...]      layer2 weights + biases
     *   [...]      layer3 weights + biases
     */
    uint8_t header[8];
    if (qspi_read(qspi_offset, header, 8) != 0)
        return -1;

    uint32_t magic = header[0] | (header[1]<<8) | (header[2]<<16) | (header[3]<<24);
    if (magic != 0xEDD70001)
        return -1;

    uint32_t n_sc = header[4] | (header[5]<<8) | (header[6]<<16) | (header[7]<<24);
    if (n_sc > CLS_MAX_SCANCODES)
        n_sc = CLS_MAX_SCANCODES;
    g_num_scancodes = n_sc;

    /* Read scancode table */
    uint8_t sc_buf[CLS_MAX_SCANCODES * 2];
    if (qspi_read(qspi_offset + 8, sc_buf, n_sc * 2) != 0)
        return -1;

    for (int i = 0; i < (int)n_sc; i++) {
        g_scancode_table[i] = sc_buf[i*2] | (sc_buf[i*2+1] << 8);
    }

    /* Read layer1 weights */
    uint32_t offset = qspi_offset + 8 + n_sc * 2;
    uint32_t l1_w_size = CLS_LAYER1_OUT * CLS_LAYER1_IN * sizeof(float);
    if (qspi_read(offset, g_layer1.weights, l1_w_size) != 0)
        return -1;
    offset += l1_w_size;

    if (qspi_read(offset, g_layer1.biases, CLS_LAYER1_OUT * sizeof(float)) != 0)
        return -1;
    offset += CLS_LAYER1_OUT * sizeof(float);

    /* Read layer2 weights */
    uint32_t l2_w_size = CLS_LAYER2_OUT * CLS_LAYER1_OUT * sizeof(float);
    if (qspi_read(offset, g_layer2.weights, l2_w_size) != 0)
        return -1;
    offset += l2_w_size;

    if (qspi_read(offset, g_layer2.biases, CLS_LAYER2_OUT * sizeof(float)) != 0)
        return -1;
    offset += CLS_LAYER2_OUT * sizeof(float);

    /* Read layer3 weights */
    uint32_t l3_w_size = g_num_scancodes * CLS_LAYER2_OUT * sizeof(float);
    if (qspi_read(offset, g_layer3.weights, l3_w_size) != 0)
        return -1;
    offset += l3_w_size;

    if (qspi_read(offset, g_layer3.biases, g_num_scancodes * sizeof(float)) != 0)
        return -1;

    g_nn_trained = 1;
    return 0;
}

void classifier_set_threshold(uint8_t thresh)
{
    g_conf_threshold = thresh;
}

/* ── Calibration: build per-key templates ─────────────────────── */

int classifier_calibrate_start(const char *name, uint16_t controller_id)
{
    (void)name;
    (void)controller_id;

    /* Clear existing templates */
    for (int i = 0; i < CLS_MAX_SCANCODES; i++) {
        g_templates[i].active = 0;
        g_templates[i].sample_count = 0;
    }
    g_num_active_templates = 0;
    g_nn_trained = 0;  /* fall back to template matching during calibration */

    return 0;
}

int classifier_calibrate_add_key(uint8_t scancode, float *features)
{
    /* Find or create template for this scancode */
    int idx = -1;
    for (int i = 0; i < CLS_MAX_SCANCODES; i++) {
        if (g_templates[i].active && g_templates[i].scancode == scancode) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        /* Find free slot */
        for (int i = 0; i < CLS_MAX_SCANCODES; i++) {
            if (!g_templates[i].active) {
                idx = i;
                g_templates[idx].scancode = scancode;
                g_templates[idx].active = 1;
                g_templates[idx].sample_count = 0;
                g_num_active_templates++;
                break;
            }
        }
    }

    if (idx < 0)
        return -1;  /* no free slots */

    /* Update running mean and variance using Welford's algorithm */
    key_template_t *t = &g_templates[idx];
    int n = t->sample_count + 1;

    for (int f = 0; f < DSP_FEATURE_DIM; f++) {
        float old_mean = t->mean_features[f];
        float delta = features[f] - old_mean;
        t->mean_features[f] = old_mean + delta / n;
        float delta2 = features[f] - t->mean_features[f];
        if (n > 1) {
            t->var_features[f] = ((n - 2) * t->var_features[f] + delta * delta2) / (n - 1);
        } else {
            t->var_features[f] = delta * delta2;
        }
    }

    t->sample_count = n;
    return 0;
}

int classifier_calibrate_finish(void)
{
    /* Mark templates as ready. In a full implementation, this would
     * also train the neural network weights from the collected samples
     * using a simple gradient descent or save the templates to QSPI/SD
     * for offline training. */
    return 0;
}