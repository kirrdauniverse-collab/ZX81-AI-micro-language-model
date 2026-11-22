/* test_clock.c - validation, weekday, part of day, season, day diffs.
 * Known weekdays: 1970-01-01 Thursday, 2000-01-01 Saturday,
 * 2024-01-01 Monday, 2026-11-17 Tuesday. */
#include <stdio.h>
#include <string.h>
#include "rt_clock.h"

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
    rt_clock c, a, b;

    check(rt_clock_leap(2000) == 1, "2000 is a leap year");
    check(rt_clock_leap(1900) == 0, "1900 is not");
    check(rt_clock_leap(2024) == 1, "2024 is");
    check(rt_clock_leap(2026) == 0, "2026 is not");
    check(rt_clock_days_in(2024, 2) == 29, "feb 2024");
    check(rt_clock_days_in(2026, 2) == 28, "feb 2026");

    check(rt_clock_weekday(1970, 1, 1) == 4, "1970-01-01 = thursday");
    check(rt_clock_weekday(2000, 1, 1) == 6, "2000-01-01 = saturday");
    check(rt_clock_weekday(2024, 1, 1) == 1, "2024-01-01 = monday");
    check(rt_clock_weekday(2026, 11, 17) == 2, "2026-11-17 = tuesday");
    check(strcmp(rt_clock_dayname(2), "tuesday") == 0, "dayname");

    rt_clock_init(&c);
    check(c.valid == 0, "clock starts invalid");
    check(rt_clock_set(&c, 2026, 11, 17, 19, 5) == 0, "valid set accepted");
    check(c.valid == 1 && c.wd == 2, "weekday stored");
    check(rt_clock_set(&c, 2026, 13, 1, 10, 0) == -1, "month 13 rejected");
    check(rt_clock_set(&c, 2026, 2, 30, 10, 0) == -1, "feb 30 rejected");
    check(rt_clock_set(&c, 2024, 2, 29, 10, 0) == 0, "feb 29 in leap year ok");
    check(rt_clock_set(&c, 2026, 11, 17, 24, 0) == -2, "hour 24 rejected");
    check(rt_clock_set(&c, 2026, 11, 17, 23, 60) == -2, "minute 60 rejected");

    check(strcmp(rt_clock_part_of_day(3), "night") == 0, "03:00 night");
    check(strcmp(rt_clock_part_of_day(8), "morning") == 0, "08:00 morning");
    check(strcmp(rt_clock_part_of_day(13), "afternoon") == 0, "13:00 afternoon");
    check(strcmp(rt_clock_part_of_day(19), "evening") == 0, "19:00 evening");
    check(strcmp(rt_clock_part_of_day(23), "night") == 0, "23:00 night");

    check(strcmp(rt_clock_season(1), "winter") == 0, "january winter");
    check(strcmp(rt_clock_season(4), "spring") == 0, "april spring");
    check(strcmp(rt_clock_season(7), "summer") == 0, "july summer");
    check(strcmp(rt_clock_season(11), "autumn") == 0, "november autumn");

    /* monday -> wednesday same week: 2 days */
    rt_clock_set(&a, 2026, 11, 16, 9, 0); /* monday */
    rt_clock_set(&b, 2026, 11, 18, 20, 0); /* wednesday */
    check(rt_clock_days_diff(&a, &b) == 2, "mon -> wed = 2 days");
    check(rt_clock_days_diff(&b, &a) == -2, "wed -> mon = -2");
    /* across a month boundary: 2026-11-30 -> 2026-12-01 */
    rt_clock_set(&a, 2026, 11, 30, 12, 0);
    rt_clock_set(&b, 2026, 12, 1, 12, 0);
    check(rt_clock_days_diff(&a, &b) == 1, "nov 30 -> dec 1");
    /* across a year: 2026-12-31 -> 2027-01-01 */
    rt_clock_set(&a, 2026, 12, 31, 12, 0);
    rt_clock_set(&b, 2027, 1, 1, 12, 0);
    check(rt_clock_days_diff(&a, &b) == 1, "dec 31 -> jan 1");
    /* a whole common year */
    rt_clock_set(&a, 2026, 1, 1, 0, 0);
    rt_clock_set(&b, 2027, 1, 1, 0, 0);
    check(rt_clock_days_diff(&a, &b) == 365, "2026 has 365 days");

    printf("test_clock: %s (%d failures)\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
