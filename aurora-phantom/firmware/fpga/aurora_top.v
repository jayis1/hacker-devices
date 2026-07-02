// aurora_top.v — Aurora-Phantom FPGA signal-core top level
// Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
//
// iCE40UP5K top-level for the Aurora-Phantom signal core. Instantiates:
//   - spi_regfile : SPI slave register file exposed to the ESP32-S3
//   - spad_tdc    : 256-channel SPAD photon time-tag buffer
//   - lockin      : per-pixel digital lock-in demodulator
//   - refresh_fft : aggregate-rate FFT + PLL refresh-rate estimator
//   - framebuf    : IQ accumulator + frame magnitude buffer
//
// All submodules are in separate files; this file wires them together and
// defines the clock/reset domain.

`default_nettype none

module aurora_top (
    input  wire clk_40m,        // 40 MHz from Si5351C (TCXO-locked)
    input  wire rst_n,          // active-low reset from ESP32

    // SPI slave to ESP32-S3 (register file access)
    input  wire spi_sck,
    input  wire spi_mosi,
    output wire spi_miso,
    input  wire spi_cs_n,

    // SPAD array event bus (256 pixels, time-multiplexed 16 lanes x 2 banks)
    input  wire [15:0] spad_hit_a,   // bank A hit pulses (active high)
    input  wire [15:0] spad_hit_b,   // bank B hit pulses
    output wire        spad_gate,    // global gate enable
    output wire [11:0] spad_bias,    // bias trim DAC

    // Event stream FIFO to ESP32 (optional raw-event path)
    output wire [15:0] fifo_data,
    output wire        fifo_valid,
    input  wire        fifo_rd_en,

    // Interrupt to ESP32
    output wire irq
);
    localparam CLK_HZ = 40_000_000;

    // ---- Reset sync ----
    reg [1:0] rst_sync;
    always @(posedge clk_40m) begin
        rst_sync <= {rst_sync[0], rst_n};
    end
    wire rst = ~rst_sync[1];

    // ---- Internal busses from regfile ----
    wire [6:0]  reg_addr;
    wire [15:0] reg_wdata;
    wire        reg_we;
    wire [15:0] reg_rdata;
    wire [15:0] ctrl_reg;
    wire [15:0] status_reg;

    spi_regfile u_regfile (
        .clk(clk_40m), .rst(rst),
        .sck(spi_sck), .mosi(spi_mosi), .miso(spi_miso), .cs_n(spi_cs_n),
        .addr(reg_addr), .wdata(reg_wdata), .we(reg_we), .rdata(reg_rdata),
        .ctrl(ctrl_reg), .status(status_reg)
    );

    // ---- SPAD TDC + time-tag buffer ----
    wire [15:0] rate_sum;
    wire        sat_any;
    spad_tdc u_spad (
        .clk(clk_40m), .rst(rst),
        .enable(ctrl_reg[1]),         // CTRL_SPAD_ENABLE
        .hit_a(spad_hit_a), .hit_b(spad_hit_b),
        .gate(spad_gate), .bias(spad_bias),
        .bias_trim(reg_wdata),        // from REG_SPAD_BIAS
        .rate_sum(rate_sum),
        .sat_any(sat_any)
    );

    // ---- Lock-in demodulator ----
    wire [15:0] lo_freq_hi, lo_freq_lo;
    wire [15:0] int_win_hi,  int_win_lo;
    wire        lockin_enable = ctrl_reg[2];
    wire [15:0] iq_i, iq_q;
    wire [7:0]  iq_idx;
    lockin u_lockin (
        .clk(clk_40m), .rst(rst),
        .enable(lockin_enable),
        .rate_in(rate_sum),
        .lo_freq_hi(lo_freq_hi), .lo_freq_lo(lo_freq_lo),
        .int_win_hi(int_win_hi), .int_win_lo(int_win_lo),
        .iq_i(iq_i), .iq_q(iq_q), .iq_idx(iq_idx)
    );

    // ---- Refresh-rate FFT + PLL ----
    wire        pll_locked;
    wire [15:0] fft_peak_mag;
    refresh_fft u_fft (
        .clk(clk_40m), .rst(rst),
        .enable(ctrl_reg[3]),
        .rate_in(rate_sum),
        .pll_locked(pll_locked),
        .peak_mag(fft_peak_mag)
    );

    // ---- Frame buffer / IQ accumulator ----
    wire [15:0] frame_mag;
    wire [7:0]  frame_idx;
    wire        frame_valid;
    framebuf u_framebuf (
        .clk(clk_40m), .rst(rst),
        .iq_i(iq_i), .iq_q(iq_q), .iq_idx(iq_idx),
        .frame_idx(frame_idx), .frame_mag(frame_mag),
        .frame_valid(frame_valid)
    );

    // ---- Event stream FIFO mux ----
    // (raw events or IQ stream -> ESP32 via SPI burst reads)
    assign fifo_data  = frame_mag;
    assign fifo_valid = frame_valid;
    assign irq        = frame_valid & ctrl_reg[7];

    // ---- Status register composition ----
    // (status_reg driven inside regfile from these signals)
    wire [15:0] status_compose =
        {8'b0, pll_locked, sat_any, frame_valid,
         1'b0 /*fifo_halffull*/, 1'b0 /*fifo_empty*/, 1'b0 /*fifo_ovfl*/, ctrl_reg[0]};

endmodule

`default_nettype wire