# Hướng dẫn thử trên board (các bước còn lại của kế hoạch mục 10)

Ngày viết: 11/11/2026. Những gì làm được trên máy đã xong: S0, bước 0, 1a, 1b, 2, 3, 4; mã firmware cho bước 5, 6, 8 đã biên dịch và chạy đúng trên giả lập M3 (bằng chứng ở `docs/bao_cao/2026-11-11_*`). Phần dưới đây **cần board và linh kiện**. Làm xong mỗi bước thì gửi lại "Cập nhật: <bước> + log" để đánh dấu ở `PROJECT_NCKH.md`.

## 0. Chuẩn bị (trên máy)

```bash
bash rtl/scripts/test.sh                       # mọi kiểm không cần board: phải TẤT CẢ PASS
bash rtl/scripts/build_gowin.sh --sram 8       # bitstream: <thư mục tạm>/gowin/impl/pnr/nckh2627.fs
#   hoặc mở gowin/nckh2627.gprj bằng Gowin EDA → Run All → gowin/impl/pnr/nckh2627.fs
#   firmware bằng Keil: mở fw_arm/keil/nckh_m3.uvprojx → Build → fw_arm/keil/Objects/nckh_m3.axf, .hex
bash fw_arm/build.sh                           # firmware: fw_arm/build/nckh_m3.bin
DOI_CHUNG=1 bash fw_arm/build.sh               # bản đối chứng bước 8 (chạy thêm CNN phần mềm, log "W,…")
```

Nạp: Gowin Programmer, thiết bị GW1NSR-4C. Nạp bitstream `.fs`, rồi nạp firmware `.bin` vào flash nhúng của M3 (thao tác MCU / Firmware của Programmer) **[CẦN XÁC MINH: tên thao tác đúng trong Gowin Programmer 1.9.11 cho GW1NSR-4C; tài liệu IPUG932 mục 2.4 trỏ tới SUG502]**.

Lưu ý điện (kế hoạch 12.5.4): tháo camera khỏi P3; không cắm USB-C và nguồn pin cùng lúc; tín hiệu 3,3 V chỉ nối các chân bank 1 (39–46).

## H1. Bộ thử trên bàn (Tang Nano 4K + GY-521 + ESP-01S + USB–UART)

Nối dây theo bảng chân mục 12.5.2 (GY-521: SCL 44, SDA 46, INT 43; ESP-01S: RXD ← 39, TXD → 40, RST qua Q1 ← 18; USB–UART: RXD ← 41, TXD → 42; GND chung). Mở cổng USB–UART 115200 8N1 (`python tools/ghi_uart.py` hoặc phần mềm terminal).

| Kiểm | Nhìn thấy | Pass |
|---|---|---|
| Khởi động | dòng `TK,1,70010,vi_tri=bap_tay,mo_hinh=THU_NGAU_NHIEN` (mô hình thử: kết quả nhận dạng vô nghĩa cho tới bước 7) | tự kiểm bộ tăng tốc đạt; **CYCLES = 70.010** (khớp mô phỏng) → thời gian suy luận = 2,59 ms [board] |
| Đọc cảm biến | các dòng `S,<ms>,<bước>,…` mỗi 0,32 s; từ bước thứ 8 cột 4 = 1 (VALID) | 100 Hz không mất mẫu (đếm bước: 1 bước / 320 ms ± 1 %) |
| ESP-01S | điện thoại thấy WiFi `NCKH2627` (mật khẩu `nckh2627`, đổi trong `main.c`), mở `http://192.168.4.1/` | trang tải được, số liệu đổi mỗi 300 ms |
| Nút Hủy trên web | bấm khi còi đang kêu | Safety FSM về MONITOR |

Nếu không thấy `TK`: kiểm tốc độ UART, chân 41/42; nếu `TK,0`: chụp màn hình gửi lại (có thể ID sai → bitstream chưa nạp; hoặc kết quả khác → số liệu cần so). Nếu ESP-01S im lặng: gõ thử `AT+GMR`, `AT+UART_CUR?` qua USB–UART nối thẳng module, ghi phiên bản firmware AT.

Các điểm **[CẦN XÁC MINH]** lần đầu cấp điện: khóa watchdog `0x1ACCE551` (không reset vô cớ trong 1 phút); I²C mềm chạy với 2 µs nửa chu kỳ (đọc được WHO_AM_I); chân 10 (LED) sáng nháy; còi chủ động hay thụ động (`PIN_BUZ_2KHZ` trong `chan.h`).

## Bước 5 + H2. Thử gây treo cảm biến (kế hoạch 7.5.5, bắt buộc)

| Thử | Cách làm | Pass |
|---|---|---|
| Nguồn chập chờn | rút/cắm nhanh dây VCC của GY-521 (hoặc firmware cố tình tắt Q3 5–50 ms), 20 lần | **20/20 tự hồi phục**, không rút pin; web "Mất cảm biến (lần)", "Cảm biến treo (lần)" tăng đúng |
| Lỗi bus | chập SDA xuống GND 1 s, 20 lần | 20/20 hồi phục ở bậc 1 hoặc 2 |
| Thời gian cắt nguồn | thử `IG_PWR_OFF_MS` = 500 / 1000 / 2000 / 5000 trong `core/imu_guard.h` | chọn giá trị nhỏ nhất hồi phục 20/20 |

## Bước 6. Web lâu dài

Mở trang 8 giờ liên tục: không treo; rút ESP-01S khi đang chạy → còi vẫn kêu khi ngã (thử bằng cách lắc mạnh rồi để yên); cắm lại → web tự lên lại (module được reset qua RST).

## Bước 8. Đối chứng CNN phần mềm

Nạp bản `DOI_CHUNG=1`. Mỗi bước có thêm dòng `W,<chu kỳ>,<lớp>,<logit0>,<logit1>,KHOP|LECH|CHUA_HOP_LE`. Pass: mọi dòng hợp lệ là `KHOP`; ghi chu kỳ trung bình (dự kiến cỡ 1,2–1,8 triệu chu kỳ ≈ 45–65 ms [ước tính]) để so với 70.010 chu kỳ của bộ tăng tốc. Đo dòng của hai bản ở bước H3 để tính năng lượng mỗi lần suy luận.

## H3. Đo dòng (kế hoạch 12.2)

Đo dòng trung bình và đỉnh của: Tang Nano 4K (P6-21), ESP-01S (sau ổn áp riêng), GY-521 (sau Q3), còi. Ghi cả hai bản firmware (thường, đối chứng). Gửi bảng số đo để chọn pin, sạc, boost.

## Bước 7. Thu dữ liệu, trục, tầng 2, mô hình bắp tay

Làm theo `docs/quy_trinh_thu_du_lieu_bap_tay.md` (thu bằng `tools/thu_bap_tay.py`, chọn trục bằng `tools/chon_truc.py`, cắt cửa sổ bằng `tools/windowing.py`, chọn ngưỡng tầng 2 bằng `tools/chon_nguong_tang2.py`). Tóm tắt cũ dưới đây giữ để đối chiếu:

1. Lắp GY-521 lên băng bắp tay, ghi hướng X/Y in trên module; đứng thẳng tay buông 10 s → gửi log để chọn `TRUC_*`, `ACC_OFF_*` trong `app/vi_tri_deo.h` (cần thêm lệnh ghi dữ liệu thô vào firmware và viết công cụ chọn trục mới; `truc_cam_bien.py` của ngực đã xóa).
2. Thu dữ liệu bắp tay nhiều người theo `docs/quy_trinh_thu_du_lieu_bap_tay.md` (thu ngã qua WiFi, không dây nối).
3. Viết tầng 2 cho bắp tay (kế hoạch 7.2), chọn ngưỡng trên tập riêng, tách tập kiểm tra; bật `NGA_BAT = 1`; huấn luyện mô hình bắp tay (`ml/train_cnn.py --data …`), xuất bằng `tools/o3_dense_2_lop.py --params …` + `tools/export_inc.py`, chép vào `rtl/src/`, `fw_arm/src/gen/`, chạy lại `rtl/scripts/test.sh`. Từ đó UART in `mo_hinh=<tên mô hình bắp tay>`.

## H4. PCB

Theo kế hoạch 12.5 sau khi H2, H3 đạt. Trên PCB chạy lại: tự kiểm (`TK,1`), thử gây treo 20/20, đeo vung tay 30 phút.
