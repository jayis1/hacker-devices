/**
 * @file    registers.h
 * @brief   HDMI-Siphon FPGA register map and bitfield definitions
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * This file defines the complete register map for the FPGA (iCE40UP5K)
 * as seen from the ESP32-S3 master. All communication between the 
 * ESP32-S3 and the FPGA occurs via a 20 MHz SPI bus using 32-bit 
 * register read/write transactions.
 *
 * Register access protocol:
 *   - Write:  [CMD_WRITE (0x80 | addr_hi)] [addr_lo] [data_hi] [data_lo]
 *   - Read:   [CMD_READ (0x00 | addr_hi)] [addr_lo] [dummy] [data_out]
 *   - Each transaction is 4 bytes over SPI (MOSI/MISO simultaneous)
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* =========================================================================
 * SPI Command Definitions
 * ========================================================================= */

/** SPI write command prefix (OR with register address high byte) */
#define SPI_CMD_WRITE                  0x80U

/** SPI read command prefix (OR with register address high byte) */
#define SPI_CMD_READ                   0x00U

/** SPI transaction size in bytes */
#define SPI_TRANSACTION_SIZE               4U

/** Max number of consecutive registers to read/write in burst mode */
#define SPI_BURST_MAX                    256U

/* =========================================================================
 * Register Map Summary
 *
 * Address Range      | Region          | Access | Description
 * -------------------|-----------------|--------|---------------------------
 * 0x0000 - 0x000F   | SYSTEM          | R/W    | System control & status
 * 0x0010 - 0x001F   | VIDEO_PATH      | R/W    | Video pipeline control
 * 0x0020 - 0x002F   | OSD             | R/W    | On-screen display overlay
 * 0x0030 - 0x003F   | CEC             | R/W    | CEC bus controller
 * 0x0040 - 0x004F   | CAPTURE         | R/W    | Frame capture engine
 * 0x0050 - 0x005F   | SDRAM_CTRL      | R/W    | SDRAM controller status
 * 0x0060 - 0x006F   | FRAME_TRANSFER  | R/W    | Frame data transfer
 * 0x0070 - 0x007F   | EDID            | R/W    | EDID management
 * 0x0080 - 0x00FF   | DEBUG           | R/W    | Debug and diagnostics
 * ========================================================================= */

/* =========================================================================
 * SYSTEM Registers (Base: 0x0000)
 * ========================================================================= */

/** @brief System Control Register (0x0000) */
#define REG_SYS_CTRL                   0x0000U
/* Bit fields for REG_SYS_CTRL */
#define SYS_CTRL_RESET                 (1U << 0)   /**< FPGA soft reset */
#define SYS_CTRL_PASSTHROUGH           (1U << 1)   /**< 1=passthrough, 0=capture mode */
#define SYS_CTRL_HDCP_BYPASS           (1U << 2)   /**< Enable HDCP stripping */
#define SYS_CTRL_STEALTH               (1U << 3)   /**< Disable all LEDs */
#define SYS_CTRL_FORCE_1080P           (1U << 4)   /**< Force 1080p EDID */
#define SYS_CTRL_ENABLE_OSD            (1U << 5)   /**< Enable OSD overlay */

/** @brief System Status Register (0x0001) */
#define REG_SYS_STATUS                 0x0001U
#define SYS_STATUS_FPGA_READY          (1U << 0)   /**< FPGA initialized */
#define SYS_STATUS_HDMI_IN_LOCKED      (1U << 1)   /**< HDMI RX has signal lock */
#define SYS_STATUS_HDMI_OUT_HOTPLUG    (1U << 2)   /**< Display connected */
#define SYS_STATUS_HDCP_ACTIVE         (1U << 3)   /**< HDCP handshake complete */
#define SYS_STATUS_CAPTURE_BUSY        (1U << 4)   /**< Frame capture in progress */
#define SYS_STATUS_BUF_READY           (1U << 5)   /**< Frame buffer ready for readout */
#define SYS_STATUS_OVERLAY_ACTIVE      (1U << 6)   /**< OSD overlay is visible */

/** @brief Video Timing Registers (0x0002-0x0007, read-only) */
#define REG_VIDEO_HTOTAL              0x0002U  /**< Horizontal total pixels   */
#define REG_VIDEO_VTOTAL              0x0003U  /**< Vertical total lines      */
#define REG_VIDEO_HACTIVE             0x0004U  /**< Horizontal active pixels  */
#define REG_VIDEO_VACTIVE             0x0005U  /**< Vertical active lines     */
#define REG_VIDEO_PCLK_KHZ            0x0006U  /**< Pixel clock in kHz       */
#define REG_VIDEO_REFRESH_HZ          0x0007U  /**< Refresh rate in Hz * 100 */

/** @brief Firmware Version Register (0x0008, read-only) */
#define REG_FW_VERSION                 0x0008U

/** @brief Error Count Register (0x0009) */
#define REG_ERROR_COUNT                0x0009U

/* =========================================================================
 * VIDEO_PATH Registers (Base: 0x0010)
 * ========================================================================= */

/** @brief Video Mode Select (0x0010) */
#define REG_VIDEO_MODE                 0x0010U
#define VIDEO_MODE_PASSTHROUGH         0x00U   /**< Direct passthrough */
#define VIDEO_MODE_INVERT_COLORS       0x01U   /**< Color inversion */
#define VIDEO_MODE_GRAYSCALE           0x02U   /**< Convert to grayscale */
#define VIDEO_MODE_MONOCHROME          0x03U   /**< Black and white only */
#define VIDEO_MODE_BLANK              0x04U   /**< Blank display (black) */
#define VIDEO_MODE_BLINK              0x05U   /**< Blink between normal/blank */
#define VIDEO_MODE_REGION_BLANK       0x06U   /**< Blank a rectangular region */

/** @brief Brightness Adjustment (0x0011, signed 8-bit, -128 to +127) */
#define REG_VIDEO_BRIGHTNESS           0x0011U

/** @brief Contrast Adjustment (0x0012, 0-255, 128=normal) */
#define REG_VIDEO_CONTRAST             0x0012U

/** @brief Region Blanking: X start (0x0013) */
#define REG_VIDEO_BLANK_XSTART         0x0013U

/** @brief Region Blanking: Y start (0x0014) */
#define REG_VIDEO_BLANK_YSTART         0x0014U

/** @brief Region Blanking: Width (0x0015) */
#define REG_VIDEO_BLANK_WIDTH          0x0015U

/** @brief Region Blanking: Height (0x0016) */
#define REG_VIDEO_BLANK_HEIGHT         0x0016U

/* =========================================================================
 * OSD Overlay Registers (Base: 0x0020)
 * ========================================================================= */

/** @brief OSD Control Register (0x0020) */
#define REG_OSD_CTRL                   0x0020U
#define OSD_CTRL_ENABLE                (1U << 0)   /**< Enable OSD plane */
#define OSD_CTRL_CLEAR                 (1U << 1)   /**< Clear OSD buffer */
#define OSD_CTRL_BLINK                 (1U << 2)   /**< Blink OSD text */
#define OSD_CTRL_WATERMARK             (1U << 3)   /**< Semi-transparent watermark */

/** @brief OSD X Position (0x0021, pixels from left) */
#define REG_OSD_XPOS                   0x0021U

/** @brief OSD Y Position (0x0022, pixels from top) */
#define REG_OSD_YPOS                   0x0022U

/** @brief OSD Color (0x0023, RGB565 format) */
#define REG_OSD_COLOR                  0x0023U

/** @brief OSD Alpha Blend (0x0024, 0=transparent, 255=opaque) */
#define REG_OSD_ALPHA                  0x0024U

/** @brief OSD Character Data (0x0025, write ASCII char to display) */
#define REG_OSD_CHAR_DATA              0x0025U

/** @brief OSD Character Cursor (0x0026, position in character grid) */
#define REG_OSD_CHAR_CURSOR            0x0026U

/** @brief OSD Font Select (0x0027, 0=8x8, 1=8x16) */
#define REG_OSD_FONT_SEL               0x0027U

/* =========================================================================
 * CEC Controller Registers (Base: 0x0030)
 * ========================================================================= */

/** @brief CEC Control Register (0x0030) */
#define REG_CEC_CTRL                   0x0030U
#define CEC_CTRL_ENABLE                (1U << 0)   /**< Enable CEC monitor */
#define CEC_CTRL_LOOPBACK              (1U << 1)   /**< Loopback CEC TX to RX */
#define CEC_CTRL_FILTER_BROADCAST      (1U << 2)   /**< Filter broadcast messages */

/** @brief CEC Status Register (0x0031, read-only) */
#define REG_CEC_STATUS                 0x0031U
#define CEC_STATUS_BUS_ACTIVE          (1U << 0)   /**< CEC bus activity detected */
#define CEC_STATUS_TX_BUSY             (1U << 1)   /**< Transmission in progress */
#define CEC_STATUS_RX_READY            (1U << 2)   /**< Received data available */
#define CEC_STATUS_ERROR               (1U << 3)   /**< CEC protocol error */

/** @brief CEC Transmit Data (0x0032) */
#define REG_CEC_TX_DATA                0x0032U

/** @brief CEC Receive Data (0x0033, read-only) */
#define REG_CEC_RX_DATA                0x0033U

/** @brief CEC Transmit Length (0x0034, bytes to send, 1-16) */
#define REG_CEC_TX_LEN                 0x0034U

/** @brief CEC Receive Length (0x0035, read-only, received bytes) */
#define REG_CEC_RX_LEN                 0x0035U

/** @brief CEC Initiator Address (0x0036) */
#define REG_CEC_INITIATOR              0x0036U

/** @brief CEC Destination Address (0x0037) */
#define REG_CEC_DESTINATION            0x0037U

/** @brief CEC Log Count (0x0038, read-only, number of logged messages) */
#define REG_CEC_LOG_COUNT              0x0038U

/* =========================================================================
 * Capture Engine Registers (Base: 0x0040)
 * ========================================================================= */

/** @brief Capture Control Register (0x0040) */
#define REG_CAPTURE_CTRL               0x0040U
#define CAPTURE_CTRL_TRIGGER           (1U << 0)   /**< Trigger single frame capture */
#define CAPTURE_CTRL_CONTINUOUS        (1U << 1)   /**< Continuous capture mode */
#define CAPTURE_CTRL_MOTION_DETECT     (1U << 2)   /**< Motion-triggered capture */
#define CAPTURE_CTRL_INTERVAL_EN       (1U << 3)   /**< Interval capture enable */
#define CAPTURE_CTRL_ABORT             (1U << 4)   /**< Abort current capture */

/** @brief Capture Status Register (0x0041, read-only) */
#define REG_CAPTURE_STATUS             0x0041U
#define CAPTURE_STATUS_IDLE            (0U << 0)   /**< No capture in progress */
#define CAPTURE_STATUS_CAPTURING       (1U << 0)   /**< Capture in progress */
#define CAPTURE_STATUS_COMPLETE        (2U << 0)   /**< Frame capture complete */
#define CAPTURE_STATUS_ERROR           (3U << 0)   /**< Capture error */
#define CAPTURE_STATUS_BUF_A_READY     (1U << 2)   /**< Buffer A has valid frame */
#define CAPTURE_STATUS_BUF_B_READY     (1U << 3)   /**< Buffer B has valid frame */

/** @brief Current Buffer Select (0x0042, 0=buffer A, 1=buffer B) */
#define REG_CAPTURE_BUF_SEL            0x0042U

/** @brief Capture Frame Counter (0x0043, read-only, increments per capture) */
#define REG_CAPTURE_FRAME_COUNT        0x0043U

/** @brief Capture Interval in frames (0x0044, capture every N frames) */
#define REG_CAPTURE_INTERVAL           0x0044U

/** @brief Motion Detection Threshold (0x0045, 0-255) */
#define REG_CAPTURE_MOTION_THRESH      0x0045U

/** @brief Motion Detection Sensitivity (0x0046, 0-255) */
#define REG_CAPTURE_MOTION_SENSITIVITY 0x0046U

/** @brief Captured pixel count (0x0047, read-only, bytes in captured frame) */
#define REG_CAPTURE_PIXEL_COUNT        0x0047U

/* =========================================================================
 * SDRAM Controller Registers (Base: 0x0050)
 * ========================================================================= */

/** @brief SDRAM Control Register (0x0050) */
#define REG_SDRAM_CTRL                 0x0050U
#define SDRAM_CTRL_INIT                (1U << 0)   /**< Initialize SDRAM */
#define SDRAM_CTRL_REFRESH_EN          (1U << 1)   /**< Enable auto-refresh */
#define SDRAM_CTRL_CLEAR               (1U << 2)   /**< Clear all SDRAM */

/** @brief SDRAM Status Register (0x0051, read-only) */
#define REG_SDRAM_STATUS               0x0051U
#define SDRAM_STATUS_INIT_DONE         (1U << 0)   /**< SDRAM initialized */
#define SDRAM_STATUS_ERROR             (1U << 1)   /**< SDRAM error detected */

/** @brief SDRAM Row Address for frame buffer A start (0x0052) */
#define REG_SDRAM_BUF_A_START          0x0052U

/** @brief SDRAM Row Address for frame buffer B start (0x0053) */
#define REG_SDRAM_BUF_B_START          0x0053U

/** @brief SDRAM refresh interval in clock cycles (0x0054) */
#define REG_SDRAM_REFRESH_INTERVAL     0x0054U

/* =========================================================================
 * Frame Transfer Registers (Base: 0x0060)
 * ========================================================================= */

/** @brief Frame Transfer Control (0x0060) */
#define REG_FRAME_XFER_CTRL            0x0060U
#define FRAME_XFER_CTRL_START          (1U << 0)   /**< Start frame transfer */
#define FRAME_XFER_CTRL_BURST          (1U << 1)   /**< Enable burst readout */

/** @brief Frame Transfer Status (0x0061, read-only) */
#define REG_FRAME_XFER_STATUS          0x0061U
#define FRAME_XFER_STATUS_IDLE         (0U << 0)
#define FRAME_XFER_STATUS_ACTIVE       (1U << 0)
#define FRAME_XFER_STATUS_DONE         (2U << 0)
#define FRAME_XFER_STATUS_ERROR        (3U << 0)

/** @brief Frame data read port (0x0062, 32-bit read returns pixel data) */
#define REG_FRAME_DATA_PORT            0x0062U

/** @brief Number of 32-bit words to transfer (0x0063) */
#define REG_FRAME_XFER_WORD_COUNT      0x0063U

/** @brief Current transfer address (0x0064) */
#define REG_FRAME_XFER_ADDR            0x0064U

/* =========================================================================
 * EDID Management Registers (Base: 0x0070)
 * ========================================================================= */

/** @brief EDID Control Register (0x0070) */
#define REG_EDID_CTRL                  0x0070U
#define EDID_CTRL_USE_LOCAL            (1U << 0)   /**< Use local EDID EEPROM */
#define EDID_CTRL_USE_PASSTHROUGH      (1U << 1)   /**< Passthrough display EDID */
#define EDID_CTRL_FORCE_INJECT         (1U << 2)   /**< Force injected EDID */
#define EDID_CTRL_DISABLE_HDCP         (1U << 3)   /**< Disable HDCP bit in EDID */

/** @brief EDID Status Register (0x0071, read-only) */
#define REG_EDID_STATUS                0x0071U
#define EDID_STATUS_VALID              (1U << 0)   /**< Current EDID is valid */
#define EDID_STATUS_FROM_DISPLAY       (1U << 1)   /**< EDID sourced from display */
#define EDID_STATUS_FROM_LOCAL         (1U << 2)   /**< EDID from local EEPROM */
#define EDID_STATUS_ERROR              (1U << 3)   /**< EDID read error */

/** @brief EDID Checksum (0x0072, read-only) */
#define REG_EDID_CHECKSUM              0x0072U

/** @brief EDID Source Address (0x0073, I²C address of current source) */
#define REG_EDID_SOURCE_ADDR           0x0073U

/* =========================================================================
 * Debug Registers (Base: 0x0080)
 * ========================================================================= */

/** @brief Debug Control Register (0x0080) */
#define REG_DBG_CTRL                   0x0080U
#define DBG_CTRL_TEST_PATTERN          (1U << 0)   /**< Output test pattern */
#define DBG_CTRL_COLOR_BARS            (1U << 1)   /**< Color bar test pattern */
#define DBG_CTRL_COUNTER               (1U << 2)   /**< Show frame counter OSD */
#define DBG_CTRL_LATENCY_TEST          (1U << 3)   /**< Enable latency test mode */

/** @brief Debug Counter (0x0081, read-only, increments every frame) */
#define REG_DBG_FRAME_COUNTER          0x0081U

/** @brief Debug Pixel Counter (0x0082, read-only) */
#define REG_DBG_PIXEL_COUNTER          0x0082U

/** @brief Debug Interrupt Status (0x0083, read-only) */
#define REG_DBG_IRQ_STATUS             0x0083U
#define DBG_IRQ_CAPTURE_DONE           (1U << 0)
#define DBG_IRQ_CEC_MESSAGE            (1U << 1)
#define DBG_IRQ_MOTION_DETECTED        (1U << 2)
#define DBG_IRQ_HDMI_LOCK_LOST         (1U << 3)
#define DBG_IRQ_BATTERY_LOW            (1U << 4)

/** @brief Debug Temperature (0x0084, read-only, degrees C * 2) */
#define REG_DBG_TEMPERATURE            0x0084U

/* =========================================================================
 * Register Access Helper Functions
 * ========================================================================= */

/**
 * @brief Write a 32-bit value to an FPGA register
 * @param reg_addr 16-bit register address
 * @param value 32-bit value to write
 */
void reg_write(uint16_t reg_addr, uint32_t value);

/**
 * @brief Read a 32-bit value from an FPGA register
 * @param reg_addr 16-bit register address
 * @return 32-bit register value
 */
uint32_t reg_read(uint16_t reg_addr);

/**
 * @brief Set specific bits in a register (read-modify-write)
 * @param reg_addr 16-bit register address
 * @param bitmask Bits to set (1 = set, 0 = unchanged)
 */
void reg_set_bits(uint16_t reg_addr, uint32_t bitmask);

/**
 * @brief Clear specific bits in a register (read-modify-write)
 * @param reg_addr 16-bit register address
 * @param bitmask Bits to clear (1 = clear, 0 = unchanged)
 */
void reg_clear_bits(uint16_t reg_addr, uint32_t bitmask);

/**
 * @brief Wait for a register bit to be set (polling, with timeout)
 * @param reg_addr Register address to poll
 * @param bitmask Bit or bits to check (all must be set)
 * @param timeout_ms Maximum wait time in milliseconds
 * @return true if bit(s) became set, false on timeout
 */
bool reg_wait_for_bit(uint16_t reg_addr, uint32_t bitmask, uint32_t timeout_ms);

/**
 * @brief Read multiple consecutive registers in burst mode
 * @param start_addr First register address
 * @param buffer Output buffer (must be at least count * 4 bytes)
 * @param count Number of 32-bit registers to read
 * @return Number of registers successfully read
 */
uint32_t reg_burst_read(uint16_t start_addr, uint32_t *buffer, uint32_t count);

/**
 * @brief Write multiple consecutive registers in burst mode
 * @param start_addr First register address
 * @param buffer Input buffer of 32-bit values
 * @param count Number of registers to write
 * @return Number of registers successfully written
 */
uint32_t reg_burst_write(uint16_t start_addr, const uint32_t *buffer, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */
