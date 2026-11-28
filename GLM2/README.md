# GLM2 — GLM retro-tinyllm 16K munkakönyvtár (2. GLM projekt)

Ez a GLM modell elkülönített munkakönyvtára a ZX81 16K mikro-nyelvimodell-projektben
("retro-tinyllm" vonal, v0.5.5 → v0.7.3). 2026-11-27-én jött létre rendrakás céljából:
több modell (ChatGPT, GROK, GLM) dolgozott párhuzamosan ugyanazon a repón, egymásról
nem tudva, ezért a projektek összeakadtak (a régi `GLM/` könyvtárban más modell által
elhelyezett TÖRÖLHETŐ.TXT is keletkezett). Az ezzel a fájlval együtt hozott tartalom a
GLM saját munkájának konszolidálása — a legfrissebb (v0.7.3-as) forrásokkal és naplókkal.

## A könyvtár tartalma

```
GLM2/
├── README.md            ez az útmutató
├── HIBA_NAPLO.md        konszolidált hiba- és javítási napló (v0.4.x → v0.7.3b)
├── VERZIO_TORTENET.md   verziótáblázat + a keresett T81 (a "nem változott" tünet oka)
├── SZALAG_LELTAR.md     minden kazetta mérete, MD5/SHA256 ujjlenyomata és lelőhelye
├── docs/                v0.7.1/v0.7.2/v0.7.3 dokumentumok (ALLAPOT, ATADAS, README-k, szim-eredmény)
├── src/engine/          gépfüggetlen motor (rt_*.c/.h) — v0.7.3 állapot
├── src/ports/zx81/      ZX81 port: app16.c (v0.7.3 javítással), boot16.c (v0.7.3 banner),
│                        hal_zx81.c, main_zx81.c, scratch_stub.c, rtframe.asm,
│                        build16k/zx81rt_crt0.asm
├── tools/               t81build16.py, t81check16_v05.py, modelinfo.py, patch_v073.py,
│                        patch_banner_v073.py, z80dis.py, tvanalyze.py, photoanalyze.py,
│                        rowsum_check.py, altint_v066.bin (+ .b64 archívum)
├── scripts/             build_v071.sh, build_v072.sh, sim_v072b.js
├── tape/                kulcskazetták: TOPICS_v072.t81, TOPICS_v073.t81 (AKTUÁLIS), TOPICS.tzx
├── reference/tapes/     GLM_v065_RT16_TOPICS.t81 — a v0.6.5/66 korszak kazettája
└── models16_v05/        a 14 modell-blob (base + 13 téma, 1970 B, "RTLM" mágia)
```

## Szabályok (a káosz megismétlődése ellen)

- Az ezutáni GLM-munka KIZÁRÓLAG itt zajlik; más modell munkakönyvtárát nem bántom.
- A régi `GLM/` könyvtár a GLM2 létrejötte után törölhető (a benne lévő
  TÖRÖLHETŐ.TXT ezt jelzi). A régi `GLM/` szöveges tartalmának teljes archív
  másolata megmarad a `CHATGPT1/GLM/` alatt (a ChatGPT 2026-11-26-i archiválásából).
- Egy T81 fájlban marad a többszörös betöltésű munkafolyamat (BOOT + BASE + 13 téma).
- Minden ismert-jó regressziót megőrzök, egyszerre csak egy réteget változtatok.
- A szim zöldje NEM bizonyíték a display/NMI úton (a szim NMI-modellje a valódi
  generátor HALT-fegyverzését nem modellezi — l. HIBA_NAPLO.md).
- A kazettafájl verzióját a fájlnévbe is beleírjuk (`_v072`, `_v073`) — a v0.7.3b
  lecke: az azonos nevű szállítások keveredtek a user gépén.

## Bináris fájlok (byte-hűen, a repóban)

A szalagok és a modell-blobok byte-hűen a repóban élnek (a `git push` bináris
fájlt is hűen visz; csak a GitHub webes/API-s szerkesztője korlátozódik UTF-8-re):

- `tape/GLM_v070_RT16_TOPICS_v072.t81` (MD5 cfe9b281…),
  `tape/GLM_v070_RT16_TOPICS_v073.t81` (MD5 48077afe… — AZ AKTUÁLIS),
  `tape/GLM_v070_RT16_TOPICS.tzx` (v0.7.1, 0x19 GENERAL blokkok).
- `reference/tapes/GLM_v065_RT16_TOPICS.t81` (MD5 1162414c…) — a vizsgálat
  tárgya: ebben a motorban még NINCS display-kikapcsolás.
- `models16_v05/` — a 14 modell-blob (base + 13 téma).
- `tools/altint_v066.bin` — a 181 B-os lánc-blob közvetlenül (a `.bin.b64`
  archívum továbbra is megvan mellette).
- A teljes v0.7.1 szalagkészlet (14×.t81/.p/.p81) a GLM_retro_tinyllm_v071_atadas.zip-ben
  él (a user gépén); ujjlenyomataik a SZALAG_LELTAR.md-ben rögzítve.

## Újraépítés

```sh
sh scripts/build_v072.sh            # z88dk 2.2: rtchat16 (13384 B) + rboot16 (5085 B)
python3 tools/patch_v073.py <rtchat16>   # a 17 bájtos $4CC3 javítás (v0.7.3)
python3 tools/t81build16.py <rtchat16> <rboot16> <models16_v05> <16k_ki_dir>
python3 tools/t81check16_v05.py     # 16 OK-t vár
SIMTAPE=tape/GLM_v070_RT16_TOPICS_v073.t81 node scripts/sim_v072b.js 2900
```

Külső szükségletek (NEM részei enket a könyvtárnak): z88dk 2.2, a `models16_v05`
modell-blobok, a szimulátor (EightyOne-magú TS-emulátor + zx81.rom — a v071 ZIP
`scripts/simenv/` mappája), Node 22, Python 3, EightyOne (valódi gépi teszt).

## Hol tart a munka (2026-11-27, v0.7.3b)

Az utolsó leszállított kazetta a v0.7.3 (MD5 48077afe25c2cd7ab2056fbe2cbb1f60,
banner: "retro-tinyllm 16k v0.7.3") — szimben tiszta: a SZIA válasz után a kép
visszatér (VIDEO RECOVERY f2520), 3 USR életben, REPORT-B=0.

A v0.7.3b tisztázás: a "nem lépett tovább" tünet oka RÉGI (v0.6.x-korú) szalag
betöltése volt — a fájlnevek minden szállításban ugyanazok voltak, és a banner
v0.6.2 óta nem frissült, ezért a régi és új szalag a TV-ről megkülönböztethetetlen
volt. A javítás óta a banner a valódi verziót írja; a teszt-eljárás a
VERZIO_TORTENET.md-ben. Részletes történet: HIBA_NAPLO.md.
