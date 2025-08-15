#include <stddef.h>
#include "allocator.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "framebufferfs.h"
#include "utils.h"
#include "vfs.h"

static int write(struct file* file, const void* buf, size_t len);
static int read(struct file* file, void* buf, size_t len);
static int open(struct vnode* file_node, struct file** target);
static int close(struct file* file);
static long lseek64(struct file* file, long offset, int whence);
struct file_operations framebufferfs_fops = {
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
struct vnode_operations framebufferfs_vops = {
    .lookup = lookup,
    .create = create,
    .mkdir = mkdir,
};

static void framebufferfs_init(void) {

}

static int setup_mount(struct filesystem* fs, struct mount* mount) {
    mount->fs = fs;
    mount->root->mount = mount;
    mount->root->v_ops = &framebufferfs_vops;
    mount->root->f_ops = &framebufferfs_fops;
    mount->root->parent = NULL; // Set parent directory to NULL for root

    framebufferfs_internal_t *inter = allocate(sizeof(framebufferfs_internal_t));
    inter->child_count = 0;
    inter->size = 0;
    inter->mode = S_IFDIR; // Set appropriate mode for root directory
    inter->content = NULL;
    mount->root->internal = inter;
    for (int i = 0; i < DIR_ENTRIES; ++i) {
        inter->dir_entries[i].vnode = NULL;
        inter->dir_entries[i].name[0] = '\0';
    }
    framebufferfs_init(); // Initialize the framebuffer filesystem with stdin, stdout, stderr
    return 0;
}



static int write(struct file* file, const void* buf, size_t len) {
    // uart_send_string("[FRAMEBUFFERFS] write called\r\n");
    // char *actual_lfb = vtop
    for (int i = 0; i < len; ++i) {
        lfb[file->f_pos + i] = ((const char*)buf)[i]; // Write to the framebuffer
    }
    file->f_pos += len; // Update the file position
    return len; // Return the number of bytes written
}

static int read(struct file* file, void* buf, size_t len) {
   return -1;
}

static int open(struct vnode* file_node, struct file** target) {
    return 0; // Open successful
}

static int close(struct file* file) {
    free(file);  // Free the file handle
    return 0; // Close successful
}

static long lseek64(struct file* file, long offset, int whence) {
    // uart_send_string("[FRAMEBUFFERFS] lseek64 called\r\n");
    file->f_pos = (size_t)offset;
    return offset; // Seek successful
}

static int lookup(struct vnode* dir_node, struct vnode** target,
                  const char* component_name) {
    return -1; // Not found
}

static int create(struct vnode* dir_node, struct vnode** target,
                   const char* component_name) {
    return -1; 
}

static int mkdir(struct vnode* dir_node, struct vnode** target,
                  const char* component_name) {
    return -1;
}

struct filesystem* framebufferfs_create(void) {
    struct filesystem* fs = allocate(sizeof(struct filesystem));

    fs->name = "framebufferfs";
    fs->setup_mount = &setup_mount;
    return fs;
}