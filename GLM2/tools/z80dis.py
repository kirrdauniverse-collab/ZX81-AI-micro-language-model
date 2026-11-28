#!/usr/bin/env python3
"""z80dis.py - minimal Z80 disassembler a ROM-analizishoz (nincs
kulso fuggoseg; a z88dk binarok a recycle-ben elvesztek).

Hasznalat: z80dis.py <bin> <org> [hossz]
Kiirod minden utasitast: cim, hex, mnemonic.
"""
import sys


# (opcode, hossz, mnemonic-sablon) - a hasznalt reszhalmaz
DD = {  # prefix nélküli
    0x00: (1, "nop"), 0x76: (1, "halt"),
    0xC9: (1, "ret"), 0xD9: (1, "exx"), 0x27: (1, "daa"),
    0x2F: (1, "cpl"), 0x3F: (1, "ccf"), 0x37: (1, "scf"),
    0xE3: (1, "ex (sp),hl"), 0xEB: (1, "ex de,hl"),
    0x08: (1, "ex af,af'"), 0xF3: (1, "di"), 0xFB: (1, "ei"),
    0xF9: (1, "ld sp,hl"), 0xE9: (1, "jp (hl)"),
    0x07: (1, "rlca"), 0x0F: (1, "rrca"), 0x17: (1, "rla"), 0x1F: (1, "rra"),
    0xC0: (1, "ret nz"), 0xC8: (1, "ret z"), 0xD0: (1, "ret nc"),
    0xD8: (1, "ret c"), 0xE0: (1, "ret po"), 0xE8: (1, "ret pe"),
    0xF0: (1, "ret p"), 0xF8: (1, "ret m"),
    0xC7: (1, "rst $00"), 0xCF: (1, "rst $08"), 0xD7: (1, "rst $10"),
    0xDF: (1, "rst $18"), 0xE7: (1, "rst $20"), 0xEF: (1, "rst $28"),
    0xF7: (1, "rst $30"), 0xFF: (1, "rst $38"), 0xC7 + 0x66: (1, "nmi-entry"),
}
R16SP = ["bc", "de", "hl", "sp"]
R16AF = ["bc", "de", "hl", "af"]
RP = ["nz", "z", "nc", "c", "po", "pe", "p", "m"]
ALU = ["add a,", "adc a,", "sub ", "sbc a,", "and ", "xor ", "or ", "cp "]


def disasm(data, org, length=None):
    out = []
    i = 0
    n = len(data) if length is None else min(length, len(data))
    while i < n:
        pc = org + i
        op = data[i]
        # DD/FD prefix (ix/iy)
        pfx = ""
        if op in (0xDD, 0xFD):
            pfx = "ix" if op == 0xDD else "iy"
            i += 1
            op = data[i]
            # egyszeru esetek kezelése a prefixszel
            if op == 0x21:  # ld ix,nn
                v = data[i+1] | (data[i+2] << 8)
                out.append((pc, data[i-1:i+3], f"ld {pfx},${v:04x}"))
                i += 3
                continue
            if op == 0xE5:
                out.append((pc, data[i-1:i+1], f"push {pfx}"))
                i += 1
                continue
            if op == 0xE1:
                out.append((pc, data[i-1:i+1], f"pop {pfx}"))
                i += 1
                continue
            if op == 0xE3:
                out.append((pc, data[i-1:i+1], f"ex (sp),{pfx}"))
                i += 1
                continue
            if op == 0xF9:
                out.append((pc, data[i-1:i+1], f"ld sp,{pfx}"))
                i += 1
                continue
            if op == 0xE9:
                out.append((pc, data[i-1:i+1], f"jp ({pfx})"))
                i += 1
                continue
            if op == 0x22:  # ld (nn),ix
                v = data[i+1] | (data[i+2] << 8)
                out.append((pc, data[i-1:i+3], f"ld (${v:04x}),{pfx}"))
                i += 3
                continue
            if op == 0x2A:  # ld ix,(nn)
                v = data[i+1] | (data[i+2] << 8)
                out.append((pc, data[i-1:i+3], f"ld {pfx},(${v:04x})"))
                i += 3
                continue
            if op == 0xCB:  # DDCB - shift (ix+d)
                d = data[i+1] - 256 if data[i+1] > 127 else data[i+1]
                op2 = data[i+2]
                ops = ["rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl"]
                out.append((pc, data[i-1:i+3], f"{ops[op2>>3&7]} ({pfx}${d & 0xff:02x})"))
                i += 3
                continue
            # ED prefix
        if op == 0xED:
            op2 = data[i+1]
            if op2 == 0xB0:
                out.append((pc, data[i:i+2], "ldir"))
            elif op2 == 0xB8:
                out.append((pc, data[i:i+2], "lddr"))
            elif op2 == 0x73:
                out.append((pc, data[i:i+2], "ld (nn),sp"))
            elif op2 == 0x47:
                out.append((pc, data[i:i+2], "ld i,a"))
            elif op2 == 0x57:
                out.append((pc, data[i:i+2], "ld a,i"))
            elif op2 == 0x4F:
                out.append((pc, data[i:i+2], "ld r,a"))
            elif op2 == 0xB0 & 0:  # unused
                pass
            else:
                out.append((pc, data[i:i+2], f"db $ed,${op2:02x}"))
            i += 2
            continue
        if op == 0xCB:
            op2 = data[i+1]
            if op2 < 0x40:
                ops = ["rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl"]
                regs = ["b", "c", "d", "e", "h", "l", "(hl)", "a"]
                out.append((pc, data[i:i+2], f"{ops[op2>>3&7]} {regs[op2&7]}"))
            else:  # bit/set/res: op2>>6 == 1(bit)/2(res)/3(set)
                ops = [None, "bit", "res", "set"]
                regs = ["b", "c", "d", "e", "h", "l", "(hl)", "a"]
                out.append((pc, data[i:i+2],
                            f"{ops[(op2>>6)&3]} {(op2>>3)&7},{regs[op2&7]}"))
            i += 2
            continue
        # 1 byte, ismert tabla
        if op in DD:
            ln, mn = DD[op]
            out.append((pc, data[i:i+ln], mn))
            i += ln
            continue
        # ld r,r' (0x40-0x7F)
        if 0x40 <= op <= 0x7F and (op & 7) != 6:
            regs = ["b", "c", "d", "e", "h", "l", "(hl)", "a"]
            out.append((pc, data[i:i+1], f"ld {regs[op>>3&7]},{regs[op&7]}"))
            i += 1
            continue
        # push/pop qq
        if 0xC5 <= op <= 0xF5 and (op & 0x0F) in (0x05,):
            regs = ["bc", "de", "hl", "af"]
            mn = "push" if op & 0x30 == 0x40 or op >= 0xC5 and op < 0xE5 else "pop"
            mn = "push" if (op >> 4) & 1 == 0 and op < 0xE5 else "pop"
            out.append((pc, data[i:i+1], f"{mn} {regs[(op>>4)&3]}"))
            i += 1
            continue
        # jp nn / call nn / jr
        if op == 0xC3 or op == 0xCD:
            v = data[i+1] | (data[i+2] << 8)
            mn = "jp" if op == 0xC3 else "call"
            out.append((pc, data[i:i+3], f"{mn} ${v:04x}"))
            i += 3
            continue
        if op == 0x18 or 0x20 <= op <= 0x38:
            d = data[i+1] - 256 if data[i+1] > 127 else data[i+1]
            tgt = org + i + 2 + d
            if op == 0x18:
                mn = "jr"
            else:
                mn = f"jr {RP[(op >> 3) & 3]}"
            out.append((pc, data[i:i+2], f"{mn} ${tgt:04x}"))
            i += 2
            continue
        # ld r,n / add a,n stb. (n operandus)
        if 0x06 <= op <= 0x3E and op % 8 == 6:
            regs = ["b", "c", "d", "e", "h", "l", "(hl)", "a"]
            out.append((pc, data[i:i+2], f"ld {regs[op>>3&7]},${data[i+1]:02x}"))
            i += 2
            continue
        if 0xC6 == op or op == 0xE6 or op == 0xF6 or op == 0xCE or op == 0xD6 or op == 0xEE or op == 0xFE:
            mn = ALU[(op >> 3) & 7].strip() + f" a,${data[i+1]:02x}" if op in (0xC6, 0xCE) else None
            if mn is None:
                m = ["add a", "adc a", "sub", "sbc a", "and", "xor", "or", "cp"][(op >> 3) & 7]
                mn = f"{m} ${data[i+1]:02x}"
            out.append((pc, data[i:i+2], mn))
            i += 2
            continue
        # ld (nn),a / ld a,(nn) / ld (nn),hl / ld hl,(nn)
        if op == 0x32:
            v = data[i+1] | (data[i+2] << 8)
            out.append((pc, data[i:i+3], f"ld (${v:04x}),a"))
            i += 3
            continue
        if op == 0x3A:
            v = data[i+1] | (data[i+2] << 8)
            out.append((pc, data[i:i+3], f"ld a,(${v:04x})"))
            i += 3
            continue
        if op == 0x22:
            v = data[i+1] | (data[i+2] << 8)
            out.append((pc, data[i:i+3], f"ld (${v:04x}),hl"))
            i += 3
            continue
        if op == 0x2A:
            v = data[i+1] | (data[i+2] << 8)
            out.append((pc, data[i:i+3], f"ld hl,(${v:04x})"))
            i += 3
            continue
        # ld rp,nn (0x01,0x11,0x21,0x31)
        if op in (0x01, 0x11, 0x21, 0x31):
            v = data[i+1] | (data[i+2] << 8)
            out.append((pc, data[i:i+3], f"ld {R16SP[op>>4&3]},${v:04x}"))
            i += 3
            continue
        # out (n),a / in a,(n)
        if op == 0xD3:
            out.append((pc, data[i:i+2], f"out (${data[i+1]:02x}),a"))
            i += 2
            continue
        if op == 0xDB:
            out.append((pc, data[i:i+2], f"in a,(${data[i+1]:02x})"))
            i += 2
            continue
        # call cc,nn
        if 0xC4 <= op <= 0xF4 and (op & 0x0F) in (0x4,):
            v = data[i+1] | (data[i+2] << 8)
            out.append((pc, data[i:i+3], f"call {RP[(op>>3)&3]},${v:04x}"))
            i += 3
            continue
        # jp cc,nn
        if 0xC2 <= op <= 0xF2 and (op & 0x0F) in (0x2,):
            v = data[i+1] | (data[i+2] << 8)
            out.append((pc, data[i:i+3], f"jp {RP[(op>>3)&3]},${v:04x}"))
            i += 3
            continue
        # add hl,rp / inc/dec rp
        if op in (0x09, 0x19, 0x29, 0x39):
            out.append((pc, data[i:i+1], f"add hl,{R16SP[op>>4&3]}"))
            i += 1
            continue
        # inc/dec r
        if 0x04 <= op <= 0x3C and (op & 0x07) in (0x4, 0x5):
            regs = ["b", "c", "d", "e", "h", "l", "(hl)", "a"]
            mn = "inc" if op & 8 == 0 else "dec"
            out.append((pc, data[i:i+1], f"{mn} {regs[op>>3&7]}"))
            i += 1
            continue
        # fallback
        out.append((pc, data[i:i+1], f"db ${op:02x}"))
        i += 1
    return out


if __name__ == "__main__":
    path, org = sys.argv[1], int(sys.argv[2], 0)
    length = int(sys.argv[3], 0) if len(sys.argv) > 3 else None
    data = open(path, "rb").read()
    for pc, raw, mn in disasm(data, org, length):
        print(f"${pc:04x}: {' '.join('%02x' % b for b in raw):12} {mn}")
