# Phase 1 - Concept and reality check

Target: ZX81 16 KB, single multi-track .T81. Language: English (less space, less CPU).

## Feasible
- Token-based tokenizer (~1-1.5 KB), typo tolerance via char n-gram similarity
- Tiny n-gram language model with temperature (SHIFT+1..4), state vector (mood, topic, context)
- 1-bit properties per byte, masked so 0x76 never appears
- Multi-track tape: boot track (login, persistent memory), engine track, knowledge track
- A few canned answers (proverbs, creator info) stored as token sequences
- Mood words and smileys, insult mirroring, slang mirroring as style bits
- Unknown words resolved from chat context without dictionary updates
- Date arithmetic via a small calendar routine from the entered date

## Limits
- No real open-ended reasoning in 16 KB; answers recombine knowledge-base patterns
- All D_FILE writes go through a filter that blocks 0x76 and command codes

## Memory budget (16 KB)
engine 4.5 KB | dictionary+tokenizer 2.5 KB | chat/temp 2 KB | knowledge 6.5 KB | system vars + D_FILE ~1 KB
Topic packs are loaded from tape to replace the base knowledge.
