/**
 * finding_filesystems
 * CS 241 - Spring 2021
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Write tests here!
    file_system *fs = open_fs("test.fs");
    off_t off = 0;
    char* write_content = "Hello Wolrd!";
    size_t bytes_written = minixfs_write(fs, "/goodies/non.txt", write_content, 14, &off);
    printf("Write to non-exist file | Bytes written: %zu\n", bytes_written);
    printf("Write to non-exist file | Final off: %zu\n", off);
    char result[15];
    off = 0;
    minixfs_read(fs, "/goodies/non.txt", result, 13, &off);
    printf("%s\n", result);
    close_fs(&fs);
}
