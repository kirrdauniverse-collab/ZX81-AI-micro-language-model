/* ZX81 front end for Pico AI v0.2 - own display layer (written compactly: sccz80 likes 16-bit variables).
   Screen: row 0 title | rows 1-12 chat (scrolls up from the bottom) | row 13 memory bar | rows 14-18 storage |
           rows 19-20 input | row 21 inverse status | row 22 key hints | row 23 state.
   Rule: display cells are only 0..63 or 128..191 - never 0x76 or any code with bit 6 set (that ends the row). */
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned int uc;
void pico_reply(const char *line, uc sd);
uc pico_count(const char *s);
u8 *pico_ctx(void);
extern u8 temperature, ntok_in, ntok_out;
extern char rbuf[];

#define INV 128
#define CH0 1
#define CH1 12
#define ST0 14
static u8 *dfile;                           /* the expanded display file the .P image already contains: 1 + 24 rows * 33 */
static u8 *cell(uc r, uc c) { return dfile + 1 + r * 33 + c; }
static void wait_frame(void) {
#asm
    halt
#endasm
}
static const char pun[] = ".,?:;-+*/=()<>\"$";
static const u8 pcd[] = {27, 26, 15, 14, 25, 22, 21, 23, 24, 20, 16, 17, 19, 18, 11, 13};
static uc zc(uc c) {                        /* ASCII -> ZX81 character code (0..63) */
    uc i;
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
    memmove(cell(CH0, 0), cell(CH0 + 1, 0), (CH1 - CH0) * 33);
    fillr(CH1, inv);
    text_at(CH1, 0, s, inv);
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
static uc hh, mm, ss, acc, lastf;
static void a_t(void) { a_2(hh); a_c(':'); a_2(mm); a_c(':'); a_2(ss); }
static void a_x(uc tn) { a_c(' '); a_u(tn / 10); a_c('.'); a_u(tn % 10); a_c('S'); }
static uc frames(void) { return 0xFFFF - *(uc *)16436; }     /* FRAMES counts down on the ZX81 */
static uc tick_clock(void) {                /* returns 1 when the second changed */
    uc now = frames(), ch = 0;
    acc += now - lastf; lastf = now;
    while (acc >= 50) { acc -= 50; ch = 1; if (++ss == 60) { ss = 0; if (++mm == 60) { mm = 0; if (++hh == 24) hh = 0; } } }
    return ch;
}

/* ---- info panel ---- */
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
static void draw_static(void) {
    uc r;
    fillr(0, INV); text_at(0, 0, "PICO AI - ZX81 MICRO LLM V0.3", INV);
    fillr(13, INV); text_at(13, 0, "MEMORY", INV);
    for (r = ST0; r <= 18; r++) fillr(r, 0);
    text_at(22, 0, "SHIFT+1..4 TEMP  SHIFT+0 DEL", 0);
}
static void store_context(void) {           /* bytes shown as hex cells: 28 + nibble is always a safe cell code */
    u8 *x = pico_ctx(); uc i, v, k;
    for (i = 0; i < 33; i++) {
        v = i ? x[i - 1] : temperature;
        k = i * 2;                          /* cell index; every cell is addressed per row so the 0x76 row ends stay intact */
        *cell(ST0 + k / 32, k % 32) = 28 + (v >> 4);
        k++;
        *cell(ST0 + k / 32, k % 32) = 28 + (v & 15);
    }
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
static void draw_input(const char *buf, uc n) {
    uc i;
    fillr(19, 0); fillr(20, 0);
    *cell(19, 0) = zc('>');
    for (i = 0; i < n; i++) *cell(19 + (i + 2) / 32, (i + 2) % 32) = zc(buf[i]);
    *cell(19 + (n + 2) / 32, (n + 2) % 32) = INV;           /* cursor */
}
static void read_line(char *buf, uc max) {
    uc n = 0, k;
    buf[0] = 0; draw_input(buf, 0);
    for (;;) {
        if (tick_clock()) draw_clock();
        k = key_in();
        if (!k) continue;
        if (k == 13) break;
        if (k == 12) { if (n) n--; }
        else if (k >= 27 && k <= 30) { temperature = k - 26; draw_status("READY"); }
        else if (k >= 32 && k < 127 && n < max) buf[n++] = k;
        buf[n] = 0;
        draw_input(buf, n);
    }
    fillr(19, 0); fillr(20, 0);
}
static void header(const char *who, uc tok, uc with_time) {
    wp = b; a_s(who); a_c(' '); a_t(); a_s(" TOK "); a_u(tok);
    if (with_time) a_x(last_tn);
    *wp = 0;
    chat_line(b, INV);
}
static uc t0;
static void answer(const char *q) {          /* run the model, time it, show the reply */
    t0 = frames();
    pico_reply(q, *(uc *)16436 | 1);
    last_tn = (frames() - t0) / 5; tok_o = ntok_out;
    tick_clock();
    header("PICO", tok_o, 1); chat_text(rbuf);
    store_context(); draw_status("READY");
}
static char line[62];
int main(void) {
    scr_init(); draw_static();
    lastf = frames();
    draw_status("SET CLOCK");
    chat_text("TYPE THE TIME AS HHMM (E.G. 1349) AND PRESS ENTER.");
    read_line(line, 4);
    if (line[3] >= '0' && line[3] <= '9') {
        hh = (line[0] - '0') * 10 + (line[1] - '0'); mm = (line[2] - '0') * 10 + (line[3] - '0');
        if (hh > 23) hh = 0;
        if (mm > 59) mm = 0;
    }
    tick_clock();
    answer("hello");
    for (;;) {
        read_line(line, 60);
        if (!line[0]) continue;
        tick_clock();
        tok_i = pico_count(line);
        header("YOU", tok_i, 0); chat_text(line);
        draw_status("THINKING...");
        answer(line);
    }
    return 0;
}
