/*
 * app.h — lõi ứng dụng M3 (thay app_core.c của bản dự thi, bỏ SPI và LED 7 đoạn). C thuần, không gọi phần cứng:
 *   mẫu thô 100 Hz → đổi trục theo vi_tri_deo.h (O9) → trừ offset → khối (A.8) + tiền xử lý (A.2)
 *   → mỗi 16 mẫu 50 Hz: 144 byte STAGE cho bộ tăng tốc (gửi ngay từ mẫu 16, ring khởi tạo 0: hợp lệ sau 8 bước = 2,56 s)
 *   → kết quả: bỏ nếu chưa VALID (WARM < 8); đổi mã 1 → 3 (O3); bỏ phiếu + Safety FSM → còi, LED.
 * Mất cảm biến (imu_guard): app_restart_window() xóa ring tiền xử lý; driver SOFT_RST bộ tăng tốc; Safety FSM giữ nguyên.
 */
#ifndef APP_H
#define APP_H
#include <stdint.h>
#include "preprocess.h"
#include "blocks.h"
#include "safety.h"
#include "pwr_btn.h"
#include "cnn_core.h"
#include "nga_bt.h"

#define APP_STEP    1u              /* app_on_raw: có STAGE mới cần gửi */
#define APP_BLOCK   2u              /* vừa đóng một khối */

typedef struct {
    pre_state_t pre;
    blocks_t    blk;
    safety_t    fsm;
    pwr_btn_t   pb;
    int16_t     gyro_off[3];
    uint32_t    n_raw, n_step, n_res, n_invalid, n_err, n_fall_win;
    uint8_t     last_cls, last_score, last_valid;
    int32_t     last_logit[2];
    uint32_t    last_cycles;
    uint8_t     consec_err, err_flag;       /* lỗi bộ tăng tốc: quá hạn hoặc bị từ chối */
    uint8_t     selftest_ok;
    uint8_t     sensor_lost;
    uint8_t     pwr_ev;
#if NGA_BAT
    nga_bt_t    t2;                                 /* tầng 2 bắp tay (core/nga_bt.h) */
#endif
    uint32_t    t_pwr, t_last_fall, n_nga;          /* số lần vào SUSPECT (web "Số lần phát hiện ngã") */
} app_t;

void     app_init(app_t *a, uint32_t now);
void     app_set_gyro_offset(app_t *a, const int16_t off[3]);
unsigned app_on_raw(app_t *a, const int16_t raw_sensor[6]);   /* trục cảm biến; trả về APP_STEP | APP_BLOCK */
/* mẫu ghi thô (core/raw_log.h): trục cảm biến, con quay đã trừ offset (bão hòa INT16) — đúng bước đầu của app_on_raw() */
void     app_raw_vec(const app_t *a, const int16_t raw_sensor[6], int16_t out[6]);
void     app_stage(const app_t *a, int8_t out[CNN_STAGE_BYTES]);
void     app_on_cnn(app_t *a, const cnn_res_t *r);
void     app_on_cnn_error(app_t *a);
void     app_restart_window(app_t *a);
void     app_tick(app_t *a, uint32_t now, uint8_t wear, uint8_t cancel_down);
void     app_web_cancel(app_t *a, uint32_t now);              /* POST /huy */
void     app_web_on(app_t *a, uint32_t now);                  /* POST /bat */
uint8_t  app_buzzer(const app_t *a, uint32_t now);
uint8_t  app_led(const app_t *a, uint32_t now);

#endif
