// BẢN SAO của rtl/src (rtl/scripts/dong_bo_gowin.sh) — KHÔNG SỬA Ở ĐÂY: sửa rtl/src rồi chạy lại script.
// rtl/src/cnn_apb.v — cổng APB2 (Gowin_EMPU, APB2 Master 1, 0x40002400, 256 B) cho bộ tăng tốc tăng dần (kế hoạch O4).
// Verilog-2001. Một miền xung nhịp: pclk là xung nhịp của APB2 (O5: chung thạch anh 27 MHz với M3).
//
// Offset  Tên          R/W  Nội dung
// 0x00    ID           R    `CNN_INC_ID ("NCI1"); M3 kiểm khi khởi động
// 0x04    CTRL         W    bit0 START (xử lý 144 byte đã nạp), bit1 SOFT_RST (warm = 0, con trỏ NEW_DATA = 0)
// 0x08    STATUS       R    bit0 BUSY, bit1 DONE (đọc thì xóa), bit2 WR_REJECT (đọc thì xóa), bit3 VALID (WARM = 8)
// 0x0C    CLASS        R    lớp argmax: 0 NORMAL, 1 FALL (firmware đổi 1 → 3 = CLS_FALL)
// 0x10    SCORE        R    clamp((top1 − top2) >> SCORE_SHIFT, 0, 127)
// 0x14+4n LOGITn       R    INT32, n < NOUT (0x14 NORMAL, 0x18 FALL)
// 0x24    CYCLES       R    số chu kỳ của bước vừa xử lý
// 0x28    NEW_DATA     W    1 byte (bit 7:0), con trỏ trong tự tăng; đủ 144 byte rồi mới START được:
//                           byte i = mẫu·6 + kênh, mẫu 0..3 = x[0..3], mẫu 4..23 = x[108..127] của cửa sổ mới
// 0x2C    WARM         R    số bước đã có trong đệm (0…8); kết quả chỉ hợp lệ khi = 8
// 0x30    WROM_ADDR    R/W  (O7) địa chỉ ROM trọng số
// 0x34    WROM_DATA    R    (O7) 1 byte ROM (có dấu, mở rộng 32 bit), rồi WROM_ADDR tự tăng; có 2 chu kỳ chờ
// 0x38    NEW_PTR      R    số byte NEW_DATA đã nạp (0…144)
// Ghi bị từ chối (đặt WR_REJECT, không làm gì): NEW_DATA hay START khi BUSY; NEW_DATA khi đã đủ 144 byte;
// START khi chưa đủ 144 byte; đọc WROM_DATA khi BUSY (trả 0).
// irq (USER_INT0) = DONE: mức cao tới khi M3 đọc STATUS.
`include "cnn_inc_params.vh"
module cnn_apb #(
    parameter WFILE0 = "rom_inc_0.mem",
    parameter WFILE1 = "rom_inc_1.mem",
    parameter WFILE2 = "rom_inc_2.mem"
) (
    input  wire        pclk,
    input  wire        psel,
    input  wire        penable,
    input  wire        pwrite,
    input  wire [7:0]  paddr,
    input  wire [31:0] pwdata,
    output reg  [31:0] prdata,
    output wire        pready,
    output wire        pslverr,
    output wire        irq
);
    localparam NSTG = 144;
    // STAGE (144 byte mẫu mới) nằm trong đệm đặc trưng của cnn_inc; ở đây chỉ có con trỏ ghi
    reg  [7:0] ptr = 8'd0;
    reg        stg_we = 1'b0;
    reg  [7:0] stg_wa = 8'd0, stg_wd = 8'd0;

    // ---------------- bộ tăng tốc ----------------
    reg         start = 1'b0, srst = 1'b0;
    wire        busy, done;
    wire [1:0]  cls;
    wire [6:0]  score;
    wire [3:0]  warm;
    wire [23:0] cycles;
    wire signed [31:0] logit;
    reg  [12:0] rom_ea = 13'd0;
    wire [7:0]  rom_q;
    cnn_inc #(.NOUT(`CNN_INC_NOUT), .SCORE_SHIFT(`CNN_INC_SCORE_SHIFT),
              .L1_SH(`CNN_INC_L1_SH), .L2_SH(`CNN_INC_L2_SH), .L3_SH(`CNN_INC_L3_SH),
              .L1_BASE(`CNN_INC_L1_BASE), .L2_BASE(`CNN_INC_L2_BASE), .L3_BASE(`CNN_INC_L3_BASE),
              .DENSE_BASE(`CNN_INC_DENSE_BASE), .BIAS_BASE(`CNN_INC_BIAS_BASE),
              .ROM_WORDS(`CNN_INC_ROM_WORDS), .WFILE0(WFILE0), .WFILE1(WFILE1), .WFILE2(WFILE2))
        u_inc (.clk(pclk), .srst(srst), .start(start), .stg_we(stg_we), .stg_wa(stg_wa), .stg_wd(stg_wd), .busy(busy),
               .done(done), .class_id(cls), .score(score), .warm(warm), .cycles(cycles),
               .logit_sel(paddr[3:2] - 2'd1), .logit(logit), .rom_ext_addr(rom_ea), .rom_q(rom_q));

    // ---------------- APB ----------------
    reg        done_f = 1'b0, rej_f = 1'b0;
    reg  [1:0] wwait = 2'd0;
    wire       acc_ph = psel & penable;
    wire [5:0] wa = paddr[7:2];
    wire       rd_wrom = acc_ph & ~pwrite & (wa == 6'h0D);
    assign pready  = ~rd_wrom | (wwait == 2'd2);
    assign pslverr = 1'b0;
    assign irq     = done_f;
    wire       xfer = acc_ph & pready;

    always @(*) begin
        case (wa)
        6'h00:   prdata = `CNN_INC_ID;
        6'h02:   prdata = {28'd0, warm == 4'd8, rej_f, done_f, busy};
        6'h03:   prdata = {30'd0, cls};
        6'h04:   prdata = {25'd0, score};
        6'h05, 6'h06, 6'h07, 6'h08:
                 prdata = (wa - 6'h05 < `CNN_INC_NOUT) ? logit : 32'd0;
        6'h09:   prdata = {8'd0, cycles};
        6'h0B:   prdata = {28'd0, warm};
        6'h0C:   prdata = {19'd0, rom_ea};
        6'h0D:   prdata = busy ? 32'd0 : {{24{rom_q[7]}}, rom_q};
        6'h0E:   prdata = {24'd0, ptr};
        default: prdata = 32'd0;
        endcase
    end

    always @(posedge pclk) begin
        start <= 1'b0;
        srst <= 1'b0;
        stg_we <= 1'b0;
        if (rd_wrom && wwait != 2'd2) wwait <= wwait + 2'd1;
        else wwait <= 2'd0;
        if (xfer && pwrite) begin
            case (wa)
            6'h01: begin                                       // CTRL
                if (pwdata[1]) begin
                    srst <= 1'b1; ptr <= 8'd0; done_f <= 1'b0;
                end else if (pwdata[0]) begin
                    if (busy || ptr != NSTG) rej_f <= 1'b1;
                    else begin start <= 1'b1; ptr <= 8'd0; done_f <= 1'b0; end
                end
            end
            6'h0A: begin                                       // NEW_DATA
                if (busy || ptr == NSTG) rej_f <= 1'b1;
                else begin stg_we <= 1'b1; stg_wa <= ptr; stg_wd <= pwdata[7:0]; ptr <= ptr + 8'd1; end
            end
            6'h0C: rom_ea <= pwdata[12:0];                     // WROM_ADDR
            default: ;
            endcase
        end
        if (xfer && !pwrite) begin
            if (wa == 6'h02) begin                             // STATUS: đọc thì xóa DONE, WR_REJECT
                if (!done) done_f <= 1'b0;
                rej_f <= 1'b0;
            end
            if (wa == 6'h0D) begin
                if (busy) rej_f <= 1'b1;
                else rom_ea <= rom_ea + 13'd1;
            end
        end
        if (done) done_f <= 1'b1;                              // DONE thắng mọi lệnh xóa cùng chu kỳ
    end
endmodule
