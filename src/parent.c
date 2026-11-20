#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void fail(const char *operation)
{
    perror(operation);
    _exit(1);
}

int parent(void)
{
    char *line = NULL;
    size_t capacity = 0;
    if (getline(&line, &capacity, stdin) < 0) {
        if (!feof(stdin))
            fail("getline");
        fputs("Enter the output file name.\n", stderr);
        free(line);
        return 1;
    }
    char *filename_end = line;
    while (*filename_end && *filename_end != '\r' && *filename_end != '\n')
        ++filename_end;
    *filename_end = '\0';

    int numbers[2], reply[2];
    if (pipe(numbers) < 0 || pipe(reply) < 0)
        fail("pipe");
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        fail("signal");

    pid_t child = fork();
    if (child < 0)
        fail("fork");
    if (child == 0) {
        close(numbers[1]);
        close(reply[0]);
        if (dup2(numbers[0], STDIN_FILENO) < 0 ||
            dup2(reply[1], STDOUT_FILENO) < 0)
            fail("dup2");
        if (numbers[0] > STDOUT_FILENO)
            close(numbers[0]);
        if (reply[1] > STDOUT_FILENO)
            close(reply[1]);
        execl("./child", "child", line, (char *)NULL);
        fail("execl");
    }

    close(numbers[0]);
    close(reply[1]);
    int result = 0;
    char ready;
    ssize_t received;
    while ((received = read(reply[0], &ready, 1)) == 1) {
        ssize_t length = getline(&line, &capacity, stdin);
        if (length < 0) {
            if (!feof(stdin)) {
                perror("getline");
                result = 1;
            }
            break;
        }

        char *end;
        errno = 0;
        long value = strtol(line, &end, 10);
        const char *tail = end;
        while (*tail == ' ' || *tail == '\t' || *tail == '\r' ||
               *tail == '\n' || *tail == '\v' || *tail == '\f')
            ++tail;
        if (end == line || errno == ERANGE || value < INT_MIN || value > INT_MAX ||
            tail != line + length) {
            fputs("Enter one integer per line.\n", stderr);
            result = 1;
            break;
        }

        int number = (int)value;
        if (write(numbers[1], &number, sizeof number) != (ssize_t)sizeof number) {
            perror("write number");
            result = 1;
            break;
        }
    }
    if (received < 0) {
        perror("read reply");
        result = 1;
    }
    free(line);
    close(numbers[1]);
    close(reply[0]);

    int status;
    while (waitpid(child, &status, 0) < 0)
        if (errno != EINTR)
            fail("waitpid");

    return result || !WIFEXITED(status) || WEXITSTATUS(status) != 0;
}
