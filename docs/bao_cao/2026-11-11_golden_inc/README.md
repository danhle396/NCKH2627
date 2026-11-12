# Golden tăng dần (kế hoạch O1, mục 10 bước 1a) — 11/11/2026

Golden: `ml/src/golden_inc.py` (mô hình địa chỉ của RTL). Kiểm: `python -I tools/kiem_golden_inc.py --report …` (log: `golden_inc.log`). Nhãn: [mô phỏng].

| Kiểm | Kết quả |
|---|---|
| 41 lần thử ngã D1 liên tục (lần dài nhất 97 bước + 40 lần ngẫu nhiên, mỗi lần ≥ 50 bước), đệm khởi tạo ngẫu nhiên | **41/41** lần khớp bit golden E2 hai lớp ở mọi bước hợp lệ: lớp, SCORE, logit **và l1, l2, l3 của cả cửa sổ** |
| D1 sinh hoạt 106 bước, SOFT_RST ở bước 30 | 92/92 bước hợp lệ khớp bit |
| D1 ghép 6 đoạn có ngã (dữ liệu nhảy giữa các đoạn) | 46/46 |
| Ngẫu nhiên + đoạn bão hòa ±127/−128, 0, xen kẽ; SOFT_RST ở bước 25 | 46/46 |
| Cờ hợp lệ (WARM = 8) đúng sau đúng 8 bước kể từ SOFT_RST | 0 lần sai |

(Dòng "D1 có ngã 3 bước, hợp lệ 0" trong log là đoạn cửa sổ D1 cắt quanh sự kiện, quá ngắn để kiểm; các lần thử ngã liên tục ở trên thay cho nó.)

**Số MAC mỗi bước: 61.824** (E2 hai lớp: 226.304 MAC mỗi cửa sổ → ít hơn **3,7 lần**) [đếm trong golden]. Kế hoạch ước thô 30–39 nghìn là thấp, vì mỗi bước phải tính lại 10 vị trí L1, 8 vị trí L2, 6 vị trí L3, gồm cả hai mép.

**Hai điều kế hoạch chưa tính tới, đã giải và chứng minh bằng chạy thử:**
1. Tính lại L1[0] (sát mép trái) cần 4 mẫu **đầu** cửa sổ x[0..3], không chỉ 16 mẫu mới + 4 mẫu cũ ở đuôi. M3 giữ cả cửa sổ nên gửi **144 byte mỗi bước** (x[0..3] + x[108..127]) thay vì 96 byte.
2. GW1NS-4 không có RAM phân tán, nên 144 byte STAGE đặt **ngay trong SDPB đệm đặc trưng**, ở 6 ô L3 sẽ tính lại trong bước đó (192 B). Các ô này đã "chết" sau Dense của bước trước, và chỉ bị ghi đè khi tính lớp 3, sau khi lớp 1 đọc xong STAGE. Golden đặt STAGE đúng chỗ đó và vẫn khớp bit ở mọi luồng trên.
