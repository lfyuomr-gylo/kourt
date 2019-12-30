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

    for (size_t bytes_read = 0; bytes_read < MAX_SYMBOLS;) {
        ssize_t currently_read = read(0, text + bytes_read, MAX_SYMBOLS - bytes_read);
        if (currently_read <= 0) {
            break;
        } else {
            bytes_read += currently_read;
        }
    }

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
