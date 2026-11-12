#!/usr/bin/env bash
# rtl/scripts/dong_bo_gowin.sh — chép MỘT CHIỀU bản gốc rtl/src → bản sao gowin/src cho project Gowin (gowin/nckh2627.gprj).
#   bash rtl/scripts/dong_bo_gowin.sh          # chép (sửa code thì sửa ở rtl/src rồi chạy lệnh này)
#   bash rtl/scripts/dong_bo_gowin.sh --kiem   # chỉ kiểm hai bản trùng nhau; lệch thì in tên file và thoát mã 1
# Bản gốc: rtl/src (mô phỏng, tools/export_inc.py ghi vào đây). File .v/.vh trong gowin/src có thêm MỘT dòng đầu ghi chú
# "bản sao"; phần còn lại phải trùng từng byte. File .mem chép nguyên (trùng từng byte).
# rtl/scripts/test.sh và rtl/scripts/build_gowin.sh gọi --kiem: lệch là FAIL / dừng dựng bitstream.
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/rtl/src"; DST="$ROOT/gowin/src"
FILES="cnn_apb.v cnn_inc.v cnn_inc_params.vh rom_inc_0.mem rom_inc_1.mem rom_inc_2.mem"
HEAD='// BẢN SAO của rtl/src (rtl/scripts/dong_bo_gowin.sh) — KHÔNG SỬA Ở ĐÂY: sửa rtl/src rồi chạy lại script.'
lech=0
for f in $FILES; do
    if [ "${1:-}" = "--kiem" ]; then
        case "$f" in
            *.mem) cmp -s "$SRC/$f" "$DST/$f" || { echo "LỆCH: gowin/src/$f khác rtl/src/$f"; lech=1; } ;;
            *)     { [ "$(head -n 1 "$DST/$f" 2>/dev/null)" = "$HEAD" ] && tail -n +2 "$DST/$f" | cmp -s - "$SRC/$f"; } \
                       || { echo "LỆCH: gowin/src/$f khác rtl/src/$f"; lech=1; } ;;
        esac
    else
        case "$f" in
            *.mem) cp "$SRC/$f" "$DST/$f" ;;
            *)     { printf '%s\n' "$HEAD"; cat "$SRC/$f"; } > "$DST/$f" ;;
        esac
        echo "chép rtl/src/$f → gowin/src/$f"
    fi
done
if [ "${1:-}" = "--kiem" ]; then
    [ $lech -eq 0 ] && echo "gowin/src trùng rtl/src (6 file)" && echo PASS || { echo "chạy: bash rtl/scripts/dong_bo_gowin.sh"; echo FAIL; }
fi
exit $lech
