#ifndef __FRAMEBUFFERFS_H__
#define __FRAMEBUFFERFS_H__

#include <stddef.h>
#include "vfs.h"

#define MAX_FILE_SIZE 4096

typedef struct framebufferfs_internal {
    struct vnode *parent;
    size_t child_count;
    char name[MAX_COMPONENT_LEN + 1];
    dir_entry_t dir_entries[DIR_ENTRIES];
    unsigned int mode;  // file type and permission
    size_t size;
    void *content;
} framebufferfs_internal_t;

struct framebuffer_info {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int isrgb;
};

struct filesystem *framebufferfs_create(void);


#endif