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

int error_dectect(size_t size1, size_t size2) {
  if (size1 == 0 && size1 != size2) {
    print_connection_closed();
    return 1;
  } 
  if (size1 < size2) {
    print_too_little_data();
    return 1;
  }
  if (size1 > size2) {
    print_received_too_much_data();
    return 1;
  }
  return 0;
}

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

size_t read_head(int socket, char *buffer, size_t count){
	size_t r_count = 0;
	while (r_count < count) {
		ssize_t ret = read(socket, buffer + r_count, 1);
		if (ret == 0 || buffer[strlen(buffer) - 1] == '\n') {
			break;
		}
		if (ret == -1){
      if (errno == EINTR){
        continue;
      }else {
        perror("read_head()");
      }
    }
		r_count += ret;
	}
	return r_count;
}
