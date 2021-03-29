/**
 * parallel_make
 * CS 241 - Spring 2021
 */

// rule_t state specification:
// -1 fails
// 1 cycle detected
// 2 satisfied

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "vector.h"
#include "dictionary.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>

//variables:
graph* g = NULL;
size_t thread_count = 0;

//functions:
int is_cyclic(void* goal);
int is_cyclic_helper(void* goal, dictionary* d);
int check_and_run(void* goal);
int run_commands(rule_t* curr_rule);

// rule_t state specification:
// -1 fails
// 1 cycle detected
// 2 satisfied

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    //set up
    thread_count = num_threads;
    g = parser_parse_makefile(makefile, targets);
    vector* goals = graph_neighbors(g, "");

    //detect cycle
    int found_cycle = 0;
    for (size_t i = 0; i < vector_size(goals); i++) {
        char* curr = vector_get(goals, i);
        if (is_cyclic((void*)curr)) {
            print_cycle_failure(curr);
            rule_t* curr_rule = (rule_t*) graph_get_vertex_value(g, (void*)curr);
            curr_rule->state = 1;
            found_cycle = 1;
        }
    }
    //exit if has cycle
    if (found_cycle) {
        vector_destroy(goals);
        graph_destroy(g);
        return 0;
    }
    //check and run the commands sequentially; part2;
    for (size_t i = 0; i < vector_size(goals); i++) {
        char* curr = vector_get(goals, i);
        rule_t* curr_rule = (rule_t*) graph_get_vertex_value(g, (void*)curr);
        if (curr_rule->state != 1) {
            check_and_run((void*)curr);
        }
    }

    vector_destroy(goals);
    graph_destroy(g);   

    return 0;
}

int is_cyclic(void* goal) {
    //set up cycle detection
    dictionary* d = string_to_int_dictionary_create();
    int zero = 0;
    vector* vertices = graph_vertices(g);
    /*
    Vector iteration macro. `vecname` is the name of the vector. `varname` is the
    name of a temporary (local) variable to refer to each element in the vector,
    and `callback` is a block of code that gets executed once for each element in
    the vector until `break;` is called.
    */
   VECTOR_FOR_EACH(vertices, curr, {dictionary_set(d, curr, &zero);});
   vector_destroy(vertices);
   int exit = is_cyclic_helper(goal, d);
   dictionary_destroy(d);
   return exit;
}

int is_cyclic_helper(void* goal, dictionary* d) {
    if (!dictionary_contains(d, goal)) {
        return 0;
    }
    //visited;in progress; -> cycle detected
    if (*(int*)dictionary_get(d, goal) == 1) {
        return 1;
    }
    //finished
    if(*(int*)dictionary_get(d, goal) == 2) {
        return 2;
    }
    int one = 1;
    dictionary_set(d, goal, &one);

    vector* neighborhood = graph_neighbors(g, goal);
    for (size_t i = 0; i < vector_size(neighborhood); i++) {
        void* curr = vector_get(neighborhood, i);
        if (is_cyclic_helper(curr, d) == 1) {
            vector_destroy(neighborhood);
            return 1;
        }
    }
    int two = 2;
    dictionary_set(d, goal, &two);
    vector_destroy(neighborhood);
    return 0;
}

int check_and_run(void* goal) {
    vector* dependencies = graph_neighbors(g, goal);
    rule_t* curr_rule = (rule_t*) graph_get_vertex_value(g, goal);
    if (vector_size(dependencies) == 0) {
        if (access(goal, F_OK) == -1) {
            //not a file
            int exit = run_commands(curr_rule);
            vector_destroy(dependencies);
            return exit;
        }
    } else {
        if (access(goal, F_OK) != -1) {
            //is file
            for (size_t i = 0; i < vector_size(dependencies); i++) {
                void* curr = vector_get(dependencies, i);
                if (access(curr, F_OK) != -1) {
                    //if dependencies are file
                    struct stat stat_rule;
                    struct stat stat_depend;
                    // failed to read file's stat
                    if (stat(curr, &stat_depend) == -1 || stat((char *)goal, &stat_rule) == -1) {
                        vector_destroy(dependencies);
                        return -1;
                    }
                    // if dependency is newer than target, run command
                    if (difftime(stat_rule.st_mtime, stat_depend.st_mtime) < 0) {
                        int exit = run_commands(curr_rule);
                        vector_destroy(dependencies);
                        return exit;
                    }
                }
            }
        } else {
            //not a file
            for (size_t i = 0; i < vector_size(dependencies); i++) {
                void* curr = vector_get(dependencies, i);
                rule_t* dep_curr_rule = (rule_t*) graph_get_vertex_value(g, curr);
                if (dep_curr_rule->state == -1) {
                    curr_rule->state = -1;
                    vector_destroy(dependencies);
                    return -1;
                }
                if (dep_curr_rule -> state != 2) {
                    int result = check_and_run(curr);
                    if (result == -1) {
                        // set the state of current rule to failed
                        curr_rule -> state = -1;
                        vector_destroy(dependencies);
                        return -1;
                    }
                }
            }
            //all dependencies satisfies
            if (curr_rule -> state == -1) {
                vector_destroy(dependencies);
                return -1;
            }
            int exit = run_commands(curr_rule);
            vector_destroy(dependencies);
            return exit;
        }
    }
    vector_destroy(dependencies);
    return 0;
}

int run_commands(rule_t* curr_rule) {
    int failed = 0;
    vector* commands = curr_rule -> commands;
    for (size_t i = 0; i < vector_size(commands); i++) {
        if (system((char*)vector_get(commands, i)) != 0) {
            //if execution failed
            failed = 1;
            curr_rule -> state = -1;
            break;
        }
    }
    if (failed) {
        vector_destroy(commands);
        return -1;
    }
    curr_rule -> state = 2;
    return 1;
}
