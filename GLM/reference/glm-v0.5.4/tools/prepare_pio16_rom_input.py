#!/usr/bin/env python3
"""Prepare the PIOCO 16K sources and tape builder.

The boot wizard deliberately delegates every keystroke to the original
ZX81 BASIC INPUT editor.  This script only prepares the ROM BASIC lines
and the C routine that reads the values after INPUT has finished.
"""
from pathlib import Path
import os
import re

ROOT = Path(__file__).resolve().parents[1]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, data):
    (ROOT / rel).write_text(data, encoding="utf-8")


def replace_once(text, old, new, label):
    if new in text:
        return text
    if old not in text:
        raise SystemExit("Cannot patch " + label + ": source did not match")
    return text.replace(old, new, 1)


# Main chat: English intent tag, chat initialization and a short bounded turn.
p = "ports/zx81/app16.c"
s = read(p)
s = replace_once(s,
    "#define MSIZE 1030              /* the class-G blob: fixed layout */",
    "#define MSIZE 1038              /* English8 QAT G2 blob */", "model size")
if '#include "intent16.h"' not in s:
    s = replace_once(s, '#include "rt_engine.h"\n',
                     '#include "rt_engine.h"\n#include "intent16.h"\n',
                     "English intent include")
if "rt16_reply_tag(line)" not in s:
    s = replace_once(s, "rt_chat_push(&chat, line);\n        gen_reply();",
        "rt_chat_push(&chat, line);\n        rt_chat_push(&chat, rt16_reply_tag(line));\n        rt_chat_push_idx(&chat, 0);\n        gen_reply();",
        "English intent tag")
if "rt_chat_init(&chat" not in s:
    s = replace_once(s, "    rt_charset_init();\n",
        "    rt_charset_init();\n"
        "    rt_chat_init(&chat, hal_entropy() ^ 0xA51F39D7UL);\n"
        "    chat.max_new = 48;\n",
        "chat initialization")
write(p, s)

# The port must use the library's genuine ZX81 single-key scanner.
hal = read("ports/zx81/hal_zx81.c")
if "#include <input.h>" not in hal or "in_Inkey()" not in hal or "in_WaitForNoKey()" not in hal:
    raise SystemExit("HAL is not using Z88DK ZX81 INKEY and key-release routines")
# Correct shifted-key values returned by the Z88DK INKEY translation table:
# SHIFT+6=9, SHIFT+7=11, SHIFT+9=10, SHIFT+5=8 (RUBOUT).
hal = hal.replace("case 10: return 0x1E;  /* SHIFT+6: scroll/page up */",
                  "case 9:  return 0x1E;  /* SHIFT+6: scroll/page up */")
hal = hal.replace("case 11: return 0x1F;  /* SHIFT+7: scroll/page down */",
                  "case 11: return 0x1F;  /* SHIFT+7: scroll/page down */")
hal = hal.replace("case 6:  return 0x06;  /* SHIFT+9: reasoning toggle */",
                  "case 10: return 0x06; /* SHIFT+9: reasoning toggle */")
# The 24-row D-FILE leaves rows 22-23 to the ROM lower screen;
# the application text console can therefore use rows 8..21.
hal = hal.replace("#define ZX81_TEXT_BOT 15", "#define ZX81_TEXT_BOT 21")
write("ports/zx81/hal_zx81.c", hal)

# Keep the documented shortcuts aligned with Z88DK's actual shifted-key codes.
app = read("ports/zx81/app16.c")
app = app.replace("#define K_PAGEUP   0x1E    /* SHIFT+5 */",
                  "#define K_PAGEUP   0x1E    /* SHIFT+6 */")
app = app.replace("#define K_PAGEDOWN 0x1F    /* SHIFT+0 */",
                  "#define K_PAGEDOWN 0x1F    /* SHIFT+7 */")
app = app.replace("#define K_RUBOUT   8       /* SHIFT+5: ZX81 RUBOUT (ASCII backspace) */",
                  "#define K_RUBOUT   8       /* SHIFT+5: ZX81 RUBOUT (ASCII backspace) */")
write("ports/zx81/app16.c", app)

# The BASIC INPUT editor stores the user's name in a string variable.
# Ignore the ZX81's inverse-video bit when translating the stored text.
boot = read("ports/zx81/boot16.c")
if "read_profile_from_rom_input" not in boot:
    raise SystemExit("boot16.c is not the ROM-INPUT profile reader")
boot = boot.replace("        rt_u8 c = p[i];\n", "        rt_u8 c = (rt_u8)(p[i] & 0x7F);\n")
boot = boot.replace("void main(void)", "int main(void)", 1)
write("ports/zx81/boot16.c", boot)

# Full 24-row screen file and correct system variables.
p = "tools/t81build16.py"
s = read(p)
s = replace_once(s,
    "VARS = DFILE + 793\nELINE = VARS + 1\nMSIZE = 1030",
    'LAYOUT = os.environ.get("RT16_LAYOUT", "legacy").lower()\n'
    'if LAYOUT not in ("legacy", "ramtop"):\n'
    '    raise ValueError("RT16_LAYOUT must be legacy or ramtop")\n'
    'DFILE_LINES = 24\n'
    'DFILE_SIZE = 1 + DFILE_LINES * 33\n'
    'VARS = DFILE + DFILE_SIZE\n'
    'ELINE = VARS + 1',
    "24-row D-FILE constants")
s = replace_once(s,
    "    s = bytearray(116)",
    '    template_path = os.environ.get("RT16_SYSVARS_TEMPLATE")\n'
    '    if template_path:\n'
    '        template_data = Path(template_path).read_bytes()\n'
    '        if len(template_data) < 116:\n'
    '            raise ValueError("Z88DK .P system-variable template is too short")\n'
    '        s = bytearray(template_data[:116])\n'
    '    else:\n'
    '        s = bytearray(116)',
    "Z88DK system-variable template")
s = replace_once(s,
    "    s[29:31] = le16(0x7FFD)     # $4026 = the 16K stack top (SP source!)",
    "    s[28:31] = bytes((0xBF, 0xFD, 0xFF))  # LAST_K=$FDBF, DEBOUNCE=$FF",
    "keyboard system variables")
s = replace_once(s,
    "    s[31] = 0                   # $4028",
    "    s[31] = 55                  # $4028 MARGIN: 55 PAL scanlines\n"
    "    s[50] = 0x40                # $403B CDFLAG: SLOW display",
    "ZX81 display system variables")
s = replace_once(s,
    "    s[32:34] = le16(DFILE)      # $4029 = D_FILE copy",
    "    s[32:34] = le16(PROG)       # $4029 NTLIN = first BASIC line",
    "NTLIN")
s = replace_once(s,
    'return b"\\x76" + (b"\\x00" * 32 + b"\\x76") * 24',
    'return b"\\x76" + (b"\\x00" * 32 + b"\\x76") * DFILE_LINES',
    "D-FILE row generator")
s = replace_once(s,
    'assert Z + len(img) < DFILE - 793, "boot chain too big"',
    'assert Z + len(img) < DFILE - DFILE_SIZE, "boot chain too big"',
    "boot image D-FILE bound")

# Use the actual generated model length rather than the old 1030-byte size.
if "def model_blob_size(model):" not in s:
    marker = "\ndef build_image(code, model, tag_idx, tag_name, load_name, with_save):"
    helper = '''

def model_blob_size(model):
    """Read RTLM v2 dimensions and verify the blob's exact length."""
    import struct
    if len(model) < 94 or model[:4] != b"RTLM":
        raise ValueError("invalid RTLM model header")
    vocab, ctx, emb, h1, h2 = struct.unpack_from("<5H", model, 8)
    n = 92 + vocab * emb + h1 + h1 * (ctx * emb + 1)
    if h2:
        n += h2 + h2 * (h1 + 1)
    n += vocab + vocab * ((h2 if h2 else h1) + 1) + 2
    if n != len(model):
        raise ValueError("RTLM size mismatch: header says %d, file has %d" % (n, len(model)))
    return n

'''
    if marker not in s:
        raise SystemExit("Cannot locate build_image for model size helper")
    s = s.replace(marker, helper + marker, 1)
s = replace_once(s,
    "    assert M <= DFILE - 17 - 116 - 12 - MSIZE, \\",
    "    msize = model_blob_size(model)\n"
    "    assert M <= DFILE - 17 - 116 - 12 - msize, \\",
    "dynamic model size bound")
s = s.replace("    tag = M + MSIZE", "    tag = M + msize")
s = re.sub(r"assert len\(model\) == MSIZE,.*",
           'assert len(model) == model_blob_size(model), "invalid RTLM blob size"', s)
# Preserve the 24-row layout in the image builder.
write(p, s)

# Emit the boot prompts as native ZX81 BASIC PRINT/INPUT statements.
helper = r'''

def zx_text_codes(text):
    out = bytearray()
    for ch in text:
        if "A" <= ch <= "Z":
            out.append(0x26 + ord(ch) - 65)
        elif "0" <= ch <= "9":
            out.append(0x1C + ord(ch) - 48)
        elif ch == " ":
            out.append(0x00)
        elif ch == "?":
            out.append(0x0F)
        elif ch == "-":
            out.append(0x16)
        elif ch == "/":
            out.append(0x18)
        else:
            raise ValueError("unsupported ZX81 prompt character: " + repr(ch))
    return bytes(out)


def boot_basic_line(number, body):
    return be16(number) + le16(len(body) + 1) + body + b"\x76"


def boot_print_line(number, text):
    return boot_basic_line(number, b"\xF5\x0B" + zx_text_codes(text) + b"\x0B")


def boot_input_line(number, letter, string_value=False):
    variable = bytes([0x26 + ord(letter) - 65])
    if string_value:
        variable += b"\x0D"  # ZX81 '$' character code
    return boot_basic_line(number, b"\xEE" + variable)  # ROM INPUT token


def boot_input_program():
    # PRINT labels followed by the ROM INPUT editor. No C-side key polling.
    lines = [
        boot_print_line(2, "PIOCO AI PROFILE"),
        boot_print_line(3, "YEAR"),
        boot_input_line(4, "Y"),
        boot_print_line(5, "MONTH"),
        boot_input_line(6, "M"),
        boot_print_line(7, "DAY"),
        boot_input_line(8, "D"),
        boot_print_line(9, "HOUR"),
        boot_input_line(10, "H"),
        boot_print_line(11, "MINUTE"),
        boot_input_line(12, "N"),
        boot_print_line(13, "NAME"),
        boot_input_line(14, "A", True),
        boot_print_line(15, "GENDER F/N"),
        boot_input_line(16, "G", True),
    ]
    load_line = usrlines("RTBASE16", False)[0]
    lines.append(be16(17) + load_line[2:])
    return lines

'''
s = read(p)
if "def boot_input_program():" not in s:
    marker = "\ndef build_boot(code):"
    if marker not in s:
        raise SystemExit("Cannot locate build_boot for ROM INPUT program")
    s = s.replace(marker, helper + marker, 1)
old = '''    lines = [be16(1) + le16(l1) + body1 + b"\\x76",
             usrlines("RTBASE16", False)]'''
new = '''    lines = [be16(1) + le16(l1) + body1 + b"\\x76"] + boot_input_program()'''
s = replace_once(s, old, new, "boot ROM INPUT line sequence")
write(p, s)

print("PIOCO 16K ROM INPUT preparation: PASS")
print("  BASIC INPUT lines: YEAR, MONTH, DAY, HOUR, MINUTE, NAME, GENDER")
print("  ZX81 D-FILE: 24 rows / 793 bytes")
print("  HAL: Z88DK in_Inkey + in_WaitForNoKey")
print("  chat: initialized, 48-token reply limit")
