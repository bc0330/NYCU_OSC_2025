#include "mini_uart.h"
#include "rootfs.h"
#include "utils.h"

void ls(void) {
    char *cur_addr = rootfs_addr;
    struct cpio_newc_header *header = (struct cpio_newc_header *)rootfs_addr;
    while (!strcmp((char *) (cur_addr + sizeof(struct cpio_newc_header)), "TRAILER!!!")) {
        int filesize = hex2dec(header->c_filesize, 8);
        int namesize = hex2dec(header->c_namesize, 8);
        
        // header followed by file name; TRAILER!!! is the last file in the archive
        // Check magic number
        for (int i = 0; i < 6; ++i) {
            if (header->c_magic[i] != "070701"[i]) {
                uart_send_string("Error: Invalid cpio magic number\r\n");
                return;
            }
        }

        uart_send_string((char *) (cur_addr + sizeof(struct cpio_newc_header)));
        uart_send('\n');

        // Move to next header
        // The size of the file is rounded up to the nearest multiple of 4 bytes
        cur_addr += ((sizeof(struct cpio_newc_header) + namesize + 3) & ~3) \
            + ((filesize + 3) & ~3);
        header = (struct cpio_newc_header *)cur_addr;
    }
}

void cat(char *filename) {
    char *cur_addr = rootfs_addr;
    struct cpio_newc_header *header = (struct cpio_newc_header *)rootfs_addr;

    while (!strcmp((char *) (cur_addr + sizeof(struct cpio_newc_header)), "TRAILER!!!")) {
        int filesize = hex2dec(header->c_filesize, 8);
        int namesize = hex2dec(header->c_namesize, 8);

        // header followed by file name; TRAILER!!! is the last file in the archive
        // Check magic number
        for (int i = 0; i < 6; ++i) {
            if (header->c_magic[i] != "070701"[i]) {
                uart_send_string("Error: Invalid cpio magic number\r\n");
                return;
            }
        }

        if (strcmp((char *) (cur_addr + sizeof(struct cpio_newc_header)), filename)) {
            // File found
            // Move to the file content
            cur_addr += ((sizeof(struct cpio_newc_header) + namesize + 3) & ~3);
            if  (filesize == 0) {
                uart_send_string("Error: Catting Directory\n");
            } else {
                uart_send_string(cur_addr);
                uart_send('\n');
            }
            return;
        }

        // Move to next header
        // The size of the file is rounded up to the nearest multiple of 4 bytes
        cur_addr += ((sizeof(struct cpio_newc_header) + namesize + 3) & ~3) \
            + ((filesize + 3) & ~3);
        header = (struct cpio_newc_header *)cur_addr;
    }

    uart_send_string("Error: File not found\r\n");
}