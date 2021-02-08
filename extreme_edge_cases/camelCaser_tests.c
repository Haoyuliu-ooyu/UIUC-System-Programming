/**
 * extreme_edge_cases
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    /*
    char* temp = "hello world.";
    char* expected = "helloWorld";
    char** result = camelCaser(temp);
    char* result_str = result[0];
    printf("expected:%s\n", expected);
    printf("result:%s\n", result_str);
    int to_return = !strcmp(expected, result_str);
    destroy(result);
    */
   char* tests[] = {
        "All the world’s a stage. and all the men and women merely players. They have their exits and their entrances. And one man in his time plays many parts.",
        ".",
        "123 test with numbers. 345test without space",
        ".start with punct.",
        "TeSt CapiTaliZe.",
        NULL
   };
   char* expected[15][15] = {
       {"allTheWorld’sAStage", "andAllTheMenAndWomenMerelyPlayers", "theyHaveTheirExitsAndTheirEntrances", "andOneManInHisTimePlaysManyParts"},
       {""},
       {"123TestWithNumbers", "345test without space"},
       {"", "startWithPunct"},
       {"testCapitalize"},
       {NULL}
   };
    int i = 0;
    char** c = tests;
    while (*c) {
        int j = 0;
        char** output = (*camelCaser)(*c);
        if (output == NULL && !strcmp(expected[i][j], "")) {
            continue;
        }
        while (output[j]) {
            printf("comparing: %s, %s\n", output[j], expected[i][j]);
            if (strcmp(output[j], expected[i][j])) {
                printf("output:%s\n", output[j]);
                printf("expected:%s\n", expected[i][j]);
                return 0;
            }
            printf("passed: %s, %s\n", output[j], expected[i][j]);
            j++;
        }
        (*destroy)(output);
        i++;
        c++;
    }

    return 1;
}
