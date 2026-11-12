# Tổng hợp + đặt – nối dây trên GW1NSR-LV4CQN48PC6/I5 (kế hoạch S0 và mục 10 bước 4) — 11/11/2026

Lệnh: `bash rtl/scripts/build_gowin.sh [--sram 2|4|8|16] [--chi-empu]`. Gowin EDA V1.9.11.03 Education, `gw_sh`, không mở IDE. IP Gowin_EMPU lấy từ `ipcore/gowin_empu_gw1ns4` trên máy, cấu hình bằng `config.v` do script viết (GPIO không IOBUF, UART0, UART1, APB2 Master 1, USER_INT0). Nhãn: [P&R].

**S0: tài nguyên riêng của EMPU (chỉ M3, `--chi-empu`)**

| SRAM của M3 | BSRAM | Logic | Fmax (clk27) | File |
|---|---|---|---|---|
| 2 KB | 2 | 45 | 238,5 MHz | `pnr_s2_c0.rpt.txt` |
| 4 KB | 2 | 46 | 238,5 MHz | `pnr_s4_c0.rpt.txt` |
| **8 KB (chọn)** | **4** | 54 | 210,6 MHz | `pnr_s8_c0.rpt.txt` |
| 16 KB | 8 | 74 | 174,1 MHz | `pnr_s16_c0.rpt.txt` |

Logic gồm cả bộ chia 2 kHz của còi trong `top.v`. Bộ sinh IP ghi "2 KB dùng 1 BSRAM"; đặt – nối dây thật cho 2 khối. Với 16 KB thì M3 lấy 8/10 khối, không còn đủ cho bộ tăng tốc.

**Bước 4: thiết kế đầy đủ (EMPU 8 KB + cnn_apb + cnn_inc), một xung nhịp 27 MHz, không PLL** (`pnr_s8_c1.rpt.txt`, `syn_resource_s8_c1.html`)

| Tài nguyên | Dùng | Ngân sách kế hoạch mục 4 |
|---|---|---|
| Logic (LUT + ALU) | **1.076 / 4.608 (24 %)** | ~785–1.180 (17–26 %), chỉ tiêu ≤ 60 % |
| Thanh ghi | 514 / 3.573 (15 %) | ~720–920 |
| **BSRAM** | **8 / 10** = EMPU 4 + bộ tăng tốc 4 (3 pROM trọng số + bias, 1 SDPB đệm + STAGE) | 8 / 10 |
| DSP | 1,25 / 8 | ~5 / 16 bộ nhân |
| I/O | 14 / 39 | |
| **Fmax clk27** | **41,1 MHz ≥ 27 MHz** (`fmax_s8_c1.txt`) | ≥ thạch anh |

File `.cst` của mục 12.5.5 (`gowin/src/tang_nano_4k_nckh.cst`) không báo lỗi bank hay loại IO; chân 10 (MODE) dùng làm GPIO nhờ `-use_mode_as_gpio 1`. 14 cảnh báo (`canh_bao_s8_c1.txt`) nằm trong IP mã hóa của Gowin (cổng I²C/SPI không bật), cộng PR1014: xung nhịp 27 MHz vào chân 45, không phải chân GCLK, nên đi đường thường **[CẦN XÁC MINH trên board]**.

Hai lần sửa trước khi đạt:
1. STAGE để riêng bị tổng hợp thành 1.152 thanh ghi, vì GW1NS-4 không có RAM phân tán (thanh ghi lên 73 %). Đã chuyển STAGE vào SDPB đệm.
2. ROM 5.608 × 8 khai báo một mảng thì Gowin xếp thành 4 pROM. Đã tách tường minh 3 bank 2.048 × 8, còn 3 pROM.
