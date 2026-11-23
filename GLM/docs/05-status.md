# Status v0.2

v0.2: own display layer instead of the z88dk console (which ignored newlines and showed lower case as inverse).
Screen: row 0 title | rows 1-12 chat, scrolling up from the bottom | row 13 MEMORY bar | rows 14-18 storage (hex cells of the
last context bitmap, a first use of the screen as storage) | rows 19-20 input | row 21 inverse status (temperature, tokens in/out,
response time, clock) | row 22 key hints | row 23 state.
Chat headers are inverse: "YOU hh:mm:ss TOK n" and "PICO hh:mm:ss TOK n t.tS". Body text is normal (never inverse).
Keys: SHIFT+1..4 temperature, SHIFT+0 delete, ENTER send. At start the clock is set by typing HHMM.
Display safety: every cell is 0-63 or 128-191; row ends (0x76) are never written; the simulator (tools/screen_sim.py) checks this
and found one real bug during development (storage row overwrote a row end).
Memory: image 14.8 KB (code incl. runtime ~7.9 KB, model 5.2 KB, buffers), 1.6 KB free for stack.
NOT tested on a real ZX81/emulator yet: keyboard scan, SHIFT detection, timing (FRAMES), display switch.
Still missing: boot profile, persistent memory/RAMTOP zone, SHIFT+5/6 scroll, SHIFT+9 reasoning, SHIFT+S save, smileys/mood/insult
mirroring, unknown-word learning, calendar, turbo tape.
