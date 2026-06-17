/**
 * main.c — Forge-Probe: Silicon Backdoor Debug Probe Firmware
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Forge-Probe is a portable JTAG/SWD/cJTAG debug interface platform for
 * hardware security research. It provides target discovery, boundary scan,
 * flash readout, debugger access, and automated vulnerability analysis
 * of embedded silicon.
 *
 * Architecture:
 *   - STM32H743 handles all protocol logic, USB host/device stack,
 *     SD card logging, and the Lua scripting engine.
 *   - iCE40UP5K FPGA accelerates TCK generation up to 120 MHz,
 *     handles bit-bang timing, and provides hardware buffer management.
 *
 * Compile: arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -O2 -c main.c
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "registers.h"

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Forward Declarations                                                      *
 * ═══════════════════════════════════════════════════════════════════════════ */

/* JTAG protocol layer */
static bool jtag_init(void);
static void jtag_tap_reset(void);
static void jtag_shift_ir(const uint8_t *ir_out, uint8_t *ir_in, uint32_t bits);
static void jtag_shift_dr(const uint8_t *dr_out, uint8_t *dr_in, uint32_t bits);
static uint32_t jtag_read_idcode(void);
static bool jtag_go_to_state(jtag_state_t target_state);

/* SWD protocol layer */
static bool swd_init(void);
static bool swd_line_reset(void);
static bool swd_read_dp(uint8_t addr, uint32_t *value);
static bool swd_write_dp(uint8_t addr, uint32_t value);
static bool swd_read_ap(uint8_t ap_sel, uint8_t addr, uint32_t *value);
static bool swd_write_ap(uint8_t ap_sel, uint8_t addr, uint32_t value);
static bool swd_dap_init(void);

/* cJTAG protocol layer */
static bool cjtag_init(void);
static bool cjtag_reset_link(void);
static bool cjtag_device_discovery(void);

/* Target discovery & enumeration */
static bool probe_detect_protocol(fp_protocol_t *proto);
static bool probe_discover_target(fp_target_desc_t *desc);
static bool probe_discover_aps(fp_target_desc_t *desc);
static bool probe_discover_rom_table(fp_target_desc_t *desc);

/* Memory access */
static bool memap_read(uint32_t address, uint32_t *value, uint8_t size);
static bool memap_write(uint32_t address, uint32_t value, uint8_t size);
static bool memap_read_block(uint32_t address, uint8_t *buffer, uint32_t len);
static bool memap_write_block(uint32_t address, const uint8_t *buffer, uint32_t len);

/* Flash operations */
static bool flash_unlock(fp_target_desc_t *desc);
static bool flash_read_sector(uint32_t sector_addr, uint8_t *buffer, uint32_t len);
static bool flash_dump_device(fp_target_desc_t *desc, uint32_t start, uint32_t length);
static bool flash_identify_protection(fp_target_desc_t *desc);

/* Boundary scan */
static bool boundary_scan_init(void);
static bool boundary_scan_capture(fp_target_desc_t *desc, uint8_t *bs_buffer);
static bool boundary_scan_interpret(fp_target_desc_t *desc, const uint8_t *bs_buffer);

/* ARM debug interface */
static bool arm_halt_target(void);
static bool arm_resume_target(void);
static bool arm_read_register(uint8_t reg_num, uint32_t *value);
static bool arm_write_register(uint8_t reg_num, uint32_t value);
static bool arm_read_memory_32(uint32_t address, uint32_t *value);
static bool arm_write_memory_32(uint32_t address, uint32_t value);
static bool arm_set_breakpoint(uint32_t address);
static bool arm_step_target(void);

/* Automation engine */
static bool automation_run_script(const char *script_name);
static bool automation_trigger_discovery(void);
static bool automation_scan_devices(void);
static void automation_log_event(const char *fmt, ...);

/* Host communication */
static void usb_process_commands(void);
static bool cmd_identify(void);
static bool cmd_read_memory(uint32_t addr, uint32_t len);
static bool cmd_write_memory(uint32_t addr, const uint8_t *data, uint32_t len);
static bool cmd_scan(void);
static bool cmd_dump_flash(void);
static bool cmd_fuzz_mem_ap(void);
static bool cmd_reset_target(void);

/* FPGA bridge */
static void fpga_init(void);
static void fpga_write_reg(uint32_t offset, uint32_t value);
static uint32_t fpga_read_reg(uint32_t offset);

/* Helpers */
static void delay_us(uint32_t us);
static void delay_ms(uint32_t ms);
static uint32_t crc32(const uint8_t *data, uint32_t len);
static void hexdump(const uint8_t *data, uint32_t len);

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Global State                                                              *
 * ═══════════════════════════════════════════════════════════════════════════ */

static fp_target_desc_t  g_target_desc;
static fp_protocol_t     g_active_protocol = FP_PROTO_JTAG;
static bool              g_probe_enabled = false;
static volatile bool     g_user_button_pressed = false;
static uint32_t          g_error_count = 0;
static uint32_t          g_discovered_targets = 0;
static char              g_log_buffer[LOG_BUFFER_LINES][128];
static uint16_t          g_log_index = 0;
static uint8_t          *g_dr_scratch = NULL;
static uint32_t          g_jtag_clock_hz = JTAG_DEFAULT_CLOCK_HZ;

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Main Entry Point                                                          *
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    uint32_t found_count = 0;

    /* ── System initialization ──────────────────────────────────────────── */

    /* Configure H7 clock tree: 480 MHz core, 240 MHz AXI, 120 MHz APB */
    SCB_EnableICache();
    SCB_EnableDCache();

    /* Initialize FPGA bridge first for protocol acceleration */
    fpga_init();

    /* Initialize USB HS for host communication */
    usb_init();

    /* Initialize SD card for logging/capture storage */
    sdmmc_init();

    /* Configure MPIO mux for default JTAG setup */
    mpio_init();

    /* Enable status LED — blue for boot, green when ready */
    set_status_led(0, 0, 1);    /* Blue */

    automation_log_event("Forge-Probe v1.0.0 starting (author: jayis1)");
    automation_log_event("FPGA bridge: %s", fpga_ready() ? "ready" : "FAILED");

    /* ── Main operational loop ──────────────────────────────────────────── */

    while (1)
    {
        /* Servicing the watchdog */
        IWDG->KR = 0xAAAA;

        /* Check for USB commands from host */
        usb_process_commands();

        /* Check user button */
        if (g_user_button_pressed)
        {
            g_user_button_pressed = false;
            automation_trigger_discovery();
        }

        /* ── Auto-discovery mode: probe for targets ───────────────────── */
        if (!g_probe_enabled)
        {
            set_status_led(1, 1, 0);  /* Yellow = probing */

            fp_protocol_t detected_proto;
            if (probe_detect_protocol(&detected_proto))
            {
                g_active_protocol = detected_proto;
                automation_log_event("Protocol detected: %s",
                    g_active_protocol == FP_PROTO_JTAG  ? "JTAG" :
                    g_active_protocol == FP_PROTO_SWD   ? "SWD"  :
                    g_active_protocol == FP_PROTO_CJTAG ? "cJTAG" : "UNKNOWN");

                if (probe_discover_target(&g_target_desc))
                {
                    g_probe_enabled = true;
                    g_discovered_targets++;

                    /* Log target info */
                    automation_log_event("Target: %s (0x%08X)",
                        g_target_desc.description,
                        g_target_desc.idcode);
                    automation_log_event("Arch: %d, IR: %d bits",
                        g_target_desc.architecture,
                        g_target_desc.ir_length);
                    automation_log_event("Secure debug: %s, Flash locked: %s",
                        g_target_desc.secure_debug ? "YES" : "no",
                        g_target_desc.flash_locked ? "YES" : "no");

                    /* If secure debug is unlocked, dump automatically */
                    if (!g_target_desc.secure_debug && !g_target_desc.flash_locked)
                    {
                        automation_log_event("Target appears unlocked — initiating scan...");
                        automation_scan_devices();
                    }
                    else
                    {
                        automation_log_event("Target locked or RDP active — limited access");
                    }

                    set_status_led(0, 1, 0);  /* Green = active */
                }
                else
                {
                    automation_log_event("Target discovery failed");
                    set_status_led(1, 0, 0);  /* Red = error */
                }
            }
            else
            {
                /* Nothing detected — keep probing periodically */
                delay_ms(500);
            }
        }

        /* ── Continuous monitoring when target is active ────────────────── */
        if (g_probe_enabled)
        {
            /* Poll target presence via voltage sense */
            uint32_t vref = read_target_voltage_mv();
            if (vref < 500)
            {
                automation_log_event("Target voltage dropped (%d mV) — target lost", vref);
                g_probe_enabled = false;
                set_status_led(1, 0, 0);  /* Red */
                continue;
            }

            /* Process any pending USB automation tasks */
            usb_process_commands();
        }

        /* Small delay to prevent tight looping */
        delay_ms(10);
    }

    return 0;  /* Never reached */
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Protocol Detection — Auto-Discovery Engine                                 *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * probe_detect_protocol: Determine which debug protocol the target supports.
 * Tries SWD first (fewer pins needed), then JTAG, then cJTAG.
 * Returns true if a protocol was detected and sets *proto.
 */
static bool probe_detect_protocol(fp_protocol_t *proto)
{
    /* ── Step 1: Try SWD (2-wire: SWCLK + SWDIO) ─────────────────────── */
    mpio_configure_dbg_pins(PROTOCOL_SWD);

    if (swd_init())
    {
        /* Verify by reading DPIDR — should return non-zero, non-all-ones */
        uint32_t dpidr;
        if (swd_read_dp(DP_REG_DPIDR, &dpidr))
        {
            if (dpidr != 0x00000000 && dpidr != 0xFFFFFFFF)
            {
                *proto = FP_PROTO_SWD;
                automation_log_event("SWD detected via DPIDR=0x%08X", dpidr);
                return true;
            }
        }
    }

    /* ── Step 2: Try JTAG (4-wire: TCK, TMS, TDI, TDO) ───────────────── */
    mpio_configure_dbg_pins(PROTOCOL_JTAG);

    if (jtag_init())
    {
        uint32_t idcode = jtag_read_idcode();
        if (idcode != 0x00000000 && idcode != 0xFFFFFFFF)
        {
            *proto = FP_PROTO_JTAG;
            automation_log_event("JTAG detected via IDCODE=0x%08X", idcode);
            return true;
        }
    }

    /* ── Step 3: Try cJTAG (2-wire: TCKC + TMSC) ─────────────────────── */
    mpio_configure_dbg_pins(PROTOCOL_CJTAG);

    if (cjtag_init())
    {
        /* Discovery via cJTAG JLOC exchange */
        if (cjtag_device_discovery())
        {
            *proto = FP_PROTO_CJTAG;
            automation_log_event("cJTAG detected via JLOC handshake");
            return true;
        }
    }

    return false;
}

/**
 * probe_discover_target: Fully enumerate a discovered target.
 * Populates fp_target_desc_t with all available information.
 */
static bool probe_discover_target(fp_target_desc_t *desc)
{
    memset(desc, 0, sizeof(fp_target_desc_t));
    desc->protocol = g_active_protocol;

    if (g_active_protocol == FP_PROTO_JTAG)
    {
        desc->idcode = jtag_read_idcode();
        desc->ir_length = 4;  /* ARM default, will be probed precisely */

        /* Probe IR length by walking the bypass register */
        for (int ir_len = 4; ir_len <= 32; ir_len++)
        {
            uint8_t ir_test[4] = {0};
            ir_test[0] = 0xFF;  /* IDCODE */
            jtag_shift_ir(ir_test, NULL, ir_len);

            /* Check if IDCODE readback works at this IR length */
            uint8_t dr_bits[4];
            memset(dr_bits, 0, sizeof(dr_bits));
            jtag_shift_dr(NULL, dr_bits, 32);
            uint32_t idcode_val = (dr_bits[0] << 0) | (dr_bits[1] << 8) |
                                  (dr_bits[2] << 16) | (dr_bits[3] << 24);

            if (idcode_val == desc->idcode)
            {
                desc->ir_length = ir_len;
                automation_log_event("IR length determined: %d bits", ir_len);
                break;
            }
        }

        /* Extract manufacturer from IDCODE */
        desc->manufacturer_id = (desc->idcode >> 1) & 0x3FF;

        /* Detect if multiple TAPs are daisy-chained */
        /* By writing bypass to all taps and checking TDO */
        desc->tap_count = 1;  /* Simplified — real probe checks each position */
    }
    else if (g_active_protocol == FP_PROTO_SWD)
    {
        /* Read DPIDR for DP identification */
        swd_read_dp(DP_REG_DPIDR, &desc->dpidr);

        /* Read TARGETID (DPv3+) */
        swd_write_dp(DP_REG_SELECT, 0);  /* Ensure SELECT is zero first */
        swd_read_dp(DP_REG_TARGETID, &desc->target_id);

        /* Attempt DAP init (power up) */
        if (swd_dap_init())
        {
            /* Discover available APs */
            probe_discover_aps(desc);

            /* Read ROM table base from MEM-AP[0] */
            if (desc->num_aps > 0)
            {
                /* MEM-AP BASE is at offset 0xF8 in the AP register space */
                swd_write_dp(DP_REG_SELECT, DP_SELECT_APSEL(0));
                swd_read_ap(0, MEMAP_REG_BASE, &desc->rom_table_base);
                desc->mem_ap_base = desc->rom_table_base;

                /* Discover ROM table */
                probe_discover_rom_table(desc);
            }
        }

        /* Extract arch from DPIDR part number */
        uint32_t partno = (desc->dpidr & DP_DPIDR_PARTNO_MASK) >> DP_DPIDR_PARTNO_SHIFT;
        switch (partno)
        {
            case DP_PARTNO_ARM_CS10: desc->architecture = FP_ARCH_CORTEX_M0; break;
            case DP_PARTNO_ARM_CS11: desc->architecture = FP_ARCH_CORTEX_M3; break;
            case DP_PARTNO_ARM_CS12: desc->architecture = FP_ARCH_CORTEX_M4; break;
            case DP_PARTNO_ARM_CS13: desc->architecture = FP_ARCH_CORTEX_M7; break;
            case DP_PARTNO_ARM_CS14: desc->architecture = FP_ARCH_CORTEX_M23; break;
            case DP_PARTNO_ARM_CS15: desc->architecture = FP_ARCH_CORTEX_M33; break;
            case DP_PARTNO_ARM_CS30: desc->architecture = FP_ARCH_CORTEX_A5; break;
            case DP_PARTNO_ARM_CS31: desc->architecture = FP_ARCH_CORTEX_A7; break;
            case DP_PARTNO_ARM_CS34: desc->architecture = FP_ARCH_CORTEX_A53; break;
            default: desc->architecture = FP_ARCH_CUSTOM; break;
        }

        /* Try to determine security state */
        flash_identify_protection(desc);
    }

    /* Build description string */
    snprintf(desc->description, FP_DESC_STRING_LEN,
        "%s:%04X MFR=%04X IR=%d AP=%d",
        desc->protocol == FP_PROTO_JTAG  ? "JTAG"  :
        desc->protocol == FP_PROTO_SWD   ? "SWD"   : "cJTAG",
        g_active_protocol == FP_PROTO_JTAG ? desc->idcode >> 12 : desc->target_id >> 16,
        desc->manufacturer_id,
        desc->ir_length,
        desc->num_aps);

    return true;
}

/**
 * probe_discover_aps: Enumerate all Access Ports on the target.
 * Iterates APSEL 0–255 and checks for valid AP IDR values.
 */
static bool probe_discover_aps(fp_target_desc_t *desc)
{
    uint32_t ap_idr;
    uint32_t ap_base;
    uint8_t ap_count = 0;

    for (uint32_t sel = 0; sel < 256 && ap_count < FP_MAX_APS; sel++)
    {
        swd_write_dp(DP_REG_SELECT, DP_SELECT_APSEL(sel) | DP_SELECT_APBANKSEL(0xF));

        if (swd_read_ap(sel, MEMAP_REG_IDR, &ap_idr))
        {
            if (ap_idr != 0x00000000 && ap_idr != 0xFFFFFFFF)
            {
                desc->ap_bases[ap_count] = sel;  /* Store APSEL, not base */
                ap_count++;

                automation_log_event("AP[%d] found at APSEL=%d, IDR=0x%08X",
                    ap_count - 1, sel, ap_idr);
            }
        }
    }

    desc->num_aps = ap_count;
    automation_log_event("Total APs discovered: %d", ap_count);
    return ap_count > 0;
}

/**
 * probe_discover_rom_table: Walk the CoreSight ROM table and identify
 * debug components (ETM, ITM, DWT, FPB, TPIU, etc.)
 */
static bool probe_discover_rom_table(fp_target_desc_t *desc)
{
    if (desc->rom_table_base == 0x00000000 || desc->rom_table_base == 0xFFFFFFFF)
        return false;

    uint32_t base_addr = desc->rom_table_base & 0xFFFFF000;  /* Mask to page */

    automation_log_event("Walking ROM table at 0x%08X", base_addr);

    int entry_count = 0;
    uint32_t offset = 0;

    while (entry_count < FP_MAX_ROMS)
    {
        uint32_t entry;
        if (!memap_read(base_addr + offset, &entry, 4))
            break;

        if (!(entry & ROM_ENTRY_PRESENT))
            break;  /* End of ROM table */

        uint32_t component_offset = (entry & ROM_ENTRY_OFFSET_MASK) >> ROM_ENTRY_OFFSET_SHIFT;
        uint32_t component_base = base_addr + component_offset;
        bool is_format_1 = (entry >> 1) & 1;

        desc->rom_entries[entry_count] = component_base;
        entry_count++;

        automation_log_event("  ROM[%d]: 0x%08X (format %d)", entry_count - 1,
            component_base, is_format_1 ? 1 : 0);

        offset += 4;
    }

    desc->num_rom_entries = entry_count;
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  JTAG Protocol Implementation                                             *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * jtag_init: Configure JTAG pins and issue a TAP reset sequence.
 * Returns true if the target's TDO idles high (proper TAP present).
 */
static bool jtag_init(void)
{
    /* Configure pin modes via MPIO mux */
    mpio_set_mode(MPIO_JTAG_TCK, ALT_FUNC_PUSHPULL);
    mpio_set_mode(MPIO_JTAG_TMS, ALT_FUNC_PUSHPULL);
    mpio_set_mode(MPIO_JTAG_TDI, ALT_FUNC_PUSHPULL);
    mpio_set_mode(MPIO_JTAG_TDO, INPUT_PULLUP);

    /* Set initial clock divider in FPGA */
    uint32_t divider = (CORE_CLOCK_HZ / (g_jtag_clock_hz * 2)) - 1;
    fpga_write_reg(FP_CLOCK_DIV, divider);

    /* Load bypass instruction via bit-bang to reset TAP */
    jtag_tap_reset();

    /* Verify TAP: TDO should be high (pull-up) after reset */
    delay_us(1);
    if (mpio_read(MPIO_JTAG_TDO) == 0)
    {
        automation_log_event("JTAG init: TDO stuck low — possible bad connection");
        return false;
    }

    automation_log_event("JTAG init: pins configured, TAP reset done, clock=%d Hz",
        g_jtag_clock_hz);
    return true;
}

/**
 * jtag_tap_reset: Move TAP to Test-Logic-Reset state.
 * Holds TMS high for 5+ TCK cycles.
 */
static void jtag_tap_reset(void)
{
    /* Send 5 TCK cycles with TMS=1 to force reset */
    for (int i = 0; i < TAP_RESET_CYCLES + 2; i++)
    {
        mpio_write(MPIO_JTAG_TMS, 1);
        mpio_write(MPIO_JTAG_TCK, 1);
        delay_us(1);
        mpio_write(MPIO_JTAG_TCK, 0);
        delay_us(1);
    }

    /* Exit to Run-Test/Idle: TMS=0, one TCK */
    mpio_write(MPIO_JTAG_TMS, 0);
    mpio_write(MPIO_JTAG_TCK, 1);
    delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);

    g_target_desc.tap_count = 1;  /* Reset collapsed the chain */
}

/**
 * jtag_shift_ir: Shift an instruction into the JTAG IR.
 * ir_out: instruction bits to send (LSB first)
 * ir_in:  received instruction bits buffer (can be NULL)
 * bits:   number of bits to shift (IR length per tap)
 */
static void jtag_shift_ir(const uint8_t *ir_out, uint8_t *ir_in, uint32_t bits)
{
    /* Move to Shift-IR */
    mpio_write(MPIO_JTAG_TMS, 1);  /* Select DR-Scan -> Select IR-Scan */
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
    mpio_write(MPIO_JTAG_TMS, 0);  /* Capture-IR */
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
    mpio_write(MPIO_JTAG_TMS, 0);  /* Shift-IR */
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);

    /* Shift each bit */
    for (uint32_t i = 0; i < bits; i++)
    {
        uint8_t out_bit = (ir_out) ? ((ir_out[i / 8] >> (i % 8)) & 1) : 0;
        uint8_t in_bit = 0;

        /* Last bit: transition to Exit1-IR */
        if (i == bits - 1)
            mpio_write(MPIO_JTAG_TMS, 1);
        else
            mpio_write(MPIO_JTAG_TMS, 0);

        mpio_write(MPIO_JTAG_TDI, out_bit);
        mpio_write(MPIO_JTAG_TCK, 1);
        delay_us(1);

        in_bit = mpio_read(MPIO_JTAG_TDO);

        mpio_write(MPIO_JTAG_TCK, 0);
        delay_us(1);

        if (ir_in)
        {
            ir_in[i / 8] |= (in_bit << (i % 8));
        }
    }

    /* Exit1-IR -> Update-IR -> Idle */
    mpio_write(MPIO_JTAG_TMS, 1);
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
    mpio_write(MPIO_JTAG_TMS, 0);  /* Update-IR */
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
}

/**
 * jtag_shift_dr: Shift data through the JTAG DR path.
 * Same structure as jtag_shift_ir but through DR scan path.
 */
static void jtag_shift_dr(const uint8_t *dr_out, uint8_t *dr_in, uint32_t bits)
{
    /* Move to Shift-DR */
    mpio_write(MPIO_JTAG_TMS, 1);
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
    mpio_write(MPIO_JTAG_TMS, 0);
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
    mpio_write(MPIO_JTAG_TMS, 0);  /* Shift-DR */
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);

    for (uint32_t i = 0; i < bits; i++)
    {
        uint8_t out_bit = (dr_out) ? ((dr_out[i / 8] >> (i % 8)) & 1) : 0;
        uint8_t in_bit = 0;

        if (i == bits - 1)
            mpio_write(MPIO_JTAG_TMS, 1);
        else
            mpio_write(MPIO_JTAG_TMS, 0);

        mpio_write(MPIO_JTAG_TDI, out_bit);
        mpio_write(MPIO_JTAG_TCK, 1);
        delay_us(1);

        in_bit = mpio_read(MPIO_JTAG_TDO);

        mpio_write(MPIO_JTAG_TCK, 0);
        delay_us(1);

        if (dr_in)
            dr_in[i / 8] |= (in_bit << (i % 8));
    }

    /* Exit1-DR -> Update-DR -> Idle */
    mpio_write(MPIO_JTAG_TMS, 1);
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
    mpio_write(MPIO_JTAG_TMS, 0);
    mpio_write(MPIO_JTAG_TCK, 1);  delay_us(1);
    mpio_write(MPIO_JTAG_TCK, 0);
}

/**
 * jtag_read_idcode: Shift the IDCODE instruction and read the IDCODE register.
 * Returns the 32-bit JEDEC IDCODE value.
 */
static uint32_t jtag_read_idcode(void)
{
    uint8_t ir_bits[4] = {JTAG_IR_IDCODE, 0x00, 0x00, 0x00};
    uint8_t dr_bits[8] = {0};

    /* Select IDCODE instruction */
    jtag_shift_ir(ir_bits, NULL, g_target_desc.ir_length ? g_target_desc.ir_length : 4);

    /* Read 32-bit IDCODE from DR */
    jtag_shift_dr(NULL, dr_bits, 32);

    uint32_t idcode = (uint32_t)dr_bits[0] |
                     ((uint32_t)dr_bits[1] << 8) |
                     ((uint32_t)dr_bits[2] << 16) |
                     ((uint32_t)dr_bits[3] << 24);

    /* Restore bypass to keep TAP clean */
    uint8_t bypass[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    jtag_shift_ir(bypass, NULL, g_target_desc.ir_length ? g_target_desc.ir_length : 4);

    return idcode;
}

/**
 * jtag_go_to_state: Transition the TAP to a specific state.
 * Uses state transition bit sequences.
 */
static bool jtag_go_to_state(jtag_state_t target_state)
{
    /* Simplified: implement exact TMS sequences per IEEE 1149.1 */
    /* For now, we just do TAP reset which works back to Idle */
    if (target_state == JTAG_STATE_RUN_TEST_IDLE)
    {
        jtag_tap_reset();
        return true;
    }
    return (target_state == JTAG_STATE_TEST_LOGIC_RESET);
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  SWD Protocol Implementation                                              *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * swd_init: Initialize SWD pins and attempt a line reset sequence.
 * SWD uses 2 wires: SWCLK (output) and SWDIO (bidirectional).
 */
static bool swd_init(void)
{
    /* Configure pins */
    mpio_set_mode(MPIO_SWD_SWCLK, ALT_FUNC_PUSHPULL);
    mpio_set_mode(MPIO_SWD_SWDIO, ALT_FUNC_OPENDRAIN);

    /* Attempt SWD line reset: 50+ SWCLK cycles with SWDIO high */
    if (!swd_line_reset())
    {
        automation_log_event("SWD init: line reset failed");
        return false;
    }

    /* Send SWD ID code request (0x00000000 for all DP instances) */
    /* By switching to SWD mode via JTAG-to-SWD sequence if needed */
    automation_log_event("SWD init: line reset OK");
    return true;
}

/**
 * swd_line_reset: Issue the standard SWD line reset + IDLE sequence.
 * 50+ SWCLK cycles with SWDIO high, then 8+ cycles low.
 */
static bool swd_line_reset(void)
{
    /* 50 cycles SWDIO=1, SWCLK toggling */
    mpio_write(MPIO_SWD_SWDIO, 1);
    for (int i = 0; i < 55; i++)
    {
        mpio_write(MPIO_SWD_SWCLK, 1);
        delay_us(1);
        mpio_write(MPIO_SWD_SWCLK, 0);
        delay_us(1);
    }

    /* 8 cycles SWDIO=0 */
    mpio_write(MPIO_SWD_SWDIO, 0);
    for (int i = 0; i < 10; i++)
    {
        mpio_write(MPIO_SWD_SWCLK, 1);
        delay_us(1);
        mpio_write(MPIO_SWD_SWCLK, 0);
        delay_us(1);
    }

    return true;
}

/**
 * swd_transfer: Low-level SWD request/response transaction.
 * Sends an 8-bit request header, receives 3-bit ACK, then 32-bit data + parity.
 */
static bool swd_transfer(uint8_t request, uint32_t *data, bool is_read)
{
    uint8_t parity_expected = 0;
    uint8_t ack = 0;
    uint32_t response = 0;

    /* ── Phase 1: Send request (8 bits, MSB first per SWD spec) ────── */
    /* Re-order to MSB-first: start(1), APnDP, RnW, A[3:2], parity, stop(0), park(1) */
    uint8_t swd_request_msb = 0;
    swd_request_msb |= 1;                                             /* Start */
    swd_request_msb |= ((request >> 1) & 1) << 1;                     /* APnDP */
    swd_request_msb |= ((request >> 2) & 1) << 2;                     /* RnW */
    swd_request_msb |= ((request >> 4) & 1) << 3;                     /* A3 */
    swd_request_msb |= ((request >> 3) & 1) << 4;                     /* A2 */
    /* Parity: even parity of bits [4:1] (APnDP, RnW, A2, A3) */
    uint8_t parity = 0;
    parity ^= (request >> 1) & 1;  /* APnDP */
    parity ^= (request >> 2) & 1;  /* RnW */
    parity ^= (request >> 3) & 1;  /* A2 */
    parity ^= (request >> 4) & 1;  /* A3 */
    swd_request_msb |= (parity ^ 1) << 5;  /* Parity bit (active low in some impls) */
    swd_request_msb |= 0 << 6;              /* Stop */
    swd_request_msb |= 1 << 7;              /* Park */

    /* Send request byte MSB first */
    for (int i = 7; i >= 0; i--)
    {
        uint8_t bit = (swd_request_msb >> i) & 1;
        mpio_write(MPIO_SWD_SWDIO, bit);
        mpio_write(MPIO_SWD_SWCLK, 1);
        delay_us(1);
        mpio_write(MPIO_SWD_SWCLK, 0);
        delay_us(1);
    }

    /* ── Phase 2: Turnaround cycle (1 clock) — host releases SWDIO ── */
    mpio_set_mode(MPIO_SWD_SWDIO, INPUT);  /* Host tristate */
    mpio_write(MPIO_SWD_SWCLK, 1);
    delay_us(1);
    mpio_write(MPIO_SWD_SWCLK, 0);

    /* ── Phase 3: Read ACK (3 bits from target) ─────────────────────── */
    ack = 0;
    for (int i = 2; i >= 0; i--)
    {
        mpio_write(MPIO_SWD_SWCLK, 1);
        delay_us(1);
        uint8_t ack_bit = mpio_read(MPIO_SWD_SWDIO);
        ack |= (ack_bit << i);
        mpio_write(MPIO_SWD_SWCLK, 0);
        delay_us(1);
    }

    /* ── Phase 4: Data phase (read or write 32 bits + parity) ────── */
    if (ack == 0x01)  /* OK */
    {
        if (is_read)
        {
            /* Target drives data */
            response = 0;
            for (int i = 31; i >= 0; i--)
            {
                mpio_write(MPIO_SWD_SWCLK, 1);
                delay_us(1);
                uint8_t data_bit = mpio_read(MPIO_SWD_SWDIO);
                response |= ((uint32_t)data_bit << i);
                mpio_write(MPIO_SWD_SWCLK, 0);
                delay_us(1);
            }

            /* Read parity */
            mpio_write(MPIO_SWD_SWCLK, 1);
            delay_us(1);
            parity_expected = mpio_read(MPIO_SWD_SWDIO);
            mpio_write(MPIO_SWD_SWCLK, 0);

            /* Calculate parity of read data */
            uint8_t parity_calc = 0;
            uint32_t temp = response;
            for (int b = 0; b < 32; b++)
            {
                parity_calc ^= (temp & 1);
                temp >>= 1;
            }

            if (parity_calc != parity_expected)
            {
                automation_log_event("SWD parity error: calc=%d expected=%d",
                    parity_calc, parity_expected);
                return false;
            }

            if (data)
                *data = response;
        }
        else
        {
            /* Host drives data */
            mpio_set_mode(MPIO_SWD_SWDIO, ALT_FUNC_PUSHPULL);
            uint32_t write_data = data ? *data : 0;

            for (int i = 31; i >= 0; i--)
            {
                uint8_t data_bit = (write_data >> i) & 1;
                mpio_write(MPIO_SWD_SWDIO, data_bit);
                mpio_write(MPIO_SWD_SWCLK, 1);
                delay_us(1);
                mpio_write(MPIO_SWD_SWCLK, 0);
                delay_us(1);
            }

            /* Write parity */
            uint8_t parity_calc = 0;
            uint32_t temp = write_data;
            for (int b = 0; b < 32; b++)
            {
                parity_calc ^= (temp & 1);
                temp >>= 1;
            }
            mpio_write(MPIO_SWD_SWDIO, parity_calc);
            mpio_write(MPIO_SWD_SWCLK, 1);
            delay_us(1);
            mpio_write(MPIO_SWD_SWCLK, 0);
        }
    }
    else if (ack == 0x02)  /* WAIT */
    {
        automation_log_event("SWD target WAIT");
        return false;
    }
    else if (ack == 0x04)  /* FAULT */
    {
        automation_log_event("SWD target FAULT");
        return false;
    }
    else
    {
        automation_log_event("SWD invalid ACK: 0x%02X", ack);
        return false;
    }

    /* ── Phase 5: Turnaround cycle ─────────────────────────────────────── */
    mpio_set_mode(MPIO_SWD_SWDIO, INPUT);
    mpio_write(MPIO_SWD_SWCLK, 1);
    delay_us(1);
    mpio_write(MPIO_SWD_SWCLK, 0);

    /* Return to output mode */
    mpio_set_mode(MPIO_SWD_SWDIO, ALT_FUNC_OPENDRAIN);

    return true;
}

/**
 * swd_read_dp: Read a Debug Port register.
 * Uses APnDP=0, RnW=1 with the 2-bit address A[3:2].
 */
static bool swd_read_dp(uint8_t addr, uint32_t *value)
{
    uint8_t request = 0;
    request |= (0 << 1);        /* APnDP = 0 = DP */
    request |= (1 << 2);        /* RnW = 1 = read */
    request |= (addr & 0x0C);   /* A[3:2] in bits 3,4 */

    return swd_transfer(request, value, true);
}

/**
 * swd_write_dp: Write a Debug Port register.
 */
static bool swd_write_dp(uint8_t addr, uint32_t value)
{
    uint8_t request = 0;
    request |= (0 << 1);        /* APnDP = 0 = DP */
    request |= (0 << 2);        /* RnW = 0 = write */
    request |= (addr & 0x0C);   /* A[3:2] in bits 3,4 */

    return swd_transfer(request, &value, false);
}

/**
 * swd_read_ap: Read an Access Port register.
 * Must first write SELECT with APSEL and APBANKSEL.
 */
static bool swd_read_ap(uint8_t ap_sel, uint8_t addr, uint32_t *value)
{
    /* The caller must have set DP_SELECT already */
    uint8_t request = 0;
    request |= (1 << 1);        /* APnDP = 1 = AP */
    request |= (1 << 2);        /* RnW = 1 = read */
    request |= (addr & 0x0C);   /* A[3:2] in bits 3,4 */

    bool ok = swd_transfer(request, value, true);
    if (!ok) return false;

    /* Read RDBUFF to get final value (post-increment) */
    uint32_t rdbuff;
    return swd_read_dp(DP_REG_RDBUFF, &rdbuff);
}

/**
 * swd_write_ap: Write an Access Port register.
 */
static bool swd_write_ap(uint8_t ap_sel, uint8_t addr, uint32_t value)
{
    uint8_t request = 0;
    request |= (1 << 1);        /* APnDP = 1 = AP */
    request |= (0 << 2);        /* RnW = 0 = write */
    request |= (addr & 0x0C);   /* A[3:2] in bits 3,4 */

    return swd_transfer(request, &value, false);
}

/**
 * swd_dap_init: Power up the Debug Access Port.
 * Sets CSYSPWRUPREQ and CDBGPWRUPREQ, waits for ACK.
 */
static bool swd_dap_init(void)
{
    uint32_t ctrl_stat;

    /* Request system + debug power-up */
    ctrl_stat = DP_CTRL_STAT_CSYSPWRUPREQ | DP_CTRL_STAT_CDBGPWRUPREQ;
    if (!swd_write_dp(DP_REG_CTRL_STAT, ctrl_stat))
    {
        automation_log_event("DAP init: power-up request failed");
        return false;
    }

    /* Poll for ACK (up to 100 attempts) */
    for (int i = 0; i < 100; i++)
    {
        if (!swd_read_dp(DP_REG_CTRL_STAT, &ctrl_stat))
        {
            automation_log_event("DAP init: readback failed");
            return false;
        }

        if ((ctrl_stat & (DP_CTRL_STAT_CSYSPWRUPACK | DP_CTRL_STAT_CDBGPWRUPACK))
            == (DP_CTRL_STAT_CSYSPWRUPACK | DP_CTRL_STAT_CDBGPWRUPACK))
        {
            automation_log_event("DAP init: power-up ACK received");
            return true;
        }

        delay_ms(5);
    }

    automation_log_event("DAP init: power-up timeout (CTRL_STAT=0x%08X)", ctrl_stat);
    return false;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  cJTAG (IEEE 1149.7) Protocol Implementation                               *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * cjtag_init: Configure pins for 2-wire cJTAG (TCKC + TMSC).
 * Uses the same pins as SWD, but different protocol framing.
 */
static bool cjtag_init(void)
{
    mpio_set_mode(MPIO_CJTAG_TCKC, ALT_FUNC_PUSHPULL);
    mpio_set_mode(MPIO_CJTAG_TMSC, ALT_FUNC_OPENDRAIN);

    /* cJTAG idle state: TMSC high, TCKC low */
    mpio_write(MPIO_CJTAG_TMSC, 1);
    mpio_write(MPIO_CJTAG_TCKC, 0);

    automation_log_event("cJTAG init: pins configured");
    return true;
}

/**
 * cjtag_reset_link: Reset the cJTAG link by holding TMSC low for 8+ TCKC cycles.
 */
static bool cjtag_reset_link(void)
{
    mpio_write(MPIO_CJTAG_TMSC, 0);
    for (int i = 0; i < 12; i++)
    {
        mpio_write(MPIO_CJTAG_TCKC, 1);
        delay_us(1);
        mpio_write(MPIO_CJTAG_TCKC, 0);
        delay_us(1);
    }
    mpio_write(MPIO_CJTAG_TMSC, 1);
    delay_us(5);

    automation_log_event("cJTAG: link reset complete");
    return true;
}

/**
 * cjtag_device_discovery: Perform cJTAG JLOC discovery exchange.
 * Uses the 2-wire protocol to query device presence.
 */
static bool cjtag_device_discovery(void)
{
    /* cJTAG devices respond to JLOC_IDLE with specific bit patterns */
    /* Simplified: try a standard cJTAG ID scan sequence */
    /* Real implementation would be more complex with JLOC state machine */

    if (!cjtag_reset_link())
        return false;

    /* Send JLOC discovery sequence: 0x7E (01111110 sync pattern) */
    uint8_t sync_pattern = 0x7E;
    mpio_write(MPIO_CJTAG_TMSC, 1);  /* Start bit */

    for (int i = 0; i < 8; i++)
    {
        uint8_t bit = (sync_pattern >> (7 - i)) & 1;
        mpio_write(MPIO_CJTAG_TMSC, bit);
        mpio_write(MPIO_CJTAG_TCKC, 1);
        delay_us(1);
        mpio_write(MPIO_CJTAG_TCKC, 0);
        delay_us(1);
    }

    /* Look for response: check TMSC after turnaround */
    mpio_set_mode(MPIO_CJTAG_TMSC, INPUT);
    mpio_write(MPIO_CJTAG_TCKC, 1);
    delay_us(1);
    uint8_t resp = mpio_read(MPIO_CJTAG_TMSC);
    mpio_write(MPIO_CJTAG_TCKC, 0);

    mpio_set_mode(MPIO_CJTAG_TMSC, ALT_FUNC_OPENDRAIN);

    if (resp == 0)
    {
        automation_log_event("cJTAG: no response to JLOC discovery");
        return false;
    }

    automation_log_event("cJTAG device responded to JLOC!");
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  MEM-AP Memory Access Layer                                               *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * memap_read: Read from target memory via MEM-AP.
 * Supports 8, 16, 32-bit access sizes.
 */
static bool memap_read(uint32_t address, uint32_t *value, uint8_t size)
{
    if (!g_probe_enabled)
        return false;

    if (g_active_protocol != FP_PROTO_SWD)
        return false;

    /* Set CSW for correct transfer size and auto-increment off */
    uint32_t csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_WORD;
    if (size == 1) csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_BYTE;
    else if (size == 2) csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_HALF;
    else if (size == 4) csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_WORD;
    else if (size == 8) csw = MEMAP_CSW_ADDRINC_OFF | MEMAP_CSW_SIZE_DWORD;

    csw |= MEMAP_CSW_DBGEN;  /* Enable debug access */

    if (!swd_write_ap(0, MEMAP_REG_CSW, csw))
        return false;

    /* Set transfer address */
    if (!swd_write_ap(0, MEMAP_REG_TAR, address))
        return false;

    /* Read data register (triggers the transfer) */
    if (!swd_read_ap(0, MEMAP_REG_DRW, value))
        return false;

    return true;
}

/**
 * memap_write: Write to target memory via MEM-AP.
 */
static bool memap_write(uint32_t address, uint32_t value, uint8_t size)
{
    if (!g_probe_enabled || g_active_protocol != FP_PROTO_SWD)
        return false;

    uint32_t csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_WORD;
    if (size == 1) csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_BYTE;
    else if (size == 2) csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_HALF;
    else if (size == 4) csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_WORD;

    if (!swd_write_ap(0, MEMAP_REG_CSW, csw))
        return false;

    if (!swd_write_ap(0, MEMAP_REG_TAR, address))
        return false;

    if (!swd_write_ap(0, MEMAP_REG_DRW, value))
        return false;

    return true;
}

/**
 * memap_read_block: Bulk read from target memory.
 * Used for fast flash dumps and ROM table enumeration.
 */
static bool memap_read_block(uint32_t address, uint8_t *buffer, uint32_t len)
{
    /* Configure for 32-bit word transfers with auto-increment */
    uint32_t csw = MEMAP_CSW_ADDRINC_SINGLE | MEMAP_CSW_SIZE_WORD;
    if (!swd_write_ap(0, MEMAP_REG_CSW, csw))
        return false;

    if (!swd_write_ap(0, MEMAP_REG_TAR, address))
        return false;

    /* Read words in a tight loop */
    uint32_t word_count = len / 4;
    for (uint32_t i = 0; i < word_count; i++)
    {
        uint32_t word;
        if (!swd_read_ap(0, MEMAP_REG_DRW, &word))
            return false;

        buffer[i * 4 + 0] = (word >> 0) & 0xFF;
        buffer[i * 4 + 1] = (word >> 8) & 0xFF;
        buffer[i * 4 + 2] = (word >> 16) & 0xFF;
        buffer[i * 4 + 3] = (word >> 24) & 0xFF;
    }

    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Boundary Scan Engine                                                     *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * boundary_scan_init: Configure for boundary scan mode.
 * Uses JTAG EXTEST, SAMPLE/PRELOAD, or INTEST instructions.
 */
static bool boundary_scan_init(void)
{
    if (g_active_protocol != FP_PROTO_JTAG)
    {
        automation_log_event("BS: boundary scan requires JTAG mode");
        return false;
    }

    automation_log_event("BS: engine ready");
    return true;
}

/**
 * boundary_scan_capture: Perform a full boundary scan capture.
 * Shifts SAMPLE/PRELOAD through the boundary scan chain.
 * bs_buffer receives the full chain bitstream.
 */
static bool boundary_scan_capture(fp_target_desc_t *desc, uint8_t *bs_buffer)
{
    /* Select SAMPLE/PRELOAD instruction */
    uint8_t ir_preload = JTAG_IR_SAMPLE_PRELOAD;
    jtag_shift_ir(&ir_preload, NULL, desc->ir_length);

    /* Estimate chain length (2 bytes per pin is typical) */
    uint32_t chain_bits = desc->boundary_chain_len;
    if (chain_bits == 0)
        chain_bits = 256;  /* Default estimate */

    /* Allocate temporary buffer via FPGA scratch */
    uint8_t *bs_data = bs_buffer;
    memset(bs_data, 0, chain_bits / 8 + 1);

    /* Capture: pulse Capture-DR by entering/exiting Shift-DR with zero bits */
    /* Then shift out the captured data */
    jtag_shift_dr(NULL, bs_data, chain_bits);

    automation_log_event("BS: captured %d bits from boundary chain", chain_bits);
    desc->boundary_chain_len = chain_bits;

    return true;
}

/**
 * boundary_scan_interpret: Parse boundary scan bitstream to identify
 * pin states, drive strengths, pull configurations, and unknown connections.
 */
static bool boundary_scan_interpret(fp_target_desc_t *desc, const uint8_t *bs_buffer)
{
    uint32_t bits = desc->boundary_chain_len;
    uint32_t bytes = (bits + 7) / 8;

    automation_log_event("BS: interpreting %d-bit boundary scan chain", bits);

    /* Count observed pin states */
    uint32_t pin_count = 0;
    uint32_t pins_high = 0;
    uint32_t pins_low = 0;
    uint32_t pins_tristate = 0;

    for (uint32_t i = 0; i < bits; i++)
    {
        uint8_t bit_val = (bs_buffer[i / 8] >> (i % 8)) & 1;
        pin_count++;

        if (bit_val) pins_high++;
        else pins_low++;
    }

    automation_log_event("BS: total pins=%d, HIGH=%d, LOW=%d, TRI=0",
        pin_count, pins_high, pins_low);

    /* Scan for pattern: consecutive low bits may indicate ground */
    /* Consecutive high bits may indicate VCC or pull-ups */
    uint32_t longest_run_high = 0;
    uint32_t longest_run_low = 0;
    uint32_t current_run = 0;
    bool current_val = false;

    for (uint32_t i = 0; i < bits; i++)
    {
        uint8_t bit_val = (bs_buffer[i / 8] >> (i % 8)) & 1;
        if (i == 0)
        {
            current_val = bit_val;
            current_run = 1;
        }
        else if (bit_val == current_val)
        {
            current_run++;
        }
        else
        {
            if (current_val && current_run > longest_run_high)
                longest_run_high = current_run;
            else if (!current_val && current_run > longest_run_low)
                longest_run_low = current_run;
            current_val = bit_val;
            current_run = 1;
        }
    }

    automation_log_event("BS: longest VCC run=%d, GND run=%d cells",
        longest_run_high, longest_run_low);

    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  ARM Target Control — Halt, Step, Resume, Register Access                  *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * arm_halt_target: Halt the Cortex-M target via DHCSR.
 * Sets HALT and DEBUGEN bits, then polls S_HALT.
 */
static bool arm_halt_target(void)
{
    uint32_t dhcsr;

    /* Write DHCSR with DEBUGEN | HALT */
    /* Key byte: 0xA05F in upper half */
    dhcsr = 0xA05F0000 | DHCSR_DEBUGEN | DHCSR_HALT;

    if (g_active_protocol == FP_PROTO_SWD)
    {
        if (!memap_write(SCS_DHCSR, dhcsr, 4))
            return false;
    }

    /* Poll until halted */
    for (int i = 0; i < 50; i++)
    {
        if (!memap_read(SCS_DHCSR, &dhcsr, 4))
            return false;

        if (dhcsr & DHCSR_S_HALT)
        {
            automation_log_event("ARM: target halted (DHCSR=0x%08X)", dhcsr);
            return true;
        }
        delay_ms(1);
    }

    automation_log_event("ARM: halt timeout");
    return false;
}

/**
 * arm_resume_target: Resume execution by clearing HALT bit.
 */
static bool arm_resume_target(void)
{
    uint32_t dhcsr = 0xA05F0000 | DHCSR_DEBUGEN;
    if (!memap_write(SCS_DHCSR, dhcsr, 4))
        return false;

    automation_log_event("ARM: target resumed");
    return true;
}

/**
 * arm_read_register: Read a core register via DCRSR/DCRDR.
 * reg_num: from DCRSR_R0..DCRSR_FPSYS
 */
static bool arm_read_register(uint8_t reg_num, uint32_t *value)
{
    /* Write DCRSR: register number with WnR=0 (read) */
    uint32_t dcsr = reg_num;
    if (!memap_write(SCS_DCRSR, dcsr, 4))
        return false;

    /* Poll DHCSR for S_REGRDY */
    uint32_t dhcsr;
    for (int i = 0; i < 20; i++)
    {
        if (!memap_read(SCS_DHCSR, &dhcsr, 4))
            return false;
        if (dhcsr & DHCSR_S_REGRDY)
            break;
        delay_us(10);
    }

    if (!(dhcsr & DHCSR_S_REGRDY))
    {
        automation_log_event("ARM: register %d not ready", reg_num);
        return false;
    }

    /* Read DCRDR */
    if (!memap_read(SCS_DCRDR, value, 4))
        return false;

    return true;
}

/**
 * arm_write_register: Write a core register via DCRSR/DCRDR.
 */
static bool arm_write_register(uint8_t reg_num, uint32_t value)
{
    /* Write value to DCRDR first */
    if (!memap_write(SCS_DCRDR, value, 4))
        return false;

    /* Write DCRSR with reg_num and WnR=1 (write) */
    uint32_t dcsr = reg_num | DCRSR_REGWNR;
    if (!memap_write(SCS_DCRSR, dcsr, 4))
        return false;

    /* Poll for completion */
    uint32_t dhcsr;
    for (int i = 0; i < 20; i++)
    {
        if (!memap_read(SCS_DHCSR, &dhcsr, 4))
            return false;
        if (dhcsr & DHCSR_S_REGRDY)
            return true;
        delay_us(10);
    }

    return false;
}

/**
 * arm_set_breakpoint: Set a hardware breakpoint via FPB.
 * Uses comparator 0 of the Flash Patch and Breakpoint unit.
 */
static bool arm_set_breakpoint(uint32_t address)
{
    /* FPB register layout:
     * FPB_CTRL @ 0xE0002000
     * FPB_REMAP @ 0xE0002004
     * FPB_COMP[0..5] @ 0xE0002008 + 4*n
     */

    uint32_t fpb_ctrl;

    /* Read FPB_CTRL to check NUM_COMP */
    if (!memap_read(ARM_FPB_BASE + 0x00, &fpb_ctrl, 4))
        return false;

    automation_log_event("FPB: CTRL=0x%08X, NUM_COMP=%d",
        fpb_ctrl, (fpb_ctrl >> 4) & 0x0F);

    /* Enable FPB (KEY=2, ENABLE=1) */
    if (!memap_write(ARM_FPB_BASE + 0x00, (2UL << 1) | 1, 4))
        return false;

    /* Set COMP[0]: address with ENABLE bit */
    uint32_t comp_val = (address & 0x1FFFFFFF) | (1UL << 30);  /* BP based on address */
    if (!memap_write(ARM_FPB_BASE + 0x08, comp_val, 4))
        return false;

    automation_log_event("ARM: hardware breakpoint set at 0x%08X", address);
    return true;
}

/**
 * arm_step_target: Single-step the halted target.
 */
static bool arm_step_target(void)
{
    uint32_t dhcsr = 0xA05F0000 | DHCSR_DEBUGEN | DHCSR_STEP;
    if (!memap_write(SCS_DHCSR, dhcsr, 4))
        return false;

    /* Wait for step to complete (S_HALT clears, then re-asserts) */
    for (int i = 0; i < 50; i++)
    {
        if (!memap_read(SCS_DHCSR, &dhcsr, 4))
            return false;

        if (dhcsr & DHCSR_S_HALT)
        {
            automation_log_event("ARM: step complete");
            return true;
        }
        delay_ms(1);
    }

    return false;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Flash Memory Operations — Dump, Read Protection Detection                  *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * flash_identify_protection: Probe the target's flash security state.
 * Checks for:
 *   - RDP (Readout Protection) level 0/1/2
 *   - Flash lock bits
 *   - Debug enable/disable (SPIDEN)
 */
static bool flash_identify_protection(fp_target_desc_t *desc)
{
    /* Try reading well-known option byte locations */
    /* These vary by manufacturer; we probe common locations */

    /* STM32 option byte locations (varies by family) */
    const uint32_t option_locations[] = {
        0x1FFF7000,  /* STM32F0 option bytes */
        0x1FFFC800,  /* STM32L0 option bytes */
        0x40022000,  /* STM32F4 FLASH_OPTCR */
        0x40023C10,  /* H743 FLASH_OPTCR */
        0x08000000,  /* First flash word */
        0x00000000
    };

    for (int i = 0; option_locations[i] != 0; i++)
    {
        uint32_t val;
        if (memap_read(option_locations[i], &val, 4))
        {
            automation_log_event("  Opt[0x%08X] = 0x%08X", option_locations[i], val);

            /* Detect known RDP patterns */
            /* RDP Level 1: often 0xAA or 0x5A at offset 0 */
            if ((val & 0xFF) == 0xAA || (val & 0xFF) == 0x5A)
            {
                desc->flash_locked = true;
                desc->debug_level = 1;
                automation_log_event("  → RDP Level 1 detected (memory readout protected)");
            }
            else if ((val & 0xFF) == 0xCC)
            {
                desc->flash_locked = true;
                desc->debug_level = 2;
                automation_log_event("  → RDP Level 2 detected (permanently locked)");
            }
            else if ((val & 0xFF) == 0x00)
            {
                desc->flash_locked = false;
                desc->debug_level = 0;
                automation_log_event("  → No RDP (fully open)");
            }
        }
    }

    /* Check SPIDEN via DHCSR or debug register */
    /* SPIDEN enables secure debug when set in device option bytes */

    return true;
}

/**
 * flash_read_sector: Read a flash sector (or memory region) into buffer.
 * Used as a building block for full device dumping.
 */
static bool flash_read_sector(uint32_t sector_addr, uint8_t *buffer, uint32_t len)
{
    if (len > SD_CAPTURE_CHUNK)
        len = SD_CAPTURE_CHUNK;

    /* Align to word boundary */
    if (sector_addr & 3)
    {
        automation_log_event("Flash: unaligned address 0x%08X", sector_addr);
        return false;
    }

    if (!memap_read_block(sector_addr, buffer, len))
    {
        automation_log_event("Flash: read failed at 0x%08X", sector_addr);
        return false;
    }

    return true;
}

/**
 * flash_dump_device: Read the entire flash contents and write to SD card.
 * Respects RDP status — refuses if target is locked at level 2.
 */
static bool flash_dump_device(fp_target_desc_t *desc, uint32_t start, uint32_t length)
{
    if (desc->debug_level >= 2)
    {
        automation_log_event("CANNOT DUMP: Target has Level 2 readout protection");
        return false;
    }

    if (desc->flash_locked && desc->debug_level == 1)
    {
        automation_log_event("WARNING: Target has Level 1 RDP — dump may be scrambled");
    }

    automation_log_event("Flash dump: 0x%08X..0x%08X (%d bytes)", start, start + length, length);

    /* Allocate chunk buffer */
    uint8_t chunk[SD_CAPTURE_CHUNK];
    uint32_t remaining = length;
    uint32_t addr = start;
    uint32_t total_read = 0;

    while (remaining > 0)
    {
        uint32_t chunk_len = (remaining > SD_CAPTURE_CHUNK) ? SD_CAPTURE_CHUNK : remaining;

        if (!flash_read_sector(addr, chunk, chunk_len))
        {
            automation_log_event("Dump failed at offset 0x%08X", addr);
            return false;
        }

        /* Write chunk to SD card */
        if (!sdmmc_write_block(addr, chunk, chunk_len))
        {
            automation_log_event("SD card write failed at offset 0x%08X", addr);
            return false;
        }

        remaining -= chunk_len;
        addr += chunk_len;
        total_read += chunk_len;

        /* Progress indicator every 64 KB */
        if ((total_read % (256 * 1024)) == 0)
        {
            automation_log_event("  Dumped: %d / %d bytes", total_read, length);
        }

        /* Feed watchdog */
        IWDG->KR = 0xAAAA;
    }

    automation_log_event("Flash dump complete: %d bytes written to SD", total_read);
    return true;
}

/**
 * flash_unlock: Attempt to unlock the flash for readout.
 * On some targets with only debug-level protection, we can use
 * the DAP to write the option byte unlock key sequence.
 * Returns true if unlock succeeded.
 */
static bool flash_unlock(fp_target_desc_t *desc)
{
    /* This is target-specific; here we implement a generic sequence */
    /* for STM32-family option byte unlocking via DAP */

    if (desc->debug_level == 2)
    {
        automation_log_event("Unlock: Level 2 RDP is irreversible");
        return false;
    }

    /* Try writing the FLASH_OPTKEYR with the unlock keys */
    /* Two keys: 0x08192A3B, 0x4C5D6E7F (STM32) */
    uint32_t keys[] = {0x08192A3B, 0x4C5D6E7F};
    uint32_t optkeyr_addr = 0x40023C08;  /* STM32H7 */
    bool unlock_attempted = false;

    for (int i = 0; i < 2; i++)
    {
        if (memap_write(optkeyr_addr, keys[i], 4))
        {
            unlock_attempted = true;
            delay_ms(10);
        }
    }

    if (!unlock_attempted)
    {
        automation_log_event("Unlock: could not write option key register");
        return false;
    }

    /* Check if unlock worked by re-reading option bytes */
    uint32_t optcr;
    if (memap_read(0x40023C10, &optcr, 4))
    {
        if ((optcr & 0xFF) == 0x00 || (optcr & 0xFF) == 0xAA)
        {
            desc->flash_locked = false;
            desc->debug_level = 0;
            automation_log_event("Unlock: SUCCESS — flash is now readable");
            return true;
        }
    }

    automation_log_event("Unlock: failed — RDP remains active");
    return false;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Automation & Vulnerability Scanning                                        *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * automation_scan_devices: Automated security analysis pipeline.
 * Runs when a target is discovered and unlocked.
 */
static bool automation_scan_devices(void)
{
    automation_log_event("=== Forge-Probe Security Scan Started ===");

    /* 1. Halt the target to get consistent state */
    if (arm_halt_target())
    {
        automation_log_event("Scan: target halted successfully");

        /* 2. Read all core registers */
        uint32_t r0, sp, pc, xpsr;
        arm_read_register(DCRSR_R0, &r0);
        arm_read_register(DCRSR_R13_MSP, &sp);
        arm_read_register(DCRSR_R15, &pc);
        arm_read_register(DCRSR_XPSR, &xpsr);

        automation_log_event("  R0=0x%08X, SP=0x%08X, PC=0x%08X, XPSR=0x%08X",
            r0, sp, pc, xpsr);

        /* 3. Read first few words of flash for identification */
        uint32_t vector_table[8];
        for (int i = 0; i < 8; i++)
        {
            memap_read(0x08000000 + i * 4, &vector_table[i], 4);
        }

        automation_log_event("  Vector table @ 0x08000000:");
        for (int i = 0; i < 8; i++)
        {
            automation_log_event("    [%d]: 0x%08X", i, vector_table[i]);
        }

        /* 4. Check for debug unlock sequences in flash patterns */
        /* Suspicious if first 4 words are all 0xFFFFFFFF (blank part) */
        bool blank_flash = true;
        for (int i = 0; i < 4; i++)
        {
            if (vector_table[i] != 0xFFFFFFFF)
            {
                blank_flash = false;
                break;
            }
        }

        if (blank_flash)
        {
            automation_log_event("WARNING: First flash sector appears blank");
        }

        /* 5. Attempt full flash dump if not locked */
        if (!g_target_desc.flash_locked)
        {
            uint32_t flash_size = 2 * 1024 * 1024;  /* 2 MB default */
            flash_dump_device(&g_target_desc, 0x08000000, flash_size);
        }

        /* 6. Resume target */
        arm_resume_target();
    }
    else
    {
        automation_log_event("Scan: could not halt target (locked or unsupported)");
    }

    automation_log_event("=== Forge-Probe Security Scan Complete ===");
    return true;
}

/**
 * automation_trigger_discovery: Start forced re-discovery of targets.
 * Called when user button is pressed.
 */
static bool automation_trigger_discovery(void)
{
    automation_log_event("User trigger: re-scanning for targets...");
    g_probe_enabled = false;
    g_jtag_clock_hz = JTAG_DEFAULT_CLOCK_HZ;

    /* Enable target power rail (3.3 V default) */
    mpio_write(EN_3V3_PORT, EN_3V3_PIN, 1);
    delay_ms(VOLTAGE_STABILIZE_MS);

    return true;
}

/**
 * automation_log_event: Log a formatted event to the ring buffer
 * and optionally to the SD card log file.
 */
static void automation_log_event(const char *fmt, ...)
{
    char line[128];
    va_list args;

    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    g_log_buffer[g_log_index][0] = '\0';
    strncpy(g_log_buffer[g_log_index], line, 127);
    g_log_buffer[g_log_index][127] = '\0';
    g_log_index = (g_log_index + 1) % LOG_BUFFER_LINES;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  FPGA Bridge — Hardware Acceleration Manager                                *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * fpga_init: Initialize the SPI bridge to the iCE40UP5K FPGA.
 * Configures FPGA with bitstream from flash, verifies CDONE.
 */
static void fpga_init(void)
{
    /* Configure FPGA interface pins */
    mpio_set_mode(FPGA_CS_PORT, FPGA_CS_PIN, ALT_FUNC_PUSHPULL);
    mpio_set_mode(FPGA_SCK_PORT, FPGA_SCK_PIN, ALT_FUNC_PUSHPULL);
    mpio_set_mode(FPGA_MOSI_PORT, FPGA_MOSI_PIN, ALT_FUNC_PUSHPULL);
    mpio_set_mode(FPGA_MISO_PORT, FPGA_MISO_PIN, INPUT);
    mpio_set_mode(FPGA_CRESET_PORT, FPGA_CRESET_PIN, ALT_FUNC_PUSHPULL);
    mpio_set_mode(FPGA_CDONE_PORT, FPGA_CDONE_PIN, INPUT);

    /* Configure SPI5 for FPGA communication */
    /* SPI5: 60 MHz, Mode 0 (CPOL=0, CPHA=0), MSB first, 8-bit */
    RCC->APB2ENR |= RCC_APB2ENR_SPI5EN;

    SPI5->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM |
                (2 << SPI_CR1_BR_Pos) |  /* 60 MHz (120 / 2) */
                SPI_CR1_SPE;

    /* Assert CRESET to start FPGA configuration */
    mpio_write(FPGA_CRESET_PORT, FPGA_CRESET_PIN, 0);
    delay_ms(5);
    mpio_write(FPGA_CRESET_PORT, FPGA_CRESET_PIN, 1);
    delay_ms(10);

    /* Check CDONE to verify configuration success */
    if (mpio_read(FPGA_CDONE_PORT, FPGA_CDONE_PIN))
    {
        automation_log_event("FPGA: configured successfully (CDONE high)");

        /* Verify by reading FPGA ID register */
        uint32_t fpga_id = fpga_read_reg(0x00);
        automation_log_event("FPGA: ID register = 0x%08X", fpga_id);
    }
    else
    {
        automation_log_event("FPGA: CDONE low — configuration FAILED");
    }
}

/**
 * fpga_write_reg: Write a 32-bit register in the FPGA's BRAM window
 * over the SPI bridge.
 */
static void fpga_write_reg(uint32_t offset, uint32_t value)
{
    uint8_t tx_buf[8];

    /* Frame: [2-byte address] [4-byte data] */
    tx_buf[0] = (offset >> 8) & 0xFF;   /* Address high byte */
    tx_buf[1] = offset & 0xFF;           /* Address low byte */
    tx_buf[2] = (value >> 24) & 0xFF;    /* Data MSB */
    tx_buf[3] = (value >> 16) & 0xFF;
    tx_buf[4] = (value >> 8) & 0xFF;
    tx_buf[5] = value & 0xFF;            /* Data LSB */

    /* Assert CS */
    mpio_write(FPGA_CS_PORT, FPGA_CS_PIN, 0);

    for (int i = 0; i < 6; i++)
    {
        while (!(SPI5->SR & SPI_SR_TXE));
        SPI5->DR = tx_buf[i];
        while (!(SPI5->SR & SPI_SR_RXNE));
        (void)SPI5->DR;  /* Discard received byte */
    }

    /* Deassert CS */
    mpio_write(FPGA_CS_PORT, FPGA_CS_PIN, 1);
}

/**
 * fpga_read_reg: Read a 32-bit register from FPGA BRAM over SPI.
 */
static uint32_t fpga_read_reg(uint32_t offset)
{
    uint8_t tx_buf[6];
    uint8_t rx_buf[6];

    /* Frame: [2-byte address with read flag] [4-byte dummy for readback] */
    tx_buf[0] = ((offset >> 8) & 0xFF) | 0x80;  /* Set MSB for read */
    tx_buf[1] = offset & 0xFF;
    tx_buf[2] = 0;
    tx_buf[3] = 0;
    tx_buf[4] = 0;
    tx_buf[5] = 0;

    /* Assert CS */
    mpio_write(FPGA_CS_PORT, FPGA_CS_PIN, 0);

    for (int i = 0; i < 6; i++)
    {
        while (!(SPI5->SR & SPI_SR_TXE));
        SPI5->DR = tx_buf[i];
        while (!(SPI5->SR & SPI_SR_RXNE));
        rx_buf[i] = SPI5->DR;
    }

    /* Deassert CS */
    mpio_write(FPGA_CS_PORT, FPGA_CS_PIN, 1);

    return ((uint32_t)rx_buf[2] << 24) |
           ((uint32_t)rx_buf[3] << 16) |
           ((uint32_t)rx_buf[4] << 8)  |
           ((uint32_t)rx_buf[5] << 0);
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  USB Host Communication — Command Processing                                *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * usb_process_commands: Poll USB for incoming commands and execute them.
 * Runs in the main loop; non-blocking.
 */
static void usb_process_commands(void)
{
    uint8_t cmd_buffer[64];
    int32_t len;

    len = usb_receive_nonblocking(cmd_buffer, sizeof(cmd_buffer));
    if (len <= 0)
        return;

    /* First byte is command opcode */
    uint8_t opcode = cmd_buffer[0];

    switch (opcode)
    {
        case 0x01: /* IDENTIFY */
            cmd_identify();
            break;

        case 0x02: /* READ_MEMORY */
            if (len >= 9)
            {
                uint32_t addr = *(uint32_t*)&cmd_buffer[1];
                uint32_t length = *(uint32_t*)&cmd_buffer[5];
                cmd_read_memory(addr, length);
            }
            break;

        case 0x03: /* WRITE_MEMORY */
            if (len >= 9)
            {
                uint32_t addr = *(uint32_t*)&cmd_buffer[1];
                uint32_t data_len = *(uint32_t*)&cmd_buffer[5];
                cmd_write_memory(addr, &cmd_buffer[9], data_len);
            }
            break;

        case 0x04: /* SCAN / DISCOVER */
            cmd_scan();
            break;

        case 0x05: /* DUMP_FLASH */
            if (len >= 9)
            {
                uint32_t start = *(uint32_t*)&cmd_buffer[1];
                uint32_t length = *(uint32_t*)&cmd_buffer[5];
                cmd_dump_flash();
            }
            break;

        case 0x06: /* FUZZ_MEM_AP */
            cmd_fuzz_mem_ap();
            break;

        case 0x07: /* RESET_TARGET */
            cmd_reset_target();
            break;

        case 0x08: /* HALT_TARGET */
            arm_halt_target();
            break;

        case 0x09: /* RESUME_TARGET */
            arm_resume_target();
            break;

        case 0x0A: /* READ_REGISTER */
            if (len >= 2)
            {
                uint32_t reg_val;
                if (arm_read_register(cmd_buffer[1], &reg_val))
                {
                    uint8_t resp[5] = {0x0A, cmd_buffer[1],
                        (reg_val >> 0) & 0xFF, (reg_val >> 8) & 0xFF,
                        (reg_val >> 16) & 0xFF, (reg_val >> 24) & 0xFF};
                    usb_send(resp, 6);
                }
            }
            break;

        case 0x0B: /* BOUNDARY_SCAN */
            {
                uint8_t bs_buf[BOUNDARY_SCAN_SIZE];
                if (boundary_scan_capture(&g_target_desc, bs_buf))
                {
                    boundary_scan_interpret(&g_target_desc, bs_buf);
                    uint8_t resp[4];
                    resp[0] = 0x0B;
                    *(uint32_t*)&resp[0] = g_target_desc.boundary_chain_len;
                    usb_send(resp, 4);
                }
            }
            break;

        default:
            automation_log_event("USB: unknown command 0x%02X", opcode);
            break;
    }
}

/**
 * cmd_identify: Send device identification to host.
 */
static bool cmd_identify(void)
{
    uint8_t resp[64];
    memset(resp, 0, sizeof(resp));

    resp[0] = 0x01;  /* Opcode response */
    resp[1] = 0x01;  /* Forge-Probe major version */
    resp[2] = 0x00;  /* Minor version */
    resp[3] = (uint8_t)(g_active_protocol);
    resp[4] = g_probe_enabled ? 1 : 0;
    resp[5] = g_target_desc.debug_level;

    /* Copy target name */
    strncpy((char*)&resp[8], g_target_desc.description, 40);
    resp[48] = '\0';

    /* Log count */
    resp[49] = g_log_index;

    usb_send(resp, 64);
    return true;
}

/**
 * cmd_read_memory: Read a block of target memory and send to host.
 */
static bool cmd_read_memory(uint32_t addr, uint32_t len)
{
    uint8_t block[1024];
    uint32_t read_size = (len > sizeof(block)) ? sizeof(block) : len;

    if (!memap_read_block(addr, block, read_size))
    {
        uint8_t err[1] = {0x02};
        usb_send(err, 1);
        return false;
    }

    usb_send(block, read_size);
    return true;
}

/**
 * cmd_scan: Run the full target discovery scan.
 */
static bool cmd_scan(void)
{
    automation_trigger_discovery();

    /* Wait briefly for discovery */
    delay_ms(200);

    /* Return results via USB */
    cmd_identify();
    return true;
}

/**
 * cmd_dump_flash: Initiate a full flash dump to SD card.
 */
static bool cmd_dump_flash(void)
{
    if (!g_probe_enabled)
    {
        uint8_t err[1] = {0x05};
        usb_send(err, 1);
        return false;
    }

    flash_dump_device(&g_target_desc, 0x08000000, 2 * 1024 * 1024);
    return true;
}

/**
 * cmd_fuzz_mem_ap: Fuzz the MEM-AP interface to find
 * unmapped or hidden memory regions. Tries random addresses
 * and detects non-erroneous responses.
 */
static bool cmd_fuzz_mem_ap(void)
{
    automation_log_event("=== MEM-AP Fuzzing Started ===");

    if (!g_probe_enabled || g_active_protocol != FP_PROTO_SWD)
    {
        automation_log_event("Fuzz: MEM-AP not available");
        return false;
    }

    uint32_t test_regions_found = 0;
    uint32_t test_addresses[] = {
        0x00000000, 0x08000000, 0x10000000, 0x20000000,
        0x40000000, 0x50000000, 0x60000000, 0x80000000,
        0xA0000000, 0xC0000000, 0xE0000000, 0xE00FF000,
        0x1FFF0000, 0x1FFFC000, 0x38000000, 0x68000000
    };

    for (int i = 0; i < sizeof(test_addresses) / sizeof(test_addresses[0]); i++)
    {
        uint32_t val;
        if (memap_read(test_addresses[i], &val, 4))
        {
            if (val != 0x00000000 && val != 0xFFFFFFFF)
            {
                automation_log_event("  FOUND: [0x%08X] = 0x%08X",
                    test_addresses[i], val);
                test_regions_found++;
            }
        }
    }

    automation_log_event("Fuzz: %d non-trivial memory regions found", test_regions_found);
    automation_log_event("=== MEM-AP Fuzzing Complete ===");
    return true;
}

/**
 * cmd_reset_target: Apply hardware reset via nSRST line.
 */
static bool cmd_reset_target(void)
{
    automation_log_event("Reset: asserting nSRST");

    /* Pull nSRST low */
    mpio_write(JTAG_nSRST_PORT, JTAG_nSRST_PIN, 0);
    delay_ms(100);

    /* Release */
    mpio_write(JTAG_nSRST_PORT, JTAG_nSRST_PIN, 1);
    delay_ms(50);

    automation_log_event("Reset: complete");
    g_probe_enabled = false;  /* Will be re-discovered */
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════ *
 *  Hardware Abstraction Layer Helpers                                        *
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * set_status_led: Set RGB status LED color.
 * r, g, b: 0 = off, 1 = on
 */
static void set_status_led(uint8_t r, uint8_t g, uint8_t b)
{
    mpio_write(STATUS_LED_R_PORT, STATUS_LED_R_PIN, r ? 0 : 1);  /* Active low */
    mpio_write(STATUS_LED_G_PORT, STATUS_LED_G_PIN, g ? 0 : 1);
    mpio_write(STATUS_LED_B_PORT, STATUS_LED_B_PIN, b ? 0 : 1);
}

/**
 * read_target_voltage_mv: Read the target's VREF voltage in millivolts.
 */
static uint32_t read_target_voltage_mv(void)
{
    /* Read ADC channel 5 (VREF) */
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC));
    uint32_t adc_val = ADC1->DR;

    /* Convert to mV with 3.3 V reference */
    return (adc_val * 3300) / 4095;
}

#endif /* MAIN_IMPL */

/* End of Forge-Probe firmware */