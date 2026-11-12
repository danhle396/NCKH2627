/* imu_guard.c — xem imu_guard.h. */
#include <string.h>
#include "imu_guard.h"

static void go(imu_guard_t *g, ig_state_t st, uint32_t now) { g->st = st; g->t = now; }

static ig_act_t fire(imu_guard_t *g, ig_act_t a) { g->pending = a; return a; }

static void start_l3(imu_guard_t *g, uint32_t now)
{
    g->n_cat_nguon++;
    go(g, IG_L3_OFF, now);
    g->pending = IG_ACT_POWER_OFF;
}

void imu_guard_init(imu_guard_t *g, uint32_t now)
{
    memset(g, 0, sizeof(*g));
    g->cnn_reset = 1;
    g->n_cat_nguon = 0;
    go(g, IG_L3_OFF, now);
    g->pending = IG_ACT_POWER_OFF;           /* poll() đầu tiên trả về POWER_OFF */
}

void imu_guard_sample(imu_guard_t *g, uint32_t now, int ok, const int16_t raw[6])
{
    if (g->st != IG_OK) return;
    g->t_last_int = now;
    if (!ok) { g->errs++; return; }
    g->errs = 0;
    if (memcmp(raw, g->last, sizeof(g->last)) == 0) { if (g->same < 0xFFFFu) g->same++; }
    else { g->same = 0; memcpy(g->last, raw, sizeof(g->last)); }
}

ig_act_t imu_guard_poll(imu_guard_t *g, uint32_t now)
{
    ig_act_t p = g->pending;
    if (p == IG_ACT_POWER_OFF || p == IG_ACT_POWER_ON || p == IG_ACT_SOFT_RESET) {
        g->pending = IG_ACT_NONE;            /* hành động không cần done(): trả về một lần */
        return p;
    }
    if (p != IG_ACT_NONE) return IG_ACT_NONE; /* đang chờ done() */
    switch (g->st) {
    case IG_OK:
        if (now - g->t_last_int > IG_INT_TIMEOUT_MS || g->errs >= IG_ERR_MAX || g->same >= IG_SAME_MAX) {
            g->n_go_bus++;
            g->cnn_reset = 1;
            g->errs = 0; g->same = 0;
            return fire(g, IG_ACT_BUS_CLEAR_CFG);
        }
        return IG_ACT_NONE;
    case IG_L2_WAIT:
        if (now - g->t >= IG_RESET_WAIT_MS) return fire(g, IG_ACT_CFG);
        return IG_ACT_NONE;
    case IG_L3_OFF:
        if (now - g->t >= IG_PWR_OFF_MS) { go(g, IG_L3_ON_WAIT, now); g->pending = IG_ACT_NONE; return IG_ACT_POWER_ON; }
        return IG_ACT_NONE;
    case IG_L3_ON_WAIT:
        if (now - g->t >= IG_PWR_ON_WAIT_MS) return fire(g, IG_ACT_RELEASE_CFG);
        return IG_ACT_NONE;
    case IG_LOST:
        if (now - g->t >= IG_LOST_RETRY_MS) { start_l3(g, now); g->pending = IG_ACT_NONE; return IG_ACT_POWER_OFF; }
        return IG_ACT_NONE;
    }
    return IG_ACT_NONE;
}

void imu_guard_done(imu_guard_t *g, uint32_t now, int ok)
{
    ig_act_t p = g->pending;
    g->pending = IG_ACT_NONE;
    if (ok) {                                    /* hồi phục: đọc lại bình thường, app chạy lại 8 bước khởi động */
        go(g, IG_OK, now);
        g->t_last_int = now;
        g->errs = 0; g->same = 0;
        g->lost = 0;
        return;
    }
    if (p == IG_ACT_BUS_CLEAR_CFG) {             /* bậc 1 lỗi → bậc 2 */
        g->n_reset_mem++;
        go(g, IG_L2_WAIT, now);
        g->pending = IG_ACT_SOFT_RESET;
    } else if (p == IG_ACT_CFG) {                /* bậc 2 lỗi → bậc 3 */
        start_l3(g, now);
    } else {                                     /* bậc 3 lỗi → bậc 4 */
        if (!g->lost) g->n_mat_cb++;
        g->lost = 1;
        go(g, IG_LOST, now);
    }
}
