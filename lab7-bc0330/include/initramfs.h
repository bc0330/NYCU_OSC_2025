#ifndef __INITRAMFS_H__
#define __INITRAMFS_H__

#include <stddef.h>
#include "vfs.h"

typedef struct initramfs_internal {
    char name[MAX_COMPONENT_LEN + 1];
    int mode;
    size_t size;
    void *content;

    dir_entry_t dir_entries[DIR_ENTRIES]; // Fixed size for simplicity
    size_t child_count; // Number of entries in dir_entries
} initramfs_internal_t; 

struct filesystem *initramfs_create(void);

#endif