/* persistent zone layout (shared by all packs) */
struct PZ {
    u8 mg0, mg1;                   /* 'P','Z' = valid */
    u8 req;                        /* pack request for the BASIC loader lines: 0 none, 1 DRAW 2 UTIL 3 STORY 4 POEM 5 CODE 7 FOOD, 6 = base */
    u8 temp;
    u8 h_, m_, s_, a_, clk;        /* clock and "clock was set" flag */
    u8 nusers, cur;
    u8 day, mon, yr, datev;        /* date when it was set (yr = year - 2000); datev = date was set */
    u8 dayc;                       /* midnights passed since then (the UTIL pack adds them) */
    char names[3][9];              /* users */
    char bot[9];                   /* the name given to Pico */
    u8 rem_on, rem_h, rem_m; char rem_txt[12];
    char learn[4][16];             /* taught words: alias[8], meaning[8] */
    u8 last_tpl, last_num;         /* memory of the CODE pack */
    char carry[48];                /* text handed to the next pack */
};
