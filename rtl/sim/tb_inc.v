// rtl/sim/tb_inc.v — kiểm cnn_apb + cnn_inc qua giao dịch APB (kế hoạch O4, bước 3 của mục 10). Icarus 0.9.7 -g2001.
// Chạy trong thư mục của tools/export_inc.py (rom_inc.mem, cnn_inc_params.vh, tb_*.mem, tb_info.vh): rtl/scripts/test.sh.
//   1. ID; SOFT_RST; START khi chưa nạp đủ 144 byte → WR_REJECT, không chạy
//   2. luồng chính TB_NS bước: mỗi bước ghi 144 byte NEW_DATA + START; trong lúc bận thử ghi NEW_DATA và START → bị từ chối;
//      chờ irq; STATUS: DONE, VALID đúng (VALID = 0 ở 7 bước đầu); CLASS, SCORE, LOGIT0..1 khớp golden ở bước hợp lệ;
//      l1, l2, l3 trong đệm (đổi địa chỉ vòng về vị trí logic) khớp golden E2 ở bước hợp lệ
//   3. SOFT_RST rồi 8 bước tự kiểm LFSR: WARM tăng 1…8, kết quả bước 8 khớp
//   4. O7: đọc 16 byte ROM ở hai địa chỉ qua WROM_ADDR / WROM_DATA
`timescale 1ns/1ps
`include "cnn_inc_params.vh"
`include "tb_info.vh"
module tb_inc;
    localparam NS = `TB_NS, NSELF = `TB_NSELF, NT = `TB_NS + `TB_NSELF, NO = `CNN_INC_NOUT, NE = `CNN_INC_NOUT + 3;
    reg clk = 0;
    always #18.518 clk = ~clk;                    // 27 MHz

    reg         psel = 0, penable = 0, pwrite = 0;
    reg  [7:0]  paddr = 0;
    reg  [31:0] pwdata = 0;
    wire [31:0] prdata;
    wire        pready, pslverr, irq;
`ifdef GL
    // netlist sau đặt – nối dây của rtl/sim/gl_wrap.v: dữ liệu đọc được chốt ở cạnh hoàn tất, đọc ra qua 4 lần chọn byte
    reg  [1:0] bsel = 2'd0;
    wire [7:0] rbyte;
    gl_wrap dut (.pclk(clk), .psel(psel), .penable(penable), .pwrite(pwrite), .paddr(paddr[7:2]), .pwdata(pwdata[12:0]),
                 .bsel(bsel), .rbyte(rbyte), .pready(pready), .irq(irq));
    assign pslverr = 1'b0;
`else
    cnn_apb dut (.pclk(clk), .psel(psel), .penable(penable), .pwrite(pwrite), .paddr(paddr), .pwdata(pwdata),
                 .prdata(prdata), .pready(pready), .pslverr(pslverr), .irq(irq));
`endif

    reg [7:0]  vstg [0:NT*144-1];
    reg [31:0] vexp [0:NT*NE-1];
    reg [7:0]  vtr  [0:NT*2048-1];
    reg [7:0]  vrom [0:`CNN_INC_ROM_WORDS-1];

    task apb_wr;
        input [7:0] a;
        input [31:0] d;
        begin
            @(posedge clk); psel <= 1; pwrite <= 1; paddr <= a; pwdata <= d; penable <= 0;
            @(posedge clk); penable <= 1;
            @(posedge clk); while (!pready) @(posedge clk);
            psel <= 0; penable <= 0; pwrite <= 0;
        end
    endtask
    task apb_rd;
        input  [7:0]  a;
        output [31:0] d;
        begin
            @(posedge clk); psel <= 1; pwrite <= 0; paddr <= a; penable <= 0;
            @(posedge clk); penable <= 1;
            @(posedge clk); while (!pready) @(posedge clk);
`ifdef GL
            psel <= 0; penable <= 0;
            @(posedge clk); #1;
            bsel = 2'd0; #1 d[7:0]   = rbyte;
            bsel = 2'd1; #1 d[15:8]  = rbyte;
            bsel = 2'd2; #1 d[23:16] = rbyte;
            bsel = 2'd3; #1 d[31:24] = rbyte;
`else
            d = prdata;
            psel <= 0; penable <= 0;
`endif
        end
    endtask

    integer err, n, i, j, c, base, t0, cmin, cmax, lbad, nvalid, nlay, wcnt, nres, e0;
    reg [31:0] r, st;
    reg [2:0]  s;
    initial begin
        $readmemh("tb_stg.mem", vstg);
        $readmemh("tb_exp.mem", vexp);
        $readmemh("tb_trace.mem", vtr);
        $readmemh("rom_inc.mem", vrom);
        err = 0; cmin = 32'h7FFFFFFF; cmax = 0; nvalid = 0; nlay = 0; nres = 0;
        repeat (3) @(posedge clk);

        apb_rd(8'h00, r);
        if (r !== `CNN_INC_ID) begin err = err + 1; $display("FAIL ID %h", r); end
        apb_wr(8'h04, 32'h2);                                         // SOFT_RST
        apb_wr(8'h04, 32'h1);                                         // START khi chưa có dữ liệu
        apb_rd(8'h08, st);
        if (st[2] !== 1'b1 || st[0] !== 1'b0) begin err = err + 1; $display("FAIL START thiếu dữ liệu: STATUS %h", st); end
        apb_rd(8'h08, st);
        if (st[2] !== 1'b0) begin err = err + 1; $display("FAIL WR_REJECT không xóa khi đọc: %h", st); end

        s = 3'd0;
        for (n = 0; n < NT; n = n + 1) begin
            if (n == NS) begin apb_wr(8'h04, 32'h2); s = 3'd0; end     // SOFT_RST trước tự kiểm
            for (i = 0; i < 144; i = i + 1) apb_wr(8'h28, {24'd0, vstg[n*144 + i]});
            if (n == 0) begin                                         // byte thứ 145 → từ chối
                apb_wr(8'h28, 32'hAA);
                apb_rd(8'h08, st);
                if (st[2] !== 1'b1) begin err = err + 1; $display("FAIL byte 145 không bị từ chối"); end
                apb_rd(8'h38, r);
                if (r !== 144) begin err = err + 1; $display("FAIL NEW_PTR = %0d", r); end
            end
            apb_wr(8'h04, 32'h1);
            apb_rd(8'h08, st);
            if (st[0] !== 1'b1) begin err = err + 1; $display("FAIL bước %0d: không BUSY sau START", n); end
            if (n == 1) begin                                         // phép âm khi đang bận
                apb_wr(8'h28, 32'h55);
                apb_wr(8'h04, 32'h1);
                apb_rd(8'h34, r);
                apb_rd(8'h08, st);
                if (st[2] !== 1'b1 || st[0] !== 1'b1 || r !== 0) begin
                    err = err + 1; $display("FAIL ghi khi bận không bị từ chối: STATUS %h WROM %h", st, r); end
                apb_rd(8'h38, r);
                if (r !== 0) begin err = err + 1; $display("FAIL NEW_PTR đổi khi bận: %0d", r); end
            end
            t0 = 0;
            while (!irq && t0 < 400000) begin @(posedge clk); t0 = t0 + 1; end
            if (!irq) begin err = err + 1; $display("FAIL bước %0d: không có irq", n); end
            apb_rd(8'h08, st);
            if (st[1] !== 1'b1 || st[0] !== 1'b0) begin err = err + 1; $display("FAIL bước %0d: STATUS %h", n, st); end
            if (st[3] !== vexp[n*NE + NO + 2][0]) begin
                err = err + 1; $display("FAIL bước %0d: VALID %0d, mong đợi %0d", n, st[3], vexp[n*NE + NO + 2][0]); end
            @(posedge clk);
            if (irq) begin err = err + 1; $display("FAIL bước %0d: irq không tắt sau khi đọc STATUS", n); end
            apb_rd(8'h2C, r);
            if (r !== ((n >= NS ? n - NS : n) + 1 > 8 ? 8 : (n >= NS ? n - NS : n) + 1)) begin
                err = err + 1; $display("FAIL bước %0d: WARM %0d", n, r); end
            apb_rd(8'h24, r);
            if (r < cmin) cmin = r;
            if (r > cmax) cmax = r;
            if (vexp[n*NE + NO + 2][0]) begin
                nvalid = nvalid + 1;
                e0 = err;
                apb_rd(8'h0C, r);
                if (r !== vexp[n*NE + NO]) begin err = err + 1; $display("FAIL bước %0d: CLASS %0d/%0d", n, r, vexp[n*NE + NO]); end
                apb_rd(8'h10, r);
                if (r !== vexp[n*NE + NO + 1]) begin err = err + 1; $display("FAIL bước %0d: SCORE %0d/%0d", n, r, vexp[n*NE + NO + 1]); end
                for (j = 0; j < NO; j = j + 1) begin
                    apb_rd(8'h14 + 4*j, r);
                    if (r !== vexp[n*NE + j]) begin
                        err = err + 1; $display("FAIL bước %0d: LOGIT%0d %0d/%0d", n, j, $signed(r), $signed(vexp[n*NE + j])); end
                end
                if (err == e0) nres = nres + 1;
                // từng tầng: vị trí logic → địa chỉ vòng với s của bước này (netlist -DGL: không còn mảng fbuf)
`ifndef GL
                lbad = 0;
                for (j = 0; j < 64; j = j + 1) for (c = 0; c < 16; c = c + 1)
                    if (dut.u_inc.fbuf[((j + 8*s) % 64)*16 + c] !== vtr[n*2048 + j*16 + c]) lbad = lbad + 1;
                for (j = 0; j < 32; j = j + 1) for (c = 0; c < 16; c = c + 1)
                    if (dut.u_inc.fbuf[1024 + ((j + 4*s) % 32)*16 + c] !== vtr[n*2048 + 1024 + j*16 + c]) lbad = lbad + 1;
                for (j = 0; j < 16; j = j + 1) for (c = 0; c < 32; c = c + 1)
                    if (dut.u_inc.fbuf[1536 + ((j + 2*s) % 16)*32 + c] !== vtr[n*2048 + 1536 + j*32 + c]) lbad = lbad + 1;
                if (lbad) begin err = err + 1; $display("FAIL bước %0d: l1/l2/l3 sai %0d byte", n, lbad); end
                else nlay = nlay + 1;
`endif
            end
            s = s + 3'd1;
        end

        // O7: đọc ROM qua APB
        wcnt = 0;
        for (j = 0; j < 2; j = j + 1) begin
            base = j ? `CNN_INC_BIAS_BASE : 100;
            apb_wr(8'h30, base);
            for (i = 0; i < 8; i = i + 1) begin
                apb_rd(8'h34, r);
                if (r !== {{24{vrom[base + i][7]}}, vrom[base + i]}) wcnt = wcnt + 1;
            end
        end
        apb_rd(8'h30, r);
        if (wcnt || r !== `CNN_INC_BIAS_BASE + 8) begin err = err + 1; $display("FAIL WROM: sai %0d byte, ADDR %0d", wcnt, r); end

`ifdef GL
        $display("KET QUA: %0d buoc (%0d luong chinh + %0d tu kiem); %0d buoc hop le: %0d khop CLASS/SCORE/LOGIT (netlist: khong so l1/l2/l3)",
                 NT, NS, NSELF, nvalid, nres);
`else
        $display("KET QUA: %0d buoc (%0d luong chinh + %0d tu kiem); %0d buoc hop le: %0d khop CLASS/SCORE/LOGIT, %0d khop l1/l2/l3",
                 NT, NS, NSELF, nvalid, nres, nlay);
`endif
        $display("CYCLES/buoc = %0d..%0d (%0d.%02d ms o 27 MHz)", cmin, cmax, cmax / 27000, (cmax % 27000) / 270);
        if (err == 0) $display("PASS"); else $display("FAIL (%0d loi)", err);
        $finish;
    end
endmodule
