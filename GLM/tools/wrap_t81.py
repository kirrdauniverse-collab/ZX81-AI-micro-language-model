#!/usr/bin/env python3
"""wrap_t81.py - build an EO81 .T81 tape from .P files (one block each).
Usage: wrap_t81.py out.t81 NAME1=file1.P NAME2=file2.P ...
Block name is padded to 32 bytes, length is 16 ASCII digits + NULs, data = the .P image (base 0x4009)."""
import sys
if len(sys.argv) < 3: sys.exit(__doc__)
out = bytearray(b'EO81')
for a in sys.argv[2:]:
    name, path = a.split('=', 1)
    d = open(path, 'rb').read()
    out += name.encode().ljust(32, b'\0')[:32] + str(len(d)).encode().ljust(16, b'\0') + d
open(sys.argv[1], 'wb').write(out)
print('wrote', sys.argv[1], len(out), 'bytes')
