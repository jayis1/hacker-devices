/* test_main.c — Host-side unit tests for Chronos-Phantom firmware
 *
 * Compiles with host GCC (no ARM toolchain) to validate engine logic.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ptp_engine.h"
#include "ntp_engine.h"
#include "skew_gen.h"
#include "covert_codec.h"
#include "eth_bridge.h"
#include "board.h"

static int test_count = 0;
static int test_fail = 0;

#define TEST(name)  do { test_count++; } while (0)
#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        test_fail++; \
    } \
} while (0)

static void test_ptp_init(void)
{
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    TEST("ptp_init");
    CHECK(st.mode == MODE_PASSIVE_SNIFF);
    CHECK(st.spoofed_gm.grandmaster_priority1 == 0);
    CHECK(st.spoofed_gm.grandmaster_clock_quality.clock_class == 6);
    CHECK(st.my_clock_identity[0] == 0x02);
    CHECK(st.my_sequence_id == 1);
    CHECK(st.skew.active == 0);
    printf("[OK] ptp_init\n");
}

static void test_bmca_compare(void)
{
    ptp_gm_descriptor_t us, them;
    memset(&us, 0, sizeof(us));
    memset(&them, 0, sizeof(them));

    us.grandmaster_priority1 = 0;
    them.grandmaster_priority1 = 128;
    CHECK(ptp_engine_would_win_bmca(&us, &them) == 1);

    us.grandmaster_priority1 = 128;
    them.grandmaster_priority1 = 128;
    us.grandmaster_clock_quality.clock_class = 6;
    them.grandmaster_clock_quality.clock_class = 248;
    CHECK(ptp_engine_would_win_bmca(&us, &them) == 1);

    us.grandmaster_clock_quality.clock_class = 248;
    them.grandmaster_clock_quality.clock_class = 248;
    us.grandmaster_clock_quality.clock_accuracy = 0x20;
    them.grandmaster_clock_quality.clock_accuracy = 0x31;
    CHECK(ptp_engine_would_win_bmca(&us, &them) == 1);

    printf("[OK] bmca_compare\n");
}

static void test_skew_step(void)
{
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    skew_config_t cfg;
    skew_gen_apply_preset(&cfg, SKEW_PRESET_KERBEROS_EXT);
    ptp_engine_set_skew(&st, &cfg);
    int64_t skew = ptp_engine_compute_skew(&st, 1000);
    CHECK(skew == 5LL * 60 * 1000000000LL);
    printf("[OK] skew_step (Kerberos +5min)\n");
}

static void test_skew_stealth(void)
{
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    skew_config_t cfg;
    skew_gen_apply_preset(&cfg, SKEW_PRESET_PMU_SLOW);
    ptp_engine_set_skew(&st, &cfg);
    /* After 10 seconds: 500 ppb * 10 s = 5000 ns */
    int64_t skew = ptp_engine_compute_skew(&st, 10000);
    CHECK(skew == 5000);
    printf("[OK] skew_stealth (5000 ns after 10 s)\n");
}

static void test_skew_sawtooth(void)
{
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    skew_config_t cfg;
    skew_gen_apply_preset(&cfg, SKEW_PRESET_PMU_SAWTOOTH);
    ptp_engine_set_skew(&st, &cfg);
    /* At half period (30 s = 30000 ms): skew = 1ms * 0.5 = 500000 ns */
    int64_t skew = ptp_engine_compute_skew(&st, 30000);
    CHECK(skew == 500000);
    printf("[OK] skew_sawtooth (500000 ns at 30s)\n");
}

static void test_announce_build(void)
{
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    uint8_t frame[ETH_MAX_FRAME];
    uint8_t src_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint32_t len = ptp_engine_build_announce(&st.spoofed_gm, src_mac, 42,
                                               frame, sizeof(frame));
    CHECK(len > 0);
    CHECK(len == 14 + PTP_ANN_END);
    /* EtherType = 0x88F7 */
    CHECK(frame[12] == 0x88 && frame[13] == 0xF7);
    /* Msg type = Announce (0xB), version 2 */
    CHECK((frame[14] & 0x0F) == PTP_MSG_ANNOUNCE);
    CHECK(((frame[14] >> 4) & 0x0F) == 2);
    /* Priority1 should be 0 */
    CHECK(frame[14 + PTP_ANN_GRANDMASTER_PRIO1] == 0);
    /* clockClass should be 6 */
    CHECK(frame[14 + PTP_ANN_GM_CLK_CLASS] == 6);
    printf("[OK] announce_build (len=%u)\n", len);
}

static void test_announce_parse(void)
{
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    uint8_t frame[ETH_MAX_FRAME];
    uint8_t src_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint32_t len = ptp_engine_build_announce(&st.spoofed_gm, src_mac, 42,
                                               frame, sizeof(frame));
    ptp_gm_descriptor_t parsed;
    int rc = ptp_engine_parse_announce(frame + 14, len - 14, &parsed);
    CHECK(rc == 0);
    CHECK(parsed.grandmaster_priority1 == 0);
    CHECK(parsed.grandmaster_clock_quality.clock_class == 6);
    CHECK(parsed.grandmaster_clock_quality.clock_accuracy == 0x20);
    printf("[OK] announce_parse\n");
}

static void test_covert_codec(void)
{
    covert_state_t st;
    covert_init(&st);
    const uint8_t msg[] = "hello";
    covert_tx_queue(&st, msg, 5);
    uint8_t b;
    CHECK(covert_tx_next_byte(&st, &b) == 0);
    CHECK(b == 'h');
    CHECK(covert_tx_next_byte(&st, &b) == 0);
    CHECK(b == 'e');
    printf("[OK] covert_codec\n");
}

static void test_covert_ptp_encode(void)
{
    uint8_t cf[8];
    ptp_engine_covert_encode_byte(cf, 0x42);
    CHECK(cf[7] == 0x42);
    CHECK(cf[0] == 0);
    uint8_t decoded = ptp_engine_covert_decode_byte(cf);
    CHECK(decoded == 0x42);
    printf("[OK] covert_ptp_encode\n");
}

static void test_ntp_responder(void)
{
    ntp_engine_state_t st;
    ntp_engine_init(&st);
    /* Build a minimal NTP client request (48 bytes) */
    uint8_t req[48];
    memset(req, 0, 48);
    req[0] = 0x1B;  /* LI=0, VN=3, Mode=3 (client) */
    /* xmit ts */
    req[40] = 0xE8; req[41] = 0x50; req[42] = 0x00; req[43] = 0x00;
    uint8_t resp[48];
    uint32_t resp_len = 0;
    int rc = ntp_engine_process(&st, req, 48, 1000000000ULL, resp,
                                  &resp_len, 1000);
    CHECK(rc == 1);
    CHECK(resp_len == 48);
    /* Mode should be 4 (server) */
    CHECK((resp[0] & 0x07) == NTP_MODE_SERVER);
    /* Stratum should be 1 (primary) */
    CHECK(resp[1] == NTP_STRATUM_PRIMARY);
    /* Ref ID should be "GPS\0" */
    CHECK(resp[12] == 'G' && resp[13] == 'P' && resp[14] == 'S');
    printf("[OK] ntp_responder (stratum=%d, ref=%c%c%c)\n",
            resp[1], resp[12], resp[13], resp[14]);
}

static void test_ntp_covert(void)
{
    uint32_t root_delay;
    ntp_engine_covert_encode_root_delay(&root_delay, 0xAB);
    CHECK((root_delay & 0xFF) == 0xAB);
    uint8_t decoded = ntp_engine_covert_decode_root_delay(root_delay);
    CHECK(decoded == 0xAB);
    printf("[OK] ntp_covert (root_delay=0x%08X → 0x%02X)\n",
            root_delay, decoded);
}

static void test_ntp_skew(void)
{
    ntp_engine_state_t st;
    ntp_engine_init(&st);
    skew_config_t cfg;
    skew_gen_apply_preset(&cfg, SKEW_PRESET_KERBEROS_EXT);
    ntp_engine_set_skew(&st, &cfg);
    uint8_t req[48];
    memset(req, 0, 48);
    req[0] = 0x1B;
    uint8_t resp[48];
    uint32_t resp_len = 0;
    ntp_engine_process(&st, req, 48, 1000000000ULL, resp,
                         &resp_len, 1000);
    /* The response xmit timestamp should be offset by +5 min.
     * Without skew: xmit_sec ≈ 1000 + NTP_EPOCH_OFFSET + 1.
     * With skew (+5 min = 300 s): xmit_sec ≈ base + 300.
     */
    uint32_t xmit_sec = ((uint32_t)resp[40] << 24) | ((uint32_t)resp[41] << 16) |
                         ((uint32_t)resp[42] << 8) | resp[43];
    uint32_t expected_no_skew = 1 + 2208988800u + 1;  /* ns=1s, +1s processing */
    CHECK(xmit_sec > expected_no_skew + 200);  /* at least +200 s (5 min=300) */
    printf("[OK] ntp_skew (xmit_sec offset by +5min)\n");
}

static void test_eth_bridge(void)
{
    eth_bridge_t br;
    eth_bridge_init(&br);
    uint8_t mac_a[6] = {0xAA, 0xBB, 0xCC, 0x00, 0x00, 0x01};
    uint8_t mac_b[6] = {0xAA, 0xBB, 0xCC, 0x00, 0x00, 0x02};
    uint8_t frame[64];
    memset(frame, 0, sizeof(frame));
    memcpy(&frame[6], mac_a, 6);  /* src = A */
    uint8_t fwd;
    eth_bridge_forward(&br, frame, 64, 0, &fwd);
    /* dst=all-zero is not multicast, so unknown → flood → port B (1) */
    CHECK(fwd == 1);
    /* Now learn B */
    memcpy(&frame[6], mac_b, 6);
    eth_bridge_forward(&br, frame, 64, 1, &fwd);
    /* Lookup A — A is on port 0, so forward to port 0 */
    memcpy(frame, mac_a, 6);  /* dst = A */
    int port = eth_bridge_lookup_port(&br, frame);
    CHECK(port == 0);
    printf("[OK] eth_bridge (learn + lookup)\n");
}

static void test_captive_cyclic(void)
{
    /* LFSR-based jitter should produce varying values */
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    skew_config_t cfg;
    skew_gen_apply_preset(&cfg, SKEW_PRESET_JITTER_100US);
    ptp_engine_set_skew(&st, &cfg);
    int64_t s1 = ptp_engine_compute_skew(&st, 1000);
    /* Jitter is non-deterministic but should be in range ±100 µs */
    CHECK(s1 >= -100000 && s1 <= 100000);
    printf("[OK] jitter (s1=%ld ns)\n", (long)s1);
}

static void test_frame_passthrough(void)
{
    ptp_engine_state_t st;
    ptp_engine_init(&st);
    st.mode = MODE_INLINE_MITM;
    /* A non-PTP frame should pass through unmodified */
    uint8_t frame[64];
    memset(frame, 0xFF, 64);
    frame[12] = 0x08; frame[13] = 0x00;  /* EtherType = IPv4 */
    uint8_t out[64];
    uint32_t out_len = 0;
    int rc = ptp_engine_rx_frame(&st, frame, 64, 0, out, &out_len);
    CHECK(rc == 1);
    CHECK(out_len == 64);
    CHECK(memcmp(frame, out, 64) == 0);
    printf("[OK] frame_passthrough (non-PTP forwarded)\n");
}

int main(void)
{
    printf("=== Chronos-Phantom Firmware Unit Tests ===\n");
    printf("Author: jayis1\n\n");

    test_ptp_init();
    test_bmca_compare();
    test_skew_step();
    test_skew_stealth();
    test_skew_sawtooth();
    test_announce_build();
    test_announce_parse();
    test_covert_codec();
    test_covert_ptp_encode();
    test_ntp_responder();
    test_ntp_covert();
    test_ntp_skew();
    test_eth_bridge();
    test_captive_cyclic();
    test_frame_passthrough();

    printf("\n=== Results: %d tests, %d failures ===\n",
            test_count, test_fail);
    return test_fail ? 1 : 0;
}

/* Author: jayis1 */
/* License: GPL-2.0 */