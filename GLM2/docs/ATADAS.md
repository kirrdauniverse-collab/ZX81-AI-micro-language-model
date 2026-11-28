# GLM retro-tinyllm v0.7.1 — ÁTADÁS (gépi olvasásra)

Dátum: 2026-11-25. Ez a ZIP „kézikönyve” a folytató modellnek: rövid, nyersszöveges
formában tartalmazza az ATADAS_GLM_v071.docx lényegét. A pontos levezetéseket a
docs/ALLAPOT.txt, a user-oldali betöltési útmutatót a docs/README_16K.txt hordozza.

## 1. Mi van a csomagban (csak a legutolsó verzió)

```
tape/GLM_v070_RT16_TOPICS.t81   227923 B  ELSŐDLEGES: 15 blokk (BOOT+BASE+13 téma)
tape/GLM_v070_RT16_TOPICS.tzx   228634 B  TZX 1.20, 0x19 GENERAL blokkok (EightyOne-betöltő)
tape/GLM_v070_rt16_<téma>.p     15889 B  14 db (base + 13 téma), ZEsarUX-út
tape/GLM_v070_rt16_<téma>.p81   15897 B  14 db, névrekordos .p
tape/GLM_v070_rt16_<téma>.t81   15945 B  14 db, egyenként betölthető (LOAD "RT-TUDOM")
```
Témakódok: base, art, asciiart, astronomy, health, hobby, learning, literature,
magyar, movies, programming, science, verse, work.
Régi verziók (v055…v069) NINCSENEK a csomagban — szándékosan.

```
src/ports/zx81/    app16.c (863 sor), boot16.c (311), hal_zx81.c (394),
                    main_zx81.c, scratch_stub.c, rtframe.asm, build16k/zx81rt_crt0.asm
src/engine/         rt_*.c/.h — a gépfüggetlen motor (~2600 sor)
src/build_v070.sh  a z88dk 2.2 build (lásd 4. pont)
tools/             t81build16.py, t81check16_v05.py, altint_v066.bin, modelinfo.py…
src/simenv/        a szimulátor (EightyOne-magú TS-emulátor, zx81.rom)
scripts/          sim_topic_v067.js + társai (szim-futtatók), build_v070.sh
docs/             ATADAS.md (ez), ALLAPOT.txt, README_16K.txt
```

## 2. Betöltés (EightyOne, 16K, valódi ROM)

1. Tape → GLM_v070_RT16_TOPICS.t81 → NEWLINE → J "" (LOAD "")
2. Interjú (év/hónap/nap/óra/perc/név/nem), minden válasznál ENTER
3. Prompt: `you>` — beírsz (pl. SZIA) + ENTER = válasz
4. `/topic 1..13` + ENTER = témacsere (betölti a blokkot, a téma-motor megy tovább)

A .tzx CSAK így él: 0x19 GENERAL blokkok (TOTP=0, TOTD=len*8, NPD=19, ASD=2,
530/520 µs). A 0x10 ROM-blokkos .tzx/.tap ZX81-re NÉMA (EventTurbo sosem írja a
TZXByte-ot) — soha ne térj vissza hozzá.

## 3. Pillanatnyi állapot (szimmel bizonyított)

- Motor v0.7.1 = 13 242 B, boot = 5 085 B (méret-guard a t81build16.py-ben!)
- Teljes lánc igazolt 5200 frame-en: boot → interju → SZIA chat → /topic 1 →
  RT-TUDOM betöltve → a topic-motor választ generál ($73db-$73e9 hurok),
  REPORT-B=0, minSP $7f31. (A v0.7.1 előtt a 3. USR report-C-vel meghalt:
  a D-file 3 munkasorát a $0207-tánc előtt nullázni kell — már a main()-ben van.)
- Verifikátor: 16/16 OK (t81 + tzx + 14 .p)
- ISMERT NYITOTT HIBA (csak valódi gépen): SZIA+ENTER után a képernyő megfagy.

## 4. Újraépítés

```sh
sh scripts/build_v070.sh          # z88dk: -> build_v070/rtchat16 (13242) + rboot16 (5085)
python3 tools/t81build16.py ports/zx81/build_v070/rtchat16 \
        ports/zx81/build_v070/rboot16 models16_v05 ports/zx81/16k_v070
python3 tools/t81check16_v05.py  # 16 OK-t vár
# szim:
MAINADDR=4CFB node scripts/sim_topic_v067.js 5200
```
z88dk a tools/src/z88dk-ban; a DEFS: RT_MAX_VOCAB=48 RT_MAX_CTX=8 RT_MAX_EMB=16
RT_MAX_HID=48 RT_APP_MODEL_MAX=10240 RT_MEM_MAX=6 RT_JOURNAL_MAX=8 RT_LINE_MAX=64.
A #asm blokkokban a fordító IX/IY-cseréje miatt „push ix” = gépi PUSH IY; hosszú
`;-kommentek az #asm-ben fordítási hibát adnak — C-kommentet a blokk elé.

## 5. A nyitott hiba gyöker (röviden)

$51A4 = kb_scanbits → a gép ÉL, a getline vár. $FDxx = display-NMI walk a D-file
sorait Z80-kódként futtatja (ROM $02BA JP (HL), HL a cellákból). A gyökér: a
gen_reply() elején RES 6 + OUT $FD (display ki), a végén a kb_rearm = SET 6 +
17×OUT $FE — a SZIM-ben elég, de a VALÓDI NMI-generátor csak a $0207 SLOW-tánc
HALT-ján át fegyverződik; ettől fagy a képernyő a gép élén. A v0.7.2 kísérlet
(tánc a gen_reply végén) a szim NMI-modelljében regresszióba futott (main-körözés)
→ visszavonva; a szim a tancviselkedést nem tudja hozni.

## 6. Javasolt sorrend a folytatáshoz

1. (a) kb_rearm + CDFLAG bit7 (SLOW) beállítása — a ROM $0F2B SLOW a bit7-et is
   állítja; a mi blokkunk csak bit6-ot. Kicsi, szimmel biztonságosan tesztelhető,
   de a bizonyítás VALÓDI GÉPEN történt (SZIA+ENTER; jó, ha a $FDxx eltűnik).
2. (b) tartalék: a display-KI fázis elhagyása (SLOW-ban generál — lassabb, biztos).
3. (c) a v0.7.2 tancos út csak valódi gépi teszttel zárható — a szim nem hozza.
4. Ezután: generálás gyorsítása ($73db-$73e9), ring-növelés, modell-csere
   (models16_v05/*.bin + training/ — a formátumleírás a docs/-ban).

## 7. Szabályok, amiket eddig megetettek a hibákkal

- A D-file cellái a ROM display-walkja elől sosem lehetnek „hostile” bájtok
  (0x00-0x3F = JR-opkódok) a tánc indításakor — nullázd a munkasorokat elöl.
- A motor mérete rögzített guard (13242/5085); ha változik, a t81build16.py-t
  frissítsd, és futtasd a t81check16_v05.py-t.
- A szim NMI-modellje (nmiGenerator flag az utolsó OUT irányán) a valódi
  generátor HALT-fegyverzését NEM modellezi — ahol a display/NMI út játszik,
  ott a szim zöldje nem elegendő.

## 8. Környezet

z88dk 2.2 (tools/src/z88dk), Node 22 (szim futtatásához), Python 3 (építés/
ellenőrzés), EightyOne (valódi gépi teszt). Minden útvonal a ZIP gykeréből
relatívan értendő.
