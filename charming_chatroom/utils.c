/**
 * charming_chatroom
 * CS 241 - Spring 2021
 partner: peiyuan3, xinshuo3
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message) {
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket) {
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket) {
    // Your code here
    ssize_t fixed_size = htonl(size);
    return write_all_to_socket(socket, (char*)&fixed_size , MESSAGE_SIZE_DIGITS);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    ssize_t read_count = 0;
    while (read_count < (ssize_t) count) {
        ssize_t num_read = read(socket, buffer + read_count, count - read_count);
        if (num_read == 0) {
            break;
        } else if (num_read > 0) {
            read_count += num_read;
        } else if (num_read == -1 && errno != EINTR) {
            return -1;
        }
    }
    return read_count;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t write_count = 0;
    while (write_count < (ssize_t) count) {
        ssize_t num_write = write(socket, buffer + write_count, count - write_count);
        if (num_write == 0) {
            break;
        } else if (num_write > 0) {
            write_count += num_write;
        } else if (num_write == -1 && errno != EINTR) {
            return -1;
        }
    }
    return write_count;
}
