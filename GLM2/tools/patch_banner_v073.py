#!/usr/bin/env python3
"""patch_banner_v073.py - a boot banner verzioszaman javitasa a szalagban.

A boot16.c banner-e ("retro-tinyllm 16k  v0.6.2") a v0.6.2 ota nem volt
frissitve, ezert minden atadott szalag ugyanazt mutatta - a felhasznalo
nem tudta megkulonboztetni a regit az ujtol (a memoria-dumpja egy v0.6x
kori blobot mutatott: LD A,($4026);LD B,A a v0.7.3-as LD A,($4028);LD C,A
helyett). Ez a script a .t81 RTBOOT01 blokkjaban levo ASCII "v0.6.2"
stringet "v0.7.3"-ra csereli (azonos hossz, semmi mas nem valtozik).
"""
import sys

OLD = b"retro-tinyllm 16k  v0.6.2"
NEW = b"retro-tinyllm 16k  v0.7.3"


def patch_t81(path):
    buf = bytearray(open(path, "rb").read())
    off, hits, patched = 4, 0, 0
    while off + 48 <= len(buf):
        off += 32  # blokknev
        end = buf.find(b"\0", off)
        ln = int(bytes(buf[off:end]).decode("latin1").strip() or "0")
        off += 16
        data = buf[off:off + ln]
        if OLD in data:
            buf[off:off + ln] = data.replace(OLD, NEW)
            patched += 1
        hits += 1
        off += ln
    open(path, "wb").write(bytes(buf))
    return hits, patched


if __name__ == "__main__":
    for path in sys.argv[1:]:
        hits, patched = patch_t81(path)
        print(f"{path}: {hits} blokk, banner csere: {patched} db")
