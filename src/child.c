#include "common.h"
#include <fcntl.h>

int main(int argc, char **argv) {
    int file = -1, result = 1, state;
    Line line;
    if (argc != 2) {
        message("Usage: child OUTPUT_FILE\n");
        goto finish;
    }
    file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file < 0) {
        system_error("open");
        goto finish;
    }
    if (!write_all(STDOUT_FILENO, "R", 1)) {
        system_error("write reply");
        goto finish;
    }
    while ((state = read_line(STDIN_FILENO, &line)) == 1) {
        int32_t number;
        if (!parse_integer(&line, &number)) {
            message("Expected one signed 32-bit integer per line.\n");
            goto finish;
        }
        if (number < 0 || is_prime(number)) {
            if (!write_all(STDOUT_FILENO, "S", 1)) {
                system_error("write stop");
                goto finish;
            }
            result = 0;
            goto finish;
        }
        if (number > 1) {
            char text[11];
            size_t size = number_text((uint32_t)number, text);
            text[size++] = '\n';
            if (!write_all(file, text, size)) {
                system_error("write file");
                goto finish;
            }
        }
        if (!write_all(STDOUT_FILENO, "C", 1)) {
            system_error("write reply");
            goto finish;
        }
    }
    if (state < 0) system_error("read");
    else result = 0;
finish:
    if (result) (void)write_all(STDOUT_FILENO, "E", 1);
    if (file >= 0 && close(file) < 0) {
        system_error("close file");
        result = 1;
    }
    _exit(result);
}
