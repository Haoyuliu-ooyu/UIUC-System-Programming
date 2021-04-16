/**
 * deepfried_dd
 * CS 241 - Spring 2021
 * partner: xinshuo3, peiyuan3
 */
#include "format.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

static int print_stat = 0;
void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        print_stat = 1;
    }
}


int main(int argc, char **argv) {
    signal(SIGUSR1, signal_handler);
    int opt = 0;
    FILE* fp_in;
    FILE* fp_out;
    long num_block_skip_in = 0;
    long num_block_skip_out = 0;
    long num_block_copy = 0;
    long block_size = 0;
    while((opt = getopt(argc, argv, "i:o:b:c:p:k:")) != -1) {
        switch(opt) {
            case 'i':
                fp_in = fopen(optarg, "r");
                if (fp_in == NULL) {
                    print_invalid_input(optarg);
                    exit(1);
                }/*
                size_t i = 0;
                char* linptr;
                if (getline(&linptr, &i, fp_in) != -1) {
                    puts(linptr); //print the line 
                }*/

                
                fclose(fp_in);
                continue;
            case 'o':
                fp_out = fopen(optarg, "w+");
                if (fp_out == NULL) {
                    print_invalid_output(optarg);
                    exit(1);
                }
                continue;
                
                
                
            case 'b':
                block_size = atol(optarg);
                continue;
            case 'c':
                num_block_copy = atol(optarg);
                continue;

            case 'p':
                num_block_skip_in = atol(optarg);
                continue;
            case 'k':
                num_block_skip_out = atol(optarg);
                continue;

            case '?':
                exit(1);
        }
    }
    fseek(fp_in, num_block_skip_in, SEEK_SET);
    long start = ftell(fp_in);
    fseek(fp_in, 0, SEEK_END);
    long end = ftell(fp_in);
    long file_size = end - start;
    fseek(fp_in, num_block_skip_in, SEEK_SET);
    fseek(fp_out, num_block_skip_out, SEEK_SET);
    clock_t before = clock();
    size_t full_blocks_in = 0;
    size_t partial_blocks_in = 0;
    size_t copy_size = 0;
    while (!feof(fp_in)) {
        if (partial_blocks_in + full_blocks_in == (unsigned long) num_block_copy) {
            break;
        }
        if (print_stat) {
            clock_t diff_ = clock() - before;
            double time_elapsed_ = 1000* diff_ / CLOCKS_PER_SEC;
            time_elapsed_ /= 1000;

            print_status_report(full_blocks_in, partial_blocks_in,
                        full_blocks_in, partial_blocks_in,
                        copy_size, time_elapsed_);
        }
        char buffer[block_size];
        if (fread((void*) buffer, block_size, 1, fp_in) == 1) {
            fwrite((void*) buffer, block_size, 1, fp_out);
            full_blocks_in++;
            copy_size += block_size;
        } else {
            partial_blocks_in++;
            fwrite((void*) buffer, end - ftell(fp_in), 1, fp_out);
            copy_size += end - ftell(fp_in);
        }
        
        fseek(fp_in, block_size, SEEK_CUR);
        fseek(fp_out, block_size, SEEK_CUR);
        
    }
    clock_t diff = clock() - before;
    double time_elapsed = 1000* diff / CLOCKS_PER_SEC;
    time_elapsed /= 1000;

    //size_t total_bytes_copied = block_size * num_block_copy;
    print_status_report(full_blocks_in, partial_blocks_in,
                        full_blocks_in, partial_blocks_in,
                        file_size, time_elapsed);
    return 0;
}