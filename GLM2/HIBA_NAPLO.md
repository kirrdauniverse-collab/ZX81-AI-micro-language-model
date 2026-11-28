# HIBA- ÉS JAVÍTÁSI NAPLÓ — GLM retro-tinyllm 16K vonal

Konszolidált napló a GLM vonal összes ismert hibájáról, javításáról és visszavont
kísérletéről (forrás: a v0.7.1 átadási csomag dokumentumai, a v0.7.2/v0.7.3
munkakönyvtárak dokumentumai és a forráskód verzió-kommentjei, valamint a
valós gépi visszajelzések). A tények és a hipotézisek elkülönítve.

## 1. Képfájl-fejléc korszak (v0.4.x – v0.5.0) — betöltési hibák

- v0.4.1: a kitalált 2 bájtos "FF 00" prefix elcsúsztatta a képet, a loader
  D_FILE=$0001-et olvasott (ROM szemét) → függőleges csíkok / görgető
  képernyő, azonnali fagyás. Javítás: a kép közvetlenül $4009-től indul.
- v0.5.0: a WORD (verem-tető) $4028-ra írva a gép SP=$0002-vel azonnal
  meghalt. Javítás: WORD a $4026-on ($7FFD).
- DF_SZ=2 (alsó képernyő két sor) és a $4029-es D_FILE-másolat fixálva.

## 2. Altint lánc-blob korszak (v0.5.7 → v0.6.6)

- A ROM saját SLOW rutinja verziófüggő (v2: $0F2B, v1: $0F28) — a $0F2B
  hívás v1 gépen utasítás közepére esik, REPORT B (pontosan a user valós
  gépén tapasztalt hiba). Javítás (v0.6.6): a 181 B-os lánc-blob ($7A40
  környéke) saját SET 6,(IY+$3B) + $0207-es tánccal verziófüggetlen;
  blob[6] hívás CALL $0F2B → CALL $0207 (altint_v066.bin).
- A blob mérete szigorúan 181 B marad: +8 B átlökte a STKEND-et $7B00 fölé,
  és a ROM számológép-verme elpusztította a D-file állapotsorait.

## 3. A generálás összeomlása (v0.6.5 / v0.6.6) — "összeomlott a kép"

A szállított v0.6.5/v0.6.6 binárisokban a válaszgenerálás kb. 30 frame-en
belül önmegsemmisített: sérült stack-RET (utolsó PC $655D) visszaütötte a
motort a BASIC-be, a 2. sor THEN LOAD-ja elsütődött, a ROM szalaghurok
kinullázta a felső memóriát (ROM-szinten bizonyítva). Ehhez a korszakhoz
tartozik a GLM_v065_RT16_TOPICS.t81 kazetta — ebben a motorban még NINCS
display-kikapcsolás (kód-elemzéssel igazolva: OUT($FD) sehol, kb_rearm hurok
sehol, CALL $0207 sehol).

## 4. A display-kikapcsolás bevezetése (v0.6.7) — a fagyás-korszak kezdete

A generálást innentől a display-megszakítások KIKAPCSOLVA futtatja (ZX81
FAST): a gen_reply elején RES 6 (CDFLAG) + OUT($FD); a végén a visszakapcsolás
kb_rearm: SET 6 (CDFLAG) + 17×OUT($FE) (DJNZ hurok). A képernyő a ~1-2 s
"gondolkodás" alatt elsötétül, majd a válasszal visszajön. A SZIMULÁTORBAN
ez működik. VALÓDI GÉPEN: ez a pont a SZIA+ENTER utáni képfagyás kezdete —
a kb_rearm 17×OUT($FE) sorozata a valódi hardveren NEM fegyverzi át az
NMI-generátort. Minden azóta szállított kazetta ezt a kódot hordozza.

## 5. v0.6.8 – v0.7.1 — más hibák javítása, a fagyás marad

- v0.6.8: a crt0 a betöltés után kapura zárja a display-t, majd a main a
  lánc telepítése (CALL $7A40) után visszakapcsol: SET CDFLAG bit6 +
  17×OUT($FE). (A topic-betöltés utáni fagyás ellen.)
- v0.7.0: a .tzx véglegesen betöltődik: minden blokk 0x19 GENERAL formátumú.
  A 0x10 ROM-blokkos .tzx/.tap ZX81-re NÉMA (az EightyOne EventTurbo-ja
  sosem írja a TZXByte-ot) — ez volt a "be se töltődik" gyökere.
- v0.7.1: a /topic lánc 3. USR-halálának gyökere és javítása: a $7A40-es
  blob eleji CALL $0207 a ROM display-walkját indítja, ami a D-file sorait
  Z80-kódként "végrehajtja"; a topic-csere UTÁN a D-file a 2. USR csomagolt
  chat-állapotát hordozza (0x00–0x3F = JR-opkódok!) — a walk rajtuk fut le,
  a PC $2-re esik, report C. JAVÍTVA: a main() a tánc ELŐTT kinullázza a
  D-file 3 munkasorát. Motor: 13127 → 13242 B (+115 B). SZIM (5200 frame):
  3 USR életben, REPORT-B=0, minSP $7f31.

## 6. A SZIA+ENTER képfagyás diagnózisa (2026-11-25, v0.7.1-nél)

A user jelentése: BASE betöltve, SZIA+ENTER után a képernyő megfagyott
(7 fekete cella a sor elején; a PC $51A4 és $FDxx tartományokban jár).

- $51A4 = kb_scanbits — a saját billentyű-szkenner ciklusa: a gép ÉL, a
  getline-ban ül. NEM összeomlás!
- $FDxx = a RAM-on túli tükör — a display-NMI walkja a D-file sorait
  "kódként" futtatja, a ROM $02BA-es JP (HL)-je a cellákból épült ellenséges
  címre ugrik — ettől fagy a kép.
- GYÖKÉR: a gen_reply eleji display-KI (RES 6 + OUT($FD)) utáni visszakapcsolás
  (kb_rearm: SET 6 + 17×OUT($FE)) a szim NMI-modelljét elégíti ki (a flag az
  utolsó OUT irányán áll), de a VALÓDI gép generátora csak a ROM $0207
  SLOW-táncának HALT-ján keresztül fegyverződik át újra.

## 7. Visszavont korai kísérletek

- SET 6 a tánc elején — a 2. USR NMI-viharban állt meg. VISSZAVONVA.
- A blob CALL NOP-olása — a boot a 3. frame-ben meghalt. VISSZAVONVA.
- v0.7.2-kísérlet #1 (a tánc közvetlenül a gen_reply végén) — szim-regresszió
  (a PC a main elejére köröz). VISSZAVONVA, majd más formában újra (l. 8.).

## 8. v0.7.2 (leszállítva): a SET 6+SET 7+CALL $0207 és a teljesen fekete kép

A v0.7.2 leszállított javítása a gen_reply végére: a D-file 3 munkasorának
kinullázása + SET 6 + SET 7 (CDFLAG=$C0) + CALL $0207 — a szim (javított
$02EC-átugrással) zöld volt: 3 USR életben, REPORT-B=0, minSP $7f2d.

A user valós gépi jelentései ez után:
- "Ez nem elsötétül, hanem összeomlott a kép!"
- "Középen egy vízszintes fekete csíkot látok futás közben is!"
- "ez képernyő memória összeomlásra utal" (user-hipotézis — l. DFAUDIT alább:
  KIZÁRVA, a D-file a halál pillanatában is szerkezetileg ép volt).
- "Nem!!!!! Ez nem lép tovább! Kiakad teljesen!"

A valós gépi gyökér (a $0207 tánc bájtszintű dekódolásából):

    $0207: LD HL,$403B / LD A,(HL) / RLA / XOR (HL) / RLA / RET NC

A tánc CSAK bit6≠bit7 esetén fut végig (az XOR+RLA pontosan akkor törli a
Carry-t, ha a két bit egyenlő) — a 17×OUT($FE) fegyverzés, az OUT($FD), a
SET 7 és a display-walk csak a végigfutó ágon él. A SET 6 + SET 7 ($C0)
esetén a RET NC azonnal visszatért: a generátor a generálás eleji OUT($FD)
óta kikapcsolva maradt → se kép, se keret (a border fehérjét is ugyanaz az
NMI-von generálja). A szim azért nem látta, mert a D-file-t közvetlenül a
memóriából olvasta, nem a videó-úton keresztül.

Mérési eredmények (sim_v072b.js műszerek):
- DFAUDIT (frame-enkénti D-file szerkezet-ellenőrzés): EGYETLEN sértés sem —
  a képernyőmemória-összeomlás hipotézis KIZÁRVA.
- TV-RENDER (valós Drawer.logika + raszter + HSYNC): a generálás alatti
  ~120 frame-es NMI=0 ablakban a TV szinkronja eltolódik — a VALÓDI TV-n
  ez nem tiszta fehér kép, hanem az ÖSSZEOMLOTT, FUTÓ FEKETE SÁV, amit a
  user látott; v0.7.2-nél ez az állapot MARADANDÓ volt (a NMI-von nem jött
  vissza).

## 9. v0.7.3 (leszállítva): a 17 bájtos végleges javítás

A gen_reply végére a main()-belépésnél valós gépen már bizonyított minta
(drop-in csere a $4CC3-nál, a méret változatlan: motor 13384 B):

1. a D-file 3 munkasorának kinullázása (32 blank + $76 minden sor),
2. CALL $7A40 — a blob: PUSH IY / LD IY,$4000 / CALL $0207 (CDFLAG=$80-cal
   hívódik: bit7 a main-belépési táncból, bit6 a gen_reply eleji RES 6-tól
   törölve → a tánc VÉGIGFUT: 17×OUT($FE) fegyverzi a valódi generátort,
   SET 7 landol, a walk az új válasz-sorokat rendereli a $7A40 láncunkon át),
3. LD A,($403B) / OR $40 / LD ($403B),A — bit6 IY-kezelés nélkül (CDFLAG=$C0),
4. LD B,$11 / OUT($FE),A / DJNZ — a disp_on burst (a tánc végi OUT($FD)
   után ez tartja életben a videót).

A bináris bájtonként: CD 40 7A | 3A 3B 40 | F6 40 | 32 3B 40 | 06 11 |
D3 FE | 10 FE (tools/patch_v073.py, idempotens).

SZIM-BIZONYÍTÉKOK (6900 frame): VIDEO RECOVERY f2520 (SZIA válasz után,
CDFLAG=$C0) és f5283 (CSILLAG topic válasz után); a generálás alatti FAST-ablak
NMI=0 a SZÁNDÉKOS állapot; 3 USR életben, REPORT-B=0, minSP $7f2d, D-file
intact. (A z88dk ragel-lépése offline nem futtatható, ezért a v0.7.3 a
v0.7.2 binárisának pontos 17 bájtos patche; a C forrás ugyanezt a blokkot
tartalmazza dokumentálva — a következő tényleges fordításnál összeér.)

## 10. v0.7.3b: a "nem lépett tovább" ügy — RÉGI SZALAG VOLT BETÖLTVE

A user jelente a v0.7.3-ról: "ez nem lépett tovább, kiakad teljesen".
A fotó elemzése (photoanalyze.py + VLM kiolvasás) megmutatta:

1. A fotó az EightyOne ablaka: a D-file tartalmából a boot végét erezte:
   a teljes párbeszéd ("NEVED? SAFI", "RENDBEN, SAFI...", "TEKERJ A
   RTBASE16-HOZ") és a "YOU> SZ" — a SZIA begépelve, a generálás elindult.
2. A memory dump $7AAE körüli bájtjai a mi altint-láncunk, DE a kilépő ágban
   a fotó "3A 26 40 47" = LD A,($4026); LD B,A — a v0.7.3-as lánchoz ez nem
   illik (nálunk "3A 28 40 4F"). A betöltött RTBASE16 egy v0.6.x-kori, RÉGI
   engine volt, amelyben a fekete-kép hiba még BENNE VAN.
3. A banner a képen "V0.6.2" — a boot16.c-ben a banner v0.6.2 óta sosem
   frissült, ezért a régi és az új szalag a TV-ről megkülönböztethetetlen
   volt. A hiba a készítőt terítette el: a user egy korábbi körben letöltött
   szalagot töltött be újra.

A válasz (nem új kód):
- a letöltésben lévő minden fájl v0.7.3-as (a $4CC3-as lezárás-patch minden
  .t81/.p81/.p-ben benne van — bájtellenőrizve),
- a banner mostantól a tényleges verziót írja: "retro-tinyllm 16k v0.7.3"
  (a kazetta mind a 15 blokkjában ellenőrizve),
- TESZT-ELJÁRÁS: betöltés után a második sorban a "RETRO-TINYLLM 16K V0.7.3"
  álljon; ha "V0.6.2" — régi szalag van betöltve. És: a "TEKERJ A RTBASE16-HOZ"
  után ENTER + a TAPE LEJÁTSZÁS INDÍTÁSA is kell — enélkül a gép a
  leader-várakozásban áll és a kép FAST-ban gyűrődik (önmagában is "összeomlott
  kép + futó fekete csík" látvány).

## 11. Aktuális állapot és következő lépések (2026-11-27)

- A v0.7.3 kazetta (MD5 48077afe25c2cd7ab2056fbe2cbb1f60) szimben teljesen
  tiszta; a fennmaradó kérdés a valódi gépi teszt a HELYES szalaggal (a
  banner-ellenőrzéssel). Ha a válasz után 10 mp-rel sem tér vissza a kép:
  valódi hiba — a memory dump $7A40-$7B60 körzete kell (mint az előző fotón).
- Maradt még: a .tzx írása az új képekre (a v0.7.1-es 0x19 GENERAL formátum
  újraírása); a z88dk ragel-lépésének offline megkerülése, hogy a forrásból
  tényleges fordítás is menjen; sebesség (a forward 32-bit MAC maradt; a valódi
  gyorsításhoz modell-újrakvantálás kell).

## 12. Szabályok, amiket a hibák megtanítottak

- A D-file cellái a ROM display-walkja elől sosem lehetnek "ellenséges"
  bájtok (0x00–0x3F = JR-opkódok) a tánc indításakor — nullázd elöl.
- A motor mérete rögzített guard (jelenleg 13384 / 5085 B); ha változik, a
  t81build16.py-t frissítsd, és futtasd a t81check16_v05.py-t.
- A szim NMI-modellje a valódi generátor HALT-fegyverzését NEM modellezi —
  ahol a display/NMI út játszik, ott a szim zöldje nem elegendő.
- A $0207 tánc csak bit6≠bit7 esetén fut végig — CDFLAG-ba soha ne írj
  egyszerre bit6-ot ÉS bit7-et a hívás ELŐTT.
- A .tzx CSAK 0x19 GENERAL blokkokkal él EightyOne-ban — a .tap és a 0x10-es
  blokk ZX81-re halott út.
- A kazettafájl-nevet soha ne használd újra verzióváltásnál változatlanul úgy,
  hogy a TV-n nem látszik a verzió — a banner-ellentmondás a v0.7.3b ügy
  oka. (A javított banner-től kezdve a TV-n ellenőrizhető.)
- A #asm blokkokban a fordító IX/IY-cseréje miatt a "push ix" gébi PUSH IY;
  hosszú `;-kommentek az #asm-ben fordítási hibát adnak.
