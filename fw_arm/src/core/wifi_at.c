/* wifi_at.c — xem wifi_at.h. */
#include <string.h>
#include "wifi_at.h"

enum { W_RST, W_BOOT, W_CMD, W_CMD_WAIT, W_IDLE, W_TX_CMD, W_TX_PROMPT, W_TX_DATA, W_TX_WAIT, W_CLOSE, W_CLOSE_WAIT };
enum { EV_OK = 1, EV_ERR = 2, EV_SENDOK = 4, EV_READY = 8, EV_PROMPT = 16 };
enum { K_PAGE = 1, K_DATA, K_OKPOST, K_404, K_RAW };
#define RAW_MIN 512u
#define RAW_MAX_MS 200u

#define T_RST_MS     50u
#define T_BOOT_MS    3000u
#define T_CMD_MS     3000u
#define T_CWSAP_MS   8000u
#define T_TX_MS      3000u
#define CHUNK_MAX    1024u

static uint8_t g_ev;                         /* sự kiện của lần poll này (một wifi_t mỗi thiết bị) */

static void go(wifi_t *w, uint8_t st, uint32_t now) { w->st = st; w->t = now; }

static uint32_t u2s(char *d, uint32_t v)
{
    char t[10];
    uint32_t n = 0, k = 0;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (n) d[k++] = t[--n];
    return k;
}

static uint32_t cat(char *d, uint32_t k, const char *s)
{
    while (*s) d[k++] = *s++;
    return k;
}

static void send_str(wifi_t *w, const char *s) { (void)w->io->write(s, (uint32_t)strlen(s)); }

static void raw_stop(wifi_t *w)
{
    if (w->raw_id >= 0) w->io->raw_on(0);
    w->raw_id = -1;
}

static void reset_module(wifi_t *w, uint32_t now)
{
    raw_stop(w);
    w->ready = 0;
    w->ipd_left = 0; w->nline = 0;
    w->io->rst(1);
    go(w, W_RST, now);
}

static void enqueue(wifi_t *w)
{
    uint8_t kind;
    w->req[w->nreq < sizeof(w->req) ? w->nreq : sizeof(w->req) - 1u] = 0;
    if (!strncmp(w->req, "GET /data", 9)) kind = K_DATA;
    else if (!strncmp(w->req, "GET /raw", 8)) kind = K_RAW;
    else if (!strncmp(w->req, "GET / ", 6) || !strncmp(w->req, "GET /index", 10)) kind = K_PAGE;
    else if (!strncmp(w->req, "POST /huy", 9)) { kind = K_OKPOST; w->io->request(WIFI_REQ_HUY); }
    else if (!strncmp(w->req, "POST /bat", 9)) { kind = K_OKPOST; w->io->request(WIFI_REQ_BAT); }
    else if (!strncmp(w->req, "GET ", 4) || !strncmp(w->req, "POST ", 5)) kind = K_404;
    else return;                             /* phần thân của yêu cầu trước: bỏ qua */
    uint8_t n = (uint8_t)((w->qh + 1u) & 3u);
    if (n == w->qt) { w->n_err++; return; } /* hàng đợi đầy */
    w->q[w->qh].id = w->ipd_id;
    w->q[w->qh].kind = kind;
    w->qh = n;
    w->n_req++;
}

static void line_done(wifi_t *w)
{
    char *l = w->line;
    l[w->nline] = 0;
    if (w->nline && l[w->nline - 1u] == '\r') l[--w->nline] = 0;
    if (!strcmp(l, "OK")) g_ev |= EV_OK;
    else if (!strcmp(l, "ERROR") || !strcmp(l, "FAIL") || !strcmp(l, "SEND FAIL")) g_ev |= EV_ERR;
    else if (!strcmp(l, "SEND OK")) g_ev |= EV_SENDOK;
    else if (!strcmp(l, "ready")) g_ev |= EV_READY;
    else if (w->raw_id >= 0 && w->nline >= 8 && !strcmp(l + w->nline - 7, ",CLOSED")
             && (uint32_t)(l[0] - '0') == (uint32_t)w->raw_id) raw_stop(w);              /* "<id>,CLOSED" */
    w->nline = 0;
}

static void rx_char(wifi_t *w, char c)
{
    if (w->ipd_left) {
        if (w->nreq < sizeof(w->req) - 1u) w->req[w->nreq++] = c;
        if (--w->ipd_left == 0) enqueue(w);
        return;
    }
    if (c == '>' && w->nline == 0) { g_ev |= EV_PROMPT; return; }
    if (c == '\n') { line_done(w); return; }
    if (w->nline < sizeof(w->line) - 1u) w->line[w->nline++] = c;
    if (c == ':' && w->nline > 5 && !strncmp(w->line, "+IPD,", 5)) {      /* +IPD,<id>,<len>: */
        uint32_t id = 0, len = 0, i = 5;
        while (i < w->nline && w->line[i] >= '0' && w->line[i] <= '9') id = id * 10u + (uint32_t)(w->line[i++] - '0');
        if (i < w->nline && w->line[i] == ',') i++;
        while (i < w->nline && w->line[i] >= '0' && w->line[i] <= '9') len = len * 10u + (uint32_t)(w->line[i++] - '0');
        w->ipd_id = (uint8_t)id;
        w->ipd_left = (uint16_t)len;
        w->nreq = 0;
        w->nline = 0;
        if (!len) enqueue(w);
    }
}

static const char *const k_cmds[] = {
    "ATE0\r\n", "AT+CWMODE=2\r\n", 0 /* CWSAP dựng từ ssid/pass */, "AT+CIPMUX=1\r\n", "AT+CIPSERVER=1,80\r\n"
};
#define N_CMDS 5u

static void send_cmd(wifi_t *w)
{
    if (w->cmd == 2u) {
        uint32_t k = cat(w->hdr, 0, "AT+CWSAP=\"");
        k = cat(w->hdr, k, w->io->ssid); k = cat(w->hdr, k, "\",\"");
        k = cat(w->hdr, k, w->io->pass); k = cat(w->hdr, k, "\",5,3\r\n");
        w->hdr[k] = 0;
        send_str(w, w->hdr);
    } else send_str(w, k_cmds[w->cmd]);
}

static void build_resp(wifi_t *w, const wifi_req_t *r)
{
    uint32_t k = 0;
    const char *ok = "HTTP/1.1 200 OK\r\n";
    w->cur_id = r->id;
    w->raw_tx = 0;
    if (r->kind == K_RAW) {                  /* đầu luồng thô: không Content-Length, giữ kết nối */
        raw_stop(w);
        w->raw_id = (int8_t)r->id;
        w->io->raw_on(1);
        w->raw_tx = 1;
        w->body = (const uint8_t *)""; w->body_len = 0;
        k = cat(w->hdr, k, ok);
        k = cat(w->hdr, k, "Content-Type: text/csv\r\nCache-Control: no-store\r\n\r\n");
        w->hdr_len = (uint16_t)k;
        w->off = 0;
        return;
    }
    if (r->kind == K_PAGE) {
        w->body = w->io->page; w->body_len = w->io->page_len;
        k = cat(w->hdr, k, ok);
        k = cat(w->hdr, k, "Content-Type: text/html; charset=utf-8\r\nContent-Encoding: gzip\r\n");
    } else if (r->kind == K_DATA) {
        w->body_len = w->io->json(w->jbuf, sizeof(w->jbuf));
        w->body = (const uint8_t *)w->jbuf;
        k = cat(w->hdr, k, ok);
        k = cat(w->hdr, k, "Content-Type: application/json\r\nCache-Control: no-store\r\n");
    } else if (r->kind == K_OKPOST) {
        w->body = (const uint8_t *)"ok"; w->body_len = 2;
        k = cat(w->hdr, k, ok);
        k = cat(w->hdr, k, "Content-Type: text/plain\r\n");
    } else {
        w->body = (const uint8_t *)"404"; w->body_len = 3;
        k = cat(w->hdr, k, "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n");
    }
    k = cat(w->hdr, k, "Content-Length: ");
    k += u2s(w->hdr + k, w->body_len);
    k = cat(w->hdr, k, "\r\nConnection: close\r\n\r\n");
    w->hdr_len = (uint16_t)k;
    w->off = 0;
}

static uint8_t at_resp(const wifi_t *w, uint32_t i) { return i < w->hdr_len ? (uint8_t)w->hdr[i] : w->body[i - w->hdr_len]; }

void wifi_init(wifi_t *w, const wifi_io_t *io, uint32_t now)
{
    memset(w, 0, sizeof(*w));
    w->io = io;
    w->raw_id = -1;
    reset_module(w, now);
    w->n_reset = 0;
}

void wifi_poll(wifi_t *w, uint32_t now)
{
    int c;
    g_ev = 0;
    for (int n = 0; n < 512 && (c = w->io->getc()) >= 0; n++) rx_char(w, (char)c);
    uint32_t dt = now - w->t;
    switch (w->st) {
    case W_RST:
        if (dt >= T_RST_MS) { w->io->rst(0); go(w, W_BOOT, now); }
        break;
    case W_BOOT:
        if ((g_ev & EV_READY) || dt >= T_BOOT_MS) { w->cmd = 0; go(w, W_CMD, now); }
        break;
    case W_CMD:
        send_cmd(w);
        go(w, W_CMD_WAIT, now);
        break;
    case W_CMD_WAIT:
        if (g_ev & EV_OK) {
            if (++w->cmd >= N_CMDS) { w->ready = 1; go(w, W_IDLE, now); }
            else go(w, W_CMD, now);
        } else if ((g_ev & EV_ERR) || dt >= (w->cmd == 2u ? T_CWSAP_MS : T_CMD_MS)) {
            w->n_reset++; w->n_err++;
            reset_module(w, now);
        }
        break;
    case W_IDLE:
        if (w->qt != w->qh) {
            build_resp(w, &w->q[w->qt]);
            w->qt = (uint8_t)((w->qt + 1u) & 3u);
            go(w, W_TX_CMD, now);
        } else if (w->raw_id >= 0) {
            uint32_t av = w->io->raw_avail();
            if (av >= RAW_MIN || (av && now - w->t_raw >= RAW_MAX_MS)) {
                w->body_len = w->io->raw_take(w->jbuf, sizeof(w->jbuf));
                w->body = (const uint8_t *)w->jbuf;
                w->hdr_len = 0;
                w->off = 0;
                w->cur_id = (uint8_t)w->raw_id;
                w->raw_tx = 1;
                w->t_raw = now;
                w->n_raw_byte += w->body_len;
                go(w, W_TX_CMD, now);
            }
        }
        break;
    case W_TX_CMD: {
        uint32_t total = w->hdr_len + w->body_len, k;
        char cmd[32];
        w->chunk = total - w->off > CHUNK_MAX ? CHUNK_MAX : total - w->off;
        w->sent = 0;
        k = cat(cmd, 0, "AT+CIPSEND=");
        k += u2s(cmd + k, w->cur_id);
        k = cat(cmd, k, ",");
        k += u2s(cmd + k, w->chunk);
        k = cat(cmd, k, "\r\n");
        (void)w->io->write(cmd, k);
        go(w, W_TX_PROMPT, now);
        break;
    }
    case W_TX_PROMPT:
        if (g_ev & EV_PROMPT) go(w, W_TX_DATA, now);
        else if ((g_ev & EV_ERR) || dt >= T_TX_MS) {
            w->n_err++;
            if (w->raw_tx) raw_stop(w);
            go(w, W_CLOSE, now);
        }
        break;
    case W_TX_DATA: {
        uint8_t b[64];
        while (w->sent < w->chunk) {
            uint32_t n = w->chunk - w->sent, i;
            if (n > sizeof(b)) n = sizeof(b);
            for (i = 0; i < n; i++) b[i] = at_resp(w, w->off + w->sent + i);
            uint32_t a = w->io->write(b, n);
            w->sent += a;
            if (a < n) break;                                 /* vòng TX đầy: lần poll sau ghi tiếp */
        }
        if (w->sent == w->chunk) go(w, W_TX_WAIT, now);
        else if (dt >= T_TX_MS) { w->n_err++; reset_module(w, now); }
        break;
    }
    case W_TX_WAIT:
        if (g_ev & EV_SENDOK) {
            w->off += w->chunk;
            go(w, w->off < (uint32_t)w->hdr_len + w->body_len ? W_TX_CMD : (w->raw_tx ? W_IDLE : W_CLOSE), now);
        } else if ((g_ev & EV_ERR) || dt >= T_TX_MS) {
            w->n_err++;
            if (w->raw_tx) raw_stop(w);
            go(w, W_CLOSE, now);
        }
        break;
    case W_CLOSE: {
        char cmd[24];
        uint32_t k = cat(cmd, 0, "AT+CIPCLOSE=");
        k += u2s(cmd + k, w->cur_id);
        k = cat(cmd, k, "\r\n");
        (void)w->io->write(cmd, k);
        go(w, W_CLOSE_WAIT, now);
        break;
    }
    case W_CLOSE_WAIT:
        if ((g_ev & (EV_OK | EV_ERR)) || dt >= T_CMD_MS) go(w, W_IDLE, now);
        break;
    }
}
