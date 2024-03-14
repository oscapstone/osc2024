#include "peripherals/mini_uart.h"

#include "peripherals/gpio.h"
#include "utils.h"


void uart_init() {
    unsigned int selector;

    selector = get32(GPFSEL1); // GPIO Function Select 1: Used to set the function of GPIO pins 10-19
    selector &= ~(7 << 12);   // clean gpio14
    selector |= 2 << 12;      // set alt5 for gpio14
    selector &= ~(7 << 15);   // clean gpio15
    selector |= 2 << 15;      // set alt5 for gpio15
    put32(GPFSEL1, selector); // Mini UART Line Status: Used to check the status of data transmission and reception

    put32(GPPUD, 0); // disable pull-up/ pull-down
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);
    put32(GPPUDCLK0, 0);

    put32(AUX_ENABLES, 1);   // Enable mini uart (this also enables access to its registers)
    put32(AUX_MU_CNTL_REG, 0);   // Disable transmitter and receiver during configuration
    put32(AUX_MU_IER_REG, 0);      // Disable receive and transmit interrupts, since we don't need it currently
    put32(AUX_MU_LCR_REG, 3);      // Enable 8 bit mode, set the data size to 8 bit
    put32(AUX_MU_MCR_REG, 0);      // Don’t need auto flow control. RTS/CTS
    put32(AUX_MU_BAUD_REG, 270);   // Set baud rate to 115200
    //put32(AUX_MU_IIR_REG, 6);   // No FIFO
    put32(AUX_MU_CNTL_REG, 3);   // Finally, enable transmitter and receiver
}

void uart_putc(char c) {
    /* wait until we can send */
    while (1) {
        if (get32(AUX_MU_LSR_REG) & 0x20) //the 6th bit of the LSR indicates the Transmit Holding Register Empty (THRE) status. 
										//If this bit is 1, it means that the UART is ready to accept a new character for transmission
            break;
    }
    /* write the character to the buffer */
    put32(AUX_MU_IO_REG, c);

    /* If the character is a newline, also send a carriage return to ensure proper display on all terminals, in windows we need \r\n */
    if (c == '\n') {
        while (1) {
            if (get32(AUX_MU_LSR_REG) & 0x20) // Check again if the transmit holding register is empty
                break;
        }

        put32(AUX_MU_IO_REG, '\r'); // Send a carriage return
    }
}

char uart_getc() {
    char r;

    /* wait until something is in the buffer */
    while (1) {
        if (get32(AUX_MU_LSR_REG) & 0x01) // the 0th bit of the LSR indicates the Data Ready (DR) status. 
										//If this bit is 1, it means that there is data available to be read from the receiver buffer
            break;
    }

    /* read it and return */
    r = (char) (get32(AUX_MU_IO_REG) & 0xFF);

    /* convert carriage return to newline */
    return r == '\r' ? '\n' : r;
}

void uart_puts(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        uart_putc((char) str[i]);
    }
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F', 10 + 55(0x37) = 65 -> ascii A, 0 + 48 = 48 -> ascii 0
        n += n > 9 ? 0x37 : 0x30;
        uart_putc(n);
    }
}

void uart_flush() {
    while (get32(AUX_MU_LSR_REG) & 0x01) { // checks the 0th bit of the LSR, which indicates the Data Ready (DR) status. If this bit is 1, it means that there is data available to be read from the receiver buffer
        get32(AUX_MU_IO_REG); // reads from the I/O register buffer to clean it
    }
}