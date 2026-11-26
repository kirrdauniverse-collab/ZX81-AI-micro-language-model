/* test_chat.c - deterministic generation with a fixed seed + caps.
 * usage: test_chat <model.bin> */
#include <stdio.h>
#include <string.h>
#include "rt_charset.h"
#include "rt_model.h"
#include "rt_chat.h"

static rt_u8 buf[512 * 1024];

static void collect(char c, void *user)
{
    char *out = (char *)user;
    int n = (int)strlen(out);
    if (n < 200)
        out[n] = c, out[n + 1] = '\0';
}

int main(int argc, char **argv)
{
    rt_model m;
    rt_chat a, b;
    char out1[256] = "", out2[256] = "";
    FILE *f;
    long n;
    int i, fail = 0;

    if (argc != 2) {
        printf("usage: test_chat model.bin\n");
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

    /* same seed + same input -> same reply, twice (newline = turn end,
     * exactly like the app pushes it) */
    rt_chat_init(&a, 424242);
    rt_chat_push(&a, "hello, how are you?");
    rt_chat_push_idx(&a, 0);
    rt_chat_gen(&a, &m, collect, out1);

    rt_chat_init(&b, 424242);
    rt_chat_push(&b, "hello, how are you?");
    rt_chat_push_idx(&b, 0);
    rt_chat_gen(&b, &m, collect, out2);

    if (strcmp(out1, out2) != 0) {
        printf("FAIL: generation not deterministic\n'%s'\nvs\n'%s'\n",
               out1, out2);
        fail = 1;
    }
    if (strlen(out1) == 0) {
        printf("FAIL: empty generation\n");
        fail = 1;
    }
    for (i = 0; out1[i]; i++) {
        if (!strchr(RT_CHARSET, out1[i])) {
            printf("FAIL: char '%c' outside charset\n", out1[i]);
            fail = 1;
            break;
        }
    }

    /* max_new cap is honored */
    {
        rt_chat c;
        rt_chat_init(&c, 7);
        c.max_new = 5;
        out1[0] = '\0';
        rt_chat_push(&c, "hi there friend");
        rt_chat_push_idx(&c, 0);
        rt_chat_gen(&c, &m, collect, out1);
        if (strlen(out1) > 5) {
            printf("FAIL: max_new ignored (%d chars)\n", (int)strlen(out1));
            fail = 1;
        }
    }

    printf("test_chat: %s\nreply sample: %s\n", fail ? "FAIL" : "PASS", out2);
    return fail ? 1 : 0;
}
