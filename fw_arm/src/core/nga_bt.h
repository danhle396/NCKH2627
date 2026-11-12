/*
 * nga_bt.h — tầng 2 cho thiết bị đeo BẮP TAY (kế hoạch 7.2): xác nhận một lần nghi ngã của CNN bằng 4 dấu hiệu vật lý.
 * C thuần, số nguyên; bản Python khớp từng bit: ml/src/nga_bt.py (kiểm ở fw_arm/test/m3_test.py).
 *
 * Mỗi mẫu 100 Hz (trục người, sau app_on_raw): |a|², |ω|² (uint32). Mỗi KHỐI 32 mẫu (0,32 s) lưu: |a|² nhỏ nhất,
 * |a|² lớn nhất, |ω|² lớn nhất, "yên" (max − min mỗi trục gia tốc < yen_a và mỗi trục con quay < yen_g). Vòng 36 khối.
 * Khi Safety FSM vào SUSPECT: nga_bt_bat_dau(); khối va chạm = khối có |a|² lớn nhất trong tim_khoi khối gần nhất.
 *   1. pha rơi    có khối trong [va chạm − roi_khoi, va chạm] với |a|²_min ≤ roi_lsb²      (rơi tự do: |a| giảm)
 *   2. va chạm    |a|²_max của khối va chạm ≥ va_lsb²
 *   3. quay mạnh  |ω|²_max trong [va chạm − 1, va chạm + 1] ≥ quay_lsb²
 *   4. bất động   có khối "yên" trong (va chạm, va chạm + yen_khoi]
 * Đủ 4 → XÁC NHẬN. Dấu hiệu 1, 2 biết ngay; 3, 4 chờ thêm khối. Không đủ khi hết yen_khoi khối sau va chạm hoặc quá
 * cho_ms → BỎ (app gọi safety_bo_nghi). NGƯỠNG CHƯA CHỌN: chọn trên dữ liệu bắp tay bằng tools/chon_nguong_tang2.py,
 * ghi vào app/vi_tri_deo.h (NGA_*), bật NGA_BAT = 1 (bước 7).
 */
#ifndef NGA_BT_H
#define NGA_BT_H
#include <stdint.h>

#define NGA_KHOI_MAU   32u
#define NGA_VONG       36u

typedef struct {
    uint16_t roi_lsb, va_lsb, quay_lsb, yen_a, yen_g;   /* LSB: gia tốc 2048/g, con quay 16,4/(°/s) */
    uint8_t  tim_khoi, roi_khoi, yen_khoi;
    uint32_t cho_ms;
} nga_bt_cfg_t;

typedef struct { uint32_t a2_min, a2_max, w2_max; uint8_t yen; } nga_khoi_t;

enum { NGA_BT_CHO = 0, NGA_BT_XAC_NHAN = 1, NGA_BT_BO = 2 };

typedef struct {
    nga_bt_cfg_t cfg;
    nga_khoi_t   k[NGA_VONG];
    uint8_t      i, n;                  /* khối sẽ ghi tiếp, số khối đã có */
    /* khối đang mở */
    uint8_t      m;
    uint32_t     a2_min, a2_max, w2_max;
    int16_t      mn[6], mx[6];
    /* lần xét */
    uint8_t      dang_xet, d_moi;       /* số khối đã đóng kể từ khối va chạm */
    uint8_t      dau_hieu;              /* bit 0..3 = dấu hiệu 1..4 */
    uint32_t     t0;
    uint32_t     n_xet, n_xac_nhan, n_bo;
} nga_bt_t;

void    nga_bt_init(nga_bt_t *g, const nga_bt_cfg_t *cfg);
void    nga_bt_mau(nga_bt_t *g, const int16_t body[6]);
void    nga_bt_bat_dau(nga_bt_t *g, uint32_t now);
int     nga_bt_buoc(nga_bt_t *g, uint32_t now);           /* NGA_BT_CHO / XAC_NHAN / BO */

#endif
