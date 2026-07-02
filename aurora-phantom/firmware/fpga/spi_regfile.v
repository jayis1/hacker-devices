// spi_regfile.v — SPI slave register file (ESP32 <-> FPGA)
// Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
//
// 7-bit address / 16-bit payload register file accessible over SPI.
// Protocol: byte0 = {R/W_n, addr[6:0]}, byte1 = hi, byte2 = lo.
// Reads return MISO during byte1/byte2 phases.

`default_nettype none
module spi_regfile (
    input  wire clk, rst,
    input  wire sck, mosi,
    output reg  miso,
    input  wire cs_n,
    output reg  [6:0]  addr,
    output reg  [15:0] wdata,
    output reg         we,
    input  wire [15:0] ctrl,
    output wire [15:0] status
);
    reg [7:0]  shift_in;
    reg [15:0] shift_out;
    reg [1:0]  phase;        // 0=addr, 1=hi, 2=lo
    reg        is_read;

    // 128 x 16 register memory
    reg [15:0] mem [0:127];

    // Read-only / hardware-driven register aliases
    wire [6:0] raddr = shift_in[6:0];

    always @(posedge clk) begin
        if (rst) begin
            miso <= 0; addr <= 0; wdata <= 0; we <= 0;
            phase <= 0; is_read <= 0;
            mem[0] <= 16'hA51D;  // MAGIC
            mem[1] <= 16'h0100;  // VERSION 1.0
        end else begin
            we <= 0;
            if (!cs_n) begin
                // Sample MOSI on sck edges (simplified; real: DDR sample)
                shift_in <= {shift_in[6:0], mosi};
                if (phase == 0 && shift_in[6:0] == raddr) begin
                    is_read <= shift_in[7];
                    addr <= raddr;
                    phase <= 1;
                    shift_out <= is_read ? mem[raddr] : 16'h0000;
                end else if (phase == 1) begin
                    wdata[15:8] <= shift_in[7:0];
                    shift_out <= {shift_out[6:0], miso};
                    phase <= 2;
                end else if (phase == 2) begin
                    wdata[7:0] <= shift_in[7:0];
                    if (!is_read) begin
                        mem[addr] <= {wdata[15:8], shift_in[7:0]};
                        we <= 1;
                    end
                    phase <= 0;
                end
            end else begin
                phase <= 0;
            end
        end
    end

    assign status = {8'h00, ctrl[0], 1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0};

endmodule
`default_nettype wire