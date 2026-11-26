/* test_charset.c - charset invariants + fold behavior. */
#include <stdio.h>
#include <string.h>
#include "rt_charset.h"

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
    int i, j;
    char seen[128];

    rt_charset_init();
    check(RT_CHARSET_LEN == 45, "45 chars");
    check(strlen(RT_CHARSET) == 45, "NUL-terminated at 45");

    for (i = 0; i < 128; i++)
        seen[i] = 0;
    for (i = 0; i < RT_CHARSET_LEN; i++) {
        unsigned char c = (unsigned char)RT_CHARSET[i];
        check(c < 128, "ascii only");
        check(!seen[c], "no duplicate chars");
        seen[c] = 1;
    }

    check(RT_CHARSET[0] == '\n', "index 0 is newline");
    check(RT_CHARSET[1] == ' ', "index 1 is space");
    check(rt_map_char('A') == rt_map_char('a'), "A folds to a");
    check(rt_map_char('Z') == rt_map_char('z'), "Z folds to z");
    check(rt_map_char('#') == rt_map_char(' '), "unknown -> space");
    check(rt_map_char('\200') == rt_map_char(' '), "high bit -> space");
    check(rt_map_char('5') == RT_CHAR_TO_IDX[(unsigned char)'5'],
          "digits map to themselves");

    for (i = 0; i < RT_CHARSET_LEN; i++)
        check(RT_CHAR_TO_IDX[(unsigned char)RT_CHARSET[i]] == i,
              "table matches string");

    printf("test_charset: %s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
