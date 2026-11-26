# retro-tinyllm

Tiny int8 language models that chat on 8/16-bit home computers — with a
portable C89 engine, topic packs on disk and **turbo tape**, and two ways
of **learning on the device**.

Targets: **Amiga 500, Enterprise 128, TVC, Commodore 64, ZX Spectrum 48K,
C16/Plus4, ZX81 (16K)** — plus a desktop host simulator where everything
in this repo is actually built and tested.

```
              corpus (English, topic-sliced)
                        │  training/ (numpy, CPU)
                        ▼
              float model (.npz)
                        │  pack.py: calibration + int8
                        ▼
        model.bin  ◄──►  topic packs (retro / weather / animals / ...)
        (355k / 73k / 29k / 7.7k params, all one format)
                        │  disk FIRST (d64 / AmigaDOS / EXDOS)
                        │  tape variant (RT-turbo ~238 B/s, CRC-16)
                        ▼
        engine/ (C89, no heap, no floats, no libc deps)
                        │  rt_hal.h - 12 functions per machine
                        ▼
        ports/ (amiga, c64, zx, zx81, c16, enterprise, tvc, host)
```

## Which model file for which machine?

Model classes are in the `class` column of `models/manifest.cfg` and in
the file names; every port's Makefile pins the class it uses.

| file                    | class | size   | runs on                                    |
|-------------------------|-------|--------|--------------------------------------------|
| `models/base_d.bin`     | D     | 7.9 KB | ZX81 (16K pack), C16/Plus4 16K, and up     |
| `models/base_c.bin`     | C     | 28 KB  | C64, ZX Spectrum 48K, C16 with RAM, 16-bit |
| `models/classa_preview.bin` | A | 348 KB | Amiga 500, Enterprise 128, TVC 64+ (preview) |
| `models/topic_*.bin`    | C     | 28 KB  | same machines as `base_c` (C64 class)      |

Rules of thumb: the class letter says the SMALLEST machine the file runs
on (A > B > C > D), bigger machines can always run smaller classes, and
topic packs are class C — a 16K machine (ZX81, stock C16) physically
cannot hold a 28K pack, so it stays on its general model. `/info` in the
app prints the loaded model's dims; the cassette label
(`tools/build_tape.sh` → `dist/tape_label.txt`) lists sizes and counters.

## Screen-RAM scratch tier

The classic utility trick: the app never prints to some screen rows, so
those bytes double as a small non-volatile store for learned facts and
journal entries (third tier next to disk and tape). ZX Spectrum 2048 B
(top third), C64 / C16 200 B (top rows), ZX81 192 B (top 8 D-file rows
packed 6 bits per cell - only legal glyph codes). Design, numbers and
ROM-safety rules: docs/SCRATCH.md. The host can emulate every layout
for testing: `RT_SCRATCH=spec|c64|c16|zx81 RT_SCRATCH_FILE=scr.bin`.

## Quickstart (host simulator)

```sh
make host
./host/host_chat models/base_c.bin
```

```
you> hello!
> hi there! nice to meet you.
you> /load retro
topic loaded: vocab 45 ctx 10
you> what is a datasette?
> a datasette is a tape drive for saving programs.
you> /teach my name = ziggy
learned.
you> what is my name?
> your name is ziggy.
you> /finetune
fine-tuning on 1 facts...
you> my name is
> my name is ziggy.            <- the model itself, after tuning
```

`/help` lists everything: `/topics`, `/load <name|n>`, `/temp 0-6`,
`/greedy on|off`, `/sim amiga|c64|zx81` (feel the per-letter pace of the
real machines), `/mem`, `/save`, `/quit`.

Full transcripts: [docs/examples/](docs/examples/) — disk session,
teach/finetune session, tape session.

## The five project requirements, and where they live

| requirement | implementation |
|---|---|
| English-only to start | 45-char lowercase charset, deterministic English corpus |
| expandable when storage grows | all dims in the model header; same engine loads any class up to its `RT_MAX_*` limits — plus on-demand topic packs |
| topic packs + turbo tape where loading improves | `models/topic_*.bin`, `/load <n>`; RT-turbo protocol ~238 B/s (4x kernal tape), CRC-checked |
| disk primary, tape variant where disk exists | per-machine matrix in [docs/TAPE_DISK.md](docs/TAPE_DISK.md) |
| able to learn | fact memory (all ports) + int8 output-layer fine-tuning (host/Amiga) — [docs/LEARNING.md](docs/LEARNING.md) |

## Model classes

| class | fits | params | blob | speed (est.) |
|-------|------|--------|------|--------------|
| A | Amiga 500 (512 KB) | 355,549 | 348 KB | ~1-2 s/char @ 7 MHz 68000 |
| B | 64 KB machines, EP128 | 72,621 | 71 KB | ~4-6 s/char @ 3.5 MHz Z80 |
| C | C64, ZX Spectrum 48K | 28,581 | 28 KB | ~2-3 s/char @ 1 MHz 6502 |
| D | ZX81 16K, C16 | 7,725 | 8 KB | ~1 s/char @ 3.25 MHz Z80 |

Preview weights checked into `models/` are smoke-test quality (trained a
few minutes on a template corpus to validate the pipeline end to end);
float-vs-int8 next-char agreement is 97.5–100% and printed by the packer.
Real quality comes from training on a real corpus — the trainer, packer
and formats are ready for it (docs/ROADMAP.md).

## Repository layout

```
engine/    portable C89 core: model, int8 forward, sampling, chat,
           fact memory, fine-tuning, RT-turbo codec, shared REPL (rt_app)
host/      desktop simulator (files as "disk", .rtp pulse file as "tape")
ports/     amiga, c64, zxspectrum, zx81, c16, enterprise, tvc
training/  corpus generator, numpy trainer, int8 packer, int simulator
tools/     tapewriter (rtp/tap/tzx), d64 writer, modelinfo, build scripts
tests/     C test suite (golden bit-exactness, tape, memory, chat,
           fine-tuning) — run with ./tests/run_tests.sh
docs/      format specs, learning design, port matrix, roadmap
           + the original 24-week plan PDF (Hungarian)
```

## v0.2: topic router, clock, journal

- **General model first**: keywords route specialist questions to topic
  packs; AMBIGUOUS words ("star": astronomy or poetry?) get a clarifying
  question before anything is loaded (`/back` returns to the base model).
  [docs/ROUTER.md](docs/ROUTER.md)
- **Tape, the 1980s way**: block 0 of the topic tape is the manifest, and
  `tapewriter --label` prints a cassette label with COUNTER values - wind
  to the number, `/load <block>`.
- **Clock + journal**: the app asks for date/time at boot, knows weekday,
  part of day and season, and keeps a dated journal of the little things
  you mention - saved when you quit, recalled on the next boot
  ("on monday you said: my head hurts").

## Engineering notes worth reading

- **Bit-exact golden test**: the numpy integer simulator and the C engine
  produce identical logits on 100 vectors per packed model — trainer,
  packer and target code can never silently disagree
  (`tests/test_golden.c`).
- **Q7 everywhere + per-row weight shifts**: the bias lane feeds a
  constant 128 (= 1.0), so biases quantize on the same grid as weights
  and there are no special cases in the matmul
  ([BINARY_FORMAT.md](docs/BINARY_FORMAT.md)).
- **RT-turbo**: disjoint pulse bands (pilot/sync/0/1), a two-pulse sync,
  CRC-16 per block, and a hard timeout on silence — round-trip, bit-flip
  rejection and ±16% tape wobble are all covered by tests.

## Building the models yourself

```sh
python3 training/corpus_gen.py                 # deterministic corpus
python3 training/train.py --class C --data training/data/base.txt \
    --out runs/base_c --steps 20000            # ~1 min on a laptop CPU
python3 training/pack.py --npz runs/base_c.npz --bin models/base_c.bin \
    --golden tests/golden_c.txt --finetune-ok
```

Or everything at once: `tools/build_models.sh` (corpus → all classes +
topic packs, ~3 minutes).

## Status

See [docs/STATUS.md](docs/STATUS.md). Short version: the engine and the
whole training/packing pipeline are **built and tested** on the host;
machine ports are **source/skeleton** — the C64 and Amiga HALs are
complete but uncompiled here (no cc65/vbcc in the sandbox), the Z80 ports
need toolchain + hardware validation.

## License

MIT — see [LICENSE](LICENSE).

---

## Magyar összefoglaló (HU)

A projekt célja: aprő, int8-ban kvantált nyelvi modellek beszélgetése
retro gépeken (Amiga 500, Enterprise 128, TVC, C64, ZX Spectrum, C16,
ZX81), egyetlen hordozható C89 motorral.

- **Angolul indul** (45 karakteres charset), a formátumban él a charset,
  így később magyar változat is gyártható.
- **Bővíthető tárolással**: a modell méretei a fájlfejlécben élnek —
  nagyobb tároló = nagyobb modell vagy több témapakk, újrafordítás
  nélkül. Témák: retro, időjárás, állatok, üdvözlés, számok, viccek.
- **Témapakkok + turbo tape**: ahol a betöltés bővíthető (szalag, nagyobb
  lemez), a témák külön-külön tölthetők; az RT-turbo protokoll ~4x
  gyorsabb a C64 kernal magnál, blokkonként CRC-16-gal.
- **Lemez az elsődleges forrás**, ahol létezik lemez — és mindenhol van
  szalagos (vagy soros) változat is.
- **Tanulás két úton**: `/teach` tények (minden gépen, memory.bin-ben
  megmaradnak) és `/finetune` int8 súly-finomhangolás (host/Amiga) —
  utóbbival a modell MAGA kezdi el mondani a tanított tényt.
- A motor, a tanítási csővezeték és a tesztek a hoston **futnak és
  zöldek** (bitpontos golden teszt a numpy szimulátor és a C motor
  között); a gépportok forrásként/skeletonként készültek el (lásd
  docs/STATUS.md).
- Az eredeti, 24 hetes fejlesztési terv (magyar PDF): docs/plan/.
