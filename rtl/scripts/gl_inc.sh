#!/usr/bin/env bash
# rtl/scripts/gl_inc.sh — kiểm chứng cấp 3 (kế hoạch mục 8): chạy rtl/sim/tb_inc.v trên NETLIST SAU ĐẶT – NỐI DÂY của
# Gowin cho cnn_apb + cnn_inc (pROM có INIT_RAM từ ROM trọng số, SDPB, DSP thật), thư viện primitive simlib/gw1n.
#   bash rtl/scripts/gl_inc.sh [--steps N] [--work <thư mục>]
# Netlist cả chip (có Gowin_EMPU) và netlist sau tổng hợp đều bị Gowin bản Education mã hóa; netlist .vo sau đặt – nối
# dây thì không. cnn_apb có 78 cổng > 39 chân nên đặt – nối dây vỏ bọc rtl/sim/gl_wrap.v (35 chân, đọc 32 bit qua 4 byte),
# cho đúng part GW1NSR-LV4CQN48PC6/I5. Không có SDF (chỉ kiểm chức năng của netlist).
# Không so l1/l2/l3 (netlist không còn mảng fbuf), chỉ so qua APB: CLASS, SCORE, LOGIT, VALID, WARM, phép âm, WROM.
# Icarus chạy netlist chậm: --steps 58 (mặc định, 8 khởi động + 50 hợp lệ) mất cỡ chục phút; --steps 9 để thử nhanh.
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
GWD="${GOWIN_DIR:-/c/Gowin/Gowin_V1.9.11.03_Education_x64/IDE}"
STEPS=58; W=""
while [ $# -gt 0 ]; do
    case "$1" in
        --steps) STEPS="$2"; shift 2 ;;
        --work) W="$2"; shift 2 ;;
        *) echo "tham số lạ: $1"; exit 2 ;;
    esac
done
[ -n "$W" ] || W="${TMPDIR:-${TEMP:-/tmp}}/nckh_gl_inc"
command -v iverilog >/dev/null || export PATH="/c/iverilog/bin:$PATH"
PY=""
for c in python3 python /c/Users/*/AppData/Local/Programs/Python/Python3*/python.exe; do
    p="$(command -v "$c" 2>/dev/null)" || continue
    "$p" -c "import numpy" 2>/dev/null && { PY="$p"; break; }
done
rm -rf "$W"; mkdir -p "$W/src"
"$PY" -I "$ROOT/tools/export_inc.py" --out "$W" --steps "$STEPS" > "$W/export.log"
for f in cnn_inc_params.vh rom_inc_0.mem rom_inc_1.mem rom_inc_2.mem; do
    cmp -s "$W/$f" "$ROOT/rtl/src/$f" || { echo "rtl/src/$f khác bản vừa xuất: chạy lại export_inc.py"; exit 1; }
done
cp "$ROOT/rtl/src/cnn_apb.v" "$ROOT/rtl/src/cnn_inc.v" "$ROOT/rtl/sim/gl_wrap.v" "$W/cnn_inc_params.vh" "$W/src/"
echo 'create_clock -name pclk -period 37.037 [get_ports {pclk}]' > "$W/src/gl.sdc"
{
    echo "set_device -name GW1NSR-4C GW1NSR-LV4CQN48PC6/I5"
    echo "add_file src/gl_wrap.v"
    echo "add_file src/cnn_apb.v"
    echo "add_file src/cnn_inc.v"
    echo "add_file src/gl.sdc"
    echo "set_option -top_module gl_wrap"
    echo "set_option -output_base_name gl"
    echo "set_option -use_mode_as_gpio 1"
    echo "set_option -use_done_as_gpio 1"
    echo "set_option -use_mspi_as_gpio 1"
    echo "set_option -gen_verilog_sim_netlist 1"
    echo "run all"
} > "$W/pnr.tcl"
(cd "$W" && "$GWD/bin/gw_sh.exe" pnr.tcl > pnr.log 2>&1) || { grep -E "ERROR" "$W/pnr.log" | head; tail -20 "$W/pnr.log"; exit 1; }
VG="$W/impl/pnr/gl.vo"
[ -f "$VG" ] || { grep -E "ERROR" "$W/pnr.log" | head; tail -20 "$W/pnr.log"; exit 1; }
grep -q "pragma protect" "$VG" && { echo "netlist .vo bị mã hóa"; exit 1; }
echo "netlist: $(grep -cE '^\s*pROM ' "$VG") pROM, $(grep -cE '^\s*SDPB ' "$VG") SDPB, $(grep -cE '^\s*MULT[A-Z0-9]* ' "$VG") MULT, \
$(grep -c 'INIT_RAM_' "$VG") dòng INIT_RAM"
# Icarus 0.9.7 không đọc được mảng 2 chiều trong các mô hình FLASH* của thư viện (netlist không dùng): bỏ các module đó
awk '/^module FLASH/{skip=1} !skip{print} skip && /^endmodule/{skip=0}' "$GWD/simlib/gw1n/prim_sim.v" > "$W/prim_sim_noflash.v"
cd "$W"
iverilog -g2001 -DGL -I . -s tb_inc -o gl.vvp "$ROOT/rtl/sim/tb_inc.v" "$VG" prim_sim_noflash.v 2>&1 \
    | grep -vE "warning: Part select|timescale" | head -20 || true
t0=$SECONDS
vvp gl.vvp > gl.log 2>&1 || true
grep -vE "^VCD" gl.log | tail -6
echo "thời gian mô phỏng: $((SECONDS - t0)) s"
[ "$(grep -E '^(PASS|FAIL)' gl.log | tail -1)" = "PASS" ]
