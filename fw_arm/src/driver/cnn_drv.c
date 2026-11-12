/* cnn_drv.c — xem cnn_drv.h. */
#include "cnn_drv.h"
#include "gw1ns4c.h"
#include "board.h"
#include "selftest_expect_inc.h"

static volatile uint8_t  g_have;
static volatile cnn_res_t g_res;

void USER_INT0_Handler(void)
{
    uint32_t st = CNN_REGS->STATUS;                     /* đọc thì xóa DONE → irq tắt */
    if (st & CNN_ST_DONE) {
        g_res.hw_cls = (uint8_t)CNN_REGS->CLASS;
        g_res.score = (uint8_t)CNN_REGS->SCORE;
        g_res.logit[0] = CNN_REGS->LOGIT[0];
        g_res.logit[1] = CNN_REGS->LOGIT[1];
        g_res.cycles = CNN_REGS->CYCLES;
        g_res.warm = (uint8_t)CNN_REGS->WARM;
        g_res.valid = (st & CNN_ST_VALID) != 0;
        g_have = 1;
    }
}

void cnn_drv_init(void)
{
    nvic_clear(USER_INT0_IRQn);
    nvic_enable(USER_INT0_IRQn);
}

void cnn_drv_soft_reset(void)
{
    CNN_REGS->CTRL = 2u;
    g_have = 0;
}

int cnn_drv_submit(const int8_t stage[CNN_STAGE_BYTES])
{
    if (CNN_REGS->STATUS & CNN_ST_BUSY) return -1;      /* đọc STATUS cũng xóa cờ cũ */
    for (unsigned i = 0; i < CNN_STAGE_BYTES; i++) CNN_REGS->NEW_DATA = (uint8_t)stage[i];
    CNN_REGS->CTRL = 1u;
    if (CNN_REGS->STATUS & CNN_ST_REJECT) return -1;
    return 0;
}

int cnn_drv_take(cnn_res_t *r)
{
    if (!g_have) return 0;
    __asm volatile("cpsid i");
    r->hw_cls = g_res.hw_cls; r->score = g_res.score; r->warm = g_res.warm; r->valid = g_res.valid;
    r->logit[0] = g_res.logit[0]; r->logit[1] = g_res.logit[1]; r->cycles = g_res.cycles;
    g_have = 0;
    __asm volatile("cpsie i");
    return 1;
}

int cnn_drv_selftest(uint32_t *cycles)
{
    int8_t st[CNN_STAGE_BYTES];
    cnn_res_t r;
    if (CNN_REGS->ID != CNN_INC_ID) return 0;
    cnn_drv_soft_reset();
    for (unsigned s = 0; s < CNN_WARM_STEPS; s++) {
        cnn_selftest_stage(s, st);
        if (cnn_drv_submit(st) != 0) return 0;
        uint32_t t0 = board_cycles();
        while (!cnn_drv_take(&r))
            if (board_cycles() - t0 > CNN_TIMEOUT_US * (SYSCLK_HZ / 1000000u)) return 0;
    }
    *cycles = r.cycles;
    return r.valid && r.hw_cls == SELFTEST_INC_CLASS && r.score == SELFTEST_INC_SCORE &&
           r.logit[0] == selftest_inc_logits[0] && r.logit[1] == selftest_inc_logits[1];
}

int8_t cnn_drv_rom_read(uint32_t addr)
{
    CNN_REGS->WROM_ADDR = addr;
    return (int8_t)CNN_REGS->WROM_DATA;
}
