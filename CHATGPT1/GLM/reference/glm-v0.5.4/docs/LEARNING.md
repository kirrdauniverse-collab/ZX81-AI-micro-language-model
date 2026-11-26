# On-device learning

The requirement: "if possible, it should be able to learn". For an 8/16-bit
machine running a <400k parameter int8 model, "learning" splits into two
mechanisms that are BOTH implemented in v0.1 - they complement each other.

## Mechanism 1: teachable fact memory (every machine)

`/teach key = value` stores a fact (max 16 in RAM, 24-byte key, 64-byte
value, charset-sanitized). When a later question shares a key word, the
answer is composed deterministically from the fact:

```
you> /teach my name = ziggy
learned.
you> what is my name?
> your name is ziggy.
```

- Works on every port including the ZX81 (pure RAM + string ops).
- `/save` persists memory.bin to disk or tape (RTM1 format: magic, count,
  fixed-size records); it is reloaded at boot.
- `/mem` lists facts, `/forget n` deletes one, re-teaching overwrites.

This is retrieval ("RAG on a budget"): it is not weight learning, but it
is real, durable, on-device knowledge acquisition - and it is what makes
the machine feel teachable in daily use.

## Mechanism 2: output-layer fine-tuning (host, Amiga)

`/finetune` (enabled where `RT_ENABLE_FINETUNE=1`) nudges the model's own
int8 weights so it starts REPRODUCING the taught facts from its
parameters:

```
you> /teach favorite computer = amiga
you> /finetune
fine-tuning on 1 facts...
done: 47/84 chars predicted correctly on the last sweep.
you> my favorite computer is the
> my favorite computer is amiga.        <- the MODEL says it now
```

How it works (`engine/rt_finetune.c`):

- The taught facts are rendered as text ("key is value."), repeated 3x so
  the positive gradient outweighs the "don't say this letter here"
  gradient from every other position.
- For each character position: normal forward pass, softmax error against
  the actual next character, accumulate `g[t][j] += (-err[t]) * h[j]`
  into int32 buffers (the bias lane accumulates `-err * 128`, matching
  the inference-time constant exactly).
- After each sweep, every output weight moves by `g >> 14`, clamped to
  +-127. Accumulating per sweep (instead of per position) is essential:
  per-position updates mostly round to 0 in int8 and occasionally jump.
- Hidden layers stay frozen - that is what keeps the memory footprint at
  `vocab * (h_last+1) * 4` bytes (17 KB for Class C) and makes the sweep
  take seconds, not minutes, on a 68000.
- `/save` after `/finetune` writes `model_finetuned.bin` with a fixed
  checksum, so the tuned model can be copied back over `model.bin`.

Honest limits (documented, not hidden): this is int8 online SGD - it
memorizes, it does not generalize; teach too many conflicting facts and
replies degrade. `/finetune` is explicit and confirmable on purpose.

## What v0.1 does NOT do

Full backprop or in-context few-shot learning at 8-12 chars of context.
The 24-week plan (docs/ROADMAP.md, Hungarian PDF in docs/plan/) puts
real fine-tuning experiments in phase 5-6, on the Amiga first.
