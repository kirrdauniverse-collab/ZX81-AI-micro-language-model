# Tape and disk strategy

Project rule (from the requirements): **wherever a disk option exists,
disk is the primary model source, and a tape variant must also exist.**
Machines with no realistic disk stay tape-only.

## Per-machine matrix (v0.1)

| machine | primary | tape variant | notes |
|---------|---------|--------------|-------|
| Amiga 500 | AmigaDOS disk | serial stream (doc'd) | no tape standard; null-modem from a PC |
| Enterprise 128 | EXDOS disk | EXOS tape channel | skeleton HAL, verify EXOS numbers |
| TVC | TDC disk (rare) | ROM cassette | skeleton HAL |
| C64 | 1541 (.d64) | kernal tape + RT-turbo | d64 via tools/d64write.py |
| ZX Spectrum | +3/TR-DOS/+D where present | RT-turbo (.tzx) / ROM blocks (.tap) | |
| C16 / Plus4 | 1551 (rare) | kernal tape | |
| ZX81 (16K) | - | ZX81 ROM tape | smallest class only |

## Topic packs (the "expandable storage" story)

`models/topic_*.bin` are standalone Class-C models fine-tuned per topic
(retro, weather, animals, greetings, numbers, jokes). They are loaded on
demand:

- disk: `/load retro` (or pick from `/topics`) - random access
- tape: `/load 2` - the n-th block on the cassette, in the order the tape
  was written (see `tools/build_tape.sh`)

Because every blob is self-describing (see BINARY_FORMAT.md), "more
storage" simply means: more/bigger topic packs and/or a bigger base model
(Class A on an expanded Amiga). No engine change, no recompile.

## RT-turbo tape protocol

One block = one file. Pulse widths in microseconds, one pulse per
half-wave:

```
pilot : 64 pulses of 1500us  (decoder needs >= 24 in band 1125..1875)
sync  : TWO pulses of 900us  (band 720..1080)
header: u16 payload length   (value-MSB-first bits)
data  : payload bytes, MSB first: 0 = 350us (262..437), 1 = 700us (525..875)
crc   : u16 CRC-16/CCITT     (poly 0x1021, init 0xFFFF), value-MSB-first
end   : one pulse of 3000us  (optional, decoder ignores it)
```

- Throughput ~1900 bit/s (~238 B/s): a 28 KB Class C model loads in about
  2 minutes, the 8 KB Class D in about half a minute.
- The pilot/sync bands and the two bit bands are pairwise disjoint, so no
  pulse can be misread at any drift within tolerance. `pulse() == 0` means
  "no edge in time" and aborts every state - silence can never hang the
  loader.
- The decoder (`engine/rt_tape.c`) is plain sequential C over a
  `rt_pulse_fn` callback: the host simulator feeds it from a file, a Z80
  port counts loop iterations between cassette-port edges (see
  `ports/zxspectrum/turbo_zx.s`), other machines use whatever timer they
  have. Round-trip, CRC rejection and noise tolerance are covered by
  `tests/test_tape.c` with +-16% simulated tape wobble.

## Tools

```
# pulse file for the host simulator + real-machine HALs:
python3 tools/tapewriter.py --out tape.rtp models/topic_retro.bin models/topic_weather.bin

# ZX Spectrum images:
python3 tools/tapewriter.py --tzx turbo.tzx ...   # RT-turbo blocks
python3 tools/tapewriter.py --zx rom.tap ...      # plain ROM blocks (fallback)

# Commodore 1541 disk:
python3 tools/d64write.py --out models_c64.d64 models/base_c.bin models/topic_retro.bin
```

`tools/build_tape.sh` puts a full set together (engine binary + base +
topics) and drops the results in `dist/`.
