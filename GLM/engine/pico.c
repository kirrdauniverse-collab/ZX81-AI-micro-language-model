/* Pico AI core - tokenizer with typo tolerance, retrieval-seeded bigram generation with temperature.
   Portable: host test build (gcc -DHOST) and ZX81 build (z88dk). */
#include "model.h"
#ifdef HOST
#include <stdio.h>
#include <string.h>
#else
#include <stdio.h>
#include <string.h>
#endif
typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned int uc;     /* sccz80 generates much smaller code for 16-bit locals */
#define END_ID 1
#define MAXT 24
#define COLS 32

static u16 seed = 1;
static u8 rnd(void) { seed = seed * 25173u + 13849u; return (u8)(seed >> 8); }
u8 temperature = 2;           /* 1..4 */
static uc unk;                /* unknown words in the last input */
static char unkw[17];         /* the first one */
static u8 ctx[32];            /* bitmap of tokens seen in the user text */
static u8 col;

static const char *vword(uc id) {                  /* words are stored one after another: scan to the id-th */
    const char *v = vstr;
    while (id-- > 2) v += strlen(v) + 1;
    return v;
}
static const u8 *succ_of(uc tok) {                 /* successor list of a token: (next,count) pairs ended by 0 */
    const u8 *p = bdat;
    while (tok--) { while (*p) p += 2; p++; }
    return p;
}

char rbuf[224];               /* reply text (lines already wrapped at 32 columns, '\n' separated) */
static u8 rlen;
u8 ntok_in, ntok_out;
static void outc(char c) { if (rlen < 222) rbuf[rlen++] = c; rbuf[rlen] = 0; }
u8 *pico_ctx(void) { return ctx; }
static void out_word(const char *w, uc nospace) {
    uc l = (uc)strlen(w);
    if (!nospace && col && col + 1 + l > COLS) { outc('\n'); col = 0; }
    else if (!nospace && col) { outc(' '); col++; }
    while (*w) { outc(*w++); col++; }
}

static void say(const char *s) {                 /* write a plain message word by word (wrapped) */
    char t[17]; uc n;
    while (*s) {
        for (n = 0; *s && *s != ' ' && n < 16; n++) t[n] = *s++;
        t[n] = 0;
        while (*s == ' ') s++;
        out_word(t, t[0] == '.' || t[0] == ',');
        ntok_out++;
    }
}

/* ---- tokenizer ---- */
static uc stem(const char *w, char *o) {          /* strips simple English endings; returns new length */
    uc n = 0;
    while (w[n] && n < 15) { o[n] = w[n]; n++; }
    o[n] = 0;
    if (n > 5 && !memcmp(o + n - 3, "ING", 3)) n -= 3;
    else if (n > 4 && !memcmp(o + n - 2, "ED", 2)) n -= 2;
    else if (n > 4 && !memcmp(o + n - 2, "ES", 2)) n -= 2;
    else if (n > 4 && !memcmp(o + n - 2, "LY", 2)) n -= 2;
    else if (n > 4 && !memcmp(o + n - 2, "ER", 2)) n -= 2;
    else if (n > 3 && o[n - 1] == 'S' && o[n - 2] != 'S') n -= 1;
    o[n] = 0;
    return n;
}
static uc edist(const char *a, uc la, const char *b, uc lb) {   /* Levenshtein, capped at 9 */
    u8 prev[17], cur[17]; uc i, j, c, m;
    if (la > 15 || lb > 15) return 9;
    for (j = 0; j <= lb; j++) prev[j] = j;
    for (i = 1; i <= la; i++) {
        cur[0] = i;
        for (j = 1; j <= lb; j++) {
            c = prev[j - 1] + (a[i - 1] != b[j - 1]);
            m = prev[j] + 1; if (m < c) c = m;
            m = cur[j - 1] + 1; if (m < c) c = m;
            cur[j] = c;
        }
        memcpy(prev, cur, lb + 1);
    }
    return prev[lb];
}
static uc lookup(const char *w) {                 /* word -> token id or 0 ; exact, stem, or near match */
    char s1[16], s2[16];
    uc l1, l2, id, best = 0, bd = 9, d, lv, lw = (uc)strlen(w);
    const char *v;
    for (id = 2, v = vstr; id < NVOC; id++, v += strlen(v) + 1)
        if (!strcmp(v, w)) return id;
    if (lw < 3) return 0;
    l1 = stem(w, s1);
    for (id = 2, v = vstr; id < NVOC; id++, v += strlen(v) + 1) {
        if (v[0] != w[0] && v[1] != w[1]) continue;       /* pre-filter: a typo rarely changes both first letters */
        lv = (uc)strlen(v);
        if (lv + 2 < lw || lw + 2 < lv || v[0] < 'A') continue;   /* and the length must be close */
        l2 = stem(v, s2);
        if (l1 == l2 && !strcmp(s1, s2)) return id;
        if (lw >= 4) {
            d = edist(w, lw, v, lv);
            if (d < bd) { bd = d; best = id; }
        }
    }
    if (best && ((lw >= 4 && bd <= 1) || (lw >= 8 && bd <= 2))) return best;
    return 0;
}
static uc tokenize(const char *line) {             /* fills ctx, returns number of recognised words */
    char w[17]; uc n = 0, hits = 0, id;
    memset(ctx, 0, sizeof ctx); unk = 0;
    for (;; line++) {
        char c = *line;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c >= 'A' && c <= 'Z' || c >= '0' && c <= '9') { if (n < 16) w[n++] = c; }
        else {
            if (n) { w[n] = 0; id = lookup(w); if (id) { ctx[id >> 3] |= 1 << (id & 7); hits++; }
                else if (n >= 3 && w[0] > '9') { if (!unk) strcpy(unkw, w); unk++; }
                n = 0; }
            if (!c) break;
        }
    }
    return hits;
}
#define INCTX(t) (ctx[(t) >> 3] & (1 << ((t) & 7)))

uc pico_count(const char *s) {
    uc n = 0, in = 0; char c;
    for (;; s++) {
        c = *s;
        if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9') { if (!in) { in = 1; n++; } }
        else { in = 0; if (c == '.' || c == ',' || c == '?' || c == '!') n++; if (!c) break; }
    }
    return n;
}

/* ---- generation ---- */
void pico_reply(const char *line, u16 sd) {
    uc i, bi = 0, cur, pos, n, nsucc, pick, divg;
    u16 bs = 0, sc, p, tot, r, follow, S;
    uc cm, sm, bcm = 0, bsm = 0, ln, mk, t;
    u8 seen[32];
    static const u8 kfol[5] = {0, 100, 60, 12, 3};   /* follow weight = successor total * k */
    if (sd) seed = sd;
    ntok_out = 0;
    tokenize(line);
    /* 1. retrieval: choose the sentence that matches the input best */
    for (i = 0; i < NSENT; i++) {
        sc = 0; cm = 0; sm = 0; ln = 0; mk = 0; memset(seen, 0, 32);
        for (p = sidx[i]; sent[p]; p++) { ln++; if (sent[p] == 255) mk = ln; }
        ln = 0;
        for (p = sidx[i]; sent[p]; p++) {
            ln++;
            t = sent[p];
            if (t != 255 && INCTX(t) && !(seen[t >> 3] & (1 << (t & 7)))) {      /* each matched word counts once per sentence */
                seen[t >> 3] |= 1 << (t & 7);
                if (vw[t]) { sc += vw[t]; cm++; } else sm += (ln < mk) ? 4 : 1;     /* small words in the trigger part count much more */
            }
        }
        sc = sc * 8 + (sm > 16 ? 16 : sm) * 4 + 40 - (ln > 40 ? 40 : ln) + (rnd() & (temperature * 3 - 1));   /* shorter wins ties; temperature adds jitter */
        if (sc >= bs) { bs = sc; bi = i; bcm = cm; bsm = sm; }
    }
    /* nothing sensible found: say so instead of guessing */
    rlen = 0; rbuf[0] = 0; col = 0;
    if (unk && bcm <= 1) {
        say("SORRY , I DO NOT KNOW THE WORD"); say(unkw); say(". TRY OTHER WORDS ."); return;
    }
    if (bcm == 0 && bsm < 3) { say("SORRY , I HAVE NO DATA ABOUT THAT . ASK ME SOMETHING ELSE ."); return; }
    /* 2. generate from the first word of that sentence, walking the bigram table */
    p = sidx[bi];
    for (r = p; sent[r]; r++) if (sent[r] == 255) { p = r + 1; break; }
    pos = 1; cur = sent[p]; divg = 0; n = 0; col = 0;
    out_word(vword(cur), 1); ntok_out++;
    while (n++ < MAXT) {
        const u8 *bp = succ_of(cur);
        tot = 0; S = 0;
        for (nsucc = 0; bp[2 * nsucc]; nsucc++) S += bp[2 * nsucc + 1];
        follow = S * kfol[temperature];
        for (nsucc = 0; bp[2 * nsucc]; nsucc++) {
            uc t = bp[2 * nsucc], c = bp[2 * nsucc + 1];
            u16 wgt = c;
            if (!divg && sent[p + pos] == t) wgt += follow;
            if (t != END_ID && INCTX(t) && vw[t]) wgt += 3;
            tot += wgt;
        }
        if (!nsucc) break;
        r = (u16)((rnd() << 8 | rnd()) % tot);
        pick = 0;
        for (i = 0; i < nsucc; i++) {
            uc t = bp[2 * i], c = bp[2 * i + 1];
            u16 wgt = c;
            if (!divg && sent[p + pos] == t) wgt += follow;
            if (t != END_ID && INCTX(t) && vw[t]) wgt += 3;
            if (r < wgt) { pick = t; break; }
            r -= wgt;
        }
        if (pick == END_ID || !pick) break;
        if (!divg) { if (sent[p + pos] == pick) pos++; else divg = 1; }
        cur = pick; ntok_out++;
        { const char *w = vword(cur); out_word(w, (w[0] == '.' || w[0] == ',' || w[0] == '?' || w[0] == '!')); }
    }
}
