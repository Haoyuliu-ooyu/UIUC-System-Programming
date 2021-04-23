/**
 * partners: peiyuan3, xinshuo3
 * mad_mad_access_patterns
 * CS 241 - Spring 2021
 
 */
#include "tree.h"
#include "utils.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

int find(long offset, char* buffer, char* word) {
  if(offset == 0) {
    return 0;
  } 
  BinaryTreeNode* n = (BinaryTreeNode *) (buffer + offset);
  if (!strcmp(word, n->word)){
     printFound(n->word, n->count, n->price);
     return 1;
  }
  if (strcmp(word, n->word) < 0) {
    if (find(n->left_child, buffer, word)){
       return 1;
    }
  }
  if (strcmp(word, n->word) > 0) {
    if (find(n->right_child, buffer, word)){
       return 1;
    }
  }
  return 0;
}
int main(int argc, char **argv) {
    if (argc < 3) {
      printArgumentUsage();
      exit(1);
    }
    char* file_name = argv[1];
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
      openFail(file_name);
      exit(2);
    }
    struct stat s;
    if (fstat(fd, &s)) {
      openFail(file_name);
      exit(2);
    }
    char* buffer = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == (void*) -1) {
      mmapFail(file_name);
      exit(2);
    }
    if (strncmp(buffer, "BTRE", 4)) {
      formatFail(file_name);
      exit(2);
    }
    int i = 2;

    for (; i < argc; i++) {
      int check = find((long) 4, buffer, argv[i]);
      if (!check) {
        printNotFound(argv[i]);
      }
    }
    close(fd);
    return 0;
}