#include "mailbox.h"
#include "mini_uart.h"
#include "utils.h"

volatile unsigned int  __attribute__((aligned(16))) mailbox[36];

int mailbox_call(void) {
    // Combine the message address (upper 28 bits) with channel number (lower 4 bits)
    // convert pointer (64-bit, addr length) to unsigned long first, then to unsigned int
    unsigned int message_addr = ((unsigned int) (unsigned long) &mailbox & ~0xF) | (MBOX_CH_PROP & 0xF); 
    
    /* wait until mailbox is not full*/

    while (get32(MBOX_STATUS) & MBOX_FULL) {
        asm volatile("nop");
    }
    /* write the address of our message with channel id to the mailbox */
    put32(MBOX_WRITE, message_addr);
    while (1) {
        /* check and wait for response */
        while (get32(MBOX_STATUS) & MBOX_EMPTY) {
            asm volatile("nop");
        }
        /* check if it corresponds to our message */
        if (message_addr == get32(MBOX_READ))
            /* check if out request has succeeded */
            return REQUEST_SUCCEEDED == mailbox[1];
        return 0;
    }
    return 0;
}

void get_board_revision(void) {
    mailbox[0] = 7 * 4;                // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION;   // tag identifier
    mailbox[3] = 4;                    // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;                    // value buffer
    // tags end
    mailbox[6] = END_TAG;

    if (mailbox_call()) { // message passing procedure call, you should implement it following the 6 steps provided above.

        uart_send_string("Board Revision: 0x"); // it should be 0xa020d3 for rpi3 b+
        uart_send_num(mailbox[5], "hex");
        uart_send('\n');
    } else {
        uart_send_string("Mailboxe ERROR!\n");
    }
}

void get_arm_mem(void) {
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = GET_ARM_MEM;       // tag identifier
    mailbox[3] = 8;                 // response length for arm memory
    mailbox[4] = TAG_REQUEST_CODE;
    mailbox[5] = 0;                 // base address
    mailbox[6] = 0;                 // size
    // tags end
    mailbox[7] = END_TAG;

    if (mailbox_call()) { // message passing procedure call, you should implement it following the 6 steps provided above.

        uart_send_string("ARM memory base address: "); 
        uart_send_num(mailbox[5], "hex");
        uart_send_string("\nARM memory size: "); 
        uart_send_num(mailbox[6], "hex");
        uart_send('\n');
    } else {
        uart_send_string("Mailbox ERROR!\n");
    }
}