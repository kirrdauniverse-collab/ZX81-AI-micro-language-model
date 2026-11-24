/* Pico AI base pack: chat with the small language model. Larger skills (drawing ...) are separate packs on the tape. */
#include "zx81_ui.c"
void pico_reply(const char *line, uc sd);
uc pico_count(const char *s);
u8 *pico_ctx(void);
extern u8 temperature, ntok_in, ntok_out;
extern char rbuf[];

static void draw_static(void) {
    uc r;
    fillr(0, INV); text_at(0, 0, "PICO AI - ZX81 MICRO LLM V0.4", INV);
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
static uc t0;
static void say_pico(const char *txt, uc tok) { header("PICO", tok, 0); chat_text(txt); }
static void answer(const char *q) {          /* run the model, time it, show the reply */
    t0 = frames();
    pico_reply(q, *(uc *)16436 | 1);
    last_tn = (frames() - t0) / 5; tok_o = ntok_out;
    tick_clock();
    header("PICO", tok_o, 1); chat_text(rbuf);
    store_context(); draw_status("READY");
}
static char line[124];
int main(void) {
    pz_init();
    scr_init(); draw_static();
    clock_boot();
    answer("hello");
    while (!quit) {
        read_line(line, 120);
        if (!line[0]) continue;
        tick_clock();
        tok_i = pico_count(line);
        header("YOU", tok_i, 0); chat_text(line);
        if (has(line, "DRAW") || has(line, "PAINT") || has(line, "SKETCH")) {      /* a skill that lives in its own pack */
            if (!pz_ok) { say_pico("I CANNOT LOAD PACKS: RAMTOP IS NOT SET. RUN THE LOADER FIRST.", 0); continue; }
            say_pico("THAT IS A SEPARATE TOPIC: DRAWING. LOAD THE DRAW PACK NOW? Y/N", 0);
            draw_status("LOAD DRAW? Y/N");
            if (ask_yn()) { request(1, line); say_pico("LOADING DRAW...", 0); }
            else { say_pico("OK, WE STAY HERE.", 0); draw_status("READY"); }
            continue;
        }
        draw_status("THINKING...");
        answer(line);
    }
    return 0;
}
