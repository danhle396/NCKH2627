#!/usr/bin/env bash
# rtl/scripts/build_gowin.sh — dựng project Gowin gowin/nckh2627.gprj (Tang Nano 4K, GW1NSR-LV4CQN48PC6/I5) bằng gw_sh, đúng
# như mở project trong Gowin IDE rồi bấm Run All, nhưng trên BẢN SAO ở thư mục tạm (không ghi impl/ vào repo).
# Kế hoạch mục 10: S0 và bước 4.
#   bash rtl/scripts/build_gowin.sh [--sram 2|4|8|16] [--chi-empu] [--work <thư mục>]
#     --sram      SRAM của M3 (mặc định 8 KB như gowin/src/gowin_empu/config.v); đo được 2 KB = 2 BSRAM, 4 = 2, 8 = 4, 16 = 8
#     --chi-empu  chỉ Gowin_EMPU, không có cổng APB và bộ tăng tốc (S0: đo tài nguyên riêng của M3)
# Mở bằng IDE: Gowin EDA → File → Open → gowin/nckh2627.gprj → Run All. RTL và ROM trong gowin/src là BẢN SAO của rtl/src
# (rtl/scripts/dong_bo_gowin.sh); script kiểm hai bản trùng nhau trước khi dựng, lệch thì dừng.
# Gowin chỉ CẢNH BÁO (EX3988) khi không mở được file ROM rồi bỏ luôn ROM: script coi đó là lỗi.
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
GWD="${GOWIN_DIR:-/c/Gowin/Gowin_V1.9.11.03_Education_x64/IDE}"
GW="$GWD/bin/gw_sh.exe"
SRAM=8; CNN=1; W=""
while [ $# -gt 0 ]; do
    case "$1" in
        --sram) SRAM="$2"; shift 2 ;;
        --chi-empu) CNN=0; shift ;;
        --work) W="$2"; shift 2 ;;
        *) echo "tham số lạ: $1"; exit 2 ;;
    esac
done
case "$SRAM" in 2) NB=1 ;; 4) NB=2 ;; 8) NB=4 ;; 16) NB=8 ;; *) echo "--sram phải là 2, 4, 8, 16"; exit 2 ;; esac
[ -n "$W" ] || W="${TMPDIR:-${TEMP:-/tmp}}/nckh_gowin_s${SRAM}_c${CNN}"
bash "$ROOT/rtl/scripts/dong_bo_gowin.sh" --kiem > /dev/null || { bash "$ROOT/rtl/scripts/dong_bo_gowin.sh" --kiem; exit 1; }
rm -rf "$W"; mkdir -p "$W"
cp -r "$ROOT/gowin" "$W/gowin"
P="$W/gowin"
rm -rf "$P/impl/gwsynthesis" "$P/impl/pnr" "$P/impl/temp"
sed -i "s/^\`define NUM_BRAMS_[0-9]*/\`define NUM_BRAMS_$NB/" "$P/src/gowin_empu/config.v"
sed -i "s/parameter CNN   = 1/parameter CNN   = $CNN/" "$P/src/top.v"
grep -q "NUM_BRAMS_$NB" "$P/src/gowin_empu/config.v" || { echo "không sửa được config.v"; exit 1; }
printf 'open_project nckh2627.gprj\nrun all\n' > "$P/build.tcl"
echo "thư mục: $P  (SRAM $SRAM KB = NUM_BRAMS_$NB, CNN = $CNN)"
(cd "$P" && "$GW" build.tcl > build.log 2>&1) || { grep -E "ERROR|error" "$P/build.log" | head -20; tail -15 "$P/build.log"; exit 1; }
R="$P/impl/pnr/nckh2627.rpt.txt"
[ -f "$R" ] || { tail -20 "$P/build.log"; exit 1; }
grep -E "ERROR|EX3988" "$P/build.log" && { echo "LỖI: xem $P/build.log"; exit 1; } || true
echo "--- tài nguyên (P&R) ---"
tr -d '\r' < "$R" | grep -E "^\s+(Logic|Register|BSRAM|DSP|I/O Port|CLS)\s+\|" || true
echo "--- thời gian ---"
T="$P/impl/pnr/nckh2627_tr_content.html"
[ -f "$T" ] && sed 's/<[^>]*>/ /g' "$T" | tr -d '\r' | tr '\n' ' ' | tr -s ' ' | grep -oE 'Max Frequency Summary.{0,110}' || true
echo "--- cảnh báo: $(grep -cE "WARN" "$P/build.log") (ngoài IP mã hóa gowin_empu.v:) ---"
grep -E "WARN" "$P/build.log" | grep -v "gowin_empu.v" | sed -E 's/\("[^"]*"\)//' | sort | uniq -c | sort -rn | head -8 || true
ls -l "$P/impl/pnr/nckh2627.fs"
