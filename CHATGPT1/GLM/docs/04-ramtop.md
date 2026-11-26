# RAMTOP architecture (idea by Sandor)

Loader block (BASIC, `boot/loader.bas`): explains the program, waits for ENTER, then
`POKE 16388,0 : POKE 16389,98 : NEW` -> RAMTOP = 98*256 = 25088 ($6200). Memory above RAMTOP survives NEW and later LOADs
(tape images start at VERSN $4009, RAMTOP at $4004 is not in the image). No reboot is needed (a reset would re-run the power-on RAM test and reset RAMTOP).
The next block's engine checks PEEK 16388/16389; if RAMTOP is wrong it asks the user to run the loader again.

## Memory plan
| Zone | Where | Size | Notes |
|---|---|---|---|
| Persistent zone | $6200-$7FFF (above RAMTOP) | ~7.5 KB | engine (4.5), dictionary+tokenizer data (2.5), state/persist (0.25), reserve |
| Program zone | $4009-$61FF | ~8.5 KB | sysvars, BASIC lines, knowledge pack in REM line (replaced by every topic LOAD), D_FILE, chat/context |
Engine is loaded once, copied up by a tiny copier in block 2, then every topic pack is just a program with REM data + `RAND USR` into the resident engine.
Knowledge pack therefore ~5.5-6 KB (not 6.5). Optional: D_FILE inside the persistent zone (GLM v0.5 proved a relocated D_FILE with its own display layer), gives back ~0.8 KB.
Saving state: engine copies the state (profile, flags, events) into the REM tail of the program, then ROM SAVE writes it (the persistent zone itself is not saved by SAVE).
RAMTOP value will be tuned (`POKE 16389,n`) when the final engine size is known.
