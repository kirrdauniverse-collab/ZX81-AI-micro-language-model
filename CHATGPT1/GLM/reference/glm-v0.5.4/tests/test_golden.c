/* test_golden.c - prove the C engine is bit-identical to the numpy
 * integer simulator that produced golden.txt.
 *
 * usage: test_golden <model.bin> <golden.txt>
 * golden line format: "ctx-ids space separated|argmax|argmax-logit|sampled"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rt_charset.h"
#include "rt_model.h"
#include "rt_engine.h"

static rt_u8 buf[512 * 1024];
static rt_i32 logits[RT_MAX_VOCAB];

int main(int argc, char **argv)
{
    FILE *gm, *gf;
    rt_model m;
    char line[512];
    int n_line = 0, n_ok = 0, n_bad = 0;

    if (argc != 3) {
        printf("usage: test_golden model.bin golden.txt\n");
        return 2;
    }
    rt_charset_init();
    gm = fopen(argv[1], "rb");
    if (!gm)
        return 2;
    { long n = (long)fread(buf, 1, sizeof buf, gm); fclose(gm);
      if (rt_model_load(&m, buf, n) != 0) {
          printf("FAIL: model load\n"); return 1; } }

    gf = fopen(argv[2], "r");
    if (!gf)
        return 2;
    while (fgets(line, sizeof line, gf)) {
        rt_u8 ctx[RT_MAX_CTX];
        char *bar = strchr(line, '|');
        int want_am, want_logit, want_samp;
        int i = 0, am = 0, got_samp;
        rt_u32 seed = 1234;
        char *p;

        if (!bar)
            continue;
        *bar = '\0';
        sscanf(bar + 1, "%d|%d|%d", &want_am, &want_logit, &want_samp);
        p = line;
        while (*p && i < RT_MAX_CTX) {
            while (*p == ' ')
                p++;
            if (!*p)
                break;
            ctx[i++] = (rt_u8)strtol(p, &p, 10);
        }
        n_line++;
        if (i != (int)m.hdr.ctx) {
            printf("FAIL line %d: ctx len %d != %d\n", n_line, i, m.hdr.ctx);
            n_bad++;
            continue;
        }
        rt_forward(&m, ctx, logits);
        for (am = 0, i = 1; i < (int)m.hdr.vocab; i++)
            if (logits[i] > logits[am])
                am = i;
        got_samp = rt_sample(&m, logits, 2, 0, &seed);
        if (am == want_am && logits[am] == want_logit && got_samp == want_samp)
            n_ok++;
        else {
            n_bad++;
            printf("FAIL line %d: am %d/%d logit %d/%d samp %d/%d\n",
                   n_line, am, want_am, (int)logits[am], want_logit,
                   got_samp, want_samp);
        }
    }
    fclose(gf);
    printf("test_golden: %d/%d ok, %d bad\n", n_ok, n_line, n_bad);
    return n_bad ? 1 : 0;
}
