#include "intent16.h"

/* Compact keyword router: the English QAT model was trained with these tags. */
static int has(const char *s, const char *w)
{
    int i, j;
    for (i = 0; s[i]; ++i) {
        for (j = 0; w[j]; ++j) {
            char a = s[i+j], b = w[j];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (!a || a != b) break;
        }
        if (!w[j]) return 1;
    }
    return 0;
}

const char *rt16_reply_tag(const char *s)
{
    if (has(s, "hello")) return ":hello:";
    if (has(s, "who are you")) return ":whoami";
    if (has(s, "earth")) return ":earth::";
    if (has(s, "sun")) return ":sun:::";
    if (has(s, "zx81")) return ":zx81ai:";
    if (has(s, "planet")) return ":planet:";
    if (has(s, "space")) return ":space:";
    return ":help!!";
}
