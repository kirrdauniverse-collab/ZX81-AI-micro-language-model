#!/usr/bin/env python3
"""Structural verifier for PIOCO's full 24-row ZX81 display file."""
from pathlib import Path
import os
import sys

os.environ.setdefault("RT16_LAYOUT", "ramtop")
sys.path.insert(0, str(Path(__file__).resolve().parent))
import t81build16 as b


def blocks(path):
    data = Path(path).read_bytes()
    if not data.startswith(b"EO81"):
        raise ValueError("missing EO81 magic")
    pos, out = 4, []
    while pos < len(data):
        if pos + 48 > len(data):
            raise ValueError("truncated block header")
        name = data[pos:pos+32].split(b"\\0", 1)[0].decode("ascii")
        size = int(data[pos+32:pos+48].split(b"\\0", 1)[0])
        pos += 48
        if pos + size > len(data):
            raise ValueError(f"truncated payload in {name}")
        out.append((name, data[pos:pos+size]))
        pos += size
    return out


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_ramtop_tape.py file.t81")
    if b.DFILE_LINES != 24 or b.DFILE != 0x7B00 or b.DFILE_SIZE != 793:
        raise SystemExit("builder is not using the full 24-row ZX81 D-FILE")

    items = blocks(sys.argv[1])
    if len(items) != 15 or items[0][0] != "RTBOOT01" or items[1][0] != "RTBASE16":
        raise SystemExit(f"expected BOOT + BASE + 13 topics, got {len(items)}")

    for name, image in items:
        if len(image) < 116:
            raise SystemExit(f"{name}: truncated image")
        if int.from_bytes(image[3:5], "little") != b.DFILE:
            raise SystemExit(f"{name}: D_FILE mismatch")
        if int.from_bytes(image[7:9], "little") != b.VARS:
            raise SystemExit(f"{name}: VARS mismatch")
        if int.from_bytes(image[11:13], "little") != b.ELINE:
            raise SystemExit(f"{name}: E_LINE mismatch")
        if int.from_bytes(image[32:34], "little") != b.PROG:
            raise SystemExit(f"{name}: NTLIN mismatch")
        if image[28:31] != bytes((0xBF, 0xFD, 0xFF)):
            raise SystemExit(f"{name}: LAST_K/DEBOUNCE mismatch")
        if image[31] != 55 or image[50] != 0x40:
            raise SystemExit(f"{name}: MARGIN/CDFLAG mismatch")

    boot = items[0][1]
    dfile_offset = b.DFILE - b.Z
    if len(boot) != b.ELINE - b.Z:
        raise SystemExit(f"BOOT length {len(boot)} != E_LINE span {b.ELINE-b.Z}")
    if boot[dfile_offset:dfile_offset+b.DFILE_SIZE] != b.dfile_blank():
        raise SystemExit("BOOT D-FILE is not 24 complete 32-character rows")

    print("PASS: 15 EO81 blocks; 24-row D_FILE=$%04X; VARS=$%04X; E_LINE=$%04X; CDFLAG=$40" %
          (b.DFILE, b.VARS, b.ELINE))
