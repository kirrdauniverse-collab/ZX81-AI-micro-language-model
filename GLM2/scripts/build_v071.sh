#!/bin/sh
# build_v071.sh - a v0.7.1 forras ujraepitese az ujonnan epitett z88dk-vel
# A struktura: retro-tinyllm/{engine, ports/zx81, tools}
set -e
cd /home/z/my-project/retro-tinyllm

Z88DK=/home/z/my-project/tools/src/z88dk
export PATH="$Z88DK/bin:$PATH"
export ZCCCFG="$Z88DK/lib/config"

OUT=ports/zx81/build_v071
ROOT=/home/z/my-project/retro-tinyllm
mkdir -p "$OUT"
cd "$OUT"

DEFS="-DRT_MAX_VOCAB=48 -DRT_MAX_CTX=8 -DRT_MAX_EMB=16 -DRT_MAX_HID=48 \
-DRT_APP_MODEL_MAX=10240 -DRT_MEM_MAX=6 -DRT_JOURNAL_MAX=8 -DRT_LINE_MAX=64"

CRT0=$ROOT/src/ports/zx81/build16k/zx81rt_crt0.asm
ENG=$ROOT/src/engine
SRC=$ROOT/src/ports/zx81

echo "== ENGINE (app16) =="
zcc +zx81 -vn -O3 -subtype=none $DEFS -I"$ENG" -crt0="$CRT0" -o rtchat16 \
    "$SRC/app16.c" "$SRC/hal_zx81.c" \
    "$ENG/rt_charset.c" "$ENG/rt_model.c" "$ENG/rt_engine.c" "$ENG/rt_chat.c"

echo "== BOOT (boot16) =="
zcc +zx81 -vn -O3 -subtype=none $DEFS -I"$ENG" -crt0="$CRT0" -o rboot16 \
    "$SRC/boot16.c" "$SRC/hal_zx81.c"

echo "== meretek =="
ls -la rtchat16 rboot16 2>/dev/null | awk '{print $NF, $5}'
