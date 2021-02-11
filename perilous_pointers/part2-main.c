/**
 * perilous_pointers
 * CS 241 - Spring 2021
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    //
    int first = 81;
    first_step(first);
    //
    int* second = malloc(sizeof(int));
    *second = 132;
    second_step(second);
    free(second);
    //
    int third = 8942;
    int* third_ptr = & third;
    double_step(&third_ptr);
    //
    char* forth = malloc(sizeof(int) + 5);
    *(int *)(forth + 5) = 15;
    strange_step(forth);
    free(forth);
    //
    char* fif = malloc(4*sizeof(char));
    fif[3] = 0;
    empty_step((void*) fif);
    free(fif);
    //
    char* s2 = malloc(4*sizeof(char));
    s2[3] = 'u';
    two_step((void*)s2, s2);
    free(s2);
    //
    char* three = malloc(5*sizeof(char));
    three_step(three, three+2, three+4);
    free(three);
    //
    char* step = malloc(4*sizeof(char));
    step[1] = 0;
    step[2] = 8;
    step[3] = 16;
    step_step_step(step, step, step);
    free(step);
    //
    char g = 1;
    it_may_be_odd(&g, (int)g);
    //
    char c[] = "CS241,CS241";
    tok_step(c);
    //
    char i[] = {1, 2, 0, 0};
    the_end((void*)i, (void*)i);
    return 0;
}
