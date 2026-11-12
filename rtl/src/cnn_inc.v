// rtl/src/cnn_inc.v — bộ tăng tốc tính tăng dần cho N2 16-16-32 + Dense 512 → NOUT (kế hoạch O1, O2, O3). Verilog-2001.
// Mô hình địa chỉ (golden): ml/src/golden_inc.py, IncEngine — từng địa chỉ ROM / đệm ở đây giống hệt ở đó.
//
// Mỗi bước trượt (16 mẫu mới) chỉ tính:  L1 j ∈ {0, 55…63},  L2 m ∈ {0, 1, 26…31},  L3 r ∈ {0, 1, 12…15},  Dense đủ 512.
//   for vị trí: for g < OC: for q < 2: acc = bias[g]; for k < 5: for ic < IC: acc += W[(g·5 + k)·IC + ic] · SRC(2p + q − 2 + k, ic)
//                best = max qua q;  ĐỆM[vị trí, g] = requant(best)       (A.5: ≤ 0 → 0, min(>> n, 127))
// Bộ nhớ:
//   ROM (O2), 1 byte/từ: trọng số 1 làn như E2, bias INT32 4 byte little-endian ở BIAS_BASE + 4·b (lớp 1, 2, 3, Dense).
//   Đệm đặc trưng 2.048 × 8 (1 SDPB), vòng theo bộ đếm bước s (0…7):
//       L1 ((j + 8s) mod 64)·16 + c,   L2 1024 + ((m + 4s) mod 32)·16 + c,   L3 1536 + ((r + 2s) mod 16)·32 + c
//   STAGE 144 byte: x[0..3] rồi x[108..127] của cửa sổ, i = mẫu·6 + kênh. Nằm ngay trong đệm, ở 6 ô L3 sẽ tính lại
//       trong bước này: 1536 + ((12 + 2s + i/32) mod 16)·32 + (i mod 32). Các ô này đã "chết" sau Dense của bước trước
//       và chỉ bị ghi đè khi tính lớp 3, sau khi lớp 1 đọc xong STAGE (GW1NS-4 không có RAM phân tán).
//       Cổng APB ghi STAGE qua stg_we/stg_wa/stg_wd khi bộ tăng tốc rảnh (dùng chung cổng ghi của SDPB).
//   ROM tách tường minh 3 bank 2.048 × 8 (để Gowin xếp đúng 3 pROM; để một mảng 5.608 × 8 thì ra 4 pROM).
// Pipeline MAC như E2: E0 phát địa chỉ → E1 BSRAM trả dữ liệu → E2 thanh ghi tích → E3 cộng dồn. 1 làn.
// Kết quả hợp lệ khi warm = 8 (8 bước kể từ srst). srst giữ nguyên đệm (không cần xóa: chứng minh ở golden_inc).
// Khi rảnh, ROM đọc theo rom_ext_addr (kế hoạch O7: M3 đọc trọng số qua cổng APB), dữ liệu ở rom_q sau 2 chu kỳ.
module cnn_inc #(
    parameter NOUT        = 2,
    parameter SCORE_SHIFT = 9,
    parameter L1_SH = 8, parameter L2_SH = 8, parameter L3_SH = 8,
    parameter L1_BASE = 0, parameter L2_BASE = 480, parameter L3_BASE = 1760,
    parameter DENSE_BASE = 4320,
    parameter BIAS_BASE  = 5344,
    parameter ROM_WORDS  = 5608,
    parameter WFILE0     = "rom_inc_0.mem",     // byte 0…2047
    parameter WFILE1     = "rom_inc_1.mem",     // byte 2048…4095
    parameter WFILE2     = "rom_inc_2.mem"      // byte 4096…ROM_WORDS−1
) (
    input  wire               clk,
    input  wire               srst,             // xóa warm, s; dừng bước đang chạy
    input  wire               start,
    input  wire               stg_we,           // ghi 1 byte STAGE (chỉ khi rảnh)
    input  wire [7:0]         stg_wa,
    input  wire [7:0]         stg_wd,
    output reg                busy = 1'b0,
    output reg                done = 1'b0,      // xung 1 chu kỳ
    output reg  [1:0]         class_id = 2'd0,
    output reg  [6:0]         score = 7'd0,
    output reg  [3:0]         warm = 4'd0,
    output reg  [23:0]        cycles = 24'd0,
    input  wire [1:0]         logit_sel,
    output wire signed [31:0] logit,
    input  wire [12:0]        rom_ext_addr,
    output wire [7:0]         rom_q
);
    // ---------------- ROM trọng số + bias (3 pROM 2.048 × 8) ----------------
    reg [7:0]  wrom0 [0:2047];
    reg [7:0]  wrom1 [0:2047];
    reg [7:0]  wrom2 [0:2047];
    initial begin
        $readmemh(WFILE0, wrom0);
        $readmemh(WFILE1, wrom1);
        $readmemh(WFILE2, wrom2, 0, ROM_WORDS - 4097);
    end
    reg [12:0] w_addr = 13'd0;
    reg [7:0]  q0 = 8'd0, q1 = 8'd0, q2 = 8'd0;
    reg [1:0]  wbank = 2'd0;
    always @(posedge clk) begin
        q0 <= wrom0[w_addr[10:0]];
        q1 <= wrom1[w_addr[10:0]];
        q2 <= wrom2[w_addr[10:0]];
        wbank <= w_addr[12:11];
    end
    wire [7:0] w_q = (wbank == 2'd0) ? q0 : (wbank == 2'd1) ? q1 : q2;
    assign rom_q = w_q;

    // ---------------- đệm đặc trưng (SDPB 2.048 × 8) ----------------
    reg [7:0]  fbuf [0:2047];
    reg [10:0] rd_addr = 11'd0, wr_addr = 11'd0;
    reg [7:0]  wr_data = 8'd0;
    reg        we = 1'b0;
    reg [7:0]  fb_q = 8'd0;
    always @(posedge clk) begin
        if (we) fbuf[wr_addr] <= wr_data;
        fb_q <= fbuf[rd_addr];
    end

    // ---------------- lớp đang chạy ----------------
    // layer 0..2 = Conv 1..3, 3 = Dense
    reg [1:0]  layer = 2'd0;
    reg [3:0]  pi = 4'd0;                // chỉ số vị trí trong tập cần tính của lớp
    reg [5:0]  g = 6'd0;                 // kênh ra (Conv) hoặc lớp đầu ra (Dense)
    reg        q = 1'b0;
    reg [2:0]  s = 3'd0;                 // bộ đếm bước, quyết định gốc vòng
    reg [5:0]  OC, IC;
    reg [3:0]  NP;
    reg [4:0]  SH;
    reg [12:0] WBASE;
    reg [6:0]  BB;                       // chỉ số bias đầu của lớp
    always @(*) begin
        case (layer)
        2'd0:    begin OC = 6'd16; IC = 6'd6;  NP = 4'd10; SH = L1_SH; WBASE = L1_BASE;    BB = 7'd0;  end
        2'd1:    begin OC = 6'd16; IC = 6'd16; NP = 4'd8;  SH = L2_SH; WBASE = L2_BASE;    BB = 7'd16; end
        2'd2:    begin OC = 6'd32; IC = 6'd16; NP = 4'd6;  SH = L3_SH; WBASE = L3_BASE;    BB = 7'd32; end
        default: begin OC = NOUT;  IC = 6'd0;  NP = 4'd1;  SH = 5'd0;  WBASE = DENSE_BASE; BB = 7'd64; end
        endcase
    end
    // vị trí logic sau gộp của phần tử pi: L1 {0, 55..63}, L2 {0, 1, 26..31}, L3 {0, 1, 12..15}
    wire [5:0] p_l1 = (pi == 4'd0) ? 6'd0 : 6'd54 + {2'd0, pi};
    wire [5:0] p_l2 = (pi < 4'd2) ? {2'd0, pi} : 6'd24 + {2'd0, pi};
    wire [5:0] p_l3 = (pi < 4'd2) ? {2'd0, pi} : 6'd10 + {2'd0, pi};
    wire [5:0] p_cur = (layer == 2'd0) ? p_l1 : (layer == 2'd1) ? p_l2 : p_l3;
    // địa chỉ ghi đầu ra của (p_cur, g)
    wire [5:0] ph1w = p_l1 + {s, 3'b000};
    wire [4:0] ph2w = p_l2[4:0] + {s, 2'b00};
    wire [3:0] ph3w = p_l3[3:0] + {s, 1'b0};
    wire [10:0] dst = (layer == 2'd0) ? {1'b0, ph1w, g[3:0]} :
                      (layer == 2'd1) ? {2'b10, ph2w, g[3:0]} : {2'b11, ph3w, g[4:0]};

    // ---------------- con trỏ MAC ----------------
    reg [2:0]  k = 3'd0;
    reg [5:0]  ic = 6'd0;
    reg [8:0]  fi = 9'd0;                // chỉ số đặc trưng Dense (t·32 + c)
    reg signed [8:0] pos = 9'sd0;        // vị trí nguồn t − 2 + k trong lớp nguồn
    reg signed [9:0] sptr = 10'sd0;      // lớp 1: (pos − off)·6 + ic trong STAGE
    // địa chỉ đọc nguồn từ đệm
    wire [5:0] pr1 = pos[5:0] + {s, 3'b000};
    wire [4:0] pr2 = pos[4:0] + {s, 2'b00};
    wire [3:0] pr3 = fi[8:5] + {s, 1'b0};
    wire [3:0] prs = 4'd12 + {s, 1'b0} + {1'b0, sptr[7:5]};
    wire [10:0] src = (layer == 2'd0) ? {2'b11, prs, sptr[4:0]} :
                      (layer == 2'd1) ? {1'b0, pr1, ic[3:0]} :
                      (layer == 2'd2) ? {2'b10, pr2, ic[3:0]} : {2'b11, pr3, fi[4:0]};
    wire [3:0] pws = 4'd12 + {s, 1'b0} + {1'b0, stg_wa[7:5]};
    wire [8:0] L_src = (layer == 2'd0) ? 9'd128 : (layer == 2'd1) ? 9'd64 : 9'd32;
    wire       z_now = (pos < 0) || (pos >= $signed(L_src));
    // điểm bắt đầu của một đầu ra Conv: t = 2p + q, pos0 = t − 2
    wire signed [8:0] pos0 = $signed({2'b00, p_cur, q}) - 9'sd2;
    wire signed [9:0] off0 = (layer == 2'd0 && p_l1 != 6'd0) ? 10'sd104 : 10'sd0;
    wire signed [9:0] rel0 = $signed({pos0[8], pos0}) - off0;
    wire signed [9:0] sptr0 = (rel0 <<< 2) + (rel0 <<< 1);                          // ·6
    wire [12:0] g5ic    = g * IC * 13'd5;                                            // ≤ 31·16·5 = 2.480
    wire [12:0] wbase_g = (layer == 2'd3) ? (WBASE + {g[3:0], 9'd0}) : (WBASE + g5ic);
    wire [12:0] baddr = BIAS_BASE + {(BB + {1'b0, g}), 2'b00};

    // ---------------- pipeline MAC ----------------
    reg        v0 = 1'b0, v1 = 1'b0, v2 = 1'b0;
    reg        z0 = 1'b0, z1 = 1'b0;
    reg signed [15:0] prod = 16'sd0;
    reg signed [31:0] acc = 32'sd0;
    reg signed [31:0] best = 32'sd0;
    reg signed [31:0] bias_r = 32'sd0;
    reg signed [31:0] lg [0:3];
    wire signed [7:0] x_q = fb_q;
    assign logit = lg[logit_sel];

    function [7:0] requant;
        input signed [31:0] v;
        input [4:0] n;
        reg signed [31:0] t;
        begin
            t = v >>> n;
            requant = (v <= 0) ? 8'd0 : ((t > 127) ? 8'd127 : t[7:0]);
        end
    endfunction

    // ---------------- điều khiển ----------------
    localparam S_IDLE = 3'd0, S_BIAS = 3'd1, S_SETUP = 3'd2, S_MAC = 3'd3, S_DRAIN = 3'd4, S_WR = 3'd5,
               S_ARG = 3'd6, S_DONE = 3'd7;
    reg [2:0]  st = S_IDLE;
    reg [2:0]  bc = 3'd0;                // đếm byte bias / chu kỳ xả
    reg [1:0]  ai = 2'd0;                // argmax
    reg signed [31:0] top1 = 32'sd0, top2 = 32'sd0;
    reg        has2 = 1'b0;
    wire       mac_last = (layer == 2'd3) ? (fi == 9'd511) : ((k == 3'd4) && (ic == IC - 1));
    wire signed [32:0] margin = {top1[31], top1} - {top2[31], top2};
    wire signed [32:0] msh = margin >>> SCORE_SHIFT;
    integer i;
    initial for (i = 0; i < 4; i = i + 1) lg[i] = 32'sd0;

    always @(posedge clk) begin
        done <= 1'b0;
        we <= 1'b0;
        if (busy && cycles != 24'hFFFFFF) cycles <= cycles + 24'd1;
        // E1 → E2 → E3
        v1 <= v0; z1 <= z0;
        v2 <= v1;
        prod <= $signed(w_q) * (z1 ? 8'sd0 : x_q);
        if (v2) acc <= acc + prod;
        v0 <= 1'b0;

        if (srst) begin
            busy <= 1'b0; warm <= 4'd0; s <= 3'd0; st <= S_IDLE;
        end else case (st)
        S_IDLE: begin
            w_addr <= rom_ext_addr;
            if (stg_we) begin wr_addr <= {2'b11, pws, stg_wa[4:0]}; wr_data <= stg_wd; we <= 1'b1; end
            if (start) begin
                busy <= 1'b1; cycles <= 24'd1;
                layer <= 2'd0; pi <= 4'd0; g <= 6'd0; q <= 1'b0; bc <= 3'd0;
                st <= S_BIAS;
            end
        end
        // ---- bias INT32 = 4 byte liên tiếp trong ROM: phát địa chỉ ở bc 0..3, nhận byte ở bc 2..5 ----
        S_BIAS: begin
            if (bc < 3'd4) w_addr <= baddr + {11'd0, bc[1:0]};
            if (bc >= 3'd2) bias_r <= {w_q, bias_r[31:8]};
            bc <= bc + 3'd1;
            if (bc == 3'd5) st <= S_SETUP;
        end
        // ---- nạp bias, đặt con trỏ cho một đầu ra ----
        S_SETUP: begin
            acc <= bias_r;
            k <= 3'd0; ic <= 6'd0; fi <= 9'd0;
            pos <= pos0;
            sptr <= sptr0;
            w_addr <= wbase_g;
            st <= S_MAC;
        end
        // ---- một MAC mỗi chu kỳ: phát địa chỉ trọng số và nguồn ----
        S_MAC: begin
            v0 <= 1'b1;
            if (!(k == 3'd0 && ic == 6'd0 && fi == 9'd0)) w_addr <= w_addr + 13'd1;
            if (layer == 2'd3) begin
                z0 <= 1'b0;
                rd_addr <= src;
                fi <= fi + 9'd1;
            end else begin
                z0 <= z_now;
                rd_addr <= src;
                sptr <= sptr + 10'sd1;
                if (ic == IC - 1) begin ic <= 6'd0; k <= k + 3'd1; pos <= pos + 9'sd1; end
                else ic <= ic + 6'd1;
            end
            if (mac_last) begin st <= S_DRAIN; bc <= 3'd0; end
        end
        // ---- phần tử cuối tới bộ cộng sau 3 cạnh ----
        S_DRAIN: begin
            bc <= bc + 3'd1;
            if (bc == 3'd3) begin
                if (layer == 2'd3) begin
                    lg[g[1:0]] <= acc;
                    if (g == NOUT - 1) begin ai <= 2'd0; st <= S_ARG; end
                    else begin g <= g + 6'd1; bc <= 3'd0; st <= S_BIAS; end
                end else if (!q) begin
                    best <= acc; q <= 1'b1; st <= S_SETUP;        // q = 1 dùng lại bias_r
                end else begin
                    if (acc > best) best <= acc;
                    q <= 1'b0; st <= S_WR;
                end
            end
        end
        // ---- ghi một đầu ra đã requant vào đệm ----
        S_WR: begin
            wr_addr <= dst;
            wr_data <= requant(best, SH);
            we <= 1'b1;
            bc <= 3'd0;
            st <= S_BIAS;
            if (g == OC - 1) begin
                g <= 6'd0;
                if (pi == NP - 1) begin
                    pi <= 4'd0;
                    layer <= layer + 2'd1;
                end else pi <= pi + 4'd1;
            end else g <= g + 6'd1;
        end
        // ---- argmax tuần tự (bằng nhau → chỉ số nhỏ), top2 cho SCORE ----
        S_ARG: begin
            if (ai == 2'd0) begin
                top1 <= lg[0]; class_id <= 2'd0; has2 <= 1'b0;
            end else if (lg[ai] > top1) begin
                top2 <= top1; top1 <= lg[ai]; class_id <= ai; has2 <= 1'b1;
            end else if (!has2 || lg[ai] > top2) begin
                top2 <= lg[ai]; has2 <= 1'b1;
            end
            if (ai == NOUT - 1) st <= S_DONE;
            else ai <= ai + 2'd1;
        end
        S_DONE: begin
            score <= (msh > 33'sd127) ? 7'd127 : msh[6:0];
            warm <= (warm == 4'd8) ? 4'd8 : warm + 4'd1;
            s <= s + 3'd1;
            busy <= 1'b0;
            done <= 1'b1;
            st <= S_IDLE;
        end
        default: st <= S_IDLE;
        endcase
    end
endmodule
