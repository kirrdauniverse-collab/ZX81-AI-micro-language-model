/* CODE pack: short ZX81 BASIC programs from templates; a number in the request is the parameter ("make it 20" changes the last one). */
static uc num_in(const char *s, uc def) {
    for (; *s; s++) if (isd(*s)) { uc v = 0; while (isd(*s)) { v = v * 10 + *s++ - '0'; if (v > 200) v = 200; } return v; }
    return def;
}
static uc ln;
static void L(const char *a, uc n, const char *b) {            /* one numbered line: a, number n (if n), b */
    ln += 10; catn(ln); catc(' '); cat(a); if (n) catn(n); cat(b); catc('\n');
}
static void handle(const char *in) {
    uc t = 0, n = num_in(in, 0), d, i; const char *p;
    rcl(); ln = 0;
    if (has(in, "TIMES") || has(in, "HELLO") || has(in, "PRINT")) t = 1;
    else if (has(in, "COUNT")) t = 2;
    else if (has(in, "TABLE") || has(in, "MULTIPLICATION")) t = 3;
    else if (has(in, "SUM") || has(in, "ADD UP") || has(in, "TOTAL")) t = 4;
    else if (has(in, "RANDOM") || has(in, "DICE")) t = 5;
    else if (has(in, "GUESS") || has(in, "GAME")) t = 6;
    else if (has(in, "BOX") || has(in, "SQUARE") || has(in, "FRAME")) t = 8;
    else if (has(in, "LINE")) t = 7;
    else if (has(in, "FACTORIAL")) t = 9;
    else if (has(in, "FOREVER") || has(in, "LOOP")) t = 10;
    else if (n && pz->last_tpl) t = pz->last_tpl;                 /* only a number: same program with the new number */
    if (!t) { cat("I CAN WRITE: PRINT HELLO 5 TIMES, COUNT TO 20, TABLE OF 7, SUM TO 10, DICE, GUESS GAME, LINE, BOX, FACTORIAL 5, FOREVER."); return; }
    pz->last_tpl = t;
    cat("ZX81 BASIC:\n");
    switch (t) {
    case 1: d = n ? n : 5; p = word_after(in, "PRINT ");
        L("FOR I=1 TO ", d, ""); L("PRINT \"", 0, ""); rl--; rb[rl] = 0;
        if (p && !isd(*p)) catw(p, 10); else cat("HELLO");
        cat("\"\n"); L("NEXT I", 0, ""); break;
    case 2: L("FOR I=1 TO ", n ? n : 10, ""); L("PRINT I", 0, ""); L("NEXT I", 0, ""); break;
    case 3: d = n ? n : 7; L("FOR I=1 TO 10", 0, ""); L("PRINT ", d, ";\" X \";I;\" = \";I*"); rl--; rb[rl] = 0; catn(d); cat("\n"); L("NEXT I", 0, ""); break;
    case 4: L("LET S=0", 0, ""); L("FOR I=1 TO ", n ? n : 10, ""); L("LET S=S+I", 0, ""); L("NEXT I", 0, ""); L("PRINT S", 0, ""); break;
    case 5: L("PRINT INT (RND*", n ? n : 6, ")+1"); break;
    case 6: L("LET N=INT (RND*", n ? n : 100, ")+1"); L("INPUT G", 0, ""); L("IF G<N THEN PRINT \"TOO LOW\"", 0, ""); L("IF G>N THEN PRINT \"TOO HIGH\"", 0, "");
        L("IF G<>N THEN GOTO 20", 0, ""); L("PRINT \"YES!\"", 0, ""); break;
    case 7: L("FOR I=0 TO ", n ? n : 40, ""); L("PLOT I,I/2", 0, ""); L("NEXT I", 0, ""); break;
    case 8: d = n ? n : 20; L("FOR I=0 TO ", d, ""); L("PLOT I,0", 0, ""); L("PLOT I,", d, ""); L("PLOT 0,I", 0, ""); L("PLOT ", d, ",I"); L("NEXT I", 0, ""); break;
    case 9: L("LET F=1", 0, ""); L("FOR I=1 TO ", n ? n : 5, ""); L("LET F=F*I", 0, ""); L("NEXT I", 0, ""); L("PRINT F", 0, ""); break;
    default: L("PRINT \"HELLO\";", 0, ""); L("GOTO 10", 0, ""); break;
    }
    cat("TYPE IT IN AND RUN.");
}
