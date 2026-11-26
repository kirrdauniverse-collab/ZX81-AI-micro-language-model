# Knowledge as facts (v0.6) - the model builds its sentences

Sandor did not like the many stored sentences. Now nothing but a few short social routines is stored as text.
Knowledge = **facts** `subject relation object` plus **cue words** and a **grammar** in the engine (`engine/pico.c`).

    FACT MARS IS PLANET      FACT MARS PROP RED      FACT MARS PROP COLD      FACT MARS ORBIT SUN      FACT SUN DEF THE
    -> "MARS IS A RED, COLD PLANET. IT IS DRY. IT GOES AROUND THE SUN."

Relations: IS (class) PROP (adjective) HAS LIVES MADE CAN LIKES PART ORBIT PRON (he/she) NAMED MIX (colours) DEF (takes "THE") ALIAS (YOU -> PICO).
Cue words (WHERE, MADE, CAN, LIKE, HAVE, NAME, COLOUR, MIX, AROUND ...) say which relation a question asks for.
How a reply is made: tokenise (typos, endings, taught words, smileys) -> find the topic (the word with most facts, YOU = Pico) ->
a social routine if one matches ("how are you") -> otherwise build sentences: "X IS A <adj>, <adj> <class>.", then "IT HAS ...", "IT LIVES IN ...",
"IT GOES AROUND ...", "IT WAS MADE BY ...", "IT CAN ...". The temperature (SHIFT+1..4) sets how many sentences are built and, at 3-4, which relations are skipped.
Questions about a class list its members ("CAR, BUS, TRAIN AND PLANE ARE VEHICLES"). Two colours give their mix.
Cost: a fact is 4 bytes, a new word about 6 - the base now holds 102 facts and 167 words in 1.6 KB (before: 42 stored sentences in 2.4 KB).

Add knowledge: edit `knowledge/kb.txt`, run `tools/build_kb.py` (`tools/build_all.sh` does it). Multi-word names use _ (BUS_DRIVER). The old
sentence-retrieval engine and its corpus are kept in `reference/`.
Limits: grammar is templated (no past tenses, plural only by the final S, "A/AN" by the first letter); no reasoning beyond what the facts say.
