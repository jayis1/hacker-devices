// tlp_parser.v — PCIe TLP parser for NVMe-Phantom
//
// Author:  jayis1
// License: GPL-2.0
//
// Parses incoming PCIe TLPs from the PEX8606 mirror port and extracts
// NVMe submission-queue entries (64-byte Memory Writes to the SSD BAR0)
// and completion-queue entries (16-byte Memory Writes to host memory).
//
// PCIe TLP format (32-bit words, little-endian over the link):
//   Word 0: [Fmt(3) Type(5) TC(3) ... ECRC(1) Attr(2) Length(10)]
//   Word 1: [Requester ID(16) Tag(8) Last BE(4) First BE(4)]   (for MRd/MWr)
//           or [Completer ID(16) Status(3) BCM(1) Length(12)]   (for Cpl)
//   Word 2: [Address low 32 bits, byte-aligned to 4]
//   Words 3..N: payload (for Memory Write) or completion data
//
// NVMe SQ entry = 64 bytes = 16 dwrds, written to BAR0 doorbell offset.

`default_nettype none

module tlp_parser (
    input  wire        clk,
    input  wire        rst_n,
    // RX interface from PCIe hard IP
    input  wire [31:0] rx_data,
    input  wire [3:0]  rx_be,
    input  wire        rx_valid,
    input  wire        rx_sop,
    input  wire        rx_eop,
    // Decoded NVMe SQ outputs
    output wire [63:0] sq_data,
    output wire        sq_valid,
    output wire [15:0] sq_cid,
    output wire [7:0]  sq_opcode,
    output wire [31:0] sq_cdw10,
    output wire [63:0] sq_prp1,
    output wire        is_mem_write,
    output wire        is_mem_read,
    output wire        is_completion
);

    // TLP type field encoding
    localparam TLP_MWR  = 6'b000000;   // Memory Write
    localparam TLP_MRD  = 6'b000001;   // Memory Read
    localparam TLP_CPL  = 6'b100010;   // Completion

    // State machine
    localparam S_IDLE    = 2'd0;
    localparam S_HDR1    = 2'd1;
    localparam S_ADDR    = 2'd2;
    localparam S_PAYLOAD = 2'd3;

    reg [1:0]  state;
    reg [31:0] hdr0;          // TLP header word 0
    reg [31:0] hdr1;          // TLP header word 1
    reg [2:0]  tlp_fmt;       // format bits
    reg [4:0]  tlp_type;      // type bits
    reg [9:0]  tlp_length;    // payload length in dwrds
    reg        is_mwr;
    reg        is_mrd;
    reg        is_cpl;
    reg [7:0]  byte_cnt;      // payload byte counter

    // 64-byte SQ entry capture buffer
    reg [63:0] sq_buf [0:15]; // 16 x 32-bit = 512 bits, but we store 64B
    reg [3:0]  sq_word_cnt;
    reg        sq_valid_r;

    assign is_mem_write  = is_mwr;
    assign is_mem_read   = is_mrd;
    assign is_completion = is_cpl;
    assign sq_valid      = sq_valid_r;
    assign sq_data       = {sq_buf[1], sq_buf[0]};  // first 64 bits
    assign sq_opcode     = sq_buf[0][7:0];
    assign sq_cid        = sq_buf[0][31:16];
    assign sq_cdw10      = sq_buf[5];               // word 10 = offset 40
    assign sq_prp1       = {sq_buf[5], sq_buf[4]};  // PRP1 = words 4..5

    integer i;
    always @(posedge clk) begin
        if (!rst_n) begin
            state       <= S_IDLE;
            is_mwr      <= 1'b0;
            is_mrd      <= 1'b0;
            is_cpl      <= 1'b0;
            sq_valid_r  <= 1'b0;
            sq_word_cnt <= 0;
            byte_cnt    <= 0;
        end else begin
            sq_valid_r <= 1'b0;     // default pulse
            if (!rx_valid) begin
                // idle
            end else begin
                case (state)
                S_IDLE: begin
                    if (rx_sop) begin
                        hdr0     <= rx_data;
                        tlp_fmt  <= rx_data[30:28];
                        tlp_type <= rx_data[26:22];
                        tlp_length <= rx_data[9:0];
                        is_mwr <= (rx_data[30:28] == 3'b010) && (rx_data[26:22] == 5'b00000);
                        is_mrd <= (rx_data[30:28] == 3'b000) && (rx_data[26:22] == 5'b00000);
                        is_cpl <= (rx_data[30:28] == 3'b000) && (rx_data[26:22] == 5'b01010);
                        state  <= S_HDR1;
                    end
                end
                S_HDR1: begin
                    hdr1 <= rx_data;
                    if (is_mwr || is_mrd) begin
                        state <= S_ADDR;
                    end else begin
                        state <= S_PAYLOAD;
                        sq_word_cnt <= 0;
                    end
                end
                S_ADDR: begin
                    // rx_data = low 32 bits of address
                    // NVMe doorbell region: check if address matches BAR0
                    // (simplified: assume any MWr with length=16 dwrds is an SQ entry)
                    if (is_mwr && (tlp_length == 10'd16)) begin
                        state       <= S_PAYLOAD;
                        sq_word_cnt <= 0;
                    end else begin
                        // Not an SQ entry — skip payload
                        state <= (rx_eop) ? S_IDLE : S_PAYLOAD;
                        sq_word_cnt <= 0;
                    end
                end
                S_PAYLOAD: begin
                    if (is_mwr && (tlp_length == 10'd16)) begin
                        // Capture SQ entry words
                        sq_buf[sq_word_cnt] <= rx_data;
                        sq_word_cnt <= sq_word_cnt + 1;
                        if (sq_word_cnt == 15) begin
                            sq_valid_r <= 1'b1;
                            state      <= S_IDLE;
                        end
                    end
                    if (rx_eop) state <= S_IDLE;
                end
                default: state <= S_IDLE;
                endcase
            end
        end
    end

endmodule

`default_nettype wire