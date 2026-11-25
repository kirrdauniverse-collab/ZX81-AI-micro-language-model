# Two releases

**Emulator** (`out/PICO_AI_emulator.t81`): every block in one T81. The emulator can only search forward, so PICO and the packs repeat 5 times
(`ROUNDS=8 tools/build_all.sh` for more round trips).

**Real machine** (`out/real/`): `PICO_AI_real.t81` and the single `.P` files (loader, PICO, DRAW, UTIL, STORY, POEM, CODE, FOOD).
Standard ZX81 tape speed is about 300 baud: a 15 KB block takes roughly 7 minutes. Order on the tape: LOADER, PICO, DRAW, UTIL, STORY, POEM, CODE, FOOD.
After a pack finishes, Pico asks for PICO again: rewind to the PICO block (the loader line does `LOAD "PICO"`, and the ROM waits for it).

**Turbo loader: not built.** A fast tape routine must be timed to the cycle and cannot be checked in an emulator that does not model it; shipping an
untested one would risk a corrupt save of a program that cannot be debugged. Plan: a small resident routine at the top of the persistent zone, a
2x-3x pulse scheme, a checksum per block, and a switch to the standard ROM loader. It needs a test on real hardware or an emulator with exact tape timing.
