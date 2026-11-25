/* Pico AI DRAW pack: draws shapes with block graphics. Loaded on request from the base pack (carry text = the user's sentence). */
#include "zx81_ui.c"
#include "draw_core.c"
u8 temperature = 2;
static char ibuf[94];
static void show(void) {
    u8 codes[512]; uc r;
    render(codes);
    for (r = 0; r < 16; r++) memcpy(cell(1 + r, 0), codes + r * 32, 32);
}
static void run(const char *t) {
    uc back = cmd(t, frames());
    if (back) { request(6, ""); dmsg = "BACK TO PICO..."; }
    show();
    fillr(23, 0); text_at(23, 0, dmsg, 0);
}
int main(void) {
    uc r;
    pz_init();
    if (!pz_ok) { scr_init(); text_at(10, 0, "RUN THE LOADER FIRST (RAMTOP)", 0); for (;;) ; }
    in0 = 18; inrows = 3;
    scr_init();
    fillr(0, INV); text_at(0, 0, "PICO AI - DRAW PACK V0.2", INV);
    fillr(17, INV); text_at(17, 0, "HOUSE CAR TREE SUN STAR HEART", INV);
    text_at(22, 0, "BIG SMALL LEFT NEXT ABOVE UNDO", 0);
    for (r = 0; r < 16; r++) fillr(1 + r, 0);
    clock_boot();
    draw_status("DRAW READY. TYPE BACK TO LEAVE");
    if (pz->carry[0]) { strcpy(ibuf, pz->carry); pz->carry[0] = 0; run(ibuf); }
    while (!quit) {
        read_line(ibuf, 90);
        if (ibuf[0]) run(ibuf);
    }
    return 0;
}
