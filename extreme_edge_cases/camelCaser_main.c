/**
 * extreme_edge_cases
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <stdlib.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int main() {
    // Feel free to add more test cases of your own!
    if (test_camelCaser(&camel_caser, &destroy)) {
        printf("SUCCESS\n");
    } else {
        printf("FAILED\n");
    }
}
