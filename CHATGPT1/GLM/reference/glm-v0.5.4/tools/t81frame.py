#!/usr/bin/env python3
# t81frame.py - build the MINIMAL ZX81 framework test files (.t81 + .P).
#
# v0.3.2 REWRITE. The previous builder had three structural bugs, found by
# generating an official reference (z88dk-appmake +zx81) and diffing:
#   1. image base 0x4007 was wrong: a .P / T81 block is a memory image
#      from 0x4009 (VERSN). Every pointer's data sat 2 bytes above where
#      the pointer claimed, and the ROM's fixed PROG walk at 0x407D read
#      garbage line headers - the machine reset on RUN.
#   2. BASIC line numbers were written little-endian; the ZX81 ROM stores
#      line NUMBERS big-endian and lengths little-endian (ROM quirk).
#   3. the D-file missed its LEADING 0x76 newline: a ZX81 display file is
#      793 bytes = 0x76 + 24 x (32 chars + 0x76), and that first 0x76
#      doubles as the end-of-program marker for the line walk.
# The image now ends at the VARS 0x80 marker exactly like an appmake .P
# (the ROM builds the E_LINE workspace itself); everything sits below
# 0x4400 so both files load on ANY emulator RAM setting (even 1K).
#
# Modes:
#   empty - just "1 REM": pure load test (RUN reports 0/0 immediately)
#   ok    - "1 REM <code>" + "2 RAND USR VAL "16514"": the code pokes
#           "RT81 OK" into display row 12 and returns to BASIC
#
# Usage: t81frame.py empty|ok <out.t81> [code.bin]
#        (also writes the .P twin next to the .t81)
import sys

# 116 system-variable bytes, byte-exact from z88dk-appmake +zx81 output
# (mem 0x4009-0x407C; MEM=0x405D confirms the base). Pointers patched in
# build(); offset 32 (mem 0x4029) is appmake's D_FILE copy.
SYSVARS = bytes.fromhex(
    "000100ac40ad40c5430000c643ca430000cb43cb43005d4000020000bffdff37"
    "ac4000000000008d0c0000b8f70000bc21184000000000000000000000000000"
    "0000000000000000000000000000000000000076000000000000000000008420"
    "0000000000000000000000000000000000000000")

Z = 0x4009                          # image base: file byte 0 -> mem 0x4009
PROG = 0x407D                       # fixed ZX81 BASIC program start
CODE_ORG = 16514                    # 0x4082, first byte after "1 REM "


def be16(v):
    """BASIC line number - the ZX81 stores these big-endian."""
    return bytes(((v >> 8) & 0xFF, v & 0xFF))


def le16(v):
    """Line length and system-variable pointers - little-endian."""
    return bytes((v & 0xFF, (v >> 8) & 0xFF))


def zx_digits(n):
    return bytes(0x1C + int(ch) for ch in str(n))


def build_image(lines, blobs=()):
    """Assemble a .P-format image: sysvars, BASIC lines, optional blobs
    (addr, data) above the program, then a 793-byte D-file ending with
    the VARS 0x80 marker. Returns (image_bytes, dfile_addr)."""
    prog = b''.join(be16(n) + le16(len(t) + 1) + t + b'\x76'
                    for n, t in lines)
    if blobs:
        # gap above the program (e.g. a model blob): add an explicit
        # end-of-program 0x76 - the D-file's leading newline is NOT
        # adjacent any more, and BE(0x76, anything) >= 0x7600 always
        # reads as a huge line number, stopping the ROM's line walk.
        prog += b'\x76'
    prog_end = PROG + len(prog)
    top = max([prog_end] + [addr + len(d) for addr, d in blobs])
    dfile = (top + 7) & ~7 if blobs else prog_end
    vars_ = dfile + 793
    eline = vars_ + 1
    image_end = vars_ + 1                     # stop at the 0x80 marker

    img = bytearray(image_end - Z)
    img[0:116] = SYSVARS

    def put(addr, data):
        img[addr - Z:addr - Z + len(data)] = data

    put(PROG, prog)
    for addr, data in blobs:
        put(addr, data)
    put(dfile, b'\x76' + (bytes(32) + b'\x76') * 24)
    put(vars_, b'\x80')

    img[3:5] = le16(dfile)                    # D_FILE      (mem 0x400C)
    img[5:7] = le16(dfile + 1)                # DF_CC       (mem 0x400E)
    img[7:9] = le16(vars_)                    # VARS        (mem 0x4010)
    img[11:13] = le16(eline)                  # E_LINE      (mem 0x4014)
    img[13:15] = le16(eline + 4)              # CH_ADD      (mem 0x4016)
    img[17:19] = le16(eline + 5)              # STKBOT      (mem 0x401A)
    img[19:21] = le16(eline + 5)              # STKEND      (mem 0x401C)
    img[32:34] = le16(dfile)                  # D_FILE copy (mem 0x4029)
    return bytes(img), dfile


def main():
    mode, out = sys.argv[1], sys.argv[2]
    code = open(sys.argv[3], 'rb').read() if len(sys.argv) > 3 else b''

    if mode == 'empty':
        lines = [(1, b'\xEA')]                # bare "1 REM"
    else:
        lines = [(1, b'\xEA' + code),
                 (2, b'\xF9\xD4\xC5\x0B' + zx_digits(CODE_ORG) + b'\x0B')]

    img, dfile = build_image(lines)
    t81 = (b'EO81' + mode.upper().encode().ljust(32, b'\x00')[:32]
           + ('%d' % len(img)).encode().ljust(16, b'\x00') + img)
    open(out, 'wb').write(t81)
    open(out.rsplit('.', 1)[0] + '.p', 'wb').write(img)
    print('t81+p: %s %d/%d bytes | mode %s | D_FILE 0x%04X | image ends'
          ' 0x%04X' % (out, len(t81), len(img), mode, dfile, Z + len(img)))


if __name__ == '__main__':
    main()
