# RTL bộ tăng tốc tăng dần + cổng APB (kế hoạch O1–O4; mục 10 bước 1b, 2, 3) — 11/11/2026

RTL: `rtl/src/cnn_inc.v`, `rtl/src/cnn_apb.v`; testbench `rtl/sim/tb_inc.v` (giao dịch APB thật). Chạy trong `bash rtl/scripts/test.sh`. Nhãn: [mô phỏng], Icarus 0.9.7 `-g2001`.

| Kiểm (`tb_inc.log`) | Kết quả |
|---|---|
| Luồng lần thử ngã D1 #1441 (người 108, mã 906), 58 bước liên tiếp + SOFT_RST + 8 bước tự kiểm LFSR | **52/52 bước hợp lệ khớp CLASS, SCORE, LOGIT0..1 và cả l1, l2, l3 trong đệm** (Pass bước 1b, 2, 3) |
| Cú ngã trong luồng | bước 27–30 ra FALL (4 cửa sổ liên tiếp) |
| VALID = 0 ở 7 bước đầu sau SOFT_RST, = 1 từ bước 8; WARM đếm 1…8 | đúng ở mọi bước |
| Phép âm: START khi chưa đủ 144 byte; byte thứ 145; NEW_DATA, START, đọc WROM khi BUSY | đều bị từ chối; WR_REJECT đặt, xóa khi đọc STATUS; con trỏ không đổi |
| irq (USER_INT0) = DONE, tắt sau khi đọc STATUS | đúng ở mọi bước |
| O7: đọc 16 byte ROM qua WROM_ADDR/WROM_DATA (vùng trọng số, vùng bias) | khớp |
| **CYCLES mỗi bước** | **70.010 chu kỳ = 2,59 ms ở 27 MHz** (chỉ tiêu ≤ 6 ms; bắt buộc < 320 ms) |

Thử đột biến (testbench có bắt lỗi không): đổi 1 trọng số lớp 3 trong ROM → FAIL 118 lỗi; RTL bỏ vị trí L1 63 → FAIL 172 lỗi.

`test_sh_ket_qua.txt`: kết quả `rtl/scripts/test.sh` lần cuối (10/10 PASS). `export_inc_report.txt`: ROM 5.608 byte = 5.344 trọng số + 66 bias × 4 B, BIAS_BASE 5344, 3 khối 2.048 × 8.
