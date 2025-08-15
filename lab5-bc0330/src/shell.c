#include "allocator.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "power_manager.h"
#include "rootfs.h"
#include "shell.h"
#include "utils.h"


void shell(void) {
    char buf[MAX_COMMAND_LENGTH];
    char argv[4][MAX_COMMAND_LENGTH];
    char c;
    while (1) {
        int arg_count = -1;
        memset(buf, 0, MAX_COMMAND_LENGTH);
        for (int i = 0; i < 4; i++) {
            memset(argv[i], 0, MAX_COMMAND_LENGTH);
        }
        uart_send_string("# ");
        while (1) {
            c = uart_recv();
            uart_send(c);
            if (c == '\n' || c == '\r') {
                uart_send_string("\r\n");
                break;
            } else if (c == ' ') {
                arg_count++;
                if (arg_count >= 4) {
                    uart_send_string("\r\nToo many arguments\r\n");
                    break;
                }
            } else if (arg_count < 0) {
                if (strlen(buf) < MAX_COMMAND_LENGTH - 2) {
                    strcat(buf, c, MAX_COMMAND_LENGTH);
                } else {
                    uart_send_string("\r\nCommand too long\r\n");
                    break;
                }
            } else {
                if (strlen(argv[arg_count]) < MAX_COMMAND_LENGTH - 2) {
                    strcat(argv[arg_count], c, MAX_COMMAND_LENGTH);
                } else {
                    uart_send_string("\r\nArgument too long\r\n");
                    break;
                }
            }
        }


        if (strlen(buf) < MAX_COMMAND_LENGTH - 1) {
            if (strcmp(buf, "help")) {
                uart_send_string("cat        :print file content\r\n");
                uart_send_string("help       :print this help menu\r\n");
                uart_send_string("hello      :print Hello, world!\r\n");
                uart_send_string("info       :print hardware information\r\n");
                uart_send_string("ls         :list files in rootfs\r\n");
                uart_send_string("memAlloc   :allocate memory\r\n");
                uart_send_string("pageAlloc  :allocate contiguous page memory, arg0: number of pages\r\n");
                uart_send_string("freePage   :arg0: address\r\n");
                uart_send_string("allocate   :allocate memory, arg0: size in bytes\r\n");
                uart_send_string("free       :free memory, arg0: address\r\n");
                uart_send_string("reserve    :reserve memory from arg0 to arg1\r\n");
                uart_send_string("reboot     :reboot the system\r\n");
            } else if (strcmp(buf, "cat")) {
                char filename[MAX_COMMAND_LENGTH];
                
                memset(filename, 0, MAX_COMMAND_LENGTH);
                uart_send_string("Enter filename: ");
                while (1) {
                    c = uart_recv();
                    uart_send(c);
                    if (c == '\n' || c == '\r') {
                        uart_send('\n');
                        break;
                    } else if (strlen(filename) < MAX_COMMAND_LENGTH - 2) {
                        strcat(filename, c, MAX_COMMAND_LENGTH);
                    } else {
                        uart_send_string("\r\nFilename too long\r\n");
                        break;
                    }
                }
                cat(filename);
            } else if (strcmp(buf, "ls")) {
                ls();
            } else if (strcmp(buf, "memAlloc")) {
                // char *ptr = simple_alloc(16);
                // if (ptr == nullptr) {
                //     uart_send_string("Memory allocation failed\n");
                // } else {
                //     uart_send_string("Memory allocated at ");
                //     uart_send_num((unsigned long) ptr, "hex");
                //     uart_send('\n');
                //     ptr = "memAlloc good!";
                //     uart_send_string(ptr);
                //     uart_send('\n');
                // }
            } else if (strcmp(buf, "hello")) {
                uart_send_string("Hello, world!\r\n");
            } else if (strcmp(buf, "info")) {
                get_board_revision();
                get_arm_mem();
            } else if (strcmp(buf, "pageAlloc")) {
                if (arg_count < 0) {
                    uart_send_string("Please provide the number of pages to allocate\n");
                    continue;
                } 
                int num_pages = atoi(argv[0]);
                char *ptr = page_alloc(num_pages);
                if (ptr == nullptr) {
                    uart_send_string("Memory allocation failed\n");
                } else {
                    uart_send_string("Memory allocated at ");
                    uart_send_num((unsigned long) ptr, "hex");
                    uart_send('\n');
                    ptr = "pageAlloc good!";
                    uart_send_string(ptr);
                    uart_send('\n');
                }
            } else if (strcmp(buf, "freePage")) {
                if (arg_count < 0) {
                    uart_send_string("Please provide the number of pages to free\n");
                    continue;
                } 
                unsigned long addr = ahex2int(argv[0]);
                free_page((void *) addr);
            } else if (strcmp(buf, "allocate")) {
                if (arg_count < 0) {
                    uart_send_string("Please provide the size to allocate\n");
                    continue;
                } 
                int size = atoi(argv[0]);
                char *ptr = allocate(size);
                if (ptr == nullptr) {
                    uart_send_string("Memory allocation failed\n");
                } else {
                    uart_send_string("Memory allocated at ");
                    uart_send_num((unsigned long) ptr, "hex");
                    uart_send('\n');
                    ptr = "allocate good!";
                    uart_send_string(ptr);
                    uart_send('\n');
                }
            } else if (strcmp(buf, "free")) {
                if (arg_count < 0) {
                    uart_send_string("Please provide the address to free\n");
                    continue;
                } 
                unsigned long addr = ahex2int(argv[0]);
                free((void *) addr);
            } else if (strcmp(buf, "reserve")) {
                if (arg_count < 1) {
                    uart_send_string("Please provide the start and end address to reserve\n");
                    continue;
                } 
                unsigned long start_addr = ahex2int(argv[0]);
                unsigned long end_addr = ahex2int(argv[1]);
                memory_reserve(start_addr, end_addr);
            } else if (strcmp(buf, "rootfs")) {
                uart_send_string("Rootfs information:\r\n");
                uart_send_string("Rootfs address: ");
                uart_send_num((unsigned long) rootfs_addr, "hex");
                uart_send_string("\r\n");
            } else if (strcmp(buf, "reboot")) {
                uart_send_string("Rebooting...\r\n");
                reset(1000);
            } else {
                uart_send_string("Command not found\r\n");
                uart_send_string(buf);
                uart_send_string("\r\n");
            }
        }
    }

}