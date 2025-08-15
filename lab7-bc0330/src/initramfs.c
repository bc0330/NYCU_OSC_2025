#include <stddef.h>
#include "allocator.h"
#include "initramfs.h"
#include "rootfs.h"
#include "mini_uart.h"
#include "utils.h"
#include "vfs.h"

/*
    Initramfs is a read-only filesystem that is loaded into memory at boot time.
*/

static int write(struct file* file, const void* buf, size_t len);
static int read(struct file* file, void* buf, size_t len);
static int open(struct vnode* file_node, struct file** target);
static int close(struct file* file);
static long lseek64(struct file* file, long offset, int whence);
struct file_operations initramfs_fops = {
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
struct vnode_operations initramfs_vops = {
    .lookup = lookup,
    .create = create,
    .mkdir = mkdir,
};

static int init_create(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    // A create method only used during initialization to add initramfs files into vfs
    if (lookup(dir_node, target, component_name) == 0) {
        uart_send_string("[ERROR | Create] File already exists\r\n");
        return -1; // File already exists
    }

    uart_send_string("[INFO | Create] Creating file: ");
    uart_send_string((char *)component_name);
    uart_send_string(" under directory: ");
    uart_send_string(((initramfs_internal_t*)dir_node->internal)->name);
    uart_send_string("\r\n");

    struct vnode* new_node = (struct vnode*)allocate(sizeof(struct vnode));
    new_node->mount = dir_node->mount;
    new_node->v_ops = &initramfs_vops;
    new_node->f_ops = &initramfs_fops;      
    new_node->parent = dir_node; // Set parent directory
    new_node->internal = allocate(sizeof(initramfs_internal_t));

    initramfs_internal_t* inter = (initramfs_internal_t*)new_node->internal;
    inter->mode = S_IFREG | O_RDONLY; // Regular file with read/write permissions
    inter->size = 0;
    inter->content = NULL;
    strcpy(inter->name, component_name);

    initramfs_internal_t* dir_inter = (initramfs_internal_t*)dir_node->internal;
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

static void init_fs() {
    // uart_send_string("[INFO | INIT] Initializing initramfs...\r\n");
    struct vnode *root_node = NULL;
    if (vfs_lookup("/initramfs", &root_node) != 0) {
        uart_send_string("[ERROR | INIT] Root directory not found\r\n");
        return; // Root directory not found
    }

    char *cur_addr = rootfs_addr;     // Address of the initramfs data
    struct cpio_newc_header *header = (struct cpio_newc_header *)rootfs_addr;
    // iterate through the initramfs data and add files to the vfs
    while (!strcmp((char *) (cur_addr + sizeof(struct cpio_newc_header)), "TRAILER!!!")) {
        int filesize = hex2dec(header->c_filesize, 8);
        int namesize = hex2dec(header->c_namesize, 8);

        // Check magic number
        for (int i = 0; i < 6; ++i) {
            if (header->c_magic[i] != "070701"[i]) {
                uart_send_string("[ERROR | INIT] Invalid cpio magic number\r\n");
                return;
            }
        }

        // print file name
        // uart_send_string("[INFO | INIT] Adding file: ");
        // uart_send_string((char *)(cur_addr + sizeof(struct cpio_newc_header)));
        // uart_send_string("\r\n");

        struct vnode *target_node = NULL;
        // Create a new vnode for the file
        if (init_create(root_node, &target_node, (char *)(cur_addr + sizeof(struct cpio_newc_header))) != 0) {
            uart_send_string("[ERROR | INIT] Failed to create file in vfs\r\n");
            return;
        }

        // Set the content of the file if it is not empty
        if (filesize > 0) {
            ((initramfs_internal_t *)target_node->internal)->size = filesize;
            ((initramfs_internal_t *)target_node->internal)->content = cur_addr + ((sizeof(struct cpio_newc_header) + namesize + 3) & ~3); // Align to 4 bytes
        }

        // Move to next header
        cur_addr += ((sizeof(struct cpio_newc_header) + namesize + 3) & ~3)
             + ((filesize + 3) & ~3);
        header = (struct cpio_newc_header *)cur_addr;
    }
}

static int setup_mount(struct filesystem* fs, struct mount* mount) {
    // uart_send_string("[INFO | INIT] Setting up initramfs mount...\r\n");
    mount->fs = fs;
    mount->root->mount = mount;
    mount->root->v_ops = &initramfs_vops;
    mount->root->f_ops = &initramfs_fops;
    mount->root->parent = NULL; // Set parent directory to NULL for root
    // uart_send_string("[INFO | INIT] Creating root vnode...\r\n");
    initramfs_internal_t *inter = allocate(sizeof(initramfs_internal_t));
    mount->root->internal = inter;
    strcpy(inter->name, "initramfs"); // Name of the root directory
    inter->mode = S_IFDIR; // Set appropriate mode for root directory
    inter->size = 0;
    inter->content = NULL;
    inter->child_count = 0;
    // uart_send_string("[INFO | INIT] Initializing directory entries...\r\n");
    for (int i = 0; i < DIR_ENTRIES; ++i) {
        inter->dir_entries[i].vnode = NULL;
        inter->dir_entries[i].name[0] = '\0';
    }

    init_fs();

    return 0;
}

static int write(struct file* file, const void* buf, size_t len) {
    return -1; // Initramfs is read-only, writing is not allowed
}

static int read(struct file* file, void* buf, size_t len) {
    struct vnode* node = file->vnode;
    initramfs_internal_t* inter = (initramfs_internal_t*)node->internal;
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
    free(file);
    return 0; // Close successful
}

static long lseek64(struct file* file, long offset, int whence) {
    return 0; // Seek successful
}

static int lookup(struct vnode* dir_node, struct vnode** target,
                  const char* component_name) {
    initramfs_internal_t* inter = (initramfs_internal_t*)dir_node->internal;
    for (int i = 0; i < DIR_ENTRIES; ++i) {
        if (strcmp(inter->dir_entries[i].name, component_name)) {
            *target = inter->dir_entries[i].vnode;
            return 0; // Found
        }
    }
    return -1; // Not found
}

static int create(struct vnode* dir_node, struct vnode** target,
                   const char* component_name) {
    return -1; // Initramfs is read-only, creating files is not allowed
}

static int mkdir(struct vnode* dir_node, struct vnode** target,
                  const char* component_name) {
    return -1; // Initramfs does not support creating directories
}

struct filesystem* initramfs_create(void) {
    struct filesystem* fs = allocate(sizeof(struct filesystem));
    fs->name = "initramfs";
    fs->setup_mount = &setup_mount;
    return fs;
}