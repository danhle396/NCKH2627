/* uart.h — UART0 (ESP-01S, lệnh AT) và UART1 (USB–UART gỡ lỗi): vòng đệm, ngắt RX/TX, không chặn (kế hoạch 7.3).
 * Thanh ghi: IPUG931 bảng 4-1. BAUDDIV = 27 MHz / baud. */
#ifndef UART_H
#define UART_H
#include <stdint.h>

#define UART_WIFI   0
#define UART_DBG    1

void     uart_init(int u, uint32_t baud);
uint32_t uart_write(int u, const void *p, uint32_t n);   /* trả về số byte nhận vào vòng TX (không chặn) */
uint32_t uart_tx_free(int u);
int      uart_getc(int u);                               /* −1 nếu rỗng */
void     uart_puts(int u, const char *s);
uint32_t uart_rx_overrun(int u);

#endif
