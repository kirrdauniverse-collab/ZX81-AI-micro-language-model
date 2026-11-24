/* boot16.c - PIOCO ZX81 profile handoff.
 *
 * All interactive input is deliberately performed by the ZX81 ROM/BASIC
 * INPUT statement.  The BASIC program in the boot cassette executes
 * PRINT/INPUT lines before it calls this routine.  Therefore character
 * editing, RUBOUT, cursor movement, ENTER and key debouncing are handled
 * by the original ROM editor, not by a C polling loop.
 */
#include <stddef.h>
#include "rt_hal.h"
#include "rt_config.h"

#define ST_CELLS 96
#define ST_BYTES 72

static char bname[9];
static char bgen;
static int byear, bmonth, bday, bhour, bmin;
static unsigned int bstamp;
static rt_u8 buf[ST_BYTES];

static void say(const char *s)
{
    while (*s)
        hal_putchar(*s++);
}

static void say_int(int v)
{
    char b[6];
    int n = 0;

    do {
        b[n++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v && n < 5);
    while (n)
        hal_putchar(b[--n]);
}

static int is_leap(int y)
{
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

static int month_days(int y, int m)
{
    static const rt_u8 md[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    if (m < 1 || m > 12)
        return 0;
    if (m == 2 && is_leap(y))
        return 29;
    return md[m - 1];
}

static int doy_calc(int y, int m, int d)
{
    int i, n = d;
    for (i = 1; i < m; i++)
        n += month_days(y, i);
    return n;
}

/* The ZX81 BASIC VARS area contains single-letter numeric variables as
 * [letter-code+64][5-byte FP], and strings as
 * [letter-code+32][length-lo][length-hi][characters...].
 * The wizard defines only single-letter scalar variables, so this parser
 * intentionally accepts only those two documented forms. */
static rt_u8 *find_basic_var(char letter, int is_string, unsigned *length)
{
    rt_u8 *p = (rt_u8 *)(*(rt_u16 *)0x4010); /* VARS */
    rt_u16 end = *(rt_u16 *)0x4014;          /* E_LINE */
    rt_u8 key = (rt_u8)(0x26 + (letter - 'A') +
                        (is_string ? 32 : 64));

    while ((rt_u16)p < end) {
        rt_u8 type = *p;
        unsigned n;

        if (type == 0x80)
            break;
        if (type >= 0x66 && type <= 0x7F) {
            if (!is_string && type == key) {
                if (length)
                    *length = 5;
                return p + 1;
            }
            p += 6;
        } else if (type >= 0x46 && type <= 0x5F) {
            n = (unsigned)p[1] | ((unsigned)p[2] << 8);
            if (n > 255 || (rt_u16)(p + 3 + n) > end)
                return (rt_u8 *)0;
            if (is_string && type == key) {
                if (length)
                    *length = n;
                return p + 3;
            }
            p += 3 + n;
        } else {
            /* No arrays or long variable names are used in this program. */
            return (rt_u8 *)0;
        }
    }
    return (rt_u8 *)0;
}

static int read_basic_number(char letter, int *value)
{
    rt_u8 *p = find_basic_var(letter, 0, (unsigned *)0);
    unsigned long mant;
    int shift;
    unsigned long v;

    if (!p)
        return 0;
    if (p[0] == 0) {
        *value = 0;
        return 1;
    }
    if (p[1] & 0x80)       /* this wizard accepts non-negative values */
        return 0;

    mant = ((unsigned long)(p[1] & 0x7F) << 24) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 8) |
           (unsigned long)p[4];
    shift = 160 - (int)p[0];
    if (shift >= 32)
        v = 0;
    else if (shift > 0)
        v = mant >> shift;
    else
        v = mant << (-shift);
    if (v > 32767UL)
        return 0;
    *value = (int)v;
    return 1;
}

static int read_basic_string(char letter, char *out, int maxlen)
{
    unsigned n = 0, i;
    rt_u8 *p = find_basic_var(letter, 1, &n);

    if (!p)
        return 0;
    if (n >= (unsigned)maxlen)
        n = (unsigned)maxlen - 1;
    for (i = 0; i < n; i++) {
        rt_u8 c = p[i];
        if (c >= 0x26 && c <= 0x3F)
            out[i] = (char)('a' + c - 0x26);
        else if (c >= 0x1C && c <= 0x25)
            out[i] = (char)('0' + c - 0x1C);
        else if (c == 0)
            out[i] = ' ';
        else
            out[i] = '_';
    }
    out[n] = 0;
    return 1;
}

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
    buf[55] = 0;
    buf[56] = 0;
    for (i = 0; i < ST_BYTES - 1; i++)
        x = (rt_u8)(x ^ buf[i]);
    buf[ST_BYTES - 1] = x;

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

static int read_profile_from_rom_input(void)
{
    rt_u8 *g;
    unsigned glen = 0;
    int ydays, yy;

    if (!read_basic_number('Y', &byear) ||
        !read_basic_number('M', &bmonth) ||
        !read_basic_number('D', &bday) ||
        !read_basic_number('H', &bhour) ||
        !read_basic_number('N', &bmin) ||
        !read_basic_string('A', bname, sizeof(bname)))
        return 0;

    if (byear < 2000 || byear > 2099 ||
        bmonth < 1 || bmonth > 12 ||
        bday < 1 || bday > month_days(byear, bmonth) ||
        bhour < 0 || bhour > 23 ||
        bmin < 0 || bmin > 59 ||
        bname[0] == 0)
        return 0;

    g = find_basic_var('G', 1, &glen);
    if (!g || glen < 1)
        return 0;
    if (g[0] >= 0x26 && g[0] <= 0x3F)
        bgen = (char)('a' + g[0] - 0x26);
    else
        return 0;
    if (bgen != 'f' && bgen != 'n')
        return 0;

    ydays = doy_calc(byear, bmonth, bday);
    bstamp = (unsigned int)((byear - 2000) * 365 + ydays);
    for (yy = 2000; yy < byear; yy++)
        if (is_leap(yy))
            bstamp++;
    return 1;
}

/* The ROM/BASIC INPUT statements have already completed when USR
 * invokes this routine. There is deliberately no getchar/getk/INKEY
 * polling here: all keyboard editing is owned by the ROM. */
void main(void)
{
    int i;

#asm
    ld ix, $4000
    call $0F2B
#endasm

    if (!read_profile_from_rom_input()) {
        say("PROFILE DATA ERROR\n");
        say("CHECK INPUT AND RUN AGAIN\n");
        return 0;
    }

    say("\nPIOCO AI PROFILE OK\n");
    say("WELCOME ");
    for (i = 0; i < 8 && bname[i]; i++)
        hal_putchar(bname[i]);
    say("!\n");
    say("YEAR ");
    say_int(byear);
    say("\nTIME ");
    say_int(bhour);
    say(":");
    if (bmin < 10)
        hal_putchar('0');
    say_int(bmin);
    say("\n");
    pack_profile();
    say("PROFILE SAVED IN MEMORY\n");
    say("REWIND TAPE TO RTBASE16\n");
    return 1;
}
