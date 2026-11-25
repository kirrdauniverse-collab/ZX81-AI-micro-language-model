/* STORY pack: a short story from templates (intro, event, dialogue, danger, ending), with the hero from the request. */
static const char *const places[6] = { "NEAR A DARK FOREST", "BY A QUIET RIVER", "ON A GREEN HILL", "IN A SMALL VILLAGE", "BY A BLUE LAKE", "UNDER A TALL MOUNTAIN" };
static const char *const things[6] = { "MAP", "KEY", "LAMP", "EGG", "BELL", "BOX" };
static const char *const adjs[5] = { "SHINY", "OLD", "STRANGE", "TINY", "GOLDEN" };
static const char *const helpers[4] = { "WISE OWL", "KIND GIRL", "OLD FROG", "TALKING CAT" };
static const char *const dangers[4] = { "STORM", "HUNGRY WOLF", "DARK CLOUD", "BIG FLOOD" };
static const char *const ends[4] = { "WENT HOME HAPPY", "FOUND A NEW FRIEND", "SAVED THE VILLAGE", "LEARNED TO BE BRAVE" };
static const char *const heroes[6] = { "FOX", "DRAGON", "ROBOT", "GIRL", "BOY", "CAT" };
static void handle(const char *in) {
    char h[12]; const char *place, *thing, *help, *dang, *end, *adj, *p, *the = "THE ", *a; uc kir = has(in, "KIRRDA"), n = 0;
    rcl();
    p = topic_word(in);
    if (has(in, "ABOUT ME") && pz->names[pz->cur][0]) { strcpy(h, pz->names[pz->cur]); the = ""; }
    else if (p) { while (*p && *p != ' ' && n < 10) h[n++] = up(*p++); h[n] = 0; }
    else strcpy(h, heroes[rr(6)]);
    place = places[rr(6)]; thing = things[rr(6)]; adj = adjs[rr(5)]; help = helpers[rr(4)]; dang = dangers[rr(4)]; end = ends[rr(4)];
    if (kir) { strcpy(h, "KIRRDAN"); place = "ON THE PEACEFUL PLANET KIRRDA"; thing = "STAR MAP"; help = "WISE ELDER"; dang = "GLOBAL DISASTER"; end = "ESCAPED TO EARTH AND FOUND A NEW HOME"; }
    a = (adj[0] == 'O') ? "AN " : "A ";
    cat("THE "); cat(adj); catc(' '); cat(thing); cat("\n");
    cat("ONCE UPON A TIME A LITTLE "); cat(h); cat(" LIVED "); cat(place); cat(". ");
    cat("ONE DAY "); cat(the); cat(h); cat(" FOUND "); cat(a); cat(adj); catc(' '); cat(thing); cat(". ");
    cat("\"WHAT IS THIS?\" ASKED "); cat(the); cat(h); cat(". ");
    cat("\"IT IS A GIFT FOR THE BRAVE,\" SAID THE "); cat(help); cat(". ");
    cat("BUT SUDDENLY A "); cat(dang); cat(" CAME. ");
    cat(rr(2) ? "\"HELP!\" CRIED " : "\"I AM AFRAID,\" WHISPERED "); cat(the); cat(h); cat(". ");
    if (kir) { cat("THE KIRRDAN FOLLOWED THE STAR MAP TO EARTH. "); }
    else { cat("WITH THE "); cat(thing); catc(' '); cat(the); cat(h); cat(" WAS STRONGER THAN THE "); cat(dang); cat(". "); }
    cat("IN THE END "); cat(the); cat(h); catc(' '); cat(end); cat(". THE END.");
}
