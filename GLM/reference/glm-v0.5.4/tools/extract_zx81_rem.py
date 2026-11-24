#!/usr/bin/env python3
"""Extract machine-code payload from a z88dk ZX81 .P file (line 1 REM)."""
from pathlib import Path
import sys
SYSVARS, REM, EOL = 116, 0xEA, 0x76
def extract(src):
    data = Path(src).read_bytes()
    if len(data) < SYSVARS + 5: raise ValueError(f"{src}: too short")
    off = SYSVARS
    line_no = int.from_bytes(data[off:off+2], "big")
    line_len = int.from_bytes(data[off+2:off+4], "little")
    start, end = off+4, off+4+line_len
    if line_no != 1 or end > len(data): raise ValueError(f"{src}: invalid first line")
    body = data[start:end]
    if len(body) < 3 or body[0] != REM or body[-1] != EOL:
        raise ValueError(f"{src}: first line is not a terminated REM")
    return body[1:-1]
if __name__ == "__main__":
    if len(sys.argv) != 3: raise SystemExit("usage: extract_zx81_rem.py input.p output.bin")
    code = extract(sys.argv[1])
    if not code: raise SystemExit("empty REM payload")
    Path(sys.argv[2]).write_bytes(code)
    print(f"extracted {len(code)} bytes")
