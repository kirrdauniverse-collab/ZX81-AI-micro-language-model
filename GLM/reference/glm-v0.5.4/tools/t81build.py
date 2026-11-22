#!/usr/bin/env python3
# t81build.py - assemble the retro-tinyllm ZX81 cassette (.t81 + .P).
#
# v0.3.2 REWRITE. The previous builder placed the whole image 2 bytes
# lower than the .P/T81 convention (base 0x4007 instead of 0x4009),
# wrote BASIC line numbers little-endian (the ROM wants them big-endian,
# lengths little-endian) and built the D-file without its leading 0x76
# newline - diffed against an official z88dk-appmake +zx81 reference.
# The format core now lives in t81frame.py; this driver adds the model
# blob layout used by the full engine cassette:
#
#   0x4009-0x407C  system variables (appmake block, pointers patched)
#   0x407D         1 REM <engine code> - code runs from 0x4082 = 16514
#   +0x76          2 RAND USR VAL "16514" loader line
#   +0x76          end-of-program marker (the model follows, not the
#                  D-file, so the marker is explicit - see t81frame.py)
#   RT_MODEL_ADDR  model blob (raw RAM: the interpreter never scans it)
#   aligned        D-file: 0x76 + 24 x (32 chars + 0x76) = 793 bytes
#   D_FILE+793     VARS 0x80 marker - the image/file ends here
#
# The engine needs a ZX81 with 48K RAM or more: the image reaches
# ~0xEB40 (display file at ~0xE820), far above a 16K machine's top.
#
# Usage: t81build.py <rtchat.bin> <base_d.bin> <out.t81>
#                   [--name RTCHAT] [--model-addr 51520]
#        (also writes the .P twin next to the .t81)
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from t81frame import build_image, zx_digits, PROG, CODE_ORG

MODEL_ADDR = 51520                 # 0xC940, right above the loader line
NAME = 'RTCHAT'


def main():
    a = sys.argv[1:]
    if len(a) < 3:
        sys.exit(__doc__)
    binf, modf, out = a[0], a[1], a[2]
    name = a[a.index('--name') + 1] if '--name' in a else NAME
    model_addr = int(a[a.index('--model-addr') + 1]) if \
        '--model-addr' in a else MODEL_ADDR

    code = open(binf, 'rb').read()
    model = open(modf, 'rb').read()

    lines = [(1, b'\xEA' + code),
             (2, b'\xF9\xD4\xC5\x0B' + zx_digits(CODE_ORG) + b'\x0B')]
    prog_end = PROG + 4 + 1 + len(code) + 1 + 4 + 11 + 1  # + end marker
    if prog_end > model_addr:
        sys.exit('engine too big: program ends 0x%X, model at 0x%X'
                 % (prog_end, model_addr))

    img, dfile = build_image(lines, [(model_addr, model)])
    t81 = (b'EO81' + name.encode().ljust(32, b'\x00')[:32]
           + ('%d' % len(img)).encode().ljust(16, b'\x00') + img)
    open(out, 'wb').write(t81)
    open(out.rsplit('.', 1)[0] + '.p', 'wb').write(img)
    print('t81+p: %s %d/%d bytes | code %d @0x%X | model %d @0x%X'
          ' | D_FILE 0x%04X | image ends 0x%04X'
          % (out, len(t81), len(img), len(code), CODE_ORG, len(model),
             model_addr, dfile, 0x4009 + len(img)))


if __name__ == '__main__':
    main()
