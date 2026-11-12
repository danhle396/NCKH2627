# Firmware M3: biên dịch + chạy mã máy Cortex-M3 trong giả lập (mục 10 bước 5, 6, 8 — phần không cần board) — 11/11/2026

Biên dịch: `bash fw_arm/build.sh` (Arm Compiler 6.24 của MDK-ARM Lite 5.43, MicroLIB, `-Os`; riêng `cnn_core.c` `-O2`). Giả lập: `bash fw_arm/build.sh test` rồi `python fw_arm/test/m3_test.py --n 100` (Unicorn 2.1.4, venv riêng). Nhãn: [giả lập] — **chưa chạy trên board**.

**Kích thước** [linker] (`kich_thuoc_*.txt`)

| Bản | Flash (32 KB) | RAM (8 KB) |
|---|---|---|
| Thường | **15,69 KB** (49 %) | **6,18 KB** (gồm ngăn xếp 1 KB, vòng đệm UART 3 KB) |
| Đối chứng (`DOI_CHUNG=1`: thêm CNN phần mềm + ROM 5,6 KB) | 24,99 KB | 7,68 KB (còn 0,3 KB) |

Trang web nén gzip: 4.028 B (gốc 8.951 B).

**Kiểm trên giả lập** (`m3_test.log`)

| Kiểm | Kết quả |
|---|---|
| CNN phần mềm trên M3 == golden E2 hai lớp (50 vector export_mem + 100 cửa sổ D1, ring cắt ở vị trí ngẫu nhiên) | **150/150** khớp bit lớp, SCORE, 2 logit |
| Số lệnh Cortex-M3 mỗi lần suy luận phần mềm | ~1,17 triệu lệnh (`-O2`) [đếm lệnh, chưa phải chu kỳ]; ước 45–65 ms ở 27 MHz [ước tính; đo bằng DWT trên board ở bước 8]. Bộ tăng tốc: 2,59 ms |
| `cnn_stage_from_window` == golden | 60/60 |
| `cnn_selftest_stage(0..7)` == luồng LFSR của `export_inc.py` | 8/8 |
| Đổi mã 1 → 3: [1, 1] và [1, 0, 1] vào SUSPECT; [0, 1, 0, 0, 1, 0] không; không đổi mã thì [1, 1, 1, 1] không bao giờ vào SUSPECT | đúng cả 4 |
| Tự gỡ cảm biến (`core/imu_guard.c`): khởi động cắt nguồn; mất INT > 50 ms → bậc 1 → 2 → 3 → 4 (MẤT CẢM BIẾN); 10 s sau thử lại và hồi phục; 6 trục đứng yên 1 s, 3 lần đọc lỗi → bậc 1 | đúng thứ tự hành động và thời điểm; đếm go_bus 3, reset_mem 1, cat_nguon 2, mat_cb 1 |
| WiFi AT + HTTP (`core/wifi_at.c`) với module ESP-01S giả: 5 lệnh khởi động, GET /data (JSON), GET / (gzip, 2 khúc CIPSEND, vòng TX đầy giữa chừng), POST /huy gọi hủy cảnh báo, module im lặng → quá hạn → đóng kết nối | PASS |

Chưa kiểm được ở đây (cần board, bước H1): thanh ghi GPIO/UART/WDT thật, I²C mềm với MPU6050 thật, lệnh AT với ESP-01S thật, trang web trên trình duyệt, thử gây treo cảm biến 20/20.
