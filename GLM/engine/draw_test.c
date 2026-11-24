#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef unsigned int uc;
static uc has(const char *h, const char *n) { uc i, j; for (i = 0; h[i]; i++) { for (j = 0; n[j]; j++) { uc c = h[i + j]; if (c >= 'a' && c <= 'z') c -= 32; if (c != n[j]) break; } if (!n[j]) return 1; } return 0; }
#include "draw_core.c"
int main(void) {
    const char *cmds[] = {"draw a circle left", "draw a box next to it", "draw a small triangle right bottom", "bigger", "draw a line", "draw a house", 0};
    uc k, r, c, m, bad = 0; u8 out[512];
    for (k = 0; cmds[k]; k++) {
        cmd(cmds[k], 10 + k * 7);
        printf("> %s -> %s\n", cmds[k], dmsg);
    }
    render(out);
    for (r = 0; r < 16; r++) {
        char l1[65], l2[65];
        for (c = 0; c < 32; c++) {
            u8 v = out[r * 32 + c];
            if ((v & 64) || v == 0x76) bad++;
            m = v < 8 ? v : 15 - (v - 128);
            l1[2*c] = m & 1 ? '#' : '.'; l1[2*c+1] = m & 2 ? '#' : '.'; l2[2*c] = m & 4 ? '#' : '.'; l2[2*c+1] = m & 8 ? '#' : '.';
        }
        l1[64] = l2[64] = 0; puts(l1); puts(l2);
    }
    printf("bad cells: %u\n", bad);
    return 0;
}
