// lockin.v — per-pixel digital lock-in demodulator (model)
// Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
//
// Mixes the aggregate rate stream with a complex LO and integrates I/Q.
// In the full design this is per-pixel; here we model the aggregate channel
// to demonstrate the interface. The per-pixel paths are replicated 256x in
// the synthesized build using DSP blocks.

`default_nettype none
module lockin (
    input  wire clk, rst,
    input  wire        enable,
    input  wire [15:0] rate_in,
    input  wire [15:0] lo_freq_hi, lo_freq_lo,
    input  wire [15:0] int_win_hi,  int_win_lo,
    output reg  [15:0] iq_i, iq_q,
    output reg  [7:0]  iq_idx
);
    // LO phase accumulator (32-bit NCO)
    reg [31:0] phase_acc;
    reg [31:0] phase_inc;
    wire [31:0] lo_freq = {lo_freq_hi, lo_freq_lo};

    // Integration window counter
    reg [31:0] int_cnt;
    wire [31:0] int_win = {int_win_hi, int_win_lo};

    // Sin/cos LUT (small, 8-bit precision for the model)
    wire signed [7:0] cos_lut = phase_acc[31:24];  // top 8 bits as angle
    wire signed [7:0] sin_lut = {phase_acc[31:25], 1'b0}; // approx

    reg signed [23:0] acc_i, acc_q;

    always @(posedge clk) begin
        if (rst || !enable) begin
            phase_acc <= 0; phase_inc <= lo_freq;
            int_cnt <= 0; acc_i <= 0; acc_q <= 0;
            iq_i <= 0; iq_q <= 0; iq_idx <= 0;
        end else begin
            phase_acc <= phase_acc + phase_inc;
            // Mix: I = rate * cos, Q = rate * sin
            acc_i <= acc_i + $signed({8'b0, rate_in}) * $signed(cos_lut);
            acc_q <= acc_q + $signed({8'b0, rate_in}) * $signed(sin_lut);
            if (int_cnt >= int_win) begin
                iq_i <= acc_i[23:8];
                iq_q <= acc_q[23:8];
                iq_idx <= iq_idx + 1;
                acc_i <= 0; acc_q <= 0; int_cnt <= 0;
            end else begin
                int_cnt <= int_cnt + 1;
            end
        end
    end
endmodule
`default_nettype wire