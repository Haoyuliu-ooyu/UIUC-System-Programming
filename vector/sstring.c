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
    vector* vec;
};
sstring* sstring_create() {
    sstring* sstr = malloc(sizeof(sstring*));
    sstr->vec = vector_create(char_copy_constructor, char_destructor, char_default_constructor);
    return sstr;
}


sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    assert(input != NULL);
    sstring* sstr = sstring_create();
    char c;
    char* cpy;
    strcpy(cpy, input);
    while((c = cpy++)) {
        vector_push_back(sstr->vec, c);
    }
    free(cpy);
    return sstr;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    assert(input != NULL);
    char* to_return = malloc(sizeof(char) * vector_size(input->vec));
    for (size_t i = 0; i < vector_size(input->vec); i++) {
        to_return[i] = vector_at(input->vec, i);
    }
    return to_return;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    assert(this != NULL && addition != NULL);
    for (size_t i = 0; i < vector_size(addition->vec); i++) {
        vector_push_back(this->vec, vector_at(addition->vec, i));
    }
    return vector_size(this->vec);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    vector* vectors = vector_create(string_copy_constructor, string_destructor, string_default_constructor);

    for (size_t i = 0; i < vector_size(this->vec); i++) {
    }

    return NULL;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    assert(offset >= 0);
    for (size_t i = offset; i < vector_size(this->vec); i++) {
        if (vector_at(this->vec, i) == target[0]) {
            
        }
    }
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    return NULL;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this != NULL);
    vector_destroy(this->vec);
    free(this);
}
