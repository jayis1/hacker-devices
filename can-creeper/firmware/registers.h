/**
 * @file registers.h
 * @brief Complete MMIO register map for nRF52840 peripherals used by CAN Creeper
 *
 * All register addresses, bit definitions, and access macros for:
 * - CLOCK, POWER, SPIM0/1, QSPI, TWIM0, USBD, TIMER0/1/2, RTC0/1/2
 * - GPIOTE, GPIO P0/P1, SAADC, NVMC, WDT, RADIO (BLE)
 * - FICR, UICR
 *
 * Based on nRF52840 Product Specification v1.1
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * BASE ADDRESSES
 *===========================================================================*/

#define NRF_CLOCK_BASE              0x40000000UL
#define NRF_POWER_BASE              0x40000000UL  /* Shared with CLOCK */
#define NRF_RADIO_BASE              0x40001000UL
#define NRF_SPIM0_BASE              0x40003000UL
#define NRF_SPIM1_BASE              0x40004000UL
#define NRF_TWIM0_BASE              0x40003000UL  /* Shared base, different instance */
#define NRF_SPIS0_BASE              0x40003000UL  /* Shared base */
#define NRF_GPIOTE_BASE             0x40006000UL
#define NRF_SAADC_BASE              0x40007000UL
#define NRF_TIMER0_BASE             0x40008000UL
#define NRF_TIMER1_BASE             0x40009000UL
#define NRF_TIMER2_BASE             0x4000A000UL
#define NRF_RTC0_BASE               0x4000B000UL
#define NRF_RTC1_BASE               0x4000C000UL
#define NRF_RTC2_BASE               0x4000D000UL
#define NRF_WDT_BASE                0x40010000UL
#define NRF_NVMC_BASE               0x4001E000UL
#define NRF_USBD_BASE               0x40027000UL
#define NRF_QSPI_BASE               0x40029000UL
#define NRF_P0_BASE                 0x50000000UL
#define NRF_P1_BASE                 0x50000300UL
#define NRF_FICR_BASE               0x10000000UL
#define NRF_UICR_BASE               0x10001000UL

/*===========================================================================
 * PERIPHERAL STRUCTURE POINTERS
 *===========================================================================*/

/* CLOCK */
typedef struct {
    volatile uint32_t TASKS_HFCLKSTART;          /* 0x000 */
    volatile uint32_t TASKS_HFCLKSTOP;           /* 0x004 */
    volatile uint32_t TASKS_LFCLKSTART;          /* 0x008 */
    volatile uint32_t TASKS_LFCLKSTOP;           /* 0x00C */
    volatile uint32_t TASKS_CAL;                 /* 0x010 */
    volatile uint32_t TASKS_CTSTART;            /* 0x014 */
    volatile uint32_t TASKS_CTSTOP;             /* 0x018 */
    volatile uint32_t RESERVED0[57];            /* 0x01C–0x0FC */
    volatile uint32_t EVENTS_HFCLKSTARTED;      /* 0x100 */
    volatile uint32_t EVENTS_LFCLKSTARTED;      /* 0x104 */
    volatile uint32_t RESERVED1;                /* 0x108 */
    volatile uint32_t EVENTS_DONE;              /* 0x10C */
    volatile uint32_t EVENTS_CTTO;              /* 0x110 */
    volatile uint32_t RESERVED2[123];           /* 0x114–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED3[63];            /* 0x30C–0x3FC */
    volatile uint32_t HFCLKRUN;                 /* 0x408 */
    volatile uint32_t HFCLKSTAT;               /* 0x40C */
    volatile uint32_t RESERVED4;                /* 0x410 */
    volatile uint32_t LFCLKRUN;                 /* 0x414 */
    volatile uint32_t LFCLKSTAT;               /* 0x418 */
    volatile uint32_t LFCLKSRCCOPY;             /* 0x41C */
    volatile uint32_t RESERVED5[62];            /* 0x420–0x514 */
    volatile uint32_t LFCLKSRC;                 /* 0x518 */
    volatile uint32_t RESERVED6[7];             /* 0x51C–0x534 */
    volatile uint32_t HFXODEBOUNCE;             /* 0x538 */
    volatile uint32_t RESERVED7[2];             /* 0x53C–0x540 */
    volatile uint32_t LFXODEBOUNCE;             /* 0x544 */
    volatile uint32_t RESERVED8;                /* 0x548 */
    volatile uint32_t CTIV;                     /* 0x54C */
    volatile uint32_t RESERVED9[8];             /* 0x550–0x56C */
    volatile uint32_t TRACECONFIG;              /* 0x570 */
} NRF_CLOCK_Type;

#define NRF_CLOCK ((NRF_CLOCK_Type *) NRF_CLOCK_BASE)

/* POWER */
typedef struct {
    volatile uint32_t RESERVED0[30];            /* 0x000–0x074 */
    volatile uint32_t TASKS_CONSTLAT;          /* 0x078 */
    volatile uint32_t TASKS_LOWPWR;             /* 0x07C */
    volatile uint32_t RESERVED1[34];            /* 0x080–0x104 */
    volatile uint32_t EVENTS_POFWARN;           /* 0x108 */
    volatile uint32_t RESERVED2[2];             /* 0x10C–0x110 */
    volatile uint32_t EVENTS_SLEEPENTER;        /* 0x114 */
    volatile uint32_t EVENTS_SLEEPEXIT;         /* 0x118 */
    volatile uint32_t RESERVED3[122];           /* 0x11C–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED4[61];            /* 0x30C–0x3FC */
    volatile uint32_t RESETREAS;                /* 0x400 */
    volatile uint32_t RESERVED5[9];             /* 0x404–0x424 */
    volatile uint32_t RAMSTATUS;                /* 0x428 */
    volatile uint32_t RESERVED6[53];            /* 0x42C–0x4FC */
    volatile uint32_t SYSTEMOFF;                /* 0x500 */
    volatile uint32_t RESERVED7[3];             /* 0x504–0x50C */
    volatile uint32_t POFCON;                   /* 0x510 */
    volatile uint32_t RESERVED8[2];             /* 0x514–0x518 */
    volatile uint32_t GPREGRET;                 /* 0x51C */
    volatile uint32_t GPREGRET2;                /* 0x520 */
    volatile uint32_t RESERVED9[21];            /* 0x524–0x574 */
    volatile uint32_t DCDCEN;                   /* 0x578 */
    volatile uint32_t RESERVED10[2];            /* 0x57C–0x580 */
    volatile uint32_t DCDCEN0;                  /* 0x584 */
    volatile uint32_t RESERVED11[47];           /* 0x588–0x640 */
    volatile uint32_t MAINREGSTATUS;            /* 0x644 */
} NRF_POWER_Type;

#define NRF_POWER ((NRF_POWER_Type *) NRF_POWER_BASE)

/* SPIM (SPI Master with EasyDMA) */
typedef struct {
    volatile uint32_t RESERVED0[4];             /* 0x000–0x00C (TASKS_START..STOP at 0x000–0x00C for SPIM0) */
    volatile uint32_t TASKS_START;              /* 0x010 */
    volatile uint32_t TASKS_STOP;               /* 0x014 */
    volatile uint32_t RESERVED1[2];             /* 0x018–0x01C */
    volatile uint32_t TASKS_SUSPEND;            /* 0x020 */
    volatile uint32_t TASKS_RESUME;             /* 0x024 */
    volatile uint32_t RESERVED2[54];            /* 0x028–0x0FC */
    volatile uint32_t EVENTS_STOPPED;           /* 0x100 */
    volatile uint32_t RESERVED3[2];             /* 0x104–0x108 */
    volatile uint32_t EVENTS_ENDRX;             /* 0x10C */
    volatile uint32_t RESERVED4;                /* 0x110 */
    volatile uint32_t EVENTS_END;               /* 0x114 */
    volatile uint32_t RESERVED5;                /* 0x118 */
    volatile uint32_t EVENTS_ENDTX;             /* 0x11C */
    volatile uint32_t RESERVED6[10];            /* 0x120–0x144 */
    volatile uint32_t EVENTS_STARTED;           /* 0x148 */
    volatile uint32_t RESERVED7[109];           /* 0x14C–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED8[125];           /* 0x30C–0x4FC */
    volatile uint32_t ENABLE;                   /* 0x500 */
    volatile uint32_t RESERVED9;                /* 0x504 */
    volatile uint32_t PSEL_SCK;                 /* 0x508 */
    volatile uint32_t PSEL_MOSI;               /* 0x50C */
    volatile uint32_t PSEL_MISO;               /* 0x510 */
    volatile uint32_t RESERVED10[3];            /* 0x514–0x51C */
    volatile uint32_t PSEL_CSN;                 /* 0x520 */
    volatile uint32_t RESERVED11;               /* 0x524 */
    volatile uint32_t FREQUENCY;               /* 0x528 */
    volatile uint32_t RESERVED12[3];            /* 0x52C–0x534 */
    volatile uint32_t RXD_PTR;                  /* 0x538 */
    volatile uint32_t RXD_MAXCNT;              /* 0x53C */
    volatile uint32_t RXD_AMOUNT;              /* 0x540 */
    volatile uint32_t RXD_LIST;                 /* 0x544 */
    volatile uint32_t TXD_PTR;                  /* 0x548 */
    volatile uint32_t TXD_MAXCNT;              /* 0x54C */
    volatile uint32_t TXD_AMOUNT;              /* 0x550 */
    volatile uint32_t TXD_LIST;                 /* 0x554 */
    volatile uint32_t CONFIG;                   /* 0x558 */
    volatile uint32_t RESERVED13[26];           /* 0x55C–0x5C0 */
    volatile uint32_t ORC;                      /* 0x5C4 */
} NRF_SPIM_Type;

#define NRF_SPIM0 ((NRF_SPIM_Type *) NRF_SPIM0_BASE)
#define NRF_SPIM1 ((NRF_SPIM_Type *) NRF_SPIM1_BASE)

/* SPIM ENABLE values */
#define SPIM_ENABLE_ENABLE          0x07UL
#define SPIM_ENABLE_DISABLE         0x00UL

/* SPIM FREQUENCY values */
#define SPIM_FREQ_125KBPS           0x02000000UL
#define SPIM_FREQ_250KBPS           0x04000000UL
#define SPIM_FREQ_500KBPS           0x08000000UL
#define SPIM_FREQ_1MBPS             0x10000000UL
#define SPIM_FREQ_2MBPS             0x20000000UL
#define SPIM_FREQ_4MBPS             0x40000000UL
#define SPIM_FREQ_8MBPS             0x80000000UL

/* SPIM CONFIG bits */
#define SPIM_CONFIG_ORDER_MsbFirst  0x00UL
#define SPIM_CONFIG_ORDER_LsbFirst  0x01UL
#define SPIM_CONFIG_CPHA_Leading    0x00UL
#define SPIM_CONFIG_CPHA_Trailing   0x02UL
#define SPIM_CONFIG_CPOL_ActiveHigh 0x00UL
#define SPIM_CONFIG_CPOL_ActiveLow  0x04UL

/* SPIM INTENSET/INTENCLR bits */
#define SPIM_INT_STOPPED            (1UL << 1)
#define SPIM_INT_ENDRX              (1UL << 4)
#define SPIM_INT_END                (1UL << 6)
#define SPIM_INT_ENDTX              (1UL << 8)
#define SPIM_INT_STARTED            (1UL << 19)

/* QSPI */
typedef struct {
    volatile uint32_t TASKS_ACTIVATE;           /* 0x000 */
    volatile uint32_t TASKS_READSTART;          /* 0x004 */
    volatile uint32_t TASKS_WRITESTART;         /* 0x008 */
    volatile uint32_t TASKS_ERASESTART;         /* 0x00C */
    volatile uint32_t TASKS_DEACTIVATE;         /* 0x010 */
    volatile uint32_t RESERVED0[59];            /* 0x014–0x0FC */
    volatile uint32_t EVENTS_READY;             /* 0x100 */
    volatile uint32_t RESERVED1[128];           /* 0x104–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED2[125];           /* 0x30C–0x4FC */
    volatile uint32_t ENABLE;                   /* 0x500 */
    volatile uint32_t RESERVED3;                /* 0x504 */
    volatile uint32_t PSEL_CSN0;               /* 0x508 */
    volatile uint32_t PSEL_CSN1;               /* 0x50C */
    volatile uint32_t PSEL_SCK;                 /* 0x510 */
    volatile uint32_t PSEL_IO0;                 /* 0x514 */
    volatile uint32_t PSEL_IO1;                 /* 0x518 */
    volatile uint32_t PSEL_IO2;                 /* 0x51C */
    volatile uint32_t PSEL_IO3;                 /* 0x520 */
    volatile uint32_t IFCONFIG0;                /* 0x524 */
    volatile uint32_t IFCONFIG1;                /* 0x528 */
    volatile uint32_t STATUS;                   /* 0x52C */
    volatile uint32_t RESERVED4;                /* 0x530 */
    volatile uint32_t CINSTRCONF;              /* 0x534 */
    volatile uint32_t CINSTRDAT0;              /* 0x538 */
    volatile uint32_t CINSTRDAT1;              /* 0x53C */
    volatile uint32_t DURATION;                 /* 0x540 */
    volatile uint32_t RESERVED5;                /* 0x544 */
    volatile uint32_t READ_SRC;                 /* 0x548 */
    volatile uint32_t READ_DST;                 /* 0x54C */
    volatile uint32_t READ_CNT;                 /* 0x550 */
    volatile uint32_t RESERVED6[3];             /* 0x554–0x55C */
    volatile uint32_t WRITE_SRC;                /* 0x560 */
    volatile uint32_t WRITE_DST;                /* 0x564 */
    volatile uint32_t WRITE_CNT;                /* 0x568 */
    volatile uint32_t RESERVED7[3];             /* 0x56C–0x574 */
    volatile uint32_t ERASE_ADDR;               /* 0x578 */
    volatile uint32_t ERASE_LEN;                /* 0x57C */
    volatile uint32_t XIPOFFSET;                /* 0x580 */
    volatile uint32_t XIPEN;                    /* 0x584 */
} NRF_QSPI_Type;

#define NRF_QSPI ((NRF_QSPI_Type *) NRF_QSPI_BASE)

/* QSPI IFCONFIG0 fields */
#define QSPI_IFCONFIG0_ADDRMODE_24BIT  0x00UL
#define QSPI_IFCONFIG0_ADDRMODE_32BIT  0x01UL
#define QSPI_IFCONFIG0_PPSIZE_256      0x00UL
#define QSPI_IFCONFIG0_PPSIZE_512      0x01UL
#define QSPI_IFCONFIG0_READOC_FASTREAD 0x00UL
#define QSPI_IFCONFIG0_READOC_READ2O   0x01UL
#define QSPI_IFCONFIG0_READOC_READ2IO  0x02UL
#define QSPI_IFCONFIG0_READOC_READ4O   0x03UL
#define QSPI_IFCONFIG0_READOC_READ4IO  0x04UL
#define QSPI_IFCONFIG0_WRITEOC_PP      0x00UL
#define QSPI_IFCONFIG0_WRITEOC_PP2O    0x01UL
#define QSPI_IFCONFIG0_WRITEOC_PP4O    0x02UL
#define QSPI_IFCONFIG0_WRITEOC_PP4IO   0x03UL

/* QSPI IFCONFIG1 fields */
#define QSPI_IFCONFIG1_SCKFREQ_DIV1    0x00UL  /* 64 MHz */
#define QSPI_IFCONFIG1_SCKFREQ_DIV2    0x01UL  /* 32 MHz */
#define QSPI_IFCONFIG1_SCKFREQ_DIV4    0x02UL  /* 16 MHz */
#define QSPI_IFCONFIG1_SCKFREQ_DIV8    0x03UL  /* 8 MHz */
#define QSPI_IFCONFIG1_SAMPLE_SHIFT    0x00UL
#define QSPI_IFCONFIG1_SPI_MODE0       0x00UL
#define QSPI_IFCONFIG1_SPI_MODE3       0x01UL

/* QSPI INTENSET bits */
#define QSPI_INT_READY                 (1UL << 2)

/* QSPI ENABLE */
#define QSPI_ENABLE_ENABLE             0x01UL
#define QSPI_ENABLE_DISABLE            0x00UL

/* TWIM (I²C Master with EasyDMA) */
typedef struct {
    volatile uint32_t TASKS_STARTRX;            /* 0x000 */
    volatile uint32_t RESERVED0;                /* 0x004 */
    volatile uint32_t TASKS_STARTTX;            /* 0x008 */
    volatile uint32_t RESERVED1[3];             /* 0x00C–0x014 */
    volatile uint32_t TASKS_STOP;               /* 0x018 */
    volatile uint32_t RESERVED2;                /* 0x01C */
    volatile uint32_t TASKS_SUSPEND;            /* 0x020 */
    volatile uint32_t TASKS_RESUME;             /* 0x024 */
    volatile uint32_t RESERVED3[54];            /* 0x028–0x0FC */
    volatile uint32_t EVENTS_STOPPED;           /* 0x100 */
    volatile uint32_t EVENTS_RXDREADY;          /* 0x104 */
    volatile uint32_t RESERVED4[4];             /* 0x108–0x114 */
    volatile uint32_t EVENTS_TXDSENT;           /* 0x118 */
    volatile uint32_t RESERVED5;                /* 0x11C */
    volatile uint32_t EVENTS_ERROR;             /* 0x120 */
    volatile uint32_t RESERVED6[8];             /* 0x124–0x144 */
    volatile uint32_t EVENTS_RXSTARTED;         /* 0x148 */
    volatile uint32_t EVENTS_TXSTARTED;         /* 0x14C */
    volatile uint32_t RESERVED7[1];             /* 0x150 */
    volatile uint32_t EVENTS_LASTRX;            /* 0x154 */
    volatile uint32_t EVENTS_LASTTX;            /* 0x158 */
    volatile uint32_t RESERVED8[105];           /* 0x15C–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED9[110];           /* 0x30C–0x4C4 */
    volatile uint32_t ERRORSRC;                 /* 0x4C8 */
    volatile uint32_t RESERVED10[14];           /* 0x4CC–0x4FC */
    volatile uint32_t ENABLE;                   /* 0x500 */
    volatile uint32_t RESERVED11;               /* 0x504 */
    volatile uint32_t PSEL_SDA;                 /* 0x508 */
    volatile uint32_t PSEL_SCL;                 /* 0x50C */
    volatile uint32_t RESERVED12[2];            /* 0x510–0x514 */
    volatile uint32_t FREQUENCY;               /* 0x518 */
    volatile uint32_t RESERVED13[3];            /* 0x51C–0x524 */
    volatile uint32_t RXD_PTR;                  /* 0x528 */
    volatile uint32_t RXD_MAXCNT;              /* 0x52C */
    volatile uint32_t RXD_AMOUNT;              /* 0x530 */
    volatile uint32_t RXD_LIST;                 /* 0x534 */
    volatile uint32_t TXD_PTR;                  /* 0x538 */
    volatile uint32_t TXD_MAXCNT;              /* 0x53C */
    volatile uint32_t TXD_AMOUNT;              /* 0x540 */
    volatile uint32_t TXD_LIST;                 /* 0x544 */
    volatile uint32_t ADDRESS;                  /* 0x548 */
} NRF_TWIM_Type;

#define NRF_TWIM0 ((NRF_TWIM_Type *) NRF_TWIM0_BASE)

/* TWIM FREQUENCY values */
#define TWIM_FREQ_100KBPS            0x01980000UL
#define TWIM_FREQ_250KBPS            0x04000000UL
#define TWIM_FREQ_400KBPS            0x06680000UL

/* USBD (USB Device) */
typedef struct {
    volatile uint32_t RESERVED0;                /* 0x000 */
    volatile uint32_t TASKS_STARTEPIN[8];       /* 0x004–0x020 */
    volatile uint32_t TASKS_STARTISOIN;         /* 0x024 */
    volatile uint32_t TASKS_STARTEPOUT[8];      /* 0x028–0x044 */
    volatile uint32_t TASKS_STARTISOOUT;        /* 0x048 */
    volatile uint32_t TASKS_EP0RCVOUT;          /* 0x04C */
    volatile uint32_t TASKS_EP0STATUS;          /* 0x050 */
    volatile uint32_t TASKS_EP0STALL;           /* 0x054 */
    volatile uint32_t TASKS_DPDMDRIVE;          /* 0x058 */
    volatile uint32_t TASKS_DPDMNODRIVE;        /* 0x05C */
    volatile uint32_t RESERVED1[40];            /* 0x060–0x0FC */
    volatile uint32_t EVENTS_USBRESET;          /* 0x100 */
    volatile uint32_t EVENTS_STARTED;           /* 0x104 */
    volatile uint32_t EVENTS_ENDEPIN[8];        /* 0x108–0x124 */
    volatile uint32_t EVENTS_EP0DATADONE;       /* 0x128 */
    volatile uint32_t EVENTS_ENDISOIN;          /* 0x12C */
    volatile uint32_t EVENTS_ENDEPOUT[8];       /* 0x130–0x14C */
    volatile uint32_t EVENTS_ENDISOOUT;         /* 0x150 */
    volatile uint32_t EVENTS_SOF;               /* 0x154 */
    volatile uint32_t EVENTS_USBEVENT;          /* 0x158 */
    volatile uint32_t EVENTS_EP0SETUP;          /* 0x15C */
    volatile uint32_t EVENTS_EPDATA;            /* 0x160 */
    volatile uint32_t RESERVED2[39];            /* 0x164–0x1FC */
    volatile uint32_t SHORTS;                   /* 0x200 */
    volatile uint32_t RESERVED3[63];            /* 0x204–0x2FC */
    volatile uint32_t INTEN;                    /* 0x300 */
    volatile uint32_t RESERVED4[63];            /* 0x304–0x3FC */
    volatile uint32_t EVENTCAUSE;               /* 0x400 */
    volatile uint32_t RESERVED5[7];             /* 0x404–0x41C */
    volatile uint32_t DPDMVALUE;                /* 0x420 */
    volatile uint32_t DTOGGLE;                  /* 0x424 */
    volatile uint32_t EPMODE;                   /* 0x428 */
    volatile uint32_t EPINEN;                   /* 0x42C */
    volatile uint32_t EPOUTEN;                  /* 0x430 */
    volatile uint32_t EPSTATUS;                 /* 0x434 */
    volatile uint32_t RESERVED6;                /* 0x438 */
    volatile uint32_t EPDATASTATUS;             /* 0x43C */
    volatile uint32_t USBADDR;                  /* 0x440 */
    volatile uint32_t RESERVED7[3];             /* 0x444–0x44C */
    volatile uint32_t BMREQUESTTYPE;            /* 0x450 */
    volatile uint32_t BREQUEST;                 /* 0x454 */
    volatile uint32_t WVALUEL;                  /* 0x458 */
    volatile uint32_t WVALUEH;                  /* 0x45C */
    volatile uint32_t WINDEXL;                  /* 0x460 */
    volatile uint32_t WINDEXH;                  /* 0x464 */
    volatile uint32_t WLENGTHL;                 /* 0x468 */
    volatile uint32_t WLENGTHH;                 /* 0x46C */
    volatile uint32_t RESERVED8[36];            /* 0x470–0x4FC */
    volatile uint32_t ENABLE;                   /* 0x500 */
    volatile uint32_t USBPULLUP;                /* 0x504 */
    volatile uint32_t RESERVED9[1];             /* 0x508 */
    volatile uint32_t EPINMAXPACKETSIZE;        /* 0x50C */
    volatile uint32_t EPOUTMAXPACKETSIZE;       /* 0x510 */
    volatile uint32_t RESERVED10[3];            /* 0x514–0x51C */
    volatile uint32_t ISOINCONFIG;              /* 0x520 */
    volatile uint32_t RESERVED11[15];           /* 0x524–0x55C */
    volatile uint32_t EPOUT[8].PTR;             /* 0x560–0x57C */
    volatile uint32_t EPOUT[8].MAXCNT;          /* 0x580–0x59C */
    volatile uint32_t EPOUT[8].AMOUNT;          /* 0x5A0–0x5BC */
    volatile uint32_t RESERVED12[8];            /* 0x5C0–0x5DC */
    volatile uint32_t EPIN[8].PTR;              /* 0x5E0–0x5FC */
    volatile uint32_t EPIN[8].MAXCNT;           /* 0x600–0x61C */
    volatile uint32_t EPIN[8].AMOUNT;           /* 0x620–0x63C */
    volatile uint32_t RESERVED13[8];            /* 0x640–0x65C */
    volatile uint32_t ISOOUT.PTR;               /* 0x660 */
    volatile uint32_t ISOOUT.MAXCNT;            /* 0x664 */
    volatile uint32_t ISOOUT.AMOUNT;            /* 0x668 */
    volatile uint32_t RESERVED14;               /* 0x66C */
    volatile uint32_t ISOIN.PTR;                /* 0x670 */
    volatile uint32_t ISOIN.MAXCNT;             /* 0x674 */
    volatile uint32_t ISOIN.AMOUNT;             /* 0x678 */
} NRF_USBD_Type;

#define NRF_USBD ((NRF_USBD_Type *) NRF_USBD_BASE)

/* USBD INTEN bits */
#define USBD_INT_USBRESET           (1UL << 0)
#define USBD_INT_STARTED            (1UL << 1)
#define USBD_INT_ENDEPIN0           (1UL << 2)
#define USBD_INT_ENDEPIN1           (1UL << 3)
#define USBD_INT_ENDEPIN2           (1UL << 4)
#define USBD_INT_EP0DATADONE        (1UL << 6)
#define USBD_INT_ENDEPOUT0          (1UL << 16)
#define USBD_INT_ENDEPOUT1          (1UL << 17)
#define USBD_INT_SOF                (1UL << 20)
#define USBD_INT_USBEVENT           (1UL << 21)
#define USBD_INT_EP0SETUP           (1UL << 22)
#define USBD_INT_EPDATA             (1UL << 23)

/* USBD ENABLE */
#define USBD_ENABLE_ENABLE          0x01UL
#define USBD_ENABLE_DISABLE         0x00UL

/* USBD USBPULLUP */
#define USBD_PULLUP_ENABLE          0x01UL
#define USBD_PULLUP_DISABLE         0x00UL

/* USBD DPDMVALUE */
#define USBD_DPDMVALUE_RESUME       0x01UL

/* TIMER */
typedef struct {
    volatile uint32_t TASKS_START;              /* 0x000 */
    volatile uint32_t TASKS_STOP;               /* 0x004 */
    volatile uint32_t TASKS_COUNT;              /* 0x008 */
    volatile uint32_t TASKS_CLEAR;              /* 0x00C */
    volatile uint32_t TASKS_SHUTDOWN;           /* 0x010 */
    volatile uint32_t RESERVED0[11];            /* 0x014–0x03C */
    volatile uint32_t TASKS_CAPTURE[6];         /* 0x040–0x054 */
    volatile uint32_t RESERVED1[42];            /* 0x058–0x0FC */
    volatile uint32_t EVENTS_COMPARE[6];        /* 0x140–0x154 */
    volatile uint32_t RESERVED2[106];           /* 0x158–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED3[126];           /* 0x30C–0x4FC */
    volatile uint32_t MODE;                     /* 0x504 */
    volatile uint32_t BITMODE;                  /* 0x508 */
    volatile uint32_t RESERVED4;                /* 0x50C */
    volatile uint32_t PRESCALER;                /* 0x510 */
    volatile uint32_t RESERVED5[11];            /* 0x514–0x53C */
    volatile uint32_t CC[6];                    /* 0x540–0x554 */
} NRF_TIMER_Type;

#define NRF_TIMER0 ((NRF_TIMER_Type *) NRF_TIMER0_BASE)
#define NRF_TIMER1 ((NRF_TIMER_Type *) NRF_TIMER1_BASE)
#define NRF_TIMER2 ((NRF_TIMER_Type *) NRF_TIMER2_BASE)

/* TIMER MODE */
#define TIMER_MODE_TIMER            0x00UL
#define TIMER_MODE_COUNTER          0x01UL

/* TIMER BITMODE */
#define TIMER_BITMODE_16BIT         0x00UL
#define TIMER_BITMODE_8BIT          0x01UL
#define TIMER_BITMODE_24BIT         0x02UL
#define TIMER_BITMODE_32BIT         0x03UL

/* TIMER PRESCALER (actual divider = 2^PRESCALER) */
#define TIMER_PRESCALER_DIV1        0x00UL
#define TIMER_PRESCALER_DIV2        0x01UL
#define TIMER_PRESCALER_DIV4        0x02UL
#define TIMER_PRESCALER_DIV8        0x03UL
#define TIMER_PRESCALER_DIV16       0x04UL

/* RTC */
typedef struct {
    volatile uint32_t TASKS_START;              /* 0x000 */
    volatile uint32_t TASKS_STOP;               /* 0x004 */
    volatile uint32_t TASKS_CLEAR;              /* 0x008 */
    volatile uint32_t TASKS_TRIGOVRFLW;         /* 0x00C */
    volatile uint32_t RESERVED0[60];            /* 0x010–0x0FC */
    volatile uint32_t EVENTS_TICK;              /* 0x100 */
    volatile uint32_t EVENTS_OVRFLW;            /* 0x104 */
    volatile uint32_t RESERVED1[14];            /* 0x108–0x13C */
    volatile uint32_t EVENTS_COMPARE[4];        /* 0x140–0x14C */
    volatile uint32_t RESERVED2[109];           /* 0x150–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED3[13];            /* 0x30C–0x33C */
    volatile uint32_t EVTEN;                    /* 0x340 */
    volatile uint32_t EVTENCLR;                 /* 0x344 */
    volatile uint32_t RESERVED4[46];            /* 0x348–0x3FC */
    volatile uint32_t COUNTER;                  /* 0x400 */
    volatile uint32_t PRESCALER;                /* 0x404 */
    volatile uint32_t RESERVED5[13];            /* 0x408–0x43C */
    volatile uint32_t CC[4];                    /* 0x440–0x44C */
} NRF_RTC_Type;

#define NRF_RTC0 ((NRF_RTC_Type *) NRF_RTC0_BASE)
#define NRF_RTC1 ((NRF_RTC_Type *) NRF_RTC1_BASE)
#define NRF_RTC2 ((NRF_RTC_Type *) NRF_RTC2_BASE)

/* GPIOTE */
typedef struct {
    volatile uint32_t TASKS_OUT[8];             /* 0x000–0x01C */
    volatile uint32_t RESERVED0[4];             /* 0x020–0x02C */
    volatile uint32_t TASKS_SET[8];             /* 0x030–0x04C */
    volatile uint32_t RESERVED1[4];             /* 0x050–0x05C */
    volatile uint32_t TASKS_CLR[8];             /* 0x060–0x07C */
    volatile uint32_t RESERVED2[32];            /* 0x080–0x0FC */
    volatile uint32_t EVENTS_IN[8];             /* 0x100–0x11C */
    volatile uint32_t RESERVED3[23];            /* 0x120–0x178 */
    volatile uint32_t EVENTS_PORT;              /* 0x17C */
    volatile uint32_t RESERVED4[97];            /* 0x180–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED5[129];           /* 0x30C–0x4FC */
    volatile uint32_t CONFIG[8];                /* 0x510–0x52C */
} NRF_GPIOTE_Type;

#define NRF_GPIOTE ((NRF_GPIOTE_Type *) NRF_GPIOTE_BASE)

/* GPIOTE CONFIG fields */
#define GPIOTE_CONFIG_MODE_Disabled  0x00UL
#define GPIOTE_CONFIG_MODE_Event     0x01UL
#define GPIOTE_CONFIG_MODE_Task      0x03UL
#define GPIOTE_CONFIG_POLARITY_LoToHi 0x02UL
#define GPIOTE_CONFIG_POLARITY_HiToLo 0x01UL
#define GPIOTE_CONFIG_POLARITY_Toggle 0x03UL
#define GPIOTE_CONFIG_OUTINIT_Low    0x00UL
#define GPIOTE_CONFIG_OUTINIT_High   0x01UL

/* GPIO Port */
typedef struct {
    volatile uint32_t RESERVED0[321];           /* 0x000–0x4FC */
    volatile uint32_t OUT;                      /* 0x504 */
    volatile uint32_t OUTSET;                   /* 0x508 */
    volatile uint32_t OUTCLR;                   /* 0x50C */
    volatile uint32_t IN;                       /* 0x510 */
    volatile uint32_t DIR;                      /* 0x514 */
    volatile uint32_t DIRSET;                   /* 0x518 */
    volatile uint32_t DIRCLR;                   /* 0x51C */
    volatile uint32_t LATCH;                    /* 0x520 */
    volatile uint32_t DETECTMODE;               /* 0x524 */
    volatile uint32_t RESERVED1[118];           /* 0x528–0x6FC */
    volatile uint32_t PIN_CNF[32];              /* 0x700–0x77C */
} NRF_GPIO_Type;

#define NRF_P0 ((NRF_GPIO_Type *) NRF_P0_BASE)
#define NRF_P1 ((NRF_GPIO_Type *) NRF_P1_BASE)

/* GPIO PIN_CNF fields */
#define GPIO_PIN_CNF_DIR_Input       0x00UL
#define GPIO_PIN_CNF_DIR_Output      0x01UL
#define GPIO_PIN_CNF_INPUT_Connect   0x00UL
#define GPIO_PIN_CNF_INPUT_Disconnect 0x01UL
#define GPIO_PIN_CNF_PULL_Disabled   0x00UL
#define GPIO_PIN_CNF_PULL_Pulldown   0x01UL
#define GPIO_PIN_CNF_PULL_Pullup     0x03UL
#define GPIO_PIN_CNF_DRIVE_S0S1      0x00UL  /* Standard 0, standard 1 */
#define GPIO_PIN_CNF_DRIVE_H0S1      0x01UL  /* High drive 0, standard 1 */
#define GPIO_PIN_CNF_DRIVE_S0H1      0x02UL  /* Standard 0, high drive 1 */
#define GPIO_PIN_CNF_DRIVE_H0H1      0x03UL  /* High drive 0, high drive 1 */
#define GPIO_PIN_CNF_SENSE_Disabled  0x00UL
#define GPIO_PIN_CNF_SENSE_High      0x02UL
#define GPIO_PIN_CNF_SENSE_Low       0x03UL

/* SAADC */
typedef struct {
    volatile uint32_t TASKS_START;              /* 0x000 */
    volatile uint32_t TASKS_SAMPLE;             /* 0x004 */
    volatile uint32_t TASKS_STOP;               /* 0x008 */
    volatile uint32_t TASKS_CALIBRATEOFFSET;    /* 0x00C */
    volatile uint32_t RESERVED0[60];            /* 0x010–0x0FC */
    volatile uint32_t EVENTS_STARTED;           /* 0x100 */
    volatile uint32_t EVENTS_END;               /* 0x104 */
    volatile uint32_t EVENTS_DONE;              /* 0x108 */
    volatile uint32_t EVENTS_RESULTDONE;        /* 0x10C */
    volatile uint32_t EVENTS_CALIBRATEDONE;     /* 0x110 */
    volatile uint32_t EVENTS_STOPPED;           /* 0x114 */
    volatile uint32_t RESERVED1[2];             /* 0x118–0x11C */
    volatile uint32_t EVENTS_CH_LIMITH[8];      /* 0x120–0x13C */
    volatile uint32_t EVENTS_CH_LIMITL[8];      /* 0x140–0x15C */
    volatile uint32_t RESERVED2[104];           /* 0x160–0x2FC */
    volatile uint32_t INTEN;                    /* 0x300 */
    volatile uint32_t INTENCLR;                 /* 0x304 */
    volatile uint32_t RESERVED3[61];            /* 0x308–0x3FC */
    volatile uint32_t STATUS;                   /* 0x400 */
    volatile uint32_t RESERVED4[63];            /* 0x404–0x4FC */
    volatile uint32_t ENABLE;                   /* 0x500 */
    volatile uint32_t RESERVED5[3];             /* 0x504–0x50C */
    volatile uint32_t CH[0].PSELP;              /* 0x510 */
    volatile uint32_t CH[0].PSELN;              /* 0x514 */
    volatile uint32_t CH[0].CONFIG;             /* 0x518 */
    volatile uint32_t CH[0].LIMIT;              /* 0x51C */
    /* ... channels 1-7 at 0x520–0x58C */
    volatile uint32_t RESERVED6[24];            /* 0x590–0x5EC */
    volatile uint32_t RESOLUTION;               /* 0x5F0 */
    volatile uint32_t OVERSAMPLE;               /* 0x5F4 */
    volatile uint32_t SAMPLERATE;               /* 0x5F8 */
    volatile uint32_t RESERVED7[12];            /* 0x5FC–0x62C */
    volatile uint32_t RESULT.PTR;               /* 0x630 */
    volatile uint32_t RESULT.MAXCNT;            /* 0x634 */
    volatile uint32_t RESULT.AMOUNT;            /* 0x638 */
} NRF_SAADC_Type;

#define NRF_SAADC ((NRF_SAADC_Type *) NRF_SAADC_BASE)

/* SAADC CH CONFIG fields */
#define SAADC_CH_CONFIG_GAIN_Gain1_6  0x00UL
#define SAADC_CH_CONFIG_GAIN_Gain1_5  0x01UL
#define SAADC_CH_CONFIG_GAIN_Gain1_4  0x02UL
#define SAADC_CH_CONFIG_GAIN_Gain1_3  0x03UL
#define SAADC_CH_CONFIG_GAIN_Gain1_2  0x04UL
#define SAADC_CH_CONFIG_GAIN_Gain1    0x05UL
#define SAADC_CH_CONFIG_GAIN_Gain2    0x06UL
#define SAADC_CH_CONFIG_GAIN_Gain4    0x07UL
#define SAADC_CH_CONFIG_REFSEL_Internal 0x00UL
#define SAADC_CH_CONFIG_TACQ_3us      0x00UL
#define SAADC_CH_CONFIG_TACQ_5us      0x01UL
#define SAADC_CH_CONFIG_TACQ_10us     0x02UL
#define SAADC_CH_CONFIG_TACQ_15us     0x03UL
#define SAADC_CH_CONFIG_TACQ_20us     0x04UL
#define SAADC_CH_CONFIG_TACQ_40us     0x05UL
#define SAADC_CH_CONFIG_MODE_SE       0x00UL  /* Single-ended */
#define SAADC_CH_CONFIG_MODE_Diff     0x01UL  /* Differential */
#define SAADC_CH_CONFIG_BURST_Disabled 0x00UL
#define SAADC_CH_CONFIG_BURST_Enabled 0x01UL

/* SAADC RESOLUTION */
#define SAADC_RESOLUTION_8bit         0x00UL
#define SAADC_RESOLUTION_10bit        0x01UL
#define SAADC_RESOLUTION_12bit        0x02UL
#define SAADC_RESOLUTION_14bit        0x03UL

/* WDT */
typedef struct {
    volatile uint32_t TASKS_START;              /* 0x000 */
    volatile uint32_t RESERVED0[63];            /* 0x004–0x0FC */
    volatile uint32_t EVENTS_TIMEOUT;           /* 0x100 */
    volatile uint32_t RESERVED1[128];           /* 0x104–0x2FC */
    volatile uint32_t INTENSET;                 /* 0x304 */
    volatile uint32_t INTENCLR;                 /* 0x308 */
    volatile uint32_t RESERVED2[61];            /* 0x30C–0x3FC */
    volatile uint32_t RUNSTATUS;                /* 0x400 */
    volatile uint32_t REQSTATUS;                /* 0x404 */
    volatile uint32_t RESERVED3[63];            /* 0x408–0x4FC */
    volatile uint32_t CRV;                      /* 0x504 */
    volatile uint32_t RREN;                     /* 0x508 */
    volatile uint32_t CONFIG;                   /* 0x50C */
    volatile uint32_t RESERVED4[60];            /* 0x510–0x5FC */
    volatile uint32_t RR[8];                    /* 0x600–0x61C */
} NRF_WDT_Type;

#define NRF_WDT ((NRF_WDT_Type *) NRF_WDT_BASE)

/* WDT CONFIG */
#define WDT_CONFIG_SLEEP_Run         0x00UL
#define WDT_CONFIG_SLEEP_Pause       0x01UL
#define WDT_CONFIG_HALT_Run          0x00UL
#define WDT_CONFIG_HALT_Pause        0x01UL

/* WDT RR */
#define WDT_RR_RR_Reload             0x6E524635UL

/* NVMC */
typedef struct {
    volatile uint32_t RESERVED0[256];           /* 0x000–0x3FC */
    volatile uint32_t READY;                    /* 0x400 */
    volatile uint32_t RESERVED1[1];             /* 0x404 */
    volatile uint32_t READYNEXT;                /* 0x408 */
    volatile uint32_t RESERVED2[62];            /* 0x40C–0x4FC */
    volatile uint32_t CONFIG;                   /* 0x504 */
    volatile uint32_t ERASEPAGE;                /* 0x508 */
    volatile uint32_t ERASEPCR1;                /* 0x50C */
    volatile uint32_t ERASEALL;                 /* 0x510 */
    volatile uint32_t ERASEPCR0;                /* 0x514 */
    volatile uint32_t ERASEUICR;                /* 0x518 */
    volatile uint32_t RESERVED3[10];            /* 0x51C–0x540 */
    volatile uint32_t ICACHECNF;                /* 0x544 */
    volatile uint32_t RESERVED4;                /* 0x548 */
    volatile uint32_t IHIT;                     /* 0x54C */
    volatile uint32_t IMISS;                    /* 0x550 */
} NRF_NVMC_Type;

#define NRF_NVMC ((NRF_NVMC_Type *) NRF_NVMC_BASE)

/* NVMC CONFIG */
#define NVMC_CONFIG_WEN_Ren          0x00UL
#define NVMC_CONFIG_WEN_Wen          0x01UL
#define NVMC_CONFIG_WEN_Een          0x02UL

/* FICR (read-only factory info) */
typedef struct {
    volatile uint32_t RESERVED0[4];             /* 0x000–0x00C */
    volatile uint32_t CODEPAGESIZE;             /* 0x010 */
    volatile uint32_t CODESIZE;                 /* 0x014 */
    volatile uint32_t RESERVED1[18];            /* 0x018–0x05C */
    volatile uint32_t DEVICEID[2];              /* 0x060–0x064 */
    volatile uint32_t RESERVED2[6];             /* 0x068–0x07C */
    volatile uint32_t ER[4];                    /* 0x080–0x08C */
    volatile uint32_t IR[4];                    /* 0x090–0x09C */
    volatile uint32_t DEVICEADDRTYPE;           /* 0x0A0 */
    volatile uint32_t DEVICEADDR[2];            /* 0x0A4–0x0A8 */
    volatile uint32_t RESERVED3[21];            /* 0x0AC–0x0FC */
    volatile uint32_t INFO_PART;                /* 0x100 */
    volatile uint32_t INFO_VARIANT;             /* 0x104 */
    volatile uint32_t INFO_PACKAGE;             /* 0x108 */
    volatile uint32_t INFO_RAM;                 /* 0x10C */
    volatile uint32_t INFO_FLASH;               /* 0x110 */
    volatile uint32_t RESERVED4[3];             /* 0x114–0x11C */
    volatile uint32_t TEMP_T0;                  /* 0x120 */
    volatile uint32_t TEMP_T1;                  /* 0x124 */
    volatile uint32_t TEMP_T2;                  /* 0x128 */
    volatile uint32_t TEMP_T3;                  /* 0x12C */
    volatile uint32_t TEMP_T4;                  /* 0x130 */
} NRF_FICR_Type;

#define NRF_FICR ((NRF_FICR_Type *) NRF_FICR_BASE)

/* UICR (user info configuration) */
typedef struct {
    volatile uint32_t RESERVED0[5];             /* 0x000–0x010 */
    volatile uint32_t NRFFW[15];                /* 0x014–0x04C */
    volatile uint32_t RESERVED1[12];            /* 0x050–0x07C */
    volatile uint32_t NRFHW[12];                /* 0x080–0x0AC */
    volatile uint32_t RESERVED2[20];            /* 0x0B0–0x0FC */
    volatile uint32_t CUSTOMER[32];             /* 0x100–0x17C */
    volatile uint32_t RESERVED3[64];            /* 0x180–0x27C */
    volatile uint32_t PSELRESET[2];             /* 0x280–0x284 */
    volatile uint32_t RESERVED4[30];            /* 0x288–0x2FC */
    volatile uint32_t APPROTECT;                /* 0x300 */
    volatile uint32_t RESERVED5;                /* 0x304 */
    volatile uint32_t REGOUT0;                  /* 0x308 */
    volatile uint32_t RESERVED6[1];             /* 0x30C */
    volatile uint32_t XTALFREQ;                 /* 0x310 */
} NRF_UICR_Type;

#define NRF_UICR ((NRF_UICR_Type *) NRF_UICR_BASE)

/* UICR APPROTECT */
#define UICR_APPROTECT_ENABLED       0xFFFFFF00UL
#define UICR_APPROTECT_DISABLED      0xFFFFFFFFUL

/* UICR REGOUT0 */
#define UICR_REGOUT0_VOUT_1V8        0xFFFFFFF0UL
#define UICR_REGOUT0_VOUT_2V1        0xFFFFFFF1UL
#define UICR_REGOUT0_VOUT_2V4        0xFFFFFFF2UL
#define UICR_REGOUT0_VOUT_2V7        0xFFFFFFF3UL
#define UICR_REGOUT0_VOUT_3V0        0xFFFFFFF4UL
#define UICR_REGOUT0_VOUT_3V3        0xFFFFFFF5UL

/*===========================================================================
 * NVIC INTERRUPT NUMBERS
 *===========================================================================*/

#define NVIC_IRQ_POWER_CLOCK         0
#define NVIC_IRQ_RADIO               1
#define NVIC_IRQ_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0 3
#define NVIC_IRQ_SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1 4
#define NVIC_IRQ_GPIOTE              6
#define NVIC_IRQ_SAADC               7
#define NVIC_IRQ_TIMER0              8
#define NVIC_IRQ_TIMER1              9
#define NVIC_IRQ_TIMER2              10
#define NVIC_IRQ_RTC0                11
#define NVIC_IRQ_RTC1                12
#define NVIC_IRQ_RTC2                13
#define NVIC_IRQ_WDT                 16
#define NVIC_IRQ_QSPI                25
#define NVIC_IRQ_USBD                39
#define NVIC_IRQ_SWI0                20
#define NVIC_IRQ_SWI1                21

/*===========================================================================
 * HELPER MACROS
 *===========================================================================*/

/* Register field manipulation */
#define REG_SET(reg, mask)           ((reg) |= (mask))
#define REG_CLR(reg, mask)           ((reg) &= ~(mask))
#define REG_GET(reg, mask)           ((reg) & (mask))
#define REG_SET_VAL(reg, mask, val)  ((reg) = ((reg) & ~(mask)) | ((val) & (mask)))

/* NVIC operations */
#define NVIC_ENABLE_IRQ(irq)         (*(volatile uint32_t *)(0xE000E100UL + ((irq) >> 5) * 4) = (1UL << ((irq) & 0x1F)))
#define NVIC_DISABLE_IRQ(irq)        (*(volatile uint32_t *)(0xE000E180UL + ((irq) >> 5) * 4) = (1UL << ((irq) & 0x1F)))
#define NVIC_SET_PRIORITY(irq, pri)  (*(volatile uint8_t *)(0xE000E400UL + (irq)) = (uint8_t)((pri) << 4))
#define NVIC_CLEAR_PENDING(irq)      (*(volatile uint32_t *)(0xE000E280UL + ((irq) >> 5) * 4) = (1UL << ((irq) & 0x1F)))

/* System control */
#define __WFE()                      __asm volatile ("wfe")
#define __WFI()                      __asm volatile ("wfi")
#define __SEV()                      __asm volatile ("sev")
#define __ISB()                      __asm volatile ("isb")
#define __DSB()                      __asm volatile ("dsb")
#define __DMB()                      __asm volatile ("dmb")

/* Critical section helpers */
#define CRITICAL_REGION_ENTER()      do { uint32_t __primask = __get_PRIMASK(); __set_PRIMASK(1); (void)__primask
#define CRITICAL_REGION_EXIT()       __set_PRIMASK(__primask); } while(0)

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */
