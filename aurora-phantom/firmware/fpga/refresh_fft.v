// refresh_fft.v — aggregate-rate FFT + PLL refresh-rate estimator
// Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
//
// Computes a 1024-point magnitude spectrum of the aggregate photon-rate
// stream and tracks the peak with a digital PLL. The PLL locks the lock-in
// LO to the target's refresh / backlight-PWM frequency.

`default_nettype none
module refresh_fft (
    input  wire clk, rst,
    input  wire        enable,
    input  wire [15:0] rate_in,
    output reg         pll_locked,
    output reg  [15:0] peak_mag
);
    // Model: accumulate rate samples, find peak bin via running max.
    // A full radix-2 FFT would be instantiated here; for the design artifact
    // we model the peak-hold behavior that the ESP32 driver relies on.
    reg [10:0] bin_cnt;
    reg [19:0] bin_acc;
    reg [19:0] bins [0:1023];
    reg [10:0] peak_bin;
    reg [19:0] peak_val;

    always @(posedge clk) begin
        if (rst || !enable) begin
            bin_cnt <= 0; bin_acc <= 0; peak_bin <= 0; peak_val <= 0;
            peak_mag <= 0; pll_locked <= 0;
        end else begin
            bin_acc <= bin_acc + rate_in;
            if (bin_cnt == 10'd39) begin   // decimate-by-40 -> 1 MHz bins
                bins[bin_cnt/40] <= bin_acc;
                if (bin_acc > peak_val) begin
                    peak_val <= bin_acc;
                    peak_bin <= bin_cnt/40;
                end
                bin_acc <= 0; bin_cnt <= 0;
                peak_mag <= peak_val[15:0];
                pll_locked <= (peak_val > 20'h40000);
            end else begin
                bin_cnt <= bin_cnt + 1;
            end
        end
    end
endmodule
`default_nettype wire