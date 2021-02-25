/**
 * mini_memcheck
 * CS 241 - Spring 2021
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

meta_data* head = NULL;
size_t total_memory_freed = 0;
size_t total_memory_requested = 0;
size_t invalid_addresses = 0;

int is_invalid_addr(void *ptr) {
    if (head == NULL)
        return 1;
    meta_data *curr = head;
    while(curr) {
        if ((void*)(curr + 1) == ptr)
            return 0;
        curr = curr->next;
    }
    return 1;
}

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here
    if (request_size == 0) {
        return NULL;
    }
    meta_data *temp = malloc(sizeof(meta_data) + request_size);
    if (temp == NULL) return NULL;
    temp->request_size = request_size;
    temp->filename = filename;
    temp->instruction = instruction;
    temp->next = NULL;
    if (head == NULL) {
        head = temp;
    } else {
        meta_data* curr = head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = temp;
    }
    total_memory_requested += request_size;
    return (void*)(temp + 1);
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    size_t request_size = num_elements * element_size;
    void* temp = mini_malloc(request_size, filename, instruction);
    char* ptr = temp;
    for (size_t i = 0; i < request_size; i++) {
        *((char*)ptr + 1) = 0;
    }
    return temp;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here

    return NULL;
}

void mini_free(void *payload) {
    // your code here
    if (payload == NULL) {
        return;
    }
    if (is_invalid_addr(payload)) {
        invalid_addresses++;
        return;
    }

    meta_data* temp = ((meta_data*)payload) - 1;
    if (temp == head) {
        head = temp->next;
    } else {
        meta_data* prev = head;
        while (prev->next != temp) {
            prev = prev->next;
        }
        prev->next = temp->next;
    }
    total_memory_freed += temp->request_size;
    free(temp);
}
