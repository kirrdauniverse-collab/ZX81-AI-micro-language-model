/* test_scratch.c - screen-RAM scratch tier: 6-bit codec, planning and
 * the store/restore cycle over a fake HAL (fake = a plain array with
 * the same cells/bits contract the real ports implement). */
#include <stdio.h>
#include <string.h>
#include "rt_scratch.h"

static int fails;

static void check(int cond, const char *what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        fails++;
    }
}

/* ---- fake HAL ---- */

static rt_u8 hal_cells[2048];
static long hal_n;
static int hal_bits;

long hal_scratch_cells(void)          { return hal_n; }
int  hal_scratch_bits(void)           { return hal_bits; }
rt_u8 hal_scratch_get(long i)         { return hal_cells[i]; }
void hal_scratch_put(long i, rt_u8 v)
{
    /* the ZX81 contract: only glyph codes may land in a 6-bit cell */
    if (hal_bits == 6 && v > 0x3F) {
        printf("FAIL: non-glyph cell 0x%02x\n", v);
        fails++;
        return;
    }
    hal_cells[i] = v;
}

/* deterministic xorshift for the round-trip data */
static rt_u32 rnd_state = 0x1234ABCD;
static rt_u8 rnd_byte(void)
{
    rnd_state ^= rnd_state << 13;
    rnd_state ^= rnd_state >> 17;
    rnd_state ^= rnd_state << 5;
    return (rt_u8)(rnd_state >> 16);
}

static void fill_fact(rt_mem_rec *r, const char *key, const char *val)
{
    memset(r, 0, sizeof *r);
    strncpy(r->key, key, RT_MEM_KEY - 1);
    strncpy(r->val, val, RT_MEM_VAL - 1);
}

static void fill_entry(rt_jentry *e, int dy, const char *text)
{
    memset(e, 0, sizeof *e);
    e->yoff = 26; e->mo = 11; e->d = dy; e->wd = 3;
    strncpy(e->text, text, RT_JOURNAL_TEXT - 1);
}

static void test_pack6(void)
{
    rt_u8 data[200], cells[260], back[200];
    long n, nc, nb;
    int i, all_glyph = 1;

    for (i = 0; i < 192; i++)
        data[i] = rnd_byte();
    n = 192;
    nc = rt_scratch_pack6(data, n, cells, sizeof cells);
    check(nc == (n * 8 + 5) / 6, "pack6 cell count");
    for (i = 0; i < nc; i++)
        if (cells[i] > 0x3F)
            all_glyph = 0;
    check(all_glyph, "pack6 cells are glyph codes only");
    nb = rt_scratch_unpack6(cells, nc, back, sizeof back);
    check(nb == n, "unpack6 byte count");
    check(memcmp(data, back, (size_t)n) == 0, "pack6 round-trip");

    /* tails: 1..5 bytes must survive too */
    for (n = 1; n <= 5; n++) {
        nc = rt_scratch_pack6(data, n, cells, sizeof cells);
        nb = rt_scratch_unpack6(cells, nc, back, sizeof back);
        check(nb == n && memcmp(data, back, (size_t)n) == 0,
              "pack6 tail round-trip");
    }
    /* capacity error path */
    check(rt_scratch_pack6(data, 192, cells, 143) < 0, "pack6 overflow");
}

static void test_plan(void)
{
    int kf, kj;

    rt_scratch_plan(192, 6, 8, &kf, &kj); /* ZX81: journal only */
    check(kf == 0 && kj == 2, "plan zx81 192B");

    rt_scratch_plan(1016, 12, 24, &kf, &kj); /* C64: proportional */
    check(kf == 3 && kj == 8, "plan c64 1016B");

    rt_scratch_plan(2048, 16, 24, &kf, &kj); /* Spectrum: split */
    check(kf == 9 && kj == 14, "plan spec 2048B");

    rt_scratch_plan(2048, 8, 8, &kf, &kj); /* everything fits */
    check(kf == 8 && kj == 8, "plan all-fit");

    rt_scratch_plan(2048, 16, 7, &kf, &kj);
    check(kf == 16 && kj == 7, "plan spec full state");

    rt_scratch_plan(8, 16, 24, &kf, &kj);
    check(kf == 0 && kj == 0, "plan tiny -> nothing");
}

static void test_cycle_6bit(void)
{
    rt_memory mem, back_mem;
    rt_journal j, back_j;
    long n;
    int i;

    memset(&mem, 0, sizeof mem);
    memset(&j, 0, sizeof j);
    mem.n = 6;
    for (i = 0; i < 6; i++) {
        char key[16], val[16];
        sprintf(key, "fact %d", i);
        sprintf(val, "value %d", i);
        fill_fact(&mem.rec[i], key, val);
    }
    j.n = 8;
    for (i = 0; i < 8; i++) {
        char text[32];
        sprintf(text, "note on day %d", i);
        fill_entry(&j.e[i], 10 + i, text);
    }

    hal_n = 256;
    hal_bits = 6;
    memset(hal_cells, 0, sizeof hal_cells);
    n = rt_scratch_store(&mem, &j);
    check(n == RT_SCRATCH_HDR + 2 * RT_SCRATCH_REC_J,
          "zx81 store: 2 journal entries only");

    memset(&back_mem, 0, sizeof back_mem);
    memset(&back_j, 0, sizeof back_j);
    check(rt_scratch_restore(&back_mem, &back_j) == 0, "zx81 restore ok");
    check(back_mem.n == 0, "zx81: facts did not fit, not restored");
    check(back_j.n == 2, "zx81: 2 journal entries back");
    check(strcmp(back_j.e[1].text, "note on day 7") == 0,
          "newest entry is last (chronological)");
    check(strcmp(back_j.e[0].text, "note on day 6") == 0,
          "second newest before it");

    /* entries already present -> restore touches nothing */
    check(rt_scratch_restore(&back_mem, &back_j) == 0, "second restore");
    check(back_j.n == 2, "journal not overwritten");
}

static void test_cycle_8bit(void)
{
    rt_memory mem, back_mem;
    rt_journal j, back_j;
    long n;
    int i;

    memset(&mem, 0, sizeof mem);
    memset(&j, 0, sizeof j);
    mem.n = 16;
    for (i = 0; i < 16; i++) {
        char key[16];
        sprintf(key, "fact %d", i);
        fill_fact(&mem.rec[i], key, "value");
    }
    j.n = 7;
    for (i = 0; i < 7; i++) {
        char text[32];
        sprintf(text, "entry %d", i);
        fill_entry(&j.e[i], 20 + i, text);
    }

    hal_n = 2048;
    hal_bits = 8;
    memset(hal_cells, 0, sizeof hal_cells);
    n = rt_scratch_store(&mem, &j);
    check(n == RT_SCRATCH_HDR + 16 * RT_SCRATCH_REC_FACT
          + 7 * RT_SCRATCH_REC_J, "spec store: everything fits");

    memset(&back_mem, 0, sizeof back_mem);
    memset(&back_j, 0, sizeof back_j);
    check(rt_scratch_restore(&back_mem, &back_j) == 0, "spec restore ok");
    check(back_mem.n == 16 && back_j.n == 7, "spec: all state back");
    check(strcmp(back_mem.rec[0].key, "fact 0") == 0, "facts in order");
    check(strcmp(back_j.e[6].text, "entry 6") == 0, "journal in order");

    /* corrupted cell -> checksum catches it */
    hal_cells[100] ^= 0x55;
    memset(&back_mem, 0, sizeof back_mem);
    memset(&back_j, 0, sizeof back_j);
    check(rt_scratch_restore(&back_mem, &back_j) < 0,
          "corrupt screen detected");
    check(back_mem.n == 0 && back_j.n == 0, "no partial restore");

    /* wiped screen (no magic) -> clean refusal */
    memset(hal_cells, 0, sizeof hal_cells);
    check(rt_scratch_restore(&back_mem, &back_j) < 0, "blank screen");
}

int main(void)
{
    test_pack6();
    test_plan();
    test_cycle_6bit();
    test_cycle_8bit();
    if (fails == 0)
        printf("== scratch tests passed ==\n");
    else
        printf("== %d scratch failures ==\n", fails);
    return fails ? 1 : 0;
}
