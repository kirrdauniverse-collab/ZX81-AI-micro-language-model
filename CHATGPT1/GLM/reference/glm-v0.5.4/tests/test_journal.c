/* test_journal.c - dated episodic memory: capture "little things",
 * pack/unpack round-trip, recall of earlier days. */
#include <stdio.h>
#include <string.h>
#include "rt_charset.h"
#include "rt_clock.h"
#include "rt_journal.h"

static int fails;

static void check(int cond, const char *what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        fails++;
    }
}

int main(void)
{
    rt_journal j, k;
    rt_clock monday, wednesday, october;
    char felt[64];
    rt_u8 buf[8 + RT_JOURNAL_MAX * (4 + RT_JOURNAL_TEXT)];
    long n;
    int idxs[8], nrec, i;

    rt_charset_init();
    rt_journal_init(&j);
    rt_clock_set(&monday, 2026, 11, 16, 9, 0);   /* monday */
    rt_clock_set(&wednesday, 2026, 11, 18, 19, 0); /* wednesday */
    rt_clock_set(&october, 2026, 10, 20, 10, 0);

    /* feeling detection */
    check(rt_journal_scan_feelings("oh my head hurts so much", felt,
                                   sizeof felt) == 1, "detects head ache");
    check(strcmp(felt, "my head hurts") == 0, "matched phrase copied");
    check(rt_journal_scan_feelings("I am tired today", felt, sizeof felt) == 1,
          "uppercase folded");
    check(rt_journal_scan_feelings("what is your name?", felt, sizeof felt) == 0,
          "question is not a feeling");
    check(rt_journal_scan_feelings("the program has a headache bug", felt,
                                   sizeof felt) == 0,
          "'a headache bug' is not about the user");

    /* add + same-day dedup */
    check(rt_journal_add(&j, &monday, "my head hurts") == 0, "add");
    check(j.n == 1, "one entry");
    check(rt_journal_add(&j, &monday, "my head hurts") == 0, "same day dup ok");
    check(j.n == 1, "duplicate not stored twice");
    check(rt_journal_add(&j, &monday, "i am tired") == 0, "second note");
    check(j.n == 2, "two entries");
    check(j.e[0].wd == 1 && j.e[0].d == 16, "date stored");

    /* round-trip */
    n = rt_journal_pack(&j, buf, sizeof buf);
    check(n > 0, "pack");
    check(rt_journal_unpack(&k, buf, n) == n, "unpack");
    check(k.n == 2 && strcmp(k.e[1].text, "i am tired") == 0,
          "entries survive");
    /* corruption must be rejected */
    buf[0] = 'X';
    check(rt_journal_unpack(&k, buf, n) == -2, "bad magic rejected");

    /* recall: on wednesday the monday notes are 2 days old */
    nrec = rt_journal_recent(&j, &wednesday, 7, idxs, 8);
    check(nrec == 2, "both monday notes recalled");
    check(strcmp(j.e[idxs[0]].text, "i am tired") == 0,
          "newest first");
    nrec = rt_journal_recent(&j, &monday, 7, idxs, 8);
    check(nrec == 0, "same-day entries are not 'remembered' again");

    /* a note from october is older than a week on wednesday */
    rt_journal_add(&j, &october, "i feel great");
    nrec = rt_journal_recent(&j, &wednesday, 7, idxs, 8);
    check(nrec == 2, "october note out of the 7 day window");
    nrec = rt_journal_recent(&j, &wednesday, 60, idxs, 8);
    check(nrec == 3, "larger window includes october");
    (void)i;

    printf("test_journal: %s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
