/* hal_zx81.c - Sinclair ZX81 16K HAL (z88dk 1.8, no stdio chain).
 *
 * 16K edition (v0.4): the engine, ONE class-E model blob and the
 * display file share a single 16K pack (see docs/T81_FORMAT.md):
 *
 *   0x4009   system variables (D_FILE=0x7B00, VARS=0x7E19, 16K RAMTOP)
 *   0x407D   line 1: 1 REM <engine code from 0x4082 = 16514>
 *   line 2   2 RAND USR VAL "16514"
 *   0x6900   model blob (RT_MODEL_BUF_ADDR, injected by t81build16.py)
 *   0x7B00   D-file (ABOVE every image end - topic loads keep it)
 *              rows 0-7  work area / scratch tier (256 x 6 bit cells)
 *                         row 0/1: live context window
 *                         rows 6-7: packed session state
 *              rows 8-23 chat console (own cursor + scroll)
 *   0x7E19   VARS 0x80 marker; stack at the 16K top ($7FFC)
 *
 * The crt0 boots SLOW (display on, FRAMES ticking - see zx81rt_crt0.asm);
 * getk() polls the ROM keyboard scan so typing works while the program
 * runs. No printf anywhere: the text layer writes glyphs straight into
 * the D-file, so the ROM's scroll can never touch our rows 0-7.
 */
#include <zx81.h>
#include <input.h>
#include "rt_hal.h"

/* --- console: ZX81 char codes in, ASCII out ---
 * Table-driven on purpose: sccz80 1.8 turns a switch into a long
 * compare chain (~550 bytes for these two maps); the tables cost 160
 * bytes of image data. The input table is the exact inverse of the
 * output table below (the v0.3 input switch had drifted from it:
 * e.g. key 0x05 rendered as '?' but decoded as '('). ZX81 letters are
 * uppercase glyphs; the chat folds them to lowercase. */
static const rt_u8 ASCII_TO_ZX81[96] = {
    /* v0.4.5: the punctuation codes were systematically wrong ('-'
     * printed as the pound-sign glyph $0C - the "strange B" the user
     * saw in "no model - bad cassette", '>' as a graphic). Every code
     * now comes from the ROM's own keyboard tables, K-UNSHIFT at $007E
     * and K-SHIFT at $00A5: '('=$10 ')'=$11 '*'=$17 '+'=$15 ','=$1A
     * '-'=$16 '.'=$1B '/'=$18 ':'=$0E ';'=$19 '<'=$13 '='=$14 '>'=$12
     * '?'=$0F '$'=$0D; digits $1C+$v; letters $26+$v. */
    /* 0x20 ' ' */ 0x00, 0x00, 0x0B, 0x00, 0x0D, 0x00, 0x00, 0x00,
    /* 0x28 '(' */ 0x10, 0x11, 0x17, 0x15, 0x1A, 0x16, 0x1B, 0x18,
    /* 0x30 '0' */ 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
    /* 0x38 '8' */ 0x24, 0x25, 0x0E, 0x19, 0x13, 0x14, 0x12, 0x0F,
    /* 0x40 '@' */ 0x00, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
    /* 0x48 'H' */ 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
    /* 0x50 'P' */ 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
    /* 0x58 'X' */ 0x3D, 0x3E, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x60 '`' */ 0x00, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
    /* 0x68 'h' */ 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
    /* 0x70 'p' */ 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
    /* 0x78 'x' */ 0x3D, 0x3E, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* The Z88DK ZX81 INKEY scanner returns one ASCII key only and
 * rejects multiple simultaneous keys. Waiting for release after each
 * accepted key gives exactly one character per physical press, even
 * in accelerated emulators. Shifted command keys are translated into
 * the app's private control codes. */
int hal_getchar(void)
{
    int k = (int)in_Inkey();

    if (k == 0)
        return -1;

    /* One press = one event. No typematic repeat in text fields. */
    in_WaitForNoKey();

    switch (k) {
    case 27: return 0x11;  /* SHIFT+1: temperature 1 */
    case 28: return 0x12;  /* SHIFT+2: temperature 2 */
    case 29: return 0x13;  /* SHIFT+3: temperature 3 */
    case 30: return 0x14;  /* SHIFT+4: temperature 4 */
    case 10: return 0x1E;  /* SHIFT+6: scroll/page up */
    case 11: return 0x1F;  /* SHIFT+7: scroll/page down */
    case 6:  return 0x06;  /* SHIFT+9: reasoning toggle */
    default: return k;    /* normal ASCII, ENTER=13, RUBOUT=8 */
    }
}

int hal_kbhit(void)
{
    return in_Inkey() != 0;
}

/* --- text area rows 8-23; rows 0-7 belong to the scratch tier ---
 * Direct D-file output with own cursor and scroll: the ROM would scroll
 * all 24 lines and scramble the stored cells. Only real single-cell
 * glyph codes are written (0x00-0x3F); keyword/control codes and the
 * 0x76 (HALT) line terminator never appear. */
#define ZX81_TEXT_TOP 0
#define ZX81_TEXT_BOT 21  /* rows 22-23 = the ROM's lower screen (DF_SZ=2):
                              never written by us, the ROM owns them */

static int zx81_row = ZX81_TEXT_TOP;
static int zx81_col = 0;

static rt_u8 *zx81_dfile(void)
{
    /* D_FILE system variable at 16396 (0x400C) - the REAL ZX81 map,
     * confirmed byte-exact against the z88dk appmake .P reference
     * (MEM=0x405D sits at 0x401F only with base 0x4009), against the
     * original ARTILLER.P (D_FILE at header word 3 = 0x4A12, exactly
     * where its 96-line chain ends), and against the user's memory
     * dump. 0x400A is E_PPC; reading it made every poke land in ROM. */
    unsigned d = *(unsigned *)0x400C;
    return (rt_u8 *)d;
}

static rt_u8 *zx81_line(int row)
{
    /* D_FILE points at the display file's LEADING 0x76 newline byte;
     * the first cell of row 0 is D_FILE+1 (= DF_CC right after CLS),
     * then 32 cells + 0x76 terminator per row. Without the +1 every
     * write lands one byte left and eats the previous row's 0x76. */
    return zx81_dfile() + 1 + row * 33;
}

/* ZX81 glyph code for an ASCII char (unknown -> space). Letters and
 * digits map through the table above; 0x20-0x7F only, anything else is
 * a space. */
static rt_u8 zx81_code(int c)
{
    if (c < 0x20 || c > 0x7F)
        return 0x00;
    return ASCII_TO_ZX81[c - 0x20];
}

static void zx81_scr_scroll(void)
{
    int row, col;
    for (row = ZX81_TEXT_TOP; row < ZX81_TEXT_BOT; row++) {
        rt_u8 *dst = zx81_line(row);
        rt_u8 *src = zx81_line(row + 1);
        for (col = 0; col < 33; col++)
            dst[col] = src[col]; /* cells + terminator together */
    }
    for (col = 0; col < 32; col++)
        zx81_line(ZX81_TEXT_BOT)[col] = 0x00; /* spaces; terminator stays */
}

static void zx81_advance(void)
{
    zx81_col = 0;
    if (++zx81_row > ZX81_TEXT_BOT) {
        zx81_scr_scroll();
        zx81_row = ZX81_TEXT_BOT;
    }
}

void hal_console_reset(void)
{
    int row, col;

    /* Rows 0-21 are the application console. The two lower rows remain
     * owned by the ROM INPUT editor. Clear only character cells; never
     * touch the $76 row terminators. */
    for (row = 0; row <= ZX81_TEXT_BOT; row++) {
        rt_u8 *line = zx81_line(row);
        for (col = 0; col < 32; col++)
            line[col] = 0x00;
    }
    zx81_row = ZX81_TEXT_TOP;
    zx81_col = 0;
}

void hal_console_char_at(long cell, char c)
{
    long row, col;
    if (cell < 0 || cell >= (long)(22 * 32))
        return;
    row = cell / 32;
    col = cell % 32;
    zx81_line((int)row)[(int)col] = zx81_code(c);
}

void hal_putchar(int c)
{
    /* RUBOUT/BS must erase the previous D-file cell as well as move
     * the software cursor back. Handle row-wrap for longer input. */
    if (c == 8 || c == 127) {
        if (zx81_col == 0 && zx81_row > ZX81_TEXT_TOP) {
            zx81_row--;
            zx81_col = 32;
        }
        if (zx81_col > 0) {
            zx81_col--;
            zx81_line(zx81_row)[zx81_col] = 0x00;
        }
        return;
    }
    if (c == '\n') {
        zx81_advance();
        return;
    }
    if (zx81_col > 31)
        zx81_advance();
    zx81_line(zx81_row)[zx81_col] = zx81_code(c);
    zx81_col++;
}

void hal_flush(void)
{
}

void hal_msleep(long ms)
{
    /* FRAMES at 16436 (0x4034), 20 ms ticks. Needs SLOW mode: the
     * counter is bumped by the display NMI, which only runs with the
     * display on - in FAST mode this loop would never end. The crt0
     * boots SLOW (see zx81rt_crt0.asm). */
    unsigned char *f = (unsigned char *)0x4034;
    unsigned start = (unsigned)(f[0] | (f[1] << 8));
    unsigned want = (unsigned)(ms / 20);
    while ((unsigned)(f[0] | (f[1] << 8)) - start < want)
        ;
}

rt_u32 hal_entropy(void)
{
    /* FRAMES counter at 0x4034 (16436), 20 ms ticks */
    unsigned char *f = (unsigned char *)0x4034;
    return (rt_u32)f[0] | ((rt_u32)f[1] << 8) | 0x81000000UL;
}

/* --- Hidden ring-buffer RAM, located after the last topic image and
 * before D_FILE. These bytes are ordinary RAM, not display cells, so
 * the ROM video generator never renders them. Build asserts that every
 * topic image ends below $7A30; this ring occupies $7A78-$7AF7. */
#define ZX81_RING_BASE  0x7A78
#define ZX81_RING_CELLS 128

rt_u8 hal_scratch_get(long i)
{
    if (i < 0 || i >= ZX81_RING_CELLS)
        return 0;
    return *((volatile rt_u8 *)(ZX81_RING_BASE + (unsigned)i));
}

void hal_scratch_put(long i, rt_u8 v)
{
    if (i < 0 || i >= ZX81_RING_CELLS)
        return;
    *((volatile rt_u8 *)(ZX81_RING_BASE + (unsigned)i)) = v;
}

void hal_scratch_char(long i, char c)
{
    hal_scratch_put(i, (rt_u8)c);
}

void hal_panic(const char *msg)
{
    const char *p = msg;

    zx81_row = ZX81_TEXT_BOT;
    zx81_col = 0;
    while (*p)
        hal_putchar(*p++);
    hal_putchar('\n');
    for (;;) /* stay visible: no way back from a bad cassette */
        ;
}
