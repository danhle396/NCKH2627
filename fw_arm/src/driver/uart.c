/* uart.c — xem uart.h. TX: nạp byte đầu khi rảnh, ngắt TX nạp tiếp; RX: ngắt RX đưa vào vòng. */
#include <string.h>
#include "uart.h"
#include "gw1ns4c.h"

#define TXN 512u
#define RXN 256u
typedef struct {
    UART_TypeDef *r;
    volatile uint16_t th, tt, rh, rt;
    volatile uint32_t ovr;
    uint8_t tx[TXN], rx[RXN];
} port_t;

static port_t g_p[2];                   /* ZI: vòng đệm không chiếm flash; r gán trong uart_init */

static void tx_pump(port_t *p)
{
    while (p->tt != p->th && !(p->r->STATE & UART_STATE_TXFULL)) {
        p->r->DATA = p->tx[p->tt];
        p->tt = (uint16_t)((p->tt + 1u) % TXN);
    }
}

static void isr(port_t *p)
{
    uint32_t st = p->r->INTSTATUS;
    p->r->INTSTATUS = st;                                   /* ghi 1 xóa */
    while (p->r->STATE & UART_STATE_RXFULL) {
        uint8_t c = (uint8_t)p->r->DATA;
        uint16_t n = (uint16_t)((p->rh + 1u) % RXN);
        if (n == p->rt) p->ovr++;
        else { p->rx[p->rh] = c; p->rh = n; }
    }
    if (p->r->STATE & UART_STATE_RXOVR) { p->r->STATE = UART_STATE_RXOVR; p->ovr++; }
    tx_pump(p);
}

void UART0_Handler(void) { isr(&g_p[0]); }
void UART1_Handler(void) { isr(&g_p[1]); }

void uart_init(int u, uint32_t baud)
{
    port_t *p = &g_p[u];
    p->r = u == 0 ? UART0 : UART1;
    p->th = p->tt = p->rh = p->rt = 0;
    p->r->BAUDDIV = SYSCLK_HZ / baud;
    p->r->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN | UART_CTRL_TXIE | UART_CTRL_RXIE;
    nvic_enable(u == 0 ? UART0_IRQn : UART1_IRQn);
}

uint32_t uart_tx_free(int u)
{
    port_t *p = &g_p[u];
    return (uint32_t)((p->tt + TXN - p->th - 1u) % TXN);
}

uint32_t uart_write(int u, const void *buf, uint32_t n)
{
    port_t *p = &g_p[u];
    const uint8_t *b = (const uint8_t *)buf;
    uint32_t k = 0;
    while (k < n) {
        uint16_t nx = (uint16_t)((p->th + 1u) % TXN);
        if (nx == p->tt) break;
        p->tx[p->th] = b[k++];
        p->th = nx;
    }
    __asm volatile("cpsid i");
    tx_pump(p);
    __asm volatile("cpsie i");
    return k;
}

void uart_puts(int u, const char *s) { (void)uart_write(u, s, (uint32_t)strlen(s)); }

int uart_getc(int u)
{
    port_t *p = &g_p[u];
    if (p->rh == p->rt) return -1;
    int c = p->rx[p->rt];
    p->rt = (uint16_t)((p->rt + 1u) % RXN);
    return c;
}

uint32_t uart_rx_overrun(int u) { return g_p[u].ovr; }
