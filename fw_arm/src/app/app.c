/* app.c — xem app.h. Còi và LED giữ cách kêu của bản dự thi (app_core.c, 28/10). */
#include <string.h>
#include "app.h"

void app_init(app_t *a, uint32_t now)
{
    safety_cfg_t sc;
    blocks_cfg_t bc = { T_STILL_A, T_STILL_G };
    memset(a, 0, sizeof(*a));
    pre_init(&a->pre);
    blocks_init(&a->blk, &bc);
    safety_default_cfg(&sc);
    safety_init(&a->fsm, &sc, now);
    pwr_btn_init(&a->pb, 1);
#if NGA_BAT
    {
        const nga_bt_cfg_t c = { NGA_ROI_LSB, NGA_VA_LSB, NGA_QUAY_LSB, NGA_YEN_A, NGA_YEN_G,
                                 NGA_TIM_KHOI, NGA_ROI_KHOI, NGA_YEN_KHOI, NGA_CHO_MS };
        nga_bt_init(&a->t2, &c);
    }
#endif
}

void app_set_gyro_offset(app_t *a, const int16_t off[3])
{
    for (int c = 0; c < 3; c++) a->gyro_off[c] = off[c];
}

static int16_t pick(const int16_t *v, int code)       /* ±1/±2/±3 = ± trục X/Y/Z */
{
    int32_t x = v[(code < 0 ? -code : code) - 1];
    return (int16_t)(code < 0 ? (x == -32768 ? 32767 : -x) : x);
}

static int16_t sat16(int32_t v) { return (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v); }

void app_raw_vec(const app_t *a, const int16_t raw[6], int16_t out[6])
{
    for (int c = 0; c < 3; c++) {
        out[c] = raw[c];
        out[3 + c] = sat16((int32_t)raw[3 + c] - a->gyro_off[c]);
    }
}

unsigned app_on_raw(app_t *a, const int16_t raw[6])
{
    int16_t g[3], r[6];
    unsigned f = 0;
    for (int c = 0; c < 3; c++) g[c] = sat16((int32_t)raw[3 + c] - a->gyro_off[c]);
    r[0] = sat16((int32_t)pick(raw, TRUC_X) - ACC_OFF_X);
    r[1] = sat16((int32_t)pick(raw, TRUC_Y) - ACC_OFF_Y);
    r[2] = sat16((int32_t)pick(raw, TRUC_Z) - ACC_OFF_Z);
    r[3] = pick(g, TRUC_X); r[4] = pick(g, TRUC_Y); r[5] = pick(g, TRUC_Z);
    a->n_raw++;
#if NGA_BAT
    nga_bt_mau(&a->t2, r);
#endif
    if (blocks_push(&a->blk, r)) {
        safety_on_block(&a->fsm, a->blk.still_run, a->blk.az_tb);
        f |= APP_BLOCK;
    }
    if ((pre_push_raw(&a->pre, r, 0) & PRE_NEW_SAMPLE) && (a->pre.n_out % PRE_HOP) == 0u) {
        a->n_step++;
        f |= APP_STEP;
    }
    return f;
}

void app_stage(const app_t *a, int8_t out[CNN_STAGE_BYTES])
{
    const int8_t *s1, *s2;
    uint16_t n1, n2;
    pre_window(&a->pre, &s1, &n1, &s2, &n2);
    cnn_stage_from_window(s1, n1, s2, n2, out);
}

void app_on_cnn(app_t *a, const cnn_res_t *r)
{
    a->consec_err = 0;
    a->err_flag = 0;
    a->last_valid = r->valid;
    a->last_cycles = r->cycles;
    if (!r->valid) { a->n_invalid++; return; }                /* WARM < 8: bỏ (kế hoạch O1) */
    a->n_res++;
    a->last_cls = cnn_map_class(r->hw_cls);                   /* O3: 1 → 3 */
    a->last_score = r->score;
    a->last_logit[0] = r->logit[0];
    a->last_logit[1] = r->logit[1];
    if (a->last_cls == CLS_FALL) a->n_fall_win++;
    safety_on_result(&a->fsm, a->last_cls, a->last_score);
}

void app_on_cnn_error(app_t *a)
{
    a->n_err++;
    if (++a->consec_err >= CNN_ERR_RESET) a->err_flag = 1;
}

void app_restart_window(app_t *a)
{
    pre_init(&a->pre);
}

void app_tick(app_t *a, uint32_t now, uint8_t wear, uint8_t cancel)
{
#if BTN_POWER
    cancel = pwr_btn_tick(&a->pb, now, cancel, (uint8_t)a->fsm.st);
    wear = (uint8_t)(wear && a->pb.on);
    if (a->pb.ev) { a->pwr_ev = a->pb.ev; a->t_pwr = now; }
#endif
    safety_state_t before = a->fsm.st;
    safety_tick(&a->fsm, now, wear, cancel);
    if (a->fsm.st == ST_SUSPECT && before != ST_SUSPECT) {
        a->t_last_fall = now;
        a->n_nga++;
#if NGA_BAT
        nga_bt_bat_dau(&a->t2, now);
#endif
    }
#if NGA_BAT
    if ((a->fsm.st == ST_SUSPECT || a->fsm.st == ST_PRE_ALARM) && nga_bt_buoc(&a->t2, now) == NGA_BT_BO)
        safety_bo_nghi(&a->fsm, now);                 /* không đủ 4 dấu hiệu: về MONITOR */
#endif
}

/* Hủy qua web = một lần nhấn CANCEL ngắn: đưa vào safety_tick hai lần (nhấn rồi nhả). */
void app_web_cancel(app_t *a, uint32_t now)
{
    safety_tick(&a->fsm, now, 1, 1);
    safety_tick(&a->fsm, now, 1, 0);
}

void app_web_on(app_t *a, uint32_t now)
{
    if (!a->pb.on) { a->pb.on = 1; a->pwr_ev = PB_EV_ON; a->t_pwr = now; }
}

uint8_t app_buzzer(const app_t *a, uint32_t now)
{
    uint32_t t = now - a->fsm.t_enter;
    if (a->sensor_lost) return (now % 2000u) < 100u;          /* MẤT CẢM BIẾN: bíp ngắn mỗi 2 s (7.5.4 bậc 4) */
    switch (a->fsm.st) {
    case ST_SUSPECT:   return (t % 1000u) < 400u;
    case ST_PRE_ALARM: return (t % 500u) < 150u;
    case ST_ALARM:     return (t % 500u) < 350u;
    default:
        if (a->pwr_ev) {
            uint32_t d = now - a->t_pwr;
            if (a->pwr_ev == PB_EV_ON && d < 300u) return d < 100u || d >= 200u;
            if (a->pwr_ev == PB_EV_OFF && d < 600u) return 1;
        }
        return 0;
    }
}

uint8_t app_led(const app_t *a, uint32_t now)
{
    if (a->err_flag || !a->selftest_ok) return (now % 200u) < 100u;
    if (a->sensor_lost) return (now % 600u) < 300u;
    switch (a->fsm.st) {
    case ST_STANDBY:   return (now % 2000u) < 50u;
    case ST_MONITOR:   return (now % 1000u) < 50u;
    case ST_SUSPECT:   return (now % 400u) < 200u;
    case ST_PRE_ALARM:
    case ST_ALARM:     return 1;
    default:           return (now % 1000u) < 500u;
    }
}
