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
#include <stdio.h>   /* getk() lives here in z88dk 1.8 (not in zx81.h!) */
#include <zx81.h>
#include "rt_hal.h"

/* --- console: ZX81 char codes in, ASCII out ---
 * Table-driven on purpose: sccz80 1.8 turns a switch into a long
 * compare chain (~550 bytes for these two maps); the tables cost 160
 * bytes of image data. The input table is the exact inverse of the
 * output table below (the v0.3 input switch had drifted from it:
 * e.g. key 0x05 rendered as '?' but decoded as '('). ZX81 letters are
 * uppercase glyphs; the chat folds them to lowercase. */
static const signed char ZX81_TO_ASCII[64] = {
    /* v0.5: the exact inverse of the corrected ASCII_TO_ZX81 table
     * below - every code from the ROM K-UNSHIFT/K-SHIFT tables
     * ($007E/$00A5). The v0.3-era table had drifted badly: no comma,
     * no slash, no period, '$' sat at 0x10. Keys 0x01-0x0A are the
     * graphics codes (no ASCII), 0x0C is the pound glyph. */
    /* 0x00 */ ' ',  -1,   -1,   -1,   -1,   -1,   -1,   -1,
    /* 0x08 */ -1,   -1,   -1,   '"',  -1,   '$',  ':',  '?',
    /* 0x10 */ '(',  ')',  '>',  '<',  '=',  '+',  '-',  '*',
    /* 0x18 */ '/',  ';',  ',',  '.',  '0', '1', '2', '3',
    /* 0x20 */ '4', '5', '6', '7', '8', '9', 'a', 'b',
    /* 0x28 */ 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    /* 0x30 */ 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
    /* 0x38 */ 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
};

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

static int zx81_ascii(int k)
{
    if (k == 0x76)
        return '\n'; /* NEWLINE key */
    /* v0.7.0: the shifted command keys fold to app-level codes so the
     * REPL can implement the temperature/reasoning/scroll keys - the
     * assignment now follows the REAL ZX81 keyboard (ROM $00A5 K-SHIFT
     * table + the official manual):
     *   SHIFT+0 = 0x74 RUBOUT -> 0x12 backspace-marker (K_RUBOUT)
     *   SHIFT+1 = 0xDA EDIT,   SHIFT+2 = 0xDE, SHIFT+3 = 0xDF (unused)
     *   SHIFT+4 = 0x72,        SHIFT+5 = 0x77 -> 0x1E page-up (K_PAGEUP)
     *   SHIFT+6 = 0x0B (the quote glyph - K_REASON via '"'), SHIFT+7
     *   = 0x71, SHIFT+8 = 0x70 -> 0x1F page-down (K_PAGEDOWN).
     * The v0.6.x table had the rows crossed: 0x74 (the REAL rubout)
     * folded to K_TEMP3 and 0x77/0x70 to rubout/page-up, so the
     * delete key deleted nothing and the arrows swapped. */
    if (k == 0x74)
        return 0x12;  /* SHIFT+0 RUBOUT = the backspace */
    if (k == 0x77)
        return 0x1E;  /* SHIFT+5 = page-up */
    if (k == 0x70)
        return 0x1F;  /* SHIFT+8 = page-down */
    if (k == 0x75)
        return 0x11;  /* SHIFT+T = K_TEMP1 */
    if (k == 0x72)
        return 0x13;  /* SHIFT+4 = K_TEMP3 */
    if (k == 0xDF)
        return 0x14;  /* SHIFT+3 = K_TEMP4 */
    if (k < 0 || (k > 63 && k != 0x0B))
        return -1;
    if (k == 0x0B)
        return '"';  /* SHIFT+6: the quote glyph = K_REASON trigger */
    return ZX81_TO_ASCII[k];
}

/* --- v0.6.7: own keyboard scan (replaces the library getk) ---
 * The stock v2.2 getk() runs zx81toasc and returns ASCII, but our
 * zx81_ascii() expects the RAW ZX81 key code (0x76 = NEWLINE, the
 * shifted command keys 0x70-0x77, the rest 0x00-0x3F) - feeding it
 * ASCII produced garbage input streams (the first v0.6.7 build
 * filled the screen with N/L junk and crashed into the ROM editor).
 * This is a direct port scan instead: 8 half-rows, the row value on
 * A8-A15 via the accumulator, port $FE, bits 0-4 (0 = pressed) -
 * exactly what the ROM's $02BB scan does, but with NO ROM calls at
 * all (version-independent, no restore81 IY clobber). The code
 * tables below are the ZX81's own key table (ROM $007D/$00A5, the
 * $0064-$02FF region is byte-identical in both ROM versions). */
unsigned char KB_NOSHIFT[40] = {
    0xE9, 0x3F, 0x3D, 0x28, 0x3B,   /* SHIFT Z X C V   (row $FE) */
    0x26, 0x38, 0x29, 0x2B, 0x2C,   /* A S D F G      (row $FD) */
    0x36, 0x3C, 0x2A, 0x37, 0x39,   /* Q W E R T      (row $FB) */
    0x1D, 0x1E, 0x1F, 0x20, 0x21,   /* 1 2 3 4 5      (row $F7) */
    0x1C, 0x25, 0x24, 0x23, 0x22,   /* 0 9 8 7 6      (row $EF) */
    0x35, 0x34, 0x2E, 0x3A, 0x3E,   /* P O I U Y      (row $DF) */
    0x76, 0x31, 0x30, 0x2F, 0x2D,   /* NEWLINE L K J H (row $BF) */
    0x00, 0x1B, 0x32, 0x33, 0x27,   /* SPACE . M N B  (row $7F) */
};
unsigned char KB_SHIFT[40] = {
    0x0E, 0x19, 0x0F, 0x18, 0xE3,   /* shift Z X C V */
    0xE1, 0xE4, 0xE5, 0xE2, 0xC0,   /* A S D F G */
    0xD9, 0xE0, 0xDB, 0xDD, 0x75,   /* Q W E R T */
    0xDA, 0xDE, 0xDF, 0x72, 0x77,   /* 1 2 3 4 5 */
    0x74, 0x73, 0x70, 0x71, 0x0B,   /* 0 9 8 7 6 */
    0x11, 0x10, 0x0D, 0xDC, 0x79,   /* P O I U Y */
    0x14, 0x15, 0x16, 0xD8, 0x0C,   /* NEWLINE L K J H */
    0x1A, 0x12, 0x13, 0x17, 0xCD,   /* SPACE . M N B */
};

int kb_code;   /* the asm scan result: 0 = no key, else the ZX81 code */

static void kb_scan(void)
{
#asm
    xor  a
    ld   (_kb_code), a    ; default: no key
    ld   b, $FE           ; B = the half-row selector (A8-A15 via A)
    ld   c, 0             ; C = the row index 0-7
    ld   e, 0             ; E = the SHIFT flag
    ld   l, $FF           ; L = the found key index ($FF = none)
kb_row:
    ld   a, b
    in   a, ($FE)         ; the row 5 keys bits 0-4, 0 = pressed)
    cpl
    and  $1F
    ld   d, a             ; D = the pattern
    ld   a, c
    or   a
    jr   nz, kb_scanbits
    bit  0, d             ; row 0 bit 0 = SHIFT: modifier, not a key
    jr   z, kb_scanbits
    ld   e, 1
    res  0, d
kb_scanbits:
    ld   a, d
    or   a
    jr   z, kb_nextrow
    ld   h, 0             ; H = the bit index within the row
kb_bitloop:
    rrca                  ; bit 0 -> carry
    jr   c, kb_got
    inc  h
    jr   kb_bitloop
kb_got:
    ld   a, c             ; the global index = row*5 + bit
    add  a, a
    add  a, a
    add  a, c
    add  a, h
    ld   l, a
    jr   kb_done          ; take the first non-SHIFT key
kb_nextrow:
    inc  c
    rlc  b                ; $FE -> $FD -> ... -> $7F
    ld   a, 8
    cp   c
    jr   nz, kb_row
kb_done:
    ld   a, l
    inc  a
    jr   z, kb_none       ; L was $FF: nothing pressed
    ld   h, 0             ; HL = the key index
    ld   a, e
    or   a
    jr   z, kb_plain
    ld   de, _KB_SHIFT
    jr   kb_lookup
kb_plain:
    ld   de, _KB_NOSHIFT
kb_lookup:
    add  hl, de
    ld   a, (hl)
    ld   (_kb_code), a
    ret
kb_none:
    ld   a, $FF            ; 255 = nincs billentyu (a 0x00 = a SPACE kodja!)
    ld   (_kb_code), a
    ret
#endasm
}

static int kb_last = -1;  /* debounce: one char per press; -1 = released
                             (NOT 0: the SPACE raw code IS 0x00!) */

static int raw_getk(void)
{
    int k;

    kb_scan();
    k = kb_code;
    if (k == 255 || k < 0) {
        kb_last = -1;     /* released: re-arm (255 = no key) */
        return -1;
    }
    if (k == kb_last)
        return -1;       /* same key still held: swallow the repeat */
    kb_last = k;
    return k;            /* 0x00 = the SPACE raw code - a real key! */
}

int hal_getchar(void)
{
    int k = raw_getk();
    /* -1 = no key; 0 = the SPACE raw code (a real key - must pass) */
    return (k < 0) ? -1 : zx81_ascii(k);
}

int hal_kbhit(void)
{
    return raw_getk() >= 0;
}

/* --- text area rows 8-23; rows 0-7 belong to the scratch tier ---
 * Direct D-file output with own cursor and scroll: the ROM would scroll
 * all 24 lines and scramble the stored cells. Only real single-cell
 * glyph codes are written (0x00-0x3F); keyword/control codes and the
 * 0x76 (HALT) line terminator never appear. */
#define ZX81_TEXT_TOP 8
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

void hal_putchar(int c)
{
    if (c == '\n') {
        zx81_advance();
        return;
    }
    if (c == 8) {
        /* v0.7.0: the backspace the app's K_RUBOUT handler emits.
         * Steps the cursor back one cell and blanks it; at the left
         * edge of a row it wraps up to the previous row's last cell
         * (the exact behaviour the ZX81 editor's own rubout has).
         * A backspace on row top with col 0 is a no-op - never touches
         * the row above the text area (the scratch tier). */
        if (zx81_col) {
            zx81_col--;
        } else if (zx81_row > ZX81_TEXT_TOP) {
            zx81_row--;
            zx81_col = 31;
        } else {
            return;
        }
        zx81_line(zx81_row)[zx81_col] = 0x00; /* blank glyph */
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

/* --- screen-RAM scratch tier: D-file rows 0-7, 256 cells x 6 bit ---
 * The ZX81 ROM renders every D-file cell, so only real glyph codes
 * (0x00-0x3F) may be stored; the app packs its session state 3 bytes
 * -> 4 cells. Rows 6-7 hold the packed state, rows 0-1 the live
 * context window (see app16.c). */
rt_u8 hal_scratch_get(long i)
{
    return zx81_line((int)(i / 32))[i % 32];
}

void hal_scratch_put(long i, rt_u8 v)
{
    /* v is a packed 6-bit value: always a legal, single-cell glyph */
    zx81_line((int)(i / 32))[i % 32] = v;
}

/* show one ASCII char in scratch cell i (row 0/1 context display) */
void hal_scratch_char(long i, char c)
{
    zx81_line((int)(i / 32))[i % 32] = zx81_code(c);
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
