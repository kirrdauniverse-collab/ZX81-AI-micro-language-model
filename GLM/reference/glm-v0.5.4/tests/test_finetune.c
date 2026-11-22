/* test_finetune.c - on-device learning: after fine-tuning on the taught
 * fact text (3x, lr 14 - same as /finetune), the model's own logit for
 * the taught continuation must rise, and the model must not collapse.
 * usage: test_finetune <model.bin>  (built with RT_ENABLE_FINETUNE=1) */
#include <stdio.h>
#include <string.h>
#include "rt_charset.h"
#include "rt_model.h"
#include "rt_engine.h"
#include "rt_finetune.h"

static rt_u8 buf[512 * 1024];
static rt_i32 logits[RT_MAX_VOCAB];

static int probe(const rt_model *m, const char *prefix, int target_idx)
{
    rt_u8 win[RT_MAX_CTX];
    int i, n = 0;
    rt_u8 hist[64];
    const char *s = prefix;
    while (s[n] && n < 63) {
        hist[n] = rt_map_char(s[n]);
        n++;
    }
    for (i = 0; i < (int)m->hdr.ctx; i++) {
        int pos = n - (int)m->hdr.ctx + i;
        win[i] = (pos < 0) ? (rt_u8)1 : hist[pos];
    }
    rt_forward(m, win, logits);
    return (int)logits[target_idx];
}

int main(int argc, char **argv)
{
    rt_model m;
    FILE *f;
    long n;
    int before, after, fail = 0;
    int idx_i, idx_space, sanity;

    if (argc != 2) {
        printf("usage: test_finetune model.bin\n");
        return 2;
    }
    rt_charset_init();
    f = fopen(argv[1], "rb");
    if (!f)
        return 2;
    n = (long)fread(buf, 1, sizeof buf, f);
    fclose(f);
    if (rt_model_load(&m, buf, n) != 0) {
        printf("FAIL: model load\n");
        return 1;
    }
    idx_i = RT_CHAR_TO_IDX[(unsigned char)'i'];
    idx_space = RT_CHAR_TO_IDX[(unsigned char)' '];

    before = probe(&m, "my favorite computer is the am", idx_i);
    rt_finetune_text(&m, "my favorite computer is the amiga.\n"
                    "my favorite computer is the amiga.\n"
                    "my favorite computer is the amiga.\n", 3, 14);
    after = probe(&m, "my favorite computer is the am", idx_i);
    sanity = probe(&m, "hello! how are you to", idx_space);

    printf("logit('i' after 'the am'): %d -> %d\n", before, after);
    if (after <= before) {
        printf("FAIL: fine-tuning did not raise the target logit\n");
        fail = 1;
    }
    if (sanity < -30000 || sanity > 30000) {
        printf("FAIL: model looks broken after fine-tuning\n");
        fail = 1;
    }
    printf("test_finetune: %s\n", fail ? "FAIL" : "PASS");
    return fail ? 1 : 0;
}
