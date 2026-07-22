/*
 * someip_driver.c — SOME/IP service discovery + fuzzer for AxleTap
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * SOME/IP (Scalable service-Oriented MiddlewarE over IP) is the
 * AUTOSAR-standard in-vehicle service framework carried over the
 * automotive Ethernet link. SOME/IP-SD (Service Discovery) uses UDP
 * port 30490 for multicast service Offers / Finds.
 *
 * This driver:
 *   - Passively discovers SOME/IP services from observed Offer frames.
 *   - Actively sends Find / Offer messages to map services faster.
 *   - Fuzzes discovered services with malformed SOME/IP payloads to
 *     test consumer robustness.
 *
 * SOME/IP frame layout (over UDP):
 *   [Eth][IP][UDP 30490][SOME/IP-SD header][Entries][Options]
 *
 * SOME/IP-SD header (12 bytes):
 *   uint32 flags
 *   uint32 reserved
 *   uint32 entries_length
 *
 * Each entry (16 bytes):
 *   uint8  type
 *   uint8  sw_index1
 *   uint16 sw_index2
 *   uint32 num_options (low 4 bits) + reserved
 *   uint16 service_id
 *   uint16 instance_id
 *   uint8  major_version
 *   uint24 minor_version
 *
 * The simplified SOME/IP header itself (16 bytes) inside the UDP payload:
 *   uint32 message_id (service_id << 16 | method_id)
 *   uint32 length
 *   uint16 request_id (client_id)
 *   uint16 session_id
 *   uint8  protocol_version
 *   uint8  interface_version
 *   uint8  message_type
 *   uint8  return_code
 */

#include "someip_driver.h"
#include "bridge_driver.h"
#include "../board.h"
#include <string.h>

static someip_service_t g_services[SOMEIP_MAX_SERVICES];
static int g_service_count;
static someip_fuzz_mode_t g_fuzz_mode = SOMEIP_FUZZ_OFF;
static uint16_t g_fuzz_service;
static uint16_t g_fuzz_method;
static uint32_t g_fuzz_rate;
static uint32_t g_fuzz_last_ms;

static uint32_t now_ms(void)
{
    static uint32_t last = 0;
    uint32_t t = (uint32_t)(TIM2->CNT / (GPTP_TIMER_HZ / 1000));
    if (t < last) last = t;
    return t;
}

void someip_init(void)
{
    memset(g_services, 0, sizeof(g_services));
    g_service_count = 0;
    g_fuzz_mode = SOMEIP_FUZZ_OFF;
}

/* Register or update a discovered service. */
static void record_service(uint16_t sid, uint16_t iid, uint16_t port,
                           uint8_t major, uint32_t minor,
                           uint8_t proto, uint32_t ipv4)
{
    for (int i = 0; i < g_service_count; i++) {
        if (g_services[i].service_id == sid &&
            g_services[i].instance_id == iid) {
            g_services[i].port = port;
            g_services[i].major_version = major;
            g_services[i].minor_version = minor;
            g_services[i].protocol = proto;
            g_services[i].ipv4_addr = ipv4;
            g_services[i].last_seen_ms = now_ms();
            g_services[i].active = 1;
            return;
        }
    }
    if (g_service_count >= SOMEIP_MAX_SERVICES) return;
    someip_service_t *s = &g_services[g_service_count++];
    s->service_id = sid;
    s->instance_id = iid;
    s->method_id = 0;
    s->major_version = major;
    s->minor_version = minor;
    s->port = port;
    s->protocol = proto;
    s->ipv4_addr = ipv4;
    s->last_seen_ms = now_ms();
    s->active = 1;
}

/* Parse a SOME/IP-SD Offer message from the captured frame. The frame
 * here is the full Ethernet frame: [DA][SA][802.1Q?][IP][UDP][SD].
 */
static void parse_sd_offer(const uint8_t *frame, uint16_t len)
{
    /* Find UDP header. We support IPv4 only. */
    int ip_off = 14;
    /* Skip VLAN tag if present */
    if (frame[12] == 0x81 && frame[13] == 0x00) ip_off = 18;
    if (len < ip_off + 20 + 8) return;
    if (frame[ip_off] != 0x45) return;   /* IPv4, IHL=5 */
    int ip_hdr_len = (frame[ip_off] & 0x0F) * 4;
    int udp_off = ip_off + ip_hdr_len;
    if (len < udp_off + 8) return;
    uint16_t sport = (frame[udp_off + 0] << 8) | frame[udp_off + 1];
    uint16_t dport = (frame[udp_off + 2] << 8) | frame[udp_off + 3];
    if (sport != SOMEIP_SD_PORT && dport != SOMEIP_SD_PORT) return;

    /* Some/IP header follows: 16 bytes */
    int sip_off = udp_off + 8;
    if (len < sip_off + 16) return;
    uint32_t msg_id = (frame[sip_off + 0] << 24) | (frame[sip_off + 1] << 16)
                    | (frame[sip_off + 2] << 8) | frame[sip_off + 3];
    /* For SD, msg_id service_id = 0xFFFF, method_id = 0x8100 */
    if ((msg_id >> 16) != 0xFFFF) return;
    /* Some/IP-SD header follows the 16-byte SOME/IP header */
    int sd_off = sip_off + 16;
    if (len < sd_off + 12) return;
    uint32_t entries_len = (frame[sd_off + 8] << 24) | (frame[sd_off + 9] << 16)
                         | (frame[sd_off + 10] << 8) | frame[sd_off + 11];
    int entries_off = sd_off + 12;
    int e = 0;
    while (e + 16 <= (int)entries_len && entries_off + e + 16 <= len) {
        const uint8_t *ent = frame + entries_off + e;
        uint8_t etype = ent[0];
        uint16_t sid = (ent[8] << 8) | ent[9];
        uint16_t iid = (ent[10] << 8) | ent[11];
        uint8_t  major = ent[12];
        uint32_t minor = (ent[13] << 16) | (ent[14] << 8) | ent[15];
        if (etype == SOMEIP_SD_OFFER) {
            /* Options follow the entries array. We assume the first
             * IPv4-Endpoint option matches. Find the option for
             * simplicity by scanning the whole remainder.
             */
            uint32_t ipv4 = 0; uint16_t port = 0; uint8_t proto = 0x11;
            int options_off = entries_off + entries_len;
            if (options_off + 12 <= len) {
                /* Walk options array */
                int o = 0;
                int opt_len_total = (frame[options_off] << 24) |
                                    (frame[options_off + 1] << 16) |
                                    (frame[options_off + 2] << 8) |
                                    (frame[options_off + 3]);
                int op = options_off + 4;
                while (o + 4 < opt_len_total && op + 4 <= len) {
                    int oe_len = (frame[op] << 8) | frame[op + 1];
                    uint8_t oe_type = frame[op + 2];
                    if (oe_type == 0x04 && oe_len == 12 && op + 12 <= len) {
                        /* IPv4-Endpoint option */
                        memcpy(&ipv4, frame + op + 4, 4);
                        port = (frame[op + 8] << 8) | frame[op + 9];
                        proto = frame[op + 11];
                        break;
                    }
                    op += 4 + oe_len;
                    o += 4 + oe_len;
                }
            }
            record_service(sid, iid, port, major, minor, proto, ipv4);
        }
        e += 16;
    }
}

void someip_on_frame(const uint8_t *frame, uint16_t len, uint8_t dir)
{
    (void)dir;
    parse_sd_offer(frame, len);
}

/* ------------------------------------------------------------------ */
/* Active discovery — send a Find message                               */
/* ------------------------------------------------------------------ */
static int send_sd_message(uint8_t sd_type, uint16_t sid, uint16_t iid)
{
    uint8_t frame[128];
    int p = 0;
    /* Eth: SD multicast 224.224.255.250 -> 01-00-5E-70-FF-FA */
    frame[p++] = 0x01; frame[p++] = 0x00; frame[p++] = 0x5E;
    frame[p++] = 0x70; frame[p++] = 0xFF; frame[p++] = 0xFA;
    frame[p++] = 0x02; frame[p++] = 0x00; frame[p++] = 0x00;
    frame[p++] = 0xFF; frame[p++] = 0xFE; frame[p++] = 0x04;
    /* EtherType IPv4 */
    frame[p++] = 0x08; frame[p++] = 0x00;
    /* IPv4 header (20 bytes) */
    frame[p++] = 0x45;
    frame[p++] = 0x00;
    uint16_t ip_total = 20 + 8 + 16 + 12 + 16;
    frame[p++] = ip_total >> 8; frame[p++] = ip_total & 0xFF;
    frame[p++] = 0x00; frame[p++] = 0x01; /* ID */
    frame[p++] = 0x00; frame[p++] = 0x00; /* flags/frag */
    frame[p++] = 0x40;                    /* TTL 64 */
    frame[p++] = 0x11;                    /* UDP */
    frame[p++] = 0x00; frame[p++] = 0x00; /* checksum 0 */
    frame[p++] = 0xC0; frame[p++] = 0xA8; /* 192.168.0.99 src */
    frame[p++] = 0x00; frame[p++] = 0x63;
    frame[p++] = 0xE0; frame[p++] = 0xE0; /* 224.224.255.250 dst */
    frame[p++] = 0xFF; frame[p++] = 0xFA;
    /* UDP header */
    frame[p++] = 0x77; frame[p++] = 0x12; /* sport 30490 */
    frame[p++] = 0x77; frame[p++] = 0x12; /* dport 30490 */
    uint16_t udp_len = 8 + 16 + 12 + 16;
    frame[p++] = udp_len >> 8; frame[p++] = udp_len & 0xFF;
    frame[p++] = 0x00; frame[p++] = 0x00; /* checksum */
    /* Some/IP header (16 bytes): service_id=0xFFFF, method_id=0x8100 */
    frame[p++] = 0xFF; frame[p++] = 0xFF;
    frame[p++] = 0x81; frame[p++] = 0x00;
    uint32_t someip_len = 8 + 12 + 16;
    frame[p++] = someip_len >> 24; frame[p++] = (someip_len >> 16) & 0xFF;
    frame[p++] = (someip_len >> 8) & 0xFF; frame[p++] = someip_len & 0xFF;
    frame[p++] = 0x00; frame[p++] = 0x00; /* client_id */
    frame[p++] = 0x00; frame[p++] = 0x01; /* session_id */
    frame[p++] = 0x01; /* proto ver */
    frame[p++] = 0x01; /* iface ver */
    frame[p++] = 0x02; /* msg type: notification (SD) */
    frame[p++] = 0x00; /* return code */
    /* SD header: flags, reserved, entries_len */
    frame[p++] = 0xC0; frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00;
    frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00;
    uint32_t elen = 16;
    frame[p++] = elen >> 24; frame[p++] = (elen >> 16) & 0xFF;
    frame[p++] = (elen >> 8) & 0xFF; frame[p++] = elen & 0xFF;
    /* Entry: Find */
    frame[p++] = sd_type;     /* 0 = Find */
    frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00;
    frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00;
    frame[p++] = sid >> 8; frame[p++] = sid & 0xFF;
    frame[p++] = iid >> 8; frame[p++] = iid & 0xFF;
    frame[p++] = 0x00; /* major */
    frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00; /* minor */
    /* Options array (4 bytes length = 0) */
    frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00; frame[p++] = 0x00;
    bridge_inject(frame, p, 0);
    bridge_inject(frame, p, 1);
    return 0;
}

int someip_send_find_all(void)
{
    if (!armed()) return -1;
    return send_sd_message(SOMEIP_SD_FIND, 0xFFFF, 0xFFFF);
}

/* ------------------------------------------------------------------ */
/* Fuzzer                                                                */
/* ------------------------------------------------------------------ */
static uint16_t build_fuzz_frame(uint8_t *frame, uint16_t sid, uint16_t mid,
                                  someip_fuzz_mode_t mode)
{
    int p = 0;
    /* Build a minimal IPv4/UDP/SOME/IP frame to a service */
    /* SA = AxleTap, DA = broadcast */
    memset(frame, 0xFF, 6);
    frame[p++] = 0x02; frame[p++] = 0x00; frame[p++] = 0x00;
    frame[p++] = 0xFF; frame[p++] = 0xFE; frame[p++] = 0x05;
    frame[p++] = 0x08; frame[p++] = 0x00; /* IPv4 */
    /* IPv4 */
    frame[p++] = 0x45; frame[p++] = 0x00;
    uint16_t ip_total = 60;
    frame[p++] = ip_total >> 8; frame[p++] = ip_total & 0xFF;
    frame[p++] = 0x00; frame[p++] = 0x02;
    frame[p++] = 0x00; frame[p++] = 0x00;
    frame[p++] = 0x40; frame[p++] = 0x11;
    frame[p++] = 0x00; frame[p++] = 0x00;
    frame[p++] = 0xC0; frame[p++] = 0xA8; frame[p++] = 0x00; frame[p++] = 0x63;
    frame[p++] = 0xC0; frame[p++] = 0xA8; frame[p++] = 0x00; frame[p++] = 0x01;
    /* UDP */
    uint16_t udp_len = 40;
    frame[p++] = 0x44; frame[p++] = 0x55; /* sport */
    frame[p++] = 0x77; frame[p++] = 0x12; /* dport 30490 */
    frame[p++] = udp_len >> 8; frame[p++] = udp_len & 0xFF;
    frame[p++] = 0x00; frame[p++] = 0x00;
    /* Some/IP header */
    uint16_t payload_len;
    switch (mode) {
    case SOMEIP_FUZZ_HEADER_TRUNC:
        /* Only send 8 of the 16-byte header */
        payload_len = 8;
        break;
    case SOMEIP_FUZZ_OVERSIZE:
        payload_len = 1400;
        break;
    case SOMEIP_FUZZ_BAD_LEN:
        payload_len = 0xFFFF;
        break;
    case SOMEIP_FUZZ_BAD_MSGID:
        payload_len = 16;
        break;
    case SOMEIP_FUZZ_BAD_VER:
        payload_len = 16;
        break;
    case SOMEIP_FUZZ_RANDOM:
        payload_len = 32;
        break;
    default:
        payload_len = 16;
        break;
    }
    uint32_t someip_len = 8 + payload_len;
    /* message_id */
    if (mode == SOMEIP_FUZZ_BAD_MSGID) {
        frame[p++] = 0xFF; frame[p++] = 0xFF; frame[p++] = 0xFF; frame[p++] = 0xFF;
    } else {
        frame[p++] = sid >> 8; frame[p++] = sid & 0xFF;
        frame[p++] = mid >> 8; frame[p++] = mid & 0xFF;
    }
    frame[p++] = someip_len >> 24; frame[p++] = (someip_len >> 16) & 0xFF;
    frame[p++] = (someip_len >> 8) & 0xFF; frame[p++] = someip_len & 0xFF;
    frame[p++] = 0x00; frame[p++] = 0x01; /* client_id */
    frame[p++] = 0x00; frame[p++] = 0x01; /* session_id */
    frame[p++] = (mode == SOMEIP_FUZZ_BAD_VER) ? 0xFF : 0x01;
    frame[p++] = 0x01;
    frame[p++] = SOMEIP_MSG_REQUEST;
    frame[p++] = 0x00; /* return code */
    /* Payload */
    if (mode == SOMEIP_FUZZ_HEADER_TRUNC) {
        return p;  /* truncated header */
    }
    for (int i = 0; i < (int)payload_len && p + 1 < ETH_MAX_FRAME; i++) {
        uint8_t b = (mode == SOMEIP_FUZZ_RANDOM) ? (uint8_t)(p ^ (i * 13)) : 0x41;
        frame[p++] = b;
    }
    return p;
}

int someip_fuzz_start(uint16_t service_id, uint16_t method_id,
                      someip_fuzz_mode_t mode, uint32_t rate_hz)
{
    if (!armed()) return -1;
    g_fuzz_service = service_id;
    g_fuzz_method = method_id;
    g_fuzz_mode = mode;
    g_fuzz_rate = rate_hz;
    g_fuzz_last_ms = now_ms();
    return 0;
}

void someip_fuzz_stop(void)
{
    g_fuzz_mode = SOMEIP_FUZZ_OFF;
}

/* Called from the main scheduler */
void someip_fuzz_poll(void)
{
    if (g_fuzz_mode == SOMEIP_FUZZ_OFF) return;
    if (!armed()) { g_fuzz_mode = SOMEIP_FUZZ_OFF; return; }
    uint32_t period_ms = 1000 / (g_fuzz_rate ? g_fuzz_rate : 1);
    uint32_t t = now_ms();
    if (t - g_fuzz_last_ms < period_ms) return;
    g_fuzz_last_ms = t;
    uint8_t frame[ETH_MAX_FRAME];
    uint16_t len = build_fuzz_frame(frame, g_fuzz_service, g_fuzz_method, g_fuzz_mode);
    if (len > 0) {
        bridge_inject(frame, len, 0);
        bridge_inject(frame, len, 1);
    }
}

const someip_service_t *someip_get_services(int *count)
{
    *count = g_service_count;
    return g_services;
}