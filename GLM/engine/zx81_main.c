/* ZX81 front end for Pico AI (first version: plain console chat, no printf) */
#include <stdio.h>
void pico_reply(const char *line, unsigned int sd);
static char buf[64];
static void puts_c(const char *s) { while (*s) fputc_cons(*s++); }
int main(void) {
    unsigned int *frames = (unsigned int *)16436;
    puts_c("PICO AI  ZX81 16K\nTYPE IN ENGLISH. ENTER SENDS.\n\n");
    for (;;) {
        puts_c("YOU> ");
        fgets_cons(buf, 60);
        puts_c("\nPICO> ");
        pico_reply(buf, *frames | 1);
        puts_c("\n");
    }
    return 0;
}
