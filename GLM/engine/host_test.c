#include <stdio.h>
#include <string.h>
void pico_reply(const char *line, unsigned int sd);
extern char rbuf[];
extern unsigned char temperature;
void think_cb(unsigned int a, unsigned int b, unsigned int c) {}
int main(int argc, char **argv) {
    char buf[128]; unsigned int sd = 7;
    if (argc > 1) temperature = argv[1][0] - '0';
    while (fgets(buf, sizeof buf, stdin)) {
        buf[strcspn(buf, "\n")] = 0;
        printf("YOU> %s\nPICO> ", buf);
        pico_reply(buf, sd); puts(rbuf); sd = sd * 31 + 17;
    }
    return 0;
}
