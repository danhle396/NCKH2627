/* fw_arm/test/test_entry.c — điểm vào cho fw_arm/test/m3_test.py: các hàm C thuần của firmware, biên dịch cho
 * Cortex-M3 bằng armclang, chạy trong Unicorn. Dữ liệu vào/ra qua con trỏ tới vùng nhớ test do Python ghi.
 * Ring cửa sổ như preprocess.c: ô cũ nhất ở `off`, cửa sổ = ring[off..767] rồi ring[0..off). */
#include <stdint.h>
#include "cnn_core.h"
#include "cnn_rom_inc.h"
#include "safety.h"

/* CNN phần mềm. Ra: res[0] = lớp (0/1), res[1] = SCORE, res[2..3] = logit. */
void t_sw(const int8_t *ring, uint32_t off, int32_t *res)
{
    int32_t lg[2];
    uint8_t sc;
    uint8_t c = cnn_sw_forward(cnn_rom_inc, ring + off, (uint16_t)(PRE_BYTES - off), ring, (uint16_t)off, lg, &sc);
    res[0] = c; res[1] = sc; res[2] = lg[0]; res[3] = lg[1];
}

void t_stage(const int8_t *ring, uint32_t off, int8_t *out)
{
    cnn_stage_from_window(ring + off, (uint16_t)(PRE_BYTES - off), ring, (uint16_t)off, out);
}

void t_selftest_stage(uint32_t step, int8_t *out)
{
    cnn_selftest_stage(step, out);
}

/* ---- imu_guard (kế hoạch 7.5): Python điều khiển từng bước ---- */
#include "imu_guard.h"
static imu_guard_t g_ig;
void     t_ig_init(uint32_t now) { imu_guard_init(&g_ig, now); }
uint32_t t_ig_poll(uint32_t now) { return (uint32_t)imu_guard_poll(&g_ig, now); }
void     t_ig_done(uint32_t now, uint32_t ok) { imu_guard_done(&g_ig, now, (int)ok); }
void     t_ig_sample(uint32_t now, uint32_t ok, const int16_t *raw) { imu_guard_sample(&g_ig, now, (int)ok, raw); }
/* out[0..7] = st, lost, cnn_reset (đọc rồi xóa), n_go_bus, n_reset_mem, n_cat_nguon, n_mat_cb, reading */
void t_ig_get(uint32_t *out)
{
    out[0] = g_ig.st; out[1] = g_ig.lost; out[2] = g_ig.cnn_reset; g_ig.cnn_reset = 0;
    out[3] = g_ig.n_go_bus; out[4] = g_ig.n_reset_mem; out[5] = g_ig.n_cat_nguon; out[6] = g_ig.n_mat_cb;
    out[7] = (uint32_t)imu_guard_reading(&g_ig);
}

/* ---- wifi_at (bước 6) với module ESP-01S giả: Python ghi byte vào t_rx, đọc byte firmware gửi ở t_tx ---- */
#include "wifi_at.h"
static wifi_t g_w;
uint8_t  t_rx[2048];
volatile uint32_t t_rx_n, t_rx_i;
uint8_t  t_tx[8192];
volatile uint32_t t_tx_n, t_tx_room = 100000u, t_rst, t_req_huy, t_req_bat;
static const uint8_t k_page[1500] = { 0x1F, 0x8B };
static int f_getc(void) { return t_rx_i < t_rx_n ? t_rx[t_rx_i++] : -1; }
static uint32_t f_write(const void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    if (n > t_tx_room) n = t_tx_room;                 /* vòng TX giả: Python giới hạn số byte nhận mỗi lần poll */
    for (uint32_t i = 0; i < n && t_tx_n < sizeof(t_tx); i++) t_tx[t_tx_n++] = b[i];
    t_tx_room -= n;
    return n;
}
static void f_rst(int on) { t_rst = (uint32_t)on; }
static void f_req(int r) { if (r == WIFI_REQ_HUY) t_req_huy++; else t_req_bat++; }
static uint32_t f_json(char *buf, uint32_t max)
{
    const char *s = "{\"t\":1,\"st\":\"MONITOR\"}";
    uint32_t n = 0;
    while (s[n] && n < max) { buf[n] = s[n]; n++; }
    return n;
}
/* nguồn thô giả: Python ghi t_rawsrc / t_rawsrc_n; t_raw_on = trạng thái raw_on() */
uint8_t  t_rawsrc[2048];
volatile uint32_t t_rawsrc_n, t_rawsrc_i, t_raw_on;
static void     f_raw_on(int on) { t_raw_on = (uint32_t)on; }
static uint32_t f_raw_avail(void) { return t_rawsrc_n - t_rawsrc_i; }
static uint32_t f_raw_take(char *b, uint32_t m)
{
    uint32_t k = 0;
    while (k < m && t_rawsrc_i < t_rawsrc_n) b[k++] = (char)t_rawsrc[t_rawsrc_i++];
    return k;
}
static const wifi_io_t k_io = { f_getc, f_write, f_rst, f_req, f_json, k_page, sizeof(k_page), "NCKH2627", "nckh2627",
                                f_raw_on, f_raw_avail, f_raw_take };

/* ---- raw_log: n mẫu (t, 6 trục, btn) → chuỗi CSV ---- */
#include "raw_log.h"
static raw_log_t g_rl;
uint32_t t_raw_log(const int32_t *m, uint32_t n, char *out)
{
    raw_log_start(&g_rl);
    uint32_t k = 0;
    for (uint32_t i = 0; i < n; i++) {
        int16_t v[6];
        for (int c = 0; c < 6; c++) v[c] = (int16_t)m[8 * i + 1 + c];
        raw_log_push(&g_rl, (uint32_t)m[8 * i], v, (uint8_t)m[8 * i + 7]);
        k += raw_log_take(&g_rl, out + k, 4096);
    }
    return k;
}
void t_wifi_init(uint32_t now) { wifi_init(&g_w, &k_io, now); }
uint32_t t_wifi_poll(uint32_t now) { wifi_poll(&g_w, now); return ((uint32_t)g_w.ready << 8) | g_w.st; }
void t_wifi_get(uint32_t *out) { out[0] = g_w.n_reset; out[1] = g_w.n_req; out[2] = g_w.n_err; }

/* ---- tầng 2 bắp tay (core/nga_bt.c) với ngưỡng do Python truyền vào ---- */
#include "nga_bt.h"
static nga_bt_t g_t2;
void     t_nga_init(const uint32_t *c)
{
    nga_bt_cfg_t cfg = { (uint16_t)c[0], (uint16_t)c[1], (uint16_t)c[2], (uint16_t)c[3], (uint16_t)c[4],
                         (uint8_t)c[5], (uint8_t)c[6], (uint8_t)c[7], c[8] };
    nga_bt_init(&g_t2, &cfg);
}
void     t_nga_mau(const int16_t *b, uint32_t n) { for (uint32_t i = 0; i < n; i++) nga_bt_mau(&g_t2, b + 6 * i); }
void     t_nga_bat_dau(uint32_t now) { nga_bt_bat_dau(&g_t2, now); }
uint32_t t_nga_buoc(uint32_t now) { return (uint32_t)nga_bt_buoc(&g_t2, now) | ((uint32_t)g_t2.dau_hieu << 8)
                                           | ((uint32_t)g_t2.d_moi << 16); }

/* ---- đường ống app: n mẫu thô (trục cảm biến) → app_on_raw → cửa sổ 768 byte hiện tại (đúng thứ tự thời gian) ---- */
#include "app.h"
static app_t g_app;
void t_app_pre(const int16_t *raw, uint32_t n, int8_t *win)
{
    const int8_t *s1, *s2;
    uint16_t n1, n2;
    app_init(&g_app, 0);
    for (uint32_t i = 0; i < n; i++) (void)app_on_raw(&g_app, raw + 6 * i);
    pre_window(&g_app.pre, &s1, &n1, &s2, &n2);
    for (uint32_t i = 0; i < n1; i++) win[i] = s1[i];
    for (uint32_t i = 0; i < n2; i++) win[n1 + i] = s2[i];
}

/* Chuỗi lớp của bộ tăng tốc → Safety FSM (đang MONITOR, đang đeo). map = 1: đổi mã như driver; 0: không đổi (lỗi cố ý).
 * Trả về trạng thái sau cùng; mỗi kết quả cách nhau 320 ms. */
uint32_t t_vote(const uint8_t *cls, uint32_t n, uint32_t map)
{
    static safety_t s;
    safety_cfg_t cfg;
    safety_default_cfg(&cfg);
    uint32_t now = 0;
    safety_init(&s, &cfg, now);
    for (int i = 0; i < 400; i++) { now += 10; safety_tick(&s, now, 1, 0); }     /* vào MONITOR */
    for (uint32_t i = 0; i < n; i++) {
        safety_on_result(&s, map ? cnn_map_class(cls[i]) : cls[i], 10);
        for (int k = 0; k < 32; k++) { now += 10; safety_tick(&s, now, 1, 0); }
        if (s.st == ST_SUSPECT) break;
    }
    return (uint32_t)s.st;
}
