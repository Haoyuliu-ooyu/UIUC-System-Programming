/**
 * password_cracker
 * CS 241 - Spring 2021
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "./includes/queue.h"
#include "thread_status.h"
#include <stdio.h>
#include <string.h>
#include <crypt.h>
#include <pthread.h>

typedef struct cracker_t {
    pthread_t tid;
    size_t index;
} cracker_t;

static int recover = 0;
static int count = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static cracker_t* crackers = NULL;
static queue* q = NULL;

void* crack_func(void* data) {
    cracker_t* c = (cracker_t*)data;
    char* to_solve = NULL;
    char username[16], hash[16], password[16];
    while ((to_solve = queue_pull(q)) != NULL) {
        sscanf(to_solve, "%s %s %s", username, hash, password);
        v1_print_thread_start(c->index, username);
        double start_time = getThreadCPUTime();
        int hash_count = 0;
        int found = 0;
        if (getPrefixLength(password) == (int)strlen(password)) {
            found = 1;
        } else {
            int suffix_length = (int)strlen(password) - getPrefixLength(password);
            struct crypt_data cdata;
            cdata.initialized = 0;
            setStringPosition(password + getPrefixLength(password), 0);
            int range = 1;
            for (int i = 0; i < suffix_length; i++) {
                range *= 26;
            }
            for (int i = 0; i < range; i++) {
                hash_count++;
                if (strcmp(crypt_r(password, "xx", &cdata), hash) == 0) {
                    // update recovered password counter
                    pthread_mutex_lock(&m);
                    recover++;
                    pthread_mutex_unlock(&m);
                    found = 1;
                    break;
                }
                incrementString(password);
            }
        }
        v1_print_thread_result(c->index, username, password, 
                        hash_count, getThreadCPUTime() - start_time, !found);
        free(to_solve);
    }
    queue_push(q, NULL);
    return NULL;
}



int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    q = queue_create(0);
    char* line = NULL;
    size_t size = 0;
    while (1) {
        int flag = getline(&line, &size, stdin);
        if(flag == -1) {break;}
        if(line[strlen(line) -1] =='\n') {line[strlen(line) -1] = '\0';}
        queue_push(q, strdup(line));
        count++;
    }
    queue_push(q, NULL);
    free(line);
    crackers = malloc(sizeof(cracker_t) * thread_count);
    for (size_t i = 0; i < thread_count; i++) {
        crackers[i].index = i+1;
        pthread_create(&(crackers[i].tid), NULL, crack_func, crackers+i);
    }
    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(crackers[i].tid, NULL);
    }
    v1_print_summary(recover, count - recover);
    queue_destroy(q);
    free(crackers);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
