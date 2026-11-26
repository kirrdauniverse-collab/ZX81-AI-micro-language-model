# PICO ZX81 — investigation breakpoint

## Last clearly known-good runtime milestone

`ports/zx81/mistral_zx81ai16.t81` on branch `pico/zx81-stable-2026-11`.

The stable documentation records V1.5 and V1.8 as verified emulator regressions, and the project conversation records the AI16 line actually reaching the HELLO greeting.

## First persistent-failure family to re-examine

The later R3/R4 line:
- `dist/PICO_AI_R3_16K.t81`
- `dist/PICO_AI_R4_16K.t81`
- alias `dist/PICO_ZX81_16K.t81`

The exact first failing binary must be established by replaying the recorded sequence, not guessed from the version number. R3 is therefore the first comparison target, with AI16 as the known-good control.

## Immediate forensic comparison

Compare, without rebuilding first:
1. AI16 .P vs R3 .P byte-for-byte.
2. AI16 .T81 vs R3 .T81 container structure.
3. D_FILE pointer and actual D-file rows.
4. E_LINE / CH_ADD / STKBOT / STKEND / VARS relations.
5. engine origin and end.
6. all absolute addresses used by the engine.
7. every byte written to D_FILE during boot/thinking.
8. RAMTOP and stack placement.
9. T81 block name, length field and payload boundaries.

Only after this comparison should a new fix be attempted.
