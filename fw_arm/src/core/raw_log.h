/*
 * raw_log.h — ghi dữ liệu thô 100 Hz để thu dữ liệu bắp tay (kế hoạch 7.4, bước 7). C thuần, chạy được trên giả lập.
 *
 * Mỗi mẫu một dòng CSV (đúng định dạng tools/windowing.py đọc):
 *     t,ax,ay,az,gx,gy,gz,btn
 *   t   = ms từ lúc bật (uint32); ax..gz = INT16 theo TRỤC CẢM BIẾN, con quay ĐÃ trừ offset hiệu chuẩn, CHƯA đổi trục,
 *         CHƯA trừ offset gia tốc (đổi trục và offset làm ở windowing.py theo app/vi_tri_deo.h, giống app_on_raw());
 *   btn = 1 khi nút đang nhấn (đánh dấu sự kiện lúc thu).
 * Dòng đầu mỗi lần bắt đầu: RAW_HEADER. Vòng đệm đầy thì bỏ mẫu và đếm n_mat (windowing.py thấy khoảng trống t).
 * Đầu ra: WiFi (GET /raw, core/wifi_at.c) hoặc UART1 (lệnh "R1"/"R0"), do main.c chọn.
 */
#ifndef RAW_LOG_H
#define RAW_LOG_H
#include <stdint.h>

#define RAW_HEADER   "t,ax,ay,az,gx,gy,gz,btn\n"
#define RAW_BUF      1024u          /* ~ 0,2 s dữ liệu (≤ 50 byte/mẫu) */

typedef struct {
    uint8_t  on;
    uint16_t h, t;                  /* đầu ghi, đầu đọc */
    uint32_t n_mau, n_mat;
    char     buf[RAW_BUF];
} raw_log_t;

void     raw_log_start(raw_log_t *r);                 /* xóa vòng, ghi dòng tiêu đề */
void     raw_log_stop(raw_log_t *r);
void     raw_log_push(raw_log_t *r, uint32_t t_ms, const int16_t v[6], uint8_t btn);
uint32_t raw_log_avail(const raw_log_t *r);
uint32_t raw_log_take(raw_log_t *r, char *dst, uint32_t max);

#endif
