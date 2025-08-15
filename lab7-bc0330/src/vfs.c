#include <stddef.h>
#include "allocator.h"
#include "devfs.h"
#include "framebufferfs.h"
#include "initramfs.h"
#include "mini_uart.h"
#include "thread.h"
#include "tmpfs.h"
#include "uartfs.h"
#include "utils.h"
#include "vfs.h"

struct mount* rootfs;
struct filesystem* fs_list[8] = {NULL};

void init_vfs(void) {
    // init rootfs
    
    struct filesystem* tmpfs = tmpfs_create();
    if (tmpfs == NULL) {
        uart_send_string("[ERROR] Failed to create root filesystem\r\n");
    }
    if (register_filesystem(tmpfs) != 0) {
        uart_send_string("[ERROR] Failed to register root filesystem\r\n");
    }
    
    rootfs = allocate(sizeof(struct mount));
    if (rootfs == NULL) {
        uart_send_string("[ERROR] Failed to allocate memory for rootfs\r\n");
    }
    rootfs->root = allocate(sizeof(struct vnode));
    tmpfs->setup_mount(tmpfs, rootfs);

    vfs_mkdir("/initramfs");
    
    struct vnode* initramfs_root = NULL;
    if (vfs_lookup("/initramfs", &initramfs_root) != 0) {
        uart_send_string("[ERROR] Failed to lookup initramfs\r\n");
    }
    if (register_filesystem(initramfs_create()) != 0) {
        uart_send_string("[ERROR] Failed to register initramfs\r\n");
    }
    struct mount* initramfs_mount = allocate(sizeof(struct mount));
    if (initramfs_mount == NULL) {
        uart_send_string("[ERROR] Failed to allocate memory for initramfs mount\r\n");
    }
    struct filesystem* initramfs = find_filesystem("initramfs");
    if (initramfs == NULL) {
        uart_send_string("[ERROR] Failed to find initramfs filesystem\r\n");
    }
    initramfs_mount->root = initramfs_root;
    initramfs->setup_mount(initramfs, initramfs_mount);
    // while(1) uart_send_string("[INFO] VFS initialized successfully\r\n");

    struct vnode *dev_root = NULL;
    vfs_mkdir("/dev");
    if (vfs_lookup("/dev", &dev_root) != 0) {
        uart_send_string("[ERROR] Failed to lookup /dev directory\r\n");
    }
    if (register_filesystem(devfs_create()) != 0) {
        uart_send_string("[ERROR] Failed to register devfs\r\n");
    }
    struct mount* dev_mount = allocate(sizeof(struct mount));
    if (dev_mount == NULL) {
        uart_send_string("[ERROR] Failed to allocate memory for devfs mount\r\n");
    }
    struct filesystem* devfs = find_filesystem("devfs");
    if (devfs == NULL) {
        uart_send_string("[ERROR] Failed to find devfs filesystem\r\n");
    }
    dev_mount->root = dev_root;
    devfs->setup_mount(devfs, dev_mount);

    struct vnode* uart_node = NULL;
    if (vfs_mkdir("/dev/uart") != 0) {
        uart_send_string("[ERROR] Failed to create /dev/uart directory\r\n");
    }
    if (vfs_lookup("/dev/uart", &uart_node) != 0) {
        uart_send_string("[ERROR] Failed to lookup /dev/uart device\r\n");
    }
    if (register_filesystem(uartfs_create()) != 0) {
        uart_send_string("[ERROR] Failed to register uartfs\r\n");
    }
    struct mount* uart_mount = allocate(sizeof(struct mount));
    if (uart_mount == NULL) {
        uart_send_string("[ERROR] Failed to allocate memory for uartfs mount\r\n");
    }
    struct filesystem* uartfs = find_filesystem("uartfs");
    if (uartfs == NULL) {
        uart_send_string("[ERROR] Failed to find uartfs filesystem\r\n");
    }
    uart_mount->root = uart_node;
    uartfs->setup_mount(uartfs, uart_mount);

    struct vnode* framebuffer_node = NULL;
    if (vfs_mkdir("/dev/framebuffer") != 0) {
        uart_send_string("[ERROR] Failed to create /dev/framebuffer directory\r\n");
    }
    if (vfs_lookup("/dev/framebuffer", &framebuffer_node) != 0) {
        uart_send_string("[ERROR] Failed to lookup /dev/framebuffer device\r\n");
    }
    if (register_filesystem(framebufferfs_create()) != 0) {
        uart_send_string("[ERROR] Failed to register framebufferfs\r\n");
    }
    struct mount* framebuffer_mount = allocate(sizeof(struct mount));
    if (framebuffer_mount == NULL) {
        uart_send_string("[ERROR] Failed to allocate memory for framebufferfs mount\r\n");
    }
    struct filesystem* framebufferfs = find_filesystem("framebufferfs");
    if (framebufferfs == NULL) {
        uart_send_string("[ERROR] Failed to find framebufferfs filesystem\r\n");
    }
    framebuffer_mount->root = framebuffer_node;
    framebufferfs->setup_mount(framebufferfs, framebuffer_mount);

}

int register_filesystem(struct filesystem* fs) {
    if (find_filesystem(fs->name) != NULL) {
        return -1; // Filesystem already registered
    }
    for (int i = 0; i < 8; ++i) {
        if (fs_list[i] == NULL) {
            fs_list[i] = fs;
            return 0; // Successfully registered
        }
    }
    return -1; // No space to register filesystem
}

struct filesystem* find_filesystem(const char* name) {
    for (int i = 0; i < 8; ++i) {
        if (fs_list[i] != NULL && strcmp(fs_list[i]->name, name)) {
            return fs_list[i];
        }
    }
    return NULL;
}

// --- file operations ---
int vfs_open(const char* pathname, int flags, struct file** target) {
    struct vnode* node = NULL;
    if (vfs_lookup(pathname, &node) != 0) {
        if (flags & O_CREAT) {
            char parent_path[MAX_PATH_LEN] = {0}, child_name[MAX_COMPONENT_LEN + 1] = {0};
            const char *last_slash = pathname + strlen(pathname) - 1;
            while (last_slash >= pathname && *last_slash != '/') {
                last_slash--; // Find the last slash
            }  
            strncpy(parent_path, pathname, last_slash - pathname);
            strncpy(child_name, last_slash + 1, strlen(last_slash + 1));
            struct vnode* parent_node = NULL;
            if (strcmp(parent_path, "")) {
                parent_node = rootfs->root; // Use root as parent if no path is given
            } else {
                if (vfs_lookup(parent_path, &parent_node) != 0) {
                    uart_send_string("[ERROR | OPEN] Parent directory not found\r\n");
                    return -1; // Parent directory not found
                }
                
            }
            if (parent_node->mount != NULL) {
                parent_node = parent_node->mount->root; // Use the root of the mount point if it exists
            }
            parent_node->v_ops->create(parent_node, &node, child_name);
        } else {
            uart_send_string("[ERROR | OPEN] File not found\r\n");
            return -1; // File not found
        }
    }
    *target = allocate(sizeof(struct file));
    (*target)->flags = flags;
    (*target)->f_pos = 0;
    (*target)->vnode = node;
    (*target)->f_ops = node->f_ops;
    return 0;
}

int vfs_close(struct file* file) {
    return file->f_ops->close(file);
}

int vfs_write(struct file* file, const void* buf, size_t len) {
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file* file, void* buf, size_t len) {
    return file->f_ops->read(file, buf, len);
}

// --- vnode operations ---
int vfs_mkdir(const char* pathname) {
    char parent_path[MAX_PATH_LEN] = {0}, child_name[MAX_COMPONENT_LEN + 1] = {0};
    struct vnode* parent_node = NULL, *child = NULL;
    const char *last_slash = pathname + strlen(pathname) - 1;
    while (last_slash >= pathname && *last_slash != '/') {
        last_slash--; // Find the last slash
    }
    strncpy(parent_path, pathname, last_slash - pathname);
    strncpy(child_name, last_slash + 1, strlen(last_slash + 1));

    vfs_lookup(parent_path, &parent_node);

    if (parent_node == NULL) {
        uart_send_string("Parent directory not found\r\n");
        return -1; // Parent directory not found
    }
    return parent_node->v_ops->mkdir(parent_node, &child, child_name);
}

int vfs_mount(const char* target, const char* filesystem) {
    // uart_send_string("File system name: ");
    // uart_send_string((char*)filesystem);
    // uart_send_string("\r\nMount point: ");
    // uart_send_string((char*)target);
    // uart_send_string("\r\n");
    struct filesystem* fs = find_filesystem(filesystem);
    if (fs == NULL) {
        uart_send_string("[ERROR | MOUNT] Filesystem not found\r\n");
        return -1; // Filesystem not found
    }

    struct vnode* node = NULL;
    if (vfs_lookup(target, &node) != 0) {
        uart_send_string("[ERROR | MOUNT] Mount point not found\r\n");
        return -1; // Mount point not found
    }


    struct mount* new_mount = allocate(sizeof(struct mount));
    if (new_mount == NULL) {
        uart_send_string("[ERROR | MOUNT] Failed to allocate memory for mount\r\n");
        return -1; // Memory allocation failed
    }
    node->mount = new_mount; // Associate the mount with the vnode

    new_mount->root = allocate(sizeof(struct vnode));
    fs->setup_mount(fs, new_mount);
    return 0; // Mount successful
}

int vfs_lookup(const char* pathname, struct vnode** result_vnode) {
    char component[MAX_COMPONENT_LEN + 1];
    struct vnode* current_vnode;
    struct vnode* next_vnode;
    const char *head;
    const char *tail;

    if (pathname == NULL || result_vnode == NULL) {
        uart_send_string("[ERROR] Null pathname or result_vnode pointer\r\n");
        return -1; // Invalid arguments
    }
    *result_vnode = NULL; // Initialize output parameter

    if (strcmp(pathname, "/") || strcmp(pathname, "")) {
        if (rootfs == NULL || rootfs->root == NULL) {
            uart_send_string("[ERROR] Root filesystem not initialized\r\n");
            return -1;
        }
        *result_vnode = rootfs->root;
        return 0; // Traversal successful (it's the root)
    }

    if (pathname[0] == '/') { // Absolute path
        if (rootfs == NULL || rootfs->root == NULL) {
            uart_send_string("Error: Root filesystem not initialized for absolute path\r\n");
            return -1;
        }
        current_vnode = rootfs->root;
        head = pathname; // Start parsing from the first '/'
    } else { // Relative path
        if (current_thread == NULL || current_thread->cwd == NULL) {
            uart_send_string("Error: Current working directory not set for relative path\r\n");
            return -1;
        }
        current_vnode = current_thread->cwd;
        head = pathname;
    }

    *result_vnode = current_vnode; // Initialize with the starting node

    // 4. Iterate through path components
    while (*head != '\0') {
        // Skip all leading slashes for the current component segment
        while (*head == '/') {
            head++;
        }

        // If we've reached the end of the string after skipping slashes (e.g., path was "/" or "/foo/")
        if (*head == '\0') {
            break;
        }

        // `tail` will find the end of the current component
        tail = head;
        while (*tail != '/' && *tail != '\0') {
            tail++;
        }

        // Extract component
        size_t len = tail - head;

        if (len > MAX_COMPONENT_LEN) {
            uart_send_string("Error: Path component too long\r\n");
            *result_vnode = NULL; // Or keep last valid node? Policy decision.
            return -1;
        }

        // Copy component to buffer (strncpy is safer but ensure null termination)
        // Using a loop for clarity and to ensure null termination within bounds
        for (size_t i = 0; i < len; ++i) {
            component[i] = head[i];
        }
        component[len] = '\0';

        // Process the component: ".", "..", or regular name
        if (strcmp(component, ".")) { // User's strcmp: component IS "."
            // Current directory, no change to current_vnode.
            // *result_vnode is already current_vnode.
        } else if (strcmp(component, "..")) { // User's strcmp: component IS ".."
            if (current_vnode == rootfs->root) {
                // ".." at the root directory stays at the root.
            } else if (current_vnode->parent != NULL) {
                current_vnode = current_vnode->parent;
            } else {
                uart_send_string("Warning: '..' on a non-root node with no parent.\r\n");\
            }
            *result_vnode = current_vnode;
        } else { // Regular component name
            if (current_vnode->v_ops == NULL || current_vnode->v_ops->lookup == NULL) {
                 uart_send_string("[ERROR] Filesystem operations not available or lookup not implemented\r\n");
                 *result_vnode = NULL;
                 return -1;
            }

            if (current_vnode->mount != NULL) {
                // If the current vnode is a mount point, we should use its root vnode
                // uart_send_string("Goint to mount root\r\n");
                current_vnode = current_vnode->mount->root;
            }

            if (current_vnode->v_ops->lookup(current_vnode, &next_vnode, component) != 0) {
                *result_vnode = NULL; // Lookup failed
                return -1;
            }
            current_vnode = next_vnode;
            *result_vnode = current_vnode;
        }

        // Move head to the position of the slash or null char that ended the component
        head = tail;
    }

    // If loop completes, *result_vnode should hold the final vnode.
    return 0; // Traversal successful
}

int vfs_chdir(const char* pathname) {
    struct vnode* node = NULL;
    if (vfs_lookup(pathname, &node) != 0) {
        uart_send_string("Directory not found\r\n");
        return -1; // Directory not found
    }
    current_thread->cwd = node;
    return 0; // Change directory successful
}
