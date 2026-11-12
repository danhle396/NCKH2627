/*
 * gw1ns4c.h — thanh ghi tối thiểu của Gowin_EMPU (GW1NS-4C) cho firmware NCKH2627.
 * Nguồn: Gowin IPUG931-2.1E "Gowin_EMPU(GW1NS-4C) Software Programming" (15/8/2025), docs/datasheet/tang_nano_4k/IPUG931E_Gowin_EMPU_GW1NS-4C_Software.pdf:
 *   bảng 2-1 (địa chỉ ngoại vi), 3-1 (bảng ngắt), 4-1 (UART), 5-1 (Timer), 6-1 (Watchdog), 7-1 (GPIO).
 * Không dùng SDK của Gowin (chưa có trên máy). Tên trường theo bảng của IPUG931; khi có SDK thật (gw1ns4c.h của Gowin)
 * thì đối chiếu lại [CẦN XÁC MINH trên board ở bước H1].
 */
#ifndef GW1NS4C_H
#define GW1NS4C_H
#include <stdint.h>

#define __IO volatile

/* ---- bảng 2-1 ---- */
#define FLASH_BASE      0x00000000u     /* 32 KB */
#define SRAM_BASE       0x20000000u     /* 2/4/8/16 KB; NCKH2627 dùng 8 KB (4 BSRAM) */
#define TIMER0_BASE     0x40000000u
#define TIMER1_BASE     0x40001000u
#define UART0_BASE      0x40004000u
#define UART1_BASE      0x40005000u
#define RTC_BASE        0x40006000u
#define WDOG_BASE       0x40008000u
#define GPIO0_BASE      0x40010000u
#define SYSCON_BASE     0x4001F000u
#define APB2_M1_BASE    0x40002400u     /* cổng bộ tăng tốc (rtl/src/cnn_apb.v) */

/* ---- bảng 4-1: UART ---- */
typedef struct {
    __IO uint32_t DATA;                 /* 0x00 [7:0] */
    __IO uint32_t STATE;                /* 0x04 [3] RX overrun (ghi 1 xóa) [2] TX overrun [1] RX đầy [0] TX đầy */
    __IO uint32_t CTRL;                 /* 0x08 [5] RXOV IE [4] TXOV IE [3] RX IE [2] TX IE [1] RX EN [0] TX EN */
    __IO uint32_t INTSTATUS;            /* 0x0C ghi 1 xóa: [3] RXOV [2] TXOV [1] RX [0] TX */
    __IO uint32_t BAUDDIV;              /* 0x10 [19:0] ≥ 16 */
} UART_TypeDef;
#define UART_STATE_TXFULL   (1u << 0)
#define UART_STATE_RXFULL   (1u << 1)
#define UART_STATE_TXOVR    (1u << 2)
#define UART_STATE_RXOVR    (1u << 3)
#define UART_CTRL_TXEN      (1u << 0)
#define UART_CTRL_RXEN      (1u << 1)
#define UART_CTRL_TXIE      (1u << 2)
#define UART_CTRL_RXIE      (1u << 3)
#define UART_INT_TX         (1u << 0)
#define UART_INT_RX         (1u << 1)

/* ---- bảng 5-1: Timer ---- */
typedef struct {
    __IO uint32_t CTRL;                 /* [3] IE [2] EXTIN là clock [1] EXTIN là enable [0] chạy */
    __IO uint32_t VALUE;
    __IO uint32_t RELOAD;
    __IO uint32_t INTSTATUS;            /* [0] ghi 1 xóa */
} TIMER_TypeDef;

/* ---- bảng 6-1: Watchdog ---- */
typedef struct {
    __IO uint32_t LOAD;                 /* 0x00 */
    __IO uint32_t VALUE;                /* 0x04 RO */
    __IO uint32_t CTRL;                 /* 0x08 [1] reset [0] ngắt */
    __IO uint32_t INTCLR;               /* 0x0C WO: xóa ngắt, nạp lại bộ đếm */
    __IO uint32_t RIS;                  /* 0x10 */
    __IO uint32_t MIS;                  /* 0x14 */
    uint32_t      rsv[0x2FA];           /* 0x18 … 0xBFC */
    __IO uint32_t LOCK;                 /* 0xC00: ghi 0x1ACCE551 để mở khóa (giá trị khóa của khối CMSDK watchdog,
                                         * IPUG931 chỉ ghi "enable register writes") [CẦN XÁC MINH] */
} WDOG_TypeDef;
#define WDOG_UNLOCK_KEY     0x1ACCE551u

/* ---- bảng 7-1: GPIO (AHB) ---- */
typedef struct {
    __IO uint32_t DATA;                 /* 0x00 đọc chân (qua 2 FF đồng bộ), ghi thanh ghi xuất */
    __IO uint32_t DATAOUT;              /* 0x04 */
    uint32_t      rsv0[2];
    __IO uint32_t OUTENSET;             /* 0x10 */
    __IO uint32_t OUTENCLR;             /* 0x14 */
    __IO uint32_t ALTFUNCSET;           /* 0x18 */
    __IO uint32_t ALTFUNCCLR;           /* 0x1C */
    __IO uint32_t INTENSET;             /* 0x20 */
    __IO uint32_t INTENCLR;             /* 0x24 */
    __IO uint32_t INTTYPESET;           /* 0x28 1 = cạnh */
    __IO uint32_t INTTYPECLR;           /* 0x2C */
    __IO uint32_t INTPOLSET;            /* 0x30 1 = mức cao / cạnh lên */
    __IO uint32_t INTPOLCLR;            /* 0x34 */
    __IO uint32_t INTSTATUS;            /* 0x38 ghi 1 xóa */
} GPIO_TypeDef;

#define UART0   ((UART_TypeDef *)UART0_BASE)
#define UART1   ((UART_TypeDef *)UART1_BASE)
#define TIMER0  ((TIMER_TypeDef *)TIMER0_BASE)
#define TIMER1  ((TIMER_TypeDef *)TIMER1_BASE)
#define WDOG    ((WDOG_TypeDef *)WDOG_BASE)
#define GPIO0   ((GPIO_TypeDef *)GPIO0_BASE)

/* ---- bảng 3-1: số ngắt (IRQn) ---- */
#define UART0_IRQn      0
#define USER_INT0_IRQn  1               /* DONE của bộ tăng tốc (cnn_apb.v irq) */
#define UART1_IRQn      2
#define PORT0_COMB_IRQn 6
#define TIMER0_IRQn     8
#define TIMER1_IRQn     9
#define PORT0_2_IRQn    18              /* GPIO[2] = imu_int */

/* ---- NVIC, SysTick (lõi Cortex-M3, ARMv7-M) ---- */
#define NVIC_ISER       ((__IO uint32_t *)0xE000E100u)
#define NVIC_ICER       ((__IO uint32_t *)0xE000E180u)
#define NVIC_ICPR       ((__IO uint32_t *)0xE000E280u)
#define SYST_CSR        (*(__IO uint32_t *)0xE000E010u)
#define SYST_RVR        (*(__IO uint32_t *)0xE000E014u)
#define SYST_CVR        (*(__IO uint32_t *)0xE000E018u)
#define DWT_CTRL        (*(__IO uint32_t *)0xE0001000u)
#define DWT_CYCCNT      (*(__IO uint32_t *)0xE0001004u)
#define DEMCR           (*(__IO uint32_t *)0xE000EDFCu)

static inline void nvic_enable(int irq)  { NVIC_ISER[0] = 1u << irq; }
static inline void nvic_disable(int irq) { NVIC_ICER[0] = 1u << irq; }
static inline void nvic_clear(int irq)   { NVIC_ICPR[0] = 1u << irq; }

#define SYSCLK_HZ       27000000u       /* thạch anh 27 MHz, không PLL (kế hoạch O5) */

#endif
