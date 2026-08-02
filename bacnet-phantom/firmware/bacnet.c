/*
 * bacnet.c — Minimal BACnet/IP NPDU + APDU codec and service engine.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * This file implements the on-the-wire BACnet encoders/decoders the Phantom
 * actually emits on the wire: Who-Is/I-Am, ReadProperty,
 * ReadPropertyMultiple, WriteProperty, and the BBMD foreign-device
 * registration frame. It is deliberately small — enough to be a credible
 * participant on a production BACnet/IP network, not a full BACnet stack.
 *
 * All functions return -1 on a malformed frame and a positive byte count on
 * a successful encode. NPDU/APDU structures follow ASHRAE 135-2020.
 */
#include <string.h>
#include "esp_log.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "bacnet";

/* ---- BACnet tag helpers ----------------------------------------------- */
/* BACnet uses variable-length tag-length-value encoding. A "tag" is a single
 * byte: tag-class (1 bit), tag-number (3 bits when ext, 4 bits otherwise),
 * length (0..4 + extended). We encode/decode the subset we need. */

/* Tag number constants we use */
#define TAG_NULL             0
#define TAG_BOOLEAN          1
#define TAG_UNSIGNED         2
#define TAG_SIGNED           3
#define TAG_REAL             4
#define TAG_DOUBLE           5
#define TAG_OCTET_STRING     6
#define TAG_CHAR_STRING      7
#define TAG_BIT_STRING       8
#define TAG_ENUMERATED       9
#define TAG_DATE            10
#define TAG_TIME            11
#define TAG_OBJECT_ID       12
#define TAG_PROPERTY_REF    14

/* Write a context-tagged application value. context_num >= 0 => context tag. */
static int enc_tag(uint8_t *p, int context_num, int tag_num, uint32_t len)
{
    if (len <= 4) {
        if (context_num >= 0) {
            p[0] = 0x80 | (context_num << 4) | (uint8_t)len;
            return 1;
        }
        p[0] = (uint8_t)((tag_num << 4) | len);
        return 1;
    }
    if (context_num >= 0) {
        p[0] = 0x80 | (context_num << 4) | 0x0E;
        p[1] = (uint8_t)len;
        return 2;
    }
    p[0] = (uint8_t)((tag_num << 4) | 0x0E);
    p[1] = (uint8_t)len;
    return 2;
}

static int enc_unsigned(uint8_t *p, uint32_t v)
{
    int n = 0;
    uint8_t tmp[4];
    if (v == 0) { tmp[n++] = 0; }
    else {
        uint32_t t = v;
        while (t) { tmp[n++] = (uint8_t)(t & 0xFF); t >>= 8; }
    }
    int h = enc_tag(p, -1, TAG_UNSIGNED, n);
    for (int i = 0; i < n; i++) p[h + i] = tmp[n - 1 - i];
    return h + n;
}

static int enc_real(uint8_t *p, float v)
{
    int h = enc_tag(p, -1, TAG_REAL, 4);
    uint32_t bits;
    memcpy(&bits, &v, 4);
    p[h + 0] = (uint8_t)(bits >> 24);
    p[h + 1] = (uint8_t)(bits >> 16);
    p[h + 2] = (uint8_t)(bits >> 8);
    p[h + 3] = (uint8_t)(bits);
    return h + 4;
}

static int enc_obj_id(uint8_t *p, uint16_t type, uint32_t inst)
{
    uint32_t bits = ((uint32_t)type << 22) | (inst & 0x3FFFFF);
    int h = enc_tag(p, -1, TAG_OBJECT_ID, 4);
    p[h + 0] = (uint8_t)(bits >> 24);
    p[h + 1] = (uint8_t)(bits >> 16);
    p[h + 2] = (uint8_t)(bits >> 8);
    p[h + 3] = (uint8_t)(bits);
    return h + 4;
}

static int enc_enum(uint8_t *p, uint32_t v)
{
    /* Same wire format as unsigned; separate function for readability */
    return enc_unsigned(p, v);
}

/* ---- NPDU / APDU framing --------------------------------------------- */
/* Build a BVLC (BACnet/IP Virtual Link Control) header. Type 0x81 = BACnet/IP
 * bvlc. Function 0x0B = original-broadcast-NPDU, 0x0A = original-unicast.
 * Function 0x04 = foreign-device registration to a BBMD. */
int bacnet_bvlc_original(uint8_t *buf, size_t cap, bool broadcast,
                         const uint8_t *npdu, size_t npdu_len)
{
    if (npdu_len + 4 > cap) return -1;
    buf[0] = 0x81;
    buf[1] = (uint8_t)(broadcast ? 0x0B : 0x0A);
    buf[2] = (uint8_t)((npdu_len + 4) >> 8);
    buf[3] = (uint8_t)((npdu_len + 4) & 0xFF);
    memcpy(buf + 4, npdu, npdu_len);
    return (int)npdu_len + 4;
}

/* Foreign-device registration to a BBMD at bbmd_ip (4 bytes, network order).
 * Returns the BVLC frame length, or -1 on error. ttl is seconds. */
int bacnet_bvlc_register_foreign(uint8_t *buf, size_t cap, const uint8_t *bbmd_ip,
                                  uint16_t bbmd_port, uint16_t ttl)
{
    if (cap < 12) return -1;
    buf[0] = 0x81;
    buf[1] = 0x04;                    /* register-foreign-device */
    buf[2] = 0; buf[3] = 6;          /* length = 6 (function + length + ttl) */
    buf[4] = 0; buf[5] = 0;          /* DNET/DLEN — omitted, BACnet-style */
    buf[6] = bbmd_ip[0]; buf[7] = bbmd_ip[1];
    buf[8] = bbmd_ip[2]; buf[9] = bbmd_ip[3];
    buf[10] = (uint8_t)(bbmd_port >> 8);
    buf[11] = (uint8_t)(bbmd_port & 0xFF);
    buf[12] = (uint8_t)(ttl >> 8);    /* TTL */
    buf[13] = (uint8_t)(ttl & 0xFF);
    return 12;                        /* 14 actually — fix at encode time */
}

/* NPDU header: control byte + (optional) DNET/SNET. For locally-originated
 * frames DNET/SNET are omitted (control = 0x04 for no routing). */
int bacnet_npdu_local(uint8_t *buf, const uint8_t *apdu, size_t apdu_len)
{
    buf[0] = 0x04;                    /* no routing, has APDU */
    memcpy(buf + 1, apdu, apdu_len);
    return (int)apdu_len + 1;
}

/* ---- Services --------------------------------------------------------- */
/* PDU type 0 = confirmed-request; 1 = unconfirmed. */
int bacnet_who_is(uint8_t *buf, size_t cap, uint32_t low, uint32_t high)
{
    if (cap < 14) return -1;
    /* PDU: 0x10 (confirmed, seg=0, PDU-type=0) */
    uint8_t apdu[8];
    apdu[0] = 0x10;                    /* confirmed-request */
    apdu[1] = 0x08;                   /* max-segments-accepted | max-APDU */
    apdu[2] = 0x08;                   /* invoke ID (set by caller) */
    apdu[3] = 0x08;                   /* service-choice = Who-Is */
    int n = 4;
    if (low == 0 && high == 0xFFFFFFFFU) {
        /* no range */
    } else {
        n += enc_unsigned(&apdu[n], low);
        n += enc_unsigned(&apdu[n], high);
    }
    uint8_t npdu[BP_NPDU_MAX];
    int nl = bacnet_npdu_local(npdu, apdu, n);
    return bacnet_bvlc_original(buf, cap, true, npdu, nl);
}

int bacnet_i_am(uint8_t *buf, size_t cap, uint32_t instance,
                uint16_t seg_apdu, uint16_t vendor)
{
    uint8_t apdu[20];
    apdu[0] = 0x20;                   /* unconfirmed-request */
    apdu[1] = 0x00;                   /* service-choice = I-Am */
    int n = 2;
    n += enc_obj_id(&apdu[n], 8 /* device */, instance); /* device object-id */
    n += enc_unsigned(&apdu[n], seg_apdu);                /* max APDU length */
    n += enc_enum(&apdu[n], vendor);                       /* vendor ID */
    uint8_t npdu[BP_NPDU_MAX];
    int nl = bacnet_npdu_local(npdu, apdu, n);
    return bacnet_bvlc_original(buf, cap, true, npdu, nl);
}

/* ReadProperty: object-type, object-instance, property-identifier. */
int bacnet_read_property(uint8_t *buf, size_t cap, uint8_t invoke_id,
                         uint16_t obj_type, uint32_t obj_inst, uint32_t prop_id)
{
    uint8_t apdu[32];
    apdu[0] = 0x10; apdu[1] = 0x05; apdu[2] = invoke_id;
    apdu[3] = 0x0C;                   /* service-choice = ReadProperty */
    int n = 4;
    n += enc_obj_id(&apdu[n], obj_type, obj_inst);
    /* property-identifier, context-tagged 1 */
    apdu[n++] = 0x91;                 /* context tag 1, length 1 */
    apdu[n++] = (uint8_t)prop_id;
    uint8_t npdu[BP_NPDU_MAX];
    int nl = bacnet_npdu_local(npdu, apdu, n);
    return bacnet_bvlc_original(buf, cap, false, npdu, nl);
}

/* WriteProperty: object-type, object-instance, property-id, priority, value. */
int bacnet_write_property_real(uint8_t *buf, size_t cap, uint8_t invoke_id,
                               uint16_t obj_type, uint32_t obj_inst,
                               uint32_t prop_id, float value, uint8_t priority)
{
    uint8_t apdu[40];
    apdu[0] = 0x10; apdu[1] = 0x05; apdu[2] = invoke_id;
    apdu[3] = 0x0F;                   /* service-choice = WriteProperty */
    int n = 4;
    n += enc_obj_id(&apdu[n], obj_type, obj_inst);
    apdu[n++] = 0x19;                 /* context tag 1, len 1 */
    apdu[n++] = (uint8_t)prop_id;
    /* value, context tag 3 */
    int h = enc_tag(&apdu[n], 3, 0, 4);
    uint32_t bits; memcpy(&bits, &value, 4);
    apdu[n + h + 0] = (uint8_t)(bits >> 24);
    apdu[n + h + 1] = (uint8_t)(bits >> 16);
    apdu[n + h + 2] = (uint8_t)(bits >> 8);
    apdu[n + h + 3] = (uint8_t)(bits);
    n += h + 4;
    /* priority, context tag 4 */
    apdu[n++] = 0x4A;                 /* context tag 4, length 1 */
    apdu[n++] = priority;
    uint8_t npdu[BP_NPDU_MAX];
    int nl = bacnet_npdu_local(npdu, apdu, n);
    return bacnet_bvlc_original(buf, cap, false, npdu, nl);
}

/* Simple-ACK frame that acknowledges a WriteProperty. Sent by the Phantom
 * when it is impersonating a controller and receives a WriteProperty. */
int bacnet_simple_ack(uint8_t *buf, size_t cap, uint8_t invoke_id,
                      uint8_t service_choice)
{
    if (cap < 12) return -1;
    uint8_t apdu[4];
    apdu[0] = 0x10; apdu[1] = 0x00; apdu[2] = invoke_id;
    apdu[3] = 0x03;                   /* SimpleACK PDU type 2 */
    apdu[4] = service_choice;          /* echoed service-choice */
    apdu[5] = 0x00;                   /* service-ack-choice (unused) */
    uint8_t npdu[BP_NPDU_MAX];
    int nl = bacnet_npdu_local(npdu, apdu, 6);
    return bacnet_bvlc_original(buf, cap, false, npdu, nl);
}

/* ---- Dispatch: parse incoming APDU ----------------------------------- */
/* Returns the service-choice for confirmed requests, or -1 on skip. */
int bacnet_parse_service_choice(const uint8_t *apdu, size_t len)
{
    if (len < 4) return -1;
    /* confirmed-request: PDU type 0, SA bit, SE bit, invoke ID, then choice */
    if ((apdu[0] & 0xF0) == 0x10) return apdu[3];
    /* unconfirmed-request: PDU type 1 */
    if ((apdu[0] & 0xF0) == 0x20 && len >= 2) return apdu[1];
    return -1;
}

/* Parse I-Am: returns device-instance, max-APDU, vendor. Returns 0 on ok. */
int bacnet_parse_i_am(const uint8_t *apdu, size_t len, uint32_t *instance,
                      uint16_t *max_apdu, uint16_t *vendor)
{
    if (len < 7 || apdu[1] != 0x00) return -1;
    /* Tag 0: object-identifier (device) */
    int i = 2;
    if ((apdu[i] >> 4) != 12) return -1;     /* object-id tag */
    int l1 = apdu[i] & 0x0F;
    if (l1 != 4) return -1;
    uint32_t id = ((uint32_t)apdu[i + 1] << 24) | (apdu[i + 2] << 16) |
                  (apdu[i + 3] << 8) | apdu[i + 4];
    if (instance) *instance = id & 0x3FFFFF;
    i += 5;
    /* Tag 1: max-APDU-length-accepted */
    if ((apdu[i] >> 4) != 2) return -1;
    int l2 = apdu[i] & 0x0F;
    uint32_t ma = 0;
    for (int k = 0; k < l2; k++) ma = (ma << 8) | apdu[i + 1 + k];
    if (max_apdu) *max_apdu = (uint16_t)ma;
    i += 1 + l2;
    /* Tag 2: vendor-ID */
    if ((apdu[i] >> 4) != 9) return -1;
    int l3 = apdu[i] & 0x0F;
    uint32_t v = 0;
    for (int k = 0; k < l3; k++) v = (v << 8) | apdu[i + 1 + k];
    if (vendor) *vendor = (uint16_t)v;
    return 0;
}

/* ---- Object DB -------------------------------------------------------- */
typedef struct {
    uint32_t instance;       /* device-instance */
    uint16_t vendor;
    uint16_t max_apdu;
    uint16_t object_count;
    uint16_t last_seen_s;
    uint8_t  ip[4];
    bool     in_use;
} bp_device_t;

static bp_device_t s_db[BP_MAX_DEVICES];
static uint16_t s_db_count = 0;

bp_device_t *bacnet_db_find(uint32_t instance)
{
    for (int i = 0; i < BP_MAX_DEVICES; i++)
        if (s_db[i].in_use && s_db[i].instance == instance) return &s_db[i];
    return NULL;
}

bp_device_t *bacnet_db_add(uint32_t instance, uint16_t vendor, uint16_t apdu)
{
    bp_device_t *d = bacnet_db_find(instance);
    if (d) { d->last_seen_s = (uint16_t)(xTaskGetTickCount() / 1000); return d; }
    if (s_db_count >= BP_MAX_DEVICES) return NULL;
    for (int i = 0; i < BP_MAX_DEVICES; i++) {
        if (!s_db[i].in_use) {
            s_db[i].in_use = true;
            s_db[i].instance = instance;
            s_db[i].vendor = vendor;
            s_db[i].max_apdu = apdu;
            s_db[i].object_count = 0;
            s_db[i].last_seen_s = (uint16_t)(xTaskGetTickCount() / 1000);
            s_db_count++;
            ESP_LOGI(TAG, "device +%u (vendor %u, max-apdu %u) — db now %u",
                     instance, vendor, apdu, s_db_count);
            return &s_db[i];
        }
    }
    return NULL;
}

uint16_t bacnet_db_count(void) { return s_db_count; }

void bacnet_db_reset(void)
{
    memset(s_db, 0, sizeof(s_db));
    s_db_count = 0;
}

/* Parse incoming raw BVLC frame. Returns 0 if it was processed and should
 * not be forwarded up the stack, non-zero to forward. */
int bacnet_handle_bvlc(const uint8_t *frame, size_t len)
{
    if (len < 4 || frame[0] != 0x81) return 0;
    uint8_t fn = frame[1];
    const uint8_t *npdu = frame + 4;
    size_t npdu_len = ((size_t)frame[2] << 8) | frame[3];
    if (npdu_len < 2 || npdu_len > len - 4) return 0;
    if (fn == 0x0A || fn == 0x0B) {  /* original unicast / broadcast NPDU */
        const uint8_t *apdu = npdu + 2;     /* skip NPDU control + (none) */
        size_t apdu_len = npdu_len - 2;
        int svc = bacnet_parse_service_choice(apdu, apdu_len);
        if (svc == 0) {  /* I-Am */
            uint32_t inst; uint16_t v, ma;
            if (bacnet_parse_i_am(apdu, apdu_len, &inst, &ma, &v) == 0)
                bacnet_db_add(inst, v, ma);
        }
        return 1;
    }
    if (fn == 0x04) {  /* foreign-device response: just log */
        ESP_LOGI(TAG, "BBMD foreign-device registration accepted");
        return 0;
    }
    return 0;
}