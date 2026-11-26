/* Pico AI core v2 - builds its sentences from facts (subject, relation, object) with small grammar templates.
   No stored sentences except a few short social "acts". Tokenizer with typo tolerance, endings, taught words and smileys.
   Portable: host test build (gcc -DHOST) and ZX81 build (z88dk). */
#include "kb.h"
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned int uc;     /* sccz80 generates much smaller code for 16-bit locals */
#define COLS 32
#define R_IS 1
#define R_PROP 2
#define R_HAS 3
#define R_LIVES 4
#define R_MADE 5
#define R_CAN 6
#define R_LIKES 7
#define R_PART 8
#define R_MIX 9
#define R_ORBIT 10
#define R_DO 11
#define R_PRON 12
#define R_NAMED 13
#define R_ALIAS 14
#define R_DEF 15

static u16 seed = 1;
static u8 rnd(void) { seed = seed * 25173u + 13849u; return (u8)(seed >> 8); }
u8 temperature = 2;           /* 1..4: how many sentences, and how freely */
static uc unk;                /* unknown words in the last input */
static char unkw[17];         /* the longest one */
static u8 ctx[32];            /* bitmap of the words seen in the user text */
static u8 col;
const char *pico_alias;       /* taught words: 4 x (alias[8], meaning[8]) in the persistent zone */
const char *pico_bot;         /* the name given to Pico */
char rbuf[180];               /* reply text (lines wrapped at 32 columns, '\n' separated) */
static u8 rlen;
u8 ntok_in, ntok_out;
u8 *pico_ctx(void) { return ctx; }
void think_cb(uc phase, uc x, uc y);              /* shows what the model is doing (defined by the front end) */

static const char *vword(uc id) {                  /* words are stored one after another: scan to the id-th */
    const char *v = vstr;
    while (id-- > 2) v += strlen(v) + 1;
    return v;
}
static void outc(char c) { if (rlen < 178) rbuf[rlen++] = c; rbuf[rlen] = 0; }
static void out_word(const char *w, uc nospace) {
    u8 l = (u8)strlen(w);
    if (!nospace && col && col + 1 + l > COLS) { outc('\n'); col = 0; }
    else if (!nospace && col) { outc(' '); col++; }
    while (*w) { outc(*w++); col++; }
}
static void el(const char *s) {                    /* emit literal words separated by spaces */
    char t[17]; uc n;
    while (*s) {
        for (n = 0; *s && *s != ' ' && n < 16; n++) t[n] = *s++;
        t[n] = 0;
        while (*s == ' ') s++;
        out_word(t, t[0] == '.' || t[0] == ',' || t[0] == '?' || t[0] == '!');
        ntok_out++;
    }
}
static void ew(uc id) {                            /* emit a vocabulary word; _ separates the words of a name */
    const char *w = vword(id); char t[25]; uc n;
    while (*w) {
        for (n = 0; *w && *w != '_' && n < 24; n++) t[n] = *w++;
        t[n] = 0;
        if (*w == '_') w++;
        out_word(t, t[0] == '.' || t[0] == ',' || t[0] == '?' || t[0] == '!');
        ntok_out++;
    }
}

/* ---- tokenizer ---- */
#define INCTX(t) (ctx[(t) >> 3] & (1 << ((t) & 7)))
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
static uc suffix_ok(const char *r) {              /* common English endings */
    return !strcmp(r, "S") || !strcmp(r, "ES") || !strcmp(r, "ED") || !strcmp(r, "D") || !strcmp(r, "ING") || !strcmp(r, "ER") || !strcmp(r, "LY");
}
static uc eqw(const char *v, const char *w) {      /* w is v, or one of the words of a name like BUS_DRIVER */
    uc n = (uc)strlen(w);
    for (;;) {
        if (!strncmp(v, w, n) && (v[n] == 0 || v[n] == '_')) return 1;
        v = strchr(v, '_');
        if (!v) return 0;
        v++;
    }
}
static uc lookup(const char *w) {                 /* word -> id or 0 ; exact, ending, taught alias or near match */
    uc id, best = 0, lv, k, lw = (uc)strlen(w);
    const char *v;
    for (id = 2, v = vstr; id < NVOC; id++, v += strlen(v) + 1)
        if (eqw(v, w)) return id;
    if (lw < 3) return 0;
    for (k = 0; pico_alias && k < 4; k++) {          /* words the user has taught */
        const char *e = pico_alias + k * 16;
        if (e[0] && !strcmp(e, w) && strcmp(e, e + 8)) return lookup(e + 8);
    }
    for (id = 2, v = vstr; id < NVOC; id++, v += strlen(v) + 1) {
        lv = (uc)strlen(v);
        if (lv >= 3 && lw > lv && !memcmp(w, v, lv) && suffix_ok(w + lv)) return id;      /* PLANETS -> PLANET, TALKING -> TALK */
        if (lw >= 4 && (v[0] == w[0] || v[1] == w[1]) && near1(w, lw, v, lv)) best = id;   /* typos */
    }
    return best;
}
uc pico_known(const char *w) { return lookup(w); }
const char *pico_word(uc id) { return vword(id); }
static void mood(const char *w) { uc id = lookup(w); if (id) ctx[id >> 3] |= 1 << (id & 7); }
static void tokenize(const char *line) {            /* fills ctx, counts unknown words */
    char w[17]; uc n = 0, id;
    memset(ctx, 0, sizeof ctx); unk = 0;
    for (;; line++) {
        char c = *line;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c == ':' && !n && (line[1] == ')' || line[1] == '(')) { mood(line[1] == ')' ? "SMILE" : "SAD"); line++; continue; }   /* smileys */
        if (c >= 'A' && c <= 'Z' || c >= '0' && c <= '9') { if (n < 16) w[n++] = c; }
        else {
            if (n) { w[n] = 0; id = lookup(w); if (id) ctx[id >> 3] |= 1 << (id & 7);
                else if (n >= 4 && w[0] > '9') { if (!unk || n > strlen(unkw)) strcpy(unkw, w); unk++; }
                n = 0; }
            if (!c) break;
        }
    }
}
uc pico_count(const char *s) {
    uc n = 0, in = 0; char c;
    for (;; s++) {
        c = *s;
        if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9') { if (!in) { in = 1; n++; } }
        else { in = 0; if (c == '.' || c == ',' || c == '?' || c == '!') n++; if (!c) break; }
    }
    return n;
}
static void say(const char *s) { el(s); }

/* ---- sentence builder ---- */
static uc sub, own, fs, want, alias_only, pid;       /* subject, speaking about myself, first sentence, asked relation */
static const char *pron;
static u8 ls[8];
static uc collect(uc s, uc r, u8 *buf, uc max) {      /* objects of the facts (s, r) */
    uc i, n = 0; const u8 *f = fact;
    for (i = 0; i < NF; i++, f += 4) if (f[0] == s && f[1] == r && n < max) buf[n++] = f[2];
    return n;
}
static void ewd(uc id) { u8 t[1]; if (collect(id, R_DEF, t, 1)) el("THE"); ew(id); }   /* THE Sun, THE Earth */
static void subj(void) { if (own) el("I"); else if (fs) ewd(sub); else el(pron); fs = 0; }
static uc plural(uc id) { const char *w = vword(id); return w[strlen(w) - 1] == 'S'; }
static void art(uc id) { char c = vword(id)[0]; el(c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' ? "AN" : "A"); }
static void elist(const u8 *ids, uc n, uc arts) {     /* A , B AND C */
    uc i;
    for (i = 0; i < n; i++) {
        if (i) el(i == n - 1 ? "AND" : ",");
        if (arts && !plural(ids[i])) { art(ids[i]); ew(ids[i]); } else ewd(ids[i]);
    }
}
static uc s_list(uc r, const char *v3, const char *v1, uc arts) {
    uc n = collect(sub, r, ls, 6);
    if (!n) return 0;
    subj(); el(own ? v1 : v3); elist(ls, n, arts); el(".");
    return 1;
}
static uc s_desc(void) {                              /* "MARS IS A RED PLANET ."  and the rest of what is, with the same words */
    u8 cl[5], pr[5]; uc nc = collect(sub, R_IS, cl, 5), np = collect(sub, R_PROP, pr, 5), k;
    if (!nc && !np) return 0;
    subj(); el(own ? "AM" : "IS");
    if (nc) {
        art(np ? pr[0] : cl[0]);
        for (k = 0; k < np && k < 2; k++) { if (k) el(","); ew(pr[k]); }
        ew(cl[0]);
        el(".");
        if (nc > 1) { subj(); el(own ? "AM" : "IS"); elist(cl + 1, nc - 1, 1); el("."); }
        if (np > 2) { subj(); el(own ? "AM" : "IS"); elist(pr + 2, np - 2, 0); el("."); }
    } else { elist(pr, np, 0); el("."); }
    return 1;
}
static uc s_members(void) {                           /* "CAR , BUS AND TRAIN ARE VEHICLES ." */
    uc i, n = 0; const u8 *f = fact;
    for (i = 0; i < NF; i++, f += 4) if (f[1] == R_IS && f[2] == sub && n < 6) ls[n++] = f[0];
    if (!n) return 0;
    elist(ls, n, 0); el("ARE"); ew(sub); outc('S'); col++; el(".");
    fs = 0;
    return 1;
}
static uc s_named(void) {
    if (own) { el("MY NAME IS"); if (pico_bot && pico_bot[0]) el(pico_bot); else el("PICO"); el("."); fs = 0; return 1; }
    return s_list(R_NAMED, "IS CALLED", "AM CALLED", 0);
}
static uc group(uc g) {
    switch (g) {
    case 0: return s_desc();
    case 1: return s_list(R_HAS, "HAS", "HAVE", 1);
    case 2: return s_list(R_LIVES, "LIVES IN", "LIVE IN", 0);
    case 3: return s_list(R_ORBIT, "GOES AROUND", "GO AROUND", 0);
    case 4: return s_list(R_MADE, "WAS MADE BY", "WAS MADE BY", 0);
    case 5: return s_list(R_CAN, "CAN", "CAN", 0);
    case 6: return s_list(R_LIKES, "LIKES", "LIKE", 0);
    case 7: return s_list(R_PART, "IS PART OF", "AM PART OF", 0);
    case 9: return s_members();
    default: return s_named();
    }
}
static const u8 wantg[15] = {0, 0, 0, 1, 2, 4, 5, 6, 7, 0, 3, 8, 0, 10, 0};   /* asked relation -> group */
static void build(void) {
    uc g, done = 0;
    own = (sub == pid); fs = 1; pron = "IT";
    if (collect(sub, R_PRON, ls, 1)) pron = vword(ls[0]);
    if (want && group(wantg[want])) { think_cb(3, 1, 0); return; }          /* the asked relation */
    for (g = 0; g < 10 && done < temperature; g++) {
        if (temperature >= 3 && g && (rnd() & 1)) continue;      /* high temperature: a freer choice of what to say */
        if (group(g)) { done++; think_cb(3, done, 0); }
    }
}
static uc mix(void) {
    uc i; const u8 *f = fact;
    for (i = 0; i < NF; i++, f += 4)
        if (f[1] == R_MIX && INCTX(f[2]) && INCTX(f[3])) { ew(f[2]); el("AND"); ew(f[3]); el("MAKE"); ew(f[0]); el("."); return 1; }
    return 0;
}
static uc act(void) {                                 /* a short social routine whose trigger words are in the text */
    const u8 *p = actd, *q, *best = 0; uc i, cnt, tot, bc = 0;
    for (i = 0; i < NACT; i++) {
        q = p + 1; cnt = 0; tot = 0;
        while (*q != 255) { tot++; if (INCTX(*q)) cnt++; q++; }
        q++;
        if ((p[0] ? cnt == tot : cnt > 0) && cnt >= bc) { best = q; bc = cnt; }
        while (*q) q++;
        p = q + 1;
    }
    if (!best) return 0;
    for (; *best; best++) ew(*best);
    return 1;
}
static uc pick(void) {                                /* the word the question is about: most facts about it */
    uc id, best = 0, bs = 0, s, i, tg; const u8 *f;
    alias_only = 0;
    for (id = 2; id < NVOC; id++) if (INCTX(id)) {
        s = 0; tg = id;
        for (i = 0, f = fact; i < NF; i++, f += 4) {
            if (f[0] == id) { if (f[1] == R_ALIAS) { tg = f[2]; s = 1; break; } s += 2; }
            else if (f[1] == R_IS && f[2] == id) s++;
        }
        if (s > bs) { bs = s; best = tg; alias_only = (tg != id); }
    }
    return best;
}
void pico_reply(const char *line, uc sd) {
    const u8 *c; uc a;
    if (sd) seed = sd;
    ntok_out = 0; rlen = 0; rbuf[0] = 0; col = 0;
    tokenize(line);
    think_cb(1, 0, 0);
    pid = lookup("PICO");
    want = 0;
    for (c = cue; *c; c += 2) if (INCTX(c[0])) { want = c[1]; break; }
    if (mix()) { think_cb(2, 0, 0); return; }
    if (want == R_MIX) want = 0;
    sub = pick();
    think_cb(2, sub, 0);
    a = 0;
    if (!want) { rlen = 0; rbuf[0] = 0; col = 0; a = act(); if (a) return; }
    if (unk && (!sub || alias_only)) { say("I DO NOT KNOW THE WORD"); say(unkw); say(". TEACH ME :"); say(unkw); say("MEANS ..."); return; }
    if (sub) { build(); if (rlen) return; }
    say("SORRY , NO DATA ABOUT THAT .");
}
