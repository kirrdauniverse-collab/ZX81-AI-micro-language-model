# Model binary format (model.bin, version 2)

A model is ONE self-contained blob: header + int8 tensors + checksum.
Because all dimensions live in the header, the same engine binary loads
any class up to its compile-time limits (`RT_MAX_*` in `engine/rt_config.h`)
- that is the whole "expandable when storage grows" story: a bigger file
just works, no recompile.

```
off  size  field
 0     4   magic "RTLM"
 4     2   u16 version        (2)
 6     2   u16 flags          bit0: fine-tuning allowed
 8     2   u16 vocab          (45, fixed charset)
10     2   u16 ctx            context chars
12     2   u16 emb            embedding width
14     2   u16 h1             hidden 1 width
16     2   u16 h2             hidden 2 width (0 = single hidden layer)
18     2   u16 sh_e           reserved (= 7, embeddings are Q7)
20     2   u16 sh1            reserved (0, per-row shifts are used)
22     2   u16 sh2            reserved (0)
24     2   u16 sh_o           reserved (0)
26     2   u16 temp_sh        default sampling temperature (right shift)
28    64   char charset[64]   NUL-padded; must equal RT_CHARSET exactly
92    ...   tensors (row-major, bias lane LAST in every row):
           emb  [vocab][emb]          int8  Q7 activations
           sh1  [h1]                  u8    per-row weight shifts
           w1   [h1][ctx*emb + 1]     int8  weights + bias lane
           sh2  [h2]                  u8    (absent when h2 == 0)
           w2   [h2][h1 + 1]          int8
           sho  [vocab]               u8
           wo   [vocab][h_last + 1]   int8
end     2   u16 additive checksum of every preceding byte
```

## Quantization (v2): Q7 activations + per-row weight shifts

Everything on the inference side is Q7 fixed point: `real = value / 128`.

- Embeddings are stored directly as Q7 int8 (scaled at pack time so the
  largest value uses the int8 range).
- Every weight row carries its own power-of-two shift `s` (u8 before the
  matrix): the row stores `round(w * 2^s)`.
- A matmul row computes

  ```
  acc = sum(x[i] * q[i]) + 128 * q_bias      // int32
  y   = acc >> s                             // back to Q7
  ```

  The bias lane input is the constant 128 (= 1.0 in Q7), so biases
  quantize on exactly the same grid as ordinary weights - no special
  cases anywhere.
- Hidden activations are ReLU'd and saturated into `[0, 127]`; logits
  stay int32 in Q7 scale.
- `pack.py` picks the embedding scale and per-row shifts by calibration
  over real text, and compensates the next layer's columns for every
  row scaling, so the int8 network computes the same function as the
  float network (97.5-100% next-char agreement on the checked-in models,
  printed at pack time).

Why per-row: hidden activations in a trained model have very different
dynamic ranges per unit. Per-row shifts give every row the full int8
range, which measurably beats per-tensor scaling on models this small.

## Classes

| class | machine fit | ctx | emb | h1 | h2 | params | blob |
|-------|-------------|-----|-----|----|----|--------|------|
| A | Amiga 500 (512 KB) | 12 | 48 | 448 | 192 | 355,549 | ~348 KB |
| B | 64 KB machines, EP128 | 10 | 32 | 160 | 96 | 72,621 | ~71 KB |
| C | C64, ZX Spectrum 48K | 10 | 24 | 96 | 0 | 28,581 | ~28 KB |
| D | ZX81 16K, C16 | 8 | 16 | 40 | 0 | 7,725 | ~8 KB |

Inspect any blob with `python3 tools/modelinfo.py <file>`.
