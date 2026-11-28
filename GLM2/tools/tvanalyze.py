#!/usr/bin/env python3
"""tvanalyze.py - a szim TV-snapshotjanak sorprofil-analizise.

Minden kepsorra kiirja: fekete/feher/szurke pixelszam (a TV 520x380).
A 'fekete sav' = olyan sor, ahol a fekete arany > 90% de a kep egyebkent el
(nem teljesen fekete frame). A kozepu sav (a TV 380 sorabol ~150-230 kozott
esik a lathato kepterulet) a ZX81 'osszeomlott kepen' a sync-tores jele.
Hasznalat: tvanalyze.py <ppm> [osszefoglalo]
"""
import sys
from PIL import Image


def analyze(path, summary=False):
    im = Image.open(path).convert("L")
    w, h = im.size
    px = im.load()
    rows = []
    for y in range(h):
        black = white = gray = 0
        for x in range(w):
            v = px[x, y]
            if v < 64:
                black += 1
            elif v > 192:
                white += 1
            else:
                gray += 1
        rows.append((black, white, gray))
    if summary:
        tot_black = sum(r[0] for r in rows)
        tot_white = sum(r[1] for r in rows)
        tot_gray = sum(r[2] for r in rows)
        tot = w * h
        # teljesen fekete sorok (a kep 'kiesese')
        dead_rows = [y for y, r in enumerate(rows) if r[0] / w > 0.95]
        # vegtelen szurke (nem megrajzolt) sorok
        untouched = [y for y, r in enumerate(rows) if r[2] / w > 0.95]
        print(f"{path}: {w}x{h}, fekete {tot_black/tot:.1%}, "
              f"feher {tot_white/tot:.1%}, szurke {tot_gray/tot:.1%}")
        if dead_rows:
            # osszefuggo savok
            bands = []
            s = dead_rows[0]
            p = dead_rows[0]
            for y in dead_rows[1:]:
                if y == p + 1:
                    p = y
                else:
                    bands.append((s, p))
                    s = p = y
            bands.append((s, p))
            print(f"  FEKETE SAVOK (>95% fekete sorok): {bands}")
        else:
            print("  nincs teljesen fekete sav")
        if untouched:
            print(f"  nem-megrajzolt (szurke) sav: {untouched[0]}-{untouched[-1]} "
                  f"({len(untouched)} sor)")
        # a lathato kepteruleten (y=32-272, a Drawer WinT..WinB) van-e tartalom
        vis = rows[32:273]
        vb = sum(r[0] for r in vis)
        vw = sum(r[1] for r in vis)
        print(f"  lathato ablak (32-272): fekete {vb}, feher {vw} "
              f"({'ELO a kep' if vw > 10000 else 'HALOTT/majd halott'})")
        return
    for y in range(0, h, 4):
        b, wh, g = rows[y]
        tag = "FEKETE-SOR" if b / w > 0.95 else ("szurke" if g / w > 0.9 else "")
        print(f"  y={y:3d}: fekete {b:3d} feher {wh:3d} szurke {g:3d} {tag}")


if __name__ == "__main__":
    analyze(sys.argv[1], len(sys.argv) > 2)
