/**
 * vector
 * CS 241 - Spring 2021
 */
#include "vector.h"
#include<stdio.h>
int main(int argc, char *argv[]) {
    // Write your test cases here
    vector* vec = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    vector_push_back(vec, "abc");
    vector_push_back(vec, "edf");
    vector_insert(vec, 1, "asdfe");
    vector_resize(vec, 10);
    vector_insert(vec, 0, "eedd");
    printf("%s\n", vector_get(vec, 0));
    printf("%s\n", vector_get(vec, 1));
    printf("%s\n", vector_get(vec, 2));
    printf("%s\n", vector_get(vec, 10));
    vector_destroy(vec);
    return 0;
}
