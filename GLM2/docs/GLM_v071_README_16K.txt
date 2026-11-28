GLM retro-tinyllm 16K - V0.7.1 szalagkeszlet (ZX81, 16K RAM)
==============================================================

MI VALTOZOTT V0.7.0 -> V0.7.1 (a visszajelzesed gyokerei)

1) A .tzx tenylegesen betolt EightyOne-ban (a "be se toltok" vegleges
   oka). AZ OK: a korabbi .tzx-em 0x10 (ROM) blokkokat irt - az
   EightyOne forrasaban (tzxEventTurbo.cpp) az EventTurbo, ami a 0x10-et
   jatszza le, SOHA nem tolti a TZXByte-ot, igy a ZX81 ROM $0356-os
   flashload-fogadoja ures kezet kapott. A ZX81-re mukodo TZX-ut
   kizarolag a 0x19 GENERAL blokk (EventGeneral -> TZXByte -> ROM).
   AZ UJ .tzx: minden blokk 0x19 GENERAL, pontosan az EightyOne sajat
   SaveGeneralBlock formatumaban (TOTP=0, TOTD=len*8, NPD=19, ASD=2,
   a ROM sajat 530/520 us bitparjaival). A .tap veglegesen kimarad:
   a LoadTAPFile ROM (0x10) blokkokat gyart - ugyanez a halott ut.
   A .t81 marad az elsodleges (az mar mindig is mukodott nalad).

2) A /topic lanc vegleges javítva: a topic-BETOLTES UTANI halal. AZ OK
   (szimben utasitasszintig bizonyitva): a $7A40-es altint-blob eleji
   'CALL $0207' (a SLOW-tanc) a ROM display-walkjat inditja el, ami a
   D-file sorait "vegrehajtja" Z80 kodkent. Boot utan ez artalmatlan
   (friss sorok), de a topic-csere utani 3. USR-ben a D-file a
   2. USR csomagolt chat-allapotat hordozza (0x00-0x3F ertekek = JR
   opcodok!) - a walk rajuk fut le, a PC $2-re esik, a ROM ujraindul
   es report C-t dob (a gep "nem lehet betolteni rola" tunete).
   A JAVITAS: a main() a tanc ELOTT C-bol kinullazza a D-file 3
   munkasorat (32 blank + $76 minden sor) - a pack_state() azutan
   visszairja az allapotot. Motor: 13127 -> 13242 B (+115 B; a modell
   $7417 -> $748A csuszott, meg mindig messze a $7A40-es ALTINT alatt).
   SZIM-BIZONYITEK (5200 frame, valodi ROM): 3 USR eletben -
   boot -> interju -> SZIA chat -> /topic 1 -> RT-TUDOM betoltve ->
   a topic-motor a CSILLAG valaszt generalja a $73db-$73e9-es
   generalo hurokban, REPORT-B=0, D-file intact, minSP $7f31.
   (REGI viselkedes: a 3. USR 2 frame alatt elpusztult PC=$2-nel.)

3) A "7786 bajt tul sok" kifogas a .p-kre: TENY-ellenorzes tortent -
   sem a naiv listing-seta, sem az E_LINE offszet-olvasasok nem
   adjak ki a 7786-ot (surplus=0 minden kepen, bajtosan). A szam a
   hasznalt kievo eszkoz sajat heurisztikajabol jon, nem a fajlbol.
   MELLekelve 14 db .p81 (nev-rekordos .p) - a ZEsarUX es mas
   eszkozok ebbe a formaba varjak, ugyanaz a kep, 8 bajtos
   nev-rekorddal az elejen.

A Fajlok
--------
GLM_v070_RT16_TOPICS.t81  - az EO81 szalag (ELSODLEGES): 15 blokk
                             (RTBOOT01 + RTBASE16 + 13 tema)
GLM_v070_RT16_TOPICS.tzx  - TZX v1.20, GENERAL (0x19) blokkokkal
                             (EightyOne-ban tenylegesen betolt)
*.t81                     - az egyes blokkok egyenkent (load "RT-TUDOM")
*.p                       - a blokkok .p formaban (a ZEsarUX es masok)
*.p81                     - nev-rekordos .p (a ZEsarUX .p81 utvonala)

Betoltes (EightyOne)
--------------------
- Tape -> beolvasod a GLM_v070_RT16_TOPICS.t81-t
- NEWLINE utan: J "" (LOAD "") - a boot betolti magat
- Az interju (ev/honap/nap/ido/nev/nem) utan ENTER a kapunal
- Chat: begepelsz, ENTER. Tema: /topic 1..13, ENTER (betolti a
  temablokkot es a tema-motor megy tovabb)
- A regota ismert kezeles marad: minden blokk AFTER E_LINE-ig tolt,
  a state sorok a D-file elso 3 soraban utaznak.
