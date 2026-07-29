/*
 * drivers/nvme_parser.c — NVMe command-set decoder
 *
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Decodes 64-byte NVMe submission-queue entries and 16-byte completion-queue
 * entries into a human-readable / loggable structure.  Used by the OLED UI
 * and by sd_pcap.c for PCAP-NG annotations.
 *
 * Reference: NVM Express 1.4 specification, sections 5.x (Admin) and 6.x
 * (NVM command set).  TCG Opal commands are carried inside Admin Security
 * Send / Security Receive (opcodes 0x81 / 0x82) and are decoded by
 * opal_probe.c.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- NVMe opcode tables ------------------------------------------------ */

static const char *admin_op_name(uint8_t op)
{
    switch (op) {
    case 0x00: return "Delete I/O SQ";
    case 0x01: return "Create I/O SQ";
    case 0x02: return "Get Log Page";
    case 0x04: return "Delete I/O CQ";
    case 0x05: return "Create I/O CQ";
    case 0x06: return "Identify";
    case 0x08: return "Abort";
    case 0x09: return "Set Features";
    case 0x0A: return "Get Features";
    case 0x0C: return "Asynchronous Event";
    case 0x0D: return "Namespace Mgmt";
    case 0x0E: return "Firmware Commit";
    case 0x10: return "Firmware Image Download";
    case 0x11: return "Device Self-test";
    case 0x12: return "Namespace Attach";
    case 0x14: return "Keep Alive";
    case 0x15: return "Directive Send";
    case 0x16: return "Directive Receive";
    case 0x19: return "Virtualization Mgmt";
    case 0x1A: return "NVMe-MI Send";
    case 0x1B: return "NVMe-MI Receive";
    case 0x1C: return "Capability";
    case 0x1D: return "Doorbell Buffer Config";
    case 0x7C: return "Format NVM";
    case 0x7D: return "Security Send";
    case 0x7E: return "Security Receive";
    case 0x81: return "Security Send (alt)";   /* some vendors use 0x81 */
    case 0x82: return "Security Receive (alt)";
    case 0x84: return "Sanitize";
    case 0x86: return "Get LBA Status";
    default:   return "Admin ??";
    }
}

static const char *nvm_op_name(uint8_t op)
{
    switch (op) {
    case 0x00: return "Flush";
    case 0x01: return "Write";
    case 0x02: return "Read";
    case 0x04: return "Write Uncorrectable";
    case 0x05: return "Compare";
    case 0x08: return "Write Zeroes";
    case 0x09: return "Dataset Management";
    case 0x0A: return "Verify";
    case 0x0B: return "Reservation Register";
    case 0x0C: return "Reservation Report";
    case 0x0D: return "Reserve";
    case 0x0E: return "Release";
    default:   return "NVM ??";
    }
}

/* ---- Public decoder ---------------------------------------------------- */

/* Decode a 64-byte submission-queue entry (raw bytes, little-endian). */
void nvme_decode_sq(const uint8_t *sq, nvme_cmd_t *out)
{
    memset(out, 0, sizeof(*out));
    out->opcode    = sq[0];
    out->flags     = sq[1];
    out->cid       = (uint16_t)sq[2] | ((uint16_t)sq[3] << 8);
    out->nsid      = (uint32_t)sq[4] | ((uint32_t)sq[5] << 8) |
                     ((uint32_t)sq[6] << 16) | ((uint32_t)sq[7] << 24);
    out->cdw0      = (uint32_t)sq[8]  | ((uint32_t)sq[9]  << 8) |
                     ((uint32_t)sq[10] << 16) | ((uint32_t)sq[11] << 24);
    /* PRP1 = bytes 16..23, PRP2 = bytes 24..31 */
    out->prp1 = (uint64_t)sq[16] | ((uint64_t)sq[17] << 8) |
                ((uint64_t)sq[18] << 16) | ((uint64_t)sq[19] << 24) |
                ((uint64_t)sq[20] << 32) | ((uint64_t)sq[21] << 40) |
                ((uint64_t)sq[22] << 48) | ((uint64_t)sq[23] << 56);
    out->prp2 = (uint64_t)sq[24] | ((uint64_t)sq[25] << 8) |
                ((uint64_t)sq[26] << 16) | ((uint64_t)sq[27] << 24) |
                ((uint64_t)sq[28] << 32) | ((uint64_t)sq[29] << 40) |
                ((uint64_t)sq[30] << 48) | ((uint64_t)sq[31] << 56);
    /* CDW10..15 = bytes 40..63 */
    for (int i = 0; i < 6; i++) {
        out->cdw10_15[i] = (uint32_t)sq[40 + i*4] |
                           ((uint32_t)sq[41 + i*4] << 8) |
                           ((uint32_t)sq[42 + i*4] << 16) |
                           ((uint32_t)sq[43 + i*4] << 24);
    }
    /* SLBA (for Read/Write) = cdw10_11 as a 64-bit LBA */
    out->slba = (uint64_t)out->cdw10_15[0] | ((uint64_t)out->cdw10_15[1] << 32);
    /* NLB (for Read/Write) = cdw12 & 0xFFFF (+1) */
    out->nlb = (uint16_t)(out->cdw10_15[2] & 0xFFFF) + 1;
    /* Classify */
    out->is_admin = (out->flags & 0x40) ? 0 : 1;   /* FUSE bit area / SQ ID */
    if (out->is_admin) {
        strncpy(out->op_name, admin_op_name(out->opcode), sizeof(out->op_name)-1);
    } else {
        strncpy(out->op_name, nvm_op_name(out->opcode), sizeof(out->op_name)-1);
    }
    out->is_security = (out->opcode == 0x7D || out->opcode == 0x7E ||
                        out->opcode == 0x81 || out->opcode == 0x82);
    out->is_firmware = (out->opcode == 0x0E || out->opcode == 0x10);
}

/* Decode a 16-byte completion-queue entry (raw bytes, little-endian). */
void nvme_decode_cq(const uint8_t *cq, nvme_cpl_t *out)
{
    memset(out, 0, sizeof(*out));
    out->cid       = (uint16_t)cq[0] | ((uint16_t)cq[1] << 8);
    out->status    = (uint16_t)cq[2] | ((uint16_t)cq[3] << 8);
    out->sqhd      = (uint16_t)cq[4] | ((uint16_t)cq[5] << 8);
    out->sqid      = (uint16_t)cq[6] | ((uint16_t)cq[7] << 8);
    out->cdw0      = (uint32_t)cq[8] | ((uint32_t)cq[9] << 8) |
                     ((uint32_t)cq[10] << 16) | ((uint32_t)cq[11] << 24);
    out->sc        = (out->status >> 1) & 0xFF;     /* status code */
    out->sct       = (out->status >> 9) & 0x7;      /* status code type */
    out->dnr       = (out->status >> 15) & 0x1;     /* do-not-retry */
}

/* Format a decoded command into a compact line for the OLED / log. */
void nvme_format_cmd(const nvme_cmd_t *cmd, char *buf, size_t buflen)
{
    /* Compact format:
     *   "R NS1 LBA=0x1234 N=8"      (Read)
     *   "W NS1 LBA=0xAB  N=16"      (Write)
     *   "Identify CNS=1"            (Admin Identify)
     *   "SecSend NS1 len=512"       (Security Send / Opal)
     *   "FWCommit slot=1 act=3"     (Firmware Commit)
     */
    if (cmd->is_admin) {
        if (cmd->opcode == 0x06) {           /* Identify */
            snprintf(buf, buflen, "Identify CNS=%u NSID=%u",
                     cmd->cdw10_15[0] & 0xFF, cmd->nsid);
        } else if (cmd->opcode == 0x0E) {    /* Firmware Commit */
            snprintf(buf, buflen, "FWCommit slot=%u act=%u",
                     (cmd->cdw10_15[0] >> 3) & 0x7,
                     cmd->cdw10_15[0] & 0x7);
        } else if (cmd->opcode == 0x10) {    /* FW Image Download */
            snprintf(buf, buflen, "FWDownload off=%u len=%u",
                     cmd->cdw10_15[0], cmd->cdw10_15[1]);
        } else if (cmd->is_security) {
            snprintf(buf, buflen, "Sec%c NSID=%u proto=%u len=%u",
                     (cmd->opcode == 0x7E || cmd->opcode == 0x82) ? 'Recv' : 'Send',
                     cmd->nsid,
                     (cmd->cdw10_15[0] >> 24) & 0xFF,
                     (cmd->cdw10_15[1] >> 2) & 0x3FF);
        } else {
            snprintf(buf, buflen, "%s NSID=%u", cmd->op_name, cmd->nsid);
        }
    } else {
        /* NVM command: show LBA + count */
        char op = '?';
        if (cmd->opcode == 0x02) op = 'R';
        else if (cmd->opcode == 0x01) op = 'W';
        else if (cmd->opcode == 0x00) op = 'F';
        else if (cmd->opcode == 0x08) op = 'Z';
        else if (cmd->opcode == 0x09) op = 'D';
        else if (cmd->opcode == 0x05) op = 'C';
        snprintf(buf, buflen, "%c NS%u LBA=0x%llX N=%u",
                 op, cmd->nsid,
                 (unsigned long long)cmd->slba, cmd->nlb);
    }
}

/* Format a completion status into a short string. */
void nvme_format_cpl(const nvme_cpl_t *cpl, char *buf, size_t buflen)
{
    static const char *sct_names[] = {
        "Generic", "CmdSpecific", "Media", "Path", "Vendor"
    };
    const char *sct = (cpl->sct < 5) ? sct_names[cpl->sct] : "?";
    snprintf(buf, buflen, "CID=%u %s/SC=%u%s",
             cpl->cid, sct, cpl->sc, cpl->dnr ? " DNR" : "");
}