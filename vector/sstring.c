/**
 * vector
 * CS 241 - Spring 2021
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    char* str;
    size_t len;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring* sstr = malloc(sizeof(sstring));
    sstr->str = malloc(strlen(input) + 1);
    strcpy(sstr->str, input);
    return sstr;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    char* cpy = malloc(strlen(input->str) + 1);
    strcpy(cpy, input->str);
    return cpy;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    this->str = realloc(this->str, strlen(this->str) + strlen(addition->str) + 1);
    strcat(this->str, addition->str);
    printf("%s\n", this->str);
    return strlen(this->str);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    if (!this) {
        return NULL;
    }
    vector* vec = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    char* c = this->str;
    while (c < this->str + strlen(this->str)) {
        char* stop = strchr(c, delimiter);
        if (!stop) {
            vector_push_back(vec, c);
            return vec;
        }
        char temp = *stop;
        *stop = '\0';
        vector_push_back(vec, c);
        *stop = temp;
        c = stop + 1;
    }
    if (*(this->str + strlen(this->str) - 1) == delimiter) {
        vector_push_back(vec, "");
    }
    return vec;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    if (offset > strlen(this->str)) {
        return -1;
    }
    char* c = strstr(this->str + offset, target);
    if (c != NULL) {
        char *temp = malloc(strlen(this->str) + strlen(substitution) - strlen(target) + 1);
        strncpy(temp, this->str, c - this->str);
        strcpy(temp + (c-this->str), substitution);
        strcpy(temp + strlen(substitution) + ((c-this->str)), strlen(target) + c);
        free(this->str);
        this->str = temp;
        printf("%s\n", this->str);
        return 0;
    } else {
        return -1;
    }
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char * c = malloc(end - start + 1);
    strncpy(c, this->str + start, end - start);
    c[end - start] = '\0';
    return c;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    if (!this) {
        return;
    }
    free(this->str);
    free(this);
}
