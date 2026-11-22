v0.5.4 - MI VALTOZOTT
  - VEGLEGES KEPERNYO-JAVITAS: a kep most mar folyamatosan latszik.
    A v0.5.3-as villodzas igazai okara rajottunk: a kazettak betoltese
    kozben a ZX81 ROM szalagtoltoje szandekosan kikapcsolja a kepernyot
    (FAST mod), es a blokkok ujrainditasa utan senki nem kapcsolta
    vissza. Most minden blokk (boot, alap, temak) az indulasa
    pillanataban visszakapcsolja a folyamatos kepet (SLOW / NMI), ugy,
    hogy elotte a ROM rendszervaltozo alapcimet (IY=$4000) is
    biztosithatja.
  - Megmaradt a v0.5.3 javitasa is: DF_SZ=2 a $4022-en (a 22-23.
    sorok a ROM also kepernyoje), a program a 8-21. sorokba ir.
  - A fajlnevekben GLM es verzio: GLM_v054_...

EZZEL A FAJLLAL INDULJ (a tobbi opcionalis):
  1. Toltsd be: GLM_v054_RT16_TOPICS.t81  (ez az EGYETEMES kazetta:
     a boot automatikusan belepeldanyositja a profilodat, majd a
     beszelgeto motor valtokozik)
  2. A kepernyon a boot logojat es kerdeseket latod (ev, ido, nev, nem),
     az utana a beszelgetes indul: "tinyllm v0.5.4"
  - HA CSAK A BESZELGETEST AKAROD (boot nelkul, alap profilral):
    told be GLM_v054_rt16_base.p -t es RUN
  - Tisztan egy temahoz: GLM_v054_rt16_<tema>.p (pl. GLM_v054_rt16_movies.p)
  - NINCS kulon boot kazetta: a boot csak a RT16_TOPICS-on belul
    jatszik, onalloan nincs mit betoltenie (ezert nem "csinalt semmit").

retro-tinyllm 16K kiadas (v0.5.0) - gyorsinditas
=================================================

A v0.5 ujrairt inditast kapott: tobb reszben tolt be, es menet
kozben profil-es emlekadatokat gyujt. A lenyegi ujdonsagok:

  - BOOT blokk (RTBOOT01): logo, utmutato-kerdes, datum/ido/nem/
    nev bekkerese, majd lancolt toltodik az alap
  - EMLIKEK: a gepelt szavaidbol a program kis 8-bites
    esemenyeket keszit (faj a fejem / boldog vagyok / szomoru /
    faradt / szerelmes), a kovetkezo inditaskor visszaidizi
    ("emlek: faj a fejem (2 napja)"); 14 nap utan az emlek
    elmultnak szamit
  - MENTES KAZETTARA: /save parancs - visszatekered a szalagot az
    RTBASE16 blokk elejere, es a ROM sajat SAVE-e felulirja a
    blokkot az emlekekkel egyutt
  - GORDITHETO CHAT: SHIFT+5 felfele lapoz az archivumba (a
    munkasorok 3-7 taroljak az utolso 5 sort), SHIFT+0 vissza
  - HOFOK: SHIFT+1..4 (1 = hatrozott, 4 = vallalkozo valaszok)
  - GONDOLKODO KAPCSOLO: SHIFT+9 - a modell elszor neman
    megfogalmazza a valasz vazlatat (mohon modszammal), majd arra
    epitve generalja a lathato valaszt
  - 13 TEMATUDASTAR a kazettan: tudomany, eletvitel, hobbi,
    egeszseg, munka, muveszet, tanulas, irodalom, FILMEK,
    programozas, CSILLAGASZAT, ascii-rajz, vers/novella

AZ EGYFAJLOS KAZETTA (ez az ajánlott)
  rt16_topics.t81    mind a 15 blokk EGY fajlban, nevesen:
                     1. RTBOOT01  - az indito (logo + kerdesek)
                     2. RTBASE16  - a motor + a napi cseveges modell
                     3-15. RT-TUDOM, RT-ELETV, RT-HOBBI, RT-EGESZ,
                        RT-MUNKA, RT-MUVES, RT-TANUL, RT-IRODA,
                        RT-FILME, RT-PROGI, RT-ASTRO, RT-ASCII,
                        RT-VERSE
                     A blokkok sorrendje szabad: a LOAD nev szerint
                     keres (a ROM nev-szerinti egyeztest hasznal).

INDITAS
  1. toltsd be a rt16_topics.t81-t, es RUN - indul a BOOT blokk
     (TELJES kep: a kepernyofajl is benne van, friss gepen sem
     borul fel semmi; a tobbi blokk ROVID, hogy az allapotsorok
     es a gorgeto tul eletik a tematovaltast)
  1b. BARMELYIK .p fajlt betoltheted egyedul friss gepre: mind
     TELJES kep (kepernyofajllal), azonnal futnak
  2. a logo utan kerd: kell-e utmutato? (i/n)
  3. datum (ev/honap/nap), ido (ora/perc), neved, nemed (f/n)
  4. a boot magatol lancolja az RTBASE16-ot: kozszont, es ha van
     mentett emlek, kiirja ("emlek: orom (3 napja)")
  5. irj kisbetukkel, entert a sor vegen
  6. a kepernyo felso harmada munkaterulet: a 0-2. sor a csomagolt
     allapot (6 bitesen, csak szimpla karakterekkel), a 3-7. sor a
     gorgeto-archiveum - a D-file-ban NINCS 0x76-os koztes es
     nincs 0x40 feletti kod, azaz semmi nem fagyasztja a gepet

PARANCSOK
  /topic      temalista (a nevek szamaival)
  /topic n    atvaltas az n-edik temara: a motor a nevtablabol a
              blokk nevet bepokoli a 2-es sorba, a BASIC LOAD-ja
              nev szerint keresi a szalagon
  /save       emlekmentes: tekerj vissza az RTBASE16 blokk
              elejere, inditsd a felvetelt (REC+PLAY), es a ROM
              SAVE-e felulirja a blokkot; kovetkezo inditaskor a
              profil es az emlekek visszajonnek
  /new        uj beszelgetes (kontextus torlese)
  /quit       kilepes

A MODELLROL OOSZESEN
  A motor karakter-szintu: a kontextus az utolso 8 karakter, a
  valasz karakterrol karakterre epul (osztaly G, 8 dimenzios
  beagyazassal, 5 rejtett egyseggel, ~830 parameter, 1030 bajtos
  blob). Nem valasztanak le mondatokat - a magyar szo-halmany
  temahelyes szavakat ad, de nem ertelmez. A temakorpuszok a
  training/data mappaban levo 50-50 parosbeszelgetesek; a filmek
  temaja tenyleg a filmekrol beszel (cimek, mufajok), de komoly
  beszelgetest ne varj tole - 16 kilobajton egy karakter-szintu
  modell ennyi bir. A hofok es a gondolkodo-kapcsolo a mintavetelt
  valtoztatja: hidegen a modell legvaloszinubb folytatasa jot,
  melegen a szo-kinalat szetszortabb; a gondolkodo modban a vazlat
  a kontextusba kerul, a valasz arra epul.

HA "no model! dfile=XXXXX" JELENEK MEG
  A motor a modellt mar nem fix cimen keresi, hanem az RTLM
  szignaturara scannol le a memoriat $7AFF-tol - akkor sem tevedhet,
  ha a NEW vagy az emulator a kepernyofajlt masik cimre epiti ujra.
  Ha megis ezt latod, a dfile= utani szamot ird meg nekem: abbol
  latszik, hova tolodott a kepernyo.

v0.5.2 - A FAGYAS GYOKERE (ha a v0.5.0/v0.5.1 fajlok fagyasztottak)
  A v0.5.0-atol a kepfejlecben a verem-top szo ($7FFD) ket bajttal
  arrabb ult ($4028), mint kellene ($4026). Az emulator a quickloadnal
  EBBOLEallasztja be a gepi stack pointert - a rossz ertek ($0002) miatt
  a gep azonnal elhal, hibauzenet nelkul, minden fajlnal. Javitas: a
  kepfejlec bajtonként egyezik a te mukodo v0.4.5-os fajloddal
  (ellenorzes: appmake + v0.4.5 + demo.T81 haromas egyezes). Ezen felul
  ket motor-hiba javitva: a modellmeret-konstans (1970->1030, a /topic
  valtas ezert volt halott) es az allapot-visszaolvaso rutin (a DE
  regiszter felulirasaval a profil/program emlekek sosem toltozdtek
  vissza). A kepernyon most "tinyllm v0.5.2" latszik.

MiERT UJRAINDITOTT A v0.5.0 (ha ezt a verziot toltotted be)
  Az elso blokk (BOOT) rovid kep volt: a kepernyofajl ($7B00)
  nem volt benne a kepben, friss gepen a kijelzo-generaltor
  elszaladt az inicializalatlan memorian -> az emulator azonnal
  ujraindult. Javitas: a BOOT es minden .p TELJES kep (a
  kepernyofajl bent), a .t81-ben csak a boot teljes; a tobbi
  blokk rovid, az allapot meg marad.

MIERT NEM FAGY LE (a v0.5 ota garantaltan)
  - a D-file cellakba csak 0x00-0x3F ertek kerulhet (a csomagolo
    maszkolja), sorvegi 0x76 sosem
  - a bemeneti tabla ROM-tablak szerint javitva: vesszo, pont,
    per, csillag, zarojelek mind gepelhetoek; a SHIFT+2 visszatorlo
  - a 2-es es 3-as sor szamliterai $7E+5 bajtos FP-farkat
    hordoznak (a ROM futasi szkennerenek S-STK-DEC kovetelmenye),
    es a 3-as sorbeli "2" most tenyleg 2 ($1E), nem 4
  - a 3-as sor (SAVE) csak a /save utan, 2-es visszateresi
    ertekkor hajtodik vegre

HA CSAK EGY .p-T TOLTSD BE
  A rt16_base.p egyedul is mukodik (friss allapottal indul), de a
  profil-kerdesek es az emlekek a boot blokkon at erdemesek. Ha a
  szalagon nincs a kert nevu blokk, a ROM keresese BREAK-kel
  szakithato.

Spektrumra, C64-re, Amigara valo port: a motor mar most
platform-fuggetlen (rt_hal.h), a modell-osztalyok (A/B/C) keszen
allnak - de most ZX81-re ez a kiadas.
