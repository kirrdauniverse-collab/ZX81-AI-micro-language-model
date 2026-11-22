#!/usr/bin/env python3
"""d64write.py - minimal 35-track Commodore 1541 .d64 image writer/reader.

Writes PRG files (model packs + the engine binary) into a blank .d64 and
verifies them by reading back. The C64 port loads these files from disk
via the kernal LOAD - disk stays the primary model source, tape is the
fallback (see docs/TAPE_DISK.md).

usage: d64write.py --out models.d64 file1.prg file2.bin ...
       d64write.py --list models.d64
"""
import argparse
import os
import struct
import sys

SECTORS = [21] * 17 + [19] * 7 + [18] * 6 + [17] * 5  # tracks 1..35
DIR_TRACK = 18
SECTOR = 256   # bytes per sector in the image (2 link + 254 data)
DATA = 254     # payload bytes per sector


class D64:
    def __init__(self):
        self.sectors = {}
        for t, n in enumerate(SECTORS, start=1):
            for s in range(n):
                self.sectors[(t, s)] = bytearray(SECTOR)
        self._bam_init()

    # --- BAM (track 18 sector 0): bit SET means sector free ---
    def _bam_init(self):
        bam = self.sectors[(DIR_TRACK, 0)]
        for t, n in enumerate(SECTORS, start=1):
            base = 4 + (t - 1) * 4
            bam[base] = n
            for s in range(n):
                if t == DIR_TRACK:
                    bam[base + 1 + s // 8] = 0  # dir track: keep allocated
                else:
                    bam[base + 1 + s // 8] |= 1 << (s % 8)

    def bam_free(self, t, s):
        bam = self.sectors[(DIR_TRACK, 0)]
        byte = 4 + (t - 1) * 4 + 1 + s // 8
        return (bam[byte] >> (s % 8)) & 1

    def bam_alloc(self, t, s):
        bam = self.sectors[(DIR_TRACK, 0)]
        byte = 4 + (t - 1) * 4 + 1 + s // 8
        bam[byte] &= ~(1 << (s % 8)) & 0xFF
        bam[4 + (t - 1) * 4] -= 1

    def alloc_sector(self):
        """First free data sector, scanning tracks in BAM order."""
        for t in range(1, 36):
            if t == DIR_TRACK:
                continue
            for s in range(SECTORS[t - 1]):
                if self.bam_free(t, s):
                    self.bam_alloc(t, s)
                    return (t, s)
        raise SystemExit("disk full")

    def write_prg(self, name: str, data: bytes, load_addr: int):
        """Store one PRG (load address + payload) as a chained file."""
        payload = struct.pack("<H", load_addr) + data
        blocks = [payload[i:i + DATA] for i in range(0, len(payload), DATA)]
        if not blocks:
            blocks = [b""]

        chain = [self.alloc_sector() for _ in blocks]
        for i, (t, s) in enumerate(chain):
            blk = self.sectors[(t, s)]
            if i + 1 < len(chain):
                blk[0], blk[1] = chain[i + 1]
            else:
                blk[0], blk[1] = 0, len(blocks[i])
            blk[2:2 + len(blocks[i])] = blocks[i]

        # directory entry in the first free slot of the dir chain
        # layout: [0-1] first data ts, [2] type, [3-4] side-sector ts,
        #         [5-20] name (PETSCII, 0xa0 padded), [21-27] rel-file
        #         fields, [28-29] size in blocks, [30-31] unused
        for ds in range(1, SECTORS[DIR_TRACK - 1]):
            blk = self.sectors[(DIR_TRACK, ds)]
            for slot in range(8):
                off = 2 + slot * 32
                if blk[off + 2] == 0:
                    petscii = name.upper().encode("ascii", "replace")[:16]
                    petscii = petscii.ljust(16, b"\xa0")
                    entry = bytes([chain[0][0], chain[0][1], 0x82, 0, 0]) \
                        + petscii + b"\x00" * 7 \
                        + struct.pack("<H", len(chain)) + b"\x00\x00"
                    blk[off:off + 32] = entry
                    return
        raise SystemExit("directory full")

    def read_prg(self, name: str):
        """Read a file back by name (verification path)."""
        want = name.upper()
        for ds in range(1, SECTORS[DIR_TRACK - 1]):
            blk = self.sectors[(DIR_TRACK, ds)]
            for slot in range(8):
                off = 2 + slot * 32
                if blk[off + 2] == 0:
                    continue
                fname = blk[off + 5:off + 21].replace(b"\xa0", b" ")
                fname = fname.decode("ascii", "replace").strip()
                if fname != want:
                    continue
                data = bytearray()
                t, s = blk[off], blk[off + 1]
                seen = set()
                while t and (t, s) in self.sectors:
                    if (t, s) in seen:
                        break
                    seen.add((t, s))
                    blk2 = self.sectors[(t, s)]
                    used = blk2[1] if blk2[0] == 0 else DATA
                    data += blk2[2:2 + used]
                    t, s = blk2[0], blk2[1]
                return bytes(data)
        return None

    def bytes(self) -> bytes:
        out = bytearray()
        for t, n in enumerate(SECTORS, start=1):
            for s in range(n):
                out += self.sectors[(t, s)]
        return bytes(out)  # 683 * 256 = 174848 bytes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", help="write files to a new .d64")
    ap.add_argument("--list", help="list files in a .d64")
    ap.add_argument("--addr", type=lambda x: int(x, 0), default=0xC000,
                    help="load address for raw .bin files (default 0xC000)")
    ap.add_argument("files", nargs="*", help="files to store (.prg or .bin)")
    args = ap.parse_args()

    if args.list:
        raw = open(args.list, "rb").read()
        print(f"{args.list}: {len(raw)} bytes, "
              f"expected {683 * 256} for 35 tracks")
        return 0

    if not args.out:
        ap.error("give --out and/or --list")

    d = D64()
    # disk header inside the BAM sector (track 18 sector 0):
    #   [0-1] first dir ts, [2] DOS version, [3-0x8f] BAM (35x4),
    #   [0x90-0x9f] disk name, [0xa0-0xa1] id, [0xa2] dos version char
    bam = d.sectors[(DIR_TRACK, 0)]
    bam[0:2] = b"\x12\x01"
    bam[2] = 0x41
    name = b"RETROTINYLLM"[:16]
    bam[0x90:0x90 + len(name)] = name
    bam[0x90 + len(name):0xA0] = b"\xa0" * (16 - len(name))
    bam[0xA0:0xA2] = b"26"
    bam[0xA2] = 0x32  # '2'
    bam[0xA3:0xA5] = b"\xa0\xa0"

    for f in args.files:
        data = open(f, "rb").read()
        base = os.path.basename(f).split(".")[0][:16]
        if f.lower().endswith(".prg"):
            addr = struct.unpack("<H", data[:2])[0]
            d.write_prg(base, data[2:], addr)
            ok = d.read_prg(base) == data
        else:
            d.write_prg(base, data, args.addr)
            ok = d.read_prg(base) == struct.pack("<H", args.addr) + data
        print(f"{base}: {len(data)} bytes -> {'OK' if ok else 'VERIFY FAIL'}")
        if not ok:
            return 1

    with open(args.out, "wb") as f:
        f.write(d.bytes())
    print(f"{args.out}: {len(d.bytes())} bytes, "
          f"{len(args.files)} files, verified by read-back")
    return 0


if __name__ == "__main__":
    sys.exit(main())
