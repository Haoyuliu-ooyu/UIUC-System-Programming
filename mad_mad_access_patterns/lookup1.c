/**
 * partner: peiyuan3, xinshuo3
 * mad_mad_access_patterns
 * CS 241 - Spring 2021
 */
#include "tree.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/
int find(FILE* fp, char* word, long offset) {
  if (offset == 0) {
    return 0;
  }
  BinaryTreeNode n;
  fseek(fp, offset, SEEK_SET);
  fread(&n, sizeof(BinaryTreeNode), 1, fp);
  char c;
  int word_length = 0;
  while ((c = fgetc(fp)) != '\0' && c != EOF) {
    word_length++;
  }
  fseek(fp, offset + sizeof(BinaryTreeNode), SEEK_SET);
  char node_word[word_length];
  fread(node_word, word_length, 1, fp);
  if (strcmp(word, node_word) == 0){
      printFound(node_word, n.count, n.price);
      return 1;
    }
    if (strcmp(word, node_word) < 0){
      if (find(fp,  word, n.left_child)){
        return 1;
      }
    }
    if (strcmp(word, node_word) > 0){
      if (find(fp,  word, n.right_child)){
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
    FILE* fp = fopen(file_name, "r");
    if (!fp) {
      openFail(file_name);
      exit(2);
    }
    char buffer[4];
    fread(buffer, 1, 4, fp);
    if (strcmp(buffer, "BTRE")) {
      formatFail(file_name);
      exit(2);
    }
    int i = 2;
    for (; i < argc; i++) {
      int check = find(fp, argv[i], (long)4 );
      if (!check) {
        printNotFound(argv[i]);
      }
    }
    fclose(fp);

    
    return 0;
}
