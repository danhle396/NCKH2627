/*
 * cnn_core.h — phần C thuần quanh bộ tăng tốc (kế hoạch O1, O3, O4, O7) và CNN phần mềm đối chứng (mục 10 bước 8).
 * Không gọi phần cứng: chạy y nguyên trên M3 và trong bộ test giả lập (fw_arm/test/m3_test.py).
 *
 *   cnn_stage_from_window()  cửa sổ 128 × 6 (ring của preprocess.c) → 144 byte NEW_DATA: x[0..3] rồi x[108..127]
 *   cnn_selftest_stage()     144 byte của bước tự kiểm n (0…7): luồng LFSR Galois 16 bit, đa thức 0xB400, mầm 0xACE1,
 *                            byte i của luồng = mẫu·6 + kênh, cửa sổ n bắt đầu ở mẫu 16n (tools/export_inc.py lfsr_stream)
 *   cnn_map_class()          lớp của bộ tăng tốc 2 lớp → mã của Safety FSM (1 → CLS_FALL = 3). Quên bước này thì
 *                            không bao giờ có phiếu ngã (kế hoạch O3)
 *   cnn_sw_forward()         N2 trên cả cửa sổ, chỉ số nguyên (A.5), đọc trọng số theo bố cục ROM của bộ tăng tốc
 *                            (gen/cnn_rom_inc.h hoặc qua WROM_ADDR/WROM_DATA, O7) — khớp bit golden E2 hai lớp
 */
#ifndef CNN_CORE_H
#define CNN_CORE_H
#include <stdint.h>
#include "fw_config.h"

typedef struct {
    uint8_t  hw_cls;            /* lớp của bộ tăng tốc (0 NORMAL, 1 FALL) */
    uint8_t  score;
    uint8_t  warm;
    uint8_t  valid;             /* warm = 8 */
    int32_t  logit[2];
    uint32_t cycles;            /* chu kỳ của bước (CYCLES của FPGA, hoặc DWT của M3 với CNN phần mềm) */
} cnn_res_t;

void    cnn_stage_from_window(const int8_t *s1, uint16_t n1, const int8_t *s2, uint16_t n2,
                              int8_t out[CNN_STAGE_BYTES]);
void    cnn_selftest_stage(unsigned step, int8_t out[CNN_STAGE_BYTES]);
uint8_t cnn_map_class(uint8_t hw_cls);

/* rom: ảnh ROM byte (trọng số 1 làn, rồi bias INT32 little-endian); x: cửa sổ 768 byte, i = mẫu·6 + kênh
 * (ring: s1 rồi s2). Trả về lớp (0/1); logit[2], *score ra. */
uint8_t cnn_sw_forward(const int8_t *rom, const int8_t *s1, uint16_t n1, const int8_t *s2, uint16_t n2,
                       int32_t logit[2], uint8_t *score);

#endif
