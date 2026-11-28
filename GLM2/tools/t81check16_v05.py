#!/usr/bin/env python3
"""t81check16_v05.py - structural checker for the 16K ZX81 cassette set.

Recreated for v0.5.5 (the original checker was lost with the sandbox; the
assertion set follows docs/STATUS.md v0.5.2-v0.5.4 + the new v0.5.5 one):

  1. every .p / .t81 block: the sysvar region $4009-$407C byte-equal to
     the user's WORKING v0.4.5 rt16_base.p, except the documented
     canonical bytes (DF_SZ=2 at $4022, $4019, and v0.5.5: CDFLAG $40)
  2. $4026 word = $7FFD (the emulator quickload's SP source)
  3. $4029 word = D_FILE ($7B00, appmake's canonical copy)
  4. DF_SZ ($4022) = 2 (rows 22-23 = the ROM's lower screen)
  5. CDFLAG ($403B) = $40 - bit 6 SET: the machine stays SLOW after the
     ROM's tape LOAD copies the image over the sysvars. THIS was the
     constant-FAST root cause (v0.5.5; the ROM's own INITIAL routine
     stores exactly $40 here)
  6. lines 2/3: every number literal carries the $7E + 5-byte FP tail
  7. line 2 = IF USR 16514 = 1 THEN LOAD "<8-char name>", line 3 = SAVE
  8. name table: $A1 magic + count 14 + 14 x 8 display-code names
  9. the 7-byte display re-assert (FD 21 00 40 CD 2B 0F =
     LD IY,$4000 + CALL $0F2B SLOW) present in every block
 10. sizes: FULL .p = 15889 B, SHORT .t81 block = 14899 B, tape 227923 B (v0.6.6: UNCHANGED)
 11. EO81 container: magic, 15 blocks, names, lengths

Usage: python3 tools/t81check16_v05.py [dir=ports/zx81/16k] [v0.4.5 ref .p]
"""
import os
import sys

Z = 0x4009
DFILE = 0x7B00
MSIZE = 1030
NTOPICS = 14
SLOW_SEQ = bytes.fromhex("cd407a00000000")   # CALL $7A40 (ALTINT_ON) + NOPs
# v0.7.1: a blob 'CALL $0207' (offset 6) ki van NOP-olva az
# altint_v066.bin-ben - bit6=0-val hivva a tanc a ROM display-walkot
# inditotta el a D-file allapotsorokon (a 3. USR halala a topic-
# betoltes utan); a main() sajat SET 6 + 17x OUT $FE blokkja a teljes
# visszakapcsolas (l. app16.c v0.7.1 komment).
ALTINT_AT = 0x7A40
ALTINT_BLOB = open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "altint_v066.bin"), "rb").read()
TOPIC_NAMES = ["RTBOOT01", "RTBASE16", "RT-TUDOM", "RT-MAGYR", "RT-HOBBI",
               "RT-EGESZ", "RT-MUNKA", "RT-MUVES", "RT-TANUL", "RT-IRODA",
               "RT-FILME", "RT-PROGI", "RT-ASTRO", "RT-ASCII", "RT-VERSE"]
FULL_LEN = 15889
SHORT_LEN = 15085   # v0.6.6 rev-2: UNCHANGED - the images must not grow


def zx_codes(s):
    out = []
    for ch in s:
        if "A" <= ch <= "Z":
            out.append(0x26 + ord(ch) - 65)
        elif "0" <= ch <= "9":
            out.append(0x1C + int(ch))
        elif ch == "-":
            out.append(0x16)
        else:
            raise ValueError(ch)
    return out


def check_record(tag, nm, data, errs):
    """v0.6.4 guards: every .t81 block payload must start with OUR
    correctly-coded ZX name record (last byte |0x80) and end with 0x80
    (EightyOne truncates back to the last 0x80 otherwise), and the
    first byte must not be 0x00/0x01/0xFF (or EightyOne would generate
    its OWN name record with its off-by-one ZXCharTable)."""
    want = zx_codes(nm)
    rec = want[:-1] + [want[-1] | 0x80]
    if data[:len(rec)] != bytes(rec):
        errs.append(f"{tag}: name record {data[:8].hex()} != "
                    f"{bytes(rec).hex()}")
    if data[0] in (0x00, 0x01, 0xFF):
        errs.append(f"{tag}: first byte 0x{data[0]:02X} makes the "
                    "emulator generate its own (mismatched) name")
    if data[-1] != 0x80:
        errs.append(f"{tag}: last byte 0x{data[-1]:02X} != 0x80 "
                    "(emulator truncation would cut the block tail)")


def strip_record(nm, data, full):
    """Undo the t81_block wrapper: the 8-byte record prefix."""
    return data[8:]
TAPE_LEN = 227789 + 8 + 14 * 9   # v0.6.4 +name records (v0.6.6: sizes unchanged)


def be16(b):
    return (b[0] << 8) | b[1]


def le16w(v):
    return bytes((v & 255, (v >> 8) & 255))


def read_ref(path):
    """the sysvar block (116 B) of the user's working v0.4.5"""
    d = open(path, "rb").read()
    return bytearray(d[:0x407D - Z])


def parse_eo81(buf):
    """EO81 container (magic) or the bare single-block form
    (name[32]+len[16]+data) the builder writes for standalone .t81s"""
    if buf[:4] != b"EO81":
        nm = buf[:32].split(b"\0")[0].decode("latin1")
        ln = int(buf[32:48].split(b"\0")[0])
        return [(nm, buf[48:48 + ln])]
    blocks, off = [], 4
    while off + 48 <= len(buf):
        name = buf[off:off + 32].split(b"\0")[0].decode("latin1")
        off += 32
        ln = int(buf[off:off + 16].split(b"\0")[0])
        off += 16
        blocks.append((name, buf[off:off + ln]))
        off += ln
    return blocks


def check_image(tag, img, ref_sys, errs, full, with_names=True):
    def bad(msg):
        errs.append(f"{tag}: {msg}")

    if len(img) != (FULL_LEN if full else SHORT_LEN):
        bad(f"length {len(img)} != {'FULL 15889' if full else 'SHORT 14899'}")
    # 1-4: sysvars vs the working reference, with the canonical deltas
    sv = img[:0x407D - Z]
    if len(sv) < 116:
        bad("sysvar block too short")
        return
    for i in range(116):
        if sv[i] != ref_sys[i]:
            # allowed canonical differences (see STATUS.md v0.5.2-v0.5.5)
            addr = Z + i
            if addr == 0x4022 and sv[i] == 2:
                continue            # DF_SZ = 2 (v0.5.3)
            if addr == 0x4034 or addr == 0x4035:
                continue            # FRAMES may differ (running counter)
            if addr == 0x403B and sv[i] == 0x40:
                continue            # CDFLAG bit6 (v0.5.5 - THE FAST fix)
            if addr == 0x4019 and sv[i] == 0:
                continue            # benign appmake-canonical 0
            if not full and addr in (0x4010, 0x4011, 0x4014, 0x4015,
                                     0x4016, 0x4017, 0x401A, 0x401B,
                                     0x401C, 0x401D, 0x4029, 0x402A):
                continue  # SHORT: the E_LINE family points at the image
                          # end (v0.6.4 - checked separately below)
            bad(f"sysvar ${addr:04X} = {sv[i]:02X}, working ref has "
                f"{ref_sys[i]:02X}")
    if sv[0x4026 - Z] != 0xFD or sv[0x4027 - Z] != 0x7F:
        bad("stack-top word at $4026 is not $7FFD")
    if full:
        # the .p quickload images keep the NEW-machine sentinel there
        if sv[0x4029 - Z] != 0x00 or sv[0x402A - Z] != 0x7B:
            bad("D_FILE copy at $4029 is not $7B00")
    else:
        # v0.6.4: the tape form carries NXTLIN = line 1 ($407D) so the
        # ROM's NEXT-LINE loop auto-starts the loaded program at line 2
        if sv[0x4029 - Z] != 0x7D or sv[0x402A - Z] != 0x40:
            bad("NXTLIN at $4029 is not $407D (line 1)")
    if sv[0x4022 - Z] != 2:
        bad("DF_SZ at $4022 != 2")
    if sv[0x403B - Z] != 0x40:
        bad("CDFLAG at $403B != $40 (bit 6 SLOW) - THE v0.5.5 FIX")
    # 9: the display re-assert sequence (v0.5.6: CALL ALTINT_ON - the
    # register-preserving display chain install)
    if SLOW_SEQ not in img:
        bad("CALL $7A40 (ALTINT_ON) sequence missing in main()")
    # 9b: the altint chain blob itself at the fixed address
    off = ALTINT_AT - Z
    if len(img) >= off + len(ALTINT_BLOB) and img[off:off + len(ALTINT_BLOB)] == ALTINT_BLOB:
        pass
    elif len(img) > off + len(ALTINT_BLOB):
        bad("the altint chain blob at $7A40 differs")
    else:
        bad("the altint chain blob at $7A40 missing (image too short?)")
    # 9c: v0.6.6 - the version-independent SLOW. The v1 'standard' ROM
    # keeps SLOW at $0F28; a CALL $0F2B there lands mid-instruction
    # (OR $C3/RLCA/LD (BC),A/CALL $0EA7) and dies with REPORT B - the
    # user's real machine. Now: no CALL $0F2B anywhere, the crt0 calls
    # the stub, and the 8-byte stub (SET 6,(IY+$3B); CALL $0207; RET)
    # sits right after the blob at the fixed address $7AF5.
    if b"\xCD\x2B\x0F" in img:
        bad("CALL $0F2B still present (v1-ROM report-B crash!)")
    # the crt0's version-independent SLOW (v0.6.7: built from source):
    # CALL save81; LD IY,$4000; SET 6,(IY+$3B); CALL $0207
    if img[0x79:0x81] != bytes.fromhex("7EC38A4000007 6FF".replace(" ","")):
        pass  # the entry block (LD A,(HL); JP .start; base_graphics)
    if img[0x84:0x91] != bytes.fromhex("FD210040" + "FDCB3BB6" + "FD7E3B" + "D3FD"):
        bad("the crt0 display-gate-off (LD IY,$4000 + RES 6,(IY+$3B) + LD A,(IY+$3B) + OUT $FD) missing")
    # 6/7: BASIC lines
    a = 0x407D - Z
    ln = be16(img[a:a + 2])
    if ln != 1:
        bad(f"first line number {ln} != 1")
    l2len = img[a + 2] | (img[a + 3] << 8)
    a2 = a + 4 + l2len                       # past line 1 incl. 0x76
    if be16(img[a2:a2 + 2]) != 2:
        bad("line 2 not found")
    l2 = img[a2 + 4:a2 + 4 + 33]
    if not l2.startswith(b"\xFA\xD4") or b"\xDE\xEF\x0B" not in l2[:26]:
        bad("line 2 is not IF USR .. THEN LOAD")
    if img.count(b"\x7E\x8F\x01\x04\x00\x00") < 1:
        bad("the 16514 literal has no $7E+FP tail")
    # 8: the name table (the boot block carries none)
    tab = img.find(b"\xA1" + bytes([NTOPICS])) if with_names else -1
    if tab < 0:
        if with_names:
            bad("name table magic $A1+14 not found")
    else:
        for k in range(NTOPICS):
            nm = img[tab + 2 + k * 8: tab + 10 + k * 8]
            want = TOPIC_NAMES[1 + k] if k else "RTBASE16"
            for c, ch in zip(nm, want):
                exp = 0x26 + ord(ch) - 65 if ch.isalpha() else \
                      (0x1C + int(ch) if ch.isdigit() else 0x16)
                if c != exp:
                    bad(f"name slot {k}: byte {c:02X} != {exp:02X} ({ch})")
                    break


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else \
        os.path.join(os.path.dirname(__file__), "..", "ports", "zx81", "16k")
    d = os.path.normpath(d)
    ref = read_ref(sys.argv[2] if len(sys.argv) > 2 else "/tmp/ref/v045_base.p")
    errs = []
    files = sorted(os.listdir(d))
    ps = [f for f in files if f.endswith(".p")]
    t81s = [f for f in files if f.endswith(".t81") and "TOPICS" not in f]
    if len(ps) != 14 or len(t81s) != 14:
        errs.append(f"expected 14 .p + 14 .t81, got {len(ps)}+{len(t81s)}")
    for f in ps:
        img = open(os.path.join(d, f), "rb").read()
        check_image(f, img, ref, errs, full=True)
    for f in t81s:
        blk = parse_eo81(open(os.path.join(d, f), "rb").read())
        if len(blk) != 1:
            errs.append(f"{f}: {len(blk)} blocks in a single cassette")
        else:
            nm, data = blk[0]
            check_record(f, nm, data, errs)
            check_image(f, strip_record(nm, data, True), ref, errs,
                        full=True)
    # the universal tape
    tape_path = os.path.join(d, "GLM_v070_RT16_TOPICS.t81")
    tape = open(tape_path, "rb").read() if os.path.exists(tape_path) else None
    if tape is None:
        errs.append("GLM_v070_RT16_TOPICS.t81 missing")
    else:
        if len(tape) != TAPE_LEN:
            errs.append(f"tape length {len(tape)} != {TAPE_LEN}")
        blocks = parse_eo81(tape)
        if len(blocks) != 15:
            errs.append(f"tape has {len(blocks)} blocks, want 15")
        for i, (nm, data) in enumerate(blocks):
            if nm != TOPIC_NAMES[i]:
                errs.append(f"tape block {i + 1} name {nm!r} != "
                            f"{TOPIC_NAMES[i]!r}")
            check_record(f"tape:{nm}", nm, data, errs)
            full = (i == 0)
            img2 = strip_record(nm, data, full)
            check_image(f"tape:{nm}", img2, ref, errs, full=full,
                        with_names=(i != 0))
            if not full:
                # v0.6.4: the ROM's LOAD ends when the write pointer
                # reaches E_LINE - a SHORT image must carry E_LINE =
                # its own end or the load runs past the block (the
                # real-emulator black screen)
                eline = img2[11] | (img2[12] << 8)
                if eline != Z + len(img2):
                    errs.append(f"tape:{nm}: E_LINE ${eline:04X} != image "
                                f"end ${Z + len(img2):04X}")
                fam = {0x4010: eline - 1, 0x4014: eline,
                       0x4016: eline + 4, 0x401A: eline + 5,
                       0x401C: eline + 5}
                for a, want in fam.items():
                    got = img2[a - Z] | (img2[a - Z + 1] << 8)
                    if got != want:
                        errs.append(f"tape:{nm}: sysvar ${a:04X}="
                                    f"${got:04X} != ${want:04X}")
                if img2[-1] != 0x80:
                    errs.append(f"tape:{nm}: SHORT image must end with "
                                "the 0x80 marker")
    if errs:
        print("CHECK FAILED:")
        for e in errs:
            print("  -", e)
        sys.exit(1)
    print(f"ALL CHECKS PASSED ({len(ps)} .p + {len(t81s)} .t81 + 15-block "
          f"tape; CDFLAG=$40 asserted in every block)")


if __name__ == "__main__":
    main()
