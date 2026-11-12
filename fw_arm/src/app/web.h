/* web.h — JSON của GET /data (trường theo trang web của bản dự thi, tools/gen_web_gz.py) và dòng log UART1.
 * Không dùng printf (tiết kiệm flash 32 KB). */
#ifndef WEB_H
#define WEB_H
#include <stdint.h>
#include "app.h"
#include "imu_guard.h"

typedef struct { char *p; uint32_t n, max; } sbuf_t;
void     sb_s(sbuf_t *b, const char *s);
void     sb_i(sbuf_t *b, int32_t v);
void     sb_u(sbuf_t *b, uint32_t v);

uint32_t web_json(const app_t *a, const imu_guard_t *g, uint32_t now, char *buf, uint32_t max);
/* dòng kết quả mỗi bước: S,<ms>,<bước>,<valid>,<lớp 0/3>,<score>,<logit0>,<logit1>,<cycles>,<st>\n */
uint32_t web_log_step(const app_t *a, uint32_t now, char *buf, uint32_t max);

#endif
