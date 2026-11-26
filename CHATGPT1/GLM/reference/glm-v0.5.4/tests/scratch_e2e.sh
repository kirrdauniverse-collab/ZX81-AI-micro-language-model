#!/bin/sh
# scratch_e2e.sh - the screen-RAM tier end to end on the host: save,
# "power cycle" (a FRESH working directory with no disk files), restore
# from the emulated ZX81 D-file cells (256 glyph cells, 6 bits each).
set -e
cd "$(dirname "$0")/.."
ROOT=$PWD
TD=$(mktemp -d)
trap 'rm -rf "$TD"' EXIT
mkdir -p "$TD"/a "$TD"/b

run_session() {
    (cd "$2" && RT_SCRATCH=zx81 RT_SCRATCH_FILE="$TD"/scr.bin \
        RT_MODELS="$2"/empty \
        "$ROOT/host/host_chat" "$ROOT/models/base_c.bin" \
        > "$2/$1.txt" 2>/dev/null)
}

printf '2026 11 18\n14 30\n/teach my login = fred\ni feel great today\n/save\n/quit\n' \
    | run_session a "$TD"/a
[ -f "$TD"/scr.bin ]
[ "$(wc -c < "$TD"/scr.bin)" -eq 256 ]

printf '2026 11 18\n15 0\nwhat is my login?\n/journal\n/quit\n' \
    | run_session b "$TD"/b
grep -q "kept alive in screen ram" "$TD"/b/b.txt
grep -q "your login is fred" "$TD"/b/b.txt
grep -q "i feel great" "$TD"/b/b.txt
echo "scratch e2e: save -> power cycle -> restore OK"
