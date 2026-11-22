/* boot16.c - retro-tinyllm v0.5 BOOT block: logo, manual, profile.
 *
 * This is a SEPARATE small app (~2.5 KB, no model, no chat engine)
 * that occupies cassette block 1 (RT-BOOT01). Flow:
 *
 *   1. draw the "ZX81 AI" logo with plain display-safe characters
 *      (letters, '=', '.', '/' - the D-file holds only codes the ROM
 *      renders as real glyphs; never $76, never >= $40 - the user's
 *      freeze rule)
 *   2. ask whether the user wants the manual; if yes, print it
 *   3. ask date, time, name and gender
 *   4. pack the profile into the D-file work rows (rows 0-2, the same
 *      6-bit 'RT2' format the base block reads back)
 *   5. return 1, so BASIC executes
 *      2 IF USR 16514 = 1 THEN LOAD "RT-BASE16"
 *      and the tape hands over to the base block
 *
 * The profile survives the block swap because the D-file sits ABOVE
 * every image end and nothing in the chain touches rows 0-2.
 *
 * Coding style note: sccz80 1.8, 8-bit math, short strings.
 */
#include <string.h>
#include <stddef.h>
#include "rt_hal.h"
#include "rt_config.h"

/* ---- session state in the D-file work tier, rows 0-2 ----
 * 57 payload bytes packed 3 -> 4 into 96 cells of 6 bits:
 * 'R','T','2' magic, version, name[8], gender, year, stamp(2: days
 * since 2000), hour, min, hist[8], hist_n, seed(4), topic, pending,
 * temp, reason, events[10][2], evcount, xor checksum.
 * The BASE block unpacks the exact same layout. */
#define ST_ROW   0
#define ST_CELLS 96
#define ST_BYTES 72

static char bname[9];
static char bgen;        /* 'f' / 'n' / 0 */
static int byear, bdoy, bhour, bmin;
static unsigned int bstamp;    /* days since 2000-01-01 */

/* ---- tiny printers ---- */

static void say(const char *s)
{
    while (*s)
        hal_putchar(*s++);
}

static void say_int(int v)
{
    char b[6];
    int n = 0;

    if (v < 0) {
        hal_putchar('-');
        v = -v;
    }
    do {
        b[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n)
        hal_putchar(b[--n]);
}

/* ---- input: read one line (max max-1 chars) ---- */

static void getline(char *buf, int max)
{
    int n = 0;

    for (;;) {
        int c = hal_getchar();
        if (c == -1)
            continue;
        if (c == '\n') {
            buf[n] = 0;
            return;
        }
        if (c < ' ' || c > 0x7F)
            continue;                 /* shifted keys, graphics */
        if (c >= 'A' && c <= 'Z')
            c += 32;                  /* the chat is lowercase */
        if (n < max - 1)
            buf[n++] = (char)c;
        hal_putchar(c);
    }
}

/* read a decimal number in [lo,hi], re-asking on garbage */
static int getnum(int lo, int hi)
{
    char buf[8];

    for (;;) {
        int v = 0, i;

        getline(buf, 6);
        for (i = 0; buf[i]; i++) {
            if (buf[i] < '0' || buf[i] > '9') {
                v = lo - 1;
                break;
            }
            v = v * 10 + buf[i] - '0';
        }
        if (v >= lo && v <= hi)
            return v;
        say("??? (");
        say_int(lo);
        say("-");
        say_int(hi);
        say(")\n");
    }
}

/* ---- the logo: plain display-safe chars only ---- */

static const char *logo[5] = {
    "=...= =...= .====. =...= =...=",
    "=.==. =...=    =   =...= =...=",
    "=.==. =...=   =    =...= =...=",
    "=..=. =...=  =     =...= =...=",
    "=.==.  ===  .====.  ===.  ===.",
};

static void logo_draw(void)
{
    int i;

    for (i = 0; i < 3; i++)
        say("\n");
    for (i = 0; i < 5; i++) {
        say(logo[i]);
        say("\n");
    }
    say("   retro-tinyllm 16k  v0.5.4\n");
    say("==============================\n");
}

/* ---- date helpers ---- */

static const char mdays[12] = {31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};

static int doy_calc(int y, int m, int d)
{
    int i, n = d;

    for (i = 0; i < m - 1; i++)
        n += mdays[i];
    if (m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        n++;
    return n;
}

/* ---- the profile pack: 3 bytes -> 4 cells of 6 bits ---- */

static rt_u8 buf[ST_BYTES];

static void pack_profile(void)
{
    int c, i;
    rt_u8 x = 0;

    for (i = 0; i < ST_BYTES; i++)
        buf[i] = 0;
    buf[0] = 'R';
    buf[1] = 'T';
    buf[2] = '2';
    buf[3] = 1;
    for (i = 0; i < 8 && bname[i]; i++)
        buf[4 + i] = (rt_u8)bname[i];
    buf[12] = (rt_u8)bgen;
    buf[13] = (rt_u8)(byear - 2000);
    buf[14] = (rt_u8)(bstamp >> 8);
    buf[15] = (rt_u8)(bstamp & 255);
    buf[16] = (rt_u8)bhour;
    buf[17] = (rt_u8)bmin;
    /* hist/seed/topic stay zero: the base block starts fresh */
    buf[55] = 0;                  /* evcount */
    buf[56] = 0;                  /* save flag */
    for (i = 0; i < ST_BYTES - 1; i++)
        x = (rt_u8)(x ^ buf[i]);
    buf[ST_BYTES - 1] = x;

    /* the SAME 3->4 group packing the base block uses (24 groups,
     * cells 0-95 = D-file work rows 0-2); cell2 carries b1 bits 4-7 */
    for (c = 0; c < 24; c++) {
        rt_u8 b0 = buf[c * 3], b1 = buf[c * 3 + 1], b2 = buf[c * 3 + 2];

        hal_scratch_put(c * 4 + 0, b0 & 0x3F);
        hal_scratch_put(c * 4 + 1,
                        (rt_u8)((b0 >> 6) | ((b1 & 0x0F) << 2)));
        hal_scratch_put(c * 4 + 2,
                        (rt_u8)(((b1 >> 4) & 0x0F) | ((b2 & 0x03) << 4)));
        hal_scratch_put(c * 4 + 3, b2 >> 2);
    }
}

/* ---- the manual ---- */

static void manual(void)
{
    say("\nHOGYAN MUKODIK:\n");
    say("- irj kisbetukkel, entert nyomj\n");
    say("- /topic: temalist es valtas (szalagrol)\n");
    say("- SHIFT+1..4: hofok 1-4 (ulerese/keverese)\n");
    say("- SHIFT+2 ures soron: gondolkodo kapcsolo\n");
    say("- SHIFT+5/SHIFT+0: regen elore/vissza tekercs\n");
    say("- /save: emlekek mentese a szalagra\n");
    say("- /new: uj beszelgetes, /quit: kilepes\n");
    say("- a felso sorok: a modell kontexusa\n\n");
}

void main(void)
{
    int i, m;

    /* v0.5.4: re-assert the display after ANY entry path. The ROM's
     * tape LOAD (our multiload chain) runs with the display off (the
     * ZX81 loads with FAST), and a quickload may enter with IY unset,
     * so the crt0's early SLOW call alone is not enough: without this
     * the screen is only regenerated during getk()'s keyboard scans -
     * the "picture flashes on keypress" symptom. IY must be $4000 for
     * every ROM sysvar access (incl. the NMI display sweep itself). */
#asm
    ; -IXIY swap: assembles as LD IY,4000h = the ROM sysvar base
    ld ix, $4000
    call $0F2B           ; SLOW: enable the continuous NMI display
#endasm

    logo_draw();
    say("Ujra itt vagyok! Utmutato kell? (i/n)\n");
    for (;;) {
        int c = hal_getchar();
        if (c == -1)
            continue;
        if (c >= 'A' && c <= 'Z')
            c += 32;
        if (c == 'i') {
            manual();
            break;
        }
        if (c == 'n')
            break;
    }

    say("Datum: ev? ");
    byear = getnum(2000, 2099);
    say("honap (1-12)? ");
    m = getnum(1, 12);
    say("nap (1-31)? ");
    bdoy = doy_calc(byear, m, getnum(1, 31));
    say("Ido: ora (0-23)? ");
    bhour = getnum(0, 23);
    say("perc (0-59)? ");
    bmin = getnum(0, 59);
    say("Neved? ");
    getline(bname, 9);
    say("Nemed (f/n)? ");
    for (;;) {
        int c = hal_getchar();
        if (c == -1)
            continue;
        if (c >= 'A' && c <= 'Z')
            c += 32;
        if (c == 'f' || c == 'n') {
            bgen = (char)c;
            hal_putchar(c);
            break;
        }
    }
    say("\n");

    bstamp = (unsigned int)((byear - 2000) * 365 + bdoy);
    pack_profile();

    say("Rendben, ");
    for (i = 0; bname[i]; i++)
        hal_putchar(bname[i]);
    say("!\nMentem a beallitasokat es toltom az alapot.\n");
    say("Tekerj fel a szalagot a RT-BASE16 blokkra!\n");

    /* leave the D-file rows 0-2 packed; the base block reads them */
    return 1;      /* BASIC runs: 2 IF USR 16514 = 1 THEN LOAD "RT-BASE16" */
}
