# Quy trình thu dữ liệu đeo bắp tay (kế hoạch mục 10, bước 7)

Viết 12/11/2026. Dữ liệu của đề tài **chỉ** là dữ liệu bắp tay nhóm tự thu theo quy trình này (không dùng dữ liệu công khai hay dữ liệu vị trí khác). Thay cho `docs/tham_khao/data_protocol.md` (đeo ngực, bản dự thi, đã xóa).

## 1. An toàn và đồng ý — bắt buộc trước buổi có ngã

- Người tham gia đọc và ký đồng ý; được dừng bất cứ lúc nào. Không thu người có bệnh xương khớp, tim mạch, đang mang thai.
- Ngã **chỉ lên nệm dày ≥ 20 cm**, phủ kín vùng ngã, có người đỡ đứng cạnh. Khởi động 5 phút trước khi ngã. Không ngã đập đầu, không ngã từ chỗ cao.
- Thiết bị trên bắp tay không có cạnh sắc; ngã sang bên đeo thiết bị chỉ làm khi nệm đủ dày.
- Mỗi người tối đa 15 lần ngã mỗi buổi, nghỉ khi mệt.

## 2. Thiết bị và kiểm tra trước mỗi buổi

1. GY-521 gắn cố định trên băng bắp tay (trái hay phải: chốt một bên cho cả nhóm, ghi vào biên bản), mặt module áp vào tay, hướng X/Y in trên module ghi lên băng. **Không tháo module giữa các buổi**; nếu tháo phải chạy lại mục 4.
2. Bật thiết bị, chờ 3 s đứng yên (hiệu chỉnh con quay, UART in `GC,…`). Kiểm UART/web: `TK,1,…` và web thấy số liệu.
3. Máy tính nối WiFi `NCKH2627`; thử `python -I tools/thu_bap_tay.py --nguoi s00 --hd di_bo --lan 1 --giay 10` (s00 = thử, không vào dữ liệu): phải ra 1.000 mẫu, mất 0.
4. Đặt băng ở cùng một vị trí trên bắp tay (giữa vai và khuỷu, mặt ngoài); chụp ảnh vị trí đeo cho biên bản.

## 3. Hoạt động và số lần (mỗi người)

Tên hoạt động đúng như trong `tools/windowing.py` (`ACTS`). Lớp: 0 NORMAL, 3 FALL. "Nút" = người đỡ bấm nút trên thiết bị đúng lúc sự kiện để đánh dấu.

| Hoạt động | Mô tả | Lần × thời lượng | Đánh dấu |
|---|---|---|---|
| `di_bo` | đi bộ thường, có vung tay tự nhiên | 2 × 2 phút | – |
| `chay` | chạy chậm | 2 × 1 phút | – |
| `cau_thang` | lên xuống cầu thang | 2 × 1 phút | – |
| `ngoi_dung` | ngồi xuống ghế rồi đứng lên, lặp | 2 × 1 phút | – |
| `nam_xuong` | nằm xuống giường rồi ngồi dậy, lặp | 2 × 1 phút | – |
| `cui_nhat` | cúi nhặt đồ dưới đất, lặp | 2 × 1 phút | – |
| `vung_tay` | vung tay mạnh, giơ tay, với đồ trên cao, chải đầu | 2 × 1 phút | – |
| `lam_viec` | ngồi gõ phím, viết, ăn uống | 1 × 3 phút | – |
| `nam_lan` | nằm trên nệm, trở mình | 1 × 1 phút | – |
| `dap_tay` | đập tay xuống bàn, vỗ tay mạnh (va chạm không phải ngã) | 2 × 30 s | nút ở mỗi lần đập |
| `ngoi_phich` | ngồi phịch mạnh xuống ghế / giường | 2 × 30 s | nút ở mỗi lần |
| `suyt_nga` | vấp, trượt chân rồi giữ được thăng bằng | 2 × 30 s | nút ở mỗi lần |
| `nga_truoc` | ngã về trước lên nệm, nằm yên 10 s | 3 lần (mỗi lần 1 file) | – (tự tìm va chạm) |
| `nga_sau` | ngã ngửa ra sau, nằm yên 10 s | 3 | – |
| `nga_trai` | ngã sang trái, nằm yên 10 s | 3 | – |
| `nga_phai` | ngã sang phải, nằm yên 10 s | 3 | – |
| `nga_ngoi` | ngã khi đang ngồi ghế | 2 | – |
| `nga_truot` | trượt chân ngã | 2 | – |
| `nga_ngat` | gục từ từ như ngất rồi nằm yên | 2 | – |

Mỗi file ngã: đứng yên 5 s → ngã → nằm yên ≥ 10 s → dừng ghi. **Một file chỉ một cú ngã.**

## 4. Chọn trục (một lần, sau khi gắn module lên băng)

Mỗi tư thế nằm yên ~12 s:

```bash
python -I tools/thu_bap_tay.py --nguoi s00 --hd nam_lan --lan 1 --giay 12   # đứng thẳng tay buông → đổi tên dung.csv
python -I tools/thu_bap_tay.py --nguoi s00 --hd nam_lan --lan 2 --giay 12   # nằm ngửa, tay dọc thân → nam_ngua.csv
python -I tools/thu_bap_tay.py --nguoi s00 --hd nam_lan --lan 3 --giay 12   # nằm nghiêng phải → nghieng_phai.csv
python -I tools/chon_truc.py --dung dung.csv --nam-ngua nam_ngua.csv --nghieng-phai nghieng_phai.csv
```

Dán các dòng `TRUC_*`, `ACC_OFF_*` in ra vào `fw_arm/src/app/vi_tri_deo.h`, build và nạp lại firmware **trước** khi thu dữ liệu (log ghi theo trục cảm biến nên đổi trục sau vẫn được, nhưng thiết bị chạy thật phải có trục đúng).

## 5. Thu và đặt tên

```bash
python -I tools/thu_bap_tay.py --nguoi s01 --hd di_bo --lan 1 2 --giay 120
python -I tools/thu_bap_tay.py --nguoi s01 --hd nga_truoc --lan 1 3          # Enter bắt đầu, Enter dừng mỗi lần
```

File ra `data/raw/<người>_<hoạt động>_<lần>.csv` (không lên git; nhóm tự sao lưu). Người: `s01`, `s02`, … theo thứ tự tham gia; biên bản giữ riêng tên thật, tuổi, giới, chiều cao, cân nặng, tay đeo (không ghi vào file dữ liệu). Thu lại lần nào có cảnh báo mất mẫu.

## 6. Xử lý, huấn luyện, chọn ngưỡng

```bash
python -I tools/windowing.py data/raw --out data/bap_tay/windows.npz --plot docs/bao_cao/<ngày>_du_lieu_bap_tay
python ml/train_cnn.py --arch n2 --data data/bap_tay/windows.npz --out ml/out/bap_tay_<ngày>
python -I tools/chon_nguong_tang2.py data/raw --kiem <2 người kiểm tra>
```

- Tách theo **người** (không tách theo cửa sổ: cửa sổ liền nhau trùng 87,5 %). Giữ ít nhất 2 người chỉ để kiểm tra cuối.
- Xem hình `--plot` của từng cú ngã: điểm sự kiện phải nằm ở va chạm.
- Sau khi có mô hình: `tools/o3_dense_2_lop.py --params …`, `tools/export_inc.py`, chép vào `rtl/src/` và `fw_arm/src/gen/`, chạy `bash rtl/scripts/dong_bo_gowin.sh` (cập nhật bản sao cho Gowin), rồi làm **bước 7b** (xóa mô hình thử và mọi dữ liệu giả lập, kế hoạch 15.6).

## 7. Chỉ tiêu tối thiểu trước khi huấn luyện [đề xuất, chốt cùng GVHD]

≥ 10 người; mỗi người đủ bảng mục 3 (≈ 18 lần ngã, ≈ 25 phút sinh hoạt); tổng ≥ 150 cú ngã, ≥ 4 giờ sinh hoạt.
