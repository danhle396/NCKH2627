/* raw_log.c — xem raw_log.h. Không dùng printf. */
#include "raw_log.h"

static uint32_t room(const raw_log_t *r) { return (uint32_t)((r->t + RAW_BUF - r->h - 1u) % RAW_BUF); }

uint32_t raw_log_avail(const raw_log_t *r) { return (uint32_t)((r->h + RAW_BUF - r->t) % RAW_BUF); }

static void put(raw_log_t *r, const char *s, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        r->buf[r->h] = s[i];
        r->h = (uint16_t)((r->h + 1u) % RAW_BUF);
    }
}

static uint32_t fmt_u(char *d, uint32_t v)
{
    char t[10];
    uint32_t n = 0, k = 0;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (n) d[k++] = t[--n];
    return k;
}

static uint32_t fmt_i(char *d, int32_t v)
{
    if (v < 0) { d[0] = '-'; return 1u + fmt_u(d + 1, (uint32_t)(-v)); }
    return fmt_u(d, (uint32_t)v);
}

void raw_log_start(raw_log_t *r)
{
    r->h = r->t = 0;
    r->n_mau = r->n_mat = 0;
    put(r, RAW_HEADER, sizeof(RAW_HEADER) - 1u);
    r->on = 1;
}

void raw_log_stop(raw_log_t *r) { r->on = 0; }

void raw_log_push(raw_log_t *r, uint32_t t_ms, const int16_t v[6], uint8_t btn)
{
    char line[64];
    uint32_t n;
    if (!r->on) return;
    n = fmt_u(line, t_ms);
    for (int c = 0; c < 6; c++) { line[n++] = ','; n += fmt_i(line + n, v[c]); }
    line[n++] = ',';
    line[n++] = btn ? '1' : '0';
    line[n++] = '\n';
    if (n > room(r)) { r->n_mat++; return; }
    put(r, line, n);
    r->n_mau++;
}

uint32_t raw_log_take(raw_log_t *r, char *dst, uint32_t max)
{
    uint32_t k = 0;
    while (k < max && r->t != r->h) {
        dst[k++] = r->buf[r->t];
        r->t = (uint16_t)((r->t + 1u) % RAW_BUF);
    }
    return k;
}
