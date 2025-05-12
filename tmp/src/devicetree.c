#include "devicetree.h"
#include "mini_uart.h"
#include "rootfs.h"
#include "utils.h"

void *__dtb_end = NULL;
char *rootfs_addr = NULL;
void *rootfs_end = NULL;

void fdt_traverse(void (*callback)(char *)) {
    struct fdt_header *fdt = (struct fdt_header *) __dtb_addr;
    
    if (be2le(fdt->magic) != 0xd00dfeed) {
        uart_send_string("Invalid device tree magic number\n");
        uart_send_num(be2le(fdt->magic), "hex");
        uart_send_string("\r\n");
        return;
    }

    uint32_t struct_size = be2le(fdt->size_dt_struct);
    char *struct_addr = (char *) fdt + be2le(fdt->off_dt_struct);
    uint32_t struct_off = 0;
    char *strings_addr = (char *) fdt + be2le(fdt->off_dt_strings);

    while (struct_off < struct_size) {
        uint32_t token = be2le(*(uint32_t *) (struct_addr + struct_off));
        struct_off += 4;

        if (token == FDT_BEGIN_NODE) {
            // uart_send_string("Node: FDT_BEGIN_NODE\n");
            // followed by node's name
            char *node_name = struct_addr + struct_off;
            struct_off += (strlen(node_name) + 1 + 3) & ~3;  // aligned to 4 bytes

        } else if (token == FDT_END_NODE) {
            // uart_send_string("Node: FDT_END_NODE\n");
            // do nothing

        } else if (token == FDT_PROP) {
            // uart_send_string("Node: FDT_PROP\n");
            uint32_t len = be2le(*(uint32_t *) (struct_addr + struct_off));
            struct_off += 4;
            uint32_t nameoff = be2le(*(uint32_t *) (struct_addr + struct_off));
            struct_off += 4;

            char *prop_name = strings_addr + nameoff;
            if (len > 0) {
                if (strcmp(prop_name, "linux,initrd-start") == 1) {
                    callback((char *)(struct_addr + struct_off));
                }
                struct_off += (len + 3) & ~3;  // proplength is aligned to 4 bytes
            }

        } else if (token == FDT_NOP) {
            // uart_send_string("Node: FDT_NOP\n");
            // do nothing

        } else if (token == FDT_END) {
            // uart_send_string("Node: FDT_END\n");
        } else {
            uart_send_string("Unknown token: ");
            uart_send_num(token, "hex");
            uart_send_string("\r\n");
            break;
        }
    }
}

void initramfs_callback(char *prop_addr) {
    rootfs_addr = (char *) be2le(*(uint64_t *) prop_addr);
    uart_send_string("Found initramfs at ");
    uart_send_num((uint64_t) rootfs_addr, "hex");
    uart_send('\n');

}