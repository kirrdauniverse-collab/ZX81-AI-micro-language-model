#include <stdio.h>
#include <string.h>
void pico_reply(const char *line, unsigned int sd);
extern unsigned char temperature;
int main(int argc, char **argv) {
    char buf[128]; unsigned int sd = 7;
    if (argc > 1) temperature = argv[1][0] - '0';
    while (fgets(buf, sizeof buf, stdin)) {
        buf[strcspn(buf, "\n")] = 0;
        printf("YOU> %s\nPICO> ", buf);
        pico_reply(buf, sd); sd = sd * 31 + 17;
    }
    return 0;
}
