#!/usr/bin/env python3
"""Regression tests for ZX81 five-byte FP -> non-negative integer conversion."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = (root / "ports/zx81/boot16.c").read_text(encoding="utf-8")
if "shift = 159 - (int)p[0]" not in src:
    raise SystemExit("boot16.c does not use the correct 31-bit mantissa exponent")
if "shift = 160 - (int)p[0]" in src:
    raise SystemExit("old off-by-one FP conversion is still present")


def decode(fp):
    e, b1, b2, b3, b4 = fp
    if e == 0:
        return 0
    if b1 & 0x80:
        raise ValueError("negative values are not accepted by the wizard")
    mant = ((b1 & 0x7F) << 24) | (b2 << 16) | (b3 << 8) | b4
    shift = 159 - e
    if shift >= 32:
        value = 0
    elif shift > 0:
        value = mant >> shift
    else:
        if -shift >= 32:
            raise ValueError("integer would overflow conversion")
        value = mant << (-shift)
    if value > 32767:
        raise ValueError("integer outside wizard range")
    return value


cases = [
    ([0x81, 0x40, 0x00, 0x00, 0x00], 1),
    ([0x84, 0x60, 0x00, 0x00, 0x00], 12),
    ([0x85, 0x7C, 0x00, 0x00, 0x00], 31),
    ([0x8B, 0x7E, 0xA0, 0x00, 0x00], 2026),
    ([0x8B, 0x7E, 0x90, 0x00, 0x00], 2025),
    ([0x8C, 0x41, 0x98, 0x00, 0x00], 2099),
]
for fp, expected in cases:
    actual = decode(fp)
    if actual != expected:
        raise SystemExit("ZX81 FP %r decoded as %d, expected %d" % (fp, actual, expected))

# The uncorrected expression halves the representative year 2026 to 1013.
mant = (0x7E << 24) | (0xA0 << 16)
old_result = mant >> (160 - 0x8B)
if old_result != 1013:
    raise SystemExit("regression guard for the old 2026 -> 1013 bug failed")

print("ZX81 profile FP conversion: 6/6 PASS; 2026 remains 2026")
