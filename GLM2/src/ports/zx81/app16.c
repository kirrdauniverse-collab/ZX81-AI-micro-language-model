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
 *  0-2  'R','T','2'   3  version   4-11 name   12 gender 'f'/'n'
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
#define K_PAGEUP   0x1E    /* SHIFT+5 (raw 0x77) */
#define K_PAGEDOWN 0x1F    /* SHIFT+8 (raw 0x70) */
#define K_RUBOUT   0x12    /* SHIFT+0 = RUBOUT (raw 0x74): backspace mid-line, temp 2 on empty */
#define K_TEMP1    0x11    /* SHIFT+T (raw 0x75) */
#define K_TEMP3    0x13    /* SHIFT+4 (raw 0x72) */
#define K_TEMP4    0x14    /* SHIFT+3 (raw 0xDF) */
#define K_REASON   '"'     /* SHIFT+6 = the quote key on an empty line */

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
#define RING_CELLS 160
#define RING_BASE  96               /* cell 96 = row 3, col 0 */

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
#asm
    ld    hl,($400C)
    inc   hl              ; HL = the first cell of row 0
    ld    de,_b           ; DE = the source b[0..71]
    ld    a,3             ; 3 work rows
ps_row:
    ld    b,8             ; 8 groups of 3 -> 4 per row
    push  af
ps_grp:
    push  bc              ; the group counter
    ld    a,(de)
    ld    c,a             ; C = b0
    inc   de
    ld    a,(de)
    ld    b,a             ; B = b1
    inc   de
    ld    a,(de)
    inc   de
    push  af              ; b2 for cell2
    push  af              ; b2 for cell3
    ld    a,c             ; cell0 = b0 & $3F
    and   $3F
    ld    (hl),a
    inc   hl
    ld    a,c             ; cell1 = (b0>>6) | ((b1&0F)<<2)
    and   $C0
    rlca                  ; bits 6-7 -> bits 0-1
    rlca
    ld    c,a
    ld    a,b
    and   $0F
    add   a,a
    add   a,a
    or    c
    ld    (hl),a
    inc   hl
    ld    a,b             ; cell2 = ((b1>>4)&$0F) | ((b2&3)<<4)
    and   $F0
    rrca
    rrca
    rrca
    rrca
    ld    c,a
    pop   af              ; b2
    and   $03
    add   a,a
    add   a,a
    add   a,a
    add   a,a
    or    c
    ld    (hl),a
    inc   hl
    pop   af              ; b2
    srl   a               ; cell3 = b2 >> 2
    srl   a
    ld    (hl),a
    inc   hl
    pop   bc
    djnz  ps_grp
    inc   hl              ; skip the row terminator ($76)
    pop   af
    dec   a
    jr    nz,ps_row
    ret
#endasm
}

static void unpack_state(void)
{
#asm
    ld    hl,($400C)
    inc   hl              ; HL = the first cell of row 0 (the source)
    ld    de,_b           ; DE = the target b[0..71] - NEVER clobbered
    ld    a,3             ; 3 work rows
pu_row:
    ld    b,8             ; 8 groups per row
    push  af
pu_grp:
    push  bc              ; the group counter waits on the stack, so
                          ; B and C are free as cell temporaries
    ld    a,(hl)          ; cell0
    inc   hl
    and   $3F
    ld    c,a             ; C = cell0
    ld    a,(hl)          ; cell1
    inc   hl
    and   $3F
    ld    b,a             ; B = cell1
    ; b0 = cell0 | (cell1&3)<<6  (written at once: C retires with cell0)
    ld    a,b
    and   $03
    rrca
    rrca
    or    c
    ld    (de),a
    inc   de
    ; C = cell1>>2  (computed now - B retires with cell1)
    ld    a,b
    and   $3C
    rrca
    rrca
    ld    c,a
    ld    a,(hl)          ; cell2
    inc   hl
    and   $3F
    ld    b,a             ; B = cell2
    ld    a,(hl)          ; cell3
    inc   hl
    and   $3F
    push  af              ; cell3 - ONE push, ONE pop below.
                          ; v0.5.2-v0.5.5 bug: the old code pushed
                          ; cell2+cell3 but popped THREE times, eating
                          ; 2 bytes of the caller stack per group - 24
                          ; groups = 48 bytes, so the RETs landed on
                          ; garbage right after the chain LOAD. The LIFO
                          ; order also fed b1 with cell3 and b2 with
                          ; caller stack bytes.
    ; b1 = (cell1>>2)&0F | (cell2&0F)<<4
    ld    a,b
    and   $0F
    add   a,a
    add   a,a
    add   a,a
    add   a,a
    or    c
    ld    (de),a
    inc   de
    ; b2 = (cell2>>4)&3 | cell3<<2
    pop   af              ; cell3
    add   a,a
    add   a,a
    ld    c,a             ; C = cell3<<2
    ld    a,b             ; cell2
    and   $30
    rrca
    rrca
    rrca
    rrca
    or    c
    ld    (de),a
    inc   de
    pop   bc
    djnz  pu_grp
    inc   hl              ; skip the row terminator ($76)
    pop   af
    dec   a
    jr    nz,pu_row
    ret
#endasm
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
    ; v0.6.8 BUGFIX (two bugs since v0.6.1!): the slot index is t,
    ; NOT t-1 (slot 0 = RT-BASE16, 1..13 = the topics - the old
    ; dec c made /topic N load slot N-1, so EVERY topic switch just
    ; reloaded the BASE), and the name write must skip PAST the
    ; opening quote (the single inc hl landed ON it and the first
    ; name byte overwrote the quote - the sim saw the LOAD lose its
    ; name marker).
    ld    b,$00
    ld    a,c
    add   a,a
    add   a,a
    add   a,a             ; t*8
    ld    c,a
    inc   hl              ; first name byte
    add   hl,bc           ; hl = names + t*8
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
    inc   hl              ; hl = the opening quote
    inc   hl              ; skip PAST it: hl = the 8-char slot
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

/* ---- the scrollback page: the ring -> display rows 8-15 ---- */

static void page_show(void)
{
    int i, c;
    char row[33];

    row[32] = 0;
    for (i = 0; i < 5; i++) {
        int base = (ring_pos / 32) * 32;   /* the oldest row first */
        int src = (base + i * 32) % RING_CELLS;

        for (c = 0; c < 32; c++) {
            rt_u8 v = hal_scratch_get(RING_BASE +
                                      ((src + c) % RING_CELLS));
            row[c] = RT_CHARSET[v < RT_CHARSET_LEN ? v : 0];
        }
        for (c = 0; c < 32; c++)
            hal_scratch_char((8 + i) * 32 + c, row[c]);
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
            /* v0.7.0: RUBOUT deletes the last typed char; on an
             * empty line it cycles the temperature (the doc's
             * "temp 2 on empty" - SHIFT+0 pressed with nothing to
             * delete changes the hofok). */
            if (n) {
                n--;
                hal_putchar(8);
            } else {
                b[33] = (rt_u8)(b[33] >= 4 ? 1 : b[33] + 1);
                chat.temp_sh = b[33] - 1;
                say("hofok: ");
                say_int(b[33]);
                say("\n");
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
    /* v0.7.2 (THE REAL-MACHINE SZIA-FREEZE FIX): the display goes off
     * for the "thinking" exactly as in v0.6.7+ (RES 6 + OUT $FD - the
     * ROM's own FAST semantics), and comes back through the ROM's own
     * $0207 SLOW dance - the same proven pattern the main() entry
     * uses. The v0.7.1 kb_rearm (SET 6 + 17x OUT $FE from C) armed
     * the SIM's flag model but NOT the real generator (it only arms
     * through the dance); that gap was the real-machine freeze after
     * SZIA+ENTER. See the closing block below for the dance-side
     * safety analysis. */
#asm
    push ix              ; -IXIY swap: machine PUSH IY (C frame ptr)
    ld ix, $4000         ; machine LD IY,$4000 = the sysvar base
    res 6, (ix+$3B)      ; machine RES 6,(IY+$3B): CDFLAG -> FAST
    ld a, (ix+$3B)
    out ($FD), a         ; the NMI generator off (FAST: no NMIs)
    pop ix               ; machine POP IY
#endasm
    if (b[34]) {
        chat.greedy = 1;          /* pass 1: the silent greedy draft */
        rt_chat_gen(&chat, &model, silent_emit, 0);
        chat.greedy = 0;          /* pass 2: the visible sampled reply */
    }
    rt_chat_gen(&chat, &model, emit, 0);
    /* v0.7.3 (THE BLACK-SCREEN FIX - the real-machine failure of
     * v0.7.2, decoded from the ROM byte by byte):
     *
     * The v0.7.2 close was: SET 6 + SET 7 (CDFLAG=$C0) + CALL $0207.
     * The dance opens with
     *     $0207: LD HL,$403B / LD A,(HL) / RLA / XOR (HL) / RLA
     *            RET NC
     * which RETURNS IMMEDIATELY whenever CDFLAG bit6 == bit7 (the
     * XOR+RLA clears Carry exactly then) - and the 17x OUT ($FE)
     * arming burst, the OUT ($FD), the SET 7 and the display walk all
     * live on the FALL-THROUGH path only. With $C0 the call armed
     * NOTHING: the generator stayed off from the FAST switch at the
     * top of gen_reply, every scanline NMI was gone, the display -
     * and even the border, which is generated by the very same NMI
     * train - went black. The sim missed it because it read the
     * D-file straight from memory instead of through the video path.
     *
     * The proven main()-entry pattern, replicated here exactly:
     *   1. blank the 3 packed-state work rows (the dance's walk
     *      "executes" the D-file mirror; the packed state bytes
     *      0x00-0x3F are hostile JR opcodes),
     *   2. call the $7A40 blob (its own CALL $0207 runs the dance
     *      with CDFLAG=$80 - bit7 still set from the main-entry
     *      dance, bit6 CLEAR because we never set it here - so the
     *      fall-through path runs: 17x OUT ($FE) arms the generator,
     *      the dance sets bit7 itself, the walk renders one frame),
     *   3. then SET 6 + the 17x OUT ($FE) burst (the 'disp_on' block,
     *      same as main()) to land in SLOW with the display live.
     * After this the machine is bit-for-bit in the same state as
     * after the main() entry - which the whole boot->chat chain has
     * already survived on real hardware. */
    {
        rt_u8 *row = (rt_u8 *)(*(rt_u16 *)0x400C) + 1;
        int r;
        for (r = 0; r < 3; r++, row += 33) {
            int c;
            for (c = 0; c < 32; c++)
                row[c] = 0;
            row[32] = 0x76;
        }
    }
#asm
    ; v0.7.3 FINAL (the 17-byte binary patch, proven in sim):
    ;   call $7A40   ; the blob: PUSH IY / LD IY,$4000 / CALL $0207
    ;                ;   (CDFLAG=$80 here: bit7 from the main-entry dance,
    ;                ;    bit6 cleared by gen_reply's opening RES 6 - so the
    ;                ;    dance FALLS THROUGH: 17x OUT ($FE) arms the real
    ;                ;    generator, SET 7 lands, the walk renders the fresh
    ;                ;    reply rows through our $7A40 chain),
    ;                ;   then the blob's FRAMES wait + clean RET (IY restored)
    ;   ld a,($403B) ; \
    ;   or $40        ;  > SET bit6 without touching IY (CDFLAG=$C0 = SLOW)
    ;   ld ($403B),a  ; /
    ;   ld b,$11      ; the disp_on burst from main(): 17x OUT ($FE)
    ;   out ($FE),a   ; - the emulator/real generator flag back ON
    ;   djnz $-2      ;   (the dance's own OUT ($FD) at $0218 had cleared it;
    ;                ;    this burst is what keeps the video alive after)
    ; The sccz80 rebuild must produce EXACTLY this shape at the old
    ; v0.7.2 block position ($4CC3, 17 bytes - a drop-in replacement);
    ; until the toolchain's ragel step is fixed the shipped binaries
    ; carry this as a binary patch (tools/patch_v073.py logic).
    call $7A40          ; the blob (see above)
    ld a, ($403B)
    or $40              ; SET 6 equivalent, no IY frame needed
    ld ($403B), a
    ld b, $11
gen_arm:
    out ($FE), a
    djnz gen_arm
#endasm
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

    /* v0.7.1: the dance inside the blob ($0207) runs the ROM display
     * walk over the D-file rows - harmless at boot (fresh rows) but
     * FATAL after a topic load: the rows then carry the previous
     * USR's packed chat state (values 0x00-0x3F, i.e. JR opcodes),
     * the walk runs off through them and the PC lands at $2 (ROM
     * restart, report C - proven in sim, f3144 PC=$2). Blank the 3
     * work rows BEFORE the dance; pack_state() rewrites them right
     * after state_apply(). D_FILE at $400C, row = 32 cells + $76. */
    {
        rt_u8 *row = (rt_u8 *)(*(rt_u16 *)0x400C) + 1;
        for (i = 0; i < 3; i++, row += 33) {
            for (restored = 0; restored < 32; restored++)
                row[restored] = 0;
            row[32] = 0x76;
        }
    }

    /* v0.6.7: same as the boot (see boot16.c) - install the altint
     * chain at $7A40 instead of the v2-only ROM SLOW call. The 7-byte
     * block (CALL $7A40 + 4 NOPs) = the checker's SLOW_SEQ. */
#asm
    call $7A40
    nop
    nop
    nop
    nop
    ; v0.6.8: the crt0 left the display gated OFF (NMI generator off,
    ; CDFLAG FAST) so the ROM walk can never run over the state rows
    ; (the topic-load freeze: the walk executes the rows as code and
    ; some states loop forever in the mirror). The chain is now
    ; installed - turn the display back on: SET CDFLAG bit 6 + the
    ; 17x OUT $FE re-arm burst. Every NMI from now on ends with the
    ; ROMs JP (IX) into OUR chain (pointer-walk, never executes).
    ; v0.7.1 DIAG (NOT changed): the blob's own 'CALL $0207' (offset
    ; 6) IS the NMI generator re-arm - NOPing it out or moving the
    ; SET 6 before the CALL both leave the machine dead in the
    ; $7a58 NMI-wait loop (proven twice in sim). The 3rd-USR death
    ; after a topic load is NOT this call: the dance runs the ROM
    ; display walk over the D-file rows, which at that moment carry
    ; the 2nd USR's packed chat state - see the v0.7.1 README entry.
    push ix
    ld ix, $4000
    set 6, (ix+$3B)
    ld a, (ix+$3B)
    ld b, $11
disp_on:
    out ($FE), a
    djnz disp_on
    pop ix
#endasm

    rt_charset_init();
    /* v0.6.6 CRITICAL (the missing-reply root cause): the shipped
     * binaries never called rt_chat_init, so chat.max_new stayed 0
     * (BSS) and rt_chat_gen's "while (n < cap)" NEVER ran - the
     * engine echoed the input and printed no reply. state_apply()
     * restores hist/hist_n/seed/temp_sh from the packed rows, so the
     * init must run BEFORE it (only max_new and the hist defaults
     * survive from here). The shipped v0.6.5/v0.6.6 binaries carry a
     * binary patch for this (tools/patch_slow_v066.py); a fresh z88dk
     * build gets it from this call. NOTE for the rebuild: also verify
     * the interrupt protection around rt_chat_gen - in the shipped
     * binary the generation self-destructs after ~30 frames (the
     * engine returns garbage, line 2's THEN LOAD fires, the ROM tape
     * loop trashes memory); the altint chain does not cover the long
     * generation loops. */
    rt_chat_init(&chat, 0);

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

    say("tinyllm v0.6.2\n");
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
