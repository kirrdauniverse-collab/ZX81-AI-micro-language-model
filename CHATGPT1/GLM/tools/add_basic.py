#!/usr/bin/env python3
"""add_basic.py in.P out.P 'REQ_ADDR' 'CODE=NAME' ...  -- adds BASIC lines after the program's last line so the pack can ask for the next tape block:
   3 IF PEEK <REQ_ADDR>=<CODE> THEN LOAD "<NAME>"
The lines are tokenised with z88dk-zx81bas and spliced into the .P image; D_FILE/VARS/E_LINE/... are moved. The machine code and its data
(inside the REM line) keep their addresses, only the display file moves (the engine reads D_FILE at run time)."""
import sys, subprocess, tempfile, os
src, dst, addr = sys.argv[1], sys.argv[2], sys.argv[3]
reqs = [a.split('=') for a in sys.argv[4:]]
d = bytearray(open(src, 'rb').read())
g = lambda a: int.from_bytes(d[a - 0x4009:a - 0x4009 + 2], 'little')
dfile = g(0x400C)
p = 0x407D - 0x4009; last = 0
while p < dfile - 0x4009:                       # walk the lines: find where the last one ends
    L = int.from_bytes(d[p + 2:p + 4], 'little'); ln = int.from_bytes(d[p:p + 2], 'big'); p += 4 + L; last = ln
t = tempfile.mkdtemp(); bas = os.path.join(t, 'x.bas')
open(bas, 'w').write('10 LET N=PEEK %s\n' % addr + ''.join('%d IF N=%s THEN LOAD "%s"\n' % (10 * (i + 2), c, n) for i, (c, n) in enumerate(reqs)))
subprocess.check_call(['z88dk-zx81bas', bas], stdout=subprocess.DEVNULL)
s = open(os.path.join(t, 'x.p'), 'rb').read()
sg = lambda a: int.from_bytes(s[a - 0x4009:a - 0x4009 + 2], 'little')
new = s[0x407D - 0x4009: sg(0x400C) - 0x4009]    # the tokenised lines
N = len(new)
d[p:p] = new                                       # p == end of the last line == old D_FILE offset
for a in (0x400C, 0x400E, 0x4010, 0x4014, 0x4016, 0x401A, 0x401C):
    v = g(a)
    if v >= dfile: d[a - 0x4009:a - 0x4009 + 2] = (v + N).to_bytes(2, 'little')
open(dst, 'wb').write(d)
print('added %d bytes of BASIC (%d lines)' % (N, len(reqs)))
