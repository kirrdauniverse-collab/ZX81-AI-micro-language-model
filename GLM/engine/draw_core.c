/* draw_core.c - tiny scene drawing engine for the DRAW pack: 64x32 pixels = 32x16 cells of 2x2 block graphics.
   Shapes are kept in a scene list so they can be made bigger/smaller and placed relative to the last one. Needs has(). */
#define PW 64
#define PH 32
static u8 bmp[256];
struct obj { u8 t, x, y, s; };
static struct obj sc[8];
static uc nobj, dseed = 1;
static const char *dmsg;
static uc drnd(void) { dseed = dseed * 25173u + 13849u; return dseed >> 8; }
static void plot(int x, int y) {
    uc i;
    if (x < 0 || x >= PW || y < 0 || y >= PH) return;
    i = y * PW + x; bmp[i >> 3] |= 1 << (i & 7);
}
static uc getp(uc x, uc y) { uc i = y * PW + x; return (bmp[i >> 3] >> (i & 7)) & 1; }
static void line(int x0, int y0, int x1, int y1) {          /* Bresenham */
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy, e2;
    for (;;) {
        plot(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * e;
        if (e2 >= dy) { e += dy; x0 += sx; }
        if (e2 <= dx) { e += dx; y0 += sy; }
    }
}
static void circle(int cx, int cy, int r) {                  /* midpoint circle */
    int x = r, y = 0, e = 1 - r;
    while (x >= y) {
        plot(cx + x, cy + y); plot(cx - x, cy + y); plot(cx + x, cy - y); plot(cx - x, cy - y);
        plot(cx + y, cy + x); plot(cx - y, cy + x); plot(cx + y, cy - x); plot(cx - y, cy - x);
        y++;
        if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
    }
}
static void rect(int x0, int y0, int x1, int y1) { line(x0, y0, x1, y0); line(x1, y0, x1, y1); line(x1, y1, x0, y1); line(x0, y1, x0, y0); }
static void draw_obj(struct obj *o) {
    int x = o->x, y = o->y, s = o->s, h = s * 3 / 4;
    switch (o->t) {
    case 1: circle(x, y, s); break;
    case 2: rect(x - s, y - h, x + s, y + h); break;
    case 3: rect(x - s, y - s, x + s, y + s); break;
    case 4: line(x - s, y + h, x + s, y - h); break;
    case 5: line(x, y - s, x - s, y + h); line(x - s, y + h, x + s, y + h); line(x + s, y + h, x, y - s); break;
    }
}
static void redraw(void) { uc i; memset(bmp, 0, sizeof bmp); for (i = 0; i < nobj; i++) draw_obj(&sc[i]); }
static void render(u8 *out) {                                /* 16 rows x 32 cells; code = quadrant mask, safe ZX81 cells only */
    uc r, c, m;
    for (r = 0; r < 16; r++)
        for (c = 0; c < 32; c++) {
            m = getp(2 * c, 2 * r) | getp(2 * c + 1, 2 * r) << 1 | getp(2 * c, 2 * r + 1) << 2 | getp(2 * c + 1, 2 * r + 1) << 3;
            *out++ = m < 8 ? m : 128 + (15 - m);              /* mask 8..15 = inverse of the complementary quadrants */
        }
}
static uc cmd(const char *t, uc sd) {                        /* returns 1 when the user wants to go back to the base pack */
    uc ty = 0, s = 0, i, nx = 0;
    int x, y;
    struct obj *o;
    dseed = sd | 1; dmsg = "OK.";
    if (has(t, "BACK") || has(t, "EXIT") || has(t, "QUIT") || has(t, "PICO")) return 1;
    if (has(t, "CLEAR") || has(t, "ERASE") || has(t, "CLS")) { nobj = 0; redraw(); dmsg = "CLEARED."; return 0; }
    if (has(t, "BIGGER") || has(t, "LARGER") || has(t, "SMALLER")) {
        if (!nobj) { dmsg = "NOTHING TO CHANGE YET."; return 0; }
        o = &sc[nobj - 1];
        o->s = has(t, "SMALLER") ? (o->s > 3 ? o->s * 2 / 3 : 2) : (o->s < 14 ? o->s * 3 / 2 + 1 : 15);
        redraw(); dmsg = "DONE."; return 0;
    }
    if (has(t, "CIRCLE") || has(t, "ROUND") || has(t, "BALL")) ty = 1;
    else if (has(t, "SQUARE")) ty = 3;
    else if (has(t, "BOX") || has(t, "RECT")) ty = 2;
    else if (has(t, "LINE")) ty = 4;
    else if (has(t, "TRIANGLE")) ty = 5;
    if (!ty) { dmsg = "I CAN DRAW: CIRCLE, BOX, SQUARE, LINE, TRIANGLE."; return 0; }
    for (i = 0; t[i]; i++) if (t[i] >= '0' && t[i] <= '9') { s = t[i] - '0'; if (t[i + 1] >= '0' && t[i + 1] <= '9') s = s * 10 + t[i + 1] - '0'; break; }
    if (!s) {
        s = 4 + drnd() % 7;
        if (has(t, "BIG") || has(t, "LARGE")) s = s * 3 / 2 + 2; else if (has(t, "SMALL") || has(t, "TINY") || has(t, "LITTLE")) s = s / 2 + 1;
    }
    if (s < 2) s = 2;
    if (s > 15) s = 15;
    if (nobj == 8) { memmove(sc, sc + 1, 7 * sizeof(struct obj)); nobj = 7; }
    if ((has(t, "NEXT") || has(t, "BESIDE") || has(t, "NEAR")) && nobj) { o = &sc[nobj - 1]; x = o->x + o->s + s + 2; y = o->y; nx = 1; }
    if (!nx) {
        x = has(t, "LEFT") ? PW / 4 : has(t, "RIGHT") ? 3 * PW / 4 : (has(t, "CENTER") || has(t, "MIDDLE")) ? PW / 2 : -1;
        y = (has(t, "TOP") || has(t, "UP")) ? PH / 4 : (has(t, "BOTTOM") || has(t, "DOWN")) ? 3 * PH / 4 : -1;
        if (x < 0) x = (y < 0) ? s + 1 + drnd() % (PW - 2 * s - 1) : PW / 2;      /* nothing said: random place */
        if (y < 0) y = (x == PW / 2 || has(t, "LEFT") || has(t, "RIGHT")) ? PH / 2 : s + 1 + drnd() % (PH - 2 * s - 1);
    }
    if (x < s) x = s;
    if (x > PW - 1 - s) x = PW - 1 - s;
    if (y < s) y = s;
    if (y > PH - 1 - s) y = PH - 1 - s;
    sc[nobj].t = ty; sc[nobj].x = x; sc[nobj].y = y; sc[nobj].s = s; nobj++;
    redraw();
    dmsg = ty == 1 ? "DREW A CIRCLE." : ty == 2 ? "DREW A BOX." : ty == 3 ? "DREW A SQUARE." : ty == 4 ? "DREW A LINE." : "DREW A TRIANGLE.";
    return 0;
}
