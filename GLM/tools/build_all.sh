#!/usr/bin/env bash
# Builds everything: model -> base pack + skill packs -> BASIC load lines -> tapes. Run from GLM/. Needs z88dk (see setup_z88dk.sh).
set -e
export ZCCCFG=${ZCCCFG:-/home/claude/z88src/z88dk/lib/config} PATH=${Z88DK_BIN:-/home/claude/z88src/z88dk/bin}:$PATH
FL="-O3 -Cc-unsigned -pragma-define:CRT_ENABLE_STDIO=0 -clib=default"
REQ=32578                                   # PZ_ADDR ($7F40) + 2: the 'req' byte of the persistent zone
ROUNDS=${ROUNDS:-5}
mkdir -p out out/real
python3 tools/build_kb.py knowledge/kb.txt engine/kb.h docs/knowledge-report.md
z88dk-zx81bas boot/loader_v05.bas && mv boot/loader_v05.p out/loader.P && rm -f boot/*.map boot/*.sym
cd engine
zcc +zx81 -vn $FL zx81_main.c pico.c -o /tmp/pico -create-app -m
for p in draw util story poem code food; do zcc +zx81 -vn $FL ${p}_main.c -o /tmp/$p -create-app; done
cd ..
python3 tools/add_basic.py /tmp/pico.P out/PICO.P $REQ 1=DRAW 2=UTIL 3=STORY 4=POEM 5=CODE 7=FOOD
for p in draw util story poem code food; do python3 tools/add_basic.py /tmp/$p.P out/$(echo $p | tr a-z A-Z).P $REQ 6=PICO > /dev/null; done
# emulator tape: it can only search forward, so PICO and the packs repeat ROUNDS times
ARGS="LOADER=out/loader.P"
for i in $(seq $ROUNDS); do ARGS="$ARGS PICO=out/PICO.P DRAW=out/DRAW.P UTIL=out/UTIL.P STORY=out/STORY.P POEM=out/POEM.P CODE=out/CODE.P FOOD=out/FOOD.P"; done
python3 tools/wrap_t81.py out/PICO_AI_emulator.t81 $ARGS PICO=out/PICO.P
# real machine: every block once, in this order (rewind by hand to the PICO block after a pack)
python3 tools/wrap_t81.py out/real/PICO_AI_real.t81 LOADER=out/loader.P PICO=out/PICO.P DRAW=out/DRAW.P UTIL=out/UTIL.P STORY=out/STORY.P POEM=out/POEM.P CODE=out/CODE.P FOOD=out/FOOD.P
cp out/loader.P out/PICO.P out/DRAW.P out/UTIL.P out/STORY.P out/POEM.P out/CODE.P out/FOOD.P out/real/
for f in PICO DRAW UTIL STORY POEM CODE FOOD; do python3 -c "
import os; n=os.path.getsize('out/$f.P'); print('%-6s %6d bytes, free to \$7F40: %5d' % ('$f', n, 0x7F40-0x4009-n))"; done
python3 reference/glm-v0.5.4/tools/t81verify.py out/PICO_AI_emulator.t81 2>&1 | head -3 || true
