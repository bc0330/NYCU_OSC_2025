#ifndef _ROOTFS_H
#define _ROOTFS_H

#define QEMU_ROOTFS  0x08000000
#define RASPI_ROOTFS 0x20000000

extern char *rootfs_addr;

struct cpio_newc_header {
    char    c_magic[6];        // "070701"
    char    c_ino[8];          // inode number
                               // The device and inode numbers from the disk.  
                               // These are used by programs that read cpio archives 
                               // to determine when two entries refer to the same file.
    char    c_mode[8];         // permission and file type
    char    c_uid[8];          // user ID of the owner
    char    c_gid[8];          // group ID of the owner
    char    c_nlink[8];        // number of hard links to the file
    char    c_mtime[8];        // modification time
    char    c_filesize[8];     // size of the file in bytes
    char    c_devmajor[8];     // major number of the device
    char    c_devminor[8];     // minor number of the device
    char    c_rdevmajor[8];    // major number of the device node
    char    c_rdevminor[8];    // minor number of the device node
    char    c_namesize[8];     // length of the filename, including the final null
    char    c_check[8];        // This field is always set	to zero	 by  writers  and  ignored  by readers
};

void ls();
void cat(char* filename);

#endif