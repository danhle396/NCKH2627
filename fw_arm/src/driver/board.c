/* board.c — xem board.h. Thanh ghi: IPUG931 bảng 6-1 (watchdog), 7-1 (GPIO); SysTick, DWT: ARMv7-M. */
#include "board.h"

static volatile uint32_t g_ms;
static volatile uint32_t g_imu_int;
static uint32_t g_wdt_load;

void SysTick_Handler(void) { g_ms++; }

void PORT0_2_Handler(void)
{
    GPIO0->INTSTATUS = PIN_IMU_INT;          /* ghi 1 xóa */
    g_imu_int++;
}

void board_init(void)
{
    /* ra: còi, LED, nguồn cảm biến TẮT, ESP-01S chạy; I2C thả (cho phép xuất = 0, dữ liệu xuất = 0) */
    GPIO0->DATAOUT = 0;
    GPIO0->OUTENCLR = 0xFFFFu;
    GPIO0->OUTENSET = PIN_OUT_MASK;
    /* imu_int: ngắt cạnh lên */
    GPIO0->INTTYPESET = PIN_IMU_INT;
    GPIO0->INTPOLSET = PIN_IMU_INT;
    GPIO0->INTSTATUS = PIN_IMU_INT;
    GPIO0->INTENSET = PIN_IMU_INT;
    nvic_enable(PORT0_2_IRQn);
    /* SysTick 1 ms ở 27 MHz, DWT đếm chu kỳ */
    SYST_RVR = SYSCLK_HZ / 1000u - 1u;
    SYST_CVR = 0;
    SYST_CSR = 7u;                           /* clock lõi, ngắt, chạy */
    DEMCR |= 1u << 24;                       /* TRCENA */
    DWT_CYCCNT = 0;
    DWT_CTRL |= 1u;
}

uint32_t board_ms(void) { return g_ms; }

void board_delay_us(uint32_t us)
{
    uint32_t t0 = DWT_CYCCNT, n = us * (SYSCLK_HZ / 1000000u);
    while (DWT_CYCCNT - t0 < n) { }
}

void board_set(uint32_t mask, int on)
{
    uint32_t v = GPIO0->DATAOUT;
    GPIO0->DATAOUT = on ? (v | mask) : (v & ~mask);
}

uint32_t board_imu_int_take(void)
{
    uint32_t n;
    __asm volatile("cpsid i");
    n = g_imu_int;
    g_imu_int = 0;
    __asm volatile("cpsie i");
    return n;
}

/* Watchdog kiểu CMSDK: hết lần 1 → ngắt, hết lần 2 → reset; nên LOAD = nửa thời gian. Xung WDOGCLK coi như bằng
 * xung hệ thống 27 MHz [CẦN XÁC MINH trên board ở H1: đo thời gian tới khi reset]. */
void board_wdt_start(uint32_t ms)
{
    g_wdt_load = ms * (SYSCLK_HZ / 1000u) / 2u;
    WDOG->LOCK = WDOG_UNLOCK_KEY;
    WDOG->LOAD = g_wdt_load;
    WDOG->CTRL = 3u;                         /* ngắt + reset */
    WDOG->LOCK = 0;
}

void board_wdt_kick(void)
{
    WDOG->LOCK = WDOG_UNLOCK_KEY;
    WDOG->INTCLR = 1u;                       /* xóa ngắt, nạp lại bộ đếm */
    WDOG->LOCK = 0;
}
