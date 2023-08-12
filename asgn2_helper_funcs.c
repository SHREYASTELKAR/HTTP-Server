#include "asgn2_helper_funcs.h"
#include "parser.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

int listener_init(Listener_Socket *sock, int port) {
    sock = (Listener_Socket *) malloc(sizeof(Listener_Socket));
    if (sock && port > 1 && port < 65535) {
        sock->fd = port;
        return 0;
    }
    return -1;

    //if return is 1 send invalid msg
}

int listener_accept(Listener_Socket *sock) {
    if (sock->fd >= 0) {
        return sock->fd;
    }
    return -1; //check errno
}

ssize_t read_until(int in, char buf[], size_t nbytes, char *string) {

    //need to check for exactly nbytes

    ssize_t bytes_read = 0;
    size_t total = 0;

    while ((bytes_read = read(in, buf, nbytes - total)) > 0) {
        if (errno == EAGAIN | errno == EWOULDBLOCK) { //timeout
            //write(STDERR_FILENO, invalid, strlen(invalid));
            return -1;
        }

        total += bytes_read;
        if (total == nbytes) { //reads n bytes
            return total;
        }

        if (strstr(buf, string) != NULL) { //substring found
            break;
        }
    }
    return total;
}

ssize_t write_all(int in, char buf[], size_t nbytes) {
    ssize_t bytes_written = 0;
    size_t total = 0;

    while ((bytes_written = write(in, buf, nbytes - total)) > 0) {
        if (errno == EAGAIN | errno == EWOULDBLOCK) {
            return -1;
        }
        total += bytes_written;
        if (total == nbytes) { //reads n bytes
            return total;
        }
    }
    return total;
}

ssize_t pass_bytes(int src, int dst, size_t nbytes) {
    size_t bytes_written = 0;
    ssize_t bytes_read = 0;
    ssize_t total_w = 0;
    char buf[4096];
    memset(&buf, 0, 4096);

    while ((bytes_read = read(src, buf, nbytes - total_w)) > 0) {
        if (errno == EINTR) {
            return -1;
        }

        bytes_written = write_all(dst, buf, bytes_read);
        total_w += bytes_written;

        if (bytes_written == nbytes) {
            break;
        }
    }
    return total_w;
}
