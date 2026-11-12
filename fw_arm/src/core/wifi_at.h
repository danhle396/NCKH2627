/*
 * wifi_at.h — web server trên M3 qua module WiFi dùng như modem (ESP-01S, firmware AT gốc; kế hoạch 7.3, bước 6).
 * Máy trạng thái KHÔNG CHẶN: mỗi vòng lặp chính gọi wifi_poll(); đọc/ghi UART qua wifi_io_t (không đụng phần cứng,
 * nên chạy được trong fw_arm/test/m3_test.py với module giả).
 *
 * Khởi động: RST mức 1 trong 50 ms (qua Q1) → chờ "ready" (≤ 3 s) → ATE0 → AT+CWMODE=2 → AT+CWSAP="<ssid>","<mk>",5,3
 *            → AT+CIPMUX=1 → AT+CIPSERVER=1,80. Lệnh nào lỗi hay quá hạn thì reset module làm lại (đếm n_reset).
 *            (Dạng lệnh chung của AT 1.x và 2.x; kiểm phiên bản bằng AT+GMR khi nhận hàng [CẦN XÁC MINH].)
 * Phục vụ: "+IPD,<id>,<len>:<dữ liệu>" → dòng yêu cầu:
 *   GET /        → trang web nén gzip (Content-Encoding: gzip) từ flash
 *   GET /data    → JSON trạng thái (io->json)
 *   POST /huy    → io->request(WIFI_REQ_HUY);  POST /bat → io->request(WIFI_REQ_BAT)
 *   GET /raw     → luồng dữ liệu thô 100 Hz (core/raw_log.h) trên kết nối giữ mở: io->raw_on(1), rồi mỗi khi có
 *                  ≥ 512 byte hoặc sau 200 ms thì gửi một khúc (không đóng kết nối); kết nối đóng / lỗi → io->raw_on(0).
 *                  Xen kẽ với các yêu cầu khác (yêu cầu trong hàng đợi được phục vụ trước).
 *   còn lại      → 404
 * Gửi: AT+CIPSEND=<id>,<n> → chờ '>' → n byte → chờ "SEND OK" (mỗi lần ≤ 1024 byte) → AT+CIPCLOSE=<id>.
 * Hàng đợi 4 yêu cầu. Module treo (quá hạn 3 s) → reset qua RST; còi vẫn là cảnh báo chính.
 */
#ifndef WIFI_AT_H
#define WIFI_AT_H
#include <stdint.h>

#define WIFI_REQ_HUY  1
#define WIFI_REQ_BAT  2

typedef struct {
    int      (*getc)(void);                         /* −1 nếu chưa có byte */
    uint32_t (*write)(const void *p, uint32_t n);   /* trả về số byte nhận (không chặn) */
    void     (*rst)(int on);                        /* 1 = giữ module ở reset */
    void     (*request)(int req);                   /* POST /huy, /bat */
    uint32_t (*json)(char *buf, uint32_t max);      /* nội dung GET /data */
    const uint8_t *page;                            /* trang web gzip */
    uint32_t page_len;
    const char *ssid, *pass;
    void     (*raw_on)(int on);                     /* bắt đầu / dừng ghi dữ liệu thô */
    uint32_t (*raw_avail)(void);
    uint32_t (*raw_take)(char *buf, uint32_t max);
} wifi_io_t;

typedef struct { uint8_t id, kind; } wifi_req_t;

typedef struct {
    const wifi_io_t *io;
    uint8_t  st, cmd;               /* trạng thái, lệnh khởi động đang chạy */
    uint32_t t;                     /* mốc của trạng thái */
    char     line[96];
    uint8_t  nline;
    uint16_t ipd_left;              /* byte dữ liệu +IPD còn phải đọc */
    uint8_t  ipd_id;
    char     req[24];
    uint8_t  nreq;
    wifi_req_t q[4];
    uint8_t  qh, qt;
    /* đang gửi */
    uint8_t  cur_id;
    char     hdr[160];
    uint16_t hdr_len;
    const uint8_t *body;
    uint32_t body_len;
    uint32_t off, chunk, sent;      /* vị trí trong (hdr + body), cỡ khúc hiện tại, byte đã ghi của khúc */
    char     jbuf[640];
    uint8_t  ready;                 /* đã mở server */
    int8_t   raw_id;                /* kết nối đang nhận luồng thô, −1 nếu không có */
    uint8_t  raw_tx;                /* khúc đang gửi là dữ liệu thô / đầu luồng (không đóng kết nối) */
    uint32_t t_raw, n_raw_byte;
    uint32_t n_reset, n_req, n_err;
} wifi_t;

void wifi_init(wifi_t *w, const wifi_io_t *io, uint32_t now);
void wifi_poll(wifi_t *w, uint32_t now);

#endif
