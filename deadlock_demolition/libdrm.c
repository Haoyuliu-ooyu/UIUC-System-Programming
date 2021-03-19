/**
partner: peiyuan3, xinshuo3
 * deadlock_demolition
 * CS 241 - Spring 2021
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>

static graph* g = NULL;
struct drm_t {
    pthread_mutex_t mutex;
};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
set* set_pro = NULL;

drm_t *drm_init() {
    /* Your code here */
    pthread_mutex_lock(&mutex);
    if (g == NULL) {
        g = shallow_graph_create();
    }
    drm_t* toReturn = malloc(sizeof(drm_t));
    pthread_mutex_init(&(toReturn->mutex), NULL);
    graph_add_vertex(g, toReturn);
    pthread_mutex_unlock(&mutex);
    return toReturn;
}

int circle_exist(void* input) {
    if (set_pro == NULL) {
        set_pro = shallow_set_create();
    }
    if (set_contains(set_pro, input)) {
        set_pro = NULL;
        return 1;
    } else {
        set_add(set_pro, input);
        vector* nei = graph_neighbors(g, input);
        size_t i = 0;
        for (; i< vector_size(nei); i++) {
            if (circle_exist(vector_get(nei, i))) {
                return 1;
            }
        }
        set_pro = NULL;
        return 0;
    }
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    pthread_mutex_lock(&mutex);
    int toReturn = 0;
    if (!graph_contains_vertex(g, thread_id)) {
        goto out;
    } else {
        if (graph_adjacent(g, drm, thread_id)) {
            toReturn = 1;
            graph_remove_edge(g, drm, thread_id);
            pthread_mutex_unlock(&drm->mutex);
        }
    }
    out:
    pthread_mutex_unlock(&mutex);
    return toReturn;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    pthread_mutex_lock(&mutex);
    graph_add_vertex(g, thread_id);
    if (!graph_adjacent(g, drm, thread_id)) {
        graph_add_edge(g, thread_id, drm);
        if (circle_exist(thread_id) == 0) {
             // if there is no deadlock
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&drm->mutex);
            pthread_mutex_lock(&mutex);
            graph_remove_edge(g, thread_id, drm);
            graph_add_edge(g, drm, thread_id);
            pthread_mutex_unlock(&mutex);
            return 1;
        } else {
            graph_remove_edge(g, thread_id, drm);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}

void drm_destroy(drm_t *drm) {
    /* Your code here */
    graph_remove_vertex(g, drm);
    pthread_mutex_destroy(&drm->mutex);
    free(drm);
    return;
}