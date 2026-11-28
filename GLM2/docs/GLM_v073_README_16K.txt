GLM retro-tinyllm 16K - V0.7.3 (a fekete-kép javítás)
======================================================

A TE TÜNETED ÉS A GYÖKERE (lépésről lépésre visszavezetve)
----------------------------------------------------------
A v0.7.2-nél SZIA+ENTER után a kép ÉS a border is fekete lett. A fekete
border a kulcs: a ZX81-en a border fehérét is a scanline-NMI von generálja
(ugyanaz a video-út, mint a képtartalom) - ha a border is fekete, a NMI
generátor teljesen kialudt, nem csak a D-file fagyott meg.

A gép közben ÉLT: a szimben a halál pillanatában a PC a $7469-$7477 RNG-
futójelén körözött (a mintavétel xorshift-je) - vagyis a modell dolgozott,
csak a videó volt halott.

A GYÖKÉR (ROM $0207 tánc bájtszintű dekódolásából):

    $0207: LD HL,$403B / LD A,(HL) / RLA / XOR (HL) / RLA / RET NC

  A tánc CSAK AKKOR fut végig (és csak akkor adja ki a 17x OUT ($FE)
  fegyverző sorozatot, az OUT ($FD)-et, a SET 7-et és a display-walk-ot),
  ha CDFLAG bit6 != bit7. Ha bit6 == bit7 (pl. $C0), a RET NC azonnal
  visszatér - fegyverzés NÉLKÜL.

  A v0.7.2 gen_reply záró blokkja: SET 6 + SET 7 (CDFLAG=$C0) + CALL $0207
  -> a bit6==bit7 miatt RET NC -> a 17x OUT ($FE) SOSEM futott le ->
  a generátor a generálás eleji OUT ($FD) óta kikapcsolva maradt ->
  se kép, se border.

  (A v0.7.1 kb_rearm-jével ellentétben itt még a szimben sem jött volna
  vissza a kép - a SET 7 miatt a tánc rögtön visszatért.)

A JAVÍTÁS (v0.7.3): a gen_reply végén a main()-beli, valós gépen bizonyított
minta fut - pontosan 17 bájt, drop-in csere a $4CC3-nál (a v0.7.2 blokk
helyén, a méret változatlan: 13384 B):

    CD 40 7A    CALL $7A40   ; a blob: PUSH IY / LD IY,$4000 / CALL $0207
                             ;   CDFLAG=$80-cal hívódik (bit7 a main-belépési
                             ;   táncból, bit6 a gen_reply eleji RES 6-tól törölve)
                             ;   -> a tánc VÉGIGFUT: 17x OUT ($FE) fegyverzi a
                             ;   VALÓDI generátort, SET 7 landol, a walk az új
                             ;   válasz-sorokat rendereli a $7A40 láncunkon át;
                             ;   a blob FRAMES-várakozása + tiszta RET (IY helyre)
    3A 3B 40    LD A,($403B)
    F6 40       OR $40       ; SET 6 IY-kezelés nélkül (CDFLAG=$C0 = SLOW)
    32 3B 40    LD ($403B),A
    06 11       LD B,$11     ; a main()-beli disp_on burst:
    D3 FE       OUT ($FE),A  ; 17x OUT ($FE) - a tánc végi OUT ($FD) után ez
    10 FE       DJNZ         ; kapcsolja vissza a generátort (szim + valóság)

A SZIMULÁTOR JAVÍTÁSA (a te kérdésedre: igen, most már lépésről lépésre
követhető): sim_v072b.js új műszerek:
  - VIDEO LIVENESS: a doScanline NMI-számlálója frame-enként. A generálás
    alatti ~100-740 frame-es NMI=0 a SZÁNDÉKOS FAST állapot; ami számít: a
    generálás UTÁN visszatér-e (VIDEO RECOVERY log) és SLOW-ban nem halott-e.
  - DANCETRACE=1: utasításszintű trace a $0207 tánc körül (PC, CDFLAG,
    Carry, nmiGen minden utasításnál) - ezzel mutattuk ki a RET NC-t és a
    javítás utáni végigfutást.
  - VIDEOTRACE=1: NMI/frame váltások.

SZIM-BIZONYÍTÉKOK (v0.7.3, 6900 frame):
  - f1419-1420: main() belépés tánca CDFLAG=$80-cal -> végigfut (nmiGen=true)
  - f2402-f2519: SZIA generálás FAST-ban (NMI=0 - normális)
  - f2520: VIDEO RECOVERY - a kép visszatért, CDFLAG=$C0 (a javított blokk)
  - f5141-f5282: CSILLAG (topic) generálás FAST-ban
  - f5283: VIDEO RECOVERY - a kép visszatért, CDFLAG=$80->$C0
  - 3 USR életben, REPORT-B=0, minSP $7f2d, D-file intact
  - t81check16_v05: ALL CHECKS PASSED (14 .p + 14 .t81 + 15 blokk; a .t81
    blokkjai a patchelt 13384 B motort hordozzák - bájtellenőrizve)

A z88dk újraépítése ebben a körben megakadt (a z80asm ragel-generálta
parse_rules lépése offline nem futtatható, a generált .c tönkrement) - ezért
a v0.7.3 a v0.7.2 binárisának pontos, 17 bájtos javítása
(tools/patch_v073.py, idempotens, bájt-azonosítható). A C forrásban ugyanez
a blokk van beírva dokumentálva (app16.c gen_reply vége), így a következő
tényleges fordításnál a forrás és a bináris összeér.

FÁJLOK
------
ports/zx81/16k_v073/GLM_v070_RT16_TOPICS.t81  - a TELJES kazetta (ELSŐDLEGES,
  EightyOne-be ez való: 15 blokk = RTBOOT01 + RTBASE16 + 13 téma)
ports/zx81/16k_v073/*.t81  - blokkonként (LOAD "RT-TUDOM" stb.)
ports/zx81/16k_v073/*.p    - .p formában (ZEsarUX és társai)
ports/zx81/16k_v073/*.p81  - név-rekordos .p (8 B fej)
tools/patch_v073.py        - a v0.7.2 -> v0.7.3 bináris patch (reprodukálható)
tools/z80dis.py            - saját Z80 disassembler (a ROM-elemzéshez)
scripts/sim_v072b.js       - a videó-életdetektoros szim (VIDEOTRACE/DANCETRACE)
src/ports/zx81/app16.c     - a forrás a v0.7.3 blokkal + teljes magyarázat
docs/GLM_v073_README_16K.txt - ez a fájl

TESZTELÉS VALÓDI GÉPEN (EightyOne)
----------------------------------
1. Tape -> GLM_v070_RT16_TOPICS.t81 betöltése
2. J "" (LOAD "") -> a boot betölt; interjú (dátum/név/nem), ENTER a kapunál
3. SZIA + ENTER: ~1-2 mp fekete (a "gondolkodás" - ez így helyes!), majd a
   válasz ÉS a kép visszatér. A /topic 1 után ugyanez a minta.
4. Ha a válasz után a kép mégis fekete maradna: az új hibakép a
   DANCETRACE-hez hasonlóan a tánc végén lévő OUT ($FD)-ről szólna - de a
   17x OUT ($FE) burst közvetlenül utána áll, szimben bájtonként végigkövetve.

KÖVETKEZŐ KÖRRE MARADT
----------------------
- .tzx változat (a v0.7.1-es 0x19 GENERAL formátum újraírása az új képekre)
- a z88dk ragel-lépésének offline megkerülése, hogy a forrásból tényleges
  fordítás is menjen (a bináris patch addig is teljes értékű)
- sebesség: a forward 32-bit MAC maradt (a valós sorközi összegek 1.5M-ig
  mennek); a valódi gyorsításhoz modell-újrakvantálás kell

================================================================================
V0.7.3 KIEGESZITES: a "fekete sav / osszeomlott kep" diagnozis (DFAUDIT + TV-RENDER)
================================================================================
A felhasznalo jelzese a v0.7.2-rol: "nem elsotetul, hanem OSSZEOMLOTT a kep,
kozepen vizszintes fekete csik futas kozben is" - kepernyomemoria-osszeomlas
gyanuja. Ezt kovetkeztetes helyett MERESSEL dontottuk el.

Uj muszerek (sim_v072b.js):
  1. TV-RENDER: a szim a valodi Drawer.draw() logikaval (raszter X/Y + HSYNC
     tolerancia) TENYLEGES TV-kepet rajzol frame-enkent (SNAPFRAMES=...),
     es a tools/tvanalyze.py sorprofil-analizist ad (fekete/feher/szurke
     soronkent, fekete sav-detektalassal).
  2. DFAUDIT: frame-enkenti D-file struktura-ellenorzes:
     (a) minden sor 33. bajtja $76 (HALT sorzaro),
     (b) nincs bit7-es bajt a 32 cellaban (inverz video = fekete sav!),
     (c) a D_FILE mutato (0x400C) nem valtozik,
     (d) a DF_CC a D-file tartomanyaban marad.

DFAUDIT EREDMENY (v0.7.2, a teljes boot->SZIA->valasz futas):
  EGYETLEN serto sem tortent: minden $76 zaro ep, nincs bit7-es cella, a
  D_FILE mozdulatlan, a DF_CC a helyen. A KEPERNYOMEMORIA-OSZEOMLAS
  HIPOTEZIS KIZARTHATO - a D-file a halal pillanataban is strukturailag
  egeszseges volt.

TV-RENDER EREDMENY (v0.7.2, a SZIA valasz kornyeken):
  f2200-f2400: ep szoveges kep (feher 62%, szoveg-fekete 2.5%)
  f2450-f2519: a kep KIURULT (0% fekete szoveg) - a generalas alatti FAST
               ablak: a NMI-von megszakadasa miatt a TV szinkrona eltolodik.
               A VALOSAGI TV-n ez nem "tiszta feher kepernyo", hanem az
               OSSZEOMLOTT, FUTO FEKETE SAV amit a felhasznalo latott!
               (a szim Drawer-e a HSYNC-tolerancia miatt 'rendezi' a
               rasztert - a valodi TV mar nem birja rendezni)
  f2521: ATMENET - 24% fekete: az osszeomlas elterulo frontja
  f2560+: A TELJES lathato ablak fekete (a NMI-von mar nem jott vissza)
         - a v0.7.2 vegeredmenye a valodi gepen: maradandosan fekete

TV-RENDER EREDMENY (v0.7.3, ugyanezek a frame-ek):
  f2200-f2400: ugyanaz az ep kep
  f2450-f2519: ugyanaz a kiurult ablak (a generalas - ez igy HELYES, kb 2
               masodperc a valodi gepen, amig a modell 'gondolkodik')
  f2521: A KEP VISSZATERT (2.7% fekete = mar szoveges tartalom!)
  f2560-f2650: stabil ep kep (a chat varakozas kepe)
  + VIDEO RECOVERY f2520 log: CDFLAG=$C0, a generator ujra fegyverzett

KULONBSEG: a v0.7.2-nel az osszeomlott allapot MARADANDO (a blokk a tanc
RET NC-jevel nem fegyverzett), a v0.7.3-nel ugyanaz az ablak utan a kep
visszater. A futtatas kozben lathato vizszintes fekete csik a v0.7.3-nal
a generalasi idoszakban (kb 2 mp) VART JELZEN - utana helyreall a kep.

A kepek: docs/tv_kepek/tv072_tv_f*.png es tv073_tv_f*.png (520x380, a
szim TV-rasztere; a szurke sav a nem-megrajzolt raszterterulet).

================================================================================
V0.7.3b VERZIO-JELoles ES A "NEM LEPT TOVABB" UGY (a felhasznalo fotojabol)
================================================================================
A felhasznalo masodik jelzese: "ez nem lept tovabb, kiakad teljesen" - a foto
(elemzese: photoanalyze.py + VLM kiolvasas) a kovetkezo allapotot mutatta:

  1. A fot nem valodi TV-kep, hanem az EIGHTYONE emulátor ablaka (TV +
     memory dump + tape browser panelekkel).
  2. A D-file tartalmabol a boot VEGET ERETT: ott a teljes parbeszed
     ("NEVED? SAFI", "NEMED (F/N)? F", "RENDBEN, SAFI...", "TEKERJ A
     RTBASE16-HOZ"), es a "YOU> SZ" - a SZIA mar begepelve, a generalas
     elindult.
  3. A memory dump $7AAE koruli bajtjai ($D3 FF 2A 0C 40...) a mi
     altint-lancunk, DE a kilepo agban a foto 3A 26 40 47 = LD A,($4026);
     LD B,A - a v0.7.3-as lanchoz ez NEM ilik (nálunk: 3A 28 40 4F =
     LD A,($4028); LD C,A). A betoltott RTBASE16 egy v0.6x-kori, REGI
     engine volt, amelyben a v0.7.2 fekete-kep hibaja meg BENNE VAN.
  4. A banner a kepen "V0.6.2" - a forrasban (boot16.c) a banner
     v0.6.2 ota sosem frissult, ezert a regi es az uj szalag a TV-rol
     MEGKULONBOZTETHETETLEN volt. Ez a hiba engem (a keszito) teritett
     el: a felhasznalo egy korabbi korben letoltott szalagot toltott
     be ujra.

A VALASZ TEHAT NEM uj kod, hanem:
  - A letoltesben levo minden fajl v0.7.3-as (ellenorizve: a $4CC3-as
    lezaras-patch minden .t81/.p81/.p-ben benne van).
  - A banner mostantol a tenyleges verziot irja (v0.7.3). A binaris
    csere minden szallitott fajlban megtortent (boot banner + a
    hal_panic "tinyllm v0.6.2" szoveg).
  - A TESZT ELJARAS: betoltes utan a masodik sorban a
    "RETRO-TINYLLM 16K V0.7.3"-at kell latni. Ha a kepernyo "v0.6.2"-t
    ir, REGI szalag van betolve - abba meg benne van a generakodas
    utan fekete marado kep hibaja (a v0.7.2 "set 6+7" lezarasa nem
    fegyverzett generatornal hivta a $0207 tancot).

A helyes teszt (EightyOne):
  1. Tape > Open: GLM_v070_RT16_TOPICS.t81 (a letoltesbol!)
  2. LOAD "" (J betu + Shift+P... vagy a menubol), ENTER
  3. A boot kerdeseire valaszolj (pl. 11, 11, 11, 11, SAFI, F)
  4. "TEKERJ A RTBASE16-HOZ! ENTER=TOLTES" - nyomj ENTER-t, es a
     TAPE LEJATSZASAT INDITSD EL (Play gomb a tape panelen) - e nelkul
     a gep a leader-varakozasban all, es a kep FAST-ban gyurodik!
  5. Betoltes utan a banner: V0.7.3. Utana gepeld: szia + ENTER.
     A generalas alatt (~2-4 mp) a kep osszenyomodik/gyurodik - ez
     NORMALIS (FAST). A valasz utan a kep VISSZATER es marad.
  6. Ha a kep a valasz utan 10 mp-el sem ter vissza: valos hiba -
     ird le mi latszik, es masold le a memory dump $7A40-$7B60
     korzetet (mint az elozo foton - az aranyat ert!).

A szim-ellenorzes a patchelt szalaggal: a teljes boot->chat->generalas->
valasz folyamat tiszta, a kep f2520-nal visszater, f2700-ig stabil
(lathato ablak: ~5000 fekete pixel = elo szoveges kep), DFAUDIT: 0 sertes.
