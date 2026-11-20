#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;

    FILE *output = fopen(argv[1], "w");
    if (output == NULL) {
        perror(argv[1]);
        return 1;
    }

    int result = 0;
    for (;;) {
        if (write(STDOUT_FILENO, "+", 1) != 1) {
            perror("write reply");
            result = 1;
            break;
        }

        int number;
        ssize_t length = read(STDIN_FILENO, &number, sizeof number);
        if (length == 0)
            break;
        if (length != (ssize_t)sizeof number) {
            if (length < 0)
                perror("read number");
            else
                fputs("Incomplete number.\n", stderr);
            result = 1;
            break;
        }

        int prime = number > 1;
        for (int d = 2; prime && d <= number / d; ++d)
            if (number % d == 0)
                prime = 0;

        if (number < 0 || prime)
            break;
        if (number > 1 &&
            (fprintf(output, "%d\n", number) < 0 || fflush(output) == EOF)) {
            perror("write output file");
            result = 1;
            break;
        }
    }

    if (fclose(output) == EOF) {
        perror("fclose output file");
        return 1;
    }
    return result;
}
