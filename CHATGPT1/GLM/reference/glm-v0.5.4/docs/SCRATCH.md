# Screen-RAM scratch tier (v0.2.2)

Borrowing display memory as data storage - the classic 8-bit trick (the
ZX Spectrum disk-copy utilities lived in the top third of the screen,
garbled image and all). Suggested for this project by the user; this
document is the design + the numbers.

The bytes of screen lines the app does NOT print to are ordinary RAM.
Instead of paying BSS for the learned-facts table and the journal, the
engine keeps them in those cells as a third persistence tier next to
disk and tape: smaller than any file, no drive needed, and it survives
model reloads, topic switches and warm reboots.

## Capacities per machine

| machine | cell store | cells | bits/cell | bytes | visual cost |
|---------|-----------|-------|-----------|-------|-------------|
| ZX Spectrum 48K | top third of pixel RAM, 0x4000-0x47FF | 2048 | 8 | 2048 | top 8 char rows show garbage |
| C64 | screen rows 0-4, 0x0400-0x04C7 | 200 | 8 | 200 | top 5 rows show garbage |
| C16/Plus4 | TED screen rows 0-4, 0x0800-0x08C7 | 200 | 8 | 200 | top 5 rows show garbage |
| ZX81 | D-file rows 0-7 (8 x 32 cells) | 256 | 6 | 192 | top 8 rows show glyph noise |
| Amiga / Enterprise / TVC | - | -1 | - | - | none (tier absent) |

The ZX81 is the special case: its ROM display routine renders every
D-file cell, so a stored byte must BE a legal character. Legal values
are the single-cell glyph codes 0x00-0x3F (space ... A-Z); keyword /
control codes (the two-cell sequences the user remembered) and the 0x76
HALT line terminator are forbidden. `rt_scratch_pack6()` therefore packs
the byte stream at 6 bits per cell: 3 bytes -> 4 cells, and only glyph
codes can ever come out. 256 cells x 6 bits = 192 bytes - close to the
~180 the user estimated.

The Amiga is a deliberate -1: Intuition/graphics.library owns the
screen and with hundreds of KB free there is nothing to gain. On the
Enterprise the video memory is EXOS-banked per segment, on the TVC it
is video-segment switched; borrowing cells there is future work.

## What is stored, and the fitting policy

Blob layout (byte stream): magic `RSC1`, version, fact count (u8),
journal count (u16 LE), then the newest facts (24 B key + 64 B value
each) and newest journal entries (4 B date + 80 B text each) that fit,
then an xor checksum of the record bytes. 9 bytes of overhead; the
checksum sits at the end so the whole stream is written sequentially.

`rt_scratch_plan(capacity, n_facts, n_journal)` decides the split:

- if everything fits (e.g. Spectrum: 16 facts = 1408 B + 7 entries =
  588 B + 9 B header = 2005 B <= 2048 B), all of it is stored;
- otherwise a proportional split by raw record size, newest-first
  within each kind. Measured plans (covered by unit tests):
  - ZX81, 192 B: 0 facts + 2 journal entries;
  - C64, 1016 B worth of policy -> 200 B tier: proportional share of
    both kinds;
  - Spectrum, 2048 B: 9 facts + 14 entries in the worst case, or all
    of a light state.

`rt_scratch_restore()` fills only what the caller is missing, checks
magic/version/counts/xor, and refuses cleanly on a blank or corrupt
screen (garbage never gets interpreted as state).

## ROM safety (why each machine needs its own text layer)

The ROM print/scroll routines would move ALL screen lines and scramble
the stored cells, so on the four machines with a tier the HAL owns the
text area below the reserved rows: direct screen/D-file writes, own
cursor, own scroll (Spectrum: rows 8-23 with the ROM font at 0x3D00;
C64/C16: rows 5-24 with PETSCII screen codes; ZX81: rows 8-23 in the
D-file). `panic()` still uses the ROM - it is a fatal path. Details
worth knowing:

- Spectrum: ROM print would scroll the whole screen; the HAL renders
  glyphs itself instead.
- C64: a VIC matrix move could free ~1 KB of invisible old screen RAM,
  but needs bank/charset handling - noted as future work; the 200-cell
  version needs none of that.
- ZX81: assumes the uniform post-CLS D-file layout (33 bytes per line,
  0x76 terminators), and the 64-glyph font has no `!` or `'` - the HAL
  substitutes the nearest punctuation.
- Real machines keep the cells only until power-off; the tier's job is
  surviving model reloads, topic switches and warm reboots, not
  replacing disk.

## Testing status

- Engine + policy + codec: unit-tested on the host (pack6 round-trip
  incl. tails, glyph-safety of every cell, the plan table above,
  store/restore both bit widths, checksum rejection of a flipped cell,
  clean refusal on a blank screen).
- End to end: the host HAL emulates all four layouts
  (`RT_SCRATCH=spec|c64|c16|zx81`) and can persist the cells to a file
  (`RT_SCRATCH_FILE`) so a save -> power-cycle -> restore run is
  scripted in `tests/scratch_e2e.sh`: teach + note, /save, fresh
  directory (no disk files), boot -> facts and journal come back from
  the emulated screen. 11/11 test suites green.
- Port-side addressing (Spectrum thirds, ZX81 D-file layout, screen
  code tables) follows the platform manuals but is NOT
  hardware-validated yet - same status as the ports themselves; see
  docs/STATUS.md.
