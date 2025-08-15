#include <stddef.h>
#include "allocator.h"
#include "mini_uart.h"
#include "tmpfs.h"
#include "utils.h"
#include "vfs.h"

static int write(struct file* file, const void* buf, size_t len);
static int read(struct file* file, void* buf, size_t len);
static int open(struct vnode* file_node, struct file** target);
static int close(struct file* file);
static long lseek64(struct file* file, long offset, int whence);
struct file_operations tmpfs_fops = {
    .write = write,
    .read = read,
    .open = open,
    .close = close,
    .lseek64 = lseek64,
};

static int lookup(struct vnode* dir_node, struct vnode** target,
                  const char* component_name);
static int create(struct vnode* dir_node, struct vnode** target,
                   const char* component_name);
static int mkdir(struct vnode* dir_node, struct vnode** target,
                  const char* component_name);
struct vnode_operations tmpfs_vops = {
    .lookup = lookup,
    .create = create,
    .mkdir = mkdir,
};

static int setup_mount(struct filesystem* fs, struct mount* mount) {
    mount->fs = fs;
    mount->root->mount = mount;
    mount->root->v_ops = &tmpfs_vops;
    mount->root->f_ops = &tmpfs_fops;
    mount->root->parent = NULL; // Set parent directory to NULL for root

    tmpfs_internal_t *inter = allocate(sizeof(tmpfs_internal_t));
    inter->child_count = 0;
    inter->size = 0;
    inter->mode = S_IFDIR; // Set appropriate mode for root directory
    inter->content = NULL;
    mount->root->internal = inter;
    for (int i = 0; i < DIR_ENTRIES; ++i) {
        inter->dir_entries[i].vnode = NULL;
        inter->dir_entries[i].name[0] = '\0';
    }
    return 0;
}

static int write(struct file* file, const void* buf, size_t len) {
    struct vnode* node = file->vnode;
    tmpfs_internal_t* inter = (tmpfs_internal_t*)node->internal;
    if ((inter->mode & S_IFMT) != S_IFREG) {                       // Check if it's a regular file
        uart_send_string("[ERROR | Write] Not a regular file\r\n");
        return -1; // Not a regular file
    }
    
    size_t to_write = (file->f_pos + len > MAX_FILE_SIZE)?
                     (MAX_FILE_SIZE - file->f_pos) : len;       // don't exceed max size
    memcpy((char*)inter->content + file->f_pos, buf, to_write); 
    file->f_pos += to_write;
    inter->size = file->f_pos; // Update the size of the file
    return (int)to_write;
}

static int read(struct file* file, void* buf, size_t len) {
    struct vnode* node = file->vnode;
    tmpfs_internal_t* inter = (tmpfs_internal_t*)node->internal;
    if ((inter->mode & S_IFMT) != S_IFREG) {                      // Check if it's a regular file
        uart_send_string("[ERROR | Read] Not a regular file\r\n");
        return -1; // Not a regular file
    }
    
    size_t to_read = (file->f_pos + len > inter->size)?
                     (inter->size - file->f_pos) : len;       // don't exceed max size
    memcpy(buf, (char*)inter->content + file->f_pos, to_read);
    file->f_pos += to_read;
    return (int)to_read;
}

static int open(struct vnode* file_node, struct file** target) {
    return 0; // Open successful
}

static int close(struct file* file) {
    free(file);  // Free the file handle
    return 0; // Close successful
}

static long lseek64(struct file* file, long offset, int whence) {
    return 0; // Seek successful
}

static int lookup(struct vnode* dir_node, struct vnode** target,
                  const char* component_name) {
    tmpfs_internal_t* inter = (tmpfs_internal_t*)dir_node->internal;
    for (int i = 0; i < DIR_ENTRIES; ++i) {
        if (strcmp(inter->dir_entries[i].name, component_name)) {
            *target = inter->dir_entries[i].vnode;
            return 0; // Found
        }
    }
    *target = NULL; // Not found
    return -1; // Not found
}

static int create(struct vnode* dir_node, struct vnode** target,
                   const char* component_name) {
    if (lookup(dir_node, target, component_name) == 0) {
        uart_send_string("[ERROR | Create] File already exists\r\n");
        return -1; // File already exists
    }

    struct vnode* new_node = allocate(sizeof(struct vnode));
    new_node->mount = NULL;
    new_node->v_ops = &tmpfs_vops;
    new_node->f_ops = &tmpfs_fops;      
    new_node->parent = dir_node;
    new_node->internal = allocate(sizeof(tmpfs_internal_t));

    tmpfs_internal_t* inter = (tmpfs_internal_t*)new_node->internal;
    inter->mode = S_IFREG | O_RDWR; // Regular file with read/write permissions
    inter->size = 0;
    inter->content = allocate(MAX_FILE_SIZE);
    memset(inter->content, 0, MAX_FILE_SIZE); // Initialize content to zero
    inter->child_count = 0;
    strcpy(inter->name, component_name);

    tmpfs_internal_t* dir_inter = (tmpfs_internal_t*)dir_node->internal;
    int i;
    for (i = 0; i < DIR_ENTRIES; ++i) { // add the new file to the directory
        if (dir_inter->dir_entries[i].vnode == NULL) {
            dir_inter->dir_entries[i].vnode = new_node;
            strcpy(dir_inter->dir_entries[i].name, component_name);
            dir_inter->child_count++;
            break;
        }
    }
    if (i == DIR_ENTRIES) {
        uart_send_string("[ERROR | Create] Directory entry full\r\n");
        free(new_node->internal);
        free(new_node);
        return -1; // Directory entry full
    }
    *target = new_node; // Return the new file node
    return 0; // Create successful
}

static int mkdir(struct vnode* dir_node, struct vnode** target,
                  const char* component_name) {
    if (lookup(dir_node, target, component_name) == 0) {
        uart_send_string("[ERROR | Mkdir] Directory already exists\r\n");
        return -1; // Directory already exists
    }


    struct vnode* new_node = allocate(sizeof(struct vnode));
    new_node->mount = NULL;
    new_node->v_ops = &tmpfs_vops;
    new_node->f_ops = &tmpfs_fops;      
    new_node->parent = dir_node;
    new_node->internal = allocate(sizeof(tmpfs_internal_t));

    tmpfs_internal_t* inter = (tmpfs_internal_t*)new_node->internal;
    inter->mode = S_IFDIR; // Directory with read/write permissions
    inter->size = 0;
    inter->content = NULL;
    inter->child_count = 0;
    for (int i = 0; i < DIR_ENTRIES; ++i) {
        inter->dir_entries[i].vnode = NULL;
        inter->dir_entries[i].name[0] = '\0';
    }
    strcpy(inter->name, component_name);

    tmpfs_internal_t* dir_inter = (tmpfs_internal_t*)dir_node->internal;
    int i;
    for (i = 0; i < DIR_ENTRIES; ++i) { // add the new directory to the parent directory
        if (dir_inter->dir_entries[i].vnode == NULL) {
            dir_inter->dir_entries[i].vnode = new_node;
            strcpy(dir_inter->dir_entries[i].name, component_name);
            dir_inter->child_count++;
            break;
        }
    }
    if (i == DIR_ENTRIES) {
        uart_send_string("[ERROR | Mkdir] Directory entry full\r\n");
        free(new_node->internal);
        free(new_node);
        return -1; // Directory entry full
    }
    *target = new_node; // Return the new directory node
    return 0; // Create successful
}

struct filesystem* tmpfs_create(void) {
    struct filesystem* fs = allocate(sizeof(struct filesystem));

    fs->name = "tmpfs";
    fs->setup_mount = &setup_mount;
    return fs;
}