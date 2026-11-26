RETRO-TINYLLM 16K ZX81 (Z80 GEPI KOD + D_FILE AI RAM)
======================================================

1. MIT TARTALMAZ A CSOMAG?
--------------------------
- tinyllm_16k.p              : Fo program (16K ZX81 .P fajl, tiszta Z80 gepi kod + alap adatbazis).
- tinyllm_16k_demo_receptek.p: Teszt verzio, amiben a RECEPTEK tema mar be van toltve a $4900 pufferbe.
- topic_receptek.bin         : Kazettarol / fajlbol betoltheto RECEPTEK int8 sulyozott temamodul.
- topic_csil_tud.bin         : CSILLAG -> 1: TUDOMANY temamodul.
- topic_csil_iro.bin         : CSILLAG -> 2: IRODALOM temamodul.
- topic_csil_muv.bin         : CSILLAG -> 3: MUVESZET temamodul.
- build_tinyllm_16k.py       : A teljes Z80 assembler + generato forraskod (sajat temak gyartasahoz).

2. MEMORIATERKEP (16K = $4000 - $7FFF)
--------------------------------------
- $4009 - $407C : ZX81 Rendszervaltozok (D_FILE = $7B00, NXTLIN = $407D)
- $407D - $408D : 10 RAND USR 16526 ($408E) inditosor
- $408E - $4580 : Z80 AI Motor + Alap Tudasbazis (~1.25 KB!)
- $4900 - $7AFF : TOPIC_RAM (12.5 KB szabad puffer a kazettas temaknak)
- $7B00 - $7E18 : Kibontott D_FILE (793 bajt)
    * 1-8. sor  ($7B01-$7C08): AI Munkaterulet & C64-szeru elo vizualizacio
      - 3. sor ($7B43): Bemeneti puffer (ide irsz)
      - 4. sor ($7B64): Token tukor (inverz karakterek)
      - 5. sor ($7B85): Int8 neuralis akkumulator savok
      - 6. sor ($7BA6): Neuralis busz animacio
      - 7. sor ($7BC7): Allapotvaltozok (100% $76-mentes!)
    * 9-24. sor ($7C09-$7E18): 16 soros gorgetheto Chat Konzol

3. HOGYAN MUKODIK A SAJAT EMULATORODBAN?
----------------------------------------
A) Inditas:
   Toltsd be a `tinyllm_16k.p` fajlt 16K RAM beallitassal. Ha nem indul el automatikusan
   (autorun), add ki a `RAND USR 16526` parancsot.

B) Alap parbeszed (kazetta nelkul):
   - Irj be: SZIA, HELLO, KI VAGY, IDO, KOSZI, HELP -> Azonnal valaszol az alap magbol.

C) Automatikus temavaltas ("FOZ" / "RECEPT"):
   - Irj be barmit, amiben benne van a FOZ vagy RECEPT szo (pl. "MIT FOZZEK MA?").
   - Az AI felismeri a kulcsszot, es keri a `RECEPTEK` kazettat!
   - A sajat emulatorodban ket modon adhatod oda neki a temat:
     1. Kazetta emulacio: Inditsd el a `topic_receptek.bin` nyers bajtfolyamat a ZX81 szalagbemeneten
        (a ROM $0347 rutin olvassa be a $4900 cimre), majd nyomj ENTER-t.
     2. Direkt RAM injektalas (teszteleshez): Masold be a `topic_receptek.bin` tartalmat
        a $4900 (18688) memóriacimre, es nyomj ENTER-t! A kod ellenorzi a "TPK1RECEPTEK"
        fejlecet a $4900 cimen, es azonnal aktivalja!

D) Keteertelmuseg feloldasa ("CSILLAG"):
   - Irj be barmit, amiben szerepel a CSILLAG szo (pl. "MI AZ A CSILLAG?").
   - Az AI megkerdezi: `1:TUDOMANY 2:IRODALOM 3:MUVESZET`
   - Nyomd meg az `1`, `2` vagy `3` gombot + ENTER-t, es a megfelelo `topic_csil_*.bin`
     fajlt keri be!
