/**
 * extreme_edge_cases
 * CS 241 - Spring 2021
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    if (!input_str) {
        return NULL;
    }
    int numSen = 0;
    int numChar = 0;
    int iter = 0;
    char c;
    printf("reached first\n");
    char** outputs_s = calloc(1, sizeof(char*));

    //initializing space for each array;
    numSen = 0;
    iter = 0;

    while ((c = input_str[iter++])) {
        if (ispunct(c)) {
            outputs_s = realloc(outputs_s, (numSen + 1)*sizeof(char*));
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
    printf("reached second\n");
    //fill out arrays
    numSen = 0;
    numChar = 0;
    char put;
    iter = 0;
    bool capitalize = false;
    bool starting = true;
    while((c = input_str[iter++])) {
        if (!outputs_s[numSen]) {
            break;
        }
        if (ispunct(c)) {
            printf("reached a\n");
            capitalize = 0;
            starting = 1;
            numChar = 0;
            numSen++;
        } else if (isspace(c)) {
            printf("reached b\n");
            capitalize = 1;
        } else {
            if (isalpha(c)) {
                printf("reached c\n");
                printf("%c\n", c);
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
    printf("reached thrid\n");
    printf("%s\n", outputs_s[0]);
    return outputs_s;

}

void destroy(char **result) {
    // TODO: Implement me!
    for (int i = 0; i < (int)(sizeof(result) / sizeof(result[0])); i++) {
        free(result[i]);
    }
    free(result);
    printf("destroyed\n");
    return;
}