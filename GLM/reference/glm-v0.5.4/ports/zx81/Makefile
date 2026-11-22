# ZX81 16K port (z88dk). Tape only, Class D model.
ZCC     = zcc
CFLAGS  = +zx81 -vn -O3 -create-app
DEFS    = -DRT_MAX_VOCAB=48 -DRT_MAX_CTX=8 -DRT_MAX_EMB=16 \
          -DRT_MAX_HID=48 -DRT_APP_MODEL_MAX=10240 -DRT_MEM_MAX=6 -DRT_JOURNAL_MAX=8 \
          -DRT_LINE_MAX=64

ENG = ../../engine

all: chat_zx81.p

chat_zx81.p: hal_zx81.c main_zx81.c $(ENG)/rt_app.c
	$(ZCC) $(CFLAGS) $(DEFS) -I$(ENG) -o chat_zx81 hal_zx81.c main_zx81.c \
	    $(ENG)/rt_charset.c $(ENG)/rt_model.c $(ENG)/rt_engine.c \
	    $(ENG)/rt_chat.c $(ENG)/rt_memory.c $(ENG)/rt_tape.c $(ENG)/rt_app.c \
	    $(ENG)/rt_clock.c $(ENG)/rt_router.c $(ENG)/rt_journal.c $(ENG)/rt_scratch.c

clean:
	rm -f chat_zx81*

.PHONY: all clean
