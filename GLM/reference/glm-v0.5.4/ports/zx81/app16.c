/* app16.c - retro-tinyllm 16K ZX81 edition v0.5: chat REPL with a
 * boot chain, profile, memory events, scrollback, temperature and a
 * reasoning toggle.
 *
 * v0.5 (the user's spec):
 * - BOOT CHAIN: cassette block 1 = RT-BOOT01 (boot16.c, no model)
 *   draws the logo, offers the manual, asks date/time/name/gender,
 *   packs the profile into the D-file work rows and returns 1, so
 *   BASIC runs 2 IF USR 16514 = 1 THEN LOAD "RT-BASE16".
 * - MODEL FINDER: the blob is located by a "RTLM" MAGIC SCAN between
 *   the program area and $7AFF, then checksummed by rt_model_load.
 *   v0.4.5's D_FILE derivation went stale when the ROM's NEW command
 *   (or an emulator quickload path) rebuilt the display file at
 *   another position - the scan finds the model wherever it lives,
 *   and the panic prints the D_FILE value as a diagnostic.
 * - MEMORY: 10 two-byte events ([type][day-stamp low byte]) in the
 *   state. The user's line is mined for keywords (faj/boldog/
 *   szomoru/faradt/szerelmes); the boot greets with the newest
 *   memory and a day-based age ("3 napja"), events older than 14
 *   days read as faded - the "headache two weeks ago" rule.
 * - PERSISTENCE: the state is mirrored into a 48-byte zone at the
 *   end of line 1's REM (tail = $4082 + line1_len - 50, derived from
 *   the $407F length word, so it is editor-shift-proof: line 1 sits
 *   BELOW any edit point). /save returns 2, so BASIC executes
 *   3 IF USR 16514 = 2 THEN SAVE "RT-BASE16" - the ROM's own SAVE
 *   writes the program including the tail; rewinding the tape to the
 *   base block and recording overwrites it WITH the memories. On the
 *   next boot the tail is read back if the work rows are gone.
 * - SCROLLBACK: an 8-row ring (256 B) at RAMTOP-445 (plain RAM above
 *   STKBOT, below the machine stack reserve) mirrors every printed
 *   row; SHIFT+5 pages it into display rows 8-15, SHIFT+0 leaves the
 *   page view.
 * - TEMPERATURE: SHIFT+1..4 on an empty line sets the sharpness
 *   1..4, mapped to rt_chat's temp_sh 0..3 (1 = determined, 4 =
 *   adventurous).
 * - REASONING: SHIFT+9 on an empty line toggles the two-pass mode -
 *   the model first drafts the answer greedily (silently, but the
 *   draft lands in the rolling context), then generates the visible
 *   reply conditioned on it.
 *
 * v0.4.5: line 2's number literals carry the $7E+5-byte FP tail the
 * ROM's run-time scanner requires (S-STK-DEC skips the digits and
 * hunts the $7E marker); the LOAD-name poke hunts the $EF token, so
 * even a re-typed line 2 is poked correctly.
 *
 * Coding style note: sccz80 1.8 emits ~10 bytes per statement, so
 * this file sticks to 8-bit arithmetic and short strings.
 */
#include <string.h>
#include <stddef.h>
#include "rt_hal.h"
#include "rt_config.h"
#include "rt_charset.h"
#include "rt_model.h"
#include "rt_chat.h"
#include "rt_engine.h"

/* ---- layout constants ---- */
#define MSIZE 1030              /* the class-G blob: fixed layout */
#define NTOPICS 13              /* chainable topic packs (RT-TUDOM..) */
#define TABLE_NAMES 14          /* + RT-BASE16 in slot 0 */

/* ---- state v2: 72 bytes packed 3 -> 4 into 96 cells (rows 0-2) ----
 *  Hidden RAM at $7A30: raw 72-byte state (survives short topic LOADs)
 *  13  year-2000      14-15 stamp (days since 2000, BE)
 *  16  hour           17  min       18-25 hist  26  hist_n
 *  27-30 seed         31  topic     32  pending 33  temp 1-4
 *  34  reason 0/1     35-54 events[10][2] = [type][stamp&255]
 *  55  evcount        56  save flag 57  xor
 */
#define ST_ROW   0
#define ST_CELLS 96
#define ST_BYTES 72
#define EV_OFF  35
#define EV_MAX  10

#define EV_PAIN 1
#define EV_JOY  2
#define EV_SAD  3
#define EV_TIRED 4
#define EV_LOVE 5

/* app-level key codes the HAL folds shifted keys into */
#define K_PAGEUP   0x1E    /* SHIFT+5 */
#define K_PAGEDOWN 0x1F    /* SHIFT+0 */
#define K_RUBOUT   8       /* SHIFT+5: ZX81 RUBOUT (ASCII backspace) */
#define K_TEMP1    0x11    /* SHIFT+1 */
#define K_TEMP3    0x13    /* SHIFT+3 */
#define K_TEMP4    0x14    /* SHIFT+4 */
#define K_REASON   0x06    /* SHIFT+9: Z88DK INKEY control code */

static rt_model model;
static rt_chat chat;
static rt_u16 model_addr;      /* where the magic scan found the blob */
#define TAG_AT     (model_addr + MSIZE)
#define NAMTAB_AT  (model_addr + MSIZE + 12)
static rt_u16 namtab;          /* handed to the poke asm */
static int ring_col;           /* 0-31 inside the current ring row */
static int ring_pos;           /* the next ring cell (cyclic) */
static rt_u8 b[ST_BYTES];
static char uname[9];
static unsigned int now;       /* days since 2000, from the profile */

/* the scrollback: work rows 3-7 = 160 cells of 6 bits = 160 chars =
 * 5 rows of 32, packed (display codes are 0x00-0x3F so 6 bits are
 * enough). Lives INSIDE the display file, so an editor shift moves it
 * with everything else and it costs no separate RAM. */
#define RING_CELLS 128
#define RING_BASE  0                /* hidden RAM ring at $7A78 */

static rt_u8 topic_idx;        /* 0 = base, 1..13 = topic */
static rt_u8 pending;          /* topic requested via /topic */

/* ---- tiny printers ---- */

static void say(const char *s)
{
    while (*s)
        hal_putchar(*s++);
}

static void say_int(int v)
{
    char t[6];
    int n = 0;

    if (v < 0) {
        hal_putchar('-');
        v = -v;
    }
    do {
        t[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n)
        hal_putchar(t[--n]);
}

/* ---- state pack/unpack: 72 bytes -> 96 cells of 6 bits ----
 * Hand-written Z80 (the C versions cost ~470 lines of sccz80 code
 * because hal_scratch_put/get would be called 192 times). Cells are
 * the D-file work rows 0-2: cell i = D_FILE+1+(i/32)*33+(i%32), so
 * every 32 cells are followed by the row's $76 terminator, which this
 * code SKIPS after each row. All written values are 0x00-0x3F
 * (plain glyph codes only - the user's freeze rule). The packing is
 * the same 3->4 scheme the boot block uses for the profile. */

static void pack_state(void)
{
    /* Persist the 72-byte session record in the hidden scratch gap.
     * This is deliberately NOT written into D_FILE: those bytes are
     * fetched by the ZX81 video generator and would appear as garbage. */
    memcpy((void *)0x7A30, b, ST_BYTES);
}

static void unpack_state(void)
{
    memcpy(b, (const void *)0x7A30, ST_BYTES);
}

static int state_valid(void)
{
    rt_u8 x = 0;
    int i;

    if (b[0] != 'R' || b[1] != 'T' || b[2] != '2')
        return 0;
    for (i = 0; i < ST_BYTES - 1; i++)
        x = (rt_u8)(x ^ b[i]);
    return x == b[ST_BYTES - 1];
}

static void state_apply(void)
{
    int i;

    for (i = 0; i < RT_MAX_CTX; i++)
        chat.hist[i] = b[18 + i] < RT_CHARSET_LEN ? b[18 + i] : 1;
    chat.hist_n = b[26] > RT_MAX_CTX ? RT_MAX_CTX : b[26];
    chat.seed = (rt_u32)b[27] | ((rt_u32)b[28] << 8);
    chat.temp_sh = b[33] < 1 ? 1 : b[33] > 4 ? 3 : b[33] - 1;
    topic_idx = b[31] > NTOPICS ? 0 : b[31];
    pending = b[32] > NTOPICS ? 0 : b[32];
}

/* ---- model finder: the "RTLM" magic scan + checksum ---- */

static int model_find(void)
{
    rt_u16 a, lo = 0x4090, hi = 0x7AFF;

    for (a = hi; a >= lo; a--) {
        if (*(rt_u8 *)a == 'R' && *(rt_u8 *)(a + 1) == 'T' &&
            *(rt_u8 *)(a + 2) == 'L' && *(rt_u8 *)(a + 3) == 'M') {
            if (rt_model_load(&model, (const rt_u8 *)a,
                              RT_APP_MODEL_MAX) == 0) {
                model_addr = a;
                namtab = a + MSIZE + 12;
                return 1;
            }
        }
    }
    return 0;
}

/* ---- memory events ---- */

static void event_add(rt_u8 type)
{
    int slot = b[55] % EV_MAX;    /* the ring: overwrite by count */

    b[EV_OFF + slot * 2] = type;
    b[EV_OFF + slot * 2 + 1] = (rt_u8)(now & 255);
    b[55]++;
}

static int match(const char *s, const char *w)
{
    int n = 0, i;

    while (w[n])
        n++;
    for (;;) {
        for (i = 0; i < n; i++)
            if (s[i] != w[i])
                goto next;
        if (!s[n] || s[n] == ' ' || s[n] == ',' || s[n] == '.' ||
            s[n] == '?' || s[n] == '!')
            return 1;
next:
        while (*s && *s != ' ')
            s++;
        if (!*s)
            return 0;
        s++;
    }
}

static void memory_scan(const char *s)
{
    if (match(s, "faj") || match(s, "fajdalom"))
        event_add(EV_PAIN);
    else if (match(s, "boldog") || match(s, "szerelmes"))
        event_add(EV_JOY);
    else if (match(s, "szomoru") || match(s, "faradt"))
        event_add(EV_SAD);
}

static void recall_print(void)
{
    static const char *ew[6] = {0, "faj a fejem", "orom",
                                "szomorusag", "faradsag", "szerelem"};
    int i, newest = -1;
    rt_u8 age;

    for (i = 0; i < EV_MAX; i++)
        if (b[EV_OFF + i * 2] &&
            (newest < 0 ||
             (rt_u8)(b[EV_OFF + i * 2 + 1] -
                     b[EV_OFF + newest * 2 + 1]) < 128))
            newest = i;
    if (newest < 0)
        return;
    age = (rt_u8)((now & 255) - b[EV_OFF + newest * 2 + 1]);
    if (age > 14)
        say("emlek: mar elmult\n");
    else {
        say("emlek: ");
        say(ew[b[EV_OFF + newest * 2]]);
        say(" (");
        say_int(age);
        say(" napja)\n");
    }
}

/* ---- LOAD-name poke: the table address comes from the finder ---- */

static void poke_load_name(rt_u8 topic)
{
#asm
    ld    hl,2
    add   hl,sp
    ld    a,(hl)          ; topic number
    ld    hl,(_namtab)    ; the name table (the C computed it)
    ld    c,a
    ld    a,(hl)          ; magic
    cp    $A1
    ret   nz
    inc   hl
    ld    a,(hl)          ; count
    cp    c
    ret   c               ; topic > count: leave line 2 untouched
    dec   c
    ld    b,$00
    ld    a,c
    add   a,a
    add   a,a
    add   a,a             ; (topic-1)*8
    ld    c,a
    inc   hl              ; first name byte
    add   hl,bc           ; hl = names + (topic-1)*8
    ex    de,hl           ; de = the source
    ld    hl,($407F)      ; line-1 body length
    ld    bc,$4085
    add   hl,bc           ; hl = the first body byte of line 2
    ld    b,$20
pkn2:
    ld    a,(hl)
    cp    $EF             ; the LOAD token
    jr    z,pkn3
    inc   hl
    djnz  pkn2
    ret                   ; no LOAD in line 2: leave it untouched
pkn3:
    inc   hl              ; skip the quote: hl = the 8-char slot
    ld    b,$08
pkn1:
    ld    a,(de)
    ld    (hl),a
    inc   hl
    inc   de
    djnz  pkn1
    ret
#endasm
}

/* ---- the scrollback page: four hidden ring rows -> console rows 8-11 ---- */

static void page_show(void)
{
    int i, c;
    char row[33];

    row[32] = 0;
    for (i = 0; i < 4; i++) {
        int base = (ring_pos / 32) * 32;   /* the oldest row first */
        int src = (base + i * 32) % RING_CELLS;

        for (c = 0; c < 32; c++) {
            rt_u8 v = hal_scratch_get(RING_BASE +
                                      ((src + c) % RING_CELLS));
            row[c] = RT_CHARSET[v < RT_CHARSET_LEN ? v : 0];
        }
        for (c = 0; c < 32; c++)
            hal_console_char_at((8 + i) * 32 + c, row[c]);
    }
}

static void emit(char c, void *user)
{
    (void)user;
    hal_putchar(c);
    if (ring_col >= 0) {
        if (c == '\n') {
            while (ring_col < 32) {
                hal_scratch_put(RING_BASE + ring_pos, ' ');
                ring_pos++;
                ring_col++;
            }
            ring_col = 0;
        } else if (ring_col < 32) {
            hal_scratch_put(RING_BASE + ring_pos, (rt_u8)c);
            ring_pos = (ring_pos + 1) % RING_CELLS;
            ring_col++;
        }
    }
    hal_flush();
}

/* slash-command compare without the string library: line = "/xyz...",
 * cmd = "xyz" */
static int eq_cmd(const char *line, const char *cmd)
{
    int i = 1;

    while (*cmd) {
        if (line[i] != *cmd)
            return 0;
        i++;
        cmd++;
    }
    return line[i] == 0 || line[i] == ' ';
}

/* the reasoning pass 1: greedy draft, discarded visually, kept in
 * the rolling context by rt_chat_gen itself */
static void silent_emit(char c, void *user)
{
    (void)user;
    (void)c;
}

/* ---- input line with the empty-line switches ---- */

static char line[40];

static int getline(void)
{
    int n = 0, c;

    for (;;) {
        c = hal_getchar();
        if (c < 0)
            continue;
        if (c == '\n' || c == '\r') {
            line[n] = 0;
            return n;
        }
        if (c == K_PAGEUP || c == K_PAGEDOWN) {
            page_show();
            continue;
        }
        if (n == 0) {
            if (c >= K_TEMP1 && c <= K_TEMP4) {
                b[33] = (rt_u8)(c - K_TEMP1 + 1);
                if (b[33] > 4)
                    b[33] = 4;
                chat.temp_sh = b[33] - 1;
                say("hofok: ");
                say_int(b[33]);
                say("\n");
                continue;
            }
            if (c == K_REASON) {
                b[34] = b[34] ? 0 : 1;
                say(b[34] ? "gondolkodom\n" : "gyors valasz\n");
                continue;
            }
        }
        if (c == K_RUBOUT) {
            if (n) {
                n--;
                hal_putchar(8);
            }
            continue;
        }
        if (c < ' ' || c > 0x7F)
            continue;
        if (c >= 'A' && c <= 'Z')
            c += 32;
        if (n < 38)
            line[n++] = (char)c;
        hal_putchar(c);
    }
}

/* ---- the generation: optional greedy draft, then the reply ---- */

static void gen_reply(void)
{
    if (b[34]) {
        chat.greedy = 1;          /* pass 1: the silent greedy draft */
        rt_chat_gen(&chat, &model, silent_emit, 0);
        chat.greedy = 0;          /* pass 2: the visible sampled reply */
    }
    rt_chat_gen(&chat, &model, emit, 0);
    if (ring_col) {
        while (ring_col < 32) {
            hal_scratch_put(RING_BASE + ring_pos, ' ');
            ring_pos = (ring_pos + 1) % RING_CELLS;
            ring_col++;
        }
        ring_col = 0;
    }
}

/* ---- the default state for a naked run (no boot, no tail) ---- */

static void state_default(void)
{
    int i;

    for (i = 0; i < ST_BYTES; i++)
        b[i] = 0;
    b[0] = 'R';
    b[1] = 'T';
    b[2] = '2';
    b[3] = 2;
    b[33] = 2;
}

int main(void)
{
    int restored, i;
    char cmd;

    /* v0.5.4: same display re-assert as the boot (see boot16.c): after
     * the ROM's LOAD (multiload chain) or a quickload entry the NMI
     * display must be re-enabled, else only getk()'s keyboard scans
     * regenerate the picture. */
#asm
    ; -IXIY swap: assembles as LD IY,4000h = the ROM sysvar base
    ld ix, $4000
    call $0F2B           ; SLOW: enable the continuous NMI display
#endasm

    hal_console_reset();
    rt_charset_init();

    /* Initialize the chat engine before restoring the saved profile.
     * v0.5.4 had omitted this call, leaving max_new=0 in the zeroed
     * static structure, so generation could silently do no work. */
    rt_chat_init(&chat, hal_entropy() ^ 0xA51F39D7UL);
    chat.max_new = 48;       /* bounded reply length for the 16K ZX81 */

    /* the /save handoff: the line-3 USR must return 2 (and clear).
     * The tail is written-only in v0.5: the work rows survive every
     * block load (they sit above all image ends), so on a normal boot
     * the rows carry the state; if the rows are gone the fresh run
     * starts with defaults until the user saves again. */
    unpack_state();
    restored = 0;
    if (state_valid()) {
        restored = 1;
        if (b[56] == 2) {
            b[56] = 0;
            pack_state();
            return 2;
        }
        state_apply();
    } else {
        state_default();
    }

    if (model_find() == 0) {
        say("no model! dfile=");
        say_int((int)(*(rt_u16 *)0x400C));
        say("\nbad cassette\n");
        return 0;
    }
    {   /* the block's own tag = which pack is really in the machine */
        rt_u8 tag = *(rt_u8 *)TAG_AT;

        if (tag > NTOPICS)
            tag = 0;
        topic_idx = tag;
        b[31] = tag;
    }

    for (i = 0; i < 8; i++)
        uname[i] = (b[4 + i] == 0) ? ' ' : b[4 + i];
    uname[8] = 0;
    now = ((unsigned int)b[14] << 8) | b[15];

    ring_col = 0;
    ring_pos = 0;

    say("tinyllm v0.5.4\n");
    if (uname[0] != ' ')
        recall_print();
    else
        say("udv! /help\n");

    for (;;) {
        int n;

        say("you> ");
        hal_flush();
        n = getline();
        say("\n");
        if (n == 0)
            continue;
        memory_scan(line);
        if (line[0] == '/') {
            cmd = line[1];
            if (line[1] == 'h' && line[2] == 'e') {
                say("/topic /save /new /quit\n");
                say("SHIFT 1-4 hofok, 9 gondolkodo, 5/0 regen\n");
            } else if (cmd == 't' && line[2] == 'o' &&
                       line[6] == ' ' && line[7] == 0) {
                for (i = 1; i <= NTOPICS; i++) {
                    say_int(i);
                    say(" ");
                }
                say("\n1 tud 2 elet 3 hobbi 4 egesz 5 munka\n");
                say("6 muvesz 7 tanul 8 irod 9 film 10 prog\n");
                say("11 csill 12 rajz 13 vers\n");
            } else if (cmd == 't' && line[7] >= '1' &&
                       line[7] <= '9') {
                rt_u8 t = (rt_u8)(line[7] - '0');

                if (t > NTOPICS)
                    t = NTOPICS;
                pending = t;
                poke_load_name(t);
                pack_state();
                say("toltom a temat...\n");
                return 1;
            } else if (cmd == 's') {
                pack_state();          /* the rows go to tape with the
                                          ROM's SAVE: the D-file sits
                                          inside $4009..VARS-end */
                b[56] = 2;
                pack_state();
                say("tekerj a RTBASE16-ra es mentsd!\n");
                return 2;
            } else if (cmd == 'n') {
                chat.hist_n = 0;
                b[26] = 0;
                pack_state();
                say("uj beszelgetes.\n");
            } else if (cmd == 'q') {
                pack_state();
                return 0;
            } else {
                say("???\n");
            }
                continue;
        }
        rt_chat_push(&chat, line);
        gen_reply();
        hal_putchar('\n');
        /* the ring: the reply rows were printed raw by emit; mirror
         * them crudely by logging the context refresh below */
        pack_state();
    }
}

    /* small-case-insensitive prefix compare moved above main */
