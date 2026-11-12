#!/usr/bin/env bash
# fw_arm/build.sh — biên dịch firmware M3 (Gowin_EMPU GW1NS-4C, Cortex-M3) bằng Arm Compiler 6 của MDK-ARM, không mở Keil.
#   bash fw_arm/build.sh          → fw_arm/build/nckh_m3.axf, .bin (nạp flash 32 KB), .map; in kích thước flash/RAM
#   bash fw_arm/build.sh test     → fw_arm/build/test.axf cho fw_arm/test/m3_test.py (Unicorn)
#   DOI_CHUNG=1 bash fw_arm/build.sh → bản đối chứng: mỗi bước chạy thêm CNN phần mềm, log "W,…" (mục 10 bước 8)
# MicroLIB; flash 0x00000000 32 KB, SRAM 0x20000000 8 KB (4 BSRAM, rtl/scripts/build_gowin.sh --sram 8).
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
K="${ARMCLANG_DIR:-/c/Users/ADMIN/AppData/Local/Keil_v5/ARM/ARMCLANG/bin}"
CC="$K/armclang.exe"; LD="$K/armlink.exe"; FE="$K/fromelf.exe"
B="$ROOT/build"; mkdir -p "$B"
CFLAGS=(--target=arm-arm-none-eabi -mcpu=cortex-m3 -mthumb -Os -ffunction-sections -fdata-sections -std=c99
        -Wall -Wextra -Wno-unused-parameter -D__MICROLIB
        ${DOI_CHUNG:+-DCNN_SW_DOI_CHUNG=1}
        -I "$ROOT/src/core" -I "$ROOT/src/app" -I "$ROOT/src/driver" -I "$ROOT/src/gen")
CORE=(core/cnn_core.c core/safety.c core/preprocess.c core/blocks.c core/pwr_btn.c
      core/imu_guard.c core/wifi_at.c core/gyro_cal.c core/raw_log.c core/nga_bt.c gen/cnn_rom_inc.c)

compile() {   # $1 = đường dẫn nguồn (tương đối với src hoặc tuyệt đối), $2 = thư mục obj
    local s="$1" o
    [ -f "$s" ] || s="$ROOT/src/$1"
    o="$2/$(basename "${s%.c}").o"
    local opt=()
    case "$s" in *cnn_core.c) opt=(-O2) ;; esac          # CNN phần mềm đối chứng: tối ưu tốc độ (bước 8)
    "$CC" "${CFLAGS[@]}" "${opt[@]}" -c "$s" -o "$o"
    echo "$o"
}

if [ "${1:-}" = "test" ]; then
    O="$B/obj_test"; rm -rf "$O"; mkdir -p "$O"
    objs=()
    for s in "${CORE[@]}"; do objs+=("$(compile "$s" "$O")"); done
    objs+=("$(compile "$ROOT/src/app/app.c" "$O")")
    objs+=("$(compile "$ROOT/test/test_entry.c" "$O")")
    "$LD" --cpu=Cortex-M3 --library_type=microlib --ro-base=0x00000000 --rw-base=0x20000000 \
        --entry=t_sw --keep="t_*" --no_startup "${objs[@]}" -o "$B/test.axf" --map --list="$B/test.map" > "$B/test_link.log" 2>&1 \
        || { cat "$B/test_link.log"; exit 1; }
    echo "build/test.axf"
    exit 0
fi

O="$B/obj"; rm -rf "$O"; mkdir -p "$O"
objs=()
for s in "${CORE[@]}" driver/startup.c driver/board.c driver/uart.c driver/i2c_mem.c driver/imu.c driver/cnn_drv.c \
         app/app.c app/web.c app/main.c gen/trang_web_gz.c; do
    objs+=("$(compile "$s" "$O")")
done
"$LD" --cpu=Cortex-M3 --library_type=microlib --scatter="$ROOT/gw1ns4c_8k.sct" --entry=Reset_Handler \
    "${objs[@]}" -o "$B/nckh_m3.axf" --map --info=sizes,totals --list="$B/nckh_m3.map" > "$B/link.log" 2>&1 \
    || { cat "$B/link.log"; exit 1; }
"$FE" --bin --output="$B/nckh_m3.bin" "$B/nckh_m3.axf"
grep -E "Total RO  Size|Total RW  Size|Total ROM Size" "$B/nckh_m3.map" | tr -s ' '
ls -l "$B/nckh_m3.bin"
