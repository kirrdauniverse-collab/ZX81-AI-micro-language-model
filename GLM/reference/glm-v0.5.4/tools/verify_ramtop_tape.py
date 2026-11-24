#!/usr/bin/env python3
"""Structural verifier for the PIOCO 18-row / 128-byte-reserve tape."""
from pathlib import Path
import os, sys
os.environ.setdefault("RT16_LAYOUT", "ramtop")
sys.path.insert(0, str(Path(__file__).resolve().parent))
import t81build16 as b
def blocks(path):
    data=Path(path).read_bytes()
    if not data.startswith(b"EO81"): raise ValueError("missing EO81 magic")
    pos,out=4,[]
    while pos<len(data):
        if pos+48>len(data): raise ValueError("truncated block header")
        name=data[pos:pos+32].split(b"\0",1)[0].decode("ascii")
        size=int(data[pos+32:pos+48].split(b"\0",1)[0])
        pos+=48
        if pos+size>len(data): raise ValueError(f"truncated payload in {name}")
        out.append((name,data[pos:pos+size])); pos+=size
    return out
if __name__=="__main__":
    if len(sys.argv)!=2: raise SystemExit("usage: verify_ramtop_tape.py file.t81")
    if b.DFILE_LINES!=18 or b.DFILE!=0x7B00 or b.RAMTOP_ADDR!=0x7F80:
        raise SystemExit("builder is not using the 18-row RAMTOP layout")
    items=blocks(sys.argv[1])
    if len(items)!=15 or items[0][0]!="RTBOOT01" or items[1][0]!="RTBASE16":
        raise SystemExit(f"expected BOOT + BASE + 13 topics, got {len(items)}")
    for name,image in items:
        if len(image)<116: raise SystemExit(f"{name}: truncated image")
        if int.from_bytes(image[3:5],"little")!=b.DFILE: raise SystemExit(f"{name}: D_FILE mismatch")
        if int.from_bytes(image[7:9],"little")!=b.VARS: raise SystemExit(f"{name}: VARS mismatch")
        if int.from_bytes(image[29:31],"little")!=b.RAMTOP_ADDR: raise SystemExit(f"{name}: RAMTOP mismatch")
    print(f"PASS: 15 EO81 blocks; D_FILE=${b.DFILE:04X}; VARS=${b.VARS:04X}; RAMTOP=${b.RAMTOP_ADDR:04X}")
