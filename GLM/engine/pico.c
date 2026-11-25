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
static const u8 *succ_of(uc tok) {                 /* successor list of a token: ids ended by 0 */
    const u8 *p = bdat;
    while (tok--) { while (*p) p++; p++; }
    return p;
}

char rbuf[180];               /* reply text (lines already wrapped at 32 columns, '\n' separated) */
static u8 rlen;
u8 ntok_in, ntok_out;
static void outc(char c) { if (rlen < 178) rbuf[rlen++] = c; rbuf[rlen] = 0; }
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

void think_cb(uc phase, uc x, uc y);              /* shows what the model is doing (defined by the front end) */

/* ---- tokenizer ---- */
static uc near1(const char *a, uc la, const char *b, uc lb) {   /* edit distance <= 1 (one typo, missing or extra letter) */
    uc i = 0, j = 0, e = 0;
    if (la > lb + 1 || lb > la + 1) return 0;
    while (i < la && j < lb) {
        if (a[i] == b[j]) { i++; j++; continue; }
        if (e++) return 0;
        if (la > lb) i++; else if (lb > la) j++; else { i++; j++; }
    }
    return e + (la - i) + (lb - j) <= 1;
}
const char *pico_alias;                           /* taught words: 4 x (alias[8], meaning[8]) in the persistent zone */
static uc suffix_ok(const char *r) {              /* common English endings */
    return !strcmp(r, "S") || !strcmp(r, "ES") || !strcmp(r, "ED") || !strcmp(r, "D") || !strcmp(r, "ING") || !strcmp(r, "ER") || !strcmp(r, "LY");
}
static uc lookup(const char *w) {                 /* word -> token id or 0 ; exact, ending, taught alias or near match */
    uc id, best = 0, lv, k, lw = (uc)strlen(w);
    const char *v;
    for (id = 2, v = vstr; id < NVOC; id++, v += strlen(v) + 1)
        if (!strcmp(v, w)) return id;
    if (lw < 3) return 0;
    for (k = 0; pico_alias && k < 4; k++) {          /* words the user has taught */
        const char *e = pico_alias + k * 16;
        if (e[0] && !strcmp(e, w) && strcmp(e, e + 8)) return lookup(e + 8);
    }
    for (id = 2, v = vstr; id < NVOC; id++, v += strlen(v) + 1) {
        lv = (uc)strlen(v);
        if (lv >= 3 && lw > lv && !memcmp(w, v, lv) && suffix_ok(w + lv)) return id;      /* PLANETS -> PLANET */
        if (lw >= 4 && (v[0] == w[0] || v[1] == w[1]) && v[0] >= 'A' && near1(w, lw, v, lv)) best = id;   /* typos */
    }
    return best;
}
uc pico_known(const char *w) { return lookup(w); }
const char *pico_word(uc id) { return vword(id); }
static void mood(const char *w) { uc id = lookup(w); if (id) ctx[id >> 3] |= 1 << (id & 7); }
static uc tokenize(const char *line) {             /* fills ctx, returns number of recognised words */
    char w[17]; uc n = 0, hits = 0, id;
    memset(ctx, 0, sizeof ctx); unk = 0;
    for (;; line++) {
        char c = *line;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c == ':' && !n && line[1]) {             /* smileys  :)  :D  :(  :O */
            id = line[1] & 0xDF;
            if (line[1] == ')' || id == 'D') { mood("SMILE"); hits++; line++; continue; }
            if (line[1] == '(') { mood("SAD"); hits++; line++; continue; }
            if (id == 'O') { mood("WOW"); hits++; line++; continue; }
        }
        if (c >= 'A' && c <= 'Z' || c >= '0' && c <= '9') { if (n < 16) w[n++] = c; }
        else {
            if (n) { w[n] = 0; id = lookup(w); if (id) { ctx[id >> 3] |= 1 << (id & 7); hits++; }
                else if (n >= 4 && w[0] > '9') { if (!unk || n > strlen(unkw)) strcpy(unkw, w); unk++; }
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
static u16 swgt(uc t, u16 nextpos, u16 follow, uc divg) {   /* weight of one successor: 1 + path bonus + context bonus */
    u16 w = 1;
    uc tg = sent[nextpos];
    if (!tg) tg = END_ID;                                 /* the end of the chosen sentence is the END token */
    if (!divg && tg == t) w += follow;
    if (t != END_ID && INCTX(t) && vw[t]) w += 3;
    return w;
}
void pico_reply(const char *line, u16 sd) {
    uc i, cur, pos, n, nsucc, pick, divg;
    u16 bs = 0, sc, p, tot, r, follow, st, bstart = 0;
    uc cm, sm, bcm = 0, bsm = 0, ln, mk, t;
    u8 seen[32];
    static const u8 kfol[5] = {0, 255, 120, 20, 4};   /* follow weight = successor total * k */
    if (sd) seed = sd;
    ntok_out = 0;
    tokenize(line);
    think_cb(1, 0, 0);
    /* 1. retrieval: choose the sentence that matches the input best */
    for (i = 0, st = 0; i < NSENT; i++) {
        sc = 0; cm = 0; sm = 0; ln = 0; mk = 0; memset(seen, 0, 32);
        for (p = st; sent[p]; p++) { ln++; if (sent[p] == 255) mk = ln; }
        ln = 0;
        for (p = st; sent[p]; p++) {
            ln++;
            t = sent[p];
            if (t != 255 && INCTX(t) && !(seen[t >> 3] & (1 << (t & 7)))) {      /* each matched word counts once per sentence */
                seen[t >> 3] |= 1 << (t & 7);
                if (vw[t]) { sc += vw[t]; cm++; } else sm += (ln < mk) ? 4 : 1;     /* small words in the trigger part count much more */
            }
        }
        sc = sc * 8 + (sm > 16 ? 16 : sm) * 4 + 40 - (ln > 40 ? 40 : ln) + (rnd() & (temperature * 3 - 1));   /* shorter wins ties; temperature adds jitter */
        if (sc >= bs) { bs = sc; bstart = st; bcm = cm; bsm = sm; }
        st = p + 1;
    }
    think_cb(2, bs >> 3, 0);
    /* nothing sensible found: say so instead of guessing */
    rlen = 0; rbuf[0] = 0; col = 0;
    if (unk && bcm <= 1) {
        say("SORRY , I DO NOT KNOW THE WORD"); say(unkw); say(". TEACH ME :"); say(unkw); say("MEANS A WORD I KNOW ."); return;
    }
    if (bcm == 0 && bsm < 3) { say("SORRY , I HAVE NO DATA ABOUT THAT . ASK ME SOMETHING ELSE ."); return; }
    /* 2. generate from the first word of that sentence, walking the bigram table */
    p = bstart;
    for (r = p; sent[r]; r++) if (sent[r] == 255) { p = r + 1; break; }
    pos = 1; cur = sent[p]; divg = 0; n = 0; col = 0;
    out_word(vword(cur), 1); ntok_out++;
    while (n++ < MAXT) {
        const u8 *bp = succ_of(cur);
        for (nsucc = 0; bp[nsucc]; nsucc++) ;
        if (!nsucc) break;
        think_cb(3, n, nsucc);
        follow = nsucc * kfol[temperature];
        for (tot = 0, i = 0; i < nsucc; i++) tot += swgt(bp[i], p + pos, follow, divg);
        r = (u16)((rnd() << 8 | rnd()) % tot);
        pick = 0;
        for (i = 0; i < nsucc; i++) {
            uc w = swgt(bp[i], p + pos, follow, divg);
            if (r < w) { pick = bp[i]; break; }
            r -= w;
        }
        if (pick == END_ID || !pick) break;
        if (!divg) { if (sent[p + pos] == pick) pos++; else divg = 1; }
        cur = pick; ntok_out++;
        { const char *w = vword(cur); out_word(w, (w[0] == '.' || w[0] == ',' || w[0] == '?' || w[0] == '!')); }
    }
}
