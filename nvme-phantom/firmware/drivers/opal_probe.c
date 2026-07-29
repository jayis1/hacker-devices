/*
 * drivers/opal_probe.c — TCG Opal security command encoder / decoder
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * TCG Opal SSC 2.01 security commands are carried inside NVMe Admin
 * Security Send (opcode 0x7D / 0x81) and Security Receive (opcode 0x7E /
 * 0x82) commands.  The payload is a ComPacket containing one or more
 * Packets, each containing a SubPacket, each containing a method call or
 * response in the Opal tokenized format.
 *
 * This driver provides:
 *   - opal_build_security_send()  — build a Security Send SQ entry with an
 *     Opal payload for a given method (e.g. ENTER_LEARNED_MODE, UNLOCK,
 *     REVERT, GENKEY).
 *   - opal_parse_security_recv()  — parse a Security Receive response payload
 *     into a list of Opal tokens.
 *   - opal_unlock()               — high-level helper: ENTER_LEARNED_MODE
 *     with the user's password to unlock a locked SED.
 *   - opal_revert()               — high-level helper: REVERT the SED to
 *     factory defaults (wipes the DEK — use with extreme caution).
 *
 * Reference: TCG Storage Opal SSC Specification 2.01, sections 3-5.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- Opal INVOCATION IDs and UIDs -------------------------------------- */

/* Opal UID references (short-form 8-byte UIDs) */
static const uint8_t OPAL_UID_ADMIN[]      = {0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x01};
static const uint8_t OPAL_UID_USER[]       = {0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x06};
static const uint8_t OPAL_UID_ADMINSP[]    = {0x00,0x00,0x02,0x05,0x00,0x00,0x00,0x01};
static const uint8_t OPAL_UID_LOCKINGSP[]  = {0x00,0x00,0x02,0x05,0x00,0x00,0x00,0x02};
static const uint8_t OPAL_UID_ENTER_PS[]   = {0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x06};

/* Opal method IDs */
static const uint8_t OPAL_METHOD_ENTER_LM[]   = {0x00,0x00,0x00,0x09,0x00,0x01,0x00,0x01};
static const uint8_t OPAL_METHOD_REVERT[]     = {0x00,0x00,0x00,0x09,0x00,0x02,0x00,0x02};
static const uint8_t OPAL_METHOD_GENKEY[]     = {0x00,0x00,0x00,0x09,0x00,0x02,0x00,0x04};
static const uint8_t OPAL_METHOD_REVERT_SP[]  = {0x00,0x00,0x00,0x09,0x00,0x02,0x00,0x03};

/* Opal token atoms */
#define OPAL_TOK_CALL         0xF0
#define OPAL_TOK_ENDLIST      0xF1
#define OPAL_TOK_ENDOFDATA    0xF2
#define OPAL_TOK_ENDSESSION   0xF3
#define OPAL_TOK_STARTLIST    0xF1   /* NOTE: STARTLIST and ENDLIST share 0xF1
                                       in Opal SSC 2.01 — distinguished by
                                       nesting context. */
#define OPAL_TOK_STARTNAME    0xF4
#define OPAL_TOK_ENDNAME      0xF5
#define OPAL_TOK_ATOM_INT_1   0x81   /* 1-byte signed integer            */
#define OPAL_TOK_ATOM_INT_4   0x84
#define OPAL_TOK_ATOM_BYTES   0xA0   /* byte string (length-prefixed)    */

/* ---- Opal token writer ------------------------------------------------- */

typedef struct {
    uint8_t *buf;
    uint16_t pos;
    uint16_t cap;
} opal_writer_t;

static void opal_putc(opal_writer_t *w, uint8_t b)
{
    if (w->pos < w->cap) w->buf[w->pos++] = b;
}

static void opal_put_bytes(opal_writer_t *w, const uint8_t *src, uint16_t n)
{
    for (uint16_t i = 0; i < n; i++) opal_putc(w, src[i]);
}

static void opal_put_uid(opal_writer_t *w, const uint8_t uid[8])
{
    opal_putc(w, OPAL_TOK_ATOM_BYTES);
    opal_putc(w, 8);                            /* length = 8 */
    opal_put_bytes(w, uid, 8);
}

static void opal_put_int4(opal_writer_t *w, int32_t v)
{
    opal_putc(w, OPAL_TOK_ATOM_INT_4);
    opal_putc(w, (uint8_t)(v & 0xFF));
    opal_putc(w, (uint8_t)(v >> 8));
    opal_putc(w, (uint8_t)(v >> 16));
    opal_putc(w, (uint8_t)(v >> 24));
}

static void opal_put_int1(opal_writer_t *w, int8_t v)
{
    opal_putc(w, OPAL_TOK_ATOM_INT_1);
    opal_putc(w, (uint8_t)v);
}

static void opal_put_string(opal_writer_t *w, const char *s)
{
    uint16_t len = (uint16_t)strlen(s);
    opal_putc(w, OPAL_TOK_ATOM_BYTES);
    if (len < 64) {
        opal_putc(w, (uint8_t)len);
    } else {
        opal_putc(w, 0xFF);                     /* extended length */
        opal_putc(w, (uint8_t)(len & 0xFF));
        opal_putc(w, (uint8_t)(len >> 8));
    }
    opal_put_bytes(w, (const uint8_t *)s, len);
}

/* ---- Build a ComPacket around a method call ---------------------------- */

static uint16_t opal_build_compacket(opal_writer_t *w, uint8_t *out, uint16_t cap)
{
    /* ComPacket header (TPer level):
     *   uint32 reserved
     *   uint8  ComID (0x0001 for Admin SP, 0x0002 for Locking SP)
     *   uint32 ComPacketLength
     * followed by a Packet:
     *   uint32 TPERSessionNumber
     *   uint32 PacketLength
     * followed by a SubPacket:
     *   uint8  Kind (0=method, 1=resp)
     *   uint32 SubPacketLength
     * followed by the method-call tokens. */
    uint16_t body_len = w->pos;
    /* For simplicity we build the ComPacket/Packet/SubPacket wrappers
     * inline.  In a full implementation each layer's length is computed
     * and back-patched. */
    uint16_t subpacket_len = body_len + 5;
    uint16_t packet_len    = subpacket_len + 8;
    uint16_t compacket_len = packet_len + 8;
    uint16_t total = compacket_len;
    if (total > cap) return 0;
    uint16_t p = 0;
    /* ComPacket header */
    out[p++]=0; out[p++]=0; out[p++]=0; out[p++]=0;        /* reserved */
    out[p++]=0; out[p++]=0x01;                             /* ComID = Admin */
    out[p++]=(uint8_t)(compacket_len >> 8); out[p++]=(uint8_t)compacket_len;
    /* Packet header */
    out[p++]=0; out[p++]=0; out[p++]=0; out[p++]=0;        /* session # */
    out[p++]=(uint8_t)(packet_len >> 8); out[p++]=(uint8_t)packet_len;
    /* SubPacket header */
    out[p++]=0;                                            /* kind = method */
    out[p++]=(uint8_t)(subpacket_len >> 24); out[p++]=(uint8_t)(subpacket_len >> 16);
    out[p++]=(uint8_t)(subpacket_len >> 8);  out[p++]=(uint8_t)subpacket_len;
    /* body */
    memcpy(out + p, w->buf, body_len);
    p += body_len;
    return p;
}

/* ---- Public API: build a Security Send SQ entry ------------------------ */

int opal_build_security_send(uint8_t sq[64], uint8_t nsid,
                             const uint8_t *payload, uint16_t payload_len)
{
    memset(sq, 0, 64);
    sq[0] = 0x7D;                               /* Security Send opcode */
    sq[4] = nsid;                               /* NSID                 */
    /* CDW10: security protocol = 0x02 (Opal), protocol-specific = 0x01 */
    uint32_t cdw10 = 0x02 | (0x01 << 24);
    for (int i = 0; i < 4; i++) sq[40 + i] = (uint8_t)(cdw10 >> (i*8));
    /* CDW11: length in 4-byte words, bits 31:30 = translength = 0 */
    uint32_t cdw11 = (payload_len / 4) << 2;
    for (int i = 0; i < 4; i++) sq[44 + i] = (uint8_t)(cdw11 >> (i*8));
    /* PRP1 = pointer to payload buffer (caller-provided; encoded as 0
     * here — the FPGA fills in the actual DMA address at inject time). */
    return 0;
}

int opal_build_security_recv(uint8_t sq[64], uint8_t nsid, uint16_t alloc_len)
{
    memset(sq, 0, 64);
    sq[0] = 0x7E;                               /* Security Receive opcode */
    sq[4] = nsid;
    uint32_t cdw10 = 0x02 | (0x01 << 24);       /* Opal protocol */
    for (int i = 0; i < 4; i++) sq[40 + i] = (uint8_t)(cdw10 >> (i*8));
    uint32_t cdw11 = (alloc_len / 4) << 2;
    for (int i = 0; i < 4; i++) sq[44 + i] = (uint8_t)(cdw11 >> (i*8));
    return 0;
}

/* ---- High-level Opal operations ---------------------------------------- */

/* Build an ENTER_LEARNED_MODE (UNLOCK) method call for the Admin SP. */
int opal_build_unlock(uint8_t *payload, uint16_t cap, const char *password)
{
    opal_writer_t w = { payload, 0, cap };
    /* Call header: [CALL][invocation_id][method_UID][STARTLIST ... ENDLIST] */
    opal_putc(&w, OPAL_TOK_CALL);
    opal_put_int4(&w, 1);                       /* invocation ID */
    opal_put_uid(&w, OPAL_METHOD_ENTER_LM);
    opal_putc(&w, OPAL_TOK_STARTLIST);
    /* [STARTNAME "hostChallenge" password ENDNAME] */
    opal_putc(&w, OPAL_TOK_STARTNAME);
    opal_put_int1(&w, 0);                       /* name ref = hostChallenge */
    opal_put_string(&w, password);
    opal_putc(&w, OPAL_TOK_ENDNAME);
    /* [STARTNAME "hostSignAuthority" UID_user ENDNAME] */
    opal_putc(&w, OPAL_TOK_STARTNAME);
    opal_put_int1(&w, 1);                       /* name ref = hostSignAuthority */
    opal_put_uid(&w, OPAL_UID_USER);
    opal_putc(&w, OPAL_TOK_ENDNAME);
    opal_putc(&w, OPAL_TOK_ENDLIST);
    opal_putc(&w, OPAL_TOK_ENDOFDATA);
    opal_putc(&w, OPAL_TOK_ENDSESSION);
    /* Wrap in ComPacket */
    return opal_build_compacket(&w, payload, cap);
}

/* Build a REVERT (factory reset — wipes DEK) method call for the Admin SP. */
int opal_build_revert(uint8_t *payload, uint16_t cap, const char *admin_password)
{
    opal_writer_t w = { payload, 0, cap };
    opal_putc(&w, OPAL_TOK_CALL);
    opal_put_int4(&w, 2);
    opal_put_uid(&w, OPAL_METHOD_REVERT);
    opal_putc(&w, OPAL_TOK_STARTLIST);
    opal_putc(&w, OPAL_TOK_STARTNAME);
    opal_put_int1(&w, 0);
    opal_put_string(&w, admin_password);
    opal_putc(&w, OPAL_TOK_ENDNAME);
    opal_putc(&w, OPAL_TOK_ENDLIST);
    opal_putc(&w, OPAL_TOK_ENDOFDATA);
    opal_putc(&w, OPAL_TOK_ENDSESSION);
    return opal_build_compacket(&w, payload, cap);
}

/* Build a GENKEY (rekey) method call for the Locking SP. */
int opal_build_genkey(uint8_t *payload, uint16_t cap)
{
    opal_writer_t w = { payload, 0, cap };
    opal_putc(&w, OPAL_TOK_CALL);
    opal_put_int4(&w, 3);
    opal_put_uid(&w, OPAL_METHOD_GENKEY);
    opal_putc(&w, OPAL_TOK_STARTLIST);
    opal_putc(&w, OPAL_TOK_ENDLIST);
    opal_putc(&w, OPAL_TOK_ENDOFDATA);
    opal_putc(&w, OPAL_TOK_ENDSESSION);
    return opal_build_compacket(&w, payload, cap);
}

/* ---- Opal response parser ---------------------------------------------- */

/* Parse a Security Receive response into a flat token list.  Returns the
 * number of tokens parsed, or -1 on error. */
int opal_parse_response(const uint8_t *payload, uint16_t len,
                        opal_token_t *tokens, uint16_t max_tokens)
{
    /* Skip ComPacket (8B) + Packet (8B) + SubPacket (5B) headers = 21B */
    if (len < 21) return -1;
    uint16_t p = 21;
    int count = 0;
    while (p < len && count < max_tokens) {
        uint8_t t = payload[p++];
        if (t == OPAL_TOK_ENDOFDATA || t == OPAL_TOK_ENDSESSION) {
            tokens[count].type = OPAL_TOK_TYPE_END;
            count++;
            break;
        }
        if (t >= 0x80 && t <= 0x8F) {
            /* integer atom: length = t & 0x0F bytes */
            uint8_t ilen = t & 0x0F;
            if (p + ilen > len) return -1;
            int32_t val = 0;
            for (uint8_t i = 0; i < ilen; i++)
                val = (val << 8) | payload[p++];
            tokens[count].type = OPAL_TOK_TYPE_INT;
            tokens[count].value = val;
            count++;
        } else if (t == 0xA0 || t == 0xA1) {
            /* byte string atom */
            uint16_t slen;
            if (payload[p] < 64) {
                slen = payload[p++];
            } else {
                p++;                            /* 0xFF extended marker */
                slen = (uint16_t)payload[p] | ((uint16_t)payload[p+1] << 8);
                p += 2;
            }
            if (p + slen > len) return -1;
            tokens[count].type = OPAL_TOK_TYPE_BYTES;
            tokens[count].data = payload + p;
            tokens[count].data_len = slen;
            p += slen;
            count++;
        } else {
            tokens[count].type = OPAL_TOK_TYPE_CTRL;
            tokens[count].value = t;
            count++;
        }
    }
    return count;
}