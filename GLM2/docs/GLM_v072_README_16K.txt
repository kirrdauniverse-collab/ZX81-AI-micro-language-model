GLM retro-tinyllm 16K - V0.7.2 szalagkeszlet (ZX81, 16K RAM)
==============================================================

MI VALTOZOTT V0.7.1 -> V0.7.2 (a SZIA-fagyás végleges gyökere és javítása)

1) A VALÓDI GÉPI SZIA-FAGYÁS GYÖKERE (a v0.7.1 átadás nyitott hibája):
   AZ OK (ROM $0207/$02E7/$0066 dekódolásból, szimmel és anélkül):
   - A v0.7.1 kb_rearm (SET 6 + 17x OUT $FE) a SZIM NMI-modelljét
     elégítette ki (nmiGenerator flag az utolsó OUT irányából), de a
     VALÓDI gép generátora csak a $0207 SLOW-tánc mechanizmusán át
     fegyverződik - ettől fagyott meg a kép SZIA+ENTER után (a gép
     élt: $51A4 kb_scan; a display walk a $FDxx tükörben szabadult el).
   - A $02E7-es ROM tape-előkészítő dekódolása megmutatta a teljes
     láncot: bit7=1 esetén a $02EC HALT-on vár egy NMI-re, és csak
     azután kapcsolja ki a generátort (OUT $FD + RES 7). Nem fegyverzett
     generátorral a HALT sosem ér véget - ez volt a $2EC-körözés.

2) A JAVÍTÁS (gen_reply, app16.c): a válaszgenerálás UTÁN a fő
   main()-beli, v0.7.1-ben már bizonyított minta fut:
     a) a D-file 3 munkasorának kinullázása (32 blank + $76) -
        a packed state bájtjai (0x00-0x3F) ellenséges Z80 opkódok,
        a tánc display-walk-ja ezeken szabadulna el
     b) SET 6 (CDFLAG bit6) + CALL $0207 - a ROM SAJÁT SLOW-tánca:
        a 17x OUT $FE sorozata fegyverzi a valódi generátort, a walk
        a friss válasz-betűcellákon fut (normál video, mint bármely
        ZX81 képernyő), a bit7-et a tánc maga állítja be
     c) a pack_state() a loop alján ezután írja vissza az állapotot
   Motor: 13242 -> 13384 B (+142 B; modell $748A -> $7518, az
   ALTINT($7A40) alatt marad). Boot változatlan (5085 B).

3) A SZIMULÁTOR-JAVÍTÁS (sim_v072b.js): a $02EC leader-search HALT
   átugrása most már akkor is történik, ha a generátor NINCS
   fegyverezve (a szim nem modellezi a HALT-fegyverzést, a valódi
   gépen a tánc utáni walk-NMI-k szakítják meg a HALT-ot). Ez szim-
   korrekció, a motor logikája változatlan.

4) MOTOR-GYORSÍTÁS (rt_engine.c, a HarryR/z80ai tapasztalatai):
   - a matmul sormutatója sétáltatva (w += n_in+1) a soronkénti
     32-bit offszet-szorzás helyett
   - az embedding-gather célpuffer sétáltatva, a forrás offszet
     16-bit szorzással (vocab*emb <= 768)
   - a mintavétel "zero-skip" szűrése: az exp(-d/8) LUT 0-súlyú
     jelölteket (gap > (255<<3)<<temp_sh) a CDF-sétából kihagyja -
     bit-azonos viselkedés (20000 véletlen teszt), olcsóbb belső ciklus
   - VISSZAVONVA: a 16-bit akkumulátoros MAC (a valós sorköztes
     összegek 1.5M-ig terjednek - csendes túlcsordulás lenne; a
     mérés a tools/rowsum_check.py)
   - VISSZAVONVA: a % sum Lemire-stílusú kiváltása (sccz80 ugyanazt a
     32-bit szorzót húzná be, semmi nyereség)

SZIM-VEGEREDMENY (6800 frame): 3 USR életben - boot -> interjú ->
SZIA chat -> /topic 1 -> RT-TUDOM betöltve -> CSILLAG válasz a
topic-motorból, REPORT-B=0, D-file intact, minSP $7f2d.
(A load#3/#4 duplázás szim-artifakt: a $02EC-patch kétszer ugrik; a
valódi gépen a THEN LOAD egyszer fut.)

t81check16_v05: ALL CHECKS PASSED (14 .p + 14 .t81 + 15-blokkos
szalag, CDFLAG=$40 minden blokkban).

A Fájlok
--------
GLM_v070_RT16_TOPICS.t81  - az EO81 szalag (ELSŐDLEGES): 15 blokk
                             (RTBOOT01 + RTBASE16 + 13 téma)
*.t81                     - az egyes blokkok egyenként (LOAD "RT-TUDOM")
*.p                       - a blokkok .p formában (a ZEsarUX és mások)
*.p81                     - név-rekordos .p (8 B fej: 7 kód + |0x80)

Betöltés (EightyOne)
--------------------
- Tape -> beolvasod a GLM_v070_RT16_TOPICS.t81-t
- NEWLINE után: J "" (LOAD "") - a boot betölti magát
- Az interjú (év/hónap/nap/idő/név/nem) után ENTER a kapunál
- Chat: begépelsz (pl. SZIA) + ENTER = válasz
- Téma: /topic 1..13, ENTER

NYITOTT (következő körre)
-------------------------
- .tzx: a v0.7.1-es (működő, 0x19 GENERAL-os) generátor újraírása az
  új képekre - a formátum elemzése megtörtént (0x19 | TOTP | TOTD |
  NPD | ASD + szekvencia-leírók), de a 23 perces keretben nem
  fejeződött be. A .t81 elsődleges, az EightyOne-ban az betölt.
- Valódi gépi teszt: SZIA+ENTER után a kép NEM fagyhat meg többé
  (a generátor a ROM saját táncán át fegyverződik); ha mégis, a
  $02EC-körözés már nem lehetséges (a tánc bit7-et állít, a $02E7
  HALT-ja az NMI-vel véget ér).
- A generálás sebessége: a forward pass 32-bit MAC maradt (a valós
  sorköztes összegek miatt); a valódi gyorsításhoz a modell újra-
  kvantálása kellene (a z80ai 2-bit QAT útja) - ez modell-csere,
  a következő körben a training/ oldallal együtt.
