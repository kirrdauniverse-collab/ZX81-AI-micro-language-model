#!/usr/bin/env python3
"""patch_v073.py - a v0.7.2 -> v0.7.3 binaris javitas (a fekete-kep fix).

A v0.7.2 gen_reply vegen a blokk (SET 6 + SET 7 + CALL $0207) a ROM tanc
RET NC agen vegzodott: a CDFLAG $C0-nal (bit6==bit7) a $0207-es tanc az
alabbi miatt azonnal visszater a fegyverzes nelkul:

    $0207: LD HL,$403B / LD A,(HL) / RLA / XOR (HL) / RLA / RET NC

A 17x OUT ($FE) fegyverzo sorozat CSAK a tovabbfuto agon van. A generator
a gen_reply eleji OUT ($FD) ota ki volt kapcsolva -> a scanline NMI-k
megszuntek -> a kepernyo ES a border (ami ugyanabbol az NMI vonbol keszul)
fekete lett - mikozben a gep folytatta a munkat. (A szim ugyanitt hibazott:
a D-file-t memoriabol olvasta, a video-ut nem modellezte - a v0.7.3 sim mar
a doScanline NMI-szamlalojat figyeli.)

A javitas (17 bajt, drop-in csere a $4CC3-nal):
    CD 40 7A    CALL $7A40   ; a blob: PUSH IY / LD IY,$4000 / CALL $0207
                             ; (CDFLAG=$80: a tanc VEGIGFUT - 17x OUT $FE
                             ;  fegyverzi a generatort, SET 7, a walk az
                             ;  uj valasz sorokat rendereli), majd a blob
                             ;  FRAMES varakozasa + tiszta RET (IY helyre)
    3A 3B 40    LD A,($403B)
    F6 40       OR $40       ; SET 6 IY nelkul (CDFLAG=$C0 = SLOW)
    32 3B 40    LD ($403B),A
    06 11       LD B,$11     ; a main()-beli disp_on burst:
    D3 FE       OUT ($FE),A  ; 17x OUT $FE - a tanc vegi OUT ($FD) utana
    10 FE       DJNZ         ; visszakapcsolja a generatort

Hasznalat: patch_v073.py <v072 rtchat16> <kimenet>
"""
import sys


def main():
    src, dst = sys.argv[1], sys.argv[2]
    d = bytearray(open(src, "rb").read())
    base = 16514
    off = 0x4CC3 - base
    regi = bytes([0xFD, 0xE5, 0xFD, 0x21, 0x00, 0x40, 0xFD, 0xCB, 0x3B,
                  0xF6, 0xCD, 0x07, 0x02, 0xFD, 0xE1])
    if d[off:off + 17] == bytes([0xCD, 0x40, 0x7A, 0x3A, 0x3B, 0x40, 0xF6,
                                 0x40, 0x32, 0x3B, 0x40, 0x06, 0x11, 0xD3,
                                 0xFE, 0x10, 0xFE]):
        print("mar patchelt - nincs teendo")
        open(dst, "wb").write(bytes(d))
        return
    assert d[off:off + len(regi)] == regi, \
        "a v0.7.2 blokk nem talalhato a $4CC3-nal: " + \
        " ".join("%02x" % b for b in d[off:off + 17])
    uj = bytes([0xCD, 0x40, 0x7A,       # CALL $7A40
                0x3A, 0x3B, 0x40,       # LD A,($403B)
                0xF6, 0x40,             # OR $40
                0x32, 0x3B, 0x40,       # LD ($403B),A
                0x06, 0x11,             # LD B,$11
                0xD3, 0xFE,             # OUT ($FE),A
                0x10, 0xFE])            # DJNZ
    assert len(uj) == 17
    d[off:off + 17] = uj
    open(dst, "wb").write(bytes(d))
    print("v0.7.3 patch kesz:", dst, "(17 bajt csere a $4CC3-nal)")


if __name__ == "__main__":
    main()
