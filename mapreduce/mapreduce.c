/**
 * mapreduce
 * CS 241 - Spring 2021
 // partner xinshuo3, peiyuan3
 */
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    // Create an input pipe for each mapper.
    char* input_file = argv[1];
    char* output_file = argv[2];
    char* mapper = argv[3];
    char* reducer = argv[4];
    int mapper_count;
    sscanf(argv[5], "%d", &mapper_count);
    int* fd[mapper_count];
    int i = 0;
    for (;i< mapper_count; i++) {
        fd[i] = calloc(2, sizeof(int));
        pipe(fd[i]);
    }
    // Create one input pipe for the reducer.
    int fd_reducer[2];
    pipe(fd_reducer);
    // Open the output file.
    int open_file = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, S_IWUSR | S_IRUSR);

    // Start a splitter process for each mapper.
    pid_t child_split[mapper_count];
    i = 0;
    for (; i< mapper_count; i++){
        child_split[i] = fork();
        if (child_split[i] == 0) { //child process
            close(fd[i][0]);
            char temp[16];
            sprintf(temp, "%d", i);
            dup2(fd[i][1], 1);
            execl("./splitter", "./splitter", input_file, argv[5], temp, NULL);
            exit(1);
        }
    }
    // Start all the mapper processes.
    pid_t child_mapping[mapper_count];
    i = 0;
    for (; i < mapper_count; i++) {
        close(fd[i][1]);
        child_mapping[i] = fork();
        if (child_mapping[i] == 0) {
            close(fd_reducer[0]);
            dup2(fd[i][0], 0);
            dup2(fd_reducer[1], 1);
            execl(mapper, mapper, NULL);
            exit(1);
        }
    }
    // Start the reducer process.
    close(fd_reducer[1]);
    pid_t child = fork();
    if (child == 0) {
        dup2(fd_reducer[0], 0);
        dup2(open_file, 1);
        execl(reducer, reducer, NULL);
        exit(1);
    }
    close(open_file);
    close(fd_reducer[0]);

    // Wait for the reducer to finish.
    i = 0;
    for (; i < mapper_count; i++) {
        int s;
        waitpid(child_split[i], &s, 0);
    } 
    i = 0;
    for (; i < mapper_count; i++) {
        close(fd[i][0]);
        int s;
        waitpid(child_mapping[i], &s, 0);
    }
    int s;
    waitpid(child, &s, 0);
    // Print nonzero subprocess exit codes.
    if (s) {
        print_nonzero_exit_status(reducer, s);
    }

    // Count the number of lines in the output file.
    print_num_lines(output_file);
    i = 0;
    for (; i< mapper_count; i++) {
        free(fd[i]);
    }
    return 0;
}