//partner: peiyuan3, xinshuo3
/**
 * utilities_unleashed
 * CS 241 - Spring 2021
 */
#include "format.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (argc < 2) {
        print_time_usage(); 
    }
    pid_t f = fork();
    if (f == -1) {
        print_fork_failed();
    } else if (f == 0) {
        execvp(argv[1], argv + 1);
        print_exec_failed();
    } else {
        int s = 0;
        waitpid(f, &s, 0);
        if (WIFEXITED(s) && WEXITSTATUS(s) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            display_results(argv, (end.tv_nsec - start.tv_nsec)/1e9 + (end.tv_sec - start.tv_sec));
        }
    }
    return 0;
}
