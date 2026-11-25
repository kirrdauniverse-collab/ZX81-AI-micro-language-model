# Status v0.5 - base + six packs

## Packs (each is a complete program: shared UI layer + its own skill; named blocks on the tape)
| Block | What it does | Size |
|---|---|---|
| PICO | chat model (typo tolerance, unknown-word and no-data replies, smileys, taught words, thinking bar, idle question), asks Y/N before loading a pack | 15.8 KB (335 B free) |
| DRAW | block-graphics canvas 64x32: shapes (circle, box, square, line, triangle) and objects (house, car, Jupiter, Saturn, tree, sun, moon, star, heart, ship, rocket, face, snowman, flower) with size words/numbers, LEFT/RIGHT/TOP/BOTTOM, NEXT/ABOVE/BELOW/"LEFT OF" relative to the thing named ("a tree next to the house"), BIGGER/SMALLER, UNDO, CLEAR | 12.7 KB |
| UTIL | set time/date, what day/time, several user names (3), name for Pico, calculator (+ - * / x plus minus times divided, whole numbers), ABC / numeric sorting, reminders, teaching words ("X means Y") | 14.2 KB |
| STORY | template story (hero from the request or your name, KIRRDA variant) | 9.5 KB |
| POEM | two rhyming couplets about the topic | 8.6 KB |
| CODE | few-line ZX81 BASIC programs (print N times, count, table, sum, dice, guess game, line, box, factorial, forever); a bare number changes the last program | 10.2 KB |
| FOOD | 7 short recipes (coffee included) | 8.5 KB |

## How packs work
Loader (BASIC) sets RAMTOP to $7F40 and NEW. The persistent zone (192 bytes at $7F40, above RAMTOP) keeps clock, date, temperature, names, reminder,
taught words and the request for the next pack. A pack asks for another by setting `req` and returning to BASIC; a spliced BASIC line runs
`IF PEEK 32578=n THEN LOAD "NAME"`. Base: "THAT IS A SEPARATE TOPIC: ... LOAD THE X PACK NOW? Y/N". The sentence is handed over (carry) and executed by the pack.

## Checked here
Host tests of the model, UTIL (maths, sort, dates incl. weekday vs. Python's calendar for 2001-2099, month roll-over), STORY/POEM/CODE/FOOD generators, the
drawing engine; screen simulator for base, UTIL, STORY, CODE, DRAW (rows, scroll, no bad cells); z88dk-ticks cycle counts (known words ~2-3.4 M T-states);
tape structure; BASIC line tokenisation; struct size (181 of 192 bytes). Two buffer overflows were found and fixed by reading the code.

## NOT verified (needs the emulator / a real machine)
RAMTOP and the zone surviving LOAD, the BASIC LOAD lines, name matching and forward search on tape, keyboard scan and SHIFT keys, block graphics codes, timing and
clock continuity during a pack change, stack depth (335 bytes assumed enough).

## Not done
Turbo loader (see 07-real-hardware.md). Reminders only beep visually (no sound on the ZX81). The base knowledge is small (42 sentences, 189 words): what is left after
the code; topic packs with real knowledge (space, health, animals, food ...) are the next step. SHIFT+5/6 scrolling, SHIFT+9 reasoning, SHIFT+S save are not implemented.
