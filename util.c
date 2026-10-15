#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) die("malloc");
    return ptr;
}

int read_all(int fd, void *buf, size_t count) {
    size_t total = 0;
    char  *p     = (char *)buf;

    while (total < count) {
        ssize_t n = read(fd, p + total, count - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            return -1;
        }
        if (n == 0) {
            fprintf(stderr, "read_all: unexpected EOF (%zu/%zu bytes)\n",
                    total, count);
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

int write_all(int fd, const void *buf, size_t count) {
    size_t      total = 0;
    const char *p     = (const char *)buf;

    while (total < count) {
        ssize_t n = write(fd, p + total, count - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("write");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}
