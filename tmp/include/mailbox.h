#ifndef _MAILBOX_H
#define _MAILBOX_H

#include "base.h"


/* Mailbox addresses */
#define MBOX_BASE    (PBASE + 0x0000B880)
#define MBOX_READ    MBOX_BASE
#define MBOX_STATUS  (MBOX_BASE + 0x00000018)
#define MBOX_WRITE   (MBOX_BASE + 0x00000020)

/* Flags */
#define MBOX_EMPTY    0x40000000
#define MBOX_FULL     0x80000000

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* Tags */
#define GET_BOARD_REVISION 0x00010002
#define GET_ARM_MEM        0x00010005
#define REQUEST_CODE       0x00000000
#define REQUEST_SUCCEEDED  0x80000000
#define REQUEST_FAILED     0x80000001
#define TAG_REQUEST_CODE   0x00000000
#define END_TAG            0x00000000


int mailbox_call(unsigned int channel, unsigned int *mailbox);
void get_board_revision(void);
void get_arm_mem(void);


#endif