# Status v0.4 - packs, persistent zone, DRAW pack

## Architecture (idea of Sandor: "virtual memory")
- Loader (BASIC): explains, then POKE 16388,128 / 16389,127 (RAMTOP = $7F80) and NEW. BASIC LOAD clears everything below RAMTOP, so
  anything that must survive a pack change lives in the persistent zone (PZ, 128 bytes at $7F80, above RAMTOP): clock, temperature,
  pack request, text handed to the next pack (later: user names, learned words, reminders).
- Every pack is a complete program (UI layer + its own skills/knowledge): `PICO` = chat model, `DRAW` = drawing. A pack asks for
  another one by setting PZ.req and returning to BASIC; two spliced BASIC lines then run `IF PEEK 32642=1 THEN LOAD "DRAW"`.
  (tools/add_basic.py). Base asks "THAT IS A SEPARATE TOPIC: DRAWING. LOAD THE DRAW PACK NOW? Y/N" first.
- Tape (emulator): LOADER, then PICO/DRAW pairs repeated 6x (+PICO), because the emulator can only search forward. Real machine: rewind by hand
  (a turbo loader and the second release for real hardware come later).
- Without the loader (RAMTOP not set) the base refuses to load packs.

## Built and checked here
Host tests of the model and of the drawing engine (ASCII canvas); screen simulator (tools/screen_sim.py: layout, scroll, 0x76 rows intact,
no bad cells); z88dk-ticks cycle counts; tape structure (t81verify); BASIC line tokenisation.
Memory: base image 15.0 KB (1.27 KB free below $7F80), DRAW image 10.9 KB.

## NOT verified (needs the emulator)
RAMTOP behaviour after NEW, PZ surviving LOAD, the BASIC LOAD lines, tape name matching and forward search, the keyboard scan and SHIFT keys,
block graphics codes in DRAW, timing, clock continuity across a pack change.

## Base content
67 sentences, 252 words (see knowledge-report.md): identity, small talk, ZX81/retro, space, health, colour mixing, directions, vehicles, shapes,
KIRRDA, e-mail (shown with an inverse 0 instead of the at sign), playful answer to rude words. Unknown word -> "I DO NOT KNOW THE WORD X".
Input: 4 rows (120 characters).

## Next
Object definitions for DRAW (house, car, planets ...) and relative placement; UTILITY pack (time/date/names, maths, ABC sort, colours);
STORY / POEM / CODE packs; smiley tokens; learning words; reminders and idle questions; thinking bar; turbo loader and real-hardware release.
