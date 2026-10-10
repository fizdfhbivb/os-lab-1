#ifndef LAB_COMMON_H
#define LAB_COMMON_H
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    char data[4096];
    size_t size;
} Line;

static inline int write_all(int fd, const char *data, size_t size) {
    while (size > 0) {
        ssize_t n = write(fd, data, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return 0;
        data += n;
        size -= (size_t)n;
    }
    return 1;
}

static inline void message(const char *text) {
    size_t size = 0;
    while (text[size]) ++size;
    (void)write_all(STDERR_FILENO, text, size);
}

static inline size_t number_text(uint32_t number, char *text) {
    char reverse[10];
    size_t size = 0;
    do {
        reverse[size++] = (char)('0' + number % 10);
        number /= 10;
    } while (number);
    for (size_t i = 0; i < size; ++i) text[i] = reverse[size - i - 1];
    return size;
}

static inline void system_error(const char *operation) {
    int code = errno;
    char text[10];
    size_t size = number_text((uint32_t)code, text);
    message(operation);
    message(": errno ");
    (void)write_all(STDERR_FILENO, text, size);
    message("\n");
}

static inline int read_line(int fd, Line *line) {
    line->size = 0;
    for (;;) {
        char ch;
        ssize_t n = read(fd, &ch, 1);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) return -1;
        if (n == 0) break;
        if (ch == '\n') {
            if (line->size && line->data[line->size - 1] == '\r')
                --line->size;
            line->data[line->size] = 0;
            return 1;
        }
        if (line->size == sizeof(line->data) - 1) {
            errno = EMSGSIZE;
            return -1;
        }
        line->data[line->size++] = ch;
    }
    line->data[line->size] = 0;
    return line->size ? 1 : 0;
}

static inline int whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\v' || ch == '\f';
}

static inline int parse_integer(const Line *line, int32_t *value) {
    size_t i = 0;
    int sign = 1;
    int64_t number = 0;
    while (i < line->size && whitespace(line->data[i])) ++i;
    if (i < line->size && (line->data[i] == '+' || line->data[i] == '-')) {
        if (line->data[i] == '-') sign = -1;
        ++i;
    }
    size_t start = i;
    while (i < line->size && line->data[i] >= '0' && line->data[i] <= '9') {
        number = number * 10 + line->data[i++] - '0';
        if (number > 2147483648LL) return 0;
    }
    if (i == start) return 0;
    while (i < line->size && whitespace(line->data[i])) ++i;
    number *= sign;
    if (i != line->size || number < INT32_MIN || number > INT32_MAX) return 0;
    *value = (int32_t)number;
    return 1;
}

static inline int is_prime(int32_t number) {
    if (number < 2) return 0;
    for (int32_t d = 2; d <= number / d; ++d)
        if (number % d == 0) return 0;
    return 1;
}

static inline void close_fd(int *fd) {
    if (*fd >= 0 && close(*fd) < 0) system_error("close");
    *fd = -1;
}
#endif
