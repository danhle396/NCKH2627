#!/usr/bin/env bash
# rtl/scripts/test.sh — kiểm toàn bộ phần không cần board của NCKH2627 (kế hoạch mục 8, cấp 1–2 + firmware giả lập).
# Không ghi vào repo (mọi thứ ra thư mục tạm), không cần Gowin.
#   bash rtl/scripts/test.sh [--work <thư mục>] [--nhanh]
#   --nhanh: bỏ hai kiểm chạy lâu (O3 20.000 cửa sổ, golden tăng dần 40 luồng)
# Dữ liệu vào là luồng tổng hợp (ml/src/luong_tong_hop.py): repo không giữ dữ liệu công khai từ 11/11/2026.
#  1. E2 (chuẩn đối chiếu, mô hình thử 4 lớp ml/out/mo_hinh_thu_4lop): rtl/src khớp bản xuất; tb_e2 P = 1, P = 4 (10 vector, mọi tầng)
#  2. O3: mô hình 2 lớp (tools/o3_dense_2_lop.py) trùng lớp 4 lớp trên 50 vector + 20.000 cửa sổ tổng hợp; khớp ml/out/mo_hinh_thu
#  3. golden tăng dần == golden E2 hai lớp (tools/kiem_golden_inc.py, 40 luồng tổng hợp × 60 bước)
#  4. tools/export_inc.py: rtl/src/{rom_inc*.mem, cnn_inc_params.vh} và fw_arm/src/gen/* trùng bản vừa xuất
#  5. tb_inc: cnn_apb + cnn_inc qua APB, 58 bước luồng tổng hợp + 8 bước tự kiểm, mọi tầng, phép âm
#  6. firmware: biên dịch (armclang), fw_arm/test/m3_test.py trong Unicorn (bỏ qua nếu không có armclang hoặc unicorn;
#     đặt PY_UNICORN=<python có unicorn>)
#  7. tools/gen_web_gz.py tái tạo đúng fw_arm/src/gen/trang_web_gz.*
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=""; NHANH=0
while [ $# -gt 0 ]; do
    case "$1" in
        --work) WORK="$2"; shift 2 ;;
        --nhanh) NHANH=1; shift ;;
        *) echo "tham số lạ: $1"; exit 2 ;;
    esac
done
[ -n "$WORK" ] || WORK="$(mktemp -d)"
command -v iverilog >/dev/null || export PATH="/c/iverilog/bin:$PATH"
command -v iverilog >/dev/null || { echo "không thấy iverilog"; exit 2; }
PY=""
for c in python3 python /c/Users/*/AppData/Local/Programs/Python/Python3*/python.exe; do
    p="$(command -v "$c" 2>/dev/null)" || continue
    "$p" -c "import numpy" 2>/dev/null && { PY="$p"; break; }
done
[ -n "$PY" ] || { echo "không thấy Python có numpy"; exit 2; }
M="$ROOT/ml/out/mo_hinh_thu_4lop/e2_p1"
PARAMS="$ROOT/$(cat "$M/params_src.txt")"
SIM="$ROOT/rtl/sim"
S="$ROOT/rtl/src"
rm -rf "$WORK"; mkdir -p "$WORK"
ROOTW="$(cd "$ROOT" && (pwd -W 2>/dev/null || pwd))"; WORKW="$(cd "$WORK" && (pwd -W 2>/dev/null || pwd))"
echo "thư mục làm việc: $WORK"
echo "Python: $PY"
declare -a NAMES RESULTS TIMES
add() { NAMES+=("$1"); RESULTS+=("$2"); TIMES+=("$3 s"); echo "  $1: $2 ($3 s)"; }
passed() { [ "$(grep -E '^(PASS|FAIL)' "$1" | tail -1)" = "PASS" ]; }
run() {   # $1 = tên, $2 = log, rồi lệnh
    local name="$1" log="$2" t0=$SECONDS; shift 2
    if "$@" > "$log" 2>&1 && passed "$log"; then add "$name" PASS $((SECONDS - t0)); else add "$name" FAIL $((SECONDS - t0)); fi
}

# 1. E2
if cmp -s "$M/cnn_params.vh" "$S/cnn_params.vh" && cmp -s "$M/weights.mem" "$S/weights_e2.mem" && cmp -s "$M/bias.mem" "$S/bias_e2.mem"
then add "E2: rtl/src = mô hình thử 4 lớp" PASS 0; else add "E2: rtl/src = mô hình thử 4 lớp" FAIL 0; fi
for P in 1 4; do
    D="$WORK/e2_p$P"
    run "tb_e2 (P = $P, 10 vector, mọi tầng)" "$WORK/tb_e2_p$P.log" bash -c "
        '$PY' -I '$ROOT/tools/export_mem.py' --params '$PARAMS' --engine e2 --lanes $P --out '$D' > /dev/null &&
        cd '$D' && iverilog -g2001 -I . -DTB_NV=10 -s tb_e2 -o tb_e2.vvp '$SIM/tb_e2.v' '$S/cnn_e2.v' && vvp tb_e2.vvp"
done

# 2–3. O3 và golden tăng dần
if [ $NHANH -eq 0 ]; then
    run "O3: 2 lớp == 4 lớp (50 vector + 20.000 cửa sổ tổng hợp)" "$WORK/o3.log" bash -c "
        '$PY' -I '$ROOT/tools/o3_dense_2_lop.py' --out '$WORK/o3' &&
        '$PY' -I -c \"import numpy as np, sys
a = np.load(r'$WORKW/o3/params_int8.npz'); b = np.load(r'$ROOTW/ml/out/mo_hinh_thu/params_int8.npz')
ok = sorted(a.files) == sorted(b.files) and all(np.array_equal(a[k], b[k]) for k in a.files)
print('ml/out/mo_hinh_thu trung ban vua tao:', ok); print('PASS' if ok else 'FAIL')\""
    run "golden tăng dần == golden E2 hai lớp" "$WORK/golden_inc.log" "$PY" -I "$ROOT/tools/kiem_golden_inc.py"
else
    add "O3, golden tăng dần (--nhanh: bỏ qua)" SKIP 0
fi

# 4. export_inc
t0=$SECONDS
X="$WORK/inc"
if "$PY" -I "$ROOT/tools/export_inc.py" --out "$X" > "$WORK/export_inc.log" 2>&1; then
    bad=""
    for f in rom_inc.mem rom_inc_0.mem rom_inc_1.mem rom_inc_2.mem cnn_inc_params.vh; do cmp -s "$X/$f" "$S/$f" || bad="$bad $f"; done
    for f in cnn_rom_inc.h cnn_rom_inc.c selftest_expect_inc.h; do cmp -s "$X/$f" "$ROOT/fw_arm/src/gen/$f" || bad="$bad $f"; done
    [ -z "$bad" ] && add "export_inc: rtl/src, fw_arm/src/gen trùng bản xuất" PASS $((SECONDS - t0)) \
                  || add "export_inc: khác ở$bad" FAIL $((SECONDS - t0))
else add "export_inc" FAIL $((SECONDS - t0)); fi

# 4b. bản sao cho project Gowin trùng bản gốc
run "gowin/src trùng rtl/src (bản sao cho Gowin)" "$WORK/dong_bo.log" bash "$ROOT/rtl/scripts/dong_bo_gowin.sh" --kiem

# 5. tb_inc
run "tb_inc (APB, 58 + 8 bước, mọi tầng, phép âm)" "$WORK/tb_inc.log" bash -c "
    cd '$X' && iverilog -g2001 -I . -s tb_inc -o tb_inc.vvp '$SIM/tb_inc.v' '$S/cnn_apb.v' '$S/cnn_inc.v' && vvp tb_inc.vvp"
grep -E "^CYCLES" "$WORK/tb_inc.log" | sed 's/^/    /'

# 6. firmware
K="${ARMCLANG_DIR:-/c/Users/ADMIN/AppData/Local/Keil_v5/ARM/ARMCLANG/bin}"
PU="${PY_UNICORN:-$PY}"
if [ -x "$K/armclang.exe" ]; then
    t0=$SECONDS
    if bash "$ROOT/fw_arm/build.sh" > "$WORK/fw_build.log" 2>&1 && ! grep -q "warning" "$WORK/fw_build.log"; then
        add "firmware M3 biên dịch, không cảnh báo ($(grep 'Total ROM Size' "$WORK/fw_build.log" | grep -oE '[0-9.]+kB') flash)" PASS $((SECONDS - t0))
    else add "firmware M3 biên dịch" FAIL $((SECONDS - t0)); fi
    if "$PU" -c "import unicorn" 2>/dev/null; then
        run "firmware trên giả lập M3 (m3_test.py)" "$WORK/m3_test.log" bash -c "
            bash '$ROOT/fw_arm/build.sh' test > /dev/null && '$PU' '$ROOT/fw_arm/test/m3_test.py' --n 50"
    else add "firmware trên giả lập M3 (không có unicorn: đặt PY_UNICORN)" SKIP 0; fi
else add "firmware M3 (không có armclang)" SKIP 0; fi

# 6b. công cụ dữ liệu bắp tay: tự kiểm thuật toán
run "chon_truc --tu-kiem (200 cách gắn module)" "$WORK/chon_truc.log" "$PY" -I "$ROOT/tools/chon_truc.py" --tu-kiem
run "chon_nguong_tang2 --tu-kiem (quyết định theo khối == nga_bt.py)" "$WORK/chon_nguong.log" \
    "$PY" -I "$ROOT/tools/chon_nguong_tang2.py" --tu-kiem

# 7. trang web
t0=$SECONDS
G="$ROOT/fw_arm/src/gen"
cp "$G/trang_web_gz.c" "$WORK/web_truoc.c"
if "$PY" -I "$ROOT/tools/gen_web_gz.py" > "$WORK/web.log" 2>&1 && cmp -s "$G/trang_web_gz.c" "$WORK/web_truoc.c"
then add "gen_web_gz tái tạo đúng trang nén" PASS $((SECONDS - t0)); else add "gen_web_gz" FAIL $((SECONDS - t0)); fi

echo
echo "| Kiểm | Kết quả | Thời gian |"
echo "|---|---|---|"
fail=0
for i in "${!NAMES[@]}"; do
    echo "| ${NAMES[$i]} | ${RESULTS[$i]} | ${TIMES[$i]} |"
    [ "${RESULTS[$i]}" = "FAIL" ] && fail=1
done
echo "Icarus: $(iverilog -V 2>&1 | head -1)"
[ $fail -eq 0 ] && echo "TẤT CẢ PASS (SKIP = bỏ qua có ghi lý do)" || echo "CÓ FAIL"
exit $fail
