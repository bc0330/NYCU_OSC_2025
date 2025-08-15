#ifndef __TMPFS_H__
#define __TMPFS_H__

#include <stddef.h>
#include "vfs.h"

#define MAX_FILE_SIZE 4096

typedef struct tmpfs_internal {
    struct vnode *parent;
    size_t child_count;
    char name[MAX_COMPONENT_LEN + 1];
    dir_entry_t dir_entries[DIR_ENTRIES];
    unsigned int mode;  // file type and permission
    size_t size;
    void *content;
} tmpfs_internal_t; 

struct filesystem *tmpfs_create(void);

#endif