#include <stdlib.h>
/* UTIL pack: time, date, names, calculator, sorting, reminders, word teaching. handle() fills rb. */
static const char *const dnames[7] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
static const char mnames[] = "JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC ";
static const u8 mdl[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static char dg[9];
static uc ndg;
static void digits(const char *s) {              /* collect the digits of a text (separators ignored) */
    ndg = 0;
    for (; *s && ndg < 8; s++) if (isd(*s)) dg[ndg++] = *s;
}
static uc d2(uc i) { return (dg[i] - '0') * 10 + dg[i + 1] - '0'; }
static uc get_time(const char *s, uc *h, uc *m) {   /* hhmm, hmm, hh:mm */
    digits(s);
    if (ndg == 4) { *h = d2(0); *m = d2(2); }
    else if (ndg == 3) { *h = dg[0] - '0'; *m = d2(1); }
    else return 0;
    return *h < 24 && *m < 60;
}
static void cat2(uc v) { catc('0' + v / 10); catc('0' + v % 10); }
static void cat_time(uc h, uc m) { cat2(h); catc(':'); cat2(m); }
/* ---- calculator ---- */
static uc wat(const char *p, const char *w) { while (*w) if (up(*p++) != *w++) return 0; return 1; }
static int cres; static uc inexact;
static uc calc(const char *s) {                  /* 0 ok, 1 no sum, 2 division by zero */
    int n[8]; char op[8]; uc cnt = 0, nop = 0, want = 1, neg = 0, i; const char *p = s;
    while (*p) {
        if (isd(*p)) {
            int v = 0;
            while (isd(*p)) v = v * 10 + *p++ - '0';
            if (neg) { v = -v; neg = 0; }
            if (cnt < 8) n[cnt++] = v;
            want = 0;
        } else if (want) { if (*p == '-') neg = 1; p++; }
        else {
            char o = 0; uc l = 1;
            if (*p == '+') o = '+'; else if (wat(p, "PLUS")) { o = '+'; l = 4; }
            else if (*p == '-') o = '-'; else if (wat(p, "MINUS")) { o = '-'; l = 5; }
            else if (*p == '*') o = '*'; else if (wat(p, "TIMES")) { o = '*'; l = 5; }
            else if ((*p == 'x' || *p == 'X') && (p == s || !isl(p[-1])) && !isl(p[1])) o = '*';
            else if (*p == '/') o = '/'; else if (wat(p, "DIVIDED")) { o = '/'; l = 7; }
            if (!o) { p++; continue; }
            if (nop < 7) op[nop++] = o;
            want = 1; p += l;
        }
    }
    if (!cnt) return 1;
    if (nop >= cnt) nop = cnt - 1;
    inexact = 0;
    for (i = 0; i < nop;) {
        if (op[i] == '*' || op[i] == '/') {
            uc j;
            if (op[i] == '/') { if (!n[i + 1]) return 2; if (n[i] % n[i + 1]) inexact = 1; n[i] /= n[i + 1]; } else n[i] *= n[i + 1];
            for (j = i + 1; j < cnt - 1; j++) n[j] = n[j + 1];
            for (j = i; j < nop - 1; j++) op[j] = op[j + 1];
            cnt--; nop--;
        } else i++;
    }
    cres = n[0];
    for (i = 0; i < nop; i++) cres += op[i] == '+' ? n[i + 1] : -n[i + 1];
    return 0;
}
/* ---- sorting ---- */
static char wb[12][12];
static uc filler(const char *w) {                /* words that are not part of the list */
    char t[16]; strcpy(t, " "); strcat(t, w); strcat(t, " ");
    return find(" THESE THOSE NUMBERS WORDS WORD NUMBER THE PLEASE ABC ORDER IN LIST ALPHABETICAL ALPHABETICALLY MY ", t) >= 0;
}
static void do_sort(const char *s) {
    uc cnt = 0, i, j, k, num = 1; const char *p = s, *q; char t[12];
    for (q = s; *q; q++) if (*q == ':') p = q + 1;
    if (p == s) { int f = find(s, "SORT"); if (f >= 0) p = s + f + 4; }
    while (*p && cnt < 12) {
        while (*p && !isd(*p) && !isl(*p)) p++;
        for (k = 0; (isd(*p) || isl(*p)) && k < 11; k++) wb[cnt][k] = up(*p++);
        while (isd(*p) || isl(*p)) p++;
        wb[cnt][k] = 0;
        if (k && !filler(wb[cnt])) cnt++;
    }
    for (i = 0; i < cnt; i++) for (k = 0; wb[i][k]; k++) if (!isd(wb[i][k])) num = 0;
    for (i = 1; i < cnt; i++)
        for (j = i; j > 0; j--) {
            int c = num ? (atoi(wb[j - 1]) - atoi(wb[j])) : strcmp(wb[j - 1], wb[j]);
            if (c <= 0) break;
            strcpy(t, wb[j - 1]); strcpy(wb[j - 1], wb[j]); strcpy(wb[j], t);
        }
    cat("SORTED: ");
    for (i = 0; i < cnt; i++) { if (i) cat(", "); cat(wb[i]); }
    cat(cnt ? "." : "NOTHING TO SORT.");
}
/* ---- date ---- */
static void say_date(void) {
    uc d = pz->day, m = pz->mon, y = pz->yr, c = pz->dayc, w;
    while (c--) { uc md = mdl[m - 1]; if (m == 2 && (y & 3) == 0) md = 29; if (++d > md) { d = 1; if (++m > 12) { m = 1; y++; } } }
    pz->day = d; pz->mon = m; pz->yr = y; pz->dayc = 0;
    { static const u8 tt[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 }; int yy = 2000 + y - (m < 3);
      w = (yy + yy / 4 - yy / 100 + yy / 400 + tt[m - 1] + d) % 7; }
    cat("IT IS "); cat(dnames[w]); cat(", "); catn(d); catc(' ');
    { uc i; for (i = 0; i < 3; i++) catc(mnames[(m - 1) * 4 + i]); }
    catc(' '); catn(2000 + y); catc('.');
}
static void set_name(char *dst, const char *p) {  /* up to 8 letters */
    uc n = 0;
    while (isl(*p) && n < 8) dst[n++] = up(*p++);
    dst[n] = 0;
}
static void handle(const char *in) {
    const char *p; uc h, m, i;
    rcl();
    if ((p = word_after(in, " MEANS ")) != 0) {           /* "X MEANS Y": teach the base a word */
        int f = find(in, " MEANS "); char *e = pz->learn[pz->last_num++ & 3]; uc k = 0;
        while (f > 0 && in[f - 1] != ' ') f--;
        while (in[f] && in[f] != ' ' && k < 7) e[k++] = up(in[f++]);
        e[k] = 0;
        for (k = 8; *p && *p != ' ' && k < 15; p++) e[k++] = up(*p);
        e[k] = 0;
        cat("OK. I WILL REMEMBER THAT "); cat(e); cat(" MEANS "); cat(e + 8); cat("."); return;
    }
    if (has(in, "REMIND")) {
        if (!get_time(in, &h, &m)) { cat("SAY: REMIND ME AT 1830 TO CALL MOM"); return; }
        pz->rem_h = h; pz->rem_m = m; pz->rem_on = 1; pz->rem_txt[0] = 0;
        if ((p = word_after(in, " TO ")) != 0) { for (i = 0; p[i] && i < 11; i++) pz->rem_txt[i] = up(p[i]); pz->rem_txt[i] = 0; }
        cat("OK. I WILL REMIND YOU AT "); cat_time(h, m); cat("."); return;
    }
    if (has(in, "SET TIME")) {
        if (!get_time(in, &h, &m)) { cat("SAY: SET TIME 1730"); return; }
        hh = h; mm = m; ss = 0; acc = 0; pz->clk = 1; pz_save();
        cat("TIME SET TO "); cat_time(h, m); cat("."); return;
    }
    if (has(in, "SET DATE")) {
        digits(in);
        if (ndg == 6 || ndg == 8) {
            uc d = d2(0), mo = d2(2), y = d2(ndg - 2);
            if (d >= 1 && d <= 31 && mo >= 1 && mo <= 12) { pz->day = d; pz->mon = mo; pz->yr = y; pz->datev = 1; pz->dayc = 0; say_date(); return; }
        }
        cat("SAY: SET DATE 241126  (DAY MONTH YEAR)"); return;
    }
    if (has(in, "WHAT TIME") || has(in, "THE TIME")) { cat("IT IS "); cat_time(hh, mm); cat("."); return; }
    if (has(in, "WHAT DAY") || has(in, "WHAT DATE") || has(in, "THE DATE")) {
        if (!pz->datev) cat("I DO NOT KNOW THE DATE YET. SAY: SET DATE 241126"); else say_date();
        return;
    }
    if ((p = word_after(in, "MY NAME IS ")) != 0 || (p = word_after(in, "CALL ME ")) != 0) {   /* several users: each name gets a slot */
        char nm[9]; set_name(nm, p);
        if (!nm[0]) { cat("WHAT IS YOUR NAME?"); return; }
        for (i = 0; i < pz->nusers && strcmp(pz->names[i], nm); i++) ;
        if (i == pz->nusers) { if (pz->nusers < 3) pz->nusers++; else i = pz->cur; strcpy(pz->names[i], nm); }
        pz->cur = i;
        cat("NICE TO MEET YOU, "); cat(nm); cat("."); return;
    }
    if ((p = word_after(in, "CALL YOU ")) != 0 || (p = word_after(in, "YOUR NAME IS ")) != 0) {
        set_name(pz->bot, p);
        cat("OK, MY NAME IS "); cat(bname()); cat(" NOW."); return;
    }
    if (has(in, "WHO AM I")) {
        if (pz->names[pz->cur][0]) { cat("YOU ARE "); cat(pz->names[pz->cur]); cat("."); } else cat("I DO NOT KNOW YOUR NAME YET.");
        return;
    }
    if ((p = word_after(in, "HELLO TO ")) != 0 || (p = word_after(in, "GREET ")) != 0) {
        char nm[9]; set_name(nm, p);
        cat("HELLO "); cat(nm); cat("! "); cat(uname()); cat(" SENDS HELLO."); return;
    }
    if (has(in, "SORT")) { do_sort(in); return; }
    i = calc(in);
    if (i == 0) {
        cat("= "); catn(cres); cat("."); if (inexact) cat(" (WHOLE NUMBERS ONLY)");
    } else if (i == 2) cat("I CANNOT DIVIDE BY ZERO.");
    else cat("I CAN: SET TIME/DATE, ANSWER WHAT DAY IT IS, REMEMBER NAMES, CALCULATE 12*7+3, SORT: B A C, REMIND ME AT 1830 TO.., LEARN: X MEANS Y.");
}
