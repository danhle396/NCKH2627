/* board.h — GPIO, thời gian (SysTick 1 ms, DWT), watchdog, ngắt imu_int của Tang Nano 4K NCKH2627 (chân: app/chan.h). */
#ifndef BOARD_H
#define BOARD_H
#include <stdint.h>
#include "gw1ns4c.h"
#include "chan.h"

void     board_init(void);
uint32_t board_ms(void);                    /* ms từ lúc khởi động (SysTick) */
static inline uint32_t board_cycles(void) { return DWT_CYCCNT; }
void     board_delay_us(uint32_t us);
void     board_set(uint32_t mask, int on);  /* chân ra (PIN_OUT_MASK) */
static inline uint32_t board_in(void) { return GPIO0->DATA; }

/* imu_int (GPIO[2]) cạnh lên → đếm; main đọc và xóa */
uint32_t board_imu_int_take(void);

/* watchdog (kế hoạch 7.5.4): reset nếu không cho ăn trong `ms` */
void     board_wdt_start(uint32_t ms);
void     board_wdt_kick(void);

#endif
