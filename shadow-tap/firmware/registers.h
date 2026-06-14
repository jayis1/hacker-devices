/*
 * registers.h — i.MX RT1062 MMIO register definitions for ShadowTap
 */

#ifndef SHADOWTAP_REGISTERS_H
#define SHADOWTAP_REGISTERS_H

#include <stdint.h>

/* ========== CCM (Clock Control Module) ========== */
typedef struct {
    volatile uint32_t CCR;          /* 0x000 */
    volatile uint32_t CCDR;        /* 0x004 */
    volatile uint32_t CSR;         /* 0x008 */
    volatile uint32_t CCSR;        /* 0x00C */
    volatile uint32_t CACRR;       /* 0x010 */
    volatile uint32_t CBCDR;       /* 0x014 */
    volatile uint32_t CBCMR;       /* 0x018 */
    volatile uint32_t CSCMR1;      /* 0x01C */
    volatile uint32_t CSCMR2;      /* 0x020 */
    volatile uint32_t CSCDR1;      /* 0x024 */
    volatile uint32_t CS1CDR;      /* 0x028 */
    volatile uint32_t CS2CDR;      /* 0x02C */
    volatile uint32_t CDCDR;       /* 0x030 */
    volatile uint32_t RESERVED_0;  /* 0x034 */
    volatile uint32_t CSCDR2;      /* 0x038 */
    volatile uint32_t CSCDR3;      /* 0x03C */
    volatile uint32_t RESERVED_1[2]; /* 0x040-0x044 */
    volatile uint32_t CDHIPR;      /* 0x048 */
    volatile uint32_t RESERVED_2[2]; /* 0x04C-0x050 */
    volatile uint32_t CCGR0;       /* 0x054 */
    volatile uint32_t CCGR1;       /* 0x058 */
    volatile uint32_t CCGR2;       /* 0x05C */
    volatile uint32_t CCGR3;       /* 0x060 */
    volatile uint32_t CCGR4;       /* 0x064 */
    volatile uint32_t CCGR5;       /* 0x068 */
    volatile uint32_t CCGR6;       /* 0x06C */
    volatile uint32_t CCGR7;       /* 0x070 */
    volatile uint32_t CCGR8;       /* 0x074 */
    volatile uint32_t CCGR9;       /* 0x078 */
    volatile uint32_t CCGR10;      /* 0x07C */
    volatile uint32_t CCGR11;      /* 0x080 */
} CCM_Type;

#define CCM ((CCM_Type *)0x400FC000U)

/* CCGR bit masks */
#define CCM_CCGR1_CG0_MASK    (3U << 0)   /* LPSPI1 */
#define CCM_CCGR1_CG10_MASK   (3U << 20)  /* ENET1 */
#define CCM_CCGR1_CG11_MASK   (3U << 22)  /* ENET2 */
#define CCM_CCGR2_CG3_MASK    (3U << 6)   /* LPI2C1 */
#define CCM_CCGR2_CG4_MASK    (3U << 8)   /* LPI2C2 */
#define CCM_CCGR5_CG12_MASK   (3U << 24)  /* LPUART1 */
#define CCM_CCGR6_CG0_MASK    (3U << 0)   /* uSDHC1 */

/* CBCMR fields */
#define CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK   (3U << 18)
#define CCM_CBCMR_PRE_PERIPH_CLK_SEL_SHIFT  18

/* ========== CCM_ANALOG ========== */
typedef struct {
    volatile uint32_t PLL_ARM;              /* 0x000 */
    volatile uint32_t PLL_USB1;             /* 0x004 */
    volatile uint32_t PLL_USB2;             /* 0x008 */
    volatile uint32_t PLL_SYS;              /* 0x00C */
    volatile uint32_t PLL_SYS_SS;           /* 0x010 */
    volatile uint32_t RESERVED_0[3];        /* 0x014-0x01C */
    volatile uint32_t PLL_AUDIO;            /* 0x020 */
    volatile uint32_t PLL_AUDIO_NUM;        /* 0x024 */
    volatile uint32_t PLL_AUDIO_DENOM;      /* 0x028 */
    volatile uint32_t PLL_VIDEO;            /* 0x02C */
    volatile uint32_t PLL_VIDEO_NUM;        /* 0x030 */
    volatile uint32_t PLL_VIDEO_DENOM;      /* 0x034 */
    volatile uint32_t PLL_ENET;             /* 0x038 */
    volatile uint32_t RESERVED_1;           /* 0x03C */
    volatile uint32_t PLL_ENET2;            /* 0x040 */
} CCM_ANALOG_Type;

#define CCM_ANALOG ((CCM_ANALOG_Type *)0x400D8000U)

/* PLL_ARM fields */
#define CCM_ANALOG_PLL_ARM_ENABLE_MASK      (1U << 13)
#define CCM_ANALOG_PLL_ARM_BYPASS_MASK      (1U << 16)
#define CCM_ANALOG_PLL_ARM_DIV_SELECT_SHIFT  0
#define CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK   (0x7FU << 0)

/* PLL_SYS fields */
#define CCM_ANALOG_PLL_SYS_ENABLE_MASK      (1U << 13)
#define CCM_ANALOG_PLL_SYS_BYPASS_MASK      (1U << 16)
#define CCM_ANALOG_PLL_SYS_DIV_SELECT_SHIFT  0
#define CCM_ANALOG_PLL_SYS_DIV_SELECT_MASK   (1U << 0)

/* PLL_ENET fields */
#define CCM_ANALOG_PLL_ENET_ENABLE_MASK         (1U << 13)
#define CCM_ANALOG_PLL_ENET_BYPASS_MASK         (1U << 16)
#define CCM_ANALOG_PLL_ENET_ENET_125M_REF_EN_MASK  (1U << 0)
#define CCM_ANALOG_PLL_ENET_ENET_50M_REF_EN_MASK   (1U << 1)
#define CCM_ANALOG_PLL_ENET_ENET_25M_REF_EN_MASK   (1U << 2)

/* ========== ENET (Ethernet MAC) ========== */
typedef struct {
    volatile uint32_t EIR;      /* 0x000: Interrupt Event */
    volatile uint32_t EIMR;     /* 0x004: Interrupt Mask */
    volatile uint32_t RESERVED_0; /* 0x008 */
    volatile uint32_t RDAR;     /* 0x00C: Receive Descriptor Active (NOTE: offset varies) */
    volatile uint32_t TDAR;     /* 0x010: Transmit Descriptor Active */
    volatile uint32_t ECR;      /* 0x014: Ethernet Control */
    volatile uint32_t RESERVED_1[2]; /* 0x018-0x01C */
    volatile uint32_t MSCR;     /* 0x020: MII Speed Control */
    volatile uint32_t RESERVED_2[7]; /* 0x024-0x03C */
    volatile uint32_t RCR;      /* 0x040: Receive Control */
    volatile uint32_t RESERVED_3; /* 0x044 */
    volatile uint32_t TCR;      /* 0x048: Transmit Control */
    volatile uint32_t PALR;     /* 0x04C: Physical Address Low */
    volatile uint32_t PAUR;     /* 0x050: Physical Address High */
    volatile uint32_t OPD;      /* 0x054: Opcode/Pause Duration */
    volatile uint32_t RESERVED_4[10]; /* 0x058-0x07C */
    volatile uint32_t IAUR;     /* 0x080: Descriptor Upper (hash) */
    volatile uint32_t IALR;     /* 0x084: Descriptor Lower (hash) */
    volatile uint32_t GAUR;     /* 0x088: Group Address Upper */
    volatile uint32_t GALR;     /* 0x08C: Group Address Lower */
    volatile uint32_t RESERVED_5[7]; /* 0x090-0x0A8 */
    volatile uint32_t TFWR;     /* 0x0AC: Transmit FIFO Watermark (NOTE: offset varies) */
    volatile uint32_t RESERVED_6; /* 0x0B0 */
    volatile uint32_t RDSR;     /* 0x0B4: Receive Descriptor Ring Start */
    volatile uint32_t TDSR;     /* 0x0B8: Transmit Descriptor Ring Start */
    volatile uint32_t MRBR;     /* 0x0BC: Maximum Receive Buffer Size */
} ENET_Type;

#define ENET1 ((ENET_Type *)0x402D8000U)
#define ENET2 ((ENET_Type *)0x402D8400U)

/* ECR bits */
#define ENET_ECR_ETHEREN_MASK       (1U << 1)
#define ENET_ECR_MAGICEN_MASK       (1U << 2)
#define ENET_ECR_SLEEP_MASK         (1U << 5)
#define ENET_ECR_SPEED_MASK         (1U << 15)

/* RCR bits */
#define ENET_RCR_LOOP_MASK          (1U << 0)
#define ENET_RCR_PROM_MASK          (1U << 3)  /* Promiscuous mode */
#define ENET_RCR_BC_REJ_MASK        (1U << 4)
#define ENET_RCR_MII_MODE_MASK      (1U << 18)
#define ENET_RCR_RGMII_MODE_MASK     (1U << 21)
#define ENET_RCR_RMII_MODE_MASK      (1U << 20)
#define ENET_RCR_NLC_MASK           (1U << 25)
#define ENET_RCR_MAX_FL_SHIFT        16
#define ENET_RCR_MAX_FL_MASK        (0x3FFFU << 16)

/* TCR bits */
#define ENET_TCR_FDEN_MASK          (1U << 0)  /* Full duplex */
#define ENET_TCR_ADDINS_MASK        (1U << 1)  /* MAC address insert */
#define ENET_TCR_CRCFWD_MASK        (1U << 5)  /* Forward CRC (don't append) */

/* MSCR bits */
#define ENET_MSCR_MII_SPEED_SHIFT    1
#define ENET_MSCR_MII_SPEED_MASK    (0x7EU << 1)
#define ENET_MSCR_DIS_PRE_MASK      (1U << 7)

/* EIR bits */
#define ENET_EIR_TXF_MASK           (1U << 13)  /* Transmit frame interrupt */
#define ENET_EIR_RXF_MASK           (1U << 15)  /* Receive frame interrupt */
#define ENET_EIR_EBERR_MASK         (1U << 22)  /* Ethernet bus error */

/* ENET Buffer Descriptor */
typedef struct {
    volatile uint16_t status;     /* Control and status */
    volatile uint16_t length;     /* Buffer data length */
    volatile uint8_t  *data;      /* Buffer data pointer (aligned) */
} enet_bd_t;

/* BD status bits (RX) */
#define ENET_RX_BD_E_MASK       (1U << 15)  /* Empty */
#define ENET_RX_BD_RO1_MASK    (1U << 14)  /* Receive software ownership 1 */
#define ENET_RX_BD_W_MASK      (1U << 13)  /* Wrap */
#define ENET_RX_BD_RO2_MASK    (1U << 12)  /* Receive software ownership 2 */
#define ENET_RX_BD_L_MASK      (1U << 11)  /* Last frame in buffer */
#define ENET_RX_BD_NO_MASK     (1U << 4)   /* Receive non-octet alignment */
#define ENET_RX_BD_CR_MASK     (1U << 2)   /* Receive CRC error */
#define ENET_RX_BD_OV_MASK     (1U << 1)   /* Receive overwrite */
#define ENET_RX_BD_TR_MASK    (1U << 0)   /* Receive truncated */

/* BD status bits (TX) */
#define ENET_TX_BD_R_MASK      (1U << 15)  /* Ready */
#define ENET_TX_BD_TO1_MASK    (1U << 14)  /* Transmit software ownership 1 */
#define ENET_TX_BD_W_MASK      (1U << 13)  /* Wrap */
#define ENET_TX_BD_TO2_MASK    (1U << 12)  /* Transmit software ownership 2 */
#define ENET_TX_BD_L_MASK      (1U << 11)  /* Last buffer in frame */
#define ENET_TX_BD_TC_MASK     (1U << 10)  /* Transmit CRC */

/* ========== LPUART ========== */
typedef struct {
    volatile uint32_t VERID;     /* 0x000 */
    volatile uint32_t PARAM;     /* 0x004 */
    volatile uint32_t CTRL;      /* 0x008 */
    volatile uint32_t GLOBAL;    /* 0x00C */
    volatile uint32_t PINCFG;    /* 0x010 */
    volatile uint32_t BAUD;      /* 0x014 */
    volatile uint32_t STAT;      /* 0x018 */
    volatile uint32_t CTRLIDLE;  /* 0x01C */
    volatile uint32_t DATA;      /* 0x020 */
    volatile uint32_t MATCH;     /* 0x024 */
    volatile uint32_t MODIR;     /* 0x028 */
    volatile uint32_t FIFO;      /* 0x02C */
    volatile uint32_t WATER;     /* 0x030 */
} LPUART_Type;

#define LPUART1 ((LPUART_Type *)0x40185000U)

/* LPUART CTRL bits */
#define LPUART_CTRL_TE_MASK      (1U << 19)
#define LPUART_CTRL_RE_MASK      (1U << 18)

/* LPUART BAUD bits */
#define LPUART_BAUD_OSR_SHIFT     24
#define LPUART_BAUD_OSR_MASK     (0xFU << 24)
#define LPUART_BAUD_SBR_SHIFT     0
#define LPUART_BAUD_SBR_MASK     (0x1FFFU << 0)
#define LPUART_BAUD_SBNS_MASK    (1U << 13)

/* LPUART STAT bits */
#define LPUART_STAT_TDRE_MASK    (1U << 23)
#define LPUART_STAT_RDRF_MASK    (1U << 21)
#define LPUART_STAT_OR_MASK      (1U << 19)

/* LPUART FIFO bits */
#define LPUART_FIFO_TXFLUSH_MASK (1U << 15)
#define LPUART_FIFO_RXFLUSH_MASK (1U << 14)
#define LPUART_FIFO_TXFE_MASK    (1U << 7)
#define LPUART_FIFO_RXFE_MASK    (1U << 3)

/* ========== GPIO ========== */
typedef struct {
    volatile uint32_t DR;        /* 0x000: Data Register */
    volatile uint32_t GDIR;      /* 0x004: Direction Register */
    volatile uint32_t PSR;       /* 0x008: Pad Status Register */
    volatile uint32_t ICR1;      /* 0x00C: Interrupt Configuration 1 */
    volatile uint32_t ICR2;      /* 0x010: Interrupt Configuration 2 */
    volatile uint32_t IMR;       /* 0x014: Interrupt Mask Register */
    volatile uint32_t ISR;       /* 0x018: Interrupt Status Register */
} GPIO_Type;

#define GPIO1 ((GPIO_Type *)0x401B8000U)

/* ========== IOMUXC ========== */
typedef struct {
    volatile uint32_t SW_MUX_CTL_PAD[128]; /* Mux control registers */
    volatile uint32_t SW_PAD_CTL_PAD[128]; /* Pad control registers */
} IOMUXC_Type;

#define IOMUXC ((IOMUXC_Type *)0x401F8000U)

/* Pad control fields */
#define IOMUXC_PAD_DSE_SHIFT     3
#define IOMUXC_PAD_DSE_MASK     (0x07U << 3)
#define IOMUXC_PAD_DSE(n)       (((n) & 0x7U) << 3)
#define IOMUXC_PAD_SRE_MASK     (1U << 0)  /* Slew rate: fast */
#define IOMUXC_PAD_PKE_MASK     (1U << 12) /* Keeper enable */
#define IOMUXC_PAD_PUE_MASK     (1U << 13) /* Pull up */
#define IOMUXC_PAD_PUS_SHIFT    14
#define IOMUXC_PAD_HYS_MASK     (1U << 16) /* Hysteresis */

/* ========== uSDHC ========== */
typedef struct {
    volatile uint32_t DS_ADDR;      /* 0x000: DMA System Address */
    volatile uint32_t BLK_ATT;       /* 0x004: Block Attributes */
    volatile uint32_t CMD_ARG;       /* 0x008: Command Argument */
    volatile uint32_t CMD_XFR_TYP;   /* 0x00C: Command Transfer Type */
    volatile uint32_t CMD_RSP0;      /* 0x010: Command Response 0 */
    volatile uint32_t CMD_RSP1;      /* 0x014: Command Response 1 */
    volatile uint32_t CMD_RSP2;      /* 0x018: Command Response 2 */
    volatile uint32_t CMD_RSP3;      /* 0x01C: Command Response 3 */
    volatile uint32_t INT_STATUS;    /* 0x020: Interrupt Status */
    volatile uint32_t INT_STATUS_EN; /* 0x024: Interrupt Status Enable */
    volatile uint32_t INT_SIGNAL_EN; /* 0x028: Interrupt Signal Enable */
    volatile uint32_t AUTOCMD12_ERR; /* 0x02C: Auto CMD12 Error */
    volatile uint32_t HOST_CTRL_CAP; /* 0x030: Host Controller Capability */
    volatile uint32_t VEND_SPEC;     /* 0x034: Vendor Specific */
    volatile uint32_t MMC_BOOT;      /* 0x038: MMC Boot */
    volatile uint32_t VEND_SPEC2;    /* 0x03C: Vendor Specific 2 */
    volatile uint32_t TUNING_CTRL;   /* 0x040: Tuning Control */
} USDHC_Type;

#define USDHC1 ((USDHC_Type *)0x402C0000U)

/* uSDHC INT_STATUS bits */
#define USDHC_INT_CC_MASK       (1U << 0)   /* Command complete */
#define USDHC_INT_TC_MASK       (1U << 1)   /* Transfer complete */
#define USDHC_INT_BGE_MASK      (1U << 2)   /* Block gap event */
#define USDHC_INT_DINT_MASK     (1U << 3)   /* DMA interrupt */
#define USDHC_INT_BWR_MASK     (1U << 4)   /* Buffer write ready */
#define USDHC_INT_BRR_MASK     (1U << 5)   /* Buffer read ready */
#define USDHC_INT_CINS_MASK    (1U << 6)   /* Card insertion */
#define USDHC_INT_CRM_MASK     (1U << 7)   /* Card removal */
#define USDHC_INT_CINT_MASK    (1U << 8)   /* Card interrupt */
#define USDHC_INT_DMAE_MASK    (1U << 28)  /* DMA error */
#define USDHC_INT_AC12E_MASK   (1U << 29)  /* Auto CMD12 error */
#define USDHC_INT_DEBE_MASK    (1U << 30)  /* Data end bit error */
#define USDHC_INT_DCE_MASK     (1U << 31)  /* Data CRC error */

/* ========== FlexSPI ========== */
typedef struct {
    volatile uint32_t MCR0;          /* 0x000 */
    volatile uint32_t MCR1;         /* 0x004 */
    volatile uint32_t MCR2;         /* 0x008 */
    volatile uint32_t AHBCR;        /* 0x00C */
    volatile uint32_t INTEN;        /* 0x010 */
    volatile uint32_t INTR;         /* 0x014 */
    volatile uint32_t LUTKEY;      /* 0x018 */
    volatile uint32_t LUTCR;       /* 0x01C */
    volatile uint32_t LUT[64];     /* 0x020-0x0BC: Lookup tables */
    volatile uint32_t RESERVED_0[40]; /* 0x0C0-0x15C */
    volatile uint32_t AHBRX_BUF0CR0; /* 0x160 */
    volatile uint32_t AHBRX_BUF0CR1; /* 0x164 */
    volatile uint32_t AHBRX_BUF1CR0; /* 0x168 */
    volatile uint32_t AHBRX_BUF1CR1; /* 0x16C */
    volatile uint32_t AHBRX_BUF2CR0; /* 0x170 */
    volatile uint32_t AHBRX_BUF2CR1; /* 0x174 */
    volatile uint32_t AHBRX_BUF3CR0; /* 0x178 */
    volatile uint32_t AHBRX_BUF3CR1; /* 0x17C */
    volatile uint32_t AHBRX_BUF4CR0; /* 0x180 */
    volatile uint32_t AHBRX_BUF4CR1; /* 0x184 */
    volatile uint32_t AHBRX_BUF5CR0; /* 0x188 */
    volatile uint32_t AHBRX_BUF5CR1; /* 0x18C */
    volatile uint32_t AHBRX_BUF6CR0; /* 0x190 */
    volatile uint32_t AHBRX_BUF6CR1; /* 0x194 */
    volatile uint32_t AHBRX_BUF7CR0; /* 0x198 */
    volatile uint32_t AHBRX_BUF7CR1; /* 0x19C */
    volatile uint32_t RESERVED_1[4]; /* 0x1A0-0x1AC */
    volatile uint32_t STS0;         /* 0x1B0 */
    volatile uint32_t STS1;         /* 0x1B4 */
    volatile uint32_t STS2;         /* 0x1B8 */
} FLEXSPI_Type;

#define FLEXSPI ((FLEXSPI_Type *)0x402A8000U)

/* FlexSPI MCR0 bits */
#define FLEXSPI_MCR0_MDIS_MASK      (1U << 1)
#define FLEXSPI_MCR0_SWRESET_MASK   (1U << 0)
#define FLEXSPI_MCR0_COMBOEN_MASK   (1U << 23)
#define FLEXSPI_MCR0_SCKFREERUNEN_MASK (1U << 9)
#define FLEXSPI_MCR0_AHBGRANTWAIT_SHIFT 16
#define FLEXSPI_MCR0_AHBGRANTWAIT_MASK (0xFFU << 16)

/* FlexSPI LUT opcodes */
#define FLEXSPI_LUT_OPCODE_CMD_SDR      0x01U
#define FLEXSPI_LUT_OPCODE_RADDR_SDR    0x02U
#define FLEXSPI_LUT_OPCODE_READ_SDR     0x03U
#define FLEXSPI_LUT_OPCODE_WRITE_SDR    0x04U
#define FLEXSPI_LUT_OPCODE_DUMMY_SDR    0x05U
#define FLEXSPI_LUT_OPCODE_CMD_DDR      0x21U
#define FLEXSPI_LUT_OPCODE_RADDR_DDR    0x22U
#define FLEXSPI_LUT_OPCODE_READ_DDR     0x23U

#endif /* SHADOWTAP_REGISTERS_H */