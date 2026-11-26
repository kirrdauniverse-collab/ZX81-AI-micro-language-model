#!/usr/bin/env python3
# fixxref.py - the ONE quirk of z88dk 1.8's sccz80 that z80asm 1.0.30
# rejects: it emits XDEF for every PROTOTYPED function name even in
# modules that only call it, and a bodyless XDEF fails at link time
# ("Symbol not defined"). Everything else in the sccz80 output (dot
# labels, plain locals, LIB declarations) assembles as-is. This pass
# converts bodyless XDEFs into XREFs and adds nothing else.
import re
import sys

path = sys.argv[1]
with open(path) as f:
    lines = f.read().split('\n')

labels = set()
for l in lines:
    m = re.match(r'^\.?([A-Za-z_.$@][\w.$@]*)', l)  # label at column 0
    if m:
        labels.add(m.group(1))

out = []
fixed = 0
for l in lines:
    m = re.match(r'\s*XDEF\s+(\S+)', l)
    if m and m.group(1) not in labels:
        out.append('\tXREF\t' + m.group(1))
        fixed += 1
        continue
    out.append(l)
with open(path, 'w') as f:
    f.write('\n'.join(out))
print('%s: %d bodyless XDEF -> XREF' % (path.split('/')[-1], fixed))
