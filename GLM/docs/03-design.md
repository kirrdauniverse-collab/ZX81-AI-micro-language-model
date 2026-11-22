# Phase 2 - Design (Pico AI, ZX81 16 KB, English)

## 1. Memory map (16 KB: $4000-$7FFF)
| Range | Size | Use |
|---|---|---|
| $4000-$407C | 125 B | system variables |
| $407D-... | ~60 B | BASIC lines: 1 REM (engine), 2 IF USR 16514=1 THEN LOAD "<next>", 3 IF USR 16514=2 THEN SAVE "<state>" |
| engine in REM line | 4.5 KB | hand-written Z80: tokenizer, sampler, UI, tape glue, calendar |
| persist tail (end of line 1) | 64 B | profile + bit flags + events; ROM SAVE writes it, loads never overwrite it |
| tokenizer data + dictionary | 2.5 KB | stems, suffix table, smileys/mood words |
| chat / temp | 2 KB | rolling context, scrollback ring (8 rows), working buffers |
| knowledge pack (tape-replaceable) | 6.5 KB | n-gram tables + canned token sequences |
| display file | ~0.8 KB | D_FILE (SLOW mode) |
| reserve | 50-150 B per module | for later edits |
Sum is within 16384 B; topic packs replace only the knowledge zone.

## 2. .T81 tracks (EO81 container)
1. BOOT: logo, name, welcome, manual yes/no, asks date, time, user name, gender, birth data; packs them into the persist tail; returns 1 -> BASIC loads track 2.
2. ENGINE+BASE: engine plus base knowledge; keeps the persist tail.
3..N. TOPIC packs (science, food, health, fun, retro era, retro tech, futurology, learning, hobby, astronomy, literature, movies, celebrities, art, work, UFO): same load address, replace only the knowledge zone.
4. STATE save: BASIC line 3 SAVEs the program with the persist tail, overwriting the old base block with fresh memories.

## 3. Screen
Row 0: inverse title bar (model name, token count). Rows 1-2: status/thinking bar. Rows 3-21: chat. Message headers are inverse: `YOU 21:08 T12 0.4s` / `PICO 21:08 T18 1.2s` (clock stamp, tokens used, response time). Row 22: input via ROM INPUT in character mode.
Work rows hold only cells 0x00-0x3F; every D_FILE write passes a filter that blocks 0x76 and any code >= 0x40, which also removes the stripes bug of the earlier attempt.

## 4. Bit fields (persist tail)
Byte 0: bit0 sex, bit1 adult, bit2 manual seen, bit3 turbo on, bit4 reasoning on, bit5-6 temperature (1-4), bit7 reserved.
Bytes 1-3: date (day, month, year-2000), 4-5: time, 6-21: name (6-bit packed), 22-23: birth (year, month/day packed).
Bytes 24-39: 8 events x 2 bytes [type+bit flags][day stamp]; older than 14 days = stale, overwritten by new info (e.g. headache bit).
Every packed byte passes a 0x76 guard: if a value would equal 0x76 it is stored with bit 7 set and a mask bit marks it.

## 5. Controls
SHIFT+1..4 temperature, SHIFT+9 reasoning (two-pass: silent draft into context, then visible reply), SHIFT+5 / SHIFT+6 scroll back / forward, SHIFT+S save state, SHIFT+SPACE abort. Turbo tape routine is built in but disabled by default (flag bit3).

## 6. Language model
- Tokenizer: lower-cased words -> stem + suffix (S, ED, ING, ER, LY), numbers kept as digit tokens, smileys/mood tokens (HAHA, WOW, NOOO, :) :( :O), typo tolerance by 3-gram similarity to dictionary entries; unknown words get a temporary token id from chat context.
- Generation: token trigram table with back-off to bigram, sampled with temperature; state vector (mood, topic, politeness, slang) biases the table; canned token sequences only for proverbs and identity answers (stored as tokens, not strings).
- Identity: "who made you?" -> "Made with GLM help by Safar Sandor"; the e-mail uses an inverse 0 instead of the at sign.
- Date: calendar routine from the entered date and the frame counter, a few hundred bytes.
- Insult/slang mirroring via the stylistic state bits.

## 7. Agent commands (text commands in the input line)
/TOPIC n, /SAVE, /LOAD, /TEMP n, /THINK, /DATE, /HELP, /ABOUT.

## 8. Build chain (all in GLM/tools, no manual steps)
knowledge compiler (text -> token tables) -> trainer -> Z80 assembler step -> t81 packer (reuses reference tools) -> verifier -> release ZIP.
