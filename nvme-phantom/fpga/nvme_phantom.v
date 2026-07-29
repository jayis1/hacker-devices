// nvme_phantom.v — NVMe-Phantom top-level FPGA (Lattice ECP5 LFE5UM-45F)
//
// Author:  jayis1
// License: GPL-2.0
//
// The FPGA hangs off DSP1 of the PEX8606 switch and sees a mirror copy of
// all Gen3 x4 traffic between the host and the SSD.  It:
//   1. Parses PCIe TLPs (Memory Read/Write) to locate NVMe submission-queue
//      entries (64-byte writes to the SSD BAR0 doorbell region) and
//      completion-queue entries (16-byte writes to host memory).
//   2. Runs a rule engine that matches decoded commands against a 256-entry
//      rule table (loaded by the MCU over SPI) and performs capture /
//      modify / inject / drop actions.
//   3. Exposes a 40 MHz SPI slave interface to the STM32H5 for rule load,
//      decoded-command readout, and injection commands.
//
// This file is the top-level wrapper; submodules (tlp_parser, rule_engine,
// nvme_decoder, spi_slave) are in separate files.

`default_nettype none

module nvme_phantom (
    // 50 MHz clock from onboard oscillator
    input  wire       clk_50mhz,
    // PCIe Gen2 x4 hard IP interface (Lattice ECP5 PCIe hard IP)
    input  wire       pcie_refclk_p,
    input  wire       pcie_refclk_n,
    input  wire [3:0] pcie_rx_n,
    input  wire [3:0] pcie_rx_p,
    output wire [3:0] pcie_tx_n,
    output wire [3:0] pcie_tx_p,
    // SPI slave to STM32H5 (40 MHz)
    input  wire       spi_sck,
    input  wire       spi_mosi,
    output wire       spi_miso,
    input  wire       spi_cs_n,
    output wire       spi_int_n,       // IRQ to MCU: decoded cmd ready
    // PEX8606 modify-override control
    output wire       pex_modify_n,    // active low: assert to enable modify
    // Status LEDs
    output wire       led_activity,
    output wire       led_error
);

    // Internal clocks
    wire clk_core;      // 125 MHz PCIe core clock
    wire clk_pcie;      // PCIe user clock
    wire clk_spi;       // SPI domain (from spi_sck)

    // PLL: 50 MHz -> 125 MHz for PCIe core
    pll_50_to_125 pll_inst (
        .clk_in  (clk_50mhz),
        .clk_out (clk_core),
        .locked  ()
    );

    // -------------------------------------------------------------------
    // PCIe hard IP wrapper (Lattice ECP5 PCIe Gen2 x4 endpoint)
    // -------------------------------------------------------------------
    wire        pcie_link_up;
    wire [31:0] pcie_rx_tlp_data;
    wire [3:0]  pcie_rx_tlp_be;
    wire        pcie_rx_tlp_valid;
    wire        pcie_rx_tlp_sop;
    wire        pcie_rx_tlp_eop;
    wire        pcie_rx_tlp_rden;
    wire [31:0] pcie_tx_tlp_data;
    wire [3:0]  pcie_tx_tlp_be;
    wire        pcie_tx_tlp_valid;
    wire        pcie_tx_tlp_sop;
    wire        pcie_tx_tlp_eop;
    wire        pcie_tx_tlp_wren;
    wire        pcie_tx_tlp_full;

    pcie_hardip pcie_inst (
        .refclk_p        (pcie_refclk_p),
        .refclk_n        (pcie_refclk_n),
        .rx_n            (pcie_rx_n),
        .rx_p            (pcie_rx_p),
        .tx_n            (pcie_tx_n),
        .tx_p            (pcie_tx_p),
        .clk_core        (clk_core),
        .clk_user        (clk_pcie),
        .link_up         (pcie_link_up),
        .rx_tlp_data     (pcie_rx_tlp_data),
        .rx_tlp_be       (pcie_rx_tlp_be),
        .rx_tlp_valid    (pcie_rx_tlp_valid),
        .rx_tlp_sop      (pcie_rx_tlp_sop),
        .rx_tlp_eop      (pcie_rx_tlp_eop),
        .rx_tlp_rden     (pcie_rx_tlp_rden),
        .tx_tlp_data     (pcie_tx_tlp_data),
        .tx_tlp_be       (pcie_tx_tlp_be),
        .tx_tlp_valid    (pcie_tx_tlp_valid),
        .tx_tlp_sop      (pcie_tx_tlp_sop),
        .tx_tlp_eop      (pcie_tx_tlp_eop),
        .tx_tlp_wren     (pcie_tx_tlp_wren),
        .tx_tlp_full     (pcie_tx_tlp_full)
    );

    // -------------------------------------------------------------------
    // TLP parser — extracts NVMe SQ entries from Memory Write TLPs
    // -------------------------------------------------------------------
    wire [63:0]  sq_entry_data;
    wire         sq_entry_valid;
    wire [15:0]  sq_entry_cid;
    wire [7:0]   sq_entry_opcode;
    wire [31:0]  sq_entry_cdw10;
    wire [63:0]  sq_entry_prp1;
    wire         is_memory_write;
    wire         is_memory_read;
    wire         is_completion;

    tlp_parser parser_inst (
        .clk            (clk_pcie),
        .rst_n          (pcie_link_up),
        .rx_data        (pcie_rx_tlp_data),
        .rx_be          (pcie_rx_tlp_be),
        .rx_valid       (pcie_rx_tlp_valid),
        .rx_sop         (pcie_rx_tlp_sop),
        .rx_eop         (pcie_rx_tlp_eop),
        .sq_data        (sq_entry_data),
        .sq_valid       (sq_entry_valid),
        .sq_cid         (sq_entry_cid),
        .sq_opcode      (sq_entry_opcode),
        .sq_cdw10       (sq_entry_cdw10),
        .sq_prp1        (sq_entry_prp1),
        .is_mem_write   (is_memory_write),
        .is_mem_read    (is_memory_read),
        .is_completion  (is_completion)
    );

    // -------------------------------------------------------------------
    // Rule engine — matches decoded commands, drives capture/inject/drop
    // -------------------------------------------------------------------
    wire        rule_match;
    wire [2:0]  rule_action;      // 0=pass,1=capture,2=modify,3=inject,4=drop
    wire [31:0] rule_modify_value;
    wire [7:0]  rule_modify_field;

    rule_engine rules_inst (
        .clk            (clk_pcie),
        .rst_n          (pcie_link_up),
        .sq_valid       (sq_entry_valid),
        .sq_opcode      (sq_entry_opcode),
        .sq_cdw10       (sq_entry_cdw10),
        .sq_prp1        (sq_entry_prp1),
        .rule_match     (rule_match),
        .rule_action    (rule_action),
        .rule_modify_value (rule_modify_value),
        .rule_modify_field (rule_modify_field),
        // SPI interface to MCU for rule load
        .spi_sck        (spi_sck),
        .spi_mosi       (spi_mosi),
        .spi_miso       (spi_miso),
        .spi_cs_n       (spi_cs_n)
    );

    // -------------------------------------------------------------------
    // Capture FIFO — stores decoded commands for MCU readout
    // -------------------------------------------------------------------
    reg  [63:0] cap_fifo [0:255];      // 256-entry x 64-byte FIFO
    reg  [7:0]  cap_wr_ptr;
    reg  [7:0]  cap_rd_ptr;
    reg         cap_overflow;

    always @(posedge clk_pcie) begin
        if (sq_entry_valid && (rule_action == 3'b001 || rule_action == 3'b010)) begin
            cap_fifo[cap_wr_ptr] <= sq_entry_data;
            cap_wr_ptr <= cap_wr_ptr + 1;
        end
    end

    // Decoded command ready interrupt to MCU
    assign spi_int_n = ~((cap_wr_ptr != cap_rd_ptr) && spi_cs_n);

    // -------------------------------------------------------------------
    // Modify override to PEX8606
    // -------------------------------------------------------------------
    // When rule_action == MODIFY, assert pex_modify_n to tell the switch
    // to accept the FPGA's rewritten TLP on DSP0.
    assign pex_modify_n = ~((rule_action == 3'b010) && rule_match);

    // -------------------------------------------------------------------
    // TX path — inject synthetic TLPs (for INJECT and DMA modes)
    // -------------------------------------------------------------------
    // The MCU pushes injection TLPs over SPI; the rule_engine module
    // routes them to the PCIe TX path.
    assign pcie_tx_tlp_wren  = (rule_action == 3'b011) && rule_match;
    assign pcie_tx_tlp_sop   = pcie_tx_tlp_wren;
    assign pcie_tx_tlp_eop   = pcie_tx_tlp_wren;
    assign pcie_tx_tlp_data  = rule_modify_value;
    assign pcie_tx_tlp_be    = 4'hF;

    // -------------------------------------------------------------------
    // Status LEDs
    // -------------------------------------------------------------------
    reg [23:0] blink_cnt;
    always @(posedge clk_core) begin
        blink_cnt <= blink_cnt + 1;
    end
    assign led_activity = pcie_link_up ? blink_cnt[23] : 1'b1;
    assign led_error    = ~pcie_link_up;

endmodule

`default_nettype wire