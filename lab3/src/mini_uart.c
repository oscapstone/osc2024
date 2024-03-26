#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "utils.h"
#include "m_string.h"

void uart_send(char c) {
    
    while (1) {
        asm volatile("nop");
        if (get32(AUX_MU_LSR_REG) & 0x20)
            break;
    }
    put32(AUX_MU_IO_REG, c);
    if(c == '\n')
        uart_send('\r');
}

char uart_recv(void) {
    while (1) {
        asm volatile("nop");
        if (get32(AUX_MU_LSR_REG) & 0x01)
            break;
    }
    char ret = get32(AUX_MU_IO_REG) & 0xFF;
    return (ret=='\r')?'\n':ret;
}

void uart_send_string(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        uart_send((char)str[i]);
    }
}

void uart_send_hex(unsigned int *n){
    char c;
    int hex;
    uart_send_string("0x");
    for(int i=28; i>=0; i-=4){
        hex = (*n >> i) & 0xF;
        c = (hex > 9)? 'A' - 10 + hex: '0' + hex;
        uart_send(c);
    }
}

void uart_send_hex64(unsigned long long *n){
    char c;
    int hex;
    uart_send_string("0x");
    for(int i=60; i>=0; i-=4){
        hex = (*n >> i) & 0xF;
        c = (hex > 9)? 'A' - 10 + hex: '0' + hex;
        uart_send(c);
    }
}

void uart_init(void) {

    unsigned int selector;
    selector = get32(GPFSEL1);  // GPFSEL1 reg control alternative funct
    selector &= ~(7 << 12);     // clean gpio14 
    selector |= 2 << 12;        // set alt5 for gpio14
    selector &= ~(7 << 15);     // clean gpio15
    selector |= 2 << 15;        // set alt5 for gpio 15
    put32(GPFSEL1, selector);

    // remove both the pull-up/down state
    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    put32(GPPUDCLK0, 0);

    // Enable mini uart (this also enables access to its registers)
    put32(AUX_ENABLES, 1);

    // Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_CNTL_REG, 0);

    // Disable receive and transmit interrupts
    put32(AUX_MU_IER_REG, 0);
    
    // Enable 8 bit mode
    put32(AUX_MU_LCR_REG, 3);
    
    // Set RTS line to be always high
    put32(AUX_MU_MCR_REG, 0);

    // Set baud rate to 115200
    put32(AUX_MU_BAUD_REG, 270);

    // No FIFO
    put32(AUX_MU_IIR_REG, 6);

    // Finally, enable transmitter and receiver
    put32(AUX_MU_CNTL_REG, 3);

}

void uart_printf(char* fmt, ...){
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    char str[2048];
    m_vsprintf(str, fmt, args);
    char *s = str;
    while (*s) uart_send(*s++);
}