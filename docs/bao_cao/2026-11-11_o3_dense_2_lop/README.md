# O3: Dense 512 → 2 (kế hoạch mục 10, bước 0) — 11/11/2026

Lệnh: `python -I tools/o3_dense_2_lop.py --report docs/bao_cao/2026-11-11_o3_dense_2_lop` (log: `o3_dense_2_lop.log`). Nhãn: [mô phỏng] (golden Python, số nguyên).

| Kiểm | Kết quả |
|---|---|
| 50 vector của `export_mem.py` (seed 4): lớp 2 lớp (đổi 1 → 3) == lớp 4 lớp | **50/50** (Pass của bước 0) |
| Toàn bộ 109.970 cửa sổ D1 | **109.970/109.970**; mô hình 4 lớp chưa lần nào chọn lớp 1 hay 2 |
| Phiếu FALL ở T_SCORE 0, 4, 8, 16, 32 | trùng hoàn toàn (0 khác nhau) |
| SCORE | khác ở 87/109.970 cửa sổ (top2 của 4 lớp có thể là logit IMPACT); không ảnh hưởng vì `T_SCORE = 0` |

**Sửa kế hoạch:** O3 ghi "hai lớp dự trữ có trọng số 0 và bias −2³⁰". Thực tế chỉ lớp 2 như vậy; **lớp 1 (IMPACT) có 488/512 trọng số khác 0, bias −53** (huấn luyện softmax 4 lớp nhưng không có mẫu IMPACT). Bỏ lớp này vẫn không đổi kết quả nhận dạng: đã kiểm trên toàn bộ D1 như bảng trên.

Mô hình 2 lớp: `ml/out/n2_11_d1_2lop/params_int8.npz` (5.344 trọng số, 66 bias; không huấn luyện lại).
