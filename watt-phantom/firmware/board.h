/*
 * board.h — WattPhantom pin assignments, constants, and data structures
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Pin assignments for the STM32G474QBT6 on the WattPhantom board.
 * The board has two USB-C ports (source and sink), two FUSB302B CC
 * transceivers, two INA226 current monitors, an nRF52840 BLE module,
 * an SSD1306 OLED, and a MAX17048 fuel gauge.
 */
#ifndef WATTPHANTOM_BOARD_H
#define WATTPHANTOM_BOARD_H

#include <stdint.h>
#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  BOARD REVISION / AUTHOR
 * ================================================================ */
#define WATTPHANTOM_VERSION   "1.0"
#define WATTPHANTOM_AUTHOR    "jayis1"
#define WATTPHANTOM_BOARD_NAME "WattPhantom"

/* ================================================================
 *  SYSTEM CLOCK
 *  STM32G474: HSI 16 MHz, PLL to 170 MHz (SYSCLK)
 *  AHB = 170 MHz, APB1 = 170 MHz, APB2 = 170 MHz
 * ================================================================ */
#define SYSCLK_HZ        170000000u
#define HSI_HZ           16000000u
#define APB1_HZ          170000000u
#define APB2_HZ          170000000u

/* ================================================================
 *  PIN ASSIGNMENTS  (STM32G474QBT6 — QFP-48)
 * ================================================================
 *
 *  Pin  | Signal       | Function
 *  -----|-------------|------------------------------------------
 *  PA2  | USART2_TX   | BLE module UART TX (to NINA-B302 RXD)
 *  PA3  | USART2_RX   | BLE module UART RX (from NINA-B302 TXD)
 *  PA9  | USART1_TX   | USB CDC debug TX
 *  PA10 | USART1_RX   | USB CDC debug RX
 *  PA11 | USB_DM      | USB D- (via FSUSB42 switch)
 *  PA12 | USB_DP      | USB D+ (via FSUSB42 switch)
 *  PB6  | I2C1_SCL    | I²C bus 1: FUSB302B #1 (source), INA226 #1
 *  PB7  | I2C1_SDA    | I²C bus 1
 *  PB10 | I2C2_SCL    | I²C bus 2: FUSB302B #2 (sink), INA226 #2,
 *  |                 |   SSD1306 OLED, MAX17048 fuel gauge
 *  PB11 | I2C2_SDA    | I²C bus 2
 *  PB13 | SPI2_SCK    | (reserved for external flash)
 *  PB14 | SPI2_MISO   | (reserved)
 *  PB15 | SPI2_MOSI   | (reserved)
 *  PC13 | GPIO_OUT    | Status LED (red)
 *  PC14 | GPIO_OUT    | Status LED (green)
 *  PC15 | GPIO_OUT    | Status LED (blue)
 *  PA0  | ADC1_IN1    | VBUS source voltage divider (1/12)
 *  PA1  | ADC1_IN2    | VBUS sink voltage divider (1/12)
 *  PA4  | GPIO_OUT    | TMUX2512 #1 SEL0 (CC routing - source)
 *  PA5  | GPIO_OUT    | TMUX2512 #1 SEL1
 *  PA6  | GPIO_OUT    | TMUX2512 #2 SEL0 (CC routing - sink)
 *  PA7  | GPIO_OUT    | TMUX2512 #2 SEL1
 *  PB0  | GPIO_OUT    | eFuse #1 EN (source VBUS eFuse)
 *  PB1  | GPIO_OUT    | eFuse #2 EN (sink VBUS eFuse)
 *  PB2  | GPIO_OUT    | MOSFET gate driver 1 EN
 *  PB3  | GPIO_OUT    | MOSFET gate driver 2 EN
 *  PB4  | GPIO_OUT    | FSUSB42 USB switch SEL
 *  PB5  | GPIO_OUT    | FSUSB42 USB switch EN
 *  PB8  | GPIO_OUT    | Boost converter EN
 *  PB9  | GPIO_OUT    | Boost converter PWM (TIM4 CH4 alt)
 *  PB12 | GPIO_OUT    | OLED RST#
 *  PB3_ALT | UCPD1_CC1 | UCPD1 CC1 (if using internal PHY)
 *  PB4_ALT | UCPD1_CC2 | UCPD1 CC2
 *  PA8  | UCPD1_FRST  | UCPD1 Framing Start
 *  PA15 | UCPD2_CC1   | UCPD2 CC1 (secondary, unused in this design)
 * ================================================================ */

/* ---- I²C bus assignments ---- */
#define I2C1_BUS       I2C1_BASE   /* FUSB302B #1 (src), INA226 #1 (src) */
#define I2C2_BUS       I2C2_BASE   /* FUSB302B #2 (snk), INA226 #2 (snk),
                                    * SSD1306, MAX17048 */

/* ---- GPIO pin helpers ---- */
#define PIN(port, num) ((port) + (num))

/* ---- CC routing modes (TMUX2512 4:1 mux) ---- */
typedef enum {
    CC_MODE_PASSTHROUGH = 0,  /* CC1_src→CC1_snk, CC2_src→CC2_snk (MITM) */
    CC_MODE_ISOLATED    = 1,  /* Both ports isolated, MCU controls each */
    CC_MODE_CROSSOVER   = 2,  /* CC1_src→CC2_snk, CC2_src→CC1_snk */
    CC_MODE_MCU_CTRL    = 3,  /* MCU directly drives CC (active attack) */
} cc_routing_mode_t;

/* ---- PD port identifiers ---- */
typedef enum {
    PD_PORT_SOURCE = 0,   /* The "upstream" USB-C (connects to charger/host) */
    PD_PORT_SINK   = 1,   /* The "downstream" USB-C (connects to target) */
    PD_PORT_COUNT  = 2,
} pd_port_t;

/* ---- PD message types (SOP) ---- */
typedef enum {
    PD_MSG_CONTROL         = 0u,
    PD_MSG_DATA            = 1u,
    PD_MSG_EXTENDED       = 2u,
} pd_msg_type_class_t;

/* Control message subtypes */
typedef enum {
    PD_CTRL_GOODCRC        = 0x01u,
    PD_CTRL_GOTOMIN        = 0x02u,
    PD_CTRL_ACCEPT         = 0x03u,
    PD_CTRL_REJECT         = 0x04u,
    PD_CTRL_PING           = 0x05u,
    PD_CTRL_PS_RDY         = 0x06u,
    PD_CTRL_SOFT_RESET     = 0x07u,
    PD_CTRL_NOT_SUPPORTED  = 0x08u,
    PD_CTRL_GET_SOURCE_CAP = 0x09u,
    PD_CTRL_GET_SINK_CAP   = 0x0Au,
    PD_CTRL_DR_SWAP        = 0x0Bu,
    PD_CTRL_PR_SWAP        = 0x0Cu,
    PD_CTRL_VCONN_SWAP     = 0x0Du,
    PD_CTRL_WAIT           = 0x0Eu,
    PD_CTRL_FR_SWAP        = 0x0Fu,
} pd_ctrl_msg_t;

/* Data message subtypes */
typedef enum {
    PD_DATA_SOURCE_CAP     = 0x01u,
    PD_DATA_REQUEST        = 0x02u,
    PD_DATA_BIST           = 0x03u,
    PD_DATA_SINK_CAP       = 0x04u,
    PD_DATA_BATTERY_STATUS = 0x05u,
    PD_DATA_BATTERY_CAP    = 0x06u,
    PD_DATA_ALERT          = 0x07u,
    PD_DATA_GET_STATUS     = 0x08u,
    PD_DATA_GET_SOURCE_INFO= 0x09u,
    PD_DATA_GET_REVISION   = 0x0Au,
    PD_DATA_VDM            = 0x0Fu,
} pd_data_msg_t;

/* ---- PDO (Power Data Object) types ---- */
typedef enum {
    PDO_TYPE_FIXED    = 0u,
    PDO_TYPE_BATTERY  = 1u,
    PDO_TYPE_VARIABLE = 2u,
    PDO_TYPE_APDO     = 3u,
} pdo_type_t;

/* Fixed PDO bit layout (32-bit) */
#define PDO_FIXED_DUAL_ROLE  (1u << 27)
#define PDO_FIXED_SUSPEND    (1u << 26)
#define PDO_FIXED_UNCONSTRAINED (1u << 24)
#define PDO_FIXED_USB_COMM   (1u << 25)
#define PDO_FIXED_DUAL_ROLE_DATA (1u << 23)
#define PDO_FIXED_VOLT_SHIFT 10u
#define PDO_FIXED_CURR_SHIFT 0u
#define PDO_TYPE_SHIFT       30u

/* Build a fixed PDO: voltage in 50mV units, current in 10mA units */
#define MAKE_FIXED_PDO(v_mv, i_ma) \
    ((uint32_t)(PDO_TYPE_FIXED << PDO_TYPE_SHIFT) | \
     ((uint32_t)((v_mv) / 50u) << PDO_FIXED_VOLT_SHIFT) | \
     ((uint32_t)((i_ma) / 10u) << PDO_FIXED_CURR_SHIFT))

/* Extract voltage (mV) and current (mA) from a fixed PDO */
#define PDO_FIXED_VOLTAGE_MV(pdo) ((((pdo) >> PDO_FIXED_VOLT_SHIFT) & 0x3FFu) * 50u)
#define PDO_FIXED_CURRENT_MA(pdo) ((((pdo) >> PDO_FIXED_CURR_SHIFT) & 0x3FFu) * 10u)

/* ---- VDM (Vendor Defined Message) types ---- */
#define VDM_TYPE_SHIFT        15u
#define VDM_TYPE_REQUEST      (0u << VDM_TYPE_SHIFT)
#define VDM_TYPE_RESPONSE     (1u << VDM_TYPE_SHIFT)
#define VDM_STRUCT            (1u << 14u)  /* Structured VDM */
#define VDM_UNSTRUCT          (0u << 14u)  /* Unstructured VDM */

/* VDM command types */
typedef enum {
    VDM_CMD_DISCOVER_IDENTITY = 1u,
    VDM_CMD_DISCOVER_SVIDS    = 2u,
    VDM_CMD_DISCOVER_MODES    = 3u,
    VDM_CMD_ENTER_MODE        = 4u,
    VDM_CMD_EXIT_MODE         = 5u,
    VDM_CMD_ATTENTION         = 6u,
    VDM_CMD_DP_STATUS         = 7u,
    VDM_CMD_DP_CONFIG         = 8u,
    VDM_CMD_COVERT_DATA       = 0x7Fu,  /* Custom command for covert channel */
} vdm_cmd_t;

/* ---- PD contract state ---- */
typedef enum {
    CONTRACT_STATE_IDLE        = 0,
    CONTRACT_STATE_SRC_SEND_CAP= 1,
    CONTRACT_STATE_SNK_REQUEST = 2,
    CONTRACT_STATE_SRC_ACCEPT  = 3,
    CONTRACT_STATE_PS_RDY_SRC  = 4,
    CONTRACT_STATE_PS_RDY_SNK  = 5,
    CONTRACT_STATE_ACTIVE      = 6,
    CONTRACT_STATE_HARD_RESET  = 7,
    CONTRACT_STATE_ERROR       = 8,
} contract_state_t;

/* ---- PD message structure ---- */
#define PD_MAX_DATA_OBJS  7u
typedef struct {
    uint16_t header;          /* PD message header (16-bit) */
    uint8_t  num_objs;        /* Number of 32-bit data objects (0-7) */
    uint32_t data_obj[PD_MAX_DATA_OBJS]; /* Data objects */
} pd_msg_t;

/* PD header bit fields */
#define PD_HDR_MSG_TYPE_SHIFT   0u
#define PD_HDR_PORT_ROLE_SHIFT  8u   /* 0=Sinking, 1=Sourcing */
#define PD_HDR_DATA_ROLE_SHIFT  9u   /* 0=UFP, 1=DFP */
#define PD_HDR_MSG_ID_SHIFT     11u
#define PD_HDR_NUM_OBJ_SHIFT    12u
#define PD_HDR_REV_SHIFT        14u  /* 1=PD2.0, 2=PD3.0, 3=PD3.1 */
#define PD_HDR_HDR_LEN          2u   /* Header is 2 bytes */

/* ---- Power monitoring ---- */
typedef struct {
    uint16_t bus_voltage_mv;  /* VBUS voltage in mV */
    int16_t  current_ma;      /* Current in mA (signed, + = sourcing) */
    uint16_t power_mw;        /* Power in mW */
    uint32_t timestamp_ms;    /* Sample timestamp */
} power_sample_t;

#define POWER_PROFILE_MAX_SAMPLES  4096u

/* ---- Fingerprint feature vector ---- */
#define FP_MAX_PDO_ENTRIES  10u
#define FP_TIMING_BINS      16u
typedef struct {
    uint32_t pdo_bitmap;               /* Bitmask of PDO types requested */
    uint8_t  pdo_request_order[FP_MAX_PDO_ENTRIES]; /* Order of PDO requests */
    uint8_t  num_pdo_requests;
    uint16_t timing_histogram[FP_TIMING_BINS]; /* Inter-message timing */
    uint16_t negotiation_time_ms;      /* Total PD negotiation time */
    uint16_t initial_current_ma;       /* Current draw at PS_RDY */
    uint16_t steady_current_ma;        /* Steady-state current */
    uint8_t  msg_count;                /* Total messages in negotiation */
    uint8_t  device_class;             /* Matched device class (0=unknown) */
} pd_fingerprint_t;

/* ---- Covert channel ---- */
#define COVERT_MAX_PAYLOAD  252u   /* Max bytes per VDM data block */
#define COVERT_QUEUE_SIZE   1024u
typedef struct {
    uint8_t  tx_queue[COVERT_QUEUE_SIZE];
    uint16_t tx_head;
    uint16_t tx_tail;
    uint8_t  rx_queue[COVERT_QUEUE_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint8_t  seq_num;
    uint8_t  active;
} covert_channel_t;

/* ---- Safety limits ---- */
typedef struct {
    uint16_t ovp_threshold_mv;   /* Max VBUS voltage allowed */
    uint16_t ocp_threshold_ma;   /* Max current allowed */
    uint8_t  attack_mode_enabled; /* If 0, safety limits enforced */
    uint8_t  thermal_limit_c;    /* Max operating temperature */
} safety_limits_t;

#define DEFAULT_OVP_MV   20000u  /* 20V default ceiling */
#define DEFAULT_OCP_MA   5000u   /* 5A default ceiling */
#define DEFAULT_THERMAL  60u     /* 60°C */

/* ---- BLE command queue ---- */
#define CMD_QUEUE_SIZE  256u
#define CMD_MAX_LEN     128u
typedef struct {
    char     buf[CMD_QUEUE_SIZE][CMD_MAX_LEN];
    uint16_t head;
    uint16_t tail;
} cmd_queue_t;

/* ---- Global device state ---- */
typedef struct {
    contract_state_t contract_state[PD_PORT_COUNT];
    uint32_t active_pdo[PD_PORT_COUNT];     /* Currently negotiated PDO */
    uint16_t vbus_mv[PD_PORT_COUNT];       /* Current VBUS voltage */
    int16_t  vbus_ma[PD_PORT_COUNT];       /* Current VBUS current */
    cc_routing_mode_t cc_mode;
    uint8_t  msg_id[PD_PORT_COUNT];        /* PD message ID counter */
    safety_limits_t safety;
    covert_channel_t covert;
    pd_fingerprint_t fingerprint;
    uint8_t  battery_pct;
    uint8_t  ble_connected;
    uint8_t  usb_data_passthrough;  /* 1 = USB data passes through */
    uint8_t  power_profiling;
    uint16_t profile_sample_count;
} device_state_t;

/* ---- Function prototypes (implemented in drivers/) ---- */
extern void board_init(void);
extern void i2c_init(uint32_t base, uint32_t timingr);
extern int  i2c_write_reg(uint32_t base, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len);
extern int  i2c_read_reg(uint32_t base, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int  i2c_write_burst(uint32_t base, uint8_t dev_addr, const uint8_t *data, uint16_t len);
extern int  i2c_read_burst(uint32_t base, uint8_t dev_addr, uint8_t *data, uint16_t len);

/* CC PHY (FUSB302B) */
extern int  cc_phy_init(pd_port_t port);
extern int  cc_phy_send_msg(pd_port_t port, const pd_msg_t *msg);
extern int  cc_phy_recv_msg(pd_port_t port, pd_msg_t *msg, uint32_t timeout_ms);
extern void cc_phy_set_role(pd_port_t port, uint8_t is_source);
extern void cc_phy_flush_rx(pd_port_t port);
extern uint8_t cc_phy_get_cc_status(pd_port_t port);

/* PD controller */
extern void pd_init(void);
extern void pd_set_routing(cc_routing_mode_t mode);
extern int  pd_send_source_cap(pd_port_t port, const uint32_t *pdos, uint8_t count);
extern int  pd_send_request(pd_port_t port, uint32_t pdo_index);
extern int  pd_send_control(pd_port_t port, pd_ctrl_msg_t ctrl);
extern int  pd_send_hard_reset(pd_port_t port);
extern int  pd_send_pr_swap(pd_port_t port);
extern int  pd_send_dr_swap(pd_port_t port);
extern int  pd_send_vdm(pd_port_t port, uint16_t vdm_header, const uint32_t *data, uint8_t count);
extern void pd_poll(void);
extern const char *pd_state_name(contract_state_t s);

/* VBUS switching */
extern void vbus_init(void);
extern int  vbus_set_voltage(pd_port_t port, uint16_t target_mv);
extern void vbus_enable_path(pd_port_t port, uint8_t enable);
extern void vbus_disconnect(pd_port_t port);
extern void vbus_set_efuse_limits(pd_port_t port, uint16_t ovp_mv, uint16_t ocp_ma);

/* Current monitor (INA226) */
extern void current_monitor_init(void);
extern int  current_monitor_read(pd_port_t port, uint16_t *voltage_mv, int16_t *current_ma, uint16_t *power_mw);
extern void current_monitor_set_alert(pd_port_t port, uint16_t ovp_mv, uint16_t ocp_ma);

/* Covert channel */
extern void covert_channel_init(void);
extern int  covert_channel_tx(const uint8_t *data, uint16_t len);
extern int  covert_channel_rx(uint8_t *data, uint16_t maxlen, uint32_t timeout_ms);
extern void covert_channel_poll(void);
extern uint16_t covert_channel_tx_pending(void);
extern uint16_t covert_channel_rx_pending(void);

/* Fingerprinting */
extern void fingerprint_init(void);
extern void fingerprint_start_capture(void);
extern int  fingerprint_match(const pd_fingerprint_t *fp, uint8_t *device_class);
extern const char *fingerprint_class_name(uint8_t cls);

/* BLE UART */
extern int  ble_uart_init(void);
extern int  ble_uart_recv(uint8_t *buf, uint32_t maxlen, uint32_t timeout_ms);
extern int  ble_uart_send(const uint8_t *buf, uint32_t len);
extern void ble_uart_poll(void);

/* USB CDC */
extern int  usb_cdc_init(void);
extern int  usb_cdc_recv(uint8_t *buf, uint32_t maxlen, uint32_t timeout_ms);
extern int  usb_cdc_send(const uint8_t *buf, uint32_t len);

/* OLED */
extern void oled_init(void);
extern void oled_clear(void);
extern void oled_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t size);
extern void oled_draw_status(const char *line1, const char *line2, const char *line3);

/* Fuel gauge */
extern int  fuel_gauge_init(void);
extern uint8_t fuel_gauge_read_percent(void);
extern uint16_t fuel_gauge_read_voltage_mv(void);

/* CC routing (TMUX2512) */
extern void cc_routing_set(cc_routing_mode_t mode);

/* USB data switch (FSUSB42) */
extern void usb_data_switch_init(void);
extern void usb_data_switch_set_passthrough(uint8_t enable);

/* Boost converter */
extern void boost_init(void);
extern int  boost_set_voltage(uint16_t target_mv);
extern void boost_enable(uint8_t enable);

/* Utility */
extern uint32_t millis(void);
extern void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* WATTPHANTOM_BOARD_H */