# GLM3 — ZX81 Pico AI munkakönyvtár (GLM / z.ai vonal)

Ez a GLM-motoros ZX81 „Pico AI" fejlesztés **aktuális munkakönyvtára** (2026-11-27-től).
Számozás: a repo gyökerében lévő `GLM/` az első GLM-vonal (v0.1–v0.4, archív,
a CHATGPT1/GLM alatt megőrizve), a retro-tinyllm repóban futott a második
(v0.5.x-vonal), ez itt a **harmadik (GLM3)**: a v0.6.5-ös újrakiadástól v0.9.2-ig.
A többi modell munkaterületei (CHATGPT1, GROK1) nem keverendők ide.

## Könyvtárszerkezet

| Könyvtár | Tartalom |
|---|---|
| `HIBA_ES_JAVITAS_TORTENET.md` | Minden verzió: élő EO-próba eredménye, gyökérok, javítás, MD5 — **a re-vizsgálat kiindulópontja is** |
| `allapot/` | `STATE.md` (menetnapló + következő menet teendői), `STATE_v0.7.1_backup.md`, `PROJECTLOG.md` |
| `kazettak/` | Minden megőrzött .T81 (v0.6.5 → v0.9.2), `MD5SUMS.txt`, és az `elozmenyek_v0.7.0_elott/` (v0.7.0 előtti, névtelen kiadások) |
| `dokumentacio/` | Verziósways dokumentációk + változásnaplók (txt/docx/pdf) |
| `forraskod_csomagok/` | Verziósways forráscsomag .zip-ek |
| `szkriptek/` | Építő/ellenőrző/diagnosztika készlet (`build_v092.py`, `verify_v081.py`, `verify_v090.py`, `diff_cassets.py`, `trap_disasm.py`, `tape_blocks_report.py`, `basic_tokens_report.py`, …) |
| `motor_v090/` | A v0.9.0 C-motor forrása (z88dk/zsdcc vonal) |
| `eszkozok_regi_repo/` | A v0.1-dev repo eszközei node_modules és .git nélkül (`asm.py`, `build_t81.py`, `zx81sim.py`, `t81_validate.py`, `kbtrain.py`, …) |
| `refs/` | Hiteles ZX81 ROM (8K) + disassembly referenciák |
| `toolchain/` | `GLM_z88dk_2.5_zx81_toolchain.tar.gz` (z88dk 2.5 + SDCC 4.2, zx81 clib) |
| `TELJES_LELTAR.txt` | Minden fájl mérete + MD5-e (a másolás-integritás ellenőrzéséhez) |

## Aktuális állapot (2026-11-27)

- Legfrissebb kazetta: **`kazettak/GLM_PicoAI_v0.9.2_kazetta.T81`** (227 923 B,
  15 blokk, MD5 `6f5fc138e66e2ccdd52b982fffdb9003`). Élő EO-próbája függőben:
  A) fagyás/csík nélkül megjelenik-e a profilkérdés; B) az RTBASE16 lánc-betöltés
  után elindul-e a chat; C) /quit viselkedése.
- **A re-vizsgálat kiinduló fájlja: v0.6.5** — ettől a verziótól maradt a tünet
  változatlan a javítások ellenére (részletes bizonyítéklánc:
  `HIBA_ES_JAVITAS_TORTENET.md`).
- Kulcstény: az RTBASE16 lánc-betöltési lépés élőben (EightyOne-ban) **soha nem
  ment el hibátlanul** — az összes eddigi javítás ezt az utat célozza.
- A retro-tinyllm repó CI-hibája (`PICO_BOOT_R5.p: line1 is not REM`) a
  ChatGPT-vonal R5 builderének ügye — ebből a könyvtárból nem javítható (a
  `t81modular_build.py` + `pico_zx81_r5_check.py` forrása itt nincs meg).

## Újraépítés (v0.9.2)

```bash
tar xzf toolchain/GLM_z88dk_2.5_zx81_toolchain.tar.gz   # z88dk + zsdcc a /tmp alá
python3 szkriptek/build_v092.py                          # kazetta-építés + verifikáció
```

Megjegyzés: a szkriptek a fejlesztő-sandbox abszolút útvonalaira íródtak
(`/home/z/my-project/...`); más környezetben futtatáskor az útvonal-konstansokat
a szkript fejlécében módosítani kell. A toolchain kicsomagolási helye a
`build_v090_sdcc.sh` fejlécében van dokumentálva.
