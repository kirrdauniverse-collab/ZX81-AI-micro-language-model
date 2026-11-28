#!/usr/bin/env python3
"""modelinfo.py - inspect a model.bin (header, tensors, sizes, checksum).

usage: modelinfo.py models/base_c.bin
"""
import struct
import sys

FIELDS = ["version", "flags", "vocab", "ctx", "emb", "h1", "h2",
          "sh_e", "sh1", "sh2", "sh_o", "temp_sh"]


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    blob = open(sys.argv[1], "rb").read()
    if blob[:4] != b"RTLM":
        print("not an RTLM model")
        return 1
    vals = dict(zip(FIELDS, struct.unpack("<12H", blob[4:28])))
    charset = blob[28:92].split(b"\x00")[0].decode("ascii", "replace")
    print(f"file:      {sys.argv[1]} ({len(blob)} bytes)")
    for k in FIELDS:
        print(f"{k:9}: {vals[k]}")
    print(f"charset:   {len(charset)} chars, ok={charset.isprintable() or chr(10) in charset}")

    h1_in = vals["ctx"] * vals["emb"] + 1
    params = vals["vocab"] * vals["emb"]
    params += vals["h1"] * h1_in
    if vals["h2"]:
        params += vals["h2"] * (vals["h1"] + 1)
        out_in = vals["h2"] + 1
    else:
        out_in = vals["h1"] + 1
    params += vals["vocab"] * out_in
    print(f"params:    {params} ({params} bytes as int8, "
          f"{params/1024:.1f} KB)")

    n = 92
    tensors = [("emb", vals["vocab"] * vals["emb"])]
    tensors.append(("sh1", vals["h1"]))
    tensors.append(("w1", vals["h1"] * h1_in))
    if vals["h2"]:
        tensors.append(("sh2", vals["h2"]))
        tensors.append(("w2", vals["h2"] * (vals["h1"] + 1)))
    tensors.append(("sho", vals["vocab"]))
    tensors.append(("wo", vals["vocab"] * out_in))
    for name, size in tensors:
        print(f"  {name:4} {size:8} bytes @ {n}")
        n += size
    ck = sum(blob[:n]) & 0xFFFF
    stored = struct.unpack("<H", blob[n:n + 2])[0]
    print(f"checksum:  computed {ck:04x}, stored {stored:04x} "
          f"-> {'OK' if ck == stored else 'BAD'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
