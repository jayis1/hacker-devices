/*
 * registers.h — DW3110 register address map and bit definitions.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The DW3110 (Qorvo / Decawave DW3000 family) is accessed through a
 * 32-bit SPI addressing scheme: a 1-byte sub-index selects a register
 * file, then the host reads or writes byte arrays within it.  Only the
 * registers touched by this firmware are listed here; the full map is
 * in the Qorvo DW3110 User Manual (rev B).
 *
 * The DW3000 family departs from the older DW1000 by introducing the
 * STS engine (Scrambled Timestamp Sequence), an in-silicon AES-128
 * block that scrambles the ranging timestamp.  Most STS registers here
 * have no DW1000 equivalent.
 */

#ifndef UWB_PHANTOM_REGISTERS_H
#define UWB_PHANTOM_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ---- Generic access helpers -------------------------------- */
/*
 *  DW3000 sub-index register IDs are 5 bits (0x00..0x1F).  A read of a
 *  "short" register file returns 1..4 bytes; "long" register files are
 *  accessed with an additional offset.  We treat them all as opaque
 *  (file_id, offset, length) tuples.
 */

typedef struct {
    uint8_t  file_id;     /* register file / sub-index */
    uint16_t offset;      /* byte offset within the file */
    uint8_t  len;         /* bytes to read/write */
} dw_reg_t;

/* ---- Top-level device IDs --------------------------------- */

#define DW3000_DEV_ID            0x0F     /* sub-index 0x00 */
#define DW3000_DEV_ID_VAL        0xDECA0302  /* DW3110 returns this */
#define DW3000_EUI_64            0x01     /* 64-bit EUI */
#define DW3000_PANADR            0x03     /* 16-bit PAN + 16-bit short addr */
#define DW3000_SYS_CFG           0x04
#define DW3000_FF_CFG            0x05     /* frame filter configuration */
#define DW3000_TX_FCTRL          0x08     /* TX frame control */
#define DW3000_TX_BUFFER         0x0C     /* TX data buffer */
#define DW3000_TX_TIME           0x0D
#define DW3000_RX_FINFO          0x0F
#define DW3000_RX_BUFFER         0x0E
#define DW3000_TX_ANTD           0x0A
#define DW3000_RX_ANTD           0x0A
#define DW3000_LDE_CFG           0x15     /* leading-edge detection */
#define DW3000_TX_POWER          0x1E
#define DW3000_CHAN_CTRL          0x1F
#define DW3000_SYS_CTRL           0x0D
#define DW3000_SYS_STATUS         0x0C
#define DW3000_SYS_TIME           0x06
#define DW3000_ACK_RESP           0x13     /* auto-ACK / auto-response */
#define DW3000_RX_CAL            0x00     /* sub-index for RX calibration */

/* ---- STS-specific registers (DW3000 only) ----------------- */

#define DW3000_STS_CFG            0x32    /* STS configuration */
#define DW3000_STS_CONTROL        0x33
#define DW3000_STS_KEY            0x34    /* 128-bit AES STS key */
#define DW3000_STS_IV             0x35    /* 128-bit initial vector */
#define DW3000_STS_STS            0x36    /* captured STS at RX */
#define DW3000_STS_PART1          0x37    /* upper STS quality metrics */
#define DW3000_STS_PART2          0x38
#define DW3000_STS_PART3          0x39
#define DW3000_STS_STS_LO_ACC     0x3A    /* accumulated Lo correlation */
#define DW3000_STS_THRESH         0x3B

#define STS_LEN_32                0x00
#define STS_LEN_64                0x01
#define STS_LEN_128               0x02
#define STS_LEN_256               0x03

#define STS_MODE_OFF              0x00    /* no STS (legacy 4a) */
#define STS_MODE_STATIC           0x01    /* static STS key, IV */
#define STS_MODE_DYNAMIC          0x02    /* dynamic STS (counter-based IV) */
#define STS_MODE_ADVANCED         0x03    /* advanced STS (with MAC) */

/* ---- SYS_CFG bits ----------------------------------------- */

#define SYS_CFG_FFEN             (1u << 0)
#define SYS_CFG_HIRQ_POL         (1u << 1)
#define SYS_CFG_RXAUTOR          (1u << 5)  /* auto re-enable RX after frame */
#define SYS_CFG_PHRMODE          (1u << 16)
#define SYS_CFG_DIS_DRXB         (1u << 12)  /* disable double RX buffer */
#define SYS_CFG_DIS_STSP         (1u << 17)  /* disable STS in TX */
#define SYS_CFG_TX_PSTM          (1u << 20)  /* frame wait timeout */

/* ---- SYS_STATUS bits -------------------------------------- */

#define SYS_STATUS_RXDFR         (1u << 0)  /* RX frame ready */
#define SYS_STATUS_TXFRS         (1u << 1)  /* TX frame sent */
#define SYS_STATUS_RXPRD         (1u << 2)  /* RX preamble done */
#define SYS_STATUS_RXSFD         (1u << 3)  /* RX SFD detected */
#define SYS_STATUS_AFF           (1u << 4)  /* auto-ACK frame finished */
#define SYS_STATUS_RXRFTO        (1u << 8)  /* RX frame timeout */
#define SYS_STATUS_RXPTO          (1u << 9)  /* RX preamble timeout */
#define SYS_STATUS_RXSFDTO       (1u << 10) /* SFD timeout */
#define SYS_STATUS_RXPHE         (1u << 12) /* PHY header error */
#define SYS_STATUS_RXFCE         (1u << 13) /* FCS / CRC error */
#define SYS_STATUS_RXRFSL        (1u << 14) /* frame sync loss */
#define SYS_STATUS_AREGD         (1u << 25) /* auto re-enable done */
#define SYS_STATUS_HPDWARN       (1u << 26) /* host power-down warn */
#define SYS_STATUS_CPLOCK        (1u << 27) /* clock PLL lock */
#define SYS_STATUS_CLKPLL_LOCK   (1u << 30)

#define SYS_STATUS_ALL_RX        (SYS_STATUS_RXDFR | SYS_STATUS_RXPRD | \
                                  SYS_STATUS_RXSFD | SYS_STATUS_RXRFTO | \
                                  SYS_STATUS_RXPTO  | SYS_STATUS_RXSFDTO | \
                                  SYS_STATUS_RXPHE  | SYS_STATUS_RXFCE  | \
                                  SYS_STATUS_RXRFSL)
#define SYS_STATUS_ALL_TX        (SYS_STATUS_TXFRS | SYS_STATUS_AFF)
#define SYS_STATUS_ALL_IRQ       (SYS_STATUS_ALL_RX | SYS_STATUS_ALL_TX)

/* ---- SYS_CTRL bits ---------------------------------------- */

#define SYS_CTRL_SFCST           (1u << 0)  /* start frame counter sync */
#define SYS_CTRL_TENSTS          (1u << 1)  /* start TX with STS */
#define SYS_CTRL_TXSTRT          (1u << 2)  /* start TX */
#define SYS_CTRL_TXSTSE          (1u << 3)  /* TX start with STS enable */
#define SYS_CTRL_CANSFCS         (1u << 4)  /* cancel frame counter sync */
#define SYS_CTRL_TRXOFF          (1u << 6)  /* transceiver off */
#define SYS_CTRL_RXENAB          (1u << 8)  /* enable RX */
#define SYS_CTRL_RXDLYE         (1u << 9)  /* RX delayed enable */
#define SYS_CTRL_TXDLYS          (1u << 10) /* TX delayed start */
#define SYS_CTRL_HRBP            (1u << 12) /* host buffer pointer */

/* ---- CHAN_CTRL bits --------------------------------------- */

#define CHAN_CTRL_TX_CHAN_BITS    0x00FF0000u
#define CHAN_CTRL_TX_CHAN_SHIFT   16
#define CHAN_CTRL_RX_CHAN_BITS    0x0000FF00u
#define CHAN_CTRL_RX_CHAN_SHIFT   8
#define CHAN_CTRL_TX_PCODE_BITS  0x000F0000u
#define CHAN_CTRL_TX_PCODE_SHIFT 16
#define CHAN_CTRL_RX_PCODE_BITS  0x00000F00u
#define CHAN_CTRL_RX_PCODE_SHIFT 8

/* ---- FF_CFG bits (frame filter) --------------------------- */

#define FF_CFG_FFBC            (1u << 0)   /* allow beacon */
#define FF_CFG_FFACK           (1u << 1)   /* allow ACK frames */
#define FF_CFG_FFMA            (1u << 3)   /* allow MAC command */
#define FF_CFG_FFRT            (1u << 5)   /* allow reserved frame types */
#define FF_CFG_FFDATA          (1u << 6)   /* allow data frames */
#define FF_CFG_FFALL           (FF_CFG_FFBC | FF_CFG_FFACK | \
                               FF_CFG_FFMA | FF_CFG_FFRT | FF_CFG_FFDATA)

/* ---- TX_FCTRL bits ---------------------------------------- */

#define TX_FCTRL_TXBR_BITS     0x00000003u   /* bitrate select */
#define TX_FCTRL_TXPRF_BITS    0x0000000Cu
#define TX_FCTRL_TXPRF_SHIFT   2
#define TX_FCTRL_TXPSR_BITS    0x0000C000u
#define TX_FCTRL_TXPSR_SHIFT   14
#define TX_FCTRL_TXPE_BITS     0x000F0000u
#define TX_FCTRL_TXPE_SHIFT    16

#define TX_PRF_16M             0x0
#define TX_PRF_64M             0x2

#define PLEN_BTF_64            0x01
#define PLEN_BTF_128           0x05
#define PLEN_BTF_256           0x09
#define PLEN_BTF_512           0x0D
#define PLEN_BTF_1024          0x02
#define PLEN_BTF_2048          0x06

/* ---- ACK_RESP_T bits -------------------------------------- */

#define ACK_RESP_T_ACK_TIM_BITS  0x0000FFFFu   /* auto-ACK delay (DTU) */
#define ACK_RESP_T_RESP_TIM_BITS 0xFFFF0000u   /* auto-response delay */
#define ACK_RESP_T_RESP_SHIFT    16

/* ---- Frame filter configuration (for TWR) ------------------ */

#define FRAME_TYPE_BEACON       0x00
#define FRAME_TYPE_DATA         0x01
#define FRAME_TYPE_ACK          0x02
#define FRAME_TYPE_MAC          0x03
#define FRAME_TYPE_RANGING       0x05     /* 802.15.4z ranging */

/* ---- Antenna delay defaults ------------------------------- */
/*
 *  Antenna delay is in DTU.  Each board must self-calibrate; these
 *  values are sane starting points for the chosen PCB stack-up.
 */

#define DEFAULT_ANT_DLY_TX      16385
#define DEFAULT_ANT_DLY_RX      16385

/* ---- STS key / IV lengths ------------------------------- */

#define STS_KEY_LEN             16        /* bytes (AES-128 key) */
#define STS_IV_LEN               16        /* bytes */
#define STS_QUALITY_LEN          16        /* captured STS samples */

/* ---- IRQ sources mask (DW3000) --------------------------- */

#define IRQ_MASK_TXFRS         SYS_STATUS_TXFRS
#define IRQ_MASK_RXDFR         SYS_STATUS_RXDFR
#define IRQ_MASK_RXSFD         SYS_STATUS_RXSFD
#define IRQ_MASK_RXFCE         SYS_STATUS_RXFCE
#define IRQ_MASK_RXRFTO        SYS_STATUS_RXRFTO
#define IRQ_MASK_AFF           SYS_STATUS_AFF

/* ---- Misc helpers ---------------------------------------- */

#define ARRAY_SIZE(a)          (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)              (((a) < (b)) ? (a) : (b))
#define MAX(a, b)              (((a) > (b)) ? (a) : (b))

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_REGISTERS_H */