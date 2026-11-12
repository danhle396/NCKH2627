#!/usr/bin/env python
# tools/tao_mo_hinh_thu.py — MÔ HÌNH THỬ: trọng số INT8 NGẪU NHIÊN, KHÔNG huấn luyện từ dữ liệu nào.
#
#   python -I tools/tao_mo_hinh_thu.py            → ml/out/mo_hinh_thu_4lop/ (4 lớp, cho E2 đối chiếu và bước O3)
#   python -I tools/o3_dense_2_lop.py             → ml/out/mo_hinh_thu/ (2 lớp, nạp vào ROM bộ tăng tốc)
#
# Mục đích duy nhất: kiểm khớp bit RTL, netlist, firmware khi CHƯA có mô hình bắp tay (repo không giữ dữ liệu nào khác
# ngoài dữ liệu bắp tay nhóm tự thu, quyết định 11/11/2026). Kết quả nhận dạng của mô hình này VÔ NGHĨA. Firmware in tên
# mô hình (CNN_MO_HINH = "THU_NGAU_NHIEN") ở UART và web để không nhầm với mô hình thật.
# Khi có mô hình bắp tay: ml/train_cnn.py --data <npz bắp tay> → o3_dense_2_lop.py --params <mô hình> → export_inc.py.
# Cấu trúc N2 như SPEC A.5: Conv 6→16, 16→16, 16→32 (k = 5), shift 8, Flatten 512, Dense 512 → 4 (lớp 1, 2 dự trữ:
# trọng số 0, bias −2³⁰). Bias Dense lớp 3 chỉnh để trên luồng tổng hợp hai lớp NORMAL/FALL xuất hiện gần đều nhau
# (để kiểm phủ cả hai nhánh).
import json
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "ml" / "src"))
import cnn_numpy as cn  # noqa: E402
from luong_tong_hop import cua_so  # noqa: E402

TEN = "THU_NGAU_NHIEN"


def main():
    sys.stdout.reconfigure(encoding="utf8")
    rng = np.random.default_rng(20261112)

    def w(*shape):
        return np.clip(np.round(rng.normal(0, 25, shape)), -127, 127).astype(np.int64)

    P = dict(w1=w(16, 6, 5), w2=w(16, 16, 5), w3=w(32, 16, 5),
             b1=np.round(rng.normal(0, 1500, 16)).astype(np.int64),
             b2=np.round(rng.normal(0, 1500, 16)).astype(np.int64),
             b3=np.round(rng.normal(0, 1500, 32)).astype(np.int64),
             wd=np.zeros((4, 512), np.int64), bd=np.array([0, 0, -(1 << 30), 0], np.int64),
             n=np.array([8, 8, 8]), head=np.array("flatten"), score_shift=np.array(9))
    P["wd"][0], P["wd"][3] = w(512), w(512)
    P["wd"][1] = 0
    P["bd"][1] = -(1 << 30)                       # lớp 1, 2 dự trữ như SPEC A.4 cũ
    Q = {k: (v if k not in ("n", "head", "score_shift") else v) for k, v in P.items()}
    Q["n"] = [8, 8, 8]
    Q["head"] = "flatten"
    _, lg, _ = cn.int_forward(cua_so(4000, 5), Q)
    P["bd"][3] = int(np.median(lg[:, 0] - lg[:, 3]))                 # cân hai lớp trên cửa sổ tổng hợp
    Q["bd"] = P["bd"]
    c, _, _ = cn.int_forward(cua_so(4000, 6), Q)
    out = ROOT / "ml/out/mo_hinh_thu_4lop"
    out.mkdir(parents=True, exist_ok=True)
    np.savez(out / "params_int8.npz", **P)
    q = {"ten": TEN, "arch": "n2", "huan_luyen": "KHÔNG — trọng số ngẫu nhiên, chỉ để kiểm khớp bit",
         "input_shape": [128, 6], "conv1_shift": 8, "conv2_shift": 8, "conv3_shift": 8, "head": "flatten",
         "score_shift": 9, "classes": {"0": "NORMAL", "1": "RESERVED", "2": "RESERVED", "3": "FALL"},
         "source": "tools/tao_mo_hinh_thu.py (seed 20261112)"}
    (out / "quant_params.json").write_text(json.dumps(q, ensure_ascii=False, indent=2) + "\n", encoding="utf8")
    (out / "README.md").write_text(
        "# Mô hình thử (KHÔNG huấn luyện)\n\nTrọng số INT8 ngẫu nhiên do `tools/tao_mo_hinh_thu.py` sinh, chỉ để kiểm khớp "
        "bit RTL / netlist / firmware khi chưa có mô hình bắp tay. Kết quả nhận dạng vô nghĩa. Thay bằng mô hình bắp tay ở "
        "bước 7 của kế hoạch.\n", encoding="utf8")
    print(f"{out.relative_to(ROOT).as_posix()}: lớp trên 4.000 cửa sổ tổng hợp khác: NORMAL {int((c == 0).sum())}, "
          f"FALL {int((c == 3).sum())}, lớp 1/2 {int(np.isin(c, [1, 2]).sum())}")


if __name__ == "__main__":
    main()
