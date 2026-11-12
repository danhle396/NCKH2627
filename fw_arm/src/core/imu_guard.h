/*
 * imu_guard.h — phát hiện và tự gỡ mất cảm biến MPU6050 (kế hoạch 7.5, bắt buộc). C thuần, không gọi phần cứng:
 * driver/imu.c thi hành hành động mà máy trạng thái trả về rồi báo lại kết quả. Thử được trong fw_arm/test/m3_test.py.
 *
 * Phát hiện (7.5.3), một trong ba: quá 50 ms không có ngắt INT; 3 lần đọc lỗi liên tiếp; 6 trục giống hệt nhau 1 s.
 * Tự gỡ (7.5.4):
 *   bậc 1  gỡ bus: 9 xung SCL + STOP, WHO_AM_I, ghi cấu hình, đọc lại kiểm                       → lỗi: bậc 2
 *   bậc 2  ghi PWR_MGMT_1 = 0x80 (DEVICE_RESET), chờ 100 ms, cấu hình lại                         → lỗi: bậc 3
 *   bậc 3  SCL, SDA ra mức 0 → sens_pwr = 0, giữ ≥ 1 s → sens_pwr = 1, chờ 100 ms → thả, gỡ bus, cấu hình → lỗi: bậc 4
 *   bậc 4  báo MẤT CẢM BIẾN (còi + web), cứ 10 s thử lại bậc 3
 * Khi bắt đầu gỡ: đặt cnn_reset = 1 để app xóa đệm bộ tăng tốc (SOFT_RST) và chạy lại 8 bước khởi động; không đưa mẫu
 * giả vào CNN. Máy trạng thái an toàn giữ nguyên (app không gọi safety_bo_nghi vì mất cảm biến).
 * Khởi động: luôn bắt đầu từ bậc 3 (7.5.4: sau watchdog reset cũng chạy cắt nguồn một lần).
 */
#ifndef IMU_GUARD_H
#define IMU_GUARD_H
#include <stdint.h>

#define IG_INT_TIMEOUT_MS   50u
#define IG_ERR_MAX          3u
#define IG_SAME_MAX         100u        /* 1 s ở 100 Hz */
#define IG_RESET_WAIT_MS    100u
#define IG_PWR_OFF_MS       1000u       /* [CẦN XÁC MINH: thử 0,5 / 1 / 2 / 5 s trên board] */
#define IG_PWR_ON_WAIT_MS   100u
#define IG_LOST_RETRY_MS    10000u

typedef enum {
    IG_OK = 0,          /* đang đọc bình thường */
    IG_L2_WAIT,         /* đã ghi DEVICE_RESET, chờ 100 ms */
    IG_L3_OFF,          /* nguồn cảm biến đang tắt */
    IG_L3_ON_WAIT,      /* vừa cấp lại nguồn, chờ 100 ms */
    IG_LOST             /* bậc 4 */
} ig_state_t;

typedef enum {          /* hành động driver phải làm ngay, rồi gọi imu_guard_done(ok) nếu có kết quả */
    IG_ACT_NONE = 0,
    IG_ACT_BUS_CLEAR_CFG,   /* bậc 1: gỡ bus + WHO_AM_I + cấu hình + đọc lại kiểm   → done(ok) */
    IG_ACT_SOFT_RESET,      /* bậc 2: ghi PWR_MGMT_1 = 0x80                         → không cần done */
    IG_ACT_CFG,             /* sau 100 ms: WHO_AM_I + cấu hình + đọc lại kiểm        → done(ok) */
    IG_ACT_POWER_OFF,       /* bậc 3: i2c_hold_low(), sens_pwr = 0                  → không cần done */
    IG_ACT_POWER_ON,        /* sens_pwr = 1 (SCL, SDA vẫn giữ 0)                    → không cần done */
    IG_ACT_RELEASE_CFG      /* thả SCL, SDA, gỡ bus, WHO_AM_I, cấu hình, đọc lại kiểm → done(ok) */
} ig_act_t;

typedef struct {
    ig_state_t st;
    ig_act_t   pending;         /* hành động đang chờ done() */
    uint32_t   t;               /* mốc thời gian của trạng thái */
    uint32_t   t_last_int;
    uint8_t    errs;
    uint16_t   same;
    int16_t    last[6];
    uint8_t    cnn_reset;       /* app đọc và xóa */
    uint8_t    lost;            /* 1 khi ở bậc 4 (báo MẤT CẢM BIẾN) */
    uint32_t   n_go_bus, n_reset_mem, n_cat_nguon, n_mat_cb;   /* đếm cho web và log (7.5.4) */
} imu_guard_t;

void     imu_guard_init(imu_guard_t *g, uint32_t now);            /* bắt đầu bằng bậc 3 */
/* mỗi lần có ngắt INT và đã đọc: ok = đọc I²C thành công, raw = 6 trục */
void     imu_guard_sample(imu_guard_t *g, uint32_t now, int ok, const int16_t raw[6]);
ig_act_t imu_guard_poll(imu_guard_t *g, uint32_t now);           /* gọi mỗi vòng lặp chính */
void     imu_guard_done(imu_guard_t *g, uint32_t now, int ok);
static inline int imu_guard_reading(const imu_guard_t *g) { return g->st == IG_OK; }

#endif
