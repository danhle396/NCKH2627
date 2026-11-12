# NCKH 2026–2027: phát hiện té ngã trên Tang Nano 4K (Cortex-M3 + FPGA)

Thiết bị đeo bắp tay: MPU6050 (module GY-521) → lõi Cortex-M3 cứng của GW1NSR-LV4C (đọc cảm biến, tiền xử lý, máy trạng thái, web qua module WiFi ESP-01S) → bộ tăng tốc CNN INT8 tính tăng dần trong phần FPGA của cùng chip, nối qua APB2 → còi + trang web.

Kế hoạch và các quyết định đã chốt: [docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md](docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md). File theo dõi chính: [PROJECT_NCKH.md](PROJECT_NCKH.md).

## Cấu trúc

```
rtl/       src/ (RTL Verilog-2001: cnn_inc.v, cnn_apb.v; cnn_e2.v làm chuẩn đối chiếu)  sim/ (tb_inc.v, tb_e2.v, gl_wrap.v)
           scripts/ (test.sh, build_gowin.sh, gl_inc.sh; gl.sh, build.sh của bản dự thi giữ làm mẫu)
gowin/     nckh2627.gprj ← MỞ BẰNG GOWIN EDA, bấm Run All (GW1NSR-LV4CQN48PC6/I5); src/ top.v, .cst, .sdc,
           src/gowin_empu/ (IP Gowin_EMPU: SRAM 8 KB, GPIO, UART0/1, APB2 M1, USER_INT0);
           src/cnn_*.v, cnn_inc_params.vh, rom_inc_*.mem = BẢN SAO của rtl/src (sửa ở rtl/src rồi chạy
           rtl/scripts/dong_bo_gowin.sh; test.sh và build_gowin.sh báo lỗi nếu hai bản lệch)
fw_arm/    build.sh, gw1ns4c_8k.sct; src/core/ (C thuần, chạy cả trên giả lập), src/driver/ (thanh ghi theo IPUG931),
           src/app/ (main, app, web, chan.h, vi_tri_deo.h), src/gen/ (sinh từ tools/); test/ (giả lập M3 bằng Unicorn)
           keil/nckh_m3.uvprojx ← MỞ BẰNG KEIL uVision, Build (F7) xem Program Size (sinh bởi tools/gen_keil.py)
ml/        train_cnn.py, src/ (golden model khớp bit, luong_tong_hop.py), out/mo_hinh_thu*/ (mô hình THỬ ngẫu nhiên, không huấn luyện)
tools/     xuất trọng số, xử lý dữ liệu và log
data/      dữ liệu tự thu (file lớn để ngoài git)
docs/      de_xuat/  ke_hoach/  so_do/  bao_cao/  tham_khao/  datasheet/
```

## Kiểm nhanh

```bash
bash rtl/scripts/test.sh                 # mọi kiểm không cần board (10 mục): Icarus 0.9.7, Python có numpy;
                                         # firmware giả lập cần armclang (MDK-ARM) và PY_UNICORN=<python có unicorn>
bash rtl/scripts/build_gowin.sh --sram 8 # tổng hợp + đặt – nối dây cả chip bằng gw_sh (Gowin EDA 1.9.11.03)
bash rtl/scripts/gl_inc.sh               # mô phỏng netlist sau đặt – nối dây (~25 phút)
bash fw_arm/build.sh                     # firmware M3 → fw_arm/build/nckh_m3.bin
```

Kết quả 11/11/2026: `rtl/scripts/test.sh` 10/10 PASS; chi tiết và bằng chứng ở `docs/bao_cao/2026-11-11_*`, tóm tắt ở [PROJECT_NCKH.md](PROJECT_NCKH.md). Việc còn lại cần board: [docs/huong_dan_thu_tren_board.md](docs/huong_dan_thu_tren_board.md).

## Nguồn gốc code dùng lại

Code và tài liệu dưới đây chép nguyên văn (so sha256) từ bản dự thi 2026, repo `danhle396/fpga2026gowin`, **commit `f26bea8`**. Bản dự thi đã xóa khỏi máy; cần file nào khác thì lấy từ lịch sử git của repo cũ. Bằng chứng kế hoạch trích từ các file đã xóa: [docs/tham_khao/bang_chung_du_thi.md](docs/tham_khao/bang_chung_du_thi.md). Các log board, slide, sơ đồ chưa commit của bản dự thi được nén ở `D:\hoctap\fpga\luu_tru\luu_tru_du_thi_2026.zip` (ngoài repo).

Chỉnh sửa sau khi chép:
- `rtl/scripts/test.sh`: đổi đường dẫn `fpga/` → `rtl/`; bỏ phần `tb_proto` (SPI) và phần kiểm sketch ESP32; tự chọn Python có numpy. Viết lại 11/11 (đêm) để chạy cả chuỗi kiểm của bản mới.
- Lõi C của ESP32 (bảng dưới) **chuyển từ `fw_arm/src/port_tu_esp32/` sang `fw_arm/src/core/`** ngày 11/11 (đêm). Sửa: `safety.c` bỏ nhánh IMPACT (O3); `fw_config.h` cũ đổi tên `core/fw_config_du_thi.h`, thay bằng `core/fw_config.h` mới; `trang_web_html.h` giữ nguyên (bản nén sinh bởi `tools/gen_web_gz.py`). Các file khác giữ nguyên nội dung.
- `tools/export_mem.py`, `ml/src/rom_layout.py`: số lớp đầu ra lấy theo mô hình (O3), thêm `CNN_NOUT`; mô hình 4 lớp xuất ra giống hệt trước.
- `docs/tham_khao/spec_v3_du_thi.md`: đổi tên từ `docs/spec.md`, nội dung giữ nguyên.
- `docs/tham_khao/bang_chung_du_thi.md`, `README.md`, `PROJECT_NCKH.md`: viết mới.

Chưa chạy được ở cấu trúc mới (cần viết lại cho lõi M3):
- `tools/cosim.py`, `tools/nga_tren_log.py` import `m0_test.py` (giả lập Cortex-M0 chạy ảnh build SN32F407), không chép sang.
- `rtl/scripts/gl.sh`, `rtl/scripts/build.sh`: còn cho GW2A / Kiwi Primer 20K, giữ làm mẫu.
- `fw_arm/src/port_tu_esp32/` (lõi ESP32/SPI) và `fw_arm/src/tham_khao/` (driver SN32F407): **đã xóa 12/11/2026**, không nạp lên board, không còn dùng; `gyro_cal.*` chuyển sang `core/` trước khi xóa.

| Đích (repo này) | Nguồn (`fpga2026gowin` @ `f26bea8`) |
|---|---|
| `rtl/src/cnn_e2.v` | `fpga/src/cnn_e2.v` |
| `rtl/src/cnn_params.vh` | `fpga/src/cnn_params.vh` |
| `rtl/src/weights_e2.mem` | `fpga/src/weights_e2.mem` |
| `rtl/src/bias_e2.mem` | `fpga/src/bias_e2.mem` |
| `rtl/sim/tb_e2.v` | `fpga/sim/tb_e2.v` |
| `rtl/scripts/test.sh` | `fpga/scripts/test.sh` |
| ~~`rtl/scripts/gl.sh`~~ (đã xóa 11/11) | `fpga/scripts/gl.sh` |
| ~~`rtl/scripts/build.sh`~~ (đã xóa 11/11) | `fpga/scripts/build.sh` |
| `docs/tham_khao/fpga_project.rpt.txt` | `fpga/bitstream/fpga_project.rpt.txt` |
| `docs/tham_khao/fpga_project_syn_resource.html` | `fpga/impl/gwsynthesis/fpga_project_syn_resource.html` |
| `ml/train_cnn.py` | `ml/train_cnn.py` |
| `ml/src/cnn_numpy.py` | `ml/src/cnn_numpy.py` |
| `ml/src/golden_model.py` | `ml/src/golden_model.py` |
| `ml/src/rom_layout.py` | `ml/src/rom_layout.py` |
| `ml/src/safety_model.py` | `ml/src/safety_model.py` |
| ~~`ml/src/posture_model.py`~~ (đã xóa 11/11) | `ml/src/posture_model.py` |
| ~~`ml/data/public/` (6 file)~~ — **đã xóa 11/11/2026** (xem dưới) | `ml/data/public/` |
| ~~`ml/out/`~~ (đã xóa 11/11) (116 file) | `ml/out/` |
| `tools/export_mem.py` | `tools/export_mem.py` |
| ~~`tools/cosim.py`~~ (đã xóa 11/11) | `tools/cosim.py` |
| ~~`tools/convert_d1.py`~~ (đã xóa 11/11) | `tools/convert_d1.py` |
| `tools/windowing.py` | `tools/windowing.py` |
| `tools/logger.py` | `tools/logger.py` |
| `tools/ghi_uart.py` | `tools/ghi_uart.py` |
| ~~`tools/nga_tren_log.py`~~ (đã xóa 11/11) | `tools/nga_tren_log.py` |
| ~~`tools/playback_log.py`~~ (đã xóa 11/11) | `tools/playback_log.py` |
| ~~`tools/plot_d1.py`~~ (đã xóa 11/11) | `tools/plot_d1.py` |
| ~~`tools/truc_cam_bien.py`~~ (đã xóa 11/11) | `tools/truc_cam_bien.py` |
| ~~`tools/posture_eval.py`~~ (đã xóa 11/11) | `tools/posture_eval.py` |
| ~~`tools/fetch_d1.sh`~~ (đã xóa 11/11) | `tools/fetch_d1.sh` |
| `fw_arm/src/core/preprocess.c` | `esp32/nguc_nga_hub/preprocess.c` |
| `fw_arm/src/core/preprocess.h` | `esp32/nguc_nga_hub/preprocess.h` |
| `fw_arm/src/core/safety.c` | `esp32/nguc_nga_hub/safety.c` |
| `fw_arm/src/core/safety.h` | `esp32/nguc_nga_hub/safety.h` |
| ~~`fw_arm/src/core/nga.c`~~ (đã xóa 11/11) | `esp32/nguc_nga_hub/nga.c` |
| ~~`fw_arm/src/core/nga.h`~~ (đã xóa 11/11) | `esp32/nguc_nga_hub/nga.h` |
| ~~`fw_arm/src/core/posture.c`~~ (đã xóa 11/11) | `esp32/nguc_nga_hub/posture.c` |
| ~~`fw_arm/src/core/posture.h`~~ (đã xóa 11/11) | `esp32/nguc_nga_hub/posture.h` |
| `fw_arm/src/core/pwr_btn.c` | `esp32/nguc_nga_hub/pwr_btn.c` |
| `fw_arm/src/core/pwr_btn.h` | `esp32/nguc_nga_hub/pwr_btn.h` |
| ~~`fw_arm/src/port_tu_esp32/app_core.c`~~ (đã xóa 12/11) | `esp32/nguc_nga_hub/app_core.c` |
| ~~`fw_arm/src/port_tu_esp32/app_core.h`~~ (đã xóa 12/11) | `esp32/nguc_nga_hub/app_core.h` |
| `fw_arm/src/core/blocks.c` | `esp32/nguc_nga_hub/blocks.c` |
| `fw_arm/src/core/blocks.h` | `esp32/nguc_nga_hub/blocks.h` |
| ~~`fw_arm/src/port_tu_esp32/hub.c`~~ (đã xóa 12/11) | `esp32/nguc_nga_hub/hub.c` |
| ~~`fw_arm/src/port_tu_esp32/hub.h`~~ (đã xóa 12/11) | `esp32/nguc_nga_hub/hub.h` |
| ~~`fw_arm/src/core/fw_config_du_thi.h`~~ (đã xóa 11/11) | `esp32/nguc_nga_hub/fw_config.h` |
| ~~`fw_arm/src/port_tu_esp32/selftest_expect_v3_e2_n2.h`~~ (đã xóa 11/11) | `esp32/nguc_nga_hub/selftest_expect_v3_e2_n2.h` |
| `fw_arm/src/core/trang_web_html.h` | `esp32/nguc_nga_hub/trang_web_html.h` |
| ~~`fw_arm/src/tham_khao/board.c`~~ (đã xóa 12/11) | `firmware/src/board.c` |
| ~~`fw_arm/src/tham_khao/board.h`~~ (đã xóa 12/11) | `firmware/src/board.h` |
| ~~`fw_arm/src/tham_khao/gyro_cal.c`~~ (đã xóa 12/11) | `firmware/src/gyro_cal.c` |
| ~~`fw_arm/src/tham_khao/gyro_cal.h`~~ (đã xóa 12/11) | `firmware/src/gyro_cal.h` |
| ~~`fw_arm/src/tham_khao/imu.c`~~ (đã xóa 12/11) | `firmware/src/imu.c` |
| ~~`fw_arm/src/tham_khao/imu.h`~~ (đã xóa 12/11) | `firmware/src/imu.h` |
| ~~`fw_arm/src/tham_khao/i2c0.c`~~ (đã xóa 12/11) | `firmware/src/i2c0.c` |
| ~~`fw_arm/src/tham_khao/i2c0.h`~~ (đã xóa 12/11) | `firmware/src/i2c0.h` |
| `docs/tham_khao/spec_v3_du_thi.md` | `docs/spec.md` |
| `docs/quy_trinh_thu_du_lieu_bap_tay.md` | `docs/data_protocol.md` |
| `docs/datasheet/MPU-6000_MPU-6050_Product_Spec_Rev3.4.pdf` | `docs/datasheet/MPU-6000_MPU-6050_Product_Spec_Rev3.4.pdf` |
| `docs/datasheet/tang_nano_4k/` (5 file) | `docs/datasheet/tang_nano_4k/` |
| `docs/de_xuat/De_xuat_NCKH.docx` | `docs/De_xuat_NCKH.docx` |
| `docs/de_xuat/De_xuat_NCKH_ban_mo.docx` | `docs/De_xuat_NCKH_ban_mo.docx` |
| `docs/de_xuat/De_xuat_NCKH_ban_mo_THEO_DOI_SUA.docx` | `docs/De_xuat_NCKH_ban_mo_THEO_DOI_SUA.docx` |
| `docs/de_xuat/De_xuat_NCKH_ban_mo_v2.docx` | `docs/De_xuat_NCKH_ban_mo_v2.docx` |
| `docs/de_xuat/De_xuat_NCKH_ban_mo_v2_THEO_DOI_SUA.docx` | `docs/De_xuat_NCKH_ban_mo_v2_THEO_DOI_SUA.docx` |
| `docs/de_xuat/De_xuat_NCKH_ban_mo_v3.docx` | `docs/De_xuat_NCKH_ban_mo_v3.docx` |
| `docs/de_xuat/De_xuat_NCKH_ban_mo_v3_THEO_DOI_SUA.docx` | `docs/De_xuat_NCKH_ban_mo_v3_THEO_DOI_SUA.docx` |
| `docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md` | `docs/ke_hoach_toi_uu_tang_nano_4k_m3.md` |
| `docs/so_do/so_do_de_xuat_tang_nano_4k_m3_wifi.drawio` | `docs/so_do_de_xuat_tang_nano_4k_m3_wifi.drawio` |
| `docs/so_do/so_do_kien_truc_arm.drawio` | `docs/so_do_kien_truc_arm.drawio` |
| `docs/so_do/so_do_kien_truc_arm_trang1.png` | `docs/so_do_kien_truc_arm_trang1.png` |
| `docs/so_do/so_do_kien_truc_arm_trang2.png` | `docs/so_do_kien_truc_arm_trang2.png` |
| `docs/so_do/so_do_chan_tang_nano_4k.drawio` | `docs/so_do_chan_tang_nano_4k.drawio` |
| `docs/so_do/so_do_chan_tang_nano_4k_trang1.png` | `docs/so_do_chan_tang_nano_4k_trang1.png` |
| `docs/so_do/so_do_chan_tang_nano_4k_trang2.png` | `docs/so_do_chan_tang_nano_4k_trang2.png` |
| `docs/so_do/so_do_chan_tang_nano_4k_trang3.png` | `docs/so_do_chan_tang_nano_4k_trang3.png` |

## Viết mới cho NCKH2627 (11/11/2026)

| File | Nội dung |
|---|---|
| `tools/o3_dense_2_lop.py` → `ml/out/n2_11_d1_2lop/` | Mô hình 2 lớp (O3) và kiểm trùng lớp 4 lớp |
| `ml/src/golden_inc.py`, `tools/kiem_golden_inc.py` | Golden tăng dần (mô hình địa chỉ của RTL) và kiểm khớp bit với golden E2 |
| `tools/export_inc.py` → `rtl/src/rom_inc*.mem`, `cnn_inc_params.vh`, `fw_arm/src/gen/` | Xuất ROM (bias trong ROM), vector testbench, bảng tự kiểm, ROM cho CNN phần mềm |
| `rtl/src/cnn_inc.v`, `rtl/src/cnn_apb.v`, `rtl/sim/tb_inc.v`, `rtl/sim/gl_wrap.v` | Bộ tăng tốc tăng dần, cổng APB, testbench, vỏ bọc mô phỏng netlist |
| `gowin/src/*`, `rtl/scripts/build_gowin.sh`, `rtl/scripts/gl_inc.sh` | Top-level, ràng buộc chân, dựng bitstream và mô phỏng netlist bằng dòng lệnh |
| `fw_arm/` (trừ `port_tu_esp32/`, `tham_khao/`, các file lõi ở bảng trên) | Firmware M3 và bộ test giả lập |
| `tools/gen_web_gz.py` | Trang web nén gzip cho flash M3 |
| `docs/spec_nckh.md`, `docs/huong_dan_thu_tren_board.md`, `docs/bao_cao/2026-11-11_*` | SPEC mới, hướng dẫn thử trên board, bằng chứng |

## Dữ liệu (11/11/2026)

- **Bộ công khai D1 (kênh đầu, ngực, thắt lưng) đã xóa khỏi repo** theo quyết định của nhóm, để không lẫn kênh/vị trí như ở bản dự thi. Vẫn còn trong lịch sử git của repo cũ `danhle396/fpga2026gowin` (commit `ea27771`). Các công cụ riêng cho D1 (`tools/convert_d1.py`, `fetch_d1.sh`, `plot_d1.py`, `posture_eval.py`, `cosim.py`) không còn dữ liệu để chạy.
- **Dữ liệu bắp tay: nhóm tự thu 100 %**, đặt ở `data/` (file lớn ngoài git), cắt cửa sổ bằng `tools/windowing.py`, huấn luyện bằng `python ml/train_cnn.py --data <npz>` (bắt buộc chỉ đường dẫn).
- Các kiểm khớp bit dùng **luồng tổng hợp** (`ml/src/luong_tong_hop.py`), không dùng để huấn luyện hay đo độ chính xác.
- **Không còn mô hình hay hằng số nào sinh từ dữ liệu khác bắp tay** (xóa 11/11/2026, khuya): mô hình kênh đầu D1, vector kiểm thử chứa cửa sổ D1, cấu hình và ngưỡng ngực, tầng 2 và nhận dạng tư thế của ngực, công cụ D1 và log bản dự thi. ROM đang nạp **mô hình THỬ ngẫu nhiên** (`tools/tao_mo_hinh_thu.py` → `ml/out/mo_hinh_thu*/`), tên `THU_NGAU_NHIEN` in ở UART (`TK,…,mo_hinh=`) và web (`mh`): chỉ để kiểm khớp bit, kết quả nhận dạng vô nghĩa. Thay bằng mô hình bắp tay ở bước 7; RTL không đổi.
- Lịch sử git cũ (thư mục `.git` hiện tại thuộc repo `fpga2026gowin`) vẫn chứa D1: kế hoạch mục 14 cất `.git` cũ ra ngoài trước khi tạo repo mới, nên repo `NCKH2627` sẽ không có D1.
| `fw_arm/src/core/raw_log.*`, `nga_bt.*`, `gyro_cal.*` (chuyển từ `tham_khao/`) | Ghi dữ liệu thô, tầng 2 bắp tay, hiệu chỉnh con quay |
| `ml/src/vi_tri.py`, `ml/src/nga_bt.py` | Cấu hình vị trí đeo cho Python (đọc `vi_tri_deo.h`), tầng 2 khớp bit với C |
| `tools/thu_bap_tay.py`, `chon_truc.py`, `chon_nguong_tang2.py`, `windowing.py` (viết lại) | Thu, chọn trục, chọn ngưỡng, cắt cửa sổ cho dữ liệu bắp tay |
| `docs/quy_trinh_thu_du_lieu_bap_tay.md` | Quy trình thu dữ liệu bắp tay |
#   N C K H 2 6 2 7  
 