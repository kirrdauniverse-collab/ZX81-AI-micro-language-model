# Status v0.1 (first real engine)

Built and tested on the host (gcc): tokenizer with stemming and typo tolerance, retrieval-seeded bigram generation with
temperature, trigger|response knowledge format. Built for the ZX81 with z88dk (11.5 KB image, 5.1 KB code incl. runtime + 5.2 KB model).
NOT yet tested on a ZX81 or emulator (no ROM here). Not yet in: boot profile, persistent memory, RAMTOP zone, inverse headers,
clock/tokens/time, SHIFT keys, save, smileys/mood/insult mirroring, unknown-word learning, calendar, turbo tape.
Tape: GLM/out/PICO_AI_v0.1.t81 = LOADER (BASIC) + PICO (engine). RAMTOP loader (loader.bas) is for the next version.
Model: GLM/knowledge/base.txt -> tools/build_model.py -> engine/model.h (253 words, 75 sentences).
Build: tools/build_model.py knowledge/base.txt engine/model.h ; zcc +zx81 -vn -O3 zx81_main.c pico.c -o pico -create-app
