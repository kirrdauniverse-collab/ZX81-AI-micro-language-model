#!/usr/bin/env python3
"""rowsum_check.py - verify that every matmul row of every shipped model
fits a signed 16-bit accumulator (the v0.7.2 16-bit MAC precondition).

For each weight row, the worst-case accumulator is
    sum |w_i| * 127   (+ |bias| * 128 for the bias lane)
because inputs are Q7 activations in [0,127] (hidden) or [-128,127]
(embeddings). If that worst case overflows 32767, the 16-bit int path
could saturate differently than the old 32-bit path for THAT model.
Prints the max per blob; exits 1 if any row exceeds the budget.
"""
import os
import struct
import sys

MODELS = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "..", "models16_v05")
Z32 = 32767


def main():
    worst = 0
    bad = 0
    for fn in sorted(os.listdir(MODELS)):
        if not fn.endswith(".bin"):
            continue
        m = open(os.path.join(MODELS, fn), "rb").read()
        vocab, ctx, emb, h1, h2 = struct.unpack("<5H", m[8:18])
        # embedding inputs are int8 [-128,127]; hidden inputs are [0,127]
        rows = []
        # w1: inputs = embeddings (int8), bias lane input 128
        base = 92 + vocab * emb + h1
        for o in range(h1):
            row = m[base + o * (ctx * emb + 1):
                    base + (o + 1) * (ctx * emb + 1)]
            rows.append((row[:-1], row[-1], 128, "w1"))
        # w2 (if any): inputs = h1 activations [0,127]
        if h2:
            base = 92 + vocab * emb + h1 + h1 * (ctx * emb + 1) + h2
            for o in range(h2):
                row = m[base + o * (h1 + 1): base + (o + 1) * (h1 + 1)]
                rows.append((row[:-1], row[-1], 127, "w2"))
        # wo: inputs = last hidden activations [0,127]
        last = h2 or h1
        base = (92 + vocab * emb + h1 + h1 * (ctx * emb + 1) +
                (h2 + h2 * (h1 + 1) if h2 else 0) + vocab)
        for o in range(vocab):
            row = m[base + o * (last + 1): base + (o + 1) * (last + 1)]
            rows.append((row[:-1], row[-1], 127, "wo"))
        mrow = 0
        for w, bias, xin, tag in rows:
            # worst case: max positive contribution
            hi = sum((127 if x >= 0 else -128) * x for x in w) + \
                 128 * (bias if bias >= 0 else -bias)
            lo = sum((-128 if x >= 0 else 127) * x for x in w) - \
                 128 * abs(bias)
            mrow = max(mrow, abs(hi), abs(lo))
        worst = max(worst, mrow)
        ok = "OK " if mrow <= Z32 else "OVERFLOW"
        if mrow > Z32:
            bad += 1
        print(f"{fn:24} vocab={vocab} ctx={ctx} emb={emb} h1={h1} h2={h2}"
              f"  worst row sum {mrow:7d} / {Z32}  {ok}")
    print(f"\nmax worst-case row sum across all models: {worst}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
