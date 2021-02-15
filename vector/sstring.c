/**
 * vector
 * CS 241 - Spring 2021
 */
#include "sstring.h"
#include "vector.h"
#include "callbacks.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    char* str;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring* sstr = malloc(sizeof(sstring));
    char* cpy = malloc(strlen(input) + 1);
    strcpy(cpy, input);
    sstr->str = cpy;
    return sstr;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    return input->str;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    char* result = realloc(this->str, strlen(this->str) + strlen(addition->str) + 1);
    strcpy(result, this->str);
    strcat(result, addition->str);
    free(this->str);
    this->str = result;
    return strlen(result);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    
    return NULL;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    assert(offset >= 0);
    
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    return NULL;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this != NULL);
    free(this->str);
    free(this);
}
