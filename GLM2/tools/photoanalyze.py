#!/usr/bin/env python3
"""photoanalyze.py - a valodi TV-rol keszult foto programozott elemzese.

A modell nem fogad kepet, de a foto tartalma szambol is kivanithato:
- hol a kepernyo (legvilagosabb nagy regiok),
- soronkenti sotetseg-profil (fekete sav detektalasa),
- van-e szoveg (kis fekete piksel-furdok a vilagos teruleten),
- a kepernyo holt-e (nincs szinkron -> teljesen osszeomlott/gyurott kep)
  vagy el (stabil, egyenletes sorok).
Kimenet: sorprofil (soronkent % fekete a kepernyo-regioban) + osszegzes.
"""
import sys
from PIL import Image, ImageFilter


def main(path):
    im = Image.open(path).convert("L")
    w, h = im.size
    px = im.load()

    # 1) glob hisztogram: a TV kepernyoje a foto legvilagosabb nagy resze
    hist = im.histogram()
    tot = w * h
    bright = sum(hist[160:]) / tot
    dark = sum(hist[:64]) / tot
    print(f"{path}: {w}x{h}, vilagos>160: {bright:.1%}, sotet<64: {dark:.1%}")

    # 2) oszlopprofil: hol van a kepernyo vizszintesen (vilagos oszlopok)
    col_b = []
    for x in range(w):
        cb = 0
        for y in range(0, h, 2):
            if px[x, y] > 160:
                cb += 1
        col_b.append(cb / (h // 2))
    # a kepernyo oszlopai: col_b > 0.3
    cols = [x for x, c in enumerate(col_b) if c > 0.3]
    if not cols:
        print("NINCS vilagos kepernyo-regio -> teljesen sotet foto "
              "(halott kepernyo vagy nagyon sotet szoba)")
        # sotet sav keresi a teljes kepen
        rows = []
        for y in range(h):
            b = sum(1 for x in range(0, w, 2) if px[x, y] > 160)
            rows.append(b / (w // 2))
        band = [y for y, r in enumerate(rows) if r > 0.25]
        print(f"  vilagos savak a sotet kepen: {len(band)} sor "
              f"({len(band) / h:.1%})" + (f" elso-utolso: {band[0]}-{band[-1]}"
              if band else ""))
        return

    x0, x1 = cols[0], cols[-1]
    print(f"kepernyo oszlopai: x={x0}..{x1} ({x1 - x0} px szeles)")

    # 3) sorprofil a kepernyo-regioban: soronkenti fekete arany
    rows = []
    for y in range(h):
        b = d = 0
        for x in range(x0, x1 + 1, 2):
            v = px[x, y]
            if v > 160:
                b += 1
            elif v < 64:
                d += 1
        n = (x1 - x0 + 1) // 2 + 1
        rows.append((b / n, d / n))

    # vilagos sorok = kepernyo sorai
    yrows = [y for y, (b, d) in enumerate(rows) if b > 0.35]
    if not yrows:
        print("a kepernyo-regioban nincs vilagos sor -> osszeomlott kep")
        return
    y0, y1 = yrows[0], yrows[-1]
    print(f"kepernyo sorai: y={y0}..{y1} ({y1 - y0} px magas)")

    # 4) elemzes a kepernyo-negyzeten belul
    print("\nsorprofil (10 reszre bontva, fentrol lentre):")
    seg = max(1, (y1 - y0) // 10)
    for i in range(10):
        s = y0 + i * seg
        e = min(y1, s + seg)
        mb = sum(rows[y][0] for y in range(s, e)) / max(1, e - s)
        md = sum(rows[y][1] for y in range(s, e)) / max(1, e - s)
        bar = "#" * int(mb * 30)
        print(f"  {i:2d}. y={s:4d}-{e:4d}: vilagos {mb:5.1%} "
              f"sotet {md:5.1%} |{bar}")

    # fekete sav: olyan savornek, ahol a sotet > 60% de a szomszedos sorok
    # vilagosak (a kepernyo el)
    bands = []
    in_band = False
    for y in range(y0, y1 + 1):
        if rows[y][1] > 0.6 and rows[y][0] < 0.3:
            if not in_band:
                in_band, bs = True, y
        else:
            if in_band:
                bands.append((bs, y - 1))
                in_band = False
    if in_band:
        bands.append((bs, y1))
    print(f"\nFEKETE SAVOK a kepernyon (sotet>60% es vilagos<30%): {bands}")

    # szoveg: a vilagos kepernyo-negyzetben kicsi fekete furdok (0.5-15% soronkent)
    txt = [y for y in range(y0, y1 + 1) if 0.005 < rows[y][1] < 0.5]
    print(f"szovegszeru sorok (1-50% sotet): {len(txt)} "
          f"({'VAN szoveg a kepen' if len(txt) > 20 else 'nincs egyertelmu szoveg'})")

    # global diagnozis
    all_dark = sum(rows[y][1] for y in range(y0, y1 + 1)) / (y1 - y0 + 1)
    all_bright = sum(rows[y][0] for y in range(y0, y1 + 1)) / (y1 - y0 + 1)
    print(f"\nOSSZEGZES: kepernyo vilagos {all_bright:.1%}, sotet {all_dark:.1%}")
    if all_bright > 0.5 and len(bands) == 0:
        print("-> ELO, stabil kepernyo (szinkron rendben)")
    elif all_bright > 0.5 and bands:
        print(f"-> ELO kepernyo FEKETE SAVVAL: {bands} (reszleges osszeomlas)")
    elif all_bright < 0.2:
        print("-> HALOTT kepernyo (nincs kep: generator le vagy CPU halt)")


if __name__ == "__main__":
    main(sys.argv[1])
