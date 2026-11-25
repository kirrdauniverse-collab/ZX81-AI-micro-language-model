/* Pico AI base pack: chat with the small language model. Bigger skills are separate packs on the tape (asked with Y/N). */
#include "zx81_ui.c"
void pico_reply(const char *line, uc sd);
uc pico_count(const char *s);
u8 *pico_ctx(void);
uc pico_known(const char *w);
const char *pico_word(uc id);
extern const char *pico_alias;
extern u8 temperature, ntok_in, ntok_out;
extern char rbuf[];

static void draw_static(void) {
    uc r;
    fillr(0, INV); text_at(0, 0, "PICO AI - ZX81 MICRO LLM V0.5", INV);
    fillr(13, INV); text_at(13, 0, "MEMORY", INV);
    for (r = ST0; r <= 16; r++) fillr(r, 0);
    text_at(22, 0, "SHIFT+1..4 TEMP  SHIFT+0 DEL", 0);
}
static void store_context(void) {           /* bytes shown as hex cells: 28 + nibble is always a safe cell code */
    u8 *x = pico_ctx(); uc i, v, k;
    for (i = 0; i < 33; i++) {
        v = i ? x[i - 1] : temperature;
        k = i * 2;                          /* every cell is addressed per row so the 0x76 row ends stay intact */
        *cell(ST0 + k / 32, k % 32) = 28 + (v >> 4);
        k++;
        *cell(ST0 + k / 32, k % 32) = 28 + (v & 15);
    }
}
/* thinking bar (row 23): what the model sees, which sentence it picked, and how many words it can choose from at each step */
void think_cb(uc ph, uc x, uc y) {
    uc id; u8 *c;
    wp = b;
    if (ph == 1) {
        a_s("SEEN");
        c = pico_ctx();
        for (id = 2; id < 254 && wp < b + 16; id++)
            if (c[id >> 3] & (1 << (id & 7))) { a_c(' '); a_s(pico_word(id)); }
    } else if (ph == 2) { a_s("MATCH SCORE "); a_u(x); }
    else { a_s("WORD "); a_u(x); a_s("  OPTIONS "); a_u(y); }
    *wp = 0; if (wp - b > 32) b[32] = 0;
    fillr(23, 0); text_at(23, 0, b, 0);
}
static void on_idle(void) {                  /* nobody typed for a while: ask something */
    header(bname(), 0, 0);
    if (pz->names[pz->cur][0]) { wp = b; a_s(pz->names[pz->cur]); a_c(','); *wp = 0; chat_text(b); }
    chat_text("ARE YOU STILL THERE? TELL ME SOMETHING NEW.");
}
/* which pack does a sentence need? 0 = none. Keyword groups; '_' = space; groups marked '+' need a writing verb (write, make, ...) */
static const char kw[] =
    "DRAW PAINT|"
    "+STORY TALE|"
    "+POEM VERSE|"
    "+PROGRAM CODE|"
    "RECIPE COOK|"
    "SORT MEANS CALCULAT MY_NAME_IS CALL_ME CALL_YOU SET_TIME SET_DATE REMIND WHAT_DAY WHAT_DATE WHAT_TIME THE_DATE THE_TIME WHO_AM_I GREET";
static const char *const pnames[6] = { "DRAW", "STORY", "POEM", "CODE", "FOOD", "UTIL" };
static const char *const topics[6] = { "DRAWING", "STORY WRITING", "POEMS", "PROGRAMMING", "RECIPES", "TOOLS: TIME, DATE, NAMES, MATHS" };
static const u8 pcodes[6] = { 1, 3, 4, 5, 7, 2 };
static uc is_math(const char *l) {
    uc i, j;
    for (i = 0; l[i]; i++)
        if (l[i] >= '0' && l[i] <= '9') {
            for (j = i + 1; l[j] == ' ' || (l[j] >= '0' && l[j] <= '9'); j++) ;
            if (l[j] == '+' || l[j] == '*' || l[j] == '/' || l[j] == '-' || l[j] == 'x' || l[j] == 'X') return 1;
        }
    return 0;
}
static const char *packname, *topic;
static uc wants(const char *l) {
    const char *p = kw; char w[12]; uc g = 0, n, mk, need, hit;
    mk = has(l, "WRITE") || has(l, "COMPOSE") || has(l, "CREATE") || has(l, "MAKE") || has(l, "GIVE ME") || has(l, "TELL ME A") || has(l, "SHOW ME");
    for (; g < 6; g++) {
        need = (*p == '+'); if (need) p++;
        hit = 0;
        while (*p && *p != '|') {
            for (n = 0; *p && *p != ' ' && *p != '|'; n++, p++) w[n] = (*p == '_') ? ' ' : *p;
            w[n] = 0;
            if (*p == ' ') p++;
            if (find(l, w) >= 0) hit = 1;
        }
        if (*p == '|') p++;
        if (g == 5 && is_math(l)) hit = 1;
        if (hit && (!need || mk)) { packname = pnames[g]; topic = topics[g]; return pcodes[g]; }
    }
    return 0;
}
static uc t0;
static void answer(const char *q) {          /* run the model, time it, show the reply */
    t0 = frames();
    pico_reply(q, *(uc *)16436 | 1);
    last_tn = (frames() - t0) / 5; tok_o = ntok_out;
    tick_clock();
    header(bname(), tok_o, 1); chat_text(rbuf);
    store_context(); draw_status("READY");
}
static char line[124];
int main(void) {
    uc code;
    pz_init();
    pico_alias = pz->learn[0];
    if (!pz_ok) { scr_init(); text_at(10, 0, "RUN THE LOADER FIRST (RAMTOP)", 0); for (;;) ; }
    scr_init(); draw_static();
    clock_boot();
    idle_limit = 150; idle_fn = on_idle;
    answer("hello");
    while (!quit) {
        read_line(line, 120);
        if (!line[0]) continue;
        tick_clock();
        tok_i = pico_count(line);
        header(uname(), tok_i, 0); chat_text(line);
        code = wants(line);
        if (code) {                          /* a skill that lives in its own pack */
            if (!pz_ok) { header(bname(), 0, 0); chat_text("I CANNOT LOAD PACKS: RAMTOP IS NOT SET. RUN THE LOADER FIRST."); continue; }
            header(bname(), 0, 0);
            chat_text("THAT IS A SEPARATE TOPIC:"); chat_text(topic);
            wp = b; a_s("LOAD THE "); a_s(packname); a_s(" PACK NOW? Y/N"); *wp = 0;
            chat_text(b);
            draw_status("LOAD PACK? Y/N");
            header(bname(), 0, 0);
            if (ask_yn()) { request(code, line); chat_text("LOADING..."); }
            else { chat_text("OK, WE STAY HERE."); draw_status("READY"); }
            continue;
        }
        draw_status("THINKING...");
        answer(line);
    }
    return 0;
}
