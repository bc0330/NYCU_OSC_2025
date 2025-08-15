#include "mailbox.h"
#include "mini_uart.h"
#include "utils.h"

unsigned int  __attribute__((aligned(16))) mailbox[36];
unsigned int width = 0;
unsigned int height = 0;
unsigned int pitch = 0;
unsigned int isrgb = 0;
unsigned char *lfb = NULL;

int mailbox_call(unsigned int channel, unsigned int *_mailbox) {
    // Combine the message address (upper 28 bits) with channel number (lower 4 bits)
    // convert pointer (64-bit, addr length) to unsigned long first, then to unsigned int
    unsigned int message_addr = ((unsigned int) (unsigned long) _mailbox & ~0xF) | (channel & 0xF); 
    
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
            return REQUEST_SUCCEEDED == _mailbox[1];
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

    if (mailbox_call(8, mailbox)) { // message passing procedure call, you should implement it following the 6 steps provided above.

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

    if (mailbox_call(8, mailbox)) { // message passing procedure call, you should implement it following the 6 steps provided above.

        uart_send_string("ARM memory base address: "); 
        uart_send_num(mailbox[5], "hex");
        uart_send_string("\nARM memory size: "); 
        uart_send_num(mailbox[6], "hex");
        uart_send('\n');
    } else {
        uart_send_string("Mailbox ERROR!\n");
    }
}

void framebuffer_init(void) {
    mailbox[0] = 35 * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    // tags begin
    mailbox[2] = 0x48003; // set phy wh
    mailbox[3] = 8;
    mailbox[4] = 8;
    mailbox[5] = 1024; // FrameBufferInfo.width
    mailbox[6] = 768;  // FrameBufferInfo.height

    mailbox[7] = 0x48004; // set virt wh
    mailbox[8] = 8;
    mailbox[9] = 8;
    mailbox[10] = 1024; // FrameBufferInfo.virtual_width
    mailbox[11] = 768;  // FrameBufferInfo.virtual_height

    mailbox[12] = 0x48009; // set virt offset
    mailbox[13] = 8;
    mailbox[14] = 8;
    mailbox[15] = 0; // FrameBufferInfo.x_offset
    mailbox[16] = 0; // FrameBufferInfo.y.offset

    mailbox[17] = 0x48005; // set depth
    mailbox[18] = 4;
    mailbox[19] = 4;
    mailbox[20] = 32; // FrameBufferInfo.depth

    mailbox[21] = 0x48006; // set pixel order
    mailbox[22] = 4;
    mailbox[23] = 4;
    mailbox[24] = 1; // RGB, not BGR preferably

    mailbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mailbox[26] = 8;
    mailbox[27] = 8;
    mailbox[28] = 4096; // FrameBufferInfo.pointer
    mailbox[29] = 0;    // FrameBufferInfo.size

    mailbox[30] = 0x40008; // get pitch
    mailbox[31] = 4;
    mailbox[32] = 4;
    mailbox[33] = 0; // FrameBufferInfo.pitch

    mailbox[34] = END_TAG;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    uart_send_string("Mailbox address: 0x");
    uart_send_num((unsigned long)mailbox, "hex");
    uart_send_string("\r\n");
    if (mailbox_call(MBOX_CH_PROP, mailbox) && mailbox[20] == 32 && mailbox[28] != 0) {
        mailbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = mailbox[5];        // get actual physical width
        height = mailbox[6];       // get actual physical height
        pitch = mailbox[33];       // get number of bytes per line
        isrgb = mailbox[24];       // get the actual channel order
        lfb = (void *)((unsigned long)mailbox[28]);
    } else {
        uart_send_string("Unable to set screen resolution to 1024x768x32\n");
    }
}