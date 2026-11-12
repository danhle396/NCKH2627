#!/usr/bin/env python
# tools/o3_dense_2_lop.py — kế hoạch O3 (bước 0 của mục 10): bỏ 2 lớp đầu ra không dùng, Dense 512 → 2.
#
#   python -I tools/o3_dense_2_lop.py --params ml/out/mo_hinh_thu_4lop/params_int8.npz --out ml/out/mo_hinh_thu \
#          --report docs/bao_cao/<ngày>_o3_dense_2_lop
#
# Lớp mới: 0 = NORMAL (lớp 0 cũ), 1 = FALL (lớp 3 cũ). Firmware đổi mã 1 → 3 (CLS_FALL) trước safety_on_result().
# Kiểm:
#   1. 50 vector của export_mem.py (tổng hợp + vector biên + ngẫu nhiên, cùng seed): lớp 2 lớp (đổi 1 → 3) == lớp 4 lớp.
#   2. 20.000 cửa sổ tổng hợp (ml/src/luong_tong_hop.py), hoặc --data <npz cửa sổ có nhãn>: như trên, đếm số cửa sổ
#      mô hình 4 lớp chọn lớp 1 hoặc 2, so SCORE và phiếu FALL. (11/11/2026 đã chạy trên 109.970 cửa sổ D1 trước khi
#      xóa D1: 109.970/109.970, docs/bao_cao/2026-11-11_o3_dense_2_lop.)
# Không huấn luyện lại: trọng số và bias của 2 hàng giữ nguyên từng bit.
import argparse
import json
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "ml" / "src"))
sys.path.insert(0, str(ROOT / "tools"))
import cnn_numpy as cn  # noqa: E402
from golden_model import forward  # noqa: E402

KEEP = [0, 3]                          # hàng Dense giữ lại: NORMAL, FALL
MAP = np.array(KEEP)                   # mã 2 lớp → mã 4 lớp (driver: 1 → 3)


def rel(p):
    p = pathlib.Path(p).resolve()
    try:
        return p.relative_to(ROOT).as_posix()
    except ValueError:
        return str(p)


def load(p):
    z = np.load(p)
    P = {k: z[k] for k in z.files}
    P["n"] = [int(v) for v in P["n"]]
    P["head"] = str(P["head"])
    P["score_shift"] = int(P["score_shift"])
    return P


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--params", type=pathlib.Path, default=ROOT / "ml/out/mo_hinh_thu_4lop/params_int8.npz")
    ap.add_argument("--out", type=pathlib.Path, default=ROOT / "ml/out/mo_hinh_thu")
    ap.add_argument("--report", type=pathlib.Path)
    ap.add_argument("--data", default="", help="npz cửa sổ có nhãn (X, y); bỏ trống = 20.000 cửa sổ tổng hợp")
    a = ap.parse_args()
    P4 = load(a.params)
    P2 = dict(P4)
    P2["wd"], P2["bd"] = P4["wd"][KEEP].copy(), P4["bd"][KEEP].copy()
    rep = []

    def log(s=""):
        print(s)
        rep.append(s)

    log(f"mô hình gốc: {rel(a.params)}; Dense {P4['wd'].shape} → {P2['wd'].shape}")
    for r in range(4):
        log(f"  hàng {r}: {np.count_nonzero(P4['wd'][r])}/512 trọng số khác 0, bias {int(P4['bd'][r])}"
            f"{'  (giữ)' if r in KEEP else '  (bỏ)'}")
    nw = sum(P2[f"w{i}"].size for i in (1, 2, 3)) + P2["wd"].size
    nb = sum(P2[f"b{i}"].size for i in (1, 2, 3)) + P2["bd"].size
    log(f"  trọng số {nw}, bias {nb}")

    # 1. 50 vector của export_mem.py
    import export_mem as em
    ns = argparse.Namespace(data=a.data, nvec=50)
    X, tag = em.vectors(ns, np.random.default_rng(4))
    bad1, sc_diff1 = 0, 0
    for x in X:
        c4, s4, _ = forward(x, P4)
        c2, s2, _ = forward(x, P2)
        bad1 += MAP[c2] != c4
        sc_diff1 += s2 != s4
    ok1 = bad1 == 0
    log(f"{'PASS' if ok1 else 'FAIL'}  50 vector export_mem (seed 4): lớp 2 lớp (đổi 1 → 3) == lớp 4 lớp: "
        f"{50 - bad1}/50; SCORE khác nhau ở {sc_diff1}/50")

    # 2. nhiều cửa sổ
    if a.data:
        d = np.load(a.data)
        Xd, y, nguon = d["X"], d["y"], "cửa sổ có nhãn"
    else:
        from luong_tong_hop import cua_so
        Xd, y, nguon = cua_so(20000, 11), None, "cửa sổ tổng hợp"
    c4, lg4, m4 = cn.int_forward(Xd, P4)
    c2, lg2, m2 = cn.int_forward(Xd, P2)
    assert np.array_equal(lg2, lg4[:, KEEP])
    n = len(Xd)
    pick12 = int(np.isin(c4, [1, 2]).sum())
    diff = int((MAP[c2] != c4).sum())
    ss = P4["score_shift"]
    s4, s2 = cn.score(m4, ss), cn.score(m2, ss)
    sdiff = int((s4 != s2).sum())
    log(f"{'PASS' if diff == 0 else 'FAIL'}  {n} {nguon}: lớp 2 lớp (đổi 1 → 3) == lớp 4 lớp ở {n - diff}/{n}; "
        f"mô hình 4 lớp chọn lớp 1 hoặc 2 ở {pick12} cửa sổ")
    log(f"      SCORE (score_shift {ss}) khác nhau ở {sdiff}/{n} cửa sổ "
        f"(hai lớp: top2 luôn là lớp còn lại; bốn lớp: top2 có thể là logit IMPACT)")
    for t in (0, 4, 8, 16, 32):
        v4 = (c4 == 3) & (s4 >= t)
        v2 = (c2 == 1) & (s2 >= t)
        log(f"      T_SCORE {t:>2}: phiếu FALL 4 lớp {int(v4.sum())}, 2 lớp {int(v2.sum())}, khác nhau {int((v4 != v2).sum())}")
    if y is not None:
        cm = np.zeros((2, 2), int)
        for t_, p_ in zip((y == 3).astype(int), c2):
            cm[t_, p_] += 1
        log(f"      ma trận nhầm lẫn 2 lớp (hàng = thật NORMAL, FALL): {cm.tolist()}")
    else:
        log(f"      lớp 2 lớp trên cửa sổ tổng hợp: NORMAL {int((c2 == 0).sum())}, FALL {int((c2 == 1).sum())}")
    ok2 = diff == 0

    # xuất
    a.out.mkdir(parents=True, exist_ok=True)
    np.savez(a.out / "params_int8.npz", **{k: v for k, v in P2.items()})
    q = json.loads((a.params.parent / "quant_params.json").read_text(encoding="utf8"))
    q.update(classes={"0": "NORMAL", "1": "FALL"}, weight_count=int(nw), bias_count=int(nb),
             source=f"tools/o3_dense_2_lop.py từ {rel(a.params)}: giữ hàng Dense {KEEP}, "
                    "firmware đổi mã 1 → 3 (CLS_FALL)")
    (a.out / "quant_params.json").write_text(json.dumps(q, ensure_ascii=False, indent=2) + "\n", encoding="utf8")
    log(f"xuất {rel(a.out)}/params_int8.npz, quant_params.json")
    ok = ok1 and ok2
    log("PASS" if ok else "FAIL")
    if a.report:
        a.report.mkdir(parents=True, exist_ok=True)
        (a.report / "o3_dense_2_lop.log").write_text("\n".join(rep) + "\n", encoding="utf8")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
