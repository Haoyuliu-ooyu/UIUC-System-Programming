/**
 * nonstop_networking
 * CS 241 - Spring 2021
 */
#include "common.h"
#include "common.h"
#include "format.h"
#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>

ssize_t read_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    size_t r_count = 0;
    while (r_count < count) {
      ssize_t ret = read(socket, buffer + r_count, count - r_count);
      if (ret == 0) break;
      if (ret == -1 && errno == EINTR) continue;
      if (ret == -1) {
        perror("read_from_socket");
        return -1;
      }
      r_count += ret;
    }
    return r_count;
}

ssize_t write_to_socket(int socket, const char *buffer, size_t count) {
    size_t w_count = 0;
    while (w_count < count) {
      ssize_t ret = write(socket, buffer + w_count, count - w_count);
      if (ret == 0) break;
      if (ret == -1 && errno == EINTR) continue;
      if (ret == -1) {
        perror("write_from_socket");
        return -1;
      }
      w_count += ret;
    }
    return w_count;
}
