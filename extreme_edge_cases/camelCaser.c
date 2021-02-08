/**
 * extreme_edge_cases
 * CS 241 - Spring 2021
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    if (!input_str) {
        return NULL;
    }
    //initializing space for each array;
    char** outputs_s = malloc(sizeof(char*));
    int numChar = 0;
    int numSen = 0;
    int iter = 0;
    char c;
    while ((c = input_str[iter++])) {
        if (ispunct(c)) {
            outputs_s = (char**)realloc(outputs_s, (numSen + 1)*sizeof(char*));
            outputs_s[numSen] = calloc(numChar + 1, sizeof(char));
            outputs_s[numSen][numChar] = '\0';
            numChar = 0;
            numSen++;
        } else if (isspace(c)) {
            continue;
        } else {
            numChar++;
        }
    }
    outputs_s[numSen] = NULL;
    //fill out arrays
    numSen = 0;
    numChar = 0;
    char put;
    iter = 0;
    int capitalize = 0;
    int starting = 1;
    while((c = input_str[iter++])) {
        if (!outputs_s[numSen]) {
            break;
        }
        if (ispunct(c)) {
            capitalize = 0;
            starting = 1;
            numChar = 0;
            numSen++;
        } else if (isspace(c)) {
            capitalize = 1;
        } else {
            if (isalpha(c)) {
                if (capitalize && (!starting)) put = toupper(c);
                else put = tolower(c);
            } else {
                put = c;
            }
            outputs_s[numSen][numChar] = put;
            capitalize = 0;
            starting = 0;
            numChar++;
        }
    }
    return outputs_s;

}

void destroy(char **result) {
    // TODO: Implement me!
    for (size_t i = 0; i < (sizeof(result) / sizeof(char*)); i++) {
        free(result[i]);
    }
    free(result);
    return;
}