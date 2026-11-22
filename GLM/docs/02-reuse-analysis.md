# Reuse analysis of earlier attempts (uploaded ZIP)

Reference copies live in `GLM/reference/` (source, docs, tools, tapes). Nothing was changed.

## What exists
| Attempt | Approach | Result |
|---|---|---|
| GLM v0.5.4 / v0.6.5 | C (z88dk) engine + tiny int8 neural net (class G: 832 params, 1030-byte blob), numpy training, `pack.py`, 13 Hungarian topic packs, boot track, date/name/gender profile, 10 memory events with 14-day decay, scrollback, SHIFT+1..4 temperature, SHIFT+9 two-pass reasoning | Full 15-block tape verified structurally; engine takes ~13.7 KB, leaving ~1 KB for the model |
| Claude_ZXAI.t81 | chat block + 6 topic blocks (EO81) | structure valid |
| Mistral_zx81ai16.t81 | single block | structure valid |
| zx81_tinyllm_16k pack (python-built Z80 asm) | hand-written Z80, engine ~1.25 KB, keyword topic switch, 12.5 KB topic RAM | smallest engine, keyword based |
| BOOT_UI_v2 "stripes" tape | boot UI | known bug: two wide black stripes (invalid data in display file) |

## Reusable (verified from the sources)
- **.T81 container** (EO81): 4-byte magic, then per block 32-byte name + 16-byte ASCII length + RAM image from 0x4007. `tools/t81verify.py`, `t81build.py`, `t81frame.py` parse and build it. Use as-is.
- **ZX81 layout facts** proven there: D_FILE points at the leading 0x76; first row cell = D_FILE+1; digits 0x1C-0x25, letters 0x26-0x3F; engine inside a REM line; model above the program; line 2 number literals need the 0x7E+5 byte FP tail; run in SLOW mode, not FAST.
- **Boot -> chain loading** with named LOAD, and persistent state in a tail inside the REM line so the ROM SAVE writes it (this is the "do not overwrite" memory idea).
- **6-bit cell packing** for D_FILE work rows (cells 0x00-0x3F, never 0x76) = the freeze-proof rule.
- **Training pipeline** (`training/train.py`, `pack.py`, `intsim.py`): float -> int8 blob bit-exact with the C engine (golden tests).

## Problems found
- The C engine is far too big for the 16 KB budget (13.7 KB vs the 3-5 KB target); sccz80 emits ~10 bytes per statement.
- A 832-parameter character-level net is too small for coherent chat; v0.5 topics are Hungarian, the new spec is English.
- Stripes bug: matches the "invalid data in the display file" diagnosis; the fix is the 0x76/0x40+ write filter already used in the work rows, applied to every D_FILE write.

## Decision for the new engine
Hand-written Z80 (like the zx81_tinyllm_16k pack's builder), reusing: T81 tools, memory/persist-tail design, 6-bit D_FILE packing, topic-pack-on-tape model, training pipeline ideas. Replacing: C engine, class-G net.
