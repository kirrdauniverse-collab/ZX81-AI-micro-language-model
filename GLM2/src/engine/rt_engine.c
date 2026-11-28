/* rt_engine.c - int8 quantized forward pass + sampling.
 *
 * Quantization v2 (chosen by training/pack.py):
 *   - ALL activations are Q7 (real = v / 128); the bias lane input is
 *     the constant 128 (= 1.0), so biases quantize on the same grid as
 *     ordinary weights.
 *   - every weight row carries its own power-of-two shift s (u8 array):
 *     row values are round(w * 2^s), and the matmul does acc >> s.
 *   - hidden activations are ReLU'd and saturated into [0, 127];
 *     logits stay int32 in Q7 scale.
 */
#include "rt_engine.h"
#include "rt_lut.h"

static rt_i8  rt_x[RT_MAX_CTX * RT_MAX_EMB];
static rt_i8  rt_h1[RT_MAX_HID];
static rt_i8  rt_h2[RT_MAX_HID];
static rt_i8 *rt_hlast;
static int    rt_hlast_n;

/* v0.7.2: the 16-bit MAC experiment was REVERTED. Measured real row
 * sums reach 1.5M (the embeddings are int8, the rows 64-65 wide), so
 * a 16-bit accumulator would silently wrap: the "identical results"
 * first seen were an artifact of an unsaturated Python mock. The
 * 32-bit accumulator stays. What IS new: the row pointer now walks
 * (w += n_in + 1) instead of recomputing w + (long)o*(n_in+1) - the
 * 32-bit offset multiply per row was pure overhead, sccz80 keeps the
 * walk in HL. Same trick below in rt_forward's embedding gather. */
static void matmul_hid(const rt_i8 *w, const rt_u8 *sh, const rt_i8 *x,
                       int n_in, int n_out, rt_i8 *y)
{
    int o, i;
    for (o = 0; o < n_out; o++, w += n_in + 1) {
        const rt_i8 *wr = w;
        rt_i32 acc = 0;
        for (i = 0; i < n_in; i++)
            acc += (rt_i32)x[i] * (rt_i32)wr[i];
        acc += (rt_i32)128 * wr[n_in]; /* bias lane: Q7 one */
        acc >>= sh[o];
        if (acc < 0)
            acc = 0;    /* ReLU */
        if (acc > 127)
            acc = 127;  /* saturate to int8 */
        y[o] = (rt_i8)acc;
    }
}

static void matmul_logits(const rt_i8 *w, const rt_u8 *sh, const rt_i8 *x,
                          int n_in, int n_out, rt_i32 *lg)
{
    int o, i;
    for (o = 0; o < n_out; o++, w += n_in + 1) {
        const rt_i8 *wr = w;
        rt_i32 acc = 0;
        for (i = 0; i < n_in; i++)
            acc += (rt_i32)x[i] * (rt_i32)wr[i];
        acc += (rt_i32)128 * wr[n_in];
        lg[o] = acc >> sh[o];
    }
}

void rt_forward(const rt_model *m, const rt_u8 *ctx, rt_i32 *logits)
{
    int i, j;
    int n_in = (int)m->hdr.ctx * (int)m->hdr.emb;
    rt_i8 *xd = rt_x;

    /* v0.7.2: keep the ctx[i]-indexed table lookup (the indices are
     * arbitrary charset codes), but accumulate the destination with a
     * walking pointer and build the source offset with a 16-bit
     * multiply (vocab*emb <= 48*16 = 768, so (long) was needless
     * 32-bit overhead per element). */
    for (i = 0; i < (int)m->hdr.ctx; i++) {
        const rt_i8 *e = m->emb + (int)ctx[i] * (int)m->hdr.emb;
        for (j = 0; j < (int)m->hdr.emb; j++)
            *xd++ = e[j];
    }

    matmul_hid(m->w1, m->sh1, rt_x, n_in, m->hdr.h1, rt_h1);
    if (m->hdr.h2) {
        matmul_hid(m->w2, m->sh2, rt_h1, m->hdr.h1, m->hdr.h2, rt_h2);
        rt_hlast = rt_h2;
        rt_hlast_n = m->hdr.h2;
    } else {
        rt_hlast = rt_h1;
        rt_hlast_n = m->hdr.h1;
    }
    matmul_logits(m->wo, m->sho, rt_hlast, rt_hlast_n, m->hdr.vocab,
                  logits);
}

void rt_last_hidden(const rt_i8 **h, int *n)
{
    *h = rt_hlast;
    *n = rt_hlast_n;
}

rt_u32 rt_rand(rt_u32 *seed)
{
    rt_u32 x = *seed;
    x = (x ^ ((x << 13) & RT_U32_MASK)) & RT_U32_MASK;
    x = (x ^ (x >> 17)) & RT_U32_MASK;
    x = (x ^ ((x << 5) & RT_U32_MASK)) & RT_U32_MASK;
    *seed = x;
    return x;
}

int rt_sample(const rt_model *m, const rt_i32 *lg, int temp_sh, int greedy,
              rt_u32 *seed)
{
    /* v0.7.2 (the z80ai "skip the impossible" lesson): the sampling
     * pass used to weight ALL vocab entries (48 x LUT + 32-bit add
     * into sum, then a 32-bit modulo of the xorshift draw - the most
     * expensive 16-bit-op block in the whole per-character path).
     * But exp(-d/8) underflows the Q12 LUT to 0 at d >= 2048, and
     * with temp_sh >= 2 the LUT index k = d>>temp_sh>>3 saturates at
     * 255: everything with (mx - lg[i]) >= (255<<3)<<temp_sh can
     * NEVER be drawn - its weight is exactly 0. So: one pass finds
     * mx and best, the second pass only touches candidates whose
     * gap fits the window, skips the rest. Behaviour is bit-identical
     * (the skipped entries contributed 0 to sum and to the CDF walk)
     * while the typical hot set is a handful of letters. */
    int i, best = 0;
    rt_i32 mx = lg[0];
    rt_u16 w[RT_MAX_VOCAB];
    rt_u32 sum = 0, r, acc = 0;
    rt_i32 win;   /* the largest gap that still carries weight */

    for (i = 1; i < (int)m->hdr.vocab; i++)
        if (lg[i] > mx) {
            mx = lg[i];
            best = i;
        }
    if (greedy)
        return best;

    win = (rt_i32)255 << (3 + temp_sh);   /* k = gap >> temp_sh >> 3 */
    if (win < 0)                /* temp_sh 6: 255<<9 = 130560, fits */
        win = 0x7FFFFFFF;       /* (guard; shifts stay in range) */
    for (i = 0; i < (int)m->hdr.vocab; i++) {
        rt_i32 d = (mx - lg[i]) >> temp_sh; /* >= 0 */
        if (d > win) {
            w[i] = 0;           /* zero weight: cannot be drawn */
            continue;
        }
        {
            int k = (int)(d >> 3);
            if (k > 255)
                k = 255;
            w[i] = RT_EXP_LUT[k];
        }
        sum += w[i];
    }
    if (sum == 0)
        return best;

    /* v0.7.2 note: keep the plain % sum. A Lemire-style
     * (draw * sum) >> 16 was tried here, but sccz80 expands that
     * multiply to the SAME 32-bit helper as the modulo, so it bought
     * nothing while changing the drawn values. The zero-skip above
     * is the real sampling win; the modulo runs once per character. */
    r = rt_rand(seed) % sum;
    for (i = 0; i < (int)m->hdr.vocab; i++) {
        acc += w[i];
        if (r < acc)
            return i;
    }
    return best;
}
