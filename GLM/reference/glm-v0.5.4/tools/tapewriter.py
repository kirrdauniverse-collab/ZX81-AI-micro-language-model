#!/usr/bin/env python3
"""tapewriter.py - build tape images for every machine.

Outputs:
  *.rtp   raw pulse stream (u16 LE, microseconds) - host simulator tape and
          any port whose HAL counts edges; this IS the RT-turbo protocol.
  *.tap   ZX Spectrum standard ROM blocks (pilot 8063/3223, sync 667/735,
          data 855/1710 T-states, MSB first, XOR parity). Loadable by the
          plain ROM loader - no custom code needed on the machine.
  *.tzx   TZX v1.13 with a turbo block per file (pulse timings = RT-turbo
          converted to T-states), for emulators and turbo-capable loaders.

Block order on tape = topic order given on the command line; the engine
selects them by index (/load 0, /load 1, ...) or loads them in sequence.

usage: tapewriter.py --out tape.rtp [--zx tape.tap] [--tzx tape.tzx]
                     file1.bin [file2.bin ...]
"""
import argparse
import struct
import sys

# RT-turbo pulse widths in microseconds (must match engine/rt_tape.h)
PILOT_US = 1500
SYNC_US = 900
ZERO_US = 350
ONE_US = 700
END_US = 3000

# ZX Spectrum ROM loader timings in T-states (3.5 MHz -> 1 T = 0.28422 us)
ZX_PILOT_T = 2168
ZX_SYNC1_T = 667
ZX_SYNC2_T = 735
ZX_ZERO_T = 855
ZX_ONE_T = 1710
ZX_HEADER_PILOTS = 8063
ZX_DATA_PILOTS = 3223
US_PER_T = 1.0 / 3.5  # us per T-state


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)
            crc &= 0xFFFF
    return crc


def rt_block_pulses(payload: bytes):
    """RT-turbo block -> list of pulse widths in microseconds.

    Field bit order matches rt_tape.c exactly: multi-byte fields (length,
    CRC) are sent MOST SIGNIFICANT BIT OF THE VALUE first, data bytes
    MSB-first per byte. The sync is a PAIR of 900us pulses."""
    pulses = [PILOT_US] * 64 + [SYNC_US, SYNC_US]
    bits = []

    def put_u16(v):
        for i in range(15, -1, -1):
            bits.append((v >> i) & 1)

    put_u16(len(payload))
    for byte in payload:
        for bit in range(7, -1, -1):
            bits.append((byte >> bit) & 1)
    put_u16(crc16_ccitt(payload))
    pulses += [ONE_US if b else ZERO_US for b in bits]
    pulses.append(END_US)
    return pulses


def write_rtp(path, files):
    with open(path, "wb") as f:
        for name in files:
            with open(name, "rb") as g:
                data = g.read()
            for us in rt_block_pulses(data):
                f.write(struct.pack("<H", us))
    print(f"{path}: {len(files)} RT-turbo blocks")


def zx_rom_block(flag: int, payload: bytes) -> bytes:
    """One ZX ROM tape block as BYTES: flag + payload + XOR parity."""
    body = bytes([flag]) + payload
    parity = flag
    for b in body:
        parity ^= b
    return body + bytes([parity])


def write_tap(path, files, start=0x8000):
    """ZX .tap: each file becomes header+data CODE block pairs, loadable
    with the plain ROM loader (LOAD ""CODE). Compatibility fallback for
    machines without the RT-turbo loader."""
    import os
    with open(path, "wb") as f:
        for i, name in enumerate(files):
            with open(name, "rb") as g:
                data = g.read()
            base = os.path.basename(name).split(".")[0][:10] or f"m{i}"
            base = base.encode("ascii", "replace") + b" " * 10
            hdr = bytes([0x03]) + base[:10] + \
                struct.pack("<HH", len(data), start) + \
                struct.pack("<HH", start, 32768)
            for blk in (zx_rom_block(0x00, hdr), zx_rom_block(0xFF, data)):
                f.write(struct.pack("<H", len(blk)) + blk)
    print(f"{path}: {len(files)} ZX ROM block pairs (compatibility fallback)")


def write_tzx(path, files):
    """TZX v1.13 with one turbo block (0x11) per file. The block payload
    is the RT-turbo logical body with big-endian u16 fields, so the bit
    stream an RT-turbo decoder sees matches rt_tape.c byte for byte."""
    with open(path, "wb") as f:
        f.write(b"ZXTape!\x1a")
        f.write(struct.pack("<BB", 1, 13))
        for name in files:
            with open(name, "rb") as g:
                data = g.read()
            f.write(b"\x11")
            f.write(struct.pack(
                "<HHHHH",
                round(PILOT_US / US_PER_T),   # pilot pulse
                round(SYNC_US / US_PER_T),    # sync 1
                round(SYNC_US / US_PER_T),    # sync 2
                round(ZERO_US / US_PER_T),    # bit 0
                round(ONE_US / US_PER_T)))    # bit 1
            f.write(struct.pack("<H", 64))    # pilot pulse count
            f.write(struct.pack("<B", 8))     # bits in last byte
            f.write(struct.pack("<H", 0))     # pause after block (ms)
            body = struct.pack(">H", len(data)) + data + \
                struct.pack(">H", crc16_ccitt(data))
            f.write(struct.pack("<I", len(body))[:3])
            f.write(body)
    print(f"{path}: {len(files)} turbo blocks")



def write_label(path, files, counter_secs=3.0):
    """Cassette label: the classic workflow - write the counter value on
    the label so you can wind the tape straight to the block you need.
    One counter unit is about `counter_secs` seconds on a typical deck
    (C64 datasette ~3 s per unit)."""
    import os
    lines = []
    lines.append("retro-tinyllm topic tape - cassette label")
    lines.append("=" * 46)
    lines.append("counter   block  size      topic")
    lines.append("-" * 46)
    t_total = 0.0
    for i, name in enumerate(files):
        with open(name, "rb") as g:
            data = g.read()
        pulses = rt_block_pulses(data)
        dur = sum(pulses) / 1e6
        base = os.path.basename(name).split(".")[0]
        base = base[6:] if base.startswith("topic_") else base
        c0 = int(t_total / counter_secs)
        c1 = int((t_total + dur) / counter_secs)
        lines.append("%04d-%04d  %5d  %7d  %s" % (c0, c1, i, len(data), base))
        t_total += dur
    lines.append("-" * 46)
    lines.append("total: %.1f min, counter up to %04d" %
                 (t_total / 60, int(t_total / counter_secs)))
    lines.append("wind to the counter, then: /load <block>")
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print("%s: label for %d blocks (total %.1f min)" %
          (path, len(files), t_total / 60))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", help="output .rtp pulse file")
    ap.add_argument("--zx", help="optional .tap (ZX ROM blocks)")
    ap.add_argument("--tzx", help="optional .tzx (turbo blocks)")
    ap.add_argument("--label", help="cassette label with counters")
    ap.add_argument("files", nargs="+", help="model .bin files in tape order")
    args = ap.parse_args()

    if not (args.out or args.zx or args.tzx or args.label):
        ap.error("nothing to do: give --out, --zx, --tzx and/or --label")
    if args.out:
        write_rtp(args.out, args.files)
    if args.zx:
        write_tap(args.zx, args.files)
    if args.tzx:
        write_tzx(args.tzx, args.files)
    if args.label:
        write_label(args.label, args.files)


if __name__ == "__main__":
    main()
