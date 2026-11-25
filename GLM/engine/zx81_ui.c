/* Shared ZX81 user-interface layer (included by every pack): display, chat window, clock, keyboard, persistent zone.
   Rule: display cells are only 0..63 or 128..191 - never 0x76 or any code with bit 6 set (that ends the row). */
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned int uc;
extern u8 temperature;

#define INV 128
#define CH0 1
#define ST0 14
#define PZ_ADDR 0x7F40             /* persistent zone (192 bytes at the very top): above RAMTOP, survives NEW and LOAD */

/* ---- persistent zone: lives above RAMTOP (the loader sets RAMTOP to $7F00); a local copy is used if RAMTOP is not set ---- */
#include "pz.h"
static struct PZ *pz;
static uc hh, mm, ss, acc, pz_ok;            /* clock kept in plain variables, copied to the zone once a second */
static void pz_save(void) { pz->h_ = hh; pz->m_ = mm; pz->s_ = ss; pz->a_ = acc; }
static void pz_init(void) {
    pz_ok = (*(uc *)16388 == PZ_ADDR);
    pz = (struct PZ *)PZ_ADDR;
    if (pz->mg0 != 'P' || pz->mg1 != 'Z') {
        memset(pz, 0, sizeof(struct PZ)); pz->mg0 = 'P'; pz->mg1 = 'Z'; pz->temp = 2;
    }
    temperature = pz->temp;
    pz->req = 0;
    hh = pz->h_; mm = pz->m_; ss = pz->s_; acc = pz->a_;
}

/* ---- display file ---- */
static uc ch1 = 12;                         /* last chat row */
static u8 *dfile;                           /* the expanded display file the .P image already contains: 1 + 24 rows * 33 */
static u8 *cell(uc r, uc c) { return dfile + 1 + r * 33 + c; }
static void wait_frame(void) {
#asm
    halt
#endasm
}
static const char pun[] = ".,?:;-+*/=()<>\"$";
static const u8 pcd[] = {27, 26, 15, 14, 25, 22, 21, 23, 24, 20, 16, 17, 19, 18, 11, 13};
static uc zc(uc c) {                        /* ASCII -> ZX81 character code; '@' becomes an inverse 0 (the ZX81 has no at sign) */
    uc i;
    if (c == '@') return 28 + INV;
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 38;
    if (c >= '0' && c <= '9') return c - '0' + 28;
    if (c == '!') c = '.';
    if (c == 39) c = '"';
    for (i = 0; pun[i]; i++) if (pun[i] == c) return pcd[i];
    return 0;
}
static void fillr(uc r, uc inv) { u8 *p = cell(r, 0); uc i; for (i = 0; i < 32; i++) *p++ = inv; }
static void text_at(uc r, uc c, const char *s, uc inv) { u8 *p = cell(r, c); while (*s && c++ < 32) *p++ = zc(*s++) | inv; }
static void scr_init(void) {
    uc r;
    dfile = (u8 *)*(uc *)16396;             /* D_FILE system variable */
    dfile[0] = 0x76;
    for (r = 0; r < 24; r++) { fillr(r, 0); *cell(r, 32) = 0x76; }
}
static void chat_line(const char *s, uc inv) {
    memmove(cell(CH0, 0), cell(CH0 + 1, 0), (ch1 - CH0) * 33);
    fillr(ch1, inv);
    text_at(ch1, 0, s, inv);
}
static void chat_text(const char *s) {     /* word wrap at 32 columns, '\n' forces a break */
    char ln[33]; uc n, i, j;
    while (*s) {
        if (*s == '\n') { s++; continue; }
        for (n = 0; s[n] && s[n] != '\n' && n < 32; n++) ;
        if (n == 32 && s[n] && s[n] != '\n' && s[n] != ' ') {
            for (j = 0, i = 1; i < 32; i++) if (s[i] == ' ') j = i;
            if (j) n = j;
        }
        memcpy(ln, s, n); ln[n] = 0;
        chat_line(ln, 0);
        s += n;
        while (*s == ' ') s++;
    }
}

/* ---- text formatting into a buffer ---- */
static char *wp;
static void a_c(char c) { *wp++ = c; }
static void a_s(const char *s) { while (*s) *wp++ = *s++; }
static void a_u(uc v) { char t[5]; uc n = 0; do { t[n++] = '0' + v % 10; v /= 10; } while (v); while (n) *wp++ = t[--n]; }
static void a_2(uc v) { *wp++ = '0' + v / 10; *wp++ = '0' + v % 10; }
static uc lastf;
static void a_t(void) { a_2(hh); a_c(':'); a_2(mm); a_c(':'); a_2(ss); }
static void a_x(uc tn) { a_c(' '); a_u(tn / 10); a_c('.'); a_u(tn % 10); a_c('S'); }
static uc frames(void) { return 0xFFFF - *(uc *)16436; }     /* FRAMES counts down on the ZX81 */
static uc tick_clock(void) {                /* returns 1 when the second changed */
    uc now = frames(), ch = 0;
    acc += now - lastf; lastf = now;
    while (acc >= 50) { acc -= 50; ch = 1; if (++ss == 60) { ss = 0; if (++mm == 60) { mm = 0; if (++hh == 24) { hh = 0; pz->dayc++; } } } }
    if (ch) pz_save();
    return ch;
}

/* ---- info panel ---- */
static const char *uname(void) { return pz->names[pz->cur][0] ? pz->names[pz->cur] : "YOU"; }
static const char *bname(void) { return pz->bot[0] ? pz->bot : "PICO"; }
static uc tok_i, tok_o, last_tn;
static char b[34];
static void draw_clock(void) { wp = b; a_t(); *wp = 0; text_at(21, 24, b, INV); }
static void draw_status(const char *state) {
    fillr(21, INV);
    wp = b; a_c('T'); a_u(temperature); a_s(" TOK "); a_u(tok_i); a_c('/'); a_u(tok_o); a_x(last_tn); *wp = 0;
    text_at(21, 0, b, INV);
    draw_clock();
    fillr(23, 0); text_at(23, 0, state, 0);
}

/* ---- keyboard: own matrix scan with the z88dk translation table (unshifted 40, shifted 40) ---- */
static const u8 ktab[80] = {
    255,'Z','X','C','V', 'A','S','D','F','G', 'Q','W','E','R','T', '1','2','3','4','5', '0','9','8','7','6',
    'P','O','I','U','Y', 13,'L','K','J','H', ' ','.','M','N','B',
    255,':',';','?','/', '~','|',92,'{','}', 131,132,133,18,20, 27,28,29,30,8, 12,6,9,11,10,
    34,')','(','$',25, 13,'=','+','-','^', 96,',','>','<','*' };
static const u8 kport[8] = { 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F };
static uc inport(uc hi) __z88dk_fastcall {  /* IN A,(hi:$FE) */
#asm
    ld      b, l
    ld      c, $fe
    in      a, (c)
    ld      l, a
    ld      h, 0
#endasm
}
static uc lastk;
static uc key_in(void) {
    uc r, k = 0, sh, bit, v;
    wait_frame();
    sh = !(inport(0xFE) & 1);
    for (r = 0; r < 8 && !k; r++) {
        v = inport(kport[r]);
        for (bit = 0; bit < 5; bit++)
            if (!(v & (1 << bit)) && (r || bit)) { k = ktab[(sh ? 40 : 0) + r * 5 + bit]; break; }
    }
    if (k == lastk) return 0;
    lastk = k;
    return k;
}
/* ---- input line: IN_ROWS rows starting at in0 ---- */
static uc idle_limit, idle_n;               /* idle question: seconds without a key, and the function to call */
static void (*idle_fn)(void);
static void header(const char *who, uc tok, uc with_time);
static void check_rem(void) {
    if (pz->rem_on && hh == pz->rem_h && mm == pz->rem_m) {
        pz->rem_on = 0;
        wp = b; a_s("REMINDER "); a_t(); *wp = 0; chat_line(b, INV);
        chat_text(pz->rem_txt[0] ? pz->rem_txt : "TIME IS UP!");
        draw_status("REMINDER!");
    }
}
static void tick_ui(void) {
    if (tick_clock()) {
        draw_clock(); check_rem();
        if (idle_fn && ++idle_n >= idle_limit) { idle_n = 0; idle_fn(); }
    }
}
static uc ask_yn(void) {                    /* waits for Y or N */
    uc k;
    for (;;) {
        tick_ui();
        k = key_in();
        if (k == 'Y') return 1;
        if (k == 'N') return 0;
    }
}
static uc in0 = 17, inrows = 4;
static void clear_input(void) { uc i; for (i = 0; i < inrows; i++) fillr(in0 + i, 0); }
static void draw_input(const char *buf, uc n) {
    uc i;
    clear_input();
    *cell(in0, 0) = zc('>');
    for (i = 0; i < n; i++) *cell(in0 + (i + 2) / 32, (i + 2) % 32) = zc(buf[i]);
    *cell(in0 + (n + 2) / 32, (n + 2) % 32) = INV;           /* cursor */
}
static void read_line(char *buf, uc max) {
    uc n = 0, k;
    buf[0] = 0; draw_input(buf, 0);
    for (;;) {
        tick_ui();
        k = key_in();
        if (!k) continue;
        idle_n = 0;
        if (k == 13) break;
        if (k == 12) { if (n) n--; }
        else if (k >= 27 && k <= 30) { temperature = pz->temp = k - 26; draw_status("READY"); }
        else if (k >= 32 && k < 127 && n < max) buf[n++] = k;
        buf[n] = 0;
        draw_input(buf, n);
    }
    clear_input();
}
static void header(const char *who, uc tok, uc with_time) {
    wp = b; a_s(who); a_c(' '); a_t(); a_s(" TOK "); a_u(tok);
    if (with_time) a_x(last_tn);
    *wp = 0;
    chat_line(b, INV);
}
static uc quit;                              /* set when a pack request is pending: main returns to the BASIC loader lines */
static void request(uc code, const char *carry) {
    pz->req = code; pz_save();
    strncpy(pz->carry, carry, 47); pz->carry[47] = 0;
    quit = 1;
}
static int find(const char *h, const char *n) {   /* case-insensitive substring search: index or -1 */
    uc i, j;
    for (i = 0; h[i]; i++) {
        for (j = 0; n[j]; j++) { uc c = h[i + j]; if (c >= 'a' && c <= 'z') c -= 32; if (c != n[j]) break; }
        if (!n[j]) return i;
    }
    return -1;
}
#define has(h, n) (find(h, n) >= 0)
static void clock_boot(void) {              /* ask for the time only when the persistent zone has none */
    static char t[6];
    lastf = frames();
    if (pz->clk) return;
    draw_status("SET CLOCK");
    chat_text("TYPE THE TIME AS HHMM (E.G. 1349) AND PRESS ENTER.");
    read_line(t, 4);
    if (t[3] >= '0' && t[3] <= '9') {
        hh = (t[0] - '0') * 10 + (t[1] - '0'); mm = (t[2] - '0') * 10 + (t[3] - '0');
        if (hh > 23) hh = 0;
        if (mm > 59) mm = 0;
    }
    ss = 0; acc = 0; pz->clk = 1; pz_save();
}
