# ZX81 notes

- 48K+ RAM required since v0.3.2 (16K cannot hold engine + model; see
  docs/T81_FORMAT.md "Size reality" - the user's dump proved a 16K
  configuration was the cause of the RUN resets: SP=$7FFC with the
  image reaching $EB3A).
- Class D: 7725 params / 7.9 KB blob, ctx 8, vocab 45, emb 16, h1 40.
  Replies are short and rough - that is the point of the smallest class.
- v0.3.2.2: the crt0 boots SLOW ($F2B), not FAST - the chat is
  interactive, FAST blanks the display and freezes FRAMES (16436),
  which hal_msleep() waits on. Display NMI is transparent to the code.
- zx81_line() = D_FILE + 1 + row*33: D_FILE points at the leading 0x76,
  the first cell of row 0 is D_FILE+1 (proven vs appmake + ARTILLER.P).
- Tape via the ROM routine (z88dk wrappers). Topic packs load in
  sequence (`/load 0`, `/load 1`); there is no random access on ZX81
  tape. Turbo on the ZX81 usually needs hardware mods (and the display
  must be disabled during loading); standard speed: a 7.9 KB model
  loads ~2 min.

## ZX81 single-cassette v0.3.2.2

The ZX81 ships as ONE EO81/T81 block (RTCHAT): a .P-convention RAM
image from 0x4009 - sysvars, "1 REM <engine @16514>", "2 RAND USR VAL
"16514"", the Class D model at $C940, the D-file at $E820, VARS 0x80
marker at $EB39 (docs/T81_FORMAT.md, tools/t81build.py). The T81
container was reverse-engineered from the user's demo.T81 and verified
with tools/t81verify.py (20 blocks, exact EOF). The full z88dk 1.8
build pipeline runs end-to-end in scripts/build_zx81.sh; structural
validation (t81check) is green.
