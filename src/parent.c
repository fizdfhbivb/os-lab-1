#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include <signal.h>
#include <sys/wait.h>

static int get_reply(int fd, char *reply) {
    ssize_t n;
    do {
        n = read(fd, reply, 1);
    } while (n < 0 && errno == EINTR);
    if (n < 0) system_error("read reply");
    else if (n == 0) message("Child closed the reply pipe.\n");
    return n == 1;
}

int main(void) {
    int to_child[2] = {-1, -1}, from_child[2] = {-1, -1};
    int result = 1, state, status;
    pid_t child = -1;
    Line line;
    char filename[4096], path[4096], reply;
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        system_error("sigaction");
        goto finish;
    }
    message("Output filename, then one integer per line.\n");
    message("Prime/negative, exit or EOF stops the program.\n");
    state = read_line(STDIN_FILENO, &line);
    if (state < 0) {
        system_error("read filename");
        goto finish;
    }
    if (state == 0 || line.size == 0) {
        message("Filename is required.\n");
        goto finish;
    }
    for (size_t i = 0; i < line.size; ++i) {
        if (!line.data[i]) {
            message("Invalid filename.\n");
            goto finish;
        }
        filename[i] = line.data[i];
    }
    filename[line.size] = 0;
    ssize_t size = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (size < 0) {
        system_error("readlink");
        goto finish;
    }
    if ((size_t)size == sizeof(path) - 1) {
        message("Executable path is too long.\n");
        goto finish;
    }
    while (size > 0 && path[size - 1] != '/') --size;
    if ((size_t)size + 6 > sizeof(path)) {
        message("Executable path is too long.\n");
        goto finish;
    }
    const char name[] = "child";
    for (size_t i = 0; i < sizeof(name); ++i) path[size + (ssize_t)i] = name[i];
    if (pipe(to_child) < 0 || pipe(from_child) < 0) {
        system_error("pipe");
        goto finish;
    }
    child = fork();
    if (child < 0) {
        system_error("fork");
        goto finish;
    }
    if (child == 0) {
        if (dup2(to_child[0], STDIN_FILENO) < 0 ||
            dup2(from_child[1], STDOUT_FILENO) < 0) {
            system_error("dup2");
            _exit(1);
        }
        for (int i = 0; i < 2; ++i) {
            if (to_child[i] > STDERR_FILENO) close_fd(&to_child[i]);
            if (from_child[i] > STDERR_FILENO) close_fd(&from_child[i]);
        }
        char *args[] = {path, filename, NULL};
        execv(path, args);
        system_error("execv");
        _exit(1);
    }
    close_fd(&to_child[0]);
    close_fd(&from_child[1]);
    if (!get_reply(from_child[0], &reply) || reply != 'R') goto finish;
    while ((state = read_line(STDIN_FILENO, &line)) == 1) {
        if (line.size == 4 && line.data[0] == 'e' && line.data[1] == 'x' &&
            line.data[2] == 'i' && line.data[3] == 't') {
            result = 0;
            goto finish;
        }
        if (!write_all(to_child[1], line.data, line.size) ||
            !write_all(to_child[1], "\n", 1)) {
            system_error("write command");
            goto finish;
        }
        if (!get_reply(from_child[0], &reply)) goto finish;
        if (reply == 'S') {
            result = 0;
            goto finish;
        }
        if (reply != 'C') goto finish;
    }
    if (state < 0) system_error("read input");
    else result = 0;
finish:
    for (int i = 0; i < 2; ++i) {
        close_fd(&to_child[i]);
        close_fd(&from_child[i]);
    }
    if (child > 0) {
        pid_t waited;
        do {
            waited = waitpid(child, &status, 0);
        } while (waited < 0 && errno == EINTR);
        if (waited < 0) {
            system_error("waitpid");
            result = 1;
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) result = 1;
    }
    _exit(result);
}
