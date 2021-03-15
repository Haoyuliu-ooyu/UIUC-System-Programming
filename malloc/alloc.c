/**
 * malloc
 * CS 241 - Spring 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>

int force_printf(const char *format,...) {
    static char buf[4096]; //fine because malloc is not multithreaded

    va_list args;
    va_start(args, format);
    vsnprintf(buf, 4096, format, args);
    buf[4095] = '\0'; // to be safe
    va_end(args);
    write(1, buf, strlen(buf));
    return 0;
}

//struct meta_data
typedef struct meta_data
{
    /* data */
    size_t size;
    int free;
    struct meta_data* next;
    struct meta_data* prev;
}meta_data;

#define META_SIZE sizeof(meta_data)

static meta_data* head = NULL;
static int flag = 0; //there is no freed block

//static meta_data* free_head = NULL;
//struct meta_data* tail = NULL;
//functions
meta_data* find_first_free_block(size_t);
meta_data* request_space(size_t size);
void merge(meta_data* ptr);
void split(meta_data* ptr, size_t split_size);



/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    void* ptr = malloc(num*size);
    if (!ptr) {
        return NULL;
    }
    return memset(ptr, 0, num*size);
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // implement malloc!
    //todo:allign size
    if (size <= 0) {
        return NULL;
    }
    meta_data* block;
    if (!head) {
        block = request_space(size);
        if (!block) {return NULL;}
    } else {
        if (!flag) {
            block = request_space(size);
            if (!block) {return NULL;}
        } else {
            block = find_first_free_block(size);
            if (!block) { // no free block
                block = request_space(size);
                if (!block) {return NULL;}
            } else { // found!
                //force_printf("reached122\n");
                if((block->size - size >= size) && (block->size - size >= META_SIZE)) {
                    //force_printf("reached123\n");
                    split(block, size);
                }
                block->free = 0;
            }
        }
    }
    return block+1;
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    flag = 1;
    if (!ptr) {
        return;
    }
    //todo:merge
    meta_data* block = (meta_data*)ptr - 1;
    if (block->free == 1) {
        return;
    }
    //force_printf("reached161\n");
    if(block->prev && block->prev->free == 1) {
        //force_printf("reached162\n");
        merge(block);
    }
    //force_printf("reached161\n");
    if(block->next && block->next->free == 1) {
        //force_printf("reached166\n");
        merge(block->next);
    }
    block->free = 1;
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    struct meta_data* block = (meta_data*)ptr - 1;
    if (block->size >= size) {
        //force_printf("reached230\n");
        if (block->size - size >= META_SIZE) {
            //force_printf("reached223\n");
            split(block,size);
            return block+1;
        }
    } else {
        if (block->prev && block->prev->free && block->size + block->prev->size +sizeof(malloc) >= size) {
            //force_printf("reached223\n");
            merge(block);
            split(block,size);
            return block+1;
        }
    }
    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    memcpy(new_ptr, ptr, block->size);
    free(ptr);
    return new_ptr;
}

meta_data* find_first_free_block(size_t size) {
    meta_data* curr = head;
    meta_data* ret = NULL;
    while (curr) {
        if (curr->free && curr->size >= size) {
            ret = curr;
            break;
        }
        curr = curr->next;
    }
    return ret;
}

meta_data* request_space(size_t size) {
    meta_data* block = sbrk(0);
    void* requested = sbrk(size + META_SIZE);
    if (requested == (void*)-1) {
        return NULL;
    }
    if (head) {
        head->prev = block;
    }
    block->size = size;
    block->next = head;
    block->prev = NULL;
    block->free = 0;
    head = block;
    
    return block;
}

void split(meta_data* temp, size_t size) {
    meta_data* new_space = (void*)temp + META_SIZE + size;
    new_space->size = temp->size - size - META_SIZE;
    new_space->free = 1;
    temp->size = size;
    new_space->next = temp;
    new_space->prev = temp->prev;
    if(temp->prev){
        temp->prev->next = new_space;
    }else{
        head = new_space;
    }
    temp->prev = new_space;
    if(new_space->prev && new_space->prev->free) merge(new_space);
}

void merge(meta_data* ptr) {
    ptr->size += ptr->prev->size + META_SIZE;
    ptr->prev = ptr->prev->prev;
    if (ptr->prev) {
      ptr->prev->next = ptr;
    }else {
      head = ptr;
    }
}
