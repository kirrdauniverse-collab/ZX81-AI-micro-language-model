/* test_router.c - keyword topic routing incl. the deliberate ambiguity:
 * "star" (and moon/sky) belongs to BOTH the space and the poems pack. */
#include <stdio.h>
#include <string.h>
#include "rt_charset.h"
#include "rt_router.h"

static int fails;

static void check(int cond, const char *what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        fails++;
    }
}

static int scan1(const char *text, char hits[][16])
{
    return rt_router_scan(text, hits, RT_ROUTER_MAX_HITS);
}

int main(void)
{
    char hits[RT_ROUTER_MAX_HITS][16];
    int n;

    rt_charset_init();

    n = scan1("how much ram does the amiga 500 have?", hits);
    check(n == 1 && strcmp(hits[0], "retro") == 0, "amiga -> retro");

    n = scan1("what is the weather like today?", hits);
    check(n == 1 && strcmp(hits[0], "weather") == 0, "weather");

    n = scan1("my dog ate my homework", hits);
    check(n == 1 && strcmp(hits[0], "animals") == 0, "dog -> animals");

    n = scan1("tell me a funny joke", hits);
    check(n == 1 && strcmp(hits[0], "jokes") == 0, "joke -> jokes");

    /* whole-word matching: "catapult" must NOT hit animals */
    n = scan1("the catapult fired over the castle", hits);
    check(n == 0, "catapult is not a cat");

    n = scan1("what is 6 plus 7?", hits);
    check(n == 1 && strcmp(hits[0], "numbers") == 0, "plus -> numbers");

    /* the ambiguity the clarifying question resolves: "stars" fits BOTH
     * the space and the poems pack ("sky" also drags in weather, so the
     * hit count can exceed two - the clarification menu just grows) */
    n = scan1("tell me about the stars in the sky", hits);
    check(n >= 2, "stars hits multiple topics");
    {
        int has_space = 0, has_poems = 0, i;
        for (i = 0; i < n; i++) {
            if (strcmp(hits[i], "space") == 0)
                has_space = 1;
            if (strcmp(hits[i], "poems") == 0)
                has_poems = 1;
        }
        check(has_space && has_poems, "stars -> space + poems");
    }

    /* without the shared "sky": exactly the two star topics */
    n = scan1("tell me about the stars tonight", hits);
    check(n == 2, "stars alone = two topics");
    if (n == 2) {
        check((strcmp(hits[0], "space") == 0 && strcmp(hits[1], "poems") == 0) ||
              (strcmp(hits[0], "poems") == 0 && strcmp(hits[1], "space") == 0),
              "stars -> space + poems only");
    }

    n = scan1("i love the moon in a poem", hits);
    check(n == 2, "moon poem hits space + poems");

    /* uppercase input folds */
    n = scan1("STARS ARE BEAUTIFUL", hits);
    check(n == 2, "uppercase stars also ambiguous");

    /* no keyword -> general */
    n = scan1("hello, how are you doing?", hits);
    check(n == 0, "small talk stays general");

    n = scan1("", hits);
    check(n == 0, "empty input is general");

    printf("test_router: %s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
