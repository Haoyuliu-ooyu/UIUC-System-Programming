/**
 * critical_concurrency
 * CS 241 - Spring 2021
 partner: xinshuo3, peiyuan3
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    struct queue* toReturn = malloc(sizeof(struct queue));
    if (toReturn == NULL) {
        return NULL;
    }
    toReturn->head = NULL;
    toReturn->tail = NULL;
    toReturn->size = 0;
    toReturn->max_size = max_size;
    pthread_cond_init((&toReturn->cv), NULL);
    pthread_mutex_init(&(toReturn->m), NULL);
    return toReturn;
}

void queue_destroy(queue *this) {
    if (this == NULL) {
        return;
    }
    /* Your code here */
    queue_node* start = this->head;
    while (start) {
        queue_node* temp = start;
        start = start->next;
        free(temp);
    }
    pthread_mutex_destroy(&(this->m));
    pthread_cond_destroy(&(this->cv));
    free(this);
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    while (this->max_size > 0 && this->size == this->max_size) {
        pthread_cond_wait(&this->cv, &this->m);
    }
    queue_node * toAdd = malloc(sizeof(queue_node));
    toAdd->data = data;
    toAdd->next = NULL;
    if (this->size == 0) {
        this->head = toAdd;
        this->tail = toAdd;
    } else {
        this->tail->next = toAdd;
        this->tail = toAdd;
    }
    this->size++;

    if (this->size > 0) {
        pthread_cond_broadcast(&this->cv);
    }
    pthread_mutex_unlock(&this->m);
}

void *queue_pull(queue *this) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    while (this->head == NULL) {
        pthread_cond_wait(&this->cv, &this->m);

    }
    queue_node * toRemove = this->head;
    void* toReturn = toRemove->data;
    if (this->head) {
        this->head = this->head->next;
    } else if (this->size == 0) {
        this->head = NULL;
        this->tail = NULL;
    }
    this->size--;
    if (this->size > 0 && this->size < this->max_size) {
        pthread_cond_broadcast(&(this->cv));
    }
    pthread_mutex_unlock(&(this->m));
    if (toRemove) {
        free(toRemove);
    }
    return toReturn;
}