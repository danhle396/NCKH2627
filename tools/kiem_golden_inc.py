#!/usr/bin/env python
# tools/kiem_golden_inc.py — kiểm golden tăng dần (ml/src/golden_inc.py) với golden E2 (ml/src/golden_model.py), bước 1a.
#
#   python -I tools/kiem_golden_inc.py [--params ml/out/mo_hinh_thu/params_int8.npz] [--report docs/bao_cao/<thư mục>]
#
# Luồng thử (ml/src/luong_tong_hop.py; repo không giữ dữ liệu công khai từ 11/11/2026):
#   - 40 luồng tổng hợp × 60 bước: trọng lực + trôi + nhiễu, đổi tư thế, va đập bão hòa, quay mạnh, đứng yên, chỗ nhảy;
#     một luồng có SOFT_RST giữa chừng.
#   - 1 luồng ngẫu nhiên đủ dải + đoạn bão hòa +127, −128, đoạn 0, xen kẽ cực trị; có SOFT_RST.
# Đệm đặc trưng khởi tạo ngẫu nhiên (như BSRAM còn dữ liệu cũ).
# Pass: mọi bước có WARM = 8 trùng golden E2 từng bit: lớp, SCORE, logit, và l1, l2, l3 của cả cửa sổ; cờ hợp lệ đúng.
# (11/11/2026, trước khi xóa D1, đã chạy thêm trên 41 lần thử ngã D1 liên tục: docs/bao_cao/2026-11-11_golden_inc.)
import argparse
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "ml" / "src"))
import golden_inc as gi  # noqa: E402
from golden_model import forward  # noqa: E402
from luong_tong_hop import luong, luong_bien  # noqa: E402


def load(p):
    z = np.load(p)
    P = {k: z[k] for k in z.files}
    P["n"] = [int(v) for v in P["n"]]
    P["head"] = str(P["head"])
    P["score_shift"] = int(P["score_shift"])
    return P


def run_stream(P, name, stream, rng, rst_at=None, log=print):
    eng = gi.IncEngine(P, buf_init=rng.integers(0, 256, gi.BUF_BYTES))
    wins = gi.windows_of(stream)
    ok = bad = early_valid = 0
    ncls = [0, 0]
    since = 0
    for n, w in enumerate(wins):
        if rst_at is not None and n == rst_at:
            eng.soft_rst()
            since = 0
        cls, sc, lg, valid = eng.step(gi.stage_of(w))
        since += 1
        if valid != (since >= gi.WARM_STEPS):
            early_valid += 1
        if not valid:
            continue
        tr = {}
        c0, s0, l0 = forward(w, P, tr)
        l1, l2, l3 = eng.layers()
        same = (cls == c0 and sc == s0 and np.array_equal(lg, l0) and np.array_equal(l1, tr["l1"])
                and np.array_equal(l2, tr["l2"]) and np.array_equal(l3, tr["l3"]))
        ok += same
        bad += not same
        ncls[c0] += 1
        if not same and bad <= 3:
            log(f"    sai ở bước {n}: lớp {cls}/{c0}, logit {lg.tolist()}/{l0.tolist()}, "
                f"l1 {np.array_equal(l1, tr['l1'])} l2 {np.array_equal(l2, tr['l2'])} l3 {np.array_equal(l3, tr['l3'])}")
    nst = len(wins)
    log(f"  {'PASS' if bad == 0 and early_valid == 0 else 'FAIL'}  {name}: {nst} bước"
        f"{'' if rst_at is None else f', SOFT_RST ở bước {rst_at}'}; hợp lệ {ok + bad}, khớp bit {ok}/{ok + bad} "
        f"(NORMAL {ncls[0]}, FALL {ncls[1]}); cờ hợp lệ sai {early_valid}; MAC/bước {eng.macs // nst}")
    return bad == 0 and early_valid == 0, eng.macs // nst, ok + bad, ncls


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--params", type=pathlib.Path, default=ROOT / "ml/out/mo_hinh_thu/params_int8.npz")
    ap.add_argument("--luong", type=int, default=40, help="số luồng tổng hợp")
    ap.add_argument("--report", type=pathlib.Path)
    a = ap.parse_args()
    P = load(a.params)
    rep = []

    def log(s=""):
        print(s)
        rep.append(s)

    rng = np.random.default_rng(20261111)
    log(f"golden tăng dần vs golden E2, mô hình {a.params.name} ({P['wd'].shape[0]} lớp), luồng tổng hợp")
    res = []
    for k in range(a.luong):
        r = run_stream(P, f"luồng tổng hợp #{k}", luong(60, 1000 + k), rng, rst_at=30 if k == 0 else None,
                       log=log if k < 2 else (lambda _s: None))
        res.append(r)
    good = sum(r[0] for r in res)
    nv = sum(r[2] for r in res)
    nc = [sum(r[3][0] for r in res), sum(r[3][1] for r in res)]
    log(f"  {'PASS' if good == len(res) else 'FAIL'}  {len(res)} luồng tổng hợp × 60 bước: {good}/{len(res)} luồng khớp bit "
        f"mọi bước hợp lệ ({nv} bước; golden NORMAL {nc[0]}, FALL {nc[1]})")
    res.append(run_stream(P, "ngẫu nhiên + đoạn bão hòa, 0, xen kẽ", luong_bien(60, 7), rng, rst_at=25, log=log))
    ok = all(r[0] for r in res)
    macs = max(r[1] for r in res)
    log(f"MAC mỗi bước: {macs} (E2 tính lại cả cửa sổ: 226.304 với 2 lớp) → ít hơn {226304 / macs:.1f} lần")
    log("PASS" if ok else "FAIL")
    if a.report:
        a.report.mkdir(parents=True, exist_ok=True)
        (a.report / "golden_inc.log").write_text("\n".join(rep) + "\n", encoding="utf8")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
