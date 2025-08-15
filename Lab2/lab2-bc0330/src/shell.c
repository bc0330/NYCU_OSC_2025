#include "allocator.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "power_manager.h"
#include "rootfs.h"
#include "shell.h"
#include "utils.h"


void shell(void) {
    char buf[MAX_COMMAND_LENGTH];
    char c;
    while (1) {
        memset(buf, 0, MAX_COMMAND_LENGTH);
        uart_send_string("# ");
        while (1) {
            c = uart_recv();
            uart_send(c);
            if (c == '\n' || c == '\r') {
                uart_send('\n');
                break;
            } else if (strlen(buf) < MAX_COMMAND_LENGTH - 2) {
                strcat(buf, c, MAX_COMMAND_LENGTH);
            } else {
                uart_send_string("\nCommand too long\n");
                break;
            }
        }


        if (strlen(buf) < MAX_COMMAND_LENGTH - 1) {
            if (strcmp(buf, "help")) {
                uart_send_string("cat      :print file content\n");
                uart_send_string("help     :print this help menu\n");
                uart_send_string("hello    :print Hello, world!\n");
                uart_send_string("info     :print hardware information\n");
                uart_send_string("ls       :list files in rootfs\n");
                uart_send_string("memAlloc :allocate memory\n");
                uart_send_string("reboot   :reboot the system\n");
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
                        uart_send_string("\nFilename too long\n");
                        break;
                    }
                }
                cat(filename);
            } else if (strcmp(buf, "ls")) {
                ls();
            } else if (strcmp(buf, "memAlloc")) {
                char *ptr = simple_alloc(16);
                if (ptr == nullptr) {
                    uart_send_string("Memory allocation failed\n");
                } else {
                    uart_send_string("Memory allocated at ");
                    uart_send_num((unsigned long) ptr, "hex");
                    uart_send('\n');
                    ptr = "memAlloc good!";
                    uart_send_string(ptr);
                    uart_send('\n');
                }
            } else if (strcmp(buf, "hello")) {
                uart_send_string("Hello, world!\n");
            } else if (strcmp(buf, "info")) {
                get_board_revision();
                get_arm_mem();
            } else if (strcmp(buf, "reboot")) {
                uart_send_string("Rebooting...\n");
                reset(1000);
            } else {
                uart_send_string("Command not found\n");
                uart_send_string(buf);
                uart_send('\n');
            }
        }
    }

}