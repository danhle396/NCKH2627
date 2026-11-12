/* web.c — xem web.h. */
#include "web.h"
#include "cnn_rom_inc.h"            /* CNN_MO_HINH: tên mô hình đang nạp ("THU_NGAU_NHIEN" = mô hình thử, chưa huấn luyện) */

void sb_s(sbuf_t *b, const char *s)
{
    while (*s && b->n + 1u < b->max) b->p[b->n++] = *s++;
    b->p[b->n] = 0;
}

void sb_u(sbuf_t *b, uint32_t v)
{
    char t[11];
    int n = 0;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (n && b->n + 1u < b->max) b->p[b->n++] = t[--n];
    b->p[b->n] = 0;
}

void sb_i(sbuf_t *b, int32_t v)
{
    if (v < 0) { sb_s(b, "-"); sb_u(b, (uint32_t)(-(v + 1)) + 1u); }
    else sb_u(b, (uint32_t)v);
}

static void kv_u(sbuf_t *b, const char *k, uint32_t v) { sb_s(b, ",\""); sb_s(b, k); sb_s(b, "\":"); sb_u(b, v); }

uint32_t web_json(const app_t *a, const imu_guard_t *g, uint32_t now, char *buf, uint32_t max)
{
    sbuf_t b = { buf, 0, max };
    sb_s(&b, "{\"t\":"); sb_u(&b, now);
    sb_s(&b, ",\"st\":\""); sb_s(&b, safety_state_name(a->fsm.st)); sb_s(&b, "\"");
    kv_u(&b, "cls", a->last_cls);
    kv_u(&b, "score", a->last_score);
    kv_u(&b, "seq", a->n_step);
    kv_u(&b, "rc", a->err_flag ? 1u : 0u);
    kv_u(&b, "tk", a->selftest_ok);
    sb_s(&b, ",\"tt\":\"KHONG_RO\",\"mh\":\""); sb_s(&b, CNN_MO_HINH); sb_s(&b, "\"");                   /* tư thế: chưa có cho bắp tay (bước 7) */
    kv_u(&b, "nga", a->n_nga);
    kv_u(&b, "t_nga", a->t_last_fall);
    kv_u(&b, "cat_mpu", g->n_cat_nguon);
    sb_s(&b, ",\"ev\":[],\"pt\":[],\"acc\":[],\"res\":[]");
    sb_s(&b, ",\"s\":{\"cyc\":"); sb_u(&b, a->last_cycles);
    kv_u(&b, "ok", a->n_res);
    kv_u(&b, "to", a->n_err);
    kv_u(&b, "loi", a->n_invalid);
    kv_u(&b, "mat_cb", g->n_mat_cb);
    kv_u(&b, "bu", g->n_go_bus);
    kv_u(&b, "mat", g->n_reset_mem);
    kv_u(&b, "hong", 0);
    sb_s(&b, "}}");
    return b.n;
}

uint32_t web_log_step(const app_t *a, uint32_t now, char *buf, uint32_t max)
{
    sbuf_t b = { buf, 0, max };
    sb_s(&b, "S,"); sb_u(&b, now);
    sb_s(&b, ","); sb_u(&b, a->n_step);
    sb_s(&b, ","); sb_u(&b, a->last_valid);
    sb_s(&b, ","); sb_u(&b, a->last_cls);
    sb_s(&b, ","); sb_u(&b, a->last_score);
    sb_s(&b, ","); sb_i(&b, a->last_logit[0]);
    sb_s(&b, ","); sb_i(&b, a->last_logit[1]);
    sb_s(&b, ","); sb_u(&b, a->last_cycles);
    sb_s(&b, ","); sb_s(&b, safety_state_name(a->fsm.st));
    sb_s(&b, "\n");
    return b.n;
}
