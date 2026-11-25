/* helpers for text packs: a reply buffer rb with cat functions, and a small random generator */
#ifndef RBSIZE
#define RBSIZE 230
#endif
static char rb[RBSIZE];
static uc rl, rseed = 1;
static void rcl(void) { rl = 0; rb[0] = 0; }
static void cat(const char *s) { while (*s && rl < RBSIZE - 4) rb[rl++] = *s++; rb[rl] = 0; }
static void catc(char c) { if (rl < RBSIZE - 4) { rb[rl++] = c; rb[rl] = 0; } }
static void catn(int v) {                       /* signed number */
    char t[6]; uc n = 0;
    if (v < 0) { catc('-'); v = -v; }
    do { t[n++] = '0' + v % 10; v /= 10; } while (v);
    while (n) catc(t[--n]);
}
static uc rr(uc n) { rseed = rseed * 25173u + 13849u; return (rseed >> 8) % n; }
static uc isd(char c) { return c >= '0' && c <= '9'; }
static uc isl(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
static uc up(char c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
static uc wat0(const char *p, const char *w) { while (*w) if (up(*p++) != *w++) return 0; return 1; }
static const char *word_after(const char *s, const char *key) {   /* first word after key, or 0 */
    int p = find(s, key);
    if (p < 0) return 0;
    s += p + strlen(key);
    while (*s == ' ') s++;
    return *s ? s : 0;
}
static void catw(const char *s, uc max) {       /* append one word (letters/digits), upper case */
    uc n = 0;
    while (*s && *s != ' ' && n++ < max) catc(up(*s++));
}
static const char *topic_word(const char *s) {   /* the word after ABOUT / ON / FOR, without a, an, the */
    const char *p = word_after(s, " ABOUT ");
    if (!p) p = word_after(s, " ON ");
    if (!p) p = word_after(s, " FOR ");
    if (p && (wat0(p, "A ") || wat0(p, "AN ") || wat0(p, "THE "))) { while (*p && *p != ' ') p++; while (*p == ' ') p++; }
    return (p && *p) ? p : 0;
}
