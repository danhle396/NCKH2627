#!/usr/bin/env python
# tools/chon_truc.py — chọn TRUC_X/Y/Z và ACC_OFF_* cho fw_arm/src/app/vi_tri_deo.h từ 3 log thô đứng yên (bước 7).
#
#   python -I tools/chon_truc.py --dung <csv> --nam-ngua <csv> --nghieng-phai <csv>
#   python -I tools/chon_truc.py --tu-kiem                 # kiểm thuật toán bằng phép toán (không phải dữ liệu)
#
# Thu bằng tools/thu_bap_tay.py, module GY-521 đã gắn cố định trên băng bắp tay, mỗi tư thế ~10 s, nằm yên:
#   --dung          đứng thẳng, tay buông xuôi              → trục người Z (lên) hướng ngược trọng lực
#   --nam-ngua      nằm ngửa, tay để dọc thân               → trục người X (trước) hướng lên
#   --nghieng-phai  nằm nghiêng phải (vai phải chạm đất)    → trục người Y (trái) hướng lên
# Trục người: X trước, Y trái, Z lên (SPEC A.2). Bỏ 1 s đầu và cuối mỗi file. Offset = trung bình phần lệch so với
# (0, 0, ±2048) trên 3 tư thế, theo trục người. In các dòng #define để dán vào vi_tri_deo.h.
import argparse
import pathlib
import sys

import numpy as np

G = 2048


def doc(path):
    a = np.loadtxt(path, delimiter=",", skiprows=1, dtype=np.int64, ndmin=2)
    acc = a[100:-100, 1:4] if len(a) > 300 else a[:, 1:4]
    return acc.mean(axis=0), acc.std(axis=0)


def chon(m_dung, m_ngua, m_phai):
    """trung bình gia tốc (trục cảm biến) của 3 tư thế → (truc [X, Y, Z] mã ±1..3, acc_off [3])."""
    truc = []
    for m in (m_ngua, m_phai, m_dung):            # tư thế làm trục X, Y, Z hướng lên
        k = int(np.argmax(np.abs(m)))
        truc.append((k + 1) * (1 if m[k] > 0 else -1))
    if len({abs(c) for c in truc}) != 3:
        raise ValueError(f"ba tư thế cho trùng trục cảm biến {truc}: kiểm lại cách nằm / gắn module")

    def body(m):
        return np.array([np.sign(c) * m[abs(c) - 1] for c in truc])
    mong = {0: np.array([G, 0, 0]), 1: np.array([0, G, 0]), 2: np.array([0, 0, G])}
    lech = [body(m) - mong[i] for i, m in enumerate((m_ngua, m_phai, m_dung))]
    return truc, np.round(np.mean(lech, axis=0)).astype(int)


def tu_kiem():
    rng = np.random.default_rng(1)
    for _ in range(200):
        perm = rng.permutation(3)
        sign = rng.choice([-1, 1], 3)
        off = rng.integers(-150, 150, 3)
        truc = [int(sign[i] * (perm[i] + 1)) for i in range(3)]

        def doc_cb(up_body):                          # cảm biến đọc gì khi trục người up_body hướng lên
            b = np.zeros(3)
            b[up_body] = G
            b = b + off
            m = np.zeros(3)
            for i in range(3):
                m[perm[i]] = sign[i] * b[i]
            return m + rng.normal(0, 3 / np.sqrt(800), 3)       # trung bình ~800 mẫu, nhiễu 3 LSB mỗi mẫu
        t2, o2 = chon(doc_cb(2), doc_cb(0), doc_cb(1))
        assert t2 == truc and np.all(np.abs(o2 - off) <= 2), (truc, t2, off, o2)
    print("PASS  tự kiểm: 200 cách gắn ngẫu nhiên (hoán vị, dấu, offset ±150 LSB, nhiễu) đều tìm đúng trục và offset ±2 LSB")
    print("PASS")


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--dung", type=pathlib.Path)
    ap.add_argument("--nam-ngua", type=pathlib.Path)
    ap.add_argument("--nghieng-phai", type=pathlib.Path)
    ap.add_argument("--tu-kiem", action="store_true")
    a = ap.parse_args()
    if a.tu_kiem:
        return tu_kiem()
    ms = []
    for ten, f in (("đứng", a.dung), ("nằm ngửa", a.nam_ngua), ("nghiêng phải", a.nghieng_phai)):
        if f is None:
            raise SystemExit("cần đủ --dung, --nam-ngua, --nghieng-phai")
        m, sd = doc(f)
        print(f"{ten:13s}: trung bình {np.round(m).astype(int)} LSB, độ lệch chuẩn {np.round(sd).astype(int)}"
              + ("  ⚠ không yên (độ lệch chuẩn > 100 LSB): thu lại" if np.any(sd > 100) else ""))
        ms.append(m)
    truc, off = chon(ms[0], ms[1], ms[2])
    print("\nDán vào khối VT_BAP_TAY của fw_arm/src/app/vi_tri_deo.h:")
    for n, c in zip("XYZ", truc):
        print(f"#define TRUC_{n}          {c if c > 0 else f'({c})'}")
    for n, o in zip("XYZ", off):
        print(f"#define ACC_OFF_{n}       {o if o >= 0 else f'({o})'}")


if __name__ == "__main__":
    main()
