/* startup.c — bảng vector và Reset_Handler cho Gowin_EMPU GW1NS-4C (bảng ngắt: IPUG931 bảng 3-1). MicroLIB: __main
 * chép RW, xóa ZI rồi gọi main(). Ngăn xếp 1 KB ở vùng ARM_LIB_STACK (EMPTY) của gw1ns4c_8k.sct: __main không xóa vùng này.
 * Reset_Handler tô vùng ngăn xếp chưa dùng bằng STACK_MAU để board_stack_dung() đo mức dùng thật (kế hoạch H1b). */
#include <stdint.h>

extern void __main(void);
void Reset_Handler(void);
void Default_Handler(void) { for (;;) { } }

#define WEAK_ALIAS __attribute__((weak, alias("Default_Handler")))
void NMI_Handler(void) WEAK_ALIAS;
void HardFault_Handler(void) WEAK_ALIAS;
void MemManage_Handler(void) WEAK_ALIAS;
void BusFault_Handler(void) WEAK_ALIAS;
void UsageFault_Handler(void) WEAK_ALIAS;
void SVC_Handler(void) WEAK_ALIAS;
void DebugMon_Handler(void) WEAK_ALIAS;
void PendSV_Handler(void) WEAK_ALIAS;
void SysTick_Handler(void) WEAK_ALIAS;
void UART0_Handler(void) WEAK_ALIAS;
void USER_INT0_Handler(void) WEAK_ALIAS;
void UART1_Handler(void) WEAK_ALIAS;
void USER_INT1_Handler(void) WEAK_ALIAS;
void USER_INT2_Handler(void) WEAK_ALIAS;
void RTC_Handler(void) WEAK_ALIAS;
void PORT0_COMB_Handler(void) WEAK_ALIAS;
void USER_INT3_Handler(void) WEAK_ALIAS;
void TIMER0_Handler(void) WEAK_ALIAS;
void TIMER1_Handler(void) WEAK_ALIAS;
void USER_INT4_Handler(void) WEAK_ALIAS;
void UARTOVF_Handler(void) WEAK_ALIAS;
void USER_INT5_Handler(void) WEAK_ALIAS;
void PORT0_0_Handler(void) WEAK_ALIAS;
void PORT0_1_Handler(void) WEAK_ALIAS;
void PORT0_2_Handler(void) WEAK_ALIAS;
void PORT0_3_Handler(void) WEAK_ALIAS;

extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Base[];
extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Limit[];
/* MicroLIB (__main) cần ký hiệu __initial_sp = đỉnh ngăn xếp */
__asm(".global __initial_sp\n.type __initial_sp, %object\n.size __initial_sp, 0\n"
      "__initial_sp = Image$$ARM_LIB_STACK$$ZI$$Limit\n");
#define STACK_MAU 0x5AA5C33Cu

typedef void (*vec_t)(void);
__attribute__((section("RESET"), used))
const vec_t __Vectors[16 + 20] = {
    (vec_t)(uintptr_t)Image$$ARM_LIB_STACK$$ZI$$Limit,
    Reset_Handler, NMI_Handler, HardFault_Handler, MemManage_Handler, BusFault_Handler, UsageFault_Handler,
    0, 0, 0, 0, SVC_Handler, DebugMon_Handler, 0, PendSV_Handler, SysTick_Handler,
    UART0_Handler,          /* 0 */
    USER_INT0_Handler,      /* 1: DONE của bộ tăng tốc */
    UART1_Handler,          /* 2 */
    USER_INT1_Handler,      /* 3 */
    USER_INT2_Handler,      /* 4 */
    RTC_Handler,            /* 5 */
    PORT0_COMB_Handler,     /* 6 */
    USER_INT3_Handler,      /* 7 */
    TIMER0_Handler,         /* 8 */
    TIMER1_Handler,         /* 9 */
    0,                      /* 10 */
    USER_INT4_Handler,      /* 11 */
    UARTOVF_Handler,        /* 12 */
    0,                      /* 13 */
    USER_INT5_Handler,      /* 14 */
    0,                      /* 15 */
    PORT0_0_Handler,        /* 16 */
    PORT0_1_Handler,        /* 17 */
    PORT0_2_Handler,        /* 18: imu_int */
    PORT0_3_Handler,        /* 19 */
};

void Reset_Handler(void)
{
    uint32_t sp, *p = Image$$ARM_LIB_STACK$$ZI$$Base;
    __asm volatile("mov %0, sp" : "=r"(sp));
    while ((uint32_t)(uintptr_t)p < sp - 64u) *p++ = STACK_MAU;    /* chừa 64 B dưới SP hiện tại */
    __main();
}

/* số byte ngăn xếp đã từng dùng: từ đỉnh xuống ô đầu tiên không còn mẫu */
uint32_t board_stack_dung(void)
{
    const uint32_t *p = Image$$ARM_LIB_STACK$$ZI$$Base;
    while (p < Image$$ARM_LIB_STACK$$ZI$$Limit && *p == STACK_MAU) p++;
    return (uint32_t)((uintptr_t)Image$$ARM_LIB_STACK$$ZI$$Limit - (uintptr_t)p);
}
