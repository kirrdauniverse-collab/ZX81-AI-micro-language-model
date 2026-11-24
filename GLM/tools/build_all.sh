#!/usr/bin/env bash
# Builds everything: model -> base pack + DRAW pack -> BASIC load lines -> emulator tape (LOADER, then PICO/DRAW pairs).
# Needs z88dk (see setup_z88dk.sh). Run from GLM/.
set -e
export ZCCCFG=${ZCCCFG:-/home/claude/z88src/z88dk/lib/config} PATH=${Z88DK_BIN:-/home/claude/z88src/z88dk/bin}:$PATH
REQ=32642                                   # PZ_ADDR + 2 (the 'req' byte of the persistent zone)
python3 tools/build_model.py knowledge/base.txt engine/model.h docs/knowledge-report.md
z88dk-zx81bas boot/loader_v04.bas && mv boot/loader_v04.p out/loader.P && rm -f boot/*.map boot/*.sym
(cd engine && zcc +zx81 -vn -O3 -clib=default zx81_main.c pico.c -o /tmp/pico -create-app -m && zcc +zx81 -vn -O3 -clib=default draw_main.c -o /tmp/draw -create-app -m)
python3 tools/add_basic.py /tmp/pico.P out/pico_pack.P $REQ 1=DRAW
python3 tools/add_basic.py /tmp/draw.P out/draw_pack.P $REQ 6=PICO
# tape: the emulator can only search forward, so the chain repeats PICO/DRAW (on a real machine: rewind by hand)
ARGS="LOADER=out/loader.P"; for i in 1 2 3 4 5 6; do ARGS="$ARGS PICO=out/pico_pack.P DRAW=out/draw_pack.P"; done
python3 tools/wrap_t81.py out/PICO_AI_v0.4.t81 $ARGS PICO=out/pico_pack.P
python3 reference/glm-v0.5.4/tools/t81verify.py out/PICO_AI_v0.4.t81 | head -4
