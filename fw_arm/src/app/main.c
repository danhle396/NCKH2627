/*
 * main.c — firmware M3 NCKH2627 (Tang Nano 4K, Gowin_EMPU GW1NS-4C). Vòng lặp chính không chặn, không RTOS (kế hoạch 7.4).
 *   khởi động: GPIO, SysTick, UART1 (gỡ lỗi 115200), UART0 (ESP-01S 115200), tự kiểm bộ tăng tốc (O4), cắt nguồn cảm biến
 *              một lần (7.5.4), watchdog 2 s
 *   mỗi vòng: tự gỡ cảm biến (imu_guard) → đọc MPU6050 khi có ngắt INT → app (tiền xử lý, khối) → mỗi 16 mẫu 50 Hz gửi
 *             144 byte STAGE qua APB → DONE (USER_INT0) → app_on_cnn (đổi mã 1 → 3, Safety FSM) → còi, LED, web
 * Log UART1: "TK,<đạt>,<cycles>,…" khi khởi động; "GC,<offx>,<offy>,<offz>" khi hiệu chỉnh con quay xong; "S,…" mỗi bước.
 * Dữ liệu thô 100 Hz (core/raw_log.h): qua WiFi http://192.168.4.1/raw, hoặc qua UART1 khi gửi lệnh "R1" (dừng: "R0");
 * lúc ghi thô qua UART1 thì không in "S,…" để hai luồng không chen nhau. Thu bằng tools/thu_bap_tay.py.
 * Thời gian bộ tăng tốc = CYCLES / 27 MHz; CNN phần mềm (bước 8) đo bằng DWT khi CNN_SW_DOI_CHUNG = 1.
 */
#include <string.h>
#include "board.h"
#include "uart.h"
#include "i2c_mem.h"
#include "imu.h"
#include "cnn_drv.h"
#include "app.h"
#include "web.h"
#include "wifi_at.h"
#include "trang_web_gz.h"
#include "cnn_rom_inc.h"
#include "gyro_cal.h"
#include "raw_log.h"

#ifndef CNN_SW_DOI_CHUNG
#define CNN_SW_DOI_CHUNG 0          /* 1: mỗi bước chạy thêm CNN phần mềm trên M3, so với FPGA, log "W,…" (mục 10 bước 8) */
#endif
#ifndef WIFI_SSID
#define WIFI_SSID "NCKH2627"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "nckh2627"        /* đổi trước khi dùng thật */
#endif

static app_t       g_app;
static imu_guard_t g_ig;
static wifi_t      g_wifi;
static uint32_t    g_now;
static gyro_cal_t  g_gc;
static raw_log_t   g_raw;
static uint8_t     g_raw_uart;                  /* 1: luồng thô ra UART1 (lệnh R1) */

static int      w_getc(void) { return uart_getc(UART_WIFI); }
static uint32_t w_write(const void *p, uint32_t n) { return uart_write(UART_WIFI, p, n); }
static void     w_rst(int on) { board_set(PIN_WIFI_RST, on); }
static void     w_req(int r) { if (r == WIFI_REQ_HUY) app_web_cancel(&g_app, g_now); else app_web_on(&g_app, g_now); }
static uint32_t w_json(char *b, uint32_t m) { return web_json(&g_app, &g_ig, g_now, b, m); }
static void     w_raw_on(int on) { g_raw_uart = 0; if (on) raw_log_start(&g_raw); else raw_log_stop(&g_raw); }
static uint32_t w_raw_avail(void) { return raw_log_avail(&g_raw); }
static uint32_t w_raw_take(char *b, uint32_t m) { return raw_log_take(&g_raw, b, m); }
static const wifi_io_t k_wifi_io = { w_getc, w_write, w_rst, w_req, w_json, trang_web_gz, TRANG_WEB_GZ_LEN,
                                     WIFI_SSID, WIFI_PASS, w_raw_on, w_raw_avail, w_raw_take };

/* lệnh UART1: "R1" ghi thô ra UART1, "R0" dừng */
static void dbg_cmd(void)
{
    static char c0;
    int c;
    while ((c = uart_getc(UART_DBG)) >= 0) {
        if (c0 == 'R' && c == '1' && g_wifi.raw_id < 0) { g_raw_uart = 1; raw_log_start(&g_raw); }
        else if (c0 == 'R' && c == '0' && g_raw_uart) { g_raw_uart = 0; raw_log_stop(&g_raw); }
        c0 = (char)c;
    }
}

static void dbg(const char *s) { uart_puts(UART_DBG, s); }

int main(void)
{
    static char line[128];
    int8_t stage[CNN_STAGE_BYTES];
    uint32_t cyc = 0, t_tick, t_submit = 0;
    uint8_t pending = 0;

    board_init();
    uart_init(UART_DBG, 115200u);
    uart_init(UART_WIFI, 115200u);              /* [CẦN XÁC MINH bằng AT+UART_CUR? khi nhận ESP-01S] */
    i2c_init();
    cnn_drv_init();
    app_init(&g_app, board_ms());
    g_app.selftest_ok = (uint8_t)cnn_drv_selftest(&cyc);
    {
        sbuf_t b = { line, 0, sizeof(line) };
        sb_s(&b, "TK,"); sb_u(&b, g_app.selftest_ok); sb_s(&b, ","); sb_u(&b, cyc);
        sb_s(&b, ",vi_tri="); sb_s(&b, VT_TEN); sb_s(&b, ",mo_hinh="); sb_s(&b, CNN_MO_HINH); sb_s(&b, "\n");
        dbg(line);
    }
    cnn_drv_soft_reset();
    gyro_cal_init(&g_gc);
    imu_guard_init(&g_ig, board_ms());
    wifi_init(&g_wifi, &k_wifi_io, board_ms());
    board_wdt_start(2000u);
    t_tick = board_ms();

    for (;;) {
        g_now = board_ms();
        board_wdt_kick();

        /* ---- cảm biến: tự gỡ (7.5) ---- */
        ig_act_t act = imu_guard_poll(&g_ig, g_now);
        if (act != IG_ACT_NONE) {
            int r = imu_do(act);
            if (r >= 0) imu_guard_done(&g_ig, g_now, r);
        }
        if (g_ig.cnn_reset) {                    /* không đưa mẫu giả vào CNN: xóa cửa sổ, chạy lại 8 bước */
            g_ig.cnn_reset = 0;
            app_restart_window(&g_app);
            cnn_drv_soft_reset();
            pending = 0;
        }
        g_app.sensor_lost = g_ig.lost;
        if (board_imu_int_take() && imu_guard_reading(&g_ig)) {
            int16_t raw[6];
            int ok = imu_read(raw);
            imu_guard_sample(&g_ig, g_now, ok, raw);
            if (ok && !g_gc.done && gyro_cal_push(&g_gc, raw)) {     /* hiệu chỉnh con quay xong (đứng yên 2 s) */
                sbuf_t b = { line, 0, sizeof(line) };
                app_set_gyro_offset(&g_app, g_gc.off);
                app_restart_window(&g_app);                            /* mẫu trước đó trừ offset 0: bỏ */
                cnn_drv_soft_reset();
                pending = 0;
                sb_s(&b, "GC,"); sb_i(&b, g_gc.off[0]); sb_s(&b, ","); sb_i(&b, g_gc.off[1]); sb_s(&b, ",");
                sb_i(&b, g_gc.off[2]); sb_s(&b, "\n");
                if (!g_raw_uart) dbg(line);
            }
            if (ok && g_raw.on) {
                int16_t v[6];
                app_raw_vec(&g_app, raw, v);
                raw_log_push(&g_raw, g_now, v, (uint8_t)!(board_in() & PIN_BTN_N));
            }
            if (ok && (app_on_raw(&g_app, raw) & APP_STEP)) {
                if (pending) app_on_cnn_error(&g_app);               /* bước trước chưa xong (không được xảy ra) */
                app_stage(&g_app, stage);
                if (cnn_drv_submit(stage) == 0) { pending = 1; t_submit = g_now; }
                else app_on_cnn_error(&g_app);
            }
        }

        /* ---- kết quả bộ tăng tốc ---- */
        cnn_res_t r;
        if (cnn_drv_take(&r)) {
            pending = 0;
            app_on_cnn(&g_app, &r);
            if (!g_raw_uart) (void)uart_write(UART_DBG, line, web_log_step(&g_app, g_now, line, sizeof(line)));
#if CNN_SW_DOI_CHUNG
            {
                const int8_t *s1, *s2;
                uint16_t n1, n2;
                int32_t lg[2];
                uint8_t sc;
                pre_window(&g_app.pre, &s1, &n1, &s2, &n2);
                uint32_t c0 = board_cycles();
                uint8_t c = cnn_sw_forward(cnn_rom_inc, s1, n1, s2, n2, lg, &sc);
                uint32_t dc = board_cycles() - c0;
                sbuf_t b = { line, 0, sizeof(line) };
                sb_s(&b, "W,"); sb_u(&b, dc); sb_s(&b, ","); sb_u(&b, c); sb_s(&b, ","); sb_i(&b, lg[0]);
                sb_s(&b, ","); sb_i(&b, lg[1]);
                sb_s(&b, (r.valid && c == r.hw_cls && lg[0] == r.logit[0] && lg[1] == r.logit[1]) ? ",KHOP\n" :
                         r.valid ? ",LECH\n" : ",CHUA_HOP_LE\n");
                dbg(line);
            }
#endif
        } else if (pending && g_now - t_submit > CNN_TIMEOUT_US / 1000u) {
            pending = 0;
            app_on_cnn_error(&g_app);
            if (g_app.err_flag) { cnn_drv_soft_reset(); app_restart_window(&g_app); }
        }

        /* ---- 10 ms: nút, Safety FSM ---- */
        while (g_now - t_tick >= 10u) {
            t_tick += 10u;
            app_tick(&g_app, t_tick, 1u, (uint8_t)!(board_in() & PIN_BTN_N));
        }
        board_set(PIN_BUZ, app_buzzer(&g_app, g_now));
        board_set(PIN_LED, app_led(&g_app, g_now));

        /* ---- WiFi: lệnh AT + web server (+ luồng thô GET /raw) ---- */
        wifi_poll(&g_wifi, g_now);

        /* ---- UART1: lệnh R1/R0, luồng thô ---- */
        dbg_cmd();
        if (g_raw_uart) {
            char tmp[64];
            uint32_t f = uart_tx_free(UART_DBG);
            uint32_t n = raw_log_take(&g_raw, tmp, f < sizeof(tmp) ? f : sizeof(tmp));
            if (n) (void)uart_write(UART_DBG, tmp, n);
        }
    }
}
