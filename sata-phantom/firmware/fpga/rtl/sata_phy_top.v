/**
 * @file sata_phy_top.v
 * @author jayis1
 * @brief Top-level SATA PHY + Link Layer for Gowin GW1N-LV1 FPGA
 *
 * This is the top-level Verilog module for the SATA Phantom's FPGA.
 * It instantiates the SATA PHY, link-layer FSM, frame inspector,
 * filter/trigger engine, injection FIFO, scratch buffer, and SPI slave.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

`timescale 1ns / 1ps

module sata_phy_top (
    // SATA bus interface (host side)
    input  wire        sata_hp_rx_p,       // Host TX+ → FPGA RX+
    input  wire        sata_hp_rx_n,
    output wire        sata_hp_tx_p,       // FPGA TX+ → Host RX+
    output wire        sata_hp_tx_n,

    // SATA bus interface (drive side)
    input  wire        sata_dp_rx_p,       // Drive TX+ → FPGA RX+
    input  wire        sata_dp_rx_n,
    output wire        sata_dp_tx_p,       // FPGA TX+ → Drive RX+
    output wire        sata_dp_tx_n,

    // Redriver control
    output wire        redriver_host_sel,   // 0=passthrough, 1=FPGA path
    output wire        redriver_drive_sel,

    // Reference clock
    input  wire        clk_25mhz,           // 25 MHz external oscillator

    // SPI slave interface (to ESP32)
    input  wire        spi_cs_n,
    input  wire        spi_sclk,
    input  wire        spi_mosi,
    output wire        spi_miso,

    // Control to ESP32
    output wire        irq_out,             // Interrupt: rule match / capture ready
    output wire        busy_out,            // FPGA processing busy
    output wire        config_done_out,     // FPGA configuration complete

    // FPGA configuration interface (from SPI flash)
    input  wire        flash_cs_n,
    input  wire        flash_clk,
    input  wire        flash_di,
    output wire        flash_do,

    // Power management
    input  wire        reset_n,             // External reset (active low)
    output wire        pll_locked           // PLL status
);

    // ===================================================================
    // Internal Signal Declarations
    // ===================================================================

    // Clock and reset
    wire        clk_sata_150mhz;    // 150 MHz for SATA Gen1 (1.5 Gbps)
    wire        clk_sata_300mhz;    // 300 MHz for SATA Gen2 (3.0 Gbps)
    wire        clk_sata_600mhz;    // 600 MHz for SATA Gen3 (6.0 Gbps) — not used directly
    wire        clk_core;           // Core processing clock (150 MHz)
    wire        clk_spi;            // SPI clock domain
    wire        rst;                // Synchronous reset (active high)

    // SATA OOB signals
    wire        oob_cominit_det;    // COMINIT detected from device
    wire        oob_comwake_det;    // COMWAKE detected
    wire        oob_comreset_det;   // COMRESET detected
    wire        oob_cominit_send;   // Send COMINIT
    wire        oob_comwake_send;   // Send COMWAKE

    // SATA link state
    wire        link_up;            // Link established
    wire  [1:0] link_speed;         // 0=1.5G, 1=3.0G, 2=6.0G

    // SATA frame data (host→drive direction)
    wire        h2d_valid;          // Frame valid from host
    wire  [9:0] h2d_data;          // 8b/10b decoded data
    wire        h2d_is_k;          // Is K-code (control character)
    wire        h2d_sof;           // Start of frame
    wire        h2d_eof;           // End of frame

    // SATA frame data (drive→host direction)
    wire        d2h_valid;
    wire  [9:0] d2h_data;
    wire        d2h_is_k;
    wire        d2h_sof;
    wire        d2h_eof;

    // Frame inspection results
    wire  [3:0] match_rule_id;     // Which rule matched
    wire        match_valid;        // Rule match strobe
    wire  [1:0] match_action;      // 0=monitor, 1=drop, 2=corrupt, 3=inject_after

    // Frame modification/corruption
    wire        modify_enable;      // Enable frame modification
    wire [31:0] modify_mask;       // Bit mask for corruption
    wire        inject_enable;     // Enable injection from FIFO
    wire  [9:0] inject_data;      // Injected frame data
    wire        inject_valid;

    // Capture interface
    wire        capture_start;
    wire        capture_done;
    wire [12:0] capture_length;    // Captured frame length in bytes
    wire [9:0]  capture_data_out;
    wire        capture_data_valid;

    // SPI register interface
    wire [15:0] reg_addr;
    wire [31:0] reg_wdata;
    wire        reg_we;
    wire        reg_re;
    wire [31:0] reg_rdata;
    wire        reg_ack;

    // Status registers
    reg  [31:0] sys_status;
    reg  [31:0] sys_version;
    reg  [31:0] ctrl_main;
    reg  [31:0] link_status;
    reg  [47:0] counter_read;
    reg  [47:0] counter_write;
    reg  [31:0] counter_dropped;
    reg  [31:0] counter_modified;
    reg  [31:0] counter_injected;
    reg  [31:0] counter_errors;
    reg  [31:0] counter_oob;
    reg  [31:0] capture_len_reg;
    reg  [31:0] capture_fis_type;
    reg  [31:0] capture_lba_lo;
    reg  [31:0] capture_lba_hi;
    reg  [31:0] capture_dir;

    // ===================================================================
    // Clock Generation (PLL)
    // ===================================================================

    clock_manager clk_mgr (
        .clk_in_25mhz   (clk_25mhz),
        .rst_n          (reset_n),
        .clk_150mhz     (clk_sata_150mhz),
        .clk_300mhz     (clk_sata_300mhz),
        .clk_core       (clk_core),
        .pll_locked     (pll_locked)
    );

    assign clk_spi = clk_core;  // SPI runs at core speed

    // Reset synchronization
    reg rst_sync_1, rst_sync_2;
    always @(posedge clk_core or negedge reset_n) begin
        if (!reset_n) begin
            rst_sync_1 <= 1'b1;
            rst_sync_2 <= 1'b1;
        end else begin
            rst_sync_1 <= 1'b0;
            rst_sync_2 <= rst_sync_1;
        end
    end
    assign rst = rst_sync_2;

    // ===================================================================
    // SATA PHY + Link Layer — Host Side
    // ===================================================================

    sata_link_layer #(
        .SIDE("HOST")
    ) host_link (
        .clk            (clk_core),
        .rst            (rst),
        .sata_rx_p      (sata_hp_rx_p),
        .sata_rx_n      (sata_hp_rx_n),
        .sata_tx_p      (sata_hp_tx_p),
        .sata_tx_n      (sata_hp_tx_n),
        .oob_cominit    (oob_cominit_det),
        .oob_comwake    (oob_comwake_det),
        .oob_comreset   (oob_comreset_det),
        .oob_send_cominit(oob_cominit_send),
        .oob_send_comwake(oob_comwake_send),
        .link_up        (link_up),
        .link_speed     (link_speed),
        .frame_valid    (h2d_valid),
        .frame_data     (h2d_data),
        .frame_is_k     (h2d_is_k),
        .frame_sof      (h2d_sof),
        .frame_eof      (h2d_eof)
    );

    // ===================================================================
    // SATA PHY + Link Layer — Drive Side
    // ===================================================================

    sata_link_layer #(
        .SIDE("DRIVE")
    ) drive_link (
        .clk            (clk_core),
        .rst            (rst),
        .sata_rx_p      (sata_dp_rx_p),
        .sata_rx_n      (sata_dp_rx_n),
        .sata_tx_p      (sata_dp_tx_p),
        .sata_tx_n      (sata_dp_tx_n),
        .oob_cominit    (),
        .oob_comwake    (),
        .oob_comreset   (),
        .oob_send_cominit(1'b0),
        .oob_send_comwake(1'b0),
        .link_up        (),
        .link_speed     (),
        .frame_valid    (d2h_valid),
        .frame_data     (d2h_data),
        .frame_is_k     (d2h_is_k),
        .frame_sof      (d2h_sof),
        .frame_eof      (d2h_eof)
    );

    // ===================================================================
    // Frame Inspector (Pattern Matching Engine)
    // ===================================================================

    frame_inspector #(
        .NUM_RULES(16)
    ) inspector (
        .clk            (clk_core),
        .rst            (rst),
        .enable         (ctrl_main[1]),  // CTRL_MAIN_MONITOR_EN
        .h2d_valid      (h2d_valid),
        .h2d_data       (h2d_data),
        .h2d_is_k       (h2d_is_k),
        .h2d_sof        (h2d_sof),
        .h2d_eof        (h2d_eof),
        .d2h_valid      (d2h_valid),
        .d2h_data       (d2h_data),
        .d2h_is_k       (d2h_is_k),
        .d2h_sof        (d2h_sof),
        .d2h_eof        (d2h_eof),
        .match_rule_id  (match_rule_id),
        .match_valid    (match_valid),
        .match_action   (match_action)
    );

    // ===================================================================
    // Filter / Trigger Engine
    // ===================================================================

    filter_trigger filter (
        .clk            (clk_core),
        .rst            (rst),
        .match_valid    (match_valid),
        .match_action   (match_action),
        .modify_enable  (modify_enable),
        .modify_mask    (modify_mask),
        .inject_enable  (inject_enable),
        .drop_frame     (drop_frame),
        .irq_out        (irq_out)
    );

    // ===================================================================
    // Injection FIFO
    // ===================================================================

    injection_fifo #(
        .FIFO_DEPTH(512)
    ) inject_fifo (
        .clk            (clk_core),
        .rst            (rst),
        .push_enable    (inject_enable),
        .push_data      (inject_data_internal),
        .pop_ready      (h2d_eof & inject_enable),
        .pop_data       (inject_data),
        .pop_valid      (inject_valid),
        .fifo_empty     (),
        .fifo_full      (),
        .fifo_level     ()
    );

    // ===================================================================
    // Scratch Buffer (8 KB dual-port SRAM)
    // ===================================================================

    scratch_buffer #(
        .DEPTH(8192)
    ) scratch (
        .clk            (clk_core),
        .rst            (rst),
        .capture_start  (capture_start),
        .capture_data   (capture_data),
        .capture_valid  (capture_valid),
        .capture_done   (capture_done),
        .capture_length (capture_length),
        .read_addr      (scratch_read_addr),
        .read_data      (scratch_read_data)
    );

    // ===================================================================
    // SPI Slave Interface
    // ===================================================================

    spi_slave spi (
        .clk            (clk_spi),
        .rst            (rst),
        .spi_cs_n       (spi_cs_n),
        .spi_sclk       (spi_sclk),
        .spi_mosi       (spi_mosi),
        .spi_miso       (spi_miso),
        .reg_addr       (reg_addr),
        .reg_wdata      (reg_wdata),
        .reg_we         (reg_we),
        .reg_re         (reg_re),
        .reg_rdata      (reg_rdata),
        .reg_ack        (reg_ack)
    );

    // ===================================================================
    // Register Read Multiplexer
    // ===================================================================

    always @(*) begin
        reg_rdata = 32'h00000000;
        case (reg_addr)
            // System
            16'h0000: reg_rdata = sys_version;
            16'h0005: reg_rdata = sys_status;

            // Control
            16'h0010: reg_rdata = ctrl_main;

            // Status
            16'h0020: reg_rdata = link_status;
            16'h0022: reg_rdata = counter_read[31:0];
            16'h0023: reg_rdata = {16'h0000, counter_read[47:32]};
            16'h0024: reg_rdata = counter_write[31:0];
            16'h0025: reg_rdata = {16'h0000, counter_write[47:32]};
            16'h0026: reg_rdata = counter_dropped;
            16'h0027: reg_rdata = counter_modified;
            16'h0029: reg_rdata = counter_injected;
            16'h0028: reg_rdata = counter_errors;

            // Capture
            16'h0031: reg_rdata = {capture_done, capture_start, 30'h00000000};
            16'h0032: reg_rdata = capture_len_reg;
            16'h0033: reg_rdata = capture_fis_type;
            16'h0034: reg_rdata = capture_lba_lo;
            16'h0035: reg_rdata = capture_lba_hi;
            16'h0036: reg_rdata = capture_dir;

            // Version
            16'h00FE: reg_rdata = 32'h01000000;  // v1.0.0

            default: reg_rdata = 32'h00000000;
        endcase
    end

    // ===================================================================
    // Register Write Decoder
    // ===================================================================

    always @(posedge clk_core or posedge rst) begin
        if (rst) begin
            ctrl_main <= 32'h00000000;
        end else if (reg_we) begin
            case (reg_addr)
                16'h0010: ctrl_main <= reg_wdata;
            endcase
        end
    end

    // ===================================================================
    // Status Register Updates
    // ===================================================================

    always @(posedge clk_core or posedge rst) begin
        if (rst) begin
            sys_status <= 32'h00000000;
        end else begin
            sys_status[0] <= pll_locked;      // Config done
            sys_status[1] <= pll_locked;      // PLL locked
            sys_status[2] <= link_up;         // Link up
            sys_status[3] <= h2d_valid;       // RX active
            sys_status[4] <= 1'b0;            // TX active (simplified)
            sys_status[5] <= capture_done;    // Capture busy
            sys_status[15] <= 1'b0;           // Fatal (no error)
        end
    end

    assign sys_version = 32'h01000000;  // v1.0.0
    assign link_status = {16'h0000, 1'b0, 3'b000, 4'h0, 4'h0, link_speed, link_up};
    assign config_done_out = pll_locked;
    assign busy_out = h2d_valid | d2h_valid;

    // Redriver control — select FPGA path when not in transparent mode
    assign redriver_host_sel = ~ctrl_main[4];  // CTRL_MAIN_PASSTHROUGH
    assign redriver_drive_sel = ~ctrl_main[4];

    // Frame counters
    always @(posedge clk_core or posedge rst) begin
        if (rst) begin
            counter_read  <= 48'h000000000000;
            counter_write <= 48'h000000000000;
        end else begin
            if (d2h_eof & d2h_valid) counter_read <= counter_read + 1;
            if (h2d_eof & h2d_valid) counter_write <= counter_write + 1;
        end
    end

endmodule
