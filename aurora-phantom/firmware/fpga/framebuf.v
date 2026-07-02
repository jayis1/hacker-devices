// framebuf.v — IQ accumulator + frame magnitude buffer
// Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
//
// Computes |IQ| per pixel via alpha-max and exposes a frame-magnitude
// stream to the ESP32 via the register-file auto-increment path.

`default_nettype none
module framebuf (
    input  wire clk, rst,
    input  wire [15:0] iq_i, iq_q,
    input  wire [7:0]  iq_idx,
    output reg  [7:0]  frame_idx,
    output reg  [15:0] frame_mag,
    output reg         frame_valid
);
    // Alpha-max: |z| ~= max(|I|,|Q|) + 0.5*min(|I|,|Q|)
    wire [15:0] ai = iq_i[15] ? ~iq_i + 1 : iq_i;
    wire [15:0] aq = iq_q[15] ? ~iq_q + 1 : iq_q;
    wire [15:0] mx = (ai > aq) ? ai : aq;
    wire [15:0] mn = (ai > aq) ? aq : ai;
    wire [15:0] mag = mx + (mn >> 1);

    reg [7:0] count;

    always @(posedge clk) begin
        if (rst) begin
            frame_idx <= 0; frame_mag <= 0; frame_valid <= 0; count <= 0;
        end else begin
            frame_mag <= mag;
            frame_idx <= iq_idx;
            frame_valid <= (iq_idx == 8'd255);
            count <= count + 1;
        end
    end
endmodule
`default_nettype wire