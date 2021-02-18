/**
 * vector
 * CS 241 - Spring 2021
 */
#include "sstring.h"



int main(int argc, char *argv[]) {
    // TODO create some tests
    sstring *sstr = cstr_to_sstring("ecfc");
    vector* vec = sstring_split(sstr, 'c');
    printf("%s\n", vector_get(vec, 0));
    printf("%s\n", vector_get(vec, 1));
    printf("%s\n", vector_get(vec, 2));
    return 0;
}
