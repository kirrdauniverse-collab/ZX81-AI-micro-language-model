/* test_tape.c - RT-turbo block round-trip + CRC rejection, entirely in
 * memory (the same decoder runs on real hardware over port pulses). */
#include <stdio.h>
#include <string.h>
#include "rt_tape.h"

static int fails;

static void check(int cond, const char *what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        fails++;
    }
}

static void check_eq(long got, long want, const char *what)
{
    if (got != want) {
        printf("FAIL: %s (got %ld, want %ld)\n", what, got, want);
        fails++;
    }
}

#define MAXP 100000
static rt_u16 pulses[MAXP];
static long np;

static void capture(rt_u16 us)
{
    if (np < MAXP)
        pulses[np++] = us;
}

static rt_u16 pos;

static rt_u16 replay(void)
{
    rt_u16 v;
    if (pos >= np)
        return 0; /* timeout: no more tape */
    v = pulses[pos++];
    /* +-16% wobble like a real cassette drive (decoder tolerates 25%) */
    v = (rt_u16)((long)v * (100 + ((int)(pos % 5) - 2) * 8) / 100);
    return v;
}

int main(void)
{
    rt_u8 data[600], back[600];
    long got;
    int i;

    for (i = 0; i < (int)sizeof data; i++)
        data[i] = (rt_u8)(i * 7 + 3);
    np = 0;
    check(rt_tape_write_block(data, sizeof data, capture) == 0, "write");

    pos = 0;
    got = rt_tape_read_block(back, sizeof back, replay);
    check_eq(got, (long)sizeof data, "read length");
    check(memcmp(data, back, sizeof data) == 0, "payload round-trip");

    /* corrupt ONE PULSE IN TRANSIT (flip a data bit) -> CRC must catch it */
    np = 0;
    rt_tape_write_block(data, sizeof data, capture);
    {
        /* data region starts after 64 pilots + 2 sync + 16 len bits */
        long p = 64 + 2 + 16 + 123 * 8; /* byte 123, MSB-first bits */
        pulses[p] = (pulses[p] == RT_T_ONE_US) ? RT_T_ZERO_US : RT_T_ONE_US;
    }
    pos = 0;
    got = rt_tape_read_block(back, sizeof back, replay);
    check_eq(got, -3, "CRC catches in-transit corruption");

    /* junk before the block must be skipped until the pilot */
    np = 0;
    capture(500); capture(1200); capture(700); capture(3000); /* noise */
    rt_tape_write_block(data, sizeof data, capture);
    pos = 0;
    got = rt_tape_read_block(back, sizeof back, replay);
    check_eq(got, (long)sizeof data, "pilot hunt skips leading noise");
    check(memcmp(data, back, sizeof data) == 0, "payload after noise");

    /* silence -> no block found */
    np = 0;
    pos = 0;
    got = rt_tape_read_block(back, sizeof back, replay);
    check_eq(got, -1, "silence gives -1");

    printf("test_tape: %s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
