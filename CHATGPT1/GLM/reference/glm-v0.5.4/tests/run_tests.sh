#!/bin/sh
# run_tests.sh - build and run every C test; needs models/ + tests/golden_*.txt
# produced by training/corpus_gen.py + train.py + pack.py (or use the
# checked-in preview models).
set -e
cd "$(dirname "$0")/.."

CC=${CC:-gcc}
FLAGS="-std=c89 -O1 -Wall -Wextra -pedantic -Iengine"
ENG="engine/rt_charset.c engine/rt_model.c engine/rt_engine.c \
     engine/rt_chat.c engine/rt_memory.c engine/rt_tape.c engine/rt_finetune.c \
     engine/rt_clock.c engine/rt_router.c engine/rt_journal.c"

pass=0
fail=0

run() {
    name=$1
    shift
    if "$@"; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
    fi
}

echo "== building tests =="
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_charset tests/test_charset.c engine/rt_charset.c
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_mem tests/test_mem.c engine/rt_charset.c engine/rt_memory.c
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_tape tests/test_tape.c engine/rt_tape.c
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_clock tests/test_clock.c engine/rt_clock.c
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_router tests/test_router.c engine/rt_charset.c engine/rt_router.c
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_journal tests/test_journal.c engine/rt_charset.c engine/rt_clock.c engine/rt_journal.c
$CC $FLAGS -o tests/test_scratch tests/test_scratch.c engine/rt_scratch.c
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_chat tests/test_chat.c $ENG
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_golden tests/test_golden.c $ENG
$CC $FLAGS -DRT_ENABLE_FINETUNE=1 -o tests/test_finetune tests/test_finetune.c $ENG

echo "== running tests =="
run charset  ./tests/test_charset
run memory   ./tests/test_mem
run tape     ./tests/test_tape
run clock    ./tests/test_clock
run router   ./tests/test_router
run journal  ./tests/test_journal
run scratch  ./tests/test_scratch
if [ -f models/base_d.bin ]; then
    run chat      ./tests/test_chat models/base_d.bin
    run finetune  ./tests/test_finetune models/base_d.bin
    if [ -f tests/golden_d.txt ]; then
        run golden ./tests/test_golden models/base_d.bin tests/golden_d.txt
    else
        echo "golden: SKIP (no tests/golden_d.txt)"
    fi
else
    echo "chat/finetune/golden: SKIP (no models/base_d.bin)"
fi

run scratch-e2e sh tests/scratch_e2e.sh
echo "== $pass passed, $fail failed =="
[ "$fail" -eq 0 ]
