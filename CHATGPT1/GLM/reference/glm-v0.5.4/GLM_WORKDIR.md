# GLM munkakonyvtar-jelzes (v0.5.4)

Ebben a kozos projektben a **GLM (z.ai) munkajarol** ez a jeloles ad
tajekoztatast, hogy mely reszek tartzoznak hozza:

## GLM munkak (v0.5.4, "GLM_v054_" fajlnev-elotag)
- `ports/zx81/` - a teljes ZX81 16k port (hal, app, boot, crt0, build)
- `ports/zx81/16k/` - a forditott kazettak: `GLM_v054_RT16_TOPICS.t81`
  (a program), `GLM_v054_rt16_*.p/.t81` (motor + temak)
- `tools/` - T81 build/check/sim szkriptek
- `models16_v05/`, `training/` - a 13 tema modelljei es tanitasuk
- `docs/` - STATUS.md, T81_FORMAT.md

## Nem GLM munkak - at nem nyulunk
- `ports/amiga, c16, c64, enterprise, tvc, zxspectrum` - mas modellek
  portjai, barmilyen modositas es atnevezes nelkul
- `ZX81 AI mukodo alapok.zip` - a felhasznalo sajat reference feltoltese

## Verzio
Minden GLM-kazetta fajlnevben hordozza a verziot (`GLM_v054_...`);
a motor kepernyo-bannerje is a verziozat (`tinyllm v0.5.4`).
