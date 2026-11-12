#!/usr/bin/env python
# tools/chon_nguong_tang2.py — chọn ngưỡng tầng 2 bắp tay (fw_arm/src/core/nga_bt.c, kế hoạch 7.2, bước 7) trên log thô thật.
#
#   python -I tools/chon_nguong_tang2.py data/raw --kiem s05 s06 [--fp-max 0.5] [--tre-suspect 1.0]
#   python -I tools/chon_nguong_tang2.py --tu-kiem          # kiểm hàm quyết định theo khối == ml/src/nga_bt.py (phép toán)
#
# Đọc log như tools/windowing.py (đổi trục theo vi_tri_deo.h). Với mỗi file:
#   - ngã (mã ≥ 900): giả sử CNN vào SUSPECT `--tre-suspect` giây sau điểm va chạm (|a| lệch 1 g nhiều nhất) → tầng 2 có
#     xác nhận không (recall);
#   - sinh hoạt: giả sử CNN nghi ở MỌI khối (trường hợp xấu nhất) → số lần tầng 2 xác nhận nhầm mỗi giờ.
# Dò lưới ngưỡng trên người huấn luyện (không thuộc --kiem): lấy bộ có recall cao nhất với xác nhận nhầm ≤ --fp-max lần/giờ;
# báo cả hai số trên người kiểm tra. In #define cho vi_tri_deo.h (rồi đặt NGA_BAT 1).
import argparse
import itertools
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "ml" / "src"))
sys.path.insert(0, str(ROOT / "tools"))
from nga_bt import NgaBT, XAC_NHAN, KHOI_MAU  # noqa: E402

LUOI = dict(roi_lsb=[600, 800, 1000, 1200], va_lsb=[4096, 5120, 6144, 8192], quay_lsb=[2460, 3280, 4920, 6560],
            yen_a=[200, 300, 400], yen_g=[200, 300, 400], tim_khoi=[8], roi_khoi=[2, 3], yen_khoi=[6, 9],
            cho_ms=[4000])


def dac_trung(body):
    """body (N, 6) trục người → mảng theo khối: a2_min, a2_max, w2_max, biên độ 6 trục (max − min)."""
    n = len(body) // KHOI_MAU
    b = body[:n * KHOI_MAU].astype(np.int64).reshape(n, KHOI_MAU, 6)
    a2 = (b[:, :, :3] ** 2).sum(2) & 0xFFFFFFFF
    w2 = (b[:, :, 3:] ** 2).sum(2) & 0xFFFFFFFF
    return a2.min(1), a2.max(1), w2.max(1), b.max(1) - b.min(1)


def quyet_dinh(f, k, c):
    """SUSPECT ngay sau khi khối k đóng (k = số khối đã đóng − 1). Trả về True nếu XÁC NHẬN — đúng logic nga_bt.c."""
    a2min, a2max, w2max, bien = f
    yen = np.all(bien[:, :3] < c["yen_a"], 1) & np.all(bien[:, 3:] < c["yen_g"], 1)
    lo = max(0, k - c["tim_khoi"] + 1, k - 35)
    seg = a2max[lo:k + 1][::-1]                          # lùi từ khối mới nhất; bằng nhau → khối mới hơn (như C)
    vc = k - int(np.argmax(seg))
    if seg.size == 0 or a2max[vc] < c["va_lsb"] ** 2:
        return False
    if not np.any(a2min[max(0, vc - c["roi_khoi"]):vc + 1] <= c["roi_lsb"] ** 2):
        return False
    nmax = min(len(a2max) - 1, vc + c["yen_khoi"])
    for j in range(k, nmax + 1):                         # j = khối mới nhất đã đóng khi xét
        quay = np.any(w2max[max(0, vc - 1):min(j, vc + 1) + 1] >= c["quay_lsb"] ** 2)
        im = np.any(yen[vc + 1:j + 1])
        if quay and im:
            return True
        if (j - k) * 320 >= c["cho_ms"]:
            return False
    return False


def tu_kiem():
    rng = np.random.default_rng(2)
    bad = 0
    for it in range(300):
        c = {k: int(rng.choice(v)) for k, v in LUOI.items()}
        body = rng.integers(-9000, 9000, (KHOI_MAU * 30, 6))
        if it % 2:
            t = 32 * 18 + int(rng.integers(0, 32))
            body[t - 20:t, :3] = rng.integers(-300, 300, (20, 3))
            body[t:t + 3, :3] = 30000
            body[t + 70:] = body[t + 70]
        k = 20
        py = NgaBT(c)
        for s in body[:KHOI_MAU * (k + 1)]:
            py.mau([int(v) for v in s])
        py.bat_dau(0)
        r, j = py.buoc(0), k + 1
        while r == 0 and j < 30:
            for s in body[KHOI_MAU * j:KHOI_MAU * (j + 1)]:
                py.mau([int(v) for v in s])
            j += 1
            r = py.buoc((j - k - 1) * 320)
        bad += (r == XAC_NHAN) != quyet_dinh(dac_trung(body), k, c)
    print(f"{'PASS' if bad == 0 else 'FAIL'}  tự kiểm: quyết định theo khối == ml/src/nga_bt.py ở {300 - bad}/300 trường hợp")
    print("PASS" if bad == 0 else "FAIL")
    return bad == 0


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="*", type=pathlib.Path)
    ap.add_argument("--kiem", nargs="*", default=[], help="người để kiểm tra (không dùng chọn ngưỡng)")
    ap.add_argument("--fp-max", type=float, default=0.5, help="xác nhận nhầm tối đa mỗi giờ sinh hoạt (tập huấn luyện)")
    ap.add_argument("--tre-suspect", type=float, default=1.0, help="giây từ va chạm tới khi CNN vào SUSPECT")
    ap.add_argument("--tu-kiem", action="store_true")
    a = ap.parse_args()
    if a.tu_kiem:
        sys.exit(0 if tu_kiem() else 1)
    from windowing import ACTS, parse_name, read_log, find_events
    from vi_tri import doc_cau_hinh, doi_truc
    cfg_vt = doc_cau_hinh()
    files = sorted(f for s in a.src for f in ([s] if s.is_file() else s.glob("*.csv")))
    ds = []
    for f in files:
        try:
            subj, _, act, _ = parse_name(f)
        except ValueError:
            continue
        _, raw, _, _ = read_log(f)
        body = doi_truc(raw, cfg_vt)
        nga = ACTS[act][1] >= 900
        k = (find_events(raw, None, "single")[0] + int(a.tre_suspect * 100)) // KHOI_MAU if nga else None
        ds.append((subj, nga, dac_trung(body), k, len(body) / 360000.0))
    if not ds:
        raise SystemExit("không có log hợp lệ")

    def danh_gia(c, chon):
        tp = nf = fp = gio = 0
        for subj, nga, f, k, h in ds:
            if (subj in a.kiem) != chon:
                continue
            if nga:
                nf += 1
                tp += k is not None and k < len(f[0]) and quyet_dinh(f, k, c)
            else:
                gio += h
                fp += sum(quyet_dinh(f, j, c) for j in range(len(f[0])))
        return tp, nf, fp, gio
    best = None
    keys = list(LUOI)
    for combo in itertools.product(*[LUOI[k] for k in keys]):
        c = dict(zip(keys, combo))
        tp, nf, fp, gio = danh_gia(c, False)
        fph = fp / gio if gio else 0.0
        if nf and fph <= a.fp_max:
            key = (tp / nf, -fph)
            if best is None or key > best[0]:
                best = (key, c)
    if best is None:
        raise SystemExit(f"không bộ ngưỡng nào đạt xác nhận nhầm ≤ {a.fp_max}/giờ: thu thêm dữ liệu hoặc nới --fp-max")
    c = best[1]
    for ten, chon in (("huấn luyện", False), ("kiểm tra", True)):
        tp, nf, fp, gio = danh_gia(c, chon)
        print(f"{ten}: bắt {tp}/{nf} cú ngã, xác nhận nhầm {fp} lần / {gio:.2f} giờ sinh hoạt")
    print("\nDán vào khối VT_BAP_TAY của fw_arm/src/app/vi_tri_deo.h (và đổi NGA_BAT thành 1):")
    for k in keys:
        print(f"#define NGA_{k.upper():12s} {c[k]}{'u' if k in ('tim_khoi', 'roi_khoi', 'yen_khoi', 'cho_ms') else ''}")


if __name__ == "__main__":
    main()
