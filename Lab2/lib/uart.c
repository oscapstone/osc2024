#include "uart.h"


/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504c))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505c))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

void uart_init(){

        /* initialize UART */
    *AUX_ENABLE |= 1;           // Enable mini UART
    *AUX_MU_CNTL = 0;       // Disable TX, RX during configuration (Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver)
    *AUX_MU_IER = 0;        // Disable interrupt
    *AUX_MU_LCR = 3;        // Set the data size to 8 bit
    *AUX_MU_MCR = 0;        // Don't need auto flow control
    *AUX_MU_BAUD = 270;     // Set baud rate to 115200
    *AUX_MU_IIR = 6;        // No FIFO
    
    /* map UART1 to GPIO pins */
    unsigned int r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15 innitial
    r |= (2 << 12) | (2 << 15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15
    r = 150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup

    *AUX_MU_CNTL = 3;

    while ((*AUX_MU_LSR & 0x01)){
        *AUX_MU_IO;
    }
}

uint8_t uart_read(){

    /* Check bit 0 for data ready field */
    while (!(*AUX_MU_LSR & 0x01)){
        asm volatile("nop");
    }
    
    uint8_t* buf_ptr = (uint8_t*)AUX_MU_IO;

    /* convert carrige return to newline */
    return *buf_ptr == '\r' ? '\n' : *buf_ptr;
}

uint8_t uart_read_bin(){

    while (!(*AUX_MU_LSR & 0x01)){
        asm volatile("nop");
    }

    uint8_t* buf_ptr = (uint8_t*)AUX_MU_IO;

    return *buf_ptr;
}

void uart_write(char ch){

    /* Check bit 5 for Transmitter empty field */
    while (!(*AUX_MU_LSR & 0x20)){
        asm volatile("nop");
    }

    char* buf_ptr = (char*)AUX_MU_IO;
    *buf_ptr = ch;

}