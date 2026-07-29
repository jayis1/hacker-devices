/*
 * drivers/opal_probe.h — TCG Opal security command header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_OPAL_PROBE_H
#define NVME_PHANTOM_OPAL_PROBE_H

#include <stdint.h>

typedef enum {
    OPAL_TOK_TYPE_INT,
    OPAL_TOK_TYPE_BYTES,
    OPAL_TOK_TYPE_CTRL,
    OPAL_TOK_TYPE_END,
} opal_tok_type_t;

typedef struct {
    opal_tok_type_t type;
    int32_t         value;
    const uint8_t  *data;
    uint16_t        data_len;
} opal_token_t;

int opal_build_security_send(uint8_t sq[64], uint8_t nsid,
                             const uint8_t *payload, uint16_t payload_len);
int opal_build_security_recv(uint8_t sq[64], uint8_t nsid, uint16_t alloc_len);
int opal_build_unlock(uint8_t *payload, uint16_t cap, const char *password);
int opal_build_revert(uint8_t *payload, uint16_t cap, const char *admin_password);
int opal_build_genkey(uint8_t *payload, uint16_t cap);
int opal_parse_response(const uint8_t *payload, uint16_t len,
                        opal_token_t *tokens, uint16_t max_tokens);

#endif /* NVME_PHANTOM_OPAL_PROBE_H */