# NCKH 2026–2027: Phát hiện té ngã đeo bắp tay trên Tang Nano 4K

Thiết bị đeo bắp tay phát hiện té ngã. Cảm biến MPU6050 (GY-521) gửi dữ liệu cho lõi Cortex-M3 trong chip GW1NSR-LV4C. M3 tiền xử lý, gửi dữ liệu mới cho bộ tăng tốc CNN INT8 chạy trong phần FPGA của cùng chip, rồi điều khiển còi và trang web qua module WiFi ESP-01S.

- Kế hoạch: [docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md](docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md)
- Theo dõi tiến độ: [PROJECT_NCKH.md](PROJECT_NCKH.md)
- Đặc tả: [docs/spec_nckh.md](docs/spec_nckh.md)

## Thư mục

| Thư mục | Nội dung |
|---|---|
| `rtl/` | RTL Verilog (bộ tăng tốc, cổng APB), testbench, script kiểm |
| `gowin/` | Project Gowin `nckh2627.gprj` |
| `fw_arm/` | Firmware Cortex-M3, project Keil `keil/nckh_m3.uvprojx`, test giả lập |
| `ml/` | Huấn luyện mô hình, mô hình tham chiếu |
| `tools/` | Xuất trọng số, thu và xử lý dữ liệu |
| `data/` | Dữ liệu bắp tay tự thu (không lên git) |
| `docs/` | Kế hoạch, đặc tả, sơ đồ, báo cáo, datasheet |

## Mở project

- FPGA: Gowin EDA → mở `gowin/nckh2627.gprj` → Run All.
- Firmware: Keil uVision → mở `fw_arm/keil/nckh_m3.uvprojx` → Build.

## Kiểm tra

```bash
bash rtl/scripts/test.sh          # kiểm toàn bộ phần không cần board
bash rtl/scripts/build_gowin.sh   # tổng hợp, đặt – nối dây cả chip
bash fw_arm/build.sh              # build firmware
```

## Trạng thái

Thiết kế FPGA và firmware đã xong, kiểm bằng mô phỏng và giả lập. Còn chạy trên board và thu dữ liệu bắp tay. Chi tiết ở [PROJECT_NCKH.md](PROJECT_NCKH.md).

Nguồn gốc code từ bản dự thi 2026: [docs/tham_khao/nguon_goc_code.md](docs/tham_khao/nguon_goc_code.md).
