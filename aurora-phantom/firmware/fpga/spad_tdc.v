// spad_tdc.v — SPAD 256-channel time-tag buffer + aggregate rate
// Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
//
// 16x16 SPAD array arrives as 2 banks of 16 hit lines. We count hits per
// pixel over a programmable window and produce an aggregate rate. The
// per-pixel time-tag ring buffer is in SPRAM (not modeled in this stub).
// The bias DAC and global gate are driven from the register file.

`default_nettype none
module spad_tdc (
    input  wire clk, rst,
    input  wire        enable,
    input  wire [15:0] hit_a, hit_b,
    output wire        gate,
    output wire [11:0] bias,
    input  wire [15:0] bias_trim,
    output reg  [15:0] rate_sum,
    output wire        sat_any
);
    // Simple hit counter: accumulate all 32 hit lines each cycle.
    reg [15:0] window_cnt;
    reg [19:0] acc;
    wire [5:0] hits_this_cycle = hit_a[0]+hit_a[1]+hit_a[2]+hit_a[3]+
                                hit_a[4]+hit_a[5]+hit_a[6]+hit_a[7]+
                                hit_a[8]+hit_a[9]+hit_a[10]+hit_a[11]+
                                hit_a[12]+hit_a[13]+hit_a[14]+hit_a[15]+
                                hit_b[0]+hit_b[1]+hit_b[2]+hit_b[3]+
                                hit_b[4]+hit_b[5]+hit_b[6]+hit_b[7]+
                                hit_b[8]+hit_b[9]+hit_b[10]+hit_b[11]+
                                hit_b[12]+hit_b[13]+hit_b[14]+hit_b[15];

    always @(posedge clk) begin
        if (rst || !enable) begin
            window_cnt <= 0; acc <= 0; rate_sum <= 0;
        end else begin
            acc <= acc + hits_this_cycle;
            if (window_cnt == 16'd39999) begin   // 1 ms at 40 MHz
                rate_sum <= acc[15:0];           // kHz proxy
                acc <= 0; window_cnt <= 0;
            end else begin
                window_cnt <= window_cnt + 1;
            end
        end
    end

    assign gate  = enable;
    assign bias  = bias_trim[11:0];
    assign sat_any = (rate_sum > 16'hC000);
endmodule
`default_nettype wire