#!/usr/bin/env python
# tools/export_inc.py — xuất mô hình 2 lớp (O3) cho bộ tăng tốc tính tăng dần rtl/src/cnn_inc.v (O1, O2) và cổng APB (O4).
#
#   python -I tools/export_inc.py [--params ml/out/mo_hinh_thu/params_int8.npz] --out <thư mục> [--steps 58]
#
# Ra:
#   rom_inc.mem          1 byte/dòng: trọng số 1 làn rồi bias INT32 little-endian ở BIAS_BASE (O2)
#   rom_inc_0/1/2.mem    cùng nội dung, tách 3 bank 2.048 byte (RTL đọc từng bank → 3 pROM)
#   cnn_inc_params.vh    shift, BASE, BIAS_BASE, ROM_WORDS, NOUT, SCORE_SHIFT, ID
#   tb_stg.mem           mỗi bước 144 byte NEW_DATA (x[0..3], x[108..127] của cửa sổ)
#   tb_exp.mem           mỗi bước: LOGIT0, LOGIT1, CLASS, SCORE, VALID (5 từ 32 bit)
#   tb_trace.mem         mỗi bước 2.048 byte l1 (j·16 + c), l2 (m·16 + c), l3 (r·32 + c) của cửa sổ (0 khi chưa hợp lệ)
#   tb_info.vh           số bước luồng chính (TB_NS), số bước tự kiểm (8), chỉ số luồng
#   selftest_expect_inc.h  kết quả mong đợi của tự kiểm 8 bước (luồng LFSR) cho firmware M3
#   export_inc_report.txt
# Luồng chính: luồng tổng hợp (ml/src/luong_tong_hop.py, --seed), `--steps` bước. (Bằng chứng 11/11/2026 trước khi xóa
# D1 dùng lần thử ngã D1 #1441: docs/bao_cao/2026-11-11_rtl_inc_apb.)
# Tự kiểm (A.10 mới): luồng 240 mẫu × 6 kênh từ LFSR Galois 16 bit (0xB400, mầm 0xACE1), 8 bước sau SOFT_RST.
import argparse
import json
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "ml" / "src"))
import golden_inc as gi  # noqa: E402
from golden_model import forward  # noqa: E402
from luong_tong_hop import luong  # noqa: E402

ID = 0x4E434931                  # "NCI1": NCKH, bộ tăng tốc tăng dần, phiên bản 1


def load(p):
    z = np.load(p)
    P = {k: z[k] for k in z.files}
    P["n"] = [int(v) for v in P["n"]]
    P["head"] = str(P["head"])
    P["score_shift"] = int(P["score_shift"])
    return P


def lfsr_stream(n_samples=gi.WIN + 7 * gi.HOP):
    """Luồng tự kiểm: byte thứ i của LFSR Galois 16 bit (đa thức 0xB400, mầm 0xACE1), xếp mẫu·6 + kênh."""
    s, out = 0xACE1, []
    for _ in range(n_samples * 6):
        lsb = s & 1
        s >>= 1
        if lsb:
            s ^= 0xB400
        out.append(s & 0xFF)
    return np.frombuffer(bytes(out), np.int8).reshape(n_samples, 6)


def h8(v):
    return f"{int(v) & 0xFF:02X}"


def h32(v):
    return f"{int(v) & 0xFFFFFFFF:08X}"


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--params", type=pathlib.Path, default=ROOT / "ml/out/mo_hinh_thu/params_int8.npz")
    ap.add_argument("--out", type=pathlib.Path, required=True)
    ap.add_argument("--steps", type=int, default=58, help="số bước luồng chính (8 khởi động + 50 hợp lệ)")
    ap.add_argument("--seed", type=int, default=1001, help="seed luồng tổng hợp (1001: có 14 bước FALL)")
    a = ap.parse_args()
    P = load(a.params)
    gi.check_shape(P)
    nout = int(P["wd"].shape[0])
    out = a.out
    out.mkdir(parents=True, exist_ok=True)
    rep = []

    def log(s):
        print(s)
        rep.append(s)

    rom, bases, bias_base = gi.pack_rom_inc(P)
    qp = a.params.parent / "quant_params.json"                 # tên mô hình: in ra UART và web (không nhầm mô hình thử)
    ten = json.loads(qp.read_text(encoding="utf8")).get("ten", a.params.parent.name) if qp.exists() else a.params.parent.name
    log(f"mô hình: {ten} ({a.params.parent.name})")
    (out / "rom_inc.mem").write_text("\n".join(h8(v) for v in rom) + "\n", newline="\n")
    assert len(rom) <= 3 * 2048
    for b in range(3):                     # 3 bank cho RTL (3 pROM)
        (out / f"rom_inc_{b}.mem").write_text("\n".join(h8(v) for v in rom[b * 2048:(b + 1) * 2048]) + "\n",
                                              newline="\n")
    log(f"ROM: {len(rom)} byte = {bias_base} trọng số + {(len(rom) - bias_base) // 4} bias × 4 B; BASE {bases}, "
        f"BIAS_BASE {bias_base}; {-(-len(rom) // 2048)} khối 2.048 × 8")

    # luồng chính: luồng tổng hợp (ml/src/luong_tong_hop.py) có cả NORMAL và FALL ở vùng hợp lệ
    ns = a.steps
    wins = gi.windows_of(luong(ns, a.seed))
    log(f"luồng chính: luồng tổng hợp seed {a.seed}, {ns} bước")
    sw = gi.windows_of(lfsr_stream())
    assert len(sw) == gi.WARM_STEPS

    stg, exp, trc, bad = [], [], [], 0
    eng = gi.IncEngine(P)
    seq = [(w, False) for w in wins] + [(w, True) for w in sw]
    for i, (w, is_self) in enumerate(seq):
        if is_self and i == ns:
            eng.soft_rst()
        cls, sc, lg, valid = eng.step(gi.stage_of(w))
        stg += [h8(v) for v in gi.stage_of(w)]
        exp += [h32(v) for v in lg] + [h32(cls), h32(sc), h32(int(valid))]
        if valid:
            tr = {}
            c0, s0, l0 = forward(w, P, tr)
            bad += not (c0 == cls and s0 == sc and np.array_equal(l0, lg))
            trc += [h8(v) for v in np.concatenate([tr["l1"].reshape(-1), tr["l2"].reshape(-1), tr["l3"].reshape(-1)])]
        else:
            trc += ["00"] * gi.BUF_BYTES
    (out / "tb_stg.mem").write_text("\n".join(stg) + "\n", newline="\n")
    (out / "tb_exp.mem").write_text("\n".join(exp) + "\n", newline="\n")
    (out / "tb_trace.mem").write_text("\n".join(trc) + "\n", newline="\n")
    nvalid = sum(1 for i in range(len(seq)) if exp[i * (nout + 3) + nout + 2] == h32(1))
    log(f"{'PASS' if bad == 0 else 'FAIL'}  golden tăng dần == golden E2 ở {nvalid - bad}/{nvalid} bước hợp lệ "
        f"(luồng chính + tự kiểm)")
    cls_main = [int(exp[i * (nout + 3) + nout], 16) for i in range(ns)]
    log(f"lớp từng bước luồng chính: {''.join(str(c) for c in cls_main)}")
    log(f"MAC mỗi bước {eng.macs // len(seq)}, đầu ra Conv mỗi bước {eng.outputs // len(seq)}")

    vh = ["// GENERATED by tools/export_inc.py - DO NOT EDIT.", "`ifndef CNN_INC_PARAMS_VH", "`define CNN_INC_PARAMS_VH",
          f"`define CNN_INC_ID 32'h{ID:08X}", f"`define CNN_INC_NOUT {nout}",
          f"`define CNN_INC_SCORE_SHIFT {P['score_shift']}", f"`define CNN_INC_ROM_WORDS {len(rom)}"]
    for i, (b, n) in enumerate(zip(bases[:3], P["n"]), 1):
        vh += [f"`define CNN_INC_L{i}_BASE {b}", f"`define CNN_INC_L{i}_SH {n}"]
    vh += [f"`define CNN_INC_DENSE_BASE {bases[3]}", f"`define CNN_INC_BIAS_BASE {bias_base}", "`endif"]
    (out / "cnn_inc_params.vh").write_text("\n".join(vh) + "\n", newline="\n")
    (out / "tb_info.vh").write_text(f"`define TB_NS {ns}\n`define TB_NSELF {gi.WARM_STEPS}\n", newline="\n")

    # tự kiểm cho firmware
    e2 = gi.IncEngine(P)
    for w in sw:
        c, s, lg, v = e2.step(gi.stage_of(w))
    assert v and np.array_equal(lg, forward(sw[-1], P)[2])
    h = ["/* GENERATED by tools/export_inc.py - DO NOT EDIT.",
         " * Tự kiểm bộ tăng tốc tăng dần (kế hoạch O4): SOFT_RST, 8 bước từ luồng LFSR Galois 16 bit",
         " * (đa thức 0xB400, mầm 0xACE1, 240 mẫu × 6 kênh, byte i = mẫu·6 + kênh), rồi so CLASS, SCORE, LOGIT. */",
         "#ifndef SELFTEST_EXPECT_INC_H", "#define SELFTEST_EXPECT_INC_H", "#include <stdint.h>",
         f"#define CNN_INC_ID          0x{ID:08X}u", f"#define CNN_INC_NOUT        {nout}u",
         f"#define SELFTEST_INC_CLASS  {c}u", f"#define SELFTEST_INC_SCORE  {s}u",
         f"static const int32_t selftest_inc_logits[{nout}] = {{ {', '.join(str(int(v)) for v in lg)} }};", "#endif"]
    (out / "selftest_expect_inc.h").write_text("\n".join(h) + "\n", encoding="utf8", newline="\n")
    # ROM cho firmware: CNN phần mềm đối chứng đọc cùng bố cục với bộ tăng tốc (mục 10 bước 8, O7)
    rh = ["/* GENERATED by tools/export_inc.py - DO NOT EDIT. Bố cục ROM của bộ tăng tốc (golden_inc.pack_rom_inc). */",
          "#ifndef CNN_ROM_INC_H", "#define CNN_ROM_INC_H", "#include <stdint.h>",
          f'#define CNN_MO_HINH       "{ten}"', f"#define CNN_ROM_BYTES     {len(rom)}u",
          f"#define CNN_L1_BASE       {bases[0]}u",
          f"#define CNN_L2_BASE       {bases[1]}u", f"#define CNN_L3_BASE       {bases[2]}u",
          f"#define CNN_DENSE_BASE    {bases[3]}u", f"#define CNN_BIAS_BASE     {bias_base}u",
          f"#define CNN_SH1           {P['n'][0]}u", f"#define CNN_SH2           {P['n'][1]}u",
          f"#define CNN_SH3           {P['n'][2]}u", f"#define CNN_SCORE_SHIFT   {P['score_shift']}",
          "extern const int8_t cnn_rom_inc[CNN_ROM_BYTES];", "#endif"]
    (out / "cnn_rom_inc.h").write_text("\n".join(rh) + "\n", encoding="utf8", newline="\n")
    rc = ["/* GENERATED by tools/export_inc.py - DO NOT EDIT. */", '#include "cnn_rom_inc.h"',
          "const int8_t cnn_rom_inc[CNN_ROM_BYTES] = {"]
    for i in range(0, len(rom), 16):
        rc.append("    " + ", ".join(str(v) for v in rom[i:i + 16]) + ",")
    rc.append("};")
    (out / "cnn_rom_inc.c").write_text("\n".join(rc) + "\n", encoding="utf8", newline="\n")
    log(f"tự kiểm: lớp {c}, SCORE {s}, logit {lg.tolist()}")
    (out / "export_inc_report.txt").write_text("\n".join(rep) + "\n", encoding="utf8")
    src = a.params.resolve()
    try:
        src = src.relative_to(ROOT).as_posix()
    except ValueError:
        src = str(src)
    (out / "params_src.txt").write_text(src, encoding="utf8")
    sys.exit(0 if bad == 0 else 1)


if __name__ == "__main__":
    main()
