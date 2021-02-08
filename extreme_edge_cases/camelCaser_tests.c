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
        "teSt CapitaLize. Up grade",
        "",
        NULL
   };
   char* expected[15][15] = {
       {"allTheWorld’sAStage", "andAllTheMenAndWomenMerelyPlayers", "theyHaveTheirExitsAndTheirEntrances", "andOneManInHisTimePlaysManyParts", NULL},
       {"", NULL},
       {"123TestWithNumbers", "345testWithoutSpace", NULL},
       {"", "", "", NULL},
       {"", "startWithPunct", NULL},
       {"testCapitalize", NULL},
       {"ayayaClap", NULL},
       {"testCapitalize", "upGrade", NULL},
       {NULL},
       {NULL}  
   };
    int i = 0;
    char** c = tests;
    while (*c) {
        int j = 0;
        char** output = (*camelCaser)(*c);
        if(!strcmp(*c, "")) {
            if (output[0] != NULL) return 0;
        }
        while (output[j]) {
            if (strcmp(output[j], expected[i][j])) {
                return 0;
            }
            j++;
        }
        i++;
        c++;
        (*destroy)(output);
    }

    return 1;
}
