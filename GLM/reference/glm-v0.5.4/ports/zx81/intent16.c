#include "intent16.h"
static int contains_ci(const char *text, const char *needle)
{
    int i, j;
    for (i = 0; text[i]; i++) {
        for (j = 0; needle[j]; j++) {
            char a = text[i + j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (!a || a != b) break;
        }
        if (!needle[j]) return 1;
    }
    return 0;
}
const char *rt16_reply_tag(const char *s)
{
    if (contains_ci(s,"hello") || contains_ci(s,"hi!") ||
        contains_ci(s,"hey") || contains_ci(s,"good morning") ||
        contains_ci(s,"good evening") || contains_ci(s,"greetings"))
        return ":hello:";
    if (contains_ci(s,"who are you") || contains_ci(s,"your name") ||
        contains_ci(s,"introduce yourself") || contains_ci(s,"about yourself"))
        return ":whoami";
    if (contains_ci(s,"earth")) return ":earth::";
    if (contains_ci(s,"sun")) return ":sun:::";
    if (contains_ci(s,"zx81") || contains_ci(s,"sinclair")) return ":zx81ai:";
    if (contains_ci(s,"planet")) return ":planet:";
    if (contains_ci(s,"star") || contains_ci(s,"moon") ||
        contains_ci(s,"space") || contains_ci(s,"astronomy") ||
        contains_ci(s,"telescope")) return ":space:";
    return ":help!!";
}
