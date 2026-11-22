/* test_mem.c - teach / match / answer / pack round-trip. */
#include <stdio.h>
#include <string.h>
#include "rt_charset.h"
#include "rt_memory.h"

static int fails;

static void check(int cond, const char *what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        fails++;
    }
}

int main(void)
{
    rt_memory mem, mem2;
    rt_u8 blob[8 + RT_MEM_MAX * (RT_MEM_KEY + RT_MEM_VAL)];
    long n;
    char ans[128];
    int hit;

    rt_charset_init();
    rt_mem_init(&mem);

    check(rt_mem_teach(&mem, "my name", "ziggy") == 0, "teach name");
    check(rt_mem_teach(&mem, "favorite computer", "amiga 500") == 0, "teach comp");
    check(mem.n == 2, "two records");
    check(rt_mem_teach(&mem, "MY NAME", "fred") == 0, "re-teach folds case");
    check(mem.n == 2, "re-teach overwrites, not adds");
    check(strcmp(mem.rec[0].val, "fred") == 0, "value updated");

    hit = rt_mem_match(&mem, "what is my name?");
    check(hit == 0, "match on name question");
    hit = rt_mem_match(&mem, "which is your favorite computer?");
    check(hit == 1, "match on computer question");
    hit = rt_mem_match(&mem, "tell me a joke");
    check(hit == -1, "no match on unknown topic");

    rt_mem_answer(&mem.rec[0], ans, sizeof ans);
    check(strcmp(ans, "your name is fred.") == 0, "my->your answer");
    rt_mem_answer(&mem.rec[1], ans, sizeof ans);
    check(strcmp(ans, "my favorite computer is amiga 500.") == 0,
          "plain answer");

    n = rt_mem_pack(&mem, blob, sizeof blob);
    check(n > 0, "pack");
    check(rt_mem_unpack(&mem2, blob, n) == n, "unpack");
    check(mem2.n == 2, "record count survives");
    check(strcmp(mem2.rec[1].key, "favorite computer") == 0, "key survives");
    check(strcmp(mem2.rec[1].val, "amiga 500") == 0, "val survives");

    rt_mem_forget(&mem2, 0);
    check(mem2.n == 1, "forget removes");
    check(strcmp(mem2.rec[0].key, "favorite computer") == 0,
          "forget shifts");

    printf("test_mem: %s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
