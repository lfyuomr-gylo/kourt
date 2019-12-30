#include <stdio.h>
#include <stdlib.h>

enum {
    BASE = 27,
    // log_27 (2^32) = 6.7
            MAX_SYMBOLS = 7
};

int
read_number_from_stdin()
{
    char text[MAX_SYMBOLS + 1];
    memset(text, 0, sizeof(text));
    read(0, text, MAX_SYMBOLS);
    return strtol(text, NULL, BASE);
}


int
main(int argc, char *argv[])
{
    double x;
    sscanf(argv[1], "%lf", &x);
    int y;
    sscanf(argv[2], "%x", &y);
    int z;

    z = strtol(argv[1], NULL, BASE);
    double result = x + y + z;
    printf("%.3f\n", result);
}
