#include "gpio.h"
#include "uart.h"
#include "types.h"
#include "exception.h"

/* Auxilary mini UART registers */
#define AUXENB              ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO_REG       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR_REG      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH_REG  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD         ((volatile unsigned int*)(MMIO_BASE+0x00215068))

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;

    /* initialize UART1 */
    *AUXENB |=1;            // enable UART1, AUX mini uart 
    *AUX_MU_CNTL_REG = 0;   // Disable transmitter and receiver (Tx, Rx)
    *AUX_MU_IER_REG = 0;    // Disable interrupt
    *AUX_MU_LCR_REG = 3;    // data size to 8 bit
    *AUX_MU_MCR_REG = 0;    // Don’t need auto flow control.
    *AUX_MU_IIR_REG = 0xc6; // disable interrupts and clear FIFO
    *AUX_MU_BAUD = 270;     // 115200 baud rate
    /* map UART1 to GPIO pins */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15));  // gpio14, gpio15 init to zero
    r|=(2<<12)|(2<<15);     // set alt5 (UART1's Tx, Rx)
    *GPFSEL1 = r;           // alternate function
    *GPPUD = 0;             // enable gpio14, gpio15
    r=150; while(r--) { asm volatile("nop"); }  // wait 150 cycles
    *GPPUDCLK0 = (1<<14)|(1<<15);   // gpio14, gpio15 is gppud
    r=150; while(r--) { asm volatile("nop"); }  // wait 150 cycles
    *GPPUDCLK0 = 0;         // flush GPIO setup avoid other GPIO interference
    *AUX_MU_CNTL_REG = 3;   // enable transmitter and receiver (Tx, Rx)
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    // check Line Status Register (LSR) Transmit Holding Register Empty 5 
    while (!(*AUX_MU_LSR_REG & 0x20)) {
        asm volatile("nop");
    }
    // 
    *AUX_MU_IO_REG = c;
}

/**
 * Receive a character
 */
char uart_getc() {
    char r;
    // check Line Status Register (LSR) Rx Buffer Data Ready 0
    while (!(*AUX_MU_LSR_REG & 0x01)) {
        asm volatile("nop");
    }
    /* read it and return */
    r=(char)(*AUX_MU_IO_REG);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}

/**
 * Display a string
 */
void uart_puts(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // Shift 'd' right by 'c' bits to align the current nibble at the right-most position and mask out the rest
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'a'-'f'
        n+=n>9?0x57:0x30;
        uart_send(n);
    }
}

#define BUFSIZE 256
char read_buf[BUFSIZE];
char write_buf[BUFSIZE];
uint32_t read_head = 0, read_end = 0;
uint32_t write_head = 0, write_end = 0;

void enable_uart_interrupt() {
    enable_read_interrupt();
    *ENABLE_IRQS_1 = (1 << 29); //interrupt controller’s Enable IRQs1(0x3f00b210)’s bit29.
                                //BCM2837 SPEC P112 table
                                //0x210 enable IRQs1
}

void disable_uart_interrupt() {
    disable_write_interrupt();
    *DISABLE_IRQS_1 = (1 << 29);
                                //BCM2837 SPEC P112 table
                                //0x21C disable IRQs1
}

//AUX_MU_IER_REG  bit 0 => set for read  RX
//                bit 1 => set for write  TX

void enable_read_interrupt() {
    *AUX_MU_IER_REG |= 0x01;
}

void disable_read_interrupt() {
    *AUX_MU_IER_REG &= ~0x01;
}

void enable_write_interrupt() {
    *AUX_MU_IER_REG |= 0x02;
}

void disable_write_interrupt() {
    *AUX_MU_IER_REG &= ~0x02;
}

void async_uart_handler() {
    disable_uart_interrupt();
    //The AUX_MU_IIR_REG register shows the interrupt status. 
    if (*AUX_MU_IIR_REG & 0x04) { // 100 //read mode   Receiver holds valid byte 
        char c = *AUX_MU_IO_REG&0xFF;
        read_buf[read_end++] = c;
        if(read_end==BUFSIZE) read_end=0;
    } else if (*AUX_MU_IIR_REG & 0x02) { //010  //send mode // check write enabled // Transmit holding register empty
        while (*AUX_MU_LSR_REG & 0x20) { //0010 0000 //Both bits [7:6] always read as 1 as the FIFOs are always enabled 
            if (write_head == write_end) {             
                    enable_read_interrupt(); 
                    break;
                }
            char c = write_buf[write_head];
            write_head++;
            *AUX_MU_IO_REG = c;
            if(write_head==BUFSIZE)write_head=0;
        }
    }
    enable_uart_interrupt();
}

uint32_t async_uart_gets(char *input, uint32_t size) {
    int len=0;
    for (len = 0; len < size - 1; len++) {
        while (read_head == read_end) asm volatile("nop");
        

        if (read_buf[read_head] == '\r' || read_buf[read_head] == '\n') {
            read_head++;
            if(read_head >= BUFSIZE -1 ) read_head -= BUFSIZE;
            break;
        }

        input[len] = read_buf[read_head];
        read_head++;
        if(read_head==BUFSIZE)read_head=0;
    }

    input[len] = '\0';

    return len;
}

void async_uart_puts(const char *s) {
    while (*s != '\0') {
        write_buf[write_end++] = *s++;
        if(write_end==BUFSIZE)write_end=0;
    }
    enable_write_interrupt();
}

void delay(int time) {
    while (time--);
}
