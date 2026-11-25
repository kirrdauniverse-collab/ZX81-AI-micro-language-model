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
static uc isd(char c) { return c >= '0' && c <= '9'; }
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
typedef signed char s8;
/* objects as primitive lists in 1/8 units of the size: op (1 line, 2 circle x,y,r, 3 rect, 4 plot), a, b, c, d ... 0 */
static const s8 objd[] = {
 /* house  */ 3,-8,-1,8,8, 1,-9,-1,0,-8, 1,0,-8,9,-1, 1,-9,-1,9,-1, 3,-2,3,2,8, 3,3,1,6,4, 0,
 /* car    */ 3,-8,0,8,5, 1,-5,0,-3,-4, 1,-3,-4,3,-4, 1,3,-4,5,0, 2,-4,5,2,0, 2,4,5,2,0, 0,
 /* jupiter*/ 2,0,0,8,0, 1,-7,-4,7,-4, 1,-8,-1,8,-1, 1,-8,2,8,2, 1,-6,5,6,5, 2,3,3,1,0, 0,
 /* tree   */ 3,-1,3,1,8, 2,0,-2,5,0, 0,
 /* sun    */ 2,0,0,4,0, 1,0,-8,0,-6, 1,0,6,0,8, 1,-8,0,-6,0, 1,6,0,8,0, 1,-6,-6,-4,-4, 1,4,4,6,6, 1,6,-6,4,-4, 1,-4,4,-6,6, 0,
 /* moon   */ 2,0,0,7,0, 2,3,-1,6,0, 0,
 /* star   */ 1,0,-8,5,6, 1,5,6,-8,-2, 1,-8,-2,8,-2, 1,8,-2,-5,6, 1,-5,6,0,-8, 0,
 /* heart  */ 2,-3,-2,3,0, 2,3,-2,3,0, 1,-6,-1,0,7, 1,6,-1,0,7, 0,
 /* ship   */ 1,-8,2,8,2, 1,-8,2,-5,6, 1,-5,6,5,6, 1,5,6,8,2, 1,0,2,0,-8, 1,0,-8,6,0, 1,6,0,0,0, 0,
 /* rocket */ 3,-2,-5,2,4, 1,-2,-5,0,-8, 1,0,-8,2,-5, 1,-2,1,-5,5, 1,2,1,5,5, 2,0,-1,1,0, 1,0,4,0,7, 0,
 /* face   */ 2,0,0,8,0, 4,-3,-2,0,0, 4,3,-2,0,0, 1,-4,3,-2,5, 1,-2,5,2,5, 1,2,5,4,3, 0,
 /* snowman*/ 2,0,-5,3,0, 2,0,2,5,0, 4,-1,-6,0,0, 4,1,-6,0,0, 0,
 /* flower */ 2,0,-7,2,0, 2,0,-1,2,0, 2,-3,-4,2,0, 2,3,-4,2,0, 4,0,-4,0,0, 1,0,0,0,8, 1,0,5,4,3, 0,
 /* saturn */ 2,0,0,5,0, 1,-9,2,9,-2, 0 };
static const char *const nn[] = { "CIRCLE", "ROUND", "BALL", "SQUARE", "BOX", "RECT", "LINE", "TRIANGLE", "HOUSE", "HOME", "CAR", "JUPITER", "PLANET", "TREE", "SUN", "MOON",
    "STAR", "HEART", "SHIP", "BOAT", "ROCKET", "FACE", "SMILEY", "SNOWMAN", "FLOWER", "SATURN" };
static const u8 nid[] = { 1, 1, 1, 3, 2, 2, 4, 5, 10, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 18, 19, 20, 20, 21, 22, 23 };
static const char *const onm[] = { "CIRCLE", "BOX", "SQUARE", "LINE", "TRIANGLE" };
static const s8 *objp(uc k) { const s8 *p = objd; while (k--) { while (*p) p += 5; p++; } return p; }
static void draw_obj(struct obj *o) {
    int x = o->x, y = o->y, s = o->s, h = s * 3 / 4;
    const s8 *p;
    if (o->t >= 10) {
        for (p = objp(o->t - 10); *p; p += 5) {
            int a = x + p[1] * s / 8, b = y + p[2] * s / 8, c = x + p[3] * s / 8, d = y + p[4] * s / 8;
            if (*p == 1) line(a, b, c, d);
            else if (*p == 2) circle(a, b, p[3] * s / 8 < 1 ? 1 : p[3] * s / 8);
            else if (*p == 3) rect(a, b, c, d);
            else plot(a, b);
        }
        return;
    }
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
    uc ty = 0, s = 0, i, ref = 0, rel = 0;
    int x, y, f, bf = 99, f2, b2 = 99;
    struct obj *o = 0;
    dseed = sd | 1; dmsg = "OK.";
    if (has(t, "BACK") || has(t, "EXIT") || has(t, "QUIT") || has(t, "PICO")) return 1;
    if (has(t, "CLEAR") || has(t, "ERASE") || has(t, "CLS")) { nobj = 0; redraw(); dmsg = "CLEARED."; return 0; }
    if (has(t, "UNDO") || has(t, "REMOVE") || has(t, "DELETE")) { if (nobj) nobj--; redraw(); dmsg = "REMOVED THE LAST ONE."; return 0; }
    if (has(t, "BIGGER") || has(t, "LARGER") || has(t, "SMALLER")) {
        if (!nobj) { dmsg = "NOTHING TO CHANGE YET."; return 0; }
        o = &sc[nobj - 1];
        o->s = has(t, "SMALLER") ? (o->s > 3 ? o->s * 2 / 3 : 2) : (o->s < 14 ? o->s * 3 / 2 + 1 : 15);
        redraw(); dmsg = "DONE."; return 0;
    }
    for (i = 0; i < sizeof nid; i++) {               /* the first name in the sentence is what to draw, a later one is what it refers to */
        f = find(t, nn[i]);
        if (f < 0) continue;
        if (f < bf) { f2 = bf; bf = f; if (ty) { ref = ty; b2 = f2; } ty = nid[i]; }
        else if (f < b2 && nid[i] != ty) { b2 = f; ref = nid[i]; }
    }
    if (!ty) { dmsg = "I CAN DRAW: CIRCLE, BOX, HOUSE, CAR, TREE, SUN, MOON, STAR, HEART, SHIP, ROCKET, FACE, FLOWER, JUPITER..."; return 0; }
    for (i = 0; t[i]; i++) if (isd(t[i])) { s = t[i] - '0'; if (isd(t[i + 1])) s = s * 10 + t[i + 1] - '0'; break; }
    if (!s) {
        s = (ty >= 10 ? 6 : 4) + drnd() % 7;
        if (has(t, "BIG") || has(t, "LARGE")) s = s * 3 / 2 + 2; else if (has(t, "SMALL") || has(t, "TINY") || has(t, "LITTLE")) s = s / 2 + 1;
    }
    if (s < 2) s = 2;
    if (s > 15) s = 15;
    if (nobj == 8) { memmove(sc, sc + 1, 7 * sizeof(struct obj)); nobj = 7; }
    if (nobj) {                                       /* relative to a shape that is already there */
        rel = (has(t, "NEXT") || has(t, "BESIDE") || has(t, "NEAR") || has(t, "RIGHT OF")) ? 1 : has(t, "LEFT OF") ? 2 :
              (has(t, "ABOVE") || has(t, "ON TOP")) ? 3 : (has(t, "BELOW") || has(t, "UNDER")) ? 4 : 0;
        if (rel) {
            o = &sc[nobj - 1];
            if (ref) for (i = nobj; i--;) if (sc[i].t == ref) { o = &sc[i]; break; }
            x = o->x; y = o->y + o->s - s;            /* bottoms line up */
            if (rel == 1) x = o->x + o->s + s + 2;
            else if (rel == 2) x = o->x - o->s - s - 2;
            else if (rel == 3) y = o->y - o->s - s - 1;
            else y = o->y + o->s + s + 1;
        }
    }
    if (!rel) {
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
    dmsg = ty < 6 ? "OK, DRAWN." : "OK, THERE IT IS.";
    return 0;
}
