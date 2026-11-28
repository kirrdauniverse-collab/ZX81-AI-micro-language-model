;       CRT0 for the ZX81
;
;       Stefano Bodrato Apr. 2000
;
;       If an error occurs (eg. out if screen) we just drop back to BASIC
;
;       ZX81 will be thrown in FAST mode by default.
;       The "startup=2" parameter forces the SLOW mode.
;	Values for "startup" from 3 to 6 activate the HRG modes
;
; - - - - - - -
;
;       $Id: zx81_crt0.asm,v 1.22 2007/10/25 14:53:04 stefano Exp $
;
; - - - - - - -


        MODULE  zx81_crt0

;-------
; Include zcc_opt.def to find out information about us
;-------

        INCLUDE "zcc_opt.def"

;-------
; v0.6.7 fix: the v2.2 zx81 classic library (zx81toasc.asm) EXTERNs
; these transform constants; the v2.2 stock crt0 defines them. Our
; crt0 base predates that - added verbatim (mode 0 = the default:
; lowercase input becomes uppercase on the ZX81, as the original
; build behaved).
;-------

        PUBLIC  __CLIB_ZX_LOWER_CASE_TRANSFORM
        PUBLIC  __CLIB_ZX_UPPER_CASE_TRANSFORM
        PUBLIC  __CLIB_ZX_CAPITAL_TRANSFORM
        PUBLIC  __CLIB_ZX_INVERSE_TRANSFORM

        IF !DEFINED_ZX_CHAR_XLATE_MODE
            defc ZX_CHAR_XLATE_MODE = 0
        ENDIF

        IF ZX_CHAR_XLATE_MODE = 0
            defc __CLIB_ZX_LOWER_CASE_TRANSFORM = $26 - 'a'
            defc __CLIB_ZX_CAPITAL_TRANSFORM = $26 - 'a'
            defc __CLIB_ZX_UPPER_CASE_TRANSFORM = $A6 - 'A'
            defc __CLIB_ZX_INVERSE_TRANSFORM = $A6 - 'A'
        ELIF ZX_CHAR_XLATE_MODE = 1
            defc __CLIB_ZX_LOWER_CASE_TRANSFORM = $A6 - 'a'
            defc __CLIB_ZX_INVERSE_TRANSFORM = $A6 - 'a'
            defc __CLIB_ZX_UPPER_CASE_TRANSFORM = $26 - 'A'
            defc __CLIB_ZX_CAPITAL_TRANSFORM = $26 - 'A'
        ELIF ZX_CHAR_XLATE_MODE = 2
            defc __CLIB_ZX_LOWER_CASE_TRANSFORM = $26 - 'a'
            defc __CLIB_ZX_INVERSE_TRANSFORM = $A6 - 'A'
            defc __CLIB_ZX_UPPER_CASE_TRANSFORM = $26 - 'A'
            defc __CLIB_ZX_CAPITAL_TRANSFORM = $26 - 'A'
        ENDIF

;-------
; Some general scope declarations
;-------

        XREF    _main           ;main() is always external to crt0 code

        XDEF    cleanup         ;jp'd to by exit()
        XDEF    l_dcal          ;jp(hl)

        XDEF    _vfprintf       ;jp to the printf() core

        XDEF    exitsp          ;atexit() variables
        XDEF    exitcount

        XDEF    __sgoioblk      ;stdio info block

        XDEF    heaplast        ;Near malloc heap variables
        XDEF    heapblocks

        ; v0.6.7 fix: these are only DEFINED for startup>=3 (the HRG
        ; modes) further down - export them conditionally to match,
        ; or z80asm fails with "undefined symbol" on startup=1
IF (startup>=3)
        XDEF    hr_rows		;Current number of text rows in graphics mode
        XDEF    _hr_rows	 ;as above for C declarations
        XDEF    text_rows	 ;as above for VT ANSI mode
ENDIF
        XDEF    base_graphics   ;Graphical variables
        XDEF    _base_graphics   ;as above for C declarations
        XDEF    coords          ;Current xy position

        XDEF    save81          ;Save ZX81 critical registers
        XDEF    restore81       ;Restore ZX81 critical registers

        XDEF    frames          ;Frame counter for time()

        org     16514


; Hide the mess in the REM line from BASIC program listing

;       jr      start
;       defb    118,255         ; block further listing

; As above, but more elegant

        ld      a,(hl)          ; hide the first 6 bytes of REM line
        jp      start           ; invisible
._base_graphics                 ; Address of the Graphics map..
.base_graphics			; it is POKEable at address 16518/16519
IF DEFINED_hrgpage
		defw	hrgpage
ELSE
		defw    0
ENDIF
        defb    118,255         ; block further listing
        ; v0.4.3: the 8-byte "Z88DK" signature block ('Z'-27 .. 149)
        ; was decorative only - dropped to reclaim space for the
        ; FLAGS save/restore.

.start
        call    save81

IF (!DEFINED_startup | (startup=1))
        ; v0.6.8: THE DISPLAY INTERRUPTS OFF AT ENTRY. The old block did
        ; SET 6,(IY+$3B) + CALL $0207 (SLOW) - but the dance's REAL
        ; switch ENTERS THE ROM's OWN DISPLAY WALK, which "executes"
        ; the D-file rows as Z80 code in the $FBxx mirror. Our packed
        ; state rows (rows 0-2, values 0x00-0x3F) are valid glyphs but
        ; HOSTILE opcodes (0x18/0x28/0x38 = JR, 0x12/0x22/0x32 = writes):
        ; depending on the state bytes the walk loops FOREVER in the
        ; mirror - the machine "freezes" with a live display (the user's
        ; RT-TUDOM topic load froze exactly so; the BASE only survived
        ; because its particular state bytes happened to be harmless).
        ; Instead: flag FAST + gate the NMI generator off (OUT $FD);
        ; the screen stays dark for the ~1 ms until main() installs the
        ; register-preserving chain (the builder's blob at $7A40) and
        ; re-enables SLOW with the port sequence - from then on every
        ; NMI lands in OUR chain, which walks the D-file by pointer,
        ; never executing it. Assembled with -Ca-IXIY: ix = machine IY.
        ld      ix,$4000        ; LD IY,$4000 = the ROM sysvar base
        res     6,(ix+$3B)      ; RES 6,(IY+$3B) = CDFLAG bit 6: FAST
        ld      a,(ix+$3B)      ; A = CDFLAG (bit 6 now 0)
        out     ($FD),a         ; NMI generator off (both ROM versions)
ENDIF

IF (startup>=2)
        call    hrg_on
 IF ((startup=3)|(startup=5))
        ld	a,1
        ld      (hrgbrkflag),a
        call    hrg_on
 ENDIF
ENDIF

	; this must be after 'hrg_on', sometimes
	; the stack will be moved to make room
	; for high-resolution graphics.

        ld      (start1+1),sp   ;Save entry stack
        ; v0.4.3: the original "ld hl,-64 / add hl,sp / ld sp,hl"
        ; atexit-stack resize became a no-op when hl was set to 0 -
        ; dropped (5 bytes) to keep the engine inside the 16K budget.
        ld      (exitsp),sp

IF !DEFINED_nostreams
IF DEFINED_ANSIstdio
; Set up the std* stuff so we can be called again
        ld      hl,__sgoioblk+2
        ld      (hl),19 ;stdin
        ld      hl,__sgoioblk+6
        ld      (hl),21 ;stdout
        ld      hl,__sgoioblk+10
        ld      (hl),21 ;stderr
ENDIF
ENDIF

        call    _main   ;Call user program
        
.cleanup
;
;       Deallocate memory which has been allocated here!
;
        push    hl		; keep return code

IF !DEFINED_nostreams
IF DEFINED_ANSIstdio
        LIB     closeall
        call    closeall
ENDIF
ENDIF

        call    restore81

IF (startup>=2)
 IF ((startup=3)|(startup=5))
        xor	a
        ld      (hrgbrkflag),a
 ELSE
        call    hrg_off		; this is valid for mode 2, too !
 ENDIF
ELSE
 IF (!DEFINED_startup | (startup=1))
        ; v0.4.3: the exit SLOW call is redundant - the entry already
        ; switched to SLOW and nothing switches away from it.
 ENDIF
ENDIF

        ; v0.4.3: restore the FLAGS sysvar ($4001). The engine's ROM
        ; calls (print/CLS) clobber it, and bit 6 ("numeric result")
        ; is what CLASS-6 checks after SCANNING - without it the
        ; IF in line 2 dies with report B ("integer out of range",
        ; error code $0B at 0D9A) as soon as the engine returns.
        ld      a,(flag81save)
        ld      ($4001),a

        pop     bc		; return code (for BASIC)
        ; v0.6.9: THE EXIT DISPLAY GATE IS GONE. The v0.6.8 gate (RES
        ; CDFLAG bit 6 + OUT $FD before a nonzero return, so the ROM's
        ; THEN LOAD would run "undisturbed") KILLED THE MACHINE instead:
        ; the ZX81 ROM's tape load is NMI-TIMED - the NMI handler's
        ; EX AF,AF/INC A builds the bit timing and the $02EC leader
        ; search sits in a HALT that only an NMI can wake. With the
        ; generator gated off the ROM HALT never wakes = the machine
        ; freezes at the very "start the tape and press ENTER" gate
        ; (the user's EightyOne froze exactly there: RTBASE16 queued,
        ; the ENTER message on screen, dead CPU). The real culprits
        ; behind the editor derailment the gate tried to patch were the
        ; missing IY=$4000 handoff (fixed in v0.6.8 restore81) and the
        ; /topic poke bugs (also v0.6.8). The display stays live across
        ; the return: every NMI still lands in our register-preserving
        ; chain, and the ROM editor + tape load both get their timing.
.start1 ld      sp,0            ;Restore stack to entry value
        ret

.l_dcal jp      (hl)            ;Used for function pointer calls
        jp      (hl)


.restore81
IF (!DEFINED_startup | (startup=1))
        ex      af,af
        ld      a,(a1save)
        ex      af,af
ENDIF
        exx
        ld	hl,(hl1save)
        ;ld	bc,(bc1save)
        ;ld	de,(de1save)
        exx
        ; v0.6.8: the "ld ix,16384" (-> machine LD IY,$4000) is BACK.
        ; It was removed in v0.6.7 because the stock library getk()
        ; called restore81 on every poll and clobbered machine IY
        ; (sccz80's library-call return register) mid-C-code - that
        ; crashed the boot's date question. Since then our HAL uses
        ; its OWN port-based keyboard scan (no getk, no restore81
        ; call anywhere in the engine) - the ONLY caller left is this
        ; crt0 cleanup, and THERE the IY=$4000 handoff is REQUIRED:
        ; the ROM editor after the USR return reads every sysvar via
        ; (IY+n); without it the editor corrupts the display/VARS
        ; (D_FILE wandered to $7B64, VARS below D_FILE, PPC=$FFFE -
        ; all reproduced in the sim) and the THEN LOAD of a /topic
        ; switch never runs.
        ld      ix,16384	; machine LD IY,$4000 - the ROM sysvar base
        ret
        
.save81
        ; v0.4.3: snapshot the FLAGS sysvar ($4001) at entry - the
        ; USR dispatcher has already set bit 6 ("numeric result") by
        ; the time we run, and we restore it verbatim on exit so the
        ; interpreter's CLASS-6 check sees the same state as before
        ; the machine code ran.
        ld      a,($4001)
        ld      (flag81save),a
IF (!DEFINED_startup | (startup=1))
        ex      af,af
        ld      (a1save),a
        ex      af,af
ENDIF
        exx
        ld	(hl1save),hl
        ;ld	(bc1save),bc
        ;ld	(de1save),de
        exx
        ret

IF (!DEFINED_startup | (startup=1))
.a1save         defb    0
ENDIF
.hl1save	defw	0
;.bc1save	defw	0
.de1save	defw	0
.flag81save     defb    0       ; FLAGS ($4001) snapshot (v0.4.3)

;-----------
; Define the stdin/out/err area. For the z88 we have two models - the
; classic (kludgey) one and "ANSI" model
;-----------
.__sgoioblk
IF DEFINED_ANSIstdio
        INCLUDE "stdio_fp.asm"
ELSE
        defw    -11,-12,-10
ENDIF


;---------------------------------
; Select which printf core we want
;---------------------------------
._vfprintf
IF DEFINED_floatstdio
        LIB     vfprintf_fp
        jp      vfprintf_fp
ELSE
        IF DEFINED_complexstdio
                LIB     vfprintf_comp
                jp      vfprintf_comp
        ELSE
                IF DEFINED_ministdio
                        LIB     vfprintf_mini
                        jp      vfprintf_mini
                ENDIF
        ENDIF
ENDIF


;---------------------------------------
; Modified IRQ handler
;---------------------------------------

IF (startup=2)
        INCLUDE "zx81_altint.def"
ENDIF

;---------------------------------------
; High Resolution Graphics (Wilf Rigter WRX mode)
; Code my Matthias Swatosch
;---------------------------------------

IF (startup>=3)
        INCLUDE "zx81_hrg.def"
ENDIF

;-----------
; Now some variables
;-----------
IF (startup>=3)
.text_rows
.hr_rows
._hr_rows
 IF (startup>=5)
		defw	8	; Current number of text rows in graphics mode
 ELSE
		defw	24	; Current number of text rows in graphics mode
 ENDIF
ENDIF

.coords         defw    0       ; Current graphics xy coordinates

IF !DEFINED_HAVESEED
		XDEF    _std_seed        ;Integer rand() seed
._std_seed       defw    0      ; Seed for integer rand() routines
ENDIF

.exitsp         defw    0       ; Address of where the atexit() stack is
.exitcount      defb    0       ; How many routines on the atexit() stack


.heaplast       defw    0       ; Address of last block on heap
.heapblocks     defw    0       ; Number of blocks
.frames         defw    0       ; counter handled with new interrupt
		defb	0	; third byte for "frames"

                defm  "Small C+ ZX81"   ;Unnecessary file signature
                defb    0

;-----------------------
; Floating point support
;-----------------------
IF NEED_floatpack
        INCLUDE "float.asm"
.fp_seed        defb    $80,$80,0,0,0,0 ;FP seed (unused ATM)
.extra          defs    6               ;FP register
.fa             defs    6               ;FP Accumulator
.fasign         defb    0               ;FP register
ENDIF

