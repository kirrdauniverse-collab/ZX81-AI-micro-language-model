#!/usr/bin/env python3
"""Guard against storing binary profile/scrollback data in the visible D-FILE."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
boot = (root / "ports/zx81/boot16.c").read_text(encoding="utf-8")
app = (root / "ports/zx81/app16.c").read_text(encoding="utf-8")
hal = (root / "ports/zx81/hal_zx81.c").read_text(encoding="utf-8")
builder = (root / "tools/t81build16.py").read_text(encoding="utf-8")

checks = {
    "boot profile stored outside display file": "rt_u8 *dst = (rt_u8 *)0x7A72" in boot,
    "app state stored outside display file": "memcpy((void *)0x7A72, b, ST_BYTES)" in app,
    "app state restored outside display file": "memcpy(b, (const void *)0x7A72, ST_BYTES)" in app,
    "scrollback ring uses hidden memory": "#define ZX81_RING_BASE  0x7ABA" in hal,
    "scrollback ring is bounded to 64 bytes": "#define ZX81_RING_CELLS 64" in hal,
    "build reserves hidden memory": "SCRATCH_BASE = 0x7A72" in builder,
    "build refuses to overwrite hidden memory": "tabend < SCRATCH_BASE" in builder,
    "console clear leaves D-FILE row terminators alone": "for (col = 0; col < 32; col++)" in hal,
    "page display uses explicit console output": "hal_console_char_at((8 + i) * 32 + c, row[c])" in app,
}
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)
if not all(checks.values()):
    raise SystemExit(1)
print("Hidden screen storage regression: %d/%d PASS" % (len(checks), len(checks)))
