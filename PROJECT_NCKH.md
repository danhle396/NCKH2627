# PROJECT_NCKH: file theo dõi chính (tên tạm)

Đề tài NCKH 2026–2027: phát hiện té ngã, thiết bị đeo bắp tay, Tang Nano 4K (GW1NSR-LV4C: Cortex-M3 cứng + FPGA) + MPU6050 (GY-521) + ESP-01S (modem AT).

- **Kế hoạch, kiến trúc, ngân sách tài nguyên, sơ đồ chân, rủi ro:** [docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md](docs/ke_hoach/ke_hoach_toi_uu_tang_nano_4k_m3.md). Khi file này và kế hoạch khác nhau, sửa cho khớp rồi ghi vào nhật ký bên dưới.
- **SPEC mới:** [docs/spec_nckh.md](docs/spec_nckh.md) (11/11/2026), dựa trên [docs/tham_khao/spec_v3_du_thi.md](docs/tham_khao/spec_v3_du_thi.md) (kế hoạch mục 9).
- **Thử trên board (việc còn lại):** [docs/huong_dan_thu_tren_board.md](docs/huong_dan_thu_tren_board.md).
- **Bằng chứng:** đánh [x] một việc chỉ khi có bằng chứng trong `docs/bao_cao/<YYYY-MM-DD>_<chủ đề>/`. Số liệu ghi nhãn nguồn: board / phát lại / mô phỏng / tổng hợp / P&R / giả lập / công khai / ước tính.
- **Kiểm toàn bộ phần không cần board:** `bash rtl/scripts/test.sh` (10 mục; firmware giả lập cần `PY_UNICORN=<python có unicorn>`).

## Trạng thái

| Mục | Trạng thái |
|---|---|
| Sắp xếp thư mục (kế hoạch mục 13) | Xong đủ 8 bước 11/11/2026 (đã đổi tên thành `NCKH2627`) |
| Đưa lên GitHub `danhle396/NCKH2627` (kế hoạch mục 14) | Chưa làm, nhóm tự chạy |
| Các bước kỹ thuật không cần board (S0, 0–4) | **Xong 11/11/2026** [mô phỏng, P&R] |
| Firmware M3 (bước 5, 6, 8) | Mã xong, biên dịch được (flash 15,7 KB / 32 KB, RAM 6,2 KB / 8 KB), chạy đúng trên giả lập M3; **chờ board** |
| Bước 7, H1–H4 | Chờ board, linh kiện, dữ liệu bắp tay (nhóm tự thu 100 %) |
| Dữ liệu, mô hình | Không còn dữ liệu, mô hình hay ngưỡng nào ngoài bắp tay (11/11). ROM nạp **mô hình THỬ ngẫu nhiên `THU_NGAU_NHIEN`** (không huấn luyện) tới bước 7 |

## Kết quả chính (11/11/2026)

| Chỉ số | Giá trị | Nhãn |
|---|---|---|
| Thời gian suy luận một bước (bộ tăng tốc) | **70.010 chu kỳ = 2,59 ms ở 27 MHz** (chỉ tiêu ≤ 6 ms, bắt buộc < 320 ms) | mô phỏng RTL + netlist |
| MAC mỗi bước | 61.824 (E2 tính lại cả cửa sổ: 226.304) → ít hơn 3,7 lần | tự tính |
| Tài nguyên cả chip (EMPU 8 KB + cổng APB + bộ tăng tốc) | Logic 1.076 / 4.608 (24 %), thanh ghi 514 (15 %), **BSRAM 8 / 10**, DSP 1,25 / 8 | P&R |
| Fmax ở xung nhịp chung 27 MHz | 41,1 MHz | P&R |
| Khớp bit | golden tăng dần 41/41 lần thử ngã; RTL 52/52 bước mọi tầng; netlist sau P&R 52/52 bước; CNN phần mềm M3 150/150 | mô phỏng, giả lập |
| CNN phần mềm trên M3 | ~1,17 triệu lệnh mỗi lần (ước 45–65 ms) | giả lập (đếm lệnh), ước tính |

## Nhiệm vụ (theo kế hoạch mục 10)

| Bước | Việc | Pass (tóm tắt, chi tiết ở kế hoạch) | Trạng thái |
|---|---|---|---|
| S0 | Tổng hợp thử EMPU ở xung nhịp thạch anh | Biết BSRAM của EMPU; Fmax ≥ xung nhịp thạch anh | [x] SRAM 2/4/8/16 KB = 2/2/4/8 BSRAM; pROM, SDPB, MULT tổng hợp được; `.cst` không lỗi — [docs/bao_cao/2026-11-11_gowin_s0_buoc4](docs/bao_cao/2026-11-11_gowin_s0_buoc4/README.md) |
| 0 | O3: Dense 512 → 2, golden E2 hai lớp | 50 cửa sổ: argmax trùng golden 4 lớp | [x] 50/50 và 109.970/109.970 cửa sổ D1 — [2026-11-11_o3_dense_2_lop](docs/bao_cao/2026-11-11_o3_dense_2_lop/README.md) |
| 1a | Golden tăng dần `ml/src/golden_inc.py` | 50 cửa sổ liên tiếp khớp bit golden E2 hai lớp | [x] 41/41 lần thử ngã (≥ 50 bước mỗi lần), mọi tầng — [2026-11-11_golden_inc](docs/bao_cao/2026-11-11_golden_inc/README.md) |
| 1b | O1 bộ tăng tốc tính tăng dần (RTL `cnn_inc.v`) | Khớp bit mọi tầng; đệm 1 SDPB | [x] 52/52 bước l1/l2/l3 + logit; đệm + STAGE = 1 SDPB — [2026-11-11_rtl_inc_apb](docs/bao_cao/2026-11-11_rtl_inc_apb/README.md) |
| 2 | O2 gộp bias vào ROM | Khớp bit; 3 pROM | [x] 3 pROM [P&R]; khớp bit |
| 3 | O4 cổng APB + testbench APB | 50 bước khớp bit qua APB | [x] 52/52 bước + phép âm (RTL và netlist sau P&R) — [2026-11-11_gl_inc](docs/bao_cao/2026-11-11_gl_inc/README.md) |
| 4 | O5 một xung nhịp, tổng hợp đầy đủ | Trong ngân sách mục 4; ≤ 6 ms mỗi bước | [x] 24 % logic, 8/10 BSRAM, 2,59 ms, Fmax 41,1 MHz |
| 5 | Firmware M3 (cảm biến, tiền xử lý, driver APB, tự kiểm, O9, `chan.h`) | Tự kiểm đạt trên board; gây treo cảm biến 20/20 tự hồi phục | [ ] mã xong + giả lập đạt ([2026-11-11_fw_m3_gia_lap](docs/bao_cao/2026-11-11_fw_m3_gia_lap/README.md)); chờ board |
| 6 | Module WiFi: AT + web server | `/data` mỗi 300 ms; nút Hủy chạy | [ ] mã xong + giả lập với module giả đạt; chờ ESP-01S |
| 7 | Tầng 2 cho bắp tay | Ngưỡng chọn trên tập riêng | [ ] code + công cụ xong (`core/nga_bt.c`, `tools/chon_nguong_tang2.py`, C == Python 130/130); chờ dữ liệu để chọn ngưỡng (`NGA_BAT = 0`) |
| **7b** | **Xóa mô hình thử `THU_NGAU_NHIEN` và mọi dữ liệu giả lập** (luồng tổng hợp, vector ngẫu nhiên, tự kiểm LFSR) ngay khi có dữ liệu bắp tay thật; từ đó chỉ dùng dữ liệu thật | Không còn tham chiếu nào tới dữ liệu giả; `test.sh` chạy trên dữ liệu thật và PASS (kế hoạch 15.6) | [ ] chờ bước 7 |
| 8 | CNN phần mềm trên M3 (đối chứng) | Khớp bit golden; đo thời gian, năng lượng | [ ] khớp bit 150/150 trên giả lập; chờ đo trên board |
| H1–H4 | Bộ thử phần cứng, thử gây treo, đo dòng, PCB | Xem kế hoạch mục 10 | [ ] theo [hướng dẫn](docs/huong_dan_thu_tren_board.md) |
| **H1b** | **Khi board chạy thật: chỉ dùng thông số đo trên board, xóa thông số mô phỏng / giả lập / ước tính** khỏi tài liệu trình bày | Mọi số trình bày có nhãn [board] + log (kế hoạch 15.8) | [ ] chờ H1 |

## Nhật ký

| Ngày | Thay đổi | Bằng chứng |
|---|---|---|
| 2026-11-11 | Tạo thư mục bản mới từ bản dự thi (`fpga2026gowin` @ `f26bea8`), theo kế hoạch mục 13. Nguồn từng file: [README.md](README.md) | `rtl/scripts/test.sh`: tb_e2 P = 1 và P = 4, 10/10 vector khớp golden [mô phỏng] |
| 2026-11-11 (tối) | Làm kế hoạch mục 10 phần không cần board: O3 (2 lớp), golden tăng dần, RTL `cnn_inc.v` + `cnn_apb.v`, project Gowin dòng lệnh (`gowin/`, `rtl/scripts/build_gowin.sh`), netlist sau P&R (`rtl/scripts/gl_inc.sh`), firmware M3 (`fw_arm/`) + giả lập (`fw_arm/test/m3_test.py`), SPEC mới, hướng dẫn thử trên board. Đổi so với kế hoạch: NEW_DATA 144 byte (thêm x[0..3]), STAGE nằm trong SDPB đệm, ROM tách 3 bank, MAC thật 61.824; lớp IMPACT có trọng số khác 0 (vẫn bỏ được). Chi tiết: kế hoạch mục 15 | `docs/bao_cao/2026-11-11_*` (6 thư mục); `rtl/scripts/test.sh` 10/10 PASS |
| 2026-11-11 (khuya) | Xóa `ml/data/public/` (D1: kênh đầu, ngực, thắt lưng) theo quyết định nhóm, tránh lẫn kênh; dữ liệu bắp tay nhóm tự thu 100 %. Các kiểm khớp bit chuyển sang luồng tổng hợp `ml/src/luong_tong_hop.py`; `ml/train_cnn.py --data` thành bắt buộc | `rtl/scripts/test.sh` 10/10 PASS không cần D1; ROM, `cnn_inc_params.vh`, `fw_arm/src/gen/*` không đổi một byte |
| 2026-11-11 (khuya, 2) | Xóa mô hình kênh đầu D1 và vector có cửa sổ D1, cấu hình/ngưỡng ngực, tầng 2 và tư thế của ngực, công cụ D1/log dự thi, script GW2A cũ; ROM nạp mô hình thử ngẫu nhiên `THU_NGAU_NHIEN` (`tools/tao_mo_hinh_thu.py`), tên in ở UART/web. Chi tiết: kế hoạch 15.5 | `test.sh` 10/10 PASS; Keil 0 lỗi; Gowin không đổi tài nguyên |
| 2026-11-11 (khuya, 3) | Thêm bước 7b theo yêu cầu nhóm: khi có dữ liệu bắp tay thật thì xóa mô hình thử và mọi dữ liệu giả lập, chỉ dùng dữ liệu thật (kế hoạch 15.6) | – |
| 2026-11-12 | Việc không cần board: ghi dữ liệu thô (WiFi `/raw`, UART `R1`), hiệu chỉnh con quay, `tools/thu_bap_tay.py`, `windowing.py` cho bắp tay (C == Python từng byte), `tools/chon_truc.py`, tầng 2 bắp tay + công cụ chọn ngưỡng, quy trình thu dữ liệu bắp tay, IPUG931 + Register Map MPU6050, bảng tóm tắt dòng 17 (Keil). Chi tiết: kế hoạch 15.7 | `m3_test.py` 10/10 mục; `test.sh` |
| 2026-11-12 | Thêm bước H1b theo yêu cầu nhóm: có board chạy thật thì chỉ lấy thông số thật, xóa thông số mô phỏng, giả lập (kế hoạch 15.8) | – |
| 2026-11-12 | Project Gowin dùng bản sao RTL trong `gowin/src` (Gowin hiện `src\cnn_*.v`); bản gốc vẫn ở `rtl/src`; `rtl/scripts/dong_bo_gowin.sh` chép một chiều, `test.sh` và `build_gowin.sh` báo lỗi nếu hai bản lệch | `build_gowin.sh`: tài nguyên không đổi (1.076 logic, 8/10 BSRAM, 41,1 MHz); thử làm lệch 1 byte → bị bắt; `test.sh` PASS |
| 2026-11-12 | Xóa `fw_arm/src/port_tu_esp32/` (lõi ESP32/SPI bản dự thi) và `fw_arm/src/tham_khao/` (driver SN32F407): không nạp lên board, không còn dùng | `build.sh` và Keil 0 lỗi 0 cảnh báo (kích thước không đổi); `m3_test.py` PASS |
