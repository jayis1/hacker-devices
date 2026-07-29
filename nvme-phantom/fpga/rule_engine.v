// rule_engine.v — Rule matching engine for NVMe-Phantom
//
// Author:  jayis1
// License: GPL-2.0
//
// Holds up to 256 rules in BRAM, each 32 bytes.  Matches decoded NVme SQ
// entries against the rules and outputs the action (pass/capture/modify/
// inject/drop) and modify parameters.  Rules are loaded by the MCU over
// the SPI slave interface.
//
// Rule format (32 bytes = 256 bits):
//   [7:0]   match_type   (0=any,1=opcode,2=LBA,3=NSID,4=Opal,5=PRP)
//   [15:8]  action       (0=pass,1=capture,2=modify,3=inject,4=drop)
//   [23:16] op_mask      (opcode to match, or 0xFF for any)
//   [31:24] nsid_mask
//   [63:32] opcode_or_lba_lo
//   [95:64] lba_hi_or_cdw
//   [159:96] param
//   [167:160] modify_field
//   [191:168] reserved
//   [207:192] modify_mask
//   [255:208] modify_value

`default_nettype none

module rule_engine (
    input  wire        clk,
    input  wire        rst_n,
    // Decoded SQ from tlp_parser
    input  wire        sq_valid,
    input  wire [7:0]  sq_opcode,
    input  wire [31:0] sq_cdw10,
    input  wire [63:0] sq_prp1,
    // Rule match output
    output reg         rule_match,
    output reg  [2:0]  rule_action,
    output reg  [31:0] rule_modify_value,
    output reg  [7:0]  rule_modify_field,
    // SPI slave to MCU (for rule load + status)
    input  wire        spi_sck,
    input  wire        spi_mosi,
    output wire        spi_miso,
    input  wire        spi_cs_n
);

    // Rule BRAM: 256 entries x 256 bits = 8 KB
    reg [255:0] rule_ram [0:255];
    reg  [7:0]  rule_idx;          // active scan index
    reg  [7:0]  load_idx;          // SPI load index
    reg         spi_shift_en;

    // SPI slave (simple 32-bit shift register, MSB first)
    reg [31:0]  spi_shift;
    reg [4:0]   spi_bit_cnt;       // 0..31
    reg         spi_active;
    reg         spi_is_cmd;        // 1 = command byte, 0 = data
    reg [7:0]   spi_cmd;
    reg [15:0]  spi_len;
    reg [7:0]   spi_byte_cnt;
    reg [255:0] spi_rule_buf;

    assign spi_miso = spi_shift[31];

    // SPI receive logic
    always @(posedge spi_sck) begin
        if (!spi_cs_n) begin
            spi_shift   <= {spi_shift[30:0], spi_mosi};
            spi_bit_cnt <= spi_bit_cnt + 1;
            if (spi_bit_cnt == 5'd31) begin
                // Full 32-bit word received
                spi_bit_cnt <= 0;
                if (spi_byte_cnt == 0) begin
                    // First word = header [0xA5][cmd][len_lo][len_hi]
                    spi_cmd      <= spi_shift[23:16];
                    spi_len      <= spi_shift[7:0];
                    spi_byte_cnt <= spi_byte_cnt + 4;
                end else begin
                    // Data word — accumulate into rule buffer
                    spi_rule_buf <= {spi_rule_buf[223:0], spi_shift};
                    spi_byte_cnt <= spi_byte_cnt + 4;
                    if (spi_byte_cnt >= 32) begin
                        // Full 32-byte rule received
                        rule_ram[load_idx] <= spi_rule_buf;
                        load_idx <= load_idx + 1;
                        spi_byte_cnt <= 0;
                    end
                end
            end
        end else begin
            spi_bit_cnt  <= 0;
            spi_byte_cnt <= 0;
            spi_active   <= 1'b0;
        end
    end

    // Rule scanning: on each sq_valid, scan all 256 rules for a match
    // (single-cycle per rule, so 256 cycles worst case — acceptable at
    // 125 MHz since NVMe submissions are spaced by host doorbell writes)
    reg [7:0]  scan_idx;
    reg        scanning;
    reg [255:0] cur_rule;

    always @(posedge clk) begin
        if (!rst_n) begin
            scanning      <= 1'b0;
            scan_idx      <= 0;
            rule_match    <= 1'b0;
            rule_action   <= 3'b000;
            rule_modify_value <= 32'h0;
            rule_modify_field <= 8'h0;
        end else begin
            rule_match <= 1'b0;     // default
            if (sq_valid && !scanning) begin
                scanning <= 1'b1;
                scan_idx <= 0;
            end
            if (scanning) begin
                cur_rule <= rule_ram[scan_idx];
                scan_idx <= scan_idx + 1;
                // Check match
                case (cur_rule[7:0])
                8'h00: begin  // match_type = any
                    rule_match <= 1'b1;
                    rule_action <= cur_rule[10:8];
                    rule_modify_value <= cur_rule[255:224];
                    rule_modify_field <= cur_rule[167:160];
                    scanning <= 1'b0;
                end
                8'h01: begin  // match_type = opcode
                    if (sq_opcode == cur_rule[23:16]) begin
                        rule_match <= 1'b1;
                        rule_action <= cur_rule[10:8];
                        rule_modify_value <= cur_rule[255:224];
                        rule_modify_field <= cur_rule[167:160];
                        scanning <= 1'b0;
                    end
                end
                8'h04: begin  // match_type = Opal (opcode = Security Send/Recv)
                    if (sq_opcode == 8'h7D || sq_opcode == 8'h7E ||
                        sq_opcode == 8'h81 || sq_opcode == 8'h82) begin
                        rule_match <= 1'b1;
                        rule_action <= cur_rule[10:8];
                        scanning <= 1'b0;
                    end
                end
                default: ; // no match for this rule
                endcase
                if (scan_idx == 8'hFF) scanning <= 1'b0;
            end
        end
    end

endmodule

`default_nettype wire