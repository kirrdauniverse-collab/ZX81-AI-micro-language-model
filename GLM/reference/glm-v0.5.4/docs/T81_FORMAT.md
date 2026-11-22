# T81 cassette format - reverse-engineered from demo.T81

Source: the user-uploaded demo.T81 (122964 bytes). Verified by a strict
parser over the whole file (20 blocks, exact EOF) - tools/t81verify.py.

## Container layout

    "EO81"                                4 bytes, file magic
    repeat per block:
        name    32 bytes   ASCII, NUL-padded (block/program name)
        length  16 bytes   ASCII decimal digits + NUL padding
        data    <length> bytes

Every block's data = a complete ZX81 RAM IMAGE from 0x4007 (the VERSN
system variable) up to the program's workspace, i.e. system variables +
BASIC lines / machine code + D-file + VARS + edit line in one contiguous
image. This is the EightyOne (EO) emulator's ZX81 quickload/tape
container; chained demo tapes (MENU -> 3DMAZE -> ...) hold one image
per program.

Checks that pinned the format: block N's start = block N-1's start +
32 + 16 + length, for all 20 blocks; the last block ends exactly at EOF.

## Our cassette (v0.3.1, tools/t81build.py)

ONE block named RTCHAT = a ZX81 RAM image 0x4007-0xEB46, laid out
EXACTLY like the .P files z88dk's own appmake generates (byte-for-byte
loader conventions, proven on every ZX81 emulator):

    0x4007-0x407C   system variables - a command-level block lifted
                    from a z88dk appmake .P (CDFLAG=1 SLOW, ERR_NR=2,
                    MEM=0x405D...), with the layout pointers recomputed:
                    D_FILE, DF_CC=D_FILE+1, VARS=D_FILE+793,
                    E_LINE=VARS+1, CH_ADD=E_LINE+4,
                    STKBOT=STKEND=E_LINE+5, D_FILE copy at 0x4027
    0x407D          BASIC line 1: 1 REM <engine> - ROM line format
                    [num u16][len u16 = text+NEWLINE][text][0x76].
                    The REM text holds the engine code from 0x4082 =
                    16514 (the z88dk crt0 org); the crt0's own
                    "ld a,(hl); jp start" prefix is what hides the REM
                    in the listing. 34478 bytes.
    line 2          2 RAND USR VAL "16514" - byte-identical to the
                    appmake loader line (F9 D4 C5 0B = RAND USR VAL",
                    digits with '0'=0x1C, closing 0B = "), so a plain
                    RUN starts the engine
    +0x76           end-of-program NEWLINE
    0xC940          Class D model blob, 7904 bytes - raw RAM above the
                    program end: the interpreter never scans past it,
                    so any byte (incl. 0x76) is safe there
    0xE820          D-file: 24 rows x (32 spaces + 0x76)
    0xEB39          VARS: 0x80 end marker (D_FILE+793, appmake spacing)
    0xEB3A          E_LINE edit line (0x76 = empty), CH_ADD/STKBOT/
                    STKEND per the appmake relations; the ROM's editor
                    and calculator workspace grows up from here to
                    RAMTOP (~5.3 KB head room on a 48K machine)

Why the engine sits inside a REM line and the model above the program:
the ZX81 ROM expects [PROG][D-file][VARS][E_LINE][workspace] in that
order, and its editor shifts the D-file/VARS region on every line
entry - so nothing of ours may live below the workspace. The first
build (v0.3) had the engine at 0x4400 ABOVE the D-file with 3DMAZE's
unpatched E_LINE/CH_ADD/STKBOT pointers (pointing straight into the
model blob) and CDFLAG=0: typing one key in the editor scribbled over
the engine and the machine reset on ENTER/RUN. v0.3.1 rebuilds the
whole program area the way z88dk does it.

The D-file HAL reads the D_FILE system variable from 16396 (0x400C) -
confirmed byte-exact against a fresh z88dk-appmake reference AND the
original ARTILLER.P, whose 96-line chain ends exactly at its D_FILE word
(0x4A12 at header offset 3) - and the ZX81 glyph table in hal_zx81.c
uses the real codes - digits are 0x1C-0x25 (proven by the digits inside
appmake's own loader line), quote 0x0B, letters 0x26-0x3F.

v0.3.2.2 adds two fixes proven by the user's memory dump (registers +
RAM) and the appmake/ARTILLER references:

  - zx81_line() adds +1: D_FILE points at the display file's LEADING
    0x76 newline; the first cell of row 0 is D_FILE+1 (= DF_CC after
    CLS). Without the +1 every row write eats the previous row's 0x76.
  - the crt0 now calls SLOW ($F2B), not FAST ($F23): this is an
    interactive chat, and FAST blanks the display for the whole session
    (nothing to see at the "you> " prompt) AND freezes the FRAMES
    counter (16436) that hal_msleep() busy-waits on - a guaranteed
    deadlock there. The display NMI is transparent to the C code.

That dump also settled the "machine resets on RUN" report for good:
SP=$7FFC means the emulator ran a 16K configuration while this image
needs 48K+ - see "Size reality" below.

## Size reality

The compiled engine (sccz80 -O2, 12 modules: chat, memory, journal,
clock, router + HAL; tape codec and the screen-RAM scratch tier are
stubbed out on ZX81 - see docs/SCRATCH.md) is 34479 bytes of Z80 code
(34478 before the v0.3.2.2 +1 cell fix). A 16K pack physically cannot
hold engine + model: the image needs a ZX81 with 48K RAM or more
(0x4000-0xFFFF; a standard emulator setting - the classic 64K
emulators work too, anything that maps RAM across the full 48K window).

This is not theoretical: the user's memory dump showed SP=$7FFC - the
stack sitting at the top of a 16K configuration - while D_FILE=$E820
and the model at $C940 need RAM far above that. With a 16K RAMTOP the
crt0 keeps BASIC's stack at $7FFC and the C stack grows straight into
the engine's own code ($4082-$C72F); the first deeper call executes
overwritten bytes and the PC runs away. Configure the emulator for
48K+ RAM before loading the cassette.

## Build pipeline (scripts/build_zx81.sh, z88dk 1.8 debs)

The tools are driven directly (zcpp -> sccz80 -> copt -> fixxref ->
z80asm -Mo per module, then one z80asm -a -m -Mo link with the ndos,
zx81_clib and z80iy_crt0 libraries, the crt0 wrapper as the source and
everything else as objects - exactly zcc 1.8's own link shape). The
z88dk 1.8 toolchain needed these documented workarounds, all captured
in the script and tools/fixxref.py:

- z80asm 1.0.30 resolves plain INCLUDEs from the CWD only -> the lib
  include files are copied into the build directory first;
- sccz80 emits INCLUDE "#name" -> bin/copt-wrap.sh strips the '#';
- sccz80 emits bodyless XDEFs for prototyped functions -> fixxref.py
  converts them to XREFs;
- z80asm has no <stddef.h> -> a NULL/size_t shim is written inline;
- getk() lives in <stdio.h>, not <zx81.h> (calling it undeclared makes
  an unresolvable _getk symbol);
- a stale crt0.o silently poisons every later link (-a reuses it by
  date stamp) -> the script deletes it before linking;
- linking the copt-optimised sources directly produces bogus "Out of
  range" errors -> link the objects, and touch them after the .asm
  twins are written so -a does not re-assemble;
- the 32-bit shift/and/or library modules cannot be linked by z80asm
  1.0.30 at all -> engine/rt_scratch.c was rewritten with 16-bit
  arithmetic (all 11 host test suites pass, incl. the scratch e2e
  round-trip).

## Testing the cassette (emulator guide)

1. Configure the emulator for a ZX81 with at least 48 KB RAM
   (EightyOne: Hardware > ZX81 > RAM 48K; 64K configurations are fine
   too). A 16 KB configuration cannot hold the image.
2. Load retro_tinyllm.t81 as a tape/quickload file. The block is named
   RTCHAT. The machine shows the usual 0/0 report with the cursor.
3. Type R (RUN) and NEWLINE. BASIC line 2 ("2 RAND USR VAL "16514"")
   jumps into line 1's REM and the engine boots: banner, "you>"
   prompt. Typing is ZX81-native; the text area is rows 8-23 of the
   D-file. /help lists the commands, /quit exits back to BASIC.

## Status

- T81 parser/verifier: DONE, validated against demo.T81 (20 blocks).
- RAM image builder: DONE (tools/t81build.py) - the produced T81
  verifies structurally and byte-exact (model == base_d.bin, code ==
  rtchat.bin, both BASIC lines decode, every appmake pointer relation
  holds, CDFLAG=1).
- Engine: full 12-module build links with 0 errors; 11/11 host tests
  green.
- OPEN: emulator validation on real EightyOne/Paulware hardware
  settings (first user test of v0.3 crashed in the editor - v0.3.1
  fixes the loader layout as described above and needs a re-test).

## Framework isolation tests (v0.3.1, tools/t81frame.py)

Two TINY cassettes to bisect loader problems - both keep every byte
below 0x4400, so they load on ANY emulator RAM setting (even 1K):

- rtframe_empty.t81 (988 bytes, block "EMPTY"): system variables +
  a single empty "1 REM" line + D-file. Loading it must show the
  normal 0/0 screen on any emulator; RUN does nothing (no executable
  line) and just reports 0/0. If even THIS resets the machine, the
  loader/sysvar block itself is broken.
- rtframe_ok.t81 (1028 bytes, block "OK"): adds 26 bytes of machine
  code in "1 REM <code>" (writes "RT81 OK" into display row 12 using
  the D_FILE pointer, then RET) plus the loader line "2 RAND USR VAL
  "16514"". After RUN you should see RT81 OK in row 12 - proving the
  full chain: loader, sysvars, both BASIC lines, USR call, D_FILE
  pointer, and a clean return to BASIC.

The engine cassette itself places its D-file at 0xE820 and the model
at 0xC940: on an emulator configured with less than 48K RAM those
addresses do not exist, and the display interrupt reading a phantom
D-file restarts the machine immediately after loading - exactly the
"cannot even inspect memory" symptom. The demo.T81 blocks keep
everything below 0x7FFF, which is why they run everywhere. Check the
RAM setting first (EightyOne: Hardware > ZX81 > RAM 48K or more).

## v0.3.2 - the T81/.P builder was wrong; found by diffing appmake output

The user suggested producing .P files too, in case the T81 construction
was bad - it was. Generating an official reference
(`z88dk-appmake +zx81 -b rtframe.bin --org 16514`) and diffing it against
our builder exposed three structural bugs, all of which explain every
symptom so far (0/0 screen and cursor but reset on RUN, and with the big
cassette an immediate reset right after loading):

1. Wrong image base: the block is a memory image from 0x4009 (VERSN),
   not 0x4007. Every pointer's target sat 2 bytes above where the
   pointer claimed, and the ROM's fixed PROG walk at 0x407D read the
   template tail + shifted headers as line numbers -> garbage lines ->
   reset on RUN. (MEM=0x405D at 0x401F in the reference confirms the
   base.)
2. BASIC line numbers were written little-endian; the ZX81 ROM stores
   line NUMBERS big-endian and line LENGTHS little-endian (ROM quirk).
   Our "line 1" read as line 256 to the ROM.
3. The D-file missed its LEADING 0x76 newline: a ZX81 display file is
   793 bytes = 0x76 + 24 x (32 chars + 0x76), and that first 0x76
   doubles as the end-of-program marker for the ROM's line walk.

The fixed builder (tools/t81frame.py + tools/t81build.py, shared core)
follows the appmake convention byte-for-byte: image ends at the VARS
0x80 marker, no E_LINE workspace in the file (the ROM builds it), no
alignment gap. Proof: the rebuilt rtframe_ok.p is BYTE-IDENTICAL to the
appmake-generated reference. The engine cassette adds an explicit 0x76
end-of-program marker before the model blob (the D-file's own leading
0x76 is not adjacent there), and keeps the model at 0xC940 / D-file at
0xE820 (48K+ still required for the big cassette).

Both builders now also emit a .P twin next to every .t81, so the same
content can be loaded the standard way if a T81 quickload misbehaves.

## v0.3.2.1 - same bug inside the engine code (found via user test)

User report: the .P and .T81 frames load and LIST perfectly, but RUN of
rtframe_ok only reported 0/2 and showed nothing. That report is not an
error: code 0 = OK, "/2" = finished at line 2 - the code RAN and
returned cleanly, it just wrote to the wrong place. Both the frame test
(ports/zx81/rtframe.asm) and the engine HAL (ports/zx81/hal_zx81.c,
zx81_dfile()) read the D_FILE system variable from 16394 (0x400A) -
the address our own pre-v0.3.2 format analysis had mislabeled. 0x400A
is E_PPC; the real D_FILE sits at 16396 (0x400C), as the appmake
reference proves. With E_PPC's value (1) as the D-file base, the pokes
landed at 0x199 - inside the ROM, a no-op, hence the invisible-but-
clean run. Both now read 0x400C; the compiled rtchat.bin contains no
`ld hl,(0x400A)` any more and exactly one `ld hl,(0x400C)`; the frame
remains byte-identical to the freshly regenerated appmake reference.
