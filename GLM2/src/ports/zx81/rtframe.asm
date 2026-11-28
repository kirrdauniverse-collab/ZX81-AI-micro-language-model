; rtframe.asm - minimal ZX81 framework test, org 16514 (0x4082)
;
; What it proves when you RUN it: the T81 loader + system variables +
; the two BASIC lines are fine, D_FILE points at a real display file,
; and machine code runs and returns to BASIC cleanly.
;
; After RUN you should see "RT81 OK" in row 12 of the screen and the
; usual 0/0 report under it. Assemble:
;   z88dk-z80asm -b -l -s rtframe

        org     16514

start:
        ld      hl,(16396)      ; D_FILE system variable (0x400C) - the
                                ; REAL ZX81 address; 0x400A is E_PPC, and
                                ; reading it sent the pokes into ROM
        ld      de,12*33+12+1   ; row 12, column 12 - the +1 skips the
                                ; leading 0x76 D_FILE points at: the
                                ; first cell of row 0 is D_FILE+1
        add     hl,de
        ld      de,msg
        ld      b,7
copy:
        ld      a,(de)
        ld      (hl),a          ; write one ZX81 glyph code
        inc     hl
        inc     de
        djnz    copy
        ret                     ; back to BASIC -> 0/0

; ZX81 glyph codes: letters 0x26+A..Z, digits 0x1C+0..9, space 0x00
msg:    defb    037h,039h,024h,01Dh,000h,034h,030h ; "RT81 OK"
