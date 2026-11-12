# Việc không cần board, 12/11/2026 (kế hoạch 15.7)

Nhãn: [giả lập] (Unicorn, mã máy Cortex-M3 thật của firmware), [mô phỏng], [linker]. Mô hình trong ROM là mô hình THỬ ngẫu nhiên; mọi luồng/mẫu là tổng hợp — **sẽ thay bằng dữ liệu và số đo thật** (bước 7b, H1b; kế hoạch 15.6, 15.8).

| Kiểm | Kết quả | File |
|---|---|---|
| `rtl/scripts/test.sh` | 12/12 PASS | `test_sh_ket_qua.txt` |
| Firmware Keil (`fw_arm/keil/nckh_m3.uvprojx`) | 0 lỗi, 0 cảnh báo; Code 12.456 + RO 4.820 B, ZI 5.880 B | `keil_build_log.txt` |
| Firmware `fw_arm/build.sh` | flash 17,54 KB / 32 KB, RAM 5,75 KB / 8 KB | `kich_thuoc_ban_thuong.txt` |
| Luồng thô `GET /raw` (module ESP-01S giả) | đầu HTTP giữ kết nối, 700 B gửi đủ, không đóng kết nối, đóng → dừng ghi | `m3_test.log` |
| `raw_log` định dạng CSV | đúng từng ký tự | `m3_test.log` |
| Đường ống thiết bị (`app_on_raw` + `preprocess.c`) == `tools/windowing.py` | 5/5 luồng, cửa sổ trùng từng byte | `m3_test.log` |
| Tầng 2 bắp tay `core/nga_bt.c` == `ml/src/nga_bt.py` | 130/130 bước (ngưỡng thử ngẫu nhiên) | `m3_test.log` |
| `tools/chon_truc.py --tu-kiem` | 200/200 cách gắn module tìm đúng trục, offset ±2 LSB | `chon_truc_tu_kiem.log` |
| `tools/chon_nguong_tang2.py --tu-kiem` | quyết định theo khối == `nga_bt.py` 300/300 | `chon_nguong_tu_kiem.log` |
| Netlist sau đặt – nối dây (`rtl/scripts/gl_inc.sh --steps 58`), ROM mô hình thử | 52/52 bước hợp lệ khớp CLASS/SCORE/LOGIT; 3 pROM, 1 SDPB, 2 MULT; CYCLES 70.010; 1.666 s | `gl_58buoc_mo_hinh_thu.log` |
