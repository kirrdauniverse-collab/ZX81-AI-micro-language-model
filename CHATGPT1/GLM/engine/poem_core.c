/* POEM pack: two rhyming couplets about the topic of the request (@ = topic). */
static const char *const cp[12] = {
    "THE @ SAILS ACROSS THE SKY\nSO LIGHT, SO FREE, SO HIGH",
    "I THINK OF @ EACH NIGHT\nIT SHINES LIKE SILVER LIGHT",
    "OH @, YOU ARE LIKE THE SEA\nSO DEEP, SO WIDE, SO FREE",
    "THE @ IS A LITTLE STAR\nTHAT SHINES FROM FAR, FAR",
    "WHEN @ COMES WITH THE SUN\nTHE DAY IS FULL OF FUN",
    "A SONG ABOUT @ IN THE RAIN\nI WILL SING IT AGAIN",
    "I DREAM OF @ UNDER A TREE\nAND THE WORLD IS KIND TO ME",
    "MY @ IS WHERE I CALL HOME\nNO NEED TO RUN, NO NEED TO ROAM",
    "WITH @ THE NIGHT BECOMES DAY\nAND ALL THE SADNESS GOES AWAY",
    "THE MOON KNOWS @ WELL\nHER SECRETS NO ONE WILL TELL",
    "OH @, YOU FILL MY HEART\nWITH A NEW AND HAPPY START",
    "@ FALLS LIKE WINTER SNOW\nAND MAKES MY WORLD GLOW" };
static const char *const dflt[4] = { "LOVE", "THE SEA", "SPRING", "FRIENDS" };
static void couplet(uc i, const char *t) {
    const char *s = cp[i];
    for (; *s; s++) { if (*s == '@') cat(t); else catc(*s); }
}
static void handle(const char *in) {
    char t[14]; const char *p = topic_word(in); uc n = 0, a, b;
    rcl();
    if (p) { while (*p && n < 12 && !(*p == ' ' && p[1] && n > 8)) t[n++] = up(*p++); t[n] = 0; } else strcpy(t, dflt[rr(4)]);
    a = rr(12); b = (a + 1 + rr(11)) % 12;
    cat("A POEM ABOUT "); cat(t); cat("\n"); couplet(a, t); cat("\n"); couplet(b, t);
}
