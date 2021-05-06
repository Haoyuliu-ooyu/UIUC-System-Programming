/**
 * finding_filesystems
 * CS 241 - Spring 2021
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

void* get_block(file_system* fs, inode* parent, uint64_t index);

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string, "Free blocks: %zd\n"
                            "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode* i = get_inode(fs, path);
    if (!i) {
        errno = ENOENT; 
        return -1;
    }
    uint16_t temp = i->mode >> RWX_BITS_NUMBER;
    i->mode = new_permissions | (temp << RWX_BITS_NUMBER);
    clock_gettime(CLOCK_REALTIME, &(i->ctim));
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode *i = get_inode(fs, path);
    //node dont  exit
    if (!i) {
        errno = ENOENT; 
        return -1;
    }
    //change user
    if (owner != ((uid_t)-1)){
        i->uid = owner;
    }
    //change group
    if (group != ((gid_t)-1)){
        i->gid = group;
    }
     //update meta time
    clock_gettime(CLOCK_REALTIME, &(i->ctim));
    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if (valid_filename(path) == 1 || get_inode(fs, path)) {
        return NULL;
    }
    const char* file_name = NULL;
    inode* parent = parent_directory(fs, path, &file_name);
    if (!parent || !is_directory(parent)) {
        return NULL;
    }
    if (first_unused_inode(fs) == -1) {
        return NULL;
    }
    inode* new_node = fs->inode_root + first_unused_inode(fs);
    init_inode(parent, new_node);
    minixfs_dirent d;
    d.name = (char*)file_name;
    d.inode_num = first_unused_inode(fs);
    int index = parent->size / sizeof(data_block);
    if (index >= NUM_DIRECT_BLOCKS) {
        return NULL;
    }
    int offset = parent->size % sizeof(data_block);
    if (!offset && add_data_block_to_inode(fs, parent) == -1) {
        return NULL;
    }
    void* start_block = get_block(fs, parent, index) + offset;
    memset(start_block, 0, sizeof(data_block));
    make_string_from_dirent(start_block, d);
    parent->size += MAX_DIR_NAME_LEN;
    return new_node;
}


ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
        ssize_t used = 0;
        char* map = GET_DATA_MAP(fs->meta);
        for(uint64_t i = 0; i < fs->meta->dblock_count; i++) {
            if (map[i] == 1) {
                used++;
            }
        }
        char* info_str = block_info_string(used);
        size_t len = strlen(info_str);
        if (*off > (off_t)len) {return 0;}
        if (count > len - *off) {count = len - *off;}
        memmove(buf, info_str + *off, count);
        *off += count;
        return count;
    } 
    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    inode* target = get_inode(fs, path);
    if (target == NULL) {
        target = minixfs_create_inode_for_path(fs, path);
        if (target == NULL) {
            errno = ENOSPC;
            return -1;
        }
    }
    uint64_t maxi = sizeof(data_block) * (NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS);
    //
    if (count + *off > maxi) {
        errno = ENOSPC;
        return -1;
    }
    //
    int block_count = (count + *off + sizeof(data_block) - 1)/sizeof(data_block);
    if (minixfs_min_blockcount(fs, path, block_count) == -1) {
        errno = ENOSPC;
        return -1;
    }
    //
    uint64_t index = *off / sizeof(data_block);
    size_t offset = *off % sizeof(data_block);
    uint64_t size = 0;
    if (count > sizeof(data_block) - offset) {
        size = sizeof(data_block) - offset;
    } else {
        size = count;
    }
    void* mem_block = get_block(fs, target, index) + offset;
    memcpy(mem_block, buf, size);
    uint64_t temp_count = size;
    *off += size;
    index++;
    while (temp_count < count) {
        if(count - temp_count > sizeof(data_block)){
            size = sizeof(data_block);
        }else{
            size = count - temp_count;
        }
        mem_block = get_block(fs, target, index);
        memcpy(mem_block, buf + temp_count, size);
        temp_count += size;
        *off += size;
        index ++;
    }
    if(count + *off > target->size){
        target->size  = count + *off;
    }
    //update atim and mtim
    clock_gettime(CLOCK_REALTIME, &target->atim);
    clock_gettime(CLOCK_REALTIME, &target->mtim);
    return temp_count;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    if (!buf) {
        errno = ENOENT;
        return -1;
    }
    inode* target = get_inode(fs, path);
    if (!target) {
        errno = ENOENT;
        return -1;
    }

    if ((uint64_t)*off > target->size) {
        return 0;
    }
    if (target->size - *off < count) {
        count = target->size - *off;
    }
    uint64_t index = *off / sizeof(data_block);
    size_t offset = *off % sizeof(data_block);
    uint64_t size = 0;
    if (count > sizeof(data_block) - offset) {
        size = sizeof(data_block) - offset;
    } else {
        size = count;
    }
    void* mem_block = get_block(fs, target, index) + offset;
    memcpy(buf, mem_block, size);
    uint64_t temp_count = size;
    *off += size;
    index++;
    while (temp_count < count) {
        if(count - temp_count > sizeof(data_block)){
            size = sizeof(data_block);
        }else{
            size = count - temp_count;
        }
        mem_block = get_block(fs, target, index);
        memcpy(buf + temp_count, mem_block, size);
        temp_count += size;
        *off += size;
        index ++;
    }
    //update atim and mtim
    clock_gettime(CLOCK_REALTIME, &target->atim);
    return temp_count;
}

void* get_block(file_system* fs, inode* parent, uint64_t index){
  data_block_number* target;
  if(index < NUM_DIRECT_BLOCKS){
    target = parent->direct;
  } else {
    target = (data_block_number*)(fs->data_root + parent->indirect);
    index -= NUM_DIRECT_BLOCKS;
  }
  return (void*) (fs->data_root + target[index]);
}
