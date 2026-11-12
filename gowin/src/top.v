// gowin/src/top.v — top-level Tang Nano 4K (GW1NSR-LV4CQN48PC6/I5): Gowin_EMPU (Cortex-M3) + cổng APB + bộ tăng tốc
// tăng dần. Cổng và chân theo docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md mục 12.5.2, 12.5.6. Verilog-2001.
// Gowin_EMPU (IP Core Generator hoặc rtl/scripts/build_gowin.sh): GPIO (không dùng IOBUF trong IP), UART0, UART1,
// APB2 Master 1, USER_INT0, SRAM 8 KB.
//   gpio[0] imu_scl   gpio[1] imu_sda   gpio[2] imu_int   gpio[3] wifi_rst   gpio[4] sens_pwr   gpio[5] buz
//   gpio[6] btn_n     gpio[7] led       gpio[8] buz_2khz (0 = mức cao liên tục, 1 = sóng 2 kHz)   gpio[15:9] trống
// I²C mềm (O6): firmware ghi 0 vào bit dữ liệu rồi bật/tắt cho phép xuất (cực máng hở; kéo lên có sẵn trên board).
// Một xung nhịp (O5): thạch anh 27 MHz vào M3; FPGA chạy theo master_pclk của APB2 (cùng nguồn).
module top #(
    parameter WFILE0 = "src/rom_inc_0.mem",
    parameter WFILE1 = "src/rom_inc_1.mem",
    parameter WFILE2 = "src/rom_inc_2.mem",
    parameter CNN   = 1                  // 0: chỉ EMPU (S0, đo tài nguyên riêng của M3)
) (
    input  wire clk27,    input  wire rst_n,
    inout  wire imu_scl,  inout  wire imu_sda,  input wire imu_int,
    output wire wifi_txd, input  wire wifi_rxd,
    output wire dbg_txd,  input  wire dbg_rxd,
    output wire wifi_rst, output wire sens_pwr, output wire buz,
    input  wire btn_n,    output wire led
);
    wire [15:0] gpio_in, gpio_out, gpio_oe;
    wire        pclk, prst, penable, pwrite, psel1, pready1, pslverr1, irq;
    wire [7:0]  paddr;
    wire [31:0] pwdata, prdata1;
    wire [3:0]  pstrb;
    wire [2:0]  pprot;

    Gowin_EMPU_Top u_empu (
        .sys_clk(clk27), .reset_n(rst_n),
        .gpioin(gpio_in), .gpioout(gpio_out), .gpioouten(gpio_oe),
        .uart0_rxd(wifi_rxd), .uart0_txd(wifi_txd),
        .uart1_rxd(dbg_rxd), .uart1_txd(dbg_txd),
        .master_pclk(pclk), .master_prst(prst), .master_penable(penable), .master_paddr(paddr),
        .master_pwrite(pwrite), .master_pwdata(pwdata), .master_pstrb(pstrb), .master_pprot(pprot),
        .master_psel1(psel1), .master_prdata1(prdata1), .master_pready1(pready1), .master_pslverr1(pslverr1),
        .user_int_0(irq)
    );

    generate if (CNN) begin : g_cnn
        cnn_apb #(.WFILE0(WFILE0), .WFILE1(WFILE1), .WFILE2(WFILE2)) u_cnn (
            .pclk(pclk), .psel(psel1), .penable(penable), .pwrite(pwrite), .paddr(paddr), .pwdata(pwdata),
            .prdata(prdata1), .pready(pready1), .pslverr(pslverr1), .irq(irq)
        );
    end else begin : g_no_cnn
        assign prdata1 = 32'd0; assign pready1 = 1'b1; assign pslverr1 = 1'b0; assign irq = 1'b0;
    end endgenerate

    // ---- I²C mềm: ba trạng thái theo cho phép xuất ----
    assign imu_scl = gpio_oe[0] ? gpio_out[0] : 1'bz;
    assign imu_sda = gpio_oe[1] ? gpio_out[1] : 1'bz;
    assign gpio_in = {gpio_out[15:7], btn_n, gpio_out[5:3], imu_int, imu_sda, imu_scl};
    assign wifi_rst = gpio_out[3];
    assign sens_pwr = gpio_out[4];
    assign led      = gpio_out[7];

    // ---- còi: mức cao liên tục (còi chủ động) hoặc 2 kHz (còi thụ động), 27e6 / 2000 / 2 = 6.750 chu kỳ ----
    reg [12:0] tdiv = 13'd0;
    reg        tone = 1'b0;
    always @(posedge clk27) begin
        if (tdiv == 13'd6749) begin tdiv <= 13'd0; tone <= ~tone; end
        else tdiv <= tdiv + 13'd1;
    end
    assign buz = gpio_out[5] & (gpio_out[8] ? tone : 1'b1);
endmodule
