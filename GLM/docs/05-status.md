# Status v0.3

Done: own display layer (v0.2); v0.3 adds: unknown-word reply ("I do not know the word X") and "no data" reply instead of random guesses,
trigger weighting (questions like "how are you" now hit their trigger), fuzzy word matching 5-6x faster (pre-filter), offset tables replaced by
sequential scans (-1 KB), English loader. Response time measured with z88dk-ticks: known words ~3 M T-states, unknown words ~6 M
(SLOW mode is about 4x slower than FAST: roughly 4 s / 8 s). Before: 35 M (about 44 s).
Memory: image 14.7 KB, 1.6 KB free for the stack. Knowledge: 75 sentences, 252 words (see knowledge-report.md).
Not tested on a real ZX81. Not done yet: persistent zone (RAMTOP), names/date/commands, math/sort/colour skills, learning words,
reminders/idle questions, thinking bar, stories/poems/drawing templates, topic packs.
