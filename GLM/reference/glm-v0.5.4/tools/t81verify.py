#!/usr/bin/env python3
# t81_verify.py - definitive EO81/T81 parser: EO81 + repeat
# [name: 32B NUL-padded][length: 16B ASCII digits + NUL padding]
# [data: length bytes]. Verifies exact EOF on demo.T81.
import sys

path = sys.argv[1] if len(sys.argv) > 1 else '/home/z/my-project/upload/demo.T81'
raw = open(path, 'rb').read()

assert raw[:4] == b'EO81', 'missing EO81 magic'
pos = 4
blocks = []
while pos < len(raw):
    name = raw[pos:pos + 32].rstrip(b'\x00')
    fld = raw[pos + 32:pos + 48]
    digits = fld.split(b'\x00')[0]
    if not digits or not digits.isdigit():
        print('BAD length field @%d: %r' % (pos + 32, fld[:8]))
        sys.exit(1)
    n = int(digits)
    d0 = pos + 48
    if d0 + n > len(raw):
        print('data overrun @%d (need %d, have %d)' % (d0, n, len(raw) - d0))
        sys.exit(1)
    blocks.append((pos, name, n, d0))
    pos = d0 + n

print('OK: %d blocks, exact EOF (%d == %d)' % (len(blocks), pos, len(raw)))
for (o, name, n, d0) in blocks:
    img = raw[d0:d0 + n]
    dfile = int.from_bytes(img[3:5], 'little')
    ends80 = img[-1] == 0x80
    print('@%-7d %-9r len=%-6d head=%s D_FILE=%04x ends0x80=%s' %
          (o, name[:8], n, img[:8].hex(' '), dfile, ends80))
