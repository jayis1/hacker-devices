/*
 * drivers/nvme_parser.h — NVMe command-set decoder header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_NVME_PARSER_H
#define NVME_PHANTOM_NVME_PARSER_H

#include <stdint.h>
#include <stddef.h>

#define NVME_OP_NAME_LEN  24

/* Decoded 64-byte NVMe submission-queue entry. */
typedef struct {
    uint8_t   opcode;          /* byte 0                       */
    uint8_t   flags;           /* byte 1 (FUSE/PSDT bits)      */
    uint16_t  cid;             /* bytes 2..3                   */
    uint32_t  nsid;            /* bytes 4..7                   */
    uint32_t  cdw0;            /* bytes 8..11 (MPTR for some)  */
    uint64_t  prp1;            /* bytes 16..23                 */
    uint64_t  prp2;            /* bytes 24..31                 */
    uint32_t  cdw10_15[6];     /* bytes 40..63                 */
    uint64_t  slba;            /* cdw10..11 as 64-bit LBA      */
    uint16_t  nlb;             /* cdw12 & 0xFFFF + 1           */
    uint8_t   is_admin;        /* 1 = admin queue, 0 = I/O     */
    uint8_t   is_security;     /* 1 = Security Send/Recv (Opal)*/
    uint8_t   is_firmware;     /* 1 = FW Commit / Download     */
    char      op_name[NVME_OP_NAME_LEN];
} nvme_cmd_t;

/* Decoded 16-byte NVMe completion-queue entry. */
typedef struct {
    uint16_t  cid;             /* bytes 0..1                   */
    uint16_t  status;          /* bytes 2..3 (phase + SC/SCT)  */
    uint16_t  sqhd;            /* bytes 4..5                   */
    uint16_t  sqid;            /* bytes 6..7                   */
    uint32_t  cdw0;            /* bytes 8..11                  */
    uint8_t   sc;              /* status code                  */
    uint8_t   sct;             /* status code type             */
    uint8_t   dnr;             /* do-not-retry                 */
} nvme_cpl_t;

void nvme_decode_sq(const uint8_t *sq, nvme_cmd_t *out);
void nvme_decode_cq(const uint8_t *cq, nvme_cpl_t *out);
void nvme_format_cmd(const nvme_cmd_t *cmd, char *buf, size_t buflen);
void nvme_format_cpl(const nvme_cpl_t *cpl, char *buf, size_t buflen);

#endif /* NVME_PHANTOM_NVME_PARSER_H */