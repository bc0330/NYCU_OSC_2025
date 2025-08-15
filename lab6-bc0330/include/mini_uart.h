#ifndef _MINI_UART_H
#define _MINI_UART_H

#include "base.h"

/*
There is a VideoCore/ARM MMU translating physical addresses to bus addresses. 
The MMU maps physical address 0x3f000000 to bus address 0x7e000000. 
In your code, you should use physical addresses instead of bus addresses. 
However, the reference uses bus addresses. You should translate them into physical one.
*/
#define AUX_ENABLES     (PBASE + 0x00215004)
#define AUX_MU_IO_REG   (PBASE + 0x00215040)
#define AUX_MU_IER_REG  (PBASE + 0x00215044)
#define AUX_MU_IIR_REG  (PBASE + 0x00215048)
#define AUX_MU_LCR_REG  (PBASE + 0x0021504C)
#define AUX_MU_MCR_REG  (PBASE + 0x00215050)
#define AUX_MU_LSR_REG  (PBASE + 0x00215054)
#define AUX_MU_MSR_REG  (PBASE + 0x00215058)
#define AUX_MU_SCRATCH  (PBASE + 0x0021505C)
#define AUX_MU_CNTL_REG (PBASE + 0x00215060)
#define AUX_MU_STAT_REG (PBASE + 0x00215064)
#define AUX_MU_BAUD_REG (PBASE + 0x00215068)

void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_num(unsigned long num, char *form);
void uart_send_string(char *str);
void reverse_string(char *str);

#endif