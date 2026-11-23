#!/usr/bin/env python3
"""screen_sim.py - runs the real ZX81 front end (engine/zx81_main.c) on the host with a fake display file and a scripted
chat, then prints the 24x32 screen as text (inverse = lower case, 'BAD CELL' = a code that would break the ZX81 display).
Needs gcc. Usage: python3 tools/screen_sim.py"""
import re, subprocess, os, tempfile
E = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'engine')
s = open(os.path.join(E, 'zx81_main.c')).read()
s = re.sub(r'static void wait_frame\(void\) \{.*?\n\}\n', 'static void wait_frame(void) {}\n', s, flags=re.S)
s = re.sub(r'/\* ---- keyboard.*?static void draw_input', 'static uc lastk;\nstatic void draw_input', s, flags=re.S)
s = s.replace('dfile = (u8 *)*(uc *)16396;', 'dfile = host_df;').replace('*(uc *)16436', 'FAKE_F')
s = s.replace('typedef unsigned int uc;', 'typedef unsigned short uc;\nstatic u8 host_df[793]; static uc FAKE_F;')
s = re.sub(r'static void read_line\(char \*buf, uc max\) \{.*?\n\}\n', 'static int sidx; extern const char *script[];\nstatic void dump(void);\nstatic void read_line(char *buf, uc max) { const char *t = script[sidx++]; if (!t) { dump(); exit(0);} strcpy(buf, t); FAKE_F -= 100; }\n', s, flags=re.S)
s = s.replace('int main(void) {', 'int zx_main(void) {').replace('pico_reply(q, FAKE_F | 1);', 'pico_reply(q, FAKE_F | 1); FAKE_F -= 37;')
s += r'''
#include <stdlib.h>
const char *script[] = {"1349", "who made you", "I have a hedache", "what is the sun", "hello", "tell me about the stars", "are you a dog", 0};
static void dump(void) {
    int r, c;
    draw_input("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG AGAIN AND AGAIN XYZ", 60);
    for (r = 0; r < 24; r++) {
        char o[40];
        for (c = 0; c < 32; c++) {
            u8 v = host_df[1 + r*33 + c], inv = v & 128, z = v & 63; char ch;
            if (z == 0) ch = ' '; else if (z == 27) ch = '.'; else if (z == 26) ch = ','; else if (z == 15) ch = '?'; else if (z == 14) ch = ':';
            else if (z >= 28 && z <= 37) ch = '0' + z - 28; else if (z >= 38) ch = 'A' + z - 38; else ch = '#';
            if (inv) ch = (ch == ' ') ? '_' : (ch >= 'A' && ch <= 'Z' ? ch + 32 : ch);
            o[c] = ch;
            if ((v & 64) || v == 0x76) printf("[BAD CELL r%d c%d %02x]", r, c, v);
        }
        o[32] = 0;
        printf("%2d|%s|%s\n", r, o, host_df[1 + r*33 + 32] == 0x76 ? "" : "  <-- NO TERMINATOR");
    }
}
int main(void) { int i; for (i = 0; i < 24; i++) host_df[1+i*33+32] = 0x76; host_df[0] = 0x76; FAKE_F = 0xFFFF; zx_main(); return 0; }
'''
d = tempfile.mkdtemp(); p = os.path.join(d, 'zx_host.c'); open(p, 'w').write(s)
subprocess.check_call(['gcc', '-O0', '-w', '-DHOST', '-I' + E, '-o', os.path.join(d, 'sim'), p, os.path.join(E, 'pico.c')])
subprocess.call([os.path.join(d, 'sim')])
