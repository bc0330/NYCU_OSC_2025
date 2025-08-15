#include <stddef.h>
#include "allocator.h"
#include "mini_uart.h"
#include "uartfs.h"
#include "utils.h"
#include "vfs.h"

static int write(struct file* file, const void* buf, size_t len);
static int read(struct file* file, void* buf, size_t len);
static int open(struct vnode* file_node, struct file** target);
static int close(struct file* file);
static long lseek64(struct file* file, long offset, int whence);
struct file_operations uartfs_fops = {
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
struct vnode_operations uartfs_vops = {
    .lookup = lookup,
    .create = create,
    .mkdir = mkdir,
};

static void uartfs_init(void) {
    struct vnode *root_node = NULL;
    if (vfs_lookup("/dev/uart", &root_node) != 0) {
        uart_send_string("[ERROR | INIT] Root directory not found\r\n");
        return; // Root directory not found
    }
    struct vnode *stdin, *stdout, *stderr;
    create(root_node, &stdin, "stdin");   // Create stdin file
    create(root_node, &stdout, "stdout"); // Create stdout file
    create(root_node, &stderr, "stderr"); // Create stderr file
}

static int setup_mount(struct filesystem* fs, struct mount* mount) {
    mount->fs = fs;
    mount->root->mount = mount;
    mount->root->v_ops = &uartfs_vops;
    mount->root->f_ops = &uartfs_fops;
    mount->root->parent = NULL; // Set parent directory to NULL for root

    uartfs_internal_t *inter = allocate(sizeof(uartfs_internal_t));
    inter->child_count = 0;
    inter->size = 0;
    inter->mode = S_IFDIR; // Set appropriate mode for root directory
    inter->content = NULL;
    mount->root->internal = inter;
    for (int i = 0; i < DIR_ENTRIES; ++i) {
        inter->dir_entries[i].vnode = NULL;
        inter->dir_entries[i].name[0] = '\0';
    }
    uartfs_init(); // Initialize the UART filesystem with stdin, stdout, stderr
    return 0;
}



static int write(struct file* file, const void* buf, size_t len) {
    if (file->flags & O_RDONLY) {
        uart_send_string("[ERROR | Write] File is opened in read-only mode\r\n");
        return -1; // Cannot write to a read-only file
    }
    for (int i = 0; i < len; ++i) {
        uart_send(((char*)buf)[i]); // Send each character to UART
    }
    return len; // Return the number of bytes written
}

static int read(struct file* file, void* buf, size_t len) {
    if (file->flags & O_WRONLY) {
        uart_send_string("[ERROR | Read] File is opened in write-only mode\r\n");
        return -1; // Cannot read from a write-only file
    }
    for (int i = 0; i < len; ++i) {
        ((char*)buf)[i] = uart_recv(); // Fill the buffer with received characters
    }
    return len; // Return the number of bytes read
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
    uartfs_internal_t* inter = (uartfs_internal_t*)dir_node->internal;
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
    new_node->v_ops = &uartfs_vops;
    new_node->f_ops = &uartfs_fops;
    new_node->parent = dir_node;
    new_node->internal = allocate(sizeof(uartfs_internal_t));

    uartfs_internal_t* inter = (uartfs_internal_t*)new_node->internal;
    inter->mode = S_IFREG | O_RDWR; // Regular file with read/write permissions
    inter->size = 0;
    // inter->content = allocate(MAX_FILE_SIZE);
    // memset(inter->content, 0, MAX_FILE_SIZE); // Initialize content to zero
    inter->child_count = 0;
    strcpy(inter->name, component_name);

    uartfs_internal_t* dir_inter = (uartfs_internal_t*)dir_node->internal;
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
    return -1;
}

struct filesystem* uartfs_create(void) {
    struct filesystem* fs = allocate(sizeof(struct filesystem));

    fs->name = "uartfs";
    fs->setup_mount = &setup_mount;
    return fs;
}