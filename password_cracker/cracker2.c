/**
 * password_cracker
 * CS 241 - Spring 2021
 */
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include <stdio.h>
#include <crypt.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;
static char* username;
static char* hash;
static char* password;
static char* recoverd_pw = NULL;
static int finished = 0;
static int found = 0;
static int count_all = 0;
static int num_thread;


void *crack_func(void* id){
    int tid = (long) id;
    char *password_strt = calloc(10, sizeof(char));
    while(1){
        pthread_barrier_wait(&barrier);
        if(finished) break;
        strcpy(password_strt, password);
        int count_temp = 0;
        long start_index = 0;
        long count = 0;

        int remain = strlen(password_strt) - getPrefixLength(password_strt);
        getSubrange(remain, num_thread, tid, &start_index, &count);
        setStringPosition(password_strt + getPrefixLength(password_strt), start_index);
        v2_print_thread_start(tid, username, start_index, password_strt);
        long i = 0;
        for (; i < count; i++){
            struct crypt_data cdata;
            cdata.initialized = 0;
            char *hash_v = crypt_r(password_strt, "xx", &cdata);
            count_temp++;
            //success thread print result, add count and then exit loop
            if(!strcmp(hash_v, hash)){
                pthread_mutex_lock(&m);
                strcpy(recoverd_pw, password_strt);
                found = 1;
                v2_print_thread_result(tid, count_temp, 0);
                count_all = count_all + count_temp;
                pthread_mutex_unlock(&m);
                break;
            }

            if(found){
                pthread_mutex_lock(&m);
                v2_print_thread_result(tid, count_temp, 1);
                count_all = count_all + count_temp;
                pthread_mutex_unlock(&m);
                break;
            }

            incrementString(password_strt);
        }

        pthread_barrier_wait(&barrier);

        if(!found){
            pthread_mutex_lock(&m);
            v2_print_thread_result(tid, count_temp, 2);
            count_all = count_all + count_temp;
            pthread_mutex_unlock(&m);
        }
        pthread_barrier_wait(&barrier);
    }
    free(password_strt);
    return NULL;
}


int start(size_t thread_count) {
    pthread_barrier_init(&barrier, NULL, thread_count+1);
    num_thread = thread_count;
    pthread_t ptd[thread_count];

    for (size_t i = 0; i < thread_count; i++){
        pthread_create(ptd+i, NULL, crack_func, (void*)(i+1));
    }

    username = calloc(10, sizeof(char));
    hash = calloc(15, sizeof(char));
    password = calloc(10,sizeof(char));
    recoverd_pw = calloc(16,sizeof(char));

    char *line = NULL;
    size_t len = 0;
    ssize_t nread = 0;
    while (1){   
        nread = getline(&line, &len, stdin);
        if(nread == -1){
            finished = 1;
            free(hash);
            free(password);
            free(username);
            free(recoverd_pw);
            free(line);
            pthread_barrier_wait(&barrier);
            break;
        }
        
        if (line[nread-1] == '\n') line[nread-1] = '\0';
        sscanf(line, "%s %s %s", username, hash, password);
        v2_print_start_user(username);

        double start_time = getTime();
        double start_cpu = getCPUTime();

        pthread_barrier_wait(&barrier);
        pthread_barrier_wait(&barrier);   
        pthread_barrier_wait(&barrier);

        double total_time = getTime() - start_time;
        double total_cpu = getCPUTime() - start_cpu;

        if(found){
            v2_print_summary(username, recoverd_pw, count_all,total_time, total_cpu, 0);  
        }else{
            v2_print_summary(username, NULL, count_all, total_time, total_cpu, 1);
        }
        found = 0;
        count_all = 0;
    }

    for (size_t j = 0; j < thread_count; j++) {
      pthread_join(ptd[j], NULL);
    }
    pthread_mutex_destroy(&m);
    pthread_barrier_destroy(&barrier);
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
