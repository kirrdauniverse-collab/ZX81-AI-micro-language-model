# retro-tinyllm - top-level Makefile
#
# targets:
#   host        build host/host_chat (the desktop simulator)
#   test        build and run all C tests (needs a packed model + golden)
#   c64         cross-build the C64 port (needs cc65)
#   clean       remove build outputs
#
# The Python side has no make targets; use:
#   python3 training/corpus_gen.py
#   python3 training/train.py --class C --data training/data/base.txt \
#       --out runs/base_c --steps 20000
#   python3 training/pack.py --npz runs/base_c.npz --bin models/base_c.bin \
#       --golden tests/golden_c.txt --finetune-ok

CC      ?= gcc
CFLAGS  ?= -O2
WARN     = -Wall -Wextra
C89      = -std=c89 -pedantic
# host-only POSIX bits (nanosleep, opendir)
HOSTFLAGS = $(C89) -D_POSIX_C_SOURCE=200809L -DRT_ENABLE_FINETUNE=1
ENG      = engine/rt_charset.c engine/rt_model.c engine/rt_engine.c \
           engine/rt_chat.c engine/rt_memory.c engine/rt_tape.c \
           engine/rt_app.c engine/rt_finetune.c engine/rt_clock.c \
           engine/rt_router.c engine/rt_journal.c \
           engine/rt_scratch.c

.PHONY: all host test c64 clean

all: host

host/host_chat: host/main_host.c host/hal_host.c $(ENG) engine/*.h
	mkdir -p host
	$(CC) $(CFLAGS) $(WARN) $(HOSTFLAGS) -Iengine -o $@ \
	    host/main_host.c host/hal_host.c $(ENG) -lm

host: host/host_chat

test: tests/run_tests.sh host/host_chat
	./tests/run_tests.sh

c64:
	$(MAKE) -C ports/c64

clean:
	rm -f host/host_chat tests/test_charset tests/test_mem tests/test_tape \
	      tests/test_chat tests/test_golden tests/test_finetune \
	      tests/test_clock tests/test_router tests/test_journal tests/test_scratch
	$(MAKE) -C ports/c64 clean 2>/dev/null || true
