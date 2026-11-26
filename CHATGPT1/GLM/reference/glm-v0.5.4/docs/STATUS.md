# Port status (v0.2.2)

Legend: **tested** = built and run in this repo's CI/sandbox;
**compiles** = build path provided, binary not produced here;
**source** = HAL written, needs the platform toolchain; **skeleton** =
structure + platform notes, entry points to verify.

| port | status | model class | console | disk | tape | learning |
|------|--------|-------------|---------|------|------|----------|
| host | tested | A/B/C/D | stdio | files | .rtp pulse file | mem + finetune |
| c64 (cc65) | source | C | kernal BSOUT | kernal LOAD + .d64 writer | kernal LOAD, turbo spec | mem |
| amiga (vbcc) | source | A/B/C | DOS console | AmigaDOS | serial stream (doc'd) | mem + finetune |
| zxspectrum (z88dk) | skeleton | C | z88dk stdio | DOS via HAL (todo) | RT-turbo (.tzx) + ROM (.tap) | mem |
| zx81 (z88dk) | built (16K T81 v0.5) | G | own D-file layer | - (model in image) | named multiload + ROM SAVE | profile + 10 events |
| c16 (cc65) | skeleton | D (C on Plus4) | kernal | 1551 (rare) | kernal tape | mem |
| enterprise (z88dk/sdcc) | skeleton | B | EXOS console | EXDOS | EXOS channel | mem |
| tvc | skeleton | C | ROM print | TDC (rare) | ROM cassette | mem |

## What IS proven (host-tested, bit-exact)

- The whole engine: int8 forward pass, per-row shifts, sampling, chat
  loop, fact memory, tape codec, fine-tuning, topic router, clock,
  dated journal, screen-RAM scratch tier (11/11 test suites green).
- The C engine is bit-identical to the numpy integer simulator
  (`tests/test_golden.c`): float model -> pack.py -> int8 blob -> same
  argmax/logits in C as in Python, on 100 golden vectors per class.
- RT-turbo round-trip incl. CRC rejection of an in-transit flipped bit
  and +-16% speed wobble.
- d64 images verified by read-back (5 files).

## Porting a new machine (checklist)

1. Copy the closest `ports/<machine>/hal_<machine>.c`.
2. Implement the 12 `rt_hal.h` functions. Console + tape-loading are the
   minimum; disk saves can return -1.
3. Pick the model class and set `-DRT_MAX_* -DRT_APP_MODEL_MAX` limits in
   the port Makefile (see BINARY_FORMAT.md for sizes).
4. Build with the engine .c files as-is; no engine edits are ever needed.
5. Test: `/info`, then a chat line, then `/teach` + ask, then `/load`.

Timing (measured patterns, see the plan PDF for details): the engine cost
per character is one int8 MAC per parameter; a Class C model is ~2-3 s per
character on a 1 MHz 6502 with software multiply, Class A ~1-2 s on a
7 MHz 68000.

## v0.2.2: screen-RAM scratch tier (docs/SCRATCH.md)

| machine | cells | bits | bytes | visual cost |
|---------|-------|------|-------|-------------|
| ZX Spectrum | 2048 | 8 | 2048 | top 8 rows show garbage |
| C64 | 200 | 8 | 200 | top 5 rows show garbage |
| C16/Plus4 | 200 | 8 | 200 | top 5 rows show garbage |
| ZX81 | 256 | 6 | 192 | top 8 rows show glyph noise |
| Amiga / Enterprise / TVC | - | - | - | OS/banking owns the screen |

Engine side is host-tested (unit tests + an end-to-end "power cycle"
through the emulated tier). The port-side cell addressing (Spectrum
thirds, ZX81 D-file layout, screen-code tables) follows the platform
manuals but is NOT hardware-validated yet - same status as the ports
themselves.


## v0.5.0: 16K ZX81 edition - boot chain, memory, cassette save

Layout (all addresses derived or scanned at runtime; D_FILE moved to
$7C00 so the model zone reaches $7BEF):

- $4009-0x407C sysvars (ROM-verified: IY=$4000, VERSN=$4009,
  D_FILE=$400C, RAMTOP=$4026)
- line 1: REM + engine (13 766 B) + 48-B persist tail (inside the
  line, below any line-2 edit point -> the ROM's own SAVE writes it)
- line 2: IF USR 16514 = 1 THEN LOAD "RTxxxxxx" ($7E+FP tails after
  every number literal - the S-STK-DEC requirement; the "2" literal
  in line 3 is $1E, not $20)
- line 3: IF USR 16514 = 2 THEN SAVE "RTBASE16" (the /save handoff)
- model: class G (ctx 8, emb 8, h1 5, 832 params, 1030-B blob) at
  the chain end, found by an "RTLM" magic scan ($7AFF..$4090) -
  immune to NEW/quickload rebuilding the display file elsewhere
- tag + name table (14 x 8 display-code names) follow the blob
- $7C00 D-file: rows 0-2 packed state ('RT2', 72 B -> 96 cells of
  6 bits), rows 3-7 the 5-row scrollback mirror; every cell
  0x00-0x3F (never $76, never >= $40 - the freeze rule)

Cassette: 15 blocks (RTBOOT01 boot + RTBASE16 + 13 Hungarian topic
packs). The boot asks date/time/name/gender, packs the profile into
the work rows and chains. app16 mines the user's lines for emotion
keywords (faj/boldog/szomoru/faradt/szerelmes) into 10 two-byte
events with a 14-day decay; /save mirrors the state into the line-1
tail and returns 2 so BASIC runs the ROM's SAVE over the rewound
tape position. SHIFT+1..4 set the sampling sharpness, SHIFT+9 the
two-pass reasoning toggle, SHIFT+5/0 page the scrollback.

The v0.4.5 pack/unpack lost byte b1's bits 6-7 (cell2 mask $03 on
pack vs $0F on unpack) - the state magic could never survive a
round trip. v0.5 fixes it in one place per binary (boot C, app asm
and $F0 + rrca x4) and sim_v05.py proves the full boot -> base ->
memory -> save -> reload round trip.

## v0.5.1 hotfix: instant emulator reset on load

Root cause: the v0.5.0 multiload's FIRST block (the boot) was a SHORT
image - the display file at $7C00 was not in the image, so on a freshly
reset machine the ROM's display generator ran away over uninitialized
RAM and the emulator auto-reset. Fix, back on the v0.4-proven pattern:
D_FILE restored to $7B00 (477 B of machine stack, the proven budget),
the boot and EVERY standalone .p are FULL images (pad + blank display
file + VARS marker - all fourteen .p files are 15 889 B, exactly the
size of the user's working v0.4.5 base), and inside the multiload only
the boot is FULL while the other fourteen blocks stay SHORT so the
packed state rows and the scrollback survive block swaps. The 48-B
line-1 REM tail is gone: the ROM's own SAVE already writes the D-file
(the state rows live inside the $4009..VARS-end save range), verified
by sim section 4. Engine 13 533 B. t81check16_v05 + sim_v05 green.

## v0.5.2 hotfix: the freeze root cause - the stack-top word at the wrong sysvar offset

Byte-comparing the new images against THREE independent references (the
z88dk appmake +zx81 canonical .P, the user's WORKING v0.4.5 rt16_base.p,
and the emulator-written demo.T81) exposed the real v0.5.0 regression:
the rewritten sysvars() had moved the $7FFD stack-top word from $4026 to
$4028 (a bogus "DF_SZ" write clobbered it). The emulator's quickload
takes the machine stack pointer from the word at $4026 - with $0002
there every call pushed into the ROM area and the machine died silently.
Also fixed: MSIZE 1970->1030 in app16.c (TAG_AT/NAMTAB_AT pointed 940 B
past the real tag/table, so the /topic name poke could never work) and
the unpack_state() asm, where the cell temporaries were loaded into D/E
while DE was the b[] target pointer - decoded state went to the ROM/void
instead of the state block, so profiles and memories never restored.
Sysvar region now byte-equal to the working v0.4.5 (one benign byte at
$4019 = appmake's canonical 0). Version strings on screen: v0.5.2.
Checker + sim green. Working dir marked GLM per the user's request.

## v0.5.3: the flicker fix - the ROM's lower screen respected

The user reported a FAST-like flashing picture ("only occasionally
visible"). The binary analysis showed the crt0 DOES call SLOW ($0F2B)
and never FAST - the flash came from the display-file/ROM state
mismatch: our files carried DF_SZ=0 while the canonical state (z88dk
appmake AND the emulator-written demo.T81, both) is DF_SZ=2 at $4022 -
the ROM owns the last TWO display rows as the lower screen, and our HAL
printed into rows 8-23 including those. Fixes: DF_SZ=2 in the builder
sysvars, HAL console narrowed to rows 8-21 (22-23 = the ROM's lower
screen, never written), the boot's hal_msleep pauses removed (an
infinite FRAMES busy-wait if the NMI ever stalls), banners v0.5.3.
Per the user's request every output file name now carries GLM + the
version (GLM_v053_rt16_*.p/.t81, GLM_v053_RT16_TOPICS.t81); the
standalone boot cassette is gone (a boot-only tape has nothing to chain
to - that was the "RT16_BOOT.T81 does nothing" report). Checker + sim
green.

## v0.5.4: the FAST symptom's real root cause - display re-assert in every block

The user still saw the FAST-mode behaviour after v0.5.3 ("the picture
only flashes for a moment when keys are pressed"). Binary analysis of
our v0.5.3 images AND the user's WORKING v0.4.0 reference (from their
uploaded "müködő alapok" zip) ruled out two earlier hypotheses: the
crt0's SLOW call is present and correct in both, and the compiled
code's IY usage is byte-identical between the working and the broken
build (5x PUSH IY, 0x IX - the -IXIY z80asm swap; not the cause). The
structural difference is the multiload chain itself: the ZX81 ROM's
tape LOAD command runs with the display deliberately OFF (FAST) for
read-timing, and after each block swap nothing re-enabled the
continuous NMI display - so the only frames ever generated were the
ones during getk()'s ROM keyboard scans, exactly one flash per key
poll. Fix: boot16.c and app16.c now re-assert the display at the very
start of main() - LD IY,$4000 (the ROM sysvar base, written as
"ld ix,$4000" because the -IXIY z80asm swap turns it into FD 21 00 40)
followed by CALL $0F2B (SLOW). This covers every entry path: emulator
quickload, the ROM's USR, and the post-LOAD chain continuation. The
checker now asserts the 7-byte sequence in all 15 blocks; engine
13542 B (was 13535 B), blocks 14899 B short, tape 225199 B, banners
v0.5.4, output names GLM_v054_*. Checker + sim green.
