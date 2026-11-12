/* nga_bt.c — xem nga_bt.h. */
#include <string.h>
#include "nga_bt.h"

static uint32_t sq3(const int16_t *v)
{
    return (uint32_t)((int32_t)v[0] * v[0]) + (uint32_t)((int32_t)v[1] * v[1]) + (uint32_t)((int32_t)v[2] * v[2]);
}

void nga_bt_init(nga_bt_t *g, const nga_bt_cfg_t *cfg)
{
    memset(g, 0, sizeof(*g));
    g->cfg = *cfg;
}

static uint8_t lui(const nga_bt_t *g, uint8_t d) { return (uint8_t)((g->i + NGA_VONG - 1u - d) % NGA_VONG); }

/* khối cách khối va chạm `d` khối (d âm = trước); khối va chạm ở vị trí lùi g->d_moi */
static const nga_khoi_t *quanh(const nga_bt_t *g, int d) { return &g->k[lui(g, (uint8_t)((int)g->d_moi - d))]; }

static void danh_gia(nga_bt_t *g)
{
    const nga_bt_cfg_t *c = &g->cfg;
    uint32_t r2 = (uint32_t)c->roi_lsb * c->roi_lsb, q2 = (uint32_t)c->quay_lsb * c->quay_lsb;
    for (int d = -(int)c->roi_khoi; d <= 0; d++)
        if ((int)g->d_moi - d < (int)g->n && quanh(g, d)->a2_min <= r2) g->dau_hieu |= 1u;
    for (int d = -1; d <= 1; d++)
        if ((int)g->d_moi - d >= 0 && (int)g->d_moi - d < (int)g->n && quanh(g, d)->w2_max >= q2) g->dau_hieu |= 4u;
    for (int d = 1; d <= (int)g->d_moi && d <= (int)c->yen_khoi; d++)
        if (quanh(g, d)->yen) g->dau_hieu |= 8u;
}

void nga_bt_mau(nga_bt_t *g, const int16_t b[6])
{
    uint32_t a2 = sq3(b), w2 = sq3(b + 3);
    if (g->m == 0) {
        g->a2_min = g->a2_max = a2;
        g->w2_max = w2;
        for (int c = 0; c < 6; c++) g->mn[c] = g->mx[c] = b[c];
    } else {
        if (a2 < g->a2_min) g->a2_min = a2;
        if (a2 > g->a2_max) g->a2_max = a2;
        if (w2 > g->w2_max) g->w2_max = w2;
        for (int c = 0; c < 6; c++) {
            if (b[c] < g->mn[c]) g->mn[c] = b[c];
            if (b[c] > g->mx[c]) g->mx[c] = b[c];
        }
    }
    if (++g->m < NGA_KHOI_MAU) return;
    g->m = 0;
    nga_khoi_t *k = &g->k[g->i];
    k->a2_min = g->a2_min; k->a2_max = g->a2_max; k->w2_max = g->w2_max;
    k->yen = 1;
    for (int c = 0; c < 6; c++)
        if (g->mx[c] - g->mn[c] >= (c < 3 ? g->cfg.yen_a : g->cfg.yen_g)) k->yen = 0;
    g->i = (uint8_t)((g->i + 1u) % NGA_VONG);
    if (g->n < NGA_VONG) g->n++;
    if (g->dang_xet) {
        if (g->d_moi < NGA_VONG - 1u) g->d_moi++;
        danh_gia(g);
    }
}

void nga_bt_bat_dau(nga_bt_t *g, uint32_t now)
{
    uint8_t best = 0;
    uint32_t a2 = 0;
    for (uint8_t d = 0; d < g->cfg.tim_khoi && d < g->n; d++)
        if (g->k[lui(g, d)].a2_max > a2) { a2 = g->k[lui(g, d)].a2_max; best = d; }
    g->dang_xet = 1;
    g->d_moi = best;
    g->t0 = now;
    g->dau_hieu = (g->n && a2 >= (uint32_t)g->cfg.va_lsb * g->cfg.va_lsb) ? 2u : 0u;
    g->n_xet++;
    danh_gia(g);
}

int nga_bt_buoc(nga_bt_t *g, uint32_t now)
{
    if (!g->dang_xet) return NGA_BT_CHO;
    if (g->dau_hieu == 0x0Fu) { g->dang_xet = 0; g->n_xac_nhan++; return NGA_BT_XAC_NHAN; }
    if ((g->dau_hieu & 3u) != 3u || g->d_moi > g->cfg.yen_khoi || now - g->t0 >= g->cfg.cho_ms) {
        g->dang_xet = 0;
        g->n_bo++;
        return NGA_BT_BO;
    }
    return NGA_BT_CHO;
}
