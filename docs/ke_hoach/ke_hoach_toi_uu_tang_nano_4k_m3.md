# Kế hoạch tối ưu tài nguyên: chạy toàn bộ trên Tang Nano 4K (M3 + FPGA) + MPU6050 + module WiFi (modem AT)

> Trạng thái: **đề xuất; mục 13 (sắp xếp thư mục) đã thực hiện bước 0–6 ngày 11/11/2026, xem 13.5**.
> - 11/11/2026 (đêm): **thực hiện mục 10 phần không cần board (S0, bước 0–4 xong; firmware bước 5, 6, 8 viết xong, chạy đúng trên giả lập)**. Kết quả và các điểm phải đổi so với kế hoạch: **mục 15**. Các chỗ sai đã sửa tại chỗ, có ghi "(sửa 11/11, mục 15)".
> - 11/11/2026 (tối): thực hiện mục 13 bước 0–6. Đường dẫn trong file này đổi sang vị trí mới (`docs/so_do/`, `docs/tham_khao/`); Pass của test.sh sửa thành "10/10 vector" theo đúng số `test.sh` chạy (trước ghi nhầm 50/50); thêm `ml/out/` vào bảng 13.3.
> - 9/11/2026: tạo; đổi Bluetooth sang module WiFi dùng như modem (lệnh AT); thay ước tính bằng số chính thức từ IPUG932; thêm tầng 2 và danh sách phần cứng.
> - 11/11/2026: thêm mục 13 (sắp xếp lại thư mục trên máy, không đưa lên git; đổi tên thư mục dự án thành `NCKH2627`); thêm mục 14 (lệnh để nhóm tự đưa bản mới lên GitHub `danhle396/NCKH2627`). Dự phòng đổi thành STM32 + Tang Nano 4K. **Thay O1 gộp RAM bằng bộ tăng tốc kiểu mới (tính tăng dần)**; M3 chỉ gửi 96 B mẫu mới mỗi bước; cả chip chạy một xung nhịp, không PLL; chỉ tiêu thời gian hai tầng: bắt buộc xong trước 0,32 s, hiệu năng ≤ 6 ms ở xung nhịp thạch anh; 1 làn MAC làm chuẩn; **chốt O3 (bỏ 2 logit không dùng)**; **bộ tăng tốc kiểu mới là bắt buộc, bỏ E2 làm dự phòng**; tách phần phụ thuộc vị trí đeo thành cấu hình (O9); bỏ việc sửa `PROJECT_MASTER.md` và SPEC cũ.
>
> - 11/11/2026 (chiều): thêm **mục 12.5 sơ đồ chân và nối dây** theo sơ đồ nguyên lý Tang Nano 4K; xác nhận thạch anh 27 MHz và mã chip C6/I5; tải datasheet vào `docs/datasheet/tang_nano_4k/`. Bỏ máy phân tích logic và 2 chân đo `probe0/1`: thời gian suy luận đo bằng thanh ghi `CYCLES`, in qua USB–UART. Cảm biến là **module GY-521 (MPU6050)**; Tang Nano 4K, ESP-01S, GY-521 cắm header cái trên PCB (mục 12.5.7). Toàn bộ linh kiện đổi sang **chân cắm** (S8050, S8550; mục 12.5.4). Thêm **mục 7.5 chống mất cảm biến** (tự gỡ 4 bậc, cắt nguồn đúng quy trình datasheet, không phải rút pin). Verify toàn file, sửa 15 điểm: còi 2 kHz trong FPGA (`gpio[8]`), O7, RST ESP-01S, dòng ESP-01S, đo dòng an toàn, chân SPI khi dự phòng STM32, bằng chứng sau mục 13, thu ngã qua WiFi, bước phần cứng H1–H4, 2 rủi ro mới; tóm tắt thành 10 đổi / 5 một phần / 3 giữ.
>
> Vị trí đeo **chốt là bắp tay**. Đề xuất NCKH nộp trường viết mở ("vị trí đeo đã chọn") để sau này mở rộng sang vị trí khác; O9 giúp việc đó.
>
> Nhãn số liệu:
> - **[tổng hợp]** = `docs/tham_khao/fpga_project_syn_resource.html` (bản dự thi: `fpga/impl/gwsynthesis/`)
> - **[P&R]** = `docs/tham_khao/fpga_project.rpt.txt` (bản dự thi: `fpga/bitstream/`)
> - **[datasheet]** = Sipeed Tang Nano 4K Datasheet v1.0 (07/8/2021)
> - **[sơ đồ NL]** = sơ đồ nguyên lý Tang Nano 4K bản 3603 (Sipeed, 20/8/2021); các nguồn của mục 12.5 ghi ở đó
> - **[IPUG932]** = Gowin_EMPU(GW1NS-4C) Hardware Design Reference Manual IPUG932-2.1E (15/8/2025), https://cdn.gowinsemi.com.cn/IPUG932E.pdf
> - **[ước tính]** = suy luận, chưa đo; **[tự tính]** = tính từ số đã có
> - **[CẦN XÁC MINH]** = phải kiểm bằng tổng hợp thử hoặc trên board
>
> Sơ đồ đi kèm: `docs/so_do/so_do_de_xuat_tang_nano_4k_m3_wifi.drawio` (3 trang), `docs/so_do/so_do_kien_truc_arm.drawio` (2 trang), `docs/so_do/so_do_chan_tang_nano_4k.drawio` (3 trang, mục 12.5).

## Tóm tắt: giữ lại và thay đổi so với bản dự thi 2026

| #  | Thành phần     | Bản dự thi                      | Bản mới                         | Kết luận |
|----|----------------|---------------------------------|---------------------------------|----------|
| 1  | Nền tảng       | 3 board: SN32F407+ESP32+GW2A-18 | Tang Nano 4K: M3 + FPGA 1 chip  | Đổi      |
| 2  | Vi xử lý       | Cortex-M0 + ESP32               | Cortex-M3 trong chip            | Đổi      |
| 3  | Vị trí đeo     | Ngực                            | Bắp tay (cấu hình O9)           | Đổi      |
| 4  | Cảm biến       | MPU6050, 100 Hz                 | Như cũ, thêm tự gỡ treo (7.5)   | Một phần |
| 5  | Tiền xử lý     | Lọc, 50 Hz, INT8, cửa sổ 128×6  | Như cũ, đổi trục theo bắp tay   | Một phần |
| 6  | Mạng nơ-ron    | N2, Dense 512→4, 6.368 trọng số | N2, Dense 512→2, 5.344 trọng số | Một phần |
| 7  | Số học         | INT8, tích lũy INT32, dịch bit  | Như cũ                          | Giữ      |
| 8  | Bộ tăng tốc    | E2: tính lại cả cửa sổ          | Tính tăng dần (O1), bắt buộc    | Đổi      |
| 9  | Bộ nhớ FPGA    | 8 khối BSRAM                    | 4 khối (O1, O2, O3)             | Đổi      |
| 10 | Giao tiếp      | SPI, 772 B/lần + CRC            | APB2, 144 B/lần + USER_INT0     | Đổi      |
| 11 | Xung nhịp      | PLL ~50 MHz, chỉ tiêu ≤ 6 ms    | Thạch anh; < 0,32 s và ≤ 6 ms   | Đổi      |
| 12 | Tầng 2         | 4 dấu hiệu theo ngực            | Viết lại cho bắp tay            | Một phần |
| 13 | Máy trạng thái | Bỏ phiếu 2/3, đếm 10 s, SOS     | Như cũ, bỏ IMPACT, sửa nghiêng  | Một phần |
| 14 | Cảnh báo       | Còi trên EVK SN32, web ESP32    | Còi + web trên M3, WiFi AT      | Đổi      |
| 15 | Đối chứng      | Ước tính trên Cortex-M0         | Đo thật trên lõi M3             | Đổi      |
| 16 | Kiểm chứng     | 4 cấp khớp từng bit             | Như cũ + golden tăng dần        | Giữ      |
| 17 | Công cụ        | Gowin EDA, Icarus, Keil         | Như cũ, Keil cho M3 (GMD dự phòng) | Giữ   |
| 18 | Thư mục        | fpga2026gowin/: esp32/, fpga/ … | NCKH2627/, repo mới (mục 13-14) | Đổi      |

Tổng 18 thành phần: **10 đổi, 5 đổi một phần (mục 4, 5, 6, 12, 13), 3 giữ nguyên**. Phần giữ lại chủ yếu là thuật toán (cảm biến, tiền xử lý, số học, lõi mạng, logic máy trạng thái) và quy trình kiểm chứng. Chi tiết từng mục ở các mục O1–O9 bên dưới.

---

## 1. Kiến trúc đích

```
             I²C (100 Hz, DRDY)                          bus nội bộ APB2
 MPU6050 ─────────────────► ┌────────────────────── GW1NSR-LV4C ───────────────────────┐
 (bắp tay)                  │  Cortex-M3 cứng                     FPGA (4.608 LUT)      │
                            │  • đọc cảm biến (I²C bằng GPIO)     • cổng APB (256 B)    │
                            │  • tầng 1: tiền xử lý, bỏ phiếu ──► • bộ tăng tốc kiểu mới│
                            │    (gửi 96 B mẫu mới mỗi 0,32 s)      (tính tăng dần)     │
                            │  • tầng 2: 4 dấu hiệu (nga.c)       • đệm đặc trưng 1 khối│
 Còi, nút, công tắc ◄─GPIO──┤  • Safety FSM                       • ROM trọng số + bias │
                            │  • web server qua lệnh AT     ◄──── USER_INT0 (DONE)      │
                            │  • CNN phần mềm (đối chứng)                               │
                            │  một xung nhịp chung từ thạch anh, không PLL             │
                            └──────────────┬────────────────────────────────────────────┘
                                           │ UART, lệnh AT
                                 Module WiFi (modem, firmware AT gốc) ──WiFi──► trình duyệt
```

So với kiến trúc dự thi (SN32F407 + ESP32 + GW2A-18):
- Bỏ ESP32 và SN32F407. Lõi M3 trong chip làm việc của cả hai: đọc cảm biến, điều phối, cảnh báo.
- Bỏ giao tiếp SPI 772 B có CRC. M3 đọc ghi FPGA qua bus APB2 trong chip, **mỗi bước chỉ gửi 16 mẫu mới (96 B)** thay vì cả cửa sổ 768 B.
- **Bộ tăng tốc kiểu mới**: mỗi 0,32 s chỉ tính phần dữ liệu mới và giữ lại kết quả cũ trong đệm (O1), thay cho E2 tính lại cả cửa sổ. **Bắt buộc làm, không có phương án lui về E2.** E2 chỉ còn hai vai trò: golden Python của E2 là chuẩn đối chiếu khớp bit, và các khối tính trong `cnn_e2.v` được dùng lại.
- **Một xung nhịp** từ thạch anh cho cả M3 và FPGA, không dùng PLL (O5).
- **Module WiFi dùng như modem**: firmware AT gốc của hãng, nhóm không lập trình. Module nào cũng có chip xử lý bên trong để chạy ngăn xếp WiFi và TCP/IP; ở đây nó chỉ truyền dữ liệu. **Web server chạy trên M3**, nội dung trang giữ như trang hiện có.
- Còi trên thiết bị là cảnh báo chính, vẫn kêu khi mất WiFi.
- Phần phụ thuộc vị trí đeo (bắp tay) gom vào một bộ cấu hình (O9).

## 2. Tài nguyên của chip GW1NSR-LV4C

| Tài nguyên | Số lượng | Nguồn |
|---|---|---|
| LUT4 | 4.608 | [datasheet] |
| Thanh ghi (FF) | 3.456 | [datasheet] |
| BSRAM | 180 Kbit = 10 khối × 18 Kbit | [datasheet], bảng họ GW1NS của Gowin |
| Bộ nhân 18 × 18 | 16 | [datasheet] |
| PLL | 2 (kế hoạch không dùng, xem O5) | [datasheet] |
| Cortex-M3 | tối đa 80 MHz theo [datasheet]; [IPUG932] ghi xung hệ thống tối đa 200 MHz tùy thiết kế và chip | [datasheet], [IPUG932] mục 1.2.2 |
| SRAM của M3 | chọn được 2 / 4 / 8 / 16 KB (mặc định 16 KB); **nằm trong phần FPGA, lấy từ BSRAM** | [IPUG932] mục 1.1, 1.2.2, 3.2.6 |
| Flash chương trình M3 | **32 KB**, là flash nhúng (chính là "user flash 256 Kb" Sipeed ghi), không lấy BSRAM | [IPUG932] mục 1.2.2 |
| Ngắt từ FPGA vào M3 | **6 đường USER_INT0–5** | [IPUG932] mục 3.2 |
| Cổng mở rộng APB2 | **12 cổng, mỗi cổng 256 B**, cổng 1 bắt đầu ở 0x40002400 | [IPUG932] bảng 3-2 |
| HyperRAM trên board | 64 Mb, bus 8 bit | [datasheet] |
| GPIO | 38 theo [datasheet]; bảng chân gói QFN48P của Gowin đếm được 39 chân I/O (`GW1NSR-LV4C_QFN48P_bang_chan.csv`). Board đã nối sẵn nhiều chân (flash SPI, JTAG, HDMI, camera). Chân 3,3 V còn dùng được chỉ 7 chân bank 1; gán chân xem **mục 12.5** | [datasheet], [sơ đồ NL] |

Tài nguyên của riêng EMPU theo [IPUG932] bảng 1-2:

| Cấu hình | LUT | FF | BSRAM | Tần số |
|---|---|---|---|---|
| Chỉ lõi MCU | 33 | 19 | 4 | 200 MHz |
| Lõi MCU + I²C master + SPI master | 456 | 282 | 4 | 100 MHz |

Bảng 1-2 không ghi 4 BSRAM ứng với SRAM bao nhiêu KB. **S0 (11/11): SRAM 2 / 4 / 8 / 16 KB = 2 / 2 / 4 / 8 BSRAM [P&R]**; chọn 8 KB = 4 khối (mục 15).

Lưu ý: DS821 ghi BSRAM của GW1NS-4 **không hỗ trợ chế độ true dual-port**. Thiết kế chỉ dùng SDPB và pROM; **S0: SDPB, pROM, MULT tổng hợp và đặt – nối dây được** [P&R]. GW1NS-4 **không có RAM phân tán** (Gowin báo IF0005), nên mọi bộ nhớ nhỏ phải nằm trong BSRAM hoặc thanh ghi (mục 15).

## 3. Hiện trạng (N2, 1 làn, bitstream 24/10)

| Khối | LUT | ALU | FF | BSRAM | DSP | Trên Tang Nano 4K |
|---|---|---|---|---|---|---|
| `u_cnn` (`cnn_e2.v`) | 456 | 172 | 571 | 7 (4 ROM trọng số, 1 ROM bias, `ram_a`, `ram_b`) | 3 | Dùng lại khối tính (MAC, lượng tử, ReLU, gộp); thay bộ điều khiển và bộ nhớ (O1) |
| `u_input_buffer` | – | – | – | 1 | – | **Bỏ**: chỉ còn đệm nhỏ cho 16 mẫu mới (O1) |
| `u_frame` (`result_frame_v3.v`) + CRC | 581 | 37 | 331 | – | – | **Bỏ** |
| `u_parser` + CRC, `u_spi_slave`, `u_tx`, top | 161 | 20 | 182 | – | – | **Bỏ** |
| **Tổng** | 1.198 | 251 | 1.084 | 8 | 2,25 (đơn vị DSP của GW2A) | |

Nguồn: [tổng hợp], tổng khớp [P&R] (Logic 1.473, FF 1.084, BSRAM 8 = 3 SDPB + 5 pROM).

**Chỗ nghẽn là BSRAM, không phải LUT.** Nếu giữ nguyên: CNN 8 khối + M3 4 khối = 12 > 10.

## 4. Ngân sách mục tiêu (1 làn)

| Thành phần | Logic | FF | BSRAM | Bộ nhân | Nhãn |
|---|---|---|---|---|---|
| Bộ tăng tốc kiểu mới (O1) + gộp bias (O2) + bỏ 2 logit (O3) | ~650–900 | ~600–700 | **4** (ROM trọng số + bias 3 + đệm 1) | ~5 | [tổng hợp] cho phần tính + [ước tính] cho bộ điều khiển và đệm |
| Cổng APB (O4) | 100–250 | 100–200 | 0 | 0 | [ước tính] |
| Lõi MCU của EMPU | 33 | 19 | 4 | 0 | [IPUG932] bảng 1-2 |
| **Tổng** | **~785–1.180 / 4.608 (17–26 %)** | **~720–920 / 3.456 (21–27 %)** | **8 / 10** (dư 2) | **~5 / 16** | |
| Nếu dùng IP I²C + SPI master của EMPU (không khuyên) | +423 | +263 | 0 | 0 | [IPUG932] bảng 1-2 |

Tổng BSRAM (M3 = 4 khối):

| Cấu hình bộ tăng tốc | Bộ tăng tốc | M3 | Tổng / 10 |
|---|---|---|---|
| E2 giữ nguyên | 8 | 4 | **12, không đủ** |
| Kiểu mới, bias để riêng, 4 logit | 6 (ROM 4 + bias 1 + đệm 1) | 4 | 10, vừa khít |
| Kiểu mới + O2, 4 logit | 5 | 4 | 9, đủ |
| **Kiểu mới + O2 + O3 (chốt)** | **4** | 4 | **8, dư 2** |

→ **Chốt O1 + O2 + O3**: bộ tăng tốc 4 khối, cùng M3 là 8/10, dư 2 khối để dự phòng. Nếu firmware cần SRAM 16 KB thì vẫn không đủ (16 KB ước khoảng 8 khối, theo tỉ lệ, **[CẦN XÁC MINH]**).

Chỉ tiêu đề xuất (chốt sau bước S0): Logic ≤ 60 %, BSRAM ≤ 10/10, bộ nhân ≤ 50 %, **mỗi bước trượt xử lý xong trước bước kế tiếp (0,32 s, bắt buộc)**, **bộ tăng tốc suy luận mỗi bước ≤ 6 ms ở xung nhịp thạch anh, không dùng PLL (chỉ tiêu hiệu năng)**, khớp bit với golden.

## 5. Bước S0: thử khả thi (làm trước mọi tối ưu)

Mục đích: biết lõi M3 thật sự lấy bao nhiêu BSRAM và LUT **trước khi** sửa RTL.

1. Tạo project Gowin mới cho part **GW1NSR-LV4CQN48PC6/I5** (mã trên board và trong Gowin EDA 1.9.11.03, mục 12.5.1) trong thư mục riêng (`gowin/` của `NCKH2627`, tạo sau khi đổi tên thư mục ở mục 13).
2. Thêm IP Gowin_EMPU (GW1NS-4C): bật UART0, UART1, GPIO, APB2 Master 1, USER_INT0; **không bật** I²C/SPI master; thử SRAM 4 KB và 8 KB.
3. Thêm `cnn_e2.v` nguyên trạng, nối một cổng APB giả để công cụ không xóa logic.
4. Tổng hợp và đặt – nối dây ở xung nhịp thạch anh với file ràng buộc chân nháp (mục 12.5.5); ghi lại Logic, FF, BSRAM, DSP, Fmax.

**Pass:** biết chính xác số BSRAM của EMPU theo từng mức SRAM; `pROM`, `SDPB`, `MULTADDALU18X18` tổng hợp được trên GW1NS-4; Fmax ≥ tần số thạch anh; file `.cst` của mục 12.5.5 không báo lỗi bank hoặc loại IO.

## 6. Các bước tối ưu

### O1. Bộ tăng tốc kiểu mới: tính tăng dần, giữ đệm đặc trưng (thay E2) — **bắt buộc**

- **Ý tưởng:** cửa sổ 128 mẫu (50 Hz) trượt 16 mẫu mỗi 0,32 s, nên hai cửa sổ liên tiếp **trùng 112/128 mẫu**. E2 tính lại toàn bộ 227.328 MAC mỗi lần. Bộ tăng tốc mới chỉ tính các vị trí mới và giữ kết quả cũ.
- **Cơ sở để vẫn khớp bit:**
  - Mỗi đầu ra Conv (k = 5) chỉ phụ thuộc ±2 vị trí lân cận.
  - Bước trượt 16 chia hết cho 8 (ba lần MaxPool 2), nên ranh giới gộp không lệch giữa các lần.
  - Các vị trí sát hai mép (chịu ảnh hưởng của đệm 0) được tính lại mỗi lần.
  - Vì vậy kết quả phải **trùng từng bit** với tính lại toàn bộ cửa sổ (golden E2). Mô hình không cần huấn luyện lại. **[CẦN XÁC MINH bằng golden Python trước khi viết RTL]**
- **Mỗi bước trượt:**
  - L1: 8 đầu ra sau gộp mới, cộng vài vị trí sát mép.
  - L2: 4 đầu ra mới; L3: 2 đầu ra mới, cộng vị trí sát mép.
  - Dense đọc đủ 16 × 32 = 512 đặc trưng.
  - Số MAC khoảng **30–39 nghìn thay vì 227.328 (≈ 6–7,5 lần ít hơn)** [ước tính thô]. **Thực tế (golden, 11/11): 61.824 MAC mỗi bước, ít hơn 3,7 lần** so với 226.304 của E2 hai lớp; mỗi bước tính L1 {0, 55…63}, L2 {0, 1, 26…31}, L3 {0, 1, 12…15} (sửa 11/11, mục 15).
- **Bộ nhớ:**
  - Đệm vòng sau gộp: L1 64 × 16 = 1.024 B, L2 32 × 16 = 512 B, L3 16 × 32 = 512 B → **2.048 B = 1 khối SDPB (2.048 × 8)** [tự tính].
  - Mẫu vào: ~~16 mẫu mới + 4 mẫu cũ sát mép, × 6 kênh = 120 B → RAM trong LUT (SSRAM)~~. **Sửa 11/11 (mục 15):** tính lại L1[0] cần cả 4 mẫu đầu cửa sổ x[0..3], nên mỗi bước M3 gửi **144 B = x[0..3] + x[108..127]**; GW1NS-4 không có RAM phân tán nên 144 B này nằm **trong chính SDPB đệm**, ở 6 ô L3 sẽ tính lại trong bước (đã "chết" sau Dense bước trước).
  - ROM trọng số giữ như E2 (gộp bias theo O2).
- **Khởi động và lỗi:** sau bật nguồn hoặc `SOFT_RST`, đệm trống; cần **8 bước trượt (2,56 s)** mới có kết quả hợp lệ, giống E2 cần đủ cửa sổ. Khi cảm biến mất mẫu, M3 xóa đệm (`SOFT_RST`) và chờ đủ lại.
- **Dùng lại từ E2:** khối MAC, lượng tử hóa (số học A.5 cũ), ReLU, MaxPool, Dense, argmax. **Viết mới:** bộ điều khiển (chọn vị trí cần tính), địa chỉ đệm vòng, cổng nạp mẫu mới.
- **File (tên tạm):** `rtl/src/cnn_inc.v` (dựa trên `cnn_e2.v`); golden `tools/golden_inc.py`; testbench `rtl/sim/tb_inc.v`.
- **Pass:**
  1. Golden tăng dần khớp bit với golden E2 trên **50 cửa sổ liên tiếp** (gồm cả 8 bước khởi động).
  2. RTL khớp golden mọi tầng.
  3. Tổng hợp: đệm dùng đúng 1 SDPB.
  4. Ghi lại số chu kỳ mỗi bước.

### O2. Gộp bias vào ROM trọng số (−1 khối) — **chốt làm**

- **Cơ sở:**
  - ROM trọng số 6.368 từ × 8 bit chiếm 4 khối (mỗi khối 2.048 từ), còn trống 1.824 từ.
  - ROM bias 68 × 32 bit chiếm riêng 1 khối.
  - 68 bias × 4 byte = 272 byte, vừa chỗ trống.
- **Cách làm:**
  - `export_mem.py` ghi bias (4 byte, thứ tự cố định) sau Dense, tại `BIAS_BASE = 6368`.
  - RTL đọc bias bằng 4 lần đọc 8 bit, thêm vài chu kỳ mỗi kênh.
- **Sau O3 (đã chốt):** `BIAS_BASE = 5344`; 5.344 trọng số + 66 bias × 4 B = 5.608 từ ≤ 6.144 → **3 khối**.
- **Pass:** khớp bit mọi tầng; tổng hợp còn 3 pROM (4 nếu chưa làm O3); số chu kỳ ghi lại.

### O3. Bỏ 2 logit không dùng, Dense 512 → 2 (−1 khối, với 1 làn) — **chốt làm (11/11)**

- **Cơ sở:**
  - Mô hình chỉ huấn luyện lớp 0 NORMAL và 3 FALL (SPEC v3.2, 16/10). Lớp 1 IMPACT để dành, lớp 2 không dùng.
  - Hai lớp dự trữ hiện có trọng số 0 và bias −2³⁰ (SPEC A.4 cũ), nên bỏ đi **không đổi kết quả nhận dạng**; không cần huấn luyện lại. **Sửa 11/11 (mục 15):** chỉ lớp 2 như vậy; lớp 1 (IMPACT) có 488/512 trọng số khác 0, bias −53. Vẫn bỏ được: trên toàn bộ 109.970 cửa sổ D1 mô hình 4 lớp chưa lần nào chọn lớp 1, lớp và phiếu FALL không đổi.
  - Số lượng: bớt 2 × 512 = **1.024 trọng số** và 2 bias → **6.368 → 5.344 trọng số (−16 %)**, 68 → 66 bias. Ba lớp tích chập (480 + 1.280 + 2.560 = 4.320 trọng số) giữ nguyên, nên trọng số không giảm nhiều hơn được bằng cách này. Muốn giảm thêm phải đổi cấu trúc mạng (ví dụ tích chập tách kênh, khảo sát ở Nội dung 3) [tự tính].
  - Bộ nhớ: 5.344 từ + 264 B bias = 5.608 ≤ 6.144 → **3 khối**. Dense mỗi bước của bộ tăng tốc kiểu mới còn 1.024 MAC thay vì 2.048; toàn cửa sổ (E2) 226.304 MAC [tự tính].
- **Lưu ý khi 4 làn:** Dense xếp `g·F + i`, 4 lớp vừa 1 nhóm 4 làn. Còn 2 lớp thì vẫn là 512 từ × 32 bit, **không tiết kiệm được** nếu không đổi cách xếp.
- **Giữ bỏ phiếu 2/3 FALL:** argmax chỉ còn trả về 0 hoặc 1. Driver phải **đổi mã 1 → 3 (`CLS_FALL`) trước khi gọi `safety_on_result()`**. Như vậy `safety.c`, `VOTE_K = 2`, `VOTE_N = 3` và trang web (mã 0/3, tô đỏ khi 3) giữ nguyên. Quên bước này thì FALL hiện thành "lớp 1" màu xanh và **không bao giờ có phiếu ngã**.
- Xóa nhánh IMPACT không còn dùng: `safety.c:45-46`, `app_core.c:70`.
- **Quyết định thiết kế (đã chốt):** bỏ hẳn lớp IMPACT để dành; ghi vào SPEC mới (mục 9). Số lớp đầu ra để thành tham số RTL, sau này muốn thêm lớp chỉ cần đổi tham số và huấn luyện lại.
- **File:** mô hình và `export_mem.py`; bộ tăng tốc (số đầu ra Dense, argmax 2 lớp); golden; vector kiểm thử; `selftest_expect_*.h`.
- **Pass:** khớp bit; ROM + bias = 3 pROM; chuỗi kết quả 0/1 cho máy trạng thái → 2/3 FALL vẫn vào SUSPECT.

### O4. Thay SPI, parser, CRC và khung kết quả bằng cổng APB (−~800 logic)

Bỏ `spi_slave.v`, `packet_parser_v3.v`, `crc16_ccitt.v`, `result_frame_v3.v`, `spi_result_tx_v3.v`, tiết kiệm 742 LUT + 57 ALU + 513 FF [tổng hợp]. Thay bằng một khối `cnn_apb.v` (Verilog-2001) gắn vào **APB2 Master 1 (0x40002400, 256 B)** [IPUG932 bảng 3-2]. Vì bộ tăng tốc kiểu mới chỉ cần mẫu mới, **mỗi bước M3 chỉ ghi ~~96 B~~ 144 B** (x[0..3] + x[108..127], 24 mẫu × 6 kênh; sửa 11/11, mục 15). Bảng thanh ghi chốt ở `docs/spec_nckh.md` A.7 (thêm STATUS bit3 VALID, 0x38 NEW_PTR).

| Offset | Tên | R/W | Nội dung |
|---|---|---|---|
| 0x00 | `ID` | R | Hằng số phiên bản (M3 tự kiểm khi khởi động) |
| 0x04 | `CTRL` | W | bit0 START (xử lý bước trượt vừa nạp), bit1 SOFT_RST (xóa đệm) |
| 0x08 | `STATUS` | R | bit0 BUSY, bit1 DONE (đọc thì xóa), bit2 WR_REJECT (ghi khi đang bận) |
| 0x0C | `CLASS` | R | Lớp argmax |
| 0x10 | `SCORE` | R | `clamp((top1 − top2) >> SCORE_SHIFT, 0, 127)`, như A.7 cũ |
| 0x14–0x18 | `LOGIT0..1` | R | INT32, 2 lớp: NORMAL, FALL (O3) |
| 0x1C–0x20 | – | – | Để trống |
| 0x24 | `CYCLES` | R | Số chu kỳ của bước vừa xử lý (để đo độ trễ) |
| 0x28 | `NEW_DATA` | W | Ghi 1 byte mẫu mới (thứ tự vị trí trước như A.6 cũ); con trỏ trong tự tăng, về 0 khi START; đủ ~~96~~ **144** byte (x[0..3] + x[108..127]) thì ghi START |
| 0x2C | `WARM` | R | Số bước trượt đã có trong đệm (0…8); kết quả chỉ hợp lệ khi = 8 |
| 0x30 | `WROM_ADDR` | R/W | (tùy chọn, O7) địa chỉ ROM trọng số |
| 0x34 | `WROM_DATA` | R | (tùy chọn, O7) đọc 1 từ ROM, `WROM_ADDR` tự tăng |

- **Báo hoàn tất bằng ngắt:** nối DONE vào **USER_INT0** [IPUG932 mục 3.2]; không phải hỏi vòng.
- Không cần CRC: bus nằm trong chip.
- **Tự kiểm khi khởi động:** M3 ghi `SOFT_RST`, nạp 8 bước dữ liệu cố định, rồi so CLASS và LOGIT với bảng kỳ vọng.
- Có thể ghép 4 byte mỗi lần ghi 32 bit (24 lần thay vì 96) nếu cần nhanh hơn; khi đó cổng APB cần một bộ đệm nhỏ.
- **Pass:** testbench mô phỏng giao dịch APB, chạy 50 bước liên tiếp, khớp bit với golden; ghi khi `BUSY = 1` bị từ chối; `WARM` đúng; tổng hợp không còn các khối SPI.

### O5. Một xung nhịp, không PLL

- **Hai tầng chỉ tiêu thời gian:**
  - **Bắt buộc (thời gian thực):** mỗi bước trượt xử lý xong trước bước kế tiếp, tức **0,32 s**.
  - **Hiệu năng bộ tăng tốc:** suy luận mỗi bước **≤ 6 ms ở xung nhịp thạch anh, không dùng PLL**; báo cáo kèm số MAC và năng lượng mỗi lần, so với chạy phần mềm trên lõi ARM. Giữ con số 6 ms của bản dự thi để so sánh được, nhưng lần này đạt **không cần PLL**.
- E2 cũ: 258.080 chu kỳ (mô phỏng 14/10) → ~9,6 ms ở 27 MHz. Bộ tăng tốc kiểu mới khoảng 35–45 nghìn chu kỳ → ~1,3–1,7 ms ở 27 MHz [ước tính]. Bộ tăng tốc mới đạt ≤ 6 ms ở 27 MHz với dư khoảng 3,5 lần [ước tính]; cả hai đều dư rất xa so với 320 ms.
- Vì vậy **cả M3 và FPGA chạy chung xung nhịp thạch anh**:
  - Không cần tạo PLL họ GW1NS (bỏ rủi ro tên primitive và Fmax).
  - Một miền xung nhịp, nên cổng APB không phải đồng bộ hóa tín hiệu giữa hai miền.
  - Xung nhịp thấp thì công suất động thấp hơn.
- Thạch anh trên Tang Nano 4K là **27 MHz** (U12, vào chân 45) [sơ đồ NL], khớp cấu hình LiteX và `.cst` mẫu của Sipeed. Các con số ở 27 MHz trong mục này dùng được.
- Nếu M3 ở tần số này không kịp phục vụ web và lệnh AT (ít khả năng), dùng PLL **chỉ cho M3**. Khi đó M3 và FPGA thành **hai miền xung nhịp**: cổng APB phải đồng bộ hóa START/DONE và dữ liệu giữa hai miền (thêm vài chục FF [ước tính]), và dòng 11 của bảng tóm tắt phải sửa theo.
- **1 làn MAC là thiết kế chuẩn** (bớt LUT và bộ nhân). 4 làn chỉ dùng cho thí nghiệm năng lượng ở Nội dung 5: so "chạy nhanh rồi nghỉ" với "chạy chậm".

### O6. I²C không tốn LUT

I²C master của EMPU là **khối mềm trong FPGA**: bật cùng SPI master thì tốn thêm 423 LUT và 263 FF [IPUG932 bảng 1-2]. Vì vậy **điều khiển chân I²C bằng phần mềm trên GPIO của M3** (0 LUT). Với 100 Hz × 14 byte ở 400 kHz, mỗi lần đọc mất khoảng 0,4 ms [ước tính]. Chỉ dùng IP I²C nếu cách phần mềm không ổn định; logic vẫn còn đủ.

### O7. Tiết kiệm flash cho CNN chạy bằng phần mềm (tùy chọn)

Phương án đối chứng (Nội dung 5) chạy cùng mạng trên M3 cần khoảng 5,6 KB trọng số và bias (5.344 trọng số + 66 bias × 4 B, sau O3) [tự tính]. Flash M3 chỉ 32 KB và đã phải chứa web server (mục 7). Nếu không đủ chỗ, cho M3 **đọc trọng số từ ROM trong FPGA** qua `WROM_ADDR`/`WROM_DATA` khi bộ tăng tốc rảnh. Hai phương án dùng chung đúng một bộ trọng số. Tốn thêm một bộ chọn địa chỉ ROM, khoảng vài chục LUT [ước tính]. Báo cáo phải ghi rõ: đọc qua bus làm thời gian CNN phần mềm chậm hơn một chút so với đọc từ flash.

### O8. Dự phòng

1. **Thiếu BSRAM hoặc Tang Nano 4K không chạy nổi cả M3 lẫn bộ tăng tốc → STM32 + Tang Nano 4K** (phương án dự phòng chính, khớp đề xuất NCKH):
   - STM32 (lõi ARM Cortex-M rời) làm việc của M3: đọc cảm biến, tiền xử lý, tầng 2, máy trạng thái, web server qua module WiFi (lệnh AT).
   - Phần FPGA của Tang Nano 4K chỉ chạy bộ tăng tốc. Không tạo Gowin_EMPU (lõi M3 bỏ trống), nên **10 khối BSRAM dành hết cho bộ tăng tốc**.
   - Giao tiếp STM32 – FPGA bằng SPI qua chân của board. Khi đó lõi M3 bỏ trống nên 7 chân bank 1 (3,3 V) vốn dành cho UART và I²C của M3 (mục 12.5.2) được giải phóng để làm SPI; cảm biến, WiFi và USB–UART nối thẳng vào STM32. Giao thức vẫn chỉ gửi 96 B mẫu mới mỗi bước.
   - Firmware C gần như dùng lại được bản viết cho M3 (cùng họ ARM Cortex-M); chỉ khác lớp driver ngoại vi.
   - Dòng STM32 cụ thể chọn khi cần, ví dụ dòng STM32F4 (Cortex-M4 có FPU) **[CẦN XÁC MINH khi chọn]**.
2. Đặt dữ liệu của M3 lên HyperRAM 64 Mb trên board. Tốn thêm bộ điều khiển: 1.163 LUT, 1.008 FF theo [IPUG932] bảng 1-2.
3. Đổi sang N2-S (8-16-16 kênh, 3.184 trọng số): ROM khoảng 2 khối. **Đổi mô hình**; Macro-F1 mới chỉ có trên dữ liệu D1, dao động theo seed.
4. Trọng số INT4: không khuyên dùng (đổi số học A.5, rủi ro độ chính xác).

### O9. Tách phần phụ thuộc vị trí đeo thành cấu hình — **chốt làm**

Mục đích: bắp tay là vị trí chốt; chuyển sang vị trí khác (cổ tay, thắt lưng…) chỉ cần **thêm một bộ cấu hình và huấn luyện lại**, RTL và phần lớn firmware giữ nguyên.

| Thành phần phụ thuộc vị trí | Đặt ở đâu (tên tạm) |
|---|---|
| Ma trận đổi trục cảm biến theo cách lắp | `fw_arm/src/vi_tri_deo.h` và cấu hình Python tương ứng |
| Danh sách dấu hiệu tầng 2 được bật và ngưỡng của từng dấu hiệu | `vi_tri_deo.h` |
| Điều kiện nghiêng ở bước bất động (`az_tb`, bật/tắt) | `vi_tri_deo.h` |
| Tên bộ trọng số mạng đã huấn luyện cho vị trí | cấu hình Python + `export_mem.py` |

Cấu hình mặc định là **bắp tay**. **Pass:** đổi vị trí chỉ phải sửa file cấu hình và trọng số, không sửa code khác.

## 7. Firmware M3

### 7.1 Chuyển từ firmware cũ

| Module | Nguồn chuyển sang | Ghi chú |
|---|---|---|
| Đọc MPU6050, khởi động lại cảm biến | `firmware/src/board.c`, `imu.c`, `i2c0.c` (SN32F407; chép vào `fw_arm/src/tham_khao/`) | Viết lại cho GPIO của EMPU; I²C bằng phần mềm (O6); tự gỡ theo mục 7.5 |
| Hiệu chỉnh con quay | `firmware/src/gyro_cal.c` | Có dùng số thực, đổi sang số cố định (M3 không có FPU) |
| Tiền xử lý | `esp32/nguc_nga_hub/preprocess.c` | Có dùng số thực; đổi trục theo cấu hình vị trí (mặc định bắp tay, O9); mỗi 0,32 s xuất 16 mẫu mới |
| Tầng 2: 4 dấu hiệu | `nga.c` | Xem 7.2 |
| Máy trạng thái, tư thế, nút | `safety.c`, `posture.c`, `pwr_btn.c` | Đã là số nguyên. Điều kiện nghiêng `az_tb` (`safety.c:135-137`) thiết kế cho đầu/thân, phải xem lại cho bắp tay (đưa vào O9) |
| Giao tiếp bộ tăng tốc | thay `spi_frame.c` bằng driver APB | Bảng thanh ghi ở O4: ghi 96 B `NEW_DATA` + START; DONE qua USER_INT0; kiểm `WARM`; đổi mã 1 → 3 (O3) |
| Web server | mới, dựa trên phần web của ESP32 | Xem 7.3 |
| Tự kiểm | `selftest_expect_v3_e2_n2.h` | Xuất lại theo bộ tăng tốc kiểu mới (8 bước cố định) |

Các file trên tổng cộng khoảng 1.400 dòng C (đếm bằng `wc -l`).

### 7.2 Tầng 2 (4 dấu hiệu vật lý, `nga.c`)

- Chạy trên M3, **tách nhánh từ mẫu thô 100 Hz** (trước khi lọc và giảm mẫu), song song với tầng 1.
- `nga_mau()` mỗi mẫu; `nga_khoi()` mỗi 32 mẫu, lưu lịch sử 36 khối (11,5 s); `nga_bat_dau()` khi vào SUSPECT; `nga_buoc()` mỗi 10 ms, tối đa 4 s. Đủ 4 dấu hiệu → xác nhận; thiếu → `safety_bo_nghi()`, về MONITOR.
- RAM: lịch sử 36 × (4 + 4 + 1 + 1 B) ≈ 360 B cộng vài biến trạng thái [tự tính từ `nga.h`]. Không ảnh hưởng ngân sách BSRAM.
- Ngưỡng và danh sách dấu hiệu đặt trong cấu hình vị trí đeo (O9).
- **Phải viết lại cho bắp tay:**

| Dấu hiệu | `nga.c` hiện tại (đeo ngực) | Đeo bắp tay |
|---|---|---|
| 1 | trước đó ngực thẳng (`posture.c`) | pha rơi trước va chạm (độ lớn gia tốc giảm) |
| 2 | va chạm ≥ 2,5 g | va chạm mạnh, ngưỡng chọn lại |
| 3 | quay ≥ 220 °/s | quay cánh tay lớn, ngưỡng chọn lại |
| 4 | ngực nghiêng ≥ 45° | bất động sau va chạm |

Ngưỡng ngực chọn từ log 28/10 của một người; mọi ngưỡng bắp tay chọn trên dữ liệu bắp tay nhiều người, tách khỏi tập kiểm tra.

### 7.3 Web server qua module WiFi (modem AT)

- Module dùng firmware AT gốc. M3 gửi lệnh AT qua UART: kết nối WiFi, `AT+CIPMUX=1`, `AT+CIPSERVER=1,80` (theo hướng dẫn của Ai-Thinker; kiểm lại với phiên bản firmware thực tế bằng `AT+GMR`).
- Đường dẫn giữ như trang hiện tại (`esp32/nguc_nga_hub/trang_web_html.h`):
  - `GET /` → trang web, lưu **dạng nén gzip** trong flash M3 (trình duyệt tự giải nén).
  - `GET /data` → JSON trạng thái (trình duyệt hỏi mỗi 300 ms).
  - `POST /huy` → hủy cảnh báo; `POST /bat` → bật lại hệ thống.
- **Lệnh AT viết dạng máy trạng thái, không chặn.** Đọc cảm biến, hai tầng và còi có ưu tiên cao hơn. Module treo thì M3 kéo chân RST của module qua Q1 (`wifi_rst`, mục 12.5) để khởi động lại; EN nối cố định 3,3 V.

### 7.4 Ngân sách bộ nhớ M3 (không dùng RTOS) [ước tính]

- **Flash (32 KB):** firmware 10–20 KB + trang web nén ~3 KB (trang gốc ~9 KB, đếm bằng `wc -c`) + mã HTTP và lệnh AT 3–5 KB → **~16–28 KB**. CNN phần mềm thêm 2–4 KB mã; trọng số đọc qua O7 nếu flash không đủ.
- **RAM:** ngăn xếp ~1 KB, đệm 16 mẫu mới ~0,1 KB, bộ đệm cảm biến ~0,5 KB, bộ đệm TCP/lệnh AT ~1–2 KB, tầng 2 ~0,4 KB, trạng thái ~0,5 KB → **~3,5–4,5 KB**. CNN phần mềm (đối chứng) cần thêm cửa sổ 768 B + đặc trưng ~2 KB → **~6–7 KB**, vừa SRAM 8 KB.
- Thu dữ liệu 100 Hz cho bộ dữ liệu: dữ liệu thô 1,2 KB/s (100 × 6 × 2 B) [tự tính]. **Thu ngã thật qua WiFi**: M3 gửi luồng TCP qua ESP-01S tới máy tính (như log TCP 3333 của bản dự thi), để người đeo không bị dây nối khi ngã; luồng này tách khỏi trang web. USB–UART chỉ dùng khi dựng hệ thống ban đầu và gỡ lỗi trên bàn.

### 7.5 Chống mất cảm biến: tự gỡ, không phải rút pin — **bắt buộc**

Ở bản dự thi, MPU6050 có lúc treo và người dùng phải rút pin khoảng 5 s mới chạy lại. Lỗi này đã gặp ít nhất 3 lần trong ngày 28/10 và một lần ngày 3/11 (theo `PROJECT_MASTER.md` nhật ký 28/10; `docs/bao_cao/2026-11-03_phat_lai_tren_board/README.md` của bản dự thi; tóm tắt ở `docs/tham_khao/bang_chung_du_thi.md` mục 1). Bản mới phải **tự phát hiện và tự gỡ trong vài giây, không cần người dùng làm gì**.

#### 7.5.1 Bằng chứng và nguyên nhân

| Bằng chứng (bản dự thi) | Nhận định | Nhãn |
|---|---|---|
| 28/10: mất cảm biến 3 lần, **đều sau khi cử động**; lúc đó SDA = SCL = 1, cấu hình lại vẫn không trả lời | Không phải bus I²C bị giữ thấp; giống **mất nguồn chập chờn hoặc lỏng dây** ở module | log board 28/10 |
| Cảm biến treo thì gỡ bus (9 xung SCL) và cấu hình lại không có tác dụng; chỉ cắt nguồn mới hết | Chip bị kẹt ở trạng thái nửa khởi động. Datasheet yêu cầu khi cấp nguồn **mọi chân đều bắt đầu từ 0 V**, VDD tăng đều trong ≤ 100 ms; nguồn chập chờn vi phạm điều này | `firmware/src/app_main.c`; [MPU6050] mục 6.3 và sơ đồ cấp nguồn trang 23; cơ chế là suy luận |
| Thử cắt nguồn bằng GPIO25 của ESP32 (300 ms) không thành công | Khi cắt VCC, SDA/SCL vẫn bị điện trở kéo lên 3,3 V của board MCU giữ ở mức cao, nên chip **được cấp điện ngược qua chân I²C** (datasheet: chân SCL, SDA, INT không được cao hơn VDD + 0,5 V). 300 ms cũng có thể quá ngắn để tụ trên module xả hết | `nguc_nga_hub.ino` dòng 48–57; [MPU6050] mục 6.9; suy luận, **[CẦN XÁC MINH]** |
| Nghi sụt áp 3,3 V khi ESP32 phát WiFi | Chưa đo | [CẦN XÁC MINH] |
| Cảm biến nối qua dây cắm dupont giữa 3 board, đất chung qua dây dài | Dây dễ lỏng khi đeo và cử động | cấu hình bản dự thi |

Sau mục 13, các file bản dự thi trích ở bảng trên (`PROJECT_MASTER.md`, `nguc_nga_hub.ino`, `app_main.c`) không còn trên máy: chúng vẫn nằm trong lịch sử git của repo `danhle396/fpga2026gowin` (commit `f26bea8`), và nội dung cần dẫn được tóm tắt vào `docs/tham_khao/bang_chung_du_thi.md` (mục 13.3).

#### 7.5.2 Phòng ngừa bằng phần cứng

1. **Bỏ dây dupont** (nguyên nhân mất cảm biến ở bản dự thi): Tang Nano 4K, ESP-01S và GY-521 cắm vào **header cái hàn trên PCB** (mục 12.5.7), có vỏ ép giữ.
2. **Nguồn cảm biến tách khỏi WiFi:** ESP-01S dùng ổn áp riêng (mục 12.5.4); GY-521 lấy 3,3 V của board qua Q3.
3. **Tụ 10 µF + 100 nF sát chân VCC của GY-521** để chịu sụt áp ngắn.
4. **Công tắc nguồn thật (Q3 S8550 PNP, Q2 S8050 lái)**, không cấp nguồn cảm biến từ chân GPIO.
5. **Điện trở xả 4,7 kΩ từ VCC cảm biến (sau Q3) xuống GND**, để khi cắt nguồn VCC về 0 V nhanh. Với khoảng 10 µF tụ, hằng số thời gian ≈ 47 ms [ước tính; tụ có sẵn trên GY-521 **CẦN XÁC MINH**]. Tốn thêm 0,7 mA khi chạy [tự tính].
6. Chạy thử gây treo (mục 7.5.5) trên module **trước khi lắp lên PCB**: chip MPU6050 trên module giá rẻ có thể không đồng đều chất lượng **[CẦN XÁC MINH]**.

#### 7.5.3 Phát hiện nhanh

Firmware coi là mất cảm biến khi có **một trong ba** dấu hiệu:
- quá **50 ms** (5 chu kỳ mẫu) không có ngắt INT;
- **3 lần đọc I²C lỗi liên tiếp** (không ACK, hoặc WHO_AM_I khác 0x68);
- **6 trục giống hệt nhau trong 1 s** (chip trả dữ liệu cũ; người đeo thật không bao giờ đứng yên tuyệt đối tới mức đó) [ước tính, kiểm bằng dữ liệu thật].

So với bản dự thi (báo sau 3 s), phát hiện nhanh hơn khoảng 60 lần [tự tính].

#### 7.5.4 Tự gỡ theo bậc (không chặn máy trạng thái)

| Bậc | Việc | Thời gian [ước tính] | Chuyển bậc khi |
|---|---|---|---|
| 1 | Gỡ bus: 9 xung SCL + STOP (I²C mềm nên làm dễ), đọc WHO_AM_I, ghi lại cấu hình, đọc lại kiểm | < 5 ms | Vẫn lỗi |
| 2 | Reset mềm: ghi `PWR_MGMT_1 = 0x80` (DEVICE_RESET), chờ 100 ms, cấu hình lại | ~ 0,1 s | Vẫn lỗi |
| 3 | **Cắt nguồn đúng quy trình** (dưới đây) | ~ 1,2 s | Vẫn lỗi |
| 4 | Báo **MẤT CẢM BIẾN** (còi + web), vẫn thử lại bậc 3 mỗi 10 s | – | Hồi phục thì về bình thường |

**Quy trình cắt nguồn (bậc 3), theo yêu cầu cấp nguồn của datasheet:**
1. Đưa SCL, SDA ra **mức 0** (không thả): chặn điện cấp ngược qua R29, R30 (kéo lên 2,5 V trên board, luôn có điện) và qua điện trở kéo lên của GY-521.
2. `sens_pwr = 0` (Q3 tắt), giữ **≥ 1 s** để VCC cảm biến về 0 V qua điện trở xả. Con số 1 s chọn lại sau khi đo thử 0,5 / 1 / 2 / 5 s **[CẦN XÁC MINH]**.
3. `sens_pwr = 1`, chờ **100 ms** (bằng thời gian tăng áp tối đa trong datasheet; Rev 3.4 không ghi số "Start-Up Time for Register Read/Write") **[CẦN XÁC MINH]**.
4. Thả SCL, SDA; gỡ bus (bậc 1); WHO_AM_I = 0x68; ghi cấu hình; đọc lại kiểm.

**Trong lúc gỡ:**
- Không đưa mẫu giả vào CNN. Xóa đệm đặc trưng, chạy lại 8 bước khởi động (`WARM`, ≈ 2,6 s) rồi mới bỏ phiếu lại.
- Máy trạng thái giữ trạng thái hiện tại. Nếu đang ở CẢNH BÁO / BÁO ĐỘNG thì **không tự hủy** vì mất cảm biến.
- Tổng thời gian "mù" ở bậc 3 khoảng 4 s [ước tính]: 1,2 s gỡ + 2,6 s khởi động lại cửa sổ.
- Web và log ghi số lần từng bậc (`go_bus`, `reset_mem`, `cat_nguon`, `mat_cb`) để biết lỗi hay gặp ở bậc nào.
- Bật watchdog của M3: nếu firmware treo thì M3 tự khởi động lại, và lúc khởi động luôn chạy bậc 3 một lần trước khi đọc cảm biến.

#### 7.5.5 Kiểm chứng (Pass)

| Thử | Cách làm | Pass |
|---|---|---|
| Gây treo bằng nguồn chập chờn | Firmware cố tình cắt sai: tắt Q3 trong 5–50 ms rồi bật lại, **không** đưa SCL/SDA về 0 trước, 20 lần | **20/20 tự hồi phục**, không rút pin; ghi bậc gỡ được |
| Gây lỗi bus | Chập SDA xuống GND 1 s, 20 lần | 20/20 hồi phục ở bậc 1 hoặc 2 |
| Đeo thật, cử động mạnh | Vung tay, chạy, ngã lên đệm 30 phút | Không có lần nào phải rút pin; mọi lần mất (nếu có) tự hồi |
| Chạy dài có WiFi | 8 giờ, trang web mở liên tục | Đếm số lần từng bậc; không có bậc 4 |
| Đo sụt áp (nếu mượn được dao động ký) | Đo VCC của GY-521 khi ESP-01S phát | Ghi biên độ sụt **[CẦN XÁC MINH dụng cụ]** |

Thử "gây treo" phải làm **trước** khi thu dữ liệu ngã, vì đó là lúc thiết bị bị rung và va đập nhiều nhất.

## 8. Kiểm chứng (giữ tháp 4 cấp)

| Cấp | Công cụ | Pass |
|---|---|---|
| 1. Mô hình tham chiếu | golden E2 (`golden_model.py`, `cosim.py`) + **golden tăng dần mới** (`golden_inc.py`); `export_mem.py` | Golden tăng dần khớp bit golden E2 trên 50 cửa sổ liên tiếp; xuất `.mem` mới (bias trong ROM, Dense 512 → 2) |
| 2. Mô phỏng RTL | Icarus 0.9.7 `-g2001`, `rtl/scripts/test.sh` (bỏ phần SPI cũ), `tb_inc.v` + testbench APB | 50 bước liên tiếp khớp bit mọi tầng (1 làn; 4 làn khi thí nghiệm); phép âm: ghi khi đang bận bị từ chối, kết quả trước khi `WARM = 8` bị bỏ |
| 3. Netlist sau đặt – nối dây | `rtl/scripts/gl.sh` viết lại theo mẫu cũ cho part GW1NSR-LV4C (thư viện mô phỏng GW1NS, testbench APB thay `tb_proto`, không còn rPLL) | 50 bước khớp bit |
| 4. Trên board | Tự kiểm khi khởi động + đối chiếu khi chạy thật | CLASS và LOGIT khớp bảng kỳ vọng; thời gian suy luận = `CYCLES` / 27 MHz, độ trễ đầu–cuối đo bằng bộ đếm thời gian của M3; cả hai in qua UART1 (USB–UART) |

## 9. Tài liệu kỹ thuật của đề tài mới

Sau khi sắp xếp thư mục (mục 13), bản dự thi và `PROJECT_MASTER.md` bị xóa khỏi máy, giao thức đồng bộ 3 file cũ không còn áp dụng. Đề tài mới dùng:

- **File theo dõi chính** `PROJECT_NCKH.md` (gốc `NCKH2627/`) (tên tạm): kiến trúc, bảng tài nguyên, nhiệm vụ, trạng thái, bằng chứng.
- **SPEC mới** `docs/spec_nckh.md`, dựa trên `docs/tham_khao/spec_v3_du_thi.md`:
  - Giữ: số học A.5 (INT8, tích lũy INT32, dịch bit, cắt 0…127).
  - Viết lại:
    - Tiền xử lý chạy trên M3 (A.2).
    - Lớp đầu ra còn 2: NORMAL, FALL (O3, A.4).
    - **Bộ nhớ và cách tính tăng dần** của bộ tăng tốc kiểu mới (thay A.6).
    - **Bảng thanh ghi APB** (thay A.7).
    - Điều kiện nghiêng cho bắp tay (A.8).
    - Tự kiểm 8 bước (A.10).
    - **4 dấu hiệu cho bắp tay** (A.12).
    - **Cấu hình vị trí đeo** (O9).
- **Đề xuất NCKH:** bản v3 (`De_xuat_NCKH_ban_mo_v3.docx`) đã khớp kiến trúc này ở mức khái quát; chỉ sửa nếu có thay đổi lớn.

## 10. Thứ tự thực hiện

| Bước | Việc | Phụ thuộc | Pass |
|---|---|---|---|
| S0 | Tổng hợp thử EMPU + `cnn_e2` ở xung nhịp thạch anh | – | Biết BSRAM của EMPU theo mức SRAM; primitive tổng hợp được; Fmax ≥ xung nhịp thạch anh |
| 0 | O3: xuất lại trọng số Dense 512 → 2 (66 bias); golden E2 hai lớp | – | Trên 50 cửa sổ, lớp argmax trùng golden 4 lớp (sau khi đổi mã 1 → 3) |
| 1a | Golden tăng dần (`golden_inc.py`) | 0 | 50 cửa sổ liên tiếp khớp bit golden E2 hai lớp |
| 1b | O1 bộ tăng tốc kiểu mới (RTL) | 1a | Khớp bit mọi tầng; đệm 1 SDPB |
| 2 | O2 gộp bias | 1b | Khớp bit; 3 pROM |
| 3 | O4 cổng APB (`NEW_DATA` 96 B, `WARM`, USER_INT0) + testbench APB | 1b, 2 | 50 bước khớp bit qua APB |
| 4 | O5 một xung nhịp, tổng hợp đầy đủ trên GW1NSR-LV4C | 3, S0 | Logic, BSRAM trong ngân sách mục 4; `CYCLES` / tần số thạch anh ≤ 6 ms mỗi bước (≪ 0,32 s) |
| 5 | Firmware M3: cảm biến, tiền xử lý, driver APB (đổi mã 1 → 3), tự kiểm, **cấu hình vị trí đeo (O9)**; chân theo mục 12.5 (`chan.h`) | 4 | Tự kiểm đạt trên board; 100 Hz không mất mẫu trong 1 giờ; 2/3 FALL vẫn vào SUSPECT; **thử gây treo cảm biến 20/20 tự hồi phục (mục 7.5.5)** |
| 6 | Module WiFi: lệnh AT + web server trên M3 | 5 | Trình duyệt nhận `/data` mỗi 300 ms; nút Hủy chạy; rút module ra thì còi vẫn kêu khi ngã |
| 7 | Tầng 2 cho bắp tay: viết lại dấu hiệu 1, 4; chọn ngưỡng | 5 + dữ liệu bắp tay | Ngưỡng chọn trên tập riêng; log `N,…` đầy đủ |
| **7b** | **Xóa mô hình thử ngẫu nhiên và MỌI dữ liệu giả lập** ngay khi có dữ liệu bắp tay thật và mô hình huấn luyện từ đó (danh sách ở mục 15.6). Từ đó chỉ dùng dữ liệu thật nhóm tự thu | 7 + mô hình bắp tay | Tìm trong repo không còn `THU_NGAU_NHIEN`, `luong_tong_hop`, `tao_mo_hinh_thu`, `mo_hinh_thu`, `cua_so(`, LFSR, vector ngẫu nhiên; `rtl/scripts/test.sh` chạy hết trên dữ liệu thật và PASS; UART in `mo_hinh=<tên mô hình bắp tay>` |
| 8 | CNN phần mềm trên M3 (đối chứng) | 5 | Khớp bit với golden; đo thời gian và năng lượng |
| H1 | Bộ thử phần cứng trên bàn: Tang Nano 4K + ESP-01S + GY-521 + USB–UART (mục 12.4 bước 2) | S0 | Nạp được M3; đọc GY-521 100 Hz; ESP-01S trả lời lệnh AT |
| **H1b** | **Khi board đã chạy thật: chỉ dùng thông số đo trên board; xóa các thông số mô phỏng, giả lập, ước tính** khỏi tài liệu (danh sách ở mục 15.8). Làm dần theo từng phép đo (H1, bước 5, 6, 8, H2, H3) | H1 | Mỗi con số trình bày về thời gian, tài nguyên chạy thật, năng lượng, độ chính xác đều có nhãn [board] và log trong `docs/bao_cao/`; tìm trong tài liệu trình bày không còn số mang nhãn [mô phỏng], [giả lập], [ước tính] thay cho phép đo đã có |
| H2 | Thử gây treo cảm biến trên bộ thử (mục 7.5.5) | H1, 5 | 20/20 tự hồi phục |
| H3 | Đo dòng từng nhánh (mục 12.2); chọn pin, sạc, boost | H1 | Có dòng trung bình và dòng đỉnh từng nhánh |
| H4 | Vẽ, đặt và lắp PCB (mục 12.5) | H2, H3 | Đo thông mạch và điện áp từng nhánh trước khi cắm module; chạy lại tự kiểm và thử gây treo trên PCB |

## 11. Rủi ro mở

| Rủi ro | Ảnh hưởng | Cách kiểm hoặc xử lý |
|---|---|---|
| Bộ tăng tốc kiểu mới không khớp bit hoặc trễ tiến độ (**không còn phương án lui về E2**) | Chậm cả các bước sau, vì đây là lõi tính toán | Làm bước 1a (golden tăng dần) **sớm nhất**, trước mọi RTL; nếu không khớp thì sửa cách tính lại các vị trí sát mép trong golden cho tới khi khớp, rồi mới viết RTL |
| 4 BSRAM của EMPU ứng với SRAM nhỏ hơn mức firmware cần | Thiếu RAM hoặc thiếu BSRAM | S0; dùng 2 khối dư; O8 |
| SDPB không hỗ trợ trên GW1NS-4 | Phải viết lại bộ nhớ | S0 |
| Flash 32 KB không đủ cho firmware + web + CNN phần mềm | Phải cắt bớt | Nén trang web; O7; không dùng RTOS |
| M3 ở 27 MHz (đã xác nhận từ sơ đồ nguyên lý) không kịp phục vụ web | M3 chậm | Đo ở bước 6; nếu cần, PLL chỉ cho M3 (O5) |
| Hết chân 3,3 V (bank 1 dùng hết 7 chân) | Không thêm được thiết bị 3,3 V | Dùng chân bank 3 qua transistor; hoặc chân 1, 47, 48 của flash SPI (mục 12.5.3) |
| Nối nhầm tín hiệu 3,3 V vào bank 3 (1,8 V), hoặc cắm USB và nguồn pin cùng lúc | Hỏng chip hoặc nguồn | Làm theo lưu ý mục 12.5.4 và trang 3 của sơ đồ chân; kiểm bằng đồng hồ trước khi cấp nguồn |
| Lệnh AT treo, module mất WiFi | Mất trang web | Máy trạng thái không chặn; reset module qua GPIO; còi vẫn là cảnh báo chính |
| Phiên bản firmware AT khác nhau giữa các lô | Lệnh không chạy | Kiểm `AT+GMR` và thử `AT+CIPSERVER` ngay khi nhận hàng |
| Nguồn 3,3 V cho module WiFi (khoảng 170 mA khi phát [ESP-01S]; Ai-Thinker khuyên nguồn > 500 mA) | Sụt áp, khởi động lại | Ổn áp 3,3 V riêng + tụ lọc gần module (mục 12.5.4); không lấy 3,3 V từ Tang Nano 4K |
| MPU6050 treo, chỉ cắt nguồn mới hết (đã gặp ở bản dự thi) | Thiết bị không bảo vệ, phải rút pin | Mục 7.5: phòng ngừa phần cứng + tự gỡ 4 bậc; thử gây treo 20/20 trước khi thu dữ liệu ngã |
| MPU6050 ±16 g bão hòa khi ngã ở bắp tay | Mất đỉnh va chạm | Thu thử 1–2 người trước khi thu đại trà |
| Dấu hiệu 1 và 4 của tầng 2 chưa định nghĩa cho bắp tay | Tầng 2 không xác nhận đúng | Bước 7, cần dữ liệu bắp tay |
| Thiếu dụng cụ đo dòng an toàn (đã bỏ khỏi kinh phí đề xuất) | Không làm được phần năng lượng (Nội dung 5); đo bằng đồng hồ vạn năng thang dòng dễ chập | Theo mục 12.2: INA226 đọc qua I²C của M3 + cầu đo jumper trên PCB, hoặc đồng hồ đo USB; mượn hoặc mua **[CẦN XÁC MINH]** |
| Header cái lỏng khi va đập, ngã | Mất cảm biến hoặc WiFi tạm thời | Header chân tròn + vỏ ép giữ; mục 7.5 tự gỡ cảm biến; thử đeo vung tay 30 phút (7.5.5) |
| Còi nhóm đang có chưa biết chủ động hay thụ động | Còi kêu sai hoặc nhỏ; còi cuộn dây khoảng 16 Ω hút dòng lớn | FPGA hỗ trợ cả hai chế độ (`gpio[8]`, mục 12.5.6); kiểm loại còi trước khi vẽ PCB; thêm điện trở nối tiếp nếu là còi cuộn dây (mục 12.5.4) |
| Bộ công cụ M3 của Gowin ít tài liệu | Chậm tiến độ firmware | Bắt đầu từ ví dụ trong `sipeed/TangNano-4K-example` |
| Bỏ lớp IMPACT (O3, đã chốt) | Muốn thêm lớp sau này phải sửa Dense, giao tiếp và huấn luyện lại | Ghi vào SPEC mới; số lớp đầu ra là tham số RTL |

## 12. Danh sách phần cứng

### 12.1 Mỗi thiết bị đeo

Cập nhật 11/11/2026: **Tang Nano 4K, ESP-01S và GY-521 cắm vào header cái hàn trên PCB** (tháo ra được, bảo vệ board, không dùng dây dupont). Transistor, điện trở, tụ, nút, công tắc **hàn thẳng lên PCB**. Toàn bộ là loại **chân cắm** để nhóm tự hàn bằng mỏ hàn thường.

| # | Nhóm | Thiết bị | Cụ thể | SL | Trạng thái | Ghi chú |
|---|---|---|---|---|---|---|
| 1 | Xử lý | Board FPGA + lõi ARM | **Sipeed Tang Nano 4K** (GW1NSR-LV4C: Cortex-M3 + FPGA) | 1 | Cần mua | Cần 5 V ±10 %, 0,5 A [datasheet]. **Tháo camera** đi kèm. Cổng USB-C chỉ để nạp code (mục 12.5) |
| 2 | Cảm biến | Gia tốc + con quay | **MPU6050, module GY-521** (một cảm biến) | 1 | Nhóm đã có | ±16 g, ±2000 °/s. AD0 nối GND (0x68), XDA/XCL để trống (mục 12.5.7) |
| 3 | Truyền thông | WiFi | **Ai-Thinker ESP-01S**, firmware AT gốc, dùng như modem | 1 | Cần mua | Dòng phát khoảng 170 mA [ESP-01S] |
| 4 | Cảnh báo | Còi | Còi 5 V nhóm đang có, lái qua **S8050 (Q4)** | 1 | Nhóm đã có | **Chưa biết chủ động hay thụ động [CẦN XÁC MINH]**. FPGA cho chọn mức cao liên tục hoặc sóng 2 kHz |
| 5 | Cảnh báo | Nút | Nút nhấn chân cắm (Hủy; giữ 3 s = SOS) | 1 | Cần mua | Nối song song nút KEY2 trên board (P6-22) |
| 6 | Chống treo cảm biến | Công tắc nguồn cảm biến | **S8550 (Q3, PNP)** + **S8050 (Q2, NPN)** lái | 1 bộ | Cần mua | Tự cắt nguồn gỡ treo MPU6050 (mục 7.5). Mạch: mục 12.5.4 |
| 7 | Điều khiển WiFi | Reset ESP-01S | **S8050 (Q1)** | 1 | Cần mua | Kéo chân RST của ESP-01S xuống khi cần reset |
| 8 | Linh kiện nhỏ | Điện trở 1/4 W | 1 kΩ × 2, 4,7 kΩ × 3, 10 kΩ × 1 | 6 | Cần mua | 4,7 kΩ gồm cả điện trở xả ở chân VCC GY-521 |
| 9 | Linh kiện nhỏ | Tụ | Tụ hóa 10 µF + tụ gốm 100 nF sát GY-521; tụ lọc cho ổn áp của ESP-01S | 1 bộ | Cần mua | Chân cắm |
| 10 | Linh kiện nhỏ | Hàng chân | Hàng 3 chân đực (TX, RX, GND) để cắm USB–UART khi gỡ lỗi | 1 | Cần mua | Thiết bị thật không cắm gì |
| 11 | Linh kiện nhỏ | Header cái cho module | Tang Nano 4K: 2 hàng 1 × 22 (P6, P7); ESP-01S: 1 hàng 2 × 4; GY-521: 1 hàng 1 × 8. Nên dùng loại chân tròn (kẹp chặt hơn) | 1 bộ | Cần mua | Bước chân 2,54 mm; kiểm lại số chân trên module thật **[CẦN XÁC MINH]** |
| 12 | Nguồn | Pin | LiPo 1S 3,7 V | 1 | Chưa chọn | **Dung lượng chọn sau khi đo dòng**. 1.000 mAh nặng khoảng 17,7–28,5 g (theo người bán) |
| 13 | Nguồn | Sạc | Module sạc LiPo 1S có mạch bảo vệ | 1 | Cần mua | Có thể dùng module "sạc + boost" gộp, thay luôn dòng 14 [CẦN XÁC MINH khi chọn mã] |
| 14 | Nguồn | Tăng áp | Module boost 3,7 → 5 V, ≥ 1 A | 1 | Cần mua | Pin không đủ 5 V cho Tang Nano 4K; còi và ổn áp WiFi cũng lấy 5 V. Tổng dòng đỉnh khoảng 0,8 A [ước tính] |
| 15 | Nguồn | Ổn áp WiFi | Ổn áp 3,3 V riêng, ≥ 500 mA | 1 | Cần mua | ESP-01S không lấy 3,3 V từ Tang Nano 4K |
| 16 | Nguồn | Công tắc nguồn | Công tắc gạt chân cắm, giữa pin và boost | 1 | Cần mua | Tắt khi cắm USB-C để nạp (không cắm USB và pin cùng lúc) |
| 17 | Cơ khí | PCB | Mạch in gắn tất cả linh kiện trên | 1 | Sắp vẽ | Theo mục 12.5 |
| 18 | Cơ khí | Đeo | Băng quấn bắp tay (dán gai), vỏ in 3D | 1 | Làm sau PCB | Chừa lỗ cho cổng USB-C để nạp code |

Khối lượng cả thiết bị ước khoảng 70–125 g [ước tính, chưa cân].

### 12.2 Chỉ dùng khi phát triển (không nằm trong thiết bị đeo)

| Thiết bị | Dùng để | Trạng thái |
|---|---|---|
| **Module USB–UART 3,3 V** (CH340 hoặc CP2102) | Xem log, thu dữ liệu trên bàn (thu ngã thật qua WiFi, mục 7.4), đọc `CYCLES` để đo thời gian suy luận, thử lệnh AT. Cắm vào hàng 3 chân trên PCB | **Bắt buộc**, cần mua (board không có UART qua USB) |
| **Dụng cụ đo dòng an toàn** | Đo dòng từng nhánh (Nội dung 5), chọn dung lượng pin. Đề xuất: module **INA226** (hoặc INA219) đọc qua I²C của M3, kết hợp cầu đo jumper trên PCB; đo tổng dòng bằng **đồng hồ đo USB** cắm giữa. Tránh dùng đồng hồ vạn năng thang dòng (dễ chập khi đo sai) | **Chờ chốt**; mượn hoặc mua (đã bỏ khỏi kinh phí đề xuất) |

### 12.3 Dự phòng

- Phương án dự phòng **STM32 + Tang Nano 4K** nếu S0 không đạt (Tang Nano 4K không đủ tài nguyên cho cả lõi M3 lẫn bộ tăng tốc): mua thêm 1 board STM32 khi cần, xem O8.

### 12.4 Thứ tự mua

1. Làm S0 và golden tăng dần trước (không cần board).
2. Mua bộ thử: Tang Nano 4K, ESP-01S, USB–UART. Thử lệnh AT, web từ M3, đọc GY-521.
3. Kiểm loại còi nhóm đang có; thử gây treo cảm biến (mục 7.5.5) trên GY-521 trước khi lắp lên PCB.
4. Mua linh kiện chân cắm (S8050, S8550, điện trở, tụ, nút, hàng chân, công tắc); kiểm thứ tự chân transistor theo datasheet của lô mua rồi mới vẽ footprint.
5. Đo dòng thật rồi mới chọn pin, module sạc và boost.
6. Vẽ và đặt PCB.

### 12.5 Sơ đồ chân và nối dây (dùng để viết `.cst`, top-level RTL và firmware)

Sơ đồ: `docs/so_do_chan_tang_nano_4k.drawio` (3 trang: bảng chân, header P6/P7, nối dây và nguồn), ảnh `so_do_chan_tang_nano_4k_trang1.png` … `trang3.png`. Tài liệu gốc chép ở `docs/datasheet/tang_nano_4k/`:

| File | Dùng để | Nguồn |
|---|---|---|
| `Tang_Nano_4K_3603_Schematic.pdf` | Chân nào nối gì trên board, điện áp từng bank, header P6/P7 | Sipeed, bản 3603, 20/8/2021 (dl.sipeed.com) |
| `Tang_Nano_4K_Datasheet.pdf` | Thông số board (5 V ±10 %, 0,5 A; 38 GPIO; BL702 làm JTAG) | Sipeed Datasheet v1.0, 7/8/2021 |
| `GW1NSR-LV4C_QFN48P_bang_chan.csv` | Bảng 48 chân của chip: tên, bank, chức năng cấu hình, cặp vi sai | Trích từ file gói `GW1NSR-4C/QFN48P.json` của Gowin EDA 1.9.11.03 trên máy |
| `ESP-01S_Specification_20250802.pdf` | Chân module WiFi, mức logic, dòng | Ai-Thinker |
| `IPUG932E_Gowin_EMPU_GW1NS-4C.pdf` | Cổng GPIO, UART0/1, APB2, USER_INT của lõi M3 | Gowin |
| `../MPU-6000_MPU-6050_Product_Spec_Rev3.4.pdf` | I²C, mức logic, INT của cảm biến | InvenSense (đã có trong repo) |

Nhãn trong mục này: **[sơ đồ NL]** = sơ đồ nguyên lý Tang Nano 4K bản 3603; **[Gowin QFN48P]** = bảng chân trong Gowin EDA; **[ESP-01S]**, **[MPU6050]**, **[IPUG932]** = các file trên.

#### 12.5.1 Những điều sơ đồ nguyên lý quyết định

| Điều | Hệ quả khi gán chân | Nguồn |
|---|---|---|
| Mã chip **GW1NSR-LV4CQN48PC6/I5** | Dùng mã này cho project Gowin (S0). Gowin EDA 1.9.11.03 bản Education chỉ có mã này cho GW1NSR-4C | [Gowin QFN48P], wiki Sipeed, LiteX |
| Thạch anh **27 MHz** (U12) vào chân **45** | Hết giả định ở O5: xung nhịp chung là 27 MHz | [sơ đồ NL], LiteX, `.cst` mẫu của Sipeed |
| Bank 0 và bank 1 = **3,3 V**; bank 2 = **2,5 V** (HDMI); bank 3 = **1,8 V** (nút, camera) | Tín hiệu nối với ESP-01S, USB–UART, MPU6050 phải ở bank 1. Bank 3 chỉ dùng cho nút có sẵn và lái transistor | [sơ đồ NL]; `key_blink.cst` của Sipeed ghi `BANK_VCCIO=1.8` cho chân 14, 15 |
| Chân 1, 2, 8, 9, 47, 48 nối **flash SPI P25Q32** | Không dùng (trừ khi thiếu chân, xem 12.5.3) | [sơ đồ NL] |
| Chân 3, 4, 6, 7 là **JTAG** nối BL702 | Không dùng | [sơ đồ NL] |
| BL702 **chỉ làm cầu JTAG**, không nối UART sang chip | Cần module USB–UART ngoài để gỡ lỗi và thu dữ liệu (đã có ở 12.2) | [sơ đồ NL] trang 2, datasheet Sipeed |
| Camera (đầu nối P3) dùng chung chân 16–23, 39–44, 46 | **Tháo camera** trước khi nối dây | [sơ đồ NL] |
| Chân 44, 46 (I²C của camera) có sẵn R29, R30 **4,7 kΩ kéo lên 2,5 V** | Dùng làm I²C cho MPU6050, không cần thêm điện trở kéo lên | [sơ đồ NL] |
| ESP-01S: mức cao vào ≥ **0,75 × 3,3 V ≈ 2,5 V**; RST, EN, IO0 có điện trở kéo lên trong module | Chân 1,8 V không lái thẳng được ESP-01S; RST lái qua transistor | [ESP-01S] mục 2, mục 5 |
| MPU6050: mức cao vào ≥ **0,7 × VLOGIC** (2,31 V khi VLOGIC = 3,3 V); I²C 400 kHz | Kéo lên của GY-521 (3,3 V) ghép với R29, R30 (2,5 V): mức cao khoảng 2,9 V [ước tính], vẫn đủ (mục 12.5.7) | [MPU6050] |

Kết quả: chân **3,3 V còn trống chỉ có 7 chân bank 1** (39, 40, 41, 42, 43, 44, 46), vừa đủ cho I²C (2), INT (1), UART0 với ESP-01S (2), UART1 gỡ lỗi (2). Các tín hiệu chỉ cần lái transistor (wifi_rst, sens_pwr, buz) đưa sang bank 3 (1,8 V).

#### 12.5.2 Bảng gán chân

| # | Tín hiệu (tên cổng top) | Hướng | Khối điều khiển | Chân chip | Tên chân | Bank / VCCIO | Header | Nối tới | Ghi chú |
|---|---|---|---|---|---|---|---|---|---|
| 1 | `clk27` | vào | Xung nhịp chung M3 + FPGA | 45 | IOT13A | 1 / 3,3 V | – | Thạch anh U12 trên board | Không PLL (O5) |
| 2 | `rst_n` | vào | Reset hệ thống | 14 | IOB4B | 3 / 1,8 V | P7-22 | Nút KEY1 trên board | Kéo lên 100 kΩ trên board |
| 3 | `imu_scl` | 2 chiều | M3 GPIO[0], I²C mềm (O6) | 44 | IOT17B | 1 / 3,3 V | P7-15 | GY-521 SCL | R29 4,7 kΩ lên 2,5 V có sẵn + kéo lên của GY-521 |
| 4 | `imu_sda` | 2 chiều | M3 GPIO[1], I²C mềm (O6) | 46 | IOT13B | 1 / 3,3 V | P7-14 | GY-521 SDA | R30 4,7 kΩ lên 2,5 V có sẵn + kéo lên của GY-521 |
| 5 | `imu_int` | vào | M3 GPIO[2] | 43 | IOT17A | 1 / 3,3 V | P7-10 | GY-521 INT | Báo dữ liệu mới 100 Hz |
| 6 | `wifi_txd` | ra | M3 UART0 TXD | 39 | IOT26A | 1 / 3,3 V | P7-6 | ESP-01S chân 4 (RXD) | Lệnh AT |
| 7 | `wifi_rxd` | vào | M3 UART0 RXD | 40 | IOT26B | 1 / 3,3 V | P7-7 | ESP-01S chân 5 (TXD) | Lệnh AT |
| 8 | `dbg_txd` | ra | M3 UART1 TXD | 41 | IOT20A | 1 / 3,3 V | P7-8 | USB–UART RXD | Gỡ lỗi, thu dữ liệu thô |
| 9 | `dbg_rxd` | vào | M3 UART1 RXD | 42 | IOT20B | 1 / 3,3 V | P7-9 | USB–UART TXD | Gỡ lỗi |
| 10 | `wifi_rst` | ra | M3 GPIO[3] | 18 | IOB13A | 3 / 1,8 V | P6-10 | Q1 S8050 → ESP-01S chân 7 (RST) | 1 = giữ module ở reset |
| 11 | `sens_pwr` | ra | M3 GPIO[4] | 17 | IOB6B | 3 / 1,8 V | P6-11 | Q2 S8050 → Q3 S8550 (PNP) → VCC GY-521 | 1 = cấp nguồn cảm biến |
| 12 | `buz` | ra | M3 GPIO[5] bật/tắt, GPIO[8] chọn kiểu; FPGA tạo 2 kHz | 16 | IOB6A | 3 / 1,8 V | P6-13 | Q4 S8050 → còi 5 V | 1 = kêu |
| 13 | `btn_n` | vào | M3 GPIO[6] | 15 | IOB5A | 3 / 1,8 V | P6-22 | KEY2 trên board + nút ngoài xuống GND | Nhấn = 0; kéo lên 100 kΩ + tụ 100 nF trên board |
| 14 | `led` | ra | M3 GPIO[7] | 10 | IOT7A | 0 / 3,3 V | P7-5 | LED DS3 trên board | Chân MODE dùng làm GPIO, như ví dụ Sipeed **[CẦN XÁC MINH ở S0]** |

Header P6, P7 đánh số theo [sơ đồ NL]; cấu hình đầu nối trong LiteX (`sipeed_tang_nano_4k.py`) cho cùng kết quả. Vị trí chân 1 trên board phải kiểm bằng mắt hoặc đồng hồ đo trước khi hàn **[CẦN XÁC MINH]**.

#### 12.5.3 Chân dự phòng, chân không dùng, chân nguồn

- **Dự phòng 1,8 V** (bank 3): chân 13 (P6-12), 19 (P6-9), 20 (P6-8), 21 (P6-7), 22 (P6-6), 23 (P6-5).
- **3,3 V: hết chân trống.** Nếu cần thêm: chân 1, 47, 48 (đang nối flash SPI), với điều kiện giữ chân 2 (MSPI_CS) ở mức 1 để flash không hoạt động **[CẦN XÁC MINH]**.
- **Không dùng:** 2, 8, 9 (flash SPI); 3, 4, 6, 7 (JTAG); 27–35 (HDMI và DVP_XCLK, 2,5 V).
- **Chân nguồn trên header:** P6-21 = 5 V vào (từ boost); P6-19, P6-20 = GND; P7-17 = 3,3 V ra (cấp GY-521 qua Q3); P7-16 = 2,5 V ra và P6-18 = 1,8 V ra (không dùng).

#### 12.5.4 Nối nguồn

```
LiPo 1S ─► module sạc có bảo vệ ─► công tắc ─► boost 5 V ≥ 1 A ─┬─► P6-21 (5 V) Tang Nano 4K
                                                               ├─► ổn áp 3,3 V riêng ≥ 500 mA ─► ESP-01S chân 8 (VCC), chân 6 (EN)
                                                               └─► còi 5 V (qua Q4)
Tang Nano 4K P7-17 (3,3 V) ─► Q3 S8550 PNP (Q2 S8050 điều khiển, từ sens_pwr) ─► VCC_IMU ─► GY-521 VCC
GND chung: P6-19 / P6-20, GND của boost, ổn áp, ESP-01S, GY-521, USB–UART
```

- **Không cắm USB-C và nguồn boost cùng lúc.** Trên board, U5 (AP2337) nối 5 V header với VBUS của cổng USB, nên hai nguồn 5 V có thể đẩy ngược nhau **[suy từ sơ đồ NL, CẦN XÁC MINH]**. Khi nạp hoặc gỡ lỗi qua USB thì tắt công tắc nguồn pin.
- ESP-01S không lấy 3,3 V từ Tang Nano 4K: dòng phát 802.11b khoảng 170 mA, Ai-Thinker khuyên nguồn > 500 mA [ESP-01S].
- Cắt nguồn cảm biến theo quy trình mục 7.5.4: đưa SCL, SDA ra mức 0 **trước** khi `sens_pwr = 0` (R29, R30 trên board kéo lên 2,5 V luôn có điện, sẽ cấp ngược qua chân I²C); điện trở xả 4,7 kΩ sau Q3; tụ 10 µF + 100 nF sát GY-521.
- **Toàn bộ linh kiện là loại chân cắm** (chốt 11/11, để nhóm tự hàn bằng mỏ hàn thường): transistor vỏ TO-92, điện trở 1/4 W, tụ hóa 10 µF và tụ gốm 100 nF chân cắm.
  - **Q1, Q2, Q4 = S8050 (NPN)**; **Q3 = S8550 (PNP)**. S8050 chịu dòng cực góp tối đa 700 mA, 20 V theo datasheet UTC; hệ số khuếch đại 85–300 tùy hạng B/C/D. Các nguồn tra cứu không thống nhất, nên lấy theo datasheet của lô mua.
  - Thứ tự chân TO-92 thường là **E-B-C** (nhìn mặt phẳng, chân hướng xuống, từ trái qua phải). Một số hãng xếp khác: **kiểm datasheet của lô mua trước khi vẽ footprint** **[CẦN XÁC MINH]**.

```
Công tắc nguồn cảm biến (Q2 + Q3):
3,3 V (P7-17) ──┬───────────── E   Q3 S8550 (PNP)
                10 kΩ                C ──► VCC GY-521 ──┬── 10 µF + 100 nF ── GND
                │                    B                   └── 4,7 kΩ (xả) ── GND
                └──── 1 kΩ ──────────┘
                      │
                      C   Q2 S8050 (NPN)
sens_pwr (1,8 V) ─ 4,7 kΩ ─ B
                      E ── GND

Reset ESP-01S (Q1):   wifi_rst ─ 4,7 kΩ ─ B  Q1 S8050;  C ─► ESP-01S chân 7 (RST);  E ─ GND
Còi (Q4):             buz ─ 1 kΩ ─ B  Q4 S8050;  C ─► (−) còi, (+) còi ─ 5 V;  E ─ GND
```

  - `sens_pwr = 1`: Q2 dẫn, kéo chân B của Q3 xuống qua 1 kΩ, Q3 dẫn, GY-521 có điện. `sens_pwr = 0`: điện trở 10 kΩ kéo B của Q3 lên 3,3 V, Q3 tắt.
  - Dòng gốc Q3 ≈ (3,3 − 0,7) / 1 kΩ ≈ 2,6 mA [tự tính], dư để Q3 bão hòa với dòng GY-521 khoảng 5–7 mA (ghi chú bản dự thi, `nguc_nga_hub.ino`). Dòng gốc Q2 ≈ (1,8 − 0,7) / 4,7 kΩ ≈ 0,23 mA [tự tính], đủ cho dòng góp khoảng 3 mA.
  - **Đánh đổi:** Q3 (PNP) làm rơi khoảng 0,1–0,2 V khi dẫn [ước tính], nên GY-521 nhận khoảng 3,1–3,2 V. Module có ổn áp riêng, chip MPU6050 chạy từ 2,375 V [MPU6050 mục 6.3]. Đo điện áp thật ở chân VCC module khi lắp **[CẦN XÁC MINH]**.
  - **Còi (Q4):** dòng gốc ≈ (1,8 − 0,7) / 1 kΩ ≈ 1,1 mA [tự tính], đủ cho còi vài chục mA. Nếu còi của nhóm là loại thụ động cuộn dây khoảng 16 Ω thì dòng ở 5 V có thể tới khoảng 300 mA [tự tính]: phải thêm điện trở nối tiếp với còi và diode chống xung song song với còi **[CẦN XÁC MINH sau khi biết loại còi]**.

#### 12.5.5 File ràng buộc chân (bản nháp, đặt ở `gowin/src/tang_nano_4k_nckh.cst` khi làm S0)

```
// Tang Nano 4K - NCKH2627 - theo ke_hoach muc 12.5 (11/11/2026)
// Part GW1NSR-LV4CQN48PC6/I5, goi QFN48P. Thao camera khoi P3 truoc khi dung.
// Bank 0, 1 = 3,3 V; bank 3 = 1,8 V (so do nguyen ly Tang Nano 4K ban 3603)

IO_LOC "clk27"    45; IO_PORT "clk27"    IO_TYPE=LVCMOS33 PULL_MODE=UP;
IO_LOC "rst_n"    14; IO_PORT "rst_n"    IO_TYPE=LVCMOS18 PULL_MODE=UP BANK_VCCIO=1.8;

// MPU6050 - I2C mem qua GPIO cua M3 (O6); keo len co san tren board
IO_LOC "imu_scl"  44; IO_PORT "imu_scl"  IO_TYPE=LVCMOS33 PULL_MODE=NONE DRIVE=8;
IO_LOC "imu_sda"  46; IO_PORT "imu_sda"  IO_TYPE=LVCMOS33 PULL_MODE=NONE DRIVE=8;
IO_LOC "imu_int"  43; IO_PORT "imu_int"  IO_TYPE=LVCMOS33 PULL_MODE=NONE;

// ESP-01S - UART0 cua M3
IO_LOC "wifi_txd" 39; IO_PORT "wifi_txd" IO_TYPE=LVCMOS33 DRIVE=8;
IO_LOC "wifi_rxd" 40; IO_PORT "wifi_rxd" IO_TYPE=LVCMOS33 PULL_MODE=UP;

// USB-UART go loi - UART1 cua M3
IO_LOC "dbg_txd"  41; IO_PORT "dbg_txd"  IO_TYPE=LVCMOS33 DRIVE=8;
IO_LOC "dbg_rxd"  42; IO_PORT "dbg_rxd"  IO_TYPE=LVCMOS33 PULL_MODE=UP;

// Bank 3 (1,8 V): lai transistor, nut
IO_LOC "wifi_rst" 18; IO_PORT "wifi_rst" IO_TYPE=LVCMOS18 DRIVE=8 BANK_VCCIO=1.8;
IO_LOC "sens_pwr" 17; IO_PORT "sens_pwr" IO_TYPE=LVCMOS18 DRIVE=8 BANK_VCCIO=1.8;
IO_LOC "buz"      16; IO_PORT "buz"      IO_TYPE=LVCMOS18 DRIVE=8 BANK_VCCIO=1.8;
IO_LOC "btn_n"    15; IO_PORT "btn_n"    IO_TYPE=LVCMOS18 PULL_MODE=UP BANK_VCCIO=1.8;

// LED tren board (chan MODE)
IO_LOC "led"      10; IO_PORT "led"      IO_TYPE=LVCMOS33 DRIVE=8;
```

Pass ở S0: Gowin đặt – nối dây không báo lỗi bank hoặc loại IO; nếu chân 10 (MODE) báo lỗi thì bật tùy chọn dùng MODE làm GPIO trong cấu hình project **[CẦN XÁC MINH]**.

#### 12.5.6 Cổng top-level RTL và GPIO của M3

```verilog
module top (
    input  wire clk27,    input  wire rst_n,
    inout  wire imu_scl,  inout  wire imu_sda,  input wire imu_int,
    output wire wifi_txd, input  wire wifi_rxd,
    output wire dbg_txd,  input  wire dbg_rxd,
    output wire wifi_rst, output wire sens_pwr, output wire buz,
    input  wire btn_n,    output wire led
);
// Gowin_EMPU: gpio[15:0] (inout); uart0 <-> wifi_*, uart1 <-> dbg_*
// gpio[0] imu_scl   gpio[1] imu_sda   gpio[2] imu_int   gpio[3] wifi_rst
// gpio[4] sens_pwr  gpio[5] buz       gpio[6] btn_n     gpio[7] led
// gpio[8] buz_2khz (0 = muc cao lien tuc cho coi chu dong, 1 = song 2 kHz cho coi thu dong)   gpio[15:9] trong
// Bo chia 27 MHz -> 2 kHz: dao muc moi 6 750 chu ky (27e6 / 2000 / 2), vai chuc LUT [uoc tinh]
// assign buz = gpio_out[5] & (gpio_out[8] ? tone_2k : 1'b1);
```

Firmware (`fw_arm/src/chan.h`, tạo ở bước 5 của mục 10) giữ cùng thứ tự bit:

```c
#define PIN_IMU_SCL   (1u << 0)   /* I2C mem: ghi 0 vao bit du lieu, bat/tat cho phep xuat */
#define PIN_IMU_SDA   (1u << 1)
#define PIN_IMU_INT   (1u << 2)   /* vao */
#define PIN_WIFI_RST  (1u << 3)   /* 1 = giu ESP-01S o reset (qua Q1) */
#define PIN_SENS_PWR  (1u << 4)   /* 1 = cap nguon MPU6050 (qua Q2, Q3) */
#define PIN_BUZ       (1u << 5)   /* 1 = coi keu */
#define PIN_BTN_N     (1u << 6)   /* vao, 0 = nhan */
#define PIN_LED       (1u << 7)
#define PIN_BUZ_2KHZ  (1u << 8)   /* 0 = coi chu dong (muc cao), 1 = coi thu dong (FPGA phat 2 kHz) */
```

Tên thanh ghi GPIO, UART trong SDK của Gowin_EMPU lấy theo ví dụ của Gowin khi tạo project **[CẦN XÁC MINH ở S0]**. Tốc độ UART của ESP-01S theo firmware AT đang nạp (thường 115200) **[CẦN XÁC MINH bằng `AT+UART_CUR?` khi nhận hàng]**.

#### 12.5.7 Gắn module GY-521 (MPU6050) lên PCB qua header cái

Nhóm dùng **module GY-521**, như bản dự thi (`esp32/nguc_nga_hub/nguc_nga_hub.ino` dòng 48; `PROJECT_MASTER.md` mục nối dây). Module có sẵn chip MPU6050, ổn áp 3,3 V nhỏ, điện trở kéo lên SCL/SDA, tụ lọc và đèn nguồn; ra 8 chân.

| Chân module | Nối trên PCB | Ghi chú |
|---|---|---|
| VCC | `VCC_IMU` (3,3 V của board sau Q3) | Bản dự thi cấp 3,3 V và đã chạy. Qua ổn áp của module, chip nhận khoảng 3,1–3,2 V [ước tính], trong dải 2,375–3,46 V [MPU6050 mục 6.3] |
| GND | GND | |
| SCL | `imu_scl` (P7-15) | |
| SDA | `imu_sda` (P7-14) | |
| XDA, XCL | Để trống | I²C phụ, không dùng (như bản dự thi) |
| AD0 | **GND trên PCB** | Địa chỉ I²C **0x68** (như bản dự thi) |
| INT | `imu_int` (P7-10) | |

Thêm trên đường `VCC_IMU`: tụ **10 µF + 100 nF** sát chân VCC của module và **điện trở xả 4,7 kΩ** xuống GND (mục 7.5.2).

**Điện trở kéo lên I²C:** module có sẵn điện trở kéo lên về 3,3 V của module; ghép với R29, R30 (2,5 V) trên Tang Nano 4K thì mức cao khoảng 2,9 V [ước tính], trên ngưỡng 0,7 × 3,3 V = 2,31 V của chip. Giá trị điện trở trên module mỗi nơi bán một khác **[CẦN XÁC MINH bằng đồng hồ trên module của nhóm]**. Vẽ sẵn chỗ cho 2 điện trở 4,7 kΩ về `VCC_IMU` nhưng **không hàn**; chỉ hàn nếu đo thấy module không có điện trở kéo lên.

**Cách gắn module lên PCB:**
- Module **cắm vào header cái 1 × 8 hàn trên PCB**. Không dùng dây dupont: đây là chỗ hay lỏng nhất ở bản dự thi (mục 7.5.1).
- Module phải nằm **song song với PCB**, cắm hết chân; vỏ in 3D ép module xuống để không bị rung hay nghiêng khi đeo.
- Đặt module **thẳng hàng với cạnh PCB**, vì lệch góc sẽ làm trục này nhạy theo trục kia. Ghi hướng trục X, Y (in trên module) lên lớp in của PCB để đổi trục cho đúng trong firmware (O9).
- Đèn nguồn trên module tốn thêm dòng; có thể gỡ đèn để tiết kiệm pin **[CẦN XÁC MINH sau khi đo dòng]**.

## 13. Sắp xếp lại thư mục (đã thực hiện bước 0–6 ngày 11/11/2026; kết quả ở 13.5)

Quyết định của nhóm (11/11/2026):
- **Xóa toàn bộ bản dự thi 2026** khỏi thư mục dự án. Code bản cũ đã có trên GitHub (kiểm 11/11: `main` trùng `origin/main`).
- **Đổi tên thư mục dự án** `D:\hoctap\fpga\fpga2026gowin` → **`D:\hoctap\fpga\NCKH2627`**. Nội dung bản mới nằm **thẳng ở gốc** `NCKH2627/` (không thêm lớp thư mục con).
- **Mục 13 không commit, không push.** Chỉ sắp xếp lại trên máy. Việc đưa bản mới lên repo `danhle396/NCKH2627` nằm ở mục 14, nhóm tự chạy. Các file của bản cũ chưa có trên git (slide, sơ đồ đã sửa) được nén chung vào file lưu trữ ngoài thư mục dự án cùng `ket_qua_board/`.
- `ket_qua_board/` (bị `.gitignore`, chỉ có trên máy): **nén vào file lưu trữ ngoài thư mục dự án**.
- Code và tài liệu dùng lại: **chép sẵn sang bản mới** trước khi xóa.

Vì bản cũ có các thư mục trùng tên với bản mới (`docs/`, `ml/`, `tools/`), bản mới được **dựng tạm trong `_moi/`**, chạy thử, xóa bản cũ, rồi mới chuyển lên gốc và đổi tên thư mục (mục 13.4).

### 13.1 Những thứ chưa có trên git (kiểm 11/11)

| Loại | File | Xử lý |
|---|---|---|
| Đã sửa, chưa commit | `docs/bao_cao_cuoi/VKU-NewBie_VCK_FPGA_MCU_2026 (1).pptx`, `docs/so_do_slide/2_data_flow.drawio`; đã xóa `docs/so_do_slide/2_data_flow.png` | Nén vào file lưu trữ (bước 1) |
| Chưa theo dõi, thuộc bản cũ | `docs/so_do_slide/2_data_flow.drawio.png`, `docs/so_do_slide/kiwi_primer_20k_chu_thich.png`, `docs/bao_cao/2026-10-28_nga_that/dac_trung_b2.txt` | Nén vào file lưu trữ (bước 1) |
| File tạm của draw.io | `docs/.$so_do_he_thong.drawio.bkp`, `docs/so_do_slide/.$2_data_flow.drawio.bkp` | Xóa (bước 0) |
| Bị `.gitignore` | `ket_qua_board/` (3,5 MB, log board 24/10 và 28/10) | Nén vào file lưu trữ (bước 1) |
| Bị `.gitignore` | `fpga/impl/` (9,8 MB): báo cáo tổng hợp và đặt – nối dây | Chép 1 báo cáo cần làm nguồn số liệu (bước 2), phần còn lại xóa |
| Bị `.gitignore` | `firmware/build/`, `firmware/keil/Listings|Objects`, `esp32/nguc_nga_hub/build/`, `__pycache__/` | Xóa (sinh lại được) |
| Tài liệu mới, chưa theo dõi | `docs/De_xuat_NCKH*.docx`, `docs/ke_hoach_toi_uu_tang_nano_4k_m3.md`, `docs/so_do_de_xuat_tang_nano_4k_m3_wifi.drawio`, `docs/so_do_kien_truc_arm.*`, `docs/so_do_chan_tang_nano_4k*`, `docs/datasheet/tang_nano_4k/` | Chuyển vào `docs/` mới (bước 3) |

### 13.2 Cấu trúc đích

```
D:\hoctap\fpga\NCKH2627\         (đổi tên từ fpga2026gowin; vẫn là repo git, nối GitHub như cũ)
├── README.md            viết lại cho đề tài mới; ghi nguồn gốc code dùng lại
├── PROJECT_NCKH.md      file theo dõi chính (mục 9)
├── .gitignore           sửa theo cấu trúc mới
├── .gitattributes
├── rtl/                 src/  sim/  scripts/
├── gowin/               project Gowin GW1NSR-LV4C + Gowin_EMPU (tạo ở bước S0)
├── fw_arm/              src/  (driver/, app/, vi_tri_deo.h) — firmware C cho lõi M3
├── ml/                  src/  data/public/  train_cnn.py
├── tools/
├── data/                dữ liệu tự thu (file lớn để ngoài git)
└── docs/
    ├── de_xuat/   ke_hoach/   so_do/   bao_cao/   spec_nckh.md
    ├── tham_khao/       tài liệu và báo cáo của bản dự thi dùng làm nguồn
    └── datasheet/
```

### 13.3 Danh sách chép sang bản mới

Cột "Đích" là vị trí cuối trong `NCKH2627/`. Ở bước 2 các file được chép vào `_moi/` cùng đường dẫn con (ví dụ `_moi/rtl/src/`).

| Nguồn (bản cũ) | Đích | Dùng để |
|---|---|---|
| `fpga/src/cnn_e2.v`, `cnn_params.vh`, `weights_e2.mem`, `bias_e2.mem` | `rtl/src/` | Dùng lại khối tính (MAC, lượng tử, ReLU, gộp, Dense) cho bộ tăng tốc kiểu mới |
| `fpga/sim/tb_e2.v` | `rtl/sim/` | Testbench khớp bit từng tầng (mẫu cho `tb_inc.v`) |
| `fpga/scripts/test.sh`, `fpga/scripts/gl.sh`, `fpga/scripts/build.sh` | `rtl/scripts/` | **Làm mẫu** cho kiểm chứng cấp 2–3 (mục 8). `test.sh`: giữ phần `tb_e2`, bỏ phần `tb_proto` (SPI). `gl.sh` và `build.sh`: viết lại cho GW1NSR-LV4C, thư viện mô phỏng GW1NS, testbench APB |
| `fpga/bitstream/fpga_project.rpt.txt`, `fpga/impl/gwsynthesis/fpga_project_syn_resource.html` | `docs/tham_khao/` | Nguồn số liệu tài nguyên bản cũ (mục 3) |
| `ml/train_cnn.py`, `ml/src/{cnn_numpy, golden_model, rom_layout, safety_model, posture_model}.py` | `ml/` | Huấn luyện, golden model khớp bit |
| `ml/data/public/` (24 MB, D1) | `ml/data/public/` | Tiền huấn luyện |
| `ml/out/` (1,4 MB: `n2_11_d1/params_int8.npz`, `e2_matrix/n2_11_p1/`) | `ml/out/` | Mô hình N2 đã huấn luyện và bản xuất của nó; `test.sh` và `export_mem.py` cần (thêm khi thực hiện, bảng cũ bị sót) |
| `tools/{export_mem, cosim, convert_d1, windowing, logger, ghi_uart, nga_tren_log, playback_log, plot_d1, truc_cam_bien, posture_eval}.py`, `tools/fetch_d1.sh` | `tools/` | Xuất trọng số, đối chiếu, xử lý dữ liệu và log. Danh sách xem lại khi chép |
| `esp32/nguc_nga_hub/{preprocess, safety, nga, posture, pwr_btn, app_core, blocks, hub}.{c,h}`, `fw_config.h`, `selftest_expect_v3_e2_n2.h`, `trang_web_html.h` | `fw_arm/src/port_tu_esp32/` | Chuyển sang lõi M3 (mục 7). `firmware/src/` có các file cùng tên: so sánh hai bản, lấy bản mới hơn **[CẦN XÁC MINH khi chép]** |
| `firmware/src/{board, gyro_cal, imu, i2c0}.{c,h}` | `fw_arm/src/tham_khao/` | Tham khảo driver đọc MPU6050, khởi động lại cảm biến (`board.c`, mục 7.1) và hiệu chỉnh con quay |
| `docs/spec.md` | `docs/tham_khao/spec_v3_du_thi.md` | Số học A.5, bố trí ROM A.6, tầng 2 A.12 (cơ sở cho SPEC mới) |
| `docs/data_protocol.md` | `docs/tham_khao/` | Tham khảo định dạng ghi dữ liệu |
| Tạo mới: `docs/tham_khao/bang_chung_du_thi.md` | `docs/tham_khao/` | Tóm tắt bằng chứng mà kế hoạch trích từ file sẽ bị xóa: nhật ký mất cảm biến 28/10 và còi 2 kHz (`PROJECT_MASTER.md`), thử cắt nguồn GPIO25 (`nguc_nga_hub.ino` dòng 48–57), gỡ bus I²C (`firmware/src/app_main.c` dòng 36–48), còi PZ1 (`firmware/src/board.h` dòng 35–40); ghi commit `f26bea8` |
| `docs/datasheet/MPU-6000_MPU-6050_Product_Spec_Rev3.4.pdf` | `docs/datasheet/` | Vẫn dùng MPU6050 |
| `docs/datasheet/tang_nano_4k/` (5 file, mục 12.5) | `docs/datasheet/tang_nano_4k/` | Sơ đồ nguyên lý, bảng chân, datasheet board, ESP-01S, IPUG932 |

Không chép: SPI và khung truyền (`spi_slave.v`, `packet_parser_v3.v`, `crc16_ccitt.v`, `result_frame_v3.v`, `spi_result_tx_v3.v`, `spi_frame.c`, `node_link.c`, `crc.c`), PLL GW2A, phần còn lại của firmware SN32F407 (`app_main.c`, project Keil), giả lập Cortex-M0 (`m0_test.py`), datasheet SN32F407, ESP32, Kiwi Primer 20K, slide và báo cáo dự thi. Cần lại thì lấy từ lịch sử git.

### 13.4 Các bước thực hiện

| Bước | Việc | Kiểm tra (Pass) |
|---|---|---|
| 0 | Xóa 2 file tạm `.bkp` của draw.io | Không còn file `.bkp` |
| 1 | Nén thành `D:\hoctap\fpga\luu_tru\luu_tru_du_thi_2026.zip` (ngoài thư mục dự án): `ket_qua_board/` và các file ở 13.1 dòng 1–2 (slide `.pptx`, `2_data_flow.drawio`, `2_data_flow.drawio.png`, `kiwi_primer_20k_chu_thich.png`, `dac_trung_b2.txt`) | Số file trong zip = số file nguồn (`ket_qua_board/` có 22 file, đếm bằng `find` ngày 11/11, cộng 5 file lẻ); mở thử được |
| 2 | Tạo khung bản mới trong `_moi/`; chép theo bảng 13.3; ghi nguồn từng file vào `_moi/README.md` | Mọi file đích tồn tại, so `sha256` với nguồn |
| 3 | Chuyển tài liệu mới vào `_moi/docs/` (`de_xuat/`, `ke_hoach/`, `so_do/`) | Mở được `.docx`, `.drawio`, ảnh PNG |
| 4 | Sửa đường dẫn trong file đã chép cho **tương đối** (`$readmemh` của `cnn_e2.v`, `test.sh`, import trong `ml/`, `tools/`) để còn đúng sau khi chuyển lên gốc; trong `test.sh` bỏ phần `tb_proto` (SPI). `gl.sh`, `build.sh` để nguyên làm mẫu, viết lại ở bước S0/4 của mục 10 | `_moi/rtl/scripts/test.sh` TẤT CẢ PASS như bản cũ (`tb_e2` P = 1 và 4, 10/10 vector khớp bit); `python _moi/ml/train_cnn.py --help` chạy được |
| 5 | Xóa bản cũ (xóa thư mục thường, không dùng lệnh git): `esp32/`, `firmware/`, `fpga/`, `ml/`, `tools/`, `docs/`, `ket_qua_board/` (sau khi bước 1 đạt), `PROJECT_MASTER.md` | Gốc chỉ còn `_moi/`, `README.md`, `.git/`, `.gitignore`, `.gitattributes` |
| 6 | Chuyển toàn bộ nội dung `_moi/` lên gốc, xóa `_moi/`; viết lại `README.md`, `.gitignore`; tạo `PROJECT_NCKH.md` (mục 9) | Chạy lại `rtl/scripts/test.sh`: vẫn TẤT CẢ PASS; không còn đường dẫn tới thư mục cũ hay `_moi/` |
| 7 | **Đóng VS Code** (và mọi chương trình đang mở file trong thư mục), đổi tên `D:\hoctap\fpga\fpga2026gowin` → `D:\hoctap\fpga\NCKH2627`, mở lại thư mục mới trong VS Code | `git status` chạy được trong `NCKH2627`; `git remote -v` vẫn trỏ GitHub cũ |
| 8 | Chuyển bộ nhớ làm việc của Claude Code sang thư mục mới (bộ nhớ gắn với đường dẫn dự án: thư mục `C:\Users\ADMIN\.claude\projects\d--hoctap-fpga-fpga2026gowin\memory\`), rồi cập nhật: giao thức đồng bộ 3 file (`PROJECT_MASTER.md`…) và trạng thái nạp board của bản cũ không còn áp dụng | Phiên mới trong `NCKH2627` đọc được bộ nhớ **[CẦN XÁC MINH tên thư mục bộ nhớ mới sau khi mở]** |

Lưu ý:
- Bước 4 phải đạt trước bước 5, để chắc code chép sang vẫn chạy được.
- Bước 7 (đổi tên) làm **cuối cùng**, khi không còn chương trình nào mở file trong thư mục. Phiên Claude Code đang chạy trong thư mục cũ sẽ không theo sang; mở phiên mới trong `NCKH2627`.
- Tạo project Gowin (bước S0) **sau** khi đổi tên, vì project Gowin có thể lưu đường dẫn tuyệt đối **[CẦN XÁC MINH]**.
- Đổi tên thư mục trên máy **không** đổi tên repo trên GitHub (`fpga2026gowin`). Muốn đổi thì làm trong phần Settings của repo trên GitHub; việc đó do nhóm tự quyết.
- Không commit nên **GitHub vẫn giữ nguyên bản dự thi**. Cho tới khi nhóm tự commit, `git status` sẽ hiện toàn bộ file bản cũ là "đã xóa" và các thư mục mới là "chưa theo dõi"; điều này là bình thường.
- Các đường dẫn `fpga/…`, `esp32/…`, `firmware/…`, `ml/…`, `tools/…`, `docs/…` trong các mục 1–12 của file này là vị trí **trong bản dự thi** (trước khi sắp xếp). Sau mục 13, tìm chúng theo bảng 13.3.

### 13.5 Kết quả thực hiện (11/11/2026)

| Bước | Kết quả | Pass |
|---|---|---|
| Trước khi làm | `main` = `origin/main` = `f26bea8` (kiểm bằng `git fetch` + `git ls-remote`); bản cũ chạy `fpga/scripts/test.sh`: TẤT CẢ PASS (tb_e2 P = 1, P = 4, tb_proto, sketch ESP32) | Chuẩn so sánh |
| 0 | Xóa 2 file `.bkp`, **sau khi** đã cho chúng vào zip ở bước 1 (thêm so với kế hoạch, để không mất bản sao nào) | Đạt |
| 1 | `D:\hoctap\fpga\luu_tru\luu_tru_du_thi_2026.zip`, 5,8 MB: 22 file `ket_qua_board/` + 5 file lẻ + 2 `.bkp` = 29 file | Đạt: 29/29, `testzip` sạch, sha256 từng file khớp |
| 2–3 | Chép 201 file (bảng 13.3, thêm `ml/out/`) | Đạt: sha256 201/201 khớp; 20 file `.docx/.drawio/.png/.pdf` mở được |
| 4 | Viết lại `rtl/scripts/test.sh` (bỏ tb_proto, kiểm ESP32; tự chọn Python có numpy) | Đạt: TẤT CẢ PASS, 10/10 vector, 258.080 chu kỳ/lần với P = 1, giống bản cũ; `train_cnn.py --help` chạy |
| 5 | Xóa `esp32/`, `firmware/`, `fpga/`, `ml/`, `tools/`, `docs/`, `ket_qua_board/`, `PROJECT_MASTER.md` | Đạt |
| 6 | Chuyển `_moi/` lên gốc; viết `README.md` (nguồn từng file), `.gitignore`, `PROJECT_NCKH.md`, `docs/tham_khao/bang_chung_du_thi.md` | Đạt: `test.sh` TẤT CẢ PASS ở gốc; 206 file |
| 7 | Đổi tên thư mục: **nhóm tự làm** (phải đóng VS Code trước) | Đạt: phiên 11/11 mở tại `D:\hoctap\fpga\NCKH2627`; `rtl/scripts/test.sh` TẤT CẢ PASS ở vị trí mới (P = 1: 27 s, P = 4: 15 s) [mô phỏng] |
| 8 | Bộ nhớ Claude Code đã chép sẵn sang `C:\Users\ADMIN\.claude\projects\d--hoctap-fpga-NCKH2627\memory\` | Đạt: phiên mới tại `NCKH2627` đọc được bộ nhớ (11/11) |

Phát hiện khi làm:
- Các file C lõi ở `esp32/nguc_nga_hub/` và `firmware/src/` **trùng từng byte**, nên điểm [CẦN XÁC MINH] ở dòng `port_tu_esp32` của 13.3 đã giải.
- `tools/cosim.py` và `tools/nga_tren_log.py` import `m0_test.py` (giả lập Cortex-M0, cần ảnh build SN32F407) nên chưa chạy được ở cấu trúc mới; phải viết lại cho M3 nếu cần dùng.
- Trên máy nhóm, `python3` trong Git Bash (msys) không có numpy; Python có numpy là `C:\Users\ADMIN\AppData\Local\Programs\Python\Python312\python.exe`.

## 14. Đưa bản mới lên GitHub `danhle396/NCKH2627` (nhóm tự chạy, sau khi mục 13 xong 100 %)

Repo đích: https://github.com/danhle396/NCKH2627 (kiểm 11/11: truy cập được, **đang trống**, chưa có nhánh nào).

Cách làm: **bắt đầu lịch sử git mới** cho `NCKH2627`, không mang theo lịch sử bản dự thi. Lịch sử bản dự thi vẫn còn trên repo cũ `danhle396/fpga2026gowin`; thư mục `.git` cũ được chuyển sang `D:\hoctap\fpga\luu_tru\` làm bản dự phòng, không xóa.

### 14.1 Điều kiện trước khi chạy

- [ ] Mục 13 đã xong đủ 8 bước; thư mục đang là `D:\hoctap\fpga\NCKH2627`.
- [ ] `rtl/scripts/test.sh` vẫn TẤT CẢ PASS ở vị trí mới.
- [ ] `.gitignore` đã viết lại (mục 13 bước 6), ít nhất có: `__pycache__/`, `*.vvp`, `gowin/impl/*`, `*.gprj.user`, `fw_arm/**/build/`, `fw_arm/**/Objects/`, `fw_arm/**/Listings/`, `data/raw/` (dữ liệu tự thu lớn), `.claude/settings.local.json`.
- [ ] Không còn `_moi/`, file `.bkp`, file tạm.

### 14.2 Lệnh (PowerShell trong VS Code, mở tại `D:\hoctap\fpga\NCKH2627`)

```powershell
cd D:\hoctap\fpga\NCKH2627

# 1. Cất lịch sử git của bản dự thi (đã có trên GitHub fpga2026gowin) ra ngoài, không xóa
Move-Item .git D:\hoctap\fpga\luu_tru\git_fpga2026gowin

# 2. Tạo lịch sử git mới, nhánh main
git init -b main

# 3. Xem trước những gì sẽ được đưa lên (kiểm không có file build, file tạm, dữ liệu lớn)
git add .
git status
git count-objects -vH

# 4. Commit đầu tiên (nhóm tự đặt nội dung; không ghi Claude là đồng tác giả)
git commit -m "Khởi tạo đề tài NCKH 2026-2027: kiến trúc xử lý lai ARM – FPGA trên Tang Nano 4K"

# 5. Nối với repo mới và đẩy lên
git remote add origin https://github.com/danhle396/NCKH2627.git
git push -u origin main
```

Lệnh trên chạy được cả trong Git Bash, trừ bước 1: Git Bash dùng `mv .git /d/hoctap/fpga/luu_tru/git_fpga2026gowin`.

### 14.3 Kiểm tra sau khi đẩy

| Kiểm tra | Cách | Pass |
|---|---|---|
| Nhánh đã lên | `git status -sb` | Hiện `## main...origin/main`, không lệch |
| File đã lên đủ | Mở https://github.com/danhle396/NCKH2627 | Thấy `README.md`, `PROJECT_NCKH.md`, `rtl/`, `fw_arm/`, `ml/`, `tools/`, `docs/` |
| Không có file thừa | Xem trên GitHub | Không có `__pycache__/`, `impl/`, `build/`, `_moi/`, `.bkp` |
| Lịch sử mới | `git log --oneline` | Chỉ có 1 commit khởi tạo |

### 14.4 Nếu gặp lỗi

| Lỗi | Nguyên nhân thường gặp | Cách xử lý |
|---|---|---|
| `rejected … fetch first` khi push | Repo trên GitHub đã có file (ví dụ tạo kèm README) | `git pull origin main --allow-unrelated-histories`, gộp xong rồi `git push -u origin main`. **Không** dùng `--force` khi chưa xem trên GitHub có gì |
| Hỏi đăng nhập / `Authentication failed` | Chưa đăng nhập GitHub trên máy, hoặc repo riêng tư | Đăng nhập qua cửa sổ Git Credential Manager khi được hỏi, hoặc dùng Personal Access Token thay mật khẩu |
| Cảnh báo file > 50 MB | Có dữ liệu lớn bị thêm vào | `git rm --cached <file>`, thêm vào `.gitignore`, commit lại trước khi push |
| `remote origin already exists` | Bước 1 chưa chuyển `.git` cũ đi | Kiểm `git remote -v`; nếu vẫn trỏ `fpga2026gowin` thì làm lại bước 1 |

Ghi chú:
- Lịch sử bản dự thi trong repo cũ chiếm khoảng 19,7 MB (kiểm 11/11 bằng `git count-objects`); bắt đầu lịch sử mới giúp repo `NCKH2627` gọn và không lẫn với bản dự thi.
- Muốn mang theo cả lịch sử bản dự thi thì bỏ bước 1–2, chỉ đổi remote: `git remote set-url origin https://github.com/danhle396/NCKH2627.git` rồi commit và push. Khi đó repo mới chứa toàn bộ lịch sử cũ.

## 15. Kết quả thực hiện mục 10 (11/11/2026, đêm)

Làm theo thứ tự mục 10 tới chỗ cần board. Bằng chứng: `docs/bao_cao/2026-11-11_*` (6 thư mục, mỗi thư mục có README). Kiểm lại toàn bộ bằng `bash rtl/scripts/test.sh` (10/10 PASS). Tài liệu mới: `docs/spec_nckh.md` (SPEC mới, mục 9), `docs/huong_dan_thu_tren_board.md` (việc còn lại).

### 15.1 Kết quả từng bước

| Bước | Kết quả | Pass | Nhãn |
|---|---|---|---|
| S0 | IP Gowin_EMPU sinh bằng dòng lệnh (`rtl/scripts/build_gowin.sh` tự viết `config.v`, không cần IP Core Generator). SRAM 2 / 4 / 8 / 16 KB = **2 / 2 / 4 / 8 BSRAM**; EMPU 8 KB: 54 logic, Fmax 210 MHz; `.cst` mục 12.5.5 không lỗi | Đạt | P&R |
| 0 | `tools/o3_dense_2_lop.py`: 50/50 vector và **109.970/109.970 cửa sổ D1** trùng lớp 4 lớp; phiếu FALL không đổi ở mọi T_SCORE | Đạt | mô phỏng |
| 1a | `ml/src/golden_inc.py` (+ `tools/kiem_golden_inc.py`): **41/41 lần thử ngã liên tục** khớp bit golden E2 mọi tầng, đệm khởi tạo ngẫu nhiên, có SOFT_RST | Đạt | mô phỏng |
| 1b, 2 | `rtl/src/cnn_inc.v`: 52/52 bước khớp l1/l2/l3 + logit; ROM 3 pROM (bias trong ROM), đệm + STAGE 1 SDPB | Đạt | mô phỏng, P&R |
| 3 | `rtl/src/cnn_apb.v` + `rtl/sim/tb_inc.v`: 52/52 bước khớp qua APB, phép âm đạt; **netlist sau P&R** (`rtl/scripts/gl_inc.sh`): 52/52 | Đạt | mô phỏng, netlist |
| 4 | Cả chip: logic **1.076 (24 %)**, thanh ghi 514, **BSRAM 8/10**, DSP 1,25/8, **Fmax 41,1 MHz**; **70.010 chu kỳ = 2,59 ms mỗi bước** ở 27 MHz | Đạt | P&R, mô phỏng |
| 5 | `fw_arm/`: driver theo IPUG931 (GPIO, UART, WDT, SysTick, APB), I²C mềm, MPU6050 + tự gỡ 4 bậc (`core/imu_guard.c`), tự kiểm, O9 (`app/vi_tri_deo.h`), `app/chan.h`. Flash 15,7 KB, RAM 6,2 KB. Giả lập M3: đạt | Chờ board | giả lập |
| 6 | `core/wifi_at.c` (AT + HTTP không chặn), trang web gzip 4.028 B. Giả lập với module ESP-01S giả: đạt | Chờ board | giả lập |
| 7 | `NGA_BAT = 0`, `TILT_EN = 0` cho bắp tay tới khi có dữ liệu | Chờ dữ liệu | – |
| 8 | `core/cnn_core.c` CNN phần mềm: **150/150 khớp bit** golden; ~1,17 triệu lệnh mỗi lần; bản `DOI_CHUNG=1` 25,0 KB flash, 7,7 KB RAM | Chờ đo trên board | giả lập, ước tính |

### 15.2 Các điểm phải đổi so với kế hoạch (đã sửa tại chỗ ở O1, O3, O4, mục 2, bảng tóm tắt)

1. **NEW_DATA 144 B thay vì 96 B.** Vị trí L1[0] chịu đệm 0 ở mép trái phải tính lại mỗi bước, cần x[0..3] của cửa sổ. M3 đã giữ cả cửa sổ nên gửi thêm 24 B; FPGA không phải giữ mẫu cũ.
2. **STAGE nằm trong SDPB đệm.** GW1NS-4 không có RAM phân tán: để riêng thì tổng hợp ra 1.152 thanh ghi (thanh ghi 73 %, CLS 92 %). Đặt vào 6 ô L3 sắp tính lại (192 B) thì không tốn gì thêm; golden chứng minh vẫn khớp bit.
3. **ROM tách tường minh 3 bank 2.048 × 8.** Một mảng 5.608 × 8 thì Gowin xếp 4 pROM.
4. **MAC mỗi bước 61.824** (ít hơn E2 3,7 lần), không phải 30–39 nghìn. Thời gian vẫn đạt: 2,59 ms ≤ 6 ms.
5. **Lớp IMPACT có trọng số khác 0** (488/512, bias −53), không phải 0 và −2³⁰ như SPEC cũ. Bỏ vẫn an toàn (bước 0).
6. Kết quả hợp lệ đầu tiên sau khởi động hay sau khi gỡ cảm biến: M3 gửi bước ngay từ mẫu 50 Hz thứ 16 (vòng khởi tạo 0), nên **vẫn là 2,56 s**, không cộng thêm 8 bước.
7. Mô phỏng netlist: netlist cả chip và netlist sau tổng hợp đều bị Gowin Education mã hóa; dùng netlist `.vo` sau P&R của vỏ bọc `rtl/sim/gl_wrap.v` (35 chân).

### 15.3 Rủi ro và điểm [CẦN XÁC MINH] mới

| Điểm | Kiểm ở |
|---|---|
| Xung nhịp 27 MHz vào chân 45 không phải chân GCLK (Gowin PR1014: đi đường thường) | H1; nếu lỗi thời gian thì đổi chân |
| `master_pclk` của APB2 bằng xung hệ thống (FPGA chạy theo `master_pclk`) | H1: CYCLES đọc được = 70.010 |
| Khóa watchdog `0x1ACCE551` (giá trị của khối CMSDK; IPUG931 không ghi số) | H1 |
| Lệnh `AT+CWMODE=2`, `AT+CWSAP=…` đúng với firmware AT của lô ESP-01S | H1 (`AT+GMR`) |
| Trang web cũ (viết cho ESP32 hub) hiển thị đúng với JSON mới; trường tư thế, lịch sử để trống | H1, trên trình duyệt |
| Bản đối chứng còn 0,3 KB RAM | bước 8; nếu thiếu thì giảm vòng UART |
| Tên thao tác nạp `.bin` cho M3 trong Gowin Programmer | H1 |

### 15.4 Dữ liệu (11/11/2026, khuya)

- **D1 đã xóa khỏi repo** (nhóm quyết định, tránh lẫn kênh đầu/ngực như bản dự thi); dữ liệu bắp tay **nhóm tự thu 100 %** (bước 7). Dòng `ml/data/public/` ở bảng 13.3 không còn đúng.
- Kiểm khớp bit (O3, golden tăng dần, tb_inc, firmware giả lập) chuyển sang luồng tổng hợp `ml/src/luong_tong_hop.py`; kết quả vẫn đạt (O3: 20.000/20.000 cửa sổ; golden tăng dần: 40/40 luồng × 60 bước; tb_inc: 52/52 bước). Số liệu trên D1 ở 15.1 là bằng chứng chạy trước khi xóa.
- Mô hình trong ROM là mô hình tạm (kênh đầu D1). Thay bằng mô hình bắp tay: huấn luyện → `tools/o3_dense_2_lop.py` → `tools/export_inc.py` → chép `rom_inc*.mem`, `cnn_inc_params.vh` vào `rtl/src/`, `gen/*` vào `fw_arm/src/gen/` → `rtl/scripts/test.sh`. RTL không đổi.

### 15.5 Xóa mọi thứ không phải dữ liệu bắp tay (11/11/2026, khuya)

Theo yêu cầu của nhóm (tránh sau này sót mà vẫn dùng dữ liệu cũ):
- Xóa: mô hình kênh đầu D1 (`ml/out/n2_11_d1*`, `e2_matrix/`, gồm cả vector kiểm thử chứa cửa sổ D1); cấu hình ngực và mọi ngưỡng đo trên ngực/đầu (`fw_config_du_thi.h`, khối `VT_NGUC`, ngưỡng tầng 2 tạm); tầng 2 và nhận dạng tư thế của ngực (`core/nga.*`, `core/posture.*`, `ml/src/posture_model.py`); công cụ D1 và log bản dự thi (`convert_d1`, `fetch_d1`, `plot_d1`, `posture_eval`, `cosim`, `playback_log`, `nga_tren_log`, `truc_cam_bien`); script GW2A cũ (`gl.sh`, `build.sh`).
- ROM nạp **mô hình THỬ ngẫu nhiên** `THU_NGAU_NHIEN` (`tools/tao_mo_hinh_thu.py`), in tên ở UART và web. Kiểm lại toàn bộ: `test.sh` 10/10 PASS, Keil 0 lỗi 0 cảnh báo, Gowin logic 1.076 / BSRAM 8/10 / Fmax 41,1 MHz (không đổi). Tự kiểm mới: lớp 0, SCORE 34, logit {18073, 423}.
- Mục 7.1, 7.2 nói "chuyển `nga.c`, `posture.c`": nay **viết mới** cho bắp tay ở bước 7. Ngưỡng khối bất động `T_STILL_A/G` và hằng số thời gian của A.8 là giá trị đầu của SPEC, chọn lại trên dữ liệu bắp tay.
- Còn giữ (code, không chứa dữ liệu): `tools/ghi_uart.py`, `logger.py`, `windowing.py` (thu và cắt log; cần sửa theo định dạng log của firmware M3 ở bước 7); `fw_arm/src/port_tu_esp32/` (lõi ESP32 tham khảo); tài liệu lịch sử ở `docs/tham_khao/` và báo cáo 11/11 có số đo trên D1.

### 15.6 Nhiệm vụ bắt buộc sau khi có dữ liệu bắp tay thật (bước 7b, yêu cầu của nhóm 11/11/2026)

**Nguyên tắc: chỉ được dùng dữ liệu thật khi nhóm đã thu dữ liệu thật.** Mô hình thử ngẫu nhiên và các luồng giả lập hiện có chỉ là chỗ đứng tạm để kiểm khớp bit trong lúc chưa có dữ liệu. Khi đã có dữ liệu bắp tay và mô hình huấn luyện từ dữ liệu đó, **xóa hết, không giữ lại để "dự phòng"**:

| Xóa | Thay bằng |
|---|---|
| `tools/tao_mo_hinh_thu.py`, `ml/out/mo_hinh_thu_4lop/` (gồm `e2_p1/` có vector tổng hợp), `ml/out/mo_hinh_thu/` | Mô hình bắp tay: `ml/train_cnn.py --data <npz bắp tay>` → `tools/o3_dense_2_lop.py --params <mô hình>` |
| ROM và tham số đang nạp: `rtl/src/rom_inc*.mem`, `cnn_inc_params.vh`, `weights_e2.mem`, `bias_e2.mem`, `cnn_params.vh`; bản sao `gowin/src/rom_inc*.mem`, `cnn_inc_params.vh` (`rtl/scripts/dong_bo_gowin.sh`); `fw_arm/src/gen/cnn_rom_inc.*`, `selftest_expect_inc.h` | Xuất lại từ mô hình bắp tay (`tools/export_inc.py`, `tools/export_mem.py`); `CNN_MO_HINH` thành tên mô hình bắp tay |
| `ml/src/luong_tong_hop.py` và mọi chỗ gọi nó: nhánh cửa sổ tổng hợp trong `tools/export_mem.py`, `tools/o3_dense_2_lop.py`; luồng tổng hợp trong `tools/kiem_golden_inc.py`, `tools/export_inc.py`; `fw_arm/test/m3_test.py` | Luồng và cửa sổ bắp tay thật (tập kiểm tra, tách khỏi tập huấn luyện) |
| Vector giả trong `tools/export_mem.py` `vectors()`: ngẫu nhiên, toàn +127/−128/0, xen kẽ, bậc thang, LFSR | Cửa sổ thật; nếu cần kiểm biên số học thì chọn các cửa sổ thật có bão hòa (va đập mạnh) |
| Luồng / mẫu ngẫu nhiên trong các kiểm thêm 12/11: `fw_arm/test/m3_test.py` mục 8–10 (raw_log, đường ống thiết bị == `windowing.py`, tầng 2 C == Python với ngưỡng thử), `tools/chon_truc.py --tu-kiem`, `tools/chon_nguong_tang2.py --tu-kiem` | Log thô bắp tay thật (cùng file chạy qua C và Python) |
| Tự kiểm khi khởi động bằng luồng LFSR (`cnn_selftest_stage()` trong `fw_arm/src/core/cnn_core.c`, `lfsr_stream()` trong `tools/export_inc.py`, A.10 của `docs/spec_nckh.md`) | 8 bước cố định cắt từ một đoạn dữ liệu bắp tay thật (240 mẫu × 6 kênh = 1.440 byte trong flash), kết quả kỳ vọng xuất cùng mô hình |
| Mục "Mô hình đang nạp" trong `docs/spec_nckh.md`; dòng mô hình thử trong `README.md`, `PROJECT_NCKH.md` | Tên, nguồn dữ liệu, số người của mô hình bắp tay |

Sau khi xóa: chạy lại `rtl/scripts/test.sh`, `rtl/scripts/gl_inc.sh`, `rtl/scripts/build_gowin.sh`, build Keil; tìm trong repo các từ ở cột Pass của bước 7b phải không còn kết quả (trừ nhật ký lịch sử trong `PROJECT_NCKH.md` và mục 15 này).

### 15.7 Việc không cần board, làm thêm 12/11/2026

| Việc | Kết quả | Nhãn |
|---|---|---|
| Ghi dữ liệu thô 100 Hz (kế hoạch 7.4) | `core/raw_log.c`: CSV `t,ax,ay,az,gx,gy,gz,btn` (trục cảm biến, con quay đã trừ offset); qua WiFi `GET /raw` (kết nối giữ mở, gửi khúc ≥ 512 B hoặc mỗi 200 ms, xen kẽ web) hoặc UART1 (lệnh `R1`/`R0`) | giả lập (module giả) |
| Hiệu chỉnh con quay khi khởi động (kế hoạch 7.1) | `core/gyro_cal.c` (từ `tham_khao/`): 2 s đứng yên, làm lại nếu rung; UART in `GC,…` | – |
| Công cụ thu | `tools/thu_bap_tay.py` (WiFi hoặc COM, kiểm định dạng, đếm mẫu mất) thay `logger.py`, `ghi_uart.py` (ESP32/ngực, đã xóa) | – |
| Cắt cửa sổ | `tools/windowing.py` viết lại cho bắp tay; đổi trục theo `vi_tri_deo.h` qua `ml/src/vi_tri.py`. **Cửa sổ của thiết bị (C) trùng `windowing.py` từng byte** | giả lập |
| Chọn trục | `tools/chon_truc.py` (đứng, nằm ngửa, nghiêng phải) → `TRUC_*`, `ACC_OFF_*` | tự kiểm 200/200 |
| Tầng 2 bắp tay (7.2) | `core/nga_bt.c` (4 dấu hiệu, ngưỡng là tham số) + `ml/src/nga_bt.py` (C == Python 130/130 bước) + `tools/chon_nguong_tang2.py` (dò lưới trên người huấn luyện, báo trên người kiểm tra). Ngưỡng **chưa chọn**, `NGA_BAT = 0` | giả lập |
| Quy trình thu | `docs/quy_trinh_thu_du_lieu_bap_tay.md` (an toàn, 19 hoạt động, chọn trục, đặt tên, xử lý, chỉ tiêu tối thiểu) thay `data_protocol.md` (ngực, đã xóa) | – |
| Tài liệu | IPUG931 và Register Map MPU6050 (RM-MPU-6000A-00 Rev 4.0) vào `docs/datasheet/`; 9 thanh ghi firmware dùng đã đối chiếu | – |
| Firmware | flash 17,5 KB / 32 KB, RAM 5,75 KB / 8 KB (vòng UART còn 512/256 B); bản đối chứng 26,9 KB / 7,25 KB | linker |

Không làm được trên máy: H1–H4 (board, đo dòng, **PCB chưa vẽ** vì cần datasheet linh kiện của lô mua và số đo dòng H3), thu dữ liệu, bước 7 (ngưỡng thật, mô hình bắp tay), 7b, đưa lên GitHub (nhóm tự chạy, mục 14).
### 15.8 Nhiệm vụ bắt buộc khi board đã chạy thật (bước H1b, yêu cầu của nhóm 12/11/2026)

**Nguyên tắc: khi đã có board chạy thật thì chỉ lấy thông số thật.** Các số hiện có đến từ mô phỏng (Icarus), giả lập (Unicorn), hoặc là ước tính; chúng chỉ dùng tạm khi chưa có board. Mỗi khi đo được trên board một đại lượng, **thay số đó và xóa số mô phỏng/giả lập/ước tính tương ứng** khỏi tài liệu trình bày (đề xuất, báo cáo, slide, `PROJECT_NCKH.md`, SPEC, kế hoạch). Không để hai số (mô phỏng và đo) cùng tồn tại cho một đại lượng.

| Thông số đang dùng (nhãn hiện tại) | Ở đâu | Thay bằng phép đo trên board |
|---|---|---|
| 70.010 chu kỳ = 2,59 ms mỗi bước [mô phỏng, netlist] | `PROJECT_NCKH.md`, SPEC A.7, kế hoạch 15.1, `docs/bao_cao/2026-11-11_rtl_inc_apb`, `gl_inc` | `CYCLES` đọc qua APB in ở dòng `TK`/`S` (H1) |
| CNN phần mềm ~1,17 triệu lệnh, ước 45–65 ms [giả lập, ước tính] | `PROJECT_NCKH.md`, `2026-11-11_fw_m3_gia_lap`, kế hoạch 15.1 | Chu kỳ DWT ở dòng `W,…` của bản `DOI_CHUNG=1` (bước 8) |
| Khớp bit CNN phần mềm 150/150, tự gỡ cảm biến, WiFi/web, `/raw` chạy đúng [giả lập] | `2026-11-11_fw_m3_gia_lap`, 15.7 | Tự kiểm `TK,1` trên board, thử gây treo 20/20 (H2), web trên trình duyệt + ESP-01S thật (bước 6), dòng `W,…KHOP` (bước 8) |
| Tầng 2 C == Python, đường ống thiết bị == `windowing.py` [giả lập] | 15.7 | Cùng log thô thật chạy qua thiết bị và Python |
| Fmax 41,1 MHz, logic 24 %, BSRAM 8/10 [P&R] | `PROJECT_NCKH.md`, `2026-11-11_gowin_s0_buoc4` | Giữ (là báo cáo của chính bitstream nạp lên board), nhưng ghi kèm "bitstream đã chạy trên board" và xác nhận chân xung nhịp 45 không lỗi thời gian (H1) |
| Flash 17,5 KB / RAM 5,75 KB [linker] | `PROJECT_NCKH.md`, 15.7 | Giữ (kích thước của chính ảnh nạp), ghi kèm "ảnh đã chạy trên board" |
| Dòng ESP-01S ~170 mA, khối lượng 70–125 g, dòng đỉnh ~0,8 A, sụt áp Q3 0,1–0,2 V, thời gian xả 47 ms, 0,4 ms đọc I²C [ước tính, datasheet] | mục 7.5, 12.1, 12.5 | Số đo H3 (dòng từng nhánh), cân thiết bị, đo điện áp VCC_IMU, thời gian đo bằng DWT |
| Độ chính xác nhận dạng: chưa có số nào (mô hình thử ngẫu nhiên) | – | Chỉ báo số đo trên **dữ liệu bắp tay thật**, tập kiểm tra tách theo người (sau bước 7, 7b) |
| Năng lượng mỗi lần suy luận (Nội dung 5): chưa có | – | Đo dòng × thời gian thật (H3 + bước 8) |

Báo cáo cũ `docs/bao_cao/2026-11-11_*`, `2026-11-12_*` giữ làm nhật ký quá trình (có ngày, có nhãn), nhưng không trích số của chúng vào tài liệu trình bày nữa khi đã có số đo.
