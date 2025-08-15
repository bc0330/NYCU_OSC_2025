#include "mini_uart.h"
#include "utils.h"
#include "gpio.h"
#include "base.h"

void uart_init(void) {
    unsigned int func_sel;
    func_sel = get32(GPFSEL1);
    func_sel &= ~(7 << 12);      // clean gpio 14
    func_sel |= 2 << 12;         // set alt5 for gpio4
    func_sel &= ~(7 << 15);      // clean gpio 15
    func_sel |= 2 << 15;         // set alt5 for gpio5

    /* Disable pull-up/down */
    put32(GPPUD, 0);                          // control signal no pull-up/down
    delay(150);
    put32(GPPUDCLK0, (1 << 14)|(1 << 15));    // clock signal to set ctrl signal for pin 14 and 15
    delay(150);
    put32(GPPUDCLK0, 0);                      // stop the clock

    put32(AUX_ENABLES, 1);                   // Enable mini uart (this also enables access to its registers)
    put32(AUX_MU_CNTL_REG, 0);               // Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_IER_REG, 0);                // Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG, 3);                // Enable 8 bit mode
    put32(AUX_MU_MCR_REG, 0);                // Set RTS line to be always high
    put32(AUX_MU_BAUD_REG, 270);             // Set baud rate to 115200
    put32(AUX_MU_CNTL_REG, 3);               // Finally, enable transmitter and receiver
}

void uart_send(char c) {
    while(1) {
        if(get32(AUX_MU_LSR_REG) & 0x20)   // transmitter ready
            break;
    }
    put32(AUX_MU_IO_REG, c);
}

char uart_recv(void) {
    while(1) {
        if(get32(AUX_MU_LSR_REG) & 0x01)   // data ready
            break;
    }
    return(get32(AUX_MU_IO_REG) & 0xFF);   // char -> 8-bit
}

unsigned int uart_recv_int(void) {
    // read little endian 4 bytes
    unsigned int data = 0;
    for (int i = 0; i < 4; ++i) {
        data |= uart_recv() << (i * 8);
    }
    return data;
}

void uart_send_string(char *str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        uart_send(str[i]);
    }
}


