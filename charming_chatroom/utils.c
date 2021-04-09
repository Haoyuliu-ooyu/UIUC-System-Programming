/**
 * charming_chatroom
 * CS 241 - Spring 2021
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
    ssize_t word_cunt = count;
    ssize_t total = 0;
    while (word_cunt > 0) {
        ssize_t result = read(socket, buffer + total, word_cunt);
        if (result > 0) {
            total += result;
            word_cunt -= result;
        }
        if (result == -1 && errno != EINTR) {
            break;
        }
        if (result == 0) {
            return 0;
        }
    }
    return count;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t word_cunt = count;
    ssize_t total = 0;
    while (word_cunt > 0) {
        ssize_t result = write(socket, buffer + total, word_cunt);
        if (result > 0) {
            total += result;
            word_cunt -= result;
        }
        if (result == -1 && errno != EINTR) {
            break;
        }
        if (result == 0) {
            return 0;
        }
    }
    return count;
}
