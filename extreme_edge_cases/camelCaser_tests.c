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
   //test null
   char** t = (*camelCaser)(NULL);
   if (t != NULL) {
       return 0;
   }
   char* tests[] = {
        "All the world’s a stage. and all the men and women merely players. They have their exits and their entrances. And one man in his time plays many parts.",
        ".",
        "123 test with numbers. 345test without space.",
        " . . .",
        ".start with punct.",
        "TeSt CapiTaliZe.",
        "ayaya clap. pog pog no punct",
        "",
        NULL
   };
   char* expected[9][9] = {
       {"allTheWorld’sAStage", "andAllTheMenAndWomenMerelyPlayers", "theyHaveTheirExitsAndTheirEntrances", "andOneManInHisTimePlaysManyParts", NULL},
       {"", NULL},
       {"123TestWithNumbers", "345testWithoutSpace", NULL},
       {"", "", "", NULL},
       {"", "startWithPunct", NULL},
       {"testCapitalize", NULL},
       {"ayayaClap", NULL},
       {NULL},
       {NULL}  
   };
    int i = 0;
    char** c = tests;
    while (*c) {
        int j = 0;
        char** output = (*camelCaser)(*c);
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
        i++;
        c++;
        (*destroy)(output);
        printf("destroyed\n");
    }

    return 1;
}
