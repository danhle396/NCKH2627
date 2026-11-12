/*
 * fw_config.h — hằng số dùng chung của firmware M3 (NCKH2627). Dựa trên fw_config_du_thi.h (SPEC v3 của bản dự thi);
 * bỏ phần SPI/UART giữa hai board, bỏ lớp IMPACT (kế hoạch O3), thêm cổng APB (O4). Phần phụ thuộc vị trí đeo
 * (đổi trục, offset, ngưỡng tầng 2, điều kiện nghiêng) nằm ở app/vi_tri_deo.h (O9).
 */
#ifndef FW_CONFIG_H
#define FW_CONFIG_H

/* ---- A.2 tiền xử lý ---- */
#define PRE_CH          6       /* AX AY AZ GX GY GZ */
#define ACC_SHIFT       7       /* 1 mức INT8 = 1/16 g ở ±16 g */
#define GYR_SHIFT       6       /* 1 mức INT8 = 3,90 °/s ở ±2000 °/s */

/* ---- A.3 cửa sổ ---- */
#define PRE_WIN         128     /* mẫu 50 Hz trong một cửa sổ (2,56 s) */
#define PRE_HOP         16      /* cửa sổ mới sau mỗi 16 mẫu (0,32 s) */
#define PRE_BYTES       (PRE_WIN * PRE_CH)      /* 768 */

/* ---- lớp (O3): bộ tăng tốc trả 0 NORMAL, 1 FALL; driver đổi 1 → CLS_FALL trước safety_on_result() ---- */
#define CLS_NORMAL      0
#define CLS_FALL        3
#define CNN_CLS_FALL    1       /* mã FALL của bộ tăng tốc 2 lớp */

/* ---- bộ tăng tốc qua APB (O4, rtl/src/cnn_apb.v) ---- */
#define CNN_STAGE_BYTES 144     /* x[0..3] + x[108..127], 24 mẫu × 6 kênh */
#define CNN_WARM_STEPS  8       /* kết quả hợp lệ khi WARM = 8 */
#define CNN_TIMEOUT_US  20000u  /* bước nào quá 20 ms chưa DONE là lỗi (thiết kế 2,6 ms ở 27 MHz) */
#define CNN_ERR_RESET   3       /* 3 lỗi liên tiếp → SOFT_RST + tự kiểm lại */

/* ---- A.8 khối và Safety FSM ---- */
#define BLK_SAMPLES     32      /* 32 mẫu thô 100 Hz = 0,32 s */
#define T_STILL_A       300     /* LSB ở ±16 g, ≈ 0,15 g — giá trị đầu của SPEC, CHƯA CHỌN cho bắp tay (bước 7) */
#define T_STILL_G       250     /* LSB ở ±2000 °/s, ≈ 15 °/s — CHƯA CHỌN cho bắp tay (bước 7) */
#define STILL_BLOCKS    8       /* 8 khối = 2,56 s */
#define VOTE_K          2
#define VOTE_N          3
#define T_SCORE         0       /* 0 = mọi cửa sổ FALL đều tính phiếu (O3 không đổi phiếu ở mọi ngưỡng, báo cáo 11/11) */
#define WEAR_OFF_MS     3000
#define WEAR_RECENT_MS  3000
#define SUSPECT_TIMEOUT_MS  8000
#define PRE_ALARM_MS    10000
#define COOLDOWN_MS     30000
#define SOS_HOLD_MS     3000
#define ALARM_HOLD_MS   1000
#ifndef BTN_POWER
#define BTN_POWER       1       /* một nút: bật/tắt + CANCEL + SOS */
#endif

#include "vi_tri_deo.h"         /* TRUC_*, ACC_OFF_*, TILT_EN, T_TILT, NGA_* theo vị trí đeo */

#endif
