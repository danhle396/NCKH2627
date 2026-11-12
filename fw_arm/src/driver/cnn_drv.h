/* cnn_drv.h — driver cổng APB của bộ tăng tốc (rtl/src/cnn_apb.v, APB2 Master 1 = 0x40002400; kế hoạch O4). */
#ifndef CNN_DRV_H
#define CNN_DRV_H
#include <stdint.h>
#include "cnn_core.h"

typedef struct {
    volatile uint32_t ID;          /* 0x00 */
    volatile uint32_t CTRL;        /* 0x04 bit0 START, bit1 SOFT_RST */
    volatile uint32_t STATUS;      /* 0x08 bit0 BUSY, bit1 DONE, bit2 WR_REJECT, bit3 VALID (đọc thì xóa DONE, REJECT) */
    volatile uint32_t CLASS;       /* 0x0C */
    volatile uint32_t SCORE;       /* 0x10 */
    volatile int32_t  LOGIT[4];    /* 0x14 … 0x20 */
    volatile uint32_t CYCLES;      /* 0x24 */
    volatile uint32_t NEW_DATA;    /* 0x28 */
    volatile uint32_t WARM;        /* 0x2C */
    volatile uint32_t WROM_ADDR;   /* 0x30 */
    volatile int32_t  WROM_DATA;   /* 0x34 */
    volatile uint32_t NEW_PTR;     /* 0x38 */
} cnn_regs_t;
#define CNN_REGS ((cnn_regs_t *)0x40002400u)

#define CNN_ST_BUSY    1u
#define CNN_ST_DONE    2u
#define CNN_ST_REJECT  4u
#define CNN_ST_VALID   8u

void    cnn_drv_init(void);                               /* bật USER_INT0 */
void    cnn_drv_soft_reset(void);
int     cnn_drv_submit(const int8_t stage[CNN_STAGE_BYTES]);  /* 0 = đã START; −1 = bận hoặc bị từ chối */
int     cnn_drv_take(cnn_res_t *r);                       /* 1 = có kết quả mới (đọc trong ngắt DONE) */
/* tự kiểm (O4): ID, SOFT_RST, 8 bước LFSR, so CLASS/SCORE/LOGIT với gen/selftest_expect_inc.h. 1 = đạt.
 * *cycles = CYCLES của bước cuối. */
int     cnn_drv_selftest(uint32_t *cycles);
int8_t  cnn_drv_rom_read(uint32_t addr);                  /* O7: 1 byte ROM trọng số (chỉ khi rảnh) */

#endif
