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
#include "queue.h"
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
queue* q = NULL;
dictionary* d = NULL;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
//number of finished thread
size_t thread_count = 0;
//functions:
int is_cyclic(void* goal);
int is_cyclic_helper(void* goal);
int check_and_run(void* goal);
int run_commands(rule_t* curr_rule);
void push_to_queue(char *target);
void* par_run(void* ptr);
// rule_t state specification:
// 0 fails
// -1 cycle detected
// -2 satisfied

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    //set up
    d = string_to_int_dictionary_create();
    g = parser_parse_makefile(makefile, targets);
    q = queue_create(-1);
    pthread_t pids[num_threads];
    vector* goals = graph_neighbors(g, "");
    //detect cycle
    for (size_t i = 0; i < vector_size(goals); i++) {
        char* curr = vector_get(goals, i);
        if (is_cyclic((void*)curr) == 1) {
            print_cycle_failure(curr);
            rule_t* curr_rule = (rule_t*) graph_get_vertex_value(g, (void*)curr);
            curr_rule->state = -1;
            vector_erase(goals, i);
            i--;
        }
    }
    //check and run the commands sequentially; part2;
    /*
    for (size_t i = 0; i < vector_size(goals); i++) {
        char* curr = vector_get(goals, i);
        rule_t* curr_rule = (rule_t*) graph_get_vertex_value(g, (void*)curr);
        if (curr_rule->state != -1) {
            check_and_run((void*)curr);
        }
    }
    */
    rule_t *root = graph_get_vertex_value(g, "");
    root->state = vector_size(goals);
    if (vector_empty(goals)) {return 0;}
    //push goals to the queue
    int zero = 0;
    vector* vertices = graph_vertices(g);
    VECTOR_FOR_EACH(vertices, curr, {dictionary_set(d, curr, &zero);});
    vector_destroy(vertices);
    VECTOR_FOR_EACH(goals, vtx, {push_to_queue(vtx);});
    //multi thread set up and calculation
    dictionary_destroy(d);
    for (size_t i = 0; i < num_threads; i++) {
        pthread_create(pids + i, NULL, par_run, NULL);
    }
    pthread_mutex_lock(&m);
    while (thread_count != vector_size(goals)) {
        pthread_cond_wait(&cond, &m);
    }
    pthread_mutex_unlock(&m);
    for (size_t i = 0; i < num_threads + 1; i++) {
        queue_push(q, NULL);
    }
    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(pids[i], NULL);
    }
    vector_destroy(goals);
    graph_destroy(g);   
    queue_destroy(q);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&m);
    return 0;
}


//detect cycles
int is_cyclic(void* goal) {
    //set up cycle detection
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
   int exit = is_cyclic_helper(goal);
   return exit;
}

int is_cyclic_helper(void* goal) {
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
        if (is_cyclic_helper(curr) == 1) {
            vector_destroy(neighborhood);
            return 1;
        }
    }
    int two = 2;
    dictionary_set(d, goal, &two);
    vector_destroy(neighborhood);
    return 0;
}
/*
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
*/

void push_to_queue(char *target) {
    if (*(int*)dictionary_get(d, target) == 1) return;
    int one = 1;
    dictionary_set(d, target, &one);
    vector* dependencies = graph_neighbors(g, target);
    //push for each item in dependencies
    VECTOR_FOR_EACH(dependencies, vt, {push_to_queue(vt);});
    if (vector_empty(dependencies)) queue_push(q, target);
    rule_t *rule = (rule_t *)graph_get_vertex_value(g, target);
    rule->state = vector_size(dependencies);
    vector_destroy(dependencies);
}

void* par_run(void* ptr) {
    while (1) {
        char* target = (char*)queue_pull(q);
        if (!target) {break;}
        rule_t* rule = (rule_t*)graph_get_vertex_value(g, target);
        int done = 1;
        int flag = 0;
        struct stat stat_inf;
        if (stat(rule->target, &stat_inf) == -1) {flag = 1;}
        if (flag == 0) {
            pthread_mutex_lock(&m);
            vector* dependencies = graph_neighbors(g, target);
            pthread_mutex_unlock(&m);
            VECTOR_FOR_EACH(dependencies, vtx, {
                rule_t* temp_rule = (rule_t*)graph_get_vertex_value(g, vtx);
                struct stat stat_temp;
                if (stat(temp_rule->target, &stat_temp) == -1 || stat_temp.st_mtime > stat_inf.st_mtime) {
                    flag = 1;
                    break;
                }
            });
            vector_destroy(dependencies);
        }
        if (flag) {
            VECTOR_FOR_EACH(rule->commands, vtx, {
                if (system(vtx) != 0) {
                    done = 0;
                    break;
                }
            });
        }
        //
        pthread_mutex_lock(&m);
        vector* anti = graph_antineighbors(g, target);
       VECTOR_FOR_EACH(anti, vtx, {
            if (done) {
                rule_t *rule_temp = graph_get_vertex_value(g, vtx);
                rule_temp->state -= 1;
                if (rule_temp->state == 0) queue_push(q, vtx);
            }
            if (!strcmp(vtx, "")) {
                thread_count++;
                pthread_cond_signal(&cond);
            }
        });
        pthread_mutex_unlock(&m);
        vector_destroy(anti);
    }
    return ptr;
}
