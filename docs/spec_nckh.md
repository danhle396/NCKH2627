# SPEC NCKH2627 — phát hiện té ngã, Tang Nano 4K (Cortex-M3 + FPGA), đeo bắp tay

Phiên bản 1.0, 11/11/2026. Viết lại từ SPEC v3 của bản dự thi (`docs/tham_khao/spec_v3_du_thi.md`) theo kế hoạch mục 9. Khi SPEC và mã khác nhau, **mã đã kiểm (golden Python) là chuẩn**; sửa SPEC cho khớp. Số mục giữ như SPEC v3 để tiện đối chiếu (A.2 … A.12).

| Mục | So với SPEC v3 | Nguồn chuẩn (mã) |
|---|---|---|
| A.2 Tiền xử lý | Giữ; chạy trên M3; đổi trục theo vị trí đeo (O9) | `fw_arm/src/core/preprocess.c` = `ml/src/golden_model.py` `decimate`, `q8` |
| A.3 Cửa sổ | Giữ | `fw_config.h` |
| A.4 Lớp đầu ra | **2 lớp** | `tools/o3_dense_2_lop.py` |
| A.5 Số học | Giữ | `ml/src/golden_model.py` |
| A.6 Bộ nhớ, cách tính | **Tính tăng dần** | `ml/src/golden_inc.py`, `rtl/src/cnn_inc.v` |
| A.7 Giao tiếp | **APB** thay SPI | `rtl/src/cnn_apb.v`, `fw_arm/src/driver/cnn_drv.c` |
| A.8 Safety FSM | Giữ; bỏ IMPACT; điều kiện nghiêng theo vị trí | `fw_arm/src/core/safety.c`, `app/vi_tri_deo.h` |
| A.10 Tự kiểm | **8 bước LFSR** | `tools/export_inc.py`, `core/cnn_core.c` |
| A.12 Tầng 2 | Viết mới cho bắp tay (bước 7) | `app/vi_tri_deo.h` |
| O9 Vị trí đeo | Mới | `app/vi_tri_deo.h` |
| 7.5 Chống mất cảm biến | Mới | `core/imu_guard.c` |

## A.2 Tiền xử lý (M3)

MPU6050 (GY-521, 0x68), 100 Hz, ±16 g, ±2000 °/s, DLPF 44 Hz. Đổi trục cảm biến → trục người (X trước, Y trái, Z lên) theo `TRUC_X/Y/Z` của `vi_tri_deo.h`, trừ offset gia tốc `ACC_OFF_*` và offset con quay. Giảm mẫu 100 → 50 Hz và lượng tử INT8 như SPEC v3:

- y[n] = (r[2n−2] + 3·r[2n−1] + 3·r[2n] + r[2n+1] + 4) >> 3, với r[−2] = r[−1] = r[0];
- q = clamp((y + 2^(s−1)) >> s, −128, 127), với s = 7 (gia tốc: 1 mức = 1/16 g), s = 6 (con quay: 1 mức = 3,90 °/s).

## A.3 Cửa sổ

128 mẫu 50 Hz × 6 kênh (2,56 s), thứ tự byte i = mẫu·6 + kênh (AX AY AZ GX GY GZ). Trượt 16 mẫu (0,32 s). M3 giữ vòng 768 byte. **Mỗi 16 mẫu 50 Hz M3 gửi một bước cho bộ tăng tốc, ngay từ mẫu thứ 16** (vòng khởi tạo 0), nên kết quả hợp lệ đầu tiên đến ở mẫu thứ 128, như khi chờ đủ cửa sổ.

## A.4 Lớp

Bộ tăng tốc trả **0 = NORMAL, 1 = FALL** (Dense 512 → 2; 5.344 trọng số, 66 bias). Firmware đổi **1 → 3 (CLS_FALL)** trước `safety_on_result()`; bỏ hẳn lớp IMPACT. Số lớp là tham số (`NOUT`) của RTL và golden.

## A.5 Số học (giữ)

Trọng số INT8, tích lũy INT32. Conv1D k = 5, đệm 2 (ngoài biên = 0), sau mỗi Conv: acc ≤ 0 → 0, ngược lại min(acc >> n, 127) (dịch số học, không làm tròn), rồi MaxPool 2. N2: Conv 6→16, 16→16, 16→32, shift n = 8, 8, 8. Flatten 16 × 32 = 512, đặc trưng i = t·32 + c. Logit = bias + Σ w·f. Argmax: bằng nhau thì lấy chỉ số nhỏ. SCORE = clamp((top1 − top2) >> 9, 0, 127).

## A.6 Bộ nhớ và cách tính tăng dần

**ROM** (3 pROM 2.048 × 8, 5.608 byte): trọng số 1 làn — Conv lớp l ở BASE_l + (g·5 + k)·IC + ic (BASE = 0, 480, 1760), Dense ở 4320 + g·512 + i — rồi **bias INT32 little-endian ở 5344 + 4·b** (b: lớp 1 = 0…15, lớp 2 = 16…31, lớp 3 = 32…63, Dense = 64, 65).

**Đệm đặc trưng** (1 SDPB 2.048 × 8), vòng theo bộ đếm bước s (0…7, xóa khi SOFT_RST):

| Vùng | Địa chỉ của vị trí logic p, kênh c |
|---|---|
| L1 (64 × 16) | ((p + 8s) mod 64)·16 + c |
| L2 (32 × 16) | 1024 + ((p + 4s) mod 32)·16 + c |
| L3 (16 × 32) | 1536 + ((p + 2s) mod 16)·32 + c |
| STAGE (144 B) | 1536 + ((12 + 2s + i/32) mod 16)·32 + (i mod 32) — 6 ô L3 sẽ tính lại trong bước |

**Mỗi bước tính** L1 p ∈ {0, 55…63}, rồi L2 p ∈ {0, 1, 26…31}, rồi L3 p ∈ {0, 1, 12…15}, rồi Dense đủ 512 (61.824 MAC). Vị trí "trong" (không chạm đệm 0: L1 1…62, L2 2…29, L3 2…13) chỉ phụ thuộc dữ liệu luồng nên dùng lại sau khi dời. Kết quả hợp lệ khi **WARM = 8**. Khớp bit với tính lại cả cửa sổ: `tools/kiem_golden_inc.py`.

## A.7 Cổng APB (APB2 Master 1, 0x40002400)

| Offset | Tên | R/W | Nội dung |
|---|---|---|---|
| 0x00 | ID | R | 0x4E434931 ("NCI1") |
| 0x04 | CTRL | W | bit0 START, bit1 SOFT_RST (WARM = 0, s = 0, con trỏ NEW_DATA = 0) |
| 0x08 | STATUS | R | bit0 BUSY, bit1 DONE, bit2 WR_REJECT, bit3 VALID (WARM = 8). Đọc thì xóa DONE, WR_REJECT |
| 0x0C | CLASS | R | 0 / 1 |
| 0x10 | SCORE | R | 0…127 |
| 0x14, 0x18 | LOGIT0, LOGIT1 | R | INT32 |
| 0x24 | CYCLES | R | chu kỳ của bước vừa xong |
| 0x28 | NEW_DATA | W | 1 byte; **144 byte mỗi bước**: x[0..3] rồi x[108..127] của cửa sổ, byte = mẫu·6 + kênh |
| 0x2C | WARM | R | 0…8 |
| 0x30 | WROM_ADDR | R/W | địa chỉ ROM (O7) |
| 0x34 | WROM_DATA | R | 1 byte ROM có dấu, rồi WROM_ADDR tự tăng (2 chu kỳ chờ) |
| 0x38 | NEW_PTR | R | số byte NEW_DATA đã nạp |

Bị từ chối (đặt WR_REJECT, không làm gì): NEW_DATA hay START khi BUSY; byte NEW_DATA thứ 145; START khi chưa đủ 144 byte; đọc WROM_DATA khi BUSY. DONE nối **USER_INT0** (IRQ 1), mức cao tới khi M3 đọc STATUS. Một xung nhịp 27 MHz cho M3 và FPGA, không PLL. Thời gian một bước: 70.010 chu kỳ = 2,59 ms [mô phỏng].

## A.8 Safety FSM (giữ, trừ hai điểm)

STANDBY ↔ MONITOR → SUSPECT (≥ 2 phiếu FALL trong 3 cửa sổ hợp lệ gần nhất, SCORE ≥ T_SCORE = 0) → PRE_ALARM (bất động 8 khối, kèm nghiêng nếu `TILT_EN`) → ALARM → COOLDOWN; hằng số trong `fw_config.h`. Khác SPEC v3: (1) bỏ IMPACT; (2) điều kiện nghiêng `az_tb` thuộc cấu hình vị trí, **tắt với bắp tay** cho tới bước 7. Kết quả có VALID = 0 không được bỏ phiếu. Hủy: nút (giữ 3 s = SOS) hoặc POST /huy.

## A.10 Tự kiểm khi khởi động

Đọc ID; SOFT_RST; 8 bước với luồng LFSR Galois 16 bit (đa thức 0xB400, mầm 0xACE1; byte thứ i của luồng là mẫu i/6, kênh i mod 6; cửa sổ bước n bắt đầu ở mẫu 16n; 240 mẫu); so bước 8 với `gen/selftest_expect_inc.h` (giá trị sinh theo mô hình đang nạp; với mô hình THỬ ngẫu nhiên hiện tại: lớp 0, SCORE 34, logit {18073, 423}). Không đạt thì LED nháy nhanh, web báo FAIL. Sau tự kiểm: SOFT_RST.

## A.12 Tầng 2 (bắp tay: chưa chốt)

`core/nga_bt.c` (bản Python `ml/src/nga_bt.py`, khớp từng bit). Mỗi khối 32 mẫu lưu |a|² nhỏ nhất, |a|² lớn nhất, |ω|² lớn nhất, cờ yên. Khi vào SUSPECT: khối va chạm = khối có |a|² lớn nhất trong `tim_khoi` khối gần nhất; xác nhận khi đủ (1) pha rơi: |a|²_min ≤ roi² trong `roi_khoi` khối trước va chạm, (2) va chạm ≥ va², (3) quay ≥ quay² quanh va chạm ±1 khối, (4) có khối yên trong `yen_khoi` khối sau va chạm; không đủ → `safety_bo_nghi()`. Ngưỡng chọn bằng `tools/chon_nguong_tang2.py` trên dữ liệu bắp tay (bước 7); đến khi đó `NGA_BAT = 0`: hệ thống chạy đúng A.8.

## O9 Cấu hình vị trí đeo

`fw_arm/src/app/vi_tri_deo.h`: chỉ có `VT_BAP_TAY` (cấu hình ngực đã xóa). Mỗi vị trí gồm: đổi trục, offset gia tốc, `TILT_EN`/`T_TILT`, `NGA_BAT` và ngưỡng tầng 2; mọi số của bắp tay chưa chọn. Đổi vị trí = thêm một khối cấu hình và huấn luyện lại mô hình; RTL và phần còn lại của firmware giữ nguyên.

## 7.5 Chống mất cảm biến

`core/imu_guard.c`: phát hiện khi quá 50 ms không có INT, khi 3 lần đọc lỗi liên tiếp, hoặc khi 6 trục giống hệt nhau 1 s. Tự gỡ theo bậc: (1) 9 xung SCL + STOP, cấu hình lại; (2) DEVICE_RESET, chờ 100 ms; (3) SCL, SDA ra 0 → cắt nguồn ≥ 1 s → cấp lại, chờ 100 ms → cấu hình; (4) báo MẤT CẢM BIẾN, cứ 10 s thử lại bậc 3. Khởi động luôn chạy bậc 3 một lần. Khi gỡ: xóa vòng tiền xử lý và SOFT_RST bộ tăng tốc; Safety FSM giữ nguyên.

## Mô hình đang nạp

Mô hình THỬ ngẫu nhiên `THU_NGAU_NHIEN` (`tools/tao_mo_hinh_thu.py`), không huấn luyện từ dữ liệu nào, chỉ để kiểm khớp bit. Không dùng để phát hiện ngã. Thay bằng mô hình bắp tay: `ml/train_cnn.py --data <npz bắp tay>` → `tools/o3_dense_2_lop.py --params …` → `tools/export_inc.py` (kế hoạch 15.4).

## Ghi dữ liệu thô và UART gỡ lỗi

- Khởi động: `TK,<đạt>,<cycles>,vi_tri=<…>,mo_hinh=<…>`; hiệu chỉnh con quay xong (2 s đứng yên): `GC,<x>,<y>,<z>`; mỗi bước: `S,<ms>,<bước>,<valid>,<lớp 0/3>,<score>,<logit0>,<logit1>,<cycles>,<trạng thái>`.
- Dữ liệu thô 100 Hz (`core/raw_log.h`): dòng `t,ax,ay,az,gx,gy,gz,btn` — trục cảm biến, con quay đã trừ offset, chưa đổi trục. Qua WiFi: `GET http://192.168.4.1/raw` (kết nối giữ mở). Qua UART1: lệnh `R1` bắt đầu, `R0` dừng (khi đó không in dòng `S`). Thu bằng `tools/thu_bap_tay.py`; cắt cửa sổ bằng `tools/windowing.py` (đổi trục, tiền xử lý đúng như thiết bị).
