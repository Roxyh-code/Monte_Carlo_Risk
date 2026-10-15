#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

void  die(const char *msg);         /* perror + exit */
void *xmalloc(size_t size);         /* malloc that exits on failure */

/* Loop until all bytes are transferred; handle EINTR and short reads/writes. */
int read_all(int fd, void *buf, size_t count);
int write_all(int fd, const void *buf, size_t count);

#endif /* UTIL_H */
