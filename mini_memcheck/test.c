/**
 * mini_memcheck
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Your tests here using malloc and free
    void *p1 = malloc(30);
    void * p2 = malloc(40);
    void *p3 = realloc(p1, 20);
    free(p2);
    return 0;
}