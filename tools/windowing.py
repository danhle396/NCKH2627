#!/usr/bin/env python
# tools/windowing.py — log thô 100 Hz đeo BẮP TAY (tools/thu_bap_tay.py) → cửa sổ INT8 có nhãn (docs/spec_nckh.md A.2–A.4).
#
#   python -I tools/windowing.py data/raw --out data/bap_tay/windows.npz [--plot docs/bao_cao/<thư mục>]
#
# Tên file: <người>_<hoạt động>_<lần>.csv (docs/quy_trinh_thu_du_lieu_bap_tay.md); người s01, s02, …
# Cột: t,ax,ay,az,gx,gy,gz,btn — trục cảm biến, con quay đã trừ offset (core/raw_log.h).
# Xử lý đúng như firmware: đổi trục + offset theo fw_arm/src/app/vi_tri_deo.h (ml/src/vi_tri.py = app_on_raw), rồi
# decimate() và q8() của ml/src/golden_model.py (= core/preprocess.c). Đổi vi_tri_deo.h thì chạy lại script này.
# Điểm sự kiện (A.4) = mẫu có |a| lệch khỏi 1 g nhiều nhất: trên cả file (hoạt động ngã, mỗi file một cú) hoặc trong mỗi
# đoạn bấm nút (cột btn). Nhãn: 0 NORMAL, 3 FALL (ml/train_cnn.py huấn luyện 4 lớp rồi tools/o3_dense_2_lop.py còn 2).
# Ra npz khóa X, y, subject, act, trial, p (+ vi_tri) → python ml/train_cnn.py --data <npz>.
import argparse
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "ml" / "src"))
from golden_model import decimate, q8  # noqa: E402
from vi_tri import doc_cau_hinh, doi_truc  # noqa: E402

WIN, HOP = 128, 16
POS = (56, 103)                  # A.4: vùng dương (vị trí điểm sự kiện trong cửa sổ, mẫu 50 Hz)
DROP = ((24, 55), (104, 127))    # A.4: vùng bỏ
NORMAL, FALL = 0, 3
G = 2048                         # LSB/g ở ±16 g
LOST_GAP_MS = 15

# hoạt động → (lớp, mã, cách tìm sự kiện). Mã ≥ 900 là ngã (train_cnn.py tính recall sự kiện); < 900 tính phiếu nhầm/giờ.
# Danh sách và cách làm từng hoạt động: docs/quy_trinh_thu_du_lieu_bap_tay.md mục 3.
ACTS = {
    "di_bo": (NORMAL, 801, "none"), "chay": (NORMAL, 802, "none"), "cau_thang": (NORMAL, 803, "none"),
    "ngoi_dung": (NORMAL, 804, "none"), "nam_xuong": (NORMAL, 805, "none"), "cui_nhat": (NORMAL, 806, "none"),
    "vung_tay": (NORMAL, 807, "none"), "lam_viec": (NORMAL, 808, "none"), "dap_tay": (NORMAL, 809, "button"),
    "ngoi_phich": (NORMAL, 810, "button"), "suyt_nga": (NORMAL, 811, "button"), "nam_lan": (NORMAL, 812, "none"),
    "nga_truoc": (FALL, 901, "single"), "nga_sau": (FALL, 902, "single"), "nga_trai": (FALL, 903, "single"),
    "nga_phai": (FALL, 904, "single"), "nga_ngoi": (FALL, 905, "single"), "nga_truot": (FALL, 906, "single"),
    "nga_ngat": (FALL, 907, "single"),
}


def parse_name(path):
    parts = path.stem.split("_")
    if len(parts) < 3:
        raise ValueError("tên file phải là <người>_<hoạt động>_<lần>.csv")
    subj, trial, act = parts[0], int(parts[-1]), "_".join(parts[1:-1])
    if act not in ACTS:
        raise ValueError(f"hoạt động lạ '{act}'")
    if not (subj.startswith("s") and subj[1:].isdigit() and int(subj[1:]) > 0):
        raise ValueError(f"mã người lạ '{subj}' (s01, s02, …; s00 là file thử trên bàn, không vào dữ liệu)")
    return subj, int(subj[1:]), act, trial


def read_log(path):
    a = np.loadtxt(path, delimiter=",", skiprows=1, dtype=np.int64, ndmin=2)
    t, raw, btn = a[:, 0], a[:, 1:7], a[:, 7]
    gaps = np.diff(t)
    lost = int(np.sum(np.round(gaps[gaps >= LOST_GAP_MS] / 10) - 1)) if len(gaps) else 0
    return t, raw, btn, lost


def find_events(raw, btn, mode):
    dev = np.abs(np.linalg.norm(raw[:, :3].astype(np.float64), axis=1) / G - 1.0)
    if mode == "none":
        return []
    if mode == "single":
        return [int(np.argmax(dev))]
    ev, on = [], np.flatnonzero(np.diff(np.r_[0, (btn > 0).astype(int), 0]))
    for a, b in zip(on[0::2], on[1::2]):                    # mỗi đoạn bấm [a, b)
        ev.append(int(a + np.argmax(dev[a:b])))
    return ev


def windows(q, ev50, label):
    out = []
    for s in range(0, len(q) - WIN + 1, HOP):
        ps = [e - s for e in ev50]
        if any(POS[0] <= p <= POS[1] for p in ps):
            out.append((s, label, next(p for p in ps if POS[0] <= p <= POS[1])))
        elif any(lo <= p <= hi for p in ps for lo, hi in DROP):
            continue
        else:
            out.append((s, NORMAL, next((p for p in ps if 0 <= p < WIN), -1)))
    return out


def tien_xu_ly(raw, cfg):
    """log thô (trục cảm biến) → cửa sổ INT8 50 Hz theo trục người, đúng như firmware."""
    return q8(decimate(doi_truc(raw, cfg)))


def plot_events(path, body, ev, outdir):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    if not ev:
        return
    fig, axs = plt.subplots(len(ev), 1, figsize=(8, 2.2 * len(ev)), squeeze=False)
    for ax, e in zip(axs[:, 0], ev):
        a, b = max(0, e - 300), min(len(body), e + 500)
        tt = (np.arange(a, b) - e) / 100
        ax.plot(tt, np.linalg.norm(body[a:b, :3], axis=1) / G, label="|a| (g)")
        ax.plot(tt, np.linalg.norm(body[a:b, 3:], axis=1) / 16.4 / 100, label="|ω| (100 °/s)")
        ax.axvline(0, color="r", lw=0.8)
        ax.set_xlabel("s so với điểm sự kiện")
        ax.legend(loc="upper right", fontsize=7)
    fig.suptitle(path.name, fontsize=9)
    fig.tight_layout()
    fig.savefig(outdir / f"{path.stem}.png", dpi=90)
    plt.close(fig)


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="+", type=pathlib.Path, help="thư mục hoặc file .csv")
    ap.add_argument("--out", type=pathlib.Path, required=True)
    ap.add_argument("--allow-lost", action="store_true")
    ap.add_argument("--plot", type=pathlib.Path)
    a = ap.parse_args()
    cfg = doc_cau_hinh()
    print(f"vị trí đeo: {cfg['ten']}, TRUC {cfg['truc']}, ACC_OFF {cfg['acc_off']} (fw_arm/src/app/vi_tri_deo.h)")
    files = sorted(f for s in a.src for f in ([s] if s.is_file() else s.glob("*.csv")))
    W = {k: [] for k in ("X", "y", "subject", "act", "trial", "p")}
    if a.plot:
        a.plot.mkdir(parents=True, exist_ok=True)
    n_ev, skipped = 0, 0
    for f in files:
        try:
            subj, sid, act, trial = parse_name(f)
        except ValueError as e:
            print(f"BỎ   {f.name}: {e}")
            skipped += 1
            continue
        label, code, mode = ACTS[act]
        t, raw, btn, lost = read_log(f)
        if lost and not a.allow_lost:
            print(f"BỎ   {f.name}: mất {lost} mẫu (thu lại, hoặc --allow-lost)")
            skipped += 1
            continue
        ev = find_events(raw, btn, mode)
        q = tien_xu_ly(raw, cfg)
        ws = windows(q, [e // 2 for e in ev], label)
        for s, y, p in ws:
            W["X"].append(q[s:s + WIN]); W["y"].append(y); W["subject"].append(sid)
            W["act"].append(code); W["trial"].append(trial); W["p"].append(p)
        n_ev += len(ev)
        npos = sum(1 for _, y, _ in ws if y != NORMAL)
        print(f"     {f.name}: {len(raw) / 100:.1f} s, {len(ev)} sự kiện, {len(ws)} cửa sổ ({npos} dương)")
        if mode == "button" and not ev:
            print(f"     ⚠ {f.name}: không thấy đoạn bấm nút nào")
        if a.plot:
            plot_events(f, doi_truc(raw, cfg), ev, a.plot)
    if not W["X"]:
        raise SystemExit("không có cửa sổ nào")
    a.out.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(a.out, X=np.stack(W["X"]).astype(np.int8), y=np.array(W["y"], np.uint8),
                        subject=np.array(W["subject"], np.int16), act=np.array(W["act"], np.int16),
                        trial=np.array(W["trial"], np.int32), p=np.array(W["p"], np.int16),
                        vi_tri=np.array(cfg["ten"]))
    y = np.array(W["y"])
    print(f"ghi {a.out}: {len(y)} cửa sổ (NORMAL {int(np.sum(y == 0))}, FALL {int(np.sum(y == 3))}), "
          f"{n_ev} sự kiện, {len(set(W['subject']))} người, bỏ {skipped} file")


if __name__ == "__main__":
    main()
