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
static u8 ctx[32];            /* bitmap of tokens seen in the user text */
static u8 col;

static const char *vword(uc id) { return vstr + voff[id - 2]; }

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
    uc l1, l2, id, best = 0, bd = 9, d, lw = (uc)strlen(w);
    l1 = stem(w, s1);
    for (id = 2; id < NVOC; id++) {
        const char *v = vword(id);
        if (!strcmp(v, w)) return id;
    }
    for (id = 2; id < NVOC; id++) {
        const char *v = vword(id);
        if (v[0] < 'A' || v[0] > 'Z') continue;
        l2 = stem(v, s2);
        if (l1 == l2 && !strcmp(s1, s2)) return id;
        if (lw >= 4) {
            d = edist(w, lw, v, (uc)strlen(v));
            if (d < bd) { bd = d; best = id; }
        }
    }
    if (best && ((lw >= 4 && bd <= 1) || (lw >= 8 && bd <= 2))) return best;
    return 0;
}
static uc tokenize(const char *line) {             /* fills ctx, returns number of recognised words */
    char w[17]; uc n = 0, hits = 0, id;
    memset(ctx, 0, sizeof ctx);
    for (;; line++) {
        char c = *line;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c >= 'A' && c <= 'Z' || c >= '0' && c <= '9') { if (n < 16) w[n++] = c; }
        else {
            if (n) { w[n] = 0; id = lookup(w); if (id) { ctx[id >> 3] |= 1 << (id & 7); hits++; } n = 0; }
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
    uc hits, i, bi = 0, cur, pos, n, nsucc, pick, divg;
    u16 bs = 0, sc, p, tot, r, follow, S;
    static const u8 kfol[5] = {0, 100, 30, 8, 2};   /* follow weight = successor total * k */
    if (sd) seed = sd;
    rlen = 0; rbuf[0] = 0; ntok_out = 0;
    hits = tokenize(line);
    /* 1. retrieval: choose the sentence that matches the input best */
    for (i = 0; i < NSENT; i++) {
        sc = 0;
        for (p = sidx[i]; sent[p]; p++) if (sent[p] != 255 && INCTX(sent[p])) sc += vw[sent[p]];
        { uc ln = 0; for (p = sidx[i]; sent[p]; p++) ln++; sc = sc * 8 + 40 - (ln > 40 ? 40 : ln) + (rnd() & (temperature * 3 - 1)); }   /* shorter wins ties; temperature adds jitter */
        if (hits == 0 && i >= 16) sc = (rnd() & 63) + 8;   /* general line */    /* nothing recognised: pick a general line */
        if (sc >= bs) { bs = sc; bi = i; }
    }
    /* 2. generate from the first word of that sentence, walking the bigram table */
    p = sidx[bi];
    for (r = p; sent[r]; r++) if (sent[r] == 255) { p = r + 1; break; }
    pos = 1; cur = sent[p]; divg = 0; n = 0; col = 0;
    out_word(vword(cur), 1); ntok_out++;
    while (n++ < MAXT) {
        u16 b = boff[cur];
        tot = 0; S = 0;
        for (nsucc = 0; bdat[b + 2 * nsucc]; nsucc++) S += bdat[b + 2 * nsucc + 1];
        follow = S * kfol[temperature];
        for (nsucc = 0; bdat[b + 2 * nsucc]; nsucc++) {
            uc t = bdat[b + 2 * nsucc], c = bdat[b + 2 * nsucc + 1];
            u16 wgt = c;
            if (!divg && sent[p + pos] == t) wgt += follow;
            if (t != END_ID && INCTX(t) && vw[t]) wgt += 3;
            tot += wgt;
        }
        if (!nsucc) break;
        r = (u16)((rnd() << 8 | rnd()) % tot);
        pick = 0;
        for (i = 0; i < nsucc; i++) {
            uc t = bdat[b + 2 * i], c = bdat[b + 2 * i + 1];
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
