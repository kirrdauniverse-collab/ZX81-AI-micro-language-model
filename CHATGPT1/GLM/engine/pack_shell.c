/* Generic text pack: title bar, 16-row chat, 4-row input, "BACK" returns to the base pack. The pack provides handle() and PACK_TITLE. */
static uc wcount(const char *s) {
    uc n = 0, in = 0;
    for (; *s; s++) { if (*s == ' ') in = 0; else if (!in) { in = 1; n++; } }
    return n;
}
static char ibuf[124];
static uc t0;
static void turn(const char *in) {
    tick_clock(); tok_i = wcount(in);
    header(uname(), tok_i, 0); chat_text(in);
    t0 = frames(); rseed = t0 | 1;
    if (has(in, "BACK") || has(in, "EXIT") || has(in, "QUIT")) { rcl(); cat("BACK TO THE CHAT..."); request(6, ""); }
    else handle(in);
    last_tn = (frames() - t0) / 5; tok_o = wcount(rb);
    header(bname(), tok_o, 1); chat_text(rb);
    draw_status("READY");
}
int main(void) {
    pz_init();
    if (!pz_ok) { scr_init(); text_at(10, 0, "RUN THE LOADER FIRST (RAMTOP)", 0); for (;;) ; }
    ch1 = 16; in0 = 17; inrows = 4;
    scr_init();
    fillr(0, INV); text_at(0, 0, PACK_TITLE, INV);
    text_at(22, 0, "TYPE BACK TO RETURN TO PICO", 0);
    clock_boot(); draw_status("READY");
    if (pz->carry[0]) { strcpy(ibuf, pz->carry); pz->carry[0] = 0; turn(ibuf); }
    while (!quit) { read_line(ibuf, 120); if (ibuf[0]) turn(ibuf); }
    return 0;
}
