# VERZIÓ-TÖRTÉNET — GLM retro-tinyllm 16K, és a keresett T81

## A LÉNYEG — a válasz a kérdésedre
("melyik T81-től kezdve nem változott a hibajelenség a javítások ellenére sem?")

**A régi, v0.6.x-korú GLM_v070_RT16_TOPICS.t81 példány — amit a gép ténylegesen
betöltött minden teszten.** A bizonyíték a v0.7.3b fotóelemzés (2026-11-26):

1. A gépedbe betöltött RTBASE16 motorja egy v0.6.x-kori, RÉGI engine volt:
   a memory-dump $7AAE körüli bájtjai "3A 26 40 47" = LD A,($4026); LD B,A —
   a v0.7.3-as láncban ez "3A 28 40 4F" = LD A,($4028); LD C,A.
2. A TV-képen a banner "V0.6.2"-t mutatott — de a boot16.c-ben a banner
   v0.6.2 óta sosem frissült, ezért a régi és az új szalag a TV-ről
   MEGKÜLÖNBÖZTETHETETLEN volt.
3. Minden szállítás UGYANAZT a fájlnevet használta (GLM_v070_RT16_TOPICS.t81):
   a v0.7.1 (MD5 740959e6...), a v0.7.2 (MD5 cfe9b281...) és a v0.7.3
   (MD5 48077afe...) kazetta is — a letöltött mappákban egymás mellett
   ülve a Windows/EightyOne könnyen a régi példányt nyitja meg újra.

**Ezért nem változott a tünet egyetlen javítás ellenére sem: a javított kód
soha nem ért el a gépig — minden teszten ugyanaz a régi szalag futott.**

A tünet-mátrix (minden verzió valós gépi viselkedése):

| Betöltött kazetta | Banner a TV-n | SZIA+ENTER után |
|---|---|---|
| v0.6.x régi (a "bűnös") | V0.6.2 | a generálás utáni fekete/összeomlott kép MARAD (a v0.7.2 előtti kb_rearm-hiba benne van) |
| v0.7.1 (13242 B motor) | V0.6.2 (!) | képfagyás (kb_rearm 17×OUT($FE) a valódi gépen nem fegyver) |
| v0.7.2 (13384 B motor) | V0.6.2 (!) | teljesen fekete kép ÉS keret (a SET 6+7 miatt a tánc RET NC-vel visszatér) |
| **v0.7.3 (13384 B motor, aktuális)** | **V0.7.3** | ~2-4 mp "gondolkodás" (FAST-ablak, a kép összenyomódik — ez NORMÁLIS), majd a válasz ÉS a kép VISSZATÉR és marad |

## A v0.7.3-as teszt-eljárás (EightyOne, valódi gép)

1. Tape > Open: GLM_v070_RT16_TOPICS.t81 — a v0.7.3-as letöltésből!
   (Bizonyossághoz md5sum: 48077afe25c2cd7ab2056fbe2cbb1f60)
2. J "" (LOAD "") — a boot betölt; az interjúra (dátum/név/nem) ENTER.
3. "TEKERJ A RTBASE16-HOZ! ENTER=TOLTES" — nyomj ENTER-t, ÉS A TAPE
   LEJÁTSZÁST INDÍTSD EL (Play a tape panelen)! E nélkül a gép a
   leader-várakozásban áll, és a kép FAST-ban gyűrődik — ez önmagában is
   "összeomlott kép + futó fekete csík" látvány!
4. Betöltés után a banner: RETRO-TINYLLM 16K V0.7.3. Ha "V0.6.2" áll ott:
   RÉGI szalag van betöltve — abban még benne van a fekete-kép hiba.
5. SZIA + ENTER: ~2-4 mp összenyomódott kép (FAST — helyes!), majd a válasz
   és a kép visszatér. Ha 10 mp múlva sem tér vissza: valódi hiba —
   írd le mi látszik, és másold a memory dump $7A40-$7B60 körzetét.

## Verziótáblázat (a teljes vonal)

| Verzió | Motor | Szállított kazetta (MD5) | Ismert állapot / tünet |
|---|---|---|---|
| v0.4.x – v0.5.0 | — | (fejléc-leckék kora) | FF 00 prefix → függőleges csíkok/görgetés; WORD rossz helyen → SP=$0002 azonnali halál |
| v0.5.7 – v0.6.6 | — | — | altint-blob: blob[6] CALL $0F2B → CALL $0207 (verziófüggetlen SLOW); a blob 181 B-nél nem növelhető |
| v0.6.5/66 | 13127 B előzmény | GLM_v065_RT16_TOPICS.t81 (1162414c...) — repo | generálás ~30 frame alatt összeomlik (PC $655D, ROM szalaghurok kinullázza a felső memóriát); NINCS display-kikapcsolás a motorban |
| **v0.6.7** | — | — | **display-kikapcsolás a generálás idejére (RES 6 + OUT($FD); kb_rearm: SET 6 + 17×OUT($FE)) — a valódi gépi fagyás-korszak kezdete** |
| v0.6.8 | — | — | crt0 display-kapuzás + main visszakapcsolás a lánctelepítés után |
| v0.7.0 | 13127 B | (nem maradt fenn) | .tzx 0x19 GENERAL blokkok — a "be se töltődik" végleges javítása |
| v0.7.1 | 13242 B | GLM_v070_RT16_TOPICS.t81 (740959e6...) | topic-lánc 3. USR-halál javítva (D-file 3 munkasora nullázva); NYITOTT: SZIA+ENTER képfagyás valódi gépen (kb_rearm nem fegyver) |
| v0.7.2-kísérlet | — | NEM SZÁLLÍTVA | a $0207 tánc a gen_reply végén — szim-regresszió (PC a main elejére köröz); visszavonva |
| v0.7.2 | 13384 B | GLM_v070_RT16_TOPICS.t81 (cfe9b281...) | gen_reply végén: D-file sorok nullázása + SET 6 + SET 7 + CALL $0207 — SZIMBEN jó, de VALÓDI GÉPEN: teljesen fekete kép és keret (a tánc RET NC-vel visszatér, mert bit6==bit7; a generátor sosem fegyverződik) |
| **v0.7.3** | **13384 B** | **GLM_v070_RT16_TOPICS.t81 (48077afe...) — AKTUÁLIS** | a 17 bájtos javítás a $4CC3-nál: CALL $7A40 (a blob CDFLAG=$80-cal futja a táncot → 17×OUT($FE) fegyver) + OR $40 (bit6, IY nélkül) + 17×OUT($FE) burst; szim: VIDEO RECOVERY f2520 és f5283, 3 USR életben, REPORT-B=0 |
| v0.7.3b | 13384 B | ugyanaz + banner-javítás | a "nem lépett tovább" eset tisztázva: RÉGI (v0.6.x) szalag volt betöltve (V0.6.2 banner + $7AAE bájtok bizonyítják); a banner mostantól a valódi verziót írja (v0.7.3 — a kazetta mind a 15 blokkjában ellenőrizve) |

## Miért futott sokáig körbe a hiba — a négy réteg összefoglalva

1. **v0.6.5/66**: a generálás maga omlott össze (sérült stack-RET) — javítva.
2. **v0.6.7-től**: a display-kikapcsolás ("gondolkodás") bejött, de a
   visszakapcsolás (kb_rearm) a valódi gépen nem fegyverezett — a szim
   NMI-modellje nem modellezte a valós generátor HALT-fegyverzését.
3. **v0.7.2**: a javítási kísérlet (SET 6+SET 7+CALL $0207) rontott a
   helyzeten (a tánc RET NC-je miatt még a szimnél is rosszabb) — a $0207
   tánc bájtszintű dekódolása derítette ki (LD HL,$403B / LD A,(HL) / RLA /
   XOR (HL) / RLA / RET NC — csak bit6≠bit7 esetén fut végig).
4. **v0.7.3b**: a megfelelő javítás (v0.7.3) elkészült és jó volt — de a
   gépbe sosem került be, mert a betöltött kazetta egy régi példány volt,
   és a sosem frissülő banner miatt ezt a TV-ről nem lehetett észrevenni.

## Mit tartalmaz pontosan a GLM_v070_RT16_TOPICS.t81

EO81 konténer: 4 bájtos "EO81" mágia, majd blokkonként név[32] + hossz[16]
(ASCII decimális, NUL-ig töltve) + adat. Az adat = a .P kép bájtjai byte-onként
($4009-től, VERSN bájt elől). 15 blokk: RTBOOT01 (15 897 B) + RTBASE16
(15 093 B) + 13 téma (mind 15 093 B). A motor a RTBASE16 1. sorának REM-jében
ül ($4082-től), a modell-blob a lánc végén, az altint-blob az $7A40-es tájon
(a CALL $0207 a $7A4E-nél), a D-file $7B00-tól, a gépi verem $7FFC-nél.
