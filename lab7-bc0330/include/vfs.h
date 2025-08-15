#ifndef __VFS_H__
#define __VFS_H__

#include <stddef.h>

#define MAX_PATH_LEN 255
#define MAX_COMPONENT_LEN 15
#define DIR_ENTRIES 16

// --- Flags ---
// Access mode
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
// File Creation
#define O_CREAT  0x0040
// File TYPE
#define S_IFMT   0170000  // file type mask
#define S_IFREG  0100000  // regular file
#define S_IFDIR  0040000  // directory
#define S_IFCHR  0020000  // character device
#define S_IFBLK  0060000  // block device
#define S_IFIFO  0010000  // FIFO
#define S_IFLNK  0120000  // symbolic link
#define S_IFSOCK 0140000  // socket

struct vnode {
  struct mount* mount;
  struct vnode_operations* v_ops;
  struct file_operations* f_ops;
  struct vnode* parent;  // parent directory

  void *internal;  // internal data for the filesystem
  // struct vnode *child_head;
  // size_t child_count;
  // struct vnode *next_sibling;
  // struct vnode *parent;
  // char name[MAX_COMPONENT_LEN + 1];
  // unsigned int mode;  // file type and permission
  // void *content;
  // size_t size;  // size of the file
};

// file handle
struct file {
  struct vnode* vnode;
  size_t f_pos;  // RW position of this file handle
  struct file_operations* f_ops;
  int flags;
};

struct mount {
  struct vnode* root;
  struct filesystem* fs;
};

struct filesystem {
  const char* name;
  int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

typedef struct dir_entry {
    char name[MAX_COMPONENT_LEN + 1];
    struct vnode *vnode;
} dir_entry_t;


struct file_operations {
  int (*write)(struct file* file, const void* buf, size_t len);
  int (*read)(struct file* file, void* buf, size_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
  long (*lseek64)(struct file* file, long offset, int whence);
};

struct vnode_operations {
  int (*lookup)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*create)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*mkdir)(struct vnode* dir_node, struct vnode** target,
              const char* component_name);
};

extern struct mount* rootfs;
extern struct filesystem* fs_list[8];

void init_vfs(void);
int register_filesystem(struct filesystem* fs);
struct filesystem* find_filesystem(const char* name);

// file operations
int vfs_open(const char* pathname, int flags, struct file** target);
int vfs_close(struct file* file);
int vfs_write(struct file* file, const void* buf, size_t len);
int vfs_read(struct file* file, void* buf, size_t len);

// file system operations
int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, struct vnode** target);

int vfs_chdir(const char* pathname);

#endif