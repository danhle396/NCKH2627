/* cnn_core.c — xem cnn_core.h. Số học A.5: tích lũy INT32, ≤ 0 → 0, ngược lại min(acc >> n, 127); `>>` số học. */
#include "cnn_core.h"
#include "cnn_rom_inc.h"        /* CNN_L*_BASE, CNN_DENSE_BASE, CNN_BIAS_BASE, CNN_SH*, CNN_SCORE_SHIFT (gen/) */

static int8_t win_at(const int8_t *s1, uint16_t n1, const int8_t *s2, unsigned i)
{
    return i < n1 ? s1[i] : s2[i - n1];
}

void cnn_stage_from_window(const int8_t *s1, uint16_t n1, const int8_t *s2, uint16_t n2,
                           int8_t out[CNN_STAGE_BYTES])
{
    (void)n2;
    for (unsigned i = 0; i < 4u * PRE_CH; i++) out[i] = win_at(s1, n1, s2, i);
    for (unsigned i = 0; i < 20u * PRE_CH; i++) out[4u * PRE_CH + i] = win_at(s1, n1, s2, 108u * PRE_CH + i);
}

void cnn_selftest_stage(unsigned step, int8_t out[CNN_STAGE_BYTES])
{
    uint32_t s = 0xACE1u;
    unsigned base = step * PRE_HOP * PRE_CH, end = base + PRE_BYTES;
    for (unsigned i = 0; i < end; i++) {
        uint32_t lsb = s & 1u;
        s >>= 1;
        if (lsb) s ^= 0xB400u;
        if (i >= base) {
            unsigned k = i - base;                      /* chỉ số trong cửa sổ */
            if (k < 4u * PRE_CH) out[k] = (int8_t)(s & 0xFFu);
            else if (k >= 108u * PRE_CH) out[k - 104u * PRE_CH] = (int8_t)(s & 0xFFu);
        }
    }
}

uint8_t cnn_map_class(uint8_t hw_cls)
{
    return hw_cls == CNN_CLS_FALL ? (uint8_t)CLS_FALL : (uint8_t)CLS_NORMAL;
}

/* ---------------- CNN phần mềm ---------------- */
static int32_t bias_at(const int8_t *rom, unsigned b)
{
    const uint8_t *p = (const uint8_t *)rom + CNN_BIAS_BASE + 4u * b;
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}

static uint8_t requant(int32_t v, unsigned n)
{
    if (v <= 0) return 0;
    v >>= n;
    return (uint8_t)(v > 127 ? 127 : v);
}

/* một lớp Conv k = 5, đệm 2, ReLU, MaxPool 2: src (L, IC) → dst (L/2, OC); src lớp 1 là cửa sổ ring */
static void conv_layer(const int8_t *rom, unsigned wbase, unsigned bbase, unsigned n,
                       const int8_t *s1, uint16_t n1, const int8_t *s2, const uint8_t *src,
                       unsigned L, unsigned IC, unsigned OC, uint8_t *dst)
{
    for (unsigned j = 0; j < L / 2u; j++) {
        for (unsigned g = 0; g < OC; g++) {
            const int8_t *w0 = rom + wbase + g * 5u * IC;
            int32_t b = bias_at(rom, bbase + g), best = 0;
            for (unsigned q = 0; q < 2u; q++) {
                int t = (int)(2u * j + q);
                int32_t acc = b;
                const int8_t *w = w0;
                for (int k = 0; k < 5; k++, w += IC) {
                    int pos = t - 2 + k;
                    if (pos < 0 || pos >= (int)L) continue;
                    if (src) {
                        const uint8_t *x = src + (unsigned)pos * IC;
                        for (unsigned ic = 0; ic < IC; ic++) acc += (int32_t)w[ic] * (int32_t)x[ic];
                    } else {
                        unsigned i0 = (unsigned)pos * IC;
                        for (unsigned ic = 0; ic < IC; ic++) acc += (int32_t)w[ic] * (int32_t)win_at(s1, n1, s2, i0 + ic);
                    }
                }
                if (q == 0u || acc > best) best = acc;
            }
            dst[j * OC + g] = requant(best, n);
        }
    }
}

static uint8_t buf_a[64 * 16];  /* L1 (64 × 16), sau đó L3 (16 × 32) */
static uint8_t buf_b[32 * 16];  /* L2 */

uint8_t cnn_sw_forward(const int8_t *rom, const int8_t *s1, uint16_t n1, const int8_t *s2, uint16_t n2,
                       int32_t logit[2], uint8_t *score)
{
    (void)n2;
    conv_layer(rom, CNN_L1_BASE, 0u,  CNN_SH1, s1, n1, s2, 0,     128u, 6u,  16u, buf_a);
    conv_layer(rom, CNN_L2_BASE, 16u, CNN_SH2, 0, 0, 0,    buf_a, 64u,  16u, 16u, buf_b);
    conv_layer(rom, CNN_L3_BASE, 32u, CNN_SH3, 0, 0, 0,    buf_b, 32u,  16u, 32u, buf_a);
    for (unsigned g = 0; g < 2u; g++) {
        const int8_t *w = rom + CNN_DENSE_BASE + g * 512u;
        int32_t acc = bias_at(rom, 64u + g);
        for (unsigned i = 0; i < 512u; i++) acc += (int32_t)w[i] * (int32_t)buf_a[i];   /* flatten: t·32 + c */
        logit[g] = acc;
    }
    uint8_t cls = logit[1] > logit[0] ? 1u : 0u;                  /* bằng nhau → chỉ số nhỏ */
    int64_t m = (int64_t)logit[cls] - (int64_t)logit[cls ^ 1u];
    m >>= CNN_SCORE_SHIFT;
    *score = (uint8_t)(m > 127 ? 127 : m);
    return cls;
}
